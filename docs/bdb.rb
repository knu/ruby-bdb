# Berkeley DB is an embedded database system that supports keyed access
# to data.
# 
# Developers may choose to store data in any of several different
# storage structures to satisfy the requirements of a particular
# application. In database terminology, these storage structures and the
# code that operates on them are called access methods.
module BDB
#    * B+tree: Stores keys in sorted order, using a default function that does
#      lexicographical ordering of keys.
class Btree < Common
end
#    * Hashing: Stores records in a hash table for fast searches based
#      on strict equality, using a default that hashes on the key as a bit
#      string. Extended Linear Hashing modifies the hash function used by the
#      table as new records are inserted, in order to keep buckets underfull in
#      the steady state.
class Hash < Common
end
#    * Fixed and Variable-Length Records: Stores fixed- or
#      variable-length records in sequential order. Record numbers may be
#      immutable, requiring that new records be added only at the end
#      of the database, or mutable, permitting new records to be inserted
#      between existing records. 
class Recno < Common
end
# Berkeley DB environment is an encapsulation of one or more databases,
# log files and shared information about the database environment such
# as shared memory buffer cache pages.
class Env
end
# The transaction subsystem makes operations atomic, consistent,
# isolated, and durable in the face of system and application
# failures. The subsystem requires that the data be properly logged and
# locked in order to attain these properties. Berkeley DB contains all
# the components necessary to transaction-protect the Berkeley DB access
# methods and other forms of data may be protected if they are logged
# and locked appropriately.
class Txn
end
# A database cursor is a sequential pointer to the database entries. It
# allows traversal of the database and access to duplicate keyed
# entries.  Cursors are used for operating on collections of records,
# for iterating over a database, and for saving handles to individual
# records, so that they can be modified after they have been read.
class Cursor
end
# The lock subsystem provides interprocess and intraprocess concurrency
# control mechanisms. While the locking system is used extensively by
# the Berkeley DB access methods and transaction system, it may also be
# used as a stand-alone subsystem to provide concurrency control to any
# set of designated resources.
class Lock
end
# The logging subsystem is the logging facility used by Berkeley DB. It
# is largely Berkeley DB specific, although it is potentially useful
# outside of the Berkeley DB package for applications wanting
# write-ahead logging support. Applications wanting to use the log for
# purposes other than logging file modifications based on a set of open
# file descriptors will almost certainly need to make source code
# modifications to the Berkeley DB code base.
class Log
end
end
module BDB
# Don't mix these methods with methods of ((|BDB::Cursor|))
 #
 # All instance methods has the same syntax than the methods of Array
class Recnum < Common
class << self

#open the database
#
#
#BDB::Recnum.open(name, subname, flags, mode)
#
#is equivalent to
#
#BDB::Recno.open(name, subname, flags, mode,
#"set_flags" => BDB::RENUMBER,
#"set_array_base" => 0)
#
def  open(name = nil, subname = nil, flags = 0, mode = 0, options = {})
end
alias  create  open
alias  new  open

#Removes the database (or subdatabase) represented by the
#name and subname combination.
#
#If no subdatabase is specified, the physical file represented by name
#is removed, incidentally removing all subdatabases that it contained.
#
def  remove(name, subname = nil) 
end
alias  db_remove  remove
alias  unlink  remove

#Upgrade the database
#
def  upgrade(name)
end
alias  db_upgrade  upgrade
end

#
def  self[nth] 
end

#
def  self[start..end] 
end

#
def  self[start, length] 
end

#
def  self[nth] = val 
end

#
def  self[start..end] = val 
end

#
def  self[start, length] = val 
end

#
def  self + other 
end

#
def  self * times 
end

#
def  self - other 
end

#
def  self & other 
end

#
def  self | other 
end

#
def  self << obj 
end

#
def  self <=> other 
end

#
def  clear 
end

#
def  concat(other) 
end

#
def  delete(val) 
end

#
def  delete_at(pos) 
end

