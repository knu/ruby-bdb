# A database cursor is a sequential pointer to the database entries. It
# allows traversal of the database and access to duplicate keyed
# entries.  Cursors are used for operating on collections of records,
# for iterating over a database, and for saving handles to individual
# records, so that they can be modified after they have been read.
#
# A cursor is created with the methods BDB::Common#cursor and 
# BDB::Common#cursor_write
#
class BDB::Cursor
   
   #Discards the cursor.
   #
   def  close()
   end
   #same than <em> close</em>
   def  c_close()
   end
   
   #Return the count of duplicate
   #
   def  count()
   end
   #same than <em> count</em>
   def  c_count()
   end
   
   #Same than <tt>get(BDB::CURRENT)</tt>
   #
   def  current()
   end
   #same than <em> current</em>
   def  c_current()
   end
   
   #Deletes the key/data pair currently referenced by the cursor.
   #
   def  del()
   end
   #same than <em> del</em>
   def  delete()
   end
   #same than <em> del</em>
   def  c_del()
   end
   
   #Creates new cursor that uses the same transaction and locker ID as
   #the original cursor. This is useful when an application is using
   #locking and requires two or more cursors in the same thread of
   #control.
   #
   #<em>flags</em> can have the value <em>BDB::DB_POSITION</em>, in this case the
   #newly created cursor is initialized to reference the same position in
   #the database as the original cursor and hold the same locks.
   #
   def  dup(flags = 0)
   end
   #same than <em> dup</em>
   def  clone(flags = 0)
   end
   #same than <em> dup</em>
   def  c_dup(flags = 0)
   end
   #same than <em> dup</em>
   def  c_clone(flags = 0)
   end
   
   #Same than <tt>get(BDB::FIRST)</tt>
   #
   def  first()
   end
   #same than <em> first</em>
   def  c_first()
   end
   
   #Retrieve key/data pair from the database
   #
   #See the description of <tt>c_get</tt> in the Berkeley distribution
   #for the different values of the <em>flags</em> parameter.
   #
   #<em>key</em> must be given if the <em>flags</em> parameter is 
   #<em>BDB::SET</em> | <em>BDB::SET_RANGE</em> | <em>BDB::SET_RECNO</em>
   #
   #<em>key</em> and <em>value</em> must be specified for <em>BDB::GET_BOTH</em>
   #
   def  get(flags, key = nil, value = nil)
   end
   #same than <em> get</em>
   def  c_get(flags, key = nil, value = nil)
   end
   
   #Same than <tt>get(BDB::LAST)</tt>
   #
   def  last()
   end
   #same than <em> last</em>
   def  c_last()
   end
   
   #Same than <tt>get(BDB::NEXT)</tt>
   #
   def  next()
   end
   #same than <em> next</em>
   def  c_next()
   end
   
   #Retrieve key/primary key/data pair from the database
   #
   def  pget(flags, key = nil, value = nil)
   end
   #same than <em> pget</em>
   def  c_pget(flags, key = nil, value = nil)
   end
   
   #Same than <tt>get(BDB::PREV)</tt>
   #
   def  prev()
   end
   #same than <em> prev</em>
   def  c_prev()
   end
   
   #Stores data value into the database.
   #
   #See the description of <tt>c_put</tt> in the Berkeley distribution
   #for the different values of the <em>flags</em> parameter.
   #
   def  put(flags, value)
   end
   #same than <em> put</em>
   def  c_put(flags, value)
   end
   
   #Stores key/data pairs into the database (only for Btree and Hash
   #access methods)
   #
   #<em>flags</em> must have the value <em>BDB::KEYFIRST</em> or
   #<em>BDB::KEYLAST</em>
   #
   def  put(flags, key, value)
   end
   #same than <em> put</em>
   def  c_put(flags, key, value)
   end
   
   #Same than <tt>get</tt> with the flags <em>BDB::SET</em> or <em>BDB::SET_RANGE</em>
   #or <em>BDB::SET_RECNO</em>
   #
   def  set(key)
   end
   #same than <em> set</em>
   def  c_set(key)
   end
   #same than <em> set</em>
   def  set_range(key)
   end
   #same than <em> set</em>
   def  c_set_range(key)
   end
   #same than <em> set</em>
   def  set_recno(key)
   end
   #same than <em> set</em>
   def  c_set_recno(key)
   end
end
