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

#define PROTECT2(comm_, libr_)                          \
  try {                                                 \
    comm_;                                              \
    libr_;                                              \
  }                                                     \
  catch (XmlException &e) {                             \
    VALUE xb_err = Qnil;                                \
    libr_;                                              \
    if ((xb_err = bdb_return_err()) != Qnil) {          \
      rb_raise(xb_eFatal, StringValuePtr(xb_err));      \
    }                                                   \
    rb_raise(xb_eFatal, e.what());                      \
  }                                                     \
  catch (DbException &e) {                              \
    VALUE xb_err = Qnil;                                \
    libr_;                                              \
    if ((xb_err = bdb_return_err()) != Qnil) {          \
      rb_raise(xb_eFatal, StringValuePtr(xb_err));      \
    }                                                   \
    rb_raise(xb_eFatal, e.what());                      \
  }                                                     \
  catch (std::exception &e) {                           \
    libr_;                                              \
    rb_raise(xb_eFatal, e.what());                      \
  }                                                     \
  catch (...) {                                         \
    libr_;                                              \
    rb_raise(xb_eFatal, "Unknown error");               \
  }

#define PROTECT(comm_) PROTECT2(comm_,)

typedef struct {
    XmlManager *man;
    VALUE env;
    VALUE rsv;
    VALUE ori;
    struct ary_st db_ary;
    struct ary_st db_res;
} xman;
      
typedef struct {
    XmlContainer *con;
    VALUE ind;
    VALUE txn;
    VALUE man;
    VALUE ori;
    int opened;
} xcon;

typedef struct {
    XmlIndexSpecification *ind;
    VALUE con;
} xind;

typedef struct {
  XmlResults *res;
  VALUE man;
} xres;

typedef struct {
    XmlDocument *doc;
    VALUE man;
} xdoc;

typedef struct {
    XmlUpdateContext *upd;
    VALUE man;
} xupd;

typedef struct {
    XmlModify *mod;
    VALUE txn;
    VALUE man;
} xmod;

typedef struct {
    XmlQueryContext *cxt;
    VALUE man;
} xcxt;

typedef struct {
    XmlQueryExpression *que;
    VALUE txn;
    VALUE man;
} xque;

typedef struct {
    XmlValue *val;
    VALUE man;
} xval;
