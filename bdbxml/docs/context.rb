# The context within which a query is performed against a Container. 
# 
# The context within which a query is performed against a Container
# is complex enough to warrent an encapsulating class. This context
# includes a namespace mapping, variable bindings, and flags that 
# determine how the query result set should be determined and returned
# to the caller.
# 
# The XPath syntax permits expressions to refer to namespace prefixes, but
# not to define them. The Context class provides a number of namespace 
# management methods so that the caller may manage the namespace prefix
# to URI mapping.
# 
# The XPath syntax also permits expressions to refer to variables, but not
# to define them. The Context class provides a number of methods so
# that the caller may manage the variable to value bindings.
class BDB::XML::Context
   #Get the value bound to a variable
   #
   def  [](variable)
   end
   
   #Bind a value to a variable
   #
   def  []=(variable, value)
   end
   
   #Delete all the namespace prefix mappings
   #
   def  clear_namespaces
   end
   #same than <em> clear_namespaces</em>
   def  clear
   end
   
   #Delete the namespace URI for a particular prefix
   #
   def  del_namespace(name)
   end
   
   #Return the evaluation type
   #
   def  evaltype
   end
   
   #Set the evaluation type
   #
   def  evaltype=(type)
   end
   
   #Get the namespace URI that a namespace prefix maps onto
   #
   def  get_namespace(name)
   end
   #same than <em> get_namespace</em>
   def  namespace[name]
   end
   
   #Initialize the object with the optional evaluation type
   #<em>BDB::XML::Context::Lazy</em> or <em>BDB::XML::Context::Eager</em>
   #and return type <em>BDB::XML::Context::Documents</em>,
   #<em>BDB::XML::Context::Values</em> or 
   #<em>BDB::XML::Context::Candidates</em>
   #
   def  initialize(returntype = nil, evaluation = nil)
   end
   
   #return <em>true</em> if the metadata is added to the document
   #
   def  metadata
   end
   #same than <em> metadata</em>
   def  with_metadata
   end
   
   #The <em>with</em> parameter specifies whether or not to add the document
   #metadata prior to the query.
   #
   def  metadata=(with)
   end
   #same than <em> metadata=</em>
   def  with_metadata=(with)
   end
   
   #Return the return type
   #
   def  returntype
   end
   
   #Set the return type
   #
   def  returntype=(type)
   end
   
   #Define a namespace prefix, providing the URI that it maps onto
   #
   #If <em>uri</em> is <em>nil</em> delete the namespace
   def  namespace[name]=(uri)
   end
   #same than <em> namespace[name]=</em>
   def  set_namespace(name, uri)
   end
end
