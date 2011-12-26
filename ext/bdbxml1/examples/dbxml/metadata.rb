#!/usr/bin/ruby -I../.. -I../../../src

require 'bdbxml'

def value(document, xpath, context)
   context.returntype = BDB::XML::Context::Values
   result = document.query(xpath, context).to_a
   if result.size != 1
      raise "Expected 1 got #{result.size}"
   end
   result[0]
ensure
   context.returntype = BDB::XML::Context::Documents
end

def timestamp(container, xpath, context)
   puts "Exercising query '#{xpath}'"
   print "Return to continue : "
   STDIN.gets
   results = container.query(xpath, context)
   puts "\n\tProduct : Price : Inventory Level"
   results.each do |r|
      meta = r.get("http://dbxmlExamples/timestamp", "timeStamp", String)
      puts "Document #{r.name} stored on #{meta}"
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

timestamp(con, "/vegetables:item", context)
