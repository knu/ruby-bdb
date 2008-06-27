#!/usr/local/bin/ruby
$LOAD_PATH.unshift("..", "../../src")
require 'bdbxml'
require 'find'

Find::find('tmp') do |f|
   File::unlink(f) if FileTest::file? f
end

env = BDB::Env.new("tmp", BDB::CREATE | BDB::INIT_TRANSACTION)
bdb = env.open_db(BDB::Btree, "tutu", nil, "a")
man = env.manager
doc = man.create_container("toto", BDB::XML::TRANSACTIONAL)
at_exit {
   doc.close
   man.close
}

2.times do |i|
   doc.put("#{i}", "<bk><ttl id='#{i}'>title nb #{i}</ttl></bk>")
   bdb[i] = "bdb#{i}"
end
que = man.prepare("collection('toto')/bk")
qc = man.create_query_context
man.begin(doc, bdb, que) do |txn, doc1, bdb1, que1|
   2.times do |i|
      bdb1[i+2] = "bdb#{i+2}"
      doc1.put("#{i+2}", "<bk><ttl id='#{i+2}'>title nb #{i+2}</ttl></bk>")
   end
   puts "=============== get ========================"
   p doc1.get("2")
   puts "================ each ======================"
   que1.execute(qc) {|x| p x }
   puts "================= bdb ======================"
   bdb1.each {|k,v| p "#{k} -- #{v}" }
   # implicit txn.abort
end
puts "=============== each ======================"
que.execute(qc) {|x| p x }
puts "================= bdb ====================="
bdb.each {|k,v| p "#{k} -- #{v}" }

