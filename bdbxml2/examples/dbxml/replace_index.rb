#!/usr/bin/ruby -I../.. -I../../../src

require 'bdbxml'

def replace(container, uri, name, itype)
   puts "Replacing index type : #{itype} to #{name}"
   index = container.index
   puts "Before index replace"
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

man = BDB::Env.new(options['home'], BDB::INIT_TRANSACTION).manager
man.begin do |txn|
   con = txn.open_container(options['container'])
   replace(con, "", "product",  
           "node-attribute-substring-string node-element-equality-string edge-element-presence-none")
   txn.commit
end

