#include "bdb.h"

static ID id_txn_close;

static void 
bdb_txn_free(txnst)
    bdb_TXN *txnst;
{
    if (txnst->txnid && txnst->parent == NULL) {
        bdb_test_error(txn_abort(txnst->txnid));
        txnst->txnid = NULL;
#if BDB_VERSION >= 40000
	if (txnst->txn_cxx) free(txnst->txn_cxx);
#endif
    }
    free(txnst);
}

static void 
bdb_txn_mark(txnst)
    bdb_TXN *txnst;
{
    if (txnst->marshal) rb_gc_mark(txnst->marshal);
    if (txnst->mutex) rb_gc_mark(txnst->mutex);
    rb_gc_mark(txnst->db_ary);
    rb_gc_mark(txnst->db_assoc);
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

    ary = rb_ary_new();
    GetTxnDB(obj, txnst);
    for (i = 0; i < argc; i++) {
	a = rb_funcall(argv[i], rb_intern("__txn_dup__"), 1, obj);
	rb_ary_push(txnst->db_assoc, a);
        rb_ary_push(ary, a);
    }
    switch (RARRAY(ary)->len) {
    case 0: return Qnil;
    case 1: return RARRAY(ary)->ptr[0];
    default: return ary;
    }
}

static void
bdb_txn_close_all(obj, result)
    VALUE obj;
    VALUE result;
{
    VALUE db;
    bdb_TXN *txnst;
    bdb_ENV *envst;
    int i;

    GetTxnDB(obj, txnst);
    GetEnvDB(txnst->env, envst);
    for (i = 0; i < RARRAY(envst->db_ary)->len; ++i) {
	if (RARRAY(envst->db_ary)->ptr[i] == obj) {
	    rb_ary_delete_at(envst->db_ary, i);
	}
    }
    while ((db = rb_ary_pop(txnst->db_ary)) != Qnil) {
	if (rb_respond_to(db, id_txn_close)) {
	    rb_funcall(db, id_txn_close, 2, result, Qtrue);
	}
    }
    while ((db = rb_ary_pop(txnst->db_assoc)) != Qnil) {
	if (rb_respond_to(db, id_txn_close)) {
	    rb_funcall(db, id_txn_close, 2, result, Qfalse);
	}
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

    rb_secure(4);
    flags = 0;
    if (rb_scan_args(argc, argv, "01", &a) == 1) {
        flags = NUM2INT(a);
    }
    GetTxnDB(obj, txnst);
#if BDB_VERSION < 30000
    bdb_test_error(txn_commit(txnst->txnid));
#else
#if BDB_VERSION >= 40000
    bdb_test_error(txnst->txnid->commit(txnst->txnid, flags));
#else
    bdb_test_error(txn_commit(txnst->txnid, flags));
#endif
#endif
    bdb_txn_close_all(obj, Qtrue);
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
#if BDB_VERSION >= 40000
    bdb_test_error(txnst->txnid->abort(txnst->txnid));
#else
    bdb_test_error(txn_abort(txnst->txnid));
#endif
    bdb_txn_close_all(obj, Qfalse);
    txnst->txnid = NULL;
    if (txnst->status == 1) {
	txnst->status = 0;
	rb_throw("__bdb__begin", Data_Wrap_Struct(bdb_cTxnCatch, 0, 0, txnst));
    }
    return Qtrue;
}

static VALUE
bdb_txn_unlock(txnv)
    VALUE txnv;
{
    bdb_TXN *txnst;
    Data_Get_Struct(txnv, bdb_TXN, txnst);
    if (txnst->mutex != Qnil) {
	rb_funcall2(txnst->mutex, rb_intern("unlock"), 0, 0);
    }
    return Qnil;
}

static VALUE
bdb_catch(val, args, self)
    VALUE val, args, self;
{
    rb_yield(args);
    return Qtrue;
}

static VALUE
bdb_txn_lock(obj)
    VALUE obj;
{
    VALUE result;
    bdb_TXN *txnst;
    VALUE txnv;

    if (TYPE(obj) == T_ARRAY) {
	txnv = RARRAY(obj)->ptr[0];
    }
    else {
	txnv = obj;
    }
    Data_Get_Struct(txnv, bdb_TXN, txnst);
    if (txnst->mutex != Qnil) {
	rb_funcall2(txnst->mutex, rb_intern("lock"), 0, 0);
    }
    txnst->status = 1;
    result = rb_catch("__bdb__begin", bdb_catch, obj);
    if (rb_obj_is_kind_of(result, bdb_cTxnCatch)) {
	bdb_TXN *txn1;
	Data_Get_Struct(result, bdb_TXN, txn1);
	if (txn1 == txnst) {
	    return Qnil;
	}
	else {
	    bdb_txn_close_all(txnv, Qfalse);
	    rb_throw("__bdb__begin", result);
	}
    }
    txnst->status = 0;
    if (txnst->txnid) {
	if (txnst->options & BDB_TXN_COMMIT) {
	    bdb_txn_commit(0, 0, txnv);
	}
	else {
	    bdb_txn_abort(txnv);
	}
    }
    return Qnil;
}

struct dbtxnopt {
    int flags;
    VALUE mutex;
    VALUE timeout, txn_timeout, lock_timeout;
};

static VALUE
bdb_txn_i_options(obj, dbstobj)
    VALUE obj, dbstobj;
{
    struct dbtxnopt *opt = (struct dbtxnopt *)dbstobj;
    VALUE key, value;
    char *options;

    key = rb_ary_entry(obj, 0);
    value = rb_ary_entry(obj, 1);
    key = rb_obj_as_string(key);
    options = RSTRING(key)->ptr;
    if (strcmp(options, "flags") == 0) {
	opt->flags = NUM2INT(value);
    }
    else if (strcmp(options, "mutex") == 0) {
	if (rb_respond_to(value, rb_intern("lock")) &&
	    rb_respond_to(value, rb_intern("unlock"))) {
	    if (!rb_block_given_p()) {
		rb_warning("a mutex is useless without a block");
	    }
	    else {
		opt->mutex = value;
	    }
	}
	else {
	    rb_raise(bdb_eFatal, "mutex must respond to #lock and #unlock");
	}
    }
#if BDB_VERSION >= 40000
    else if (strcmp(options, "timeout") == 0) {
	opt->timeout = value;
    }
    else if (strcmp(options, "txn_timeout") == 0) {
	opt->txn_timeout = value;
    }
    else if (strcmp(options, "lock_timeout") == 0) {
	opt->lock_timeout = value;
    }
#endif
    return Qnil;
}
	
#if BDB_VERSION >= 40000
static VALUE bdb_txn_set_timeout _((VALUE, VALUE));
static VALUE bdb_txn_set_txn_timeout _((VALUE, VALUE));
static VALUE bdb_txn_set_lock_timeout _((VALUE, VALUE));
#endif

VALUE
bdb_env_rslbl_begin(origin, argc, argv, obj)
    VALUE origin;
    int argc;
    VALUE *argv;
    VALUE obj;
{
    bdb_TXN *txnst, *txnstpar;
    DB_TXN *txn, *txnpar;
    DB_ENV *envp;
    bdb_ENV *envst;
    VALUE txnv, b, env;
    int flags, commit;
    VALUE marshal, options;
    struct dbtxnopt opt;

    txnpar = 0;
    flags = commit = 0;
    envst = 0;
    env = 0;
    options = Qnil;
    opt.flags = 0;
    opt.mutex = opt.timeout = opt.txn_timeout = opt.lock_timeout = Qnil;
    if (argc > 0 && TYPE(argv[argc - 1]) == T_HASH) {
	options = argv[argc - 1];
	argc--;
	rb_iterate(rb_each, options, bdb_txn_i_options, (VALUE)&opt);
	flags = opt.flags;
	if (flags & BDB_TXN_COMMIT) {
	    commit = 1;
	    flags &= ~BDB_TXN_COMMIT;
	}
    }
    if (argc > 0 && FIXNUM_P(argv[0])) {
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
	GetEnvDB(env, envst);
	envp = envst->envp;
	marshal = txnstpar->marshal;
    }
    else {
        GetEnvDB(obj, envst);
	env = obj;
        envp = envst->envp;
	marshal = envst->marshal;
    }
#if BDB_VERSION < 30000
    if (envp->tx_info == NULL) {
	rb_raise(bdb_eFatal, "Transaction Manager not enabled");
    }
    bdb_test_error(txn_begin(envp->tx_info, txnpar, &txn));
#else
#if BDB_VERSION >= 40000
    if (origin == Qfalse) {
	bdb_test_error(envp->txn_begin(envp, txnpar, &txn, flags));
    }
    else {
	txn = ((struct txn_rslbl *)origin)->txn;
    }
#else
    bdb_test_error(txn_begin(envp, txnpar, &txn, flags));
#endif
#endif
    txnv = Data_Make_Struct(bdb_cTxn, bdb_TXN, bdb_txn_mark, bdb_txn_free, txnst);
    txnst->env = env;
    txnst->marshal = marshal;
    txnst->txnid = txn;
    txnst->parent = txnpar;
    txnst->status = 0;
    txnst->options = envst->options & BDB_INIT_LOCK;
    txnst->db_ary = rb_ary_new2(0);
    txnst->db_assoc = rb_ary_new2(0);
    txnst->mutex = opt.mutex;
    rb_ary_unshift(envst->db_ary, txnv);
    if (commit) {
	txnst->options |= BDB_TXN_COMMIT;
    }
#if BDB_VERSION >= 40000
    if (origin != Qfalse) {
	txnst->txn_cxx = ((struct txn_rslbl *)origin)->txn_cxx;
    }
#endif
     b = bdb_txn_assoc(argc, argv, txnv);
#if BDB_VERSION >= 40000
    if (!NIL_P(options)) {
	bdb_txn_set_timeout(txnv, opt.timeout);
	bdb_txn_set_txn_timeout(txnv, opt.txn_timeout);
	bdb_txn_set_lock_timeout(txnv, opt.lock_timeout);
    }
#endif
    {
	VALUE tmp;
	if (b == Qnil) {
	    tmp = txnv;
	}
	else {
	    tmp = rb_assoc_new(txnv, b);
	    rb_funcall2(tmp, rb_intern("flatten!"), 0, 0);
	}
	if (rb_block_given_p()) {
	    if (txnst->mutex != Qnil) {
		return rb_ensure(bdb_txn_lock, tmp, bdb_txn_unlock, txnv);
	    }
	    else {
		return bdb_txn_lock(tmp);
	    }
	}
	return tmp;
    }
}

static VALUE
bdb_env_begin(int argc, VALUE *argv, VALUE obj)
{
    return bdb_env_rslbl_begin(Qfalse, argc, argv, obj);
}

static VALUE
bdb_txn_id(obj)
    VALUE obj;
{
    bdb_TXN *txnst;
    int res;

    GetTxnDB(obj, txnst);
#if BDB_VERSION >= 40000
    res = txnst->txnid->id(txnst->txnid);
#else
    res = txn_id(txnst->txnid);
#endif
    return INT2FIX(res);
}

static VALUE
#if BDB_VERSION >= 30300
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
#if BDB_VERSION >= 40000
    id = (unsigned char)NUM2INT(txnid);
    bdb_test_error(txnst->txnid->prepare(txnst->txnid, &id));
#else
#if BDB_VERSION >= 30300
    id = (unsigned char)NUM2INT(txnid);
    bdb_test_error(txn_prepare(txnst->txnid, &id));
#else
    bdb_test_error(txn_prepare(txnst->txnid));
#endif
#endif
    return Qtrue;
}

