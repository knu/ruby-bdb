#include <ruby.h>
#include <db.h>
#include <errno.h>

#define DB_RB_MARSHAL 1
#define DB_RB_TXN 2
#define DB_RB_RE_SOURCE 4

#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
#define DB_RMW 0
#endif

static VALUE db_cEnv;
static VALUE db_eFatal, db_eLock, db_eExist;
static VALUE db_mDb;
static VALUE db_cCommon, db_cBtree, db_cHash, db_cRecno, db_cUnknown;
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1
static VALUE db_sKeyrange;
#endif

#if DB_VERSION_MAJOR == 3
static VALUE db_cQueue;
#endif

static VALUE db_cTxn, db_cTxnCatch;
static VALUE db_cCursor;
static VALUE db_cLock, db_cLockid;

static VALUE db_mMarshal;
static ID id_dump, id_load, id_to_s, id_eql;

static VALUE dberrstr;
static int dberrcall = 0;

typedef struct  {
    int marshal, no_thread;
    int flags27;
    VALUE db_ary;
    DB_ENV *dbenvp;
} db_ENV;

typedef struct {
    int status;
    int marshal;
    int flags27;
    VALUE db_ary;
    VALUE env;
    DB_TXN *txnid;
    DB_TXN *parent;
} db_TXN;

typedef struct {
    int options;
    int flags27;
    DBTYPE type;
    VALUE env;
    DB *dbp;
    db_TXN *txn;
    u_int32_t flags;
    u_int32_t partial;
    u_int32_t dlen;
    u_int32_t doff;
#if DB_VERSION_MAJOR < 3
    DB_INFO *dbinfo;
#else
    int re_len;
#endif
} db_DB;

typedef struct {
    DBC *dbc;
    db_DB *dbst;
} db_DBC;

typedef struct {
    unsigned int lock;
    DB_ENV *dbenvp;
} db_LOCKID;

typedef struct {
#if DB_VERSION_MAJOR < 3
    DB_LOCK lock;
#else
    DB_LOCK *lock;
#endif
    DB_ENV *dbenvp;
} db_LOCK;

typedef struct {
    db_DB *dbst;
    DBT *key;
    VALUE value;
} db_VALUE;

#if DB_VERSION_MAJOR < 3

static char *
db_strerror(int err)
{
    if (err == 0)
        return "" ;

    if (err > 0)
        return strerror(err) ;

    switch (err) {
    case DB_INCOMPLETE:
        return ("DB_INCOMPLETE: Sync was unable to complete");
    case DB_KEYEMPTY:
        return ("DB_KEYEMPTY: Non-existent key/data pair");
    case DB_KEYEXIST:
        return ("DB_KEYEXIST: Key/data pair already exists");
    case DB_LOCK_DEADLOCK:
        return ("DB_LOCK_DEADLOCK: Locker killed to resolve a deadlock");
    case DB_LOCK_NOTGRANTED:
        return ("DB_LOCK_NOTGRANTED: Lock not granted");
    case DB_LOCK_NOTHELD:
        return ("DB_LOCK_NOTHELD: Lock not held by locker");
    case DB_NOTFOUND:
        return ("DB_NOTFOUND: No matching key/data pair found");
#if DB_VERSION_MINOR >= 6
    case DB_RUNRECOVERY:
        return ("DB_RUNRECOVERY: Fatal error, run database recovery");
#endif
    default:
        return "Unknown Error" ;
    }
}

#endif

static int
test_error(comm)
    int comm;
{
    VALUE error;

    switch (comm) {
    case 0:
    case DB_NOTFOUND:
    case DB_KEYEMPTY:
        break;
    default:
        error = (comm == DB_LOCK_DEADLOCK || comm == EAGAIN)?db_eLock:
	    (comm == DB_KEYEXIST)?db_eExist:db_eFatal;
        if (dberrcall) {
            dberrcall = 0;
            rb_raise(error, "%s -- %s", RSTRING(dberrstr)->ptr, db_strerror(comm));
        }
        else
            rb_raise(error, "%s", db_strerror(comm));
    }
    return comm;
}

struct db_stoptions {
    db_ENV *env;
    VALUE config;
};

static VALUE
dbenv_i_options(obj, db_st)
    VALUE obj;
    struct db_stoptions *db_st;
{
    char *options;
    DB_ENV *dbenvp;
    VALUE key, value;
    db_ENV *dbenvst;

    dbenvst = db_st->env;

    key = rb_ary_entry(obj, 0);
    value = rb_ary_entry(obj, 1);
    dbenvp = dbenvst->dbenvp;
    key = rb_funcall(key, rb_intern("to_s"), 0);
    options = RSTRING(key)->ptr;
    if (strcmp(options, "set_cachesize") == 0) {
#if DB_VERSION_MAJOR < 3
        dbenvp->mp_size = NUM2INT(value);
#else
        Check_Type(value, T_ARRAY);
        if (RARRAY(value)->len < 3)
            rb_raise(db_eFatal, "expected 3 values for cachesize");
        test_error(dbenvp->set_cachesize(dbenvp, 
                                      NUM2INT(RARRAY(value)->ptr[0]),
                                      NUM2INT(RARRAY(value)->ptr[1]),
                                      NUM2INT(RARRAY(value)->ptr[2])));
#endif
    }
#if DB_VERSION_MAJOR == 3
    else if (strcmp(options, "set_region_init") == 0) {
        test_error(dbenvp->set_region_init(dbenvp, NUM2INT(value)));
    }
    else if (strcmp(options, "set_tas_spins") == 0) {
        test_error(dbenvp->set_tas_spins(dbenvp, NUM2INT(value)));
    }
    else if (strcmp(options, "set_tx_max") == 0) {
        test_error(dbenvp->set_tx_max(dbenvp, NUM2INT(value)));
    }
#endif
#if DB_VERSION_MAJOR < 3
    else if  (strcmp(options, "set_verbose") == 0) {
        test_error(db_verbose(dbenvp, NUM2INT(value)));
    }
#else
    else if (strcmp(options, "set_verb_chkpoint") == 0) {
        test_error(dbenvp->set_verbose(dbenvp, DB_VERB_CHKPOINT, NUM2INT(value)));
    }
    else if (strcmp(options, "set_verb_deadlock") == 0) {
        test_error(dbenvp->set_verbose(dbenvp, DB_VERB_DEADLOCK, NUM2INT(value)));
    }
    else if (strcmp(options, "set_verb_recovery") == 0) {
        test_error(dbenvp->set_verbose(dbenvp, DB_VERB_RECOVERY, NUM2INT(value)));
    }
    else if (strcmp(options, "set_verb_waitsfor") == 0) {
        test_error(dbenvp->set_verbose(dbenvp, DB_VERB_WAITSFOR, NUM2INT(value)));
    }
#endif
    else if (strcmp(options, "set_lk_detect") == 0) {
#if DB_VERSION_MAJOR < 3
	dbenvp->lk_detect = NUM2INT(value);
#else
        test_error(dbenvp->set_lk_detect(dbenvp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_lk_max") == 0) {
#if DB_VERSION_MAJOR < 3
	dbenvp->lk_max = NUM2INT(value);
#else
        test_error(dbenvp->set_lk_max(dbenvp, NUM2INT(value)));
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
		rb_raise(db_eFatal, "invalid array for lk_conflicts");
	    }
	    for (j = 0; j < l; j++, p++) {
		if (TYPE(RARRAY(RARRAY(value)->ptr[i])->ptr[j] != T_FIXNUM)) {
		    free(conflits);
		    rb_raise(db_eFatal, "invalid value for lk_conflicts");
		}
		v = NUM2INT(RARRAY(RARRAY(value)->ptr[i])->ptr[j]);
		if (v != 0 && v != 1) {
		    free(conflits);
		    rb_raise(db_eFatal, "invalid value for lk_conflicts");
		}
		*p = (unsigned char)v;
	    }
	}
#if DB_VERSION_MAJOR < 3
	dbenvp->lk_modes = l;
	dbenvp->lk_conflicts = conflits;
#else
        test_error(dbenvp->set_lk_conflicts(dbenvp, conflits, l));
#endif
    }
#if DB_VERSION_MAJOR == 3
    else if (strcmp(options, "set_data_dir") == 0) {
	char *tmp;

	Check_SafeStr(value);
#if DB_VERSION_MINOR >= 1
	test_error(dbenvp->set_data_dir(dbenvp, RSTRING(value)->ptr));
#else
	tmp = ALLOCA_N(char, strlen("set_data_dir") + strlen(RSTRING(value)->ptr) + 2);
	sprintf(tmp, "DB_DATA_DIR %s", RSTRING(value)->ptr);
	rb_ary_push(db_st->config, rb_str_new2(tmp));
#endif
    }
    else if (strcmp(options, "set_lg_dir") == 0) {
	char *tmp;

	Check_SafeStr(value);
#if DB_VERSION_MINOR >= 1
	test_error(dbenvp->set_lg_dir(dbenvp, RSTRING(value)->ptr));
#else
	tmp = ALLOCA_N(char, strlen("set_lg_dir") + strlen(RSTRING(value)->ptr) + 2);
	sprintf(tmp, "DB_LOG_DIR %s", RSTRING(value)->ptr);
	rb_ary_push(db_st->config, rb_str_new2(tmp));
#endif
    }
    else if (strcmp(options, "set_tmp_dir") == 0) {
	char *tmp;

	Check_SafeStr(value);
#if DB_VERSION_MINOR >= 1
	test_error(dbenvp->set_tmp_dir(dbenvp, RSTRING(value)->ptr));
#else
	tmp = ALLOCA_N(char, strlen("set_tmp_dir") + strlen(RSTRING(value)->ptr) + 2);
	sprintf(tmp, "DB_TMP_DIR %s", RSTRING(value)->ptr);
	rb_ary_push(db_st->config, rb_str_new2(tmp));
#endif
    }
#if DB_VERSION_MINOR >= 1
    else if (strcmp(options, "set_server") == 0) {
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
		rb_raise(db_eFatal, "Empty array for \"set_server\"");
		break;
	    }
	    break;
	default:
	    rb_raise(db_eFatal, "Invalid type for \"set_server\"");
	    break;
	}
	test_error(dbenvp->set_server(dbenvp, host, cl_timeout, sv_timeout, flags));
    }
#endif

#endif
    else if (strcmp(options, "marshal") == 0) {
        switch (value) {
        case Qtrue: dbenvst->marshal = 1; break;
        case Qfalse: dbenvst->marshal = 0; break;
        default: rb_raise(rb_eFatal, "marshal value must be true or false");
        }
    }
    else if (strcmp(options, "thread") == 0) {
        switch (value) {
        case Qtrue: dbenvst->no_thread = 0; break;
        case Qfalse: dbenvst->no_thread = 1; break;
        default: rb_raise(rb_eFatal, "thread value must be true or false");
        }
    }
    return Qnil;
}

#if DB_VERSION_MAJOR < 3
#if 0
#define env_close(envp)				\
    if (envp->tx_info) {			\
	test_error(txn_close(envp->tx_info));	\
    }						\
    if (envp->mp_info) {			\
	test_error(memp_close(envp->mp_info));	\
    }						\
    if (envp->lg_info) {			\
	test_error(log_close(envp->lg_info));	\
    }						\
    if (envp->lk_info) {			\
	test_error(lock_close(envp->lk_info));	\
    }						\
    free(envp);
#endif
#define env_close(envp) free(envp); envp = NULL;

#else
#define env_close(envp)				\
    test_error(envp->close(envp, 0));		\
    envp = NULL;
#endif

static VALUE db_close(int, VALUE *, VALUE);

static void
dbenv_free(dbenvst)
    db_ENV *dbenvst;
{
    VALUE db;

    if (dbenvst->dbenvp) {
	while ((db = rb_ary_pop(dbenvst->db_ary)) != Qnil) {
	    db_close(0, 0, db);
	}
	env_close(dbenvst->dbenvp);
    }
    free(dbenvst);
}

static void
dbenv_mark(dbenvst)
    db_ENV *dbenvst;
{
    rb_gc_mark(dbenvst->db_ary);
}

#define GetEnvDB(obj, dbenvst)                          \
{                                                       \
    Data_Get_Struct(obj, db_ENV, dbenvst);              \
    if (dbenvst->dbenvp == 0)                           \
        rb_raise(db_eFatal, "closed environment");      \
}

static VALUE
dbenv_close(obj)
    VALUE obj;
{
    db_ENV *dbenvst;
    VALUE db;

    if (!OBJ_TAINTED(obj) && rb_safe_level() >= 4)
	rb_raise(rb_eSecurityError, "Insecure: can't close the environnement");
    GetEnvDB(obj, dbenvst);
    while ((db = rb_ary_pop(dbenvst->db_ary)) != Qnil) {
	db_close(0, 0, db);
    }
    env_close(dbenvst->dbenvp);
    return Qtrue;
}

static void
dbenv_errcall(errpfx, msg)
    char *errpfx, *msg;
{
    dberrcall = 1;
    dberrstr = rb_tainted_str_new2(msg);
}

static VALUE
dbenv_each_failed(dbenvst)
    db_ENV *dbenvst;
{
    VALUE d;

    free(dbenvst->dbenvp);
    free(dbenvst);
    d = rb_funcall(rb_gv_get("$!"), id_to_s, 0);
    rb_raise(db_eFatal, "%.*s", RSTRING(d)->len, RSTRING(d)->ptr);
}

#ifndef NT
struct timeval {
        long    tv_sec;
        long    tv_usec;
};
#endif /* NT */

static int 
db_func_sleep(sec, usec)
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
db_func_yield()
{
    rb_thread_schedule();
    return 0;
}

static VALUE
db_set_func(dbenvst)
    db_ENV *dbenvst;
{
#if DB_VERSION_MAJOR == 3
    test_error(dbenvst->dbenvp->set_func_sleep(dbenvst->dbenvp, db_func_sleep));
    test_error(dbenvst->dbenvp->set_func_yield(dbenvst->dbenvp, db_func_yield));
#else
    test_error(db_jump_set((void *)db_func_sleep, DB_FUNC_SLEEP));
    test_error(db_jump_set((void *)db_func_yield, DB_FUNC_YIELD));
#endif
    return Qtrue;
}

static VALUE
dbenv_each_options(args)
    VALUE *args;
{
    return rb_iterate(rb_each, args[0], dbenv_i_options, args[1]);
}

