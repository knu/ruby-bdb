#include "bdb.h"

static ID id_txn_close;

static VALUE
txn_close_i(VALUE ary[])
{
    if (bdb_respond_to(ary[0], id_txn_close)) {
        rb_funcall(ary[0], id_txn_close, 2, ary[1], ary[2]);
    }
    return Qnil;
}

static void
clean_ary(bdb_TXN *txnst, VALUE result)
{
    VALUE *ary, tmp[3];
    int i, len;

    tmp[0] = Qnil;
    tmp[1] = result;
    tmp[2] = Qtrue;
    if (txnst->db_ary.ptr) {
        txnst->db_ary.mark = Qtrue;
	ary = txnst->db_ary.ptr;
	len = txnst->db_ary.len;

	for (i = 0; i < len; i++) {
            tmp[0] = ary[i];
	    txn_close_i(tmp);
	}
        txnst->db_ary.mark = Qfalse;
	txnst->db_ary.ptr = 0;
	txnst->db_ary.total = txnst->db_ary.len = 0;
	free(ary);
    }
    tmp[2] = Qfalse;
    if (txnst->db_assoc.ptr) {
        txnst->db_assoc.mark = Qtrue;
	ary = txnst->db_assoc.ptr;
	len = txnst->db_assoc.len;
	for (i = 0; i < len; i++) {
            tmp[0] = ary[i];
	    txn_close_i(tmp);
	}
        txnst->db_assoc.mark = Qfalse;
	txnst->db_assoc.ptr = 0;
	txnst->db_assoc.total = txnst->db_assoc.len = 0;
	free(ary);
    }
}

static VALUE 
txn_free(bdb_TXN *txnst)
{
    if (txnst->txnid && txnst->parent == NULL) {
#if HAVE_ST_DB_TXN_ABORT
#if HAVE_DBXML_INTERFACE
	if (txnst->txn_cxx_abort)
	    bdb_test_error(txnst->txn_cxx_abort(txnst->txn_cxx) );
	else
#endif
	    bdb_test_error(txnst->txnid->abort(txnst->txnid));
#else
	txn_abort(txnst->txnid);
#endif
        txnst->txnid = NULL;
    }
#if HAVE_DBXML_INTERFACE
    if (txnst->txn_cxx_free)
        txnst->txn_cxx_free(&(txnst->txn_cxx));
    if (txnst->txn_cxx) 
        free(txnst->txn_cxx);
#endif
    clean_ary(txnst, Qfalse);
    return Qnil;
}

static void
bdb_txn_free(bdb_TXN *txnst)
{
    txn_free(txnst);
    free(txnst);
}

static void 
bdb_txn_mark(bdb_TXN *txnst)
{
    rb_gc_mark(txnst->marshal);
    rb_gc_mark(txnst->mutex);
#if HAVE_DBXML_INTERFACE
    rb_gc_mark(txnst->man);
#endif
    bdb_ary_mark(&txnst->db_ary);
    bdb_ary_mark(&txnst->db_assoc);
}

static VALUE
bdb_txn_assoc(int argc, VALUE *argv, VALUE obj)
{
    int i;
    VALUE ary, a;
    bdb_TXN *txnst;

    ary = rb_ary_new();
    GetTxnDB(obj, txnst);
    for (i = 0; i < argc; i++) {
	a = rb_funcall(argv[i], rb_intern("__txn_dup__"), 1, obj);
	bdb_ary_push(&txnst->db_assoc, a);
        rb_ary_push(ary, a);
    }
    switch (RARRAY_LEN(ary)) {
    case 0: return Qnil;
    case 1: return RARRAY_PTR(ary)[0];
    default: return ary;
    }
}

static void
bdb_txn_close_all(VALUE obj, VALUE result)
{
    bdb_TXN *txnst;
    bdb_ENV *envst;

    GetTxnDB(obj, txnst);
    GetEnvDB(txnst->env, envst);
    bdb_clean_env(txnst->env, obj);
    clean_ary(txnst, result);
}

#define THROW 1
#define COMMIT 2
#define ROLLBACK 3

