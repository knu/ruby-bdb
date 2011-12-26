#
# The XML::Container class encapsulates a document container and its 
# related indices and statistics. XML::Container exposes methods for
# managing (putting and deleting) XML::Document objects, managing indices,
# and retrieving container statistics.
class BDB::XML::Container
   include Enumerable
   # Remove the document from the container
   #
   # * <em>doc_or_name</em>
   #   doc_or_name is an <em>BDB::XML::Document</em> previously stored,
   #   or the name of an existent document
   #
   # * <em>context</em> an optional Update context
   def  delete(doc_or_name, context = nil)
   end
   
   # return the current manager for the container, or <em>nil</em>
   #
   def manager
   end
   
   #Iterate over all documents
   #
   def  each 
      yield doc
   end
   
   #Fetch the document from the container
   #
   #* <em>id</em>
   #  the id assigned to the document when it was first added to a container
   #
   #* <em>flags</em>
   #  flags can has the value 0 or <em>BDB::DIRTY_READ</em>, <em>BDB::RMW</em>
   #
   def  self[id]
   end
   #same than <em> self[id]</em>
   def  get(id, flags = 0)
   end
   
   #Replace the document (see also #update)
   #
   def  self[id] = document
   end
   
   #set the indexing : <em>index</em> must be an <em>XML::Index</em> object
   #
   def  index=(index)
   end
   
   #Retrieve the <em>BDB::XML::Index</em>
   #
   #Return <em>nil</em> if no indexing was specified
   #
   def  index
   end
   
   # return the name of the container
   #
   def  name
   end

   # Return the type of the container
   def type
   end
   
   # Add a document to the container and return an ID
   #
   # * <em>document</em>
   #   an object <em>BDB::XML::Document</em>
   #
   # * <em>flags</em>
   #   flags can be 0 or <em>BDB::XML::GEN_NAME</em>
   #
   def  put(document, flags = 0)
   end
   
   # return the transaction associated with the container, or <em>nil</em>
   #
   def  transaction
   end
   
   # return <em>true</em> if the container is associated with a transaction
   #
   def  in_transaction?
   end
   #same than <em> in_transaction?</em>
   def  transaction?
   end
   
   # Update a document within the container
   #
   # * <em>document</em> an XML::Document to be updated
   # * <em>context</em> an optional Update context
   def  update(document, context = nil)
   end

   # Flush database pages for the container to disk
   def sync
   end

   # Returns true if the container is configured to create node indexes.
   def index?
   end

   # Return database page size.
   def pagesize
   end

   # call-seq:
   #    add_index(uri, name, index, context = nil)
   #    add_index(uri, name, type, syntax, context = nil)
   #
   # Add a new index : this is a convenient method. See <em>XML::Index#add</em>
   def add_index()
   end

   # Add the default index : this is a convenient method. See 
   # <em>XML::Index#add_default</em>
   def add_default(index, context = nil)
   end

   # call-seq:
   #    delete_index(uri, name, index, context = nil)
   #    delete_index(uri, name, type, syntax, context = nil)
   #
   # Delete the index : this is a convenient method. See <em>XML::Index#delete</em>
   def delete_index()
   end

   # Delete the default index : this is a convenient method. See 
   # <em>XML::Index#delete_default</em>
   def delete_default(index, context = nil)
   end

   # call-seq:
   #    replace_index(uri, name, index, context = nil)
   #    replace_index(uri, name, type, syntax, context = nil)
   #
   # Replace the index : this is a convenient method. See <em>XML::Index#replace</em>
   def replace_index()
   end

   # Replace the default index : this is a convenient method. See 
   # <em>XML::Index#replace_default</em>
   def replace_default(index, context = nil)
   end

   # Get the flags used to open the container.
   def flags
   end

   # Get the specified node.
   #
   # * <em>handle</em> must be obtained using XML::Value.node_handle
   # * <em>flags</em> must be set to 0 or, <em>BDB::READ_UNCOMMITED</em>
   #   <em>BDB::READ_UNCOMMITED</em>, <em>BDB::RMW</em>
   #   <em>BDB::XML::LAZY_DOCS</em>
   def node(handle, flags = 0)
   end

   # Begins insertion of an <em>XML::Document</em> into the container through use of an 
   # <em>XML::EventWriter</em> object.
   #
   # Return the <em>XML::EventWriter</em> which must be closed at end.
   #
   def event_writer(document, update_context, flags = 0)
   end

end
