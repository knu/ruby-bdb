module BDB
# 
# Berkeley DB XML is an embedded native XML datastore that provides for
# the efficient storage and retrieval of XML encoded information.
# 
# Retrieval is supported by an XPath query engine that derives its efficiency
# from indices generated from the stored XML data.
module XML
# The container name provides the base for the filenames of the database
# files used to store the container content. The directory within which
# the files are opened is taken from the environment passed through the
# Container constructor.  If no environment was provided, then the
# directory used will be the current working directory.
class Container
end
# define the indexing strategy for a Container
# 
class Index
end
# A Document is the unit of storage within a Container. 
# 
# A Document contains a stream of bytes that may be of type XML.
# The Container only indexes the content of Documents
# that contain XML.
# 
# Document supports annotation attributes.
# 
class Document
end
# The context within which a query is performed against a Container. 
# 
# The context within which a query is performed against a Container
# is complex enough to warrent an encapsulating class. This context
# includes a namespace mapping, variable bindings, and flags that 
# determine how the query result set should be determined and returned
# to the caller.
# 
# The XPath syntax permits expressions to refer to namespace prefixes, but
# not to define them. The Context class provides a number of namespace 
# management methods so that the caller may manage the namespace prefix
# to URI mapping.
# 
# The XPath syntax also permits expressions to refer to variables, but not
# to define them. The Context class provides a number of methods so
# that the caller may manage the variable to value bindings.
class Context
end
# 
# The results of a query are a collection of values. The values could
# be either String, Float, true, false, BDB::XML::Documents or BDB::XML::Nodes
# 
class Results
end
end
end
module BDB
module XML
class Container
include Enumerable
class << self

#
#allocate a new Container object
#
#* <em>name</em>
#  the name of the container
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
def  allocate(name = nil, options = {})
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

#Verify the container <em>name</em>, and save the content in <em>filename</em>
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

#open the container
#
#* <em>name</em>
#  see <em>allocate</em>
#
#* <em>flags</em>
#  The flags must be the string "r", "r+", "w", "w+", "a", "a+" or
#  and integer value.
#
#* <em>mode</em>
#  mode for creation (see chmod(2))
#
def  initialize(name, flags = 0, mode = 0)
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
end
end
module BDB
module XML
class Context
class << self

#Allocate a new Context object
def  allocate
end
end

#Get the value bound to a variable
#
def  self[variable]
end

#Bind a value to a variable
#
def  self[variable] = value
end

#Delete all the namespace prefix mappings
#
def  clear_namespaces
end
#same than <em> clear_namespaces</em>
def  clear
end

#Delete the namespace URI for a particular prefix
#
def  del_namespace(name)
end

#Return the evaluation type
#
def  evaltype
end

#Set the evaluation type
#
def  evaltype=(type)
end

#Get the namespace URI that a namespace prefix maps onto
#
def  get_namespace(name)
end
#same than <em> get_namespace</em>
def  namespace[name]
end

#Initialize the object with the optional evaluation type
#<em>BDB::XML::Context::Lazy</em> or <em>BDB::XML::Context::Eager</em>
#and return type <em>BDB::XML::Context::Documents</em>,
#<em>BDB::XML::Context::Values</em> or <em>BDB::XML::Context::Candidates</em>
#
def  initialize(returntype = nil, evaluation = nil)
end

#Return the return type
#
def  returntype
end

#Set the return type
#
def  returntype=(type)
end

#Define a namespace prefix, providing the URI that it maps onto
#
#If <em>uri</em> is <em>nil</em> delete the namespace
def  namespace[name]=(uri)
end
#same than <em> namespace[name]=</em>
def  set_namespace(name, uri)
end
end
end
end
module BDB
module XML
class Document
class << self

#Allocate a new object
#
def  allocate
end
end

#Return the value of an attribute
#
def  self[attr]
end

#Set the value of an attribute
#
def  self[attr] = val
end

#Return the content of the document
#
def  content
end

#Set the content of the document
#
def  content=(val)
end

#Return the value of an attribute. <em>uri</em> specify the namespace
#where reside the attribute.
#
def  get(uri = "", attr)
end

#Return the ID
#
def  id
end

#Initialize the document with the content specified
#
def  initialize(content = "")
end

#Return the name of the document
#
def  name
end

#Set the name of the document
#
def  name=(val)
end

#Return the default prefix for the namespace
#
def  prefix
end

#Set the default prefix used by <em>set</em>
#
def  prefix=(val)
end

#Execute the XPath expression <em>xpath</em> against the document
#Return an <em>BDB::XML::Results</em>
#
def  query(xpath, context = nil)
end

#Set an attribute in the namespace <em>uri</em>. <em>prefix</em> is the prefix
#for the namespace
#
def  set(uri = "", prefix = "", attr, value)
end

#Return the document as a String object
#
def  to_s
end
#same than <em> to_s</em>
def  to_str
end

#Return the default namespace
#
def  uri
end

#Set the default namespace used by <em>set</em> and <em>get</em>
#
def  uri=(val)
end
end
end
end
module BDB
module XML
class Index
class << self

#Allocate a new Index object
def  allocate
end
end

#
#add a new index
#
#* <em>uri</em>
#  The namespace for the element (optional)
#
#* <em>name</em>
#  The name parameter provides the fully qualified element type
#  to be indexed.
#
#* <em>index</em>
#  The index string is a comma separated list of the following indexing
#  strategy names
#
#  * none-none-none-none 
#  * node-element-presence 
#  * node-attribute-presence 
#  * node-element-equality-string 
#  * node-element-equality-number 
#  * node-element-substring-string 
#  * node-attribute-equality-string 
#  * node-attribute-equality-number 
#  * node-attribute-substring-string 
#  * edge-element-presence 
#  * edge-attribute-presence 
#  * edge-element-equality-string 
#  * edge-element-equality-number 
#  * edge-element-substring-string 
#  * edge-attribute-equality-string 
#  * edge-attribute-equality-number 
#  * edge-attribute-substring-string 
#
def  add(uri = "", name, index)
end
#same than <em> add</em>
def  push(uri = "", name, index)
end

#Delete the specified index
#
def  delete(uri = "", name, index)
end

#Iterate over all indexes
#
def  each 
yield uri, name, index
end

#Initialize the index with the optional values [uri, name, value]
#
def  initialize([[uri0, name0, index0] [, [uri1, name1, index1], ... ]])
end

#Replace the specified index
#
def  replace(uri = "", name, index)
end

#Return an Array of all indexes
#
def  to_a
end
end
end
end
module BDB
module XML
class Results
include Enumerable

#Iterate on each values
#
def  each 
yield val
end
end
end
end
