#include "bdbxml.h"

static VALUE xb_eFatal, xb_cTxn, xb_cEnv, xb_cRes;
static VALUE xb_cCon, xb_cDoc, xb_cCxt, xb_cPat, xb_cTmp;
static VALUE xb_cInd, xb_cUpd;

#if defined(DBXML_DOM_XERCES2)
static VALUE xb_cNol, xb_cNod;
#endif

static ID id_current_env;

static VALUE xb_internal_ary;

static VALUE
xb_s_new(int argc, VALUE *argv, VALUE obj)
{
    VALUE res = rb_funcall2(obj, rb_intern("allocate"), 0, 0);
    rb_obj_call_init(res, argc, argv);
    return res;
}

static VALUE
xb_int_close(VALUE obj)
{
    return rb_funcall2(obj, rb_intern("close"), 0, 0);
}

static VALUE
xb_s_open(int argc, VALUE *argv, VALUE obj)
{
    VALUE res = rb_funcall2(obj, rb_intern("new"), argc, argv);
    if (rb_block_given_p()) {
	return rb_ensure(RMF(rb_yield), res, RMF(xb_int_close), res);
    }
    return res;
}

static void
xb_doc_free(xdoc *doc)
{
    if (doc->doc) {
	delete doc->doc;
    }
}

static void
xb_doc_mark(xdoc *doc)
{
    rb_gc_mark(doc->uri);
    rb_gc_mark(doc->prefix);
}

static VALUE
xb_doc_s_alloc(VALUE obj)
{
    xdoc *doc;
    VALUE res = Data_Make_Struct(obj, xdoc, (RDF)xb_doc_mark, (RDF)xb_doc_free, doc);
    doc->doc = new XmlDocument();
    return res;
}

static VALUE xb_doc_content_set(VALUE, VALUE);

static VALUE
xb_doc_init(int argc, VALUE *argv, VALUE obj)
{
    VALUE a;

    if (rb_scan_args(argc, argv, "01", &a)) {
	xb_doc_content_set(obj, a);
    }
    return obj;
}

static VALUE
xb_doc_name_get(VALUE obj)
{
    xdoc *doc;
    Data_Get_Struct(obj, xdoc, doc);
    return rb_tainted_str_new2(doc->doc->getName().c_str());
}

static VALUE
xb_doc_name_set(VALUE obj, VALUE a)
{
    xdoc *doc;
    char *str = StringValuePtr(a);
    Data_Get_Struct(obj, xdoc, doc);
    PROTECT(doc->doc->setName(str));
    return a;
}

static VALUE
xb_doc_content_get(VALUE obj)
{
    xdoc *doc;
    std::string str;

    Data_Get_Struct(obj, xdoc, doc);
    doc->doc->getContentAsString(str);
    return rb_tainted_str_new2(str.c_str());
}

static VALUE
xb_doc_content_set(VALUE obj, VALUE a)
{
    xdoc *doc;
    char *str = NULL;

    Data_Get_Struct(obj, xdoc, doc);
    str = StringValuePtr(a);
    PROTECT(doc->doc->setContent(str));
    return a;
}

static VALUE xb_xml_val(XmlValue *, VALUE);
static XmlValue xb_val_xml(VALUE);

static VALUE
xb_doc_get(int argc, VALUE *argv, VALUE obj)
{
    VALUE a, b, c;
    xdoc *doc;
    XmlValue val = "";
    bool result;
    std::string uri = "";
    std::string name;

    Data_Get_Struct(obj, xdoc, doc);
    if (TYPE(doc->uri) == T_STRING) {
	uri = StringValuePtr(doc->uri);
    }
    switch (rb_scan_args(argc, argv, "12", &a, &b, &c)) {
    case 3:
	if (!NIL_P(c) && TYPE(c) != T_CLASS) {
	    rb_raise(xb_eFatal, "expected a class object");
	}
	if (c == rb_cTrueClass || c == rb_cFalseClass) {
	    val = true;
	}
	else if (RTEST(rb_funcall(rb_cNumeric, rb_intern(">="), 1, c))) {
	    val = 12.0;
	}
	else if (NIL_P(c) || c == rb_cNilClass ||
		 RTEST(rb_funcall(rb_cString, rb_intern(">="), 1, c))) {
	    val = "";
	}
	else {
	    rb_raise(xb_eFatal, "invalid class object");
	}
	// ... //
    case 2:
	if (!NIL_P(b)) {
	    uri = StringValuePtr(b);
	}
	break;
    }
    name = StringValuePtr(a);
    PROTECT(result = doc->doc->getMetaData(uri, name, val));
    if (result) {
	return xb_xml_val(&val, 0);
    }
    return rb_str_new2("");
}

static VALUE
xb_doc_set(int argc, VALUE *argv, VALUE obj)
{
    xdoc *doc;
    VALUE a, b, c, d;
    std::string uri = "";
    std::string prefix = "";
    std::string name;

    Data_Get_Struct(obj, xdoc, doc);
    if (TYPE(doc->uri) == T_STRING) {
	uri = StringValuePtr(doc->uri);
    }
    if (TYPE(doc->prefix) == T_STRING) {
	prefix = StringValuePtr(doc->prefix);
    }
    switch (rb_scan_args(argc, argv, "22", &a, &b, &c, &d)) {
    case 4:
	uri = StringValuePtr(a);
	prefix = StringValuePtr(b);
	name = StringValuePtr(c);
	break;
    case 3:
	prefix = StringValuePtr(a);
	name = StringValuePtr(b);
	d = c;
	break;
    case 2:
	name = StringValuePtr(a);
	d = b;
	break;
    }
    doc->doc->setMetaData(uri, prefix, name, xb_val_xml(d));
    return b;
}

static VALUE
xb_doc_id_get(VALUE obj)
{
    xdoc *doc;

    Data_Get_Struct(obj, xdoc, doc);
    return INT2FIX(doc->doc->getID());
}

static VALUE
xb_doc_uri_get(VALUE obj)
{
    xdoc *doc;

    Data_Get_Struct(obj, xdoc, doc);
    if (TYPE(doc->uri) == T_STRING) {
	return rb_str_dup(doc->uri);
    }
    return rb_str_new2("");
}

static VALUE
xb_doc_uri_set(VALUE obj, VALUE a)
{
    xdoc *doc;

    Check_Type(a, T_STRING);
    Data_Get_Struct(obj, xdoc, doc);
    doc->uri = rb_str_dup(a);
    return obj;
}

static VALUE
xb_doc_prefix_get(VALUE obj)
{
    xdoc *doc;

    Data_Get_Struct(obj, xdoc, doc);
    if (TYPE(doc->prefix) == T_STRING) {
	return rb_str_dup(doc->prefix);
    }
    return rb_str_new2("");
}

static VALUE
xb_doc_prefix_set(VALUE obj, VALUE a)
{
    xdoc *doc;

    Check_Type(a, T_STRING);
    Data_Get_Struct(obj, xdoc, doc);
    doc->prefix = rb_str_dup(a);
    return obj;
}

#if defined(DBXML_DOM_XERCES2)

#include <sstream>

#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMAttr.hpp>
#include <xercesc/dom/DOMCharacterData.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMEntity.hpp>
#include <xercesc/dom/DOMNotation.hpp>
#include <xercesc/dom/DOMProcessingInstruction.hpp>
#include <xercesc/dom/DOMException.hpp>
#include <xercesc/dom/DOMWriter.hpp>
#include <xercesc/util/XMLString.hpp>

#define XCNQ XERCES_CPP_NAMESPACE_QUALIFIER

const char *serialiseNode(XCNQ DOMNode *toWrite)
{
    std::ostringstream result;
    char *tmp;
    char *nodeName = XCNQ XMLString::transcode(toWrite->getNodeName());
    char *nodeValue = XCNQ XMLString::transcode(toWrite->getNodeValue());

    switch (toWrite->getNodeType()) {
    case XCNQ DOMNode::DOCUMENT_NODE:
    {
	result << "<?xml version='1.0' encoding='UTF-8' ?>\n";
	XCNQ DOMNode *child = toWrite->getFirstChild();
	while (child != 0) {
	    result << serialiseNode(child);
	    child = child->getNextSibling();
	}
	break;
    }

    case XCNQ DOMNode::DOCUMENT_TYPE_NODE:
    {
	XCNQ DOMDocumentType *docTypeNode = (XCNQ DOMDocumentType *)toWrite;

	result << "<!DOCTYPE " << docTypeNode->getName();
	char *pubID = XCNQ XMLString::transcode(docTypeNode->getPublicId());
	char *sysID = XCNQ XMLString::transcode(docTypeNode->getSystemId());

	if (pubID != NULL && pubID[0] != 0) {
	    result << " PUBLIC \"" << pubID << "\" \"" << sysID << "\"";
	} 
	else {
	    if (sysID != NULL && sysID[0] != 0) {
		result << " SYSTEM \"" << sysID << "\"";
	    }
	}
	XCNQ XMLString::release(&pubID);
	XCNQ XMLString::release(&sysID);
	result << ">";
	rb_warning("A DOCTYPE tag was output, contents of tag will be empty");
	break;
    }

    case XCNQ DOMNode::ELEMENT_NODE:
    {
	result << "<" << nodeName;
	XCNQ DOMNamedNodeMap *attributes = toWrite->getAttributes();
	int attrCount = attributes->getLength();
	for (int i = 0; i < attrCount; i++) {
	    XCNQ DOMNode *attribute = attributes->item(i);
	    tmp = XCNQ XMLString::transcode(attribute->getNodeName());
	    result << " " << tmp;
	    XCNQ XMLString::release(&tmp);
	    tmp = XCNQ XMLString::transcode(attribute->getNodeValue());
	    result << "=\"" << tmp;
	    XCNQ XMLString::release(&tmp);
	    result << "\"";
	}
	if (toWrite->hasChildNodes()) {
	    result << ">";
	    XCNQ DOMNode *child = toWrite->getFirstChild();
	    while (child != 0) {
		result << serialiseNode(child);
		child = child->getNextSibling();
	    }
	    result << "</" << nodeName << ">";
	} else {
	    result << "/>";
	}
	break;
    }

    case XCNQ DOMNode::ATTRIBUTE_NODE:
	tmp = XCNQ XMLString::transcode(toWrite->getNamespaceURI());
	if (tmp && tmp[0]) {
	    result << "{" << tmp << "}";
	    XCNQ XMLString::release(&tmp);
	}
	result << nodeName << "=\"" << nodeValue << "\"";
	break;

    case XCNQ DOMNode::TEXT_NODE:
	result << nodeValue;
	break;

    case XCNQ DOMNode::ENTITY_REFERENCE_NODE:
    {
	XCNQ DOMNode *child = toWrite->getFirstChild();

	while (child != 0) {
	    result << serialiseNode(child);
	    child = child->getNextSibling();
	}
	break;
    }

    case XCNQ DOMNode::PROCESSING_INSTRUCTION_NODE:
	result << "<?" << nodeName << " " << nodeValue << "?>";
	break;

    case XCNQ DOMNode::CDATA_SECTION_NODE:
	result << "<![CDATA[" << nodeValue << "]]>";
	break;

    case XCNQ DOMNode::COMMENT_NODE:
	result << "<!--" << nodeValue << "-->";
	break;

    case XCNQ DOMNode::DOCUMENT_FRAGMENT_NODE:
	if (toWrite->hasChildNodes()) {
	    XCNQ DOMNode *child = toWrite->getFirstChild();
	    while (child != 0) {
		result << serialiseNode(child);
		child = child->getNextSibling();
	    }
	}
	break;

    default:
	rb_warning("Unrecognized node type: %d", toWrite->getNodeType());
	break;
    }
    XCNQ XMLString::release(&nodeName);
    XCNQ XMLString::release(&nodeValue);
    return result.str().c_str();
}

