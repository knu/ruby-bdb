# The XML::Index class encapsulates the indexing specification of a container.
# An indexing specification can be retrieved with the XML::Container#index
# method, and modified using the XML::Container#index= method.
#
# Note that adding an index to a container results in re-indexing all of
# the documents in that container, which can take a very long time. It is 
# good practice to design an application to add useful indices before 
# populating a container.
class BDB::XML::Index
   # call-seq:
   #    add(uri, name, index)
   #    add(uri, name, type, syntax)
   #
   # Add a new index
   #
   # * <em>uri</em>
   #   The namespace for the element
   #
   # * <em>name</em>
   #   The name parameter provides the fully qualified element type
   #   to be indexed.
   #
   # * <em>index</em>
   #   The index string is a comma separated list of strings that represent 
   #   the indexing strategy
   #
   # * <em>type</em> series of  values bitwise OR'd together to form the 
   #   index strategy.
   #
   # * <em>syntax</em> Identifies the type of information being indexed
   def  add(uri, name, type, syntax)
   end

   # Adds an indexing strategy to the default index specification.
   def add_default(index)
   end

   # return the default index specification
   def default
   end
   
   # call-seq:
   #    delete(uri, name, index)
   #    delete(uri, name, type, syntax)
   #
   # Delete the specified index
   #
   # * <em>uri</em>
   #   The namespace for the element
   #
   # * <em>name</em>
   #   The name parameter provides the fully qualified element type
   #   to be indexed.
   #
   # * <em>index</em>
   #   The index string is a comma separated list of strings that represent 
   #   the indexing strategy
   #
   # * <em>type</em> series of  values bitwise OR'd together to form the 
   #   index strategy.
   #
   # * <em>syntax</em> Identifies the type of information being indexed
   def  delete(uri, name, type, syntax)
   end

   # call-seq:
   #    delete_default(index)
   #    delete_default(type, syntax)
   #
   # Delete the identified index from the default index specification.
   #
   # * <em>index</em>
   #   The index string is a comma separated list of strings that represent 
   #   the indexing strategy
   #
   # * <em>type</em> series of  values bitwise OR'd together to form the 
   #   index strategy.
   #
   # * <em>syntax</em> Identifies the type of information being indexed
   def delete_default(type, syntax)
   end

   #Iterate over all indexes
   #
   def  each 
      yield uri, name, index
   end
   
   #Find the indexing startegy associated with <em>uri</em>, and <em>name</em>
   #
   #Return <em>nil</em> is no indexing strategy was defined
   #
   def  find(uri = "", name)
   end

   # Return the manager associated with the Index
   def manager
   end
   
   # call-seq:
   #    replace(uri, name, index)
   #    replace(uri, name, type, syntax)
   #
   # Replaces the indexing strategies for a named document or metadata node.
   # All existing indexing strategies for that node are deleted, and the 
   # indexing strategy identified by this method is set for the node.
   #
   # * <em>uri</em>
   #   The namespace for the element
   #
   # * <em>name</em>
   #   The name parameter provides the fully qualified element type
   #   to be indexed.
   #
   # * <em>index</em>
   #   The index string is a comma separated list of strings that represent 
   #   the indexing strategy
   #
   # * <em>type</em> series of  values bitwise OR'd together to form the 
   #   index strategy.
   #
   # * <em>syntax</em> Identifies the type of information being indexed
   def  replace(uri, name, type, syntax)
   end

   # call-seq:
   #    replace_default(index)
   #    replace_default(type, syntax)
   #
   # Replace the default indexing strategy for the container. 
   #
   # * <em>index</em>
   #   The index string is a comma separated list of strings that represent 
   #   the indexing strategy
   #
   # * <em>type</em> series of  values bitwise OR'd together to form the 
   #   index strategy.
   #
   # * <em>syntax</em> Identifies the type of information being indexed
   def replace_default(type, syntax)
   end
   
   #Return an Array of all indexes
   #
   def  to_a
   end
end
