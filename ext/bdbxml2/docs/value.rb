# 
# The XML::Value class encapsulates the value of a node in an XML document.
# 
class BDB::XML::Value
   # return an XML::Results that contains all of the attributes appearing
   # on this node.
   # 
   # node type must be XML::Value::Node
   def attributes
   end

   # return true if type is XML::Value::BOOLEAN
   def boolean?
   end

   # returns  current node's first child node, or nil
   # 
   # node type must be XML::Value::Node
   def first_child
   end

   # returns  current node's last child node, or nil
   # 
   # node type must be XML::Value::Node
   def last_child
   end

   # returns the node's local name.
   # 
   # node type must be XML::Value::Node
   def local_name
   end

   # returns the URI used for the node's namespace.
   # 
   # node type must be XML::Value::Node
   def namespace
   end

   # returns the sibling node immediately following this node in the
   # document, or nil.
   # 
   # node type must be XML::Value::Node
   def next_sibling
   end

   # return true if type is XML::Value::NONE
   def nil?
   end

   # return true if type is XML::Value::NODE
   def node?
   end

   # return the type of the value contained in this value
   # 
   # node type must be XML::Value::Node
   def node_name
   end

   # return the node's value
   # 
   # node type must be XML::Value::Node
   def node_value
   end

   # return true if type is one of the numeric types
   def number?
   end

   # If the current node is an attribute node, returns the document
   # element node that contains this attribute node. Otherwise, raise
   # an exception
   # 
   # node type must be XML::Value::Node
   def owner_element
   end

   # returns current node's parent, or nil
   # 
   # node type must be XML::Value::Node
   def parent_node
   end

   # returns the prefix set for the node's namespace.
   # 
   # node type must be XML::Value::Node
   def prefix
   end

   # returns  the sibling node immediately preceding this node
   # in the document, or nil
   # 
   # node type must be XML::Value::Node
   def previous_sibling
   end

   # return true if type is XML::Value::STRING
   def string?
   end

   # return true if the value has the specified type
   def type?(type)
   end

   # return the type associated with a value
   def type
   end

end
