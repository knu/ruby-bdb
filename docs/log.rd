=begin

== Logging subsystem

# module BDB
#
This subsystem is used when recovery from application or system
failure is necessary.

The log is stored in one or more files in the environment
directory. Each file is named using the format log.NNNNNNNNNN, where
NNNNNNNNNN is the sequence number of the file within the log.

If the log region is being created and log files are already present,
the log files are reviewed and subsequent log writes are appended to
the end of the log, rather than overwriting current log entries.


The lock subsystem is created, initialized, and opened by calls to
(({BDB::Env#open})) with the ((|BDB::INIT_LOG|)) flag specified. 

The following options can be given when the environnement is created

  : ((|"set_lg_bsize"|))
    Set log buffer size

  : ((|"set_lg_dir"|))
    Set the environment logging directory

  : ((|"set_lg_max"|))
    Set log file size
#

# class Env

=== BDB::Env

==== Methods

--- log_archive(flags = 0)
    The log_archive function return an array of log or database file names.

    ((|flags|)) value must be set to 0 or the value ((|BDB::ARCH_DATA|))
     ((|BDB::ARCH_DATA|)), ((|BDB::ARCH_LOG|))

--- log_checkpoint(string)

    same as ((|log_put(string, BDB::CHECKPOINT)|))

--- log_curlsn(string)

    same as ((|log_put(string, BDB::CURLSN)|))

--- log_each { |string, lsn| ... }

    Implement an iterator inside of the log

--- log_flush([string])

    same as ((|log_put(string, BDB::FLUSH)|))

    Without argument, garantee that all records are written to the disk

--- log_get(flag)

    The ((|log_get|)) return an array ((|[String, BDB::Lsn]|)) according to
    the ((|flag|)) value.

    ((|flag|)) can has the value ((|BDB::CHECKPOINT|)), ((|BDB::FIRST|)), 
    ((|BDB::LAST|)), ((|BDB::NEXT|)), ((|BDB::PREV|)), ((|BDB::CURRENT|))

--- log_put(string, flag = 0)

    The ((|log_put|)) function appends records to the log. It return
    an object ((|BDB::Lsn|))

    ((|flag|)) can have the value ((|BDB::CHECKPOINT|)), ((|BDB::CURLSN|)),
    ((|BDB::FLUSH|))


--- log_reverse_each { |string, lsn| ... }

    Implement an iterator inside of the log

--- log_stat

    return log statistics

# end

# class Common

=== BDB::Common

==== Methods

--- log_register(name)
  
    The ((|log_register|)) function registers a file ((|name|)).

--- log_unregister()
  
    The ((|log_unregister|)) function unregisters a file name.
# end
## a BDB::Lsn object is created by the method log_checkpoint, log_curlsn,
## log_flush, log_put
# class Lsn
# include Comparable

=== BDB::Lsn

include Comparable

--- <=>

    compare 2 ((|BDB::Lsn|))

--- log_file(name)
--- file(name)

    The ((|log_file|)) function maps ((|BDB::Lsn|)) structures to file 
    ((|name|))s

--- log_flush
--- flush

    The ((|log_flush|)) function guarantees that all log records whose
    ((|DBB:Lsn|)) are less than or equal to the current lsn have been written
    to disk.

--- log_get
--- get

    return the ((|String|)) associated with this ((|BDB::Lsn|))

# end
# end

=end