static VALUE
dbenv_s_new(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    DB_ENV *dbenvp;
    db_ENV *dbenvst;
    VALUE a, c, d, options, retour;
    char *db_home, **db_config;
    int ret, mode, flags;
    VALUE st_config;

    st_config = 0;
    db_config = 0;
    options = Qnil;
    mode = flags = 0;
#if DB_VERSION_MAJOR < 3
    dbenvp = ALLOC(DB_ENV);
    MEMZERO(dbenvp, DB_ENV, 1);
    dbenvp->db_errpfx = "BDB::";
    dbenvp->db_errcall = dbenv_errcall;
#else
    test_error(db_env_create(&dbenvp, 0));
    dbenvp->set_errpfx(dbenvp, "BDB::");
    dbenvp->set_errcall(dbenvp, dbenv_errcall);
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
    dbenvst = ALLOC(db_ENV);
    MEMZERO(dbenvst, db_ENV, 1);
    dbenvst->dbenvp = dbenvp;
    if (!NIL_P(options)) {
	VALUE subargs[2];
	struct db_stoptions db_st;

	st_config = rb_ary_new();
	db_st.env = dbenvst;
	db_st.config = st_config;

	subargs[0] = options;
	subargs[1] = (VALUE)&db_st;
	rb_rescue(dbenv_each_options, (VALUE)subargs, dbenv_each_failed, (VALUE)dbenvst);
    }
    if (flags & DB_CREATE) {
	rb_secure(4);
    }
    if (flags & DB_USE_ENVIRON) {
	rb_secure(1);
    }
    if (!dbenvst->no_thread) {
	rb_rescue(db_set_func, (VALUE)dbenvst, dbenv_each_failed, (VALUE)dbenvst);
    }
    flags |= DB_THREAD;
#if DB_VERSION_MAJOR < 3
    if ((ret = db_appinit(db_home, db_config, dbenvp, flags)) != 0) {
        free(dbenvp);
        free(dbenvst);
        if (dberrcall) {
            dberrcall = 0;
            rb_raise(db_eFatal, "%s -- %s", RSTRING(dberrstr)->ptr, db_strerror(ret));
        }
        else
            rb_raise(db_eFatal, "%s", db_strerror(ret));
    }
#else
#if DB_VERSION_MINOR >= 1
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
        if (dberrcall) {
            dberrcall = 0;
            rb_raise(db_eFatal, "%s -- %s", RSTRING(dberrstr)->ptr, db_strerror(ret));
        }
        else
            rb_raise(db_eFatal, "%s", db_strerror(ret));
    }
#endif
    dbenvst->db_ary = rb_ary_new2(0);
    dbenvst->flags27 = flags;
    retour = Data_Wrap_Struct(obj, dbenv_mark, dbenv_free, dbenvst);
    rb_obj_call_init(retour, argc, argv);
    return retour;
}

static VALUE
dbenv_remove(obj)
    VALUE obj;
{
    db_ENV *dbenvst;

    rb_secure(2);
    GetEnvDB(obj, dbenvst);
#if DB_VERSION_MAJOR == 3
#if DB_VERSION_MINOR >=1
    test_error(dbenvst->dbenvp->remove(dbenvst->dbenvp, NULL, DB_FORCE));
#else
    test_error(dbenvst->dbenvp->remove(dbenvst->dbenvp, NULL, NULL, DB_FORCE));
#endif
#else
    if (dbenvst->dbenvp->tx_info) {
	test_error(txn_unlink(NULL, 1, dbenvst->dbenvp));
    }
    if (dbenvst->dbenvp->mp_info) {
	test_error(memp_unlink(NULL, 1, dbenvst->dbenvp));
    }
    if (dbenvst->dbenvp->lg_info) {
	test_error(log_unlink(NULL, 1, dbenvst->dbenvp));
    }
    if (dbenvst->dbenvp->lk_info) {
	test_error(lock_unlink(NULL, 1, dbenvst->dbenvp));
    }
#endif
    dbenvst->dbenvp = NULL;
    return Qtrue;
}

/* DATABASE */    
    
