# A Document is the unit of storage within a Container. A document consists
# of content, a name, and a set of metadata attributes.
#
# The document content is a byte stream. It must be well formed XML, 
# but need not be valid.
class BDB::XML::Document
   include Enumerable
   
   # Returns the value of the specified metadata.
   #
   def  [](name)
   end
   
   # Set the value of the specified metadata.
   #
   def  []=(name, val)
   end
   
   # Return the content of the document
   #
   def  content
   end
   
   # Set the content of the document
   #
   def  content=(val)
   end

   # call-seq:
   #    each
   #    each_metadata
   #
   # Iterate over all metadata
   def each
       yield uri, name, value
   end
   
   # call-seq:
   #    get_metadata(name)
   #    get_metadata(uri, name)
   #    get(name)
   #    get(uri, name)
   # 
   # Get the identified metadata from the document.
   #
   # * <em>uri</em> The namespace within which the name resides.
   # * <em>name</em> The name of the metadata attribute to be retrieved.
   #
   def  get_metadata(uri, name)
   end

   # Return the manager
   def manager
   end

   # Return the name of the document
   #
   def  name
   end
   
   # Set the name of the document
   #
   def  name=(val)
   end

   # call-seq:
   #    remove_metadata(name)
   #    remove_metadata(uri, name)
   # 
   # Remove the identified metadata from the document.
   #
   # * <em>uri</em> The namespace within which the name resides.
   # * <em>name</em> The name of the metadata attribute to be removed.
   #
   def  remove_metadata(uri, name)
   end
   
   # call-seq:
   #    set_metadata(name, value)
   #    set_metadata(uri, name, value)
   #    set(name, value)
   #    set(uri, name, value)
   # 
   # Set the identified metadata from the document.
   #
   # * <em>uri</em> The namespace within which the name resides.
   # * <em>name</em> The name of the metadata attribute to be set.
   # * <em>value</em> an XML::Value object
   #
   def  set_metadata(uri, name, value)
   end

   # Return the document as a String object
   #
   def  to_s
   end

   # same than <em> to_s</em>
   def  to_str
   end

   # Retrieve content as an XML::EventReader.
   def event_reader
   end
end
