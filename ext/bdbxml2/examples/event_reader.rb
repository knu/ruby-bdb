#!/usr/bin/ruby

$LOAD_PATH.unshift Dir.pwd.sub(%r{(bdb-\d\.\d\.\d/).*}, '\1bdbxml2')
require 'bdbxml'

File::unlink("exa.dbxml") rescue nil

man = BDB::XML::Manager.new
con = man.create_container("exa.dbxml")
at_exit {
   con.close
   man.close
}
con.put("doc", "<root><a a1=\"val1\">a text</a><b><c cattr=\"c1\"><d/></c></b></root>")

doc = con.get("doc")
rdr = doc.event_reader
while rdr.next?
   case rdr.next
   when BDB::XML::EventReader::StartElement
      puts "BDB::XML::EventReader::StartElement"
   when BDB::XML::EventReader::EndElement
      puts "BDB::XML::EventReader::EndElement"
   when BDB::XML::EventReader::StartDocument
      puts "BDB::XML::EventReader::StartDocument"
   when BDB::XML::EventReader::EndDocument
      puts "BDB::XML::EventReader::EndDocument"
   when BDB::XML::EventReader::StartEntityReference
      puts "BDB::XML::EventReader::StartEntityReference"
   when BDB::XML::EventReader::EndEntityReference
      puts "BDB::XML::EventReader::EndEntityReference"
   when BDB::XML::EventReader::Characters
      puts "BDB::XML::EventReader::Characters"
   when BDB::XML::EventReader::CDATA
      puts "BDB::XML::EventReader::CDATA"
   when BDB::XML::EventReader::Comment
      puts "BDB::XML::EventReader::Comment"
   when BDB::XML::EventReader::Whitespace
      puts "BDB::XML::EventReader::Whitespace"
   when BDB::XML::EventReader::DTD
      puts "BDB::XML::EventReader::DTD"
   when BDB::XML::EventReader::ProcessingInstruction
      puts "BDB::XML::EventReader::ProcessingInstruction"
   end
end
rdr.close

val = BDB::XML::Value.new(doc)
rdr = val.event_reader
doc1 = man.create_document
doc1.name = 'doc1'
doc1.content = rdr
con.put(doc1)
doc1 = con.get('doc1')
puts doc1.content
