#include "bdb.h"

ID bdb_id_call;

#if BDB_VERSION >= 30000
static ID id_feedback;
#endif

static VALUE bdb_env_internal_ary;

ID bdb_id_current_env;

#if BDB_VERSION >= 40100
static ID id_app_dispatch;
#endif

struct db_stoptions {
    bdb_ENV *env;
    VALUE config;
    int lg_max, lg_bsize;
};

#if BDB_VERSION >= 40000

static int
bdb_env_rep_transport(env, control, rec, envid, flags)
    DB_ENV *env;
    const DBT *control;
    const DBT *rec;
    int envid;
    u_int32_t flags;
{
    VALUE obj, av, bv, res;
    bdb_ENV *envst;

    if ((obj = rb_thread_local_aref(rb_thread_current(), bdb_id_current_env)) == Qnil) {
	rb_raise(bdb_eFatal, "BUG : current_env not set");
    }
    GetEnvDB(obj, envst);
    av = rb_tainted_str_new(control->data, control->size);
    bv = rb_tainted_str_new(rec->data, rec->size);
    if (envst->rep_transport == 0) {
	res = rb_funcall(obj, rb_intern("bdb_rep_transport"), 4, av, bv,
			 INT2FIX(envid), INT2FIX(flags));
    }
    else {
	res = rb_funcall(envst->rep_transport, bdb_id_call, 4, 
			 av, bv, INT2FIX(envid), INT2FIX(flags));
    }
    return NUM2INT(res);
}

static VALUE
bdb_env_rep_elect(env, nb, pri, ti)
    VALUE env, nb, pri, ti;
{
    bdb_ENV *envst;
    int envid = 0;

    GetEnvDB(env, envst);
    bdb_test_error(envst->envp->rep_elect(envst->envp, NUM2INT(nb),
					  NUM2INT(pri), NUM2INT(ti), &envid));
    return INT2NUM(envid);
}

static VALUE
bdb_env_rep_process_message(env, av, bv, ev)
    VALUE env, av, bv, ev;
{
    bdb_ENV *envst;
    DBT control, rec;
    int ret, envid;
    VALUE result;

    GetEnvDB(env, envst);
    av = rb_str_to_str(av);
    bv = rb_str_to_str(bv);
    MEMZERO(&control, DBT, 1);
    MEMZERO(&rec, DBT, 1);
    control.size = RSTRING(av)->len;
    control.data = StringValuePtr(av);
    rec.size = RSTRING(bv)->len;
    rec.data = StringValuePtr(bv);
    envid = NUM2INT(ev);
    ret = envst->envp->rep_process_message(envst->envp, &control, &rec, &envid);
    if (ret == DB_RUNRECOVERY) {
	bdb_test_error(ret);
    }
    result = rb_ary_new2(3);
    rb_ary_push(result, INT2NUM(ret));
    rb_ary_push(result, rb_str_new(rec.data, rec.size));
    rb_ary_push(result, INT2NUM(envid));
    return result;
}

static VALUE
bdb_env_rep_start(env, ident, flags)
    VALUE env, ident, flags;
{
    bdb_ENV *envst;
    DBT cdata;

    GetEnvDB(env, envst);
    if (!NIL_P(ident)) {
	ident = rb_str_to_str(ident);
	MEMZERO(&cdata, DBT, 1);
	cdata.size = RSTRING(ident)->len;
	cdata.data = StringValuePtr(ident);
    }
    bdb_test_error(envst->envp->rep_start(envst->envp, 
					  NIL_P(ident)?NULL:&cdata,
					  NUM2INT(flags)));
    return Qnil;
}

#if BDB_VERSION >= 40100

static VALUE
bdb_env_rep_limit(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    bdb_ENV *envst;
    VALUE a, b;
    u_int32_t gbytes, bytes;

    GetEnvDB(obj, envst);
    gbytes = bytes = 0;
    switch(rb_scan_args(argc, argv, "11", &a, &b)) {
    case 1:
	if (TYPE(a) == T_ARRAY) {
	    if (RARRAY(a)->len != 2) {
		rb_raise(bdb_eFatal, "Expected an Array with 2 values");
	    }
	    gbytes = NUM2UINT(RARRAY(a)->ptr[0]);
	    bytes = NUM2UINT(RARRAY(a)->ptr[1]);
	}
	else {
	    bytes = NUM2UINT(RARRAY(a)->ptr[1]);
	}
	break;
    case 2:
	gbytes = NUM2UINT(a);
	bytes = NUM2UINT(b);
	break;
    }
    bdb_test_error(envst->envp->set_rep_limit(envst->envp, gbytes, bytes));
    return obj;
}
#endif
 
#endif

#if BDB_VERSION >= 30000
static void
bdb_env_feedback(DB_ENV *envp, int opcode, int pct)
{
    VALUE obj;
    bdb_ENV *envst;
    if ((obj = rb_thread_local_aref(rb_thread_current(), bdb_id_current_env)) == Qnil) {
	rb_raise(bdb_eFatal, "BUG : current_env not set");
    }
    Data_Get_Struct(obj, bdb_ENV, envst);
    if (NIL_P(envst->feedback)) {
	return;
    }
    if (envst->feedback == 0) {
	rb_funcall(obj, id_feedback, 2, INT2NUM(opcode), INT2NUM(pct));
    }
    else {
	rb_funcall(envst->feedback, bdb_id_call, 2, INT2NUM(opcode), INT2NUM(pct));
    }
}

#endif

#if BDB_VERSION >= 40100

