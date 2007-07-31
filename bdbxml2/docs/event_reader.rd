=begin
== BDB::XML::EventReader

Class which enables applications to read document content via a pull interface without 
materializing XML as text.

An XML::EventReader can be obtained with the method XML::Document#event_reader
or XML::Value#event_reader

=== Methods

--- attribute_count

    If the current event is StartElement, return the number of attributes available.

--- attribute_local_name(index)

    If the current event is StartElement, return the local name for the attribute at the 
    specified offset

--- attribute_namespace_uri(index)

    If the current event is StartElement, return the namespace URI for the attribute at the
    specified offset.

--- attribute_prefix(index)

    If the current event is StartElement, return the namespace prefix for the attribute at the
    specified offset.

--- attribute_specified?(index)

    If the current event is StartElement, return whether the attribute at the index indicated
    is specified.

--- attribute_value(index)

    If the current event is StartElement, return the value for the attribute at the specified
    offset.

--- close

    close the object

--- document_standalone?

    Return whether the document is standalone

--- empty_element?

    Return true if the current event is StartElement and the element has not content. 
    If the current even is not StartElement, an exception will be raised.

--- empty_element_info?

    Returns true if the XML::EventReader object will return whether an element is empty 
    or not in the context of the StartElement event. 

--- encoding

    Returns the encoding for the document, if available.

--- encoding?

    Checks if the encoding was explicitly set.

--- entity_escape?

    If the current event is Characters, and XML::EventReader#entity_escape_info? is true, 
    returns whether the current text string requires escaping of entities for XML serialization.

--- entity_escape_info?

    Returns true if the XML::EventReader object is able to return information about text strings
    indicating that they may have entities requiring escaping for XML serialization.

--- entity_info?

    Gets whether the XML::EventReader will include events of type StartEntityReference
    and EndEntityReference.
    
--- entity_info=(boolean)

    The events of type StartEntityReference and EndEntityReference are used to report the start
    and end of XML that was originally an entity reference in the XML text, but has since been 
    expanded. By default, these events are not reported.

--- event_type

    Return the event type of the current event

--- expand_entities?

    Gets whether the XML::EventReader will include events associated with expanded entities.

--- expand_entities=(boolean)

    Sets whether the XML::EventReader should include events associated with expanded entities.

--- local_name

    If the current event is StartElement or EndElement, return the local name for the element   

--- namespace_uri

    If the current event is StartElement or EndElement, return the namespace URI for the element

--- next?

    Check if there are additional events available

--- next

    Move to the next event

--- next_tag

    Move to the next StartElement or EndElement 
    
--- prefix

    If the current event is StartElement or EndElement, return the namespace prefix for the element

--- standalone?

    Checks if the standalone attribute was explicitly set.

--- system_id

    Returns the document's system ID, if available.    

--- value

    If the current event is Characters, return the text value. If the current event is 
    ProcessingInstruction, return the data portion of the processing instruction.

--- version

    Returns the XML version string, if available.

--- whitespace?
 
    Return true if the current text value is entirely white space. The current event must be 
    one of Whitespace, Characters, or CDATA, or an exception will be raised..

=end
