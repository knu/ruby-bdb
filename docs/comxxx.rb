# Implementation of a sorted, balanced tree structure
#
# Inherit from BDB::Common
#
class BDB::Btree < BDB::Common
   # Return an Hash with the fields (description for 4.0.14)
   # * bt_magic : Magic number that identifies the file as a Btree database. 
   # * bt_version : The version of the Btree database. 
   # * bt_nkeys : the number of unique keys in the database.
   # * bt_ndata : the number of key/data pairs in the database.
   # * bt_pagesize : Underlying database page size, in bytes. 
   # * bt_minkey : The minimum keys per page. 
   # * bt_re_len : The length of fixed-length records. 
   # * bt_re_pad : The padding byte value for fixed-length records. 
   # * bt_levels : Number of levels in the database. 
   # * bt_int_pg : Number of database internal pages. 
   # * bt_leaf_pg : Number of database leaf pages. 
   # * bt_dup_pg : Number of database duplicate pages. 
   # * bt_over_pg : Number of database overflow pages. 
   # * bt_free : Number of pages on the free list. 
   # * bt_int_pgfree : Number of bytes free in database internal pages. 
   # * bt_leaf_pgfree : Number of bytes free in database leaf pages. 
   # * bt_dup_pgfree :  Number of bytes free in database duplicate pages. 
   # * bt_over_pgfree : Number of bytes free in database overflow pages.
   def stat(flags = 0)
   end
end

# Implementation of Extended Linear Hashing
#
# Inherit from BDB::Common
#
class BDB::Hash < BDB::Common
   # Return an Hash with the fields (description for 4.0.14)
   # * hash_magic : Magic number that identifies the file as a Hash file. 
   # * hash_version : The version of the Hash database. 
   # * hash_nkeys : The number of unique keys in the database. 
   # * hash_ndata : The number of key/data pairs in the database.
   # * hash_pagesize : The underlying Hash database page (and bucket) size, in bytes. 
   # * hash_nelem : The estimated size of the hash table specified at database-creation time. 
   # * hash_ffactor :  The desired fill factor (number of items per bucket) specified at database-creation time. 
   # * hash_buckets :  The number of hash buckets. 
   # * hash_free : The number of pages on the free list. 
   # * hash_bfree :The number of bytes free on bucket pages. 
   # * hash_bigpages : The number of big key/data pages. 
   # * hash_big_bfree : The number of bytes free on big item pages. 
   # * hash_overflows : The number of overflow pages
   # * hash_ovfl_free :  The number of bytes free on overflow pages. 
   # * hash_dup : The number of duplicate pages. 
   # * hash_dup_free : The number of bytes free on duplicate pages. 
   def stat(flags = 0)
   end
end

# Stores both fixed and variable-length records with logical record 
# numbers as keys
#
# Inherit from BDB::Common
#
class BDB::Recno < BDB::Common
   #Removes and returns an association from the database.
   #
   def  shift 
   end
   
   # Return an Hash with the fields (description for 4.0.14)
   # * bt_magic : Magic number that identifies the file as a Btree database. 
   # * bt_version : The version of the Btree database. 
   # * bt_nkeys : The exact number of records in the database. 
   # * bt_ndata : The exact number of records in the database.
   # * bt_pagesize : Underlying database page size, in bytes. 
   # * bt_minkey : The minimum keys per page. 
   # * bt_re_len : The length of fixed-length records. 
   # * bt_re_pad : The padding byte value for fixed-length records. 
   # * bt_levels : Number of levels in the database. 
   # * bt_int_pg : Number of database internal pages. 
   # * bt_leaf_pg : Number of database leaf pages. 
   # * bt_dup_pg : Number of database duplicate pages. 
   # * bt_over_pg : Number of database overflow pages. 
   # * bt_free : Number of pages on the free list. 
   # * bt_int_pgfree : Number of bytes free in database internal pages. 
   # * bt_leaf_pgfree : Number of bytes free in database leaf pages. 
   # * bt_dup_pgfree : Number of bytes free in database duplicate pages. 
   # * bt_over_pgfree : Number of bytes free in database overflow pages. 
   def stat(flags = 0)
   end
end

# Stores fixed-length records with logical record numbers as keys.
# It is designed for fast inserts at the tail and has a special cursor
# consume operation that deletes and returns a record from the head of
# the queue
#
# Inherit from BDB::Common
#
class BDB::Queue < BDB::Common
   #Push the values
   #
   def  push values
   end
   
   #Removes and returns an association from the database.
   #
   def  shift 
   end
   
   # Return an Hash with the fields (description for 4.0.14)
   # * qs_magic : Magic number that identifies the file as a Queue file. 
   # * qs_version : The version of the Queue file type. 
   # * qs_nkeys : The number of records in the database.
   # * qs_ndata : The number of records in the database.
   # * qs_pagesize : Underlying database page size, in bytes. 
   # * qs_extentsize : Underlying database extent size, in pages. 
   # * qs_pages : Number of pages in the database. 
   # * qs_re_len : The length of the records. 
   # * qs_re_pad : The padding byte value for the records. 
   # * qs_pgfree : Number of bytes free in database pages. 
   # * qs_first_recno : First undeleted record in the database. 
   # * qs_cur_recno : Last allocated record number in the database. 
   def stat(flags = 0)
   end
end