static VALUE
db_i_options(obj, dbst)
    VALUE obj;
    db_DB *dbst;
{
    char *options;
    DB *dbp;
    VALUE key, value;

    key = rb_ary_entry(obj, 0);
    value = rb_ary_entry(obj, 1);
    dbp = dbst->dbp;
    key = rb_funcall(key, rb_intern("to_s"), 0);
    options = RSTRING(key)->ptr;
    if (strcmp(options, "bt_minkey") == 0) {
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->bt_minkey = NUM2INT(value);
#else
        test_error(dbp->set_bt_minkey(dbp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_cachesize") == 0) {
        Check_Type(value, T_ARRAY);
        if (RARRAY(value)->len < 3)
            rb_raise(db_eFatal, "expected 3 values for cachesize");
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->db_cachesize = NUM2INT(RARRAY(value)->ptr[1]);
#else
        test_error(dbp->set_cachesize(dbp, 
                                      NUM2INT(RARRAY(value)->ptr[0]),
                                      NUM2INT(RARRAY(value)->ptr[1]),
                                      NUM2INT(RARRAY(value)->ptr[2])));
#endif
    }
    else if (strcmp(options, "set_flags") == 0) {
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->flags = NUM2UINT(value);
#else
        test_error(dbp->set_flags(dbp, NUM2UINT(value)));
#endif
	dbst->flags |= NUM2UINT(value);
    }
    else if (strcmp(options, "set_h_ffactor") == 0) {
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->h_ffactor = NUM2INT(value);
#else
        test_error(dbp->set_h_ffactor(dbp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_h_nelem") == 0) {
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->h_nelem = NUM2INT(value);
#else
        test_error(dbp->set_h_nelem(dbp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_lorder") == 0) {
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->db_lorder = NUM2INT(value);
#else
        test_error(dbp->set_lorder(dbp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_pagesize") == 0) {
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->db_pagesize = NUM2INT(value);
#else
        test_error(dbp->set_pagesize(dbp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_re_delim") == 0) {
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->re_delim = NUM2INT(value);
	dbst->dbinfo->flags |= DB_DELIMITER;
#else
        test_error(dbp->set_re_delim(dbp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_re_len") == 0) {
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->re_len = NUM2INT(value);
	dbst->dbinfo->flags |= DB_FIXEDLEN;
#else
        test_error(dbp->set_re_len(dbp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_re_pad") == 0) {
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->re_pad = NUM2INT(value);
	dbst->dbinfo->flags |= DB_PAD;
#else
        test_error(dbp->set_re_pad(dbp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_re_source") == 0) {
        if (TYPE(value) != T_STRING)
            rb_raise(db_eFatal, "re_source must be a filename");
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->re_source = RSTRING(value)->ptr;
#else
        test_error(dbp->set_re_source(dbp, RSTRING(value)->ptr));
#endif
	dbst->options |= DB_RB_RE_SOURCE;
    }
/*
    else if (strcmp(options, "txn") == 0) {
	db_TXN *txnst;

	if (!rb_obj_is_kind_of(value, db_cTxn)) {
	    rb_raise(rb_eFatal, "argument of txn must be a transaction");
	}
	Data_Get_Struct(value, db_TXN, txnst);
	dbst->txn = txnst;
    }
*/
    else if (strcmp(options, "marshal") == 0) {
        switch (value) {
        case Qtrue: dbst->options |= DB_RB_MARSHAL; break;
        case Qfalse: dbst->options &= ~DB_RB_MARSHAL; break;
        default: rb_raise(rb_eFatal, "marshal value must be true or false");
        }
    }
    return Qnil;
}

static void
db_free(dbst)
    db_DB *dbst;
{
    if (dbst->dbp && !(dbst->options & DB_RB_TXN)) {
        test_error(dbst->dbp->close(dbst->dbp, 0));
        dbst->dbp = NULL;
    }
    free(dbst);
}

#define test_dump(dbst, key, a)                         \
{                                                       \
    if (dbst->options & DB_RB_MARSHAL)                  \
        a = rb_funcall(db_mMarshal, id_dump, 1, a);     \
    else {                                              \
        a = rb_funcall(a, id_to_s, 0);                  \
    }                                                   \
    key.data = RSTRING(a)->ptr;                         \
    key.size = RSTRING(a)->len;                         \
}

static VALUE
test_load(dbst, a)
    db_DB *dbst;
    DBT a;
{
    VALUE res;
    if (dbst->options & DB_RB_MARSHAL)
        res = rb_funcall(db_mMarshal, id_load, 1, rb_str_new(a.data, a.size));
    else
        res = rb_tainted_str_new(a.data, a.size);
    if (a.flags & DB_DBT_MALLOC) {
	free(a.data);
    }
    return res;
}

#define GetDB(obj, dbst)			\
{						\
    Data_Get_Struct(obj, db_DB, dbst);		\
    if (dbst->dbp == 0) {			\
        rb_raise(db_eFatal, "closed DB");	\
    }						\
}

static VALUE
db_env(obj)
    VALUE obj;
{
    db_DB *dbst;

    GetDB(obj, dbst);
    return dbst->env;
}


static VALUE
db_has_env(obj)
    VALUE obj;
{
    db_DB *dbst;

    GetDB(obj, dbst);
    return (dbst->env == 0)?Qfalse:Qtrue;
}

static VALUE
db_close(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    VALUE opt;
    db_DB *dbst;
    int flags = 0;

    if (!OBJ_TAINTED(obj) && rb_safe_level() >= 4)
	rb_raise(rb_eSecurityError, "Insecure: can't close the database");
    Data_Get_Struct(obj, db_DB, dbst);
    if (dbst->dbp != NULL) {
	if (rb_scan_args(argc, argv, "01", &opt))
	    flags = NUM2INT(opt);
	test_error(dbst->dbp->close(dbst->dbp, flags));
	dbst->dbp = NULL;
    }
    return Qnil;
}

static VALUE
db_i_each_options(args)
    VALUE *args;
{
    return rb_iterate(rb_each, args[0], db_i_options, args[1]);
}

static VALUE
db_i_each_failed(args)
    db_DB *args;
{
    VALUE d;

    free(args);
    d = rb_funcall(rb_gv_get("$!"), id_to_s, 0);
    rb_raise(db_eFatal, "%.*s", RSTRING(d)->len, RSTRING(d)->ptr);
}

static db_DB *
db_open_common(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    db_DB *dbst;
#if DB_VERSION_MAJOR == 3
    DB *dbp;
#endif
    DB_ENV *dbenvp;
    int flags, mode, ret, nb;
    char *name, *subname;
    VALUE a, b, c, d, e, f;
#if DB_VERSION_MAJOR < 3
    DB_INFO dbinfo;

    MEMZERO(&dbinfo, DB_INFO, 1);
#endif
    mode = flags = 0;
    a = b = d = e = f = Qnil;
    if (argc && TYPE(argv[argc - 1]) == T_HASH) {
        f = argv[argc - 1];
        argc--;
    }
    switch(nb = rb_scan_args(argc, argv, "14", &c, &a, &b, &d, &e)) {
    case 5:
	mode = NUM2INT(e);
    case 4:
	flags = NUM2INT(d);
    }
    if (NIL_P(a))
        name = NULL;
    else {
	Check_SafeStr(a);
        name = RSTRING(a)->ptr;
    }
    if (NIL_P(b))
        subname = NULL;
    else {
	Check_SafeStr(b);
        subname = RSTRING(b)->ptr;
    }
    dbst = ALLOC(db_DB);
    dbenvp = 0;
    MEMZERO(dbst, db_DB, 1);
    if (!NIL_P(f)) {
	VALUE v;

        if (TYPE(f) != T_HASH) {
	    free(dbst);
            rb_raise(db_eFatal, "options must be an hash");
	}
	if ((v = rb_hash_aref(f, rb_str_new2("txn"))) != RHASH(f)->ifnone) {
	    db_ENV *dbenvst;
	    db_TXN *txnst;

	    if (!rb_obj_is_kind_of(v, db_cTxn)) {
		rb_raise(rb_eFatal, "argument of txn must be a transaction");
	    }
	    Data_Get_Struct(v, db_TXN, txnst);
	    dbst->txn = txnst;
	    dbst->env = txnst->env;
	    Data_Get_Struct(txnst->env, db_ENV, dbenvst);
	    dbenvp = dbenvst->dbenvp;
	    if (txnst->marshal)
		dbst->options |= DB_RB_MARSHAL;
	}
	else if ((v = rb_hash_aref(f, rb_str_new2("env"))) != RHASH(f)->ifnone) {
	    db_ENV *dbenvst;

	    if (!rb_obj_is_kind_of(v, db_cEnv)) {
		free(dbst);
		rb_raise(rb_eFatal, "argument of env must be an environnement");
	    }
	    Data_Get_Struct(v, db_ENV, dbenvst);
	    dbst->env = v;
	    dbenvp = dbenvst->dbenvp;
	    if (dbenvst->marshal)
		dbst->options |= DB_RB_MARSHAL;
	}
    }
#if DB_VERSION_MAJOR == 3
    test_error(db_create(&dbp, dbenvp, 0));
    dbp->set_errpfx(dbp, "BDB::");
    dbp->set_errcall(dbp, dbenv_errcall);
    dbst->dbp = dbp;
#else
    dbst->dbinfo = &dbinfo;
#endif
    if (!NIL_P(f)) {
	VALUE subargs[2];

        if (TYPE(f) != T_HASH) {
	    free(dbst);
            rb_raise(db_eFatal, "options must be an hash");
	}
	subargs[0] = f;
	subargs[1] = (VALUE)dbst;
	rb_rescue(db_i_each_options, (VALUE)subargs, db_i_each_failed, (VALUE)dbst);
   }
    if (flags & DB_TRUNCATE) {
	rb_secure(2);
    }
    if (flags & DB_CREATE) {
	rb_secure(4);
    }
    if (rb_safe_level() >= 4) {
	flags |= DB_RDONLY;
    }
    if (!(dbst->options & DB_RB_RE_SOURCE)) 
	flags |= DB_THREAD;
#if DB_VERSION_MAJOR < 3
    if ((ret = db_open(name, NUM2INT(c), flags, mode, dbenvp,
			 dbst->dbinfo, &dbst->dbp)) != 0) {
#else
    if ((ret = dbp->open(dbp, name, subname, NUM2INT(c), flags, mode)) != 0) {
        dbp->close(dbp, 0);
#endif
        free(dbst);
        if (dberrcall) {
            dberrcall = 0;
            rb_raise(db_eFatal, "%s -- %s", RSTRING(dberrstr)->ptr, db_strerror(ret));
        }
        else
            rb_raise(db_eFatal, "%s", db_strerror(ret));
    }
    if (dbenvp)
	dbst->flags27 = dbenvp->flags;
    return dbst;
}

static VALUE dbrecno_length(VALUE);

static VALUE
db_s_new(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    VALUE *nargv;
    VALUE res, cl, bb;
    db_DB *ret;

    cl = obj;
    while (cl) {
	if (cl == db_cBtree || RCLASS(cl)->m_tbl == RCLASS(db_cBtree)->m_tbl) {
	    bb = INT2NUM(DB_BTREE); 
	    break;
	}
	else if (cl == db_cHash || RCLASS(cl)->m_tbl == RCLASS(db_cHash)->m_tbl) {
	    bb = INT2NUM(DB_HASH); 
	    break;
	}
	else if (cl == db_cRecno || RCLASS(cl)->m_tbl == RCLASS(db_cRecno)->m_tbl) {
	    bb = INT2NUM(DB_RECNO);
	    break;
    }
#if DB_VERSION_MAJOR == 3
	else if (cl == db_cQueue || RCLASS(cl)->m_tbl == RCLASS(db_cQueue)->m_tbl) {
	    bb = INT2NUM(DB_QUEUE);
	    break;
	}
#endif
	else if (cl == db_cUnknown || RCLASS(cl)->m_tbl == RCLASS(db_cUnknown)->m_tbl) {
	    bb = INT2NUM(DB_UNKNOWN);
	    break;
	}
	cl = RCLASS(cl)->super;
    }
    if (!cl) {
	rb_raise(db_eFatal, "unknown database type");
    }
    nargv = ALLOCA_N(VALUE, argc + 1);
    nargv[0] = bb;
    MEMCPY(nargv + 1, argv, VALUE, argc);
    ret = db_open_common(argc + 1, nargv, obj);
#if DB_VERSION_MAJOR < 3
    ret->type = ret->dbp->type;
#else
    ret->type = ret->dbp->get_type(ret->dbp);
#endif
    res = Data_Wrap_Struct(obj, 0, db_free, ret);
    rb_obj_call_init(res, argc, argv);
    if (ret->type == DB_BTREE && (ret->flags & DB_RECNUM)) {
	rb_define_singleton_method(res, "size", dbrecno_length, 0);
	rb_define_singleton_method(res, "length", dbrecno_length, 0);
    }
    if (ret->txn != NULL) {
	rb_ary_push(ret->txn->db_ary, res);
    }
    else if (ret->env != 0) {
	db_ENV *dbenvp;
	Data_Get_Struct(ret->env, db_ENV, dbenvp);
	rb_ary_push(dbenvp->db_ary, res);
    }
    return res;
}

#if DB_VERSION_MAJOR == 3

struct re {
    int re_len, re_pad;
};

static VALUE
dbqueue_i_search_re_len(obj, rest)
    VALUE obj;
    struct re *rest;
{
    VALUE key, value;

    key = rb_ary_entry(obj, 0);
    value = rb_ary_entry(obj, 1);
    key = rb_funcall(key, rb_intern("to_s"), 0);
    if (strcmp(RSTRING(key)->ptr, "set_re_len") == 0) {
	rest->re_len = NUM2INT(value);
    }
    else if (strcmp(RSTRING(key)->ptr, "set_re_pad") == 0) {
	rest->re_pad = NUM2INT(value);
    }
    return Qnil;
}

#define DEFAULT_RECORD_LENGTH 132
#define DEFAULT_RECORD_PAD 0x20

static VALUE dbqueue_s_new(argc, argv, obj) 
    int argc; VALUE *argv; VALUE obj;
{
    VALUE *nargv, ret;
    struct re rest;
    db_DB *dbst;

    rest.re_len = -1;
    rest.re_pad = -1;
    if (argc && TYPE(argv[argc - 1]) == T_HASH) {
	rb_iterate(rb_each, argv[argc - 1], dbqueue_i_search_re_len, (VALUE)&rest);
	if (rest.re_len <= 0) {
	    rest.re_len = DEFAULT_RECORD_LENGTH;
	    rb_hash_aset(argv[argc - 1], rb_tainted_str_new2("set_re_len"), INT2NUM(rest.re_len));
	}
	if (rest.re_pad <= 0) {
	    rest.re_pad = DEFAULT_RECORD_PAD;
	    rb_hash_aset(argv[argc - 1], rb_tainted_str_new2("set_re_pad"), INT2NUM(rest.re_pad));
	}
	nargv = argv;
    }
    else {
	nargv = ALLOCA_N(VALUE, argc + 1);
	MEMCPY(nargv, argv, VALUE, argc);
	nargv[argc] = rb_hash_new();
	rest.re_len = DEFAULT_RECORD_LENGTH;
	rb_hash_aset(nargv[argc], rb_tainted_str_new2("set_re_len"), INT2NUM(DEFAULT_RECORD_LENGTH));
	rb_hash_aset(nargv[argc], rb_tainted_str_new2("set_re_pad"), INT2NUM(DEFAULT_RECORD_PAD));
	argc += 1;
    }
    ret = db_s_new(argc, nargv, obj);
    Data_Get_Struct(ret, db_DB, dbst);
    dbst->re_len = rest.re_len;
    return ret;
}
#endif

static VALUE
dbenv_open_db(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    VALUE cl;

    if (argc < 1)
        rb_raise(db_eFatal, "Invalid number of arguments");
    cl = argv[0];
    if (FIXNUM_P(cl)) {
	switch (NUM2INT(cl)) {
	case DB_BTREE: cl = db_cBtree; break;
	case DB_HASH: cl = db_cHash; break;
	case DB_RECNO: cl = db_cRecno; break;
#if DB_VERSION_MAJOR == 3
	case DB_QUEUE: cl = db_cQueue; break;
#endif
	case DB_UNKNOWN: cl = db_cUnknown; break;
	default: rb_raise(db_eFatal, "Unknown database type");
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
    rb_hash_aset(argv[argc - 1], rb_tainted_str_new2("env"), obj);
    return db_s_new(argc, argv, cl);
}

#define init_txn(txnid, obj, dbst) {					  \
  txnid = NULL;								  \
  GetDB(obj, dbst);							  \
  if (dbst->txn != NULL) {						  \
  if (dbst->txn->txnid == 0)						  \
    rb_warning("using a db handle associated with a closed transaction"); \
    txnid = dbst->txn->txnid;						  \
  }									  \
}

static VALUE
db_append(obj, a)
    VALUE obj, a;
{
    db_DB *dbst;
    DB_TXN *txnid;
    DBT key, data;
    db_recno_t recno;

    rb_secure(4);
    init_txn(txnid, obj, dbst);
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    recno = 1;
    key.data = &recno;
    key.size = sizeof(db_recno_t);
    test_dump(dbst, data, a);
#if DB_VERSION_MAJOR == 3
    if (dbst->type == DB_QUEUE && dbst->re_len < data.size) {
	rb_raise(db_eFatal, "size > re_len for Queue");
    }
#endif
    test_error(dbst->dbp->put(dbst->dbp, txnid, &key, &data, DB_APPEND));
    return INT2NUM(recno);
}

#if DB_VERSION_MAJOR < 3
#define DB_QUEUE DB_RECNO
#endif

#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
#define init_recno(dbst, key, recno)				\
{								\
    recno = 1;							\
    if (dbst->type == DB_RECNO || dbst->type == DB_QUEUE) {	\
        key.data = &recno;					\
        key.size = sizeof(db_recno_t);				\
        key.flags |= DB_DBT_MALLOC;                             \
    }								\
    else {							\
        key.flags |= DB_DBT_MALLOC;				\
    }								\
}

#define free_key(dbst, key)					\
{								\
    if ((key.flags & DB_DBT_MALLOC) && 				\
	(dbst->type != DB_RECNO && dbst->type != DB_QUEUE)) {	\
	free(key.data);						\
    }								\
}

#else

#define init_recno(dbst, key, recno)				\
{								\
    recno = 1;							\
    if (dbst->type == DB_RECNO || dbst->type == DB_QUEUE) {	\
        key.data = &recno;					\
        key.size = sizeof(db_recno_t);				\
    }								\
    else {							\
        key.flags |= DB_DBT_MALLOC;				\
    }								\
}

#define free_key(dbst, key)			\
{						\
    if (key.flags & DB_DBT_MALLOC) {		\
	free(key.data);				\
    }						\
}

#endif

#define test_recno(dbst, key, recno, a)                         \
{                                                               \
    if (dbst->type == DB_RECNO || dbst->type == DB_QUEUE) {     \
        recno = NUM2INT(a);                                     \
        key.data = &recno;                                      \
        key.size = sizeof(db_recno_t);                          \
    }                                                           \
    else {                                                      \
        test_dump(dbst, key, a);                                \
    }                                                           \
}
 
static VALUE
db_put(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    VALUE a, b, c;
    db_DB *dbst;
    DB_TXN *txnid;
    DBT key, data;
    int ret, flags;
    db_recno_t recno;

    rb_secure(4);
    init_txn(txnid, obj, dbst);
    flags = 0;
    a = b = c = Qnil;
    if (rb_scan_args(argc, argv, "21", &a, &b, &c) == 3) {
        flags = NUM2INT(c);
    }
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    test_recno(dbst, key, recno, a);
    if (NIL_P(b)) {
	ret = test_error(dbst->dbp->del(dbst->dbp, txnid, &key, 0));
	if (ret == DB_NOTFOUND)
	    return Qnil;
	else
	    return obj;
    }
    test_dump(dbst, data, b);
#if DB_VERSION_MAJOR == 3
    if (dbst->type == DB_QUEUE && dbst->re_len < data.size) {
	rb_raise(db_eFatal, "size > re_len for Queue");
    }
#endif
    ret = test_error(dbst->dbp->put(dbst->dbp, txnid, &key, &data, flags));
    if (ret == DB_NOOVERWRITE)
	return Qnil;
    else
	return Qtrue;
}

static VALUE
db_assoc(dbst, recno, key, data)
    db_DB *dbst;
    int recno;
    DBT key, data;
{
    if (dbst->type == DB_RECNO || dbst->type == DB_QUEUE)
        return rb_assoc_new(INT2NUM(*(db_recno_t *)key.data), test_load(dbst, data));
    return rb_assoc_new(test_load(dbst, key), test_load(dbst, data));
}

#define set_partial(db, data)			\
    data.flags |= db->partial;			\
    data.dlen = db->dlen;			\
    data.doff = db->doff;


#define test_init_lock(dbst) (((dbst)->flags27 & DB_INIT_LOCK)?DB_RMW:0)

static VALUE
db_get(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    VALUE a, b, c;
    db_DB *dbst;
    DB_TXN *txnid;
    DBT key, data;
    int ret, flags;
    db_recno_t recno;

    init_txn(txnid, obj, dbst);
    flags = 0;
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
    data.flags |= DB_DBT_MALLOC;
    switch(rb_scan_args(argc, argv, "12", &a, &b, &c)) {
    case 3:
        flags = NUM2INT(c);
#if DB_VERSION_MAJOR > 2 || (DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR >= 6)
        if (flags & DB_GET_BOTH) {
            test_dump(dbst, data, b);
        }
#endif
	break;
    case 2:
	flags = NUM2INT(b);
	break;
    }
    test_recno(dbst, key, recno, a);
    set_partial(dbst, data);
    flags |= test_init_lock(dbst);
    ret = test_error(dbst->dbp->get(dbst->dbp, txnid, &key, &data, flags));
    if (ret == DB_NOTFOUND || ret == DB_KEYEMPTY)
        return Qnil;
#if DB_VERSION_MAJOR > 2 || (DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR >= 6)
    if (flags & DB_GET_BOTH)
        return db_assoc(dbst, recno, key, data);
#endif
    if (flags & DB_SET_RECNO)
	return db_assoc(dbst, recno, key, data);
    return test_load(dbst, data);
}

#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1

static VALUE
dbbtree_key_range(obj, a)
    VALUE obj, a;
{
    db_DB *dbst;
    DB_TXN *txnid;
    DBT key;
    int ret, flags;
    db_recno_t recno;
    DB_KEY_RANGE key_range;

    init_txn(txnid, obj, dbst);
    flags = 0;
    memset(&key, 0, sizeof(DBT));
    test_recno(dbst, key, recno, a);
    test_error(dbst->dbp->key_range(dbst->dbp, txnid, &key, &key_range, flags));
    return rb_struct_new(db_sKeyrange, 
			 rb_float_new(key_range.less),
			 rb_float_new(key_range.equal), 
			 rb_float_new(key_range.greater));
}
#endif



static VALUE
db_count(obj, a)
    VALUE obj, a;
{
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    rb_raise(db_eFatal, "DB_NEXT_DUP needs Berkeley DB 2.6 or later");
#else
    db_DB *dbst;
    DB_TXN *txnid;
    DBC *dbcp;
    DBT key, data;
    int ret, flags, flags27;
    db_recno_t recno;
    db_recno_t count;

    init_txn(txnid, obj, dbst);
    flags = 0;
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
    data.flags |= DB_DBT_MALLOC;
    test_recno(dbst, key, recno, a);
    set_partial(dbst, data);
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
    set_partial(dbst, data);
    flags27 = test_init_lock(dbst);
    ret = test_error(dbcp->c_get(dbcp, &key, &data, DB_SET | flags27));
    if (ret == DB_NOTFOUND) {
        test_error(dbcp->c_close(dbcp));
        return INT2NUM(0);
    }
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1
    test_error(dbcp->c_count(dbcp, &count, 0));
    test_error(dbcp->c_close(dbcp));
    return INT2NUM(count);
#else
    count = 1;
    while (1) {
	ret = test_error(dbcp->c_get(dbcp, &key, &data, DB_NEXT_DUP | flags27));
	if (ret == DB_NOTFOUND) {
	    test_error(dbcp->c_close(dbcp));
	    return INT2NUM(count);
	}
	free_key(dbst, key);
	if (data.flags & DB_DBT_MALLOC)
	    free(data.data);
	count++;
    }
    return INT2NUM(-1);
#endif
#endif
}

#if DB_VERSION_MAJOR == 3

static VALUE
db_consume(obj)
    VALUE obj;
{
    db_DB *dbst;
    DB_TXN *txnid;
    DBT key, data;
    int ret;
    db_recno_t recno;

    rb_secure(4);
    init_txn(txnid, obj, dbst);
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    recno = 1;
    key.data = &recno;
    key.size = sizeof(db_recno_t);
    test_error(dbst->dbp->put(dbst->dbp, txnid, &key, &data, DB_CONSUME));
    return db_assoc(dbst, recno, key, data);
}

#endif

static VALUE
db_has_key(obj, key)
    VALUE obj, key;
{
    if (db_get(1, &key, obj) == Qnil)
        return Qfalse;
    else
        return Qtrue;
}

#if DB_VERSION_MAJOR > 2 || (DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR >= 6)
static VALUE
db_has_both(obj, key, value)
    VALUE obj, key, value;
{
    VALUE *argv;
    VALUE flags = INT2FIX(DB_GET_BOTH);
    argv = ALLOCA_N(VALUE, 3);
    argv[0] = key;
    argv[1] = value;
    argv[2] = flags;
    if (db_get(3, argv, obj) == Qnil)
        return Qfalse;
    else
        return Qtrue;
}
#endif

static VALUE
db_del(obj, a)
    VALUE a, obj;
{
    db_DB *dbst;
    DB_TXN *txnid;
    DBT key;
    int ret;
    db_recno_t recno;

    rb_secure(4);
    init_txn(txnid, obj, dbst);
    memset(&key, 0, sizeof(DBT));
    test_recno(dbst, key, recno, a);
    ret = test_error(dbst->dbp->del(dbst->dbp, txnid, &key, 0));
    if (ret == DB_NOTFOUND)
        return Qnil;
    else
        return obj;
}

static VALUE
db_shift(obj)
    VALUE obj;
{
    db_DB *dbst;
    DB_TXN *txnid;
    DBC *dbcp;
    DBT key, data;
    int ret, flags;
    db_recno_t recno;

    rb_secure(4);
    init_txn(txnid, obj, dbst);
    memset(&key, 0, sizeof(key));
    init_recno(dbst, key, recno);
    memset(&data, 0, sizeof(data));
    data.flags = DB_DBT_MALLOC;
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    set_partial(dbst, data);
    flags = test_init_lock(dbst);
    ret = test_error(dbcp->c_get(dbcp, &key, &data, DB_FIRST | flags));
    if (ret == DB_NOTFOUND) {
        test_error(dbcp->c_close(dbcp));
        return Qnil;
    }
    test_error(dbcp->c_del(dbcp, 0));
    test_error(dbcp->c_close(dbcp));
    return db_assoc(dbst, recno, key, data);
}

static VALUE
db_empty(obj)
    VALUE obj;
{
    db_DB *dbst;
    DB_TXN *txnid;
    DBC *dbcp;
    DBT key, data;
    int ret, flags;
    db_recno_t recno;

    init_txn(txnid, obj, dbst);
    memset(&key, 0, sizeof(key));
    init_recno(dbst, key, recno);
    memset(&data, 0, sizeof(data));
    data.flags = DB_DBT_MALLOC;
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    set_partial(dbst, data);
    flags = test_init_lock(dbst);
    ret = test_error(dbcp->c_get(dbcp, &key, &data, DB_FIRST | flags));
    if (ret == DB_NOTFOUND) {
        test_error(dbcp->c_close(dbcp));
        return Qtrue;
    }
    free_key(dbst, key);
    if (data.flags & DB_DBT_MALLOC)
	free(data.data);
    test_error(dbcp->c_close(dbcp));
    return Qfalse;
}

static VALUE
db_delete_if(obj)
    VALUE obj;
{
    db_DB *dbst;
    DB_TXN *txnid;
    DBC *dbcp;
    DBT key, data;
    int ret, flags;
    db_recno_t recno;

    rb_secure(4);
    init_txn(txnid, obj, dbst);
    memset(&key, 0, sizeof(key));
    init_recno(dbst, key, recno);
    memset(&data, 0, sizeof(data));
    data.flags = DB_DBT_MALLOC;
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    set_partial(dbst, data);
    flags = test_init_lock(dbst);
    do {
        ret = test_error(dbcp->c_get(dbcp, &key, &data, DB_NEXT | flags));
        if (ret == DB_NOTFOUND) {
            test_error(dbcp->c_close(dbcp));
            return Qnil;
        }
        if (RTEST(rb_yield(db_assoc(dbst, recno, key, data)))) {
            test_error(dbcp->c_del(dbcp, 0));
        }
    } while (1);
    return obj;
}

static VALUE
db_length(obj)
    VALUE obj;
{
    db_DB *dbst;
    DB_TXN *txnid;
    DBC *dbcp;
    DBT key, data;
    int ret, value, flags;
    db_recno_t recno;

    init_txn(txnid, obj, dbst);
    memset(&key, 0, sizeof(key));
    init_recno(dbst, key, recno);
    memset(&data, 0, sizeof(data));
    data.flags = DB_DBT_MALLOC;
    value = 0;
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    set_partial(dbst, data);
    flags = test_init_lock(dbst);
    do {
        ret = test_error(dbcp->c_get(dbcp, &key, &data, DB_NEXT | flags));
        if (ret == DB_NOTFOUND) {
            test_error(dbcp->c_close(dbcp));
            return INT2NUM(value);
        }
	free_key(dbst, key);
	if (data.flags & DB_DBT_MALLOC)
	    free(data.data);
        value++;
    } while (1);
    return INT2NUM(value);
}

typedef struct {
    int sens;
    db_DB *dbst;
    DBC *dbcp;
} eachst;

static VALUE
db_each_ensure(st)
    eachst *st;
{
    st->dbcp->c_close(st->dbcp);
    return Qnil;
}

static VALUE
db_i_each_value(st)
    eachst *st;
{
    db_DB *dbst;
    DBC *dbcp;
    DBT key, data;
    int ret;
    db_recno_t recno;
    
    dbst = st->dbst;
    dbcp = st->dbcp;
    memset(&key, 0, sizeof(key));
    init_recno(dbst, key, recno);
    memset(&data, 0, sizeof(data));
    data.flags = DB_DBT_MALLOC;
    set_partial(dbst, data);
    do {
        ret = test_error(dbcp->c_get(dbcp, &key, &data, st->sens));
        if (ret == DB_NOTFOUND) {
            return Qnil;
        }
	free_key(dbst, key);
        rb_yield(test_load(dbst, data));
    } while (1);
}

static VALUE
db_each_valuec(obj, sens)
    VALUE obj;
    int sens;
{
    db_DB *dbst;
    DB_TXN *txnid;
    DBC *dbcp;
    eachst st;

    init_txn(txnid, obj, dbst);
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    st.dbst = dbst;
    st.dbcp = dbcp;
    st.sens = sens | test_init_lock(dbst);
    rb_ensure(db_i_each_value, (VALUE)&st, db_each_ensure, (VALUE)&st); 
    return obj;
}

static VALUE db_each_value(obj) VALUE obj;{ return db_each_valuec(obj, DB_NEXT); }
static VALUE db_each_eulav(obj) VALUE obj;{ return db_each_valuec(obj, DB_PREV); }

static VALUE 
db_i_each_key(st)
    eachst *st;
{
    db_DB *dbst;
    DBC *dbcp;
    DBT key, data;
    int ret;
    db_recno_t recno;
    
    dbst = st->dbst;
    dbcp = st->dbcp;
    memset(&key, 0, sizeof(key));
    init_recno(dbst, key, recno);
    memset(&data, 0, sizeof(data));
    data.flags = DB_DBT_MALLOC;
    set_partial(dbst, data);
    do {
        ret = test_error(dbcp->c_get(dbcp, &key, &data, st->sens));
        if (ret == DB_NOTFOUND) {
            return Qnil;
        }
	if (data.flags & DB_DBT_MALLOC)
	    free(data.data);
        if (dbst->type == DB_RECNO || dbst->type == DB_QUEUE)
            rb_yield(INT2NUM(*(db_recno_t *)key.data));
        else
            rb_yield(test_load(dbst, key));
    } while (1);
    return Qnil;
}

static VALUE
db_each_keyc(obj, sens)
    VALUE obj;
    int sens;
{
    db_DB *dbst;
    DB_TXN *txnid;
    DBC *dbcp;
    eachst st;

    init_txn(txnid, obj, dbst);
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    st.dbst = dbst;
    st.dbcp = dbcp;
    st.sens = sens | test_init_lock(dbst);
    rb_ensure(db_i_each_key, (VALUE)&st, db_each_ensure, (VALUE)&st); 
    return obj;
}

static VALUE db_each_key(obj) VALUE obj;{ return db_each_keyc(obj, DB_NEXT); }
static VALUE db_each_yek(obj) VALUE obj;{ return db_each_keyc(obj, DB_PREV); }

static VALUE 
db_i_each_common(st)
    eachst *st;
{
    db_DB *dbst;
    DBC *dbcp;
    DBT key, data;
    int ret;
    db_recno_t recno;
    
    dbst = st->dbst;
    dbcp = st->dbcp;
    memset(&key, 0, sizeof(key));
    init_recno(dbst, key, recno);
    memset(&data, 0, sizeof(data));
    data.flags = DB_DBT_MALLOC;
    set_partial(dbst, data);
    do {
        ret = test_error(dbcp->c_get(dbcp, &key, &data, st->sens));
        if (ret == DB_NOTFOUND) {
            return Qnil;
        }
        rb_yield(db_assoc(dbst, recno, key, data));
    } while (1);
    return Qnil;
}

static VALUE
db_each_common(obj, sens)
    VALUE obj;
    int sens;
{
    db_DB *dbst;
    DB_TXN *txnid;
    DBC *dbcp;
    eachst st;

    init_txn(txnid, obj, dbst);
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    st.dbst = dbst;
    st.dbcp = dbcp;
    st.sens = sens | test_init_lock(dbst);
    rb_ensure(db_i_each_common, (VALUE)&st, db_each_ensure, (VALUE)&st); 
    return obj;
}

static VALUE db_each_pair(obj) VALUE obj;{ return db_each_common(obj, DB_NEXT); }
static VALUE db_each_riap(obj) VALUE obj;{ return db_each_common(obj, DB_PREV); }

static VALUE
db_to_a(obj)
    VALUE obj;
{
    db_DB *dbst;
    DB_TXN *txnid;
    DBC *dbcp;
    DBT key, data;
    int ret, flags;
    db_recno_t recno;
    VALUE ary;

    ary = rb_ary_new();
    init_txn(txnid, obj, dbst);
    memset(&key, 0, sizeof(key));
    init_recno(dbst, key, recno);
    memset(&data, 0, sizeof(data));
    data.flags = DB_DBT_MALLOC;
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    set_partial(dbst, data);
    flags = DB_NEXT | test_init_lock(dbst);
    do {
        ret = test_error(dbcp->c_get(dbcp, &key, &data, flags));
        if (ret == DB_NOTFOUND) {
            test_error(dbcp->c_close(dbcp));
            return ary;
        }
        rb_ary_push(ary, db_assoc(dbst, recno, key, data));
    } while (1);
    return ary;
}

static VALUE
db_values(obj)
    VALUE obj;
{
    db_DB *dbst;
    DB_TXN *txnid;
    DBC *dbcp;
    DBT key, data;
    int ret, flags;
    db_recno_t recno;
    VALUE ary;

    ary = rb_ary_new();
    init_txn(txnid, obj, dbst);
    memset(&key, 0, sizeof(key));
    init_recno(dbst, key, recno);
    memset(&data, 0, sizeof(data));
    data.flags = DB_DBT_MALLOC;
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    set_partial(dbst, data);
    flags = DB_NEXT | test_init_lock(dbst);
    do {
        ret = test_error(dbcp->c_get(dbcp, &key, &data, flags));
        if (ret == DB_NOTFOUND) {
            test_error(dbcp->c_close(dbcp));
            return ary;
        }
	free_key(dbst, key);
        rb_ary_push(ary, test_load(dbst, data));
    } while (1);
    return ary;
}

static VALUE
db_has_value(obj, a)
    VALUE obj, a;
{
    db_DB *dbst;
    DB_TXN *txnid;
    DBC *dbcp;
    DBT key, data;
    int ret, flags;
    db_recno_t recno;

    init_txn(txnid, obj, dbst);
    memset(&key, 0, sizeof(key));
    init_recno(dbst, key, recno);
    memset(&data, 0, sizeof(data));
    data.flags = DB_DBT_MALLOC;
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    set_partial(dbst, data);
    flags = DB_NEXT | test_init_lock(dbst);
    do {
        ret = test_error(dbcp->c_get(dbcp, &key, &data, flags));
        if (ret == DB_NOTFOUND) {
            test_error(dbcp->c_close(dbcp));
            return Qfalse;
        }
	free_key(dbst, key);
        if (rb_funcall(a, id_eql, 1, test_load(dbst, data)) == Qtrue) {
            test_error(dbcp->c_close(dbcp));
            return Qtrue;
        }
    } while (1);
    return Qfalse;
}

static VALUE
db_keys(obj)
    VALUE obj;
{
    db_DB *dbst;
    DB_TXN *txnid;
    DBC *dbcp;
    DBT key, data;
    int ret, flags;
    db_recno_t recno;
    VALUE ary;

    ary = rb_ary_new();
    init_txn(txnid, obj, dbst);
    memset(&key, 0, sizeof(key));
    init_recno(dbst, key, recno);
    memset(&data, 0, sizeof(data));
    data.flags = DB_DBT_MALLOC;
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    set_partial(dbst, data);
    flags = DB_NEXT | test_init_lock(dbst);
    do {
        ret = test_error(dbcp->c_get(dbcp, &key, &data, flags));
        if (ret == DB_NOTFOUND) {
            test_error(dbcp->c_close(dbcp));
            return ary;
        }
	if (data.flags & DB_DBT_MALLOC)
	    free(data.data);
        if (dbst->type == DB_RECNO || dbst->type == DB_QUEUE)
            rb_ary_push(ary, INT2NUM(recno));
        else
            rb_ary_push(ary, test_load(dbst, data));
    } while (1);
    return ary;
}

static VALUE
db_sync(obj)
    VALUE obj;
{
    db_DB *dbst;

    if (!OBJ_TAINTED(obj) && rb_safe_level() >= 4)
	rb_raise(rb_eSecurityError, "Insecure: can't sync the database");
    GetDB(obj, dbst);
    test_error(dbst->dbp->sync(dbst->dbp, 0));
    return Qtrue;
}

#if DB_VERSION_MAJOR == 3
static VALUE
dbhash_stat(obj)
    VALUE obj;
{
    db_DB *dbst;
    DB_HASH_STAT *stat;
    VALUE hash;
    int ret;
    GetDB(obj, dbst);
    test_error(dbst->dbp->stat(dbst->dbp, &stat, 0, 0));
    hash = rb_hash_new();
    rb_hash_aset(hash, rb_tainted_str_new2("hash_magic"), INT2NUM(stat->hash_magic));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_version"), INT2NUM(stat->hash_version));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_pagesize"), INT2NUM(stat->hash_pagesize));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_nrecs"), INT2NUM(stat->hash_nrecs));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_nelem"), INT2NUM(stat->hash_nelem));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_ffactor"), INT2NUM(stat->hash_ffactor));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_buckets"), INT2NUM(stat->hash_buckets));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_free"), INT2NUM(stat->hash_free));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_bfree"), INT2NUM(stat->hash_bfree));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_bigpages"), INT2NUM(stat->hash_bigpages));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_big_bfree"), INT2NUM(stat->hash_big_bfree));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_overflows"), INT2NUM(stat->hash_overflows));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_ovfl_free"), INT2NUM(stat->hash_ovfl_free));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_dup"), INT2NUM(stat->hash_dup));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_dup_free"), INT2NUM(stat->hash_dup_free));
    free(stat);
    return hash;
}
#endif

static VALUE
dbtree_stat(obj)
    VALUE obj;
{
    db_DB *dbst;
    DB_BTREE_STAT *stat;
    VALUE hash;

    GetDB(obj, dbst);
    test_error(dbst->dbp->stat(dbst->dbp, &stat, 0, 0));
    hash = rb_hash_new();
    rb_hash_aset(hash, rb_tainted_str_new2("bt_magic"), INT2NUM(stat->bt_magic));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_version"), INT2NUM(stat->bt_version));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_dup_pg"), INT2NUM(stat->bt_dup_pg));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_dup_pgfree"), INT2NUM(stat->bt_dup_pgfree));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_free"), INT2NUM(stat->bt_free));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_int_pg"), INT2NUM(stat->bt_int_pg));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_int_pgfree"), INT2NUM(stat->bt_int_pgfree));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_leaf_pg"), INT2NUM(stat->bt_leaf_pg));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_leaf_pgfree"), INT2NUM(stat->bt_leaf_pgfree));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_levels"), INT2NUM(stat->bt_levels));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_minkey"), INT2NUM(stat->bt_minkey));
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1
    rb_hash_aset(hash, rb_tainted_str_new2("bt_nrecs"), INT2NUM(stat->bt_nkeys));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_nkeys"), INT2NUM(stat->bt_nkeys));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_ndata"), INT2NUM(stat->bt_ndata));
