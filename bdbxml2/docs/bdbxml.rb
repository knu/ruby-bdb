# 
# Berkeley DB XML is an embedded native XML datastore that provides for
# the efficient storage and retrieval of XML encoded information.
# 
# Retrieval is supported by an Query query engine that derives its efficiency
# from indices generated from the stored XML data.
#
# The following classes are defined
#
# * BDB::XML::Manager
#
#   Provides a high-level object used to manage various aspects of Berkeley
#   DB XML usage. You use XML::Manager to perform activities such as container
#   management (including creation and open), preparing XQuery queries, 
#   executing one-off queries, creating transaction objects, 
#   creating update and query context objects
#
#   A XML::Manager object can be created with BDB::Env#manager
#
#   BDB::Transaction respond to the same method than BDB::XML::Manager
#
# * BDB::XML::Container
#
#   The XML::Container class encapsulates a document container and its 
#   related indices and statistics. XML::Container exposes methods for
#   managing (putting and deleting) XML::Document objects, managing indices,
#   and retrieving container statistics.
#
# * BDB::XML::Index
#
#   Define the indexing strategy for a Container
# 
#   Indexing is specified by providing the name of a node and a list of
#   indexing strategies for that node.
#
# * BDB::XML::Document
#
#   A Document is the unit of storage within a Container. A document consists
#   of content, a name, and a set of metadata attributes.
#
#   The document content is a byte stream. It must be well formed XML, 
#   but need not be valid.
#
# * BDB::XML::Context
#
#   The Context class encapsulates the context within which a query
#   is performed against an Container. The context includes namespace 
#   mappings, variable bindings, and flags that indicate how the query result 
#   set should be determined and returned to the caller. Multiple queries can 
#   be executed within the same Context;
#   
#   Context allows you to define whether queries executed within the
#   context are to be evaluated lazily or eagerly, and whether the query
#   is to return live or dead values.
#
#   The Query syntax permits expressions to refer to namespace prefixes,
#   but not to define them. The Context class provides namespace
#   management methods so that the caller may manage the namespace prefix to
#   URI mapping. By default the prefix "dbxml" is defined to be
#   "http://www.sleepycat.com/2002/dbxml". 
#
#   The Query syntax also permits expressions to refer to externally
#   defined variables. The XmlQueryContext class provides methods that
#   allow the caller to manage the externally-declared variable to value
#   bindings. 
#
# * BDB::XML::Modify
#
#   The XML::Modify class encapsulates the context within which a set of one
#   or more documents specified by an XML::Query query can be modified in
#   place. The modification is performed using an XML::Modify object, and a
#   series of methods off that object that identify how the document is to
#   be modified. Using these methods, the modification steps are
#   identified. When the object is executed, these steps are performed in
#   the order that they were specified. 
# 
#   The modification steps are executed against one or more documents
#   using XML::Modify::execute. This method can operate on a single document
#   stored in an XML::Value, or against multiple documents stored in an
#   XML::Results set that was created as the result of a container or
#   document query. 
#
# * BDB::XML::Results
#
#   The results of a query are a collection of values. The values could
#   be either String, Float, true, false, BDB::XML::Value
#   
# * BDB::XML::Value
#
#   Class which encapsulates the value of a node in an XML document.
#
# * BDB::XML::EventWriter
#
#   Class which enables applications to construct document content without using serialized XML.
#
# * BDB::XML::EventReader
#
#   Class which enables applications to read document content via a pull interface without
#   materializing XML as text.
#
#
module BDB::XML
end
