=begin
== Acces Methods

These are the common methods for ((|BDB::Btree|)), ((|BDB::Hash|)),
((|BDB::Recno|)) and ((|BDB::Queue|))

* ((<Class Methods>))
* ((<Methods>))
* ((<Methods specific to BDB::Recno and BDB::Queue>))

## Berkeley DB is an embedded database system that supports keyed access
## to data.
# module BDB
## Implementation of a sorted, balanced tree structure
# class Btree < Common
## Return an Hash with the fields (description for 4.0.14)
## * bt_magic : Magic number that identifies the file as a Btree database. 
## * bt_version : The version of the Btree database. 
## * bt_nkeys : the number of unique keys in the database.
## * bt_ndata : the number of key/data pairs in the database.
## * bt_pagesize : Underlying database page size, in bytes. 
## * bt_minkey : The minimum keys per page. 
## * bt_re_len : The length of fixed-length records. 
## * bt_re_pad : The padding byte value for fixed-length records. 
## * bt_levels : Number of levels in the database. 
## * bt_int_pg : Number of database internal pages. 
## * bt_leaf_pg : Number of database leaf pages. 
## * bt_dup_pg : Number of database duplicate pages. 
## * bt_over_pg : Number of database overflow pages. 
## * bt_free : Number of pages on the free list. 
## * bt_int_pgfree : Number of bytes free in database internal pages. 
## * bt_leaf_pgfree : Number of bytes free in database leaf pages. 
## * bt_dup_pgfree :  Number of bytes free in database duplicate pages. 
## * bt_over_pgfree : Number of bytes free in database overflow pages.
# def stat(flags = 0)
# end 
# end
## Implementation of Extended Linear Hashing
# class Hash < Common
## Return an Hash with the fields (description for 4.0.14)
## * hash_magic : Magic number that identifies the file as a Hash file. 
## * hash_version : The version of the Hash database. 
## * hash_nkeys : The number of unique keys in the database. 
## * hash_ndata : The number of key/data pairs in the database.
## * hash_pagesize : The underlying Hash database page (and bucket) size, in bytes. 
## * hash_nelem : The estimated size of the hash table specified at database-creation time. 
## * hash_ffactor :  The desired fill factor (number of items per bucket) specified at database-creation time. 
## * hash_buckets :  The number of hash buckets. 
## * hash_free : The number of pages on the free list. 
## * hash_bfree :The number of bytes free on bucket pages. 
## * hash_bigpages : The number of big key/data pages. 
## * hash_big_bfree : The number of bytes free on big item pages. 
## * hash_overflows : The number of overflow pages
## * hash_ovfl_free :  The number of bytes free on overflow pages. 
## * hash_dup : The number of duplicate pages. 
## * hash_dup_free : The number of bytes free on duplicate pages. 
# def stat(flags = 0)
# end
# end
## Stores both fixed and variable-length records with logical record 
## numbers as keys
# class Recno < Common
## Return an Hash with the fields (description for 4.0.14)
## * bt_magic : Magic number that identifies the file as a Btree database. 
## * bt_version : The version of the Btree database. 
## * bt_nkeys : The exact number of records in the database. 
## * bt_ndata : The exact number of records in the database.
## * bt_pagesize : Underlying database page size, in bytes. 
## * bt_minkey : The minimum keys per page. 
## * bt_re_len : The length of fixed-length records. 
## * bt_re_pad : The padding byte value for fixed-length records. 
## * bt_levels : Number of levels in the database. 
## * bt_int_pg : Number of database internal pages. 
## * bt_leaf_pg : Number of database leaf pages. 
## * bt_dup_pg : Number of database duplicate pages. 
## * bt_over_pg : Number of database overflow pages. 
## * bt_free : Number of pages on the free list. 
## * bt_int_pgfree : Number of bytes free in database internal pages. 
## * bt_leaf_pgfree : Number of bytes free in database leaf pages. 
## * bt_dup_pgfree : Number of bytes free in database duplicate pages. 
## * bt_over_pgfree : Number of bytes free in database overflow pages. 
# def stat(flags = 0)
# end
# end
## Stores fixed-length records with logical record numbers as keys.
## It is designed for fast inserts at the tail and has a special cursor
## consume operation that deletes and returns a record from the head of
## the queue
# class Queue < Common
## Return an Hash with the fields (description for 4.0.14)
## * qs_magic : Magic number that identifies the file as a Queue file. 
## * qs_version : The version of the Queue file type. 
## * qs_nkeys : The number of records in the database.
## * qs_ndata : The number of records in the database.
## * qs_pagesize : Underlying database page size, in bytes. 
## * qs_extentsize : Underlying database extent size, in pages. 
## * qs_pages : Number of pages in the database. 
## * qs_re_len : The length of the records. 
## * qs_re_pad : The padding byte value for the records. 
## * qs_pgfree : Number of bytes free in database pages. 
## * qs_first_recno : First undeleted record in the database. 
## * qs_cur_recno : Last allocated record number in the database. 
# def stat(flags = 0)
# end
# end
## Implement common methods for access to data
# class Common
# class << self

