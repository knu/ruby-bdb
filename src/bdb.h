#include <ruby.h>
#include <version.h>
#include <rubysig.h>
#include <rubyio.h>

#include <db.h>
#include <errno.h>

#ifndef StringValue
#define StringValue(x) do { 				\
    if (TYPE(x) != T_STRING) x = rb_str_to_str(x); 	\
} while (0)
#endif

#ifndef StringValuePtr
#define StringValuePtr(x) STR2CSTR(x)
#endif

#ifndef SafeStringValue
#define SafeStringValue(x) Check_SafeStr(x)
#endif

#ifndef RSTRING_PTR
# define RSTRING_PTR(x_) RSTRING(x_)->ptr
# define RSTRING_LEN(x_) RSTRING(x_)->len
#endif

#ifndef RARRAY_PTR
# define RARRAY_PTR(x_) RARRAY(x_)->ptr
# define RARRAY_LEN(x_) RARRAY(x_)->len
#endif

#ifdef close
#undef close
#endif

#ifdef stat
#undef stat
#endif

#ifdef rename
#undef rename
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef DB_AUTO_COMMIT
#define DB_AUTO_COMMIT 0
#endif

#define BDB_VERSION (10000*DB_VERSION_MAJOR+100*DB_VERSION_MINOR+DB_VERSION_PATCH)

#define BDB_MARSHAL        (1<<0)
#define BDB_NOT_OPEN       (1<<1)
#define BDB_RE_SOURCE      (1<<2)
#define BDB_BT_COMPARE     (1<<3)
#define BDB_BT_PREFIX      (1<<4)
#define BDB_DUP_COMPARE    (1<<5)
#define BDB_H_HASH         (1<<6)
#define BDB_APPEND_RECNO   (1<<7)

#define BDB_FEEDBACK       (1<<8)
#define BDB_AUTO_COMMIT    (1<<9)
#define BDB_NO_THREAD      (1<<10)
#define BDB_INIT_LOCK      (1<<11)

#define BDB_NIL            (1<<12)

#define BDB_TXN_COMMIT     (1<<0)

#define BDB_APP_DISPATCH   (1<<0)
#define BDB_REP_TRANSPORT  (1<<1)
#define BDB_ENV_ENCRYPT    (1<<2)
#define BDB_ENV_NOT_OPEN   (1<<3)
 
#if BDB_VERSION >= 30300
#define BDB_ERROR_PRIVATE 44444
#endif

#define BDB_INIT_TRANSACTION (DB_INIT_LOCK | DB_INIT_MPOOL | DB_INIT_TXN | DB_INIT_LOG)
#define BDB_INIT_LOMP (DB_INIT_LOCK | DB_INIT_MPOOL | DB_INIT_LOG)
#define BDB_NEED_CURRENT (BDB_MARSHAL | BDB_BT_COMPARE | BDB_BT_PREFIX | BDB_DUP_COMPARE | BDB_H_HASH | BDB_APPEND_RECNO | BDB_FEEDBACK)

#define BDB_NEED_ENV_CURRENT (BDB_FEEDBACK | BDB_APP_DISPATCH)

#if BDB_VERSION < 20600
#define DB_RMW 0
#endif

extern VALUE bdb_cEnv;
extern VALUE bdb_eFatal;
extern VALUE bdb_eLock, bdb_eLockDead, bdb_eLockHeld, bdb_eLockGranted;
extern VALUE bdb_mDb;
extern VALUE bdb_cCommon, bdb_cBtree, bdb_cRecnum, bdb_cHash, bdb_cRecno, bdb_cUnknown;
extern VALUE bdb_cDelegate;

#if BDB_VERSION >= 30100
extern VALUE bdb_sKeyrange;
#endif

#if BDB_VERSION >= 30000
extern VALUE bdb_cQueue;
#endif

extern VALUE bdb_cTxn, bdb_cTxnCatch;
extern VALUE bdb_cCursor;
extern VALUE bdb_cLock, bdb_cLockid;
extern VALUE bdb_cLsn;

extern VALUE bdb_mMarshal;

extern ID bdb_id_dump, bdb_id_load;
extern ID bdb_id_current_db, bdb_id_current_env;

extern VALUE bdb_deleg_to_orig _((VALUE));

#if BDB_VERSION >= 40000
extern VALUE bdb_env_s_rslbl _((int, VALUE *,VALUE, DB_ENV *));
#endif

