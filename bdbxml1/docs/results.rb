# 
# The results of a query are a collection of values. The values could
# be either String, Float, true, false, BDB::XML::Documents or BDB::XML::Nodes
# 
class BDB::XML::Results
   include Enumerable
   
   #Iterate on each values
   #
   def  each 
      yield val
   end
end
