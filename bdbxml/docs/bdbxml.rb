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
# A Document is the unit of storage within a Container. 
# 
# A Document contains a stream of bytes that may be of type XML
# or BYTES. The Container only indexes the content of Documents
# that contain XML. The BYTES type is supported to allow the storage
# of arbitrary content along with XML content.
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

#Return the encoding
#
def  encoding
end

#Return the ID
#
def  id
end

#Initialize the document with the type (or the content) specified
#
def  initialize(type = BDB::XML::XML)
end

#Return the name of the document
#
def  name
end

#Set the name of the document
#
def  name=(val)
end

#Return the document as a String object
#
def  to_s
end
#same than <em> to_s</em>
def  to_str
end

#Return the type of the document
#
def  types
end

#Set the type of the document
#
def  types=(val)
end
end
end
end
module BDB
module XML
class Container
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
#  document can be an ID or an <em>BDB::XML::Document</em> previuosly stored
#
#* <em>flags</em>
#  flags can has the value 0 or <em>BDB::AUTO_COMMIT</em>
#
def  delete(document, flags = 0)
end

#Iterate over the result of a query
#
#<em>returntype</em> can have the values <em>BDB::XML::Content::Document</em>
#or <em>BDB::XML::Content::Values</em>
#
#the query is evaluated lazily
#
def  each(xpath, returntype = BDB::XML::Content::Document) 
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

#Declare the indexing required for a particular element type.   
#
#* <em>element</em>
#  The element parameter provides the fully qualified element type
#  to be indexed.
#
#* <em>index</em>
#  The index string is a comma separated list of the following indexing
#  strategy names
#
#  * none-none-none-none
#  * node-element-presence-none
#  * node-element-equality-string
#  * node-element-equality-number
#  * node-element-substring-string
#  * edge-element-presence-none
#  * node-attribute-presence-none
#  * node-attribute-equality-string
#  * node-attribute-equality-number
#  * node-attribute-substring-string
#  * edge-attribute-presence-none
#
#
def  index(element, index)
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

#Remove the container
#
def  remove
end

#Rename the container
#
def  rename(newname)
end
end
end
end
module BDB
module XML
class Results

#Iterate on each values
#
def  each 
yield val
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

#Set the evaluation type
#
def  evaltype=(type)
end

#Get the namespace URI that a namespace prefix maps onto
#
def  get_namespace(name)
end
#same than <em> get_namespace</em>
def  namespace(name)
end

#Initialize the object with the optional evaluation type
#<em>BDB::XML::Context::Lazy</em> or <em>BDB::XML::Context::Eager</em>
#and return type <em>BDB::XML::Context::Documents</em>,
#<em>BDB::XML::Context::Values</em> or <em>BDB::XML::Context::Candidates</em>
#
def  initialize(evaluation = nil, returntype = nil)
end

#Set the return type
#
def  returntype=(type)
end

#Define a namespace prefix, providing the URI that it maps onto
#
def  set_namespace(name, uri)
end
end
end
end
