=begin
= BDB::XML

# module BDB
#^

Berkeley DB XML is an embedded native XML datastore that provides for
the efficient storage and retrieval of XML encoded information.

Retrieval is supported by an XPath query engine that derives its efficiency
from indices generated from the stored XML data.
#^
# module XML
#

((<BDB::XML::Container|URL:docs/container.html>))

#^
The container name provides the base for the filenames of the database
files used to store the container content. The directory within which
the files are opened is taken from the environment passed through the
Container constructor.  If no environment was provided, then the
directory used will be the current working directory.
#^
# class Container
# end

((<BDB::XML::Index|URL:docs/index.html>))

#^
Define the indexing strategy for a Container

Indexing is specified by providing the name of a node and a list of indexing
strategies for that node.
#^
# class Index
# end

((<BDB::XML::Document|URL:docs/document.html>))

#^
A Document is the unit of storage within a Container. 

A Document contains a stream of bytes that may be of type XML.
The Container only indexes the content of Documents
that contain XML.

Document supports annotation attributes.

#^
# class Document
# end

((<BDB::XML::Context|URL:docs/context.html>))

#^
The context within which a query is performed against a Container. 

The context within which a query is performed against a Container
is complex enough to warrent an encapsulating class. This context
includes a namespace mapping, variable bindings, and flags that 
determine how the query result set should be determined and returned
to the caller.

The XPath syntax permits expressions to refer to namespace prefixes, but
not to define them. The Context class provides a number of namespace 
management methods so that the caller may manage the namespace prefix
to URI mapping.

The XPath syntax also permits expressions to refer to variables, but not
to define them. The Context class provides a number of methods so
that the caller may manage the variable to value bindings.
#^
# class Context
# end

((<BDB::XML::Modify|URL:docs/modify.html>))

#^
Modify encapsulates the context within which a set of documents specified by
a query can be modified in place. 

The modification is performed using the methods 
((|BDB::XML::Container#modify|)) and ((|BDB::XML::Document#modify|))

There are two parts to the object -- the query and the operation. 
The query identifies target nodes against which the operation is run. 
The operation specifies what modifications to perform.
#^
# class Modify
# end

((<BDB::XML::Results|URL:docs/results.html>))
#^

The results of a query are a collection of values. The values could
be either String, Float, true, false, BDB::XML::Documents or BDB::XML::Nodes

#^

Implement only the method ((|each|))
#
# class Results
# end
# end
# end

=end
