# Berkeley DB environment is an encapsulation of one or more databases,
# log files and shared information about the database environment such
# as shared memory buffer cache pages.
# 
# The simplest way to administer a Berkeley DB application environment
# is to create a single home directory that stores the files for the
# applications that will share the environment. The environment home
# directory must be created before any Berkeley DB applications are run.
# Berkeley DB itself never creates the environment home directory. The
# environment can then be identified by the name of that directory.
class BDB::Env
   class << self
      
      #open the Berkeley DB environment
      #
      #* <em>home</em>
      #  If this argument is non-NULL, its value may be used as the
      #  database home, and files named relative to its path. 
      #
      #* <em>mode</em>
      #  mode for creation (see chmod(2))
      #
      #* <em>flags</em>
      #  must be set to 0 or by OR'ing with  
      #
      #  * <em>BDB::INIT_CDB</em> Initialize locking.
      #  * <em>BDB::INIT_LOCK</em> Initialize the locking subsystem.
      #  * <em>BDB::INIT_LOG</em>  Initialize the logging subsystem.
      #  * <em>BDB::INIT_MPOOL</em> Initialize the shared memory buffer pool subsystem.
      #  * <em>BDB::INIT_TXN</em> Initialize the transaction subsystem.
      #  * <em>BDB::INIT_TRANSACTION</em> 
      #    Equivalent to DB_INIT_LOCK|DB_INIT_MPOOL|DB_INIT_TXN|DB_INIT_LOG
      #  * <em>BDB::RECOVER</em> 
      #    Run normal recovery on this environment before opening it for normal
      #    use. If this flag is set, the DB_CREATE flag must also be set since
      #    the regions will be removed and recreated.
      #
      #  * <em>BDB::RECOVER_FATAL</em>
      #    Run catastrophic recovery on this environment before opening
      #    it for normal use. If this flag is set, the DB_CREATE flag
      #    must also be set since the regions will be removed and recreated.
      #
      #  * <em>BDB::USE_ENVIRON</em>
      #    The Berkeley DB process' environment may be permitted to
      #    specify information to be used when naming files
      #
      #  * <em>BDB::USE_ENVIRON_ROOT</em>
      #    The Berkeley DB process' environment may be permitted to
      #    specify information to be used when naming files;
      #    if the DB_USE_ENVIRON_ROOT flag is set, environment
      #    information will be used for file naming only for users with
      #    appropriate permissions
      #
      #  * <em>BDB::CREATE</em>
      #    Cause Berkeley DB subsystems to create any underlying
      #    files, as necessary.
      #      
      #  * <em>BDB::LOCKDOWN</em>
      #    Lock shared Berkeley DB environment files and memory mapped
      #    databases into memory.
      #      
      #  * <em>BDB::NOMMAP</em>
      #    Always copy read-only database files in this environment
      #    into the local cache instead of potentially mapping
      #    them into process memory 
      #      
      #  * <em>BDB::PRIVATE</em>
      #    Specify that the environment will only be accessed by a
      #    single process
      #      
      #  * <em>BDB::SYSTEM_MEM</em>
      #    Allocate memory from system shared memory instead of from
      #    memory backed by the filesystem.
      #      
      #  * <em>BDB::TXN_NOSYNC</em>
      #    Do not synchronously flush the log on transaction commit or
      #    prepare. This means that transactions exhibit the
      #    ACI (atomicity, consistency and isolation) properties, but not
      #    D (durability), i.e., database integrity will
      #    be maintained but it is possible that some number of the
      #    most recently committed transactions may be undone
      #    during recovery instead of being redone. 
      #
      #  * <em>BDB::CDB_ALLDB</em>
      #    For Berkeley DB Concurrent Data Store applications, perform
      #    locking on an environment-wide basis rather than per-database.
      #      
      #* <em>options</em>
      #  Hash, Possible options are (see the documentation of Berkeley DB
      #  for more informations) 
      #
      #  * <em>set_app_dispatch</em> : configure application recovery interface (DB >= 4.1)
      #  * <em>set_cachesize</em> :   set the database cache size
      #  * <em>set_data_dir</em> : set the environment data directory (DB >= 3)
      #  * <em>set_encrypt</em> : set the environment cryptographic key (DB >= 4.1)
      #  * <em>set_feedback</em> : set feedback callback (DB >= 3)
      #  * <em>set_flags</em> : environment configuration (DB >= 3.2)
      #  * <em>set_lg_bsize</em> : set log buffer size (DB >= 3)
      #  * <em>set_lg_dir</em> : set the environment logging directory (DB >= 3)
      #  * <em>set_lg_max</em> : set log file size
      #  * <em>set_lg_regionmax</em> : set logging region size (DB >= 3)
      #  * <em>set_lk_conflicts</em> : set lock conflicts matrix (DB >= 3)
      #  * <em>set_lk_detect</em> : set automatic deadlock detection
      #  * <em>set_lk_max_lockers</em> : set maximum number of lockers
      #  * <em>set_lk_max_locks</em> : set maximum number of locks
      #  * <em>set_lk_max_objects</em> : set maximum number of lock objects
      #  * <em>set_rep_transport</em> : configure replication transport (DB >= 4)
      #  * <em>set_rep_limit</em> : limit data sent in response to a single message (DB >= 4.1)
      #  * <em>set_rep_nsites</em> : configure replication group site count (DB >= 4.5)
      #  * <em>set_rep_priority</em> : configure replication site priority (DB >= 4.5)
      #  * <em>set_rep_config</em> : configure the replication subsystem (DB >= 4.5)
      #  * <em>set_rep_timeout</em> : configure replication timeouts (DB >= 4.5)
      #  * <em>set_rpc_server</em> : establish an RPC server connection (DB >= 3.1)
      #  * <em>set_tas_spins</em> : set the number of test-and-set spins (DB >= 3)
      #  * <em>set_tmp_dir</em> : set the environment temporary file directory (DB >= 3)
      #  * <em>set_timeout</em> : set lock and transaction timeout (DB >= 4)
      #  * <em>set_tx_max</em> : set maximum number of transactions (DB >= 3)
      #  * <em>set_tx_timestamp</em> : set recovery timestamp (DB >= 3.1)
      #  * <em>set_verbose</em> : set verbose messages
      #  * <em>set_verb_chkpoint</em> :display checkpoint location information when searching the log for checkpoints. (DB >= 3)
      #  * <em>set_verb_deadlock</em> : display additional information when doing deadlock detection. (DB >= 3)
      #  * <em>set_verb_recovery</em> : display additional information when performing recovery. (DB >= 3)
      #  * <em>set_verb_replication</em> : display additional information when processing replication messages. (DB >= 4)
      #  * <em>set_verb_waitsfor</em> : display the waits-for table when doing deadlock detection. (DB >= 3)
      #
      #  Proc given to <em>set_feedback</em>, <em>set_app_dispatch</em> and
      #  <em>set_rep_transport</em> can be also specified as a method
      #  (replace the prefix <em>set_</em> with <em>bdb_</em>)
      #
      #  For <em>bdb_rep_transport</em> the constant <em>ENVID</em> must be defined
      #
      #  The constant <em>BDB::ENCRYPT</em> can be used to replace <em>set_encrypt</em>
      #
      def  open(home, flags = 0, mode = 0, options = {})
      end
      #same than <em> open</em>
      def  create(home, flags = 0, mode = 0, options = {})
      end
      #same than <em> open</em>
      def  new(home, flags = 0, mode = 0, options = {})
      end
      
      #remove the environnement
      #
      def  remove()
      end
      #same than <em> remove</em>
      def  unlink()
      end
   end
   
   #close the environnement
   #
   def  close()
   end
   
   #only with BDB::VERSION_MAJOR == 4 && BDB::VERSION_MINOR >= 1
   #
   #remove the database specified by <em>file</em> and <em>database</em>. If no
   #<em>database</em> is <em>nil</em>, the underlying file represented by 
   #<em>file</em> is removed, incidentally removing all databases
   #that it contained. 
   #
   #The <em>flags</em> value must be set to 0 or <em>BDB::AUTO_COMMIT</em>
   #
   def  dbremove(file, database = nil, flags = 0)
   end
   
   #only with BDB::VERSION_MAJOR == 4 && BDB::VERSION_MINOR >= 1
   #
   #rename the database specified by <em>file</em> and <em>database</em> to
   #<em>newname</em>. If <em>database</em> is <em>nil</em>, the underlying file
   #represented by <em>file</em> is renamed, incidentally renaming all databases
   #that it contained. 
   #
   #The <em>flags</em> value must be set to 0 or <em>BDB::AUTO_COMMIT</em>
   #
   def  dbrename(file, database, newname, flags = 0)
   end
   
   #monitor the progress of some operations
   #
   def  feedback=(proc)
   end
   
   #return the name of the directory
   #
   def  home()
   end
   
   #Acquire a locker ID
   #
   def  lock()
   end
   #same than <em> lock</em>
   def  lock_id()
   end
   
   #The lock_detect function runs one iteration of the deadlock
   #detector. The deadlock detector traverses the lock table, and for each
   #deadlock it finds, marks one of the participating transactions for
   #abort.
   #
   #<em>type</em> can have one the value <em>BDB::LOCK_OLDEST</em>,
   #<em>BDB::LOCK_RANDOM</em> or <em>BDB::LOCK_YOUNGUEST</em>
   #
   #<em>flags</em> can have the value <em>BDB::LOCK_CONFLICT</em>, in this case
   #the deadlock detector is run only if a lock conflict has occurred
   #since the last time that the deadlock detector was run.   
   #
   #return the number of transactions aborted by the lock_detect function
   #if <em>BDB::VERSION_MAJOR >= 3</em> or <em>zero</em>
   #
   def  lock_detect(type, flags = 0)
   end
   
   #Return lock subsystem statistics
   #
   #
   def  lock_stat()
   end
   
   #The log_archive function return an array of log or database file names.
   #
   #<em>flags</em> value must be set to 0 or the value <em>BDB::ARCH_DATA</em>,
   #<em>BDB::ARCH_ABS</em>, <em>BDB::ARCH_LOG</em>
   #
   def  log_archive(flags = 0)
   end
   
   #
   #same as <em>log_put(string, BDB::CHECKPOINT)</em>
   #
   def  log_checkpoint(string)
   end
   
   #
   #same as <em>log_put(string, BDB::CURLSN)</em>
   #
   def  log_curlsn(string)
   end
   
   #
   #Implement an iterator inside of the log
   #
   def  log_each 
      yield string, lsn
   end
   
   #
   #same as <em>log_put(string, BDB::FLUSH)</em>
   #
   #Without argument, garantee that all records are written to the disk
   #
   def  log_flush(string = nil)
   end
   
   #
   #The <em>log_get</em> return an array <em>[String, BDB::Lsn]</em> according to
   #the <em>flag</em> value.
   #
   #<em>flag</em> can has the value <em>BDB::CHECKPOINT</em>, <em>BDB::FIRST</em>, 
   #<em>BDB::LAST</em>, <em>BDB::NEXT</em>, <em>BDB::PREV</em>, <em>BDB::CURRENT</em>
   #
   def  log_get(flag)
   end
   
   #
   #The <em>log_put</em> function appends records to the log. It return
   #an object <em>BDB::Lsn</em>
   #
   #<em>flag</em> can have the value <em>BDB::CHECKPOINT</em>, <em>BDB::CURLSN</em>,
   #<em>BDB::FLUSH</em>
   #
   def  log_put(string, flag = 0)
   end
   
   #
   #Implement an iterator inside of the log
   #
   def  log_reverse_each 
      yield string, lsn
   end
   
   #
   #return log statistics
   #
   def  log_stat
   end
   
   #open the database in the current environment. type must be one of
   #the constant <em>BDB::BTREE</em>, <em>BDB::HASH</em>, <em>BDB::RECNO</em>, 
   #<em>BDB::QUEUE</em>. See <em>open</em> for other
   #arguments
   #
   def  open_db(type, name = nil, subname = nil, flags = 0, mode = 0)
   end
   
   #only with BDB::VERSION_MAJOR == 3 && BDB::VERSION_MINOR >= 3
   #
   #iterate over all prepared transactions. The transaction <em>txn</em>
   #must be made a call to #abort, #commit, #discard
   #
   #<em>id</em> is the global transaction ID for the transaction
   #
   def  recover 
      yield txn, id
   end
   
   #only with BDB::VERSION_MAJOR == 3 && BDB::VERSION_MINOR >= 2
   #
   #<em>flags</em> can have the value <em>BDB::CDB_ALLDB</em>, <em>BDB::NOMMAP</em>
   #<em>BDB::TXN_NOSYNC</em>
   #
   #if <em>onoff</em> is false, the specified flags are cleared
   #
   #      
   def  set_flags(flags, onoff = true) 
   end
   
   #begin a transaction (the transaction manager must be enabled). flags
   #can have the value <em>DBD::TXN_COMMIT</em>, in this case the transaction
   #will be commited at end.
   #
   def  begin(flags = 0)
   end
   #same than <em> begin</em>
   def  txn_begin(flags = 0)
   end
   
   #The txn_checkpoint function flushes the underlying memory pool,
   #writes a checkpoint record to the log and then flushes the log.
   #
   #If either kbyte or min is non-zero, the checkpoint is only done
   #if more than min minutes have passed since the last checkpoint, or if
   #more than kbyte kilobytes of log data have been written since the last
   #checkpoint.
   #
   def  checkpoint(kbyte, min = 0)
   end
   #same than <em> checkpoint</em>
   def  txn_checkpoint(kbyte, min = 0)
   end
   
   #Return transaction subsystem statistics
   #
   #
   def  stat()
   end
   #same than <em> stat</em>
   def  txn_stat()
   end
   
   #Only for DB >= 4
   #
   #Holds an election for the master of a replication group, returning the
   #new master's ID
   #
   #Raise <em>BDB::RepUnavail</em> if the <em>timeout</em> expires
   #
   def  elect(sites, priority, timeout)
   end
   #same than <em> elect</em>
   def  rep_elect(sites, priority, timeout)
   end
   
   #Only for DB >= 4
   #
   #Processes an incoming replication message sent by a member of the
   #replication group to the local database environment
   #
   def  process_message(control, rec, envid)
   end
   #same than <em> process_message</em>
   def  rep_process_message(control, rec, envid)
   end
   
   #Only for DB >= 4
   #
   #<em>cdata</em> is an identifier
   #<em>flags</em> must be one of <em>BDB::REP_CLIENT</em>, <em>BDB::REP_MASTER</em>
   #or <em>BDB::REP_LOGSONLY</em>
   #
   def  start(cdata, flags)
   end
   #same than <em> start</em>
   def  rep_start(cdata, flags)
   end

   #Only for DB >= 4.4
   #
   #Reset database file LSN
   #
   #The Env#lsn_reset method allows database files to be moved from one transactional 
   #database environment to another.
   #
   #<em>file</em>The name of the physical file in which the LSNs are to be cleared.
   #<em>flags</em> must be set to 0 or <em>BDB::ENCRYPT</em>
   #
   def lsn_reset(file, flags = 0)
   end

   #Only for DB >= 4.4
   #
   #Reset database file ID
   #
   #The Env#fileid_reset method allows database files to be copied, and then the copy 
   #used in the same database environment as the original.
   #
   #<em>file</em>The name of the physical file in which new file IDs are to be created.
   #<em>flags</em> must be set to 0 or <em>BDB::ENCRYPT</em>
   #
   def fileid_reset(file, flags = 0)
   end

   #Only for DB >= 4.4
   #
   #There are interfaces in the Berkeley DB library which either directly output informational 
   #messages or statistical information : Env#msgcall is used to set callback which will
   #called by BDB
   #
   #The value given must be <em>nil</em> to unconfigure the callback, or and object
   #which respond to <em>#call</em> : it will called with a String as argument
   #
   def msgcall=(call_proc)
   end

   #Only for DB >= 4.4
   #
   #Declare a proc object which returns a unique identifier pair for the current 
   #thread of control.
   #
   #The proc must return a pair
   # *<em>pid</em>: process ID of the current thread
   # *<em>tid</em>: thread ID of the current thread
   #
   def thread_id=(call_proc)
   end

   #Only for DB >= 4.4
   #
   #Declare a proc that formats a process ID and thread ID identifier pair for display.
   #
   #The proc will be called with 2 arguments and must return a String
   #
   def thread_id_string=(call_proc)
   end
  
   #Only for DB >= 4.4
   #
   #Declare a proc that returns if a thread of control (either a true thread or
   #a process) is still running. 
   #
   #The proc will be called with 2 arguments (pid, tid)
   #
   def is_alive=(call_proc)
   end

   #Only for DB >= 4.4
   #
   #The method checks for threads of control (either a true 
   #thread or a process) that have exited while manipulating Berkeley DB library 
   #data structures
   #
   #<em>flag</em> is actually unused and must be set to 0
   #
   def failcheck(flag = 0)
   end

   #Only for DB >= 4.5
   #
   #Adds a new replication site to the replication manager's list of known sites.
   #
   #Return the environment ID assigned to the remote site
   def repmgr_add_remote(host, port, flag = 0)
   end

   #Only for DB >= 4.5
   #
   #Specifies how master and client sites will handle acknowledgment of replication
   #messages which are necessary for "permanent" records.
   #
   #<em>policy</em> must be set to one of the following values 
   #<em>BDB::REPMGR_ACKS_ALL</em>, <em>BDB::REPMGR_ACKS_ALL_PEERS</em>,
   #<em>BDB::REPMGR_ACKS_NONE</em>, <em>BDB::REPMGR_ACKS_ONE</em>,
   #<em>BDB::REPMGR_ACKS_ONE_PEER</em>, <em>BDB::REPMGR_ACKS_QUORUM</em>
   def repmgr_ack_policy=(policy)
   end

   #Only for DB >= 4.5
   #
   #Returns the replication manager's client acknowledgment policy.
   def repmgr_ack_policy
   end

   #Only for DB >= 4.5
   #
   #Returns an array with the status of the sites currently known by the
   #replication manager.
   def repmgr_site_list
   end

   #Only for DB >= 4.5
   #
   #Specifies the host identification string and port number for the local system.
   def repmgr_set_local_site(host, port, flag = 0)
   end

   #Only for DB >= 4.5
   #
   #Starts the replication manager.
   def repmgr_start(count, flag)
   end

   #Only for DB >= 4.5
   #
   #Configures the Berkeley DB replication subsystem.
   #
   #<em>which</em> can have the value <em>BDB::REP_CONF_BULK</em>,
   #<em>BDB::REP_CONF_DELAYCLIENT</em>, <em>BDB::REP_CONF_NOAUTOINIT</em>,
   #<em>BDB::REP_CONF_NOWAIT</em>
   #
   #<em>onoff</em> can have the value <em>true</em> or <em>false</em>
   def rep_config[]=(which, onoff)
   end

   #Only for DB >= 4.5
   #
   #Returns <em>true</em> if the specified <em>which</em> parameter is currently set or not.
   def rep_config?[](which)
   end

   #Only for DB >= 4.5
   #
   #Specifies the total number of sites in a replication group.
   def rep_nsites=(sites)
   end

   #Only for DB >= 4.5
   #
   #Returns the total number of sites in a replication group.
   def rep_nsites
   end

   #Only for DB >= 4.5
   #
   #Specifies the priority in the replication group elections.
   def rep_priority=(priority)
   end

   #Only for DB >= 4.5
   #
   #Returns the database environment priority.
   def rep_priority
   end

   #Only for DB >= 4.5
   #
   #Specifies the timeout in the replication group elections.
   #
   #<em>which</em> can have the value <em>BDB::REP_ACK_TIMEOUT</em>,
   #<em>BDB::REP_ELECTION_TIMEOUT</em>, <em>BDB::REP_ELECTION_RETRY</em>,
   #<em>BDB::REP_CONNECTION_RETRY</em>
   def rep_timeout[]=(which, timeout)
   end

   #Only for DB >= 4.5
   #
   #Returns the database environment timeout for <em>which</em>
   def rep_timeout[](which)
   end


end

