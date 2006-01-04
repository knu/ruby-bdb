# Implement common methods for access to data
#
class BDB::Common
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
      #  * <em>store_nil_as_null</em>: if `true' will store `nil' as `\000', otherwise as an empty string (default `false')
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
      #  * <em>set_append_recno</em> : modify the stored data for <em>BDB::APPEND</em>
      #  * <em>set_encrypt</em> : set the password used
      #  * <em>set_feedback</em> : set the function to monitor some operations
      #  * <em>env</em> :  open the database in the environnement given as the value
      #  * <em>txn</em> :  open the database in the transaction given as the value
      #
      #  <em>set_append_recno</em> will be called with (key, value) and
      #  it must return <em>nil</em> or the modified value
      #
      #  <em>set_encrypt</em> take an Array as arguments with the values
      #  [password, flags], where flags can be 0 or <em>BDB::ENCRYPT_AES</em>
      #
      #  Proc given to <em>set_bt_compare</em>, <em>set_bt_prefix</em>, 
      #  <em>set_dup_compare</em>, <em>set_h_hash</em>, <em>set_store_key</em>
      #  <em>set_fetch_key</em>, <em>set_store_value</em>, <em>set_fetch_value</em>
      #  <em>set_feedback</em> and <em>set_append_recno</em>
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
      
      #same than <em> open</em>
      def  create(name = nil, subname = nil, flags = 0, mode = 0, options = {})
      end
      
      #same than <em> open</em>
      def  new(name = nil, subname = nil, flags = 0, mode = 0, options = {})
      end
      
      #Removes the database (or subdatabase) represented by the
      #name and subname combination.
      #
      #If no subdatabase is specified, the physical file represented by name
      #is removed, incidentally removing all subdatabases that it contained.
      #
      def  remove(name, subname = nil) 
      end
      
      #same than <em> remove</em>
      def  db_remove(name, subname = nil) 
      end
      
      #same than <em> remove</em>
      def  unlink(name, subname = nil) 
      end
      
      #Upgrade the database
      #
      def  upgrade(name)
      end
      
      #same than <em> upgrade</em>
      def  db_upgrade(name)
      end
   end
   
   #Returns the value corresponding the <em>key</em>
   #
   def  [](key)
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
   
   #return the current priority value
   #
   def  cache_priority
   end
   
   #set the priority value : can be <em>BDB::PRIORITY_VERY_LOW</em>,
   #<em>BDB::PRIORITY_LOW</em>,  <em>BDB::PRIORITY_DEFAULT</em>,
   #<em>BDB::PRIORITY_HIGH</em> or <em>BDB::PRIORITY_VERY_HIGH</em>
   #
   def  cache_priority=value
   end
   
   #create a new sequence (see also <em>open_sequence</em>)
   #
   #equivalent to 
   #<em>open_sequence(key, BDB::CREATE|BDB::EXCL, init, options)</em>
   #
   #return (or yield) an object BDB::Sequence
   def create_sequence(key, init = nil, options = {})
      yield sequence
   end

   #create or open a sequence (see BDB::Sequence)
   #
   #<em>key</em> : key for the sequence
   #
   #<em>flags</em> : flags can have BDB::CREATE, BDB::EXCL, BDB::AUTO_COMMIT,
   #BDB::THREAD
   #
   #<em>init</em> : initial value for the sequence
   #
   #<em>options</em> : hash with the possible keys "set_cachesize",
   #"set_flags" and "set_range"
   #
   #return (or yield) an object BDB::Sequence
   def open_sequence(key, flags = 0, init = nil, options = {})
      yield sequence
   end
   
   #   
   #monitor the progress of some operations
   #
   def  feedback=(proc)
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
   
   #same than <em> get</em>
   def  db_get(key, flags = 0)
   end
   
   #same than <em> get</em>
   def  fetch(key, flags = 0)
   end
   
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
   def  []=(key, value)
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
   
   #same than <em> put</em>
   def  db_put(key, value, flags = 0)
   end
   
   #same than <em> put</em>
   def  store(key, value, flags = 0)
   end
   
   #Append the <em>value</em> associating with <em>key</em>
   #
   def  append(key, value)
   end
   
   #same than <em> append</em>
   def  db_append(key, value)
   end
   
   #Return if the underlying database is in host order
   #
   def  byteswapped?
   end
   
   #same than <em> byteswapped?</em>
   def  get_byteswapped
   end
   
   #Clear partial set.
   #
   def  clear_partial
   end
   
   #same than <em> clear_partial</em>
   def  partial_clear
   end
   
   #Closes the file.
   #
   def  close(flags = 0)
   end
   
   #same than <em> close</em>
   def  db_close(flags = 0)
   end

   #Only for Btree and Recno (DB VERSION >= 4.4)
   #
   #* <em>start</em> starting point for compaction in a Btree or Recno database.
   #  Compaction will start at the smallest key greater than or equal to the
   #  specified key. 
   #
   #* <em>stop</em> the stopping point for compaction in a Btree or Recno database.
   #  Compaction will stop at the page with the smallest key greater 
   #  than the specified key
   #
   #* <em>options</em> hash with the possible keys
   #
   #  * <em>flags</em> with the value 0, <em>BDB::FREELIST_ONLY</em>, or
   #    <em>BDB::FREE_SPACE</em>
   #
   #  * <em>compact_fillpercent</em>the goal for filling pages, specified as a
   #    percentage between 1 and 100.
   #
   #  * <em>compact_timeout</em> the lock timeout set for implicit transactions, 
   #    in microseconds.
   #
   def compact(start = nil, stop = nil, options = nil)
   end

   #Return the count of duplicate for <em>key</em>
   #
   def  count(key)
   end
   
   #same than <em> count</em>
   def  dup_count(key)
   end
   
   #Open a new cursor.
   #
   def  cursor(flags = 0)
   end
   
   #same than <em> cursor</em>
   def  db_cursor(flags = 0)
   end
 
   #Open a new cursor with the flag <em>BDB::WRITECURSOR</em>
   #
   def  cursor_write()
   end
   
   #same than <em> cursor_write</em>
   def  db_cursor_write(flags = 0)
   end
   
   #Return the subname
   # 
   def  database()
   end
   
   #same than <em> database</em>
   def  subname()
   end
   
   #Removes the association from the key. 
   #
   #It return the object deleted or <em>nil</em> if the specified
   #key don't exist.
   #
   def  delete(key)
   end
   
   #same than <em> delete</em>
   def  db_del(key)
   end
   
   #Deletes associations if the evaluation of the block returns true. 
   #
   #<em>set</em>
   #
   def  delete_if(set = nil) 
      yield key, value
   end
   
   #same than <em> delete_if</em>
   def  reject!(set = nil) 
      yield key, value
   end
   
   #Return an array of all duplicate associations for <em>key</em>
   #
   #if <em>assoc</em> is <em>false</em> return only the values.
   #
   def  duplicates(key , assoc = true)
   end
   
   #Iterates over associations.
   #
   #<em>set</em> <em>bulk</em>
   #
   def  each(set = nil, bulk = 0, "flags" => 0) 
      yield key, value
   end
   
   #same than <em> each</em>
   def  each_pair(set = nil, bulk = 0) 
      yield key, value
   end

   #iterate over associations, where the key begin with
   #<em>prefix</em>
   def each_by_prefix(prefix = nil)
      yield key, value
   end
   
   #Iterates over each duplicate associations for <em>key</em>
   #
   #<em>bulk</em>
   #
   def  each_dup(key, bulk = 0) 
      yield key, value
   end
   
   #Iterates over each duplicate values for <em>key</em>
   #
   #<em>bulk</em>
   #
   def  each_dup_value(key, bulk = 0) 
      yield value
   end
   
   #Iterates over keys. 
   #
   #<em>set</em> <em>bulk</em>
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
   #<em>set</em> <em>bulk</em>
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
   
   #same than <em> has_key?</em>
   def  key?(key) 
   end
   
   #same than <em> has_key?</em>
   def  include?(key) 
   end
   
   #same than <em> has_key?</em>
   def  member?(key) 
   end
   
   #Returns true if the association from <em>key</em> is <em>value</em> 
   #
   def  has_both?(key, value)
   end
   
   #same than <em> has_both?</em>
   def  both?(key, value)
   end
   
   #Returns true if the association to the <em>value</em> exists. 
   #
   def  has_value?(value) 
   end
   
   #same than <em> has_value?</em>
   def  value?(value) 
   end
   
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
   
   #same than <em> length </em>
   def  size 
   end
   
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
   #<em>set</em>
   #
   def  reverse_each(set = nil) 
      yield key, value
   end
   
   #same than <em> reverse_each</em>
   def  reverse_each_pair(set = nil) 
      yield key, value
   end

   #iterate over associations in reverse order, where the key begin with
   #<em>prefix</em>
   def reverse_each_by_prefix(prefix = nil)
      yield key, value
   end
   
   #Iterates over keys in reverse order 
   #
   #<em>set</em>
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
   #<em>set</em>
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
   #same than <em> truncate</em>
   def  clear
   end
   
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
