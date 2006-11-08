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

# module BDB
## Berkeley DB environment is an encapsulation of one or more databases,
## log files and shared information about the database environment such
## as shared memory buffer cache pages.
## 
## The simplest way to administer a Berkeley DB application environment
## is to create a single home directory that stores the files for the
## applications that will share the environment. The environment home
## directory must be created before any Berkeley DB applications are run.
## Berkeley DB itself never creates the environment home directory. The
## environment can then be identified by the name of that directory.
# class Env
# class << self

=== Class Methods

--- open(home, flags = 0, mode = 0, options = {})
--- create(home, flags = 0, mode = 0, options = {})
--- new(home, flags = 0, mode = 0, options = {})
    open the Berkeley DB environment

    : ((|home|))
      If this argument is non-NULL, its value may be used as the
      database home, and files named relative to its path. 

    : ((|mode|))
      mode for creation (see chmod(2))

    : ((|flags|))
      must be set to 0 or by OR'ing with  

       : ((|BDB::INIT_CDB|)) Initialize locking.
       : ((|BDB::INIT_LOCK|)) Initialize the locking subsystem.
       : ((|BDB::INIT_LOG|))  Initialize the logging subsystem.
       : ((|BDB::INIT_MPOOL|)) Initialize the shared memory buffer pool subsystem.
       : ((|BDB::INIT_TXN|)) Initialize the transaction subsystem.
       : ((|BDB::INIT_TRANSACTION|)) 
         Equivalent to ((|DB_INIT_LOCK | DB_INIT_MPOOL | DB_INIT_TXN | DB_INIT_LOG|))
       : ((|BDB::RECOVER|)) 
         Run normal recovery on this environment before opening it for normal
         use. If this flag is set, the DB_CREATE flag must also be set since
         the regions will be removed and recreated.

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
      
    : ((|options|))
      Hash, Possible options are (see the documentation of Berkeley DB
      for more informations) 

      : ((|set_app_dispatch|)) : configure application recovery interface (DB >= 4.1)
      : ((|set_cachesize|)) :   set the database cache size
      : ((|set_data_dir|)) : set the environment data directory (DB >= 3)
      : ((|set_encrypt|)) : set the environment cryptographic key (DB >= 4.1)
      : ((|set_feedback|)) : set feedback callback (DB >= 3)
      : ((|set_flags|)) : environment configuration (DB >= 3.2)
      : ((|set_lg_bsize|)) : set log buffer size (DB >= 3)
      : ((|set_lg_dir|)) : set the environment logging directory (DB >= 3)
      : ((|set_lg_max|)) : set log file size
      : ((|set_lg_regionmax|)) : set logging region size (DB >= 3)
      : ((|set_lk_conflicts|)) : set lock conflicts matrix (DB >= 3)
      : ((|set_lk_detect|)) : set automatic deadlock detection
      : ((|set_lk_max_lockers|)) : set maximum number of lockers
      : ((|set_lk_max_locks|)) : set maximum number of locks
      : ((|set_lk_max_objects|)) : set maximum number of lock objects
      : ((|set_rep_transport|)) : configure replication transport (DB >= 4)
      : ((|set_rep_limit|)) : limit data sent in response to a single message (DB >= 4.1)
      : ((|set_rep_nsites|)) : configure replication group site count (DB >= 4.5)
      : ((|set_rep_priority|)) : configure replication site priority (DB >= 4.5)
      : ((|set_rep_config|)) : configure the replication subsystem (DB >= 4.5)
      : ((|set_rep_timeout|)) : configure replication timeouts (DB >= 4.5)
      : ((|set_rpc_server|)) : establish an RPC server connection (DB >= 3.1)
      : ((|set_tas_spins|)) : set the number of test-and-set spins (DB >= 3)
      : ((|set_tmp_dir|)) : set the environment temporary file directory (DB >= 3)
      : ((|set_timeout|)) : set lock and transaction timeout (DB >= 4)
      : ((|set_tx_max|)) : set maximum number of transactions (DB >= 3)
      : ((|set_tx_timestamp|)) : set recovery timestamp (DB >= 3.1)
      : ((|set_verbose|)) : set verbose messages
      : ((|set_verb_chkpoint|)) :display checkpoint location information when searching the log for checkpoints. (DB >= 3)
      : ((|set_verb_deadlock|)) : display additional information when doing deadlock detection. (DB >= 3)
      : ((|set_verb_recovery|)) : display additional information when performing recovery. (DB >= 3)
      : ((|set_verb_replication|)) : display additional information when processing replication messages. (DB >= 4)
      : ((|set_verb_waitsfor|)) : display the waits-for table when doing deadlock detection. (DB >= 3)

      Proc given to ((|set_feedback|)), ((|set_app_dispatch|)) and
      ((|set_rep_transport|)) can be also specified as a method
      (replace the prefix ((|set_|)) with ((|bdb_|)))

      For ((|bdb_rep_transport|)) the constant ((|ENVID|)) must be defined

      The constant ((|BDB_ENCRYPT|)) can be used to replace ((|set_encrypt|))

