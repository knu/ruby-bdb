#!/usr/bin/ruby -I../.. -I../../../src

require 'bdbxml'

def query_context(manager, name, query, context)
   query = "collection('#{name}')#{query}"
   puts "Exercising query '#{query}'"
   print "Return to continue : "
   STDIN.gets
   results = manager.query(query, context)
   results.each {|r| puts r}
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
cxt["aDessert"] = "Blueberry Boy Bait"

query_context(man, con.name, "/vendor", cxt)
query_context(man, con.name, "/item/product[text()='Lemon Grass']", cxt)
query_context(man, con.name, "/fruits:item/product[text()='Lemon Grass']", cxt)
query_context(man, con.name, "/vegetables:item", cxt)
query_context(man, con.name, "/desserts:item/product[text()=$aDessert]", cxt)
