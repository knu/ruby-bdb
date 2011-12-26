# Class which enables applications to read document content via a pull interface without 
# materializing XML as text.
# 
# An XML::EventReader can be obtained with the method XML::Document#event_reader
# or XML::Value#event_reader
class  BDB::XML::EventReader


   # If the current event is StartElement, return the number of attributes available.
   def attribute_count
   end

   # If the current event is StartElement, return the local name for the attribute at the 
   # specified offset
   def attribute_local_name(index)
   end

   # If the current event is StartElement, return the namespace URI for the attribute at the
   # specified offset.
   def attribute_namespace_uri(index)
   end

   # If the current event is StartElement, return the namespace prefix for the attribute at the
   # specified offset.
   def attribute_prefix(index)
   end

   # If the current event is StartElement, return whether the attribute at the index indicated
   # is specified.
   def attribute_specified?(index)
   end

   # If the current event is StartElement, return the value for the attribute at the specified
   # offset.
   def attribute_value(index)
   end

   # close the object
   def close
   end

   # Return whether the document is standalone
   def document_standalone?
   end

   # Return true if the current event is StartElement and the element has not content. 
   # If the current even is not StartElement, an exception will be raised.
   def empty_element?
   end

   # Returns true if the XML::EventReader object will return whether an element is empty 
   # or not in the context of the StartElement event. 
   def empty_element_info?
   end

   # Returns the encoding for the document, if available.
   def encoding
   end

   # Checks if the encoding was explicitly set.
   def encoding?
   end

   # If the current event is Characters, and XML::EventReader#entity_escape_info? is true, 
   # returns whether the current text string requires escaping of entities for XML serialization.
   def entity_escape?
   end

   # Returns true if the XML::EventReader object is able to return information about text strings
   # indicating that they may have entities requiring escaping for XML serialization.
   def entity_escape_info?
   end

   # Gets whether the XML::EventReader will include events of type StartEntityReference
   # and EndEntityReference.
   def entity_info?
   end

   # The events of type StartEntityReference and EndEntityReference are used to report the start
   # and end of XML that was originally an entity reference in the XML text, but has since been 
   # expanded. By default, these events are not reported.
   def entity_info=(boolean)
   end

   # Return the event type of the current event
   def event_type
   end

   # Gets whether the XML::EventReader will include events associated with expanded entities.
   def expand_entities?
   end

   # Sets whether the XML::EventReader should include events associated with expanded entities.
   def expand_entities=(boolean)
   end

   # If the current event is StartElement or EndElement, return the local name for the element   
   def local_name
   end

   # If the current event is StartElement or EndElement, return the namespace URI for the element
   def namespace_uri
   end

   # Check if there are additional events available
   def next?
   end

   # Move to the next event
   def next
   end

   # Move to the next StartElement or EndElement 
   def next_tag
   end

   # If the current event is StartElement or EndElement, return the namespace prefix for the element
   def prefix
   end

   # Checks if the standalone attribute was explicitly set.
   def standalone?
   end

   # Returns the document's system ID, if available.    
   def system_id
   end

   # If the current event is Characters, return the text value. If the current event is 
   # ProcessingInstruction, return the data portion of the processing instruction.
   def value
   end

   # Returns the XML version string, if available.
   def version
   end
 
   # Return true if the current text value is entirely white space. The current event must be 
   # one of Whitespace, Characters, or CDATA, or an exception will be raised..
   def whitespace?
   end

end
