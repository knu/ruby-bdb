#!/usr/bin/ruby
require './clean.rb'

BDB::Env.cleanup("tmp", true)

module BDB
   class Btreesort < Btree
      def bdb_bt_compare(a, b)
	 b.downcase <=> a.downcase
      end
   end
end
a = { "gamma" => 4, "Alpha" => 1, "delta" => 3, "Beta" => 2, "delta" => 3}
env = BDB::Env.open "tmp", BDB::CREATE | BDB::INIT_MPOOL
db = BDB::Btreesort.open "alpha", nil, "w", "env" => env
a.each do |x, y|
   db[x] = y
end
db.each do |x, y|
   puts "SORT : #{x} -- #{y}"
end
db.close
db = BDB::Hash.open "alpha", nil, "w", "env" => env, 
   "set_h_hash" => lambda { |x| x.hash }
a.each do |x, y|
   puts "#{x} -- #{y}"
   db[x] = y
end
db.each do |x, y|
   puts "HASH : #{x} -- #{y}"
end