static VALUE
bdb_env_check(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    unsigned int kbyte, min, flag;
    bdb_ENV *envst;
    VALUE a, b, c;

    kbyte = min = flag = 0;
    a = b = Qnil;
#if BDB_VERSION >= 40000
    switch (rb_scan_args(argc, argv, "03", &a, &b, &c)) {
    case 3:
	flag = NUM2INT(c);
    case 2:
	min = NUM2UINT(b);
    }
#else
    if (rb_scan_args(argc, argv, "02", &a, &b) == 2) {
	min = NUM2UINT(b);
    }
#endif
    if (!NIL_P(a))
	kbyte = NUM2UINT(a);
    GetEnvDB(obj, envst);
#if BDB_VERSION < 30000
    if (envst->envp->tx_info == NULL) {
	rb_raise(bdb_eFatal, "Transaction Manager not enabled");
    }
    bdb_test_error(txn_checkpoint(envst->envp->tx_info, kbyte, min));
#else
#if BDB_VERSION >= 40000
    bdb_test_error(envst->envp->txn_checkpoint(envst->envp, kbyte, min, flag));
#else
#if BDB_VERSION >= 30100
    bdb_test_error(txn_checkpoint(envst->envp, kbyte, min, flag));
#else
    bdb_test_error(txn_checkpoint(envst->envp, kbyte, min));
#endif
#endif
#endif
    return Qnil;
}

