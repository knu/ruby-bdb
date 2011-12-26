=begin
== BDB::XML::IndexLookup

The XML::IndexLookup class encapsulates the context within which an
index lookup operation can be performed on an XML::Container object. The
lookup is performed using an XML::IndexLookup object, and a series of
methods of that object that specify how the lookup is to be
performed. Using these methods, it is possible to specify inequality
lookups, range lookups, and simple value lookups, as well as the sort
order of the results. By default, results are returned in the sort
order of the index.

XML::IndexLookup objects are created using XML::Manager::create_index_lookup,
or Txn::create_index_lookup

The following constant are defined ((|NONE|)), ((|EQ|)), ((|GT|)),
((|GTE|)), ((|LT|)), ((|LTE|))

=== Methods

--- container
    Retrieve the current container
   
--- container=(con)
    Sets the container to be used for the index lookup operation. 
    The same object may be used for lookup in multiple containers by changing 
    this configuration.

--- execute(context = nil, flags = 0)
    Executes the index lookup operation specified by the configuration.
   
    : ((|context|))
      a XML::QueryContext object to use for this query

    : ((|flags|))
      the flags must be set to ((|0|)), OR'ing one of the value
      ((|BDB::XML::LAZY_DOCS|)), ((|BDB::XML::REVERSE_ORDER|)),
      ((|BDB::XML::CACHE_DOCUMENTS|)), ((|BDB::DEGREE_2|)),
      ((|BDB::DIRTY_READ|)), ((|BDB::RMW|))


--- high_bound
    Retrieve the operation and value used for the upper bound


--- high_bound=([value, operation])
    call-seq:
      self.high_bound = [value, operation]

    Sets the operation and value to be used for the upper bound for a range 
    index lookup operation. The high bound must be specified to indicate a 
    range lookup.

--- index
    Retrieve the indexing strategy

--- index=(string)
    Set the indexing strategy to be used for the index lookup operation. 
    Only one index may be specified, and substring indexes are not supported.

--- low_bound
    Retrieve the operation and value used for the lower bound


--- low_bound=([value, operation])
    call-seq:
      self.low_bound = [value, operation]

    Sets the operation and value to be used for the index lookup operation.
    If the operation is a simple inequality lookup, the lower bound is used as 
    the single value and operation for the lookup. If the operation is a range
    lookup, in which an upper bound is specified, the lower bound is used 
    as the lower boundary value and operation for the lookup.


--- manager
    Return the manager associated with the IndexLookup


--- node
    Retrieve the namespace and the name of the node


--- node_uri
    Retrieve the namespace of the node
 

--- node_name
    Retrieve the name of the node


--- node=([uri, name])
    Sets the name of the node to be used along with the indexing strategy
    for the index lookup operation.
   
    : ((|uri|))
      The namespace of the node to be used. The default namespace is selected 
      by passing an empty string for the namespace
    
    : ((|name|))
      The name of the element or attribute node to be used.
   

--- parent
    Retrieve the namespace and the name of the parent node


--- parent_uri
    Retrieve the namespace of the parent node
 

--- parent_name
    Retrieve the name of the parent node


--- parent=([uri, name])
    Sets the name of the parent node to be used for an edge index lookup
    operation. If the index is not an edge index, this configuration is ignored.
   
    : ((|uri|))
      The namespace of the parent node to be used. The default namespace is 
      selected  by passing an empty string for the namespace
   
    : ((|name|))
      The name of the parent element node to be used.
   

--- transaction
    Return the transaction associated with the IndexLookup, if it was opened
    in a transaction


--- transaction?
    Return true if the IndexLookup was opened in a transaction


=end
