#!/usr/bin/ruby
data = "/home/ts/db/dbxml-1.1.0/examples/xmlData"
unless FileTest.directory?(data) && FileTest.directory?(data + "/simpleData")
   puts "Error can't find DbXml examples"
   puts "path : #{data}"
   exit
end
puts "Loading from #{data}"
[["simpleData", "simple"], ["nsData", "name"]].each do |dir, name|
   xml = Dir[data + "/#{dir}/*.xml"]
   system("ruby ./loadex.rb -h env -c #{name}.xml #{xml.join(' ')}")
end