#if BDB_VERSION >= 30300

static VALUE
bdb_env_recover(obj)
    VALUE obj;
{
    unsigned int flags;
    long count, retp;
    unsigned char id;
    DB_PREPLIST preplist[1];
    bdb_ENV *envst;
    DB_TXN *txn;
    bdb_TXN *txnst;
    VALUE txnv;

    if (!rb_block_given_p()) {
        rb_raise(bdb_eFatal, "call out of an iterator");
    }
    rb_secure(4);
    GetEnvDB(obj, envst);
    txnv = Data_Make_Struct(bdb_cTxn, bdb_TXN, bdb_txn_mark, bdb_txn_free, txnst);
    txnst->env = obj;
    txnst->marshal = envst->marshal;
    txnst->options = envst->options & BDB_INIT_LOCK;
    txnst->db_ary = rb_ary_new2(0);
    txnst->db_assoc = rb_ary_new2(0);
    flags = DB_FIRST;
    while (1) {
#if BDB_VERSION >= 40000
	bdb_test_error(envst->envp->txn_recover(envst->envp, preplist, 1, &retp, flags));
#else
	bdb_test_error(txn_recover(envst->envp, preplist, 1, &retp, flags));
#endif
	if (retp == 0) break;
	txnst->txnid = preplist[0].txn;
	id = (unsigned char)preplist[0].gid[0];
	rb_yield(rb_assoc_new(txnv, INT2NUM(id)));
	flags = DB_NEXT;
    }
    return obj;
}

