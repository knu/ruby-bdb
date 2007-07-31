=begin
== BDB::XML::Manager

Provides a high-level object used to manage various aspects of Berkeley
DB XML usage. You use XML::Manager to perform activities such as container
management (including creation and open), preparing XML::Query queries, 
executing one-off queries, creating transaction objects, 
creating update and query context objects

A XML::Manager object can be created with BDB::Env#manager

BDB::Transaction respond to the same method than BDB::XML::Manager

=== Class Methods

--- new(flags = 0)
    create a new Manager object that uses a private internal 
    database environment
    
    : ((|flags|))
      flags can have the value
      
      : BDB::XML::ALLOW_EXTERNAL_ACCESS 
        to allow queries to access external data sources
      : BDB::XML::ALLOW_AUTO_OPEN 
        to allow queries that reference unopened containers will
        automatically open those containers

=== Methods

--- create_container(name, flags = 0, type = XML::Container::NodeContainer, mode = 0)
    Creates and opens a container. If the container already exists at 
    the time this method is called, an exception is thrown.
   
    : ((|name|))
      container's name
    : ((|flags|)) 
      the flags to use for this container creation.
    : ((|type|)) 
      the type of container to create.
    : ((|mode|)) 
      mode for creation (see chmod(2))

--- create_document
    create a XML::Document object

--- create_modify
    create a XML::Modify object

--- create_query_context(rt = XML::Context::LiveValues, et = XML::Context::Eager)
    create a XML::Context object
    : ((|rt|)) 
      specifies whether to return live (XML::Context::LiveValues)
      or dead values (XML::Context::DeadValues)
    : ((|et|)) 
      specifies if the resultants values are derived and 
      stored in-memory before query evaluation is completed 
      (XML::Context::Eager) or if they are calculated as you ask for them
      (XML::Query::Lazy)

--- create_results
    create a XML::Results object

--- begin(flags = 0)
    create a new transaction
    *((|flags|))

--- create_update_context
    create a XML::Update object

--- dump_container(name, path)
--- dump_container(name, io, to_close = false)

    Dumps the contents of the specified container.
   
    : ((|name|)) 
      name of the container
    : ((|path|))
      the pathname of the file where the contents will be dumped
    : ((|io|)) 
      object which respond to #write
    : ((|to_close|)) 
      if an IO object was given, specify if this object
      must be closed at the end.

--- env
    Returns the underlying environnement

--- home
    Returns the home directory for the underlying database environment

--- load_container(name, path, update = nil)
--- load_container(name, io, to_close = false, update = nil)
   
    Loads data from the specified path (or io) into the container.
   
    : ((|name|))
      name of the container to load.
    : ((|path|)) 
      the pathname of the file which contains the data.
    : ((|io|)) 
      an object which respond to #read
    : ((|update|)) 
      an optional update context

--- open_container(name, flags = 0)
    Opens a container and return an XML::Container object
    : ((|name|)) 
      the container's name
    : ((|flags|)) 
      the flags to use for this container open. Specify
      BDB::CREATE to create and open a new container.

--- prepare(query, context = nil)
    Compile a Query expression and return a new XML::Query object.
   
    You can then use XML::Query#execute
   
    : ((|query|))
      the query to compile
    : ((|context|))
      an optional query context

