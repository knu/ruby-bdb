#!/usr/bin/ruby -I../.. -I../../../src

require 'bdbxml'

def timestamp(manager, name, query, context)
   query = "collection('#{name}')#{query}"
   puts "Exercising query '#{query}'"
   print "Return to continue : "
   STDIN.gets
   results = manager.query(query, context)
   results.each do |r|
      r = r.to_document
      meta = r.get("http://dbxmlExamples/timestamp", "timeStamp")
      puts "Document '#{r.name}' stored on #{meta}"
   end
   puts "#{results.size} objects returned for expression '#{query}'"
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

timestamp(man, con.name, "/vegetables:item", cxt)
