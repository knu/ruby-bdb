#!/usr/bin/ruby -I../src
$LOAD_PATH.unshift "../src"
require 'bdbxml'

p BDB::XML::Name

env = BDB::Env.new("tmp", BDB::CREATE | BDB::INIT_TRANSACTION)
doc = env.open_xml("toto", "a")
doc.each {|x| p x }
index = doc.index
index.add("http://moulon.inra.fr/", "reference", 
	   "node-attribute-equality-string")
doc.index = index
bdb = env.open_db(BDB::Btree, "tutu", nil, "a")
2.times do |i|
   doc.push("<bk><ttl id='#{i}'>title nb #{i}</ttl></bk>")
   bdb[i] = "bdb#{i}"
end
env.begin(doc, bdb) do |txn, doc1, bdb1|
   2.times do |i|
      bdb1[i+2] = "bdb#{i+2}"
      doc1.push("<bk><ttl id='#{i+2}'>title nb #{i+2}</ttl></bk>")
   end
   puts "========================================="
   doc1.each {|x| p x }
   bdb1.each {|k,v| p "#{k} -- #{v}" }
   # implicit txn.abort
end
puts "========================================="
doc.each {|x| p x }
bdb.each {|k,v| p "#{k} -- #{v}" }

