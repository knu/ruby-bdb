=begin
= BDB

((<Download|URL:ftp://moulon.inra.fr/pub/ruby/>))

Berkeley DB is an embedded database system that supports keyed access
to data.

Developers may choose to store data in any of several different
storage structures to satisfy the requirements of a particular
application. In database terminology, these storage structures and the
code that operates on them are called access methods.

== ...............................................................

With bdb >= 0.5.5 ((|nil|)) is stored as an empty string (when marshal is not
used).

Open the database with

     "store_nil_as_null" => true

if you want the old behavior (((|nil|)) stored as `\000')
== ...............................................................


The library includes support for the following access methods:

   * B+tree: Stores keys in sorted order, using a default function that does
     lexicographical ordering of keys.

   * Hashing: Stores records in a hash table for fast searches based
     on strict equality, using a default that hashes on the key as a bit
     string. Extended Linear Hashing modifies the hash function used by the
     table as new records are inserted, in order to keep buckets underfull in
     the steady state.

   * Fixed and Variable-Length Records: Stores fixed- or
     variable-length records in sequential order. Record numbers may be
     immutable, requiring that new records be added only at the end
     of the database, or mutable, permitting new records to be inserted
     between existing records. 

((<Berkeley DB environnement|URL:docs/env.html>))

Berkeley DB environment is an encapsulation of one or more databases,
log files and shared information about the database environment such
as shared memory buffer cache pages.

Acces Methods
   
    * ((<Hash like interface|URL:docs/hashlike.html>))
      these are the common methods for BDB::Btree,
      BDB::Hash, BDB::Recno, BDB::Queue

    * ((<Array like interface|URL:docs/arraylike.html>))
      methods for BDB::Recnum


((<Sequence|URL:docs/sequence.html>))

A sequence is created with BDB::Common::create_sequence or
BDB::Common::open_sequence (only with db >= 4.3)

((<Transaction Manager|URL:docs/transaction.html>))

The transaction subsystem makes operations atomic, consistent,
isolated, and durable in the face of system and application
failures. The subsystem requires that the data be properly logged and
locked in order to attain these properties. Berkeley DB contains all
the components necessary to transaction-protect the Berkeley DB access
methods and other forms of data may be protected if they are logged
and locked appropriately.

((<Cursor operation|URL:docs/cursor.html>))

A database cursor is a sequential pointer to the database entries. It
allows traversal of the database and access to duplicate keyed
entries.  Cursors are used for operating on collections of records,
for iterating over a database, and for saving handles to individual
records, so that they can be modified after they have been read.

See also iterators in ((<Acces Methods|URL:docs/hashlike.html#each>))

((<Lock system|URL:docs/lock.html>))

The lock subsystem provides interprocess and intraprocess concurrency
control mechanisms. While the locking system is used extensively by
the Berkeley DB access methods and transaction system, it may also be
used as a stand-alone subsystem to provide concurrency control to any
set of designated resources.

((<Logging subsystem|URL:docs/log.html>))

The logging subsystem is the logging facility used by Berkeley DB. It
is largely Berkeley DB specific, although it is potentially useful
outside of the Berkeley DB package for applications wanting
write-ahead logging support. Applications wanting to use the log for
purposes other than logging file modifications based on a set of open
file descriptors will almost certainly need to make source code
modifications to the Berkeley DB code base.

=end
