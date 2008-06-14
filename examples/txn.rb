#!/usr/bin/ruby
require './clean.rb'

BDB::Env.cleanup("tmp", true)


module BDB
   class ThreadHash < Hash
      def start(val, num)
         Thread.start do
            self.env.begin(self) do |txn, b1|
               p "Thread #{num} Enter"
               b1.delete_if do |k, v|
                  print "\t#{num} #{k} -- #{v}\n"
                  k == val
               end
               p "Thread #{num} Pass"
#              Thread.pass
               b1.each do |k, v|
                  print "\t#{num} #{k} -- #{v}\n"
               end
               if num == 1
                  txn.abort
               else
                  txn.commit
               end
            end
            p "End Thread #{num}"
         end
      end
   end
end

env = BDB::Env.new("tmp", BDB::CREATE|BDB::INIT_TRANSACTION)
db = env.open_db(BDB::ThreadHash, "txn", nil, BDB::CREATE)
{"alpha" => "beta", "gamma" => "delta", "delta" => "epsilon"}.
    each do |key, value|
    db[key] = value
end
db.each do |k, v|
    print "0 #{k} -- #{v}\n"
end
t1 = db.start("alpha", 1)
t2 = db.start("gamma", 2)
t1.join
t2.join
db.each do |k, v|
    print "0 #{k} -- #{v}\n"
end
env.close
