#!/usr/bin/ruby
require 'mkmf'
if prefix = with_config("db-prefix")
   $CFLAGS += " -I#{prefix}/include"
   $LDFLAGS += " -L#{prefix}/lib"
   case Config::CONFIG["arch"]
   when /solaris2/
      $LDFLAGS += " -R#{prefix}/lib"
   end
end
$CFLAGS += " -DBDB_NO_THREAD" if enable_config("thread") == false
$CFLAGS += " -I#{incdir}" if incdir = with_config("db-include-dir")
if libdir = with_config("db-lib-dir")
   $LDFLAGS += " -I#{libdir}" 
   case Config::CONFIG["arch"]
   when /solaris2/
      $LDFLAGS += " -R#{libdir}"
   end
end

unique = if with_config("uniquename")
	    "_" + with_config("uniquename")
	 else
	    ""
	 end

version  = with_config('db-version', "-4,4,3,2,").split(/,/, -1)

catch(:done) do
   version.each do |with_ver|
      db_version = "db_version" + (("#{with_ver[-1,1]}" > "2") ? unique : "")
      throw :done if have_library("db#{with_ver}", db_version)
   end
   raise "libdb#{version[-1]} not found"
end

create_makefile("bdb")
begin
   make = open("Makefile", "a")
   make.print <<-EOF

unknown: $(DLLIB)
\t@echo "main() {}" > /tmp/a.c
\t$(CC) -static /tmp/a.c $(OBJS) $(CPPFLAGS) $(DLDFLAGS) -lruby #{CONFIG["LIBS"]} $(LIBS) $(LOCAL_LIBS)
\t@-rm /tmp/a.c a.out
   EOF
ensure
   make.close
end
