#!/usr/bin/ruby -I../.. -I../../../src

require 'bdbxml'

def replace(container, uri, name, itype)
   puts "Replacing index type : #{itype} to #{name}"
   index = container.index
   puts "Before index delete"
   count = 0
   index.each do |u, n, i|
      puts "\tFor node #{n}, found index #{i}."
      count += 1
   end
   puts "#{count} indexes found"
   index.replace(uri, name, itype)
   container.index = index
   puts "After index replace"
   count = 0
   index.each do |u, n, i|
      puts "\tFor node #{n}, found index #{i}."
      count += 1
   end
   puts "#{count} indexes found"
rescue
   puts "Replace index failed #{$!}"
end

options = {'home' => 'env', 'container' => 'name.xml'}

BDB::Env.new(options['home'], BDB::INIT_TRANSACTION).begin do |txn|
   con = txn.open_xml(options['container'])
   replace(con, "", "product", 
	   "node-attribute-substring-string node-element-equality-string")
   txn.commit
end