static VALUE
xb_nod_to_s(VALUE obj)
{
    XCNQ DOMNode *node;
    Data_Get_Struct(obj, XCNQ DOMNode, node);
    return rb_str_new2(serialiseNode(node));
}

static VALUE
xb_nod_inspect(VALUE obj)
{
    return rb_str_inspect(xb_nod_to_s(obj));
}

static void
xb_nol_free(xnol *nol)
{
    ::free(nol);
}

static void
xb_nol_mark(xnol *nol)
{
    rb_gc_mark(nol->cxt_val);
}

static VALUE
xb_nol_size(VALUE obj)
{
    xnol *nol;

    Data_Get_Struct(obj, xnol, nol);
    return INT2NUM(nol->nol->getLength());
}

static VALUE
xb_nol_get(VALUE obj, VALUE a)
{
    xnol *nol;
    XCNQ DOMNode *item;
    int index;

    Data_Get_Struct(obj, xnol, nol);
    index = NUM2INT(a);
    if (index < 0) {
	index += nol->nol->getLength(); 
    }
    if (index < 0) {
	return Qnil;
    }
    item = nol->nol->item(index);
    if (item == NULL) {
	return Qnil;
    }
    return Data_Wrap_Struct(xb_cNod, 0, 0, item);
}

static VALUE
xb_nol_each(VALUE obj)
{
    int i = 0;
    VALUE item;

    while ((item = xb_nol_get(obj, INT2NUM(i))) != Qnil) {
	rb_yield(item);
	i += 1;
    }
    return Qnil;
}

static VALUE
xb_nol_to_ary(VALUE obj)
{
    int i = 0;
    VALUE item, res;

    res = rb_ary_new();
    while ((item = xb_nol_get(obj, INT2NUM(i))) != Qnil) {
	rb_ary_push(res, item);
	i += 1;
    }
    return res;
}

static VALUE
xb_nol_inspect(VALUE obj)
{
    return rb_funcall2(xb_nol_to_ary(obj), rb_intern("inspect"), 0, 0);
}

static VALUE
xb_nol_to_str(VALUE obj)
{
    xnol *nol;
    xcxt *cxt;
    XmlQueryContext *qcxt = 0;

    Data_Get_Struct(obj, xnol, nol);
    if (nol->cxt_val) {
	Data_Get_Struct(nol->cxt_val, xcxt, cxt);
	qcxt = cxt->cxt;
    }
    return rb_tainted_str_new2(XmlValue(nol->nol).asString(qcxt).c_str());
}

#endif

static VALUE
xb_xml_val(XmlValue *val, VALUE cxt_val)
{
    VALUE res;
    xdoc *doc;
#if defined(DBXML_DOM_XERCES2)
    xnol *nol;
#endif
    xcxt *cxt;
    XmlQueryContext *qcxt = 0;

    if (cxt_val) {
	Data_Get_Struct(cxt_val, xcxt, cxt);
	qcxt = cxt->cxt;
    }
    if (val->isNull()) {
	return Qnil;
    }
    switch (val->getType(qcxt)) {
    case XmlValue::STRING:
	res = rb_tainted_str_new2(val->asString(qcxt).c_str());
	break;
    case XmlValue::NUMBER:
	res = rb_float_new(val->asNumber(qcxt));
	break;
    case XmlValue::BOOLEAN:
	res = (val->asBoolean(qcxt))?Qtrue:Qfalse;
	break;
    case XmlValue::DOCUMENT:
	res = Data_Make_Struct(xb_cDoc, xdoc, (RDF)xb_doc_mark, (RDF)xb_doc_free, doc);
	doc->doc = new XmlDocument(val->asDocument(qcxt));
	break;
#if defined(DBXML_DOM_XERCES2)
    case XmlValue::NODELIST:
	res = Data_Make_Struct(xb_cNol, xnol, (RDF)xb_nol_mark, (RDF)xb_nol_free, nol);
	nol->nol = val->asNodeList(qcxt);
	nol->cxt_val = cxt_val;
	break;
#endif
    case XmlValue::VARIABLE:
	rb_raise(xb_eFatal, "a context is required");
	break;
    default:
	rb_raise(xb_eFatal, "Unknown type");
	break;
    }
    return res;
}

static XmlValue
xb_val_xml(VALUE a)
{
    XmlValue val;
    xdoc *doc;
#if defined(DBXML_DOM_XERCES2)
    xnol *nol;
#endif

    if (NIL_P(a)) {
	return XmlValue();
    }
    switch (TYPE(a)) {
    case T_STRING:
	val = XmlValue(StringValuePtr(a));
	break;
    case T_FIXNUM:
    case T_FLOAT:
    case T_BIGNUM:
	val = XmlValue(NUM2DBL(a));
	break;
    case T_TRUE:
	val = XmlValue(true);
	break;
    case T_FALSE:
	val = XmlValue(false);
	break;
    case T_DATA:
	if (RDATA(a)->dfree == (RDF)xb_doc_free) {
	    Data_Get_Struct(a, xdoc, doc);
	    val = XmlValue(*(doc->doc));
	    break;
	}
#if defined(DBXML_DOM_XERCES2)
	if (RDATA(a)->dfree == (RDF)xb_nol_free) {
	    Data_Get_Struct(a, xnol, nol);
	    val = XmlValue(nol->nol);
	    break;
	}
#endif
	/* ... */
    default:
	val = XmlValue(StringValuePtr(a));
	break;
    }
    return val;
}

static void
xb_pat_free(XmlQueryExpression *pat)
{
    delete pat;
}

static VALUE
xb_pat_to_str(VALUE obj)
{
    XmlQueryExpression *pat;
    Data_Get_Struct(obj, XmlQueryExpression, pat);
    return rb_tainted_str_new2(pat->getXPathQuery().c_str());
}

static void
xb_cxt_free(xcxt *cxt)
{
    delete cxt->cxt;
}

static VALUE
xb_cxt_s_alloc(VALUE obj)
{
    VALUE res;
    xcxt *cxt;

    res = Data_Make_Struct(obj, xcxt, 0, (RDF)xb_cxt_free, cxt);
    cxt->cxt = new XmlQueryContext();
    cxt->returntype = XmlQueryContext::ResultDocuments;
    cxt->evaltype = XmlQueryContext::Eager;
    return res;
}

static VALUE
xb_cxt_init(int argc, VALUE *argv, VALUE obj)
{
    xcxt *cxt;
    XmlQueryContext::ReturnType rt;
    XmlQueryContext::EvaluationType et;
    VALUE a, b;

    Data_Get_Struct(obj, xcxt, cxt);
    switch (rb_scan_args(argc, argv, "02", &a, &b)) {
    case 2:
	et = XmlQueryContext::EvaluationType(NUM2INT(b));
	PROTECT(cxt->cxt->setEvaluationType(et));
	cxt->evaltype = et;
	/* ... */
    case 1:
	if (!NIL_P(a)) {
	    rt = XmlQueryContext::ReturnType(NUM2INT(a));
	    PROTECT(cxt->cxt->setReturnType(rt));
	    cxt->returntype = rt;
	}
    }
    return obj;
}

static VALUE
xb_cxt_name_set(VALUE obj, VALUE a, VALUE b)
{
    xcxt *cxt;
    char *pre, *uri;

    Data_Get_Struct(obj, xcxt, cxt);
    if (NIL_P(b)) {
	cxt->cxt->removeNamespace(StringValuePtr(a));
	return a;
    }
    pre = StringValuePtr(a);
    uri = StringValuePtr(b);
    PROTECT(cxt->cxt->setNamespace(pre, uri));
    return obj;
}

static VALUE
xb_cxt_name(VALUE obj)
{
    xcxt *cxt;
    VALUE res;

    Data_Get_Struct(obj, xcxt, cxt);
    res = Data_Wrap_Struct(xb_cTmp, 0, 0, cxt);
    rb_define_singleton_method(res, "[]=", RMF(xb_cxt_name_set), 2);
    return res;
}

static VALUE
xb_cxt_name_get(VALUE obj, VALUE a)
{
    xcxt *cxt;
    Data_Get_Struct(obj, xcxt, cxt);
    return rb_tainted_str_new2(cxt->cxt->getNamespace(StringValuePtr(a)).c_str());
}

static VALUE
xb_cxt_name_del(VALUE obj, VALUE a)
{
    xcxt *cxt;
    Data_Get_Struct(obj, xcxt, cxt);
    cxt->cxt->removeNamespace(StringValuePtr(a));
    return a;
}

