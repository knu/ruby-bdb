# The XML::Modify class encapsulates the context within which a set of one
# or more documents specified by an XML::Query query can be modified in
# place. The modification is performed using an XML::Modify object, and a
# series of methods off that object that identify how the document is to
# be modified. Using these methods, the modification steps are
# identified. When the object is executed, these steps are performed in
# the order that they were specified. 
# 
# The modification steps are executed against one or more documents
# using XML::Modify::execute. This method can operate on a single document
# stored in an XML::Value, or against multiple documents stored in an
# XML::Results set that was created as the result of a container or
# document query. 
class BDB::XML::Modify
   # Appends the provided data to the selected node's child nodes.
   #
   # * <em>query</em> provides the Query expression used to target the 
   #   location in the document where the modification is to be performed.
   #
   # * <em>type</em>Identifies the type of information to be inserted.
   #   It can have the value XML::Modify::Element, XML::Modify::Attribute,
   #   XML::Modify::Text, XML::Modify::Comment, orXML::Modify::PI
   #
   # * <em>name</em>the name of the node, attribute, or processing instruction
   #   to insert
   #
   # * <em>content</em>The content to insert.
   #
   # * <em>location</em> Identifies the position in the child node list
   #   where the provided content is to be inserted
   def append(query, type, name, content, location = -1)
   end

   # Inserts the provided data into the document after the selected node.
   #
   # * <em>query</em> provides the Query expression used to target the 
   #   location in the document where the modification is to be performed.
   #
   # * <em>type</em>Identifies the type of information to be inserted.
   #   It can have the value XML::Modify::Element, XML::Modify::Attribute,
   #   XML::Modify::Text, XML::Modify::Comment, orXML::Modify::PI
   #
   # * <em>name</em>the name of the node, attribute, or processing instruction
   #   to insert
   #
   # * <em>content</em>The content to insert.
   #
   def insert_after(query, type, name, content)
   end

   # Inserts the provided data into the document before the selected node.
   #
   # * <em>query</em> provides the Query expression used to target the 
   #   location in the document where the modification is to be performed.
   #
   # * <em>type</em>Identifies the type of information to be inserted.
   #   It can have the value XML::Modify::Element, XML::Modify::Attribute,
   #   XML::Modify::Text, XML::Modify::Comment, orXML::Modify::PI
   #
   # * <em>name</em>the name of the node, attribute, or processing instruction
   #   to insert
   #
   # * <em>content</em>The content to insert.
   #
   def insert_before(query, type, name, content)
   end

   # Removes the node targeted by the selection expression.
   #
   # * <em>query</em> provides the Query expression used to target the 
   #   location in the document where the modification is to be performed.
   def remove(query)
   end

   # Renames an element node, attribute node, or processing instruction.
   #
   # * <em>query</em> provides the Query expression used to target the 
   #   location in the document where the modification is to be performed.
   #
   # * <em>name</em> The new name to give the targeted node.
   def rename(query, name)
   end


   # Replaces the targeted node's content with text.
   #
   # * <em>query</em> provides the Query expression used to target the 
   #   location in the document where the modification is to be performed.
   #
   # * <em>content</em> The content to use as the targeted node's content.
   def update(query, content)
   end

   # Executes one or more document modification operations (or steps) against
   # an XML::Results or XML::Value object. Upon completing the modification,
   # the modified document is automatically updated in the backing
   # XML::Container for you.
   #
   # * <em>obj</em>The object to modify using this collection of
   #   modification steps.
   #
   # * <em>context</em>The XML::Context object to use for this modification.
   #
   # * <em>update</em>The XML::Update object to use for this modification.
   def execute(obj, context, update = nil)
   end

   # Specify a new character encoding for the modified documents.
   #
   def  encoding=(string)
   end
end
