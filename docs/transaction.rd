=begin
== BDB::Txn

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

The following option can be given when the environnement is created

 : ((|"set_tx_max"|))
   Set maximum number of transactions

The transaction is created with ((<BDB::Env#begin|URL:env.html#begin>))
or with ((<begin>)) 

See also ((<BDB::Env#txn_stat|URL:env.html#txn_stat>)) and
((<BDB::Env#txn_checkpoint|URL:env.html#txn_checkpoint>))

=== Methods

--- abort()
--- txn_abort()
    Abort the transaction. This is will terminate the transaction.

--- assoc(db [, db, ...])
--- associate(db [, db, ...])
--- txn_assoc(db [, db, ...])
    Associate a database with the transaction, return a new database
    handle which is transaction protected.

--- begin([flags, db, ...])
--- begin([flags, db, ...]) { |txn [, db, ...]| ...}
--- txn_begin([flags, db, ...])
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


--- commit([flags])
--- close([flags])
--- txn_commit([flags])
--- txn_close([flags])
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

--- id()
--- txn_id()
     The txn_id function returns the unique transaction id associated
     with the specified transaction. Locking calls made on behalf of
     this transaction should use the value returned from txn_id as the
     locker parameter to the lock_get or lock_vec calls.

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

=end
