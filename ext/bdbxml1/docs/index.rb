# Define the indexing strategy for a Container
# 
# Indexing is specified by providing the name of a node and a list of indexing
# strategies for that node.
class BDB::XML::Index
   #
   #add a new index
   #
   #* <em>uri</em>
   #  The namespace for the element (optional)
   #
   #* <em>name</em>
   #  The name parameter provides the fully qualified element type
   #  to be indexed.
   #
   #* <em>index</em>
   #  The index string is a comma separated list of the following indexing
   #  strategy names
   #
   #  * none-none-none-none 
   #  * node-element-presence 
   #  * node-attribute-presence 
   #  * node-element-equality-string 
   #  * node-element-equality-number 
   #  * node-element-substring-string 
   #  * node-attribute-equality-string 
   #  * node-attribute-equality-number 
   #  * node-attribute-substring-string 
   #  * edge-element-presence 
   #  * edge-attribute-presence 
   #  * edge-element-equality-string 
   #  * edge-element-equality-number 
   #  * edge-element-substring-string 
   #  * edge-attribute-equality-string 
   #  * edge-attribute-equality-number 
   #  * edge-attribute-substring-string 
   #
   def  add(uri = "", name, index)
   end
   
   #Delete the specified index
   #
   def  delete(uri = "", name, index)
   end
   
   #Iterate over all indexes
   #
   def  each 
      yield uri, name, index
   end
   
   #Initialize the index with the optional values [uri, name, value]
   #
   def  initialize([[uri0, name0, index0] [, [uri1, name1, index1], ... ]])
   end
   
   #Find the indexing startegy associated with <em>uri</em>, and <em>name</em>
   #
   #Return <em>nil</em> is no indexing strategy was defined
   #
   def  find(uri = "", name)
   end
   
   #Replace the specified index
   #
   def  replace(uri = "", name, index)
   end
   
   #Return an Array of all indexes
   #
   def  to_a
   end
end