=== Class Methods

--- open(name = nil, subname = nil, flags = 0, mode = 0, options = {})
--- create(name = nil, subname = nil, flags = 0, mode = 0, options = {})
--- new(name = nil, subname = nil, flags = 0, mode = 0, options = {})
    open the database

    : ((|name|))
      The argument name is used as the name of a single physical
      file on disk that will be used to back the database.

    : ((|subname|))
      The subname argument allows applications to have
      subdatabases, i.e., multiple databases inside of a single physical
      file. This is useful when the logical databases are both
      numerous and reasonably small, in order to avoid creating a large
      number of underlying files. It is an error to attempt to open a
      subdatabase in a database file that was not initially
      created using a subdatabase name.

    : ((|flags|))
      The flags must be the string "r", "r+", "w", "w+", "a", "a+" or
      and integer value.

      The flags value must be set to 0 or by bitwise inclusively
      OR'ing together one or more of the following values

       : ((|BDB::CREATE|))
         Create any underlying files, as necessary. If the files
         do not already exist and the DB_CREATE flag is not
         specified, the call will fail. 

       : ((|BD::EXCL|))
         Return an error if the database already exists. Underlying
         filesystem primitives are used to implement this
         flag. For this reason it is only applicable to the
         physical database file and cannot be used to test if a
         subdatabase already exists. 

       : ((|BDB::NOMMAP|))
         Do not map this database into process memory.

       : ((|BDB::RDONLY|))
         Open the database for reading only. Any attempt to
         modify items in the database will fail regardless of the
         actual permissions of any underlying files. 

       : ((|BDB::TRUNCATE|))
         Physically truncate the underlying database file,
         discarding all previous subdatabases or databases.
         Underlying filesystem primitives are used to implement
         this flag. For this reason it is only applicable to the
         physical database file and cannot be used to discard
         subdatabases.

         The DB_TRUNCATE flag cannot be transaction protected,
         and it is an error to specify it in a transaction
         protected environment. 

    : ((|options|))
      Hash, Possible options are (see the documentation of Berkeley DB
      for more informations) 

      : ((|store_nil_as_null|)): if `true' will store `nil' as `\000', otherwise as an empty string (default `false')
      : ((|set_array_base|)): base index for BDB::Recno, BDB::Queue or BDB::Btree (with BDB::RECNUM). Must be 0 or 1
      : ((|set_bt_compare|)) :  specify a Btree comparison function 
      : ((|set_bt_minkey|)) :   set the minimum number of keys per Btree page
      : ((|set_bt_prefix|)) :   specify a Btree prefix comparison function 
      : ((|set_cachesize|)) :   set the database cache size
      : ((|set_dup_compare|)) :  specify a duplicate comparison function 
      : ((|set_store_key|)) : specify a Proc called before a key is stored
      : ((|set_fetch_key|)) : specify a Proc called after a key is read
      : ((|set_store_value|)) : specify a Proc called before a value is stored
      : ((|set_fetch_value|)) : specify a Proc called after a value is read
      : ((|set_flags|)) :  general database configuration
      : ((|set_h_ffactor|)) :  set the Hash table density
      : ((|set_h_hash|)) :  specify a hashing function 
      : ((|set_h_nelem|)) :  set the Hash table size
      : ((|set_lorder|)) :  set the database byte order
      : ((|set_pagesize|)) :  set the underlying database page size
      : ((|set_re_delim|)) :  set the variable-length record delimiter
      : ((|set_re_len|)) :   set the fixed-length record length
      : ((|set_re_pad|)) :   set the fixed-length record pad byte
      : ((|set_re_source|)) :  set the backing Recno text file
      : ((|set_append_recno|)) : modify the stored data for ((|BDB::APPEND|))
      : ((|set_encrypt|)) : set the password used
      : ((|set_feedback|)) : set the function to monitor some operations
      : ((|env|)) :  open the database in the environnement given as the value
      : ((|txn|)) :  open the database in the transaction given as the value

      ((|set_append_recno|)) will be called with (key, value) and
      it must return ((|nil|)) or the modified value

      ((|set_encrypt|)) take an Array as arguments with the values
      [password, flags], where flags can be 0 or ((|BDB::ENCRYPT_AES|))

      Proc given to ((|set_bt_compare|)), ((|set_bt_prefix|)), 
      ((|set_dup_compare|)), ((|set_h_hash|)), ((|set_store_key|))
      ((|set_fetch_key|)), ((|set_store_value|)), ((|set_fetch_value|))
      ((|set_feedback|)) and ((|set_append_recno|))
      can be also specified as a method (replace the prefix ((|set_|)) 
      with ((|bdb_|)))

          For example 

            module BDB
               class Btreesort < Btree
                  def bdb_bt_compare(a, b)
                     b.downcase <=> a.downcase
                  end
               end
            end

--- remove(name, subname = nil) 
--- db_remove(name, subname = nil) 
--- unlink(name, subname = nil) 
     Removes the database (or subdatabase) represented by the
     name and subname combination.

     If no subdatabase is specified, the physical file represented by name
     is removed, incidentally removing all subdatabases that it contained.

--- upgrade(name)
--- db_upgrade(name)
    Upgrade the database

# end
=== Methods

--- self[key]
    Returns the value corresponding the ((|key|))

--- associate(db, flag = 0) { |db, key, value| }
    associate a secondary index db

    ((|flag|)) can have the value ((|BDB::RDONLY|))

    The block must return the new key, or ((|Qfalse|)) in this case the
    secondary index will not contain any reference to key/value

--- cache_priority
    return the current priority value

--- cache_priority=value
    set the priority value : can be ((|BDB::PRIORITY_VERY_LOW|)),
    ((|BDB::PRIORITY_LOW|)),  ((|BDB::PRIORITY_DEFAULT|)),
    ((|BDB::PRIORITY_HIGH|)) or ((|BDB::PRIORITY_VERY_HIGH|))

--- feedback=(proc)
    monitor the progress of some operations

--- get(key, flags = 0)
--- db_get(key, flags = 0)
--- fetch(key, flags = 0)
    Returns the value correspondind the ((|key|))

    ((|flags|)) can have the values ((|BDB::GET_BOTH|)), 
    ((|BDB::SET_RECNO|)) or ((|BDB::RMW|))

    In presence of duplicates it will return the first data item, use
    #duplicates if you want all duplicates (see also #each_dup)

--- pget(key, flags = 0)
    Returns the primary key and the value corresponding to ((|key|))
    in the secondary index

    only with >= 3.3.11

--- self[key] = value
    Stores the ((|value|)) associating with ((|key|))

    If ((|nil|)) is given as the value, the association from the key will be
    removed. 

--- put(key, value, flags = 0)
--- db_put(key, value, flags = 0)
--- store(key, value, flags = 0)
    Stores the ((|value|)) associating with ((|key|))

    If ((|nil|)) is given as the value, the association from the ((|key|))
    will be removed. It return the object deleted or ((|nil|)) if the
    specified key don't exist.

    ((|flags|)) can have the value ((|DBD::NOOVERWRITE|)), in this case
    it will return ((|nil|)) if the specified key exist, otherwise ((|true|))

--- append(key, value)
--- db_append(key, value)
    Append the ((|value|)) associating with ((|key|))

--- byteswapped?
--- get_byteswapped
    Return if the underlying database is in host order

--- clear_partial
--- partial_clear
    Clear partial set.

--- close(flags = 0)
--- db_close(flags = 0)
    Closes the file.

--- count(key)
--- dup_count(key)
    Return the count of duplicate for ((|key|))

--- cursor(flags = 0)
--- db_cursor(flags = 0)
    Open a new cursor.

--- database()
--- subname()
    Return the subname
 
--- delete(key)
--- db_del(key)
    Removes the association from the key. 

    It return the object deleted or ((|nil|)) if the specified
    key don't exist.

--- delete_if(set = nil) { |key, value| ... }
--- reject!(set = nil) { |key, value| ... }
    Deletes associations if the evaluation of the block returns true. 

    ((<set>))

--- duplicates(key , assoc = true)
    Return an array of all duplicate associations for ((|key|))

    if ((|assoc|)) is ((|false|)) return only the values.

--- each(set = nil, bulk = 0, "flags" => 0) { |key, value| ... }
--- each_pair(set = nil, bulk = 0) { |key, value| ... }
    Iterates over associations.

    ((<set>)) ((<bulk>))

--- each_by_prefix(prefix) {|key, value| ... }
    Iterate over associations, where the key begin with ((|prefix|))

--- each_dup(key, bulk = 0) { |key, value| ... }
    Iterates over each duplicate associations for ((|key|))

    ((<bulk>))

--- each_dup_value(key, bulk = 0) { |value| ... }
    Iterates over each duplicate values for ((|key|))

    ((<bulk>))

--- each_key(set = nil, bulk = 0) { |key| ... }
    Iterates over keys. 

    ((<set>)) ((<bulk>))

--- each_primary(set = nil) { |skey, pkey, pvalue| ... }
    Iterates over secondary indexes and give secondary key, primary key
    and value

--- each_value(set = nil, bulk = 0) { |value| ... }
    Iterates over values. 

    ((<set>)) ((<bulk>))

--- empty?() 
    Returns true if the database is empty. 

--- filename()
    Return the name of the file

--- has_key?(key) 
--- key?(key) 
--- include?(key) 
--- member?(key) 
    Returns true if the association from the ((|key|)) exists.

--- has_both?(key, value)
--- both?(key, value)
    Returns true if the association from ((|key|)) is ((|value|)) 

--- has_value?(value) 
--- value?(value) 
    Returns true if the association to the ((|value|)) exists. 

--- index(value)
    Returns the first ((|key|)) associated with ((|value|))

--- indexes(value1, value2, )
    Returns the ((|keys|)) associated with ((|value1, value2, ...|))

--- join(cursor , flag = 0) { |key, value| ... }
    Perform a join. ((|cursor|)) is an array of ((|BDB::Cursor|))

--- keys 
    Returns the array of the keys in the database

--- length 
--- size 
    Returns the number of association in the database.

--- log_register(name)
  
    The ((|log_register|)) function registers a file ((|name|)).

--- log_unregister()
  
    The ((|log_unregister|)) function unregisters a file name.

--- reject { |key, value| ... }
    Create an hash without the associations if the evaluation of the
    block returns true. 

--- reverse_each(set = nil) { |key, value| ... }
--- reverse_each_pair(set = nil) { |key, value| ... }
    Iterates over associations in reverse order 

    ((<set>))

--- reverse_each_by_prefix(prefix) {|key, value| ... }
    Iterate over associations in reverse order, where the key begin
    with ((|prefix|))


--- reverse_each_key(set = nil) { |key| ... }
    Iterates over keys in reverse order 

    ((<set>))

--- reverse_each_primary(set = nil) { |skey, pkey, pvalue| ... }
    Iterates over secondary indexes in reverse order and give secondary
    key, primary key and value

--- reverse_each_value(set = nil) { |value| ... }
    Iterates over values in reverse order.

    ((<set>))

--- set_partial(len, offset)
    Set the partial value ((|len|)) and ((|offset|))

--- stat
    Return database statistics.

--- to_a
    Return an array of all associations [key, value]

--- to_hash
    Return an hash of all associations {key => value}

--- truncate
--- clear
    Empty a database
       
--- values 
    Returns the array of the values in the database.

--- verify(file = nil, flags = 0)
    Verify the integrity of the DB file, and optionnally output the
    key/data to ((|file|)) (file must respond to #to_io)

--- compact(start = nil, stop = nil, options = nil)
    Only for Btree and Recno (DB VERSION >= 4.4)
    
    * ((|start|)) starting point for compaction in a Btree or Recno database.
      Compaction will start at the smallest key greater than or equal to the
      specified key. 
    
    * ((|stop|)) the stopping point for compaction in a Btree or Recno database.
      Compaction will stop at the page with the smallest key greater 
      than the specified key
    
    * ((|options|)) hash with the possible keys
    
      * ((|flags|)) with the value 0, ((|BDB::FREELIST_ONLY|)), or
        ((|BDB::FREE_SPACE|))
    
      * ((|compact_fillpercent|))the goal for filling pages, specified as a
        percentage between 1 and 100.
    
      * ((|compact_timeout|)) the lock timeout set for implicit transactions, 
        in microseconds.
    

# end
# class Recno < Common ## class Queue < Common

=== Methods specific to BDB::Recno and BDB::Queue

--- pop
    Returns the last couple [key, val] (only for BDB::Recno)

--- push values
    Push the values

--- shift 
    Removes and returns an association from the database.

# end


=== Remark

==== set

If the parameter ((|set|)) is given, an initial call will be made
with the option ((|BDB::SET_RANGE|)) to move the cursor to the specified
key before iterating.

==== bulk

Only with >= 3.3.11 : if the parameter ((|bulk|)) is given, 
an application buffer of size ((|bulk * 1024|)) will be used 
(see "Retrieving records in bulk" in the documentation of BerkeleyDB)

=end