static VALUE
bdb_txn_commit(int argc, VALUE *argv, VALUE obj)
{
    bdb_TXN *txnst;
    VALUE a;
    int flags;

    rb_secure(4);
    flags = 0;
    if (rb_scan_args(argc, argv, "01", &a) == 1) {
        flags = NUM2INT(a);
    }
    GetTxnDB(obj, txnst);
    bdb_txn_close_all(obj, Qtrue);
#if HAVE_ST_DB_TXN_COMMIT
#if HAVE_DBXML_INTERFACE
    if (txnst->txn_cxx_commit)
        bdb_test_error(txnst->txn_cxx_commit(txnst->txn_cxx, flags) );
    else
#endif
	bdb_test_error(txnst->txnid->commit(txnst->txnid, flags));
#else
#if HAVE_TXN_COMMIT_2
    bdb_test_error(txn_commit(txnst->txnid, flags));
#else
    bdb_test_error(txn_commit(txnst->txnid));
#endif
#endif
    txnst->txnid = NULL;
    if (txnst->status == THROW) {
	txnst->status = COMMIT;
	rb_throw("__bdb__begin", Data_Wrap_Struct(bdb_cTxnCatch, 0, 0, txnst));
    }
    return Qtrue;
}

static VALUE
bdb_txn_abort(VALUE obj)
{
    bdb_TXN *txnst;

    GetTxnDB(obj, txnst);
    bdb_txn_close_all(obj, Qfalse);
#if HAVE_ST_DB_TXN_ABORT
#if HAVE_DBXML_INTERFACE
    if (txnst->txn_cxx_abort)
        bdb_test_error(txnst->txn_cxx_abort(txnst->txn_cxx) );
    else
#endif
	bdb_test_error(txnst->txnid->abort(txnst->txnid));
#else
    bdb_test_error(txn_abort(txnst->txnid));
#endif
    txnst->txnid = NULL;
    if (txnst->status == THROW) {
	txnst->status = ROLLBACK;
	rb_throw("__bdb__begin", Data_Wrap_Struct(bdb_cTxnCatch, 0, 0, txnst));
    }
    return Qtrue;
}

static VALUE
bdb_txn_unlock(VALUE txnv)
{
    bdb_TXN *txnst;
    Data_Get_Struct(txnv, bdb_TXN, txnst);
    if (txnst->mutex != Qnil) {
	rb_funcall2(txnst->mutex, rb_intern("unlock"), 0, 0);
    }
    return Qnil;
}

static VALUE
bdb_catch(VALUE val, VALUE args, VALUE self)
{
    rb_yield(args);
    return Qtrue;
}

