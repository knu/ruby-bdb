#!/usr/bin/ruby -I../.. -I../../../src

require 'bdbxml'

def add(container, uri, name, itype)
   puts "Adding index type : #{itype} to #{name}"
   index = container.index
   puts "Before index add"
   count = 0
   index.each do |u, n, i|
      puts "\tFor node #{n}, found index #{i}."
      count += 1
   end
   puts "#{count} indexes found"
   index.add(uri, name, itype)
   container.index = index
   puts "After index add"
   count = 0
   index.each do |u, n, i|
      puts "\tFor node #{n}, found index #{i}."
      count += 1
   end
   puts "#{count} indexes found"
rescue
   puts "Add index failed #{$!}"
end

options = {'home' => 'env', 'container' => 'name.xml'}

man = BDB::Env.new(options['home'], BDB::INIT_TRANSACTION).manager
man.begin do |txn|
   con = txn.open_container(options['container'])
   add(con, "", "product", "node-element-equality-string")
   add(con, "", "product", "edge-element-presence")
   txn.commit
end

