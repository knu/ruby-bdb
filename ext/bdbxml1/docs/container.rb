# The container name provides the base for the filenames of the database
# files used to store the container content. The directory within which
# the files are opened is taken from the environment passed through the
# Container constructor.  If no environment was provided, then the
# directory used will be the current working directory.
class BDB::XML::Container
   include Enumerable
   class << self
      #
      #create a new Container object
      #
      #* <em>name</em>
      #  the name of the container
      #
      #* <em>flags</em>
      #  The flags must be the string "r", "r+", "w", "w+", "a", "a+" or
      #  and integer value.
      #
      #* <em>mode</em>
      #  mode for creation (see chmod(2))
      #
      #* <em>options</em>
      #  Hash with the possible keys
      #
      #  * <em>env</em>
      #    the Berkeley DB environment within which all database
      #    operations are to be performed.
      #
      #  * <em>txn</em>
      #    the transaction within which all database
      #    operations are to be performed.
      #
      #  * <em>set_pagesize</em>
      #    Set the pagesize of the primary database (512 < size < 64K)
      #
      def  new(name = nil, flags = 0, mode = 0, options = {})
      end
      
      #Dump the container <em>name</em> into the specified file.
      #
      def  dump(name, filename)
      end
      
      #Load data from the specified file into the container <em>name</em>
      #
      def  load(name, filename)
      end
      
      #Remove the container <em>name</em>
      #
      def  remove(name)
      end
      
      #Rename the container <em>name</em>
      #
      def  rename(name, newname)
      end
      
      #Verify the container <em>name</em>, and save the content in 
      #<em>filename</em>
      #
      #* <em>flags</em>
      #  flags can has the value <em>BDB::AGGRESSIVE</em>
      #
      def  salvage(name, filename, flags = 0)
      end
      
      #Set the name for the container <em>name</em>. The underlying files for the
      #container are not renamed - for that, see <em>Container::rename</em>
      #
      def  set_name(name, str)
      end
      
      #Verify the container <em>name</em>
      #
      #
      def  verify(name)
      end
   end
   
   #close an open container
   #
   #* <em>flags</em>
   #  flags can has the value 0 or <em>BDB::NOSYNC</em>
   #
   def  close(flags = 0)
   end
   
   #Remove the document from the container
   #
   #* <em>document</em>
   #  document can be an ID or an <em>BDB::XML::Document</em> previously stored
   #
   #* <em>flags</em>
   #  flags can has the value 0 or <em>BDB::AUTO_COMMIT</em>
   #
   def  delete(document, flags = 0)
   end
   
   #return the current environment for the container, or <em>nil</em>
   #
   def  environment
   end
   #same than <em> environment</em>
   def  env
   end
   
   #return <em>true</em> if the container was opend in an environment
   #
   def  environment?
   end
   #same than <em> environment?</em>
   def  env?
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
   
   #set the indexing : <em>index</em> must be an <em>BDB::XML::Index</em> object
   #
   def  index=(index)
   end
   
   #Retrieve the <em>BDB::XML::Index</em>
   #
   #Return <em>nil</em> if no indexing was specified
   #
   def  index
   end
   
   #in-place modification of all documents according to the state of the 
   #<em>BDB::XML::Modify</em> object, which contains an XPath expression to 
   #target document nodes, as well as specification of the modifications 
   #to perform
   #
   #<em>context</em> is an optional <em>BDB::XML::Context</em> used for the update
   #operations on the container.
   #
   #<em>flags</em> must be set to zero or <em>BDB::RMW</em> to acquire a write lock
   #
   def  modify(mod, context = nil, flags = 0)
   end
   
   #return the name of the container
   #
   def  name
   end
   
   #Set the name of the container. Can be called only on a closed container
   #See also <em>Container::set_name</em>
   #
   def  name=(str)
   end
   
   #return <em>true</em> if the container is open
   #
   def  open?
   end
   
   #Pre-parse an XPath query and return an <em>BDB::XML::XPath</em> object
   #
   #* <em>query</em>
   #  the XPath query to execute against the container
   #
   #* <em>context</em>
   #  the context within which the query will be executed
   #
   def  parse(query, context = nil)
   end
   
   #Add a document to the container and return an ID
   #
   #* <em>document</em>
   #  an object <em>BDB::XML::Document</em> or any object suitable for 
   #  <em>BDB::XML::Document::new</em>
   #
   #* <em>flags</em>
   #  flags can be 0 or <em>BDB::AUTO_COMMIT</em>
   #
   def  push(document, flags = 0)
   end
   
   #Add a document to the container and return <em>self</em>
   #
   def  <<(document)
   end
   
   #Query the container with an XPath expression, which can be an object
   #<em>BDB::XML::XPath</em> or a <em>String</em>
   #
   #* <em>flags</em>
   #  flags can have the value 0 or <em>BDB::DIRTY_READ</em>, <em>BDB::RMW</em>
   #
   #return a <em>BDB::XML::Results</em> object
   #
   def  query(xpath, flags = 0)
   end
   
   #Iterate over the result of a query
   #
   #<em>returntype</em> can have the values <em>BDB::XML::Context::Document</em>
   #or <em>BDB::XML::Context::Values</em>
   #
   #the query is evaluated lazily
   #
   def  search(xpath, returntype = BDB::XML::Context::Document) 
      yield doc
   end
   
   #return the transaction associated with the container, or <em>nil</em>
   #
   def  transaction
   end
   
   #return <em>true</em> if the container is associated with a transaction
   #
   def  in_transaction?
   end
   #same than <em> in_transaction?</em>
   def  transaction?
   end
   
   #Update a document within the container
   #
   def  update(document)
   end
   
   #return an <em>BDB::XML::UpdateContext</em> which can be used to perform
   #<em>[]</em>, <em>[]=</em>, <em>push</em>, <em>delete</em>, <em>update</em> operation
   #    
   #This can be used for a performance improvement
   # 
   def  update_context 
      yield cxt
   end
   #same than <em> update_context {|cxt| ... }</em>
   def  context 
      yield cxt
   end
end
