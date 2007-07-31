=begin
== BDB::XML::EventWriter

Class which enables applications to construct document content without using serialized XML.

An XML::EventWriter can be obtained with the method XML::Document#event_writer

=== Methods

--- attribute(local_name, prefix = nil, uri = nil, value, speficied)

    Write a single attribute 

--- close

    Close the object

--- dtd(text)

    Write the DTD

--- end_document

    Write an EndDocument event

--- end_element(local_name, prefix = nil, uri = nil)

    Write an EndElement event

--- end_entity(name)

    Write an EndEntity event

--- processing_instruction(target, data)

    Write a ProcessingInstruction event

--- start_document(version = nil, encoding = nil, standalone = nil)

    Write a StartDocument event

--- start_element(local_name, prefix = nil, uri = nil, nattr, empty)

    Write an element event

--- start_entity(name, expanded)

    Write StartEntityReference event

--- text(type, txt)

    Write an text event

    type must be one of XML::EventReader::Characters, XML::EventReader::Whitespace, 
    XML::EventReader::CDATA, or XML::EventReader::Comment

=end