static VALUE
xb_cxt_clear(VALUE obj)
{
    xcxt *cxt;
    Data_Get_Struct(obj, xcxt, cxt);
    cxt->cxt->clearNamespaces();
    return obj;
}

static VALUE
xb_cxt_get(VALUE obj, VALUE a)
{
    xcxt *cxt;
    XmlValue val;
    Data_Get_Struct(obj, xcxt, cxt);
    cxt->cxt->getVariableValue(StringValuePtr(a), val);
    if (val.isNull()) {
	return Qnil;
    }
    return xb_xml_val(&val, obj);
}

static VALUE
xb_cxt_set(VALUE obj, VALUE a, VALUE b)
{
    xcxt *cxt;
    XmlValue val = xb_val_xml(b);
    VALUE res = xb_cxt_get(obj, a);

    Data_Get_Struct(obj, xcxt, cxt);
    cxt->cxt->setVariableValue(StringValuePtr(a), val);
    return res;
}

static VALUE
xb_cxt_return_get(VALUE obj)
{
    xcxt *cxt;
    Data_Get_Struct(obj, xcxt, cxt);
    return INT2NUM(cxt->returntype);
}

static VALUE
xb_cxt_return_set(VALUE obj, VALUE a)
{
    xcxt *cxt;
    XmlQueryContext::ReturnType rt;

    Data_Get_Struct(obj, xcxt, cxt);
    rt = XmlQueryContext::ReturnType(NUM2INT(a));
    PROTECT(cxt->cxt->setReturnType(rt));
    cxt->returntype = rt;
    return a;
}

static VALUE
xb_cxt_eval_get(VALUE obj)
{
    xcxt *cxt;
    Data_Get_Struct(obj, xcxt, cxt);
    return INT2NUM(cxt->evaltype);
}

static VALUE
xb_cxt_eval_set(VALUE obj, VALUE a)
{
    xcxt *cxt;
    XmlQueryContext::EvaluationType et;

    Data_Get_Struct(obj, xcxt, cxt);
    et = XmlQueryContext::EvaluationType(NUM2INT(a));
    PROTECT(cxt->cxt->setEvaluationType(et));
    cxt->evaltype = et;
    return a;
}

static void
xb_res_free(xres *xes)
{
    delete xes->res;
    ::free(xes);
}

static void
xb_res_mark(xres *xes)
{
    rb_gc_mark(xes->txn_val);
    rb_gc_mark(xes->cxt_val);
}

static VALUE
xb_res_search(VALUE obj)
{
    xres *xes;
    XmlValue val;
    DbTxn *txn = 0;
    VALUE res = Qnil;

    Data_Get_Struct(obj, xres, xes);
    if (xes->txn_val) {
	bdb_TXN *txnst;
	GetTxnDBErr(xes->txn_val, txnst, xb_eFatal);
	txn = static_cast<DbTxn *>(txnst->txn_cxx);
    }
    if (!rb_block_given_p()) {
	res = rb_ary_new();
    }
    try {xes->res->reset(); } catch (...) {}
    while (true) {
	PROTECT(xes->res->next(txn, val));
	if (val.isNull()) {
	    break;
	}
	if (NIL_P(res)) {
	    rb_yield(xb_xml_val(&val, xes->cxt_val));
	}
	else {
	    rb_ary_push(res, xb_xml_val(&val, xes->cxt_val));
	}
    }
    if (NIL_P(res)) {
	return obj;
    }
    return res;
}

static VALUE
xb_res_each(VALUE obj)
{
    if (!rb_block_given_p()) {
	rb_raise(rb_eArgError, "block not supplied");
    }
    return xb_res_search(obj);
}

struct xb_eiv {
    xcon *con;
};

static void
xb_con_mark(xcon *con)
{
    rb_gc_mark(con->env_val);
    rb_gc_mark(con->txn_val);
    rb_gc_mark(con->name);
}

static void
xb_con_i_close(xcon *con, int flags, bool protect)
{
    VALUE db_ary;
    bdb_ENV *envst;
    bdb_TXN *txnst;
    int i;

    if (!con->closed) {
	db_ary = 0;
	if (con->txn_val) {
	    Data_Get_Struct(con->txn_val, bdb_TXN, txnst);
	    db_ary = txnst->db_ary;
	}
	else if (con->env_val) {
	    Data_Get_Struct(con->env_val, bdb_ENV, envst);
	    db_ary = envst->db_ary;
	}
	if (db_ary) {
	    for (i = 0; i < RARRAY(db_ary)->len; ++i) {
		if (RARRAY(db_ary)->ptr[i] == con->ori_val) {
		    rb_ary_delete_at(db_ary, i);
		    break;
		}
	    }
	}
	else {
	    struct xb_eiv *civ;
	    
	    for (i = 0; i < RARRAY(xb_internal_ary)->len; ++i) {
		Data_Get_Struct(RARRAY(xb_internal_ary)->ptr[i], struct xb_eiv, civ);
		if (civ->con == con) {
		    rb_ary_delete_at(xb_internal_ary, i);
		    break;
		}
	    }
	}
	if (protect) {
	    try { con->con->close(flags); } catch (...) {}
	}
	else {
	    con->closed = Qtrue;
	    PROTECT(con->con->close(flags));
	}
    }
}

static void
xb_con_free(xcon *con)
{
    if (!con->closed && con->con) {
	xb_con_i_close(con, 0, true);
	if (!NIL_P(con->closed)) {
	  delete con->con;
	}
    }
    ::free(con);
}

static VALUE
xb_con_s_alloc(VALUE obj)
{
    xcon *con;
    VALUE res;

    res = Data_Make_Struct(obj, xcon, (RDF)xb_con_mark, (RDF)xb_con_free, con);
    con->ori_val = res;
#ifndef BDB_NO_THREAD
    con->flag |= DB_THREAD;
#endif
    return res;
}

static VALUE
xb_con_s_new(int argc, VALUE *argv, VALUE obj)
{
    DbEnv *envcc = NULL;
    bdb_ENV *envst = NULL;
    bdb_TXN *txnst = NULL;
    int flags = 0, pagesize = 0, nargc;
    char *name = NULL;
    xcon *con;
    VALUE res;
    bool init = true;

    res = rb_funcall2(obj, rb_intern("allocate"), 0, 0);
    Data_Get_Struct(res, xcon, con);
    nargc = argc;
    if (argc && TYPE(argv[argc - 1]) == T_HASH) {
	VALUE env = Qfalse;
	VALUE v, f = argv[argc - 1];

	if ((v = rb_hash_aref(f, rb_str_new2("txn"))) != RHASH(f)->ifnone) {
	    bdb_TXN *txnst;

	    if (!rb_obj_is_kind_of(v, xb_cTxn)) {
		rb_raise(xb_eFatal, "argument of txn must be a transaction");
	    }
	    GetTxnDBErr(v, txnst, xb_eFatal);
	    env = txnst->env;
	    con->txn_val = v;
	}
	else if ((v = rb_hash_aref(f, rb_str_new2("env"))) != RHASH(f)->ifnone) {
	    if (!rb_obj_is_kind_of(v, xb_cEnv)) {
		rb_raise(xb_eFatal, "argument of env must be an environnement");
	    }
	    env = v;
	    GetEnvDBErr(env, envst, id_current_env, xb_eFatal);
	    rb_ary_push(envst->db_ary, res);
	}
	if (env) {
	    con->env_val = env;
	    GetEnvDBErr(env, envst, id_current_env, xb_eFatal);
	    if (envst->options & BDB_NO_THREAD) {
		con->flag &= ~DB_THREAD;
	    }
	    envcc = static_cast<DbEnv *>(envst->envp->app_private);
	}
	if ((v = rb_hash_aref(f, rb_str_new2("flags"))) != RHASH(f)->ifnone) {
	    flags = NUM2INT(v);
	}
	if ((v = rb_hash_aref(f, rb_str_new2("set_pagesize"))) != RHASH(f)->ifnone) {
	    pagesize = NUM2INT(v);
	}
	if ((v = rb_hash_aref(f, INT2NUM(rb_intern("__ts__no__init__")))) != RHASH(f)->ifnone) {
	    init = false;
	}
	nargc--;
    }
    if (nargc) {
	SafeStringValue(argv[0]);
	con->name = rb_str_dup(argv[0]);
	name = StringValuePtr(argv[0]);
    }
    PROTECT(con->con = new XmlContainer(envcc, name, flags));
    if (pagesize) {
	PROTECT(con->con->setPageSize(pagesize));
    }
    if (init) {
	rb_obj_call_init(res, nargc, argv);
	if (txnst) {
	    rb_ary_push(txnst->db_ary, res);
	}
	else if (envst) {
	    rb_ary_push(envst->db_ary, res);
	}
	else {
	    VALUE st;
	    struct xb_eiv *civ;

	    st = Data_Make_Struct(rb_cData, struct xb_eiv, 0, free, civ);
	    civ->con = con;
	    rb_ary_push(xb_internal_ary, st);
	}
    }
    return res;
}

static VALUE
xb_con_i_options(VALUE obj, VALUE conobj)
{
    VALUE key, value;
    char *options;
    xcon *con;

    Data_Get_Struct(conobj, xcon, con);
    key = rb_ary_entry(obj, 0);
    value = rb_ary_entry(obj, 1);
    key = rb_obj_as_string(key);
    options = StringValuePtr(key);
    if (strcmp(options, "thread") == 0) {
	switch (value) {
	case Qtrue: con->flag &= ~DB_THREAD; break;
	case Qfalse: con->flag |= DB_THREAD; break;
	default: rb_raise(xb_eFatal, "thread value must be true or false");
	}
    }
    return Qnil;
}

