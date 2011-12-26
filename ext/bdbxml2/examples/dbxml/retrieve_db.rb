#!/usr/bin/ruby -I../.. -I../../../src

require 'bdbxml'

def value(manager, document, xpath, context)
   query = manager.prepare(xpath, context)
   result = query.execute(document, context).to_a
   if result.size != 1
      raise "Expected 1 got #{result.size}"
   end
   result[0].to_s
end

options = {'home' => 'env', 'container' => 'name.xml'}

env = BDB::Env.new(options['home'], BDB::INIT_TRANSACTION)
man = env.manager
context = man.create_query_context
man.begin do |txn|
   con = txn.open_container(options['container'])
   bdb = txn.open_db(BDB::Btree, 'bdb', nil, 'a')
   txn.query("collection('#{options['container']}')/vendor") do |doc|
      res = value(txn, doc, "/vendor/salesrep/name/text()", context)
      puts "For key '#{res}', retrieved : "
      puts "\t #{bdb[res]}"
   end
   txn.commit
end

