=begin
== Acces Methods

These are the methods for ((|BDB::Recnum|))

Don't mix these methods with methods of ((|BDB::Cursor|))

=== Class Methods

--- create([name, subname, flags, mode, options])
--- new([name, subname, flags, mode, options])
--- open([name, subname, flags, mode, options])
     open the database


       BDB::Recnum.open(name, subname, flags, mode)

       is equivalent to

       BDB::Recno.open(name, subname, flags, mode,
                       "set_flags" => BDB::RENUMBER,
                       "set_array_base" => 0)

--- db_remove(name [, subname]) 
--- remove(name [, subname]) 
--- unlink(name [,subname]) 
     Removes the database (or subdatabase) represented by the
     name and subname combination.

     If no subdatabase is specified, the physical file represented by name
     is removed, incidentally removing all subdatabases that it contained.

--- db_upgrade(name)
--- upgrade(name)
     Upgrade the database

=== Methods

((*All this methods has the same syntax than for the class ((|Array|))*))

self[nth] 

self[start..end] 

self[start, length] 


self[nth] = val 

self[start..end] = val 

self[start, length] = val 

self + other 

self * times 

self - other 

self & other 

self | other 

self << obj 

self <=> other 

clear 

concat(other) 

delete(val) 

delete_at(pos) 

delete_if {...} 

reject!{|x|...} 

each {...} 

each_index {...} 

empty? 

fill(val) 

fill(val, start[, length]) 

fill(val, start..end) 

filter{|item| ..} 

freeze 

frozen 

include?(val) 

index(val) 

indexes(index_1,..., index_n) 

indices(index_1,..., index_n) 

join([sep]) 

length
 
size 

nitems 

pop 

push(obj...) 

replace(other) 

reverse 

reverse! 

reverse_each {...} 

rindex(val) 

shift 

unshift(obj) 

=end