#else
    rb_hash_aset(hash, rb_tainted_str_new2("bt_nrecs"), INT2NUM(stat->bt_nrecs));
#endif
    rb_hash_aset(hash, rb_tainted_str_new2("bt_over_pg"), INT2NUM(stat->bt_over_pg));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_over_pgfree"), INT2NUM(stat->bt_over_pgfree));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_pagesize"), INT2NUM(stat->bt_pagesize));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_re_len"), INT2NUM(stat->bt_re_len));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_re_pad"), INT2NUM(stat->bt_re_pad));
    free(stat);
    return hash;
}

static VALUE
dbrecno_length(obj)
    VALUE obj;
{
    db_DB *dbst;
    DB_BTREE_STAT *stat;
    VALUE hash;

    GetDB(obj, dbst);
    test_error(dbst->dbp->stat(dbst->dbp, &stat, 0, DB_RECORDCOUNT));
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1
    hash = rb_assoc_new(INT2NUM(stat->bt_nkeys), INT2NUM(stat->bt_ndata));
#else
    hash = rb_assoc_new(INT2NUM(stat->bt_nrecs), 0);
#endif
    free(stat);
    return hash;
}

#if DB_VERSION_MAJOR == 3
static VALUE
dbqueue_stat(obj)
    VALUE obj;
{
    db_DB *dbst;
    DB_QUEUE_STAT *stat;
    VALUE hash;

    GetDB(obj, dbst);
    test_error(dbst->dbp->stat(dbst->dbp, &stat, 0, 0));
    hash = rb_hash_new();
    rb_hash_aset(hash, rb_tainted_str_new2("qs_magic"), INT2NUM(stat->qs_magic));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_version"), INT2NUM(stat->qs_version));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_nrecs"), INT2NUM(stat->qs_nrecs));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_pages"), INT2NUM(stat->qs_pages));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_pagesize"), INT2NUM(stat->qs_pagesize));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_pgfree"), INT2NUM(stat->qs_pgfree));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_re_len"), INT2NUM(stat->qs_re_len));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_re_pad"), INT2NUM(stat->qs_re_pad));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_start"), INT2NUM(stat->qs_start));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_first_recno"), INT2NUM(stat->qs_first_recno));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_cur_recno"), INT2NUM(stat->qs_cur_recno));
    free(stat);
    return hash;
}
#endif

