=begin
== Acces Methods

These are the methods for ((|BDB::Recnum|))

# module BDB
#^
Don't mix these methods with methods of ((|BDB::Cursor|))
#^
##
## All instance methods has the same syntax than the methods of Array
# class Recnum < Common
# class << self

=== Class Methods

--- open(name = nil, subname = nil, flags = 0, mode = 0, options = {})
--- create(name = nil, subname = nil, flags = 0, mode = 0, options = {})
--- new(name = nil, subname = nil, flags = 0, mode = 0, options = {})
     open the database


       BDB::Recnum.open(name, subname, flags, mode)

       is equivalent to

       BDB::Recno.open(name, subname, flags, mode,
                       "set_flags" => BDB::RENUMBER,
                       "set_array_base" => 0)

--- remove(name, subname = nil) 
--- db_remove(name, subname = nil) 
--- unlink(name, subname = nil) 
     Removes the database (or subdatabase) represented by the
     name and subname combination.

     If no subdatabase is specified, the physical file represented by name
     is removed, incidentally removing all subdatabases that it contained.

--- upgrade(name)
--- db_upgrade(name)
     Upgrade the database

# end

=== Methods

((*All this methods has the same syntax than for the class ((|Array|))*))

--- self[nth] 
    retrieves the ((|nth|)) item from an array. Index starts from zero.
    If index is the negative, counts backward from the end of the array. 
    The index of the last element is -1. Returns ((|nil|)), if the ((|nth|))
    element is not exist in the array. 

--- self[start..end] 
    returns an array containing the objects from ((|start|)) to ((|end|)),
    including both ends. if end is larger than the length of the array,
    it will be rounded to the length. If ((|start|)) is out of an array 
    range , returns ((|nil|)).
    And if ((|start|)) is larger than end with in array range, returns
    empty array ([]). 

--- self[start, length] 
    returns an array containing ((|length|)) items from ((|start|)). 
    Returns ((|nil|)) if ((|length|)) is negative. 

--- self[nth] = val 
    changes the ((|nth|)) element of the array into ((|val|)). 
    If ((|nth|)) is larger than array length, the array shall be extended
    automatically. Extended region shall be initialized by ((|nil|)). 

--- self[start..end] = val 
    replace the items from ((|start|)) to ((|end|)) with ((|val|)). 
    If ((|val|)) is not an array, the type of ((|val|)) will be converted
    into the Array using ((|to_a|)) method. 

--- self[start, length] = val 
    replace the ((|length|)) items from ((|start|)) with ((|val|)). 
    If ((|val|)) is not an array, the type of ((|val|)) will be converted 
    into the Array using ((|to_a|)). 

--- self + other 
    concatenation

--- self * times 
    repetition

--- self - other 
    substraction

--- self & other 
    returns a new array which contains elements belong to both elements.

--- self | other 
    join

--- self << obj 
    append a new item with value ((|obj|)). Return ((|self|))

--- self <=> other 
    comparison : return -1, 0 or 1

--- clear
    delete all elements 

--- collect {|item| ..} 
    Returns a new array by invoking block once for every element,
    passing each element as a parameter to block. The result of block
    is used as the given element 

--- collect! {|item| ..} 
    invokes block once for each element of db, replacing the element
    with the value returned by block.

--- concat(other) 
    append ((|other|)) to the end

--- delete(val) 
    delete the item which matches to ((|val|))

--- delete_at(pos) 
    delete the item at ((|pos|))

--- delete_if {|x| ...} 
    delete the item if the block return ((|true|))

--- reject!{|x|...} 
    delete the item if the block return ((|true|))

--- each {|x| ...} 
    iterate over each item

--- each_index {|i| ...} 
    iterate over each index

--- empty?
    return ((|true|)) if the db file is empty 

--- fill(val)
    set the entire db with ((|val|)) 

--- fill(val, start[, length])
    fill the db with ((|val|)) from ((|start|)) 

--- fill(val, start..end)
    set the db with ((|val|)) from ((|start|)) to ((|end|)) 

--- include?(val) 
    returns true if the given ((|val|)) is present

--- index(val) 
    returns the index of the item which equals to ((|val|)). 
    If no item found, returns ((|nil|))

--- indexes(index_1,..., index_n) 
    returns an array consisting of elements at the given indices

--- indices(index_1,..., index_n) 
    returns an array consisting of elements at the given indices

--- join([sep]) 
    returns a string created by converting each element to a string

--- length
--- size 
    return the number of elements of the db file

--- nitems 
    return the number of non-nil elements of the db file

--- pop 
    pops and returns the last value

--- push(obj...) 
    appends obj

--- replace(other) 
    replaces the contents of the db file  with the contents of ((|other|))

--- reverse 
    returns the array of the items in reverse order

--- reverse! 
    replaces the items in reverse order.

--- reverse_each {|x| ...} 
    iterate over each item in reverse order

--- rindex(val) 
    returns the index of the last item which verify ((|item == val|))

--- shift 
    remove and return the first element

--- to_a
--- to_ary
    return an ((|Array|)) with all elements

--- unshift(obj) 
    insert ((|obj|)) to the front of the db file

# end
# end
=end
