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

def details(container, xpath, context)
   puts "Exercising query '#{xpath}'"
   print "Return to continue : "
   STDIN.gets
   results = container.query(xpath, context)
   puts "\n\tProduct : Price : Inventory Level"
   results.each do |r|
      item = value(r, "/*/product/text()", context)
      price = value(r, "/*/inventory/price/text()", context)
      inventory = value(r, "/*/inventory/inventory/text()", context)
      puts "\t#{item} : #{price} : #{inventory}"
   end
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

details(con, "/fruits:item/product[text() = 'Zulu Nut']", context)
details(con, "/vegetables:item/product[starts-with(text(),'A')]", context)
