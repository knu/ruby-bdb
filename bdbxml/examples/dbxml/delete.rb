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

def delete(container, xpath, context)
   puts "Deleting document for '#{xpath}'"
   print "Return to continue : "
   STDIN.gets
   results = container.query(xpath, context)
   puts "\nFound #{results.size} documents matching the expression '#{xpath}'"
   results.each do |r|
      puts "Deleting document #{value(r, '/*/product/text()', context)}"
      container.delete(r)
      puts "Deleted document #{value(r, '/*/product/text()', context)}"
   end
   puts
rescue
   puts "Query #{xpath} failed : #{$!}"
end

def confirm(container, xpath, context)
   puts "Conforming the delete document"
   puts "The query : '#{xpath}' should get result set size 0"
   if container.query(xpath, context).size == 0
      puts "Deletion confirmed."
   else
      puts "Document deletion failed."
   end
end

options = {'home' => 'env', 'container' => 'name.xml'}

BDB::Env.new(options['home'], BDB::INIT_TRANSACTION).begin do |txn|
   con = txn.open_xml(options['container'])
   context = BDB::XML::Context.new
   context.set_namespace("fruits", "http://groceryItem.dbxml/fruits")
   context.set_namespace("vegetables", "http://groceryItem.dbxml/vegetables")
   context.set_namespace("desserts", "http://groceryItem.dbxml/desserts");
   delete(con, "/fruits:item[product = 'Mabolo']", context)
   confirm(con, "/fruits:item[product = 'Mabolo']", context)
   txn.commit
end