#
def  delete_if {...} 
end

#
def  reject! 
yield x
end

#
def  each {...} 
end

#
def  each_index {...} 
end

#
def  empty? 
end

#
def  fill(val) 
end

#
def  fill(val, start[, length]) 
end

#
def  fill(val, start..end) 
end

#
def  filter 
yield item
end

#
def  freeze 
end

#
def  frozen 
end

#
def  include?(val) 
end

#
def  index(val) 
end

#
def  indexes(index_1,..., index_n) 
end

#
def  indices(index_1,..., index_n) 
end

#
def  join([sep]) 
end

# 
def  length
end

#
def  size 
end

#
def  nitems 
end

#
def  pop 
end

#
def  push(obj...) 
end

#
def  replace(other) 
end

#
def  reverse 
end

#
def  reverse! 
end

#
def  reverse_each {...} 
end

#
def  rindex(val) 
end

#
def  shift 
end

#
def  unshift(obj) 
end
end
end
module BDB
 # A database cursor is a sequential pointer to the database entries. It
 # allows traversal of the database and access to duplicate keyed
 # entries.  Cursors are used for operating on collections of records,
 # for iterating over a database, and for saving handles to individual
 # records, so that they can be modified after they have been read.
class Cursor

#Discards the cursor.
#
def  close()
end
alias  c_close  close

#Return the count of duplicate
#
def  count()
end
alias  c_count  count

#Same than <tt>get(BDB::CURRENT)</tt>
#
def  current()
end
alias  c_current  current

#Deletes the key/data pair currently referenced by the cursor.
#
def  del()
end
alias  delete  del
alias  c_del  del

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
alias  clone  dup
alias  c_dup  dup
alias  c_clone  dup

#Same than <tt>get(BDB::FIRST)</tt>
#
def  first()
end
alias  c_first  first

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
alias  c_get  get

#Same than <tt>get(BDB::LAST)</tt>
#
def  last()
end
alias  c_last  last

#Same than <tt>get(BDB::NEXT)</tt>
#
def  next()
end
alias  c_next  next

#Retrieve key/primary key/data pair from the database
#
def  pget(flags, key = nil, value = nil)
end
alias  c_pget  pget

#Same than <tt>get(BDB::PREV)</tt>
#
def  prev()
end
alias  c_prev  prev

#Stores data value into the database.
#
#See the description of <tt>c_put</tt> in the Berkeley distribution
#for the different values of the <em>flags</em> parameter.
#
def  put(flags, value)
end
alias  c_put  put

#Stores key/data pairs into the database (only for Btree and Hash
#access methods)
#
#<em>flags</em> must have the value <em>BDB::KEYFIRST</em> or
#<em>BDB::KEYLAST</em>
#
def  put(flags, key, value)
end
alias  c_put  put

#Same than <tt>get</tt> with the flags <em>BDB::SET</em> or <em>BDB::SET_RANGE</em>
#or <em>BDB::SET_RECNO</em>
#
def  set(key)
end
alias  c_set  set
alias  set_range  set
alias  c_set_range  set
alias  set_recno  set
alias  c_set_recno  set
end
end
module BDB
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
class Env
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
#    Equivalent to ((|DB_INIT_LOCK | DB_INIT_MPOOL | DB_INIT_TXN | DB_INIT_LOG|))
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
#  hash. See the documentation of Berkeley DB for possible values.
#
def  open(home, flags = 0, mode = 0, options = {})
end
alias  create  open
alias  new  open

#remove the environnement
#
def  remove()
end
alias  unlink  remove
end

#close the environnement
#
def  close()
end

#return the name of the directory
#
def  home()
end

#Acquire a locker ID
#
def  lock()
end
alias  lock_id  lock

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
#<em>BDB::QUEUE</em>. See <<???open|URL:access.html#open???>> for other
#arguments
#
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
alias  txn_begin  begin
alias  begin  begin

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
alias  txn_checkpoint  checkpoint

