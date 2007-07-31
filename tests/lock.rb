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

$env, $db = nil, nil
$object = "OBJECT"
$matrix = [
   [0, 0, 0],
   [0, 0, 1],
   [0, 1, 1]
]

   
clean

print "\nVERSION of BDB is #{BDB::VERSION}\n"

Inh = defined?(RUNIT) ? RUNIT : Test::Unit

class TestLock < Inh::TestCase

   def test_01_error
      assert_raises(BDB::Fatal) do
	 BDB::Env.open("tmp", BDB::CREATE | BDB::INIT_LOCK,
		       "set_lk_conflicts" => [[0, 0, 0]])
      end
      assert_raises(BDB::Fatal) do
	 BDB::Env.open("tmp", BDB::CREATE | BDB::INIT_LOCK,
		       "set_lk_conflicts" => [[0], [2]])
      end
=begin
      assert_raises(TypeError) do
	 BDB::Env.open("tmp", BDB::CREATE | BDB::INIT_LOCK,
		       "set_lk_max" => "a")
      end
=end
   end

   def test_02_init
      assert_kind_of(BDB::Env, 
		     $env = BDB::Env.open("tmp", BDB::CREATE | BDB::INIT_LOCK,
					  "set_lk_conflicts" => $matrix, 
					  "set_lk_max" => 1000))
   end

   def test_03_get_put
      assert_kind_of(BDB::Lockid, $lockid = $env.lock)
      [BDB::LOCK_NG, BDB::LOCK_READ, BDB::LOCK_WRITE].each do |mode|
	 assert_kind_of(BDB::Lock, lock = $lockid.lock_get("object #{mode}", mode))
	 assert_equal(nil, lock.put, "<release>")
      end
   end

   def test_04_multiple
      $locks = []
      [BDB::LOCK_NG, BDB::LOCK_READ, BDB::LOCK_WRITE].each do |mode|
	 assert_kind_of(BDB::Lock, lock = $lockid.lock_get($object, mode))
	 $locks << lock
      end

      intern = []
      10.times do
	 assert_kind_of(BDB::Lock, lock = $lockid.lock_get($object, BDB::LOCK_WRITE, true))
	 intern << lock
      end
      intern.each do |lock|
	 assert_equal(nil, lock.release)
      end
   end

   def test_05_newlock
      assert_kind_of(BDB::Lockid, lockid = $env.lock)
      [BDB::LOCK_READ, BDB::LOCK_WRITE].each do |mode|
	 assert_raises(BDB::LockGranted) { lockid.lock_get($object, mode, true) }
      end
      assert_equal(nil, lockid.close)
   end

   def test_06_release
      $locks.each do |lock|
	 assert_equal(nil, lock.release, "<release>")
      end
      assert_equal(nil, $lockid.close)
   end

   def test_07_renew
      assert_kind_of(BDB::Lockid, lockid = $env.lock)
      [BDB::LOCK_READ, BDB::LOCK_WRITE].each do |mode|
	 assert_kind_of(BDB::Lock, lock = lockid.lock_get($object, mode, true))
	 assert_equal(nil, lock.put, "<put>")
      end
      assert_equal(nil, lockid.close)
      assert_equal(nil, $env.close)
   end
   
   def test_08_transaction
      clean
      if BDB::VERSION_NUMBER <= 30110
	 puts "skipping  test for this version"
	 return
      end
      assert_kind_of(BDB::Env, $env = BDB::Env.open("tmp", BDB::CREATE | BDB::INIT_TRANSACTION))
      assert_kind_of(BDB::Queue, $db = $env.open_db(BDB::Queue, "aa", nil, "a", "set_re_len" => 10))
      assert_kind_of(BDB::Txn, txn1 = $env.begin("flags" => BDB::TXN_NOWAIT))
      assert_kind_of(BDB::Queue, db1 = txn1.assoc($db))
      assert_equal(1, inc = db1.push("record1")[0], "<set db>")
      assert_kind_of(BDB::Txn, txn2 = $env.begin("flags" => BDB::TXN_NOWAIT))
      assert_kind_of(BDB::Queue, db2 = txn2.assoc($db))
      if BDB::VERSION_MAJOR >= 4 && BDB::VERSION_MINOR >= 2
	 assert_raises(BDB::LockDead) { db2[inc] }
      else
	 assert_raises(BDB::LockGranted) { db2[inc] }
      end
      assert_equal(true, txn1.commit, "<commit>")
      if BDB::VERSION_MAJOR >= 4
	 assert_equal(1, $env.lock_stat["st_nlocks"], "<st_nlocks>")
      end
      assert_equal(true, txn2.commit, "<commit>")
      if BDB::VERSION_MAJOR >= 4
	 assert_equal(1, $env.lock_stat["st_nlocks"], "<st_nlocks>")
      end
   end

   def test_forward(meth1, meth2)
      assert_kind_of(BDB::Txn, txn1 = $env.begin("flags" => BDB::TXN_NOWAIT))
      assert_kind_of(BDB::Queue, db1 = txn1.assoc($db))
      ind1, = db1.push("record1")
      assert_kind_of(Numeric, ind1)
      if BDB::VERSION_MAJOR >= 4
	 assert_equal(2, $env.lock_stat["st_nlocks"], "<lock stat 1>")
      end
      assert_kind_of(BDB::Txn, txn2 = $env.begin("flags" => BDB::TXN_NOWAIT))
      assert_kind_of(BDB::Queue, db2 = txn2.assoc($db))
      ind2, = db2.push("record2")
      assert_kind_of(Numeric, ind2)
      if BDB::VERSION_MAJOR >= 4
	 assert_equal(3, $env.lock_stat["st_nlocks"], "<lock stat 2>")
      end
      txn1.send(meth1)
      if BDB::VERSION_MAJOR >= 4
	 assert_equal(2, $env.lock_stat["st_nlocks"], "<lock stat 1>")
      end
      assert_equal("record2", db2[ind2], "<after txn1>")
      if meth1 == "abort"
	 assert_equal(nil, db2[ind1], "<after txn1 nil>")
      else
	 assert_equal("record1", db2[ind1], "<after txn1 record1>")
      end
      txn2.send(meth2)
      if BDB::VERSION_MAJOR >= 4
	 assert_equal(1, $env.lock_stat["st_nlocks"], "<lock stat 1>")
      end
      if meth1 == "commit"
	 assert_equal("record1", $db[ind1], "<commit txn1>")
      else
	 assert_equal(nil, $db[ind1], "<abort txn1>")
      end
      if meth2 == "commit"
	 assert_equal("record2", $db[ind2], "<commit txn2>")
      else
	 assert_equal(nil, $db[ind2], "<abort txn2>")
      end
   end

   def test_09_forward
      if BDB::VERSION_NUMBER <= 30114
	 puts "skipping  test for this version"
	 return
      end
      ["commit", "abort"].each do |t1|
	 ["commit", "abort"].each do |t2|
	    test_forward(t1, t2)
	 end
      end
   end

   def test_reverse(meth1, meth2)
      assert_kind_of(BDB::Txn, txn1 = $env.begin("flags" => BDB::TXN_NOWAIT))
      assert_kind_of(BDB::Queue, db1 = txn1.assoc($db))
      ind1, = db1.push("record1")
      assert_kind_of(Numeric, ind1)
      if BDB::VERSION_MAJOR >= 4
	 assert_equal(2, $env.lock_stat["st_nlocks"], "<lock stat 1>")
      end
      assert_kind_of(BDB::Txn, txn2 = $env.begin("flags" => BDB::TXN_NOWAIT))
      assert_kind_of(BDB::Queue, db2 = txn2.assoc($db))
      ind2, = db2.push("record2")
      assert_kind_of(Numeric, ind2)
      if BDB::VERSION_MAJOR >= 4
	 assert_equal(3, $env.lock_stat["st_nlocks"], "<lock stat 2>")
      end
      txn2.send(meth2)
      if BDB::VERSION_MAJOR >= 4
	 assert_equal(2, $env.lock_stat["st_nlocks"], "<lock stat 1>")
      end
      assert_equal("record1", db1[ind1], "<after txn2>")
      if meth2 == "abort"
	 assert_equal(nil, db1[ind2], "<after txn2 nil>")
      else
	 assert_equal("record2", db1[ind2], "<after txn1 record1>")
      end
      txn1.send(meth1)
      if BDB::VERSION_MAJOR >= 4
	 assert_equal(1, $env.lock_stat["st_nlocks"], "<lock stat 1>")
      end
      if meth1 == "commit"
	 assert_equal("record1", $db[ind1], "<commit txn1>")
      else
	 assert_equal(nil, $db[ind1], "<abort txn1>")
      end
      if meth2 == "commit"
	 assert_equal("record2", $db[ind2], "<commit txn2>")
      else
	 assert_equal(nil, $db[ind2], "<abort txn2>")
      end
   end

   def test_10_reverse
      if BDB::VERSION_NUMBER <= 30114
	 puts "skipping  test for this version"
	 return
      end
      ["commit", "abort"].each do |t1|
	 ["commit", "abort"].each do |t2|
	    test_reverse(t1, t2)
	 end
      end
   end


end

if defined?(RUNIT)
   RUNIT::CUI::TestRunner.run(TestLog.suite)
end