static VALUE
xb_con_init(int argc, VALUE *argv, VALUE obj)
{
    xcon *con;
    DB_ENV *envp = NULL;
    DbTxn *txn = 0;
    int flags = 0;
    int mode = 0;
    char *name = NULL;
    VALUE a, b, c, hash_arg;

    GetConTxn(obj, con, txn);
    hash_arg = Qnil;
    if (argc && TYPE(argv[argc - 1]) == T_HASH) {
	hash_arg = argv[argc - 1];
	argc--;
    }
    switch (rb_scan_args(argc, argv, "12", &a, &b, &c)) {
    case 3:
	mode = NUM2INT(c);
	/* ... */
    case 2:
	if (NIL_P(b)) {
	    flags = 0;
	}
	else if (TYPE(b) == T_STRING) {
	    if (strcmp(StringValuePtr(b), "r") == 0)
		flags = DB_RDONLY;
	    else if (strcmp(StringValuePtr(b), "r+") == 0)
		flags = 0;
	    else if (strcmp(StringValuePtr(b), "w") == 0 ||
		     strcmp(StringValuePtr(b), "w+") == 0)
		flags = DB_CREATE | DB_TRUNCATE;
	    else if (strcmp(StringValuePtr(b), "a") == 0 ||
		     strcmp(StringValuePtr(b), "a+") == 0)
		flags = DB_CREATE;
	    else {
		rb_raise(xb_eFatal, "flags must be r, r+, w, w+, a or a+");
	    }
	}
	else {
	    flags = NUM2INT(b);
	}
    }
    if (flags & DB_TRUNCATE) {
	rb_secure(2);
    }
    if (flags & DB_CREATE) {
	rb_secure(4);
    }
    if (rb_safe_level() >= 4) {
	flags |= DB_RDONLY;
    }
    if (!txn && con->env_val) {
	bdb_ENV *envst = NULL;
	GetEnvDBErr(con->env_val, envst, id_current_env, xb_eFatal);
	if (envst->options & BDB_AUTO_COMMIT) {
	    flags |= DB_AUTO_COMMIT;
	    con->flag |= BDB_AUTO_COMMIT;
	}
    }
    if (con->flag & DB_THREAD) {
	flags |= DB_THREAD;
    }
    PROTECT2(con->con->open(txn, flags, mode), con->closed = Qnil);
    return obj;
}

static VALUE
xb_con_close(int argc, VALUE *argv, VALUE obj)
{
    VALUE a;
    xcon *con;
    int flags = 0;

    if (!OBJ_TAINTED(obj) && rb_safe_level() >= 4) {
	rb_raise(rb_eSecurityError, "Insecure: can't close the container");
    }
    Data_Get_Struct(obj, xcon, con);
    if (!con->closed && con->con) {
	if (rb_scan_args(argc, argv, "01", &a)) {
	    flags = NUM2INT(a);
	}
	xb_con_i_close(con, flags, false);
	delete con->con;
    }
    return Qnil;
}

static VALUE
xb_env_open_xml(int argc, VALUE *argv, VALUE obj)
{
    int nargc;
    VALUE *nargv;

    if (argc && TYPE(argv[argc - 1]) == T_HASH) {
	nargc = argc;
	nargv = argv;
    }
    else {
	nargc = argc + 1;
	nargv = ALLOCA_N(VALUE, nargc);
	MEMCPY(nargv, argv, VALUE, argc);
	nargv[argc] = rb_hash_new();
    }
    if (rb_obj_is_kind_of(obj, xb_cEnv)) {
	rb_hash_aset(nargv[nargc - 1], rb_tainted_str_new2("env"), obj);
    }
    else {
	rb_hash_aset(nargv[nargc - 1], rb_tainted_str_new2("txn"), obj);
    }
    return xb_con_s_new(nargc, nargv, xb_cCon);
}

static VALUE
xb_con_env(VALUE obj)
{
    xcon *con;

    Data_Get_Struct(obj, xcon, con);
    if (con->closed || !con->con) {
	rb_raise(xb_eFatal, "closed container");
    }
    if (RTEST(con->env_val)) {
	return con->env_val;
    }
    return Qnil;
}

static VALUE
xb_con_env_p(VALUE obj)
{
    xcon *con;

    Data_Get_Struct(obj, xcon, con);
    if (con->closed || !con->con) {
	rb_raise(xb_eFatal, "closed container");
    }
    return RTEST(con->env_val);
}

static VALUE
xb_con_txn(VALUE obj)
{
    xcon *con;

    Data_Get_Struct(obj, xcon, con);
    if (con->closed || !con->con) {
	rb_raise(xb_eFatal, "closed container");
    }
    if (RTEST(con->txn_val)) {
	return con->txn_val;
    }
    return Qnil;
}

static VALUE
xb_con_txn_p(VALUE obj)
{
    xcon *con;

    Data_Get_Struct(obj, xcon, con);
    if (con->closed || !con->con) {
	rb_raise(xb_eFatal, "closed container");
    }
    return RTEST(con->txn_val);
}

static void
xb_ind_free(XmlIndexSpecification *ind)
{
    delete ind;
}

static VALUE
xb_con_index(VALUE obj)
{
    xcon *con;
    DbTxn *txn;
    XmlIndexSpecification ind;

    GetConTxn(obj, con, txn);
    PROTECT(ind = con->con->getIndexSpecification(txn));
    return Data_Wrap_Struct(xb_cInd, 0, xb_ind_free, 
			    new XmlIndexSpecification(ind));
}

static VALUE
xb_con_index_set(VALUE obj, VALUE a)
{
    xcon *con;
    DbTxn *txn;
    XmlIndexSpecification *ind;

    if (TYPE(a) != T_DATA || RDATA(a)->dfree != xb_ind_free) {
	    a = rb_funcall2(xb_cInd, rb_intern("new"), 1, &a);
    }
    GetConTxn(obj, con, txn);
    Data_Get_Struct(a, XmlIndexSpecification, ind);
    PROTECT(con->con->setIndexSpecification(txn, *ind));
    return a;
}

static VALUE
xb_ind_s_alloc(VALUE obj)
{
    return Data_Wrap_Struct(obj, 0, xb_ind_free, new XmlIndexSpecification());
}

static VALUE
xb_ind_add(int argc, VALUE *argv, VALUE obj)
{
    XmlIndexSpecification *ind;
    char *as, *bs, *cs;

    Data_Get_Struct(obj, XmlIndexSpecification, ind);
    if (argc == 2) {
	bs = StringValuePtr(argv[0]);
	cs = StringValuePtr(argv[1]);
	PROTECT(ind->addIndex("", bs, cs));
    }
    else {
	if (argc != 3) {
	    rb_raise(xb_eFatal, "invalid number of arguments (%d for 3)", argc);
	}
	as = StringValuePtr(argv[0]);
	bs = StringValuePtr(argv[1]);
	cs = StringValuePtr(argv[2]);
	PROTECT(ind->addIndex(as, bs, cs));
    }
    return obj;
}

static VALUE
xb_ind_init(int argc, VALUE *argv, VALUE obj)
{
    VALUE val, tmp;
    int i;

    if (!argc) {
	return obj;
    }
    if (argc != 1) {
	rb_raise(rb_eArgError, "invalid number of arguments (%d for 1)", argc);
    }
    tmp = rb_Array(argv[0]);
    for (i = 0; i < RARRAY(tmp)->len; ++i) {
	val = rb_Array(RARRAY(tmp)->ptr[i]);
	xb_ind_add(RARRAY(val)->len, RARRAY(val)->ptr, obj);
    }
    return obj;
}

static VALUE
xb_ind_replace(int argc, VALUE *argv, VALUE obj)
{
    XmlIndexSpecification *ind;
    char *as, *bs, *cs;

    Data_Get_Struct(obj, XmlIndexSpecification, ind);
    if (argc == 2) {
	bs = StringValuePtr(argv[0]);
	cs = StringValuePtr(argv[1]);
	PROTECT(ind->replaceIndex("", bs, cs));
    }
    else {
	if (argc != 3) {
	    rb_raise(xb_eFatal, "invalid number of arguments (%d for 3)", argc);
	}
	as = StringValuePtr(argv[0]);
	bs = StringValuePtr(argv[1]);
	cs = StringValuePtr(argv[2]);
	PROTECT(ind->replaceIndex(as, bs, cs));
    }
    return obj;
}

static VALUE
xb_ind_delete(int argc, VALUE *argv, VALUE obj)
{
    XmlIndexSpecification *ind;
    char *as, *bs, *cs;

    Data_Get_Struct(obj, XmlIndexSpecification, ind);
    if (argc == 2) {
	bs = StringValuePtr(argv[0]);
	cs = StringValuePtr(argv[1]);
	PROTECT(ind->deleteIndex("", bs, cs));
    }
    else {
	if (argc != 3) {
	    rb_raise(xb_eFatal, "invalid number of arguments (%d for 3)", argc);
	}
	as = StringValuePtr(argv[0]);
	bs = StringValuePtr(argv[1]);
	cs = StringValuePtr(argv[2]);
	PROTECT(ind->deleteIndex(as, bs, cs));
    }
    return obj;
}

static VALUE
xb_ind_each(VALUE obj)
{
    XmlIndexSpecification *ind;
    std::string uri, name, index;
    VALUE ret;

    Data_Get_Struct(obj, XmlIndexSpecification, ind);
    PROTECT(ind->reset());
    while (ind->next(uri, name, index)) {
	ret = rb_ary_new2(3);
	rb_ary_push(ret, rb_tainted_str_new2(uri.c_str()));
	rb_ary_push(ret, rb_tainted_str_new2(name.c_str()));
	rb_ary_push(ret, rb_tainted_str_new2(index.c_str()));
	rb_yield(ret);
    }
    return Qnil;
}

static VALUE
xb_ind_find(int argc, VALUE *argv, VALUE obj)
{
    XmlIndexSpecification *ind;
    char *uri, *name;
    std::string index;
    VALUE ret;

    Data_Get_Struct(obj, XmlIndexSpecification, ind);
    if (argc == 1) {
	name = StringValuePtr(argv[0]);
	uri = "";
    }
    else {
	if (argc != 2) {
	    rb_raise(xb_eFatal, "invalid number of arguments (%d for 2)", argc);
	}
	uri = StringValuePtr(argv[0]);
	name = StringValuePtr(argv[1]);
    }
    if (ind->find(uri, name, index)) {
	return rb_tainted_str_new2(index.c_str());
    }
    return Qnil;
}