--- remove()
--- unlink()
    remove the environnement

# end

=== Methods
      

--- close()
    close the environnement

--- dbremove(file, database = nil, flags = 0)
    only with BDB::VERSION_MAJOR == 4 && BDB::VERSION_MINOR >= 1

    remove the database specified by ((|file|)) and ((|database|)). If no
    ((|database|)) is ((|nil|)), the underlying file represented by 
    ((|file|)) is removed, incidentally removing all databases
    that it contained. 

    The ((|flags|)) value must be set to 0 or ((|BDB::AUTO_COMMIT|))

--- dbrename(file, database, newname, flags = 0)
    only with BDB::VERSION_MAJOR == 4 && BDB::VERSION_MINOR >= 1

    rename the database specified by ((|file|)) and ((|database|)) to
    ((|newname|)). If ((|database|)) is ((|nil|)), the underlying file
    represented by ((|file|)) is renamed, incidentally renaming all databases
    that it contained. 

    The ((|flags|)) value must be set to 0 or ((|BDB::AUTO_COMMIT|))

--- feedback=(proc)
    monitor the progress of some operations

--- home()
    return the name of the directory

--- lock()
--- lock_id()
    Acquire a locker ID

--- lock_detect(type, flags = 0)
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


--- log_archive(flags = 0)
    The log_archive function return an array of log or database file names.

    ((|flags|)) value must be set to 0 or the value ((|BDB::ARCH_DATA|)),
    ((|BDB::ARCH_ABS|)), ((|BDB::ARCH_LOG|))

--- log_checkpoint(string)

    same as ((|log_put(string, BDB::CHECKPOINT)|))

--- log_curlsn(string)

    same as ((|log_put(string, BDB::CURLSN)|))

--- log_each { |string, lsn| ... }

    Implement an iterator inside of the log

--- log_flush(string = nil)

    same as ((|log_put(string, BDB::FLUSH)|))

    Without argument, garantee that all records are written to the disk

--- log_get(flag)

    The ((|log_get|)) return an array ((|[String, BDB::Lsn]|)) according to
    the ((|flag|)) value.

    ((|flag|)) can has the value ((|BDB::CHECKPOINT|)), ((|BDB::FIRST|)), 
    ((|BDB::LAST|)), ((|BDB::NEXT|)), ((|BDB::PREV|)), ((|BDB::CURRENT|))

--- log_put(string, flag = 0)

    The ((|log_put|)) function appends records to the log. It return
    an object ((|BDB::Lsn|))

    ((|flag|)) can have the value ((|BDB::CHECKPOINT|)), ((|BDB::CURLSN|)),
    ((|BDB::FLUSH|))

