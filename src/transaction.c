#include "bdb.h"

static void 
bdb_txn_free(txnst)
    bdb_TXN *txnst;
{
    if (txnst->txnid && txnst->parent == NULL) {
        bdb_test_error(txn_abort(txnst->txnid));
        txnst->txnid = NULL;
    }
    free(txnst);
}

static void 
bdb_txn_mark(txnst)
    bdb_TXN *txnst;
{
    if (txnst->marshal) rb_gc_mark(txnst->marshal);
    rb_gc_mark(txnst->db_ary);
}

#define GetTxnDB(obj, txnst)				\
{							\
    Data_Get_Struct(obj, bdb_TXN, txnst);		\
    if (txnst->txnid == 0)				\
        rb_raise(bdb_eFatal, "closed transaction");	\
}

static VALUE
bdb_txn_assoc(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    int i;
    VALUE ary, a;
    bdb_TXN *txnst;
    bdb_DB *dbp, *dbh;

    ary = rb_ary_new();
    GetTxnDB(obj, txnst);
    for (i = 0; i < argc; i++) {
        if (!rb_obj_is_kind_of(argv[i], bdb_cCommon))
            rb_raise(bdb_eFatal, "argument must be a database handle");
        a = Data_Make_Struct(CLASS_OF(argv[i]), bdb_DB, bdb_mark, bdb_free, dbh);
        GetDB(argv[i], dbp);
        MEMCPY(dbh, dbp, bdb_DB, 1);
        dbh->txn = txnst;
	dbh->orig = argv[i];
	dbh->options |= BDB_TXN;
	dbh->flags27 = txnst->flags27;
        rb_ary_push(ary, a);
    }
    switch (RARRAY(ary)->len) {
    case 0: return Qnil;
    case 1: return RARRAY(ary)->ptr[0];
    default: return ary;
    }
}

static VALUE
bdb_txn_commit(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    bdb_TXN *txnst;
    VALUE a, db;
    int flags;
    bdb_DB *dbst, *dbst1;

    rb_secure(4);
    flags = 0;
    if (rb_scan_args(argc, argv, "01", &a) == 1) {
        flags = NUM2INT(a);
    }
    GetTxnDB(obj, txnst);
#if DB_VERSION_MAJOR < 3
    bdb_test_error(txn_commit(txnst->txnid));
#else
    bdb_test_error(txn_commit(txnst->txnid, flags));
#endif
    while ((db = rb_ary_pop(txnst->db_ary)) != Qnil) {
	if (TYPE(db) == T_DATA) {
	    Data_Get_Struct(db, bdb_DB, dbst);
	    Data_Get_Struct(dbst->orig, bdb_DB, dbst1);
	    dbst1->len = dbst->len;
	    bdb_close(0, 0, db);
	}
    }
    txnst->txnid = NULL;
    if (txnst->status == 1) {
	txnst->status = 0;
	rb_throw("__bdb__begin", Data_Wrap_Struct(bdb_cTxnCatch, 0, 0, txnst));
    }
    return Qtrue;
}

static VALUE
bdb_txn_abort(obj)
    VALUE obj;
{
    bdb_TXN *txnst;
    VALUE db;

    GetTxnDB(obj, txnst);
    bdb_test_error(txn_abort(txnst->txnid));
    while ((db = rb_ary_pop(txnst->db_ary)) != Qnil) {
	if (TYPE(db) == T_DATA) {
	    bdb_close(0, 0, db);
	}
    }
    txnst->txnid = NULL;
    if (txnst->status == 1) {
	txnst->status = 0;
	rb_throw("__bdb__begin", Data_Wrap_Struct(bdb_cTxnCatch, 0, 0, txnst));
    }
    return Qtrue;
}

static VALUE
bdb_catch(val, args, self)
    VALUE val, args, self;
{
    rb_yield(args);
    return Qtrue;
}

