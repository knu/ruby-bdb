#!/usr/bin/ruby
require './clean.rb'
if BDB::VERSION_MAJOR == 2 && BDB::VERSION_MINOR < 6
      raise "join exist only with db >= 2.6"
end

BDB::Env.cleanup("tmp", true)

module BDB
   class Btree
      def cursor_set(a)
	 c = cursor
	 c.set(a)
	 c
      end
   end
end

primary = {
   "apple" => "Convenience Store",
   "peach" => "Shopway",
   "pear" => "Farmer's Market",
   "raspberry" => "Shopway",
   "strawberry" => "Farmer's Market",
   "blueberry" => "Farmer's Market"
}
sec1 = {
   "red" => ["apple", "raspberry", "strawberry"],
   "yellow" => ["peach", "pear"],
   "blue" => ["bluberry"]
}
sec2 = {
   "expensive" => ["blueberry", "peach", "pear", "strawberry"],
   "inexpensive" => ["apple", "pear", "raspberry"]
}

env = BDB::Env.open "tmp", BDB::INIT_MPOOL | BDB::CREATE | BDB::INIT_LOCK

p = BDB::Btree.open "primary", nil, BDB::CREATE, "env" => env
primary.each do |k, v|
   p[k] = v
end

s1 = env.open_db BDB::Btree, "sec1", nil, BDB::CREATE,
   "set_flags" => BDB::DUP | BDB::DUPSORT
sec1.each do |k, v|
   v.each do |v1|
      s1[k] = v1
   end
end

s2 = BDB::Btree.open "sec2", nil, BDB::CREATE, "env" => env,
   "set_flags" => BDB::DUP | BDB::DUPSORT
sec2.each do |k, v|
   v.each do |v2|
      s2[k] = v2
   end
end

print "red AND inexpensive\n"
p.join([s1.cursor_set("red"), s2.cursor_set("inexpensive")]) do |k, v|
   print "\t#{k} -- #{v}\n"
end
print "expensive\n"
p.join([s2.cursor_set("expensive")]) do |k, v|
   print "\t#{k} -- #{v}\n"
end
s1.close
s2.close