#Return transaction subsystem statistics
#
#
def  stat()
end
alias  txn_stat  stat

#Only for DB >= 4
#
#Holds an election for the master of a replication group, returning the
#new master's ID
#
#Raise <em>BDB::RepUnavail</em> if the <em>timeout</em> expires
#
def  elect(sites, priority, timeout)
end
alias  rep_elect  elect

#Only for DB >= 4
#
#Processes an incoming replication message sent by a member of the
#replication group to the local database environment
#
def  process_message(control, rec, envid)
end
alias  rep_process_message  process_message

#Only for DB >= 4
#
#<em>cdata</em> is an identifier
#<em>flags</em> must be one of <em>BDB::REP_CLIENT</em>, <em>BDB::REP_MASTER</em>
#or <em>BDB::REP_LOGSONLY</em>
#
def  start(cdata, flags)
end
alias  rep_start  start
end
end
 # Berkeley DB is an embedded database system that supports keyed access
 # to data.
module BDB
 # Implementation of a sorted, balanced tree structure
class Btree < Common
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
class Hash < Common
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
class Recno < Common
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
class Queue < Common
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
 # Implement common methods for access to data
class Common
class << self

#open the database
#
#* <em>name</em>
#  The argument name is used as the name of a single physical
#  file on disk that will be used to back the database.
#
#* <em>subname</em>
#  The subname argument allows applications to have
#  subdatabases, i.e., multiple databases inside of a single physical
#  file. This is useful when the logical databases are both
#  numerous and reasonably small, in order to avoid creating a large
#  number of underlying files. It is an error to attempt to open a
#  subdatabase in a database file that was not initially
#  created using a subdatabase name.
#
#* <em>flags</em>
#  The flags must be the string "r", "r+", "w", "w+", "a", "a+" or
#  and integer value.
#
#  The flags value must be set to 0 or by bitwise inclusively
#  OR'ing together one or more of the following values
#
#  * <em>BDB::CREATE</em>
#    Create any underlying files, as necessary. If the files
#    do not already exist and the DB_CREATE flag is not
#    specified, the call will fail. 
#
#  * <em>BD::EXCL</em>
#    Return an error if the database already exists. Underlying
#    filesystem primitives are used to implement this
#    flag. For this reason it is only applicable to the
#    physical database file and cannot be used to test if a
#    subdatabase already exists. 
#
#  * <em>BDB::NOMMAP</em>
#    Do not map this database into process memory.
#
#  * <em>BDB::RDONLY</em>
#    Open the database for reading only. Any attempt to
#    modify items in the database will fail regardless of the
#    actual permissions of any underlying files. 
#
#  * <em>BDB::TRUNCATE</em>
#    Physically truncate the underlying database file,
#    discarding all previous subdatabases or databases.
#    Underlying filesystem primitives are used to implement
#    this flag. For this reason it is only applicable to the
#    physical database file and cannot be used to discard
#    subdatabases.
#
#    The DB_TRUNCATE flag cannot be transaction protected,
#    and it is an error to specify it in a transaction
#    protected environment. 
#
#* <em>options</em>
#  Hash, Possible options are (see the documentation of Berkeley DB
#  for more informations) 
#
#  * <em>set_array_base</em>: base index for BDB::Recno, BDB::Queue or BDB::Btree (with BDB::RECNUM). Must be 0 or 1
#  * <em>set_bt_compare</em> :  specify a Btree comparison function 
#  * <em>set_bt_minkey</em> :   set the minimum number of keys per Btree page
#  * <em>set_bt_prefix</em> :   specify a Btree prefix comparison function 
#  * <em>set_cachesize</em> :   set the database cache size
#  * <em>set_dup_compare</em> :  specify a duplicate comparison function 
#  * <em>set_store_key</em> : specify a Proc called before a key is stored
#  * <em>set_fetch_key</em> : specify a Proc called after a key is read
#  * <em>set_store_value</em> : specify a Proc called before a value is stored
#  * <em>set_fetch_value</em> : specify a Proc called after a value is read
#  * <em>set_flags</em> :  general database configuration
#  * <em>set_h_ffactor</em> :  set the Hash table density
#  * <em>set_h_hash</em> :  specify a hashing function 
#  * <em>set_h_nelem</em> :  set the Hash table size
#  * <em>set_lorder</em> :  set the database byte order
#  * <em>set_pagesize</em> :  set the underlying database page size
#  * <em>set_re_delim</em> :  set the variable-length record delimiter
#  * <em>set_re_len</em> :   set the fixed-length record length
#  * <em>set_re_pad</em> :   set the fixed-length record pad byte
#  * <em>set_re_source</em> :  set the backing Recno text file
#  * <em>env</em> :  open the database in the environnement given as the value
#  * <em>txn</em> :  open the database in the transaction given as the value
#
#  Proc given to <em>set_bt_compare</em>, <em>set_bt_prefix</em>, 
#  <em>set_dup_compare</em>, <em>set_h_hash</em>, <em>set_store_key</em>
#  <em>set_fetch_key</em>, <em>set_store_value</em> and <em>set_fetch_value</em>
#  can be also specified as a method (replace the prefix <em>set_</em> 
#  with <em>bdb_</em>)
#
#      For example 
#
#        module BDB
#           class Btreesort < Btree
#              def bdb_bt_compare(a, b)
#                 b.downcase <=> a.downcase
#              end
#           end
#        end
#
def  open(name = nil, subname = nil, flags = 0, mode = 0, options = {})
end
alias  create  open
alias  new  open

