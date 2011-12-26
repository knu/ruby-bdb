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

def new_document(document, context)
   res = value(document,"/*/inventory/inventory/text()", context).to_s
   document.to_s.sub(/#{res}/, "#{res}A")
end

def update(container, xpath, context)
   puts "Updating document for '#{xpath}'"
   print "Return to continue : "
   STDIN.gets
   results = container.query(xpath, context)
   puts "\nFound #{results.size} documents matching the expression '#{xpath}'"
   results.each do |r|
      puts "Updating document #{r.name}"
      puts r
      print "Return to continue : "
      STDIN.gets
      r.content = new_document(r, context)
      container.update(r)
   end
   puts
rescue
   puts "Query #{xpath} failed : #{$!}"
end

def retrieve(container, xpath, context)
   puts "Retrieving document for '#{xpath}'"
   print "Return to continue : "
   STDIN.gets
   results = container.query(xpath, context)
   puts "\nFound #{results.size} documents matching the expression '#{xpath}'"
   results.each do |r|
      puts "Document #{r.name}"
      puts r
      print "Return to continue : "
      STDIN.gets
   end
   puts
rescue
   puts "Query #{xpath} failed : #{$!}"
end

options = {'home' => 'env', 'container' => 'name.xml'}

BDB::Env.new(options['home'], BDB::INIT_TRANSACTION).begin do |txn|
   con = txn.open_xml(options['container'])
   context = BDB::XML::Context.new
   context.set_namespace("fruits", "http://groceryItem.dbxml/fruits")
   context.set_namespace("vegetables", "http://groceryItem.dbxml/vegetables")
   context.set_namespace("desserts", "http://groceryItem.dbxml/desserts");
   update(con, "/fruits:item/product[text() = 'Zapote Blanco']", context)
   retrieve(con, "/fruits:item/product[text() = 'Zapote Blanco']", context)
   txn.commit
end

