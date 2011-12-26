#
# Class which enables applications to construct document content without using serialized XML.
# 
# An XML::EventWriter can be obtained with the method XML::Document#event_writer
#
class BDB::XML::EventWriter

   # Write a single attribute 
   def attribute(local_name, prefix = nil, uri = nil, value, speficied)
   end

   # Close the object
   def close
   end

   # Write the DTD
   def dtd(text)
   end

   # Write an EndDocument event
   def end_document
   end

   # Write an EndElement event
   def end_element(local_name, prefix = nil, uri = nil)
   end

   # Write an EndEntity event
   def end_entity(name)
   end

   # Write a ProcessingInstruction event
   def processing_instruction(target, data)
   end

   # Write a StartDocument event
   def start_document(version = nil, encoding = nil, standalone = nil)
   end

   # Write an element event
   def start_element(local_name, prefix = nil, uri = nil, nattr, empty)
   end

   # Write StartEntityReference event
   def start_entity(name, expanded)
   end

   # Write an text event
   #
   # type must be one of XML::EventReader::Characters, XML::EventReader::Whitespace, 
   # XML::EventReader::CDATA, or XML::EventReader::Comment
   def text(type, txt)
   end

end