static VALUE
bdb_txn_discard(obj)
    VALUE obj;
{
    bdb_TXN *txnst;
    int flags;

    rb_secure(4);
    flags = 0;
    GetTxnDB(obj, txnst);
#if BDB_VERSION >= 40000
    bdb_test_error(txnst->txnid->discard(txnst->txnid, flags));
#else
    bdb_test_error(txn_discard(txnst->txnid, flags));
#endif
    txnst->txnid = NULL;
    return Qtrue;
}

#endif

static VALUE
bdb_env_stat(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    bdb_ENV *envst;
    VALUE a, b;
    DB_TXN_STAT *bdb_stat;
    int flags;

#if BDB_VERSION >= 40000
    flags = 0;
    if (rb_scan_args(argc, argv, "01", &b) == 1) {
	flags = NUM2INT(b);
    }
#else
    if (argc != 0) {
	rb_raise(rb_eArgError, "invalid number of arguments (%d for 0)", argc);
    }
#endif
    GetEnvDB(obj, envst);
#if BDB_VERSION < 30000
    if (envst->envp->tx_info == NULL) {
	rb_raise(bdb_eFatal, "Transaction Manager not enabled");
    }
    bdb_test_error(txn_stat(envst->envp->tx_info, &bdb_stat, malloc));
#else
#if BDB_VERSION >= 40000
    bdb_test_error(envst->envp->txn_stat(envst->envp, &bdb_stat, flags));
#else
#if BDB_VERSION < 30300
    bdb_test_error(txn_stat(envst->envp, &bdb_stat, malloc));
#else
    bdb_test_error(txn_stat(envst->envp, &bdb_stat));
#endif
#endif
#endif
    a = rb_hash_new();
    rb_hash_aset(a, rb_tainted_str_new2("st_time_ckp"), INT2NUM(bdb_stat->st_time_ckp));
    rb_hash_aset(a, rb_tainted_str_new2("st_last_txnid"), INT2NUM(bdb_stat->st_last_txnid));
    rb_hash_aset(a, rb_tainted_str_new2("st_maxtxns"), INT2NUM(bdb_stat->st_maxtxns));
    rb_hash_aset(a, rb_tainted_str_new2("st_naborts"), INT2NUM(bdb_stat->st_naborts));
    rb_hash_aset(a, rb_tainted_str_new2("st_nbegins"), INT2NUM(bdb_stat->st_nbegins));
    rb_hash_aset(a, rb_tainted_str_new2("st_ncommits"), INT2NUM(bdb_stat->st_ncommits));
    rb_hash_aset(a, rb_tainted_str_new2("st_nactive"), INT2NUM(bdb_stat->st_nactive));
#if BDB_VERSION >= 30000
    rb_hash_aset(a, rb_tainted_str_new2("st_maxnactive"), INT2NUM(bdb_stat->st_maxnactive));
    rb_hash_aset(a, rb_tainted_str_new2("st_regsize"), INT2NUM(bdb_stat->st_regsize));
    rb_hash_aset(a, rb_tainted_str_new2("st_region_wait"), INT2NUM(bdb_stat->st_region_wait));
    rb_hash_aset(a, rb_tainted_str_new2("st_region_nowait"), INT2NUM(bdb_stat->st_region_nowait));
#endif
#if BDB_VERSION >= 40000
    {
	struct dblsnst *lsnst;
	VALUE obj, ary, hash, lsn;
	int i;

	rb_hash_aset(a, rb_tainted_str_new2("st_nrestores"), INT2NUM(bdb_stat->st_nrestores));
	lsn = bdb_makelsn(obj);
	Data_Get_Struct(lsn, struct dblsnst, lsnst);
	MEMCPY(lsnst->lsn, &bdb_stat->st_last_ckp, DB_LSN, 1);
	rb_hash_aset(a, rb_tainted_str_new2("st_last_ckp"), lsn);
#if BDB_VERSION < 40100
	lsn = bdb_makelsn(obj);
	Data_Get_Struct(lsn, struct dblsnst, lsnst);
	MEMCPY(lsnst->lsn, &bdb_stat->st_pending_ckp, DB_LSN, 1);
	rb_hash_aset(a, rb_tainted_str_new2("st_pending_ckp"), lsn);
#endif
	ary = rb_ary_new2(bdb_stat->st_nactive);
	for (i = 0; i < bdb_stat->st_nactive; i++) {
	    hash = rb_hash_new();
	    rb_hash_aset(hash, rb_tainted_str_new2("txnid"), INT2NUM(bdb_stat->st_txnarray[i].txnid));
	    rb_hash_aset(hash, rb_tainted_str_new2("parentid"), INT2NUM(bdb_stat->st_txnarray[i].parentid));
	    lsn = bdb_makelsn(obj);
	    Data_Get_Struct(lsn, struct dblsnst, lsnst);
	    MEMCPY(lsnst->lsn, &bdb_stat->st_txnarray[i].lsn, DB_LSN, 1);
	    rb_hash_aset(hash, rb_tainted_str_new2("lsn"), lsn);
	    rb_ary_push(ary, hash);
	}
    }
#endif
    free(bdb_stat);
    return a;
}

