#!/usr/bin/ruby
require 'bdb'
module BDB
   class Bcomp < Btree
      extend Marshal

      def bdb_bt_compare(a, b)
	 a <=> b
      end
   end
end

record1Class = Struct.new('Record1', :f1, :f2 ,:f3)
record2Class = Struct.new('Record2', :f3, :f2 ,:f1)

File.delete('hash1.bdb') if File.exists?('hash1.bdb')

hash1 = BDB::Bcomp.create('hash1.bdb', nil, "w", 0644,
			  'set_pagesize' => 1024) 
f1 = 'aaaa'
f2 = 'lmno'
f3 = 'rrrr'
100.times do |i|
   rec1 = record1Class.new
   rec1.f1 = f1 = f1.succ
   rec1.f2 = f2 = f2.succ
   rec1.f3 = f3 = f3.succ
   hash1[rec1.f1] = rec1 
end

File.delete('hash2.bdb') if File.exists?('hash2.bdb')

hash2 = BDB::Bcomp.create('hash2.bdb', nil, "w", 0644, 
			  'set_pagesize' => 1024)
hash1.each do |k,rec1|
   rec2 = record2Class.new
   rec2.f1 = rec1.f1
   rec2.f2 = rec1.f2
   rec2.f3 = rec1.f3
   hash2[rec2.f1] = rec2
   puts "Record2 #{hash2[rec2.f1].inspect}"
end
