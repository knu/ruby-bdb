#include "bdb.h"

ID bdb_id_call;

#if DB_VERSION_MAJOR >= 3
static ID id_feedback;
#endif

ID bdb_id_current_env;

#if DB_VERSION_MAJOR == 4 & DB_VERSION_MINOR >= 1
static ID id_app_dispatch;
#endif

struct db_stoptions {
    bdb_ENV *env;
    VALUE config;
    int lg_max, lg_bsize;
};

#if DB_VERSION_MAJOR >= 4

static int
bdb_env_rep_transport(dbenv, control, rec, envid, flags)
    DB_ENV *dbenv;
    const DBT *control;
    const DBT *rec;
    int envid;
    u_int32_t flags;
{
    VALUE obj, av, bv, res;
    bdb_ENV *dbenvst;

    if ((obj = rb_thread_local_aref(rb_thread_current(), bdb_id_current_env)) == Qnil) {
	rb_raise(bdb_eFatal, "BUG : current_env not set");
    }
    GetEnvDB(obj, dbenvst);
    av = rb_tainted_str_new(control->data, control->size);
    bv = rb_tainted_str_new(rec->data, rec->size);
    if (dbenvst->rep_transport == 0) {
	res = rb_funcall(obj, rb_intern("bdb_rep_transport"), 4, av, bv,
			 INT2FIX(envid), INT2FIX(flags));
    }
    else {
	res = rb_funcall(dbenvst->rep_transport, bdb_id_call, 4, 
			 av, bv, INT2FIX(envid), INT2FIX(flags));
    }
    return NUM2INT(res);
}

static VALUE
bdb_env_rep_elect(env, nb, pri, ti)
    VALUE env, nb, pri, ti;
{
    bdb_ENV *dbenvst;
    int envid = 0;

    GetEnvDB(env, dbenvst);
    bdb_test_error(dbenvst->dbenvp->rep_elect(dbenvst->dbenvp, NUM2INT(nb),
					      NUM2INT(pri), NUM2INT(ti),
					      &envid));
    return INT2NUM(envid);
}

static VALUE
bdb_env_rep_process_message(env, av, bv, ev)
    VALUE env, av, bv, ev;
{
    bdb_ENV *dbenvst;
    DBT control, rec;
    int ret, envid;
    VALUE result;

    GetEnvDB(env, dbenvst);
    av = rb_str_to_str(av);
    bv = rb_str_to_str(bv);
    MEMZERO(&control, DBT, 1);
    MEMZERO(&rec, DBT, 1);
    control.size = RSTRING(av)->len;
    control.data = RSTRING(av)->ptr;
    rec.size = RSTRING(bv)->len;
    rec.data = RSTRING(bv)->ptr;
    envid = NUM2INT(ev);
    ret = dbenvst->dbenvp->rep_process_message(dbenvst->dbenvp,
					       &control, &rec,
					       &envid);
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
    bdb_ENV *dbenvst;
    DBT cdata;

    GetEnvDB(env, dbenvst);
    if (!NIL_P(ident)) {
	ident = rb_str_to_str(ident);
	MEMZERO(&cdata, DBT, 1);
	cdata.size = RSTRING(ident)->len;
	cdata.data = RSTRING(ident)->ptr;
    }
    bdb_test_error(dbenvst->dbenvp->rep_start(dbenvst->dbenvp, 
					      NIL_P(ident)?NULL:&cdata,
					      NUM2INT(flags)));
    return Qnil;
}

#if DB_VERSION_MINOR >= 1
static VALUE
bdb_env_rep_limit(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    bdb_ENV *dbenvst;
    VALUE a, b;
    u_int32_t gbytes, bytes;

    GetEnvDB(obj, dbenvst);
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
    bdb_test_error(dbenvst->dbenvp->set_rep_limit(dbenvst->dbenvp,
						  gbytes, bytes));
    return obj;
}
#endif
 
#endif

#if DB_VERSION_MAJOR >= 3
static void
bdb_env_feedback(DB_ENV *envp, int opcode, int pct)
{
    VALUE obj;
    bdb_ENV *dbenvst;
    if ((obj = rb_thread_local_aref(rb_thread_current(), bdb_id_current_env)) == Qnil) {
	rb_raise(bdb_eFatal, "BUG : current_env not set");
    }
    Data_Get_Struct(obj, bdb_ENV, dbenvst);
    if (NIL_P(dbenvst->feedback)) {
	return;
    }
    if (dbenvst->feedback == 0) {
	rb_funcall(obj, id_feedback, 2, INT2NUM(opcode), INT2NUM(pct));
    }
    else {
	rb_funcall(dbenvst->feedback, bdb_id_call, 2, INT2NUM(opcode), INT2NUM(pct));
    }
}

