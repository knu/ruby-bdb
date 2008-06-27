#!/usr/bin/ruby 
$LOAD_PATH.unshift("../../src", "..")
require 'bdbxml'

include BDB

File::unlink("exa.dbxml") rescue nil

man = XML::Manager.new
con = man.create_container("exa.dbxml", XML::INDEX_NODES)
at_exit {
   con.close
   man.close
}

upd = man.create_update_context

con.add_index("", "foo", "node-element-equality-string")
con.add_index("http://www.example.com/schema", "foo", "node-element-equality-string")
con.add_index("", "foo", "node-element-presence")
con.add_index("http://www.example.com/schema", "foo", "node-element-presence")
con.add_index("", "len", "edge-attribute-equality-decimal")
con.add_index("", "len", "edge-attribute-presence")
con.add_index("", "date", "edge-element-equality-date")

con.put("docA", <<-EOT, upd)
<docA>
  <foo>hello</foo>
  <foo>charlie</foo>
  <foo>brown</foo>
  <foo>aNd</foo>
  <foo>Lucy</foo>
</docA>
EOT
con.put("docB", <<-EOT, upd)
<docB xmlns:bar='http://www.example.com/schema'>
  <bar:foo>hello</bar:foo>
  <bar:foo>charlie</bar:foo>
  <bar:foo>brown</bar:foo>
  <bar:foo>aNd</bar:foo>
  <bar:foo>Lucy</bar:foo>
</docB>
EOT
con.put("docC",  <<-EOT, upd)
<docC>
  <foobar>
    <baz len='6.7'>tall guy</baz>
    <baz len='75'>30 yds</baz>
    <baz len='75'>30 yds</baz>
    <baz len='5.0'>five feeet</baz>
    <baz len='0.2'>point two</baz>
    <baz len='60.2'>five feet</baz>
  </foobar>
</docC>
EOT
con.put("docD", <<-EOT, upd)
<docD>
 <dates1>
  <date>2005-08-02</date>
  <date>2003-06-12</date>
  <date>1005-12-12</date>
 </dates1>
 <dates2>
  <date>1492-05-30</date>
  <date>2000-01-01</date>
  <date>1984-12-25</date>
 </dates2>
</docD>
EOT

puts "content"
con.each do |value|
   puts "\n\tDocument #{value.to_document.name}"
   puts value
end

puts "\nnode-element-presence\n"
xil = man.create_index_lookup(con, "", "foo", "node-element-presence")
["", "http://www.example.com/schema" ].each do |uri|
   xil.node_uri = uri
   puts "\tnode : #{xil.node.inspect}"
   xil.execute.each {|p| puts "\t\t#{p}"}
end

puts "\nnode-element-equality-string : charlie"
xil = man.create_index_lookup(con, "", "foo", "node-element-equality-string")
[[XML::IndexLookup::LT, "<"], [XML::IndexLookup::LTE, "<="],
 [XML::IndexLookup::GT, ">"], [XML::IndexLookup::GTE, ">="]].each do 
   |comp, rep|
   xil.low_bound = ["charlie", comp]
   puts "\n low_bound : charlie #{rep} #{xil.low_bound[0]}"
   ["", "http://www.example.com/schema" ].each do |uri|
      xil.node_uri = uri
      puts "\tnode : #{xil.node.inspect}"
      xil.execute.each {|p| puts "\t\t#{p}"}
      puts "\tnode : #{xil.node.inspect} -- reverse"
      xil.execute(nil, XML::REVERSE_ORDER).each {|p| puts "\t\t#{p}"}
   end
end

puts "\nedge-attribute-equality-decimal : 40"
xil = man.create_index_lookup(con, "", "len", "edge-attribute-equality-decimal")
xil.parent = ["", "baz"]
[[XML::IndexLookup::LT, "<"], [XML::IndexLookup::LTE, "<="],
 [XML::IndexLookup::GT, ">"], [XML::IndexLookup::GTE, ">="]].each do 
   |comp, rep|
   xil.low_bound = [XML::Value.new(XML::Value::DECIMAL, 40), comp]
   puts "\n low_bound : len #{rep} #{xil.low_bound[0]}"
   puts "\tnode : #{xil.node.inspect}"
   xil.execute.each {|p| puts "\t\t#{p}"}
   puts "\tnode : #{xil.node.inspect} -- reverse"
   xil.execute(nil, XML::REVERSE_ORDER).each {|p| puts "\t\t#{p}"}
end


puts "\nedge-element-equality-date : date == 2003-06-12"
xil = man.create_index_lookup(con, "", "date", "edge-element-equality-date",
                              XML::Value.new(XML::Value::DATE, "2003-06-12"),
                              XML::IndexLookup::EQ)
xil.parent = ["", "dates1"]
puts "\tparent = dates1"
xil.execute.each {|p| puts "\t\t#{p}"}
xil.parent = ["", "dates2"]
puts "\tparent = dates2"
xil.execute(nil, XML::REVERSE_ORDER).each {|p| puts "\t\t#{p}"}