static VALUE
bdb_env_begin(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    bdb_TXN *txnst, *txnstpar;
    DB_TXN *txn, *txnpar;
    DB_ENV *dbenvp;
    bdb_ENV *dbenvst;
    VALUE txnv, b, env;
    int flags, commit;
    VALUE marshal;

    txnpar = 0;
    flags = commit = 0;
    dbenvst = 0;
    env = 0;
    if (argc >= 1 && !rb_obj_is_kind_of(argv[0], bdb_cCommon)) {
        flags = NUM2INT(argv[0]);
	if (flags & BDB_TXN_COMMIT) {
	    commit = 1;
	    flags &= ~BDB_TXN_COMMIT;
	}
        argc--; argv++;
    }
    if (rb_obj_is_kind_of(obj, bdb_cTxn)) {
        GetTxnDB(obj, txnstpar);
        txnpar = txnstpar->txnid;
	env = txnstpar->env;
	GetEnvDB(env, dbenvst);
	dbenvp = dbenvst->dbenvp;
	marshal = txnstpar->marshal;
    }
    else {
        GetEnvDB(obj, dbenvst);
	env = obj;
        dbenvp = dbenvst->dbenvp;
	marshal = dbenvst->marshal;
    }
#if DB_VERSION_MAJOR < 3
    if (dbenvp->tx_info == NULL) {
	rb_raise(bdb_eFatal, "Transaction Manager not enabled");
    }
    bdb_test_error(txn_begin(dbenvp->tx_info, txnpar, &txn));
#else
    bdb_test_error(txn_begin(dbenvp, txnpar, &txn, flags));
#endif
    txnv = Data_Make_Struct(bdb_cTxn, bdb_TXN, bdb_txn_mark, bdb_txn_free, txnst);
    txnst->env = env;
    txnst->marshal = marshal;
    txnst->txnid = txn;
    txnst->parent = txnpar;
    txnst->status = 0;
    txnst->flags27 = dbenvst->flags27;
    txnst->db_ary = rb_ary_new2(0);
    b = bdb_txn_assoc(argc, argv, txnv);
    if (rb_block_given_p()) {
	VALUE result;
	txnst->status = 1;
	result = rb_catch("__bdb__begin", bdb_catch, (b == Qnil)?txnv:rb_assoc_new(txnv, b));
	if (rb_obj_is_kind_of(result, bdb_cTxnCatch)) {
	    bdb_TXN *txn1;
	    Data_Get_Struct(result, bdb_TXN, txn1);
	    if (txn1 == txnst)
		return Qnil;
	    else
		rb_throw("__bdb__begin", result);
	}
	txnst->status = 0;
	if (txnst->txnid) {
	    if (commit)
		bdb_txn_commit(0, 0, txnv);
	    else
		bdb_txn_abort(txnv);
	}
	return Qnil;
    }
    if (b == Qnil)
        return txnv;
    else
        return rb_assoc_new(txnv, b);
}

static VALUE
bdb_txn_id(obj)
    VALUE obj;
{
    bdb_TXN *txnst;
    int res;

    GetTxnDB(obj, txnst);
    res = txn_id(txnst->txnid);
    return INT2FIX(res);
}

static VALUE
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3
bdb_txn_prepare(obj, txnid)
    VALUE obj, txnid;
#else
bdb_txn_prepare(obj)
    VALUE obj;
#endif
{
    bdb_TXN *txnst;
    unsigned char id;

    GetTxnDB(obj, txnst);
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3
    id = (unsigned char)NUM2INT(txnid);
    bdb_test_error(txn_prepare(txnst->txnid, &id));
#else
    bdb_test_error(txn_prepare(txnst->txnid));
#endif
    return Qtrue;
}

static VALUE
bdb_env_check(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    unsigned int kbyte, min;
    bdb_ENV *dbenvst;
    VALUE a, b;

    kbyte = min = 0;
    a = b = Qnil;
    if (rb_scan_args(argc, argv, "02", &a, &b) == 2) {
	min = NUM2UINT(b);
    }
    if (!NIL_P(a))
	kbyte = NUM2UINT(a);
    GetEnvDB(obj, dbenvst);
#if DB_VERSION_MAJOR < 3
    if (dbenvst->dbenvp->tx_info == NULL) {
	rb_raise(bdb_eFatal, "Transaction Manager not enabled");
    }
    bdb_test_error(txn_checkpoint(dbenvst->dbenvp->tx_info, kbyte, min));
#else
#if DB_VERSION_MINOR >= 1
    bdb_test_error(txn_checkpoint(dbenvst->dbenvp, kbyte, min, 0));
#else
    bdb_test_error(txn_checkpoint(dbenvst->dbenvp, kbyte, min));
#endif
#endif
    return Qnil;
}

