#!/usr/bin/ruby
require './clean.rb'

BDB::Env.cleanup("tmp", true)

env = BDB::Env.open "tmp", BDB::CREATE | BDB::INIT_LOG, "thread" => false
lsn = []
100.times do |i|
   lsn.push(env.log_put("test toto #{i}"))
end
env.log_flush
i = 0
env.log_each do |l,|
   if l != "test toto #{i}"
      puts "Error #{i} -- #{l}"
   end
   i += 1
end
i = 99
env.log_reverse_each do |l,|
   if l != "test toto #{i}"
      puts "Error #{l}"
   end
   i -= 1 
end
1000.times do
   nb = rand(lsn.size)
   if lsn[nb].log_get != "test toto #{nb}"
      puts "Error #{nb}"
   end
end
env.close

BDB::Env.cleanup "tmp" 

max_log = 45000
env = BDB::Env.open "tmp", BDB::CREATE | BDB::INIT_LOG, "thread" => false, 
   "set_lg_bsize" => 10000, "set_lg_max" => max_log
init = "a" * 200
lsn = []
rec = []
500.times do |i|
   j = env.log_put "init %d" % i
   if (i % 25) == 0
      rec.push "init %d" % i
      lsn.push j
   end
end
env.log_flush
log_file = Dir.glob("tmp/log*")
l = nil
lsn.each_with_index do |ls, i|
   if l
      if ls <= l
	 puts "Error compare"
      end
   end
   if ls.log_get != rec[i]
      puts "Error retrieve"
   end
   l = ls
end
lsn.each do |ls|
   if !log_file.include? ls.log_file
      puts "Error #{ls.log_file}"
   end
end
env.log_stat.each do |k, v|
   puts "%s\t%s" % [k, v]
end
env.close

BDB::Env.cleanup "tmp"



