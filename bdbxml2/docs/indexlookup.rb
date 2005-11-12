# The XML::IndexLookup class encapsulates the context within which an
# index lookup operation can be performed on an XML::Container object. The
# lookup is performed using an XML::IndexLookup object, and a series of
# methods of that object that specify how the lookup is to be
# performed. Using these methods, it is possible to specify inequality
# lookups, range lookups, and simple value lookups, as well as the sort
# order of the results. By default, results are returned in the sort
# order of the index.
# 
# XML::IndexLookup objects are created using XML::Manager::create_index_lookup,
# or Txn::create_index_lookup
# 
# The following constant are defined <em>NONE</em>, <em>EQ</em>, <em>GT</em>,
# <em>GTE</em>, <em>LT</em>, <em>LTE</em>
class BDB::XML::IndexLookup
   # Retrieve the current container
   def container
   end
   
   # Sets the container to be used for the index lookup operation. 
   # The same object may be used for lookup in multiple containers by changing 
   # this configuration.
   def container=(con)
   end

   # Executes the index lookup operation specified by the configuration.
   # 
   # * <em>context</em>
   #   a XML::QueryContext object to use for this query
   #
   # * <em>flags</em>
   #   the flags must be set to <em>0</em>, OR'ing one of the value
   #   <em>BDB::XML::LAZY_DOCS</em>, <em>BDB::XML::REVERSE_ORDER</em>,
   #   <em>BDB::XML::CACHE_DOCUMENTS</em>, <em>BDB::DEGREE_2</em>,
   #   <em>BDB::DIRTY_READ</em>, <em>BDB::RMW</em>
   #
   def execute(context = nil, flags = 0)
   end

   # Retrieve the operation and value used for the upper bound
   #
   def high_bound
   end

   # call-seq:
   #   self.high_bound = [value, operation]
   #
   # Sets the operation and value to be used for the upper bound for a range 
   # index lookup operation. The high bound must be specified to indicate a 
   # range lookup.
   def high_bound=([value, operation])
   end

   # Retrieve the indexing strategy
   def index
   end

   # Set the indexing strategy to be used for the index lookup operation. 
   # Only one index may be specified, and substring indexes are not supported.
   def index=(string)
   end

   # Retrieve the operation and value used for the lower bound
   #
   def low_bound
   end

   # call-seq:
   #   self.low_bound = [value, operation]
   #
   # Sets the operation and value to be used for the index lookup operation.
   # If the operation is a simple inequality lookup, the lower bound is used as 
   # the single value and operation for the lookup. If the operation is a range
   # lookup, in which an upper bound is specified, the lower bound is used 
   # as the lower boundary value and operation for the lookup.
   def low_bound=([value, operation])
   end

   # Return the manager associated with the IndexLookup
   def manager
   end

   # Retrieve the namespace and the name of the node
   def node
   end

   # Retrieve the namespace of the node
   def node_uri
   end
 
   # Retrieve the name of the node
   def node_name
   end

   # Sets the name of the node to be used along with the indexing strategy
   # for the index lookup operation.
   #
   # * <em>uri</em>
   #   The namespace of the node to be used. The default namespace is selected 
   #   by passing an empty string for the namespace
   #
   # * <em>name</em>
   #   The name of the element or attribute node to be used.
   #
   def node=([uri, name])
   end

   # Retrieve the namespace and the name of the parent node
   def parent
   end

   # Retrieve the namespace of the parent node
   def parent_uri
   end
 
   # Retrieve the name of the parent node
   def parent_name
   end

   # Sets the name of the parent node to be used for an edge index lookup
   # operation. If the index is not an edge index, this configuration is ignored.
   #
   # * <em>uri</em>
   #   The namespace of the parent node to be used. The default namespace is 
   #   selected  by passing an empty string for the namespace
   #
   # * <em>name</em>
   #   The name of the parent element node to be used.
   #
   def parent=([uri, name])
   end

   # Return the transaction associated with the IndexLookup, if it was opened
   # in a transaction
   def transaction
   end

   # Return true if the IndexLookup was opened in a transaction
   def transaction?
   end


end
