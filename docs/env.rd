=begin
== BDB::Env

Berkeley DB environment is an encapsulation of one or more databases,
log files and shared information about the database environment such
as shared memory buffer cache pages.

The simplest way to administer a Berkeley DB application environment
is to create a single home directory that stores the files for the
applications that will share the environment. The environment home
directory must be created before any Berkeley DB applications are run.
Berkeley DB itself never creates the environment home directory. The
environment can then be identified by the name of that directory.
 
* ((<Class Methods>))
* ((<Methods>))

=== Class Methods

--- create(home [, flags, mode, options])
--- new(home [, flags, mode, options])
--- open(home [, flags, mode, options])
     open the Berkeley DB environment


      : ((|home|))
         If this argument is non-NULL, its value may be used as the
         database home, and files named relative to its path. 

      : ((|mode|))
          mode for creation (see chmod(2))

      :  ((|flags|))
          must be set to 0 or by OR'ing with  

            : ((|BDB::INIT_CDB|))
                 Initialize locking.

            : ((|BDB::INIT_LOCK|))
                 Initialize the locking subsystem.

            : ((|BDB::INIT_LOG|))
                 Initialize the logging subsystem.

            : ((|BDB::INIT_MPOOL|))
                 Initialize the shared memory buffer pool subsystem.
      
            : ((|BDB::INIT_TXN|))
                 Initialize the transaction subsystem.

            : ((|BDB::INIT_TRANSACTION|))
                 Equivalent to 
                 ((|DB_INIT_LOCK | DB_INIT_MPOOL | DB_INIT_TXN | DB_INIT_LOG|))

      
            : ((|BDB::RECOVER|))
               Run normal recovery on this environment before opening it for
               normal use. If this flag is set, the DB_CREATE
               flag must also be set since the regions will be removed and
               recreated.
      
            : ((|BDB::RECOVER_FATAL|))
                Run catastrophic recovery on this environment before opening
                it for normal use. If this flag is set, the DB_CREATE flag
                must also be set since the regions will be removed and recreated.
      
            : ((|BDB::USE_ENVIRON|))
                The Berkeley DB process' environment may be permitted to
                specify information to be used when naming files
      
            : ((|BDB::USE_ENVIRON_ROOT|))
                The Berkeley DB process' environment may be permitted to
                specify information to be used when naming files;
                if the DB_USE_ENVIRON_ROOT flag is set, environment
                information will be used for file naming only for users with
                appropriate permissions
      
            : ((|BDB::CREATE|))
                 Cause Berkeley DB subsystems to create any underlying
                 files, as necessary.
      
            : ((|BDB::LOCKDOWN|))
                 Lock shared Berkeley DB environment files and memory mapped
                 databases into memory.
      
            : ((|BDB::NOMMAP|))
                 Always copy read-only database files in this environment
                 into the local cache instead of potentially mapping
                 them into process memory 
      
      
            : ((|BDB::PRIVATE|))
                 Specify that the environment will only be accessed by a
                 single process
      
            : ((|BDB::SYSTEM_MEM|))
                 Allocate memory from system shared memory instead of from
                 memory backed by the filesystem.
      
            : ((|BDB::TXN_NOSYNC|))
                Do not synchronously flush the log on transaction commit or
                prepare. This means that transactions exhibit the
                ACI (atomicity, consistency and isolation) properties, but not
                D (durability), i.e., database integrity will
                be maintained but it is possible that some number of the
                most recently committed transactions may be undone
                during recovery instead of being redone. 

            : ((|BDB::CDB_ALLDB|))
                For Berkeley DB Concurrent Data Store applications, perform
                locking on an environment-wide basis rather than per-database.
      
      :  ((|options|))
          hash. See the documentation of Berkeley DB for possible values.


--- remove()
--- unlink()
     remove the environnement

=== Methods
      

--- close()
     close the environnement

--- lock()
--- lock_id()
     Acquire a locker ID

--- lock_detect(type [, flags])
     The lock_detect function runs one iteration of the deadlock
     detector. The deadlock detector traverses the lock table, and for each
     deadlock it finds, marks one of the participating transactions for
     abort.

     ((|type|)) can have one the value ((|BDB::LOCK_OLDEST|)),
     ((|BDB::LOCK_RANDOM|)) or ((|BDB::LOCK_YOUNGUEST|))

     ((|flags|)) can have the value ((|BDB::LOCK_CONFLICT|)), in this case
     the deadlock detector is run only if a lock conflict has occurred
     since the last time that the deadlock detector was run.   

     return the number of transactions aborted by the lock_detect function
     if ((|BDB::VERSION_MAJOR >= 3|)) or ((|zero|))