#if BDB_VERSION >= 40000

static VALUE
bdb_txn_set_txn_timeout(obj, a)
    VALUE obj, a;
{
    bdb_TXN *txnst;
    bdb_ENV envst;

    if (!NIL_P(a)) {
	GetTxnDB(obj, txnst);
	bdb_test_error(txnst->txnid->set_timeout(txnst->txnid, NUM2UINT(a), DB_SET_TXN_TIMEOUT));
    }
    return obj;
}

static VALUE
bdb_txn_set_lock_timeout(obj, a)
    VALUE obj, a;
{
    bdb_TXN *txnst;
    bdb_ENV envst;

    if (!NIL_P(a)) {
	GetTxnDB(obj, txnst);
	bdb_test_error(txnst->txnid->set_timeout(txnst->txnid, NUM2UINT(a), DB_SET_LOCK_TIMEOUT));
    }
    return obj;
}

static VALUE
bdb_txn_set_timeout(obj, a)
    VALUE obj, a;
{
    if (!NIL_P(a)) {
	if (TYPE(a) == T_ARRAY) {
	    if (RARRAY(a)->len >= 1 && !NIL_P(RARRAY(a)->ptr[0])) {
		bdb_txn_set_txn_timeout(obj, RARRAY(a)->ptr[0]);
	    }
	    if (RARRAY(a)->len == 2 && !NIL_P(RARRAY(a)->ptr[1])) {
		bdb_txn_set_lock_timeout(obj, RARRAY(a)->ptr[0]);
	    }
	}
	else {
	    bdb_txn_set_txn_timeout(obj, a);
	}
    }
    return obj;
} 

#if BDB_VERSION >= 40100

static VALUE
bdb_env_dbremove(argc, argv, obj)
    int argc;
    VALUE *argv, obj;
{
    VALUE a, b, c;
    char *file, *database;
    int flags;
    bdb_ENV *envst;
    bdb_TXN *txnst;
    DB_TXN *txnid;

    rb_secure(2);
    a = b = c = Qnil;
    file = database = NULL;
    flags = 0;
    rb_scan_args(argc, argv, "03", &a, &b, &c);
    if (!NIL_P(a)) {
	Check_SafeStr(a);
	file = RSTRING(a)->ptr;
    }
    if (!NIL_P(b)) {
	Check_SafeStr(b);
	database = RSTRING(b)->ptr;
    }
    if (!NIL_P(c)) {
	flags = NUM2INT(c);
    }
    txnid = NULL;
    if (rb_obj_is_kind_of(obj, bdb_cTxn)) {
        GetTxnDB(obj, txnst);
	txnid = txnst->txnid;
	GetEnvDB(txnst->env, envst);
    }
    else {
	GetEnvDB(obj, envst);
    }
    if (txnid == NULL && (envst->options & BDB_AUTO_COMMIT)) {
      flags |= DB_AUTO_COMMIT;
    }
    bdb_test_error(envst->envp->dbremove(envst->envp, txnid,
					 file, database, flags));
    return Qnil;
}

