# Content : an empty root element
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?><root/>
#
# XML::Modify.new("/root",XML::Modify::Append,XML::Modify::Element,"new","new content")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><new>new content</new></root>
#
# XML::Modify.new("/root",XML::Modify::Append,XML::Modify::Attribute,"new","foo")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root new="foo"/>
#
# XML::Modify.new("/root",XML::Modify::Append,XML::Modify::PI,"newPI","PIcontent")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><?newPI PIcontent?></root>
#
# XML::Modify.new("/root",XML::Modify::Append,XML::Modify::Comment,"","comment content")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><!--comment content--></root>
#
# XML::Modify.new("/root",XML::Modify::Append,XML::Modify::Text,"","text content")
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
# XML::Modify.new("/root/b/@att1",XML::Modify::Remove,XML::Modify::None)
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a att1="att1">a content</a>
<b att2="att2">b content 1</b><b att2="att2">b content 2</b>
<!--comment--></root>
#
# XML::Modify.new("/root/b[text()='b content 2']",XML::Modify::Remove,XML::Modify::None)
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a att1="att1">a content</a>
<b att1="att1" att2="att2">b content 1</b>
<!--comment--></root>
#
# XML::Modify.new("/root/comment()",XML::Modify::Remove,XML::Modify::None)
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a att1="att1">a content</a>
<b att1="att1" att2="att2">b content 1</b>
<b att1="att1" att2="att2">b content 2</b>
</root>
#
# XML::Modify.new("/root/a/text()",XML::Modify::Remove,XML::Modify::None)
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a att1="att1"/>
<b att1="att1" att2="att2">b content 1</b>
<b att1="att1" att2="att2">b content 2</b>
<!--comment--></root>
#
# XML::Modify.new("/root",XML::Modify::Append,XML::Modify::Element,"new")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a att1="att1">a content</a>
<b att1="att1" att2="att2">b content 1</b>
<b att1="att1" att2="att2">b content 2</b>
<!--comment--><new/></root>
#
# XML::Modify.new("/root",XML::Modify::Append,XML::Modify::Element,"new","",0)
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><new/><a att1="att1">a content</a>
<b att1="att1" att2="att2">b content 1</b>
<b att1="att1" att2="att2">b content 2</b>
<!--comment--></root>
#
# XML::Modify.new("/root",XML::Modify::Append,XML::Modify::Element,"new","",2)
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a att1="att1">a content</a>
<b att1="att1" att2="att2">b content 1</b>
<new/><b att1="att1" att2="att2">b content 2</b>
<!--comment--></root>
#
# XML::Modify.new("/root/a",XML::Modify::InsertBefore,XML::Modify::Element,"new")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><new/><a att1="att1">a content</a>
<b att1="att1" att2="att2">b content 1</b>
<b att1="att1" att2="att2">b content 2</b>
<!--comment--></root>
#
# XML::Modify.new("/root/a",XML::Modify::InsertAfter,XML::Modify::Element,"new")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a att1="att1">a content</a><new/>
<b att1="att1" att2="att2">b content 1</b>
<b att1="att1" att2="att2">b content 2</b>
<!--comment--></root>
#
# XML::Modify.new("/root/a",XML::Modify::Rename,XML::Modify::None,"x")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><x att1="att1">a content</x>
<b att1="att1" att2="att2">b content 1</b>
<b att1="att1" att2="att2">b content 2</b>
<!--comment--></root>
#
#  XML::Modify.new("/root/a/@att1",XML::Modify::Rename,XML::Modify::None,"att2")
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
#  XML::Modify.new("/root/comment()",XML::Modify::Update,XML::Modify::None,"","new comment")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a>a content 1<c/>a content 2</a>
<b>b content 1</b>
<!--new comment--></root>
#
#  XML::Modify.new("/root/a",XML::Modify::Update,XML::Modify::None,"","new a text")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a><c/>new a text</a>
<b>b content 1</b>
<!--comment--></root>
#
#  XML::Modify.new("/root/a",XML::Modify::Update,XML::Modify::None)
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a><c/></a>
<b>b content 1</b>
<!--comment--></root>
#
#  XML::Modify.new("/root",XML::Modify::Update,XML::Modify::None,"","new root text")
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a>a content 1<c/>a content 2</a>
<b>b content 1</b>
<!--comment-->new root text</root>
#
#  XML::Modify.new("/root/b",XML::Modify::Update,XML::Modify::None,"","new b text")
#
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<root><a>a content 1<c/>a content 2</a>
<b>new b text</b>
<!--comment--></root>
#
