#!/usr/bin/ruby
require './clean.rb'

BDB::Env.cleanup("tmp", true)

out = open("tmp/recno.rb", "w")
File.foreach("recno.rb") do |line|
   out.print line
end
out.close
db = BDB::Recno.open nil, nil, "set_re_source" => "tmp/recno.rb",
   "set_flags" => BDB::RENUMBER | BDB::SNAPSHOT
print "====> BEFORE\n"
db.each do |k, v|
   print "line #{k} : #{v}\n"
   db[k] = nil if /^db/ =~ v
end
print "====> AFTER\n"
db.each do |k, v|
   print "line #{k} : #{v}\n"
end
print "====> STAT\n"
db.stat.each do |k, v|
   print "stat #{k} : #{v}\n"
end
db.close
