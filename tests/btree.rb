#!/usr/bin/ruby
$LOAD_PATH.unshift *%w{../src src tests}
require 'bdb'
require 'runit_'

$VERBOSE = nil

module BDB
   class BTCompare < Btree
      def bdb_bt_compare(a, b)
	 a <=> b
      end
   end

   class AZ < Btree
      def bdb_store_key(a)
	 "xx_" + a
      end
      def bdb_fetch_key(a)
	 a.sub(/^xx_/, '')
      end
      def bdb_store_value(a)
	 "yy_" + a
      end
      def bdb_fetch_value(a)
	 a.sub(/^yy_/, '')
      end
   end
end

def clean
   Dir.foreach('tmp') do |x|
      if FileTest.file?("tmp/#{x}")
	 File.unlink("tmp/#{x}")
      end
   end
end

$bdb, $env = nil, nil
clean

print "\nVERSION of BDB is #{BDB::VERSION}\n"

Inh = defined?(RUNIT) ? RUNIT : Test::Unit

class TestBtree < Inh::TestCase
   def test_00_error
      assert_raises(BDB::Fatal, "invalid name") do
	 BDB::Btree.new(".", nil, "a")
      end
      assert_raises(BDB::Fatal, "invalid Env") do
	 BDB::Btree.open("tmp/aa", nil, "env" => 1)
      end
      assert_raises(TypeError) { BDB::Btree.new("tmp/aa", nil, "set_cachesize" => "a") }
      assert_raises(TypeError) { BDB::Btree.new("tmp/aa", nil, "set_pagesize" => "a") }
      assert_raises(TypeError) { BDB::Btree.new("tmp/aa", nil, "set_bt_minkey" => "a") }
      assert_raises(BDB::Fatal) { BDB::Btree.new("tmp/aa", nil, "set_bt_compare" => "a") }
      assert_raises(BDB::Fatal) { BDB::Btree.new("tmp/aa", nil, "set_bt_prefix" => "a") }
      assert_raises(BDB::Fatal) { BDB::Btree.new("tmp/aa", nil, "set_fetch_key" => "a") }
      assert_raises(BDB::Fatal) { BDB::Btree.new("tmp/aa", nil, "set_store_key" => "a") }
      assert_raises(BDB::Fatal) { BDB::Btree.new("tmp/aa", nil, "set_fetch_value" => "a") }
      assert_raises(BDB::Fatal) { BDB::Btree.new("tmp/aa", nil, "set_store_value" => "a") }
      assert_raises(TypeError) { BDB::Btree.new("tmp/aa", nil, "set_lorder" => "a") }
   end
   def test_01_init
      assert_kind_of(BDB::Btree, $bdb = BDB::Btree.new("tmp/aa", nil, "a"), "<open>")
   end
   def test_02_get_set
      assert_equal(true, $bdb.empty?, "<empty>")
      assert_equal("alpha", $bdb["alpha"] = "alpha", "<set value>")
      assert_equal("alpha", $bdb["alpha"], "<retrieve value>")
      assert_equal(nil, $bdb["gamma"] = nil, "<set nil>")
      assert_equal(nil, $bdb["gamma"], "<retrieve nil>")
      assert($bdb.key?("alpha"), "<has key>")
      assert_equal(false, $bdb.key?("unknown"), "<has unknown key>")
      assert($bdb.value?("alpha"), "<has value>")
      assert_equal(false, $bdb.value?("unknown"), "<has unknown value>")
      assert_equal(false, $bdb.put("alpha", "gamma", BDB::NOOVERWRITE), "<nooverwrite>")
      assert_equal("beta", $bdb.fetch("xxx", "beta"), "<fetch nil>")
      assert_equal("xxx", $bdb.fetch("xxx") {|x| x}, "<fetch nil>")
      assert_raises(IndexError) { $bdb.fetch("xxx") }
      assert_raises(ArgumentError) { $bdb.fetch("xxx", "beta") {} }
      assert_equal("alpha", $bdb["alpha"], "<must not be changed>")
      assert($bdb.both?("alpha", "alpha"), "<has both>")
      assert_equal(["alpha", "alpha"], 
		   $bdb.get("alpha", "alpha", BDB::GET_BOTH), 
		   "<get both>")
      assert(! $bdb.both?("alpha", "beta"), "<don't has both>")
      assert(! $bdb.get("alpha", "beta", BDB::GET_BOTH), "<don't has both>")
      assert(! $bdb.both?("unknown", "alpha"), "<don't has both>")
      assert_equal([1, 2, 3], $bdb["array"] = [1, 2, 3], "<array>")
      assert_equal([1, 2, 3].to_s, $bdb["array"], "<retrieve array>")
      assert_equal({"a" => "b"}, $bdb["hash"] = {"a" => "b"}, "<hash>")
      assert_equal({"a" => "b"}.to_s, $bdb["hash"], "<retrieve hash>")
      assert($bdb.sync, "<sync>")
   end

   def test_03_delete
      size = $bdb.size
      i = 0
      $bdb.each do |key, value|
	 assert_equal($bdb, $bdb.delete(key), "<delete value>")
	 i += 1
      end
      assert(size == i, "<delete count>")
      assert_equal(0, $bdb.size, "<empty>")
      $hash = {}
      (33 .. 126).each do |i|
	 key = i.to_s * 5
	 $bdb[key] = i.to_s * 7
	 $hash[key] = i.to_s * 7
	 assert_equal($bdb[key], $hash[key], "<set #{key}>")
      end
      assert_equal($bdb.size, $hash.size, "<size after load>")
      assert_raises(ArgumentError) { $bdb.select("xxx") {}}
      assert_equal([], $bdb.select { false }, "<select none>")
      assert_equal($hash.sort, $bdb.select { true }.sort, "<select all>")
      arr0 = []
      $bdb.each_key {|key| arr0 << key }
      assert_equal($hash.keys.sort, arr0.sort, "<each key>")
      arr1 = []
      $bdb.reverse_each_key {|key| arr1 << key }
      assert_equal($hash.keys.sort, arr1.sort, "<reverse each key>")
      assert_equal(arr0, arr1.reverse, "<reverse>")
      assert_equal($hash.invert, $bdb.invert, "<invert>")
      $bdb.each do |key, value|
         if rand > 0.5
            assert_equal($bdb, $bdb.delete(key), "<delete value>")
            $hash.delete(key)
         end
      end
      assert_equal($bdb.size, $hash.size, "<size after load>")
      $bdb.each do |key, value|
	 assert_equal($hash[key], value, "<after delete>")
      end
      $hash.each do |key, value|
	 assert($bdb.key?(key), "<key after delete>")
	 assert_equal($bdb, $bdb.delete(key), "<delete value>")
      end
   end

   def test_04_cursor
      array = ["a", "b", "c", "d", "e", "f", "g", "h", "i"]
      array.each do |x|
	 assert_equal(x, $bdb[x] = x, "<set value>")
      end
      assert(array.size == $bdb.size, "<set count>")
      arr = []
      $bdb.each_value do |x|
	 arr << x
      end
      assert_equal(array, arr.sort, "<order>")
      arr = []
      $bdb.reverse_each_value do |x|
	 arr << x
      end
      assert_equal(array, arr.sort, "<reverse order>")
      arr = []
      $bdb.each_value("d") do |x|
	 arr << x
      end
      assert_equal(array - ["a", "b", "c"], arr.sort, "<order>")
      arr = []
      $bdb.reverse_each_value("g") do |x|
	 arr << x
      end
      assert_equal(array - ["h", "i"], arr.sort, "<reverse order>")
      arr = $bdb.reject {|k, v| k == "e" || v == "i" }
      has = array.reject {|k, v| k == "e" || k == "i" }
      assert_equal(has, arr.keys.sort, "<reject>")
      $bdb.reject! {|k, v| k == "e" || v == "i" }
      array.reject! {|k, v| k == "e" || k == "i" }
      assert_equal(array, $bdb.keys.sort, "<keys after reject>")
      assert_equal(array, $bdb.values.sort, "<values after reject>")
   end

   def test_05_reopen
      assert_equal(nil, $bdb.close, "<close>")
      assert_kind_of(BDB::Btree, $bdb = BDB::Btree.open("tmp/aa", nil, "w", 
	"set_flags" => BDB::DUP,
	"set_dup_compare" => lambda {|a, b| a <=> b}),
        "<reopen with DB_DUP>")
      assert_equal(0, $bdb.size, "<must be 0 after reopen>")
   end

   def test_06_dup
      assert_equal("a", $bdb["0"] = "a", "<set dup>")
      assert_equal("b", $bdb["0"] = "b", "<set dup>")
      assert_equal("c", $bdb["0"] = "c", "<set dup>")
      assert_equal("d", $bdb["0"] = "d", "<set dup>")
      assert_equal("aa", $bdb["1"] = "aa", "<set dup>")
      assert_equal("bb", $bdb["1"] = "bb", "<set dup>")
      assert_equal("cc", $bdb["1"] = "cc", "<set dup>")
      assert_equal("aaa", $bdb["2"] = "aaa", "<set dup>")
      assert_equal("bbb", $bdb["2"] = "bbb", "<set dup>")
      assert_equal("aaaa", $bdb["3"] = "aaaa", "<set dup>")
      rep = [["a", "b", "c", "d"], ['aa', 'bb', 'cc'], ['aaa', 'bbb'], ['aaaa']]
      if BDB::VERSION_MAJOR > 2 || BDB::VERSION_MINOR >= 6
	 for i in [0, 1, 2, 3]
	    k0, v0 = [], []
	    $bdb.duplicates(i.to_s, false).each {|v| v0 << v}
	    assert_equal(v0.sort, rep[i], "<dup val #{i}>")
	    k0, v0 = [], []
	    $bdb.each_dup(i.to_s) {|k, v| k0 << k; v0 << v}
	    assert_equal(k0, [i.to_s] * (4 - i), "<dup key #{i}>")
	    assert_equal(v0.sort, rep[i], "<dup val #{i}>")
	    v0 = []
	    $bdb.each_dup_value(i.to_s) {|v|  v0 << v}
	    assert_equal(v0.sort, rep[i], "<dup val #{i}>")
	 end
	 assert_equal(4, $bdb.count("0"), "<count dup 0>")
	 assert_equal(3, $bdb.count("1"), "<count dup 1>")
	 assert_equal(2, $bdb.count("2"), "<count dup 2>")
	 assert_equal(1, $bdb.count("3"), "<count dup 3>")
	 assert_equal(0, $bdb.count("4"), "<count dup 4>")
	 assert_equal(["a", "b", "c", "d"], $bdb.get_dup("0").sort, "<get dup 0>")
	 assert_equal(["aa", "bb", "cc"], $bdb.get_dup("1").sort, "<get dup 1>")
	 assert_equal(["aaa", "bbb"], $bdb.get_dup("2").sort, "<get dup 2>")
	 assert_equal(["aaaa"], $bdb.get_dup("3"), "<get dup 3>")
	 assert_equal([], $bdb.get_dup("4"), "<get dup 4>")
      end
   end

   def test_07_in_memory
      assert_equal(nil, $bdb.close, "<close>")
      assert_kind_of(BDB::Btree, $bdb = BDB::Btree.open, "<open in memory>")
      assert_equal(0, $bdb.size, "<must be 0 after reopen>")
   end

   def test_08_in_memory_get_set
      assert_equal("aa", $bdb["bb"] = "aa", "<set in memory>")
      assert_equal("cc", $bdb["bb"] = "cc", "<set in memory>")
      assert_equal("cc", $bdb["bb"], "<get in memory>")
      assert_equal(nil, $bdb.close, "<close>")
   end

   def test_09_partial_get
      assert_kind_of(BDB::Btree, $bdb = BDB::Btree.open("tmp/aa", nil, "w"), "<reopen>")
      {
	 "red"	=> "boat",
	 "green"=> "house",
	 "blue"	=> "sea",
      }.each do |x, y|
	 assert_equal(y, $bdb[x] = y, "<set>")
      end
      pon, off, len = $bdb.set_partial(0, 2)
      assert_equal(false, pon, "<pon>")
      assert_equal(0, off, "<off>")
      assert_equal(0, len, "<len>")
      assert_equal("bo", $bdb["red"], "<partial (0, 2) get red>")
      assert_equal("ho", $bdb["green"], "<partial (0, 2) get green>")
      assert_equal("se", $bdb["blue"], "<partial (0, 2) get blue>")
      pon, off, len = $bdb.set_partial(3, 2)
      assert(pon, "<pon>")
      assert_equal(0, off, "<off>")
      assert_equal(2, len, "<len>")
      assert_equal("t", $bdb["red"], "<partial (3, 2) get red>")
      assert_equal("se", $bdb["green"], "<partial (3, 2) get green>")
      assert_equal(nil, $bdb["blue"], "<partial (3, 2) get blue>")
      pon, off, len = $bdb.partial_clear
      assert(pon, "<pon>")
      assert_equal(3, off, "<off>")
      assert_equal(2, len, "<len>")
      assert_equal("boat", $bdb["red"], "<partial clear get red>")
      assert_equal("house", $bdb["green"], "<partial clear get green>")
      assert_equal("sea", $bdb["blue"], "<partial clear get blue>")
   end

   def test_10_partial_set
      $bdb.set_partial(0, 2)
      assert_equal("", $bdb["red"] = "", "<partial set>")
      assert_equal("AB", $bdb["green"] = "AB", "<partial set>")
      assert_equal("XYZ", $bdb["blue"] = "XYZ", "<partial set>")
      assert_equal("KLM", $bdb["yellow"] = "KLM", "<partial set>")
      pon, off, len = $bdb.clear_partial
      assert(pon, "<pon>")
      assert_equal(0, off, "<off>")
      assert_equal(2, len, "<len>")
      assert_equal("at", $bdb["red"], "<partial get>")
      assert_equal("ABuse", $bdb["green"], "<partial get>")
      assert_equal("XYZa", $bdb["blue"], "<partial get>")
      assert_equal("KLM", $bdb["yellow"], "<partial get>")
      pon, off, len = $bdb.set_partial(3, 2)
      assert(!pon, "<pon>")
      assert_equal(0, off, "<off>")
      assert_equal(0, len, "<len>")
      assert_equal("PPP", $bdb["red"] = "PPP", "<partial set>")
      assert_equal("Q", $bdb["green"] = "Q", "<partial set>")
      assert_equal("XYZ", $bdb["blue"] = "XYZ", "<partial set>")
      assert_equal("TU", $bdb["yellow"] = "TU", "<partial set>")
      pon, off, len = $bdb.clear_partial
      assert(pon, "<pon>")
      assert_equal(3, off, "<off>")
      assert_equal(2, len, "<len>")
      assert_equal("at\0PPP", $bdb["red"], "<partial get>")
      assert_equal("ABuQ", $bdb["green"], "<partial get>")
      assert_equal("XYZXYZ", $bdb["blue"], "<partial get>")
      assert_equal("KLMTU", $bdb["yellow"], "<partial get>")
      $bdb.close
   end

   def test_11_recnum
      assert_kind_of(BDB::Btree, $bdb = BDB::Btree.open("tmp/aa", nil, "w",
      "set_flags" => BDB::RECNUM), "<reopen RECNUM>")
      assert_equal(0, $bdb.size, "<empty RECNUM>")
      arr = ["A 0", "B 1", "C 2", "D 3", "E 4", "F 5", "G 6"]
      i = 0
      arr.each do |x|
	 assert_equal(x, $bdb[i] = x, "<set recno>")
	 i += 1;
      end
      i = 0
      $bdb.each_value do |x|
	 assert_equal(arr[i], x, "<each recno>")
	 i += 1
      end
      $bdb.delete(3);
      assert_equal(arr.size - 1, $bdb.size, "<size RECNUM>")
      assert_equal(nil, $bdb[3], "<empty record>")
      $bdb.close
   end

   def test_12_unknown
      $bdb = nil
      assert_kind_of(BDB::Btree, unknown = BDB::Unknown.open("tmp/aa", nil, "r"), "<unknown>")
      unknown.close
   end

   def test_13_env
      clean
      assert_kind_of(BDB::Env, $env = BDB::Env.open("tmp", BDB::CREATE|BDB::INIT_TRANSACTION))
      assert_kind_of(BDB::Btree, $bdb = BDB::Btree.open("aa", nil, "a", "env" => $env), "<open>")
   end

   def test_14_txn_commit
      assert_kind_of(BDB::Txn, txn = $env.begin, "<transaction>")
      assert_kind_of(BDB::Btree, db = txn.assoc($bdb), "<assoc>")
      assert_equal("aa", db["aa"] = "aa", "<set>")
      assert_equal("bb", db["bb"] = "bb", "<set>")
      assert_equal("cc", db["cc"] = "cc", "<set>")
      assert_equal(3, db.size, "<size in txn>")
      assert(txn.commit, "<commit>")
      assert_equal(3, $bdb.size, "<size after commit>")
   end

   def test_15_txn_abort
      assert_kind_of(BDB::Txn, txn = $env.begin, "<transaction>")
      assert_kind_of(BDB::Btree, db = txn.assoc($bdb), "<assoc>")
      assert_equal("aa", db["aa1"] = "aa", "<set>")
      assert_equal("bb", db["bb1"] = "bb", "<set>")
      assert_equal("cc", db["cc1"] = "cc", "<set>")
      assert_equal(6, db.size, "<size in txn>")
      assert(txn.abort, "<abort>")
      assert_equal(3, $bdb.size, "<size after abort>")
   end