static VALUE
db_set_partial(obj, a, b)
    VALUE obj, a, b;
{
    db_DB *dbst;
    VALUE ret;

    GetDB(obj, dbst);
    ret = rb_ary_new2(3);
    rb_ary_push(ret, (dbst->partial == DB_DBT_PARTIAL)?Qtrue:Qnil);
    rb_ary_push(ret, INT2NUM(dbst->doff));
    rb_ary_push(ret, INT2NUM(dbst->dlen));
    dbst->doff = NUM2UINT(a);
    dbst->dlen = NUM2UINT(b);
    dbst->partial = DB_DBT_PARTIAL;
    return ret;
}

static VALUE
db_clear_partial(obj)
    VALUE obj;
{
    db_DB *dbst;
    VALUE ret;

    GetDB(obj, dbst);
    ret = rb_ary_new2(3);
    rb_ary_push(ret, (dbst->partial == DB_DBT_PARTIAL)?Qtrue:Qnil);
    rb_ary_push(ret, INT2NUM(dbst->doff));
    rb_ary_push(ret, INT2NUM(dbst->dlen));
    dbst->doff = dbst->dlen = dbst->partial = 0;
    return ret;
}

#if DB_VERSION_MAJOR == 3
static VALUE
db_i_create(obj)
    VALUE obj;
{
    DB *dbp;
    db_ENV *dbenvst;
    DB_ENV *dbenvp;
    db_DB *dbst;
    VALUE ret, env;
    int val;

    dbenvp = NULL;
    env = 0;
    if (rb_obj_is_kind_of(obj, db_cEnv)) {
        GetEnvDB(obj, dbenvst);
        dbenvp = dbenvst->dbenvp;
        env = obj;
    }
    test_error(db_create(&dbp, dbenvp, 0), val, "create");
    dbp->set_errpfx(dbp, "BDB::");
    dbp->set_errcall(dbp, dbenv_errcall);
    ret = Data_Make_Struct(db_cCommon, db_DB, 0, db_free, dbst);
    rb_obj_call_init(ret, 0, 0);
    dbst->env = env;
    dbst->dbp = dbp;
    if (dbenvp)
	dbst->flags27 = dbenvp->flags;
    return ret;
}
#endif

static VALUE
db_s_upgrade(argc, argv, obj)
    VALUE obj;
    int argc;
    VALUE *argv;
{
#if DB_VERSION_MAJOR == 3
    db_DB *dbst;
    VALUE a, b;
    int flags;
    VALUE val;
    int ret;

    rb_secure(4);
    flags = 0;
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	flags = NUM2INT(b);
    }
    Check_SafeStr(a);
    val = db_i_create(obj);
    GetDB(val, dbst);
    test_error(dbst->dbp->upgrade(dbst->dbp, RSTRING(a)->ptr, flags));
    return val;

#else
    rb_raise(db_eFatal, "You can't upgrade a database with this version of DB");
#endif
}

static VALUE
db_s_remove(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
#if DB_VERSION_MAJOR == 3
    db_DB *dbst;
    int ret;
    VALUE a, b, c;
    char *name, *subname;

    rb_secure(2);
    c = db_i_create(obj);
    GetDB(c, dbst);
    name = subname = NULL;
    a = b = Qnil;
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
        if (!NIL_P(b)) {
	    Check_SafeStr(b);
            subname = RSTRING(b)->ptr;
        }
    }
    Check_SafeStr(a);
    name = RSTRING(a)->ptr;
    test_error(dbst->dbp->remove(dbst->dbp, name, subname, 0));
#endif
    return Qtrue;
}

static void dbcursor_free(db_DBC *dbcst);

#define GetCursorDB(obj, dbcst)                 \
{                                               \
    Data_Get_Struct(obj, db_DBC, dbcst);        \
    if (dbcst->dbc == 0)                        \
        rb_raise(db_eFatal, "closed cursor");   \
}

#if DB_VERSION_MAJOR > 2 || (DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR >= 6)

static VALUE
db_i_joinclose(st)
    eachst *st;
{
    if (st->dbcp && st->dbst && st->dbst->dbp) {
	st->dbcp->c_close(st->dbcp);
    }
    return Qnil;
}

 
static VALUE
db_i_join(st)
    eachst *st;
{
    int ret;
    DBT key, data;
    db_recno_t recno;

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    init_recno(st->dbst, key, recno);
    data.flags |= DB_DBT_MALLOC;
    set_partial(st->dbst, data);
    do {
	ret = test_error(st->dbcp->c_get(st->dbcp, &key, &data, st->sens));
	if (ret  == DB_NOTFOUND || ret == DB_KEYEMPTY)
	    return Qnil;
	rb_yield(db_assoc(st->dbst, recno, key, data));
    } while (1);
    return Qnil;
}
 
static VALUE
db_join(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    db_DB *dbst;
    db_DBC *dbcst;
    DBC *dbc, **dbcarr;
    int flags, i;
    eachst st;
    VALUE a, b, c;

    c = 0;
#if DB_VERSION_MAJOR == 2 && (DB_VERSION_MINOR < 5 || (DB_VERSION_MINOR == 5 && DB_VERSION_PATCH < 2))
    rb_raise(db_eFatal, "join needs Berkeley DB 2.5.2 or later");
#endif
    flags = 0;
    GetDB(obj, dbst);
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	flags = NUM2INT(b);
    }
    if (TYPE(a) != T_ARRAY) {
        rb_raise(db_eFatal, "first argument must an array of cursors");
    }
    if (RARRAY(a)->len == 0) {
        rb_raise(db_eFatal, "empty array");
    }
    dbcarr = ALLOCA_N(DBC *, RARRAY(a)->len + 1);
    {
	DBC **dbs;

	for (dbs = dbcarr, i = 0; i < RARRAY(a)->len; i++, dbs++) {
	    if (!rb_obj_is_kind_of(RARRAY(a)->ptr[i], db_cCursor)) {
		rb_raise(db_eFatal, "element %d is not a cursor");
	    }
	    GetCursorDB(RARRAY(a)->ptr[i], dbcst);
	    *dbs = dbcst->dbc;
	}
	*dbs = 0;
    }
    dbc = 0;
#if DB_VERSION_MAJOR < 3
    test_error(dbst->dbp->join(dbst->dbp, dbcarr, 0, &dbc));
#else
    test_error(dbst->dbp->join(dbst->dbp, dbcarr, &dbc, 0));
#endif
    st.dbst = dbst;
    st.dbcp = dbc;
    st.sens = flags | test_init_lock(dbst);
    rb_ensure(db_i_join, (VALUE)&st, db_i_joinclose, (VALUE)&st);
    return obj;
}

static VALUE
db_byteswapp(obj)
    VALUE obj;
{
    db_DB *dbst;
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 5
    rb_raise(db_eFatal, "byteswapped needs Berkeley DB 2.5 or later");
#endif
    GetDB(obj, dbst);
#if DB_VERSION_MAJOR < 3
    return dbst->dbp->byteswapped?Qtrue:Qfalse;
#else
    return dbst->dbp->get_byteswapped(dbst->dbp)?Qtrue:Qfalse;
#endif
}

#endif
/* CURSOR */

static void 
dbcursor_free(dbcst)
    db_DBC *dbcst;
{
    if (dbcst->dbc && dbcst->dbst && dbcst->dbst->dbp) {
        test_error(dbcst->dbc->c_close(dbcst->dbc));
        dbcst->dbc = NULL;
    }
    free(dbcst);
}

static VALUE
db_cursor(obj)
    VALUE obj;
{
    db_DB *dbst;
    DB_TXN *txnid;
    db_DBC *dbcst;
    DBC *dbc;
    VALUE a;
    int flags;

    init_txn(txnid, obj, dbst);
    flags = 0;
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbc));
#else
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbc, flags));
#endif
    a = Data_Make_Struct(db_cCursor, db_DBC, 0, dbcursor_free, dbcst);
    rb_obj_call_init(a, 0, 0);
    dbcst->dbc = dbc;
    dbcst->dbst = dbst;
    return a;
}

static VALUE
dbcursor_close(obj)
    VALUE obj;
{
    db_DBC *dbcst;
    
    if (!OBJ_TAINTED(obj) && rb_safe_level() >= 4)
	rb_raise(rb_eSecurityError, "Insecure: can't close the cursor");
    GetCursorDB(obj, dbcst);
    test_error(dbcst->dbc->c_close(dbcst->dbc));
    dbcst->dbc = NULL;
    return Qtrue;
}
  
static VALUE
dbcursor_del(obj)
    VALUE obj;
{
    int flags = 0;
    db_DBC *dbcst;
    
    rb_secure(4);
    GetCursorDB(obj, dbcst);
    test_error(dbcst->dbc->c_del(dbcst->dbc, flags));
    return Qtrue;
}

#if DB_VERSION_MAJOR == 3

static VALUE
dbcursor_dup(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    int ret, flags = 0;
    VALUE a, b;
    db_DBC *dbcst, *dbcstdup;
    DBC *dbcdup;
    
    if (rb_scan_args(argc, argv, "01", &a))
        flags = NUM2INT(a);
    GetCursorDB(obj, dbcst);
    test_error(dbcst->dbc->c_dup(dbcst->dbc, &dbcdup, flags));
    b = Data_Make_Struct(db_cCursor, db_DBC, 0, dbcursor_free, dbcstdup);
    rb_obj_call_init(b, 0, 0);
    dbcstdup->dbc = dbcdup;
    dbcstdup->dbst = dbcst->dbst;
    return b;
}

#endif

static VALUE
dbcursor_count(obj)
    VALUE obj;
{
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    rb_raise(db_eFatal, "DB_NEXT_DUP needs Berkeley DB 2.6 or later");
#else
    int ret;

#if !(DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1)
    DBT key, data;
    DBT key_o, data_o;
#endif
    db_DBC *dbcst;
    db_recno_t count;

    GetCursorDB(obj, dbcst);
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1
    test_error(dbcst->dbc->c_count(dbcst->dbc, &count, 0));
    return INT2NUM(count);
#else
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.flags |= DB_DBT_MALLOC;
    data.flags |= DB_DBT_MALLOC;
    memset(&key_o, 0, sizeof(key_o));
    memset(&data_o, 0, sizeof(data_o));
    key_o.flags |= DB_DBT_MALLOC;
    data_o.flags |= DB_DBT_MALLOC;
    set_partial(dbcst->dbst, data);
    ret = test_error(dbcst->dbc->c_get(dbcst->dbc, &key_o, &data_o, DB_CURRENT));
    if (ret == DB_NOTFOUND || ret == DB_KEYEMPTY)
        return INT2NUM(0);
    count = 1;
    while (1) {
	ret = test_error(dbcst->dbc->c_get(dbcst->dbc, &key, &data, DB_NEXT_DUP));
	if (ret == DB_NOTFOUND) {
	    test_error(dbcst->dbc->c_get(dbcst->dbc, &key_o, &data_o, DB_SET));

	    free_key(dbcst->dbst, key_o);
	    if (data_o.flags & DB_DBT_MALLOC)
		free(data_o.data);
	    return INT2NUM(count);
	}
	free_key(dbst, key);
	if (data.flags & DB_DBT_MALLOC)
	    free(data.data);
	count++;
    }
    return INT2NUM(-1);
#endif
#endif
}

static VALUE
dbcursor_get(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    VALUE a, b, c;
    int flags, cnt, ret;
    DBT key, data;
    db_DBC *dbcst;
    db_recno_t recno;

    cnt = rb_scan_args(argc, argv, "12", &a, &b, &c);
    flags = NUM2INT(a);
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    GetCursorDB(obj, dbcst);
    if (flags == DB_SET_RECNO) {
	if (dbcst->dbst->type != DB_BTREE || !(dbcst->dbst->flags & DB_RECNUM)) {
	    rb_raise(db_eFatal, "database must be Btree with RECNUM for SET_RECNO");
	}
        if (cnt != 2)
            rb_raise(db_eFatal, "invalid number of arguments");
	recno = NUM2INT(b);
	key.data = &recno;
	key.size = sizeof(db_recno_t);
	key.flags |= DB_DBT_MALLOC;
	data.flags |= DB_DBT_MALLOC;
    }
    else if (flags == DB_SET || flags == DB_SET_RANGE) {
        if (cnt != 2)
            rb_raise(db_eFatal, "invalid number of arguments");
        test_recno(dbcst->dbst, key, recno, b);
	data.flags |= DB_DBT_MALLOC;
    }
#if DB_VERSION_MAJOR > 2 || (DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR >= 6)
    else if (flags == DB_GET_BOTH) {
        if (cnt != 3)
            rb_raise(db_eFatal, "invalid number of arguments");
        test_recno(dbcst->dbst, key, recno, b);
        test_dump(dbcst->dbst, data, c);
    }
#endif
    else {
	if (cnt != 1) {
	    rb_raise(db_eFatal, "invalid number of arguments");
	}
	key.flags |= DB_DBT_MALLOC;
	data.flags |= DB_DBT_MALLOC;
    }
    set_partial(dbcst->dbst, data);
    ret = test_error(dbcst->dbc->c_get(dbcst->dbc, &key, &data, flags));
    if (ret == DB_NOTFOUND || ret == DB_KEYEMPTY)
        return Qnil;
    return db_assoc(dbcst->dbst, recno, key, data);
}

static VALUE
dbcursor_set_xxx(obj, a, flag)
    VALUE obj, a;
    int flag;
{
    VALUE *b;
    b = ALLOCA_N(VALUE, 2);
    b[0] = INT2NUM(flag);
    b[1] = a;
    return dbcursor_get(2, b, obj);
}

static VALUE 
dbcursor_set(obj, a)
    VALUE obj, a;
{ 
    return dbcursor_set_xxx(obj, a, DB_SET);
}

static VALUE 
dbcursor_set_range(obj, a)
    VALUE obj, a;
{ 
    return dbcursor_set_xxx(obj, a, DB_SET_RANGE);
}

