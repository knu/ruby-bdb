# A Document is the unit of storage within a Container. 
# 
# A Document contains a stream of bytes that may be of type XML.
# The Container only indexes the content of Documents
# that contain XML.
# 
# Document supports annotation attributes.
# 
class BDB::XML::Document
   
   #Return the value of an attribute
   #
   def  [](attr)
   end
   
   #Set the value of an attribute
   #
   def  []=(attr, val)
   end
   
   #Return the content of the document
   #
   def  content
   end
   
   #Set the content of the document
   #
   def  content=(val)
   end
   
   #Return the value of an attribute. <em>uri</em> specify the namespace
   #where reside the attribute.
   #
   #the optional <em>class</em> argument give the type (String, Numeric, ...)
   #
   def  get(uri = "", attr [, class])
	 end
   
   #Return the ID
   #
   def  id
   end
   
   #Initialize the document with the content specified
   #
   def  initialize(content = "")
   end
   
   #Modifies the document contents based on the information contained in the
   #<em>BDB::XML::Modify</em> object
   #
   def  modify(mod)
   end
   
   #Return the name of the document
   #
   def  name
   end
   
   #Set the name of the document
   #
   def  name=(val)
   end
   
   #Return the default prefix for the namespace
   #
   def  prefix
   end
   
   #Set the default prefix used by <em>set</em>
   #
   def  prefix=(val)
   end
   
   #Execute the XPath expression <em>xpath</em> against the document
   #Return an <em>BDB::XML::Results</em>
   #
   def  query(xpath, context = nil)
   end
   
   #Set an attribute in the namespace <em>uri</em>. <em>prefix</em> is the prefix
   #for the namespace
   #
   def  set(uri = "", prefix = "", attr, value)
   end
   
   #Return the document as a String object
   #
   def  to_s
   end
   #same than <em> to_s</em>
   def  to_str
   end
   
   #Return the default namespace
   #
   def  uri
   end
   
   #Set the default namespace used by <em>set</em> and <em>get</em>
   #
   def  uri=(val)
   end
end