struct ary_st {
    int len, total, mark;
    VALUE *ptr;
};

typedef struct  {
    int options;
    VALUE marshal;
    struct ary_st db_ary;
    VALUE home;
    DB_ENV *envp;
#if BDB_VERSION <= 30105
    u_int32_t fidp;
#endif
#if BDB_VERSION >= 40000
    VALUE rep_transport;
#endif
#if BDB_VERSION >= 30000
    VALUE feedback;
#endif
#if BDB_VERSION >= 40100
    VALUE app_dispatch;
#endif
#if BDB_VERSION >= 40416
    VALUE msgcall;
    VALUE thread_id;
    VALUE thread_id_string;
    VALUE isalive;
#endif
} bdb_ENV;

#if BDB_VERSION >= 40000

struct txn_rslbl {
    DB_TXN *txn;
    void *txn_cxx;
    VALUE man;
};

#endif

typedef struct {
    int status, options;
    VALUE marshal, mutex;
    struct ary_st db_ary;
    struct ary_st db_assoc;
    VALUE env;
    DB_TXN *txnid;
    DB_TXN *parent;
#if BDB_VERSION >= 40000
    void *txn_cxx;
    VALUE man;
#endif
} bdb_TXN;

#define FILTER_KEY 0
#define FILTER_VALUE 1

#define FILTER_FREE 2

typedef struct {
    int options;
    VALUE marshal;
    DBTYPE type;
    VALUE env, orig, secondary, txn;
    VALUE filename, database;
    VALUE bt_compare, bt_prefix, dup_compare, h_hash;
    VALUE filter[4];
    VALUE ori_val;
    DB *dbp;
    long len;
    u_int32_t flags;
    u_int32_t partial;
    u_int32_t dlen;
    u_int32_t doff;
    int array_base;
#if BDB_VERSION < 30000
    DB_INFO *dbinfo;
#else
    int re_len;
    char re_pad;
    VALUE feedback;
#endif
#if BDB_VERSION >= 40100
    VALUE append_recno;
#endif
#ifdef DB_PRIORITY_DEFAULT
    int priority;
#endif
} bdb_DB;

#if BDB_VERSION >= 40300

typedef struct {
    DB_SEQUENCE *seqp;
    VALUE db, txn, orig;
    DB_TXN *txnid;
} bdb_SEQ;

#define GetSEQ(obj_,seq_) do {                          \
    Data_Get_Struct(obj_, bdb_SEQ, seq_);               \
    if (seq_->seqp == 0) {                              \
        rb_raise(bdb_eFatal, "closed sequence");        \
    }                                                   \
} while (0)

#endif

typedef struct {
    DBC *dbc;
    VALUE db;
} bdb_DBC;

typedef struct {
    unsigned int lock;
    VALUE env, self;
} bdb_LOCKID;

typedef struct {
#if BDB_VERSION < 30000
    DB_LOCK lock;
#else
    DB_LOCK *lock;
#endif
    VALUE env;
} bdb_LOCK;

typedef struct {
    bdb_DB *dbst;
    DBT *key;
    VALUE value;
} bdb_VALUE;

struct deleg_class {
    int type;
    VALUE db;
    VALUE obj;
    VALUE key;
};

struct dblsnst {
    VALUE env, self;
    DB_LSN *lsn;
#if BDB_VERSION >= 40000
    DB_LOGC *cursor;
    int flags;
#endif
};

#if BDB_VERSION < 30000
#define DB_QUEUE DB_RECNO
#endif

#define RECNUM_TYPE(dbst) ((dbst->type == DB_RECNO || dbst->type == DB_QUEUE) || (dbst->type == DB_BTREE && (dbst->flags & DB_RECNUM)))

#define GetCursorDB(obj, dbcst, dbst)		\
{						\
    Data_Get_Struct(obj, bdb_DBC, dbcst);	\
    if (dbcst->db == 0)				\
        rb_raise(bdb_eFatal, "closed cursor");	\
    GetDB(dbcst->db, dbst);			\
}

#define GetEnvDBErr(obj, envst, id_c, eErr)				\
{									\
    Data_Get_Struct(obj, bdb_ENV, envst);				\
    if (envst->envp == 0)						\
        rb_raise(eErr, "closed environment");				\
    if (envst->options & BDB_NEED_ENV_CURRENT) {			\
	VALUE th = rb_thread_current();					\
									\
	if (!RTEST(th) || !RBASIC(th)->flags) {				\
	    rb_raise(eErr, "invalid thread object");			\
	}								\
    	rb_thread_local_aset(th, id_c, obj);				\
    }									\
}