--- query(query, context = nil) [{res| ... }
    Execute a query. If a block is given it will call Xml::Results#each for
    the results, or it will return an Xml::Results object
   
    This is equivalent to call XML::Manager#prepare and then 
    XML::Query#execute
   
    : ((|query|))
      the query to compile
    : ((|context|))
      an optional query context

--- resolver = object
    Register a resolver

    A resolver is an object which can respond to #resolve_collection,
    #resolve_document, #resolve_entity, #resolve_schema, #resolve_module
    or #resolve_module_location

    These methods (if implemented) must return ((|nil|)) if they can't resolve


    def resolve_collection(txn_or_manager, uri)
       Xml::Results.new
    end
    
    def resolve_document(txn_or_manager, uri)
       Xml::Value.new
    end

    def resolve_entity(txn_or_manager, system_id, public_id)
       'an object which respond to #read'
    end

    def resolve_schema(txn_or_manager, schema_location, namespace)
       'an object which respond to #read'
    end

    def resolve_module(txn_or_manager, module_location, namespace)
       'an object which respond to #read'
    end

    def resolve_module_location(txn_or_manager, namespace)
       Xml::Results.new
    end


--- remove_container(name)
    Remove a container
   
    : ((|name|))
      the name of the container to be removed. The container
      must be closed

--- rename_container(old_name, new_name)
    Rename a container
   
    : ((|old_name|))
      the name of the container to be renamed. 
      The container must be closed and must have been opened at least once
    : ((|new_name|))
      its new name

--- container_flags
    Return the default container flags

--- container_flags = flags
    Set the default container flags for new XML::Container

--- container_type
    Return the default container type

--- container_type = type
    Set the default container type for new XML::Container

--- container_version(file)

    Examines the named ((|file|)), and if it is a container, returns a non-zero 
    database format version. If the ((|file|)) does not exist, or is not a container,
    zero is returned.

--- create_index_lookup(container, uri, name, index, value = nil, op = XML::IndexLookup::EQ)

    Instantiates an new XML::IndexLookup object for performing index lookup
    operations. Only a single index may be specified, and substring indexes
    are not supported.
   
    : ((|container|))
      The target container for the lookup operation
   
    : ((|uri|))
      The namespace of the node to be used. The default namespace is selected
      by passing an empty string for the namespace. 
   
    : ((|name|))
      The name of the element or attribute node to be used. 
   
    : ((|index|))
      A comma-separated list of strings that represent the indexing strategy.
   
    : ((|value|))
      The value to be used as the single value for an equality or inequality
      lookup, or as the lower bound of a range lookup.
   
    : ((|op|))
      Selects the operation to be performed. Must be one of: 
      XML::IndexLookup::NONE, XML::IndexLookup::EQ, XML::IndexLookup::GT, 
      XML::IndexLookup::GTE, XML::IndexLookup::LT, XML::IndexLookup::LTE . 
   

--- pagesize
    Return the current page size value

--- pagesize = size
    Set the current page size value

--- reindex_container(name, context = nil, flags = 0)

    Reindex an entire container. The container should be backed up prior to using
    this method, as it destroys existing indexes before reindexing. If the operation 
    fails, and your container is not backed up, you may lose information.
   
    Use this call to change the type of indexing used for a container between
    document-level indexes and node-level indexes. This method can take a very
    long time to execute, depending on the size of the container, and should
    not be used casually.
    
    : ((|name|)) 
      The path to the container to be reindexed.

    : ((|context|)) 
      The update context to use for the reindex operation.

    : ((|flags|)) 
      Use XML::INDEX_NODES in order to index the container at
      the node level; otherwise, it will be indexed at the document level.


--- sequence_increment = increment

    Sets the integer ((|increment|)) to be used when pre-allocating document ids
    for new documents created by XML::Container::put

--- sequence_increment

    Retrieve the integer increment.

--- update_container(name, context = nil)
    Upgrades the container from a previous version of Berkeley DB XML
   
    : ((|name|))
      name of the container to be upgraded
    : ((|context|))
      an optional update context

--- verify_container(name, flags = 0)
--- verify_container(name, path, flags = BDB::SALVAGE)
--- verify_container(name, io, to_close = false, flags = BDB::SALVAGE)
   
    Checks that the container data files are not corrupt, and optionally 
    writes the salvaged container data to the specified pathname.
   
    : ((|name|))
      name of the container
    : ((|path|))
      name of the file where the container will be
      dumped if BDB::SALVAGE is set.
    : ((|io|))
      an object which respond to #write if BDB::SALVAGE is set
    : ((|to_close|))
      if true the io will be closed at end.
    : ((|flags|))
      can have the value BDB::SALVAGE, BDB::AGGRESSIVE

--- flags

    Get the flags used to open the manager.

--- implicit_timezone

    Get the implicit timezone used for queries

--- implicit_timezone=(tz)

    Set the implicit timezone used for queries

--- compact_container(name, context = Xml::Context.new)

    Compact the databases comprising the container.

--- truncate_container(name, context = XML::Context.new)

    Truncate the container.

=end
