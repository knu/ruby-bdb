=begin
== Acces Methods

These are the common methods for ((|BDB::Btree|)), ((|BDB::Hash|)),
((|BDB::Recno|)) and ((|BDB::Queue|))

* ((<Class Methods>))
* ((<Methods>))

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

           : ((|set_bt_minkey|))
                set the minimum number of keys per Btree page
           : ((|set_cachesize|))
                set the database cache size
           : ((|set_flags|))
                general database configuration
           : ((|set_h_ffactor|))
                set the Hash table density
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

--- db_get(key [, flags])
--- get(key [, flags])
     Returns the value correspondind the ((|key|))

     ((|flags|)) can have the values ((|BDB::GET_BOTH|)), ((|BDB::SET_RECNO|))
     or ((|BDB::RMW|))

--- self[key] = value
     Stores the ((|value|)) associating with ((|key|))

     If ((|nil|)) is given as the value, the association from the key will be
     removed. 

--- db_put(key, value [, flags])
--- put(key, value [, flags])
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
     Clear partial set.

--- close([flags])
--- db_close([flags])
    Closes the file.

--- count(key)
    Return the count of duplicate for ((|key|))

--- cursor()
--- db_cursor()
    Open a new cursor.

--- delete(key)
--- db_del(key)
      Removes the association from the key. 

      It return the object deleted or ((|nil|)) if the specified
      key don't exist.

--- delete_if { |key, value| ... }
      Deletes associations if the evaluation of the block returns true. 

--- each { |key, value| ... }
--- each_pair { |key, value| ... }
      Iterates over associations. 

--- each_key { |key| ... }
      Iterates over keys. 

--- each_value { |value| ... }
      Iterates over values. 

--- empty?() 
       Returns true if the database is empty. 

--- has_key?(key) 
--- key?(key) 
--- include?(key) 
       Returns true if the association from the ((|key|)) exists.

--- has_both?(key, value)
--- both?(key, value)
      Returns true if the association from ((|key|)) is ((|value|)) 

--- has_value?(value) 
--- value?(value) 
       Returns true if the association to the ((|value|)) exists. 

--- join(cursor [, flag]) { |key, value| ... }
       Perform a join. ((|cursor|)) is an array of ((|BDB::Cursor|))

--- keys 
       Returns the array of the keys in the database

--- length 
--- size 
       Returns the number of association in the database.

--- reverse_each { |key, value} ... }
--- reverse_each_pair { |key, value} ... }
      Iterates over associations in reverse order 

--- reverse_each_key { |key| ... }
      Iterates over keys in reverse order 

--- reverse_each_value { |value| ... }
      Iterates over values in reverse order.

--- set_partial(len, offset)
       Set the partial value ((|len|)) and ((|offset|))

--- shift 
       Removes and returns an association from the database. 

--- stat
       Return database statistics.

--- to_a
       Return an array of all associations [key, value]

--- values 
       Returns the array of the values in the database.
=end
