#!/usr/bin/ruby
require 'bdb'

cat = Struct.new("Cat", :name, :age, :life)

bdb = BDB::Btree.open "tmp/aa", nil, "w", "marshal" => true
aux = BDB::Btree.open "tmp/bb", nil, "w", 
   "set_flags" => BDB::DUPSORT, "marshal" => true
bdb.associate(aux) { |aux, key, value| value.life }
36.times do |i|
   bdb["a" + i.to_s] = cat.new "cat" + i.to_s, 1 + rand(24), 1 + rand(7)
end
aux.each do |k, v|
   puts "key : #{k} -- value #{v.inspect}"
end
7.times do |i|
   p aux.duplicates(1 + i)
end
aux.each_dup_value(7) do |v|
   puts "value #{v.inspect}"
end
aux.each_primary do |sk, pk, pv|
   puts "pk : #{pk} pv : #{pv.inspect} sk : #{sk}"
end

aux.reverse_each_primary do |sk, pk, pv|
   puts "pk : #{pk} pv : #{pv.inspect} sk : #{sk}"
end



