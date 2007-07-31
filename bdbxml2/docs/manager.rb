#
# Provides a high-level object used to manage various aspects of Berkeley
# DB XML usage. You use XML::Manager to perform activities such as container
# management (including creation and open), preparing XML::Query queries, 
# executing one-off queries, creating transaction objects, 
# creating update and query context objects
#
# A XML::Manager object can be created with BDB::Env#manager
#
# BDB::Transaction respond to the same method than BDB::XML::Manager
#
class BDB::XML::Manager
   class << self
      # create a new Manager object that uses a private internal 
      # database environment
      # 
      # * <em>flags</em>
      #   flags can have the value
      #   
      #   * BDB::XML::ALLOW_EXTERNAL_ACCESS to allow queries to access
      #     external data sources
      #   * BDB::XML::ALLOW_AUTO_OPEN to allow queries that reference 
      #     unopened containers will automatically open those containers
      def new(flags = 0)
      end
   end

   # Creates and opens a container. If the container already exists at 
   # the time this method is called, an exception is thrown.
   #
   # * <em>name</em> container's name
   # * <em>flags</em> the flags to use for this container creation.
   # * <em>type</em> the type of container to create.
   # * <em>mode</em> mode for creation (see chmod(2))
   def create_container(name, flags = 0, type = XML::Container::NodeContainer,
                        mode = 0)
   end

   # create a XML::Document object
   def create_document
   end

   # create a XML::Modify object
   def create_modify
   end

   # create a XML::Context object
   # * <em>rt</em> specifies whether to return live (XML::Context::LiveValues)
   #   or dead values (XML::Context::DeadValues)
   # * <em>et</em> specifies if the resultants values are derived and 
   #   stored in-memory before query evaluation is completed 
   #   (XML::Context::Eager) or if they are calculated as you ask for them
   #   (XML::Query::Lazy)
   def create_query_context(rt = XML::Context::LiveValues,
                            et = XML::Context::Eager)
   end

   # create a XML::Results object
   def create_results
   end

   # create a new transaction
   # *<em>flags</em>
   def begin(flags = 0)
   end   

   # Examines the named file, and if it is a container, returns a non-zero 
   # database format version. If the file does not exist, or is not a container,
   # zero is returned.
   def container_version(file)
   end

   # Instantiates an new XML::IndexLookup object for performing index lookup
   # operations. Only a single index may be specified, and substring indexes
   # are not supported.
   #
   # * <em>container</em>
   #   The target container for the lookup operation
   #
   # * <em>uri</em>
   #   The namespace of the node to be used. The default namespace is selected
   #   by passing an empty string for the namespace. 
   #
   # * <em>name</em>
   #   The name of the element or attribute node to be used. 
   #
   # * <em>index</em>
   #   A comma-separated list of strings that represent the indexing strategy.
   #
   # * <em>value</em>
   #   The value to be used as the single value for an equality or inequality
   #   lookup, or as the lower bound of a range lookup.
   #
   # * <em>op</em>
   #   Selects the operation to be performed. Must be one of: 
   #   XML::IndexLookup::NONE, XML::IndexLookup::EQ, XML::IndexLookup::GT, 
   #   XML::IndexLookup::GTE, XML::IndexLookup::LT, XML::IndexLookup::LTE . 
   #
   def create_index_lookup(container, uri, name, index, value = nil, op = XML::IndexLookup::EQ)
   end

   # create a XML::Update object
   def create_update_context
   end

   # call-seq:
   #    dump_container(name, path)
   #    dump_container(name, io, to_close = false)
   # 
   # Dumps the contents of the specified container.
   #
   # * <em>name</em> name of the container
   # * <em>path</em>  or the pathname of the file where the contents
   #    will be dumped
   # * <em>io</em> object which respond to #write
   # * <em>to_close</em> if an IO object was given, specify if this object
   #   must be closed at the end.
   def dump_container(name, io, to_close = false)
   end

   # Returns the underlying environnement
   def env
   end

   # Returns the home directory for the underlying database environment
   def home
   end

   # call-seq:
   #    load_container(name, path, update = nil)
   #    load_container(nmae, io, to_close = false, update = nil)
   #
   # Loads data from the specified path (or io) into the container.
   #
   # * <em>name</em> name of the container to load.
   # * <em>path</em> the pathname of the file which contains the data.
   # * <em>io</em> an object which respond to #read
   # * <em>to_close</em> if true, the object will be closed at end
   # * <em>update</em> an optional update context
   def load_container(name, path, update = nil)
   end

   # Opens a container and return an XML::Container object
   # * <em>name</em> the container's name
   # * <em>flags</em> the flags to use for this container open. Specify
   #   BDB::CREATE to create and open a new container.
   def open_container(name, flags = 0)
   end

   # Compile a Query expression and return a new XML::Query object.
   #
   # You can then use XML::Query#execute
   #
   # * <em>query</em> the query to compile
   # * <em>context</em> an optional query context
   def prepare(query, context = nil)
   end

   # Execute a query. If a block is given it will call XML::::Results#each for
   # the results, or it will return an XML::::Results object
   #
   # This is equivalent to call XML::Manager#prepare and then 
   # XML::Query#execute
   #
   # * <em>query</em> the query to compile
   # * <em>context</em> an optional query context
   def query(query, context = nil)
      yield value
   end

   # Reindex an entire container. The container should be backed up prior to using
   # this method, as it destroys existing indexes before reindexing. If the operation 
   # fails, and your container is not backed up, you may lose information.
   #
   # Use this call to change the type of indexing used for a container between
   # document-level indexes and node-level indexes. This method can take a very
   # long time to execute, depending on the size of the container, and should
   # not be used casually.
   # 
   # * <em>name</em> The path to the container to be reindexed.
   # * <em>context</em> The update context to use for the reindex operation.
   # * <em>flags</em> Use XML::INDEX_NODES in order to index the container at
   #   the node level; otherwise, it will be indexed at the document level.
   def reindex_container(name, context = nil, flags = 0)
   end

   # Register a resolver
   #
   # A resolver is an object which can respond to #resolve_collection,
   # #resolve_document, #resolve_entity, #resolve_schema, #resolve_module
   # or #resolve_module_location
   #
   # These methods (if implemented) must return <em>nil</em> if they can't resolve
   #
   #
   # def resolve_collection(txn_or_manager, uri)
   #    Xml::Results.new
   # end
   # 
   # def resolve_document(txn_or_manager, uri)
   #    Xml::Value.new
   # end
   #
   # def resolve_entity(txn_or_manager, system_id, public_id)
   #    'an object which respond to #read'
   # end
   #
   # def resolve_schema(txn_or_manager, schema_location, namespace)
   #    'an object which respond to #read'
   # end
   #
   # def resolve_module(txn_or_manager, module_location, namespace)
   #    'an object which respond to #read'
   # end
   #
   # def resolve_module_location(txn_or_manager, namespace)
   #    Xml::Results.new
   # end
   #
   def resolver=(object)
   end

   # Remove a container
   #
   # * <em>name</em> the name of the container to be removed. The container
   #   must be closed
   def remove_container(name)
   end

   # Rename a container
   #
   # * <em>old_name</em> the name of the container to be renamed. 
   #   The container must be closed and must have been opened at least once
   # * <em>new_name</em> its new name
   def rename_container(old_name, new_name)
   end

   # Return the default container flags
   def container_flags
   end

   # Set the default container flags for new XML::Container
   def container_flags=(flags)
   end

   # Return the default container type
   def container_type
   end

   # Set the default container type for new XML::Container
   def container_type=(type)
   end

   # Return the current page size value
   def pagesize
   end

   # Set the current page size value
   def pagesize=(size)
   end

   # Upgrades the container from a previous version of Berkeley DB XML
   #
   # * <em>name</em> name of the container to be upgraded
   # * <em>context</em> an optional update context
   def update_container(name, context = nil)
   end

   # call-seq:
   #    verify_container(name, flags = 0)
   #    verify_container(name, path, flags = BDB::SALVAGE)
   #    verify_container(name, io, to_close = false, flags = BDB::SALVAGE)
   #
   # Checks that the container data files are not corrupt, and optionally 
   # writes the salvaged container data to the specified pathname.
   #
   # * <em>name</em> name of the container
   # * <em>path</em> name of the file where the container will be
   #   dumped if BDB::SALVAGE is set.
   # * <em>io</em> an object which respond to #write if BDB::SALVAGE is set
   # * <emto_close</em> if true, the io will be closed at end
   # * <em>flags</em> can have the value BDB::SALVAGE, BDB::AGGRESSIVE
   def verify_container(name, path, flags = 0)
   end

   # Sets the integer increment to be used when pre-allocating document ids
   # for new documents created by XML::Container::put
   def sequence_increment=(increment)
   end

   # Retrieve the integer increment.
   def sequence_increment
   end

   # Get the flags used to open the manager.
   def flags
   end

   # Get the implicit timezone used for queries
   def implicit_timezone
   end

   # Set the implicit timezone used for queries
   def implicit_timezone=(tz)
   end

   # Compact the databases comprising the container.
   def compact_container(name, context = Xml::Context.new)
   end

   # Truncate the container.
   def truncate_container(name, context = XML::Context.new)
   end

end
