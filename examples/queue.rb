#!/usr/bin/ruby
require 'bdb'
db = BDB::Queue.open "set_re_len" => 42
File.foreach("queue.rb") do |line|
   db.push line.chomp
end
db.each do |x, y|
   p "#{x} -- #{y}"
end
a = db.shift
print "====> STAT\n"
db.stat.each do |k, v|
   print "stat #{k} : #{v}\n"
end
db.close
