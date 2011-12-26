#!/usr/bin/ruby
$LOAD_PATH.unshift *%w{../src . tests}

$dir = Dir.pwd

def clean
   Dir.foreach('tmp') do |x|
      if FileTest.file?("tmp/#{x}")
         File.unlink("tmp/#{x}")
      end
   end
end

at_exit do
   Dir.chdir($dir)
   clean()
end

require 'bdbxml'
require 'runit_'

$glo, $bdb, $env, $man = nil, nil, nil, nil
$time = Time.now.to_s
$reference = {"matz" => [], "object" => [], "ruby" => []}

clean

puts "\nVERSION of BDB is #{BDB::VERSION}\n"
puts "\nVERSION of BDB::XML is #{BDB::XML::VERSION}\n"

Inh = defined?(RUNIT) ? RUNIT : Test::Unit

class TestXML < Inh::TestCase

   def assert_quote(a, b)
      a = a.to_s
      b = b.to_s
      a.gsub!(/"/, "'")
      b.gsub!(/"/, "'")
      assert_equal(a, b)
   end

   def test_00_env
      @flag ||= BDB::INIT_LOMP
      assert_kind_of(BDB::Env, $env = BDB::Env.new("tmp", 
						   BDB::CREATE | @flag))
      $man.close if $man
      assert_kind_of(BDB::XML::Manager, $man = $env.manager)
      if (@flag & BDB::INIT_TXN) != 0
         assert_kind_of(BDB::XML::Container, $glo = $man.create_container("glossary", BDB::XML::TRANSACTIONAL))
      else
         assert_kind_of(BDB::XML::Container, $glo = $man.create_container("glossary"))
      end
      assert_equal("glossary", $glo.name)
      $base = "."
   end

   def int_01_doc(man, glo)
      $id, $names = [], []
      @mask ||= "[a-z]"
      Dir["#{$base}/glossary/#{@mask}*"].each do |file|
	 assert_kind_of(BDB::XML::Document, a = man.create_document)
	 content = IO.readlines(file, nil)[0]
	 $reference.each do |k, v|
	    if content.index("reference>#{k}")
	       v << file
	    end
	 end
	 assert_quote(content, a.content = content)
	 assert_equal(file, a.name = file)
         assert($time, a['time'] = $time)
         glo[file] = a
	 $id << file
	 $names << file
      end
   end

   def test_01_doc
      int_01_doc($man, $glo)
   end

   def test_02_each
      $id.each do |file|
	 assert_kind_of(BDB::XML::Document, doc = $glo[file])
	 assert_equal(file, doc.name)
	 content = IO.readlines(doc.name, nil)[0]
	 assert_quote(content, doc.content)
	 assert_quote(content, doc.to_s)
=begin
	 assert_equal($time, doc['time'])
	 assert_equal($time, doc.get(nil, 'time'))
=end
      end
   end

   def test_03_search
      names = []
      $man.query("collection('glossary')/*") do |doc| 
         names << doc.to_document.name
      end
      assert_equal($id.sort, names.sort)
      que = $man.prepare("collection('glossary')/*")
      assert_kind_of(BDB::XML::Query, que)
      names = []
      que.execute do |doc| 
         names << doc.to_document.name
      end
      assert_equal($id.sort, names.sort)
      assert_equal("collection('glossary')/*", que.to_s)
   end

   def int_04_query(man)
      $reference.each do |k, v|
         query = man.query("collection('glossary')/entry[reference[contains(text(), '#{k}')]]")
         file = query.collect {|doc| doc.to_document.name}
         assert_equal(v.sort, file.sort)
      end
   end

   def test_04_query
      int_04_query($man)
   end

   def test_05_dump
      assert_equal(nil, $man.close)
      assert_equal(nil, $env.close)
      assert_kind_of(BDB::Env, $env = BDB::Env.new("tmp", BDB::INIT_LOMP))
      assert_kind_of(BDB::XML::Manager, $man = $env.manager)
      assert_equal($man, $man.dump("glossary", "tmp/dumpee"))
#      assert_equal($man, $man.load("glossary", "tmp/dumpee"))
      assert_equal(nil, $env.close)
   end

   def test_07_reinit
      @flag = BDB::INIT_TRANSACTION
      @mask = "[a-m]"
      $reference = {"matz" => [], "object" => [], "ruby" => []}
      clean
      test_00_env
      $man.begin($glo) do |txn, glo|
         int_01_doc(txn, glo)
         int_04_query(txn)
         txn.commit
      end
   end

   def test_08_transaction
      old_ref = {}
      $reference.each{|k,v| old_ref[k] = v.dup}
      $man.begin($glo) do |txn, glo|
         @mask = "[n-z]"
         int_01_doc(txn, glo)
         int_04_query(txn)
      end
      $reference = old_ref
      $man.begin {|txn| int_04_query(txn) }
      $man.begin($glo) do |txn, glo|
         @mask = "[n-z]"
         int_01_doc(txn, glo)
         int_04_query(txn)
         txn.commit
      end
      $man.begin {|txn| int_04_query(txn) }
   end

   def test_09_single
      $man.close
      $env.close
      clean
      begin
         Dir.chdir('tmp')
         $base = ".."
         $reference.each {|k,v| v.clear}
         assert_kind_of(BDB::XML::Manager, $man = BDB::XML::Manager.new)
         assert_kind_of(BDB::XML::Container, $glo = $man.create_container("glossary"))
         test_01_doc
         test_02_each
         test_03_search
         test_04_query
      ensure
         $man.close
         Dir.chdir("..")
      end
   end

   def expr_modify(man, content, expression, result)
      doc = man.create_document
      doc.content = content
      eval expression
      assert_equal(result, doc.to_s, expression)
   end

   def test_10_modify
      clean
      assert_kind_of(BDB::XML::Manager, man = BDB::XML::Manager.new)
      data = File.new('tests/data.t')
      begin
         expression, content, result = nil, nil, nil
         while line = data.gets
            next if /\A#\s*\z/ =~ line || /\A\s*\z/ =~ line
            if /\A#\s*Content/i =~ line
               while line = data.gets
                  break if /\A#\s*\z/ !~ line
               end
               content = line
               while line = data.gets
                  break if /\A#\s*\z/ =~ line
                  content << line
               end
               content.gsub!(/\n/, '')
            end
            if /\A#\s*que = / =~ line
               expression = line.gsub(/\A#\s*/, '')
               while line = data.gets
                  break if /\A#\s*/ !~ line
                  expression << line.sub(/\A#\s*/, '')
               end
               expression<< "mod.execute(doc)"
               result = line
               while line = data.gets
                  break if /\A#\s*\z/ =~ line
                  result << line
               end
               result.gsub!(/\n/, '')
            end
            if result
               expr_modify(man, content, expression, result)
               result = nil
            end
         end
      ensure
         data.close
         man.close
      end
   end

end

if defined?(RUNIT)
   RUNIT::CUI::TestRunner.run(TestXML.suite)
end