static VALUE
xb_ind_to_a(VALUE obj)
{
    XmlIndexSpecification *ind;
    std::string uri, name, index;
    VALUE ret, res;

    Data_Get_Struct(obj, XmlIndexSpecification, ind);
    PROTECT(ind->reset());
    res = rb_ary_new();
    while (ind->next(uri, name, index)) {
	ret = rb_ary_new2(3);
	rb_ary_push(ret, rb_tainted_str_new2(uri.c_str()));
	rb_ary_push(ret, rb_tainted_str_new2(name.c_str()));
	rb_ary_push(ret, rb_tainted_str_new2(index.c_str()));
	rb_ary_push(res, ret);
    }
    return res;
}

static VALUE
xb_con_name(VALUE obj)
{
    xcon *con;
    Data_Get_Struct(obj, xcon, con);
    return rb_tainted_str_new2(con->con->getName().c_str());
}

static VALUE
xb_con_name_set(VALUE obj, VALUE a)
{
    xcon *con;
    std::string str;

    Data_Get_Struct(obj, xcon, con);
    str = StringValuePtr(a);
    PROTECT(con->con->setName(str));
    return obj;
}

static VALUE
xb_con_get(int argc, VALUE *argv, VALUE obj)
{
    xcon *con;
    DbTxn *txn;
    xdoc *doc;
    VALUE a, b, res;
    int flags = 0;

    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	flags = NUM2INT(b);
    }
    u_int32_t id = NUM2INT(a);
    GetConTxn(obj, con, txn);
    res = Data_Make_Struct(xb_cDoc, xdoc, (RDF)xb_doc_mark, (RDF)xb_doc_free, doc);
    PROTECT(doc->doc = new XmlDocument(con->con->getDocument(txn, id, flags)));
    return res;
}

static VALUE
xb_int_update(int argc, VALUE *argv, VALUE obj, XmlUpdateContext *upd)
{
    xcon *con;
    xdoc *doc;
    DbTxn *txn;
    VALUE a;
    XmlDocument *document;

    rb_secure(4);
    GetConTxn(obj, con, txn);
    if (rb_scan_args(argc, argv, "10", &a) != 1) {
	rb_raise(rb_eArgError, "invalid number of arguments (%d for 1)", argc);
    }
    if (TYPE(a) != T_DATA || RDATA(a)->dfree != (RDF)xb_doc_free) {
	a = xb_s_new(1, &a, xb_cDoc);
    }
    Data_Get_Struct(a, xdoc, doc);
    PROTECT(con->con->updateDocument(txn, *(doc->doc), upd));
    return a;
}

static VALUE
xb_con_update(int argc, VALUE *argv, VALUE obj)
{
    return xb_int_update(argc, argv, obj, 0);
}

static VALUE
xb_int_push(int argc, VALUE *argv, VALUE obj, XmlUpdateContext *upd)
{
    xcon *con;
    DbTxn *txn = NULL;
    xdoc *doc;
    u_int32_t id;
    VALUE a, b;
    int flags = 0;

    rb_secure(4);
    GetConTxn(obj, con, txn);
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	flags = NUM2INT(b);
    }
    if (TYPE(a) != T_DATA || RDATA(a)->dfree != (RDF)xb_doc_free) {
	a = xb_s_new(1, &a, xb_cDoc);
    }
    Data_Get_Struct(a, xdoc, doc);
    PROTECT(id = con->con->putDocument(txn, *(doc->doc), upd, flags));
    return INT2NUM(id);
}

static VALUE
xb_con_push(int argc, VALUE *argv, VALUE obj)
{
    return xb_int_push(argc, argv, obj, 0);
}

static VALUE
xb_con_add(VALUE obj, VALUE a)
{
    xb_con_push(1, &a, obj);
    return obj;
}

static VALUE
xb_con_parse(int argc, VALUE *argv, VALUE obj)
{
    xcon *con;
    DbTxn *txn;
    VALUE a, b;
    std::string str;
    XmlQueryExpression *pat;
    xcxt *cxt;
    XmlQueryContext tmp, *qcxt;

    GetConTxn(obj, con, txn);
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	if (TYPE(b) != T_DATA || RDATA(b)->dfree != (RDF)xb_cxt_free) {
	    rb_raise(xb_eFatal, "Expected a Context object");
	}
	Data_Get_Struct(b, xcxt, cxt);
	qcxt = cxt->cxt;
    }
    else {
	tmp = XmlQueryContext();
	qcxt = &tmp;
    }
    str = StringValuePtr(a);
    PROTECT(pat = new XmlQueryExpression(con->con->parseXPathExpression(txn, str, qcxt)));
    return Data_Wrap_Struct(xb_cPat, 0, (RDF)xb_pat_free, pat);
}

static bool
xb_test_txn(VALUE env, DbTxn **txn)
{
  bdb_ENV *envst;
  DbEnv *envcc;

  if (env) {
      GetEnvDBErr(env, envst, id_current_env, xb_eFatal);
      if (envst->options & BDB_AUTO_COMMIT) {
	  envcc = static_cast<DbEnv *>(envst->envp->app_private);
	  PROTECT(envcc->txn_begin(0, txn, 0));
	  return true;
      }
  }
  return false;
}

static VALUE
xb_doc_query(int argc, VALUE *argv, VALUE obj)
{
    xdoc *doc;
    xres *xes;
    VALUE a, b, res;
    std::string str;
    xcxt *cxt;
    XmlQueryContext *qcxt = 0;

    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	if (TYPE(b) != T_DATA || RDATA(b)->dfree != (RDF)xb_cxt_free) {
	    rb_raise(xb_eFatal, "Expected a Context object");
	}
	Data_Get_Struct(b, xcxt, cxt);
	qcxt = cxt->cxt;
    }
    if (NIL_P(b)) {
	b = Qfalse;
    }
    str = StringValuePtr(a);
    res = Data_Make_Struct(xb_cRes, xres, (RDF)xb_res_mark, (RDF)xb_res_free, xes);
    xes->cxt_val = b;
    PROTECT(xes->res = new XmlResults(doc->doc->queryWithXPath(str, qcxt)));
    return res;
}

static VALUE
xb_con_query(int argc, VALUE *argv, VALUE obj)
{
    xcon *con;
    DbTxn *txn = NULL;
    xres *xes;
    VALUE a, b, c, res;
    char *str = 0;
    int flags = 0;
    XmlQueryExpression *pat;
    xcxt *cxt;
    XmlQueryContext *qcxt = 0;
    bool temporary = false;

    GetConTxn(obj, con, txn);
    switch (rb_scan_args(argc, argv, "12", &a, &b, &c)) {
    case 3:
	if (NIL_P(b)) {
	    b = Qfalse;
	}
	else {
	    if (TYPE(b) != T_DATA || RDATA(b)->dfree != (RDF)xb_cxt_free) {
		rb_raise(xb_eFatal, "Expected a Context object");
	    }
	    Data_Get_Struct(b, xcxt, cxt);
	    qcxt = cxt->cxt;
	}
	flags = NUM2INT(c);
	break;
    case 2:
	if (TYPE(b) == T_DATA && RDATA(b)->dfree == (RDF)xb_cxt_free) {
	    Data_Get_Struct(b, xcxt, cxt);
	    qcxt = cxt->cxt;
	}
	else {
	    flags = NUM2INT(b);
	    b = Qfalse;
	}
	break;
    case 1:
	b = Qfalse;
	break;
    }
    res = Data_Make_Struct(xb_cRes, xres, (RDF)xb_res_mark, (RDF)xb_res_free, xes);
    xes->txn_val = con->txn_val;
    xes->cxt_val = b;
    if (!txn) {
	temporary = xb_test_txn(con->env_val, &txn);
    }
    if (TYPE(a) == T_DATA && RDATA(a)->dfree == (RDF)xb_pat_free) {
	if (argc == 2) {
	    rb_warning("a Context is useless with an XPath");
	}
	Data_Get_Struct(a, XmlQueryExpression, pat);
	PROTECT(xes->res = new XmlResults(con->con->queryWithXPath(txn, *pat, flags)));
    }
    else {
	str = StringValuePtr(a);
 	PROTECT(xes->res = new XmlResults(con->con->queryWithXPath(txn, str, qcxt, flags)));
    }
    if (temporary) {
	PROTECT(txn->commit(0));
    }
    return res;
}

static VALUE
xb_con_search(int argc, VALUE *argv, VALUE obj)
{
    int nargc;
    VALUE *nargv;

    if (argc == 1 || (argc == 2 && FIXNUM_P(argv[1]))) {
	xcxt *cxt;
	VALUE res;

	res = Data_Make_Struct(xb_cCxt, xcxt, 0, (RDF)xb_cxt_free, cxt);
	cxt->cxt = new XmlQueryContext();
	cxt->evaltype = XmlQueryContext::Lazy;
	cxt->cxt->setEvaluationType(XmlQueryContext::Lazy);
	if (argc == 2) {
	    cxt->cxt->setReturnType(XmlQueryContext::ReturnType(NUM2INT(argv[1])));
	    cxt->returntype = NUM2INT(argv[1]);
	    nargv = argv;
	}
	else {
	    nargv = ALLOCA_N(VALUE, 2);
	    nargv[0] = argv[0];
	}
	nargc = 2;
	nargv[1] = res;
    }
    else {
	nargc = argc;
	nargv = argv;
    }
    return xb_res_search(xb_con_query(nargc, nargv, obj));
}

static VALUE
xb_con_each(VALUE obj)
{
    VALUE query = rb_str_new2("//*");
    if (!rb_block_given_p()) {
	rb_raise(rb_eArgError, "block not supplied");
    }
    return xb_con_search(1, &query, obj);
}

