=begin

== Cursor operation

A database cursor is a sequential pointer to the database entries. It
allows traversal of the database and access to duplicate keyed
entries.  Cursors are used for operating on collections of records,
for iterating over a database, and for saving handles to individual
records, so that they can be modified after they have been read.

# module BDB
## A database cursor is a sequential pointer to the database entries. It
## allows traversal of the database and access to duplicate keyed
## entries.  Cursors are used for operating on collections of records,
## for iterating over a database, and for saving handles to individual
## records, so that they can be modified after they have been read.
# class Cursor

See also iterators in ((<Acces Methods|URL:docs/access.html#each>))

=== Methods

--- close()
--- c_close()
    Discards the cursor.

--- count()
--- c_count()
    Return the count of duplicate

--- current()
--- c_current()
    Same than (({get(BDB::CURRENT)}))

--- del()
--- delete()
--- c_del()
    Deletes the key/data pair currently referenced by the cursor.

--- dup(flags = 0)
--- clone(flags = 0)
--- c_dup(flags = 0)
--- c_clone(flags = 0)
    Creates new cursor that uses the same transaction and locker ID as
    the original cursor. This is useful when an application is using
    locking and requires two or more cursors in the same thread of
    control.

    ((|flags|)) can have the value ((|BDB::DB_POSITION|)), in this case the
    newly created cursor is initialized to reference the same position in
    the database as the original cursor and hold the same locks.

--- first()
--- c_first()
    Same than (({get(BDB::FIRST)}))

--- get(flags, key = nil, value = nil)
--- c_get(flags, key = nil, value = nil)
    Retrieve key/data pair from the database

    See the description of (({c_get})) in the Berkeley distribution
    for the different values of the ((|flags|)) parameter.

    ((|key|)) must be given if the ((|flags|)) parameter is 
    ((|BDB::SET|)) | ((|BDB::SET_RANGE|)) | ((|BDB::SET_RECNO|))

    ((|key|)) and ((|value|)) must be specified for ((|BDB::GET_BOTH|))

--- last()
--- c_last()
    Same than (({get(BDB::LAST)}))

--- next()
--- c_next()
    Same than (({get(BDB::NEXT)}))

--- pget(flags, key = nil, value = nil)
--- c_pget(flags, key = nil, value = nil)
    Retrieve key/primary key/data pair from the database

--- prev()
--- c_prev()
    Same than (({get(BDB::PREV)}))

--- put(flags, value)
--- c_put(flags, value)
    Stores data value into the database.

    See the description of (({c_put})) in the Berkeley distribution
    for the different values of the ((|flags|)) parameter.

--- put(flags, key, value)
--- c_put(flags, key, value)
    Stores key/data pairs into the database (only for Btree and Hash
    access methods)

    ((|flags|)) must have the value ((|BDB::KEYFIRST|)) or
    ((|BDB::KEYLAST|))

--- set(key)
--- c_set(key)
--- set_range(key)
--- c_set_range(key)
--- set_recno(key)
--- c_set_recno(key)
    Same than (({get})) with the flags ((|BDB::SET|)) or ((|BDB::SET_RANGE|))
    or ((|BDB::SET_RECNO|))

# end
# end
=end
