#!/usr/bin/ruby
ARGV.collect! {|x| x.sub(/^--with-db-prefix=/, "--with-db-dir=") }

require 'mkmf'

if unknown = enable_config("unknown")
   libs = if CONFIG.key?("LIBRUBYARG_STATIC")
	     Config::expand(CONFIG["LIBRUBYARG_STATIC"].dup).sub(/^-l/, '')
	  else
	     Config::expand(CONFIG["LIBRUBYARG"].dup).sub(/lib([^.]*).*/, '\\1')
	  end
   unknown = find_library(libs, "ruby_init", 
			  Config::expand(CONFIG["archdir"].dup))
end

_,lib_dir = dir_config("db", "/usr")
case Config::CONFIG["arch"]
when /solaris2/
   $DLDFLAGS += " -R#{lib_dir}"
end

$CFLAGS += " -DBDB_NO_THREAD_COMPILE" if enable_config("thread") == false

unique = if with_config("uniquename")
	    "_" + with_config("uniquename")
	 else
	    ""
	 end

version  = with_config('db-version', "-4.1,-4.0,-4,4,3,2,").split(/,/, -1)
version << "" if version.empty?

catch(:done) do
   version.each do |with_ver|
      db_version = "db_version" + unique
      throw :done if have_library("db#{with_ver}", db_version)
      if with_ver != "" && unique == ""
	 /(\d)\.?(\d)?/ =~ with_ver
	 major = $1.to_i
	 minor = $2.to_i
	 db_version = "db_version_" + (1000 * major + minor).to_s
	 throw :done if have_library("db#{with_ver}", db_version)
      end
   end
   raise "libdb#{version[-1]} not found"
end

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
