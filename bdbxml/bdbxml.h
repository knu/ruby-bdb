#include "bdb.h"

#include <fstream>
#include "DbXml.hpp"

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

#define PROTECT2(comm, libr)				\
  try {							\
    comm;						\
  }							\
  catch (XmlException &e) {				\
    VALUE xb_err = Qnil;				\
    libr;						\
    if ((xb_err = bdb_return_err()) != Qnil) {          \
      rb_raise(xb_eFatal, StringValuePtr(xb_err));      \
    }                                                   \
    rb_raise(xb_eFatal, e.what());			\
  }							\
  catch (std::exception &e) {				\
    libr;						\
    rb_raise(xb_eFatal, e.what());			\
  }							\
  catch (...) {						\
    libr;						\
    rb_raise(xb_eFatal, "Unknown error");		\
  }

#define PROTECT(comm) PROTECT2(comm,)

#define GetConTxn(obj, con, txn)			\
  {							\
    Data_Get_Struct(obj, xcon, con);			\
    if (con->closed || !con->con) {			\
	rb_raise(xb_eFatal, "closed container");	\
    }							\
    txn = 0;						\
    if (con->txn_val) {					\
      bdb_TXN *txnst;					\
      Data_Get_Struct(con->txn_val, bdb_TXN, txnst);	\
      txn = (DbTxn *)txnst->txn_cxx;			\
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
  XERCES_CPP_NAMESPACE_QUALIFIER DOMNodeList *nol;
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
