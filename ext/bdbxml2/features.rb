class Object
   remove_const('CONFTEST_C')
end

CONFTEST_C = 'conftest.cxx'

unless "a".respond_to?(:tr_cpp)
   class String
      def tr_cpp
         strip.upcase.tr_s("^A-Z0-9_", "_")
      end
   end
end

def try_dbxml_compile(mes, func, src)
   new_src = "
#include <dbxml/DbXml.hpp>

using namespace DbXml;

int main()
{
    #{src}
    return 1;
}
"
   print "checking for #{mes}... "
   if try_compile(new_src)
      $defs << "-DHAVE_DBXML_#{func}"
      puts "yes"
      true
   else
      $defs << "-DNOT_HAVE_DBXML_#{func}"
      puts "no"
      false
   end
end

def have_dbxml_const(const)
   const.each do |c|
      try_dbxml_compile(c, "CONST_#{c.tr_cpp}", <<EOT)
   int x = #{c};
EOT
   end
end

print "checking rb_each() prototype ..."
if ! try_compile(<<EOT)
#include <ruby.h>

int main()
{
   rb_iterate(RUBY_METHOD_FUNC(rb_each), (VALUE)0, RUBY_METHOD_FUNC(0), (VALUE )0);

}
EOT
   $defs << '-DHAVE_DBXML_RB_NEW_PROTO'
else
   $defs << '-DNOT_HAVE_DBXML_RB_NEW_PROTO'
end  
puts 

have_dbxml_const([
  'CATEGORY_ALL', 'CATEGORY_CONTAINER', 'CATEGORY_DICTIONARY', 'CATEGORY_INDEXER',
  'CATEGORY_MANAGER', 'CATEGORY_NODESTORE', 'CATEGORY_NONE', 'CATEGORY_OPTIMIZER',
  'CATEGORY_QUERY', 
  'DBXML_ADOPT_DBENV', 'DBXML_ALLOW_AUTO_OPEN', 'DBXML_ALLOW_EXTERNAL_ACCESS',
  'DBXML_ALLOW_VALIDATION', 'DBXML_CACHE_DOCUMENTS', 'DBXML_CHKSUM_SHA1',
  'DBXML_DOCUMENT_PROJECTION', 'DBXML_ENCRYPT', 'DBXML_GEN_NAME',
  'DBXML_INDEX_NODES', 'DBXML_INDEX_VALUES', 'DBXML_LAZY_DOCS', 
  'DBXML_NO_AUTO_COMMIT', 'DBXML_NO_INDEX_NODES', 'DBXML_NO_STATISTICS', 
  'DBXML_REVERSE_ORDER', 'DBXML_STATISTICS', 'DBXML_TRANSACTIONAL', 
  'DBXML_WELL_FORMED_ONLY', 'LEVEL_ALL', 'LEVEL_DEBUG', 'LEVEL_ERROR', 
  'LEVEL_INFO', 'LEVEL_NONE', 'LEVEL_WARNING'])

have_dbxml_const([
                  'XmlContainer::NodeContainer',
                  'XmlContainer::WholedocContainer',
                  'XmlEventReader::CDATA',
                  'XmlEventReader::Characters',
                  'XmlEventReader::Comment',
                  'XmlEventReader::DTD',
                  'XmlEventReader::EndDocument',
                  'XmlEventReader::EndElement',
                  'XmlEventReader::EndEntityReference',
                  'XmlEventReader::ProcessingInstruction',
                  'XmlEventReader::StartDocument',
                  'XmlEventReader::StartElement',
                  'XmlEventReader::StartEntityReference',
                  'XmlEventReader::Whitespace',
                  'XmlIndexLookup::EQ',
                  'XmlIndexLookup::GT',
                  'XmlIndexLookup::GTE',
                  'XmlIndexLookup::LT',
                  'XmlIndexLookup::LTE',
                  'XmlIndexLookup::NONE'])

have_dbxml_const([
                  'XmlIndexSpecification::KEY_EQUALITY',
                  'XmlIndexSpecification::KEY_NONE',
                  'XmlIndexSpecification::KEY_PRESENCE',
                  'XmlIndexSpecification::KEY_SUBSTRING',
                  'XmlIndexSpecification::NODE_ATTRIBUTE',
                  'XmlIndexSpecification::NODE_ELEMENT',
                  'XmlIndexSpecification::NODE_METADATA',
                  'XmlIndexSpecification::NODE_NONE',
                  'XmlIndexSpecification::PATH_EDGE',
                  'XmlIndexSpecification::PATH_NODE',
                  'XmlIndexSpecification::PATH_NONE',
                  'XmlIndexSpecification::UNIQUE_OFF',
                  'XmlIndexSpecification::UNIQUE_ON',
                  'XmlModify::Attribute',
                  'XmlModify::Comment',
                  'XmlModify::Element',
                  'XmlModify::ProcessingInstruction',
                  'XmlModify::ProcessingInstruction',
                  'XmlModify::Text',
                  'XmlQueryContext::DeadValues',
                  'XmlQueryContext::Eager',
                  'XmlQueryContext::Lazy',
                  'XmlQueryContext::LiveValues'])

