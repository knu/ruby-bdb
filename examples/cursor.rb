#!/usr/bin/ruby
require './clean.rb'

BDB::Env.cleanup("tmp", true)

db = BDB::Btree.open "tmp/cursor", nil, BDB::CREATE | BDB::TRUNCATE, 0644,
     "set_pagesize" => 1024, "set_flags" => BDB::RECNUM
indice = 0
File.foreach("wordlist") do |line|
    line.chomp!
    line = format("%05d", indice) + line
    db[indice] = line.reverse
    indice += 1
end
print "======== db.stat\n"
db.stat.each do |k, v|
    print "#{k}\t#{v}\n"
end
print "===============\n"
total = db.stat["bt_nrecs"]
cursor = db.cursor
10.times do
    indice = rand(total)
    k, v = cursor.set_recno(indice + 1)
    print "Recno : #{indice}\t: #{k} -- #{v}\n"
    k, v = cursor.next
    print "        NEXT\t: #{k} -- #{v}\n"
    k, v = cursor.prev
    print "        PREV\t: #{k} -- #{v}\n"
end
cursor.close
db.close
