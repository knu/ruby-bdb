#!/usr/bin/ruby -I../.. -I../../../src

require 'bdbxml'

def value(manager, document, xpath, context)
   doc = manager.prepare(xpath, context)
   result = doc.execute(document, context).to_a
   if result.size != 1
      raise "Expected 1 got #{result.size}"
   end
   result[0]
end

def details(manager, name, query, context)
   query = "collection('#{name}')#{query}"
   puts "Exercising query '#{query}'"
   print "Return to continue : "
   STDIN.gets
   results = manager.query(query, context)
   puts "\n\tProduct : Price : Inventory Level"
   results.each do |r|
      r = r.to_document
      item = value(manager, r, "/*/product/text()", context)
      price = value(manager, r, "/*/inventory/price/text()", context)
      inventory = value(manager, r, "/*/inventory/inventory/text()", context)
      puts "\t#{item} : #{price} : #{inventory}"
   end
   puts
rescue
   puts "Query #{query} failed : #{$!}"
end

options = {'home' => 'env', 'container' => 'name.xml'}

man = BDB::Env.new(options['home'], BDB::INIT_LOMP).manager
con = man.open_container(options['container'])
cxt = man.create_query_context
cxt.set_namespace("fruits", "http://groceryItem.dbxml/fruits")
cxt.set_namespace("vegetables", "http://groceryItem.dbxml/vegetables")
cxt.set_namespace("desserts", "http://groceryItem.dbxml/desserts");

details(man, con.name, "/fruits:item/product[text() = 'Zulu Nut']", cxt)
details(man, con.name, "/vegetables:item/product[starts-with(text(),'A')]", cxt)
