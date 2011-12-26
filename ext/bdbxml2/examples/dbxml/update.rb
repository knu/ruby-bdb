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

def new_document(man, doc, context)
   res = value(man, doc, "/*/inventory/inventory/text()", context).to_s
   doc.to_s.sub(/#{res}/, "#{res}__NEW_VALUE")
end

def update(man, con, query, context)
   query = "collection('#{con.name}')/#{query}"
   puts "Updating document for '#{query}'"
   print "Return to continue : "
   STDIN.gets
   results = man.query(query, context)
   puts "\nFound #{results.size} documents matching the expression '#{query}'"
   results.each do |r|
      r = r.to_document
      puts "Updating document #{r.name}"
      puts r
      print "Return to continue : "
      STDIN.gets
      r.content = new_document(man, r, context)
      con.update(r)
   end
   puts
rescue
   puts "Query #{query} failed : #{$!}"
end

def retrieve(man, con, query, context)
   query = "collection('#{con.name}')/#{query}"
   puts "Retrieving document for '#{query}'"
   print "Return to continue : "
   STDIN.gets
   results = man.query(query, context)
   puts "\nFound #{results.size} documents matching the expression '#{query}'"
   results.each do |r|
      r = r.to_document
      puts "Document #{r.name}"
      puts r
      print "Return to continue : "
      STDIN.gets
   end
   puts
rescue
   puts "Query #{query} failed : #{$!}"
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
   update(txn, con, "/fruits:item/product[text() = 'Zapote Blanco']", cxt)
   retrieve(txn, con, "/fruits:item/product[text() = 'Zapote Blanco']", cxt)
   txn.commit
end

