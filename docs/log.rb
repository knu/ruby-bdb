# a BDB::Lsn object is created by the method log_checkpoint, log_curlsn,
# log_flush, log_put
class BDB::Lsn
   include Comparable
   
   #
   #compare 2 <em>BDB::Lsn</em>
   #
   def  <=>(other)
   end
   
   #
   #The <em>log_file</em> function maps <em>BDB::Lsn</em> structures to file 
   #<em>name</em>s
   #
   def  log_file(name)
   end
   
   #same than <em> log_file</em>
   def  file(name)
   end
   
   #
   #The <em>log_flush</em> function guarantees that all log records whose
   #<em>DBB:Lsn</em> are less than or equal to the current lsn have been written
   #to disk.
   #
   def  log_flush
   end
   #same than <em> log_flush</em>
   def  flush
   end
   
   #
   #return the <em>String</em> associated with this <em>BDB::Lsn</em>
   #
   def  log_get
   end
   #same than <em> log_get</em>
   def  get
   end
end
