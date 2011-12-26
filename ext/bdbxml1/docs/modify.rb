# Modify encapsulates the context within which a set of documents specified by
# a query can be modified in place. 
# 
# The modification is performed using the methods 
# <em>BDB::XML::Container#modify</em> and <em>BDB::XML::Document#modify</em>
# 
# There are two parts to the object -- the query and the operation. 
# The query identifies target nodes against which the operation is run. 
# The operation specifies what modifications to perform.
class BDB::XML::Modify
   #return the number of node hits in the specified query.
   #
   def  count
   end
   
   #
   #<em>xpath</em> is a <em>String</em>, or an <em>BDB::XML::Context</em> object
   #
   #<em>operation</em> can have the value <em>BDB::XML::Modify::InsertAfter</em>, 
   #<em>BDB::XML::Modify::InsertBefore</em>, <em>BDB::XML::Modify::Append</em>,
   #<em>BDB::XML::Modify::Update</em>, <em>BDB::XML::Modify::Remove</em> or 
   #<em>BDB::XML::Modify::Rename</em>.
   #
   #<em>type</em> can have the value <em>BDB::XML::Modify::Element</em>, 
   #<em>BDB::XML::Modify::Attribute</em>, <em>BDB::XML::Modify::Text</em>,
   #<em>BDB::XML::Modify::ProcessingInstruction</em>, <em>BDB::XML::Modify::Comment</em> or
   #<em>BDB::XML::Modify::None</em>.
   #
   #<em>name</em> is the name for the new content.
   # 
   #<em>content</em> the content for operations that require content.
   #
   #<em>location</em> indicate where a new child node will be placed for the
   #append operation
   #
   #<em>context</em> is valid only when a String is given as the first argument.
   #This must be a <em>BDB::XML::Context</em> which  contains the variable 
   #bindings, the namespace prefix to URI mapping, and the query processing
   #flags.
   #
   def  initialize(xpath, operation, type = BDB::XML::Modify::None, name = "", content = "", location = -1, context = nil)
   end
   
   #Specify a new character encoding for the modified documents.
   #
   def  encoding=(string)
   end
end
