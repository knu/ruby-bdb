#!/usr/bin/ruby
require 'mkmf'
if prefix = with_config("db-prefix")
    $CFLAGS += " -I#{prefix}/include"
    $LDFLAGS += " -L#{prefix}/lib"
end
$CFLAGS += " -DBDB_NO_THREAD" if enable_config("thread") == false
$CFLAGS += " -I#{incdir}" if incdir = with_config("db-include-dir")
$LDFLAGS += " -I#{libdir}" if libdir = with_config("db-lib-dir")
if ! have_library("db", "db_version") 
    raise "libdb.a not found"
end
create_makefile("bdb")
begin
   make = open("Makefile", "a")
   make.print <<-EOF

%.html: %.rd
	rd2 $< > ${<:%.rd=%.html}

HTML = bdb.html docs/arraylike.html  docs/env.html  docs/transaction.html \\
       docs/cursor.html docs/hashlike.html

html: $(HTML)

test: $(DLLIB) $(HTML)
   EOF
   Dir.foreach('tests') do |x|
      next if /^\./ =~ x || /(_\.rb|~)$/ =~ x
      next if FileTest.directory?(x)
      make.print "	ruby tests/#{x}\n"
   end
ensure
   make.close
end
