=begin
== BDB::XML::Context

The Context class encapsulates the context within which a query
is performed against an Container. The context includes namespace 
mappings, variable bindings, and flags that indicate how the query result 
set should be determined and returned to the caller. Multiple queries can 
be executed within the same Context;

Context allows you to define whether queries executed within the
context are to be evaluated lazily or eagerly, and whether the query
is to return live or dead values.

The Query syntax permits expressions to refer to namespace prefixes,
but not to define them. The Context class provides namespace
management methods so that the caller may manage the namespace prefix to
URI mapping. By default the prefix "dbxml" is defined to be
"http://www.sleepycat.com/2002/dbxml". 

The Query syntax also permits expressions to refer to externally
defined variables. The XmlQueryContext class provides methods that
allow the caller to manage the externally-declared variable to value
bindings. 

=== Methods

--- self[variable]

    Get the value bound to a variable
   
--- self[variable] = value

    Bind a value to a variable
   
--- clear_namespaces

    Delete all the namespace prefix mappings
   
--- collection

    Discover the name of the default collection used by fn:collection()
    with no arguments in an XQuery expression.

--- collection = uri

    Set theURI specifying the name of the collection. 
   
    The default collection is that which is used by fn:collection()
    without any arguments in an XQuery expression.

--- del_namespace(name)

    Delete the namespace URI for a particular prefix
   
--- evaltype

    Return the evaluation type
   
--- evaltype = type

    Set the evaluation type
   
--- get_namespace(name)

    Get the namespace URI that a namespace prefix maps onto
   
--- get_results(name)

    Same than Context#[] but return the sequence of values bound to the variable.

--- returntype

    Return the return type
   
--- returntype = type

    Set the return type
   
--- set_namespace(name, uri)

    Define a namespace prefix, providing the URI that it maps onto
    
    If ((|uri|)) is ((|nil|)) delete the namespace

--- uri
 
    Returns the base URI used for relative paths in query expressions.
   
--- uri = val
    
    Sets the base URI used for relative paths in query expressions.
=end
