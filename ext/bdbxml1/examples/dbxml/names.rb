#!/usr/bin/ruby -I../.. -I../../../src

require 'bdbxml'

def names(container, xpath, context)
   puts "Exercising query '#{xpath}'"
   print "Return to continue : "
   STDIN.gets
   results = container.query(xpath, context)
   results.each do |r|
      puts "Document name : #{r.name}"
      puts r
   end
   puts "#{results.size} objects returned for expression '#{xpath}'"
   puts
rescue
   puts "Query #{xpath} failed : #{$!}"
end

options = {'home' => 'env', 'container' => 'name.xml'}

con = BDB::Env.new(options['home'], BDB::INIT_LOMP).
   open_xml(options['container'])

context = BDB::XML::Context.new
context.set_namespace("fruits", "http://groceryItem.dbxml/fruits")
context.set_namespace("vegetables", "http://groceryItem.dbxml/vegetables")
context.set_namespace("desserts", "http://groceryItem.dbxml/desserts");

names(con, "//*[@dbxml:name='ZuluNut.xml']", context)
names(con, "//*[@dbxml:name='TrifleOrange.xml']", context)
names(con, "//*[@dbxml:name='TriCountyProduce.xml']", context)
