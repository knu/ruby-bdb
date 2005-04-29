#!/usr/bin/ruby
$LOAD_PATH.unshift *%w{../src src tests}
require 'bdb'
require 'runit_'

def clean
   Dir.foreach('tmp') do |x|
      if FileTest.file?("tmp/#{x}")
	 File.unlink("tmp/#{x}")
      end
   end
end

module BDB
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
   end
   def test_01_init
      assert_kind_of(BDB::Btree, $bdb = BDB::Btree.new("tmp/aa", nil, "a", "marshal" => Marshal), "<open>")
   end
   def test_02_get_set
      assert_equal([12, "alpha"], $bdb["alpha"] = [12, "alpha"], "<set value>")
      assert_equal([12, "alpha"], $bdb["alpha"].to_orig, "<retrieve value>")
      assert_equal(nil, $bdb["gamma"] = nil, "<set nil>")
      assert_equal(nil, $bdb["gamma"].to_orig, "<retrieve nil>")
      assert($bdb.key?("alpha"), "<has key>")
      assert_equal(false, $bdb.key?("unknown"), "<has unknown key>")
      assert($bdb.value?(nil), "<has nil>")
      assert($bdb.value?([12, "alpha"]), "<has value>")
      assert_equal(false, $bdb.value?("unknown"), "<has unknown value>")
      assert_equal(false, $bdb.put("alpha", "gamma", BDB::NOOVERWRITE), "<nooverwrite>")
      assert_equal([12, "alpha"], $bdb["alpha"].to_orig, "<must not be changed>")
      assert($bdb.both?("alpha", [12, "alpha"]), "<has both>")
      assert_equal(["alpha", [12, "alpha"]], 
		   $bdb.get("alpha", [12, "alpha"], BDB::GET_BOTH), 
		   "<get both>")
      assert(! $bdb.both?("alpha", "beta"), "<don't has both>")
      assert(! $bdb.get("alpha", "beta", BDB::GET_BOTH), "<don't has both>")
      assert(! $bdb.both?("unknown", "alpha"), "<don't has both>")
      assert_equal([1, 2, [3, 4]], $bdb["array"] = [1, 2, [3, 4]], "<array>")
      assert_equal([1, 2, [3, 4]], $bdb["array"].to_orig, "<retrieve array>")
      assert_equal({"a" => "b"}, $bdb["hash"] = {"a" => "b"}, "<hash>")
      assert_equal({"a" => "b"}, $bdb["hash"].to_orig, "<retrieve hash>")
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
   end
   def test_04_cursor
      cat = Struct.new("Cat", :name, :age, :life)
      array = ["abc", [1, 3], {"aa" => 12}, [2, {"bb" => "cc"}, 4],
	 cat.new("cat", 15, 7)]
      array.each do |x|
	 assert_equal(x, $bdb[x] = x, "<set value>")
      end
      assert(array.size == $bdb.size, "<set count>")
      $bdb.each_value do |x|
	 assert(array.index(x) != nil)
      end
      $bdb.reverse_each_value do |x|
	 assert(array.index(x) != nil)
      end
   end
   def test_05_reopen
      assert_equal(nil, $bdb.close, "<close>")
      assert_kind_of(BDB::Btree, $bdb = BDB::Btree.open("tmp/aa", nil, "w", 
	"set_flags" => BDB::DUP, "marshal" => Marshal),
        "<reopen with DB_DUP>")
      assert_equal(0, $bdb.size, "<must be 0 after reopen>")
   end
   def test_06_dup
      array = [[[0, "a"], [1, "b"], [2, "c"], [3, "d"]],
	 [{"aa" => 0}, {"bb" => 1}, {"cc" => 2}],
	 [["aaa", 12], [12, "bbb"]],
	 ["aaaa"]]
      ind = 0
      array.each do |arr|
	 arr.each do |i|
	    assert_equal(i, $bdb[ind.to_s] = i, "<set dup>")
	 end
	 ind += 1
      end
      if BDB::VERSION_MAJOR > 2 || BDB::VERSION_MINOR >= 6
	 assert_equal(4, $bdb.count("0"), "<count dup 0>")
	 assert_equal(3, $bdb.count("1"), "<count dup 1>")
	 assert_equal(2, $bdb.count("2"), "<count dup 2>")
	 assert_equal(1, $bdb.count("3"), "<count dup 3>")
	 assert_equal(0, $bdb.count("4"), "<count dup 4>")
	 array.size.times do |i|
	    $bdb.get_dup(i.to_s) do |val|
	       assert(array[i].index(val) != nil)
	    end
	 end
	 assert_equal([], $bdb.get_dup("4"), "<get dup 4>")
      end
   end
   def test_07_in_memory
      assert_equal(nil, $bdb.close, "<close>")
      assert_kind_of(BDB::Btree, $bdb = BDB::Btree.open("marshal" => Marshal), "<open in memory>")
      assert_equal(0, $bdb.size, "<must be 0 after reopen>")
   end
   def test_08_in_memory_get_set
      assert_equal([1, 2, [3, 4]], $bdb["array"] = [1, 2, [3, 4]], "<set in memory>")
      assert_equal([1, 2, [3, 4]], $bdb["array"].to_orig, "<get in memory>")
      assert_equal({"a" => "b"}, $bdb["hash"] = {"a" => "b"}, "<set in memory>")
      assert_equal({"a" => "b"}, $bdb["hash"].to_orig, "<get in memory>")
      assert_equal("cc", $bdb["bb"] = "cc", "<set in memory>")
      assert_equal("cc", $bdb["bb"].to_orig, "<get in memory>")
      assert_equal(nil, $bdb.close, "<close>")
   end
   def test_09_modify
      assert_kind_of(BDB::Btree, $bdb = BDB::Btree.open("tmp/aa", nil, "w",
							"marshal" => Marshal),
		     "<reopen RECNUM>")
      array = [1, "a", {"a" => 12}]
      assert_equal(array, $bdb["a"] = array, "<set>")
      arr = $bdb["a"]
      arr.push [1, 2]; array.push [1, 2]
      assert_equal(array, $bdb["a"].to_orig, "<get>")
      $bdb["a"][-1] = 4; array[-1] = 4
      assert_equal(array, $bdb["a"].to_orig, "<get>")
      $bdb["a"][-1] = ["abc", 4]; array[-1] = ["abc", 4]
      assert_equal(array, $bdb["a"].to_orig, "<get>")
      assert_equal(nil, $bdb.close, "<close>")
      clean
   end
   def test_10_recnum
      assert_kind_of(BDB::Btree, $bdb = BDB::Btree.open("tmp/aa", nil, "w",
      "set_flags" => BDB::RECNUM, "marshal" => Marshal), "<reopen RECNUM>")
      assert_equal(0, $bdb.size, "<empty RECNUM>")
      arr = ["A 0", "B 1", "C 2", "D 3", "E 4", "F 5", "G 6"]
      i = 0
      arr.each do |x|
	 x = x.split
	 assert_equal(x, $bdb[i] = x, "<set recno>")
	 i += 1;
      end
      i = 0
      $bdb.each_value do |x|
	 assert_equal(arr[i].split, x, "<each recno>")
	 i += 1
      end
      $bdb.delete(3);
      assert_equal(arr.size - 1, $bdb.size, "<size RECNUM>")
      assert_equal(nil, $bdb[3], "<empty record>")
   end
   def test_11_unknown
      $bdb.close
      $bdb = nil
      assert_kind_of(BDB::Btree, unknown = BDB::Unknown.open("tmp/aa", nil, "r", "marshal" => Marshal), "<unknown>")
      unknown.close
   end
   def test_12_env
      clean
      assert_kind_of(BDB::Env, $env = BDB::Env.open("tmp", BDB::CREATE|BDB::INIT_TRANSACTION))
      assert_kind_of(BDB::Btree, $bdb = BDB::Btree.open("aa", nil, "a", "env" => $env, "marshal" => Marshal), "<open>")
   end
   def test_13_txn_commit
      assert_kind_of(BDB::Txn, txn = $env.begin, "<transaction>")
      assert_kind_of(BDB::Btree, db = txn.assoc($bdb), "<assoc>")
      assert_equal(["a", "b"], db["aa"] = ["a", "b"], "<set>")
      assert_equal({"bb" => "cc"}, db["bb"] = {"bb" => "cc"}, "<set>")
      assert_equal("cc", db["cc"] = "cc", "<set>")
      db["aa"][0] = "e"
      db["aa"][1] = "f"
      assert_equal(["e", "f"], db["aa"].to_orig, "<change array>");
      db["bb"]["bb"] = "dd"
      db["bb"]["cc"] = "ee"
      assert_equal({"bb" => "dd", "cc" => "ee"}, db["bb"].to_orig, "<change hash>");
      assert_equal(3, db.size, "<size in txn>")
      assert(txn.commit, "<commit>")
      assert_equal(3, $bdb.size, "<size after commit>")
      assert_equal(["e", "f"], $bdb["aa"].to_orig, "<change array>");
      assert_equal({"bb" => "dd", "cc" => "ee"}, $bdb["bb"].to_orig, "<change hash>");
   end
   def test_14_txn_abort
      assert_kind_of(BDB::Txn, txn = $env.begin, "<transaction>")
      assert_kind_of(BDB::Btree, db = txn.assoc($bdb), "<assoc>")
      assert_equal("aa", db["aa1"] = "aa", "<set>")
      assert_equal("bb", db["bb1"] = "bb", "<set>")
      assert_equal("cc", db["cc1"] = "cc", "<set>")
      db["aa"].push [1, 2]
      assert_equal(["e", "f", [1, 2]], db["aa"].to_orig, "<change array>");
      db["bb"]["dd"] = [3, 4]
      assert_equal({"bb" => "dd", "cc" => "ee", "dd" => [3, 4]}, db["bb"].to_orig, "<change hash>");
      assert_equal(6, db.size, "<size in txn>")
      assert(txn.abort, "<abort>")
      assert_equal(["e", "f"], $bdb["aa"].to_orig, "<change array>");
      assert_equal({"bb" => "dd", "cc" => "ee"}, $bdb["bb"].to_orig, "<change hash>");
      assert_equal(3, $bdb.size, "<size after abort>")
   end
   def test_15_txn_abort2
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
	    db1["aa"].push({"a" => "c"})
	    assert_equal(["e", "f", {"a" => "c"}], db["aa"].to_orig, "<change array>");
            assert_equal(6, db1.size, "<size in txn>")
            txn.abort
            assert_fail("<after an abort>")
         end
         assert_fail("<after an abort>")
      end
      assert_equal(["e", "f"], $bdb["aa"].to_orig, "<change array>");
      assert_equal(3, $bdb.size, "<size after abort>")
   end
   def test_16_txn_commit2
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
	    db1["aa"].push({"a" => "c"})
	    res = db["aa"].to_orig
	    assert_equal(["e", "f", {"a" => "c"}], res, "<change array>");
            assert_equal(6, db1.size, "<size in txn>")
            txn.commit
            assert_fail("<after an commit>")
         end
         assert_fail("<after an commit>")
      end
      res = $bdb["aa"].to_orig
      assert_equal(["e", "f", {"a" => "c"}], res, "<change array>");
      assert_equal(6, $bdb.size, "<size after commit>")
   end

   def test_17_sh
      val = 'a' .. 'zz'
      assert_equal(nil, $bdb.close, "<close>")
      $bdb = BDB::AZ.open("tmp/aa", nil, "w")
      assert_kind_of(BDB::Btree, $bdb, "<sh>")
      val.each do |l|
	 $bdb[l] = l
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