static VALUE
bdb_env_stat(obj)
    VALUE obj;
{
    bdb_ENV *dbenvst;
    VALUE a;
    DB_TXN_STAT *stat;

    GetEnvDB(obj, dbenvst);
#if DB_VERSION_MAJOR < 3
    if (dbenvst->dbenvp->tx_info == NULL) {
	rb_raise(bdb_eFatal, "Transaction Manager not enabled");
    }
    bdb_test_error(txn_stat(dbenvst->dbenvp->tx_info, &stat, malloc));
#else
#if DB_VERSION_MINOR < 3
    bdb_test_error(txn_stat(dbenvst->dbenvp, &stat, malloc));
#else
    bdb_test_error(txn_stat(dbenvst->dbenvp, &stat));
#endif
#endif
    a = rb_hash_new();
    rb_hash_aset(a, rb_tainted_str_new2("st_time_ckp"), INT2NUM(stat->st_time_ckp));
    rb_hash_aset(a, rb_tainted_str_new2("st_last_txnid"), INT2NUM(stat->st_last_txnid));
    rb_hash_aset(a, rb_tainted_str_new2("st_maxtxns"), INT2NUM(stat->st_maxtxns));
    rb_hash_aset(a, rb_tainted_str_new2("st_naborts"), INT2NUM(stat->st_naborts));
    rb_hash_aset(a, rb_tainted_str_new2("st_nbegins"), INT2NUM(stat->st_nbegins));
    rb_hash_aset(a, rb_tainted_str_new2("st_ncommits"), INT2NUM(stat->st_ncommits));
    rb_hash_aset(a, rb_tainted_str_new2("st_nactive"), INT2NUM(stat->st_nactive));
#if DB_VERSION_MAJOR > 2 
    rb_hash_aset(a, rb_tainted_str_new2("st_maxnactive"), INT2NUM(stat->st_maxnactive));
    rb_hash_aset(a, rb_tainted_str_new2("st_regsize"), INT2NUM(stat->st_regsize));
    rb_hash_aset(a, rb_tainted_str_new2("st_region_wait"), INT2NUM(stat->st_region_wait));
    rb_hash_aset(a, rb_tainted_str_new2("st_region_nowait"), INT2NUM(stat->st_region_nowait));
#endif
    free(stat);
    return a;
}

void bdb_init_transaction()
{
    bdb_cTxn = rb_define_class_under(bdb_mDb, "Txn", rb_cObject);
    bdb_cTxnCatch = rb_define_class("DBTxnCatch", bdb_cTxn);
    rb_undef_method(CLASS_OF(bdb_cTxn), "new");
    rb_define_method(bdb_cEnv, "begin", bdb_env_begin, -1);
    rb_define_method(bdb_cEnv, "txn_begin", bdb_env_begin, -1);
    rb_define_method(bdb_cEnv, "transaction", bdb_env_begin, -1);
    rb_define_method(bdb_cEnv, "stat", bdb_env_stat, 0);
    rb_define_method(bdb_cEnv, "txn_stat", bdb_env_stat, 0);
    rb_define_method(bdb_cEnv, "checkpoint", bdb_env_check, -1);
    rb_define_method(bdb_cEnv, "txn_checkpoint", bdb_env_check, -1);
    rb_define_method(bdb_cTxn, "begin", bdb_env_begin, -1);
    rb_define_method(bdb_cTxn, "txn_begin", bdb_env_begin, -1);
    rb_define_method(bdb_cTxn, "transaction", bdb_env_begin, -1);
    rb_define_method(bdb_cTxn, "commit", bdb_txn_commit, -1);
    rb_define_method(bdb_cTxn, "txn_commit", bdb_txn_commit, -1);
    rb_define_method(bdb_cTxn, "close", bdb_txn_commit, -1);
    rb_define_method(bdb_cTxn, "txn_close", bdb_txn_commit, -1);
    rb_define_method(bdb_cTxn, "abort", bdb_txn_abort, 0);
    rb_define_method(bdb_cTxn, "txn_abort", bdb_txn_abort, 0);
    rb_define_method(bdb_cTxn, "id", bdb_txn_id, 0);
    rb_define_method(bdb_cTxn, "txn_id", bdb_txn_id, 0);
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3
    rb_define_method(bdb_cTxn, "prepare", bdb_txn_prepare, 1);
    rb_define_method(bdb_cTxn, "txn_prepare", bdb_txn_prepare, 1);
#else
    rb_define_method(bdb_cTxn, "prepare", bdb_txn_prepare, 0);
    rb_define_method(bdb_cTxn, "txn_prepare", bdb_txn_prepare, 0);
#endif
    rb_define_method(bdb_cTxn, "assoc", bdb_txn_assoc, -1);
    rb_define_method(bdb_cTxn, "txn_assoc", bdb_txn_assoc, -1);
    rb_define_method(bdb_cTxn, "associate", bdb_txn_assoc, -1);
    rb_define_method(bdb_cTxn, "open_db", bdb_env_open_db, -1);
}