=begin
   def test_16_txn_abort2
      if BDB::VERSION_MAJOR == 2 && BDB::VERSION_MINOR < 7
	 $stderr.print "skipping test for this version"
	 return "pass"
      end
      $env.begin($bdb) do |txn, db|
	 db["aa1"] = "aa"
	 db["bb1"] = "bb"
         assert_equal(5, db.size, "<size in txn>")
         txn.begin(db) do |txn1, db1|
            db1["cc1"] = "cc"
            assert_equal(6, db1.size, "<size in txn>")
            txn.abort
            assert_fail("<after an abort>")
         end
         assert_fail("<after an abort>")
      end
      assert_equal(3, $bdb.size, "<size after abort>")
   end

   def test_17_txn_commit2
      if BDB::VERSION_MAJOR == 2 && BDB::VERSION_MINOR < 7
	 $stderr.print "skipping test for this version"
	 return "pass"
      end
      $env.begin($bdb) do |txn, db|
	 db["aa1"] = "aa"
         db["bb1"] = "bb"
         assert_equal(5, db.size, "<size in txn>")
         txn.begin(db) do |txn1, db1|
            db1["cc1"] = "cc"
            assert_equal(6, db1.size, "<size in txn>")
            txn.commit
            assert_fail("<after an commit>")
         end
         assert_fail("<after an commit>")
      end
      assert_equal(6, $bdb.size, "<size after commit>")
      assert_equal(nil, $bdb.close, "<close>")
      $env.close
   end
