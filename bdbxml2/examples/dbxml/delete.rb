#!/usr/bin/ruby -I../.. -I../../../src

require 'bdbxml'

def value(man, doc, query, context)
   doc_expr = man.prepare(query, context)
   result = doc_expr.execute(doc, context).to_a
   if result.size != 1
      raise "Expected 1 got #{result.size}"
   end
   result[0]
end

def delete(man, con, query, context)
   puts "Deleting document for '#{query}'"
   print "Return to continue : "
   STDIN.gets
   results = man.query(query, context)
   puts "\nFound #{results.size} documents matching the expression '#{query}'"
   results.each do |r|
      r = r.to_document
      puts "Deleting document #{value(man, r, '/*/product/text()', context)}"
      con.delete(r)
      puts "Deleted document #{value(man, r, '/*/product/text()', context)}"
   end
   puts
rescue
   puts "Query #{query} failed : #{$!}"
end

def confirm(man, con, query, context)
   puts "Conforming the delete document"
   puts "The query : '#{query}' should get result set size 0"
   if man.query(query, context).size == 0
      puts "Deletion confirmed."
   else
      puts "Document deletion failed."
   end
end

options = {'home' => 'env', 'container' => 'name.xml'}

env = BDB::Env.new(options['home'], BDB::INIT_TRANSACTION)
man = env.manager
man.begin do |txn|
   con = txn.open_container(options['container'])
   cxt = man.create_query_context
   cxt.set_namespace("fruits", "http://groceryItem.dbxml/fruits")
   cxt.set_namespace("vegetables", "http://groceryItem.dbxml/vegetables")
   cxt.set_namespace("desserts", "http://groceryItem.dbxml/desserts");
   query = "collection('#{con.name}')/fruits:item[product = 'Mabolo']"
   delete(txn, con, query, cxt)
   confirm(txn, con, query, cxt)
   txn.commit
end

