# Content : an empty root element
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?><root/>
#
# BDB::XML::Modify.new("/root",BDB::XML::Modify::Append,BDB::XML::Modify::Element,"new","new content")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><new>new content</new></root>
#
# BDB::XML::Modify.new("/root",BDB::XML::Modify::Append,BDB::XML::Modify::Attribute,"new","foo")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root new="foo"/>
#
# BDB::XML::Modify.new("/root",BDB::XML::Modify::Append,BDB::XML::Modify::PI,"newPI","PIcontent")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><?newPI PIcontent?></root>
#
# BDB::XML::Modify.new("/root",BDB::XML::Modify::Append,BDB::XML::Modify::Comment,"","comment content")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><!--comment content--></root>
#
# BDB::XML::Modify.new("/root",BDB::XML::Modify::Append,BDB::XML::Modify::Text,"","text content")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root>text content</root>
#
# Content : a little structure.
# 
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a att1="att1">a content</a>
<b att1="att1" att2="att2">b content 1</b>
<b att1="att1" att2="att2">b content 2</b>
<!--comment--></root>
#
# BDB::XML::Modify.new("/root/b/@att1",BDB::XML::Modify::Remove,BDB::XML::Modify::None)
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a att1="att1">a content</a>
<b att2="att2">b content 1</b><b att2="att2">b content 2</b>
<!--comment--></root>
#
# BDB::XML::Modify.new("/root/b[text()='b content 2']",BDB::XML::Modify::Remove,BDB::XML::Modify::None)
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a att1="att1">a content</a>
<b att1="att1" att2="att2">b content 1</b>
<!--comment--></root>
#
# BDB::XML::Modify.new("/root/comment()",BDB::XML::Modify::Remove,BDB::XML::Modify::None)
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a att1="att1">a content</a>
<b att1="att1" att2="att2">b content 1</b>
<b att1="att1" att2="att2">b content 2</b>
</root>
#
# BDB::XML::Modify.new("/root/a/text()",BDB::XML::Modify::Remove,BDB::XML::Modify::None)
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a att1="att1"/>
<b att1="att1" att2="att2">b content 1</b>
<b att1="att1" att2="att2">b content 2</b>
<!--comment--></root>
#
# BDB::XML::Modify.new("/root",BDB::XML::Modify::Append,BDB::XML::Modify::Element,"new")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a att1="att1">a content</a>
<b att1="att1" att2="att2">b content 1</b>
<b att1="att1" att2="att2">b content 2</b>
<!--comment--><new/></root>
#
# BDB::XML::Modify.new("/root",BDB::XML::Modify::Append,BDB::XML::Modify::Element,"new","",0)
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><new/><a att1="att1">a content</a>
<b att1="att1" att2="att2">b content 1</b>
<b att1="att1" att2="att2">b content 2</b>
<!--comment--></root>
#
# BDB::XML::Modify.new("/root",BDB::XML::Modify::Append,BDB::XML::Modify::Element,"new","",2)
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a att1="att1">a content</a>
<b att1="att1" att2="att2">b content 1</b>
<new/><b att1="att1" att2="att2">b content 2</b>
<!--comment--></root>
#
# BDB::XML::Modify.new("/root/a",BDB::XML::Modify::InsertBefore,BDB::XML::Modify::Element,"new")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><new/><a att1="att1">a content</a>
<b att1="att1" att2="att2">b content 1</b>
<b att1="att1" att2="att2">b content 2</b>
<!--comment--></root>
#
# BDB::XML::Modify.new("/root/a",BDB::XML::Modify::InsertAfter,BDB::XML::Modify::Element,"new")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a att1="att1">a content</a><new/>
<b att1="att1" att2="att2">b content 1</b>
<b att1="att1" att2="att2">b content 2</b>
<!--comment--></root>
#
# BDB::XML::Modify.new("/root/a",BDB::XML::Modify::Rename,BDB::XML::Modify::None,"x")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><x att1="att1">a content</x>
<b att1="att1" att2="att2">b content 1</b>
<b att1="att1" att2="att2">b content 2</b>
<!--comment--></root>
#
#  BDB::XML::Modify.new("/root/a/@att1",BDB::XML::Modify::Rename,BDB::XML::Modify::None,"att2")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a att2="att1">a content</a>
<b att1="att1" att2="att2">b content 1</b>
<b att1="att1" att2="att2">b content 2</b>
<!--comment--></root>
#
# Content test update
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a>a content 1<c/>a content 2</a>
<b>b content 1</b>
<!--comment--></root>
#
#  BDB::XML::Modify.new("/root/comment()",BDB::XML::Modify::Update,BDB::XML::Modify::None,"","new comment")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a>a content 1<c/>a content 2</a>
<b>b content 1</b>
<!--new comment--></root>
#
#  BDB::XML::Modify.new("/root/a",BDB::XML::Modify::Update,BDB::XML::Modify::None,"","new a text")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a><c/>new a text</a>
<b>b content 1</b>
<!--comment--></root>
#
#  BDB::XML::Modify.new("/root/a",BDB::XML::Modify::Update,BDB::XML::Modify::None)
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a><c/></a>
<b>b content 1</b>
<!--comment--></root>
#
#  BDB::XML::Modify.new("/root",BDB::XML::Modify::Update,BDB::XML::Modify::None,"","new root text")
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a>a content 1<c/>a content 2</a>
<b>b content 1</b>
<!--comment-->new root text</root>
#
#  BDB::XML::Modify.new("/root/b",BDB::XML::Modify::Update,BDB::XML::Modify::None,"","new b text")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a>a content 1<c/>a content 2</a>
<b>new b text</b>
<!--comment--></root>
#