static VALUE 
dbcursor_set_recno(obj, a)
    VALUE obj, a;
{ 
    return dbcursor_set_xxx(obj, a, DB_SET_RECNO);
}

static VALUE
dbcursor_xxx(obj, val)
    VALUE obj;
    int val;
{
    VALUE b;
    b = INT2NUM(val);
    return dbcursor_get(1, &b, obj);
}

static VALUE
dbcursor_next(obj)
    VALUE obj;
{
    return dbcursor_xxx(obj, DB_NEXT);
}

static VALUE
dbcursor_prev(obj)
    VALUE obj;
{
    return dbcursor_xxx(obj, DB_PREV);
}

static VALUE
dbcursor_first(obj)
    VALUE obj;
{
    return dbcursor_xxx(obj, DB_FIRST);
}

static VALUE
dbcursor_last(obj)
    VALUE obj;
{
    return dbcursor_xxx(obj, DB_LAST);
}

static VALUE
dbcursor_current(obj)
    VALUE obj;
{
    return dbcursor_xxx(obj, DB_CURRENT);
}

static VALUE
dbcursor_put(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    int flags, cnt;
    DBT key, data;
    db_DBC *dbcst;
    VALUE a, b, c;
    db_recno_t recno;

    rb_secure(4);
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    cnt = rb_scan_args(argc, argv, "21", &a, &b, &c);
    GetCursorDB(obj, dbcst);
    flags = NUM2INT(c);
    if (flags & (DB_KEYFIRST | DB_KEYLAST)) {
        if (cnt != 3)
            rb_raise(db_eFatal, "invalid number of arguments");
        test_recno(dbcst->dbst, key, recno, b);
        test_dump(dbcst->dbst, data, c);
    }
    else {
        test_dump(dbcst->dbst, data, b);
    }
    test_error(dbcst->dbc->c_put(dbcst->dbc, &key, &data, flags));
    if (cnt == 3) {
	free_key(dbcst->dbst, key);
    }
    if (data.flags & DB_DBT_MALLOC)
	free(data.data);
    return Qtrue;
}

/* TRANSACTION */

static void 
dbtxn_free(txnst)
    db_TXN *txnst;
{
    if (txnst->txnid && txnst->parent == NULL) {
        test_error(txn_abort(txnst->txnid));
        txnst->txnid = NULL;
    }
    free(txnst);
}

static void 
dbtxn_mark(txnst)
    db_TXN *txnst;
{
    rb_gc_mark(txnst->db_ary);
}

#define GetTxnDB(obj, txnst)                            \
{                                                       \
    Data_Get_Struct(obj, db_TXN, txnst);                \
    if (txnst->txnid == 0)                              \
        rb_raise(db_eFatal, "closed transaction");      \
}

static VALUE
dbtxn_assoc(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    int i;
    VALUE ary, a;
    db_TXN *txnst;
    db_DB *dbp, *dbh;

    ary = rb_ary_new();
    GetTxnDB(obj, txnst);
    for (i = 0; i < argc; i++) {
        if (!rb_obj_is_kind_of(argv[i], db_cCommon))
            rb_raise(db_eFatal, "argument must be a database handle");
        a = Data_Make_Struct(CLASS_OF(argv[i]), db_DB, 0, db_free, dbh);
        GetDB(argv[i], dbp);
        MEMCPY(dbh, dbp, db_DB, 1);
        dbh->txn = txnst;
	dbh->options |= DB_RB_TXN;
	dbh->flags27 = txnst->flags27;
        rb_obj_call_init(a, 0, 0);
        rb_ary_push(ary, a);
    }
    switch (RARRAY(ary)->len) {
    case 0: return Qnil;
    case 1: return RARRAY(ary)->ptr[0];
    default: return ary;
    }
}

static VALUE
dbtxn_commit(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    db_TXN *txnst;
    VALUE a, db;
    int flags;

    rb_secure(4);
    flags = 0;
    if (rb_scan_args(argc, argv, "01", &a) == 1) {
        flags = NUM2INT(a);
    }
    GetTxnDB(obj, txnst);
#if DB_VERSION_MAJOR < 3
    test_error(txn_commit(txnst->txnid));
#else
    test_error(txn_commit(txnst->txnid, flags));
#endif
    while ((db = rb_ary_pop(txnst->db_ary)) != Qnil) {
	db_close(0, 0, db);
    }
    txnst->txnid = NULL;
    if (txnst->status == 1) {
	txnst->status = 0;
	rb_throw("__bdb__begin", Data_Wrap_Struct(db_cTxnCatch, 0, 0, txnst));
    }
    return Qtrue;
}

static VALUE
dbtxn_abort(obj)
    VALUE obj;
{
    db_TXN *txnst;
    VALUE db;

    GetTxnDB(obj, txnst);
    test_error(txn_abort(txnst->txnid));
    while ((db = rb_ary_pop(txnst->db_ary)) != Qnil) {
	db_close(0, 0, db);
    }
    txnst->txnid = NULL;
    if (txnst->status == 1) {
	txnst->status = 0;
	rb_throw("__bdb__begin", Data_Wrap_Struct(db_cTxnCatch, 0, 0, txnst));
    }
    return Qtrue;
}

#define DBD_TXN_COMMIT 0x80

static VALUE
bdb_catch(val, args, self)
    VALUE val, args, self;
{
    rb_yield(args);
    return Qtrue;
}

static VALUE
dbenv_begin(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    db_TXN *txnst, *txnstpar;
    DB_TXN *txn, *txnpar;
    DB_ENV *dbenvp;
    db_ENV *dbenvst;
    VALUE txnv, b, env;
    int flags, commit, marshal;

    txnpar = 0;
    flags = commit = 0;
    dbenvst = 0;
    env = 0;
    if (argc >= 1 && !rb_obj_is_kind_of(argv[0], db_cCommon)) {
        flags = NUM2INT(argv[0]);
	if (flags & DBD_TXN_COMMIT) {
	    commit = 1;
	    flags &= ~DBD_TXN_COMMIT;
	}
        argc--; argv++;
    }
    if (rb_obj_is_kind_of(obj, db_cTxn)) {
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
	rb_raise(db_eFatal, "Transaction Manager not enabled");
    }
    test_error(txn_begin(dbenvp->tx_info, txnpar, &txn));
#else
    test_error(txn_begin(dbenvp, txnpar, &txn, flags));
#endif
    txnv = Data_Make_Struct(db_cTxn, db_TXN, dbtxn_mark, dbtxn_free, txnst);
    rb_obj_call_init(txnv, argc, argv);
    txnst->env = env;
    txnst->marshal = marshal;
    txnst->txnid = txn;
    txnst->parent = txnpar;
    txnst->status = 0;
    txnst->flags27 = dbenvst->flags27;
    txnst->db_ary = rb_ary_new2(0);
    b = dbtxn_assoc(argc, argv, txnv);
    if (rb_iterator_p()) {
	VALUE result;
	txnst->status = 1;
	result = rb_catch("__bdb__begin", bdb_catch, (b == Qnil)?txnv:rb_assoc_new(txnv, b));
	if (rb_obj_is_kind_of(result, db_cTxnCatch)) {
	    db_TXN *txn1;
	    Data_Get_Struct(result, db_TXN, txn1);
	    if (txn1 == txnst)
		return Qnil;
	    else
		rb_throw("__bdb__begin", result);
	}
	txnst->status = 0;
	if (txnst->txnid) {
	    if (commit)
		dbtxn_commit(0, 0, txnv);
	    else
		dbtxn_abort(txnv);
	}
	return Qnil;
    }
    if (b == Qnil)
        return txnv;
    else
        return rb_assoc_new(txnv, b);
}

static VALUE
dbtxn_id(obj)
    VALUE obj;
{
    db_TXN *txnst;
    GetTxnDB(obj, txnst);
    return INT2FIX(txn_id(txnst->txnid));
}

static VALUE
dbtxn_prepare(obj)
    VALUE obj;
{
    db_TXN *txnst;

    GetTxnDB(obj, txnst);
    test_error(txn_prepare(txnst->txnid));
    return Qtrue;
}
        
/* LOCK */

static VALUE
dbenv_lockid(obj)
    VALUE obj;
{
    unsigned int idp;
    db_ENV *dbenvst;
    db_LOCKID *dblockid;
    VALUE a;

    GetEnvDB(obj, dbenvst);
#if DB_VERSION_MAJOR < 3
    test_error(lock_id(dbenvst->dbenvp->lk_info, &idp));
#else
    test_error(lock_id(dbenvst->dbenvp, &idp));
#endif
    a = Data_Make_Struct(db_cLockid, db_LOCKID, 0, free, dblockid);
    dblockid->lock = idp;
    dblockid->dbenvp = dbenvst->dbenvp;
    return a;
}

static VALUE
dbenv_lockdetect(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    VALUE a, b;
    db_ENV *dbenvst;
    int flags, atype, aborted;

    flags = atype = aborted = 0;
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	flags = NUM2INT(b);
    }
    atype = NUM2INT(a);
    GetEnvDB(obj, dbenvst);
#if DB_VERSION_MAJOR < 3
    test_error(lock_detect(dbenvst->dbenvp->lk_info, flags, atype));
#else
    test_error(lock_detect(dbenvst->dbenvp, flags, atype, &aborted));
#endif
    return INT2NUM(aborted);
}

static VALUE
dbenv_lockstat(obj)
    VALUE obj;
{
    db_ENV *dbenvst;
    DB_LOCK_STAT *statp;
    VALUE a;

    GetEnvDB(obj, dbenvst);
#if DB_VERSION_MAJOR < 3
    test_error(lock_stat(dbenvst->dbenvp->lk_info, &statp, 0));
    a = rb_hash_new();
    rb_hash_aset(a, rb_tainted_str_new2("st_magic"), INT2NUM(statp->st_magic));
    rb_hash_aset(a, rb_tainted_str_new2("st_version"), INT2NUM(statp->st_version));
    rb_hash_aset(a, rb_tainted_str_new2("st_refcnt"), INT2NUM(statp->st_refcnt));
    rb_hash_aset(a, rb_tainted_str_new2("st_numobjs"), INT2NUM(statp->st_numobjs));
    rb_hash_aset(a, rb_tainted_str_new2("st_regsize"), INT2NUM(statp->st_regsize));
    rb_hash_aset(a, rb_tainted_str_new2("st_maxlocks"), INT2NUM(statp->st_maxlocks));
    rb_hash_aset(a, rb_tainted_str_new2("st_nmodes"), INT2NUM(statp->st_nmodes));
    rb_hash_aset(a, rb_tainted_str_new2("st_nlockers"), INT2NUM(statp->st_nlockers));
    rb_hash_aset(a, rb_tainted_str_new2("st_nconflicts"), INT2NUM(statp->st_nconflicts));
    rb_hash_aset(a, rb_tainted_str_new2("st_nrequests"), INT2NUM(statp->st_nrequests));
    rb_hash_aset(a, rb_tainted_str_new2("st_ndeadlocks"), INT2NUM(statp->st_ndeadlocks));
    rb_hash_aset(a, rb_tainted_str_new2("st_region_wait"), INT2NUM(statp->st_region_wait));
    rb_hash_aset(a, rb_tainted_str_new2("st_region_nowait"), INT2NUM(statp->st_region_nowait));
#else
    test_error(lock_stat(dbenvst->dbenvp, &statp, 0));
    a = rb_hash_new();
    rb_hash_aset(a, rb_tainted_str_new2("st_lastid"), INT2NUM(statp->st_lastid));
    rb_hash_aset(a, rb_tainted_str_new2("st_nmodes"), INT2NUM(statp->st_nmodes));
    rb_hash_aset(a, rb_tainted_str_new2("st_maxlocks"), INT2NUM(statp->st_maxlocks));
    rb_hash_aset(a, rb_tainted_str_new2("st_nlockers"), INT2NUM(statp->st_nlockers));
    rb_hash_aset(a, rb_tainted_str_new2("st_maxnlockers"), INT2NUM(statp->st_maxnlockers));
    rb_hash_aset(a, rb_tainted_str_new2("st_nconflicts"), INT2NUM(statp->st_nconflicts));
    rb_hash_aset(a, rb_tainted_str_new2("st_nrequests"), INT2NUM(statp->st_nrequests));
    rb_hash_aset(a, rb_tainted_str_new2("st_ndeadlocks"), INT2NUM(statp->st_ndeadlocks));
    rb_hash_aset(a, rb_tainted_str_new2("st_regsize"), INT2NUM(statp->st_regsize));
    rb_hash_aset(a, rb_tainted_str_new2("st_region_wait"), INT2NUM(statp->st_region_wait));
    rb_hash_aset(a, rb_tainted_str_new2("st_region_nowait"), INT2NUM(statp->st_region_nowait));
#endif
    free(statp);
    return a;
}

#define GetLockid(obj, lockid)				\
{							\
    Data_Get_Struct(obj, db_LOCKID, lockid);		\
    if (lockid->dbenvp == 0)				\
        rb_raise(db_eFatal, "closed environment");	\
}

static void
lock_free(lock)
    db_LOCK *lock;
{
    if (lock->dbenvp) {
#if DB_VERSION_MAJOR < 3
	lock_put(lock->dbenvp->lk_info, lock->lock);
#else
	lock_put(lock->dbenvp, lock->lock);
	free(lock->lock);
#endif
    }
    free(lock);
}

static VALUE
dblockid_get(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    db_LOCKID *lockid;
    DB_LOCK lock;
    db_LOCK *lockst;
    DBT objet;
    unsigned int flags;
    int lock_mode;
    VALUE a, b, c, res;

    rb_secure(2);
    flags = 0;
    if (rb_scan_args(argc, argv, "21", &a, &b, &c) == 3) {
	flags = NUM2UINT(c);
    }
    Check_SafeStr(a);
    memset(&objet, 0, sizeof(objet));
    objet.data = RSTRING(a)->ptr;
    objet.size = RSTRING(a)->len;
    lock_mode = NUM2INT(b);
    GetLockid(obj, lockid);
#if DB_VERSION_MAJOR < 3
    test_error(lock_get(lockid->dbenvp->lk_info, lockid->lock, flags,
			&objet, lock_mode, &lock));
#else
    test_error(lock_get(lockid->dbenvp, lockid->lock, flags,
			&objet, lock_mode, &lock));
#endif
    res = Data_Make_Struct(db_cLock, db_LOCK, 0, lock_free, lockst);
    rb_obj_call_init(res, 0, 0);
#if DB_VERSION_MAJOR < 3
    lockst->lock = lock;
#else
    lockst->lock = ALLOC(DB_LOCK);
    MEMCPY(lockst->lock, &lock, sizeof(lock), 1);
#endif
    lockst->dbenvp = lockid->dbenvp;
    return res;
} 

#define GetLock(obj, lock)				\
{							\
    Data_Get_Struct(obj, db_LOCK, lock);		\
    if (lock->dbenvp == 0)				\
        rb_raise(db_eFatal, "closed environment");	\
}

