#!/usr/bin/ruby
$LOAD_PATH.unshift *%w{../src . tests}

$dir = Dir.pwd

at_exit do
   Dir.chdir($dir)
   clean()
end

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

puts "\nVERSION of BDB is #{BDB::VERSION}\n"
puts "\nVERSION of BDB::XML is #{BDB::XML::VERSION}\n"

Inh = defined?(RUNIT) ? RUNIT : Test::Unit

class TestXML < Inh::TestCase

   def test_00_env
      @flag ||= BDB::INIT_LOMP
      assert_kind_of(BDB::Env, $env = BDB::Env.new("tmp", 
						   BDB::CREATE | @flag))
      assert_kind_of(BDB::XML::Container, $glo = $env.open_xml("glossary", "a"))
      assert_kind_of(BDB::XML::Index, index = $glo.index)
      $glo.index = [["http://moulon.inra.fr/", "reference", "node-attribute-equality-string"]]
      assert_equal("glossary", $glo.name)
      $base = "."
   end

   def test_01_doc
      $id, $names = [], []
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
	 $names << file
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
	 assert_equal($time, doc.get('time', nil, String))
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

   def test_05_names
      assert_kind_of(BDB::XML::Context,
		     cxt = BDB::XML::Context.new(BDB::XML::Context::Values))
      assert_kind_of(BDB::XML::Results,
		     query = $glo.query("//*//@dbxml:name", cxt))
      file = query.collect{|name| name}
      names = $id.collect{|id| id[0]}
      assert_equal(names.sort, file.sort)
      ids = []
      $glo.search("//*//@dbxml:id", BDB::XML::Context::Values) {|i| ids << i}
      id = $id.collect{|id| id[1]}
      assert_equal(ids.sort, id.sort)
   end
      
   def test_06_dump
      assert_equal(nil, $glo.close)
      assert_equal(nil, $env.close)
      assert_equal(nil, BDB::XML::Container.dump("tmp/glossary", "tmp/dumpee"))
      assert_equal(nil, BDB::XML::Container.load("tmp/glossary", "tmp/dumpee"))
      assert_equal(nil, BDB::XML::Container.remove("tmp/glossary"))
   end

   def test_07_reinit
      @flag = BDB::INIT_TRANSACTION
      @mask = "[a-m]"
      $reference = {"matz" => [], "object" => [], "ruby" => []}
      clean
      test_00_env
      test_01_doc
      test_04_query
   end

   def test_08_transaction
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

   def test_09_single
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
   end


end


if defined?(RUNIT)
   RUNIT::CUI::TestRunner.run(TestXML.suite)
end
