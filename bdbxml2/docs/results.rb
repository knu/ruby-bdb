# 
# The results of a query are a collection of values. The values could
# be either String, Float, true, false, BDB::XML::Documents or BDB::XML::Nodes
# 
class BDB::XML::Results
   include Enumerable
   
   # Add a new value
   def add(val)
   end

   # Iterate on each values
   #
   def  each 
      yield val
   end

   # Return the number of elements
   def size
   end

end