#define GetEnvDB(obj, envst)	GetEnvDBErr(obj, envst, bdb_id_current_env, bdb_eFatal)

#define GetDB(obj, dbst)						   	\
{									   	\
    Data_Get_Struct(obj, bdb_DB, dbst);					   	\
    if (dbst->dbp == 0) {						   	\
        rb_raise(bdb_eFatal, "closed DB");				   	\
    }									   	\
    if (dbst->options & BDB_NEED_CURRENT) {				   	\
	VALUE th = rb_thread_current();						\
										\
	if (!RTEST(th) || !RBASIC(th)->flags) {					\
	    rb_raise(bdb_eFatal, "invalid thread object");			\
	}									\
    	rb_thread_local_aset(th, bdb_id_current_db, obj); 			\
    }									   	\
}


#if BDB_VERSION < 20600

#define INIT_RECNO(dbst, key, recno)		\
{						\
    recno = 1;					\
    if (RECNUM_TYPE(dbst)) {			\
        key.data = &recno;			\
        key.size = sizeof(db_recno_t);		\
        key.flags |= DB_DBT_MALLOC;		\
    }						\
    else {					\
        key.flags |= DB_DBT_MALLOC;		\
    }						\
}

#define FREE_KEY(dbst, key)					\
{								\
    if (((key).flags & DB_DBT_MALLOC) && !RECNUM_TYPE(dbst)) {	\
	free((key).data);					\
    }								\
}

#else

#define INIT_RECNO(dbst, key, recno)		\
{						\
    recno = 1;					\
    if (RECNUM_TYPE(dbst)) {			\
        key.data = &recno;			\
        key.size = sizeof(db_recno_t);		\
    }						\
    else {					\
        key.flags |= DB_DBT_MALLOC;		\
    }						\
}

#define FREE_KEY(dbst, key)			\
{						\
    if ((key).flags & DB_DBT_MALLOC) {		\
	free((key).data);			\
    }						\
}

#endif

#define INIT_TXN(txnid, obj, dbst) {					  \
  txnid = NULL;								  \
  GetDB(obj, dbst);							  \
  if (RTEST(dbst->txn)) {						  \
      bdb_TXN *txnst;							  \
      Data_Get_Struct(dbst->txn, bdb_TXN, txnst);			  \
  if (txnst->txnid == 0)						  \
    rb_warning("using a db handle associated with a closed transaction"); \
    txnid = txnst->txnid;						  \
  }									  \
}

#define SET_PARTIAL(db, data)			\
    (data).flags |= db->partial;		\
    (data).dlen = db->dlen;			\
    (data).doff = db->doff;

#define GetTxnDBErr(obj, txnst, error)		\
{						\
    Data_Get_Struct(obj, bdb_TXN, txnst);	\
    if (txnst->txnid == 0)			\
        rb_raise(error, "closed transaction");	\
}

#define GetTxnDB(obj, txnst) GetTxnDBErr(obj, txnst, bdb_eFatal)

#define BDB_VALID(obj, type) (RTEST(obj) && BUILTIN_TYPE(obj) == (type))

#if BDB_VERSION < 30000
#define TEST_INIT_LOCK(dbst) (((dbst)->options & BDB_INIT_LOCK)?DB_RMW:0)
#else
#define TEST_INIT_LOCK(dbst) (0)
#endif

#define BDB_ST_KEY    1
#define BDB_ST_VALUE  2
#define BDB_ST_KV     3
#define BDB_ST_DELETE 4
#define BDB_ST_DUPU   (5 | BDB_ST_DUP)
#define BDB_ST_DUPKV  (6 | BDB_ST_DUP)
#define BDB_ST_DUPVAL (7 | BDB_ST_DUP)
#define BDB_ST_REJECT 8
#define BDB_ST_DUP    32
#define BDB_ST_ONE    64
#define BDB_ST_SELECT 128
#define BDB_ST_PREFIX 256

extern VALUE bdb_errstr;
extern int bdb_errcall;

extern int bdb_test_error _((int));

#ifdef DB_INCOMPLETE

