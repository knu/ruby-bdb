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
} xman;
      
typedef struct {
    XmlContainer *con;
    VALUE ind;
    VALUE txn;
    VALUE man;
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

static VALUE xb_eFatal, xb_cTxn;

static void xb_man_free(xman *);

static inline xman *
get_man(VALUE obj)
{
    xman *man;
    if (TYPE(obj) != T_DATA || RDATA(obj)->dfree != (RDF)xb_man_free) {
        rb_raise(xb_eFatal, "invalid Manager objects");
    }
    Data_Get_Struct(obj, xman, man);
    if (!man->man) {
        rb_raise(rb_eArgError, "invalid Manager object");
    }
    return man;
}

static inline xres *
get_res(VALUE obj)
{
    xres *res;

    Data_Get_Struct(obj, xres, res);
    if (!res->res) {
        rb_raise(rb_eArgError, "invalid Results");
    }
    xman *man = get_man(res->man);
    return res;
}

static void xb_doc_mark(xdoc *);

static inline xdoc *
get_doc(VALUE obj)
{
    xdoc *doc;
    if (TYPE(obj) != T_DATA || RDATA(obj)->dmark != (RDF)xb_doc_mark) {
        rb_raise(rb_eArgError, "invalid document");
    }
    Data_Get_Struct(obj, xdoc, doc);
    if (!doc->doc) {
        rb_raise(rb_eArgError, "invalid document");
    }
    return doc;
}  

static inline XmlTransaction *
get_txn(VALUE obj)
{
    if (rb_obj_is_kind_of(obj, xb_cTxn)) {
	bdb_TXN *txnst;

	GetTxnDBErr(obj, txnst, xb_eFatal);
        return (XmlTransaction *)txnst->txn_cxx;
    }
    return 0;
}

static inline xman *
get_man_txn(VALUE obj)
{
    if (rb_obj_is_kind_of(obj, xb_cTxn)) {
	bdb_TXN *txnst;

	GetTxnDBErr(obj, txnst, xb_eFatal);
        return get_man(txnst->man);
    }
    else {
        return get_man(obj);
    }
}

static void xb_upd_free(xupd *);

static inline xupd *
get_upd(VALUE obj)
{
    xupd *upd;
    if (TYPE(obj) != T_DATA || RDATA(obj)->dfree != (RDF)xb_upd_free) {
        rb_raise(rb_eArgError, "expected an Update Context");
    }
    Data_Get_Struct(obj, xupd, upd);
    if (!upd->upd) {
        rb_raise(rb_eArgError, "invalid Update Context");
    }
    xman *man = get_man(upd->man);
    return upd;
}

static inline VALUE
get_txn_man(VALUE obj)
{
    if (rb_obj_is_kind_of(obj, xb_cTxn)) {
        bdb_TXN *txnst;

        GetTxnDBErr(obj, txnst, xb_eFatal);
        return txnst->man;
    }
    return 0;
}

static void xb_cxt_free(xcxt *);

static inline xcxt *
get_cxt(VALUE obj)
{
    xcxt *cxt;
    if (TYPE(obj) != T_DATA || RDATA(obj)->dfree != (RDF)xb_cxt_free) {
        rb_raise(rb_eArgError, "expected a Query Context");
    }
    Data_Get_Struct(obj, xcxt, cxt);
    if (!cxt->cxt) {
        rb_raise(rb_eArgError, "invalid QueryContext");
    }
    xman *man = get_man(cxt->man);
    return cxt;
}

static void xb_con_mark(xcon *);

static inline xcon *
get_con(VALUE obj)
{
    xcon *con;

    if (TYPE(obj) != T_DATA || RDATA(obj)->dmark != (RDF)xb_con_mark) {
        rb_raise(rb_eArgError, "invalid Container");
    }
    Data_Get_Struct(obj, xcon, con);
    if (!con->opened) {
        rb_raise(rb_eArgError, "closed container");
    }
    if (!con->con || !con->man) {
        rb_raise(rb_eArgError, "invalid Container");
    }
    xman *man = get_man(con->man);
    return con;
} 

static inline XmlTransaction *
get_con_txn(xcon *con)
{
    if (RTEST(con->txn)) {
        bdb_TXN *txnst;

	GetTxnDBErr(con->txn, txnst, xb_eFatal);
        return (XmlTransaction *)txnst->txn_cxx;
    }
    return 0;
}

static void xb_ind_free(xind *);

static inline xind *
get_ind(VALUE obj)
{
    if (TYPE(obj) != T_DATA || RDATA(obj)->dfree != (RDF)xb_ind_free) {
        rb_raise(rb_eArgError, "expected an IndexSpecification object");
    }
    xind *ind;
    Data_Get_Struct(obj, xind, ind);
    if (!ind->ind) {
        rb_raise(rb_eArgError, "expected an IndexSpecification object");
    }
    return ind;
}

static void xb_que_mark(xque *);

static inline xque *
get_que(VALUE obj)
{
    xque *que;

    if (TYPE(obj) != T_DATA || RDATA(obj)->dmark != (RDF)xb_que_mark) {
        rb_raise(rb_eArgError, "expected a Query Expression");
    }
    Data_Get_Struct(obj, xque, que);
    if (!que->que) {
        rb_raise(rb_eArgError, "invalid QueryExpression");
    }
    xman *man = get_man(que->man);
    return que;
}

static inline xmod *
get_mod(VALUE obj)
{
    xmod *mod;
    Data_Get_Struct(obj, xmod, mod);
    if (!mod->mod) {
        rb_raise(rb_eArgError, "invalid Modify");
    }
    xman *man = get_man(mod->man);
    return mod;
}

static void xb_res_free(xres *res);
static void xb_res_mark(xres *res);

