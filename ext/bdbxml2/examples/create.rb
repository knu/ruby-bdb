#!/usr/local/bin/ruby
$LOAD_PATH.unshift("..", "../../src")
require 'bdbxml'

include BDB::XML

File.unlink("exa.dbxml") rescue nil

man = Manager.new
con = man.create_container("exa.dbxml")
at_exit {
   con.close
   man.close
}
con.put("book12", 
        "<book><title>Knowledge Discovery in Databases I</title></book>")
con.put("book21", 
        "<book><title>Knowledge Discovery in Databases II</title></book>")
doc = con.get("book12")
puts "#{doc.name} = #{doc}"
res = Results.new(man)
res.add(doc)
man.query("collection('exa.dbxml')/book") do |val|
   puts "#{val.to_document.name} = #{val}"
end
man.query("collection('exa.dbxml')/book")
