#!/usr/bin/ruby
require './clean.rb'

BDB::Env.cleanup("tmp", true)

module ZeroC
   def bdb_fetch_value(a)
      a.sub(/\0$/, '')
   end
   def bdb_store_value(a)
      a + "\0"
   end
   alias bdb_fetch_key bdb_fetch_value
   alias bdb_store_key bdb_store_value
end
      
module BDB
   class A < Btree
      include ZeroC
   end
end

$option = {"set_pagesize" => 1024, "set_cachesize" => [0, 32 * 1024, 0]}

db = BDB::A.open "tmp/basic", nil, "w", $option
File.foreach("wordlist") do |line|
    line.chomp!
    db[line] = line.reverse
end
db.each do |k, v|
   if k != v.reverse || /\0/ =~ k || /\0/ =~ v
      print "ERROR : #{k.inspect} -- #{v.inspect}\n"
   end
end
db.close

db = BDB::Btree.open "tmp/basic", $option
db.each do |k, v|
   if k[-1] != 0 || v[-1] != 0
      print "ERROR : #{k.inspect} -- #{v.inspect}\n"
   end
end
