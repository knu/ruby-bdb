=begin

== BDB::XML::Container

The container name provides the base for the filenames of the database
files used to store the container content. The directory within which
the files are opened is taken from the environment passed through the
Container constructor.  If no environment was provided, then the
directory used will be the current working directory.

# module BDB
# module XML
# class Container
# class << self

=== Class Methods

--- allocate(name = nil, options = {})

    allocate a new Container object

    : ((|name|))
      the name of the container

    : ((|options|))
      Hash with the possible keys

      : ((|env|))
        the Berkeley DB environment within which all database
        operations are to be performed.

      : ((|txn|))
        the transaction within which all database
        operations are to be performed.

      : ((|set_pagesize|))
        Set the pagesize of the primary database (512 < size < 64K)

# end

=== Methods

--- close(flags = 0)
    close an open container

    : ((|flags|))
      flags can has the value 0 or ((|BDB::NOSYNC|))

--- delete(document, flags = 0)
    Remove the document from the container

    : ((|document|))
      document can be an ID or an ((|BDB::XML::Document|)) previuosly stored

    : ((|flags|))
      flags can has the value 0 or ((|BDB::AUTO_COMMIT|))

--- each(xpath, returntype = BDB::XML::Content::Document) {|doc| ... }
    Iterate over the result of a query

    ((|returntype|)) can have the values ((|BDB::XML::Content::Document|))
    or ((|BDB::XML::Content::Values|))

    the query is evaluated lazily

--- self[id]
--- get(id, flags = 0)
    Fetch the document from the container

    : ((|id|))
      the id assigned to the document when it was first added to a container

    : ((|flags|))
      flags can has the value 0 or ((|BDB::DIRTY_READ|)), ((|BDB::RMW|))

--- index(element, index)
    Declare the indexing required for a particular element type.   

    : ((|element|))
      The element parameter provides the fully qualified element type
      to be indexed.

    : ((|index|))
      The index string is a comma separated list of the following indexing
      strategy names

         : none-none-none-none
         : node-element-presence-none
         : node-element-equality-string
         : node-element-equality-number
         : node-element-substring-string
         : edge-element-presence-none
         : node-attribute-presence-none
         : node-attribute-equality-string
         : node-attribute-equality-number
         : node-attribute-substring-string
         : edge-attribute-presence-none


--- initialize(name, flags = 0, mode = 0)
    open the container

    : ((|name|))
      see ((|allocate|))

    : ((|flags|))
      The flags must be the string "r", "r+", "w", "w+", "a", "a+" or
      and integer value.

    : ((|mode|))
      mode for creation (see chmod(2))

--- name
    return the name of the container

--- parse(query, context = nil)
    Pre-parse an XPath query and return an ((|BDB::XML::XPath|)) object

    : ((|query|))
      the XPath query to execute against the container

    : ((|context|))
      the context within which the query will be executed

--- push(document, flags = 0)
    Add a document to the container and return an ID

    : ((|document|))
      an object ((|BDB::XML::Document|)) or any object suitable for 
      ((|BDB::XML::Document::new|))

    : ((|flags|))
      flags can be 0 or ((|BDB::AUTO_COMMIT|))

--- <<(document)
    Add a document to the container and return ((|self|))

--- query(xpath, flags = 0)
--- query(string, context, flags = 0)
    Query the container with an XPath expression, which can be an object
    ((|BDB::XML::XPath|)) or a ((|String|))

    : ((|flags|))
      flags can have the value 0 or ((|BDB::DIRTY_READ|)), ((|BDB::RMW|))

    return a ((|BDB::XML::Results|)) object

--- remove
    Remove the container

--- rename(newname)
    Rename the container

# end
# end
# end

=end