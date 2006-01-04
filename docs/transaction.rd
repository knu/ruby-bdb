=begin
== BDB::Txn

# module BDB
#^

The transaction subsystem makes operations atomic, consistent,
isolated, and durable in the face of system and application
failures. The subsystem requires that the data be properly logged and
locked in order to attain these properties. Berkeley DB contains all
the components necessary to transaction-protect the Berkeley DB access
methods and other forms of data may be protected if they are logged
and locked appropriately.


The transaction subsystem is created, initialized, and opened by calls
to ((<BDB::Env#open|URL:env.html#open>)) with the ((|BDB::INIT_TXN|))
flag (or ((|BDB::INIT_TRANSACTION|))) specified.
Note that enabling transactions automatically enables
logging, but does not enable locking, as a single thread of control
that needed atomicity and recoverability would not require it.

#^
The following option can be given when the environnement is created

 : ((|"set_tx_max"|))
   Set maximum number of transactions

and with DB >= 4.0

 : ((|"set_timeout"|))
 : ((|"set_txn_timeout"|))
 : ((|"set_lock_timeout"|))

#^
The transaction is created with ((<BDB::Env#begin|URL:env.html#begin>))
or with ((<begin>)) 
#^

See also ((<BDB::Env#txn_stat|URL:env.html#txn_stat>)) and
((<BDB::Env#txn_checkpoint|URL:env.html#txn_checkpoint>))
# class Txn

=== Methods

--- abort()
--- txn_abort()
    Abort the transaction. This is will terminate the transaction.

--- assoc(db, ...)
--- associate(db, ...)
--- txn_assoc(db, ...)
    Associate a database with the transaction, return a new database
    handle which is transaction protected.

--- begin(flags = 0, db, ...) { |txn, db, ...| ...}
--- begin(flags = 0, db, ...)
--- txn_begin(flags = 0, db, ...)
     begin a transaction (the transaction manager must be enabled). flags
     can have the value ((|DBD::TXN_COMMIT|)), in this case the transaction
     will be commited at end.

     Return a new transaction object, and the associated database handle
     if specified.

     If ((|#begin|)) is called as an iterator, ((|#commit|)) and ((|#abort|))
     will  terminate the iterator.

        env.begin(db) do |txn, b|
        ...
        end

        is the same than

        env.begin do |txn|
            b = txn.assoc(db)
            ...
        end

     An optional hash can be given with the possible keys ((|"flags"|)),
     ((|"set_timeout"|)), ((|"set_txn_timeout"|)), ((|"set_lock_timeout"|))
    

--- commit(flags = 0)
--- close(flags = 0)
--- txn_commit(flags = 0)
--- txn_close(flags = 0)
     Commit the transaction. This will finish the transaction.
     The ((|flags|)) can have the value 

     ((|BDB::TXN_SYNC|)) Synchronously flush the log. This means the
     transaction will exhibit all of the ACID (atomicity, consistency
     and isolation and durability) properties. This is the default value.

     ((|BDB::TXN_NOSYNC|)) Do not synchronously flush the log. This
     means the transaction will exhibit the ACI (atomicity, consistency
     and isolation) properties, but not D (durability), i.e., database
     integrity will be maintained but it is possible that this
     transaction may be undone during recovery instead of being redone.

     This behavior may be set for an entire Berkeley DB environment as
     part of the open interface.

--- discard
--- txn_discard
     only with BDB::VERSION_MAJOR == 3 && BDB::VERSION_MINOR >= 3

     Discard a prepared but not resolved transaction handle, must be called
     only within BDB::Env#recover

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

--- id()
--- txn_id()
     The txn_id function returns the unique transaction id associated
     with the specified transaction. Locking calls made on behalf of
     this transaction should use the value returned from txn_id as the
     locker parameter to the lock_get or lock_vec calls.

--- open_db(type, name = nil, subname = nil, flags = 0, mode = 0)
    Only with DB >= 4.1

    open the database in the current transaction. type must be one of
    the constant ((|BDB::BTREE|)), ((|BDB::HASH|)), ((|BDB::RECNO|)), 
    ((|BDB::QUEUE|)). See ((<open|URL:access.html#open>)) for other
    arguments

--- prepare()
--- txn_prepare()
--- prepare(id)        # version 3.3.11
--- txn_prepare(id)    # version 3.3.11
    The txn_prepare function initiates the beginning of a two-phase commit.

    In a distributed transaction environment, Berkeley DB can be used
    as a local transaction manager. In this case, the distributed
    transaction manager must send prepare messages to each local
    manager. The local manager must then issue a txn_prepare and await its
    successful return before responding to the distributed transaction
    manager. Only after the distributed transaction manager receives
    successful responses from all of its prepare messages should it issue
    any commit messages.

--- name    # version 4.4
    Return the string associated with a transaction
    
--- name=(string)  # version 4.4
    Set the string associated with a transaction

# end
# end

=end
