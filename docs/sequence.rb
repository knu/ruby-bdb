# A sequence is created with the method <em>BDB::Common#create_sequence</em>
# or <em>BDB::Common#open_sequence</em>
#
class BDB::Sequence
   #return the current cache size
   def cachesize
   end

   #close the sequence
   def close
   end

   #return the bdb file associated with the sequence
   def db
   end

   #return the current flags
   def flags
   end

   #return the next available element in the sequence and changes
   #the sequence value by <em>delta</em>
   #
   #<em>flags</em> can have the value BDB::AUTO_COMMIT, BDB::TXN_NOSYNC
   def get(delta = 1, flags = 0)
   end

   #return the key associated with the sequence
   def key
   end

   #return the range of values in the sequence
   def range
   end

   #remove the sequence
   #
   #<em>flags</em> can have the value BDB::AUTO_COMMIT, BDB::TXN_NOSYNC
   def remove(flags = 0)
   end

   #return statistics about the sequence
   #
   #<em>flags</em> can have the value BDB::STAT_CLEAR
   def stat(flags = 0)
   end

end