static VALUE
dblockid_each(obj, list)
    VALUE obj;
    DB_LOCKREQ *list;
{
    VALUE key, value;
    char *options;

    key = rb_ary_entry(obj, 0);
    value = rb_ary_entry(obj, 1);
    key = rb_funcall(key, rb_intern("to_s"), 0);
    options = RSTRING(key)->ptr;
    if (strcmp(options, "op") == 0) {
	list->op = NUM2INT(value);
    }
    else if (strcmp(options, "obj") == 0) {
	Check_Type(value, T_STRING);
	list->obj = ALLOC(DBT);
	MEMZERO(list->obj, DBT, 1);
	list->obj->data = RSTRING(value)->ptr;
	list->obj->size = RSTRING(value)->len;
    }
    else if (strcmp(options, "mode") == 0) {
	list->mode = NUM2INT(value);
    }
    else if (strcmp(options, "lock") == 0) {
	db_LOCK *lockst;

	if (!rb_obj_is_kind_of(value, db_cLock)) {
	    rb_raise(db_eFatal, "BDB::Lock expected");
	}
	GetLock(obj, lockst);
#if DB_VERSION_MAJOR < 3
	list->lock = lockst->lock;
#else
	MEMCPY(&list->lock, lockst->lock, sizeof(list->lock), 1);
#endif
    }
    return Qnil;
}

static VALUE
dblockid_vec(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    DB_LOCKREQ *list;
    db_LOCKID *lockid;
    db_LOCK *lockst;
    unsigned int flags;
    VALUE a, b, c, res;
    int i, n, err;

    flags = 0;
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	flags = NUM2UINT(b);
    }
    Check_Type(a, T_ARRAY);
    list = ALLOCA_N(DB_LOCKREQ, RARRAY(a)->len);
    MEMZERO(list, DB_LOCKREQ, RARRAY(a)->len);
    for (i = 0; i < RARRAY(a)->len; i++) {
	b = RARRAY(a)->ptr[i];
	Check_Type(b, T_HASH);
	rb_iterate(rb_each, b, dblockid_each, (VALUE)&list[i]);
    }
    GetLockid(obj, lockid);
#if DB_VERSION_MAJOR < 3
    err = lock_vec(lockid->dbenvp->lk_info, lockid->lock, flags,
		   list, RARRAY(a)->len, NULL);
#else
    err = lock_vec(lockid->dbenvp, lockid->lock, flags,
		   list, RARRAY(a)->len, NULL);
#endif
    if (err != 0) {
	for (i = 0; i < RARRAY(a)->len; i++) {
	    if (list[i].obj)
		free(list[i].obj);
	}
	res = (err == DB_LOCK_DEADLOCK)?db_eLock:db_eFatal;
        if (dberrcall) {
            dberrcall = 0;
            rb_raise(res, "%s -- %s", RSTRING(dberrstr)->ptr, db_strerror(err));
        }
        else
            rb_raise(res, "%s", db_strerror(err));
    }			
    res = rb_ary_new();
    n = 0;
    for (i = 0; i < RARRAY(a)->len; i++) {
	if (list[i].op == DB_LOCK_GET) {
	    c = Data_Make_Struct(db_cLock, db_LOCK, 0, lock_free, lockst);
#if DB_VERSION_MAJOR < 3
	    lockst->lock = list[i].lock;
#else
	    lockst->lock = ALLOC(DB_LOCK);
	    MEMCPY(lockst->lock, &list[i].lock, sizeof(DB_LOCK), 1);
#endif
	    lockst->dbenvp = lockid->dbenvp;
	    rb_ary_push(res, c);
	    n++;
	}
    }
    if (!n)
	return Qnil;
    else
	return res;
}

static VALUE
dblock_put(obj)
    VALUE obj;
{
    db_LOCK *lockst;

    GetLock(obj, lockst);
#if DB_VERSION_MAJOR < 3
    test_error(lock_put(lockst->dbenvp->lk_info, lockst->lock));
#else
    test_error(lock_put(lockst->dbenvp, lockst->lock));
#endif
    return Qnil;
} 



static VALUE
dbenv_check(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    unsigned int kbyte, min;
    db_ENV *dbenvst;
    VALUE a, b;

    kbyte = min = 0;
    a = b = Qnil;
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	min = NUM2UINT(b);
    }
    if (!NIL_P(a))
	kbyte = NUM2UINT(a);
    GetEnvDB(obj, dbenvst);
#if DB_VERSION_MAJOR < 3
    if (dbenvst->dbenvp->tx_info == NULL) {
	rb_raise(db_eFatal, "Transaction Manager not enabled");
    }
    test_error(txn_checkpoint(dbenvst->dbenvp->tx_info, kbyte, min));
#else
#if DB_VERSION_MINOR >= 1
    test_error(txn_checkpoint(dbenvst->dbenvp, kbyte, min, 0));
#else
    test_error(txn_checkpoint(dbenvst->dbenvp, kbyte, min));
#endif
#endif
    return Qnil;
}

