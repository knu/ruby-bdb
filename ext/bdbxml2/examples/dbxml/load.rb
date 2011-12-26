#!/usr/local/bin/ruby
data = "#{ENV['HOME']}/ruby/db/dbxml-2.4.13/dbxml/examples/xmlData"
unless FileTest.directory?(data) && FileTest.directory?(data + "/simpleData")
   puts "Error can't find DbXml examples"
   puts "path : #{data}"
   exit
end
Dir.foreach('env') do |x|
   if FileTest.file?("env/#{x}")
      File.unlink("env/#{x}")
   end
end
puts "Loading from #{data}"
[["simpleData", "simple"], ["nsData", "name"]].each do |dir, name|
   system("ruby ./loadex.rb -h env -c #{name}.xml -d #{data}/#{dir}")
end