--- lock_stat()
     Return lock subsystem statistics


--- log_archive([flags])
    The log_archive function return an array of log or database file names.

    ((|flags|)) value must be set to 0 or the value ((|BDB::ARCH_DATA|)),
    ((|BDB::ARCH_ABS|)), ((|BDB::ARCH_LOG|))

--- log_checkpoint(string)

    same as ((|log_put(string, BDB::CHECKPOINT)|))

--- log_curlsn(string)

    same as ((|log_put(string, BDB::CURLSN)|))

--- log_each { |string, lsn| ... }

    Implement an iterator inside of the log

--- log_flush([string])

    same as ((|log_put(string, BDB::FLUSH)|))

    Without argument, garantee that all records are written to the disk

--- log_get(flag)

    The ((|log_get|)) return an array ((|[String, BDB::Lsn]|)) according to
    the ((|flag|)) value.

    ((|flag|)) can has the value ((|BDB::CHECKPOINT|)), ((|BDB::FIRST|)), 
    ((|BDB::LAST|)), ((|BDB::NEXT|)), ((|BDB::PREV|)), ((|BDB::CURRENT|))

--- log_put(string [, flag])

    The ((|log_put|)) function appends records to the log. It return
    an object ((|BDB::Lsn|))

    ((|flag|)) can have the value ((|BDB::CHECKPOINT|)), ((|BDB::CURLSN|)),
    ((|BDB::FLUSH|))

--- log_reverse_each { |string, lsn| ... }

    Implement an iterator inside of the log

--- log_stat

    return log statistics

--- open_db(type [, name, subname, flags, mode])
     open the database in the current environment. type must be one of
     the constant ((|BDB::BTREE|)), ((|BDB::HASH|)), ((|BDB::RECNO|)), 
     ((|BDB::QUEUE|)). See ((<open|URL:access.html#open>)) for other
     arguments


--- recover { |txn, id| ... }
     only with BDB::VERSION_MAJOR == 3 && BDB::VERSION_MINOR >= 3

     iterate over all prepared transactions. The transaction ((|txn|))
     must be made a call to #abort, #commit, #discard

     ((|id|)) is the global transaction ID for the transaction

--- set_flags(flags [, onoff]) 
      only with BDB::VERSION_MAJOR == 3 && BDB::VERSION_MINOR >= 2

      ((|flags|)) can have the value ((|BDB::CDB_ALLDB|)), ((|BDB::NOMMAP|))
      ((|BDB::TXN_NOSYNC|))

      if ((|onoff|)) is zero, the specified flags are cleared

      
--- txn_begin([flags])
--- begin([flags])
--- begin([flags, db, ...]) { |txn, db, ...| ...}
     begin a transaction (the transaction manager must be enabled). flags
     can have the value ((|DBD::TXN_COMMIT|)), in this case the transaction
     will be commited at end.

--- txn_checkpoint(kbyte [, min])
--- checkpoint(kbyte [, min])
     The txn_checkpoint function flushes the underlying memory pool,
     writes a checkpoint record to the log and then flushes the log.

     If either kbyte or min is non-zero, the checkpoint is only done
     if more than min minutes have passed since the last checkpoint, or if
     more than kbyte kilobytes of log data have been written since the last
     checkpoint.

--- txn_stat()
--- stat()
     Return transaction subsystem statistics

=== Only for DB >= 4

--- rep_elect(sites, priority, timeout)
--- elect(sites, priority, timeout)
     Holds an election for the master of a replication group, returning the
     new master's ID

     Raise ((|BDB::RepUnavail|)) if the ((|timeout|)) expires

--- rep_process_message(control, rec, envid)
--- process_message(control, rec, envid)
     Processes an incoming replication message sent by a member of the
     replication group to the local database environment

--- rep_start(cdata, flags)
--- start(cdata, flags)
     ((|cdata|)) is an identifier
     ((|flags|)) must be one of ((|BDB::REP_CLIENT|)), ((|BDB::REP_MASTER|))
     or ((|BDB::REP_LOGSONLY|))

=end
