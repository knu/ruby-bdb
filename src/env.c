#include "bdb.h"

struct db_stoptions {
    bdb_ENV *env;
    VALUE config;
    int lg_max, lg_bsize;
};

VALUE
bdb_obj_init(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    return Qtrue;
}

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

    if ((obj = rb_thread_local_aref(rb_thread_current(), rb_intern("bdb_current_env"))) == Qnil) {
	rb_raise(bdb_eFatal, "BUG : current_dbenv not set");
    }
    GetEnvDB(obj, dbenvst);
    av = rb_tainted_str_new(control->data, control->size);
    bv = rb_tainted_str_new(rec->data, rec->size);
    if (dbenvst->rep_transport == 0) {
	res = rb_funcall(obj, rb_intern("bdb_rep_transport"), 4, av, bv,
			 INT2FIX(envid), INT2FIX(flags));
    }
    else {
	res = rb_funcall(dbenvst->rep_transport, rb_intern("call"), 4, 
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
	    if (!rb_respond_to(value, id_load) ||
		!rb_respond_to(value, id_dump)) {
		rb_raise(bdb_eFatal, "marshal value must be true or false");
	    }
	    dbenvst->marshal = value;
	    break;
        }
    }
    else if (strcmp(options, "thread") == 0) {
        switch (value) {
        case Qtrue: dbenvst->no_thread = 0; break;
        case Qfalse: dbenvst->no_thread = 1; break;
        default: rb_raise(bdb_eFatal, "thread value must be true or false");
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
	if (!rb_obj_is_kind_of(RARRAY(value)->ptr[1], rb_cProc)) {
	    rb_raise(bdb_eFatal, "expected a Proc object for the 2nd arg of set_rep_transport");
	}
	dbenvst->rep_transport = RARRAY(value)->ptr[1];
	bdb_test_error(dbenvst->dbenvp->set_rep_transport(dbenvst->dbenvp, NUM2INT(RARRAY(value)->ptr[0]), bdb_env_rep_transport));
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
    return Qnil;
}

#if DB_VERSION_MAJOR < 3
#define env_close(envst)			\
    bdb_test_error(db_appexit(envst->dbenvp));	\
    free(envst->dbenvp);			\
    envst->dbenvp = NULL;
#else
#define env_close(envst)					\
    bdb_test_error(envst->dbenvp->close(envst->dbenvp, 0));	\
    envst->dbenvp = NULL;
#endif

static void
bdb_env_free(dbenvst)
    bdb_ENV *dbenvst;
{
    VALUE db;

    if (dbenvst->dbenvp) {
	while ((db = rb_ary_pop(dbenvst->db_ary)) != Qnil) {
	    if (TYPE(db) == T_DATA) {
		bdb_close(0, 0, db);
	    }
	}
	env_close(dbenvst);
    }
    free(dbenvst);
}

static void
bdb_env_mark(dbenvst)
    bdb_ENV *dbenvst;
{
    if (dbenvst->marshal) rb_gc_mark(dbenvst->marshal);
#if DB_VERSION_MAJOR >= 4
    if (dbenvst->rep_transport) rb_gc_mark(dbenvst->rep_transport);
#endif
    rb_gc_mark(dbenvst->db_ary);
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
    return bdb_s_new(argc, argv, cl);
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
    while ((db = rb_ary_pop(dbenvst->db_ary)) != Qnil) {
	if (TYPE(db) == T_DATA) {
	    bdb_close(0, 0, db);
	}
    }
    env_close(dbenvst);
    return Qtrue;
}

void
bdb_env_errcall(errpfx, msg)
    const char *errpfx;
    char *msg;
{
    bdb_errcall = 1;
    bdb_errstr = rb_tainted_str_new2(msg);
}

static VALUE
bdb_env_each_failed(dbenvst)
    bdb_ENV *dbenvst;
{
    VALUE d;

    free(dbenvst->dbenvp);
    free(dbenvst);
    d = rb_obj_as_string(rb_gv_get("$!"));
    rb_raise(bdb_eFatal, "%.*s", RSTRING(d)->len, RSTRING(d)->ptr);
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
bdb_env_each_options(args)
    VALUE *args;
{
    VALUE res;
    DB_ENV *dbenvp;
    struct db_stoptions *db_st;

    res = rb_iterate(rb_each, args[0], bdb_env_i_options, args[1]);
    Data_Get_Struct(args[1], struct db_stoptions, db_st);
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
bdb_env_s_new(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    DB_ENV *dbenvp;
    bdb_ENV *dbenvst;
    VALUE a, c, d, options, retour;
    char *db_home, **db_config;
    int ret, mode, flags;
    VALUE st_config;
    VALUE envid;

    envid = 0;
    st_config = 0;
    db_config = 0;
    options = Qnil;
    mode = flags = 0;
#if DB_VERSION_MAJOR < 3
    dbenvp = ALLOC(DB_ENV);
    MEMZERO(dbenvp, DB_ENV, 1);
    dbenvp->db_errpfx = "BDB::";
    dbenvp->db_errcall = bdb_env_errcall;
#else
    bdb_test_error(db_env_create(&dbenvp, 0));
    dbenvp->set_errpfx(dbenvp, "BDB::");
    dbenvp->set_errcall(dbenvp, bdb_env_errcall);
#if DB_VERSION_MINOR >= 3
    bdb_test_error(dbenvp->set_alloc(dbenvp, malloc, realloc, free));
#endif
#endif
    if (argc && TYPE(argv[argc - 1]) == T_HASH) {
	options = argv[argc - 1];
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
    dbenvst = ALLOC(bdb_ENV);
    MEMZERO(dbenvst, bdb_ENV, 1);
    dbenvst->dbenvp = dbenvp;
    if (!NIL_P(options)) {
	VALUE subargs[2], db_stobj;
	struct db_stoptions *db_st;

	st_config = rb_ary_new();
	db_stobj = Data_Make_Struct(rb_cObject, struct db_stoptions, 0, free, db_st);
	db_st->env = dbenvst;
	db_st->config = st_config;

	subargs[0] = options;
	subargs[1] = db_stobj;
	rb_rescue(bdb_env_each_options, (VALUE)subargs, bdb_env_each_failed, (VALUE)dbenvst);
    }
    if (flags & DB_CREATE) {
	rb_secure(4);
    }
    if (flags & DB_USE_ENVIRON) {
	rb_secure(1);
    }
#ifndef BDB_NO_THREAD
    if (!dbenvst->no_thread) {
	rb_rescue(bdb_set_func, (VALUE)dbenvst, bdb_env_each_failed, (VALUE)dbenvst);
	flags |= DB_THREAD;
    }
#endif
#if DB_VERSION_MAJOR < 3
    if ((ret = db_appinit(db_home, db_config, dbenvp, flags)) != 0) {
        free(dbenvp);
        free(dbenvst);
        if (bdb_errcall) {
            bdb_errcall = 0;
            rb_raise(bdb_eFatal, "%s -- %s", RSTRING(bdb_errstr)->ptr, db_strerror(ret));
        }
        else
            rb_raise(bdb_eFatal, "%s", db_strerror(ret));
    }
#else
#if DB_VERSION_MAJOR >= 4
    if (dbenvst->rep_transport == 0 && rb_method_boundp(obj, rb_intern("bdb_rep_transport"), 0) == Qtrue) {
	if (!rb_const_defined(obj, rb_intern("ENVID"))) {
	    rb_raise(bdb_eFatal, "ENVID must be defined to use rep_transport");
	}
	envid = rb_const_get(obj, rb_intern("ENVID"));
	bdb_test_error(dbenvp->set_rep_transport(dbenvp, NUM2INT(envid), bdb_env_rep_transport));
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
        free(dbenvst);
        if (bdb_errcall) {
            bdb_errcall = 0;
            rb_raise(bdb_eFatal, "%s -- %s", RSTRING(bdb_errstr)->ptr, db_strerror(ret));
        }
        else
            rb_raise(bdb_eFatal, "%s", db_strerror(ret));
    }
#endif
    dbenvst->db_ary = rb_ary_new2(0);
    dbenvst->flags27 = flags;
    dbenvst->home = rb_tainted_str_new2(db_home);
    OBJ_FREEZE(dbenvst->home);
    retour = Data_Wrap_Struct(obj, bdb_env_mark, bdb_env_free, dbenvst);
#if DB_VERSION_MAJOR >= 4
    if (envid != 0 || dbenvst->rep_transport != 0) {
	rb_thread_local_aset(rb_thread_current(), rb_intern("bdb_current_env"), retour);
    }
#endif
    rb_obj_call_init(retour, NIL_P(options)?argc:argc+1, argv);
    return retour;
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

    if ((env = rb_thread_local_aref(rb_thread_current(), rb_intern("bdb_current_env"))) != Qnil) {
	rb_thread_local_aset(obj, rb_intern("bdb_current_env"), env);
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

void bdb_init_env()
{
    bdb_cEnv = rb_define_class_under(bdb_mDb, "Env", rb_cObject);
    rb_define_private_method(bdb_cEnv, "initialize", bdb_obj_init, -1);
    rb_define_singleton_method(bdb_cEnv, "new", bdb_env_s_new, -1);
    rb_define_singleton_method(bdb_cEnv, "create", bdb_env_s_new, -1);
    rb_define_singleton_method(bdb_cEnv, "open", bdb_env_s_new, -1);
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
    rb_define_method(bdb_cEnv, "rep_start", bdb_env_rep_start, 2);
    if (!rb_method_boundp(rb_cThread, rb_intern("__bdb_thread_init__"), 1)) {
	rb_alias(rb_cThread, rb_intern("__bdb_thread_init__"), rb_intern("initialize"));
	rb_define_method(rb_cThread, "initialize", bdb_thread_init, -1);
    }
#endif
}
