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

$env = nil
clean

print "\nVERSION of BDB is #{BDB::VERSION}\n"

class TestLog < RUNIT::TestCase
   def test_01_init
      assert_kind_of(BDB::Env, $env = BDB::Env.open("tmp", BDB::CREATE | BDB::INIT_LOG, "thread" => false) , "<env open>")
   end

   def test_02_push
      $lsn = []
      100.times do |i|
	 assert_kind_of(BDB::Lsn, lsn = $env.log_put("test #{i}"), "<lsn>")
	 $lsn.push lsn
      end
      assert_equal(100, $lsn.size, "lsn size")
   end

   def test_03_each
      i = 0
      $env.log_each do |r, l|
	 assert_equal($lsn[i], l, "lsn ==")
	 assert_equal("test #{i}", r, "value ==")
	 i += 1
      end
   end

   def test_04_reverse_each
      i = 99
      $env.log_reverse_each do |r, l|
	 assert_equal($lsn[i], l, "lsn == reverse")
	 assert_equal("test #{i}", r, "value == reverse")
	 i -= 1
      end
   end

   def test_05_random
      1000.times do
	 nb = rand($lsn.size)
	 assert_equal($lsn[nb].log_get,"test #{nb}", "<log_get>")
      end
      $env.close
   end

   def test_06_reinit
      clean
      assert_kind_of(BDB::Env, 
		     $env = BDB::Env.open("tmp",  BDB::CREATE | BDB::INIT_LOG,
					  "thread" => false, 
					  "set_lg_bsize" => 10000,
					  "set_lg_max" => 45000),
		     "<open>")
   end

   def test_07_put
      $lsn = []
      $rec = []
      500.times do |i|
	 assert_kind_of(BDB::Lsn, j = $env.log_put("init %d" % i), "<put>")
	 if (i % 25) == 0
	    $rec.push "init %d" % i
	    $lsn.push j
	 end
      end
   end

   def test_08_log_get
      l = nil
      $lsn.each_with_index do |ls, i|
	 if l
	    assert(ls > l, "<compare>")
	 end
	 assert_equal(ls.log_get, $rec[i], "<equal>")
	 l = ls
      end
   end

   def test_09_file
      log_file = Dir.glob("tmp/log*")
      $lsn.each do |ls|
	 assert(log_file.include?(ls.log_file), "<include>")
      end
   end

   def test_10_end
      $env.close
      clean
   end

end

RUNIT::CUI::TestRunner.run(TestLog.suite)
