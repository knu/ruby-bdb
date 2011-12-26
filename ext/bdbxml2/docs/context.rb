# The Context class encapsulates the context within which a query
# is performed against an Container. The context includes namespace 
# mappings, variable bindings, and flags that indicate how the query result 
# set should be determined and returned to the caller. Multiple queries can 
# be executed within the same Context;
# 
# Context allows you to define whether queries executed within the
# context are to be evaluated lazily or eagerly, and whether the query
# is to return live or dead values.
#
# The Query syntax permits expressions to refer to namespace prefixes,
# but not to define them. The Context class provides namespace
# management methods so that the caller may manage the namespace prefix to
# URI mapping. By default the prefix "dbxml" is defined to be
# "http://www.sleepycat.com/2002/dbxml". 
#
# The Query syntax also permits expressions to refer to externally
# defined variables. The XmlQueryContext class provides methods that
# allow the caller to manage the externally-declared variable to value
# bindings. 
class BDB::XML::Context
   # Get the value bound to a variable
   #
   def  [](variable)
   end
   
   # Bind a value to a variable
   #
   def  []=(variable, value)
   end
   
   # Delete all the namespace prefix mappings
   #
   def  clear_namespaces
   end
   
   # Delete the namespace URI for a particular prefix
   #
   def  del_namespace(name)
   end
   
   # Return the evaluation type
   #
   def  evaltype
   end
   
   # Set the evaluation type
   #
   def  evaltype=(type)
   end
   
   # Get the namespace URI that a namespace prefix maps onto
   #
   def  get_namespace(name)
   end
   
   # Return the return type
   #
   def  returntype
   end
   
   # Set the return type
   #
   def  returntype=(type)
   end
   
   #Define a namespace prefix, providing the URI that it maps onto
   #
   #If <em>uri</em> is <em>nil</em> delete the namespace
   def  set_namespace(name, uri)
   end

   # Returns the base URI used for relative paths in query expressions.
   def uri
   end 
   
   # Sets the base URI used for relative paths in query expressions.
   def uri=(val)
   end    

   # Discover the name of the default collection used by fn:collection()
   # with no arguments in an XQuery expression.
   def collection
   end

   # Set theURI specifying the name of the collection. 
   #
   # The default collection is that which is used by fn:collection()
   # without any arguments in an XQuery expression.
   def collection=(uri)
   end

   # Same than Context#[] but return the sequence of values bound to the variable.
   def get_results(name)
   end

end
