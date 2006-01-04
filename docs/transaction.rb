# 
# The transaction subsystem makes operations atomic, consistent,
# isolated, and durable in the face of system and application
# failures. The subsystem requires that the data be properly logged and
# locked in order to attain these properties. Berkeley DB contains all
# the components necessary to transaction-protect the Berkeley DB access
# methods and other forms of data may be protected if they are logged
# and locked appropriately.
# 
# 
# The transaction subsystem is created, initialized, and opened by calls
# to <em>BDB::Env#open</em> with the <em>BDB::INIT_TXN</em>
# flag (or <em>BDB::INIT_TRANSACTION</em>) specified.
# Note that enabling transactions automatically enables
# logging, but does not enable locking, as a single thread of control
# that needed atomicity and recoverability would not require it.
# 
# The transaction is created with <em>BDB::Env#begin</em>
# or with <em>begin</em> 
class BDB::Txn
   
   #Abort the transaction. This is will terminate the transaction.
   #
   def  abort()
   end
   #same than <em> abort</em>
   def  txn_abort()
   end
   
   #Associate a database with the transaction, return a new database
   #handle which is transaction protected.
   #
   def  assoc(db, ...)
   end
   #same than <em> assoc</em>
   def  associate(db, ...)
   end
   #same than <em> assoc</em>
   def  txn_assoc(db, ...)
   end
   
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
   #same than <em> begin</em>
   def  txn_begin(flags = 0, db, ...)
   end
   
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
   #same than <em> commit</em>
   def  close(flags = 0)
   end
   #same than <em> commit</em>
   def  txn_commit(flags = 0)
   end
   #same than <em> commit</em>
   def  txn_close(flags = 0)
   end
   
   #only with BDB::VERSION_MAJOR == 3 && BDB::VERSION_MINOR >= 3
   #
   #Discard a prepared but not resolved transaction handle, must be called
   #only within BDB::Env#recover
   #
   def  discard
   end
   #same than <em> discard</em>
   def  txn_discard
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
   
   #The txn_id function returns the unique transaction id associated
   #with the specified transaction. Locking calls made on behalf of
   #this transaction should use the value returned from txn_id as the
   #locker parameter to the lock_get or lock_vec calls.
   #
   def  id()
   end
   #same than <em> id</em>
   def  txn_id()
   end
   
   #Only with DB >= 4.1
   #
   #open the database in the current transaction. type must be one of
   #the constant <em>BDB::BTREE</em>, <em>BDB::HASH</em>, <em>BDB::RECNO</em>, 
   #<em>BDB::QUEUE</em>. See <em>open</em> for other
   #arguments
   #
   def  open_db(type, name = nil, subname = nil, flags = 0, mode = 0)
   end
   
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
   #same than <em> prepare</em>
   def  txn_prepare()
   end
   #same than <em> prepare</em>
   def  txn_prepare(id)    # version 3.3.11
   end

   #Only with DB >= 4.4
   #
   #Return the string associated with a transaction
   #
   def name
   end

   #Only with DB >= 4.4
   #
   #Set the string associated with a transaction
   #
   def name=(string)
   end
   
end

