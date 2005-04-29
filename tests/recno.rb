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

$bdb, $env = nil, nil
clean

print "\nVERSION of BDB is #{BDB::VERSION}\n"

Inh = defined?(RUNIT) ? RUNIT : Test::Unit

class TestRecno < Inh::TestCase
   def test_00_error
      assert_raises(BDB::Fatal, "invalid name") do
	 BDB::Recno.open("tmp/aa", nil, "env" => 1)
      end
      assert_raises(BDB::Fatal, "invalid name") do
	 BDB::Recno.open("tmp/aa", nil, "env" => 1)
      end
   end
   def test_01_init
      assert_kind_of(BDB::Recno, $bdb = BDB::Recno.new("tmp/aa", nil, "a"), "<open>")
   end
   def test_02_get_set
      assert_equal("alpha", $bdb[1] = "alpha", "<set value>")
      assert_equal("alpha", $bdb[1], "<retrieve value>")
      assert_equal(nil, $bdb[2] = nil, "<set nil>")
      assert_equal(nil, $bdb[2], "<retrieve nil>")
      assert($bdb.key?(1), "<has key>")
      assert_equal(false, $bdb.key?(3), "<has unknown key>")
      assert($bdb.value?("alpha"), "<has value>")
      assert_equal(false, $bdb.value?("unknown"), "<has unknown value>")
      assert_equal(false, $bdb.put(1, "gamma", BDB::NOOVERWRITE), "<nooverwrite>")
      assert_equal("alpha", $bdb[1], "<must not be changed>")
      assert($bdb.both?(1, "alpha"), "<has both>")
      assert(! $bdb.both?(1, "beta"), "<don't has both>")
      assert(! $bdb.both?(3, "alpha"), "<don't has both>")
      assert_equal([1, 2, 3], $bdb[4] = [1, 2, 3], "<array>")
      assert_equal([1, 2, 3].to_s, $bdb[4], "<retrieve array>")
      assert_equal({"a" => "b"}, $bdb[5] = {"a" => "b"}, "<hash>")
      assert_equal({"a" => "b"}.to_s, $bdb[5], "<retrieve hash>")
      assert($bdb.sync, "<sync>")
   end
   def test_03_delete
      size = $bdb.size
      i = 0
      $bdb.each_index do |key|
	 assert_equal($bdb, $bdb.delete(key), "<delete value>")
	 i  += 1
      end
      assert(size == i, "<delete count>")
      assert_equal(0, $bdb.size, "<empty>")
   end
   def test_04_cursor
      array = ["a", "b", "c", "d", "e", "f", "g", "h", "i"]
      i = 1
      array.each do |x|
	 assert_equal(x, $bdb[i] = x, "<set value>")
	 i += 1
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
      $bdb.each_value(4) do |x|
	 arr << x
      end
      assert_equal(array - ["a", "b", "c"], arr.sort, "<order>")
      arr = []
      $bdb.reverse_each_value(7) do |x|
	 arr << x
      end
      assert_equal(array - ["h", "i"], arr.sort, "<reverse order>")
      arr = $bdb.reject {|k, v| k == 5 || v == "i" }
      has = array.reject {|k| k == "e" || k == "i" }
      assert_equal(has, arr.values.sort, "<reject>")
   end
   def test_05_reopen
      assert_equal(nil, $bdb.close, "<close>")
      assert_kind_of(BDB::Recno, $bdb = BDB::Recno.open("tmp/aa", nil, "w", 
	"set_flags" => BDB::RENUMBER,
        "set_array_base" => 0), "<reopen with BDB::RENUMBER>")
      assert_equal(0, $bdb.size, "<must be 0 after reopen>")
   end
   def test_06_renumber
      array = ["a", "b", "c", "d", "e", "f", "g", "h", "i"]
      array.each_index do |x|
	 assert_equal(array[x], $bdb[x] = array[x], "<set value>")
      end
      assert_equal($bdb, $bdb.delete(2), "<delete>")
      assert_equal($bdb, $bdb.delete(5), "<delete>")
      array.delete_at(2)
      array.delete_at(5)
      array.each_index do |x|
	 assert_equal(array[x], $bdb[x], "<retrieve value>")
      end
   end
   def test_07_in_memory
      assert_equal(nil, $bdb.close, "<close>")
      assert_kind_of(BDB::Recno, $bdb = BDB::Recno.open(nil, nil), "<open in memory>")
      assert_equal(0, $bdb.size, "<must be 0 after reopen>")
   end
   def test_08_in_memory_get_set
      assert_equal("aa", $bdb[1] = "aa", "<set in memory>")
      assert_equal("cc", $bdb[1] = "cc", "<set in memory>")
      assert_equal("cc", $bdb[1], "<get in memory>")
      assert_equal(nil, $bdb.close, "<close>")
   end
   def test_09_partial_get
      assert_kind_of(BDB::Recno, $bdb = BDB::Recno.open("tmp/aa", nil, "w"), "<reopen>")
      {
	 2	=> "boat",
	 6      => "house",
	 12	=> "sea",
      }.each do |x, y|
	 assert_equal(y, $bdb[x] = y, "<set>")
      end
      pon, off, len = $bdb.set_partial(0, 2)
      assert_equal(false, pon, "<pon>")
      assert_equal(0, off, "<off>")
      assert_equal(0, len, "<len>")
      assert_equal("bo", $bdb[2], "<partial (0, 2) get red>")
      assert_equal("ho", $bdb[6], "<partial (0, 2) get green>")
      assert_equal("se", $bdb[12], "<partial (0, 2) get blue>")
      pon, off, len = $bdb.set_partial(3, 2)
      assert(pon, "<pon>")
      assert_equal(0, off, "<off>")
      assert_equal(2, len, "<len>")
      assert_equal("t", $bdb[2], "<partial (3, 2) get red>")
      assert_equal("se", $bdb[6], "<partial (3, 2) get green>")
      assert_equal(nil, $bdb[12], "<partial (3, 2) get blue>")
      pon, off, len = $bdb.partial_clear
      assert(pon, "<pon>")
      assert_equal(3, off, "<off>")
      assert_equal(2, len, "<len>")
      assert_equal("boat", $bdb[2], "<partial clear get red>")
      assert_equal("house", $bdb[6], "<partial clear get green>")
      assert_equal("sea", $bdb[12], "<partial clear get blue>")
   end
   def test_10_partial_set
      $bdb.set_partial(0, 2)
      assert_equal("", $bdb[2] = "", "<partial set>")
      assert_equal("AB", $bdb[6] = "AB", "<partial set>")
      assert_equal("XYZ", $bdb[12] = "XYZ", "<partial set>")
      assert_equal("KLM", $bdb[10] = "KLM", "<partial set>")
      pon, off, len = $bdb.clear_partial
      assert(pon, "<pon>")
      assert_equal(0, off, "<off>")
      assert_equal(2, len, "<len>")
      assert_equal("at", $bdb[2], "<partial get>")
      assert_equal("ABuse", $bdb[6], "<partial get>")
      assert_equal("XYZa", $bdb[12], "<partial get>")
      assert_equal("KLM", $bdb[10], "<partial get>")
      pon, off, len = $bdb.set_partial(3, 2)
      assert(!pon, "<pon>")
      assert_equal(0, off, "<off>")
      assert_equal(0, len, "<len>")
      assert_equal("PPP", $bdb[2] = "PPP", "<partial set>")
      assert_equal("Q", $bdb[6] = "Q", "<partial set>")
      assert_equal("XYZ", $bdb[12] = "XYZ", "<partial set>")
      assert_equal("TU", $bdb[10] = "TU", "<partial set>")
      pon, off, len = $bdb.clear_partial
      assert(pon, "<pon>")
      assert_equal(3, off, "<off>")
      assert_equal(2, len, "<len>")
      assert_equal("at\0PPP", $bdb[2], "<partial get>")
      assert_equal("ABuQ", $bdb[6], "<partial get>")
      assert_equal("XYZXYZ", $bdb[12], "<partial get>")
      assert_equal("KLMTU", $bdb[10], "<partial get>")
   end
