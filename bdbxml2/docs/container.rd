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
   
--- manager

    return the current manager for the container, or ((|nil|))
   
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
   
=end