static int
bdb_env_app_dispatch(DB_ENV *envp, DBT *log_rec, DB_LSN *lsn, db_recops op)
{
    VALUE obj, lsnobj, logobj, res;
    bdb_ENV *envst;
    struct dblsnst *lsnst;

    if ((obj = rb_thread_local_aref(rb_thread_current(), bdb_id_current_env)) == Qnil) {
	rb_raise(bdb_eFatal, "BUG : current_env not set");
    }
    lsnobj = bdb_makelsn(obj);
    Data_Get_Struct(lsnobj, struct dblsnst, lsnst);
    MEMCPY(lsnst->lsn, lsn, DB_LSN, 1);
    logobj = rb_str_new(log_rec->data, log_rec->size);
    if (envst->app_dispatch == 0) {
	res = rb_funcall(obj, id_app_dispatch, 3, logobj, lsnobj, INT2NUM(op));
    }
    else {
	res = rb_funcall(envst->app_dispatch, bdb_id_call, 3, logobj, lsnobj, INT2NUM(op));
    }
    return NUM2INT(res);
}

#endif

static VALUE
bdb_env_i_options(obj, db_stobj)
    VALUE obj, db_stobj;
{
    char *options;
    DB_ENV *envp;
    VALUE key, value;
    bdb_ENV *envst;
    struct db_stoptions *db_st;

    Data_Get_Struct(db_stobj, struct db_stoptions, db_st);
    envst = db_st->env;

    key = rb_ary_entry(obj, 0);
    value = rb_ary_entry(obj, 1);
    envp = envst->envp;
    key = rb_obj_as_string(key);
    options = StringValuePtr(key);
    if (strcmp(options, "set_cachesize") == 0) {
	switch (TYPE(value)) {
	case T_FIXNUM:
	case T_FLOAT:
	case T_BIGNUM:
#if BDB_VERSION < 30000
	    envp->mp_size = NUM2INT(value);
#else
	    bdb_test_error(envp->set_cachesize(envp, 0, NUM2UINT(value), 0));
#endif
	    break;
	default:
	    Check_Type(value, T_ARRAY);
	    if (RARRAY(value)->len < 3) {
		rb_raise(bdb_eFatal, "expected 3 values for cachesize");
	    }
#if BDB_VERSION < 30000
	    envp->mp_size = NUM2INT(RARRAY(value)->ptr[1]);
#else
	    bdb_test_error(envp->set_cachesize(envp, 
					       NUM2UINT(RARRAY(value)->ptr[0]),
					       NUM2UINT(RARRAY(value)->ptr[1]),
					       NUM2INT(RARRAY(value)->ptr[2])));
#endif
	    break;
	}
    }
#if DB_VERSION_MAJOR == 3
    else if (strcmp(options, "set_region_init") == 0) {
#if BDB_VERSION < 30114
        bdb_test_error(envp->set_region_init(envp, NUM2INT(value)));
#else
        bdb_test_error(db_env_set_region_init(NUM2INT(value)));
#endif
    }
#endif
#if BDB_VERSION >= 30000
    else if (strcmp(options, "set_tas_spins") == 0) {
#if BDB_VERSION < 30114 || BDB_VERSION >= 40000
        bdb_test_error(envp->set_tas_spins(envp, NUM2INT(value)));
#else
        bdb_test_error(db_env_set_tas_spins(NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_tx_max") == 0) {
        bdb_test_error(envp->set_tx_max(envp, NUM2INT(value)));
    }
#if BDB_VERSION >= 30100
    else if (strcmp(options, "set_tx_timestamp") == 0) {
	time_t ti;
	value = rb_Integer(value);
	ti = (time_t)NUM2INT(value);
        bdb_test_error(envp->set_tx_timestamp(envp, &ti));
    }
#endif
#endif
#if BDB_VERSION < 30000
    else if  (strcmp(options, "set_verbose") == 0) {
        envp->db_verbose = NUM2INT(value);
    }
#else
    else if (strcmp(options, "set_verb_chkpoint") == 0) {
        bdb_test_error(envp->set_verbose(envp, DB_VERB_CHKPOINT, NUM2INT(value)));
    }
    else if (strcmp(options, "set_verb_deadlock") == 0) {
        bdb_test_error(envp->set_verbose(envp, DB_VERB_DEADLOCK, NUM2INT(value)));
    }
    else if (strcmp(options, "set_verb_recovery") == 0) {
        bdb_test_error(envp->set_verbose(envp, DB_VERB_RECOVERY, NUM2INT(value)));
    }
    else if (strcmp(options, "set_verb_waitsfor") == 0) {
        bdb_test_error(envp->set_verbose(envp, DB_VERB_WAITSFOR, NUM2INT(value)));
    }
#if BDB_VERSION >= 40000
    else if (strcmp(options, "set_verb_replication") == 0) {
        bdb_test_error(envp->set_verbose(envp, DB_VERB_REPLICATION, NUM2INT(value)));
    }
#endif
#endif
    else if (strcmp(options, "set_lk_detect") == 0) {
#if BDB_VERSION < 30000
	envp->lk_detect = NUM2INT(value);
#else
        bdb_test_error(envp->set_lk_detect(envp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_lk_max") == 0) {
#if BDB_VERSION < 30000
	envp->lk_max = NUM2INT(value);
#else
        bdb_test_error(envp->set_lk_max(envp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_lk_conflicts") == 0) {
	int i, j, l, v;
	unsigned char *conflits, *p;

	Check_Type(value, T_ARRAY);
	l = RARRAY(value)->len;
	p = conflits = ALLOC_N(unsigned char, l * l);
	for (i = 0; i < l; i++) {
	    if (TYPE(RARRAY(value)->ptr[i]) != T_ARRAY ||
		RARRAY(RARRAY(value)->ptr[i])->len != l) {
		free(conflits);
		rb_raise(bdb_eFatal, "invalid array for lk_conflicts");
	    }
	    for (j = 0; j < l; j++, p++) {
		if (TYPE(RARRAY(RARRAY(value)->ptr[i])->ptr[j] != T_FIXNUM)) {
		    free(conflits);
		    rb_raise(bdb_eFatal, "invalid value for lk_conflicts");
		}
		v = NUM2INT(RARRAY(RARRAY(value)->ptr[i])->ptr[j]);
		if (v != 0 && v != 1) {
		    free(conflits);
		    rb_raise(bdb_eFatal, "invalid value for lk_conflicts");
		}
		*p = (unsigned char)v;
	    }
	}
#if BDB_VERSION < 30000
	envp->lk_modes = l;
	envp->lk_conflicts = conflits;
#else
        bdb_test_error(envp->set_lk_conflicts(envp, conflits, l));
#endif
    }
    else if (strcmp(options, "set_lg_max") == 0) {
	db_st->lg_max = NUM2INT(value);
    }
#if BDB_VERSION >= 30000
    else if (strcmp(options, "set_lg_bsize") == 0) {
	db_st->lg_bsize = NUM2INT(value);
    }
#endif
    else if (strcmp(options, "set_data_dir") == 0) {
	char *tmp;

	SafeStringValue(value);
#if BDB_VERSION >= 30100
	bdb_test_error(envp->set_data_dir(envp, StringValuePtr(value)));
#else
	tmp = ALLOCA_N(char, strlen("DB_DATA_DIR") + RSTRING(value)->len + 2);
	sprintf(tmp, "DB_DATA_DIR %s", StringValuePtr(value));
	rb_ary_push(db_st->config, rb_str_new2(tmp));
#endif
    }
    else if (strcmp(options, "set_lg_dir") == 0) {
	char *tmp;

	SafeStringValue(value);
#if BDB_VERSION >= 30100
	bdb_test_error(envp->set_lg_dir(envp, StringValuePtr(value)));
#else
	tmp = ALLOCA_N(char, strlen("DB_LOG_DIR") + RSTRING(value)->len + 2);
	sprintf(tmp, "DB_LOG_DIR %s", StringValuePtr(value));
	rb_ary_push(db_st->config, rb_str_new2(tmp));
#endif
    }
    else if (strcmp(options, "set_tmp_dir") == 0) {
	char *tmp;

	SafeStringValue(value);
#if BDB_VERSION >= 30100
	bdb_test_error(envp->set_tmp_dir(envp, StringValuePtr(value)));
#else
	tmp = ALLOCA_N(char, strlen("DB_TMP_DIR") + RSTRING(value)->len + 2);
	sprintf(tmp, "DB_TMP_DIR %s", StringValuePtr(value));
	rb_ary_push(db_st->config, rb_str_new2(tmp));
#endif
    }
#if BDB_VERSION >= 30100
    else if (strcmp(options, "set_server") == 0 ||
	strcmp(options, "set_rpc_server") == 0) {
	char *host;
	long sv_timeout, cl_timeout;
	unsigned long flags;

	host = 0;
	sv_timeout = cl_timeout = 0;
	flags = 0;
	switch (TYPE(value)) {
	case T_STRING:
	    SafeStringValue(value);
	    host = StringValuePtr(value);
	    break;
	case T_ARRAY:
	    switch (RARRAY(value)->len) {
	    default:
	    case 3:
		sv_timeout = NUM2INT(RARRAY(value)->ptr[2]);
	    case 2:
		cl_timeout = NUM2INT(RARRAY(value)->ptr[1]);
	    case 1:
		SafeStringValue(RARRAY(value)->ptr[0]);
		host = StringValuePtr(RARRAY(value)->ptr[0]);
		break;
	    case 0:
		rb_raise(bdb_eFatal, "Empty array for \"set_server\"");
		break;
	    }
	    break;
	default:
	    rb_raise(bdb_eFatal, "Invalid type for \"set_server\"");
	    break;
	}
#if BDB_VERSION >= 30300
	bdb_test_error(envp->set_rpc_server(envp, NULL, host, cl_timeout, sv_timeout, flags));
#else
	bdb_test_error(envp->set_server(envp, host, cl_timeout, sv_timeout, flags));
#endif
    }
#endif
#if BDB_VERSION >= 30200
    else if (strcmp(options, "set_flags") == 0) {
        bdb_test_error(envp->set_flags(envp, NUM2UINT(value), 1));
    }
#endif
    else if (strcmp(options, "marshal") == 0) {
        switch (value) {
        case Qtrue: envst->marshal = bdb_mMarshal; break;
        case Qfalse: envst->marshal = Qfalse; break;
        default: 
	    if (!rb_respond_to(value, bdb_id_load) ||
		!rb_respond_to(value, bdb_id_dump)) {
		rb_raise(bdb_eFatal, "marshal value must be true or false");
	    }
	    envst->marshal = value;
	    break;
        }
    }
    else if (strcmp(options, "thread") == 0) {
	if (RTEST(value)) {
	    envst->options &= ~BDB_NO_THREAD;
	}
	else {
	    envst->options |= BDB_NO_THREAD;
	}
    }
#if BDB_VERSION >= 40000
    else if (strcmp(options, "set_rep_transport") == 0) {
	if (TYPE(value) != T_ARRAY || RARRAY(value)->len != 2) {
	    rb_raise(bdb_eFatal, "expected an Array of length 2 for set_rep_transport");
	}
	if (!FIXNUM_P(RARRAY(value)->ptr[0])) {
	    rb_raise(bdb_eFatal, "expected a Fixnum for the 1st arg of set_rep_transport");
	}
	if (!rb_respond_to(RARRAY(value)->ptr[1], bdb_id_call)) {
	    rb_raise(bdb_eFatal, "2nd arg must respond to #call");
	}
	envst->rep_transport = RARRAY(value)->ptr[1];
	bdb_test_error(envst->envp->set_rep_transport(envst->envp, NUM2INT(RARRAY(value)->ptr[0]), bdb_env_rep_transport));
	envst->options |= BDB_REP_TRANSPORT;
    }
    else if (strcmp(options, "set_timeout") == 0) {
	if (TYPE(value) == T_ARRAY) {
	    if (RARRAY(value)->len >= 1 && !NIL_P(RARRAY(value)->ptr[0])) {

		bdb_test_error(envst->envp->set_timeout(envst->envp, 
							NUM2UINT(RARRAY(value)->ptr[0]),
							DB_SET_TXN_TIMEOUT));
	    }
	    if (RARRAY(value)->len == 2 && !NIL_P(RARRAY(value)->ptr[1])) {
		bdb_test_error(envst->envp->set_timeout(envst->envp, 
							NUM2UINT(RARRAY(value)->ptr[0]),
							DB_SET_LOCK_TIMEOUT));
	    }
	}
	else {
	    bdb_test_error(envst->envp->set_timeout(envst->envp,
						    NUM2UINT(value),
						    DB_SET_TXN_TIMEOUT));
	}
    }
    else if (strcmp(options, "set_txn_timeout") == 0) {
	bdb_test_error(envst->envp->set_timeout(envst->envp,
						NUM2UINT(value),
						DB_SET_TXN_TIMEOUT));
    }
    else if (strcmp(options, "set_lock_timeout") == 0) {
	bdb_test_error(envst->envp->set_timeout(envst->envp,
						NUM2UINT(value),
						DB_SET_LOCK_TIMEOUT));
    }
#endif
#if BDB_VERSION >= 40100
    else if (strcmp(options, "set_encrypt") == 0) {
	char *passwd;
	int flags = DB_ENCRYPT_AES;
	if (TYPE(value) == T_ARRAY) {
	    if (RARRAY(value)->len != 2) {
		rb_raise(bdb_eFatal, "Expected an Array with 2 values");
	    }
	    passwd = StringValuePtr(RARRAY(value)->ptr[0]);
	    flags = NUM2INT(RARRAY(value)->ptr[1]);
	}
	else {
	    passwd = StringValuePtr(value);
	}
	bdb_test_error(envst->envp->set_encrypt(envst->envp, passwd, flags));
	envst->options |= BDB_ENV_ENCRYPT;
    }
    else if (strcmp(options, "set_rep_limit") == 0) {
	u_int32_t gbytes, bytes;
	gbytes = bytes = 0;
	if (TYPE(value) == T_ARRAY) {
	    if (RARRAY(value)->len != 2) {
		rb_raise(bdb_eFatal, "Expected an Array with 2 values");
	    }
	    gbytes = NUM2UINT(RARRAY(value)->ptr[0]);
	    bytes = NUM2UINT(RARRAY(value)->ptr[1]);
	}
	else {
	    bytes = NUM2UINT(RARRAY(value)->ptr[1]);
	}
	bdb_test_error(envst->envp->set_rep_limit(envst->envp, gbytes, bytes));
    }
#endif
#if BDB_VERSION >= 30000
    else if (strcmp(options, "set_feedback") == 0) {
	if (!rb_respond_to(value, bdb_id_call)) {
	    rb_raise(bdb_eFatal, "arg must respond to #call");
	}
	envst->options |= BDB_FEEDBACK;
	envst->feedback = value;
	envst->envp->set_feedback(envst->envp, bdb_env_feedback);
    }
#endif
#if BDB_VERSION >= 40100
    else if (strcmp(options, "set_app_dispatch") == 0) {
	if (!rb_respond_to(value, bdb_id_call)) {
	    rb_raise(bdb_eFatal, "arg must respond to #call");
	}
	envst->options |= BDB_APP_DISPATCH;
	envst->app_dispatch = value;
	envst->envp->set_app_dispatch(envst->envp, bdb_env_app_dispatch);
    }
#endif
    return Qnil;
}

struct env_iv {
    bdb_ENV *envst;
    VALUE env;
};

static void
bdb_final(envst)
    bdb_ENV *envst;
{
    VALUE db, obj;
    bdb_ENV *thst;

    if (envst->db_ary) {
	while ((db = rb_ary_pop(envst->db_ary)) != Qnil) {
	    if (rb_respond_to(db, rb_intern("close"))) {
		rb_funcall(db, rb_intern("close"), 0, 0);
	    }
	}
	envst->db_ary = 0;
    }
    if (envst->envp) {
	struct env_iv *eiv;
	int i;

	if (!(envst->options & BDB_ENV_NOT_OPEN)) {
#if BDB_VERSION < 30000
	    db_appexit(envst->envp);
	    free(envst->envp);
#else
	    envst->envp->close(envst->envp, 0);
#endif
	}
	envst->envp = NULL;
	for (i = 0; i < RARRAY(bdb_env_internal_ary)->len; ++i) {
	    eiv = (struct env_iv *)DATA_PTR(RARRAY(bdb_env_internal_ary)->ptr[i]);
	    if (eiv->envst == envst) {
		rb_ary_delete_at(bdb_env_internal_ary, i);
		break;
	    }
	}
    }
#if BDB_VERSION >= 30000
    obj = rb_thread_local_aref(rb_thread_current(), bdb_id_current_env);
    if (obj != Qnil) {
	Data_Get_Struct(obj, bdb_ENV, thst);
	if (thst == envst) {
	    rb_thread_local_aset(rb_thread_current(), bdb_id_current_env, Qnil);
	}
    }
#endif
}

static void
bdb_env_free(envst)
    bdb_ENV *envst;
{
    bdb_final(envst);
    free(envst);
}

static VALUE
bdb_env_close(obj)
    VALUE obj;
{
    bdb_ENV *envst;
    VALUE db;
    int i;

    if (!OBJ_TAINTED(obj) && rb_safe_level() >= 4) {
	rb_raise(rb_eSecurityError, "Insecure: can't close the environnement");
    }
    GetEnvDB(obj, envst);
    bdb_final(envst);
    return Qnil;
}

static void
bdb_env_finalize(ary)
    VALUE ary;
{
    VALUE env;
    struct env_iv *eiv;

    while ((env = rb_ary_pop(bdb_env_internal_ary)) != Qnil) {
	Data_Get_Struct(env, struct env_iv, eiv);
	rb_funcall2(eiv->env, rb_intern("close"), 0, 0);
    }
}

static void
bdb_env_mark(envst)
    bdb_ENV *envst;
{
    rb_gc_mark(envst->marshal);
#if BDB_VERSION >= 40000
    rb_gc_mark(envst->rep_transport);
#if BDB_VERSION >= 40100
    rb_gc_mark(envst->app_dispatch);
#endif
#endif
#if BDB_VERSION >= 30000
    rb_gc_mark(envst->feedback);
#endif
    rb_gc_mark(envst->db_ary);
    rb_gc_mark(envst->home);
}

VALUE
bdb_env_open_db(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    VALUE cl;

    if (argc < 1)
        rb_raise(bdb_eFatal, "Invalid number of arguments");
    cl = argv[0];
    if (FIXNUM_P(cl)) {
	switch (NUM2INT(cl)) {
	case DB_BTREE: cl = bdb_cBtree; break;
	case DB_HASH: cl = bdb_cHash; break;
	case DB_RECNO: cl = bdb_cRecno; break;
#if BDB_VERSION >= 30000
	case DB_QUEUE: cl = bdb_cQueue; break;
#endif
	case DB_UNKNOWN: cl = bdb_cUnknown; break;
	default: rb_raise(bdb_eFatal, "Unknown database type");
	}
    }
    else if (TYPE(cl) != T_CLASS) {
	cl = CLASS_OF(cl);
    }
    MEMCPY(argv, argv + 1, VALUE, argc - 1);
    if (argc > 1 && TYPE(argv[argc - 2]) == T_HASH) {
	argc--;
    }
    else {
	argv[argc - 1] = rb_hash_new();
    }
    if (rb_obj_is_kind_of(obj, bdb_cEnv)) {
	rb_hash_aset(argv[argc - 1], rb_tainted_str_new2("env"), obj);
    }
    else {
	rb_hash_aset(argv[argc - 1], rb_tainted_str_new2("txn"), obj);
    }
    return rb_funcall2(cl, rb_intern("new"), argc, argv);
}

void
bdb_env_errcall(errpfx, msg)
    const char *errpfx;
    char *msg;
{
    bdb_errcall = 1;
    bdb_errstr = rb_tainted_str_new2(msg);
}

VALUE
bdb_return_err()
{
    if (bdb_errcall) {
	bdb_errcall = 0;
	return bdb_errstr;
    }
    return Qnil;
}

#ifndef NT
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
struct timeval {
    long    tv_sec;
    long    tv_usec;
};
#endif /* HAVE_SYS_TIME_H */
#endif /* NT */

static int
bdb_func_sleep(sec, usec)
    long sec;
    long usec;
{
    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;
    rb_thread_wait_for(timeout);
    return 0;
}

static int
bdb_func_yield()
{
    rb_thread_schedule();
    return 0;
}

static void *
bdb_func_malloc(size)
    size_t size;
{
    return malloc(size);
}

static VALUE
bdb_set_func(envst)
    bdb_ENV *envst;
{
#if BDB_VERSION >= 30000
#if BDB_VERSION < 30114
    bdb_test_error(envst->envp->set_func_sleep(envst->envp, bdb_func_sleep));
    bdb_test_error(envst->envp->set_func_yield(envst->envp, bdb_func_yield));
#else
    bdb_test_error(db_env_set_func_sleep(bdb_func_sleep));
    bdb_test_error(db_env_set_func_yield(bdb_func_yield));
#endif
#else
    bdb_test_error(db_jump_set((void *)bdb_func_sleep, DB_FUNC_SLEEP));
    bdb_test_error(db_jump_set((void *)bdb_func_yield, DB_FUNC_YIELD));
#endif
    return Qtrue;
}

static VALUE
bdb_env_each_options(opt, stobj)
    VALUE opt, stobj;
{
    VALUE res;
    DB_ENV *envp;
    struct db_stoptions *db_st;

    res = rb_iterate(rb_each, opt, bdb_env_i_options, stobj);
    Data_Get_Struct(stobj, struct db_stoptions, db_st);
    envp = db_st->env->envp;
#if BDB_VERSION >= 30000
    if (db_st->lg_bsize) {
	bdb_test_error(envp->set_lg_bsize(envp, db_st->lg_bsize));
    }
#endif
    if (db_st->lg_max) {
#if BDB_VERSION < 30000
	envp->lg_max = db_st->lg_max;
#else
	bdb_test_error(envp->set_lg_max(envp, db_st->lg_max));
#endif
    }
    return res;
}

static VALUE
bdb_env_s_i_options(obj, flags)
    VALUE obj;
    int *flags;
{
    char *options;
    VALUE key, value;

    key = rb_ary_entry(obj, 0);
    value = rb_ary_entry(obj, 1);
    key = rb_obj_as_string(key);
    options = StringValuePtr(key);
    if (strcmp(options, "env_flags") == 0) {
	*flags = NUM2INT(value);
    }
#ifdef DB_CLIENT
    else if (strcmp(options, "set_rpc_server") == 0 ||
	     strcmp(options, "set_server") == 0) {
	*flags |= DB_CLIENT;
    }
#endif
    return Qnil;
}

static VALUE
bdb_env_s_alloc(obj)
    VALUE obj;
{
    VALUE res;
    bdb_ENV *envst;

    res = Data_Make_Struct(obj, bdb_ENV, bdb_env_mark, bdb_env_free, envst);
    envst->options |= BDB_ENV_NOT_OPEN;
    return res;
}

static VALUE
bdb_env_s_new(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    bdb_ENV *envst;
    struct env_iv *eiv;
    VALUE res, final;
    int flags = 0;

    res = rb_funcall2(obj, rb_intern("allocate"), 0, 0);
    Data_Get_Struct(res, bdb_ENV, envst);
#if BDB_VERSION < 30000
    envst->envp = ALLOC(DB_ENV);
    MEMZERO(envst->envp, DB_ENV, 1);
    envst->envp->db_errpfx = "BDB::";
    envst->envp->db_errcall = bdb_env_errcall;
#else
#if BDB_VERSION >= 30100
    if (argc && TYPE(argv[argc - 1]) == T_HASH) {
	rb_iterate(rb_each, argv[argc - 1], bdb_env_s_i_options, (VALUE)&flags);
    }
#endif
    bdb_test_error(db_env_create(&(envst->envp), flags));
    envst->envp->set_errpfx(envst->envp, "BDB::");
    envst->envp->set_errcall(envst->envp, bdb_env_errcall);
#if BDB_VERSION >= 30300
    bdb_test_error(envst->envp->set_alloc(envst->envp, malloc, realloc, free));
#endif
#endif
    rb_obj_call_init(res, argc, argv);
    final = Data_Make_Struct(rb_cData, struct env_iv, 0, free, eiv);
    eiv->env = res;
    eiv->envst = envst;
    rb_ary_push(bdb_env_internal_ary, final);
    return res;
}


#if BDB_VERSION >= 40000

VALUE 
bdb_env_s_rslbl(int argc, VALUE *argv, VALUE obj, DB_ENV *env)
{
    bdb_ENV *envst;
    VALUE res;

    res = rb_funcall2(obj, rb_intern("allocate"), 0, 0);
    Data_Get_Struct(res, bdb_ENV, envst);
    envst->envp = env;
    envst->envp->set_errpfx(envst->envp, "BDB::");
    envst->envp->set_errcall(envst->envp, bdb_env_errcall);
    bdb_test_error(envst->envp->set_alloc(envst->envp, malloc, realloc, free));
    rb_obj_call_init(res, argc, argv);
    return res;
}

#endif

static VALUE
bdb_env_init(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    DB_ENV *envp;
    bdb_ENV *envst;
    VALUE a, c, d;
    char *db_home, **db_config;
    int ret, mode, flags;
    VALUE st_config;
    VALUE envid;

    envid = 0;
    st_config = 0;
    db_config = 0;
    mode = flags = 0;
    Data_Get_Struct(obj, bdb_ENV, envst);
    envp = envst->envp;
#if BDB_VERSION >= 40100
    if (rb_const_defined(CLASS_OF(obj), rb_intern("BDB_ENCRYPT"))) {
	char *passwd;
	int flags = DB_ENCRYPT_AES;
	VALUE value = rb_const_get(CLASS_OF(obj), rb_intern("BDB_ENCRYPT"));
	if (TYPE(value) == T_ARRAY) {
	    if (RARRAY(value)->len != 2) {
		rb_raise(bdb_eFatal, "Expected an Array with 2 values");
	    }
	    passwd = StringValuePtr(RARRAY(value)->ptr[0]);
	    flags = NUM2INT(RARRAY(value)->ptr[1]);
	}
	else {
	    passwd = StringValuePtr(value);
	}
	bdb_test_error(envp->set_encrypt(envp, passwd, flags));
	envst->options |= BDB_ENV_ENCRYPT;
    }
#endif
    if (argc && TYPE(argv[argc - 1]) == T_HASH) {
	int i;
	VALUE db_stobj;
	struct db_stoptions *db_st;

	st_config = rb_ary_new();
	db_stobj = Data_Make_Struct(rb_cObject, struct db_stoptions, 0, free, db_st);
	db_st->env = envst;
	db_st->config = st_config;
	bdb_env_each_options(argv[argc - 1], db_stobj);
	if (RARRAY(st_config)->len > 0) {
	    db_config = ALLOCA_N(char *, RARRAY(st_config)->len + 1);
	    for (i = 0; i < RARRAY(st_config)->len; i++) {
		db_config[i] = StringValuePtr(RARRAY(st_config)->ptr[i]);
	    }
	    db_config[RARRAY(st_config)->len] = 0;
	}
	argc--;
    }
    rb_scan_args(argc, argv, "12", &a, &c, &d);
    SafeStringValue(a);
    db_home = StringValuePtr(a);
    switch (argc) {
    case 3:
	mode = NUM2INT(d);
    case 2:
	flags = NUM2INT(c);
        break;
    }
    if (flags & DB_CREATE) {
	rb_secure(4);
    }
    if (flags & DB_USE_ENVIRON) {
	rb_secure(1);
    }
#ifndef BDB_NO_THREAD_COMPILE
    if (!(envst->options & BDB_NO_THREAD)) {
	bdb_set_func(envst);
	flags |= DB_THREAD;
    }
#endif
#if BDB_VERSION < 30000
    if ((ret = db_appinit(db_home, db_config, envp, flags)) != 0) {
	if (envst->envp) {
	    free(envst->envp);
	}
	envst->envp = NULL;
        if (bdb_errcall) {
            bdb_errcall = 0;
            rb_raise(bdb_eFatal, "%s -- %s", StringValuePtr(bdb_errstr), db_strerror(ret));
        }
        else
            rb_raise(bdb_eFatal, "%s", db_strerror(ret));
    }
#else
#if BDB_VERSION >= 40000
    if (envst->rep_transport == 0 && rb_respond_to(obj, rb_intern("bdb_rep_transport")) == Qtrue) {
	if (!rb_const_defined(CLASS_OF(obj), rb_intern("ENVID"))) {
	    rb_raise(bdb_eFatal, "ENVID must be defined to use rep_transport");
	}
	envid = rb_const_get(CLASS_OF(obj), rb_intern("ENVID"));
	bdb_test_error(envp->set_rep_transport(envp, NUM2INT(envid),
						 bdb_env_rep_transport));
	envst->options |= BDB_REP_TRANSPORT;
    }
#endif
#if BDB_VERSION >= 30000
    if (envst->feedback == 0 && rb_respond_to(obj, id_feedback) == Qtrue) {
	envp->set_feedback(envp, bdb_env_feedback);
	envst->options |= BDB_FEEDBACK;
    }
#endif
#if BDB_VERSION >= 40100
    if (envst->app_dispatch == 0 && rb_respond_to(obj, id_app_dispatch) == Qtrue) {
	envp->set_app_dispatch(envp, bdb_env_app_dispatch);
	envst->options |= BDB_APP_DISPATCH;
    }
#endif
#if BDB_VERSION >= 30100
    if ((ret = envp->open(envp, db_home, flags, mode)) != 0)
#else
    if ((ret = envp->open(envp, db_home, db_config, flags, mode)) != 0)
#endif
    {
        envp->close(envp, 0);
	envst->envp = NULL;
        if (bdb_errcall) {
            bdb_errcall = 0;
            rb_raise(bdb_eFatal, "%s -- %s", StringValuePtr(bdb_errstr), db_strerror(ret));
        }
        else
            rb_raise(bdb_eFatal, "%s", db_strerror(ret));
    }
#endif
    envst->options &= ~BDB_ENV_NOT_OPEN;
    envst->db_ary = rb_ary_new2(0);
    if (flags & DB_INIT_LOCK) {
	envst->options |= BDB_INIT_LOCK;
    }
    if (flags & DB_INIT_TXN) {
	envst->options |= BDB_AUTO_COMMIT;
    }
    envst->home = rb_tainted_str_new2(db_home);
    OBJ_FREEZE(envst->home);
#if BDB_VERSION >= 30000
    if (envst->options & BDB_NEED_ENV_CURRENT) {
	rb_thread_local_aset(rb_thread_current(), bdb_id_current_env, obj);
    }
#endif
    return obj;
}

static VALUE
bdb_env_internal_close(obj)
    VALUE obj;
{
    return rb_funcall2(obj, rb_intern("close"), 0, 0);
}

static VALUE
bdb_env_s_open(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    VALUE res = rb_funcall2(obj, rb_intern("new"), argc, argv);
    if (rb_block_given_p()) {
	return rb_ensure(rb_yield, res, bdb_env_internal_close, res);
    }
    return res;
}

static VALUE
bdb_env_s_remove(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    DB_ENV *envp;
    VALUE a, b;
    char *db_home;
    int flag = 0;

    rb_secure(2);
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	flag = NUM2INT(b);
    }
    db_home = StringValuePtr(a);
#if BDB_VERSION < 30000
    envp = ALLOCA_N(DB_ENV, 1);
    MEMZERO(envp, DB_ENV, 1);
    envp->db_errpfx = "BDB::";
    envp->db_errcall = bdb_env_errcall;
    if (lock_unlink(db_home, flag, envp) == EBUSY) {
	rb_raise(bdb_eFatal, "The shared memory region was in use");
    }
    if (log_unlink(db_home, flag, envp) == EBUSY) {
	rb_raise(bdb_eFatal, "The shared memory region was in use");
    }
    if (memp_unlink(db_home, flag, envp) == EBUSY) {
	rb_raise(bdb_eFatal, "The shared memory region was in use");
    }
    if (txn_unlink(db_home, flag, envp) == EBUSY) {
	rb_raise(bdb_eFatal, "The shared memory region was in use");
    }
#else
    bdb_test_error(db_env_create(&envp, 0));
    envp->set_errpfx(envp, "BDB::");
    envp->set_errcall(envp, bdb_env_errcall);
#if BDB_VERSION < 30100
    bdb_test_error(envp->remove(envp, db_home, NULL, flag));
#else
    bdb_test_error(envp->remove(envp, db_home, flag));
#endif
#endif
    return Qtrue;
}

static VALUE
bdb_env_set_flags(argc, argv, obj)
    int argc;
    VALUE *argv, obj;
{
#if BDB_VERSION >= 30200
    bdb_ENV *envst;
    VALUE opt, flag;
    int state = 1;

    GetEnvDB(obj, envst);
    if (rb_scan_args(argc, argv, "11", &flag, &opt)) {
	switch (TYPE(opt)) {
	case T_TRUE:
	    state = 1;
	    break;
	case T_FALSE:
	    state = 0;
	    break;
	case T_FIXNUM:
	    state = NUM2INT(opt);
	    break;
	default:
	    rb_raise(bdb_eFatal, "invalid value for onoff");
	}
    }
    bdb_test_error(envst->envp->set_flags(envst->envp, NUM2INT(flag), state));
#endif
    return Qnil;
}

static VALUE
bdb_env_home(obj)
    VALUE obj;
{
    bdb_ENV *envst;
    GetEnvDB(obj, envst);
    return envst->home;
}

#if BDB_VERSION >= 40000

static VALUE
bdb_env_iterate(tmp)
    VALUE *tmp;
{
    return rb_funcall2(tmp[0], rb_intern("__bdb_thread_init__"), 
		       (int)tmp[1], (VALUE *)tmp[2]);
}

static VALUE
bdb_thread_init(argc, argv, obj)
    int argc;
    VALUE *argv, obj;
{
    VALUE env;

    if ((env = rb_thread_local_aref(rb_thread_current(), bdb_id_current_env)) != Qnil) {
	rb_thread_local_aset(obj, bdb_id_current_env, env);
    }
    if (rb_block_given_p()) {
	VALUE tmp[3];
	tmp[0] = obj;
	tmp[1] = (VALUE)argc;
	tmp[2] = (VALUE)argv;
	return rb_iterate(bdb_env_iterate, (VALUE)tmp, rb_yield, obj);
    }
    return rb_funcall2(obj, rb_intern("__bdb_thread_init__"), argc, argv);
}

#endif

#if BDB_VERSION >= 30000

static VALUE
bdb_env_feedback_set(VALUE obj, VALUE a)
{
    bdb_ENV *envst;

    GetEnvDB(obj, envst);
    if (NIL_P(a)) {
	envst->feedback = a;
    }
    else {
	if (!rb_respond_to(a, bdb_id_call)) {
	    rb_raise(bdb_eFatal, "arg must respond to #call");
	}
	envst->feedback = a;
	if (!(envst->options & BDB_NEED_ENV_CURRENT)) {
	    envst->options |= BDB_FEEDBACK;
	    rb_thread_local_aset(rb_thread_current(), bdb_id_current_env, obj);
	}
    }
    return a;
}

#endif

void bdb_init_env()
{
    bdb_id_call = rb_intern("call");
#if BDB_VERSION >= 30000
    id_feedback = rb_intern("bdb_feedback");
#endif
    bdb_id_current_env = rb_intern("bdb_current_env");
#if BDB_VERSION >= 40100
    id_app_dispatch = rb_intern("bdb_app_dispatch");
#endif

    bdb_env_internal_ary = rb_ary_new();
    rb_set_end_proc(bdb_env_finalize, bdb_env_internal_ary);

    bdb_cEnv = rb_define_class_under(bdb_mDb, "Env", rb_cObject);
    rb_define_private_method(bdb_cEnv, "initialize", bdb_env_init, -1);
#if RUBY_VERSION_CODE >= 180
    rb_define_alloc_func(bdb_cEnv, bdb_env_s_alloc);
#else
    rb_define_singleton_method(bdb_cEnv, "allocate", bdb_env_s_alloc, 0);
#endif
    rb_define_singleton_method(bdb_cEnv, "new", bdb_env_s_new, -1);
    rb_define_singleton_method(bdb_cEnv, "create", bdb_env_s_new, -1);
    rb_define_singleton_method(bdb_cEnv, "open", bdb_env_s_open, -1);
    rb_define_singleton_method(bdb_cEnv, "remove", bdb_env_s_remove, -1);
    rb_define_singleton_method(bdb_cEnv, "unlink", bdb_env_s_remove, -1);
    rb_define_method(bdb_cEnv, "open_db", bdb_env_open_db, -1);
    rb_define_method(bdb_cEnv, "close", bdb_env_close, 0);
    rb_define_method(bdb_cEnv, "set_flags", bdb_env_set_flags, -1);
    rb_define_method(bdb_cEnv, "home", bdb_env_home, 0);
#if BDB_VERSION >= 40000
    rb_define_method(bdb_cEnv, "rep_elect", bdb_env_rep_elect, 3);
    rb_define_method(bdb_cEnv, "elect", bdb_env_rep_elect, 3);
    rb_define_method(bdb_cEnv, "rep_process_message", bdb_env_rep_process_message, 3);
    rb_define_method(bdb_cEnv, "process_message", bdb_env_rep_process_message, 3);
    rb_define_method(bdb_cEnv, "rep_start", bdb_env_rep_start, 2);
    if (!rb_method_boundp(rb_cThread, rb_intern("__bdb_thread_init__"), 1)) {
	rb_alias(rb_cThread, rb_intern("__bdb_thread_init__"), rb_intern("initialize"));
	rb_define_method(rb_cThread, "initialize", bdb_thread_init, -1);
    }
#if BDB_VERSION >= 40100
    rb_define_method(bdb_cEnv, "rep_limit=", bdb_env_rep_limit, -1);
#endif
#endif
#if BDB_VERSION >= 30000
    rb_define_method(bdb_cEnv, "feedback=", bdb_env_feedback_set, 1);
#endif
}
