#!/usr/bin/ruby
require 'mkmf'
if prefix = with_config("db-prefix")
    $CFLAGS = "-I#{prefix}/include"
    $LDFLAGS += " -L#{prefix}/lib"
end
$CFLAGS = "-I#{incdir}" if incdir = with_config("db-include-dir")
$LDFLAGS += " -I#{libdir}" if libdir = with_config("db-lib-dir")
if ! have_library("db", "db_version") 
    raise "libdb.a not found"
end
create_makefile("bdb")
open("Makefile", "a") do |make|
    make.print <<-EOF

%.html: %.rd
	rd2 $< > ${<:%.rd=%.html}

HTML = bdb.html docs/access.html  docs/env.html  docs/transaction.html \\
       docs/cursor.html

html: $(HTML)

    EOF
end
