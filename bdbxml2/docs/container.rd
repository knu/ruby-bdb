=begin
== BDB::XML::Container

The XML::Container class encapsulates a document container and its 
related indices and statistics. XML::Container exposes methods for
managing (putting and deleting) XML::Document objects, managing indices,
and retrieving container statistics.

Module included : Enumerable

=== Methods

--- delete(doc_or_name, context = nil)

    Remove the document from the container
    
    : ((|doc_or_name|))
      doc_or_name is an ((|BDB::XML::Document|)) previously stored,
      or the name of an existent document
    
    : ((|context|)) 
      an optional Update context
   
--- index?

    Returns true if the container is configured to create node indexes.

--- manager

    return the current manager for the container, or ((|nil|))

--- pagesize

    Returns true if the container is configured to create node indexes.
   
--- each {|doc| ... }
    Iterate over all documents
    
--- self[id]

    Fetch the document from the container
    
    : ((|id|))
      the id assigned to the document when it was first added to a container
    
    : ((|flags|))
      flags can has the value 0 or ((|BDB::DIRTY_READ|)), ((|BDB::RMW|))
    
--- get(id, flags = 0)

    same than ((| self[id]|))
   
--- self[id] = document

    Replace the document (see also #update)
    
   
--- index = index

    set the indexing : ((|index|)) must be an ((|XML::Index|)) object
    
   
--- index

    Retrieve the ((|BDB::XML::Index|))
    
    Return ((|nil|)) if no indexing was specified

--- add_index(uri, name, index, context = nil)
--- add_index(uri, name, type, syntax, context = nil)    

    Add a new index : this is a convenient method. See ((|XML::Index#add|))

--- add_default_index(index, context = nil)

    Add default index : this is a convenient method. See ((|XML::Index#add_default|))
 
--- delete_index(index, context = nil)

    Delete an index : this is a convenient method. See ((|XML::Index#delete|))

--- delete_default_index(index, context = nil)

    Delete default index : this is a convenient method. See ((|XML::Index#delete_default|)) 

--- replace_index(index, context = nil)

    Replace an index : this is a convenient method. See ((|XML::Index#replace|))

--- replace_default_index(index, context = nil)

    Replace default index : this is a convenient method. See ((|XML::Index#replace_default|)) 

--- name

    return the name of the container
    

--- type

    Return the type of the container
   
--- put(document, flags = 0)

    Add a document to the container and return an ID
    
    : ((|document|))
      an object ((|BDB::XML::Document|))
    
    : ((|flags|))
      flags can be 0 or ((|BDB::XML::GEN_NAME|))
    
   
--- transaction

    return the transaction associated with the container, or ((|nil|))
    
   
--- in_transaction?

    return ((|true|)) if the container is associated with a transaction
    
--- transaction?

    same than ((| in_transaction?|))
   
--- update(document, context = nil)

    Update a document within the container
    
    : ((|document|)) 
      an XML::Document to be updated
    : ((|context|)) 
      an optional Update context

--- sync

    Flush database pages for the container to disk

--- flags

    Get the flags used to open the container.

--- node(handle, flags = 0)

    Get the specified node.
   
    : ((|handle|)) 
      must be obtained using XML::Value.node_handle
    : ((|flags|)) 
      must be set to 0 or, ((|BDB::READ_UNCOMMITED|))
      ((|BDB::READ_UNCOMMITED|)), ((|BDB::RMW|))
      ((|BDB::XML::LAZY_DOCS|))

--- event_writer(document, update_context, flags = 0)

    Begins insertion of an ((|XML::Document|)) into the container through use of an 
    ((|XML::EventWriter|)) object.

    Return the ((|XML::EventWriter|)) which must be closed at end.

   
=end