#Removes the database (or subdatabase) represented by the
#name and subname combination.
#
#If no subdatabase is specified, the physical file represented by name
#is removed, incidentally removing all subdatabases that it contained.
#
def  remove(name, subname = nil) 
end
alias  db_remove  remove
alias  unlink  remove

#Upgrade the database
#
def  upgrade(name)
end
alias  db_upgrade  upgrade
end

#Returns the value corresponding the <em>key</em>
#
def  self[key]
end

#associate a secondary index db
#
#<em>flag</em> can have the value <em>BDB::RDONLY</em>
#
#The block must return the new key, or <em>Qfalse</em> in this case the
#secondary index will not contain any reference to key/value
#
def  associate(db, flag = 0) 
yield db, key, value
end

#Returns the value correspondind the <em>key</em>
#
#<em>flags</em> can have the values <em>BDB::GET_BOTH</em>, 
#<em>BDB::SET_RECNO</em> or <em>BDB::RMW</em>
#
#In presence of duplicates it will return the first data item, use
##duplicates if you want all duplicates (see also #each_dup)
#
def  get(key, flags = 0)
end
alias  db_get  get
alias  fetch  get

#Returns the primary key and the value corresponding to <em>key</em>
#in the secondary index
#
#only with >= 3.3.11
#
def  pget(key, flags = 0)
end

#Stores the <em>value</em> associating with <em>key</em>
#
#If <em>nil</em> is given as the value, the association from the key will be
#removed. 
#
def  self[key] = value
end

#Stores the <em>value</em> associating with <em>key</em>
#
#If <em>nil</em> is given as the value, the association from the <em>key</em>
#will be removed. It return the object deleted or <em>nil</em> if the
#specified key don't exist.
#
#<em>flags</em> can have the value <em>DBD::NOOVERWRITE</em>, in this case
#it will return <em>nil</em> if the specified key exist, otherwise <em>true</em>
#
def  put(key, value, flags = 0)
end
alias  db_put  put
alias  store  put

#Append the <em>value</em> associating with <em>key</em>
#
def  append(key, value)
end
alias  db_append  append

#Return if the underlying database is in host order
#
def  byteswapped?
end
alias  get_byteswapped  byteswapped?

#Clear partial set.
#
def  clear_partial
end
alias  partial_clear  clear_partial

#Closes the file.
#
def  close(flags = 0)
end
alias  db_close  close

