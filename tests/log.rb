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

$env = nil
clean

print "\nVERSION of BDB is #{BDB::VERSION}\n"

Inh = defined?(RUNIT) ? RUNIT : Test::Unit

class TestLog < Inh::TestCase
   def test_01_init
      assert_kind_of(BDB::Env, $env = BDB::Env.open("tmp", BDB::CREATE | BDB::INIT_LOG, "thread" => false) , "<env open>")
   end

   def test_02_push
      $lsn = []
      100.times do |i|
	 assert_kind_of(BDB::Lsn, lsn = $env.log_put("test #{i}"), "<lsn>")
	 $lsn.push lsn
      end
      assert_equal($env, $env.log_flush, "<log flush>")
      assert_equal(100, $lsn.size, "lsn size")
      assert_equal($env, $lsn[0].env, "<env>")
      log = Dir["tmp/log*"][0].sub(/\Atmp./,'')
      assert_equal([log], $env.log_archive(BDB::ARCH_LOG), "<log archive>")
      assert_equal("tmp/#{log}", $lsn[0].file, "<log file>")
      assert_equal($lsn[0], $lsn[0].flush, "<lsn flush>")
   end

   def test_03_each
      i = 0
      $env.log_each do |r, l|
	 assert_equal($lsn[i], l, "lsn ==")
	 assert_equal("test #{i}", r, "value ==")
	 i += 1
      end
      assert_equal(100, i, "<end each>")
   end

   def test_04_reverse_each
      i = 99
      $env.log_reverse_each do |r, l|
	 assert_equal($lsn[i], l, "lsn == reverse")
	 assert_equal("test #{i}", r, "value == reverse")
	 i -= 1
      end
      assert_equal(-1, i, "<end reverse each>")
   end
 
   def test_05_each
      if $lsn[0].respond_to?(:each)
	 i = 24
	 $lsn[24].log_each do |r, l|
	    assert_equal($lsn[i], l, "lsn ==")
	    assert_equal("test #{i}", r, "value ==")
	    i += 1
	 end
	 assert_equal(100, i, "<end each>")
     end
   end

   def test_06_reverse_each
      if $lsn[0].respond_to?(:reverse_each)
	 i = 55
	 $lsn[55].log_reverse_each do |r, l|
	    assert_equal($lsn[i], l, "lsn == reverse")
	    assert_equal("test #{i}", r, "value == reverse")
	    i -= 1
	 end
	 assert_equal(-1, i, "<end reverse each>")
      end
   end
 
   def test_07_random
      begin
	 1000.times do
	    nb = rand($lsn.size)
	    assert_equal("test #{nb}", $lsn[nb].log_get, "<log_get>")
	 end
      ensure
	 $env.close
      end
   end

   def test_08_reinit
      clean
      assert_kind_of(BDB::Env, 
		     $env = BDB::Env.open("tmp",  BDB::CREATE | BDB::INIT_LOG | BDB::INIT_TXN,
					  "thread" => false, 
					  :set_lg_bsize => 10000,
					  "set_lg_max" => 45000),
		     "<open>")
   end

   def test_09_put
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

   def test_10_log_get
      l = nil
      $lsn.each_with_index do |ls, i|
	 if l
	    assert(ls > l, "<compare>")
	 end
	 assert_equal(ls.log_get, $rec[i], "<equal>")
	 l = ls
      end
   end

   def test_11_file
      if BDB::VERSION_MAJOR < 4
	 log_file = Dir.glob("tmp/*")
	 $lsn.each do |ls|
	    assert(log_file.include?(ls.log_file), "<include>")
	 end
      end
   end

   def test_12_end
      $env.close
      clean
   end

end

if defined?(RUNIT)
   RUNIT::CUI::TestRunner.run(TestLog.suite)
end
