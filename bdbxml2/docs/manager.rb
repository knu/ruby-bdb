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

   # Execute a query. If a block is given it will call Xml::Results#each for
   # the results, or it will return an Xml::Results object
   #
   # This is equivalent to call XML::Manager#prepare and then 
   # XML::Query#execute
   #
   # * <em>query</em> the query to compile
   # * <em>context</em> an optional query context
   def query(query, context = nil)
      yield value
   end

   # Register a resolver
   #
   # A resolver is an object which can respond to #resolve_collection,
   # #resolve_document, #resolve_entity, or #resolve_schema
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
end
