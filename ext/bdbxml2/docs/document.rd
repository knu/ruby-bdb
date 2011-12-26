=begin
== BDB::XML::Document

A Document is the unit of storage within a Container. A document consists
of content, a name, and a set of metadata attributes.

The document content is a byte stream. It must be well formed XML, 
but need not be valid.

included module : Enumerable

=== Methods
   
--- self[name]

    Returns the value of the specified metadata.

--- self[name] =  val

    Set the value of the specified metadata.

   
--- content

    Return the content of the document

   
--- content = val

    Set the content of the document


--- each {|uri, name, value| ... }
--- each_metadata {|uri, name, value| ... }

    Iterate over all metadata
   
--- get_metadata(uri, name)
--- get_metadata(name)
--- get(uri, name)
--- get(name)
    
    Get the identified metadata from the document.

    * ((|uri|))
      The namespace within which the name resides.
    * ((|name|))
      The name of the metadata attribute to be retrieved.

--- manager

    Return the manager

--- name

    Return the name of the document
   
--- name = val

    Set the name of the document


--- remove_metadata(uri, name)
--- remove_metadata(name)
    
    Remove the identified metadata from the document.

    * ((|uri|))
      The namespace within which the name resides.
    * ((|name|))
      The name of the metadata attribute to be removed.
      
   
--- set_metadata(name, value)
--- set_metadata(uri, name, value)
--- set(name, value)
--- set(uri, name, value)
    
    Set the identified metadata from the document.

    * ((|uri|))
      The namespace within which the name resides.
    * ((|name|))
      The name of the metadata attribute to be set.
    * ((|value|))
      an XML::Value object


--- to_s
    Return the document as a String object


--- to_str
    same than ((|to_s|))


--- event_reader

    Retrieve content as an XML::EventReader.

=end
