=begin
== BDB::XML::Document

A Document is the unit of storage within a Container. 

A Document contains a stream of bytes.

Document supports annotation attributes.

# module BDB
# module XML
# class Document
# class << self

=== Class Methods

--- allocate
    Allocate a new object

# end

=== Methods

--- self[attr]
    Return the value of an attribute

--- self[attr] = val
    Set the value of an attribute

--- content
    Return the content of the document

--- content=(val)
    Set the content of the document

--- get(uri = "", attr [, class])
    Return the value of an attribute. ((|uri|)) specify the namespace
    where reside the attribute.

    the optional ((|class|)) argument give the type (String, Numeric, ...)

--- id
    Return the ID

--- initialize(content = "")
    Initialize the document with the content specified

--- modify(mod)
    Modifies the document contents based on the information contained in the
    ((|BDB::XML::Modify|)) object

--- name
    Return the name of the document

--- name=(val)
    Set the name of the document

--- prefix
    Return the default prefix for the namespace

--- prefix=(val)
    Set the default prefix used by ((|set|))

--- query(xpath, context = nil)
    Execute the XPath expression ((|xpath|)) against the document
    Return an ((|BDB::XML::Results|))

--- set(uri = "", prefix = "", attr, value)
    Set an attribute in the namespace ((|uri|)). ((|prefix|)) is the prefix
    for the namespace

--- to_s
--- to_str
    Return the document as a String object

--- uri
    Return the default namespace

--- uri=(val)
    Set the default namespace used by ((|set|)) and ((|get|))

# end
# end
# end

=end
