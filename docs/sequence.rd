=begin
== BDB::Sequence

# module BDB
#^

A sequence is created with BDB::Common::create_sequence or 
BDB::Common::open_sequence (only with db >= 4.3)

#^

=== class BDB::Common

--- create_sequence(key, init = nil, options = {}) {|sequence| }
     create a new sequence (see also ((|open_sequence|)))
     
     equivalent to 
     ((|open_sequence(key, BDB::CREATE|BDB::EXCL, init, options)|))
     
     return (or yield) an object BDB::Sequence

--- open_sequence(key, flags = 0, init = nil, options = {}) {|sequence| }
     create or open a sequence (see BDB::Sequence)
     
     ((|key|)) : key for the sequence
     
     ((|flags|)) : flags can have BDB::CREATE, BDB::EXCL, BDB::AUTO_COMMIT,
     BDB::THREAD
     
     ((|init|)) : initial value for the sequence
     
     ((|options|)) : hash with the possible keys "set_cachesize",
     "set_flags" and "set_range"
     
     return (or yield) an object BDB::Sequence
 
#^

# class Sequence

=== Methods

--- cachesize
     return the current cache size

--- close
     close the sequence

--- db
     return the bdb file associated with the sequence

--- flags
     return the current flags

--- get(delta = 1, flags = 0)
     return the next available element in the sequence and changes
     the sequence value by ((|delta|))
     
     ((|flags|)) can have the value BDB::AUTO_COMMIT, BDB::TXN_NOSYNC

--- key
     return the key associated with the sequence

--- range
     return the range of values in the sequence

--- remove(flags = 0)
     remove the sequence
     
     ((|flags|)) can have the value BDB::AUTO_COMMIT, BDB::TXN_NOSYNC

--- stat(flags = 0)
     return statistics about the sequence
     
     ((|flags|)) can have the value BDB::STAT_CLEAR

# end
# end

=end