# Don't mix these methods with methods of <em>BDB::Cursor</em>
#
# All instance methods has the same syntax than the methods of Array
#
#BDB::Recnum.open(name, subname, flags, mode)
#
#is equivalent to
#
#BDB::Recno.open(name, subname, flags, mode,
#"set_flags" => BDB::RENUMBER, "set_array_base" => 0)
#
# Inherit from BDB::Common
class BDB::Recnum < BDB::Common
   #Element reference - with the following syntax
   #
   #self[nth]
   #
   #retrieves the <em>nth</em> item from an array. Index starts from zero.
   #If index is the negative, counts backward from the end of the array. 
   #The index of the last element is -1. Returns <em>nil</em>, if the <em>nth</em>
   #element is not exist in the array. 
   #
   #self[start..end]
   #
   #returns an array containing the objects from <em>start</em> to <em>end</em>,
   #including both ends. if end is larger than the length of the array,
   #it will be rounded to the length. If <em>start</em> is out of an array 
   #range , returns <em>nil</em>.
   #And if <em>start</em> is larger than end with in array range, returns
   #empty array ([]). 
   #
   #self[start, length]    
   #
   #returns an array containing <em>length</em> items from <em>start</em>. 
   #Returns <em>nil</em> if <em>length</em> is negative. 
   #
   def  [](args) 
   end
   
   #Element assignement -- with the following syntax
   #
   #self[nth] = val 
   #
   #changes the <em>nth</em> element of the array into <em>val</em>. 
   #If <em>nth</em> is larger than array length, the array shall be extended
   #automatically. Extended region shall be initialized by <em>nil</em>. 
   #
   #self[start..end] = val 
   #
   #replace the items from <em>start</em> to <em>end</em> with <em>val</em>. 
   #If <em>val</em> is not an array, the type of <em>val</em> will be 
   #converted into the Array using <em>to_a</em> method. 
   #
   #self[start, length] = val 
   #
   #replace the <em>length</em> items from <em>start</em> with <em>val</em>. 
   #If <em>val</em> is not an array, the type of <em>val</em> will be
   #converted into the Array using <em>to_a</em>. 
   #
   def  []=(args, val)
   end
   
   #concatenation
   #
   def  +(other)
   end
   
   #repetition
   #
   def  *(times)
   end
   
   #substraction
   #
   def  -(other)
   end
   
   #returns a new array which contains elements belong to both elements.
   #
   def  &(other)
   end
   
   #join
   #
   def  |(other)
   end

   #append a new item with value <em>obj</em>. Return <em>self</em>
   #
   def  <<(obj)
   end
   
   #comparison : return -1, 0 or 1
   #
   def  <=>(other) 
   end
   
   #delete all elements 
   #
   def  clear
   end
   
   #Returns a new array by invoking block once for every element,
   #passing each element as a parameter to block. The result of block
   #is used as the given element 
   #
   def  collect  
      yield item
   end
   
   #invokes block once for each element of db, replacing the element
   #with the value returned by block.
   #
   def  collect!  
      yield item
   end
   
   #append <em>other</em> to the end
   #
   def  concat(other) 
   end
   
   #delete the item which matches to <em>val</em>
   #
   def  delete(val) 
   end
   
   #delete the item at <em>pos</em>
   #
   def  delete_at(pos) 
   end
   
   #delete the item if the block return <em>true</em>
   #
   def  delete_if  
      yield x
   end
   
   #delete the item if the block return <em>true</em>
   #
   def  reject! 
      yield x
   end
   
   #iterate over each item
   #
   def  each  
      yield x
   end
   
   #iterate over each index
   #
   def  each_index  
      yield i
   end
   
   #return <em>true</em> if the db file is empty 
   #
   def  empty?
   end
   
   #set the entire db with <em>val</em> 
   #
   def  fill(val)
   end
   
   #fill the db with <em>val</em> from <em>start</em> 
   #
   def  fill(val, start[, length])
   end
   
   #set the db with <em>val</em> from <em>start</em> to <em>end</em> 
   #
   def  fill(val, start..end)
   end
   
   #returns true if the given <em>val</em> is present
   #
   def  include?(val) 
   end
   
   #returns the index of the item which equals to <em>val</em>. 
   #If no item found, returns <em>nil</em>
   #
   def  index(val) 
   end
   
   #returns an array consisting of elements at the given indices
   #
   def  indexes(index_1,..., index_n) 
   end
   
   #returns an array consisting of elements at the given indices
   #
   def  indices(index_1,..., index_n) 
   end
   
   #returns a string created by converting each element to a string
   #
   def  join([sep]) 
   end
   
   #return the number of elements of the db file
   #
   def  length
   end
   #same than <em> length</em>
   def  size 
   end
   
   #return the number of non-nil elements of the db file
   #
   def  nitems 
   end
   
   #pops and returns the last value
   #
   def  pop 
   end
   
   #appends obj
   #
   def  push(obj, ...) 
   end
   
   #replaces the contents of the db file  with the contents of <em>other</em>
   #
   def  replace(other) 
   end
   
   #returns the array of the items in reverse order
   #
   def  reverse 
   end
   
   #replaces the items in reverse order.
   #
   def  reverse! 
   end
   
   #iterate over each item in reverse order
   #
   def  reverse_each  
      yield x
   end
   
   #returns the index of the last item which verify <em>item == val</em>
   #
   def  rindex(val) 
   end
   
   #remove and return the first element
   #
   def  shift 
   end
   
   #return an <em>Array</em> with all elements
   #
   def  to_a
   end
   #same than <em> to_a</em>
   def  to_ary
   end
   
   #insert <em>obj</em> to the front of the db file
   #
   def  unshift(obj) 
   end
end