#Return the count of duplicate for <em>key</em>
#
def  count(key)
end
alias  dup_count  count

#Open a new cursor.
#
def  cursor()
end
alias  db_cursor  cursor

#Return the subname
# 
def  database()
end
alias  subname  database

#Removes the association from the key. 
#
#It return the object deleted or <em>nil</em> if the specified
#key don't exist.
#
def  delete(key)
end
alias  db_del  delete

#Deletes associations if the evaluation of the block returns true. 
#
#<<???set???>>
#
def  delete_if(set = nil) 
yield key, value
end
alias  reject!  delete_if

#Return an array of all duplicate associations for <em>key</em>
#
#if <em>assoc</em> is <em>false</em> return only the values.
#
def  duplicates(key , assoc = true)
end

#Iterates over associations.
#
#<<???set???>> <<???bulk???>>
#
def  each(set = nil, bulk = 0]) 
yield key, value
end
alias  each_pair  each

#Iterates over each duplicate associations for <em>key</em>
#
#<<???bulk???>>
#
def  each_dup(key, bulk = 0) 
yield key, value
end

#Iterates over each duplicate values for <em>key</em>
#
#<<???bulk???>>
#
def  each_dup_value(key, bulk = 0) 
yield value
end

#Iterates over keys. 
#
#<<???set???>> <<???bulk???>>
#
def  each_key(set = nil, bulk = 0) 
yield key
end

#Iterates over secondary indexes and give secondary key, primary key
#and value
#
def  each_primary(set = nil) 
yield skey, pkey, pvalue
end

#Iterates over values. 
#
#<<???set???>> <<???bulk???>>
#
def  each_value(set = nil, bulk = 0) 
yield value
end

#Returns true if the database is empty. 
#
def  empty?() 
end

#Return the name of the file
#
def  filename()
end

#Returns true if the association from the <em>key</em> exists.
#
def  has_key?(key) 
end
alias  key?  has_key?
alias  include?  has_key?
alias  member?  has_key?

#Returns true if the association from <em>key</em> is <em>value</em> 
#
def  has_both?(key, value)
end
alias  both?  has_both?

#Returns true if the association to the <em>value</em> exists. 
#
def  has_value?(value) 
end
alias  value?  has_value?

#Returns the first <em>key</em> associated with <em>value</em>
#
def  index(value)
end

#Returns the <em>keys</em> associated with <em>value1, value2, ...</em>
#
def  indexes(value1, value2, )
end

#Perform a join. <em>cursor</em> is an array of <em>BDB::Cursor</em>
#
def  join(cursor , flag = 0) 
yield key, value
end

#Returns the array of the keys in the database
#
def  keys 
end

#Returns the number of association in the database.
#
def  length 
end
alias  size   length 

#  
#The <em>log_register</em> function registers a file <em>name</em>.
#
def  log_register(name)
end

#  
#The <em>log_unregister</em> function unregisters a file name.
#
def  log_unregister()
end

#Create an hash without the associations if the evaluation of the
#block returns true. 
#
def  reject 
yield key, value
end

#Iterates over associations in reverse order 
#
#<<???set???>>
#
def  reverse_each(set = nil) 
yield key, value
end
alias  reverse_each_pair  reverse_each

#Iterates over keys in reverse order 
#
#<<???set???>>
#
def  reverse_each_key(set = nil) 
yield key
end

#Iterates over secondary indexes in reverse order and give secondary
#key, primary key and value
#
def  reverse_each_primary(set = nil) 
yield skey, pkey, pvalue
end

#Iterates over values in reverse order.
#
#<<???set???>>
#
def  reverse_each_value(set = nil) 
yield value
end

#Set the partial value <em>len</em> and <em>offset</em>
#
def  set_partial(len, offset)
end

#Return database statistics.
#
def  stat
end

#Return an array of all associations [key, value]
#
def  to_a
end

#Return an hash of all associations {key => value}
#
def  to_hash
end

