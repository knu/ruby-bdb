#include <ruby.h>
#include "bdb.h"

#include <fstream>
#include "DbXml.hpp"

using namespace DbXml;

#define RMF(func) RUBY_METHOD_FUNC(func)
#define RDF RUBY_DATA_FUNC

#define PROTECT2(comm, libr)                            \
  try {                                                 \
    comm;                                               \
  }                                                     \
  catch (XmlException &e) {                             \
    VALUE xb_err = Qnil;                                \
    libr;                                               \
    if ((xb_err = bdb_return_err()) != Qnil) {          \
      rb_raise(xb_eFatal, RSTRING(xb_err)->ptr);        \
    }                                                   \
    rb_raise(xb_eFatal, e.what());                      \
  }

#define PROTECT(comm) PROTECT2(comm,)

#define GetConTxn(obj, con, txn)			\
  {							\
    Data_Get_Struct(obj, xcon, con);			\
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

typedef struct {
  DOM_NodeList *nol;
  VALUE cxt_val;
} xnol;

typedef struct {
  XmlDocument *doc;
  VALUE uri, prefix;
} xdoc;

