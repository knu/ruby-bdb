#!/usr/bin/ruby
$LOAD_PATH.unshift("../src")
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

system("ps aux |head -1")
10000000.times do |i|
   index.find("http://moulon/", "ref2")
   if (i % 50000) == 0
      system("ps aux | grep ruby | grep -v grep")
   end
end
