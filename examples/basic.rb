#!/usr/bin/ruby
require './clean.rb'

BDB::Env.cleanup "tmp", true

db = BDB::Btree.open "tmp/basic", nil, BDB::CREATE | BDB::TRUNCATE, 0644,
     "set_pagesize" => 1024, "set_cachesize" => [0, 32 * 1024, 0]
File.foreach("wordlist") do |line|
    line.chomp!
    db[line] = line.reverse
end
db.stat.each do |k, v|
    print "#{k}\t#{v}\n"
end
db.each do |k, v|
    if k != v.reverse
	print "ERROR : #{k} -- #{v}\n"
    end
end
db.close
