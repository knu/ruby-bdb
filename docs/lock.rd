=begin

== Lock system

The lock subsystem provides interprocess and intraprocess concurrency
control mechanisms. While the locking system is used extensively by
the Berkeley DB access methods and transaction system, it may also be
used as a stand-alone subsystem to provide concurrency control to any
set of designated resources.

The lock subsystem is created, initialized, and opened by calls to
(({BDB::Env#open})) with the ((|DB_INIT_LOCK|)) or 
((|DB_INIT_CDB|)) flags specified. 

The following options can be given when the environnement is created

  : ((|"set_lk_conflicts"|))
    Set lock conflicts matrix

  : ((|"set_lk_detect"|))
    Set automatic deadlock detection 

  : ((|"set_lk_max"|))
    Set maximum number of locks

=== BDB::Lockid

A lock ID can be obtained with ((<BDB::Env#lock_id|URL:env.html#lock_id>))

See also ((<BDB::Env#lock_stat|URL:env.html#lock_stat>)) and
((<BDB::Env#lock_detect|URL:env.html#lock_detect>))

==== Methods

--- get(string, mode [, flags])
--- lock_get(string, mode [, flags])
    The lock_get function acquires a lock from the lock table, it return
    an object ((|BDB::Lock|))

    ((|string|)) specifies the object to be locked or released.

    ((|mode|)) is an index into the environment's lock conflict array

    ((|flags|)) value must be set to 0 or the value ((|BDBD::LOCK_NOWAIT|))
    in this case if a lock cannot be granted because the requested
    lock conflicts with an existing lock, return immediately
    instead of waiting for the lock to become available.


--- vec(array [, flags]) 
--- lock_vec(array [, flags]) 
    The ((|lock_vec|)) function atomically obtains and releases one or more
    locks from the lock table. The ((|lock_vec|)) function is intended to
    support acquisition or trading of multiple locks under one lock table
    semaphore, as is needed for lock coupling or in multigranularity
    locking for lock escalation.

     : ((|array|)) 
       ARRAY of HASH with the following keys

          : ((|"op"|))  
            the operation to be performed, which must be set to one
            of the following values ((|BDB::LOCK_GET|)), ((|BDB::LOCK_PUT|)),
            ((|BDB::LOCK_PUT_ALL|)) or ((|BDB::LOCK_PUT_OBJ|))

          : ((|"obj"|)) 
            the object (String) to be locked or released

          : ((|"mode"|)) 
            is an index into the environment's lock conflict array

          : ((|"lock"|)) 
            an object ((|BDB::Lock|))

     : ((|flags|)) 
       value must be set to 0 or the value ((|BDBD::LOCK_NOWAIT|))
       in this case if a lock cannot be granted because the requested
       lock conflicts with an existing lock, return immediately
       instead of waiting for the lock to become available.

=== BDB::Lock

==== Methods

--- put()
--- lock_put()
--- release()
--- delete()
    The ((|lock_put|)) function releases lock from the lock table. 


=end