=begin
   def test_11_unknown
      $bdb.close
      $bdb = nil
      assert_kind_of(BDB::Recno, BDB::Unknown.open("tmp/aa", nil, "r"), "<unknown>")
   end
=end
   def test_12_env
      $bdb.close
      Dir.foreach('tmp') do |x|
	 if FileTest.file?("tmp/#{x}")
	    File.unlink("tmp/#{x}")
	 end
      end
      assert_kind_of(BDB::Env, $env = BDB::Env.open("tmp", BDB::CREATE|BDB::INIT_TRANSACTION))
      assert_kind_of(BDB::Recno, $bdb = BDB::Recno.open("aa", nil, "a", "env" => $env), "<open>")
   end
   def test_13_txn_commit
      assert_kind_of(BDB::Txn, txn = $env.begin, "<transaction>")
      assert_kind_of(BDB::Recno, db = txn.assoc($bdb), "<assoc>")
      assert_equal("aa", db[1] = "aa", "<set>")
      assert_equal("bb", db[2] = "bb", "<set>")
      assert_equal("cc", db[3] = "cc", "<set>")
      assert_equal(3, db.size, "<size in txn>")
      assert(txn.commit, "<commit>")
      assert_equal(3, $bdb.size, "<size after commit>")
   end
   def test_14_txn_abort
      assert_kind_of(BDB::Txn, txn = $env.begin, "<transaction>")
      assert_kind_of(BDB::Recno, db = txn.assoc($bdb), "<assoc>")
      assert_equal("aa", db[4] = "aa", "<set>")
      assert_equal("bb", db[5] = "bb", "<set>")
      assert_equal("cc", db[6] = "cc", "<set>")
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
	 db[4] = "aa"
	 db[5] = "bb"
         assert_equal(5, db.size, "<size in txn>")
         txn.begin(db) do |txn1, db1|
            db1[6] = "cc"
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
	 db[4] = "aa"
	 db[5] = "bb"
         assert_equal(5, db.size, "<size in txn>")
         txn.begin(db) do |txn1, db1|
            db1[6] = "cc"
            assert_equal(6, db1.size, "<size in txn>")
            txn.commit
            assert_fail("<after an commit>")
         end
         assert_fail("<after an commit>")
      end
      assert_equal(6, $bdb.size, "<size after commit>")
   end
   def test_17_file
      begin
	 File.truncate("tmp/bb", 0)
      rescue
	 f = File.open("tmp/bb", "w")
      ensure
	 f.close if f
      end
      assert_kind_of(BDB::Recno, $bdb = BDB::Recno.open("tmp/aa", nil, "w",
	"set_array_base" => 0, "set_re_source" => "tmp/bb"), "<open source>")
      $bdb[0] = "abcd"
      $bdb[1] = "efgh"
      $bdb[4] = "ijkl"
      $bdb.close
      begin
	 lines = File.readlines("tmp/bb").join
      rescue
	 assert_fail("<write source>")
      end
      assert_equal("abcd\nefgh\n\n\nijkl\n", lines, "<concat source>")
   end
   def test_18_file_delim
      File.truncate("tmp/bb", 0)
      assert_kind_of(BDB::Recno, $bdb = BDB::Recno.open("tmp/aa", nil, "w",
	"set_array_base" => 0, "set_re_source" => "tmp/bb",
        "set_re_delim" => "|"), "<open source>")
      $bdb[0] = "abcd"
      $bdb[1] = "efgh"
      $bdb[4] = "ijkl"
      $bdb.close
      begin
	 lines = File.readlines("tmp/bb").join
      rescue
	 assert_fail("<write source>")
      end
      assert_equal("abcd|efgh|||ijkl|", lines, "<concat source>")
   end
   def test_19_file_len
      File.truncate("tmp/bb", 0)
      assert_kind_of(BDB::Recno, $bdb = BDB::Recno.open("tmp/aa", nil, "w",
	"set_array_base" => 0, "set_re_source" => "tmp/bb",
        "set_re_len" => 6), "<open source>")
      $bdb[0] = "abcd"
      $bdb[1] = "efg"
      $bdb[4] = "ij"
      $bdb.close
      begin
	 lines = File.readlines("tmp/bb").join
      rescue
	 assert_fail("<write source>")
      end
      assert_equal("abcd  efg               ij    ", lines, "<concat source>")
   end
   def test_20_file_pad
      File.truncate("tmp/bb", 0)
      assert_kind_of(BDB::Recno, $bdb = BDB::Recno.open("tmp/aa", nil, "w",
	"set_array_base" => 0, 
        "set_re_source" => "tmp/bb",
        "set_re_pad" => "+",
        "set_re_len" => 6), "<open source>")
      $bdb[0] = "abcd"
      $bdb[1] = "efg"
      $bdb[4] = "ij"
      $bdb.close
      begin
	 lines = File.readlines("tmp/bb").join
      rescue
	 assert_fail("<write source>")
      end
      assert_equal("abcd++efg+++++++++++++++ij++++", lines, "<concat source>")
   end
   def test_21_memory
      begin
	 File.truncate("tmp/bb", 0)
      rescue
	 f = File.open("tmp/bb", "w")
      ensure
	 f.close if f
      end
      assert_kind_of(BDB::Recno, $bdb = BDB::Recno.open(nil, nil, "w",
	"set_array_base" => 0, "set_re_source" => "tmp/bb"), "<open source>")
      $bdb[0] = "abcd"
      $bdb[1] = "efgh"
      $bdb[4] = "ijkl"
      $bdb.close
      begin
	 lines = File.readlines("tmp/bb").join
      rescue
	 assert_fail("<write source>")
      end
      assert_equal("abcd\nefgh\n\n\nijkl\n", lines, "<concat source>")
   end
   def test_22_memory_delim
      File.truncate("tmp/bb", 0)
      assert_kind_of(BDB::Recno, $bdb = BDB::Recno.open(nil, nil, "w",
	"set_array_base" => 0, "set_re_source" => "tmp/bb",
        "set_re_delim" => "|"), "<open source>")
      $bdb[0] = "abcd"
      $bdb[1] = "efgh"
      $bdb[4] = "ijkl"
      $bdb.close
      begin
	 lines = File.readlines("tmp/bb").join
      rescue
	 assert_fail("<write source>")
      end
      assert_equal("abcd|efgh|||ijkl|", lines, "<concat source>")
   end
   def test_23_memory_len
      File.truncate("tmp/bb", 0)
      assert_kind_of(BDB::Recno, $bdb = BDB::Recno.open(nil, nil, "w",
	"set_array_base" => 0, "set_re_source" => "tmp/bb",
        "set_re_len" => 6), "<open source>")
      $bdb[0] = "abcd"
      $bdb[1] = "efg"
      $bdb[4] = "ij"
      $bdb.close
      begin
	 lines = File.readlines("tmp/bb").join
      rescue
	 assert_fail("<write source>")
      end
      assert_equal("abcd  efg               ij    ", lines, "<concat source>")
   end
   def test_24_memory_pad
      File.truncate("tmp/bb", 0)
      assert_kind_of(BDB::Recno, $bdb = BDB::Recno.open(nil, nil, "w",
	"set_array_base" => 0, 
        "set_re_source" => "tmp/bb",
        "set_re_pad" => "+",
        "set_re_len" => 6), "<open source>")
      $bdb[0] = "abcd"
      $bdb[1] = "efg"
      $bdb[4] = "ij"
      $bdb.close
      begin
	 lines = File.readlines("tmp/bb").join
      rescue
	 assert_fail("<write source>")
      end
      assert_equal("abcd++efg+++++++++++++++ij++++", lines, "<concat source>")
   end
end

if defined?(RUNIT)
   RUNIT::CUI::TestRunner.run(TestRecno.suite)
end