static VALUE
xb_int_delete(int argc, VALUE *argv, VALUE obj, XmlUpdateContext *upd)
{
    xcon *con;
    DbTxn *txn = NULL;
    xdoc *doc;
    VALUE a, b;
    int flags = 0;

    rb_secure(4);
    GetConTxn(obj, con, txn);
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	flags = NUM2INT(b);
    }
    if (TYPE(a) == T_DATA && RDATA(a)->dfree == (RDF)xb_doc_free) {
	Data_Get_Struct(a, xdoc, doc);
	PROTECT(con->con->deleteDocument(txn, *(doc->doc), upd, flags));
    }
    else {
	u_int32_t id = NUM2INT(a);
	PROTECT(con->con->deleteDocument(txn, id, upd, flags));
    }
    return obj;
}

static VALUE
xb_con_delete(int argc, VALUE *argv, VALUE obj)
{
    return xb_int_delete(argc, argv, obj, 0);
}

static VALUE
xb_con_i_alloc(VALUE obj, VALUE name)
{
    int argc;
    VALUE klass, argv[2];

    argc = 2;
    argv[0] = name;
    argv[1] = rb_hash_new();
    rb_hash_aset(argv[1], INT2NUM(rb_intern("__ts__no__init__")), Qtrue);
    klass = obj;
    if (rb_obj_is_kind_of(obj, xb_cTxn)) {
	argc = 2;
	klass = xb_cCon;
	rb_hash_aset(argv[1], rb_tainted_str_new2("txn"), obj);
    }
    return rb_funcall2(klass, rb_intern("new"), argc, argv);
}

static VALUE
xb_con_remove(int argc, VALUE *argv, VALUE obj)
{
    VALUE a, b, c;
    int flags = 0;
    xcon *con;
    DbTxn *txn = NULL;

    rb_secure(2);
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	flags = NUM2INT(b);
    }
    SafeStringValue(a);
    c = xb_con_i_alloc(obj, a);
    GetConTxn(c, con, txn);
    PROTECT(con->con->remove(txn, flags));
    return Qnil;
}

static VALUE
xb_con_rename(int argc, VALUE *argv, VALUE obj)
{
    VALUE a, b, c, d;
    int flags = 0;
    xcon *con;
    char *str;
    DbTxn *txn = NULL;

    rb_secure(2);
    if (rb_scan_args(argc, argv, "21", &a, &b, &c) == 3) {
	flags = NUM2INT(c);
    }
    SafeStringValue(a);
    d = xb_con_i_alloc(obj, a);
    SafeStringValue(b);
    str = StringValuePtr(b);
    GetConTxn(d, con, txn);
    PROTECT(con->con->rename(txn, str, flags));
    return Qnil;
}

static VALUE
xb_con_s_name(VALUE obj, VALUE a, VALUE b)
{
    VALUE c;
    xcon *con;
    DbTxn *txn;
    std::string str;

    SafeStringValue(a);
    c = xb_con_i_alloc(obj, a);
    GetConTxn(c, con, txn);
    str = StringValuePtr(b);
    PROTECT(con->con->setName(str));
    return Qnil;
}

static VALUE
xb_con_dump(int argc, VALUE *argv, VALUE obj)
{
    xcon *con;
    VALUE a, b, c;
    u_int32_t flags = 0;
    DbTxn *txn = NULL;

    if (rb_scan_args(argc, argv, "21", &a, &b, &c) == 3) {
	flags = NUM2INT(c);
    }
    c = xb_con_i_alloc(obj, a);
    GetConTxn(c, con, txn);
    Check_Type(b, T_STRING);
    char *name = StringValuePtr(b);
    std::ofstream out(name);
    PROTECT2(con->con->dump(&out, flags), out.close());
    out.close();
    return Qnil;
}

static VALUE
xb_con_load(int argc, VALUE *argv, VALUE obj)
{
    xcon *con;
    VALUE a, b, c, d;
    u_int32_t flags = 0;
    unsigned long lineno = 0;
    DbTxn *txn = NULL;

    switch (rb_scan_args(argc, argv, "22", &a, &b, &c, &d)) {
    case 4:
	flags = NUM2INT(d);
	/* ... */
    case 3:
	lineno = NUM2INT(c);
    }
    c = xb_con_i_alloc(obj, a);
    GetConTxn(c, con, txn);
    Check_Type(b, T_STRING);
    char *name = StringValuePtr(b);
    std::ifstream in(name);
    PROTECT2(con->con->load(&in, &lineno, flags), in.close());
    in.close();
    return Qnil;
}

static VALUE
xb_con_verify(VALUE obj, VALUE a)
{
    xcon *con;
    std::ofstream out;
    DbTxn *txn = NULL;

    a = xb_con_i_alloc(obj, a);
    GetConTxn(a, con, txn);
    PROTECT(con->con->verify(&out, 0));
    return Qnil;
}

static VALUE
xb_con_verify_salvage(int argc, VALUE *argv, VALUE obj)
{
    xcon *con;
    VALUE a, b, c;
    u_int32_t flags = 0;
    DbTxn *txn = NULL;

    if (rb_scan_args(argc, argv, "21", &a, &b, &c) == 2) {
	flags = NUM2INT(c);
    }
    c = xb_con_i_alloc(obj, a);
    GetConTxn(c, con, txn);
    flags |= DB_SALVAGE;
    Check_Type(b, T_STRING);
    char *name = StringValuePtr(b);
    std::ofstream out(name);
    PROTECT2(con->con->verify(&out, flags), out.close());
    out.close();
    return Qnil;
}

static VALUE
xb_con_is_open(VALUE obj)
{
    xcon *con;
    Data_Get_Struct(obj, xcon, con);
    if (con->closed || !con->con || con->con->isOpen()) {
	return Qtrue;
    }
    return Qfalse;
}

static void
xb_upd_free(xupd *upd)
{
    delete upd->upd;
    ::free(upd);
}

static void
xb_upd_mark(xupd *upd)
{
    rb_gc_mark(upd->con);
}

static VALUE
xb_con_context(VALUE obj)
{
    xcon *con;
    DbTxn *txn;
    xupd *upd;
    VALUE res;

    GetConTxn(obj, con, txn);
    res = Data_Make_Struct(xb_cUpd, xupd, xb_upd_mark, xb_upd_free, upd);
    upd->con = obj;
    upd->upd = new XmlUpdateContext(*con->con);
    if (rb_block_given_p()) {
	return rb_yield(res);
    }
    return res;
}

static VALUE
xb_upd_push(int argc, VALUE *argv, VALUE obj)
{
    xupd *upd;
    Data_Get_Struct(obj, xupd, upd);
    return xb_int_push(argc, argv, upd->con, upd->upd);
}

static VALUE
xb_upd_add(VALUE obj, VALUE a)
{
    xb_upd_push(1, &a, obj);
    return obj;
}

static VALUE
xb_upd_delete(int argc, VALUE *argv, VALUE obj)
{
    xupd *upd;
    Data_Get_Struct(obj, xupd, upd);
    return xb_int_delete(argc, argv, upd->con, upd->upd);
}

static VALUE
xb_upd_update(int argc, VALUE *argv, VALUE obj)
{
    xupd *upd;
    Data_Get_Struct(obj, xupd, upd);
    return xb_int_update(argc, argv, upd->con, upd->upd);
}

static VALUE
xb_int_set(VALUE obj, VALUE a, VALUE b, XmlUpdateContext *upd)
{
    VALUE c = xb_con_get(1, &a, obj);
    return xb_int_update(1, &c, obj, upd);
}

static VALUE
xb_con_set(VALUE obj, VALUE a, VALUE b)
{
    return xb_int_set(obj, a, b, 0);
}

static VALUE
xb_upd_set(VALUE obj, VALUE a, VALUE b)
{
    xupd *upd;
    Data_Get_Struct(obj, xupd, upd);
    return xb_int_set(upd->con, a, b, upd->upd);
}

static VALUE
xb_i_txn(VALUE obj, int *flags)
{
    VALUE key, value;
    char *options;

    key = rb_ary_entry(obj, 0);
    value = rb_ary_entry(obj, 1);
    key = rb_obj_as_string(key);
    options = StringValuePtr(key);
    if (strcmp(options, "flags") == 0) {
	*flags = NUM2INT(value);
    }
    return Qnil;
}

static VALUE
xb_env_s_new(int argc, VALUE *argv, VALUE obj)
{
    DbEnv *env = new DbEnv(0);
    DB_ENV *envd = env->get_DB_ENV();
    envd->app_private = static_cast<void *>(env);
    return bdb_env_s_rslbl(argc, argv, obj, envd);
}

static VALUE
xb_env_begin(int argc, VALUE *argv, VALUE obj)
{
    DbTxn *p = NULL;
    struct txn_rslbl txnr;
    DbTxn *q = NULL;
    VALUE env;
    bdb_ENV *envst;
    DbEnv *env_cxx;
    int flags = 0;

    if (argc) {
	if (TYPE(argv[argc - 1]) == T_HASH) {
	    rb_iterate(RMFS(rb_each), argv[argc - 1], RMF(xb_i_txn), (VALUE)&flags);
	}
	if (FIXNUM_P(argv[0])) {
	    flags = NUM2INT(argv[0]);
	}
	flags &= ~BDB_TXN_COMMIT;
    }
    if (rb_obj_is_kind_of(obj, xb_cTxn)) {
	bdb_TXN *txnst;
	GetTxnDBErr(obj, txnst, xb_eFatal);
	p = static_cast<DbTxn *>(txnst->txn_cxx);
	env = txnst->env;
    }
    else {
	env = obj;
    }
    GetEnvDBErr(env, envst, id_current_env, xb_eFatal);
    env_cxx = static_cast<DbEnv *>(envst->envp->app_private);
    PROTECT(env_cxx->txn_begin(p, &q, flags));
    txnr.txn_cxx = static_cast<void *>(q);
    txnr.txn = q->get_DB_TXN();
    return bdb_env_rslbl_begin((VALUE)&txnr, argc, argv, obj);
}

