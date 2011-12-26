#!/usr/bin/ruby

$LOAD_PATH.unshift Dir.pwd.sub(%r{(bdb-\d\.\d\.\d/).*}, '\1bdbxml2')
require 'bdbxml'

File::unlink("exa.dbxml") rescue nil

man = BDB::XML::Manager.new
con = man.create_container("exa.dbxml", 0, BDB::XML::Container::NodeContainer)
doc = man.create_document
at_exit {
   con.close
   man.close
}
doc.name = 'doc'

wrt = con.event_writer(doc, man.create_update_context)
wrt.start_document
wrt.start_element("root", nil, nil, 0, false)
wrt.start_element("a", nil, nil, 1, false)
wrt.attribute("a1", nil, nil, "val1", true)
wrt.text(BDB::XML::EventReader::Characters, "a text")
wrt.end_element("a")
wrt.start_element("b", nil, nil, 0, false)
wrt.start_element("c", nil, nil, 1, false)
wrt.attribute("cattr", nil, nil, "c1", true)
wrt.start_element("d", nil, nil, 0, true)
wrt.end_element("c")
wrt.text(BDB::XML::EventReader::Characters, "b text")
wrt.text(BDB::XML::EventReader::Characters,"plus")
wrt.end_element("b")
wrt.end_element("root")
wrt.end_document 
wrt.close

puts con.get('doc')
