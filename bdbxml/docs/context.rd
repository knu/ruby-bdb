=begin
== BDB::XML::Context

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

# module BDB
# module XML
# class Context
# class << self

=== Class Methods

--- allocate
    Allocate a new Context object
# end

=== Methods

--- self[variable]
    Get the value bound to a variable

--- self[variable] = value
    Bind a value to a variable

--- clear_namespaces
--- clear
    Delete all the namespace prefix mappings

--- del_namespace(name)
    Delete the namespace URI for a particular prefix

--- evaltype
    Return the evaluation type

--- evaltype=(type)
    Set the evaluation type

--- get_namespace(name)
--- namespace[name]
    Get the namespace URI that a namespace prefix maps onto

--- initialize(returntype = nil, evaluation = nil)
    Initialize the object with the optional evaluation type
    ((|BDB::XML::Context::Lazy|)) or ((|BDB::XML::Context::Eager|))
    and return type ((|BDB::XML::Context::Documents|)),
    ((|BDB::XML::Context::Values|)) or ((|BDB::XML::Context::Candidates|))

--- metadata
--- with_metadata
    return ((|true|)) if the metadata is added to the document

--- metadata=(with)
--- with_metadata=(with)
    The ((|with|)) parameter specifies whether or not to add the document
    metadata prior to the query.

--- returntype
    Return the return type

--- returntype=(type)
    Set the return type

--- namespace[name]=(uri)
--- set_namespace(name, uri)
    Define a namespace prefix, providing the URI that it maps onto

    If ((|uri|)) is ((|nil|)) delete the namespace
# end
# end
# end

=end
