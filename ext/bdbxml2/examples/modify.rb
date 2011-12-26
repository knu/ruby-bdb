#!/usr/local/bin/ruby
$LOAD_PATH.unshift("..", "../../src")
require 'bdbxml'

include BDB::XML

File.unlink("exa.dbxml") rescue nil

man = Manager.new
doc = man.create_document
at_exit {
   doc.close
   man.close
}
doc.name = "test1"
doc.content = '<?xml version="1.0" encoding="utf-8" standalone="no"?><root/>'
puts "#{doc.name} = #{doc}"
que = man.prepare("/root")
mod = Modify.new(man)
mod.append(que, Modify::Element, "new", "foo")
mod.execute(doc)
puts "#{doc.name} = #{doc}"