static VALUE
dbenv_stat(obj)
    VALUE obj;
{
    db_ENV *dbenvst;
    VALUE a;
    DB_TXN_STAT *stat;

    GetEnvDB(obj, dbenvst);
#if DB_VERSION_MAJOR < 3
    if (dbenvst->dbenvp->tx_info == NULL) {
	rb_raise(db_eFatal, "Transaction Manager not enabled");
    }
    test_error(txn_stat(dbenvst->dbenvp->tx_info, &stat, malloc));
#else
    test_error(txn_stat(dbenvst->dbenvp, &stat, malloc));
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

void
Init_bdb()
{
    int major, minor, patch;
    VALUE version;
    version = rb_tainted_str_new2(db_version(&major, &minor, &patch));
    if (major != DB_VERSION_MAJOR || minor != DB_VERSION_MINOR
	|| patch != DB_VERSION_PATCH) {
        rb_raise(rb_eNotImpError, "\nBDB needs compatible versions of libdb & db.h\n\tyou have db.h version %d.%d.%d and libdb version %d.%d.%d\n",
		 DB_VERSION_MAJOR, DB_VERSION_MINOR, DB_VERSION_PATCH,
		 major, minor, patch);
    }
    db_mMarshal = rb_const_get(rb_cObject, rb_intern("Marshal"));
    id_dump = rb_intern("dump");
    id_load = rb_intern("load");
    id_to_s = rb_intern("to_s");
    id_eql = rb_intern("eql?");
    db_mDb = rb_define_module("BDB");
    db_eFatal = rb_define_class("BDBFatal", rb_eStandardError);
    db_eLock = rb_define_class("BDBDeadLock", db_eFatal);
    db_eExist = rb_define_class("BDBKeyExist", db_eFatal);
/* CONSTANT */
    rb_define_const(db_mDb, "VERSION", version);
    rb_define_const(db_mDb, "VERSION_MAJOR", INT2FIX(major));
    rb_define_const(db_mDb, "VERSION_MINOR", INT2FIX(minor));
    rb_define_const(db_mDb, "VERSION_PATCH", INT2FIX(patch));
    rb_define_const(db_mDb, "BTREE", INT2FIX(DB_BTREE));
    rb_define_const(db_mDb, "HASH", INT2FIX(DB_HASH));
    rb_define_const(db_mDb, "RECNO", INT2FIX(DB_RECNO));
#if DB_VERSION_MAJOR < 3
    rb_define_const(db_mDb, "QUEUE", INT2FIX(0));
#else
    rb_define_const(db_mDb, "QUEUE", INT2FIX(DB_QUEUE));
#endif
    rb_define_const(db_mDb, "UNKNOWN", INT2FIX(DB_UNKNOWN));
    rb_define_const(db_mDb, "AFTER", INT2FIX(DB_AFTER));
    rb_define_const(db_mDb, "APPEND", INT2FIX(DB_APPEND));
    rb_define_const(db_mDb, "ARCH_ABS", INT2FIX(DB_ARCH_ABS));
    rb_define_const(db_mDb, "ARCH_DATA", INT2FIX(DB_ARCH_DATA));
    rb_define_const(db_mDb, "ARCH_LOG", INT2FIX(DB_ARCH_LOG));
    rb_define_const(db_mDb, "BEFORE", INT2FIX(DB_BEFORE));
    rb_define_const(db_mDb, "CHECKPOINT", INT2FIX(DB_CHECKPOINT));
#if DB_VERSION_MAJOR < 3
    rb_define_const(db_mDb, "CONSUME", INT2FIX(0));
#else
    rb_define_const(db_mDb, "CONSUME", INT2FIX(DB_CONSUME));
#endif
    rb_define_const(db_mDb, "CREATE", INT2FIX(DB_CREATE));
    rb_define_const(db_mDb, "CURLSN", INT2FIX(DB_CURLSN));
    rb_define_const(db_mDb, "CURRENT", INT2FIX(DB_CURRENT));
    rb_define_const(db_mDb, "DBT_MALLOC", INT2FIX(DB_DBT_MALLOC));
    rb_define_const(db_mDb, "DBT_PARTIAL", INT2FIX(DB_DBT_PARTIAL));
#if DB_VERSION_MAJOR < 3
    rb_define_const(db_mDb, "DBT_REALLOC", INT2FIX(0));
#else
    rb_define_const(db_mDb, "DBT_REALLOC", INT2FIX(DB_DBT_REALLOC));
#endif
    rb_define_const(db_mDb, "DBT_USERMEM", INT2FIX(DB_DBT_USERMEM));
    rb_define_const(db_mDb, "DUP", INT2FIX(DB_DUP));
#if DB_VERSION_MAJOR > 2 || (DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR >= 6)
    rb_define_const(db_mDb, "DUPSORT", INT2FIX(DB_DUPSORT));
#endif
    rb_define_const(db_mDb, "FIRST", INT2FIX(DB_FIRST));
    rb_define_const(db_mDb, "FLUSH", INT2FIX(DB_FLUSH));
#if DB_VERSION_MAJOR < 3
    rb_define_const(db_mDb, "FORCE", INT2FIX(1));
#else
    rb_define_const(db_mDb, "FORCE", INT2FIX(DB_FORCE));
#endif
#if DB_VERSION_MAJOR > 2 || (DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR >= 6)
    rb_define_const(db_mDb, "GET_BOTH", INT2FIX(DB_GET_BOTH));
#endif
    rb_define_const(db_mDb, "GET_RECNO", INT2FIX(DB_GET_RECNO));
#if DB_VERSION_MAJOR > 2 || (DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR >= 6)
    rb_define_const(db_mDb, "INIT_CDB", INT2FIX(DB_INIT_CDB));
#endif
    rb_define_const(db_mDb, "INIT_LOCK", INT2FIX(DB_INIT_LOCK));
    rb_define_const(db_mDb, "INIT_LOG", INT2FIX(DB_INIT_LOG));
    rb_define_const(db_mDb, "INIT_MPOOL", INT2FIX(DB_INIT_MPOOL));
    rb_define_const(db_mDb, "INIT_TXN", INT2FIX(DB_INIT_TXN));
    rb_define_const(db_mDb, "INIT_TRANSACTION", INT2FIX(DB_INIT_LOCK | DB_INIT_MPOOL | DB_INIT_TXN | DB_INIT_LOG));
#if DB_VERSION_MAJOR > 2 || (DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR >= 6)
    rb_define_const(db_mDb, "JOIN_ITEM", INT2FIX(DB_JOIN_ITEM));
#endif
    rb_define_const(db_mDb, "KEYFIRST", INT2FIX(DB_KEYFIRST));
    rb_define_const(db_mDb, "KEYLAST", INT2FIX(DB_KEYLAST));
    rb_define_const(db_mDb, "LAST", INT2FIX(DB_LAST));
    rb_define_const(db_mDb, "LOCK_CONFLICT", INT2FIX(DB_LOCK_CONFLICT));
    rb_define_const(db_mDb, "LOCK_DEADLOCK", INT2FIX(DB_LOCK_DEADLOCK));
    rb_define_const(db_mDb, "LOCK_DEFAULT", INT2FIX(DB_LOCK_DEFAULT));
    rb_define_const(db_mDb, "LOCK_GET", INT2FIX(DB_LOCK_GET));
    rb_define_const(db_mDb, "LOCK_NOTGRANTED", INT2FIX(DB_LOCK_NOTGRANTED));
    rb_define_const(db_mDb, "LOCK_NOWAIT", INT2FIX(DB_LOCK_NOWAIT));
    rb_define_const(db_mDb, "LOCK_OLDEST", INT2FIX(DB_LOCK_OLDEST));
    rb_define_const(db_mDb, "LOCK_PUT", INT2FIX(DB_LOCK_PUT));
    rb_define_const(db_mDb, "LOCK_PUT_ALL", INT2FIX(DB_LOCK_PUT_ALL));
    rb_define_const(db_mDb, "LOCK_PUT_OBJ", INT2FIX(DB_LOCK_PUT_OBJ));
    rb_define_const(db_mDb, "LOCK_RANDOM", INT2FIX(DB_LOCK_RANDOM));
    rb_define_const(db_mDb, "LOCK_YOUNGEST", INT2FIX(DB_LOCK_YOUNGEST));
    rb_define_const(db_mDb, "LOCK_NG", INT2FIX(DB_LOCK_NG));
    rb_define_const(db_mDb, "LOCK_READ", INT2FIX(DB_LOCK_READ));
    rb_define_const(db_mDb, "LOCK_WRITE", INT2FIX(DB_LOCK_WRITE));
    rb_define_const(db_mDb, "LOCK_IWRITE", INT2FIX(DB_LOCK_IWRITE));
    rb_define_const(db_mDb, "LOCK_IREAD", INT2FIX(DB_LOCK_IREAD));
    rb_define_const(db_mDb, "LOCK_IWR", INT2FIX(DB_LOCK_IWR));
    rb_define_const(db_mDb, "MPOOL_CREATE", INT2FIX(DB_MPOOL_CREATE));
    rb_define_const(db_mDb, "MPOOL_LAST", INT2FIX(DB_MPOOL_LAST));
    rb_define_const(db_mDb, "MPOOL_NEW", INT2FIX(DB_MPOOL_NEW));
    rb_define_const(db_mDb, "NEXT", INT2FIX(DB_NEXT));
#if DB_VERSION_MAJOR > 2 || (DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR >= 6)
    rb_define_const(db_mDb, "NEXT_DUP", INT2FIX(DB_NEXT_DUP));
#endif
    rb_define_const(db_mDb, "NOMMAP", INT2FIX(DB_NOMMAP));
    rb_define_const(db_mDb, "NOOVERWRITE", INT2FIX(DB_NOOVERWRITE));
    rb_define_const(db_mDb, "NOSYNC", INT2FIX(DB_NOSYNC));
#if DB_VERSION_MAJOR < 3
    rb_define_const(db_mDb, "POSITION", INT2FIX(0));
#else
    rb_define_const(db_mDb, "POSITION", INT2FIX(DB_POSITION));
#endif
    rb_define_const(db_mDb, "PREV", INT2FIX(DB_PREV));
#if DB_VERSION_MAJOR < 3
    rb_define_const(db_mDb, "PRIVATE", INT2FIX(0));
#else
    rb_define_const(db_mDb, "PRIVATE", INT2FIX(DB_PRIVATE));
#endif
    rb_define_const(db_mDb, "RDONLY", INT2FIX(DB_RDONLY));
    rb_define_const(db_mDb, "RECNUM", INT2FIX(DB_RECNUM));
    rb_define_const(db_mDb, "RECORDCOUNT", INT2FIX(DB_RECORDCOUNT));
    rb_define_const(db_mDb, "RECOVER", INT2FIX(DB_RECOVER));
    rb_define_const(db_mDb, "RECOVER_FATAL", INT2FIX(DB_RECOVER_FATAL));
    rb_define_const(db_mDb, "RENUMBER", INT2FIX(DB_RENUMBER));
    rb_define_const(db_mDb, "RMW", INT2FIX(DB_RMW));
    rb_define_const(db_mDb, "SET", INT2FIX(DB_SET));
    rb_define_const(db_mDb, "SET_RANGE", INT2FIX(DB_SET_RANGE));
    rb_define_const(db_mDb, "SET_RECNO", INT2FIX(DB_SET_RECNO));
    rb_define_const(db_mDb, "SNAPSHOT", INT2FIX(DB_SNAPSHOT));
#if DB_VERSION_MAJOR < 3
    rb_define_const(db_mDb, "SYSTEM_MEM", INT2FIX(0));
#else
    rb_define_const(db_mDb, "SYSTEM_MEM", INT2FIX(DB_SYSTEM_MEM));
#endif
    rb_define_const(db_mDb, "THREAD", INT2FIX(DB_THREAD));
    rb_define_const(db_mDb, "TRUNCATE", INT2FIX(DB_TRUNCATE));
    rb_define_const(db_mDb, "TXN_NOSYNC", INT2FIX(DB_TXN_NOSYNC));
    rb_define_const(db_mDb, "USE_ENVIRON", INT2FIX(DB_USE_ENVIRON));
    rb_define_const(db_mDb, "USE_ENVIRON_ROOT", INT2FIX(DB_USE_ENVIRON_ROOT));
#if DB_VERSION_MAJOR < 3
    rb_define_const(db_mDb, "TXN_NOWAIT", INT2FIX(0));
    rb_define_const(db_mDb, "TXN_SYNC", INT2FIX(0));
    rb_define_const(db_mDb, "WRITECURSOR", INT2FIX(0));
#else
    rb_define_const(db_mDb, "TXN_NOWAIT", INT2FIX(DB_TXN_NOWAIT));
    rb_define_const(db_mDb, "TXN_SYNC", INT2FIX(DB_TXN_SYNC));
    rb_define_const(db_mDb, "WRITECURSOR", INT2FIX(DB_WRITECURSOR));
#endif
    rb_define_const(db_mDb, "TXN_COMMIT", INT2FIX(DBD_TXN_COMMIT));
/* ENV */
    db_cEnv = rb_define_class_under(db_mDb, "Env", rb_cObject);
    rb_define_singleton_method(db_cEnv, "new", dbenv_s_new, -1);
    rb_define_singleton_method(db_cEnv, "create", dbenv_s_new, -1);
    rb_define_singleton_method(db_cEnv, "open", dbenv_s_new, -1);
    rb_define_method(db_cEnv, "open_db", dbenv_open_db, -1);
    rb_define_method(db_cEnv, "remove", dbenv_remove, 0);
    rb_define_method(db_cEnv, "unlink", dbenv_remove, 0);
    rb_define_method(db_cEnv, "close", dbenv_close, 0);
    rb_define_method(db_cEnv, "begin", dbenv_begin, -1);
    rb_define_method(db_cEnv, "txn_begin", dbenv_begin, -1);
    rb_define_method(db_cEnv, "transaction", dbenv_begin, -1);
    rb_define_method(db_cEnv, "lock_id", dbenv_lockid, 0);
    rb_define_method(db_cEnv, "lock", dbenv_lockid, 0);
    rb_define_method(db_cEnv, "lock_stat", dbenv_lockstat, 0);
    rb_define_method(db_cEnv, "lock_detect", dbenv_lockdetect, -1);
    rb_define_method(db_cEnv, "checkpoint", dbenv_check, -1);
    rb_define_method(db_cEnv, "txn_checkpoint", dbenv_check, -1);
    rb_define_method(db_cEnv, "stat", dbenv_stat, 0);
    rb_define_method(db_cEnv, "txn_stat", dbenv_stat, 0);
/* DATABASE */
    db_cCommon = rb_define_class_under(db_mDb, "Common", rb_cObject);
    rb_include_module(db_cCommon, rb_mEnumerable);
    rb_define_singleton_method(db_cCommon, "new", db_s_new, -1);
    rb_define_singleton_method(db_cCommon, "create", db_s_new, -1);
    rb_define_singleton_method(db_cCommon, "open", db_s_new, -1);
    rb_define_singleton_method(db_cCommon, "remove", db_s_remove, -1);
    rb_define_singleton_method(db_cCommon, "db_remove", db_s_remove, -1);
    rb_define_singleton_method(db_cCommon, "unlink", db_s_remove, -1);
    rb_define_singleton_method(db_cCommon, "upgrade", db_s_upgrade, -1);
    rb_define_singleton_method(db_cCommon, "db_upgrade", db_s_upgrade, -1);
    rb_define_method(db_cCommon, "close", db_close, -1);
    rb_define_method(db_cCommon, "db_close", db_close, -1);
    rb_define_method(db_cCommon, "put", db_put, -1);
    rb_define_method(db_cCommon, "db_put", db_put, -1);
    rb_define_method(db_cCommon, "[]=", db_put, -1);
    rb_define_method(db_cCommon, "env", db_env, 0);
    rb_define_method(db_cCommon, "has_env?", db_has_env, 0);
    rb_define_method(db_cCommon, "env?", db_has_env, 0);
    rb_define_method(db_cCommon, "count", db_count, 1);
    rb_define_method(db_cCommon, "get", db_get, -1);
    rb_define_method(db_cCommon, "db_get", db_get, -1);
    rb_define_method(db_cCommon, "[]", db_get, -1);
    rb_define_method(db_cCommon, "delete", db_del, 1);
    rb_define_method(db_cCommon, "del", db_del, 1);
    rb_define_method(db_cCommon, "db_del", db_del, 1);
    rb_define_method(db_cCommon, "sync", db_sync, 0);
    rb_define_method(db_cCommon, "db_sync", db_sync, 0);
    rb_define_method(db_cCommon, "flush", db_sync, 0);
    rb_define_method(db_cCommon, "db_cursor", db_cursor, 0);
    rb_define_method(db_cCommon, "cursor", db_cursor, 0);
    rb_define_method(db_cCommon, "each", db_each_pair, 0);
    rb_define_method(db_cCommon, "each_value", db_each_value, 0);
    rb_define_method(db_cCommon, "reverse_each_value", db_each_eulav, 0);
    rb_define_method(db_cCommon, "each_key", db_each_key, 0);
    rb_define_method(db_cCommon, "reverse_each_key", db_each_yek, 0);
    rb_define_method(db_cCommon, "each_pair", db_each_pair, 0);
    rb_define_method(db_cCommon, "reverse_each", db_each_riap, 0);
    rb_define_method(db_cCommon, "reverse_each_pair", db_each_riap, 0);
    rb_define_method(db_cCommon, "keys", db_keys, 0);
    rb_define_method(db_cCommon, "values", db_values, 0);
    rb_define_method(db_cCommon, "shift", db_shift, 0);
    rb_define_method(db_cCommon, "delete_if", db_delete_if, 0);
    rb_define_method(db_cCommon, "include?", db_has_key, 1);
    rb_define_method(db_cCommon, "has_key?", db_has_key, 1);
    rb_define_method(db_cCommon, "key?", db_has_key, 1);
    rb_define_method(db_cCommon, "has_value?", db_has_value, 1);
    rb_define_method(db_cCommon, "value?", db_has_value, 1);
#if DB_VERSION_MAJOR > 2 || (DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR >= 6)
    rb_define_method(db_cCommon, "has_both?", db_has_both, 2);
    rb_define_method(db_cCommon, "both?", db_has_both, 2);
#endif
    rb_define_method(db_cCommon, "to_a", db_to_a, 0);
    rb_define_method(db_cCommon, "empty?", db_empty, 0);
    rb_define_method(db_cCommon, "length", db_length, 0);
    rb_define_alias(db_cCommon,  "size", "length");
    rb_define_method(db_cCommon, "set_partial", db_set_partial, 2);
    rb_define_method(db_cCommon, "clear_partial", db_clear_partial, 0);
#if DB_VERSION_MAJOR > 2 || (DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR >= 6)
    rb_define_method(db_cCommon, "join", db_join, -1);
    rb_define_method(db_cCommon, "byteswapped?", db_byteswapp, 0);
    rb_define_method(db_cCommon, "get_byteswapped", db_byteswapp, 0);
#endif
    db_cBtree = rb_define_class_under(db_mDb, "Btree", db_cCommon);
    rb_define_method(db_cBtree, "stat", dbtree_stat, 0);
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1
    db_sKeyrange = rb_struct_define("Keyrange", "less", "equal", "greater");
    rb_global_variable(&db_sKeyrange);
    rb_define_method(db_cBtree, "key_range", dbbtree_key_range, 1);
#endif
    db_cHash = rb_define_class_under(db_mDb, "Hash", db_cCommon);
#if DB_VERSION_MAJOR == 3
    rb_define_method(db_cHash, "stat", dbhash_stat, 0);
#endif
    db_cRecno = rb_define_class_under(db_mDb, "Recno", db_cCommon);
    rb_define_method(db_cRecno, "push", db_append, 1);
    rb_define_method(db_cRecno, "stat", dbtree_stat, 0);
    rb_define_method(db_cRecno, "length", dbrecno_length, 0);
    rb_define_alias(db_cRecno,  "size", "length");
#if DB_VERSION_MAJOR == 3
    db_cQueue = rb_define_class_under(db_mDb, "Queue", db_cCommon);
    rb_define_singleton_method(db_cQueue, "new", dbqueue_s_new, -1);
    rb_define_singleton_method(db_cQueue, "create", dbqueue_s_new, -1);
    rb_define_singleton_method(db_cQueue, "open", dbqueue_s_new, -1);
    rb_define_method(db_cQueue, "push", db_append, 1);
    rb_define_method(db_cQueue, "shift", db_consume, 1);
    rb_define_method(db_cQueue, "stat", dbqueue_stat, 0);
#endif
    db_cUnknown = rb_define_class_under(db_mDb, "Unknown", db_cCommon);
/* TRANSACTION */
    db_cTxn = rb_define_class_under(db_mDb, "Txn", rb_cObject);
    db_cTxnCatch = rb_define_class("DBTxnCatch", db_cTxn);
    rb_undef_method(CLASS_OF(db_cTxn), "new");
    rb_define_method(db_cTxn, "begin", dbenv_begin, -1);
    rb_define_method(db_cTxn, "txn_begin", dbenv_begin, -1);
    rb_define_method(db_cTxn, "transaction", dbenv_begin, -1);
    rb_define_method(db_cTxn, "commit", dbtxn_commit, -1);
    rb_define_method(db_cTxn, "txn_commit", dbtxn_commit, -1);
    rb_define_method(db_cTxn, "close", dbtxn_commit, -1);
    rb_define_method(db_cTxn, "txn_close", dbtxn_commit, -1);
    rb_define_method(db_cTxn, "abort", dbtxn_abort, 0);
    rb_define_method(db_cTxn, "txn_abort", dbtxn_abort, 0);
    rb_define_method(db_cTxn, "id", dbtxn_id, 0);
    rb_define_method(db_cTxn, "txn_id", dbtxn_id, 0);
    rb_define_method(db_cTxn, "prepare", dbtxn_prepare, 0);
    rb_define_method(db_cTxn, "txn_prepare", dbtxn_prepare, 0);
    rb_define_method(db_cTxn, "assoc", dbtxn_assoc, -1);
    rb_define_method(db_cTxn, "txn_assoc", dbtxn_assoc, -1);
    rb_define_method(db_cTxn, "associate", dbtxn_assoc, -1);
/* CURSOR */
    db_cCursor = rb_define_class_under(db_mDb, "Cursor", rb_cObject);
    rb_undef_method(CLASS_OF(db_cCursor), "new");
    rb_define_method(db_cCursor, "close", dbcursor_close, 0);
    rb_define_method(db_cCursor, "c_close", dbcursor_close, 0);
    rb_define_method(db_cCursor, "c_del", dbcursor_del, 0);
    rb_define_method(db_cCursor, "del", dbcursor_del, 0);
    rb_define_method(db_cCursor, "delete", dbcursor_del, 0);
#if DB_VERSION_MAJOR == 3
    rb_define_method(db_cCursor, "dup", dbcursor_dup, -1);
    rb_define_method(db_cCursor, "c_dup", dbcursor_dup, -1);
    rb_define_method(db_cCursor, "clone", dbcursor_dup, -1);
    rb_define_method(db_cCursor, "c_clone", dbcursor_dup, -1);
#endif
    rb_define_method(db_cCursor, "count", dbcursor_count, 0);
    rb_define_method(db_cCursor, "c_count", dbcursor_count, 0);
    rb_define_method(db_cCursor, "get", dbcursor_get, -1);
    rb_define_method(db_cCursor, "c_get", dbcursor_get, -1);
    rb_define_method(db_cCursor, "put", dbcursor_put, -1);
    rb_define_method(db_cCursor, "c_put", dbcursor_put, -1);
    rb_define_method(db_cCursor, "c_next", dbcursor_next, 0);
    rb_define_method(db_cCursor, "next", dbcursor_next, 0);
    rb_define_method(db_cCursor, "c_first", dbcursor_first, 0);
    rb_define_method(db_cCursor, "first", dbcursor_first, 0);
    rb_define_method(db_cCursor, "c_last", dbcursor_last, 0);
    rb_define_method(db_cCursor, "last", dbcursor_last, 0);
    rb_define_method(db_cCursor, "c_current", dbcursor_current, 0);
    rb_define_method(db_cCursor, "current", dbcursor_current, 0);
    rb_define_method(db_cCursor, "c_prev", dbcursor_prev, 0);
    rb_define_method(db_cCursor, "prev", dbcursor_prev, 0);
    rb_define_method(db_cCursor, "c_set", dbcursor_set, 1);
    rb_define_method(db_cCursor, "set", dbcursor_set, 1);
    rb_define_method(db_cCursor, "c_set_range", dbcursor_set_range, 1);
    rb_define_method(db_cCursor, "set_range", dbcursor_set_range, 1);
    rb_define_method(db_cCursor, "c_set_recno", dbcursor_set_recno, 1);
    rb_define_method(db_cCursor, "set_recno", dbcursor_set_recno, 1);
/* LOCK */
    db_cLockid = rb_define_class_under(db_mDb, "Lockid", rb_cObject);
    rb_undef_method(CLASS_OF(db_cLockid), "new");
    rb_define_method(db_cLockid, "lock_get", dblockid_get, -1);
    rb_define_method(db_cLockid, "get", dblockid_get, -1);
    rb_define_method(db_cLockid, "lock_vec", dblockid_vec, -1);
    rb_define_method(db_cLockid, "vec", dblockid_vec, -1);
    db_cLock = rb_define_class_under(db_cLockid, "Lock", rb_cObject);
    rb_undef_method(CLASS_OF(db_cLock), "new");
    rb_define_method(db_cLock, "put", dblock_put, 0);
    rb_define_method(db_cLock, "lock_put", dblock_put, 0);
    rb_define_method(db_cLock, "release", dblock_put, 0);
    rb_define_method(db_cLock, "delete", dblock_put, 0);
    dberrstr = rb_tainted_str_new(0, 0);
    rb_global_variable(&dberrstr);
}