#Empty a database
#       
def  truncate
end
alias  clear  truncate

#Returns the array of the values in the database.
#
def  values 
end

#Verify the integrity of the DB file, and optionnally output the
#key/data to <em>file</em> (file must respond to #to_io)
#
def  verify(file = nil, flags = 0)
end
end
class Recno < Common
#Returns the last couple [key, val] (only for BDB::Recno)
#
def  pop
end
end
class Queue < Common
#Returns the last couple [key, val] (only for BDB::Recno)
#
def  pop
end
end
class Recno < Common
#Push the values
#
def  push values
end
end
class Queue < Common
#Push the values
#
def  push values
end
end
class Recno < Common
#Removes and returns an association from the database.
#
def  shift 
end
end
class Queue < Common
#Removes and returns an association from the database.
#
def  shift 
end
end
end
module BDB
# 
# The lock subsystem provides interprocess and intraprocess concurrency
# control mechanisms. While the locking system is used extensively by
# the Berkeley DB access methods and transaction system, it may also be
# used as a stand-alone subsystem to provide concurrency control to any
# set of designated resources.
class Lockid
end
# Exception generated by lock call
class LockError < Exception
end
# Lock not held by locker
class LockHeld < LockError
end
# Lock not granted
class LockGranted < LockError
end
# Locker killed to resolve a deadlock
class LockDead < LockError
end
class Lockid

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
alias  lock_get  get

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
alias  lock_vec  vec
end
class Lock

#The <em>lock_put</em> function releases lock from the lock table. 
#
def  put()
end
alias  lock_put  put
alias  release  put
alias  delete  put
end
end
module BDB
# This subsystem is used when recovery from application or system
# failure is necessary.
# 
# The log is stored in one or more files in the environment
# directory. Each file is named using the format log.NNNNNNNNNN, where
# NNNNNNNNNN is the sequence number of the file within the log.
# 
# If the log region is being created and log files are already present,
# the log files are reviewed and subsequent log writes are appended to
# the end of the log, rather than overwriting current log entries.
# 
# 
# The lock subsystem is created, initialized, and opened by calls to
# (({BDB::Env#open})) with the ((|BDB::INIT_LOG|)) flag specified. 
# 
# The following options can be given when the environnement is created
# 
#   : ((|"set_lg_bsize"|))
#     Set log buffer size
# 
#   : ((|"set_lg_dir"|))
#     Set the environment logging directory
# 
#   : ((|"set_lg_max"|))
#     Set log file size
class Env

#The log_archive function return an array of log or database file names.
#
#<em>flags</em> value must be set to 0 or the value <em>BDB::ARCH_DATA</em>
#<em>BDB::ARCH_DATA</em>, <em>BDB::ARCH_LOG</em>
#
def  lock_archive(flags = 0)
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
def  log_flush([string])
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
end
class Common

#  
#The <em>log_register</em> function registers a file <em>name</em>.
#
def  log_register(name)
end

#  
#The <em>log_unregister</em> function unregisters a file name.
def  log_unregister()
end
end
class Lsn
include Comparable

#
#compare 2 <em>BDB::Lsn</em>
#
def  <=>
end

#
#The <em>log_file</em> function maps <em>BDB::Lsn</em> structures to file 
#<em>name</em>s
#
def  log_file(name)
end
alias  file  log_file

#
#The <em>log_flush</em> function guarantees that all log records whose
#<em>DBB:Lsn</em> are less than or equal to the current lsn have been written
#to disk.
#
def  log_flush
end
alias  flush  log_flush

