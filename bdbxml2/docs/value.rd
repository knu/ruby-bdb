=begin
== BDB::XML::Value

The XML::Value class encapsulates the value of a node in an XML document.

=== Methods

--- attributes

    return an XML::Results that contains all of the attributes appearing
    on this node.
    
    node type must be XML::Value::Node

--- boolean?

    return true if type is XML::Value::BOOLEAN

--- first_child

    returns  current node's first child node, or nil
    
    node type must be XML::Value::Node

--- last_child

    returns  current node's last child node, or nil
    
    node type must be XML::Value::Node

--- local_name

    returns the node's local name.
    
    node type must be XML::Value::Node

--- namespace

    returns the URI used for the node's namespace.
    
    node type must be XML::Value::Node

--- next_sibling

    returns the sibling node immediately following this node in the
    document, or nil.
    
    node type must be XML::Value::Node

--- nil?

    return true if type is XML::Value::NONE

--- node?

    return true if type is XML::Value::NODE

--- node_name

    return the type of the value contained in this value
    
    node type must be XML::Value::Node

--- node_value

    return the node's value
    
    node type must be XML::Value::Node

--- number?

    return true if type is one of the numeric types

--- owner_element

    If the current node is an attribute node, returns the document
    element node that contains this attribute node. Otherwise, raise
    an exception
    
    node type must be XML::Value::Node

--- parent_node

    returns current node's parent, or nil
    
    node type must be XML::Value::Node

--- prefix

    returns the prefix set for the node's namespace.
    
    node type must be XML::Value::Node

--- previous_sibling

    returns  the sibling node immediately preceding this node
    in the document, or nil
    
    node type must be XML::Value::Node

--- string?

    return true if type is XML::Value::STRING

--- type?(type)

    return true if the value has the specified type

--- type

    return the type associated with a value

=end
