=begin

== BDB::XML::Modify

Modify encapsulates the context within which a set of documents specified by
a query can be modified in place. 

The modification is performed using the methods 
((|BDB::XML::Container#modify|)) and ((|BDB::XML::Document#modify|))

There are two parts to the object -- the query and the operation. 
The query identifies target nodes against which the operation is run. 
The operation specifies what modifications to perform.

# module BDB
# module XML
# class Modify
# class << self

=== Class Methods

--- allocate
    Allocate a new object

# end

=== Methods

--- count
    return the number of node hits in the specified query.

--- initialize(xpath, operation, type = BDB::XML::Modify::None, name = "", content = "", location = -1, context = nil)

    ((|xpath|)) is a ((|String|)), or an ((|BDB::XML::Context|)) object

    ((|operation|)) can have the value ((|BDB::XML::Modify::InsertAfter|)), 
    ((|BDB::XML::Modify::InsertBefore|)), ((|BDB::XML::Modify::Append|)),
    ((|BDB::XML::Modify::Update|)), ((|BDB::XML::Modify::Remove|)) or 
    ((|BDB::XML::Modify::Rename|)).

    ((|type|)) can have the value ((|BDB::XML::Modify::Element|)), 
    ((|BDB::XML::Modify::Attribute|)), ((|BDB::XML::Modify::Text|)),
    ((|BDB::XML::Modify::ProcessingInstruction|)), ((|BDB::XML::Modify::Comment|)) or
    ((|BDB::XML::Modify::None|)).

    ((|name|)) is the name for the new content.
 
    ((|content|)) the content for operations that require content.

    ((|location|)) indicate where a new child node will be placed for the
    append operation

    ((|context|)) is valid only when a String is given as the first argument.
    This must be a ((|BDB::XML::Context|)) which  contains the variable 
    bindings, the namespace prefix to URI mapping, and the query processing
    flags.

--- encoding=(string)
    Specify a new character encoding for the modified documents.

# end
# end
# end

=end