#
#return the <em>String</em> associated with this <em>BDB::Lsn</em>
#
def  log_get
end
alias  get  log_get
end
end
module BDB
# 
# The transaction subsystem makes operations atomic, consistent,
# isolated, and durable in the face of system and application
# failures. The subsystem requires that the data be properly logged and
# locked in order to attain these properties. Berkeley DB contains all
# the components necessary to transaction-protect the Berkeley DB access
# methods and other forms of data may be protected if they are logged
# and locked appropriately.
# 
# The transaction subsystem is created, initialized, and opened by calls
# to ((<BDB::Env#open|URL:env.html#open>)) with the ((|BDB::INIT_TXN|))
# flag (or ((|BDB::INIT_TRANSACTION|))) specified.
# Note that enabling transactions automatically enables
# logging, but does not enable locking, as a single thread of control
# that needed atomicity and recoverability would not require it.
# 
# The following option can be given when the environnement is created
# 
#  : ((|"set_tx_max"|))
#    Set maximum number of transactions
# 
# and with DB >= 4.0
# 
#  : ((|"set_timeout"|))
#  : ((|"set_txn_timeout"|))
#  : ((|"set_lock_timeout"|))
# 
# The transaction is created with ((<BDB::Env#begin|URL:env.html#begin>))
# or with ((<begin>)) 
class Txn

#Abort the transaction. This is will terminate the transaction.
#
def  abort()
end
alias  txn_abort  abort

#Associate a database with the transaction, return a new database
#handle which is transaction protected.
#
def  assoc(db, ...)
end
alias  associate  assoc
alias  txn_assoc  assoc

#begin a transaction (the transaction manager must be enabled). flags
#can have the value <em>DBD::TXN_COMMIT</em>, in this case the transaction
#will be commited at end.
#
#Return a new transaction object, and the associated database handle
#if specified.
#
#If <em>#begin</em> is called as an iterator, <em>#commit</em> and <em>#abort</em>
#will  terminate the iterator.
#
#env.begin(db) do |txn, b|
#...
#end
#
#is the same than
#
#env.begin do |txn|
#b = txn.assoc(db)
#...
#end
#
#An optional hash can be given with the possible keys <em>"flags"</em>,
#<em>"set_timeout"</em>, <em>"set_txn_timeout"</em>, <em>"set_lock_timeout"</em>
#    
#
def  begin(flags = 0, db, ...) 
yield txn, db, ...
end
alias  begin  begin
alias  txn_begin  begin

#Commit the transaction. This will finish the transaction.
#The <em>flags</em> can have the value 
#
#<em>BDB::TXN_SYNC</em> Synchronously flush the log. This means the
#transaction will exhibit all of the ACID (atomicity, consistency
#and isolation and durability) properties. This is the default value.
#
#<em>BDB::TXN_NOSYNC</em> Do not synchronously flush the log. This
#means the transaction will exhibit the ACI (atomicity, consistency
#and isolation) properties, but not D (durability), i.e., database
#integrity will be maintained but it is possible that this
#transaction may be undone during recovery instead of being redone.
#
#This behavior may be set for an entire Berkeley DB environment as
#part of the open interface.
#
def  commit(flags = 0)
end
alias  close  commit
alias  txn_commit  commit
alias  txn_close  commit

#only with BDB::VERSION_MAJOR == 3 && BDB::VERSION_MINOR >= 3
#
#Discard a prepared but not resolved transaction handle, must be called
#only within BDB::Env#recover
#
def  discard
end
alias  txn_discard  discard

#The txn_id function returns the unique transaction id associated
#with the specified transaction. Locking calls made on behalf of
#this transaction should use the value returned from txn_id as the
#locker parameter to the lock_get or lock_vec calls.
#
def  id()
end
alias  txn_id  id

#The txn_prepare function initiates the beginning of a two-phase commit.
#
#In a distributed transaction environment, Berkeley DB can be used
#as a local transaction manager. In this case, the distributed
#transaction manager must send prepare messages to each local
#manager. The local manager must then issue a txn_prepare and await its
#successful return before responding to the distributed transaction
#manager. Only after the distributed transaction manager receives
#successful responses from all of its prepare messages should it issue
#any commit messages.
#
def  prepare()
end
alias  txn_prepare  prepare
alias  prepare  prepare
alias  txn_prepare  prepare
end
end
