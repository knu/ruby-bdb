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

  --with-db-version=<list of comma separated suffixes to add to libdb>
      default=auto-detected if above values include one, or suffixes
              of all supported versions)

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

inc_dir, lib_dir = dir_config("db", "/usr/include", "/usr/lib")
case Config::CONFIG["arch"]
when /solaris2/
   $DLDFLAGS ||= ""
   $DLDFLAGS += " -R#{lib_dir}"
end
$bdb_libdir = lib_dir

$CFLAGS += " -DBDB_NO_THREAD_COMPILE" if enable_config("thread") == false

unique = with_config("db-uniquename") || ''

if with_config("db-pthread")
   $LDFLAGS += " -lpthread"
end

if csv = with_config('db-version')
   version = csv.split(',', -1)
   version << '' if version.empty?
elsif m = lib_dir.match(%r{/db(?:([2-9])|([2-9])([0-9])|-([2-9]).([0-9]))(?:$|/)}) ||
          inc_dir.match(%r{/db(?:([2-9])|([2-9])([0-9])|-([2-9]).([0-9]))(?:$|/)})
   if m[1]
      version = [m[1], '']
   else
      if m[2]
         major, minor = m[2], m[3]
      else
         major, minor = m[4], m[5]
      end
      version = ['-%d.%d' % [major, minor], '%d%d' % [major, minor], '']
   end
else
   version = [
      %w[4.7 4.6 4.5 4.4 4.3 4.2 4.1 4.0 4 3 2].map { |ver|
         major, minor = ver.split('.')
         minor ? ['-%d.%d' % [major, minor], '%d%d' % [major, minor]] : major
      }, ''
   ].flatten
end

catch(:done) do
   pthread = /-lpthread/ =~ $LDFLAGS
   loop do
      version.each do |with_ver|
         if unique.is_a?(String)
            db_version = "db_version" + unique
            throw :done if have_library("db#{with_ver}", db_version, "db.h")
         end
         next if with_ver.empty?
         if !unique.is_a?(String) || unique.empty?
            m = with_ver.match(/^[^0-9]*([2-9])\.?([0-9]{0,3})/)
            major = m[1].to_i
            minor = m[2].to_i
            db_version = "db_version_" + (1000 * major + minor).to_s
            throw :done if have_library("db#{with_ver}", db_version, "db.h")
         end
      end
      break if pthread
      $LDFLAGS += " -lpthread"
      pthread = true
      puts 'Trying with -lpthread'
   end
   raise "libdb#{version.last} not found"
end

headers = ["ruby.h"]
if have_header("ruby/io.h")
   headers << "ruby/io.h"
else
   headers << "rubyio.h"
end
["rb_frame_this_func", "rb_block_proc", "rb_io_stdio_file", "rb_block_call"].each do |f|
   have_func(f, "ruby.h")
end

have_type('rb_io_t', headers)

if enable_config('db-xml')
   $defs << '-DHAVE_DBXML_INTERFACE'
else
   $defs << '-DNOT_HAVE_DBXML_INTERFACE'
end

require File.expand_path('features', File.dirname(__FILE__))

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

# Local variables:
# ruby-indent-tabs-mode: nil
# ruby-indent-level: 3
# end:
# vim: sw=3