--- log_reverse_each { |string, lsn| ... }

    Implement an iterator inside of the log

--- log_stat

    return log statistics

--- open_db(type, name = nil, subname = nil, flags = 0, mode = 0)
    open the database in the current environment. type must be one of
    the constant ((|BDB::BTREE|)), ((|BDB::HASH|)), ((|BDB::RECNO|)), 
    ((|BDB::QUEUE|)). See ((<open|URL:access.html#open>)) for other
    arguments

--- recover { |txn, id| ... }
    only with BDB::VERSION_MAJOR == 3 && BDB::VERSION_MINOR >= 3

    iterate over all prepared transactions. The transaction ((|txn|))
    must be made a call to #abort, #commit, #discard

    ((|id|)) is the global transaction ID for the transaction

--- set_flags(flags, onoff = true) 
    only with BDB::VERSION_MAJOR == 3 && BDB::VERSION_MINOR >= 2

    ((|flags|)) can have the value ((|BDB::CDB_ALLDB|)), ((|BDB::NOMMAP|))
    ((|BDB::TXN_NOSYNC|))

    if ((|onoff|)) is false, the specified flags are cleared

      
--- begin(flags = 0)
--- txn_begin(flags = 0)
--- begin(flags = 0, db, ...) { |txn, db, ...| ...}
    begin a transaction (the transaction manager must be enabled). flags
    can have the value ((|DBD::TXN_COMMIT|)), in this case the transaction
    will be commited at end.

--- checkpoint(kbyte, min = 0)
--- txn_checkpoint(kbyte, min = 0)
    The txn_checkpoint function flushes the underlying memory pool,
    writes a checkpoint record to the log and then flushes the log.

    If either kbyte or min is non-zero, the checkpoint is only done
    if more than min minutes have passed since the last checkpoint, or if
    more than kbyte kilobytes of log data have been written since the last
    checkpoint.

--- stat()
--- txn_stat()
    Return transaction subsystem statistics

--- repmgr_add_remote(host, port, flag = 0)
    Only for DB >= 4.5

    Adds a new replication site to the replication manager's list of known sites.

    Return the environment ID assigned to the remote site

--- repmgr_ack_policy=(policy)
--- repmgr_set_ack_policy(policy)
    Only for DB >= 4.5

    Specifies how master and client sites will handle acknowledgment of replication
    messages which are necessary for "permanent" records.

    ((|policy|)) must be set to one of the following values 
    ((|BDB::REPMGR_ACKS_ALL|)), ((|BDB::REPMGR_ACKS_ALL_PEERS|)),
    ((|BDB::REPMGR_ACKS_NONE|)), ((|BDB::REPMGR_ACKS_ONE|)),
    ((|BDB::REPMGR_ACKS_ONE_PEER|)), ((|BDB::REPMGR_ACKS_QUORUM|))

--- repmgr_ack_policy
--- repmgr_get_ack_policy
    Only for DB >= 4.5

    Returns the replication manager's client acknowledgment policy.

--- repmgr_site_list
    Only for DB >= 4.5

    Returns an array with the status of the sites currently known by the
    replication manager.

--- repmgr_set_local_site(host, port, flag = 0)
    Only for DB >= 4.5

    Specifies the host identification string and port number for the local system.

--- repmgr_start(count, flag)
    Only for DB >= 4.5

    Starts the replication manager.

--- rep_config[]=(which, onoff)
    Only for DB >= 4.5

    Configures the Berkeley DB replication subsystem.

    ((|which|)) can have the value ((|BDB::REP_CONF_BULK|)),
    ((|BDB::REP_CONF_DELAYCLIENT|)), ((|BDB::REP_CONF_NOAUTOINIT|)),
    ((|BDB::REP_CONF_NOWAIT|))

    ((|onoff|)) can have the value ((|true|)) or ((|false|))

--- rep_config?[](which)
    Only for DB >= 4.5

    Returns ((|true|)) if the specified ((|which|)) parameter is currently set or not.

--- rep_nsites=(sites)
    Only for DB >= 4.5

    Specifies the total number of sites in a replication group.

--- rep_nsites
    Only for DB >= 4.5

    Returns the total number of sites in a replication group.

--- rep_priority=(priority)
    Only for DB >= 4.5

    Specifies the priority in the replication group elections.

--- rep_priority
    Only for DB >= 4.5

    Returns the database environment priority.

--- rep_timeout[]=(which, timeout)
    Only for DB >= 4.5

    Specifies the timeout in the replication group elections.

    ((|which|)) can have the value ((|BDB::REP_ACK_TIMEOUT|)),
    ((|BDB::REP_ELECTION_TIMEOUT|)), ((|BDB::REP_ELECTION_RETRY|)),
    ((|BDB::REP_CONNECTION_RETRY|))

--- rep_timeout[](which)
    Only for DB >= 4.5

    Returns the database environment timeout for ((|which|))

--- elect(sites, priority, timeout)
--- rep_elect(sites, priority, timeout)
    Only for DB >= 4

    Holds an election for the master of a replication group, returning the
    new master's ID

    Raise ((|BDB::RepUnavail|)) if the ((|timeout|)) expires

--- process_message(control, rec, envid)
--- rep_process_message(control, rec, envid)
    Only for DB >= 4

    Processes an incoming replication message sent by a member of the
    replication group to the local database environment

--- start(cdata, flags)
--- rep_start(cdata, flags)
    Only for DB >= 4

    ((|cdata|)) is an identifier
    ((|flags|)) must be one of ((|BDB::REP_CLIENT|)), ((|BDB::REP_MASTER|))
    or ((|BDB::REP_LOGSONLY|))

--- lsn_reset(file, flags = 0)
    Only for DB >= 4.4
    
    Reset database file LSN
    
    The Env#lsn_reset method allows database files to be moved from one transactional 
    database environment to another.
    
    ((|file|))The name of the physical file in which the LSNs are to be cleared.
    ((|flags|)) must be set to 0 or ((|BDB::ENCRYPT|))
    
--- fileid_reset(file, flags = 0)
    Only for DB >= 4.4
    
    Reset database file ID
    
    The Env#fileid_reset method allows database files to be copied, and then the copy 
    used in the same database environment as the original.
    
    ((|file|))The name of the physical file in which new file IDs are to be created.
    ((|flags|)) must be set to 0 or ((|BDB::ENCRYPT|))
    
--- msgcall=(call_proc)
    Only for DB >= 4.4
    
    There are interfaces in the Berkeley DB library which either directly output informational 
    messages or statistical information : Env#msgcall is used to set callback which will
    called by BDB
    
    The value given must be ((|nil|)) to unconfigure the callback, or and object
    which respond to ((|#call|)) : it will called with a String as argument
    
--- thread_id=(call_proc)
    Only for DB >= 4.4
    
    Declare a proc object which returns a unique identifier pair for the current 
    thread of control.
    
    The proc must return a pair
     *((|pid|)): process ID of the current thread
     *((|tid|)): thread ID of the current thread
    
--- thread_id_string=(call_proc)
    Only for DB >= 4.4
    
    Declare a proc that formats a process ID and thread ID identifier pair for display.
    
    The proc will be called with 2 arguments and must return a String
    
--- is_alive=(call_proc)
    Only for DB >= 4.4
    
    Declare a proc that returns if a thread of control (either a true thread or
    a process) is still running. 
    
    The proc will be called with 2 arguments (pid, tid)
    
--- failcheck(flag = 0)
    Only for DB >= 4.4
    
    The method checks for threads of control (either a true 
    thread or a process) that have exited while manipulating Berkeley DB library 
    data structures
    
    ((|flag|)) is actually unused and must be set to 0
    


# end
# end

=end
