#include <ruby.h>
#include <version.h>
#include <rubysig.h>
#include <db.h>
#include <errno.h>

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef DB_AUTO_COMMIT
#define DB_AUTO_COMMIT 0
#endif

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

#define BDB_TXN_COMMIT     (1<<0)

#define BDB_APP_DISPATCH   (1<<0)
#define BDB_REP_TRANSPORT  (1<<1)
#define BDB_ENV_ENCRYPT    (1<<2)

#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3) || DB_VERSION_MAJOR >= 4
#define BDB_ERROR_PRIVATE 44444
#endif

#define BDB_INIT_TRANSACTION (DB_INIT_LOCK | DB_INIT_MPOOL | DB_INIT_TXN | DB_INIT_LOG)
#define BDB_INIT_LOMP (DB_INIT_LOCK | DB_INIT_MPOOL | DB_INIT_LOG)
#define BDB_NEED_CURRENT (BDB_MARSHAL | BDB_BT_COMPARE | BDB_BT_PREFIX | BDB_DUP_COMPARE | BDB_H_HASH | BDB_APPEND_RECNO | BDB_FEEDBACK)

#define BDB_NEED_ENV_CURRENT (BDB_FEEDBACK | BDB_APP_DISPATCH)

#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
#define DB_RMW 0
#endif

extern VALUE bdb_cEnv;
extern VALUE bdb_eFatal;
extern VALUE bdb_eLock, bdb_eLockDead, bdb_eLockHeld, bdb_eLockGranted;
extern VALUE bdb_mDb;
extern VALUE bdb_cCommon, bdb_cBtree, bdb_cRecnum, bdb_cHash, bdb_cRecno, bdb_cUnknown;
extern VALUE bdb_cDelegate;

#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1) || DB_VERSION_MAJOR >= 4
extern VALUE bdb_sKeyrange;
#endif

#if DB_VERSION_MAJOR >= 3
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

#if DB_VERSION_MAJOR >= 4
extern VALUE bdb_env_s_rslbl _((int, VALUE *, VALUE, DB_ENV *));
#endif

typedef struct  {
    int options;
    VALUE marshal;
    VALUE db_ary;
    VALUE home;
    DB_ENV *dbenvp;
#if DB_VERSION_MAJOR < 3 || 					\
    (DB_VERSION_MAJOR == 3 &&					\
       (DB_VERSION_MINOR < 1 || 				\
        (DB_VERSION_MINOR == 1 && DB_VERSION_PATCH <= 5)))
    u_int32_t fidp;
#endif
#if DB_VERSION_MAJOR >= 4
    VALUE rep_transport;
#endif
#if DB_VERSION_MAJOR >= 3
    VALUE feedback;
#endif
#if DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 1
    VALUE app_dispatch;
#endif
} bdb_ENV;

#if DB_VERSION_MAJOR >= 4

struct txn_rslbl {
    DB_TXN *txn;
    void *txn_cxx;
};

#endif

typedef struct {
    int status, options;
    VALUE marshal, mutex;
    VALUE db_ary;
    VALUE db_assoc;
    VALUE env;
    DB_TXN *txnid;
    DB_TXN *parent;
#if DB_VERSION_MAJOR >= 4
    void *txn_cxx;
#endif
} bdb_TXN;

#define FILTER_KEY 0
#define FILTER_VALUE 1

typedef struct {
    int options;
    VALUE marshal;
    DBTYPE type;
    VALUE env, orig, secondary;
    VALUE filename, database;
    VALUE bt_compare, bt_prefix, dup_compare, h_hash;
    VALUE filter[4];
    DB *dbp;
    bdb_TXN *txn;
    long len;
    u_int32_t flags;
    u_int32_t partial;
    u_int32_t dlen;
    u_int32_t doff;
    int array_base;
#if DB_VERSION_MAJOR < 3
    DB_INFO *dbinfo;
#else
    int re_len;
    char re_pad;
    VALUE feedback;
#endif
#if DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 1
    VALUE append_recno;
#endif
#ifdef DB_PRIORITY_DEFAULT
    int priority;
#endif
} bdb_DB;