extern "C" {
  
    static VALUE
    xb_con_txn_dup(VALUE obj, VALUE a)
    {
	xcon *con, *con1;
	VALUE res;
	bdb_TXN *txnst;
	int argc, i;
	VALUE *argv;

	GetTxnDBErr(a, txnst, xb_eFatal);
	Data_Get_Struct(obj, xcon, con);
	res = xb_env_open_xml(1, &con->name, a);
	return res;
    }

    static VALUE
    xb_con_txn_close(VALUE obj, VALUE commit, VALUE real)
    {
	if (!real) {
	    xcon *con;

	    Data_Get_Struct(obj, xcon, con);
	    con->closed = Qtrue;
	}
	else {
	    xb_con_close(0, 0, obj);
	}
	return Qnil;
    }

    static void
    xb_finalize(VALUE ary)
    {
	VALUE con;
	struct xb_eiv *civ;

	while ((con = rb_ary_pop(xb_internal_ary)) != Qnil) {
	    Data_Get_Struct(con, struct xb_eiv, civ);
	    rb_funcall2(civ->con->ori_val, rb_intern("close"), 0, 0);
	}
    }

    static void xb_const_set(VALUE hash, const char *cvalue, char *ckey)
    {
	VALUE key, value;

	key = rb_str_new2(ckey);
	rb_obj_freeze(key);
	value = rb_str_new2(cvalue);
	rb_obj_freeze(value);
	rb_hash_aset(hash, key, value);
    }

    void Init_bdbxml()
    {
	int major, minor, patch;
	VALUE version;

	static VALUE xb_mDb,  xb_mXML;
#if defined(DBXML_DOM_XERCES2)
	static VALUE xb_mDom;
#endif

	if (rb_const_defined_at(rb_cObject, rb_intern("BDB"))) {
	    rb_raise(rb_eNameError, "module already defined");
	}
	rb_require("bdb");
	id_current_env = rb_intern("bdb_current_env");
	xb_internal_ary = rb_ary_new();
	rb_set_end_proc(RMFF(xb_finalize), xb_internal_ary);
	xb_mDb = rb_const_get(rb_cObject, rb_intern("BDB"));
	major = NUM2INT(rb_const_get(xb_mDb, rb_intern("VERSION_MAJOR")));
	minor = NUM2INT(rb_const_get(xb_mDb, rb_intern("VERSION_MINOR")));
	patch = NUM2INT(rb_const_get(xb_mDb, rb_intern("VERSION_PATCH")));
	if (major != DB_VERSION_MAJOR || minor != DB_VERSION_MINOR
	    || patch != DB_VERSION_PATCH) {
	    rb_raise(rb_eNotImpError, "\nBDB::XML needs compatible versions of BDB\n\tyou have BDB::XML version %d.%d.%d and BDB version %d.%d.%d\n",
		     DB_VERSION_MAJOR, DB_VERSION_MINOR, DB_VERSION_PATCH,
		     major, minor, patch);
	}
	version = rb_tainted_str_new2(dbxml_version(&major, &minor, &patch));
	if (major != DBXML_VERSION_MAJOR || minor != DBXML_VERSION_MINOR
	    || patch != DBXML_VERSION_PATCH) {
	    rb_raise(rb_eNotImpError, "\nBDB::XML needs compatible versions of DbXml\n\tyou have DbXml.hpp version %d.%d.%d and libdbxml version %d.%d.%d\n",
		     DB_VERSION_MAJOR, DB_VERSION_MINOR, DB_VERSION_PATCH,
		     major, minor, patch);
	}

	xb_eFatal = rb_const_get(xb_mDb, rb_intern("Fatal"));
	xb_cEnv = rb_const_get(xb_mDb, rb_intern("Env"));
	rb_define_singleton_method(xb_cEnv, "new", RMF(xb_env_s_new), -1);
	rb_define_singleton_method(xb_cEnv, "create", RMF(xb_env_s_new), -1);
	rb_define_method(xb_cEnv, "open_xml", RMF(xb_env_open_xml), -1);
	rb_define_method(xb_cEnv, "begin", RMF(xb_env_begin), -1);
	rb_define_method(xb_cEnv, "txn_begin", RMF(xb_env_begin), -1);
	rb_define_method(xb_cEnv, "transaction", RMF(xb_env_begin), -1);
	DbXml::setLogLevel(DbXml::LEVEL_ALL, false);
	xb_cTxn = rb_const_get(xb_mDb, rb_intern("Txn"));
	rb_define_method(xb_cTxn, "open_xml", RMF(xb_env_open_xml), -1);
	rb_define_method(xb_cTxn, "begin", RMF(xb_env_begin), -1);
	rb_define_method(xb_cTxn, "txn_begin", RMF(xb_env_begin), -1);
	rb_define_method(xb_cTxn, "transaction", RMF(xb_env_begin), -1);
	rb_define_method(xb_cTxn, "rename_xml", RMF(xb_con_rename), -1);
	rb_define_method(xb_cTxn, "remove_xml", RMF(xb_con_remove), -1);
	xb_mXML = rb_define_module_under(xb_mDb, "XML");
	rb_define_const(xb_mXML, "VERSION", version);
	rb_define_const(xb_mXML, "VERSION_MAJOR", INT2FIX(major));
	rb_define_const(xb_mXML, "VERSION_MINOR", INT2FIX(minor));
	rb_define_const(xb_mXML, "VERSION_PATCH", INT2FIX(patch));
	{
	    VALUE name = rb_hash_new();
	    rb_define_const(xb_mXML, "Name", name);
	    xb_const_set(name, metaDataName_name, "name");
	    xb_const_set(name, metaDataName_default, "default");
	    xb_const_set(name, metaDataName_content, "content");
	    xb_const_set(name, metaDataName_id, "id");
	    xb_const_set(name, metaDataName_uri_id, "uri_id");
	    xb_const_set(name, metaDataName_uri_default, "uri_default");
	    rb_obj_freeze(name);
	    name = rb_hash_new();
	    rb_define_const(xb_mXML, "Namespace", name);
	    xb_const_set(name, metaDataNamespace_uri, "uri");
	    xb_const_set(name, metaDataNamespace_prefix, "prefix");
	    xb_const_set(name, metaDataNamespace_prefix_debug, "prefix_debug");
	    rb_obj_freeze(name);
	}
	xb_cCon = rb_define_class_under(xb_mXML, "Container", rb_cObject);
	rb_include_module(xb_cCon, rb_mEnumerable);
#if RUBY_VERSION_CODE >= 180
	rb_define_alloc_func(xb_cCon, RMFS(xb_con_s_alloc));
#else
	rb_define_singleton_method(xb_cCon, "allocate", RMFS(xb_con_s_alloc), 0);
#endif
	rb_define_singleton_method(xb_cCon, "new", RMF(xb_con_s_new), -1);
	rb_define_singleton_method(xb_cCon, "open", RMF(xb_con_s_new), -1);
	rb_define_singleton_method(xb_cCon, "set_name", RMF(xb_con_s_name), 2);
	rb_define_singleton_method(xb_cCon, "rename", RMF(xb_con_rename), -1);
	rb_define_singleton_method(xb_cCon, "remove", RMF(xb_con_remove), -1);
	rb_define_singleton_method(xb_cCon, "dump", RMF(xb_con_dump), -1);
	rb_define_singleton_method(xb_cCon, "load", RMF(xb_con_load), -1);
	rb_define_singleton_method(xb_cCon, "verify", RMF(xb_con_verify), 1);
	rb_define_singleton_method(xb_cCon, "salvage", RMF(xb_con_verify_salvage), -1);
	rb_define_singleton_method(xb_cCon, "verify_and_salvage", RMF(xb_con_verify_salvage), -1);
	rb_define_private_method(xb_cCon, "initialize", RMF(xb_con_init), -1);
	rb_define_private_method(xb_cCon, "__txn_dup__", RMF(xb_con_txn_dup), 1);
	rb_define_private_method(xb_cCon, "__txn_close__", RMF(xb_con_txn_close), 2);
	rb_define_method(xb_cCon, "environment", RMF(xb_con_env), 0);
	rb_define_method(xb_cCon, "env", RMF(xb_con_env), 0);
	rb_define_method(xb_cCon, "environment?", RMF(xb_con_env_p), 0);
	rb_define_method(xb_cCon, "env?", RMF(xb_con_env_p), 0);
	rb_define_method(xb_cCon, "has_environment?", RMF(xb_con_env_p), 0);
	rb_define_method(xb_cCon, "has_env?", RMF(xb_con_env_p), 0);
	rb_define_method(xb_cCon, "transaction", RMF(xb_con_txn), 0);
	rb_define_method(xb_cCon, "txn", RMF(xb_con_txn), 0);
	rb_define_method(xb_cCon, "transaction?", RMF(xb_con_txn_p), 0);
	rb_define_method(xb_cCon, "txn?", RMF(xb_con_txn_p), 0);
	rb_define_method(xb_cCon, "in_transaction?", RMF(xb_con_txn_p), 0);
	rb_define_method(xb_cCon, "in_txn?", RMF(xb_con_txn_p), 0);
	rb_define_method(xb_cCon, "index", RMF(xb_con_index), 0);
	rb_define_method(xb_cCon, "index=", RMF(xb_con_index_set), 1);
	rb_define_method(xb_cCon, "name", RMF(xb_con_name), 0);
	rb_define_method(xb_cCon, "name=", RMF(xb_con_name_set), 1);
	rb_define_method(xb_cCon, "[]", RMF(xb_con_get), -1);
	rb_define_method(xb_cCon, "get", RMF(xb_con_get), -1);
	rb_define_method(xb_cCon, "push", RMF(xb_con_push), -1);
	rb_define_method(xb_cCon, "<<", RMF(xb_con_add), 1);
	rb_define_method(xb_cCon, "update", RMF(xb_con_update), -1);
	rb_define_method(xb_cCon, "[]=", RMF(xb_con_set), 2);
	rb_define_method(xb_cCon, "parse", RMF(xb_con_parse), -1);
	rb_define_method(xb_cCon, "query", RMF(xb_con_query), -1);
	rb_define_method(xb_cCon, "delete", RMF(xb_con_delete), -1);
	rb_define_method(xb_cCon, "close", RMF(xb_con_close), -1);
	rb_define_method(xb_cCon, "search", RMF(xb_con_search), -1);
	rb_define_method(xb_cCon, "each", RMF(xb_con_each), 0);
	rb_define_method(xb_cCon, "open?", RMF(xb_con_is_open), 0);
	rb_define_method(xb_cCon, "is_open?", RMF(xb_con_is_open), 0);
	rb_define_method(xb_cCon, "context", RMF(xb_con_context), 0);
	rb_define_method(xb_cCon, "update_context", RMF(xb_con_context), 0);
	xb_cInd = rb_define_class_under(xb_mXML, "Index", rb_cObject);
#if RUBY_VERSION_CODE >= 180
	rb_define_alloc_func(xb_cInd, RMFS(xb_ind_s_alloc));
#else
	rb_define_singleton_method(xb_cInd, "allocate", RMFS(xb_ind_s_alloc), 0);
#endif
	rb_define_singleton_method(xb_cInd, "new", RMF(xb_s_new), -1);
	rb_define_private_method(xb_cInd, "initialize", RMF(xb_ind_init), -1);
	rb_define_method(xb_cInd, "add", RMF(xb_ind_add), -1);
	rb_define_method(xb_cInd, "delete", RMF(xb_ind_delete), -1);
	rb_define_method(xb_cInd, "replace", RMF(xb_ind_replace), -1);
	rb_define_method(xb_cInd, "each", RMF(xb_ind_each), 0);
	rb_define_method(xb_cInd, "find", RMF(xb_ind_find), -1);
	rb_define_method(xb_cInd, "to_a", RMF(xb_ind_to_a), 0);
	xb_cUpd = rb_define_class_under(xb_mXML, "UpdateContext", rb_cObject);
	rb_undef_method(CLASS_OF(xb_cUpd), "allocate");
	rb_undef_method(CLASS_OF(xb_cUpd), "new");
	rb_define_method(xb_cUpd, "[]", RMF(xb_con_get), -1);
	rb_define_method(xb_cUpd, "get", RMF(xb_con_get), -1);
	rb_define_method(xb_cUpd, "[]=", RMF(xb_upd_set), 2);
	rb_define_method(xb_cUpd, "push", RMF(xb_upd_push), -1);
	rb_define_method(xb_cUpd, "<<", RMF(xb_upd_add), 1);
	rb_define_method(xb_cUpd, "delete", RMF(xb_upd_delete), -1);
	rb_define_method(xb_cUpd, "update", RMF(xb_upd_update), -1);
	xb_cTmp = rb_define_class_under(xb_mXML, "Tmp", rb_cData);
	rb_undef_method(CLASS_OF(xb_cTmp), "allocate");
	rb_undef_method(CLASS_OF(xb_cTmp), "new");
	rb_define_method(xb_cTmp, "[]", RMF(xb_cxt_name_get), 1);
	rb_define_method(xb_cTmp, "[]=", RMF(xb_cxt_name_set), 2);
	xb_cDoc = rb_define_class_under(xb_mXML, "Document", rb_cObject);
#if RUBY_VERSION_CODE >= 180
	rb_define_alloc_func(xb_cDoc, RMFS(xb_doc_s_alloc));
#else
	rb_define_singleton_method(xb_cDoc, "allocate", RMFS(xb_doc_s_alloc), 0);
#endif
	rb_define_singleton_method(xb_cDoc, "new", RMF(xb_s_new), -1);
	rb_define_private_method(xb_cDoc, "initialize", RMF(xb_doc_init), -1);
	rb_define_method(xb_cDoc, "name", RMF(xb_doc_name_get), 0);
	rb_define_method(xb_cDoc, "name=", RMF(xb_doc_name_set), 1);
	rb_define_method(xb_cDoc, "content", RMF(xb_doc_content_get), 0);
	rb_define_method(xb_cDoc, "content=", RMF(xb_doc_content_set), 1);
	rb_define_method(xb_cDoc, "[]", RMF(xb_doc_get), -1);
	rb_define_method(xb_cDoc, "[]=", RMF(xb_doc_set), -1);
	rb_define_method(xb_cDoc, "get", RMF(xb_doc_get), -1);
	rb_define_method(xb_cDoc, "set", RMF(xb_doc_set), -1);
	rb_define_method(xb_cDoc, "get_metadata", RMF(xb_doc_get), -1);
	rb_define_method(xb_cDoc, "set_metadata", RMF(xb_doc_set), -1);
	rb_define_method(xb_cDoc, "id", RMF(xb_doc_id_get), 0);
	rb_define_method(xb_cDoc, "to_s", RMF(xb_doc_content_get), 0);
	rb_define_method(xb_cDoc, "to_str", RMF(xb_doc_content_get), 0);
	rb_define_method(xb_cDoc, "uri", RMF(xb_doc_uri_get), 0);
	rb_define_method(xb_cDoc, "uri=", RMF(xb_doc_uri_set), 1);
	rb_define_method(xb_cDoc, "prefix", RMF(xb_doc_prefix_get), 0);
	rb_define_method(xb_cDoc, "prefix=", RMF(xb_doc_prefix_set), 1);
	rb_define_method(xb_cDoc, "query", RMF(xb_doc_query), -1);
	xb_cCxt = rb_define_class_under(xb_mXML, "Context", rb_cObject);
	rb_const_set(xb_mXML, rb_intern("QueryContext"), xb_cCxt);
	rb_define_const(xb_cCxt, "Documents", INT2FIX(XmlQueryContext::ResultDocuments));
	rb_define_const(xb_cCxt, "Values", INT2FIX(XmlQueryContext::ResultValues));
	rb_define_const(xb_cCxt, "Candidates", INT2FIX(XmlQueryContext::CandidateDocuments));
	rb_define_const(xb_cCxt, "Eager", INT2FIX(XmlQueryContext::Eager));
	rb_define_const(xb_cCxt, "Lazy", INT2FIX(XmlQueryContext::Lazy));
#if RUBY_VERSION_CODE >= 180
	rb_define_alloc_func(xb_cCxt, RMFS(xb_cxt_s_alloc));
#else
	rb_define_singleton_method(xb_cCxt, "allocate", RMFS(xb_cxt_s_alloc), 0);
#endif
	rb_define_singleton_method(xb_cCxt, "new", RMF(xb_s_new), -1);
	rb_define_private_method(xb_cCxt, "initialize", RMF(xb_cxt_init), -1);
	rb_define_method(xb_cCxt, "set_namespace", RMF(xb_cxt_name_set), 2);
	rb_define_method(xb_cCxt, "get_namespace", RMF(xb_cxt_name_get), 1);
	rb_define_method(xb_cCxt, "namespace", RMF(xb_cxt_name), 0);
	rb_define_method(xb_cCxt, "del_namespace", RMF(xb_cxt_name_del), 1);
	rb_define_method(xb_cCxt, "clear", RMF(xb_cxt_clear), 0);
	rb_define_method(xb_cCxt, "clear_namespaces", RMF(xb_cxt_clear), 0);
	rb_define_method(xb_cCxt, "[]", RMF(xb_cxt_get), 1);
	rb_define_method(xb_cCxt, "[]=", RMF(xb_cxt_set), 2);
	rb_define_method(xb_cCxt, "returntype", RMF(xb_cxt_return_get), 0);
	rb_define_method(xb_cCxt, "returntype=", RMF(xb_cxt_return_set), 1);
	rb_define_method(xb_cCxt, "evaltype", RMF(xb_cxt_eval_get), 0);
	rb_define_method(xb_cCxt, "evaltype=", RMF(xb_cxt_eval_set), 1);
	xb_cPat = rb_define_class_under(xb_mXML, "XPath", rb_cObject);
	rb_undef_method(CLASS_OF(xb_cPat), "allocate");
	rb_undef_method(CLASS_OF(xb_cPat), "new");
	rb_define_method(xb_cPat, "to_s", RMF(xb_pat_to_str), 0);
	rb_define_method(xb_cPat, "to_str", RMF(xb_pat_to_str), 0);
	xb_cRes = rb_define_class_under(xb_mXML, "Results", rb_cObject);
	rb_include_module(xb_cRes, rb_mEnumerable);
	rb_undef_method(CLASS_OF(xb_cRes), "allocate");
	rb_undef_method(CLASS_OF(xb_cRes), "new");
	rb_define_method(xb_cRes, "each", RMF(xb_res_each), 0);
#if defined(DBXML_DOM_XERCES2)
	xb_mDom = rb_define_class_under(xb_mDb, "DOM", rb_cObject);
	xb_cNod = rb_define_class_under(xb_mDom, "Node", rb_cObject);
#if RUBY_VERSION_CODE >= 180
	rb_undef_method(CLASS_OF(xb_cNod), "allocate");
#endif
	rb_undef_method(CLASS_OF(xb_cNod), "new");
	rb_define_method(xb_cNod, "to_s", RMF(xb_nod_to_s), 0);
	rb_define_method(xb_cNod, "to_str", RMF(xb_nod_to_s), 0);
	rb_define_method(xb_cNod, "inspect", RMF(xb_nod_inspect), 0);
	xb_cNol = rb_define_class_under(xb_mDom, "NodeList", rb_cObject);
	rb_const_set(xb_mDom, rb_intern("List"), xb_cNol);
	rb_include_module(xb_cNol, rb_mEnumerable);
#if RUBY_VERSION_CODE >= 180
	rb_undef_method(CLASS_OF(xb_cNol), "allocate");
#endif
	rb_undef_method(CLASS_OF(xb_cNol), "new");
	rb_define_method(xb_cNol, "size", RMF(xb_nol_size), 0);
	rb_define_method(xb_cNol, "length", RMF(xb_nol_size), 0);
	rb_define_method(xb_cNol, "[]", RMF(xb_nol_get), 1);
	rb_define_method(xb_cNol, "each", RMF(xb_nol_each), 0);
	rb_define_method(xb_cNol, "to_s", RMF(xb_nol_to_str), 0);
	rb_define_method(xb_cNol, "to_str", RMF(xb_nol_to_str), 0);
	rb_define_method(xb_cNol, "inspect", RMF(xb_nol_inspect), 0);
	rb_define_method(xb_cNol, "to_a", RMF(xb_nol_to_ary), 0);
	rb_define_method(xb_cNol, "to_ary", RMF(xb_nol_to_ary), 0);
#endif
    }
}
    