#endif

#if DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 1

static int
bdb_env_app_dispatch(DB_ENV *envp, DBT *log_rec, DB_LSN *lsn, db_recops op)
{
    VALUE obj, lsnobj, logobj, res;
    bdb_ENV *dbenvst;
    struct dblsnst *lsnst;

    if ((obj = rb_thread_local_aref(rb_thread_current(), bdb_id_current_env)) == Qnil) {
	rb_raise(bdb_eFatal, "BUG : current_env not set");
    }
    lsnobj = bdb_makelsn(obj);
    Data_Get_Struct(lsnobj, struct dblsnst, lsnst);
    MEMCPY(lsnst->lsn, lsn, DB_LSN, 1);
    logobj = rb_str_new(log_rec->data, log_rec->size);
    if (dbenvst->app_dispatch == 0) {
	res = rb_funcall(obj, id_app_dispatch, 3, logobj, lsnobj, INT2NUM(op));
    }
    else {
	res = rb_funcall(dbenvst->app_dispatch, bdb_id_call, 3, logobj, lsnobj, INT2NUM(op));
    }
    return NUM2INT(res);
}

#endif

static VALUE
bdb_env_i_options(obj, db_stobj)
    VALUE obj, db_stobj;
{
    char *options;
    DB_ENV *dbenvp;
    VALUE key, value;
    bdb_ENV *dbenvst;
    struct db_stoptions *db_st;

    Data_Get_Struct(db_stobj, struct db_stoptions, db_st);
    dbenvst = db_st->env;

    key = rb_ary_entry(obj, 0);
    value = rb_ary_entry(obj, 1);
    dbenvp = dbenvst->dbenvp;
    key = rb_obj_as_string(key);
    options = RSTRING(key)->ptr;
    if (strcmp(options, "set_cachesize") == 0) {
#if DB_VERSION_MAJOR < 3
        dbenvp->mp_size = NUM2INT(value);
#else
        Check_Type(value, T_ARRAY);
        if (RARRAY(value)->len < 3)
            rb_raise(bdb_eFatal, "expected 3 values for cachesize");
        bdb_test_error(dbenvp->set_cachesize(dbenvp, 
                                      NUM2INT(RARRAY(value)->ptr[0]),
                                      NUM2INT(RARRAY(value)->ptr[1]),
                                      NUM2INT(RARRAY(value)->ptr[2])));
#endif
    }
#if DB_VERSION_MAJOR == 3
    else if (strcmp(options, "set_region_init") == 0) {
#if DB_VERSION_MAJOR == 3 && \
    ((DB_VERSION_MINOR >=1 && DB_VERSION_PATCH >= 14) || \
      DB_VERSION_MINOR >= 2)
        bdb_test_error(db_env_set_region_init(NUM2INT(value)));
#else
        bdb_test_error(dbenvp->set_region_init(dbenvp, NUM2INT(value)));
#endif
    }
#endif
#if DB_VERSION_MAJOR >= 3
    else if (strcmp(options, "set_tas_spins") == 0) {
#if DB_VERSION_MAJOR == 3 && \
    ((DB_VERSION_MINOR >=1 && DB_VERSION_PATCH >= 14) || \
      DB_VERSION_MINOR >= 2)
        bdb_test_error(db_env_set_tas_spins(NUM2INT(value)));
#else
        bdb_test_error(dbenvp->set_tas_spins(dbenvp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_tx_max") == 0) {
        bdb_test_error(dbenvp->set_tx_max(dbenvp, NUM2INT(value)));
    }
#if DB_VERSION_MAJOR >= 4 || (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1)
    else if (strcmp(options, "set_tx_timestamp") == 0) {
	time_t ti;
	value = rb_Integer(value);
	ti = (time_t)NUM2INT(value);
        bdb_test_error(dbenvp->set_tx_timestamp(dbenvp, &ti));
    }
#endif
#endif
#if DB_VERSION_MAJOR < 3
    else if  (strcmp(options, "set_verbose") == 0) {
        dbenvp->db_verbose = NUM2INT(value);
    }
#else
    else if (strcmp(options, "set_verb_chkpoint") == 0) {
        bdb_test_error(dbenvp->set_verbose(dbenvp, DB_VERB_CHKPOINT, NUM2INT(value)));
    }
    else if (strcmp(options, "set_verb_deadlock") == 0) {
        bdb_test_error(dbenvp->set_verbose(dbenvp, DB_VERB_DEADLOCK, NUM2INT(value)));
    }
    else if (strcmp(options, "set_verb_recovery") == 0) {
        bdb_test_error(dbenvp->set_verbose(dbenvp, DB_VERB_RECOVERY, NUM2INT(value)));
    }
    else if (strcmp(options, "set_verb_waitsfor") == 0) {
        bdb_test_error(dbenvp->set_verbose(dbenvp, DB_VERB_WAITSFOR, NUM2INT(value)));
    }
#if DB_VERSION_MAJOR >= 4
    else if (strcmp(options, "set_verb_replication") == 0) {
        bdb_test_error(dbenvp->set_verbose(dbenvp, DB_VERB_REPLICATION, NUM2INT(value)));
    }
#endif
#endif
    else if (strcmp(options, "set_lk_detect") == 0) {
#if DB_VERSION_MAJOR < 3
	dbenvp->lk_detect = NUM2INT(value);
#else
        bdb_test_error(dbenvp->set_lk_detect(dbenvp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_lk_max") == 0) {
#if DB_VERSION_MAJOR < 3
	dbenvp->lk_max = NUM2INT(value);
#else
        bdb_test_error(dbenvp->set_lk_max(dbenvp, NUM2INT(value)));
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
#if DB_VERSION_MAJOR < 3
	dbenvp->lk_modes = l;
	dbenvp->lk_conflicts = conflits;
#else
        bdb_test_error(dbenvp->set_lk_conflicts(dbenvp, conflits, l));
#endif
    }
    else if (strcmp(options, "set_lg_max") == 0) {
	db_st->lg_max = NUM2INT(value);
    }
#if DB_VERSION_MAJOR >= 3
    else if (strcmp(options, "set_lg_bsize") == 0) {
	db_st->lg_bsize = NUM2INT(value);
    }
    else if (strcmp(options, "set_data_dir") == 0) {
	char *tmp;

	Check_SafeStr(value);
#if DB_VERSION_MINOR >= 1 || DB_VERSION_MAJOR >= 4
	bdb_test_error(dbenvp->set_data_dir(dbenvp, RSTRING(value)->ptr));
#else
	tmp = ALLOCA_N(char, strlen("set_data_dir") + strlen(RSTRING(value)->ptr) + 2);
	sprintf(tmp, "DB_DATA_DIR %s", RSTRING(value)->ptr);
	rb_ary_push(db_st->config, rb_str_new2(tmp));
#endif
    }
    else if (strcmp(options, "set_lg_dir") == 0) {
	char *tmp;

	Check_SafeStr(value);
#if DB_VERSION_MINOR >= 1 || DB_VERSION_MAJOR >= 4
	bdb_test_error(dbenvp->set_lg_dir(dbenvp, RSTRING(value)->ptr));
#else
	tmp = ALLOCA_N(char, strlen("set_lg_dir") + strlen(RSTRING(value)->ptr) + 2);
	sprintf(tmp, "DB_LOG_DIR %s", RSTRING(value)->ptr);
	rb_ary_push(db_st->config, rb_str_new2(tmp));
#endif
    }
    else if (strcmp(options, "set_tmp_dir") == 0) {
	char *tmp;

	Check_SafeStr(value);
#if DB_VERSION_MINOR >= 1 || DB_VERSION_MAJOR >= 4
	bdb_test_error(dbenvp->set_tmp_dir(dbenvp, RSTRING(value)->ptr));
#else
	tmp = ALLOCA_N(char, strlen("set_tmp_dir") + strlen(RSTRING(value)->ptr) + 2);
	sprintf(tmp, "DB_TMP_DIR %s", RSTRING(value)->ptr);
	rb_ary_push(db_st->config, rb_str_new2(tmp));
#endif
    }
#if DB_VERSION_MINOR >= 1 || DB_VERSION_MAJOR >= 4
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
	    Check_SafeStr(value);
	    host = RSTRING(value)->ptr;
	    break;
	case T_ARRAY:
	    switch (RARRAY(value)->len) {
	    default:
	    case 3:
		sv_timeout = NUM2INT(RARRAY(value)->ptr[2]);
	    case 2:
		cl_timeout = NUM2INT(RARRAY(value)->ptr[1]);
	    case 1:
		Check_SafeStr(RARRAY(value)->ptr[0]);
		host = RSTRING(RARRAY(value)->ptr[0])->ptr;
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
#if DB_VERSION_MINOR >= 3 || DB_VERSION_MAJOR >= 4
	bdb_test_error(dbenvp->set_rpc_server(dbenvp, NULL, host, cl_timeout, sv_timeout, flags));
#else
	bdb_test_error(dbenvp->set_server(dbenvp, host, cl_timeout, sv_timeout, flags));
#endif
    }
#endif

#endif
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 2) || DB_VERSION_MAJOR >= 4
    else if (strcmp(options, "set_flags") == 0) {
        bdb_test_error(dbenvp->set_flags(dbenvp, NUM2UINT(value), 1));
    }
#endif
    else if (strcmp(options, "marshal") == 0) {
        switch (value) {
        case Qtrue: dbenvst->marshal = bdb_mMarshal; break;
        case Qfalse: dbenvst->marshal = Qfalse; break;
        default: 
	    if (!rb_respond_to(value, bdb_id_load) ||
		!rb_respond_to(value, bdb_id_dump)) {
		rb_raise(bdb_eFatal, "marshal value must be true or false");
	    }
	    dbenvst->marshal = value;
	    break;
        }
    }
    else if (strcmp(options, "thread") == 0) {
	if (RTEST(value)) {
	    dbenvst->options &= ~BDB_NO_THREAD;
	}
	else {
	    dbenvst->options |= BDB_NO_THREAD;
	}
    }
#if DB_VERSION_MAJOR >= 4
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
	dbenvst->rep_transport = RARRAY(value)->ptr[1];
	bdb_test_error(dbenvst->dbenvp->set_rep_transport(dbenvst->dbenvp, NUM2INT(RARRAY(value)->ptr[0]), bdb_env_rep_transport));
	dbenvst->options |= BDB_REP_TRANSPORT;
    }
    else if (strcmp(options, "set_timeout") == 0) {
	if (TYPE(value) == T_ARRAY) {
	    if (RARRAY(value)->len >= 1 && !NIL_P(RARRAY(value)->ptr[0])) {

		bdb_test_error(dbenvst->dbenvp->set_timeout(dbenvst->dbenvp, 
							    NUM2UINT(RARRAY(value)->ptr[0]),
							    DB_SET_TXN_TIMEOUT));
	    }
	    if (RARRAY(value)->len == 2 && !NIL_P(RARRAY(value)->ptr[1])) {
		bdb_test_error(dbenvst->dbenvp->set_timeout(dbenvst->dbenvp, 
							    NUM2UINT(RARRAY(value)->ptr[0]),
							    DB_SET_LOCK_TIMEOUT));
	    }
	}
	else {
	    bdb_test_error(dbenvst->dbenvp->set_timeout(dbenvst->dbenvp,
							NUM2UINT(value),
							 DB_SET_TXN_TIMEOUT));
	}
    }
    else if (strcmp(options, "set_txn_timeout") == 0) {
	bdb_test_error(dbenvst->dbenvp->set_timeout(dbenvst->dbenvp,
						    NUM2UINT(value),
						    DB_SET_TXN_TIMEOUT));
    }
    else if (strcmp(options, "set_lock_timeout") == 0) {
	bdb_test_error(dbenvst->dbenvp->set_timeout(dbenvst->dbenvp,
						    NUM2UINT(value),
						    DB_SET_LOCK_TIMEOUT));
    }
#endif
#if DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 1
    else if (strcmp(options, "set_encrypt") == 0) {
	char *passwd;
	int flags = DB_ENCRYPT_AES;
	if (TYPE(value) == T_ARRAY) {
	    if (RARRAY(value)->len != 2) {
		rb_raise(bdb_eFatal, "Expected an Array with 2 values");
	    }
	    passwd = STR2CSTR(RARRAY(value)->ptr[0]);
	    flags = NUM2INT(RARRAY(value)->ptr[1]);
	}
	else {
	    passwd = STR2CSTR(value);
	}
	bdb_test_error(dbenvst->dbenvp->set_encrypt(dbenvst->dbenvp,
						    passwd, flags));
	dbenvst->options |= BDB_ENV_ENCRYPT;
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
	bdb_test_error(dbenvst->dbenvp->set_rep_limit(dbenvst->dbenvp,
						      gbytes, bytes));
    }
#endif
#if DB_VERSION_MAJOR >= 3
    else if (strcmp(options, "set_feedback") == 0) {
	if (!rb_respond_to(value, bdb_id_call)) {
	    rb_raise(bdb_eFatal, "arg must respond to #call");
	}
	dbenvst->options |= BDB_FEEDBACK;
	dbenvst->feedback = value;
	dbenvst->dbenvp->set_feedback(dbenvst->dbenvp, bdb_env_feedback);
    }
#endif
#if DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 1
    else if (strcmp(options, "set_app_dispatch") == 0) {
	if (!rb_respond_to(value, bdb_id_call)) {
	    rb_raise(bdb_eFatal, "arg must respond to #call");
	}
	dbenvst->options |= BDB_APP_DISPATCH;
	dbenvst->app_dispatch = value;
	dbenvst->dbenvp->set_app_dispatch(dbenvst->dbenvp, bdb_env_app_dispatch);
    }
#endif
    return Qnil;
}

static void
bdb_final(dbenvst)
    bdb_ENV *dbenvst;
{
    VALUE db, obj;
    bdb_ENV *thst;

    if (dbenvst->db_ary) {
	while ((db = rb_ary_pop(dbenvst->db_ary)) != Qnil) {
	    if (rb_respond_to(db, rb_intern("close"))) {
		rb_funcall(db, rb_intern("close"), 0, 0);
	    }
	}
	dbenvst->db_ary = 0;
    }
    if (dbenvst->dbenvp) {
#if DB_VERSION_MAJOR < 3
	db_appexit(dbenvst->dbenvp);
	free(dbenvst->dbenvp);
#else
	dbenvst->dbenvp->close(dbenvst->dbenvp, 0);
#endif
	dbenvst->dbenvp = NULL;
    }
#if DB_VERSION_MAJOR >= 3
    obj = rb_thread_local_aref(rb_thread_current(), bdb_id_current_env);
    if (obj != Qnil) {
	Data_Get_Struct(obj, bdb_ENV, thst);
	if (thst == dbenvst) {
	    rb_thread_local_aset(rb_thread_current(), bdb_id_current_env, Qnil);
	}
    }
#endif
}

static void
bdb_env_free(dbenvst)
    bdb_ENV *dbenvst;
{
    bdb_final(dbenvst);
    free(dbenvst);
}

static VALUE
bdb_env_close(obj)
    VALUE obj;
{
    bdb_ENV *dbenvst;
    VALUE db;

    if (!OBJ_TAINTED(obj) && rb_safe_level() >= 4)
	rb_raise(rb_eSecurityError, "Insecure: can't close the environnement");
    GetEnvDB(obj, dbenvst);
    bdb_final(dbenvst);
    return Qnil;
}

static void
bdb_env_mark(dbenvst)
    bdb_ENV *dbenvst;
{
    if (dbenvst->marshal) rb_gc_mark(dbenvst->marshal);
#if DB_VERSION_MAJOR >= 4
    if (dbenvst->rep_transport) rb_gc_mark(dbenvst->rep_transport);
#if DB_VERSION_MINOR >= 1
    if (dbenvst->app_dispatch) rb_gc_mark(dbenvst->app_dispatch);
#endif
#endif
#if DB_VERSION_MAJOR >= 3
    if (dbenvst->feedback) rb_gc_mark(dbenvst->feedback);
#endif
    if (dbenvst->db_ary) rb_gc_mark(dbenvst->db_ary);
    rb_gc_mark(dbenvst->home);
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
#if DB_VERSION_MAJOR >= 3
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
bdb_set_func(dbenvst)
    bdb_ENV *dbenvst;
{
#if DB_VERSION_MAJOR >= 3
#if (DB_VERSION_MINOR >=1 && DB_VERSION_PATCH >= 14) || DB_VERSION_MINOR >= 2 || DB_VERSION_MAJOR >= 4
    bdb_test_error(db_env_set_func_sleep(bdb_func_sleep));
    bdb_test_error(db_env_set_func_yield(bdb_func_yield));
#else
    bdb_test_error(dbenvst->dbenvp->set_func_sleep(dbenvst->dbenvp, bdb_func_sleep));
    bdb_test_error(dbenvst->dbenvp->set_func_yield(dbenvst->dbenvp, bdb_func_yield));
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
    DB_ENV *dbenvp;
    struct db_stoptions *db_st;

    res = rb_iterate(rb_each, opt, bdb_env_i_options, stobj);
    Data_Get_Struct(stobj, struct db_stoptions, db_st);
    dbenvp = db_st->env->dbenvp;
#if DB_VERSION_MAJOR >= 3
    if (db_st->lg_bsize) {
	bdb_test_error(dbenvp->set_lg_bsize(dbenvp, db_st->lg_bsize));
    }
#endif
    if (db_st->lg_max) {
#if DB_VERSION_MAJOR < 3
	dbenvp->lg_max = db_st->lg_max;
#else
	bdb_test_error(dbenvp->set_lg_max(dbenvp, db_st->lg_max));
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
    options = RSTRING(key)->ptr;
    if (strcmp(options, "env_flags") == 0) {
	*flags = NUM2INT(value);
    }
#ifdef DB_CLIENT
    else if (strcmp(options, "set_rpc_server") == 0 ||
	     strcmp(options, "set_server")) {
	*flags |= DB_CLIENT;
    }
#endif
    return Qnil;
}

static VALUE
bdb_env_s_alloc(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    bdb_ENV *dbenvst;
    VALUE res;
    int flags = 0;

    res = Data_Make_Struct(obj, bdb_ENV, bdb_env_mark, bdb_env_free, dbenvst);
#if DB_VERSION_MAJOR < 3
    dbenvst->dbenvp = ALLOC(DB_ENV);
    MEMZERO(dbenvst->dbenvp, DB_ENV, 1);
    dbenvst->dbenvp->db_errpfx = "BDB::";
    dbenvst->dbenvp->db_errcall = bdb_env_errcall;
#else
#if DB_VERSION_MINOR >= 1 || DB_VERSION_MAJOR >= 4
    if (argc && TYPE(argv[argc - 1]) == T_HASH) {
	rb_iterate(rb_each, argv[argc - 1], bdb_env_s_i_options, (VALUE)&flags);
    }
#endif
    bdb_test_error(db_env_create(&(dbenvst->dbenvp), flags));
    dbenvst->dbenvp->set_errpfx(dbenvst->dbenvp, "BDB::");
    dbenvst->dbenvp->set_errcall(dbenvst->dbenvp, bdb_env_errcall);
#if DB_VERSION_MINOR >= 3 || DB_VERSION_MAJOR >= 4
    bdb_test_error(dbenvst->dbenvp->set_alloc(dbenvst->dbenvp, malloc, realloc, free));
#endif
#endif
    return res;
}

#if DB_VERSION_MAJOR >= 4

VALUE 
bdb_env_s_rslbl(int argc, VALUE *argv, VALUE obj, DB_ENV *env)
{
    bdb_ENV *dbenvst;
    VALUE res;

    res = Data_Make_Struct(obj, bdb_ENV, bdb_env_mark, bdb_env_free, dbenvst);
    dbenvst->dbenvp = env;
    dbenvst->dbenvp->set_errpfx(dbenvst->dbenvp, "BDB::");
    dbenvst->dbenvp->set_errcall(dbenvst->dbenvp, bdb_env_errcall);
    bdb_test_error(dbenvst->dbenvp->set_alloc(dbenvst->dbenvp, malloc, realloc, free));
    return res;
}

#endif

static VALUE
bdb_env_init(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    DB_ENV *dbenvp;
    bdb_ENV *dbenvst;
    VALUE a, c, d;
    char *db_home, **db_config;
    int ret, mode, flags;
    VALUE st_config;
    VALUE envid;

    envid = 0;
    st_config = 0;
    db_config = 0;
    mode = flags = 0;
    Data_Get_Struct(obj, bdb_ENV, dbenvst);
    dbenvp = dbenvst->dbenvp;
#if DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 1
    if (rb_const_defined(CLASS_OF(obj), rb_intern("BDB_ENCRYPT"))) {
	char *passwd;
	int flags = DB_ENCRYPT_AES;
	VALUE value = rb_const_get(CLASS_OF(obj), rb_intern("BDB_ENCRYPT"));
	if (TYPE(value) == T_ARRAY) {
	    if (RARRAY(value)->len != 2) {
		rb_raise(bdb_eFatal, "Expected an Array with 2 values");
	    }
	    passwd = STR2CSTR(RARRAY(value)->ptr[0]);
	    flags = NUM2INT(RARRAY(value)->ptr[1]);
	}
	else {
	    passwd = STR2CSTR(value);
	}
	bdb_test_error(dbenvp->set_encrypt(dbenvp, passwd, flags));
	dbenvst->options |= BDB_ENV_ENCRYPT;
    }
#endif
    if (argc && TYPE(argv[argc - 1]) == T_HASH) {
	VALUE db_stobj;
	struct db_stoptions *db_st;

	st_config = rb_ary_new();
	db_stobj = Data_Make_Struct(rb_cObject, struct db_stoptions, 0, free, db_st);
	db_st->env = dbenvst;
	db_st->config = st_config;
	bdb_env_each_options(argv[argc - 1], db_stobj);
	argc--;
    }
    rb_scan_args(argc, argv, "12", &a, &c, &d);
    Check_SafeStr(a);
    db_home = RSTRING(a)->ptr;
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
    if (!(dbenvst->options & BDB_NO_THREAD)) {
	bdb_set_func(dbenvst);
	flags |= DB_THREAD;
    }
#endif
#if DB_VERSION_MAJOR < 3
    if ((ret = db_appinit(db_home, db_config, dbenvp, flags)) != 0) {
        if (bdb_errcall) {
            bdb_errcall = 0;
            rb_raise(bdb_eFatal, "%s -- %s", RSTRING(bdb_errstr)->ptr, db_strerror(ret));
        }
        else
            rb_raise(bdb_eFatal, "%s", db_strerror(ret));
    }
#else
#if DB_VERSION_MAJOR >= 4
    if (dbenvst->rep_transport == 0 && rb_respond_to(obj, rb_intern("bdb_rep_transport")) == Qtrue) {
	if (!rb_const_defined(CLASS_OF(obj), rb_intern("ENVID"))) {
	    rb_raise(bdb_eFatal, "ENVID must be defined to use rep_transport");
	}
	envid = rb_const_get(CLASS_OF(obj), rb_intern("ENVID"));
	bdb_test_error(dbenvp->set_rep_transport(dbenvp, NUM2INT(envid),
						 bdb_env_rep_transport));
	dbenvst->options |= BDB_REP_TRANSPORT;
    }
#endif
#if DB_VERSION_MAJOR >= 3
    if (dbenvst->feedback == 0 && rb_respond_to(obj, id_feedback) == Qtrue) {
	dbenvp->set_feedback(dbenvp, bdb_env_feedback);
	dbenvst->options |= BDB_FEEDBACK;
    }
#endif
#if DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 1
    if (dbenvst->app_dispatch == 0 && rb_respond_to(obj, id_app_dispatch) == Qtrue) {
	dbenvp->set_app_dispatch(dbenvp, bdb_env_app_dispatch);
	dbenvst->options |= BDB_APP_DISPATCH;
    }
#endif
#if DB_VERSION_MINOR >= 1 || DB_VERSION_MAJOR >= 4
    if ((ret = dbenvp->open(dbenvp, db_home, flags, mode)) != 0) {
#else
    {
	int i;
	if (st_config && RARRAY(st_config)->len > 0) {
	    db_config = ALLOCA_N(char *, RARRAY(st_config)->len + 1);
	    for (i = 0; i < RARRAY(st_config)->len; i++) {
		db_config[i] = RSTRING(RARRAY(st_config)->ptr[i])->ptr;
	    }
	    db_config[RARRAY(st_config)->len] = 0;
	}
    }
    if ((ret = dbenvp->open(dbenvp, db_home, db_config, flags, mode)) != 0) {
#endif
        dbenvp->close(dbenvp, 0);
	dbenvst->dbenvp = NULL;
        if (bdb_errcall) {
            bdb_errcall = 0;
            rb_raise(bdb_eFatal, "%s -- %s", RSTRING(bdb_errstr)->ptr, db_strerror(ret));
        }
        else
            rb_raise(bdb_eFatal, "%s", db_strerror(ret));
    }
#endif
    dbenvst->db_ary = rb_ary_new2(0);
    if (flags & DB_INIT_LOCK) {
	dbenvst->options |= BDB_INIT_LOCK;
    }
    if (flags & DB_INIT_TXN) {
	dbenvst->options |= BDB_AUTO_COMMIT;
    }
    dbenvst->home = rb_tainted_str_new2(db_home);
    OBJ_FREEZE(dbenvst->home);
#if DB_VERSION_MAJOR >= 3
    if (dbenvst->options & BDB_NEED_ENV_CURRENT) {
	rb_thread_local_aset(rb_thread_current(), bdb_id_current_env, obj);
    }
#endif
    return obj;
}

static VALUE
bdb_env_s_open(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    VALUE res = rb_funcall2(obj, rb_intern("new"), argc, argv);
    if (rb_block_given_p()) {
	return rb_ensure(rb_yield, res, bdb_env_close, res);
    }
    return res;
}

static VALUE
bdb_env_s_remove(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    DB_ENV *dbenvp;
    VALUE a, b;
    char *db_home;
    int flag = 0;

    rb_secure(2);
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	flag = NUM2INT(b);
    }
    db_home = STR2CSTR(a);
#if DB_VERSION_MAJOR < 3
    dbenvp = ALLOC(DB_ENV);
    MEMZERO(dbenvp, DB_ENV, 1);
    dbenvp->db_errpfx = "BDB::";
    dbenvp->db_errcall = bdb_env_errcall;
    if (lock_unlink(db_home, flag, dbenvp) == EBUSY) {
	rb_raise(bdb_eFatal, "The shared memory region was in use");
    }
    if (log_unlink(db_home, flag, dbenvp) == EBUSY) {
	rb_raise(bdb_eFatal, "The shared memory region was in use");
    }
    if (memp_unlink(db_home, flag, dbenvp) == EBUSY) {
	rb_raise(bdb_eFatal, "The shared memory region was in use");
    }
    if (txn_unlink(db_home, flag, dbenvp) == EBUSY) {
	rb_raise(bdb_eFatal, "The shared memory region was in use");
    }
#else
    bdb_test_error(db_env_create(&dbenvp, 0));
    dbenvp->set_errpfx(dbenvp, "BDB::");
    dbenvp->set_errcall(dbenvp, bdb_env_errcall);
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR < 1
    bdb_test_error(dbenvp->remove(dbenvp, db_home, NULL, flag));
#else
    bdb_test_error(dbenvp->remove(dbenvp, db_home, flag));
#endif
#endif
    return Qtrue;
}

static VALUE
bdb_env_set_flags(argc, argv, obj)
    int argc;
    VALUE *argv, obj;
{
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 2) || DB_VERSION_MAJOR >= 4
    bdb_ENV *dbenvst;
    VALUE opt, flag;
    int state = 1;

    GetEnvDB(obj, dbenvst);
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
    bdb_test_error(dbenvst->dbenvp->set_flags(dbenvst->dbenvp, NUM2INT(flag), state));
#endif
    return Qnil;
}

static VALUE
bdb_env_home(obj)
    VALUE obj;
{
    bdb_ENV *dbenvst;
    GetEnvDB(obj, dbenvst);
    return dbenvst->home;
}

#if DB_VERSION_MAJOR >= 4

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

#if DB_VERSION_MAJOR >= 3

static VALUE
bdb_env_feedback_set(VALUE obj, VALUE a)
{
    bdb_ENV *dbenvst;

    GetEnvDB(obj, dbenvst);
    if (NIL_P(a)) {
	dbenvst->feedback = a;
    }
    else {
	if (!rb_respond_to(a, bdb_id_call)) {
	    rb_raise(bdb_eFatal, "arg must respond to #call");
	}
	dbenvst->feedback = a;
	if (!(dbenvst->options & BDB_NEED_ENV_CURRENT)) {
	    dbenvst->options |= BDB_FEEDBACK;
	    rb_thread_local_aset(rb_thread_current(), bdb_id_current_env, obj);
	}
    }
    return a;
}

#endif

void bdb_init_env()
{
    bdb_id_call = rb_intern("call");
#if DB_VERSION_MAJOR >= 3
    id_feedback = rb_intern("bdb_feedback");
#endif
    bdb_id_current_env = rb_intern("bdb_current_env");
#if DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 1
    id_app_dispatch = rb_intern("bdb_app_dispatch");
#endif
    bdb_cEnv = rb_define_class_under(bdb_mDb, "Env", rb_cObject);
    rb_define_private_method(bdb_cEnv, "initialize", bdb_env_init, -1);
    rb_define_singleton_method(bdb_cEnv, "allocate", bdb_env_s_alloc, -1);
    rb_define_singleton_method(bdb_cEnv, "new", bdb_s_new, -1);
    rb_define_singleton_method(bdb_cEnv, "create", bdb_s_new, -1);
    rb_define_singleton_method(bdb_cEnv, "open", bdb_env_s_open, -1);
    rb_define_singleton_method(bdb_cEnv, "remove", bdb_env_s_remove, -1);
    rb_define_singleton_method(bdb_cEnv, "unlink", bdb_env_s_remove, -1);
    rb_define_method(bdb_cEnv, "open_db", bdb_env_open_db, -1);
    rb_define_method(bdb_cEnv, "close", bdb_env_close, 0);
    rb_define_method(bdb_cEnv, "set_flags", bdb_env_set_flags, -1);
    rb_define_method(bdb_cEnv, "home", bdb_env_home, 0);
#if DB_VERSION_MAJOR >= 4
    rb_define_method(bdb_cEnv, "rep_elect", bdb_env_rep_elect, 3);
    rb_define_method(bdb_cEnv, "elect", bdb_env_rep_elect, 3);
    rb_define_method(bdb_cEnv, "rep_process_message", bdb_env_rep_process_message, 3);
    rb_define_method(bdb_cEnv, "process_message", bdb_env_rep_process_message, 3);
    rb_define_method(bdb_cEnv, "rep_start", bdb_env_rep_start, 2);
    if (!rb_method_boundp(rb_cThread, rb_intern("__bdb_thread_init__"), 1)) {
	rb_alias(rb_cThread, rb_intern("__bdb_thread_init__"), rb_intern("initialize"));
	rb_define_method(rb_cThread, "initialize", bdb_thread_init, -1);
    }
#if DB_VERSION_MINOR >= 1
    rb_define_method(bdb_cEnv, "rep_limit=", bdb_env_rep_limit, -1);
#endif
#endif
#if DB_VERSION_MAJOR >= 3
    rb_define_method(bdb_cEnv, "feedback=", bdb_env_feedback_set, 1);
#endif
}
