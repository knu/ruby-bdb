#!/usr/bin/ruby
require './clean.rb'

if BDB::VERSION_MAJOR == 2 || 
      (BDB::VERSION_MAJOR == 3 && BDB::VERSION_MINOR < 3)
   raise "associate exist only with DB >= 3.3"
end

BDB::Env.cleanup("tmp", true)

cat = Struct.new("Cat", :name, :age, :life)

bdb = BDB::Btree.open "tmp/aa", nil, "w", "marshal" => true
aux = BDB::Btree.open "tmp/bb", nil, "w", 
   "set_flags" => BDB::DUPSORT, "marshal" => true
bdb.associate(aux) { |aux1, key, value| value.life }
36.times do |i|
   bdb["a" + i.to_s] = cat.new "cat" + i.to_s, 1 + rand(24), 1 + rand(7)
end
p "======================= each =================================="
aux.each do |k, v|
   puts "key : #{k} -- value #{v.inspect}"
end
p "======================== duplicates ==========================="
7.times do |i|
   p aux.duplicates(1 + i)
end
p "======================== each_dup_value ======================="
aux.each_dup_value(7) do |v|
   puts "value #{v.inspect}"
end
p "========================= each_primary ========================"
aux.each_primary do |sk, pk, pv|
   puts "pk : #{pk} pv : #{pv.inspect} sk : #{sk}"
end

p "===================== reverse_each_primary ===================="
aux.reverse_each_primary do |sk, pk, pv|
   puts "pk : #{pk} pv : #{pv.inspect} sk : #{sk}"
end

p "======================= cursor.pget ==========================="
cursor = aux.cursor
while pkv = cursor.pget(BDB::NEXT)
   sk, pk, pv = pkv
   puts "pk : #{pk.inspect} pv : #{pv.inspect} sk : #{sk}"
end