#define bdb_cache_error(commande_, correction_, result_) do {	\
    result_ = commande_;					\
								\
    switch (result_) {						\
    case 0:							\
    case DB_NOTFOUND:						\
    case DB_KEYEMPTY:						\
    case DB_KEYEXIST:						\
        break;							\
    case DB_INCOMPLETE:						\
	result_ = 0;						\
        break;							\
    default:							\
        correction_;						\
        bdb_test_error(result_);				\
    }								\
} while (0);

#else

#define bdb_cache_error(commande_, correction_, result_) do {	\
    result_ = commande_;					\
								\
    switch (result_) {						\
    case 0:							\
    case DB_NOTFOUND:						\
    case DB_KEYEMPTY:						\
    case DB_KEYEXIST:						\
        break;							\
    default:							\
        correction_;						\
        bdb_test_error(result_);				\
    }								\
} while (0);

#endif

extern VALUE bdb_obj_init _((int, VALUE *, VALUE));

extern ID bdb_id_call;

#if BDB_VERSION < 30000
extern char *db_strerror _((int));
#endif

extern VALUE bdb_local_aref _(());
extern VALUE bdb_test_recno _((VALUE, DBT *, db_recno_t *, VALUE));
extern VALUE bdb_test_dump _((VALUE, DBT *, VALUE, int));
extern VALUE bdb_test_ret _((VALUE, VALUE, VALUE, int));
extern VALUE bdb_test_load_key _((VALUE, DBT *));
extern VALUE bdb_assoc _((VALUE, DBT *, DBT *));
extern VALUE bdb_assoc3 _((VALUE, DBT *, DBT *, DBT *));
extern VALUE bdb_assoc_dyna _((VALUE, DBT *, DBT *));
extern VALUE bdb_clear _((int, VALUE *, VALUE));
extern VALUE bdb_del _((VALUE, VALUE));
extern void bdb_deleg_mark _((struct deleg_class *));
extern VALUE bdb_each_eulav _((int, VALUE *, VALUE));
extern VALUE bdb_each_key _((int, VALUE *, VALUE));
extern VALUE bdb_each_value _((int, VALUE *, VALUE));
extern VALUE bdb_each_valuec _((int, VALUE *, VALUE, int, VALUE));
extern VALUE bdb_each_kvc _((int, VALUE *, VALUE, int, VALUE, int));
#if BDB_VERSION >= 40300
extern void bdb_env_errcall _((const DB_ENV *, const char *, const char *));
#else
extern void bdb_env_errcall _((const char *, char *));
#endif
extern VALUE bdb_env_init _((int, VALUE *, VALUE));
extern VALUE bdb_protect_close _((VALUE));
extern VALUE bdb_env_open_db _((int, VALUE *, VALUE));
extern VALUE bdb_get _((int, VALUE *, VALUE));
extern VALUE bdb_env_p _((VALUE));
extern VALUE bdb_has_value _((VALUE, VALUE));
extern VALUE bdb_index _((VALUE, VALUE));
extern VALUE bdb_internal_value _((VALUE, VALUE, VALUE, int));
extern VALUE bdb_put _((int, VALUE *, VALUE));
extern VALUE bdb_test_load _((VALUE, DBT *, int));
extern VALUE bdb_to_type _((VALUE, VALUE, VALUE));
extern VALUE bdb_tree_stat _((int, VALUE *, VALUE));
extern VALUE bdb_init _((int, VALUE *, VALUE));
extern void bdb_init_env _((void));
extern void bdb_init_common _((void));
extern void bdb_init_recnum _((void));
extern void bdb_init_transaction _((void));
extern void bdb_init_cursor _((void));
extern void bdb_init_lock _((void));
extern void bdb_init_log _((void));
extern void bdb_init_delegator _((void));
#if BDB_VERSION >= 40300
extern void bdb_init_sequence _((void));
#endif
extern void bdb_clean_env _((VALUE, VALUE));
extern VALUE bdb_makelsn _((VALUE));
extern VALUE bdb_env_rslbl_begin _((VALUE, int, VALUE *, VALUE));
extern VALUE bdb_return_err _((void));
extern void bdb_ary_push _((struct ary_st *, VALUE));
extern void bdb_ary_unshift _((struct ary_st *, VALUE));
extern VALUE bdb_ary_delete _((struct ary_st *, VALUE));
extern void bdb_ary_mark _((struct ary_st *));
extern VALUE bdb_respond_to _((VALUE, ID));

#if defined(__cplusplus)
}
#endif
