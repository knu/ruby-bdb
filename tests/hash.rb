#!/usr/bin/ruby -I.. -Itests
require 'bdb'
require 'runit_'

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

class TestHash < RUNIT::TestCase
   def test_00_error
      assert_error(BDB::Fatal, 'BDB::Hash.new(".", nil, "a")', "invalid name")
      assert_error(BDB::Fatal, 'BDB::Hash.open("tmp/aa", nil, "env" => 1)', "invalid Env")
   end
   def test_01_init
      assert_kind_of(BDB::Hash, $bdb = BDB::Hash.new("tmp/aa", nil, "a"), "<open>")
   end
   def test_02_get_set
      assert_equal("alpha", $bdb["alpha"] = "alpha", "<set value>")
      assert_equal("alpha", $bdb["alpha"], "<retrieve value>")
      assert_equal(nil, $bdb["gamma"] = nil, "<set nil>")
      assert_equal(nil, $bdb["gamma"], "<retrieve nil>")
      assert($bdb.key?("alpha"), "<has key>")
      assert_equal(false, $bdb.key?("unknown"), "<has unknown key>")
      assert($bdb.value?(nil), "<has nil>")
      assert($bdb.value?("alpha"), "<has value>")
      assert_equal(false, $bdb.value?("unknown"), "<has unknown value>")
      assert_equal(false, $bdb.put("alpha", "gamma", BDB::NOOVERWRITE), "<nooverwrite>")
      assert_equal("alpha", $bdb["alpha"], "<must not be changed>")
      assert($bdb.both?("alpha", "alpha"), "<has both>")
      assert(! $bdb.both?("alpha", "beta"), "<don't has both>")
      assert(! $bdb.both?("unknown", "alpha"), "<don't has both>")
      assert_equal([1, 2, 3].to_s, $bdb["array"] = [1, 2, 3], "<array>")
      assert_equal([1, 2, 3].to_s, $bdb["array"], "<retrieve array>")
      assert_equal({"a" => "b"}.to_s, $bdb["hash"] = {"a" => "b"}, "<hash>")
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
   end
   def test_05_reopen
      assert_equal(nil, $bdb.close, "<close>")
      assert_kind_of(BDB::Hash, $bdb = BDB::Hash.open("tmp/aa", nil, "w", 
	"set_flags" => BDB::DUP,
	"set_dup_compare" => lambda {|a, b| b <=> a }), 
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
      if BDB::VERSION_MAJOR < 3 && BDB::VERSION_MINOR < 6
	 assert_error(BDB::Fatal, '$bdb.count("0")', "<count dup 0>")
	 assert_error(BDB::Fatal, '$bdb.dup("0")', "<dup 0 must fail>")
      else
	 assert_equal(4, $bdb.count("0"), "<count dup 0>")
	 assert_equal(3, $bdb.count("1"), "<count dup 1>")
	 assert_equal(2, $bdb.count("2"), "<count dup 2>")
	 assert_equal(1, $bdb.count("3"), "<count dup 3>")
	 assert_equal(0, $bdb.count("4"), "<count dup 4>")
	 assert_equal(["a", "b", "c", "d"], $bdb.dup("0").sort, "<get dup 0>")
	 assert_equal(["aa", "bb", "cc"], $bdb.dup("1").sort, "<get dup 1>")
	 assert_equal(["aaa", "bbb"], $bdb.dup("2").sort, "<get dup 2>")
	 assert_equal(["aaaa"], $bdb.dup("3"), "<get dup 3>")
	 assert_equal([], $bdb.dup("4"), "<get dup 4>")
      end
   end
   def test_07_in_memory
      assert_equal(nil, $bdb.close, "<close>")
      assert_kind_of(BDB::Hash, $bdb = BDB::Hash.open(nil, nil), "<open in memory>")
      assert_equal(0, $bdb.size, "<must be 0 after reopen>")
   end
   def test_08_in_memory_get_set
      assert_equal("aa", $bdb["bb"] = "aa", "<set in memory>")
      assert_equal("cc", $bdb["bb"] = "cc", "<set in memory>")
      assert_equal("cc", $bdb["bb"], "<get in memory>")
      assert_equal(nil, $bdb.close, "<close>")
   end
   def test_09_partial_get
      assert_kind_of(BDB::Hash, $bdb = BDB::Hash.open("tmp/aa", nil, "w"), "<reopen>")
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
      assert_equal("", $bdb["blue"], "<partial (3, 2) get blue>")
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
      assert_equal("at", $bdb["red"] = "", "<partial set>")
      assert_equal("AB", $bdb["green"] = "AB", "<partial set>")
      assert_equal("XY", $bdb["blue"] = "XYZ", "<partial set>")
      assert_equal("KL", $bdb["yellow"] = "KLM", "<partial set>")
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
      assert_equal("PP", $bdb["red"] = "PPP", "<partial set>")
      assert_equal("Q", $bdb["green"] = "Q", "<partial set>")
      assert_equal("XY", $bdb["blue"] = "XYZ", "<partial set>")
      assert_equal("TU", $bdb["yellow"] = "TU", "<partial set>")
      pon, off, len = $bdb.clear_partial
      assert(pon, "<pon>")
      assert_equal(3, off, "<off>")
      assert_equal(2, len, "<len>")
      assert_equal("at\0PPP", $bdb["red"], "<partial get>")
      assert_equal("ABuQ", $bdb["green"], "<partial get>")
      assert_equal("XYZXYZ", $bdb["blue"], "<partial get>")
      assert_equal("KLMTU", $bdb["yellow"], "<partial get>")
   end
   def test_11_unknown
      $bdb.close
      $bdb = nil
      assert_kind_of(BDB::Hash, BDB::Unknown.open("tmp/aa", nil, "r"), "<unknown>")
   end
   def test_12_env
      clean
      assert_kind_of(BDB::Env, $env = BDB::Env.open("tmp", BDB::CREATE|BDB::INIT_TRANSACTION))
      assert_kind_of(BDB::Hash, $bdb = BDB::Hash.open("aa", nil, "a", "env" => $env), "<open>")
   end
   def test_13_txn_commit
      assert_kind_of(BDB::Txn, txn = $env.begin, "<transaction>")
      assert_kind_of(BDB::Hash, db = txn.assoc($bdb), "<assoc>")
      assert_equal("aa", db["aa"] = "aa", "<set>")
      assert_equal("bb", db["bb"] = "bb", "<set>")
      assert_equal("cc", db["cc"] = "cc", "<set>")
      assert_equal(3, db.size, "<size in txn>")
      assert(txn.commit, "<commit>")
      assert_equal(3, $bdb.size, "<size after commit>")
   end
   def test_14_txn_abort
      assert_kind_of(BDB::Txn, txn = $env.begin, "<transaction>")
      assert_kind_of(BDB::Hash, db = txn.assoc($bdb), "<assoc>")
      assert_equal("aa", db["aa1"] = "aa", "<set>")
      assert_equal("bb", db["bb1"] = "bb", "<set>")
      assert_equal("cc", db["cc1"] = "cc", "<set>")
      assert_equal(6, db.size, "<size in txn>")
      assert(txn.abort, "<abort>")
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
            assert_equal(6, db1.size, "<size in txn>")
            txn.abort
            assert_fail("<after an abort>")
         end
         assert_fail("<after an abort>")
      end
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
            assert_equal(6, db1.size, "<size in txn>")
            txn.commit
            assert_fail("<after an commit>")
         end
         assert_fail("<after an commit>")
      end
      assert_equal(6, $bdb.size, "<size after commit>")
   end
end

RUNIT::CUI::TestRunner.run(TestHash.suite)
