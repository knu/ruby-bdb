#!/usr/bin/ruby
$LOAD_PATH.unshift *%w{../src . tests}
require 'bdbxml'
require 'runit_'

def clean
   Dir.foreach('tmp') do |x|
      if FileTest.file?("tmp/#{x}")
         File.unlink("tmp/#{x}")
      end
   end
end

$glo, $bdb, $env = nil, nil, nil
$time = Time.now.to_s
$reference = {"matz" => [], "object" => [], "ruby" => []}

clean

print "\nVERSION of BDB is #{BDB::VERSION}\n"

Inh = defined?(RUNIT) ? RUNIT : Test::Unit

class TestXML < Inh::TestCase

   def test_00_env
      @flag ||= BDB::INIT_LOMP
      assert_kind_of(BDB::Env, $env = BDB::Env.new("tmp", 
						   BDB::CREATE | @flag))
      assert_kind_of(BDB::XML::Container, $glo = $env.open_xml("glossary", "a"))
      assert_equal($glo, 
		   $glo.index("http://moulon.inra.fr/", "reference", 
			      "node-attribute-equality-string"))
      assert_equal("glossary", $glo.name)
      $base = "."
   end

   def test_01_doc
      $id = []
      @mask ||= "[a-z]"
      Dir["#{$base}/glossary/#{@mask}*"].each do |file|
	 assert_kind_of(BDB::XML::Document, a = BDB::XML::Document.new)
	 content = IO.readlines(file, nil)[0]
	 $reference.each do |k, v|
	    if content.index("reference>#{k}")
	       v << file
	    end
	 end
	 assert_equal(content, a.content = content)
	 assert_equal(file, a.name = file)
	 assert_equal($time, a['time'] = $time)
	 $id << [file, $glo.push(a)]
      end
   end

   def test_02_each
      $id.each do |file, ind|
	 assert_kind_of(BDB::XML::Document, doc = $glo[ind])
	 assert_equal(file, doc.name)
	 content = IO.readlines(doc.name, nil)[0]
	 assert_equal(content, doc.content)
	 assert_equal(content, doc.to_s)
	 assert_equal($time, doc['time'])
      end
   end

   def test_03_search
      names = []
      $glo.search("//*") {|doc| names << [doc.name, doc.id] }
      assert_equal($id.sort, names.sort)
      names = $glo.collect {|doc| [doc.name, doc.id] }
      assert_equal($id.sort, names.sort)
      names = $glo.query("//*").collect {|doc| [doc.name, doc.id] }
      assert_equal($id.sort, names.sort)
      names = $glo.query($glo.parse("//*")).collect {|doc| [doc.name, doc.id] }
      assert_equal($id.sort, names.sort)
   end

   def test_04_query
      $reference.each do |k, v|
	 query = $glo.query("/entry[reference[contains(text(), '#{k}')]]")
	 file = query.collect {|doc| doc.name}
	 assert_equal(v.sort, file.sort)
      end
   end

   def test_05_dump
      assert_equal(nil, $glo.close)
      assert_equal(nil, $env.close)
      assert_equal(nil, BDB::XML::Container.dump("tmp/glossary", "tmp/dumpee"))
      assert_equal(nil, BDB::XML::Container.load("tmp/glossary", "tmp/dumpee"))
      assert_equal(nil, BDB::XML::Container.remove("tmp/glossary"))
      clean
   end

   def test_06_reinit
      @flag = BDB::INIT_TRANSACTION
      @mask = "[a-m]"
      $reference = {"matz" => [], "object" => [], "ruby" => []}
      test_00_env
      test_01_doc
      test_04_query
   end

   def test_07_transaction
      old_ref = {}
      $reference.each{|k,v| old_ref[k] = v.dup}
      old_glo = $glo
      $env.begin($glo) do |txn, $glo|
	 @mask = "[n-z]"
	 test_01_doc
	 test_04_query
      end
      $glo, $reference = old_glo, old_ref
      test_04_query
      $env.begin($glo) do |txn, $glo|
	 @mask = "[n-z]"
	 test_01_doc
	 test_04_query
	 txn.commit
      end
      $glo = old_glo
      test_04_query
   end

   def test_08_single
      $glo.close
      $env.close
      clean
      Dir.chdir('tmp')
      $base = ".."
      $reference.each {|k,v| v.clear}
      assert_kind_of(BDB::XML::Container, $glo = BDB::XML::Container.new("glossary", "a"))
      test_01_doc
      test_02_each
      test_03_search
      test_04_query
      Dir.chdir("..")
      clean
   end


end


if defined?(RUNIT)
   RUNIT::CUI::TestRunner.run(TestXML.suite)
end