typedef struct {
    DBC *dbc;
    VALUE db;
} bdb_DBC;

typedef struct {
    unsigned int lock;
    VALUE env;
} bdb_LOCKID;

typedef struct {
#if DB_VERSION_MAJOR < 3
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
    VALUE env;
    DB_LSN *lsn;
#if DB_VERSION_MAJOR >= 4
    DB_LOGC *cursor;
    int flags;
#endif
};

#if DB_VERSION_MAJOR < 3
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

#define GetEnvDBErr(obj, dbenvst, id_c, eErr)			\
{								\
    Data_Get_Struct(obj, bdb_ENV, dbenvst);			\
    if (dbenvst->dbenvp == 0)					\
        rb_raise(eErr, "closed environment");			\
    if (dbenvst->options & BDB_NEED_ENV_CURRENT) {		\
    	rb_thread_local_aset(rb_thread_current(), id_c, obj);	\
    }								\
}

#define GetEnvDB(obj, dbenvst)	GetEnvDBErr(obj, dbenvst, bdb_id_current_env, bdb_eFatal)

#define GetDB(obj, dbst)						   \
{									   \
    Data_Get_Struct(obj, bdb_DB, dbst);					   \
    if (dbst->dbp == 0) {						   \
        rb_raise(bdb_eFatal, "closed DB");				   \
    }									   \
    if (dbst->options & BDB_NEED_CURRENT) {				   \
    	rb_thread_local_aset(rb_thread_current(), bdb_id_current_db, obj); \
    }									   \
}


#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6

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
  if (dbst->txn != NULL) {						  \
  if (dbst->txn->txnid == 0)						  \
    rb_warning("using a db handle associated with a closed transaction"); \
    txnid = dbst->txn->txnid;						  \
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

#if DB_VERSION_MAJOR < 3
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

extern VALUE bdb_errstr;
extern int bdb_errcall;

extern int bdb_test_error _((int));
extern VALUE bdb_obj_init _((int, VALUE *, VALUE));

extern ID bdb_id_call;

#if DB_VERSION_MAJOR < 3
extern char *db_strerror _((int));
#endif

extern VALUE bdb_test_recno _((VALUE, DBT *, db_recno_t *, VALUE));
extern VALUE bdb_test_dump _((VALUE, DBT *, VALUE, int));
extern VALUE bdb_test_ret _((VALUE, VALUE, VALUE, int));
extern VALUE bdb_assoc _((VALUE, DBT *, DBT *));
extern VALUE bdb_assoc3 _((VALUE, DBT *, DBT *, DBT *));
extern VALUE bdb_assoc_dyna _((VALUE, DBT *, DBT *));
extern VALUE bdb_clear _((int, VALUE *, VALUE));
extern VALUE bdb_del _((VALUE, VALUE));
extern void bdb_deleg_mark _((struct deleg_class *));
extern void bdb_deleg_free _((struct deleg_class *));
extern VALUE bdb_each_eulav _((int, VALUE *, VALUE));
extern VALUE bdb_each_key _((int, VALUE *, VALUE));
extern VALUE bdb_each_value _((int, VALUE *, VALUE));
extern VALUE bdb_each_valuec _((int, VALUE *, VALUE, int, VALUE));
extern VALUE bdb_each_kvc _((int, VALUE *, VALUE, int, VALUE, int));
extern void bdb_env_errcall _((const char *, char *));
extern VALUE bdb_env_open_db _((int, VALUE *, VALUE));
extern VALUE bdb_get _((int, VALUE *, VALUE));
extern VALUE bdb_has_env _((VALUE));
extern VALUE bdb_has_value _((VALUE, VALUE));
extern VALUE bdb_index _((VALUE, VALUE));
extern VALUE bdb_internal_value _((VALUE, VALUE, VALUE, int));
extern VALUE bdb_put _((int, VALUE *, VALUE));
extern VALUE bdb_s_new _((int, VALUE *, VALUE));
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
extern VALUE bdb_makelsn _((VALUE));
extern VALUE bdb_env_rslbl_begin _((VALUE, int, VALUE *, VALUE));
extern VALUE bdb_return_err _((void));

#if defined(__cplusplus)
}
#endif
