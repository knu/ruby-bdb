#!/usr/bin/ruby
$LOAD_PATH.unshift(*%w{../src src tests})
require 'bdb'
require 'runit_'

def clean
   Dir.foreach('tmp') do |x|
      if FileTest.file?("tmp/#{x}")
	 File.unlink("tmp/#{x}")
      end
   end
end

def reinject(i)
   (1 .. i).each do |i|
      $array.push i.to_s
      $bdb.push i
   end
end
   
$bdb, $env = nil, nil
clean

print "\nVERSION of BDB is #{BDB::VERSION}\n"

Inh = defined?(RUNIT) ? RUNIT : Test::Unit

class TestRecnum < Inh::TestCase
   def test_00_error
      assert_raises(BDB::Fatal, "invalid name") do
	 BDB::Recnum.open("tmp/aa", nil, "env" => 1)
      end
      assert_raises(BDB::Fatal, "invalid name") do
	 BDB::Recnum.open("tmp/aa", nil, "env" => 1)
      end
   end
   def test_01_init
      assert_kind_of(BDB::Recnum, $bdb = BDB::Recnum.new("tmp/aa", nil, "w"), "<open>")
      $array = []
   end
   def test_02_get_set
      reinject(99)
      $bdb.each_index do |i| 
	 assert_equal($bdb[i], $array[i], "<sequential get>")
      end
      -1.downto(-99) do |i| 
	 assert_equal($bdb[i], $array[i], "<sequential get reversed>")
      end
      200.times do
	 i = rand($array.size)
	 assert_equal($bdb[i], $array[i], "<random get>")
      end
   end

   def test_03_subseq
      assert_equal($bdb[-1], $array[-1], "<negative index>")
      assert_equal($bdb[2 .. 7], $array[2 .. 7], "<subseq>")
      $bdb[2 .. 7] = "a", "b"
      $array[2 .. 7] = "a", "b"
      assert_equal($bdb.size, $array.size, "<size>")
      assert_equal($bdb.to_a, $array, "<small subseq>")
      $bdb[3 .. 6] = "a", "b", "c", "d", "e", "g", "h", "i"
      $array[3 .. 6] = "a", "b", "c", "d", "e", "g", "h", "i"
      assert_equal($bdb.size, $array.size, "<size>")
      assert_equal($bdb.to_a, $array, "<big subseq>")
   end

   def test_04_op
      assert_equal($bdb + ["3", "4"], $array + ["3", "4"], "<plus>")
      assert_equal(["3", "4"] + $bdb, ["3", "4"] + $array, "<plus>")
      assert_equal($bdb - ["3", "4"], $array - ["3", "4"], "<minus>")
      assert_equal(["3", "4"] - $bdb, ["3", "4"] - $array, "<minus>")
      assert_equal($bdb * 2, $array * 2, "<multiply>")
      assert_equal($bdb * ":", $array * ":", "<multiply>")
      assert_equal($bdb * "", $array * "", "<multiply>")
      assert_equal($bdb.size, $array.size, "<size>")
   end

   def test_05_at
      assert_equal($bdb.to_a, $array, "<equal array>")
      assert_equal($bdb.at(0), $array.at(0), "<positive at>")
      assert_equal($bdb.at(10), $array.at(10), "<positive at>")
      assert_equal($bdb.at(99), $array.at(99), "<positive at>")
      assert_equal($bdb.at(205), $array.at(205), "<positive at>")
      assert_equal($bdb.at(-1), $array.at(-1), "<negative at>")
      assert_equal($bdb.at(-100), $array.at(-100), "<negative at>")
      assert_equal($bdb.at(205), $array.at(205), "<negative at>")
   end

   def test_06_slice
      assert_equal($bdb.to_a, $array, "<equal array>")
      100.times do 
	 i = rand($bdb.size)
	 assert_equal($bdb.slice(i), $array.slice(i), "<slice>")
      end
      100.times do
	 i = rand($bdb.size)
	 assert_equal($bdb.slice(-i), $array.slice(-i), "<negative slice>")
      end
      100.times do
	 i = rand($bdb.size)
	 j = rand($bdb.size)
	 assert_equal($bdb.slice(i, j), $array.slice(i, j), "<slice>")
      end
      100.times do
	 i = rand($bdb.size)
	 j = rand($bdb.size)
	 assert_equal($bdb.slice(-i, j), $array.slice(-i, j), "<negative slice>")
      end      
      100.times do
	 i = rand($bdb.size)
	 j = rand($bdb.size)
	 assert_equal($bdb.slice(i .. j), $array.slice(i .. j), "<range>")
      end
      100.times do
	 i = rand($bdb.size)
	 j = rand($bdb.size)
	 assert_equal($bdb.slice(-i, j), $array.slice(-i, j), "<negative range>")
      end
   end

   def test_06_slice_bang
      assert_equal($bdb.to_a, $array, "<equal array>")
      10.times do |iter|
	 i = rand($bdb.size)
	 assert_equal($bdb.slice!(i), $array.slice!(i), "<#{iter} slice!(#{i})>")
	 assert_equal($bdb.size, $array.size, "<size after slice!(#{i})  #{iter}>")
	 reinject(20) if $bdb.size < 20
      end
      10.times do |iter|
	 i = rand($bdb.size)
	 assert_equal($bdb.slice!(-i), $array.slice!(-i), "<slice!(#{-i})>")
	 assert_equal($bdb.size, $array.size, "<size after slice!(#{-i}) #{iter}>")
	 reinject(20) if $bdb.size < 20
      end
      10.times do  |iter|
	 i = rand($bdb.size)
	 j = rand($bdb.size)
	 assert_equal($bdb.slice!(i, j), $array.slice!(i, j), "<slice!(#{i}, #{j})>")
	 assert_equal($bdb.size, $array.size, "<size after slice!(#{i}, #{j}) #{iter}>")
	 reinject(20) if $bdb.size < 20
      end
      10.times do |iter|
	 i = rand($bdb.size)
	 j = i + rand($bdb.size - i)
	 assert_equal($bdb.slice!(-i, j), $array.slice!(-i, j), "<slice!(#{-i}, #{j})>")
	 assert_equal($bdb.size, $array.size, "<size after slice!(#{-i}, #{j}) #{iter}>")
	 reinject(20) if $bdb.size < 20
      end      
      reinject(20)
      another = 0
      10.times do |iter|
	 i = rand($bdb.size)
	 j = i + rand($bdb.size - i)
	 if ! $array.slice(i .. j)
	    assert_error(RangeError, '', "<invalid range>") { $bdb.slice!(i .. j) }
	    another += 1
	    redo if another < 10
	    another = 0
	    next
         end
	 assert_equal($bdb.slice!(i .. j), $array.slice!(i .. j), "<slice!(#{i} .. #{j})>")
	 assert_equal($bdb.size, $array.size, "<size after slice! #{iter}>")
	 another = 0
	 reinject(20) if $bdb.size < 20
      end
      another = 0
      10.times do |iter|
	 i = rand($bdb.size)
	 j = 1 + rand($bdb.size - i)
	 if ! $array.slice(-i .. -j)
	    assert_raises(RangeError, "<invalid range>") { $bdb.slice!(-i .. -j) }
	    another += 1
	    redo if another < 10
	    another = 0
	    next
         end
	 assert_equal($bdb.slice!(-i .. -j), $array.slice!(-i .. -j), "<slice!(#{-i} .. #{-j})>")
	 assert_equal($bdb.size, $array.size, "<size after slice! #{iter}>")
	 another = 0
	 reinject(20) if $bdb.size < 20
      end
      reinject(40) if $bdb.size < 40
      assert_equal($bdb.size, $array.size, "<size end slice!>")
      assert_equal($bdb.to_a, $array, "<size end slice!>")
   end

   def test_07_reverse
      assert_equal($bdb.reverse, $array.reverse, "<reverse>")
      $array.reverse! ; $bdb.reverse!
      assert_equal($bdb.to_a, $array, "<reverse bang>")
      assert_equal($bdb.size, $array.size, "<size after reverse>")
   end

   def test_08_index
      100.times do
	 i = rand($bdb.size)
	 assert_equal($bdb.index(i), $array.index(i), "<index #{i}>")
	 assert_equal($bdb.index(-i), $array.index(-i), "<negative index #{i}>")
      end
      100.times do
	 i = rand($bdb.size)
	 assert_equal($bdb.rindex(i), $array.rindex(i), "<rindex #{i}>")
	 assert_equal($bdb.rindex(-i), $array.rindex(-i), "<negative rindex #{i}>")
      end
      100.times do
	 aa = []
	 rand(12).times do 
	    aa.push rand($bdb.size)
	 end
	 assert_equal($array.indices(*aa), $bdb.indices(*aa), "<indices>")
      end
      100.times do
	 aa = []
	 rand(12).times do 
	    aa.push(-1 * rand($bdb.size))
	 end
	 assert_equal($array.indices(*aa), $bdb.indices(*aa), "<negative indices #{aa}>")
      end
   end

   def test_09_delete
      assert_equal($bdb.to_a, $array, "<before delete>")
      100.times do
	 i = rand($bdb.size)
	 assert_equal($bdb.delete(i), $array.delete(i), "<delete #{i}>")
	 reinject(20) if $bdb.size < 20
      end
      assert_equal($bdb.to_a, $array, "<after delete>")
      100.times do
	 i = rand($bdb.size)
	 assert_equal($bdb.delete_at(i), $array.delete_at(i), "<delete at #{i}>")
	 reinject(20) if $bdb.size < 20
      end
      assert_equal($bdb.to_a, $array, "<after delete_at>")
      reinject(60) if $bdb.size < 60
      assert_equal($bdb.delete_if { false }, $bdb, "<delete_if false>")
      assert_equal($bdb.to_a, $array, "<after delete_if false>")
      assert_equal($bdb.delete_if { true }, $bdb, "<delete_if true>")
      assert_equal($bdb.size, 0, "<after delete_if true>")
      $bdb.push(*$array)
      assert_equal($bdb.to_a, $array, "<after push>")
      100.times do
	 i = rand($bdb.size)
	 assert_equal($bdb.delete_if {|i| i.to_i > 32}, $bdb, "<delete_if condition>")
	 $array.delete_if {|i| i.to_i > 32}
	 assert_equal($bdb.to_a, $array, "<delete_if condition compare>")
	 reinject(60) if $bdb.size < 60
      end
      reinject(99) if $bdb.size < 99
   end

   def test_10_reject
      assert_equal($bdb.to_a, $array, "<before reject!>")
      assert_equal(nil, $bdb.reject! { false }, "<reject! false>")
   end

   def test_11_clear
      $bdb.clear ; $array.clear
      assert_equal($array.size, $bdb.size, "<size after clear>")
      assert_equal($array, $bdb.to_a, "<after clear>")
   end

   def test_12_fill
      $bdb.fill "32", 0, 99; $array.fill "32", 0, 99
      assert_equal($array.size, $bdb.size, "<size after fill>")
      assert_equal($array, $bdb.to_a, "<after fill>")
      $bdb.fill "42"; $array.fill "42"
      assert_equal($array.size, $bdb.size, "<size after fill>")
      assert_equal($array, $bdb.to_a, "<after fill>")
      10.times do |iter|
	 k = rand($bdb.size).to_s
	 i = rand($bdb.size)
	 assert_equal($bdb.fill(k, i).to_a, $array.fill(k, i), "<#{iter} fill(#{i})>")
	 assert_equal($bdb.size, $array.size, "<size after fill(#{i})  #{iter}>")
	 reinject(20) if $bdb.size < 20
      end
      10.times do |iter|
	 k = rand($bdb.size).to_s
	 i = rand($bdb.size)
	 assert_equal($bdb.fill(k, -i).to_a, $array.fill(k, -i), "<fill(#{-i})>")
	 assert_equal($bdb.size, $array.size, "<size after fill(#{-i}) #{iter}>")
	 reinject(20) if $bdb.size < 20
      end
      10.times do  |iter|
	 k = rand($bdb.size).to_s
	 i = rand($bdb.size)
	 j = rand($bdb.size)
	 assert_equal($bdb.fill(k, i, j).to_a, $array.fill(k, i, j), "<fill(#{i}, #{j})>")
	 assert_equal($bdb.size, $array.size, "<size after fill(#{i}, #{j}) #{iter}>")
	 reinject(20) if $bdb.size < 20
      end
      10.times do |iter|
	 k = rand($bdb.size).to_s
	 i = rand($bdb.size)
	 j = i + rand($bdb.size - i)
	 assert_equal($bdb.fill(k, -i, j).to_a, $array.fill(k, -i, j), "<fill(#{-i}, #{j})>")
	 assert_equal($bdb.size, $array.size, "<size after fill(#{-i}, #{j}) #{iter}>")
	 reinject(20) if $bdb.size < 20
      end      
      reinject(20)
      another = 0
      10.times do |iter|
	 k = rand($bdb.size).to_s
	 i = rand($bdb.size)
	 j = i + rand($bdb.size - i)
	 if ! $array.slice(i .. j)
	    assert_raises(RangeError, "<invalid range>") { $bdb.fill(k, i .. j) }
	    another += 1
	    redo if another < 10
	    another = 0
	    next
         end
	 assert_equal($bdb.fill(k, i .. j).to_a, $array.fill(k, i .. j), "<fill(#{i} .. #{j})>")
	 assert_equal($bdb.size, $array.size, "<size after fill #{iter}>")
	 another = 0
	 reinject(20) if $bdb.size < 20
      end
      another = 0
      10.times do |iter|
	 k = rand($bdb.size).to_s
	 i = rand($bdb.size)
	 j = 1 + rand($bdb.size - i)
	 if ! $array.slice(-i .. -j)
	    assert_raises(RangeError, "<invalid range>") { $bdb.fill(k, -i .. -j) }
	    another += 1
	    redo if another < 10
	    another = 0
	    next
         end
	 assert_equal($bdb.fill(k, -i .. -j).to_a, $array.fill(k, -i .. -j), "<fill(#{-i} .. #{-j})>")
	 assert_equal($bdb.size, $array.size, "<size after fill #{iter}>")
	 another = 0
	 reinject(20) if $bdb.size < 20
      end
      reinject(60) if $bdb.size < 60
      assert_equal($bdb.size, $array.size, "<size end fill>")
      assert_equal($bdb.to_a, $array, "<size end fill>")
   end

   def test_13_include
      $bdb.clear; $array.clear; reinject(99)
      assert_equal($bdb.size, $array.size, "<size end clear>")
      assert_equal($bdb.to_a, $array, "<size end clear>")
      100.times do 
	 k = rand($bdb.size + 20).to_s
	 assert_equal($array.include?(k), $bdb.include?(k), "<include(#{k})>")
      end
   end

   def test_14_replace
      $array.replace(('a' .. 'zz').to_a)
      $bdb.replace(('a' .. 'zz').to_a)
      assert_equal($bdb.size, $array.size, "<size end fill>")
      assert_equal($bdb.to_a, $array, "<size end fill>")
   end  

   def test_15_reopen
      $bdb.close
      assert_kind_of(BDB::Recnum, $bdb = BDB::Recnum.new("tmp/aa", nil, "a"), "<open>")
      assert_equal($bdb.size, $array.size, "<size end reopen>")
      assert_equal($bdb.to_a, $array, "<size end open>")
      $bdb.close
      assert_kind_of(BDB::Recnum, $bdb = BDB::Unknown.new("tmp/aa", nil, "r+"), "<open>")
      assert_equal($bdb.size, $array.size, "<size end reopen Unknown>")
      assert_equal($bdb.to_a, $array, "<size end open Unknown>")
   end
       

      
end

if defined?(RUNIT)
   RUNIT::CUI::TestRunner.run(TestRecnum.suite)
end
