#!/usr/local/bin/ruby -I../.. -I../../../src

require 'bdbxml'

query = [
   '/vendor', 
   '/vendor[@type="wholesale"]', 
   '/product/item[text()="Lemon Grass"]', 
   '/product/inventory[number(price)<=0.11]',
   '/product[number(inventory/price)<=0.11 and category/text()="vegetables"]',
]

options = {'home' => 'env', 'container' => 'simple.xml'}

env = BDB::Env.new(options['home'], BDB::INIT_LOMP)
man = env.manager
con = man.open_container(options['container'])
query.each do |q|
   fq = "collection('#{con.name}')#{q}"
   puts "Exercising query '#{fq}'"
   print "Return to continue : "
   STDIN.gets
   cxt = man.create_query_context
   results = man.query(fq, cxt) 
   results.each {|r| puts r}
   puts "#{results.size} objects returned for expression '#{fq}'"
   puts
end

   
      

      

