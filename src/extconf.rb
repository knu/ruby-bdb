#!/usr/bin/ruby
ARGV.collect! {|x| x.sub(/^--with-db-prefix=/, "--with-db-dir=") }

require 'mkmf'

if ARGV.include?('--help') || ARGV.include?('-h')
   puts <<EOT

 ------------------------------------------------------------
 Options

  --with-db-dir=<prefix for library and include of Berkeley DB>
      default=/usr

  --with-db-include=<include file directory for Berkeley DB>
      default=/usr/include

  --with-db-lib=<library directory for Berkeley DB>
      default=/usr/lib

  --with-db-version=<list of comma separated suffix to add to libdb>
      default=-4.2,42,-4.1,41,-4.0,-4,40,4,3,2,

  --with-db-uniquename=<unique name associated with db_version>
      option --with-uniquename=NAME when Berkeley DB was build
  
  --disable-thread
     disable the use of DB_THREAD

------------------------------------------------------------

EOT
   exit
end

if unknown = enable_config("unknown")
   libs = if CONFIG.key?("LIBRUBYARG_STATIC")
	     Config::expand(CONFIG["LIBRUBYARG_STATIC"].dup).sub(/^-l/, '')
	  else
	     Config::expand(CONFIG["LIBRUBYARG"].dup).sub(/lib([^.]*).*/, '\\1')
	  end
   unknown = find_library(libs, "ruby_init", 
			  Config::expand(CONFIG["archdir"].dup))
end

_,lib_dir = dir_config("db", "/usr/include", "/usr/lib")
case Config::CONFIG["arch"]
when /solaris2/
   $DLDFLAGS ||= ""
   $DLDFLAGS += " -R#{lib_dir}"
end
$bdb_libdir = lib_dir

$CFLAGS += " -DBDB_NO_THREAD_COMPILE" if enable_config("thread") == false

unique = if with_config("db-uniquename")
	    with_config("db-uniquename")
	 else
	    ""
	 end

if with_config("db-pthread")
   $LDFLAGS += " -lpthread"
end

version  = with_config('db-version', "-4.7,47,-4.6,46,-4.5,45,-4.4,44,-4.3,43,-4.2,42,-4.1,41,-4.0,-4,40,4,3,2,").split(/,/, -1)
version << "" if version.empty?

catch(:done) do
   pthread = /-lpthread/ =~ $LDFLAGS
   loop do
      version.each do |with_ver|
         if unique != true
            db_version = "db_version" + unique
            throw :done if have_library("db#{with_ver}", db_version, "db.h")
         end
         if with_ver != "" && (unique == "" || unique == true)
            /(\d)\.?(\d)?/ =~ with_ver
            major = $1.to_i
            minor = $2.to_i
            db_version = "db_version_" + (1000 * major + minor).to_s
            throw :done if have_library("db#{with_ver}", db_version, "db.h")
         end
      end
      break if pthread
      $LDFLAGS += " -lpthread"
      pthread = true
      puts 'Trying with -lpthread'
   end
   raise "libdb#{version[-1]} not found"
end

["rb_frame_this_func", "rb_block_proc", "rb_io_stdio_file", "rb_block_call"].each do |f|
   have_func(f, "ruby.h")
end

["insert", "values_at"].each do |f|
   print "checking for Array##{f}... "
   if [].respond_to?(f)
      puts "yes"
      $defs << "-DHAVE_RB_ARY_#{f.upcase}"
   else
      puts "no"
   end
end

have_type('rb_io_t', ['ruby.h', 'rubyio.h'])

if enable_config('db-xml')
   $defs << '-DHAVE_DBXML_INTERFACE'
else
   $defs << '-DNOT_HAVE_DBXML_INTERFACE'
end

require 'features'

create_makefile("bdb")
if unknown
   begin
      make = open("Makefile", "a")
      make.print <<-EOF

unknown: $(DLLIB)
\t@echo "main() {}" > /tmp/a.c
\t$(CC) -static /tmp/a.c $(OBJS) $(CPPFLAGS) $(LIBPATH) $(LIBS) $(LOCAL_LIBS)
\t@-rm /tmp/a.c a.out

EOF
   ensure
      make.close
   end
end
