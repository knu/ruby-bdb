#include "bdb.h"

#include <fstream>
#include <dbxml/DbXml.hpp>

using namespace DbXml;

#define RMF(func) RUBY_METHOD_FUNC(func)
#define RDF RUBY_DATA_FUNC

#if RUBY_VERSION_CODE >= 180
#define RMFS(func) ((VALUE (*)(VALUE))func)
#define RMFF(func) ((void (*)(VALUE))func)
#else
#define RMFS(func) RMF(func)
#define RMFF(func) ((void (*)())func)
#endif

#define BDBXML_VERSION (10000*DBXML_VERSION_MAJOR+100*DBXML_VERSION_MINOR+DBXML_VERSION_PATCH)

#define PROTECT2(comm_, libr_)				\
  try {							\
    comm_;						\
  }							\
  catch (XmlException &e) {				\
    VALUE xb_err = Qnil;				\
    libr_;						\
    if ((xb_err = bdb_return_err()) != Qnil) {          \
      rb_raise(xb_eFatal, StringValuePtr(xb_err));      \
    }                                                   \
    rb_raise(xb_eFatal, e.what());			\
  }							\
  catch (std::exception &e) {				\
    libr_;						\
    rb_raise(xb_eFatal, e.what());			\
  }							\
  catch (...) {						\
    libr_;						\
    rb_raise(xb_eFatal, "Unknown error");		\
  }

#define PROTECT(comm_) PROTECT2(comm_,)

#define GetConTxn(obj_, con_, txn_)			\
  {							\
    Data_Get_Struct(obj_, xcon, con_);			\
    if (con_->closed || !con_->con) {			\
	rb_raise(xb_eFatal, "closed container");	\
    }							\
    txn_ = 0;						\
    if (con_->txn_val) {				\
      bdb_TXN *txnst;					\
      Data_Get_Struct(con_->txn_val, bdb_TXN, txnst);	\
      txn_ = (DbTxn *)txnst->txn_cxx;			\
    }							\
  }
      
typedef struct {
  XmlContainer *con;
  VALUE env_val;
  VALUE txn_val;
  VALUE ori_val;
  VALUE name;
  int closed, flag;
} xcon;

typedef struct {
  XmlResults *res;
  VALUE txn_val;
  VALUE cxt_val;
} xres;

#if defined(DBXML_DOM_XERCES2)

extern "C" void Init_bdbxml_dom();

typedef struct {
#if BDBXML_VERSION < 10101
  XERCES_CPP_NAMESPACE_QUALIFIER DOMNodeList *nol;
#else
  XERCES_CPP_NAMESPACE_QUALIFIER DOMNode *nol;
#endif
  VALUE cxt_val;
} xnol;

#endif

typedef struct {
    XmlDocument *doc;
    VALUE uri, prefix;
} xdoc;

typedef struct {
    XmlQueryContext *cxt;
} xcxt;

typedef struct {
    VALUE con;
    XmlUpdateContext *upd;
} xupd;