have_dbxml_const([
                  'XmlValue::ANY_SIMPLE_TYPE',
                  'XmlValue::ANY_URI',
                  'XmlValue::BASE_64_BINARY',
                  'XmlValue::BOOLEAN',
                  'XmlValue::DATE',
                  'XmlValue::DATE_TIME',
                  'XmlValue::DAY_TIME_DURATION',
                  'XmlValue::DECIMAL',
                  'XmlValue::DOUBLE',
                  'XmlValue::DURATION',
                  'XmlValue::FLOAT',
                  'XmlValue::G_DAY',
                  'XmlValue::G_MONTH',
                  'XmlValue::G_MONTH_DAY',
                  'XmlValue::G_YEAR',
                  'XmlValue::G_YEAR_MONTH',
                  'XmlValue::HEX_BINARY',
                  'XmlValue::NODE',
                  'XmlValue::NONE',
                  'XmlValue::NOTATION',
                  'XmlValue::QNAME',
                  'XmlValue::STRING',
                  'XmlValue::TIME',
                  'XmlValue::UNTYPED_ATOMIC ',
                  'XmlValue::YEAR_MONTH_DURATION'])
#

try_dbxml_compile('XmlManager::compactContainer', 'MAN_COMPACT_CONTAINER', <<EOT)
   XmlManager *man = new XmlManager((u_int32_t)0);
   XmlUpdateContext *upd = 0;
   const std::string name = "";
   man->compactContainer(name, *upd, (u_int32_t)0);
EOT

try_dbxml_compile('XmlManager::truncateContainer', 'MAN_TRUNCATE_CONTAINER', <<EOT)
   XmlManager *man = new XmlManager((u_int32_t)0);
   XmlUpdateContext *upd = 0;
   const std::string name = "";
   man->truncateContainer(name, *upd, (u_int32_t)0);
EOT

try_dbxml_compile('XmlManager::getFlags', 'MAN_GET_FLAGS', <<EOT)
   XmlManager *man = new XmlManager((u_int32_t)0);
   man->getFlags();
EOT

try_dbxml_compile('XmlManager::getImplicitTimezone', 
                  'MAN_GET_IMPLICIT_TIMEZONE', <<EOT)
   XmlManager *man = new XmlManager((u_int32_t)0);
   man->getImplicitTimezone();
EOT

try_dbxml_compile('XmlManager::existsContainer',
                  'MAN_EXISTS_CONTAINER', <<EOT)
   XmlManager *man = new XmlManager((u_int32_t)0);
   man->existsContainer("");
EOT

try_dbxml_compile('XmlManager::reindexContainer', 'MAN_REINDEX_CONTAINER', <<EOT)
   XmlManager *man = new XmlManager((u_int32_t)0);
   XmlUpdateContext *upd = 0;
   const std::string name = "";
   man->reindexContainer(name, *upd, (u_int32_t)0);
EOT

try_dbxml_compile('XmlManager::getDefaultSequenceIncrement',
                  'MAN_DEFAULT_SEQUENCE_INCREMENT', <<EOT)
   XmlManager *man = new XmlManager((u_int32_t)0);
   man->getDefaultSequenceIncrement();
EOT

try_dbxml_compile('XmlIndexLookup::Operation',
                  'XML_INDEX_LOOKUP', <<EOT)
   XmlIndexLookup::Operation x = (XmlIndexLookup::Operation)0;
EOT

try_dbxml_compile('XmlContainer::getIndexNodes',
                  'CON_INDEX_NODES', <<EOT)
  XmlManager *man = new XmlManager((u_int32_t)0);
  XmlContainer con = man->openContainer((char *)"", 0);
  con.getIndexNodes();
EOT

try_dbxml_compile('XmlContainer::getPageSize',
                  'CON_PAGESIZE', <<EOT)
  XmlManager *man = new XmlManager((u_int32_t)0);
  XmlContainer con = man->openContainer((char *)"", 0);
  con.getPageSize();
EOT

try_dbxml_compile('XmlContainer::getFlags',
                  'CON_FLAGS', <<EOT)
  XmlManager *man = new XmlManager((u_int32_t)0);
  XmlContainer con = man->openContainer((char *)"", 0);
  con.getFlags();
EOT

try_dbxml_compile('XmlContainer::getNode',
                  'CON_NODE', <<EOT)
  XmlManager *man = new XmlManager((u_int32_t)0);
  XmlContainer con = man->openContainer((char *)"", 0);
  con.getNode("", 0);
EOT

try_dbxml_compile('XmlEventWriter',
                  'XML_EVENT_WRITER', <<EOT)
  XmlManager *man = new XmlManager((u_int32_t)0);
  XmlUpdateContext *upd = 0;
  XmlDocument *doc = 0;
  XmlContainer con = man->openContainer((char *)"", 0);
  con.putDocumentAsEventWriter(*doc, *upd, 0);
EOT

