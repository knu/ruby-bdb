=begin
== Acces Methods

These are the common methods for ((|BDB::Btree|)), ((|BDB::Hash|)),
((|BDB::Recno|)) and ((|BDB::Queue|))

* ((<Class Methods>))
* ((<Methods>))
* ((<Methods specific to BDB::Recno and BDB::Queue>))

=== Class Methods

--- create([name, subname, flags, mode, options])
--- new([name, subname, flags, mode, options])
--- open([name, subname, flags, mode, options])
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

           : ((|set_array_base|))
                base index for BDB::Recno, BDB::Queue or BDB::Btree 
                (with BDB::RECNUM). Must be 0 or 1
           : ((|set_bt_compare|))
                specify a Btree comparison function 
          : ((|set_bt_minkey|))
                set the minimum number of keys per Btree page
           : ((|set_bt_prefix|))
                specify a Btree prefix comparison function 
           : ((|set_cachesize|))
                set the database cache size
           : ((|set_dup_compare|))
                specify a duplicate comparison function 
           : ((|set_flags|))
                general database configuration
           : ((|set_h_ffactor|))
                set the Hash table density
           : ((|set_h_hash|))
                specify a hashing function 
           : ((|set_h_nelem|))
                set the Hash table size
           : ((|set_lorder|))
                set the database byte order
           : ((|set_pagesize|))
                set the underlying database page size
           : ((|set_re_delim|))
                set the variable-length record delimiter
           : ((|set_re_len|))
                set the fixed-length record length
           : ((|set_re_pad|))
                set the fixed-length record pad byte
           : ((|set_re_source|))
                set the backing Recno text file
           : ((|env|))
                open the database in the environnement given as the value
           : ((|txn|))
                open the database in the transaction given as the value

          Proc given to ((|set_bt_compare|)), ((|set_bt_prefix|)), 
          ((|set_dup_compare|)) and ((|set_h_hash|)) can be also specified as
          a method (replace the prefix ((|set_|)) with ((|bdb_|))

          For example 

            module BDB
               class Btreesort < Btree
                  def bdb_bt_compare(a, b)
                     b.downcase <=> a.downcase
                  end
               end
            end
 
--- db_remove(name [, subname]) 
--- remove(name [, subname]) 
--- unlink(name [,subname]) 
     Removes the database (or subdatabase) represented by the
     name and subname combination.

     If no subdatabase is specified, the physical file represented by name
     is removed, incidentally removing all subdatabases that it contained.

--- db_upgrade(name)
--- upgrade(name)
     Upgrade the database

=== Methods

--- self[key]
     Returns the value corresponding the ((|key|))

--- associate(db, flag) { |db, key, value| }
     associate a secondary index db
     ((|flag|)) can have the value ((|BDB::RDONLY|))
     The block must return the new key, or ((|Qfalse|)) in this case the
     secondary index will not contain any reference to key/value

--- db_get(key [, flags])
--- get(key [, flags])
--- fetch(key [, flags])
     Returns the value correspondind the ((|key|))

     ((|flags|)) can have the values ((|BDB::GET_BOTH|)), 
     ((|BDB::SET_RECNO|)) or ((|BDB::RMW|))

     In presence of duplicates it will return the first data item, use
     #duplicates if you want all duplicates (see also #each_dup)

--- pget(key [, flags])
     Returns the primary key and the value corresponding to ((|key|))
     in the secondary index

     only with >= 3.3.11

--- self[key] = value
     Stores the ((|value|)) associating with ((|key|))

     If ((|nil|)) is given as the value, the association from the key will be
     removed. 

--- db_put(key, value [, flags])
--- put(key, value [, flags])
--- store(key, value [, flags])
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

--- close([flags])
--- db_close([flags])
    Closes the file.

--- count(key)
--- dup_count(key)
    Return the count of duplicate for ((|key|))

--- cursor()
--- db_cursor()
    Open a new cursor.

--- delete(key)
--- db_del(key)
      Removes the association from the key. 

      It return the object deleted or ((|nil|)) if the specified
      key don't exist.

--- delete_if([bulk]) { |key, value| ... }
--- reject!([bulk]) { |key, value| ... }
      Deletes associations if the evaluation of the block returns true. 

      ((<bulk>))

--- duplicates(key [, assoc = true])
      Return an array of all duplicate associations for ((|key|))

      if ((|assoc|)) is ((|false|)) return only the values.

--- each([bulk]) { |key, value| ... }
--- each_pair([bulk]) { |key, value| ... }
      Iterates over associations.

      ((<bulk>))

--- each_dup(key) { |key, value| ... }
      Iterates over each duplicate associations for ((|key|))

--- each_dup_value(key) { |value| ... }
      Iterates over each duplicate values for ((|key|))

--- each_key([bulk]) { |key| ... }
      Iterates over keys. 

      ((<bulk>))

--- each_primary { |skey, pkey, pvalue| ... }
      Iterates over secondary indexes and give secondary key, primary key
      and value

--- each_value([bulk]) { |value| ... }
      Iterates over values. 

      ((<bulk>))

--- empty?() 
       Returns true if the database is empty. 

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

--- join(cursor [, flag]) { |key, value| ... }
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

--- reverse_each { |key, value| ... }
--- reverse_each_pair { |key, value| ... }
      Iterates over associations in reverse order 

--- reverse_each_key { |key| ... }
      Iterates over keys in reverse order 

--- reverse_each_primary { |skey, pkey, pvalue| ... }
      Iterates over secondary indexes in reverse order and give secondary key,
      primary key and value

--- reverse_each_value { |value| ... }
      Iterates over values in reverse order.

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

=== Methods specific to BDB::Recno and BDB::Queue

--- pop
       Returns the last couple [key, val] (only for BDB::Recno)

--- push value, ...
       Push the values

--- shift 
       Removes and returns an association from the database.

=== Remark

==== bulk

Only with >= 3.3.11 : if the parameter ((|bulk|)) is given, 
an application buffer of size ((|bulk * 1024|)) will be used 
(see "Retrieving records in bulk" in the documentation of BerkeleyDB)

=end
