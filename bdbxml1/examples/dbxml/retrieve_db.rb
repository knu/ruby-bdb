#!/usr/bin/ruby -I../.. -I../../../src

require 'bdbxml'

def value(document, xpath, context)
   result = document.query(xpath, context).to_a
   if result.size != 1
      raise "Expected 1 got #{result.size}"
   end
   result[0].to_s
end

options = {'home' => 'env', 'container' => 'name.xml'}

env = BDB::Env.new(options['home'], BDB::INIT_TRANSACTION)
context = BDB::XML::Context.new
context.returntype = BDB::XML::Context::Values
env.begin do |txn|
   con = txn.open_xml(options['container'])
   bdb = txn.open_db(BDB::Btree, 'bdb', nil, 'a')
   con.search("/vendor") do |doc|
      res = value(doc, "/vendor/salesrep/name/text()", context)
      puts "For key '#{res}', retrieved : "
      puts "\t #{bdb[res]}"
   end
   txn.commit
end