=end

   def intern_btree_delete
      $hash = {}
      File.foreach("examples/wordtest") do |line|
	 line.chomp!
	 $hash[line] = line.reverse
	 $bdb[line] = line.reverse
      end
      $bdb.each do |k, v|
	 assert_equal($hash[k], v, "<value>")
      end
      $bdb.delete_if {|k, v| k[0] == ?a}
      $hash.delete_if {|k, v| k[0] == ?a}
      assert_equal($bdb.size, $hash.size, "<size after delete_if>")
      $bdb.each do |k, v|
	 assert_equal($hash[k], v, "<value>")
      end
   end

   def test_18_btree_delete
      clean
      assert_kind_of(BDB::BTCompare, 
		     $bdb = BDB::BTCompare.open("tmp/aa", nil, "w", 
					       "set_pagesize" => 1024,
					       "set_cachesize" => [0, 32 * 1024, 0]),
		     "<open>")
      intern_btree_delete
      $bdb.close
   end

   def test_19_btree_delete
      clean
      assert_kind_of(BDB::Btree, 
		     $bdb = BDB::Btree.open("tmp/aa", nil, "w", 
					    "set_bt_compare" => proc {|a, b| a <=> b},
					    "set_pagesize" => 1024,
					    "set_cachesize" => [0, 32 * 1024, 0]),
		     "<open>")
      intern_btree_delete
   end

   def test_20_index
      lines = $hash.keys
      array = []
      10.times do
	 h = lines[rand(lines.size - 1)]
	 array.push h
	 assert_equal($hash.index(h.reverse), $bdb.index(h.reverse), "<index>")
      end
      assert_equal($hash.indexes(array), $bdb.indexes(array), "<indexes>")
      array.each do |k|
	 assert($bdb.has_both?(k, k.reverse), "<has both>")
      end
   end

   def test_21_convert
      h = $bdb.to_hash
      h.each do |k, v|
	 assert_equal(v, $hash[k], "<to_hash>")
      end
      $hash.each do |k, v|
	 assert_equal(v, h[k], "<to_hash>")
      end
      h1 = $hash.to_a
      h2 = $bdb.to_a
      assert_equal(h1.size, h2.size, "<equal>")
      h1.each do |k|
	 assert(h2.include?(k), "<include>")
      end
   end

   if BDB::VERSION_MAJOR == 3 && BDB::VERSION_MINOR >= 3
      def test_22_blurb
	 $bdb.each(nil, 10) do |k, v|
	    assert_equal($hash[k], v, "<value>")
	 end
	 $bdb.each_key(nil, 10) do |k|
	    assert($hash.key?(k), "<key>")
	 end
	 $hash.each_key do |k|
	    assert(!!$bdb.key?(k), "<key>")
	 end
      end
   end

   def test_23_sh
      val = 'a' .. 'zz'
      assert_equal(nil, $bdb.close, "<close>")
      File::unlink('tmp/aa') if FileTest.file?('tmp/aa')
      assert_kind_of(BDB::Btree, $bdb = BDB::AZ.open("tmp/aa", nil, "w"), "<sh>")
      val.each do |l|
	 assert_equal(l, $bdb[l] = l, "<store>")
      end
      $bdb.each do |k, v|
	 assert_equal(k, v, "<fetch>")
      end
      assert_equal(nil, $bdb.close, "<close>")
      assert_kind_of(BDB::Btree, $bdb = BDB::Btree.open("tmp/aa"), "<sh>")
      val.each do |l|
	 assert_equal("yy_#{l}", $bdb["xx_#{l}"], "<fetch value>")
      end
      $bdb.each do |k, v|
	 assert_equal("xx_", k[0, 3], "<fetch key>")
	 assert_equal("yy_", v[0, 3], "<fetch key>")
      end
      assert_equal(nil, $bdb.close, "<close>")
      clean
   end

end

if defined?(RUNIT)
   RUNIT::CUI::TestRunner.run(TestBtree.suite)
end

