=begin
== BDB::XML::Index

The XML::Index class encapsulates the indexing specification of a container.
An indexing specification can be retrieved with the XML::Container#index
method, and modified using the XML::Container#index= method.

Note that adding an index to a container results in re-indexing all of
the documents in that container, which can take a very long time. It is 
good practice to design an application to add useful indices before 
populating a container.

Included module : Enumerable

=== Methods

--- add(uri, name, index)
--- add(uri, name, type, syntax)

    call-seq:
       add(uri, name, index)
       add(uri, name, type, syntax)
    
    Add a new index
    
    : ((|uri|))
      The namespace for the element
    
    : ((|name|))
      The name parameter provides the fully qualified element type
      to be indexed.
    
    : ((|index|))
      The index string is a comma separated list of strings that represent 
      the indexing strategy
    
    : ((|type|)) 
      series of  values bitwise OR'd together to form the 
      index strategy.
    
    : ((|syntax|)) 
      Identifies the type of information being indexed

--- add_default(index)

    Adds an indexing strategy to the default index specification.

--- default

    return the default index specification
   
--- delete(uri, name, index)
--- delete(uri, name, type, syntax)
    
    Delete the specified index
    
    : ((|uri|))
      The namespace for the element
    
    : ((|name|))
      The name parameter provides the fully qualified element type
      to be indexed.
    
    : ((|index|))
      The index string is a comma separated list of strings that represent 
      the indexing strategy
    
    : ((|type|)) 
      series of  values bitwise OR'd together to form the 
      index strategy.
    
    : ((|syntax|)) 
      Identifies the type of information being indexed

--- delete_default(index)
--- delete_default(type, syntax)
    
    Delete the identified index from the default index specification.
    
    : ((|index|))
      The index string is a comma separated list of strings that represent 
      the indexing strategy
    
    : ((|type|)) 
      series of  values bitwise OR'd together to form the 
      index strategy.
    
    : ((|syntax|)) 
      Identifies the type of information being indexed

--- each {|uri, name, index| ... }

    Iterate over all indexes
   
--- find(uri = "", name)

    Find the indexing startegy associated with ((|uri|)), and ((|name|))
    
    Return ((|nil|)) is no indexing strategy was defined

--- manager

    Return the manager associated with the Index
   
--- replace(uri, name, index)
--- replace(uri, name, type, syntax)
    
    Replaces the indexing strategies for a named document or metadata node.
    All existing indexing strategies for that node are deleted, and the 
    indexing strategy identified by this method is set for the node.
    
    : ((|uri|))
      The namespace for the element
    
    : ((|name|))
      The name parameter provides the fully qualified element type
      to be indexed.
    
    : ((|index|))
      The index string is a comma separated list of strings that represent 
      the indexing strategy
    
    : ((|type|)) 
      series of  values bitwise OR'd together to form the 
      index strategy.
    
    : ((|syntax|)) 
      Identifies the type of information being indexed

--- replace_default(index)
--- replace_default(type, syntax)
    
    Replace the default indexing strategy for the container. 
    
    : ((|index|))
      The index string is a comma separated list of strings that represent 
      the indexing strategy
    
    : ((|type|)) 
      series of  values bitwise OR'd together to form the 
      index strategy.
    
    : ((|syntax|)) 
      Identifies the type of information being indexed
   
--- to_a

    Return an Array of all indexes

=end
