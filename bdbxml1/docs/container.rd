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
# include Enumerable
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

--- dump(name, filename)
    Dump the container ((|name|)) into the specified file.

--- load(name, filename)
    Load data from the specified file into the container ((|name|))

--- remove(name)
    Remove the container ((|name|))

--- rename(name, newname)
    Rename the container ((|name|))

--- salvage(name, filename, flags = 0)
    Verify the container ((|name|)), and save the content in ((|filename|))

    : ((|flags|))
      flags can has the value ((|BDB::AGGRESSIVE|))

--- set_name(name, str)
    Set the name for the container ((|name|)). The underlying files for the
    container are not renamed - for that, see ((|Container::rename|))

--- verify(name)
    Verify the container ((|name|))


# end

=== Methods

--- close(flags = 0)
    close an open container

    : ((|flags|))
      flags can has the value 0 or ((|BDB::NOSYNC|))

--- delete(document, flags = 0)
    Remove the document from the container

    : ((|document|))
      document can be an ID or an ((|BDB::XML::Document|)) previously stored

    : ((|flags|))
      flags can has the value 0 or ((|BDB::AUTO_COMMIT|))

--- environment
--- env
    return the current environment for the container, or ((|nil|))

--- environment?
--- env?
    return ((|true|)) if the container was opend in an environment

--- each {|doc| ... }
    Iterate over all documents

--- self[id]
--- get(id, flags = 0)
    Fetch the document from the container

    : ((|id|))
      the id assigned to the document when it was first added to a container

    : ((|flags|))
      flags can has the value 0 or ((|BDB::DIRTY_READ|)), ((|BDB::RMW|))

--- self[id] = document
    Replace the document (see also #update)

--- index=(index)
    set the indexing : ((|index|)) must be an ((|BDB::XML::Index|)) object

--- index
    Retrieve the ((|BDB::XML::Index|))

    Return ((|nil|)) if no indexing was specified

--- initialize(name, flags = 0, mode = 0)
    open the container

    : ((|name|))
      see ((|allocate|))

    : ((|flags|))
      The flags must be the string "r", "r+", "w", "w+", "a", "a+" or
      and integer value.

    : ((|mode|))
      mode for creation (see chmod(2))

--- modify(mod, context = nil, flags = 0)
    in-place modification of all documents according to the state of the 
    ((|BDB::XML::Modify|)) object, which contains an XPath expression to 
    target document nodes, as well as specification of the modifications 
    to perform

    ((|context|)) is an optional ((|BDB::XML::Context|)) used for the update
    operations on the container.

    ((|flags|)) must be set to zero or ((|BDB::RMW|)) to acquire a write lock

--- name
    return the name of the container

--- name=(str)
    Set the name of the container. Can be called only on a closed container
    See also ((|Container::set_name|))

--- open?
    return ((|true|)) if the container is open

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

--- search(xpath, returntype = BDB::XML::Context::Document) {|doc| ... }
    Iterate over the result of a query

    ((|returntype|)) can have the values ((|BDB::XML::Context::Document|))
    or ((|BDB::XML::Context::Values|))

    the query is evaluated lazily

--- transaction
    return the transaction associated with the container, or ((|nil|))

--- in_transaction?
--- transaction?
    return ((|true|)) if the container is associated with a transaction

--- update(document)
    Update a document within the container

--- update_context {|cxt| ... }
--- context {|cxt| ... }
    return an ((|BDB::XML::UpdateContext|)) which can be used to perform
    ((|[]|)), ((|[]=|)), ((|push|)), ((|delete|)), ((|update|)) operation
    
    This can be used for a performance improvement
 
# end
# end
# end

=end