#!/usr/bin/ruby -I../.. -I../../../src

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
con = env.open_xml(options['container'])
query.each do |q|
   puts "Exercising query '#{q}'"
   print "Return to continue : "
   STDIN.gets
   results = con.query(q)
   results.each {|r| puts r}
   puts "#{results.size} objects returned for expression '#{q}'"
   puts
end

   
      

      

