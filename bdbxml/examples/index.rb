#!/usr/bin/ruby -I../../src -I..
require 'bdbxml'

index = BDB::XML::Index.new

index.add(BDB::XML::Namespace["uri"], BDB::XML::Name["default"], 
	  "node-attribute-substring-string")

index.add("http://moulon/", "ref2", "node-attribute-equality-string")
index.add("http://moulon/", "ref0", "node-attribute-equality-string")
index.add("http://moulon/", "ref1", "node-element-presence-none")

index.each do |url, name, val|
   puts "#{url}, #{name}, #{val}"
end

p index.find("http://moulon/", "ref2")
p index.find("", "ref2")

index.add("http://moulon/", "ref0", "node-element-presence-none")
index.add("http://moulon/", "ref0", "edge-attribute-equality-string")
p index.find("http://moulon/", "ref0")

index.delete("http://moulon/", "ref0", "node-attribute-equality-string")
p index.find("http://moulon/", "ref0")

index.replace("http://moulon/", "ref0", "edge-element-presence-none")
p index.find("http://moulon/", "ref0")

index.replace("http://moulon/", "ref0", "none-none-none-none")
p index.find("http://moulon/", "ref0")


