#!/usr/bin/ruby -I../.. -I../../../src

require 'bdbxml'

def delete(container, uri, name, itype)
   puts "Deleting index type : #{itype} to #{name}"
   index = container.index
   puts "Before index delete"
   count = 0
   index.each do |u, n, i|
      puts "\tFor node #{n}, found index #{i}."
      count += 1
   end
   puts "#{count} indexes found"
   index.delete(uri, name, itype)
   container.index = index
   puts "After index delete"
   count = 0
   index.each do |u, n, i|
      puts "\tFor node #{n}, found index #{i}."
      count += 1
   end
   puts "#{count} indexes found"
rescue
   puts "delete index failed #{$!}"
end

options = {'home' => 'env', 'container' => 'name.xml'}

man = BDB::Env.new(options['home'], BDB::INIT_TRANSACTION).manager
man.begin do |txn|
   con = txn.open_container(options['container'])
   delete(con, "", "product", "node-element-equality-string")
   delete(con, "", "product", "edge-element-presence-none")
   txn.commit
end

