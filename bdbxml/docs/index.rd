=begin
== BDB::XML::Index

The index specification

# module BDB
# module XML
# class Index
# class << self
=== Class Methods

--- allocate
    Allocate a new Index object
# end

=== Methods

--- add(uri = "", name, index)

    add a new index

    : ((|uri|))
      The namespace for the element (optional)

    : ((|name|))
      The name parameter provides the fully qualified element type
      to be indexed.

    : ((|index|))
      The index string is a comma separated list of the following indexing
      strategy names

         : none-none-none-none 
         : node-element-presence 
         : node-attribute-presence 
         : node-element-equality-string 
         : node-element-equality-number 
         : node-element-substring-string 
         : node-attribute-equality-string 
         : node-attribute-equality-number 
         : node-attribute-substring-string 
         : edge-element-presence 
         : edge-attribute-presence 
         : edge-element-equality-string 
         : edge-element-equality-number 
         : edge-element-substring-string 
         : edge-attribute-equality-string 
         : edge-attribute-equality-number 
         : edge-attribute-substring-string 

--- delete(uri = "", name, index)
    Delete the specified index

--- each {|uri, name, index| ... }
    Iterate over all indexes

--- initialize([[uri0, name0, index0] [, [uri1, name1, index1], ... ]])
    Initialize the index with the optional values [uri, name, value]

--- find(uri = "", name)
    Find the indexing startegy associated with ((|uri|)), and ((|name|))

    Return ((|nil|)) is no indexing strategy was defined

--- replace(uri = "", name, index)
    Replace the specified index

--- to_a
    Return an Array of all indexes

# end
# end
# end

=end
