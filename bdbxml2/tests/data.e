# Content
<?xml version="1.0" encoding="utf-8" standalone="no" ?><root/>
#
BDB::XML::Modify.new("/root",BDB::XML::Modify::InsertBefore,BDB::XML::Modify::Element,"new","")
BDB::XML::Modify.new("/root",BDB::XML::Modify::InsertAfter,BDB::XML::Modify::Element,"new","")
BDB::XML::Modify.new("/root",BDB::XML::Modify::Remove,BDB::XML::Modify::None,"","")
BDB::XML::Modify.new("/root",BDB::XML::Modify::Append,BDB::XML::Modify::Attribute,"","val")
BDB::XML::Modify.new("/root",BDB::XML::Modify::Append,BDB::XML::Modify::Attribute,"name","")
BDB::XML::Modify.new("/root",BDB::XML::Modify::Append,BDB::XML::Modify::Element,"","val")
# Content
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a att1="att1">a content</a><b att1="att1" att2="att2">b content 1</b>
<b att1="att1" att2="att2">b content 2</b><!--comment--></root>
#
BDB::XML::Modify.new("/root/a/@att1",BDB::XML::Modify::Append,BDB::XML::Modify::Attribute,"name","val")
BDB::XML::Modify.new("/root/comment()",BDB::XML::Modify::Append,BDB::XML::Modify::Attribute,"name","val")
BDB::XML::Modify.new("/root/a/@att1",BDB::XML::Modify::Append,BDB::XML::Modify::Element,"name","val")
BDB::XML::Modify.new("/root/comment()",BDB::XML::Modify::Append,BDB::XML::Modify::Element,"name","val")