try_dbxml_compile('new XmlEventWriter',
                  'XML_EVENT_WRITER_ALLOC', <<EOT)
  XmlEventWriter ewr = new XmlEventWriter();
EOT

try_dbxml_compile('XmlEventReader',
                  'XML_EVENT_READER', <<EOT)
     XmlEventReader::XmlEventType type = XmlEventReader::XmlEventType(0);
EOT

try_dbxml_compile('new XmlEventReader',
                  'XML_EVENT_READER_ALLOC', <<EOT)
  XmlEventReader ewr = new XmlEventReader();
EOT

try_dbxml_compile('XmlUpdateContext::getApplyChangesToContainers',
                  'UPDATE_APPLY_CHANGES', <<EOT)
  XmlManager *man = new XmlManager((u_int32_t)0);
  XmlUpdateContext upd = man->createUpdateContext();
  upd.getApplyChangesToContainers();
EOT

try_dbxml_compile('XmlQueryContext::getVariableValue',
                  'CXT_VARIABLE_VALUE', <<EOT)
  XmlManager *man = new XmlManager((u_int32_t)0);
  XmlQueryContext::ReturnType rt = XmlQueryContext::LiveValues;
  XmlQueryContext::EvaluationType et = XmlQueryContext::Eager;
  XmlQueryContext cxt = man->createQueryContext(rt, et);
  XmlResults res;
  cxt.getVariableValue((char *)"", res);
EOT

try_dbxml_compile('XmlQueryContext::getDefaultCollection',
                  'CXT_COLLECTION', <<EOT)
  XmlManager *man = new XmlManager((u_int32_t)0);
  XmlQueryContext::ReturnType rt = XmlQueryContext::LiveValues;
  XmlQueryContext::EvaluationType et = XmlQueryContext::Eager;
  XmlQueryContext cxt = man->createQueryContext(rt, et);
  XmlResults res;
  cxt.getDefaultCollection();
EOT

try_dbxml_compile('XmlQueryContext::interruptQuery',
                  'CXT_INTERRUPT', <<EOT)
  XmlManager *man = new XmlManager((u_int32_t)0);
  XmlQueryContext::ReturnType rt = XmlQueryContext::LiveValues;
  XmlQueryContext::EvaluationType et = XmlQueryContext::Eager;
  XmlQueryContext cxt = man->createQueryContext(rt, et);
  XmlResults res;
  cxt.interruptQuery();
EOT

try_dbxml_compile('XmlQueryContext::getQueryTimeoutSeconds',
                  'CXT_TIMEOUT', <<EOT)
  XmlManager *man = new XmlManager((u_int32_t)0);
  XmlQueryContext::ReturnType rt = XmlQueryContext::LiveValues;
  XmlQueryContext::EvaluationType et = XmlQueryContext::Eager;
  XmlQueryContext cxt = man->createQueryContext(rt, et);
  XmlResults res;
  cxt.getQueryTimeoutSeconds();
EOT

try_dbxml_compile('XmlQueryExpression::isUpdateExpression',
                  'QUE_UPDATE', <<EOT)
  XmlManager *man = new XmlManager((u_int32_t)0);
  XmlQueryContext cxt = man->createQueryContext();
  XmlQueryExpression que =  man->prepare((char *)"", cxt);
  que.isUpdateExpression();
EOT

try_dbxml_compile('XmlResults::getEvaluationType',
                  'RES_EVAL', <<EOT)
  XmlResults res;
  res.getEvaluationType();
EOT

try_dbxml_compile('XmlModify::setNewEncoding',
                  'MOD_ENCODING', <<EOT)
  XmlModify mod;
  mod.setNewEncoding((char *)"");
EOT

try_dbxml_compile('XmlModify::addAppendStep',
                  'MOD_APPEND_STEP_RES', <<EOT)
  XmlModify mod;
  XmlQueryExpression que;
  XmlModify::XmlObject type = XmlModify::XmlObject(0);
  XmlResults res;
  mod.addAppendStep(que, type, (char *)"", res, 0);
EOT

try_dbxml_compile('XmlValue::getTypeURI',
                  'VAL_TYPE_URI', <<EOT)
   XmlValue val;
   val.getTypeURI();
EOT

try_dbxml_compile('XmlValue::getTypeName',
                  'VAL_TYPE_NAME', <<EOT)
   XmlValue val;
   val.getTypeName();
EOT

try_dbxml_compile('XmlValue::getNodeHandle',
                  'VAL_NODE_HANDLE', <<EOT)
   XmlValue val;
   val.getNodeHandle();
EOT
#

open('bdbxml_features.h', 'w') do |gen|
   $defs.delete_if do |k|
      if /\A-DHAVE/ =~ k
         k.sub!(/\A-D/, '#define ')
         k << ' 1'
         gen.puts k
         true
      end
   end
   $defs.delete_if do |k|
      if /\A-DNOT_HAVE/ =~ k
         k.sub!(/\A-DNOT_/, '#define ')
         k << ' 0'
         gen.puts k
         true
      end
   end
end

