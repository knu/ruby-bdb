=begin

== BDB::XML::Modify

The XML::Modify class encapsulates the context within which a set of one
or more documents specified by an XML::Query query can be modified in
place. The modification is performed using an XML::Modify object, and a
series of methods off that object that identify how the document is to
be modified. Using these methods, the modification steps are
identified. When the object is executed, these steps are performed in
the order that they were specified. 

The modification steps are executed against one or more documents
using XML::Modify::execute. This method can operate on a single document
stored in an XML::Value, or against multiple documents stored in an
XML::Results set that was created as the result of a container or
document query. 

=== Methods

--- append(query, type, name, content, location = -1)

    Appends the provided data to the selected node's child nodes.
    
    : ((|query|)) 
      provides the Query expression used to target the 
      location in the document where the modification is to be performed.
    
    : ((|type|))
      Identifies the type of information to be inserted.
      It can have the value XML::Modify::Element, XML::Modify::Attribute,
      XML::Modify::Text, XML::Modify::Comment, orXML::Modify::PI
    
    : ((|name|))
      the name of the node, attribute, or processing instruction
      to insert
    
    : ((|content|))
      The content to insert.
    
    : ((|location|)) 
      Identifies the position in the child node list
      where the provided content is to be inserted

--- insert_after(query, type, name, content)

    Inserts the provided data into the document after the selected node.
    
    : ((|query|))
      provides the Query expression used to target the 
      location in the document where the modification is to be performed.
    
    : ((|type|))
      Identifies the type of information to be inserted.
      It can have the value XML::Modify::Element, XML::Modify::Attribute,
      XML::Modify::Text, XML::Modify::Comment, orXML::Modify::PI
    
    : ((|name|))
      the name of the node, attribute, or processing instruction
      to insert
    
    : ((|content|))
      The content to insert.
    

--- insert_before(query, type, name, content)

    Inserts the provided data into the document before the selected node.
    
    : ((|query|))
      provides the Query expression used to target the 
      location in the document where the modification is to be performed.
    
    : ((|type|))
      Identifies the type of information to be inserted.
      It can have the value XML::Modify::Element, XML::Modify::Attribute,
      XML::Modify::Text, XML::Modify::Comment, orXML::Modify::PI
    
    : ((|name|))
      the name of the node, attribute, or processing instruction
      to insert
    
    : ((|content|))The content to insert.
    

--- remove(query)

    Removes the node targeted by the selection expression.
    
    : ((|query|))
      provides the Query expression used to target the 
      location in the document where the modification is to be performed.

--- rename(query, name)

    Renames an element node, attribute node, or processing instruction.
    
    : ((|query|))
      provides the Query expression used to target the 
      location in the document where the modification is to be performed.
    
    : ((|name|))
      The new name to give the targeted node.


--- update(query, content)

    Replaces the targeted node's content with text.
    
    : ((|query|)) 
      provides the Query expression used to target the 
      location in the document where the modification is to be performed.
    
    : ((|content|)) 
      The content to use as the targeted node's content.

--- execute(obj, context, update = nil)

    Executes one or more document modification operations (or steps) against
    an XML::Results or XML::Value object. Upon completing the modification,
    the modified document is automatically updated in the backing
    XML::Container for you.
    
    : ((|obj|))
      The object to modify using this collection of
      modification steps.
    
    : ((|context|))
      The XML::Context object to use for this modification.
    
    : ((|update|))
      The XML::Update object to use for this modification.

--- encoding = string

    Specify a new character encoding for the modified documents.
    


=end