static VALUE
bdb_env_dbrename(argc, argv, obj)
    int argc;
    VALUE *argv, obj;
{
    VALUE a, b, c, d;
    char *file, *database, *newname;
    int flags;
    bdb_ENV *envst;
    bdb_TXN *txnst;
    DB_TXN *txnid;

    rb_secure(2);
    a = b = c = Qnil;
    file = database = newname = NULL;
    flags = 0;
    if (rb_scan_args(argc, argv, "22", &a, &b, &c, &d) == 2) {
	c = b;
	b = d = Qnil;
    }
    if (!NIL_P(a)) {
	Check_SafeStr(a);
	file = RSTRING(a)->ptr;
    }
    if (!NIL_P(b)) {
	Check_SafeStr(b);
	database = RSTRING(b)->ptr;
    }
    if (!NIL_P(c)) {
	Check_SafeStr(c);
	newname = RSTRING(c)->ptr;
    }
    else {
	rb_raise(bdb_eFatal, "newname not specified");
    }
    if (!NIL_P(d)) {
	flags = NUM2INT(d);
    }
    txnid = NULL;
    if (rb_obj_is_kind_of(obj, bdb_cTxn)) {
        GetTxnDB(obj, txnst);
	txnid = txnst->txnid;
	GetEnvDB(txnst->env, envst);
    }
    else {
	GetEnvDB(obj, envst);
    }
    if (txnid == NULL && (envst->options & BDB_AUTO_COMMIT)) {
      flags |= DB_AUTO_COMMIT;
    }
    bdb_test_error(envst->envp->dbrename(envst->envp, txnid,
					 file, database, newname, flags));
    return Qnil;
}

#endif

#endif


void bdb_init_transaction()
{
    id_txn_close = rb_intern("__txn_close__");
    bdb_cTxn = rb_define_class_under(bdb_mDb, "Txn", rb_cObject);
    bdb_cTxnCatch = rb_define_class_under(bdb_mDb, "DBTxnCatch", bdb_cTxn);
    rb_undef_method(CLASS_OF(bdb_cTxn), "allocate");
    rb_undef_method(CLASS_OF(bdb_cTxn), "new");
    rb_define_method(bdb_cEnv, "begin", bdb_env_begin, -1);
    rb_define_method(bdb_cEnv, "txn_begin", bdb_env_begin, -1);
    rb_define_method(bdb_cEnv, "transaction", bdb_env_begin, -1);
    rb_define_method(bdb_cEnv, "stat", bdb_env_stat, 0);
    rb_define_method(bdb_cEnv, "txn_stat", bdb_env_stat, 0);
    rb_define_method(bdb_cEnv, "checkpoint", bdb_env_check, -1);
    rb_define_method(bdb_cEnv, "txn_checkpoint", bdb_env_check, -1);
#if BDB_VERSION >= 30300
    rb_define_method(bdb_cEnv, "txn_recover", bdb_env_recover, 0);
    rb_define_method(bdb_cEnv, "recover", bdb_env_recover, 0);
#endif
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
#if BDB_VERSION >= 30300
    rb_define_method(bdb_cTxn, "discard", bdb_txn_discard, 0);
    rb_define_method(bdb_cTxn, "txn_discard", bdb_txn_discard, 0);
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
#if BDB_VERSION >= 40000
    rb_define_method(bdb_cTxn, "set_timeout", bdb_txn_set_timeout, 1);
    rb_define_method(bdb_cTxn, "set_txn_timeout", bdb_txn_set_txn_timeout, 1);
    rb_define_method(bdb_cTxn, "set_lock_timeout", bdb_txn_set_lock_timeout, 1);
#if BDB_VERSION >= 40100
    rb_define_method(bdb_cEnv, "dbremove", bdb_env_dbremove, -1);
    rb_define_method(bdb_cTxn, "dbremove", bdb_env_dbremove, -1);
    rb_define_method(bdb_cEnv, "dbrename", bdb_env_dbrename, -1);
    rb_define_method(bdb_cTxn, "dbrename", bdb_env_dbrename, -1);
#endif
#endif    
}
