# a BDB::Lockid object is created by the method lock, lock_id
class BDB::Lockid
   
   #The lock_get function acquires a lock from the lock table, it return
   #an object <em>BDB::Lock</em>
   #
   #<em>string</em> specifies the object to be locked or released.
   #
   #<em>mode</em> is an index into the environment's lock conflict array
   #
   #<em>flags</em> value must be set to 0 or the value <em>BDBD::LOCK_NOWAIT</em>
   #in this case if a lock cannot be granted because the requested
   #lock conflicts with an existing lock, raise an error <em>BDB::LockGranted</em>
   #
   def  get(string, mode , flags = 0)
   end
   #same than <em> get</em>
   def  lock_get(string, mode [, flags])
   end
   
   #The <em>lock_vec</em> function atomically obtains and releases one or more
   #locks from the lock table. The <em>lock_vec</em> function is intended to
   #support acquisition or trading of multiple locks under one lock table
   #semaphore, as is needed for lock coupling or in multigranularity
   #locking for lock escalation.
   #
   #* <em>array</em> 
   #  ARRAY of HASH with the following keys
   #
   #  * <em>"op"</em>  
   #    the operation to be performed, which must be set to one
   #    of the following values <em>BDB::LOCK_GET</em>, <em>BDB::LOCK_PUT</em>,
   #    <em>BDB::LOCK_PUT_ALL</em> or <em>BDB::LOCK_PUT_OBJ</em>
   #
   #  * <em>"obj"</em> 
   #    the object (String) to be locked or released
   #
   #  * <em>"mode"</em> 
   #    is an index into the environment's lock conflict array
   #
   #  * <em>"lock"</em> 
   #    an object <em>BDB::Lock</em>
   #
   #* <em>flags</em> 
   #  value must be set to 0 or the value <em>BDBD::LOCK_NOWAIT</em>
   #  in this case if a lock cannot be granted because the requested
   #  lock conflicts with an existing lock,  raise an error
   #  <em>BDB::LockGranted</em>return immediately
   #
   def  vec(array , flags = 0) 
   end
   #same than <em> vec</em>
   def  lock_vec(array [, flags]) 
   end
end

class BDB::Lock
   
   #The <em>lock_put</em> function releases lock from the lock table. 
   #
   def  put()
   end
   #same than <em> put</em>
   def  lock_put()
   end
   #same than <em> put</em>
   def  release()
   end
   #same than <em> put</em>
   def  delete()
   end
end