static VALUE
bdb_txn_lock(VALUE obj)
{
    VALUE result;
    bdb_TXN *txnst;
    VALUE txnv;

    if (TYPE(obj) == T_ARRAY) {
	txnv = RARRAY_PTR(obj)[0];
    }
    else {
	txnv = obj;
    }
    Data_Get_Struct(txnv, bdb_TXN, txnst);
    if (!NIL_P(txnst->mutex)) {
	rb_funcall2(txnst->mutex, rb_intern("lock"), 0, 0);
    }
    txnst->status = THROW;
    result = rb_catch("__bdb__begin", bdb_catch, obj);
    if (rb_obj_is_kind_of(result, bdb_cTxnCatch)) {
	bdb_TXN *txn1;
	Data_Get_Struct(result, bdb_TXN, txn1);
	if (txn1 == txnst) {
	    return Qnil;
	}
	txnst->status = 0;
	bdb_txn_close_all(txnv, txn1->status == COMMIT);
	txnst->txnid = NULL;
	return result;
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
bdb_txn_i_options(VALUE obj, VALUE dbstobj)
{
    struct dbtxnopt *opt = (struct dbtxnopt *)dbstobj;
    VALUE key, value;
    char *options;

    key = rb_ary_entry(obj, 0);
    value = rb_ary_entry(obj, 1);
    key = rb_obj_as_string(key);
    options = StringValuePtr(key);
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
#if HAVE_ST_DB_TXN_SET_TIMEOUT
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
	
#if HAVE_ST_DB_TXN_SET_TIMEOUT
static VALUE bdb_txn_set_timeout _((VALUE, VALUE));
static VALUE bdb_txn_set_txn_timeout _((VALUE, VALUE));
static VALUE bdb_txn_set_lock_timeout _((VALUE, VALUE));
#endif

VALUE
bdb_env_rslbl_begin(VALUE origin, int argc, VALUE *argv, VALUE obj)
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
#if HAVE_ST_DB_ENV_TX_INFO
    if (envp->tx_info == NULL) {
	rb_raise(bdb_eFatal, "Transaction Manager not enabled");
    }
    bdb_test_error(txn_begin(envp->tx_info, txnpar, &txn));
#else
#if HAVE_ST_DB_ENV_TXN_BEGIN
    if (origin == Qfalse) {
	bdb_test_error(envp->txn_begin(envp, txnpar, &txn, flags));
    }
#if HAVE_DBXML_INTERFACE
    else {
	txn = ((struct txn_rslbl *)origin)->txn;
    }
#endif
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
    txnst->mutex = opt.mutex;
    bdb_ary_unshift(&envst->db_ary, txnv);
    if (commit) {
	txnst->options |= BDB_TXN_COMMIT;
    }
#if HAVE_DBXML_INTERFACE
    if (origin != Qfalse) {
	txnst->txn_cxx = ((struct txn_rslbl *)origin)->txn_cxx;
	txnst->txn_cxx_free = ((struct txn_rslbl *)origin)->txn_cxx_free;
	txnst->txn_cxx_abort   = ((struct txn_rslbl *)origin)->txn_cxx_abort;
	txnst->txn_cxx_commit  = ((struct txn_rslbl *)origin)->txn_cxx_commit;
	txnst->txn_cxx_discard = ((struct txn_rslbl *)origin)->txn_cxx_discard;
	txnst->man = ((struct txn_rslbl *)origin)->man;
    }
#endif
    b = bdb_txn_assoc(argc, argv, txnv);
#if HAVE_ST_DB_TXN_SET_TIMEOUT
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
            tmp = rb_ary_new();
            rb_ary_push(tmp, txnv);
            if (TYPE(b) == T_ARRAY) {
                long i;

                for (i = 0; i < RARRAY_LEN(b); i++) {
                    rb_ary_push(tmp, RARRAY_PTR(b)[i]);
                }
            }
            else {
                rb_ary_push(tmp, b);
            }
	}
	if (rb_block_given_p()) {
	    int state = 0;

	    tmp = rb_protect(bdb_txn_lock, tmp, &state);
	    if (!NIL_P(txnst->mutex)) {
		bdb_txn_unlock(txnv);
	    }
	    if (state) {
                txnst->status = ROLLBACK;
		bdb_txn_abort(txnv);
		rb_jump_tag(state);
	    }
	    if (!NIL_P(tmp)) {
		rb_throw("__bdb__begin", tmp);
	    }
	    return Qnil;
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
bdb_txn_id(VALUE obj)
{
    bdb_TXN *txnst;
    int res;

    GetTxnDB(obj, txnst);
#if HAVE_ST_DB_TXN_ID
    res = txnst->txnid->id(txnst->txnid);
#else
    res = txn_id(txnst->txnid);
#endif
    return INT2FIX(res);
}

static VALUE
#if HAVE_ST_DB_TXN_PREPARE || HAVE_TXN_PREPARE
bdb_txn_prepare(VALUE obj, VALUE txnid)
#else
bdb_txn_prepare(VALUE obj)
#endif
{
    bdb_TXN *txnst;
    unsigned char id;

    GetTxnDB(obj, txnst);
#if HAVE_ST_DB_TXN_PREPARE
    id = (unsigned char)NUM2INT(txnid);
    bdb_test_error(txnst->txnid->prepare(txnst->txnid, &id));
#else
#if HAVE_TXN_PREPARE_2
    id = (unsigned char)NUM2INT(txnid);
    bdb_test_error(txn_prepare(txnst->txnid, &id));
#else
    bdb_test_error(txn_prepare(txnst->txnid));
#endif
#endif
    return Qtrue;
}

static VALUE
bdb_env_check(int argc, VALUE *argv, VALUE obj)
{
    unsigned int kbyte, min, flag;
    bdb_ENV *envst;
    VALUE a, b, c;

    kbyte = min = flag = 0;
    a = b = Qnil;
#if HAVE_ST_DB_ENV_TXN_CHECKPOINT || HAVE_TXN_CHECKPOINT_4
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
#if HAVE_ST_DB_ENV_TX_INFO
    if (envst->envp->tx_info == NULL) {
	rb_raise(bdb_eFatal, "Transaction Manager not enabled");
    }
    bdb_test_error(txn_checkpoint(envst->envp->tx_info, kbyte, min));
#else
#if HAVE_ST_DB_ENV_TXN_CHECKPOINT
    bdb_test_error(envst->envp->txn_checkpoint(envst->envp, kbyte, min, flag));
#else
#if HAVE_TXN_CHECKPOINT_4
    bdb_test_error(txn_checkpoint(envst->envp, kbyte, min, flag));
#else
    bdb_test_error(txn_checkpoint(envst->envp, kbyte, min));
#endif
#endif
#endif
    return Qnil;
}

#if HAVE_ST_DB_ENV_TXN_RECOVER || HAVE_TXN_RECOVER

static VALUE
bdb_env_recover(VALUE obj)
{
    unsigned int flags;
    long retp;
    unsigned char id;
    DB_PREPLIST preplist[1];
    bdb_ENV *envst;
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
    flags = DB_FIRST;
    while (1) {
#if HAVE_ST_DB_ENV_TXN_RECOVER
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

#endif

#if HAVE_ST_DB_TXN_DISCARD || HAVE_TXN_DISCARD

static VALUE
bdb_txn_discard(VALUE obj)
{
    bdb_TXN *txnst;
    int flags;

    rb_secure(4);
    flags = 0;
    GetTxnDB(obj, txnst);
#if HAVE_ST_DB_TXN_DISCARD
#if HAVE_DBXML_INTERFACE
    if (txnst->txn_cxx_discard)
        bdb_test_error(txnst->txn_cxx_discard(txnst->txn_cxx, flags) );
    else
#endif
        bdb_test_error(txnst->txnid->discard(txnst->txnid, flags));
#else
    bdb_test_error(txn_discard(txnst->txnid, flags));
#endif
    txnst->txnid = NULL;
    return Qtrue;
}

#endif

static VALUE
bdb_env_stat(int argc, VALUE *argv, VALUE obj)
{
    bdb_ENV *envst;
    VALUE a, b;
    DB_TXN_STAT *bdb_stat;
    int flags;

#if HAVE_ST_DB_ENV_TXN_STAT
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
#if HAVE_ST_DB_ENV_TX_INFO
    if (envst->envp->tx_info == NULL) {
	rb_raise(bdb_eFatal, "Transaction Manager not enabled");
    }
    bdb_test_error(txn_stat(envst->envp->tx_info, &bdb_stat, malloc));
#else
#if HAVE_ST_DB_ENV_TXN_STAT
    bdb_test_error(envst->envp->txn_stat(envst->envp, &bdb_stat, flags));
#else
#if HAVE_TXN_STAT_3
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
#if HAVE_ST_DB_TXN_STAT_ST_MAXNACTIVE
    rb_hash_aset(a, rb_tainted_str_new2("st_maxnactive"), INT2NUM(bdb_stat->st_maxnactive));
#endif
#if HAVE_ST_DB_TXN_STAT_ST_REGSIZE
    rb_hash_aset(a, rb_tainted_str_new2("st_regsize"), INT2NUM(bdb_stat->st_regsize));
#endif
#if HAVE_ST_DB_TXN_STAT_ST_REGION_WAIT
    rb_hash_aset(a, rb_tainted_str_new2("st_region_wait"), INT2NUM(bdb_stat->st_region_wait));
#endif
#if HAVE_ST_DB_TXN_STAT_ST_REGION_NOWAIT
    rb_hash_aset(a, rb_tainted_str_new2("st_region_nowait"), INT2NUM(bdb_stat->st_region_nowait));
#endif
#if HAVE_ST_DB_TXN_STAT_ST_LAST_CKP
    {
	struct dblsnst *lsnst;
	VALUE ary, hash, lsn;
	int i;

#if HAVE_ST_DB_TXN_STAT_ST_NRESTORES
	rb_hash_aset(a, rb_tainted_str_new2("st_nrestores"), INT2NUM(bdb_stat->st_nrestores));
#endif
	lsn = bdb_makelsn(obj);
	Data_Get_Struct(lsn, struct dblsnst, lsnst);
	MEMCPY(lsnst->lsn, &bdb_stat->st_last_ckp, DB_LSN, 1);
	rb_hash_aset(a, rb_tainted_str_new2("st_last_ckp"), lsn);
#if HAVE_ST_DB_TXN_STAT_ST_PENDING_CKP
	lsn = bdb_makelsn(obj);
	Data_Get_Struct(lsn, struct dblsnst, lsnst);
	MEMCPY(lsnst->lsn, &bdb_stat->st_pending_ckp, DB_LSN, 1);
	rb_hash_aset(a, rb_tainted_str_new2("st_pending_ckp"), lsn);
#endif
	ary = rb_ary_new2(bdb_stat->st_nactive);
	for (i = 0; i < bdb_stat->st_nactive; i++) {
	    hash = rb_hash_new();
	    rb_hash_aset(hash, rb_tainted_str_new2("txnid"), INT2NUM(bdb_stat->st_txnarray[i].txnid));
#if HAVE_ST_DB_TXN_ACTIVE_PARENTID
	    rb_hash_aset(hash, rb_tainted_str_new2("parentid"), INT2NUM(bdb_stat->st_txnarray[i].parentid));
#endif
	    lsn = bdb_makelsn(obj);
	    Data_Get_Struct(lsn, struct dblsnst, lsnst);
	    MEMCPY(lsnst->lsn, &bdb_stat->st_txnarray[i].lsn, DB_LSN, 1);
	    rb_hash_aset(hash, rb_tainted_str_new2("lsn"), lsn);
#if HAVE_ST_DB_TXN_ACTIVE_TID
	    rb_hash_aset(hash, rb_tainted_str_new2("thread_id"), INT2NUM(bdb_stat->st_txnarray[i].tid));
#endif
#if HAVE_ST_DB_TXN_ACTIVE_NAME
	    rb_hash_aset(hash, rb_tainted_str_new2("name"), rb_tainted_str_new2(bdb_stat->st_txnarray[i].name));
#endif
	    rb_ary_push(ary, hash);
	}
    }
#endif
    free(bdb_stat);
    return a;
}

#if HAVE_ST_DB_TXN_SET_TIMEOUT

static VALUE
bdb_txn_set_txn_timeout(VALUE obj, VALUE a)
{
    bdb_TXN *txnst;

    if (!NIL_P(a)) {
	GetTxnDB(obj, txnst);
	bdb_test_error(txnst->txnid->set_timeout(txnst->txnid, NUM2UINT(a), DB_SET_TXN_TIMEOUT));
    }
    return obj;
}

static VALUE
bdb_txn_set_lock_timeout(VALUE obj, VALUE a)
{
    bdb_TXN *txnst;

    if (!NIL_P(a)) {
	GetTxnDB(obj, txnst);
	bdb_test_error(txnst->txnid->set_timeout(txnst->txnid, NUM2UINT(a), DB_SET_LOCK_TIMEOUT));
    }
    return obj;
}

static VALUE
bdb_txn_set_timeout(VALUE obj, VALUE a)
{
    if (!NIL_P(a)) {
	if (TYPE(a) == T_ARRAY) {
	    if (RARRAY_LEN(a) >= 1 && !NIL_P(RARRAY_PTR(a)[0])) {
		bdb_txn_set_txn_timeout(obj, RARRAY_PTR(a)[0]);
	    }
	    if (RARRAY_LEN(a) == 2 && !NIL_P(RARRAY_PTR(a)[1])) {
		bdb_txn_set_lock_timeout(obj, RARRAY_PTR(a)[1]);
	    }
	}
	else {
	    bdb_txn_set_txn_timeout(obj, a);
	}
    }
    return obj;
} 

#endif

#if HAVE_ST_DB_ENV_DBREMOVE

static VALUE
bdb_env_dbremove(int argc, VALUE *argv, VALUE obj)
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
	SafeStringValue(a);
	file = StringValuePtr(a);
    }
    if (!NIL_P(b)) {
	SafeStringValue(b);
	database = StringValuePtr(b);
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
#if HAVE_CONST_DB_AUTO_COMMIT
    if (txnid == NULL && (envst->options & BDB_AUTO_COMMIT)) {
      flags |= DB_AUTO_COMMIT;
    }
#endif
    bdb_test_error(envst->envp->dbremove(envst->envp, txnid,
					 file, database, flags));
    return Qnil;
}

#endif

#if HAVE_ST_DB_ENV_DBRENAME

static VALUE
bdb_env_dbrename(int argc, VALUE *argv, VALUE obj)
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
	SafeStringValue(a);
	file = StringValuePtr(a);
    }
    if (!NIL_P(b)) {
	SafeStringValue(b);
	database = StringValuePtr(b);
    }
    if (!NIL_P(c)) {
	SafeStringValue(c);
	newname = StringValuePtr(c);
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
#if HAVE_CONST_DB_AUTO_COMMIT
    if (txnid == NULL && (envst->options & BDB_AUTO_COMMIT)) {
      flags |= DB_AUTO_COMMIT;
    }
#endif
    bdb_test_error(envst->envp->dbrename(envst->envp, txnid,
					 file, database, newname, flags));
    return Qnil;
}

#endif

#if HAVE_ST_DB_TXN_SET_NAME

static VALUE
bdb_txn_set_name(VALUE obj, VALUE a)
{
    bdb_TXN *txnst;

    GetTxnDB(obj, txnst);
    bdb_test_error(txnst->txnid->set_name(txnst->txnid, StringValuePtr(a)));
    return a;
}

static VALUE
bdb_txn_get_name(VALUE obj)
{
    bdb_TXN *txnst;
    const char *name;

    GetTxnDB(obj, txnst);
    bdb_test_error(txnst->txnid->get_name(txnst->txnid, &name));
    return rb_tainted_str_new2(name);
}

#endif

void bdb_init_transaction()
{
    id_txn_close = rb_intern("__txn_close__");
    bdb_cTxn = rb_define_class_under(bdb_mDb, "Txn", rb_cObject);
    bdb_cTxnCatch = rb_define_class_under(bdb_mDb, "DBTxnCatch", bdb_cTxn);
#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
    rb_undef_alloc_func(bdb_cTxn);
#else
    rb_undef_method(CLASS_OF(bdb_cTxn), "allocate");
#endif
    rb_undef_method(CLASS_OF(bdb_cTxn), "new");
    rb_define_method(bdb_cEnv, "begin", bdb_env_begin, -1);
    rb_define_method(bdb_cEnv, "txn_begin", bdb_env_begin, -1);
    rb_define_method(bdb_cEnv, "transaction", bdb_env_begin, -1);
    rb_define_method(bdb_cEnv, "stat", bdb_env_stat, -1);
    rb_define_method(bdb_cEnv, "txn_stat", bdb_env_stat, -1);
    rb_define_method(bdb_cEnv, "checkpoint", bdb_env_check, -1);
    rb_define_method(bdb_cEnv, "txn_checkpoint", bdb_env_check, -1);
#if HAVE_ST_DB_ENV_TXN_RECOVER || HAVE_TXN_RECOVER
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
#if HAVE_ST_DB_TXN_DISCARD || HAVE_TXN_DISCARD
    rb_define_method(bdb_cTxn, "discard", bdb_txn_discard, 0);
    rb_define_method(bdb_cTxn, "txn_discard", bdb_txn_discard, 0);
#endif
#if HAVE_ST_DB_TXN_PREPARE || HAVE_TXN_PREPARE
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
#if HAVE_ST_DB_TXN_SET_TIMEOUT
    rb_define_method(bdb_cTxn, "set_timeout", bdb_txn_set_timeout, 1);
    rb_define_method(bdb_cTxn, "set_txn_timeout", bdb_txn_set_txn_timeout, 1);
    rb_define_method(bdb_cTxn, "set_lock_timeout", bdb_txn_set_lock_timeout, 1);
    rb_define_method(bdb_cTxn, "timeout=", bdb_txn_set_timeout, 1);
    rb_define_method(bdb_cTxn, "txn_timeout=", bdb_txn_set_txn_timeout, 1);
    rb_define_method(bdb_cTxn, "lock_timeout=", bdb_txn_set_lock_timeout, 1);
#endif
#if HAVE_ST_DB_ENV_DBREMOVE
    rb_define_method(bdb_cEnv, "dbremove", bdb_env_dbremove, -1);
    rb_define_method(bdb_cTxn, "dbremove", bdb_env_dbremove, -1);
#endif
#if HAVE_ST_DB_ENV_DBRENAME
    rb_define_method(bdb_cEnv, "dbrename", bdb_env_dbrename, -1);
    rb_define_method(bdb_cTxn, "dbrename", bdb_env_dbrename, -1);
#endif
#if HAVE_ST_DB_TXN_SET_NAME
    rb_define_method(bdb_cTxn, "name", bdb_txn_get_name, 0);
    rb_define_method(bdb_cTxn, "name=", bdb_txn_set_name, 1);
#endif
}
