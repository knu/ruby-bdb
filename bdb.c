#include <ruby.h>
#include <db.h>
#include <errno.h>

#define BDB_MARSHAL 1
#define BDB_TXN 2
#define BDB_RE_SOURCE 4
#define BDB_BT_COMPARE 8
#define BDB_BT_PREFIX  16
#define BDB_DUP_COMPARE 32
#define BDB_H_HASH 64

#define BDB_NEED_CURRENT (BDB_BT_COMPARE | BDB_BT_PREFIX | BDB_DUP_COMPARE | BDB_H_HASH)


#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
#define DB_RMW 0
#endif

static VALUE bdb_cEnv;
static VALUE bdb_eFatal, bdb_eLock;
static VALUE bdb_mDb;
static VALUE bdb_cCommon, bdb_cBtree, bdb_cHash, bdb_cRecno, bdb_cUnknown;
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1
static VALUE bdb_sKeyrange;
#endif

#if DB_VERSION_MAJOR == 3
static VALUE bdb_cQueue;
#endif

static VALUE bdb_cTxn, bdb_cTxnCatch;
static VALUE bdb_cCursor;
static VALUE bdb_cLock, bdb_cLockid;

static VALUE bdb_mMarshal;
static ID id_dump, id_load, id_current_db;
static ID id_bt_compare, id_bt_prefix, id_dup_compare, id_h_hash, id_proc_call;

static VALUE bdb_errstr;
static int bdb_errcall = 0;

typedef struct  {
    int marshal, no_thread;
    int flags27;
    VALUE db_ary;
    DB_ENV *dbenvp;
} bdb_ENV;

typedef struct {
    int status;
    int marshal;
    int flags27;
    VALUE db_ary;
    VALUE env;
    DB_TXN *txnid;
    DB_TXN *parent;
} bdb_TXN;

typedef struct {
    int options, no_thread;
    int flags27;
    DBTYPE type;
    VALUE env;
    VALUE bt_compare, bt_prefix, dup_compare, h_hash;
    DB *dbp;
    bdb_TXN *txn;
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
#endif
} bdb_DB;

typedef struct {
    DBC *dbc;
    bdb_DB *dbst;
} bdb_DBC;

typedef struct {
    unsigned int lock;
    DB_ENV *dbenvp;
} bdb_LOCKID;

typedef struct {
#if DB_VERSION_MAJOR < 3
    DB_LOCK lock;
#else
    DB_LOCK *lock;
#endif
    DB_ENV *dbenvp;
} bdb_LOCK;

typedef struct {
    bdb_DB *dbst;
    DBT *key;
    VALUE value;
} bdb_VALUE;

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
    case DB_KEYEXIST:
        break;
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 2
    case DB_INCOMPLETE:
	comm = 0;
        break;
#endif
    default:
        error = (comm == DB_LOCK_DEADLOCK || comm == EAGAIN)?bdb_eLock:bdb_eFatal;
        if (bdb_errcall) {
            bdb_errcall = 0;
            rb_raise(error, "%s -- %s", RSTRING(bdb_errstr)->ptr, db_strerror(comm));
        }
        else
            rb_raise(error, "%s", db_strerror(comm));
    }
    return comm;
}

static VALUE
bdb_obj_init(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    return Qtrue;
}

struct db_stoptions {
    bdb_ENV *env;
    VALUE config;
};

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
        test_error(dbenvp->set_cachesize(dbenvp, 
                                      NUM2INT(RARRAY(value)->ptr[0]),
                                      NUM2INT(RARRAY(value)->ptr[1]),
                                      NUM2INT(RARRAY(value)->ptr[2])));
#endif
    }
#if DB_VERSION_MAJOR == 3
    else if (strcmp(options, "set_region_init") == 0) {
#if (DB_VERSION_MINOR >=1 && DB_VERSION_PATCH >= 14) || DB_VERSION_MINOR >= 2
        test_error(db_env_set_region_init(NUM2INT(value)));
#else
        test_error(dbenvp->set_region_init(dbenvp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_tas_spins") == 0) {
#if  (DB_VERSION_MINOR >=1 && DB_VERSION_PATCH >= 14) || DB_VERSION_MINOR >= 2
        test_error(db_env_set_tas_spins(NUM2INT(value)));
#else
        test_error(dbenvp->set_tas_spins(dbenvp, NUM2INT(value)));
#endif
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
		rb_raise(bdb_eFatal, "Empty array for \"set_server\"");
		break;
	    }
	    break;
	default:
	    rb_raise(bdb_eFatal, "Invalid type for \"set_server\"");
	    break;
	}
	test_error(dbenvp->set_server(dbenvp, host, cl_timeout, sv_timeout, flags));
    }
#endif

#endif
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 2
    else if (strcmp(options, "set_flags") == 0) {
        test_error(dbenvp->set_flags(dbenvp, NUM2UINT(value), 1));
    }
#endif
    else if (strcmp(options, "marshal") == 0) {
        switch (value) {
        case Qtrue: dbenvst->marshal = 1; break;
        case Qfalse: dbenvst->marshal = 0; break;
        default: rb_raise(bdb_eFatal, "marshal value must be true or false");
        }
    }
    else if (strcmp(options, "thread") == 0) {
        switch (value) {
        case Qtrue: dbenvst->no_thread = 0; break;
        case Qfalse: dbenvst->no_thread = 1; break;
        default: rb_raise(bdb_eFatal, "thread value must be true or false");
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

static VALUE bdb_close(int, VALUE *, VALUE);

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
	env_close(dbenvst->dbenvp);
    }
    free(dbenvst);
}

static void
bdb_env_mark(dbenvst)
    bdb_ENV *dbenvst;
{
    rb_gc_mark(dbenvst->db_ary);
}

#define GetEnvDB(obj, dbenvst)				\
{							\
    Data_Get_Struct(obj, bdb_ENV, dbenvst);		\
    if (dbenvst->dbenvp == 0)				\
        rb_raise(bdb_eFatal, "closed environment");	\
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
    env_close(dbenvst->dbenvp);
    return Qtrue;
}

static void
bdb_env_errcall(errpfx, msg)
    char *errpfx, *msg;
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
    rb_warn("malloc applele");
    return malloc(size);
}

static VALUE
bdb_set_func(dbenvst)
    bdb_ENV *dbenvst;
{
#if DB_VERSION_MAJOR == 3
#if (DB_VERSION_MINOR >=1 && DB_VERSION_PATCH >= 14) || DB_VERSION_MINOR >= 2
    test_error(db_env_set_func_sleep(bdb_func_sleep));
    test_error(db_env_set_func_yield(bdb_func_yield));
#else
    test_error(dbenvst->dbenvp->set_func_sleep(dbenvst->dbenvp, bdb_func_sleep));
    test_error(dbenvst->dbenvp->set_func_yield(dbenvst->dbenvp, bdb_func_yield));
#endif
#else
    test_error(db_jump_set((void *)bdb_func_sleep, DB_FUNC_SLEEP));
    test_error(db_jump_set((void *)bdb_func_yield, DB_FUNC_YIELD));
#endif
    return Qtrue;
}

static VALUE
bdb_env_each_options(args)
    VALUE *args;
{
    return rb_iterate(rb_each, args[0], bdb_env_i_options, args[1]);
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
    test_error(db_env_create(&dbenvp, 0));
    dbenvp->set_errpfx(dbenvp, "BDB::");
    dbenvp->set_errcall(dbenvp, bdb_env_errcall);
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
    }
    flags |= DB_THREAD;
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
    retour = Data_Wrap_Struct(obj, bdb_env_mark, bdb_env_free, dbenvst);
    rb_obj_call_init(retour, argc, argv);
    return retour;
}

static VALUE
bdb_env_remove(obj)
    VALUE obj;
{
    bdb_ENV *dbenvst;

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

#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 2
static VALUE
bdb_env_set_flags(argc, argv, obj)
    int argc;
    VALUE *argv, obj;
{
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
    test_error(dbenvst->dbenvp->set_flags(dbenvst->dbenvp, NUM2INT(flag), state));
    return Qnil;
}
#endif
	
/* DATABASE */
#define test_dump(dbst, key, a)					\
{								\
    int _bdb_is_nil = 0;					\
    VALUE _bdb_tmp_;						\
    if (dbst->options & BDB_MARSHAL)				\
        _bdb_tmp_ = rb_funcall(bdb_mMarshal, id_dump, 1, a);	\
    else {							\
        _bdb_tmp_ = rb_obj_as_string(a);			\
        if (a == Qnil)						\
            _bdb_is_nil = 1;					\
        else							\
            a = _bdb_tmp_;					\
    }								\
    key.data = RSTRING(_bdb_tmp_)->ptr;				\
    key.size = RSTRING(_bdb_tmp_)->len + _bdb_is_nil;		\
}

static VALUE
test_load(dbst, a)
    bdb_DB *dbst;
    DBT a;
{
    VALUE res;
    int i;

    if (dbst->options & BDB_MARSHAL)
        res = rb_funcall(bdb_mMarshal, id_load, 1, rb_str_new(a.data, a.size));
    else {
#if DB_VERSION_MAJOR == 3
	if (dbst->type == DB_QUEUE) {
	    for (i = a.size - 1; i >= 0; i--) {
		if (((char *)a.data)[i] != dbst->re_pad)
		    break;
	    }
	    a.size = i + 1;
	}
#endif
	if (a.size == 1 && ((char *)a.data)[0] == '\000') {
	    res = Qnil;
	}
	else {
	    res = rb_tainted_str_new(a.data, a.size);
	}
    }
    if (a.flags & DB_DBT_MALLOC) {
	free(a.data);
    }
    return res;
}

static int
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 2
bdb_bt_compare(dbbd, a, b)
    DB *dbbd;
    DBT *a, *b;
#else
bdb_bt_compare(a, b)
    DBT *a, *b;
#endif
{
    VALUE obj, av, bv, res;
    bdb_DB *dbst;

    if ((obj = rb_thread_local_aref(rb_thread_current(), id_current_db)) == Qnil) {
	rb_raise(bdb_eFatal, "BUG : current_db not set");
    }
    Data_Get_Struct(obj, bdb_DB, dbst);
    a->flags = b->flags = 0;
    av = test_load(dbst, *a);
    bv = test_load(dbst, *b);
    if (dbst->bt_compare == 0)
	res = rb_funcall(obj, id_bt_compare, 2, av, bv);
    else
	res = rb_funcall(dbst->bt_compare, id_proc_call, 2, av, bv);
    return NUM2INT(res);
} 

static size_t
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 2
bdb_bt_prefix(dbbd, a, b)
    DB *dbbd;
    DBT *a, *b;
#else
bdb_bt_prefix(a, b)
    DBT *a, *b;
#endif
{
    VALUE obj, av, bv, res;
    bdb_DB *dbst;

    if ((obj = rb_thread_local_aref(rb_thread_current(), id_current_db)) == Qnil) {
	rb_raise(bdb_eFatal, "BUG : current_db not set");
    }
    Data_Get_Struct(obj, bdb_DB, dbst);
    a->flags = b->flags = 0;
    av = test_load(dbst, *a);
    bv = test_load(dbst, *b);
    if (dbst->bt_prefix == 0)
	res = rb_funcall(obj, id_bt_prefix, 2, av, bv);
    else
	res = rb_funcall(dbst->bt_prefix, id_proc_call, 2, av, bv);
    return NUM2INT(res);
} 

static int
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 2
bdb_dup_compare(dbbd, a, b)
    DB * dbbd;
    DBT *a, *b;
#else
bdb_dup_compare(a, b)
    DBT *a, *b;
#endif
{
    VALUE obj, av, bv, res;
    bdb_DB *dbst;

    if ((obj = rb_thread_local_aref(rb_thread_current(), id_current_db)) == Qnil) {
	rb_raise(bdb_eFatal, "BUG : current_db not set");
    }
    Data_Get_Struct(obj, bdb_DB, dbst);
    a->flags = b->flags = 0;
    av = test_load(dbst, *a);
    bv = test_load(dbst, *b);
    if (dbst->dup_compare == 0)
	res = rb_funcall(obj, id_dup_compare, 2, av, bv);
    else
	res = rb_funcall(dbst->dup_compare, id_proc_call, 2, av, bv);
    return NUM2INT(res);
}

static u_int32_t
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 2
bdb_h_hash(dbbd, bytes, length)
    DB *dbbd;
    void *bytes;
    u_int32_t length;
#else
bdb_h_hash(bytes, length)
    void *bytes;
    u_int32_t length;
#endif
{
    VALUE obj, st, res;
    bdb_DB *dbst;

    if ((obj = rb_thread_local_aref(rb_thread_current(), id_current_db)) == Qnil) {
	rb_raise(bdb_eFatal, "BUG : current_db not set");
    }
    Data_Get_Struct(obj, bdb_DB, dbst);
    st = rb_tainted_str_new((char *)bytes, length);
    if (dbst->h_hash == 0)
	res = rb_funcall(obj, id_h_hash, 1, st);
    else
	res = rb_funcall(dbst->h_hash, id_proc_call, 1, st);
    return NUM2UINT(res);
}

static VALUE
bdb_i_options(obj, dbstobj)
    VALUE obj, dbstobj;
{
    char *options;
    DB *dbp;
    VALUE key, value;
    bdb_DB *dbst;

    Data_Get_Struct(dbstobj, bdb_DB, dbst);
    key = rb_ary_entry(obj, 0);
    value = rb_ary_entry(obj, 1);
    dbp = dbst->dbp;
    key = rb_obj_as_string(key);
    options = RSTRING(key)->ptr;
    if (strcmp(options, "set_bt_minkey") == 0) {
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->bt_minkey = NUM2INT(value);
#else
        test_error(dbp->set_bt_minkey(dbp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_bt_compare") == 0) {
	if (!rb_obj_is_kind_of(value, rb_cProc)) 
	    rb_raise(bdb_eFatal, "expected a Proc object");
	dbst->options |= BDB_BT_COMPARE;
	dbst->bt_compare = value;
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->bt_compare = bdb_bt_compare;
#else
	test_error(dbp->set_bt_compare(dbp, bdb_bt_compare));
#endif
    }
    else if (strcmp(options, "set_bt_prefix") == 0) {
	if (!rb_obj_is_kind_of(value, rb_cProc)) 
	    rb_raise(bdb_eFatal, "expected a Proc object");
	dbst->options |= BDB_BT_PREFIX;
	dbst->bt_prefix = value;
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->bt_prefix = bdb_bt_prefix;
#else
	test_error(dbp->set_bt_prefix(dbp, bdb_bt_prefix));
#endif
    }
    else if (strcmp(options, "set_dup_compare") == 0) {
#ifdef DB_DUPSORT
	if (!rb_obj_is_kind_of(value, rb_cProc)) 
	    rb_raise(bdb_eFatal, "expected a Proc object");
	dbst->options |= BDB_DUP_COMPARE;
	dbst->dup_compare = value;
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->dup_compare = bdb_dup_compare;
#else
	test_error(dbp->set_dup_compare(dbp, bdb_dup_compare));
#endif
#else
	rb_warning("dup_compare need db >= 2.5.9");
#endif
    }
    else if (strcmp(options, "set_h_hash") == 0) {
	if (!rb_obj_is_kind_of(value, rb_cProc)) 
	    rb_raise(bdb_eFatal, "expected a Proc object");
	dbst->options |= BDB_H_HASH;
	dbst->h_hash = value;
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->h_hash = bdb_h_hash;
#else
	test_error(dbp->set_h_hash(dbp, bdb_h_hash));
#endif
    }
    else if (strcmp(options, "set_cachesize") == 0) {
        Check_Type(value, T_ARRAY);
        if (RARRAY(value)->len < 3)
            rb_raise(bdb_eFatal, "expected 3 values for cachesize");
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
	int ch;
	if (TYPE(value) == T_STRING) {
	    ch = RSTRING(value)->ptr[0];
	}
	else {
	    ch = NUM2INT(value);
	}
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->re_delim = ch;
	dbst->dbinfo->flags |= DB_DELIMITER;
#else
        test_error(dbp->set_re_delim(dbp, ch));
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
	int ch;
	if (TYPE(value) == T_STRING) {
	    ch = RSTRING(value)->ptr[0];
	}
	else {
	    ch = NUM2INT(value);
	}
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->re_pad = ch;
	dbst->dbinfo->flags |= DB_PAD;
#else
        test_error(dbp->set_re_pad(dbp, ch));
#endif
    }
    else if (strcmp(options, "set_re_source") == 0) {
        if (TYPE(value) != T_STRING)
            rb_raise(bdb_eFatal, "re_source must be a filename");
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->re_source = RSTRING(value)->ptr;
#else
        test_error(dbp->set_re_source(dbp, RSTRING(value)->ptr));
#endif
	dbst->options |= BDB_RE_SOURCE;
    }
    else if (strcmp(options, "marshal") == 0) {
        switch (value) {
        case Qtrue: dbst->options |= BDB_MARSHAL; break;
        case Qfalse: dbst->options &= ~BDB_MARSHAL; break;
        default: rb_raise(bdb_eFatal, "marshal value must be true or false");
        }
    }
    else if (strcmp(options, "set_array_base") == 0 ||
	     strcmp(options, "array_base") == 0) {
	int ary_base = NUM2INT(value);
	switch (ary_base) {
	case 0: dbst->array_base = 1; break;
	case 1: dbst->array_base = 0; break;
	default: rb_raise(bdb_eFatal, "array base must be 0 or 1");
	}
    }
    else if (strcmp(options, "thread") == 0) {
        switch (value) {
        case Qtrue: dbst->no_thread = 0; break;
        case Qfalse: dbst->no_thread = 1; break;
        default: rb_raise(bdb_eFatal, "thread value must be true or false");
        }
    }
    return Qnil;
}

static void
bdb_free(dbst)
    bdb_DB *dbst;
{
    if (dbst->dbp && dbst->flags27 != -1 && !(dbst->options & BDB_TXN)) {
        test_error(dbst->dbp->close(dbst->dbp, 0));
        dbst->dbp = NULL;
    }
    free(dbst);
}


#define GetDB(obj, dbst)						\
{									\
    Data_Get_Struct(obj, bdb_DB, dbst);					\
    if (dbst->dbp == 0) {						\
        rb_raise(bdb_eFatal, "closed DB");				\
    }									\
    if (dbst->options & BDB_NEED_CURRENT) {				\
    	rb_thread_local_aset(rb_thread_current(), id_current_db, obj);	\
    }									\
}

static VALUE
bdb_env(obj)
    VALUE obj;
{
    bdb_DB *dbst;

    GetDB(obj, dbst);
    return dbst->env;
}


static VALUE
bdb_has_env(obj)
    VALUE obj;
{
    bdb_DB *dbst;

    GetDB(obj, dbst);
    return (dbst->env == 0)?Qfalse:Qtrue;
}

static VALUE
bdb_close(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    VALUE opt;
    bdb_DB *dbst;
    int flags = 0;

    if (!OBJ_TAINTED(obj) && rb_safe_level() >= 4)
	rb_raise(rb_eSecurityError, "Insecure: can't close the database");
    Data_Get_Struct(obj, bdb_DB, dbst);
    if (dbst->dbp != NULL) {
	if (rb_scan_args(argc, argv, "01", &opt))
	    flags = NUM2INT(opt);
	test_error(dbst->dbp->close(dbst->dbp, flags));
	dbst->dbp = NULL;
    }
    return Qnil;
}

static VALUE
bdb_open_common(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    bdb_DB *dbst;
    DB *dbp;
    bdb_ENV *dbenvst;
    DB_ENV *dbenvp;
    int flags, mode, ret, nb;
    char *name, *subname;
    VALUE a, b, c, d, e, f;
    VALUE res;
    int flags27;
#if DB_VERSION_MAJOR < 3
    DB_INFO dbinfo;

    MEMZERO(&dbinfo, DB_INFO, 1);
#endif
    mode = flags = flags27 = 0;
    a = b = d = e = f = Qnil;
    if (argc && TYPE(argv[argc - 1]) == T_HASH) {
        f = argv[argc - 1];
        argc--;
    }
    switch(nb = rb_scan_args(argc, argv, "14", &c, &a, &b, &d, &e)) {
    case 5:
	mode = NUM2INT(e);
    case 4:
	if (TYPE(d) == T_STRING) {
	    if (strcmp(RSTRING(d)->ptr, "r") == 0)
		flags = DB_RDONLY;
	    else if (strcmp(RSTRING(d)->ptr, "r+") == 0)
		flags = 0;
	    else if (strcmp(RSTRING(d)->ptr, "w") == 0 ||
		     strcmp(RSTRING(d)->ptr, "w+") == 0)
		flags = DB_CREATE | DB_TRUNCATE;
	    else if (strcmp(RSTRING(d)->ptr, "a") == 0 ||
		     strcmp(RSTRING(d)->ptr, "a+") == 0)
		flags = DB_CREATE;
	    else {
		rb_raise(bdb_eFatal, "flags must be r, r+, w, w+ a or a+");
	    }
	}
	else if (d == Qnil)
	    flags = 0;
	else
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
    dbenvp = 0;
    dbenvst = 0;
    res = Data_Make_Struct(obj, bdb_DB, 0, bdb_free, dbst);
    if (!NIL_P(f)) {
	VALUE v;

        if (TYPE(f) != T_HASH) {
            rb_raise(bdb_eFatal, "options must be an hash");
	}
	if ((v = rb_hash_aref(f, rb_str_new2("txn"))) != RHASH(f)->ifnone) {
	    bdb_TXN *txnst;

	    if (!rb_obj_is_kind_of(v, bdb_cTxn)) {
		rb_raise(bdb_eFatal, "argument of txn must be a transaction");
	    }
	    Data_Get_Struct(v, bdb_TXN, txnst);
	    dbst->txn = txnst;
	    dbst->env = txnst->env;
	    Data_Get_Struct(txnst->env, bdb_ENV, dbenvst);
	    dbenvp = dbenvst->dbenvp;
	    flags27 = dbenvst->flags27;
	    if (txnst->marshal)
		dbst->options |= BDB_MARSHAL;
	}
	else if ((v = rb_hash_aref(f, rb_str_new2("env"))) != RHASH(f)->ifnone) {
	    if (!rb_obj_is_kind_of(v, bdb_cEnv)) {
		rb_raise(bdb_eFatal, "argument of env must be an environnement");
	    }
	    Data_Get_Struct(v, bdb_ENV, dbenvst);
	    dbst->env = v;
	    dbenvp = dbenvst->dbenvp;
	    flags27 = dbenvst->flags27;
	    if (dbenvst->marshal)
		dbst->options |= BDB_MARSHAL;
	}
    }
#if DB_VERSION_MAJOR == 3
    test_error(db_create(&dbp, dbenvp, 0));
    dbp->set_errpfx(dbp, "BDB::");
    dbp->set_errcall(dbp, bdb_env_errcall);
    dbst->dbp = dbp;
    dbst->flags27 = -1;
#else
    dbst->dbinfo = &dbinfo;
#endif
    if (!NIL_P(f)) {
	if (TYPE(f) != T_HASH) {
	    rb_raise(bdb_eFatal, "options must be an hash");
	}
	rb_iterate(rb_each, f, bdb_i_options, res);
    }
    if (dbst->bt_compare == 0 && rb_method_boundp(obj, id_bt_compare, 0) == Qtrue) {
	    dbst->options |= BDB_BT_COMPARE;
#if DB_VERSION_MAJOR < 3
	    dbst->dbinfo->bt_compare = bdb_bt_compare;
#else
	    test_error(dbp->set_bt_compare(dbp, bdb_bt_compare));
#endif
    }
    if (dbst->bt_prefix == 0 && rb_method_boundp(obj, id_bt_prefix, 0) == Qtrue) {
	    dbst->options |= BDB_BT_PREFIX;
#if DB_VERSION_MAJOR < 3
	    dbst->dbinfo->bt_prefix = bdb_bt_prefix;
#else
	    test_error(dbp->set_bt_prefix(dbp, bdb_bt_prefix));
#endif
    }
#ifdef DB_DUPSORT
    if (dbst->dup_compare == 0 && rb_method_boundp(obj, id_dup_compare, 0) == Qtrue) {
	    dbst->options |= BDB_DUP_COMPARE;
#if DB_VERSION_MAJOR < 3
	    dbst->dbinfo->dup_compare = bdb_dup_compare;
#else
	    test_error(dbp->set_dup_compare(dbp, bdb_dup_compare));
#endif
    }
#endif
    if (dbst->h_hash == 0 && rb_method_boundp(obj, id_h_hash, 0) == Qtrue) {
	    dbst->options |= BDB_H_HASH;
#if DB_VERSION_MAJOR < 3
	    dbst->dbinfo->h_hash = bdb_h_hash;
#else
	    test_error(dbp->set_h_hash(dbp, bdb_h_hash));
#endif
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
#ifdef DB_DUPSORT
    if (dbst->options & BDB_DUP_COMPARE) {
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->flags = DB_DUP | DB_DUPSORT;
#else
        test_error(dbp->set_flags(dbp, DB_DUP | DB_DUPSORT));
#endif
    }
#endif
#ifndef BDB_NO_THREAD
    if (!(dbst->options & BDB_RE_SOURCE))
	flags |= DB_THREAD;
#endif
    if (dbst->options & BDB_NEED_CURRENT) {
	rb_thread_local_aset(rb_thread_current(), id_current_db, res);
    }

#if DB_VERSION_MAJOR < 3
    if ((ret = db_open(name, NUM2INT(c), flags, mode, dbenvp,
			 dbst->dbinfo, &dbp)) != 0) {
#else
    if ((ret = dbp->open(dbp, name, subname, NUM2INT(c), flags, mode)) != 0) {
        dbp->close(dbp, 0);
#endif
        if (bdb_errcall) {
            bdb_errcall = 0;
            rb_raise(bdb_eFatal, "%s -- %s", RSTRING(bdb_errstr)->ptr, db_strerror(ret));
        }
        else
            rb_raise(bdb_eFatal, "%s", db_strerror(ret));
    }
    if (NUM2INT(c) == DB_UNKNOWN) {
	VALUE res1;
	bdb_DB *dbst1;
	int type;

#if DB_VERSION_MAJOR < 3
	type = dbp->type;
#else
	type = dbp->get_type(dbp);
#endif
	switch(type) {
	case DB_BTREE:
	    res1 = Data_Make_Struct(bdb_cBtree, bdb_DB, 0, bdb_free, dbst1);
	    break;
	case DB_HASH:
	    res1 = Data_Make_Struct(bdb_cHash, bdb_DB, 0, bdb_free, dbst1);
	    break;
	case DB_RECNO:
	    res1 = Data_Make_Struct(bdb_cRecno, bdb_DB, 0, bdb_free, dbst1);
	    break;
#if  DB_VERSION_MAJOR == 3
	case DB_QUEUE:
	    res1 = Data_Make_Struct(bdb_cQueue, bdb_DB, 0, bdb_free, dbst1);
	    break;
#endif
	default:
	    dbp->close(dbp, 0);
	    rb_raise(bdb_eFatal, "Unknown DB type");
	}
	MEMCPY(dbst1, dbst, bdb_DB, 1);
	dbst1->dbp = dbp;
	dbst1->flags27 = flags27;
	return res1;
    }
    else {
	dbst->dbp = dbp;
	dbst->flags27 = flags27;
	return res;
    }
}

static VALUE bdb_recno_length(VALUE);

static VALUE
bdb_s_new(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    VALUE *nargv;
    VALUE res, cl, bb;
    bdb_DB *ret;

    cl = obj;
    while (cl) {
	if (cl == bdb_cBtree || RCLASS(cl)->m_tbl == RCLASS(bdb_cBtree)->m_tbl) {
	    bb = INT2NUM(DB_BTREE); 
	    break;
	}
	else if (cl == bdb_cHash || RCLASS(cl)->m_tbl == RCLASS(bdb_cHash)->m_tbl) {
	    bb = INT2NUM(DB_HASH); 
	    break;
	}
	else if (cl == bdb_cRecno || RCLASS(cl)->m_tbl == RCLASS(bdb_cRecno)->m_tbl) {
	    bb = INT2NUM(DB_RECNO);
	    break;
    }
#if DB_VERSION_MAJOR == 3
	else if (cl == bdb_cQueue || RCLASS(cl)->m_tbl == RCLASS(bdb_cQueue)->m_tbl) {
	    bb = INT2NUM(DB_QUEUE);
	    break;
	}
#endif
	else if (cl == bdb_cUnknown || RCLASS(cl)->m_tbl == RCLASS(bdb_cUnknown)->m_tbl) {
	    bb = INT2NUM(DB_UNKNOWN);
	    break;
	}
	cl = RCLASS(cl)->super;
    }
    if (!cl) {
	rb_raise(bdb_eFatal, "unknown database type");
    }
    nargv = ALLOCA_N(VALUE, argc + 1);
    nargv[0] = bb;
    MEMCPY(nargv + 1, argv, VALUE, argc);
    res = bdb_open_common(argc + 1, nargv, obj);
    Data_Get_Struct(res, bdb_DB, ret);
#if DB_VERSION_MAJOR < 3
    ret->type = ret->dbp->type;
#else
    ret->type = ret->dbp->get_type(ret->dbp);
#endif
    rb_obj_call_init(res, argc, argv);
    if (ret->txn != NULL) {
	rb_ary_push(ret->txn->db_ary, res);
    }
    else if (ret->env != 0) {
	bdb_ENV *dbenvp;
	Data_Get_Struct(ret->env, bdb_ENV, dbenvp);
	rb_ary_push(dbenvp->db_ary, res);
    }
    return res;
}

#if DB_VERSION_MAJOR == 3

struct re {
    int re_len, re_pad;
};

static VALUE
bdb_queue_i_search_re_len(obj, restobj)
    VALUE obj;
{
    VALUE key, value;
    struct re *rest;

    Data_Get_Struct(restobj, struct re, rest);
    key = rb_ary_entry(obj, 0);
    value = rb_ary_entry(obj, 1);
    key = rb_obj_as_string(key);
    if (strcmp(RSTRING(key)->ptr, "set_re_len") == 0) {
	rest->re_len = NUM2INT(value);
    }
    else if (strcmp(RSTRING(key)->ptr, "set_re_pad") == 0) {
	int ch;
	if (TYPE(value) == T_STRING) {
	    ch = RSTRING(value)->ptr[0];
	}
	else {
	    ch = NUM2INT(value);
	}
	rest->re_pad = ch;
    }
    return Qnil;
}

#define DEFAULT_RECORD_LENGTH 132
#define DEFAULT_RECORD_PAD 0x20

static VALUE
bdb_queue_s_new(argc, argv, obj) 
    int argc;
    VALUE *argv;
    VALUE obj;
{
    VALUE *nargv, ret, restobj;
    struct re *rest;
    bdb_DB *dbst;

    restobj = Data_Make_Struct(obj, struct re, 0, free, rest);
    rest->re_len = -1;
    rest->re_pad = -1;
    if (argc && TYPE(argv[argc - 1]) == T_HASH) {
	rb_iterate(rb_each, argv[argc - 1], bdb_queue_i_search_re_len, restobj);
	if (rest->re_len <= 0) {
	    rest->re_len = DEFAULT_RECORD_LENGTH;
	    rb_hash_aset(argv[argc - 1], rb_tainted_str_new2("set_re_len"), INT2NUM(rest->re_len));
	}
	if (rest->re_pad < 0) {
	    rest->re_pad = DEFAULT_RECORD_PAD;
	    rb_hash_aset(argv[argc - 1], rb_tainted_str_new2("set_re_pad"), INT2NUM(rest->re_pad));
	}
	nargv = argv;
    }
    else {
	nargv = ALLOCA_N(VALUE, argc + 1);
	MEMCPY(nargv, argv, VALUE, argc);
	nargv[argc] = rb_hash_new();
	rest->re_len = DEFAULT_RECORD_LENGTH;
	rest->re_pad = DEFAULT_RECORD_PAD;
	rb_hash_aset(nargv[argc], rb_tainted_str_new2("set_re_len"), INT2NUM(DEFAULT_RECORD_LENGTH));
	rb_hash_aset(nargv[argc], rb_tainted_str_new2("set_re_pad"), INT2NUM(DEFAULT_RECORD_PAD));
	argc += 1;
    }
    ret = bdb_s_new(argc, nargv, obj);
    Data_Get_Struct(ret, bdb_DB, dbst);
    dbst->re_len = rest->re_len;
    dbst->re_pad = rest->re_pad;
    return ret;
}
#endif

static VALUE
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
#if DB_VERSION_MAJOR == 3
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

#define init_txn(txnid, obj, dbst) {					  \
  txnid = NULL;								  \
  GetDB(obj, dbst);							  \
  if (dbst->txn != NULL) {						  \
  if (dbst->txn->txnid == 0)						  \
    rb_warning("using a db handle associated with a closed transaction"); \
    txnid = dbst->txn->txnid;						  \
  }									  \
}

#define set_partial(db, data)			\
    data.flags |= db->partial;			\
    data.dlen = db->dlen;			\
    data.doff = db->doff;

static VALUE
bdb_append(argc, argv, obj)
    int argc;
    VALUE *argv, obj;
{
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBT key, data;
    db_recno_t recno;
    int i;
    VALUE *a;

    rb_secure(4);
    if (argc < 1)
	return obj;
    init_txn(txnid, obj, dbst);
    memset(&key, 0, sizeof(key));
    recno = 1;
    key.data = &recno;
    key.size = sizeof(db_recno_t);
    for (i = 0, a = argv; i < argc; i++, a++) {
	memset(&data, 0, sizeof(data));
	test_dump(dbst, data, *a);
	set_partial(dbst, data);
#if DB_VERSION_MAJOR == 3
	if (dbst->type == DB_QUEUE && dbst->re_len < data.size) {
	    rb_raise(bdb_eFatal, "size > re_len for Queue");
	}
#endif
	test_error(dbst->dbp->put(dbst->dbp, txnid, &key, &data, DB_APPEND));
    }
    return obj;
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
        recno = NUM2INT(a) + dbst->array_base;                  \
        key.data = &recno;                                      \
        key.size = sizeof(db_recno_t);                          \
    }                                                           \
    else {                                                      \
        test_dump(dbst, key, a);                                \
    }                                                           \
}

static VALUE bdb_get();

#if DB_VERSION_MAJOR < 3
#define test_init_lock(dbst) (((dbst)->flags27 & DB_INIT_LOCK)?DB_RMW:0)
#else
#define test_init_lock(dbst) (0)
#endif

static VALUE
bdb_put(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    VALUE a, b, c;
    bdb_DB *dbst;
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
/*
  FOR new 1.6 can store Qnil

    if (NIL_P(b)) {
	ret = test_error(dbst->dbp->del(dbst->dbp, txnid, &key, 0));
	if (ret == DB_NOTFOUND)
	    return Qnil;
	else
	    return obj;
    }
*/
    test_dump(dbst, data, b);
    set_partial(dbst, data);
#if DB_VERSION_MAJOR == 3
    if (dbst->type == DB_QUEUE && dbst->re_len < data.size) {
	rb_raise(bdb_eFatal, "size > re_len for Queue");
    }
#endif
    ret = test_error(dbst->dbp->put(dbst->dbp, txnid, &key, &data, flags));
    if (ret == DB_KEYEXIST)
	return Qfalse;
    else {
	if (dbst->partial) {
	    if (flags & DB_APPEND) {
		a = INT2NUM((int)key.data);
	    }
	    return bdb_get(1, &a, obj);
	}
	else {
	    return b;
	}
    }
}

static VALUE
test_load_key(dbst, key)
    bdb_DB *dbst;
    DBT key;
{
    if (dbst->type == DB_RECNO || dbst->type == DB_QUEUE)
        return INT2NUM((*(db_recno_t *)key.data) - dbst->array_base);
    return test_load(dbst, key);
}

static VALUE
bdb_assoc(dbst, recno, key, data)
    bdb_DB *dbst;
    int recno;
    DBT key, data;
{
    return rb_assoc_new(test_load_key(dbst, key), test_load(dbst, data));
}

static VALUE bdb_has_both(VALUE, VALUE, VALUE);
static VALUE bdb_has_both_internal(VALUE, VALUE, VALUE, VALUE);

static VALUE
bdb_get_internal(argc, argv, obj, notfound)
    int argc;
    VALUE *argv;
    VALUE obj;
    VALUE notfound;
{
    VALUE a, b, c;
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBT key, data;
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
#define DB_GET_BOTH 9
#endif
    DBT datas;
    int flagss;
    int ret, flags;
    db_recno_t recno;

    init_txn(txnid, obj, dbst);
    flagss = flags = 0;
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
    data.flags |= DB_DBT_MALLOC;
    switch(rb_scan_args(argc, argv, "12", &a, &b, &c)) {
    case 3:
        flags = NUM2INT(c);
        if ((flags & ~DB_RMW) == DB_GET_BOTH) {
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
	    return bdb_has_both_internal(obj, a, b, Qtrue);
#else
            test_dump(dbst, data, b);
#endif
        }
	break;
    case 2:
	flagss = flags = NUM2INT(b);
	break;
    }
    test_recno(dbst, key, recno, a);
    set_partial(dbst, data);
    flags |= test_init_lock(dbst);
    ret = test_error(dbst->dbp->get(dbst->dbp, txnid, &key, &data, flags));
    if (ret == DB_NOTFOUND || ret == DB_KEYEMPTY)
        return notfound;
    if (((flags & ~DB_RMW) == DB_GET_BOTH) ||
	((flags & ~DB_RMW) == DB_SET_RECNO)) {
        return bdb_assoc(dbst, recno, key, data);
    }
    return test_load(dbst, data);
}

static VALUE
bdb_get(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    return bdb_get_internal(argc, argv, obj, Qnil);
}

#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1

static VALUE
bdb_btree_key_range(obj, a)
    VALUE obj, a;
{
    bdb_DB *dbst;
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
    return rb_struct_new(bdb_sKeyrange, 
			 rb_float_new(key_range.less),
			 rb_float_new(key_range.equal), 
			 rb_float_new(key_range.greater));
}
#endif



static VALUE
bdb_count(obj, a)
    VALUE obj, a;
{
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    rb_raise(bdb_eFatal, "DB_NEXT_DUP needs Berkeley DB 2.6 or later");
#else
    bdb_DB *dbst;
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

static VALUE
bdb_get_dup(obj, a)
    VALUE obj, a;
{
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    rb_raise(bdb_eFatal, "DB_NEXT_DUP needs Berkeley DB 2.6 or later");
#else
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBC *dbcp;
    DBT key, data;
    int ret, flags, flags27;
    db_recno_t recno;
    db_recno_t count;
    VALUE result;

    init_txn(txnid, obj, dbst);
    flags = 0;
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
    data.flags |= DB_DBT_MALLOC;
    test_recno(dbst, key, recno, a);
    set_partial(dbst, data);
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
    flags27 = test_init_lock(dbst);
    result = rb_ary_new2(0);
    ret = test_error(dbcp->c_get(dbcp, &key, &data, DB_SET | flags27));
    if (ret == DB_NOTFOUND) {
        test_error(dbcp->c_close(dbcp));
	return result;
    }
    rb_ary_push(result, test_load(dbst, data));
    while (1) {
	ret = test_error(dbcp->c_get(dbcp, &key, &data, DB_NEXT_DUP | flags27));
	if (ret == DB_NOTFOUND) {
	    test_error(dbcp->c_close(dbcp));
	    return result;
	}
	free_key(dbst, key);
	rb_ary_push(result, test_load(dbst, data));
    }
    return result;
#endif
}

#if DB_VERSION_MAJOR == 3

static VALUE
bdb_consume(obj)
    VALUE obj;
{
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBT key, data;
    DBC *dbcp;
    int ret;
    db_recno_t recno;

    rb_secure(4);
    init_txn(txnid, obj, dbst);
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    recno = 1;
    key.data = &recno;
    key.size = sizeof(db_recno_t);
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
    ret = test_error(dbcp->c_get(dbcp, &key, &data, DB_CONSUME));
    test_error(dbcp->c_close(dbcp));
    if (ret == DB_NOTFOUND) {
	return Qnil;
    }
    return bdb_assoc(dbst, recno, key, data);
}

#endif

static VALUE
bdb_has_key(obj, key)
    VALUE obj, key;
{
    return bdb_get_internal(1, &key, obj, Qfalse);
}

static VALUE
bdb_has_both_internal(obj, a, b, flag)
    VALUE obj, a, b, flag;
{
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBC *dbcp;
    DBT key, data;
    DBT datas;
    int flagss;
    int ret, flags;
    db_recno_t recno;

    init_txn(txnid, obj, dbst);
    flagss = flags = 0;
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
    data.flags |= DB_DBT_MALLOC;
    test_dump(dbst, datas, b);
    test_recno(dbst, key, recno, a);
    set_partial(dbst, data);
    flags |= test_init_lock(dbst);
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    ret = test_error(dbcp->c_get(dbcp, &key, &data, DB_SET));
    if (ret == DB_NOTFOUND || ret == DB_KEYEMPTY) {
	dbcp->c_close(dbcp);
        return (flag == Qtrue)?Qnil:Qfalse;
    }
    if (datas.size == data.size &&
	memcmp(datas.data, data.data, data.size) == 0) {
	dbcp->c_close(dbcp);
	if (flag == Qtrue) {
	    return bdb_assoc(dbst, recno, key, data);
	}
	else {
	    free_key(dbst, key);
	    free(data.data);
	    return Qtrue;
	}
    }
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
#define DB_NEXT_DUP 0
    free_key(dbst, key);
    free(data.data);
    dbcp->c_close(dbcp);
    return (flag == Qtrue)?Qnil:Qfalse;
#endif
#if DB_VERSION_MAJOR < 3 || DB_VERSION_MINOR < 1
    if (dbst->type == DB_RECNO || dbst->type == DB_QUEUE) {
	free(data.data);
	dbcp->c_close(dbcp);
	return Qfalse;
    }
#endif
    while (1) {
	free_key(dbst, key);
	free(data.data);
	memset(&data, 0, sizeof(DBT));
	data.flags |= DB_DBT_MALLOC;
	ret = test_error(dbcp->c_get(dbcp, &key, &data, DB_NEXT_DUP));
	if (ret == DB_NOTFOUND || ret == DB_KEYEMPTY) {
	    dbcp->c_close(dbcp);
	    return Qfalse;
	}
	if (datas.size == data.size &&
	    memcmp(datas.data, data.data, data.size) == 0) {
	    free_key(dbst, key);
	    free(data.data);
	    dbcp->c_close(dbcp);
	    return Qtrue;
	}
    }
    return Qfalse;
}

static VALUE
bdb_has_both(obj, a, b)
    VALUE obj, a, b;
{
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBT key, data;
    DBT datas;
    int ret, flags;
    db_recno_t recno;

#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    return bdb_has_both_internal(obj, a, b, Qfalse);
#else
    init_txn(txnid, obj, dbst);
#if DB_VERSION_MAJOR < 3 || DB_VERSION_MINOR < 1
    if (dbst->type == DB_RECNO || dbst->type == DB_QUEUE) {
	return bdb_has_both_internal(obj, a, b, Qfalse);
    }
#endif
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
    data.flags |= DB_DBT_MALLOC;
    test_dump(dbst, data, b);
    test_recno(dbst, key, recno, a);
    set_partial(dbst, data);
    flags = DB_GET_BOTH | test_init_lock(dbst);
    ret = test_error(dbst->dbp->get(dbst->dbp, txnid, &key, &data, flags));
    if (ret == DB_NOTFOUND || ret == DB_KEYEMPTY)
        return Qfalse;
    free(data.data);
    if (dbst->type != DB_RECNO && dbst->type != DB_QUEUE) {
	free(key.data);
    }
    return Qtrue;
#endif
}

static VALUE
bdb_del(obj, a)
    VALUE a, obj;
{
    bdb_DB *dbst;
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
bdb_intern_shift_pop(obj, depart)
    VALUE obj;
    int depart;
{
    bdb_DB *dbst;
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
    ret = test_error(dbcp->c_get(dbcp, &key, &data, depart | flags));
    if (ret == DB_NOTFOUND) {
        test_error(dbcp->c_close(dbcp));
        return Qnil;
    }
    test_error(dbcp->c_del(dbcp, 0));
    test_error(dbcp->c_close(dbcp));
    return bdb_assoc(dbst, recno, key, data);
}

static VALUE
bdb_shift(obj)
    VALUE obj;
{
    return bdb_intern_shift_pop(obj, DB_FIRST);
}

static VALUE
bdb_pop(obj)
    VALUE obj;
{
    return bdb_intern_shift_pop(obj, DB_LAST);
}

static VALUE
bdb_empty(obj)
    VALUE obj;
{
    bdb_DB *dbst;
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
    free(data.data);
    test_error(dbcp->c_close(dbcp));
    return Qfalse;
}

static VALUE
bdb_delete_if(obj)
    VALUE obj;
{
    bdb_DB *dbst;
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
        if (RTEST(rb_yield(bdb_assoc(dbst, recno, key, data)))) {
            test_error(dbcp->c_del(dbcp, 0));
        }
    } while (1);
    return obj;
}

static VALUE
bdb_length(obj)
    VALUE obj;
{
    bdb_DB *dbst;
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
	free(data.data);
        value++;
    } while (1);
    return INT2NUM(value);
}

typedef struct {
    int sens;
    bdb_DB *dbst;
    DBC *dbcp;
} eachst;

static VALUE
bdb_each_ensure(st)
    eachst *st;
{
    st->dbcp->c_close(st->dbcp);
    return Qnil;
}

static VALUE
bdb_i_each_value(st)
    eachst *st;
{
    bdb_DB *dbst;
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
bdb_each_valuec(obj, sens)
    VALUE obj;
    int sens;
{
    bdb_DB *dbst;
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
    rb_ensure(bdb_i_each_value, (VALUE)&st, bdb_each_ensure, (VALUE)&st); 
    return obj;
}

static VALUE bdb_each_value(obj) VALUE obj;{ return bdb_each_valuec(obj, DB_NEXT); }
static VALUE bdb_each_eulav(obj) VALUE obj;{ return bdb_each_valuec(obj, DB_PREV); }

static VALUE 
bdb_i_each_key(st)
    eachst *st;
{
    bdb_DB *dbst;
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
	free(data.data);
	rb_yield(test_load_key(dbst, key));
    } while (1);
    return Qnil;
}

static VALUE
bdb_each_keyc(obj, sens)
    VALUE obj;
    int sens;
{
    bdb_DB *dbst;
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
    rb_ensure(bdb_i_each_key, (VALUE)&st, bdb_each_ensure, (VALUE)&st); 
    return obj;
}

static VALUE bdb_each_key(obj) VALUE obj;{ return bdb_each_keyc(obj, DB_NEXT); }
static VALUE bdb_each_yek(obj) VALUE obj;{ return bdb_each_keyc(obj, DB_PREV); }

static VALUE 
bdb_i_each_common(st)
    eachst *st;
{
    bdb_DB *dbst;
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
        rb_yield(bdb_assoc(dbst, recno, key, data));
    } while (1);
    return Qnil;
}

static VALUE
bdb_each_common(obj, sens)
    VALUE obj;
    int sens;
{
    bdb_DB *dbst;
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
    rb_ensure(bdb_i_each_common, (VALUE)&st, bdb_each_ensure, (VALUE)&st); 
    return obj;
}

static VALUE bdb_each_pair(obj) VALUE obj;{ return bdb_each_common(obj, DB_NEXT); }
static VALUE bdb_each_riap(obj) VALUE obj;{ return bdb_each_common(obj, DB_PREV); }

static VALUE
bdb_to_a(obj)
    VALUE obj;
{
    bdb_DB *dbst;
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
        rb_ary_push(ary, bdb_assoc(dbst, recno, key, data));
    } while (1);
    return ary;
}


static VALUE
each_pair(obj)
    VALUE obj;
{
    return rb_funcall(obj, rb_intern("each_pair"), 0, 0);
}

static VALUE
bdb_update_i(pair, obj)
    VALUE pair, obj;
{
    VALUE argv[2];

    Check_Type(pair, T_ARRAY);
    if (RARRAY(pair)->len < 2) {
	rb_raise(rb_eArgError, "pair must be [key, value]");
    }
    bdb_put(2, pair, obj);
    return Qnil;
}

static VALUE
bdb_update(obj, other)
    VALUE obj, other;
{
    rb_iterate(each_pair, other, bdb_update_i, obj);
    return obj;
}

static VALUE
bdb_clear(obj)
    VALUE obj;
{
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBC *dbcp;
    DBT key, data;
    int ret, flags;
    db_recno_t recno;
    VALUE ary;

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
    flags = DB_NEXT | test_init_lock(dbst);
    do {
        ret = test_error(dbcp->c_get(dbcp, &key, &data, flags));
        if (ret == DB_NOTFOUND) {
            test_error(dbcp->c_close(dbcp));
            return obj;
        }
	test_error(dbcp->c_del(dbcp, 0));
    } while (1);
    return obj;
}

static VALUE
bdb_replace(obj, other)
{
    bdb_clear(obj);
    rb_iterate(each_pair, other, bdb_update_i, obj);
    return obj;
}

static VALUE
bdb_to_hash_intern(obj, tes)
    VALUE obj, tes;
{
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBC *dbcp;
    DBT key, data;
    int ret, flags;
    db_recno_t recno;
    VALUE hash;

    hash = rb_hash_new();
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
            return hash;
        }
	if (tes == Qtrue) {
	    rb_hash_aset(hash, test_load_key(dbst, key), test_load(dbst, data));
	}
	else {
	    rb_hash_aset(hash, test_load(dbst, data), test_load_key(dbst, key));
	}
    } while (1);
    return hash;
}

static VALUE
bdb_invert(obj)
    VALUE obj;
{
    return bdb_to_hash_intern(obj, Qfalse);
}

static VALUE
bdb_to_hash(obj)
    VALUE obj;
{
    return bdb_to_hash_intern(obj, Qtrue);
}

static VALUE
bdb_reject(obj)
    VALUE obj;
{
    return rb_hash_delete_if(bdb_to_hash(obj));
}
 
static VALUE
bdb_values(obj)
    VALUE obj;
{
    bdb_DB *dbst;
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
bdb_internal_value(obj, a, b)
    VALUE obj, a, b;
{
    bdb_DB *dbst;
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
        if (rb_equal(a, test_load(dbst, data)) == Qtrue) {
	    VALUE d;

            test_error(dbcp->c_close(dbcp));
	    d = (b == Qfalse)?Qtrue:test_load_key(dbst, key);
	    free_key(dbst, key);
	    return  d;
        }
	free_key(dbst, key);
    } while (1);
    return Qfalse;
}

static VALUE
bdb_index(obj, a)
    VALUE obj, a;
{
    return bdb_internal_value(obj, a, Qtrue);
}

static VALUE
bdb_indexes(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    VALUE indexes;
    int i;

    indexes = rb_ary_new2(argc);
    for (i = 0; i < argc; i++) {
	RARRAY(indexes)->ptr[i] = bdb_get(1, &argv[i], obj);
    }
    RARRAY(indexes)->len = i;
    return indexes;
}

static VALUE
bdb_has_value(obj, a)
    VALUE obj, a;
{
    return bdb_internal_value(obj, a, Qfalse);
}

static VALUE
bdb_keys(obj)
    VALUE obj;
{
    bdb_DB *dbst;
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
	free(data.data);
	rb_ary_push(ary, test_load_key(dbst, key));
	free_key(dbst, key);
    } while (1);
    return ary;
}

static VALUE
bdb_sync(obj)
    VALUE obj;
{
    bdb_DB *dbst;

    if (!OBJ_TAINTED(obj) && rb_safe_level() >= 4)
	rb_raise(rb_eSecurityError, "Insecure: can't sync the database");
    GetDB(obj, dbst);
    test_error(dbst->dbp->sync(dbst->dbp, 0));
    return Qtrue;
}

#if DB_VERSION_MAJOR == 3
static VALUE
bdb_hash_stat(obj)
    VALUE obj;
{
    bdb_DB *dbst;
    DB_HASH_STAT *stat;
    VALUE hash;
    int ret;
    GetDB(obj, dbst);
    test_error(dbst->dbp->stat(dbst->dbp, &stat, 0, 0));
    hash = rb_hash_new();
    rb_hash_aset(hash, rb_tainted_str_new2("hash_magic"), INT2NUM(stat->hash_magic));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_version"), INT2NUM(stat->hash_version));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_pagesize"), INT2NUM(stat->hash_pagesize));
#if (DB_VERSION_MINOR >= 1 && DB_VERSION_PATCH >= 14) || DB_VERSION_MINOR >= 2
    rb_hash_aset(hash, rb_tainted_str_new2("hash_nkeys"), INT2NUM(stat->hash_nkeys));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_nrecs"), INT2NUM(stat->hash_nkeys));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_ndata"), INT2NUM(stat->hash_ndata));
#else
    rb_hash_aset(hash, rb_tainted_str_new2("hash_nrecs"), INT2NUM(stat->hash_nrecs));
#endif
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
bdb_tree_stat(obj)
    VALUE obj;
{
    bdb_DB *dbst;
    DB_BTREE_STAT *stat;
    VALUE hash;
    char pad;

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
    pad = (char)stat->bt_re_pad;
    rb_hash_aset(hash, rb_tainted_str_new2("bt_re_pad"), rb_tainted_str_new(&pad, 1));
    free(stat);
    return hash;
}

static VALUE
bdb_recno_length(obj)
    VALUE obj;
{
    bdb_DB *dbst;
    DB_BTREE_STAT *stat;
    VALUE hash;

    GetDB(obj, dbst);
    test_error(dbst->dbp->stat(dbst->dbp, &stat, 0, DB_RECORDCOUNT));
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1
    hash = INT2NUM(stat->bt_nkeys);
#else
    hash = INT2NUM(stat->bt_nrecs);
#endif
    free(stat);
    return hash;
}

#if DB_VERSION_MAJOR == 3
static VALUE
bdb_queue_stat(obj)
    VALUE obj;
{
    bdb_DB *dbst;
    DB_QUEUE_STAT *stat;
    VALUE hash;
    char pad;

    GetDB(obj, dbst);
    test_error(dbst->dbp->stat(dbst->dbp, &stat, 0, 0));
    hash = rb_hash_new();
    rb_hash_aset(hash, rb_tainted_str_new2("qs_magic"), INT2NUM(stat->qs_magic));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_version"), INT2NUM(stat->qs_version));
#if (DB_VERSION_MINOR >= 1 && DB_VERSION_PATCH >= 14) || DB_VERSION_MINOR >= 2
    rb_hash_aset(hash, rb_tainted_str_new2("qs_nrecs"), INT2NUM(stat->qs_nkeys));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_nkeys"), INT2NUM(stat->qs_nkeys));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_ndata"), INT2NUM(stat->qs_ndata));
#else
    rb_hash_aset(hash, rb_tainted_str_new2("qs_nrecs"), INT2NUM(stat->qs_nrecs));
#endif
    rb_hash_aset(hash, rb_tainted_str_new2("qs_pages"), INT2NUM(stat->qs_pages));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_pagesize"), INT2NUM(stat->qs_pagesize));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_pgfree"), INT2NUM(stat->qs_pgfree));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_re_len"), INT2NUM(stat->qs_re_len));
    pad = (char)stat->qs_re_pad;
    rb_hash_aset(hash, rb_tainted_str_new2("qs_re_pad"), rb_tainted_str_new(&pad, 1));
#if DB_VERSION_MINOR < 2
    rb_hash_aset(hash, rb_tainted_str_new2("qs_start"), INT2NUM(stat->qs_start));
#endif
    rb_hash_aset(hash, rb_tainted_str_new2("qs_first_recno"), INT2NUM(stat->qs_first_recno));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_cur_recno"), INT2NUM(stat->qs_cur_recno));
    free(stat);
    return hash;
}

static VALUE
bdb_queue_padlen(obj)
    VALUE obj;
{
    bdb_DB *dbst;
    DB_QUEUE_STAT *stat;
    VALUE hash;
    char pad;

    GetDB(obj, dbst);
    test_error(dbst->dbp->stat(dbst->dbp, &stat, 0, 0));
    pad = (char)stat->qs_re_pad;
    hash = rb_assoc_new(rb_tainted_str_new(&pad, 1), INT2NUM(stat->qs_re_len));
    free(stat);
    return hash;
}
#endif

static VALUE
bdb_set_partial(obj, a, b)
    VALUE obj, a, b;
{
    bdb_DB *dbst;
    VALUE ret;

    GetDB(obj, dbst);
    ret = rb_ary_new2(3);
    rb_ary_push(ret, (dbst->partial == DB_DBT_PARTIAL)?Qtrue:Qfalse);
    rb_ary_push(ret, INT2NUM(dbst->doff));
    rb_ary_push(ret, INT2NUM(dbst->dlen));
    dbst->doff = NUM2UINT(a);
    dbst->dlen = NUM2UINT(b);
    dbst->partial = DB_DBT_PARTIAL;
    return ret;
}

static VALUE
bdb_clear_partial(obj)
    VALUE obj;
{
    bdb_DB *dbst;
    VALUE ret;

    GetDB(obj, dbst);
    ret = rb_ary_new2(3);
    rb_ary_push(ret, (dbst->partial == DB_DBT_PARTIAL)?Qtrue:Qfalse);
    rb_ary_push(ret, INT2NUM(dbst->doff));
    rb_ary_push(ret, INT2NUM(dbst->dlen));
    dbst->doff = dbst->dlen = dbst->partial = 0;
    return ret;
}

#if DB_VERSION_MAJOR == 3
static VALUE
bdb_i_create(obj)
    VALUE obj;
{
    DB *dbp;
    bdb_ENV *dbenvst;
    DB_ENV *dbenvp;
    bdb_DB *dbst;
    VALUE ret, env;
    int val;

    dbenvp = NULL;
    env = 0;
    if (rb_obj_is_kind_of(obj, bdb_cEnv)) {
        GetEnvDB(obj, dbenvst);
        dbenvp = dbenvst->dbenvp;
        env = obj;
    }
    test_error(db_create(&dbp, dbenvp, 0), val, "create");
    dbp->set_errpfx(dbp, "BDB::");
    dbp->set_errcall(dbp, bdb_env_errcall);
    ret = Data_Make_Struct(bdb_cCommon, bdb_DB, 0, bdb_free, dbst);
    rb_obj_call_init(ret, 0, 0);
    dbst->env = env;
    dbst->dbp = dbp;
    if (dbenvp)
	dbst->flags27 = dbenvp->flags;
    return ret;
}
#endif

static VALUE
bdb_s_upgrade(argc, argv, obj)
    VALUE obj;
    int argc;
    VALUE *argv;
{
#if DB_VERSION_MAJOR == 3
    bdb_DB *dbst;
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
    val = bdb_i_create(obj);
    GetDB(val, dbst);
    test_error(dbst->dbp->upgrade(dbst->dbp, RSTRING(a)->ptr, flags));
    return val;

#else
    rb_raise(bdb_eFatal, "You can't upgrade a database with this version of DB");
#endif
}

static VALUE
bdb_s_remove(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
#if DB_VERSION_MAJOR == 3
    bdb_DB *dbst;
    int ret;
    VALUE a, b, c;
    char *name, *subname;

    rb_secure(2);
    c = bdb_i_create(obj);
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

static void bdb_cursor_free(bdb_DBC *dbcst);

#define GetCursorDB(obj, dbcst)			\
{						\
    Data_Get_Struct(obj, bdb_DBC, dbcst);	\
    if (dbcst->dbc == 0)			\
        rb_raise(bdb_eFatal, "closed cursor");	\
}

#if DB_VERSION_MAJOR > 2 || (DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR >= 6)

static VALUE
bdb_i_joinclose(st)
    eachst *st;
{
    if (st->dbcp && st->dbst && st->dbst->dbp) {
	st->dbcp->c_close(st->dbcp);
    }
    return Qnil;
}

 
static VALUE
bdb_i_join(st)
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
	rb_yield(bdb_assoc(st->dbst, recno, key, data));
    } while (1);
    return Qnil;
}
 
static VALUE
bdb_join(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    bdb_DB *dbst;
    bdb_DBC *dbcst;
    DBC *dbc, **dbcarr;
    int flags, i;
    eachst st;
    VALUE a, b, c;

    c = 0;
#if DB_VERSION_MAJOR == 2 && (DB_VERSION_MINOR < 5 || (DB_VERSION_MINOR == 5 && DB_VERSION_PATCH < 2))
    rb_raise(bdb_eFatal, "join needs Berkeley DB 2.5.2 or later");
#endif
    flags = 0;
    GetDB(obj, dbst);
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	flags = NUM2INT(b);
    }
    if (TYPE(a) != T_ARRAY) {
        rb_raise(bdb_eFatal, "first argument must an array of cursors");
    }
    if (RARRAY(a)->len == 0) {
        rb_raise(bdb_eFatal, "empty array");
    }
    dbcarr = ALLOCA_N(DBC *, RARRAY(a)->len + 1);
    {
	DBC **dbs;

	for (dbs = dbcarr, i = 0; i < RARRAY(a)->len; i++, dbs++) {
	    if (!rb_obj_is_kind_of(RARRAY(a)->ptr[i], bdb_cCursor)) {
		rb_raise(bdb_eFatal, "element %d is not a cursor");
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
    rb_ensure(bdb_i_join, (VALUE)&st, bdb_i_joinclose, (VALUE)&st);
    return obj;
}

static VALUE
bdb_byteswapp(obj)
    VALUE obj;
{
    bdb_DB *dbst;
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 5
    rb_raise(bdb_eFatal, "byteswapped needs Berkeley DB 2.5 or later");
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
bdb_cursor_free(dbcst)
    bdb_DBC *dbcst;
{
    if (dbcst->dbc && dbcst->dbst && dbcst->dbst->dbp) {
        test_error(dbcst->dbc->c_close(dbcst->dbc));
        dbcst->dbc = NULL;
    }
    free(dbcst);
}

static VALUE
bdb_cursor(obj)
    VALUE obj;
{
    bdb_DB *dbst;
    DB_TXN *txnid;
    bdb_DBC *dbcst;
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
    a = Data_Make_Struct(bdb_cCursor, bdb_DBC, 0, bdb_cursor_free, dbcst);
    dbcst->dbc = dbc;
    dbcst->dbst = dbst;
    return a;
}

static VALUE
bdb_cursor_close(obj)
    VALUE obj;
{
    bdb_DBC *dbcst;
    
    if (!OBJ_TAINTED(obj) && rb_safe_level() >= 4)
	rb_raise(rb_eSecurityError, "Insecure: can't close the cursor");
    GetCursorDB(obj, dbcst);
    test_error(dbcst->dbc->c_close(dbcst->dbc));
    dbcst->dbc = NULL;
    return Qtrue;
}
  
static VALUE
bdb_cursor_del(obj)
    VALUE obj;
{
    int flags = 0;
    bdb_DBC *dbcst;
    
    rb_secure(4);
    GetCursorDB(obj, dbcst);
    test_error(dbcst->dbc->c_del(dbcst->dbc, flags));
    return Qtrue;
}

#if DB_VERSION_MAJOR == 3

static VALUE
bdb_cursor_dup(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    int ret, flags = 0;
    VALUE a, b;
    bdb_DBC *dbcst, *dbcstdup;
    DBC *dbcdup;
    
    if (rb_scan_args(argc, argv, "01", &a))
        flags = NUM2INT(a);
    GetCursorDB(obj, dbcst);
    test_error(dbcst->dbc->c_dup(dbcst->dbc, &dbcdup, flags));
    b = Data_Make_Struct(bdb_cCursor, bdb_DBC, 0, bdb_cursor_free, dbcstdup);
    dbcstdup->dbc = dbcdup;
    dbcstdup->dbst = dbcst->dbst;
    return b;
}

#endif

static VALUE
bdb_cursor_count(obj)
    VALUE obj;
{
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    rb_raise(bdb_eFatal, "DB_NEXT_DUP needs Berkeley DB 2.6 or later");
#else
    int ret;

#if !(DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1)
    DBT key, data;
    DBT key_o, data_o;
#endif
    bdb_DBC *dbcst;
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
	    free(data_o.data);
	    return INT2NUM(count);
	}
	free_key(dbst, key);
	free(data.data);
	count++;
    }
    return INT2NUM(-1);
#endif
#endif
}

static VALUE
bdb_cursor_get(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    VALUE a, b, c;
    int flags, cnt, ret;
    DBT key, data;
    bdb_DBC *dbcst;
    db_recno_t recno;

    cnt = rb_scan_args(argc, argv, "12", &a, &b, &c);
    flags = NUM2INT(a);
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    GetCursorDB(obj, dbcst);
    if (flags == DB_SET_RECNO) {
	if (dbcst->dbst->type != DB_BTREE || !(dbcst->dbst->flags & DB_RECNUM)) {
	    rb_raise(bdb_eFatal, "database must be Btree with RECNUM for SET_RECNO");
	}
        if (cnt != 2)
            rb_raise(bdb_eFatal, "invalid number of arguments");
	recno = NUM2INT(b);
	key.data = &recno;
	key.size = sizeof(db_recno_t);
	key.flags |= DB_DBT_MALLOC;
	data.flags |= DB_DBT_MALLOC;
    }
    else if (flags == DB_SET || flags == DB_SET_RANGE) {
        if (cnt != 2)
            rb_raise(bdb_eFatal, "invalid number of arguments");
        test_recno(dbcst->dbst, key, recno, b);
	data.flags |= DB_DBT_MALLOC;
    }
#if DB_VERSION_MAJOR > 2 || (DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR >= 6)
    else if (flags == DB_GET_BOTH) {
        if (cnt != 3)
            rb_raise(bdb_eFatal, "invalid number of arguments");
        test_recno(dbcst->dbst, key, recno, b);
        test_dump(dbcst->dbst, data, c);
    }
#endif
    else {
	if (cnt != 1) {
	    rb_raise(bdb_eFatal, "invalid number of arguments");
	}
	key.flags |= DB_DBT_MALLOC;
	data.flags |= DB_DBT_MALLOC;
    }
    set_partial(dbcst->dbst, data);
    ret = test_error(dbcst->dbc->c_get(dbcst->dbc, &key, &data, flags));
    if (ret == DB_NOTFOUND || ret == DB_KEYEMPTY)
        return Qnil;
    return bdb_assoc(dbcst->dbst, recno, key, data);
}

static VALUE
bdb_cursor_set_xxx(obj, a, flag)
    VALUE obj, a;
    int flag;
{
    VALUE *b;
    b = ALLOCA_N(VALUE, 2);
    b[0] = INT2NUM(flag);
    b[1] = a;
    return bdb_cursor_get(2, b, obj);
}

static VALUE 
bdb_cursor_set(obj, a)
    VALUE obj, a;
{ 
    return bdb_cursor_set_xxx(obj, a, DB_SET);
}

static VALUE 
bdb_cursor_set_range(obj, a)
    VALUE obj, a;
{ 
    return bdb_cursor_set_xxx(obj, a, DB_SET_RANGE);
}

static VALUE 
bdb_cursor_set_recno(obj, a)
    VALUE obj, a;
{ 
    return bdb_cursor_set_xxx(obj, a, DB_SET_RECNO);
}

static VALUE
bdb_cursor_xxx(obj, val)
    VALUE obj;
    int val;
{
    VALUE b;
    b = INT2NUM(val);
    return bdb_cursor_get(1, &b, obj);
}

static VALUE
bdb_cursor_next(obj)
    VALUE obj;
{
    return bdb_cursor_xxx(obj, DB_NEXT);
}

static VALUE
bdb_cursor_prev(obj)
    VALUE obj;
{
    return bdb_cursor_xxx(obj, DB_PREV);
}

static VALUE
bdb_cursor_first(obj)
    VALUE obj;
{
    return bdb_cursor_xxx(obj, DB_FIRST);
}

static VALUE
bdb_cursor_last(obj)
    VALUE obj;
{
    return bdb_cursor_xxx(obj, DB_LAST);
}

static VALUE
bdb_cursor_current(obj)
    VALUE obj;
{
    return bdb_cursor_xxx(obj, DB_CURRENT);
}

static VALUE
bdb_cursor_put(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    int flags, cnt;
    DBT key, data;
    bdb_DBC *dbcst;
    VALUE a, b, c, f;
    db_recno_t recno;
    int ret;

    rb_secure(4);
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    cnt = rb_scan_args(argc, argv, "21", &a, &b, &c);
    GetCursorDB(obj, dbcst);
    flags = NUM2INT(c);
    if (flags & (DB_KEYFIRST | DB_KEYLAST)) {
        if (cnt != 3)
            rb_raise(bdb_eFatal, "invalid number of arguments");
        test_recno(dbcst->dbst, key, recno, b);
        test_dump(dbcst->dbst, data, c);
	f = c;
    }
    else {
        test_dump(dbcst->dbst, data, b);
	f = b;
    }
    set_partial(dbcst->dbst, data);
    ret = test_error(dbcst->dbc->c_put(dbcst->dbc, &key, &data, flags));
    if (cnt == 3) {
	free_key(dbcst->dbst, key);
    }
    if (data.flags & DB_DBT_MALLOC)
	free(data.data);
    if (ret == DB_KEYEXIST) {
	return Qfalse;
    }
    else {
	if (dbcst->dbst->partial) {
	    return bdb_cursor_current(obj);
	}
	else {
	    return f;
	}
    }
}

/* TRANSACTION */

static void 
bdb_txn_free(txnst)
    bdb_TXN *txnst;
{
    if (txnst->txnid && txnst->parent == NULL) {
        test_error(txn_abort(txnst->txnid));
        txnst->txnid = NULL;
    }
    free(txnst);
}

static void 
bdb_txn_mark(txnst)
    bdb_TXN *txnst;
{
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
        a = Data_Make_Struct(CLASS_OF(argv[i]), bdb_DB, 0, bdb_free, dbh);
        GetDB(argv[i], dbp);
        MEMCPY(dbh, dbp, bdb_DB, 1);
        dbh->txn = txnst;
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
bdb_txn_abort(obj)
    VALUE obj;
{
    bdb_TXN *txnst;
    VALUE db;

    GetTxnDB(obj, txnst);
    test_error(txn_abort(txnst->txnid));
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

#define BDB_TXN_COMMIT 0x80

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
    int flags, commit, marshal;

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
    test_error(txn_begin(dbenvp->tx_info, txnpar, &txn));
#else
    test_error(txn_begin(dbenvp, txnpar, &txn, flags));
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
    if (rb_iterator_p()) {
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
bdb_txn_prepare(obj)
    VALUE obj;
{
    bdb_TXN *txnst;

    GetTxnDB(obj, txnst);
    test_error(txn_prepare(txnst->txnid));
    return Qtrue;
}
        
/* LOCK */

static VALUE
bdb_env_lockid(obj)
    VALUE obj;
{
    unsigned int idp;
    bdb_ENV *dbenvst;
    bdb_LOCKID *dblockid;
    VALUE a;

    GetEnvDB(obj, dbenvst);
#if DB_VERSION_MAJOR < 3
    test_error(lock_id(dbenvst->dbenvp->lk_info, &idp));
#else
    test_error(lock_id(dbenvst->dbenvp, &idp));
#endif
    a = Data_Make_Struct(bdb_cLockid, bdb_LOCKID, 0, free, dblockid);
    dblockid->lock = idp;
    dblockid->dbenvp = dbenvst->dbenvp;
    return a;
}

static VALUE
bdb_env_lockdetect(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    VALUE a, b;
    bdb_ENV *dbenvst;
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
bdb_env_lockstat(obj)
    VALUE obj;
{
    bdb_ENV *dbenvst;
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
    Data_Get_Struct(obj, bdb_LOCKID, lockid);		\
    if (lockid->dbenvp == 0)				\
        rb_raise(bdb_eFatal, "closed environment");	\
}

static void
lock_free(lock)
    bdb_LOCK *lock;
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
bdb_lockid_get(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    bdb_LOCKID *lockid;
    DB_LOCK lock;
    bdb_LOCK *lockst;
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
    res = Data_Make_Struct(bdb_cLock, bdb_LOCK, 0, lock_free, lockst);
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
    Data_Get_Struct(obj, bdb_LOCK, lock);		\
    if (lock->dbenvp == 0)				\
        rb_raise(bdb_eFatal, "closed environment");	\
}

struct lockreq {
    DB_LOCKREQ *list;
};

static VALUE
bdb_lockid_each(obj, listobj)
    VALUE obj, listobj;
{
    VALUE key, value;
    DB_LOCKREQ *list;
    struct lockreq *listst;
    char *options;

    Data_Get_Struct(listobj, struct lockreq, listst);
    list = listst->list;
    key = rb_ary_entry(obj, 0);
    value = rb_ary_entry(obj, 1);
    key = rb_obj_as_string(key);
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
	bdb_LOCK *lockst;

	if (!rb_obj_is_kind_of(value, bdb_cLock)) {
	    rb_raise(bdb_eFatal, "BDB::Lock expected");
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
bdb_lockid_vec(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    DB_LOCKREQ *list;
    bdb_LOCKID *lockid;
    bdb_LOCK *lockst;
    unsigned int flags;
    VALUE a, b, c, res;
    int i, n, err;
    VALUE listobj;
    struct lockreq *listst;

    flags = 0;
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	flags = NUM2UINT(b);
    }
    Check_Type(a, T_ARRAY);
    list = ALLOCA_N(DB_LOCKREQ, RARRAY(a)->len);
    MEMZERO(list, DB_LOCKREQ, RARRAY(a)->len);
    listobj = Data_Make_Struct(obj, struct lockreq, 0, free, listst);
    for (i = 0; i < RARRAY(a)->len; i++) {
	b = RARRAY(a)->ptr[i];
	Check_Type(b, T_HASH);
	listst->list = &list[i];
	rb_iterate(rb_each, b, bdb_lockid_each, listobj);
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
	res = (err == DB_LOCK_DEADLOCK)?bdb_eLock:bdb_eFatal;
        if (bdb_errcall) {
            bdb_errcall = 0;
            rb_raise(res, "%s -- %s", RSTRING(bdb_errstr)->ptr, db_strerror(err));
        }
        else
            rb_raise(res, "%s", db_strerror(err));
    }			
    res = rb_ary_new();
    n = 0;
    for (i = 0; i < RARRAY(a)->len; i++) {
	if (list[i].op == DB_LOCK_GET) {
	    c = Data_Make_Struct(bdb_cLock, bdb_LOCK, 0, lock_free, lockst);
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
bdb_lock_put(obj)
    VALUE obj;
{
    bdb_LOCK *lockst;

    GetLock(obj, lockst);
#if DB_VERSION_MAJOR < 3
    test_error(lock_put(lockst->dbenvp->lk_info, lockst->lock));
#else
    test_error(lock_put(lockst->dbenvp, lockst->lock));
#endif
    return Qnil;
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
    bdb_mMarshal = rb_const_get(rb_cObject, rb_intern("Marshal"));
    id_dump = rb_intern("dump");
    id_load = rb_intern("load");
    id_current_db = rb_intern("bdb_current_db");
    id_bt_compare = rb_intern("bdb_bt_compare");
    id_bt_prefix = rb_intern("bdb_bt_prefix");
    id_dup_compare = rb_intern("bdb_dup_compare");
    id_h_hash = rb_intern("bdb_h_hash");
    id_proc_call = rb_intern("call");
    bdb_mDb = rb_define_module("BDB");
    bdb_eFatal = rb_define_class_under(bdb_mDb, "Fatal", rb_eStandardError);
    bdb_eLock = rb_define_class_under(bdb_mDb, "DeadLock", bdb_eFatal);
/* CONSTANT */
    rb_define_const(bdb_mDb, "VERSION", version);
    rb_define_const(bdb_mDb, "VERSION_MAJOR", INT2FIX(major));
    rb_define_const(bdb_mDb, "VERSION_MINOR", INT2FIX(minor));
    rb_define_const(bdb_mDb, "VERSION_PATCH", INT2FIX(patch));
    rb_define_const(bdb_mDb, "BTREE", INT2FIX(DB_BTREE));
    rb_define_const(bdb_mDb, "HASH", INT2FIX(DB_HASH));
    rb_define_const(bdb_mDb, "RECNO", INT2FIX(DB_RECNO));
#if DB_VERSION_MAJOR < 3
    rb_define_const(bdb_mDb, "QUEUE", INT2FIX(0));
#else
    rb_define_const(bdb_mDb, "QUEUE", INT2FIX(DB_QUEUE));
#endif
    rb_define_const(bdb_mDb, "UNKNOWN", INT2FIX(DB_UNKNOWN));
    rb_define_const(bdb_mDb, "AFTER", INT2FIX(DB_AFTER));
    rb_define_const(bdb_mDb, "APPEND", INT2FIX(DB_APPEND));
    rb_define_const(bdb_mDb, "ARCH_ABS", INT2FIX(DB_ARCH_ABS));
    rb_define_const(bdb_mDb, "ARCH_DATA", INT2FIX(DB_ARCH_DATA));
    rb_define_const(bdb_mDb, "ARCH_LOG", INT2FIX(DB_ARCH_LOG));
    rb_define_const(bdb_mDb, "BEFORE", INT2FIX(DB_BEFORE));
    rb_define_const(bdb_mDb, "CHECKPOINT", INT2FIX(DB_CHECKPOINT));
#if DB_VERSION_MAJOR < 3
    rb_define_const(bdb_mDb, "CONSUME", INT2FIX(0));
#else
    rb_define_const(bdb_mDb, "CONSUME", INT2FIX(DB_CONSUME));
#ifdef DB_CDB_ALLDB
    rb_define_const(bdb_mDb, "CDB_ALLDB", INT2FIX(DB_CDB_ALLDB));
#endif
#endif
    rb_define_const(bdb_mDb, "CREATE", INT2FIX(DB_CREATE));
    rb_define_const(bdb_mDb, "CURLSN", INT2FIX(DB_CURLSN));
    rb_define_const(bdb_mDb, "CURRENT", INT2FIX(DB_CURRENT));
    rb_define_const(bdb_mDb, "DBT_MALLOC", INT2FIX(DB_DBT_MALLOC));
    rb_define_const(bdb_mDb, "DBT_PARTIAL", INT2FIX(DB_DBT_PARTIAL));
#if DB_VERSION_MAJOR < 3
    rb_define_const(bdb_mDb, "DBT_REALLOC", INT2FIX(0));
#else
    rb_define_const(bdb_mDb, "DBT_REALLOC", INT2FIX(DB_DBT_REALLOC));
#endif
    rb_define_const(bdb_mDb, "DBT_USERMEM", INT2FIX(DB_DBT_USERMEM));
    rb_define_const(bdb_mDb, "DUP", INT2FIX(DB_DUP));
#ifdef DB_DUPSORT
    rb_define_const(bdb_mDb, "DUPSORT", INT2FIX(DB_DUPSORT));
#endif
    rb_define_const(bdb_mDb, "FIRST", INT2FIX(DB_FIRST));
    rb_define_const(bdb_mDb, "FLUSH", INT2FIX(DB_FLUSH));
#if DB_VERSION_MAJOR < 3
    rb_define_const(bdb_mDb, "FORCE", INT2FIX(1));
#else
    rb_define_const(bdb_mDb, "FORCE", INT2FIX(DB_FORCE));
#endif
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    rb_define_const(bdb_mDb, "GET_BOTH", INT2FIX(9));
#else
    rb_define_const(bdb_mDb, "GET_BOTH", INT2FIX(DB_GET_BOTH));
#endif
    rb_define_const(bdb_mDb, "GET_RECNO", INT2FIX(DB_GET_RECNO));
#ifdef DB_INIT_CDB
    rb_define_const(bdb_mDb, "INIT_CDB", INT2FIX(DB_INIT_CDB));
#endif
    rb_define_const(bdb_mDb, "INIT_LOCK", INT2FIX(DB_INIT_LOCK));
    rb_define_const(bdb_mDb, "INIT_LOG", INT2FIX(DB_INIT_LOG));
    rb_define_const(bdb_mDb, "INIT_MPOOL", INT2FIX(DB_INIT_MPOOL));
    rb_define_const(bdb_mDb, "INIT_TXN", INT2FIX(DB_INIT_TXN));
    rb_define_const(bdb_mDb, "INIT_TRANSACTION", INT2FIX(DB_INIT_LOCK | DB_INIT_MPOOL | DB_INIT_TXN | DB_INIT_LOG));
#ifdef DB_JOIN_ITEM
    rb_define_const(bdb_mDb, "JOIN_ITEM", INT2FIX(DB_JOIN_ITEM));
#endif
    rb_define_const(bdb_mDb, "KEYFIRST", INT2FIX(DB_KEYFIRST));
    rb_define_const(bdb_mDb, "KEYLAST", INT2FIX(DB_KEYLAST));
    rb_define_const(bdb_mDb, "LAST", INT2FIX(DB_LAST));
    rb_define_const(bdb_mDb, "LOCK_CONFLICT", INT2FIX(DB_LOCK_CONFLICT));
    rb_define_const(bdb_mDb, "LOCK_DEADLOCK", INT2FIX(DB_LOCK_DEADLOCK));
    rb_define_const(bdb_mDb, "LOCK_DEFAULT", INT2FIX(DB_LOCK_DEFAULT));
    rb_define_const(bdb_mDb, "LOCK_GET", INT2FIX(DB_LOCK_GET));
    rb_define_const(bdb_mDb, "LOCK_NOTGRANTED", INT2FIX(DB_LOCK_NOTGRANTED));
    rb_define_const(bdb_mDb, "LOCK_NOWAIT", INT2FIX(DB_LOCK_NOWAIT));
    rb_define_const(bdb_mDb, "LOCK_OLDEST", INT2FIX(DB_LOCK_OLDEST));
    rb_define_const(bdb_mDb, "LOCK_PUT", INT2FIX(DB_LOCK_PUT));
    rb_define_const(bdb_mDb, "LOCK_PUT_ALL", INT2FIX(DB_LOCK_PUT_ALL));
    rb_define_const(bdb_mDb, "LOCK_PUT_OBJ", INT2FIX(DB_LOCK_PUT_OBJ));
    rb_define_const(bdb_mDb, "LOCK_RANDOM", INT2FIX(DB_LOCK_RANDOM));
    rb_define_const(bdb_mDb, "LOCK_YOUNGEST", INT2FIX(DB_LOCK_YOUNGEST));
    rb_define_const(bdb_mDb, "LOCK_NG", INT2FIX(DB_LOCK_NG));
    rb_define_const(bdb_mDb, "LOCK_READ", INT2FIX(DB_LOCK_READ));
    rb_define_const(bdb_mDb, "LOCK_WRITE", INT2FIX(DB_LOCK_WRITE));
    rb_define_const(bdb_mDb, "LOCK_IWRITE", INT2FIX(DB_LOCK_IWRITE));
    rb_define_const(bdb_mDb, "LOCK_IREAD", INT2FIX(DB_LOCK_IREAD));
    rb_define_const(bdb_mDb, "LOCK_IWR", INT2FIX(DB_LOCK_IWR));
    rb_define_const(bdb_mDb, "MPOOL_CREATE", INT2FIX(DB_MPOOL_CREATE));
    rb_define_const(bdb_mDb, "MPOOL_LAST", INT2FIX(DB_MPOOL_LAST));
    rb_define_const(bdb_mDb, "MPOOL_NEW", INT2FIX(DB_MPOOL_NEW));
    rb_define_const(bdb_mDb, "NEXT", INT2FIX(DB_NEXT));
#if DB_NEXT_DUP
    rb_define_const(bdb_mDb, "NEXT_DUP", INT2FIX(DB_NEXT_DUP));
#endif
    rb_define_const(bdb_mDb, "NOMMAP", INT2FIX(DB_NOMMAP));
    rb_define_const(bdb_mDb, "NOOVERWRITE", INT2FIX(DB_NOOVERWRITE));
    rb_define_const(bdb_mDb, "NOSYNC", INT2FIX(DB_NOSYNC));
#if DB_VERSION_MAJOR < 3
    rb_define_const(bdb_mDb, "POSITION", INT2FIX(0));
#else
    rb_define_const(bdb_mDb, "POSITION", INT2FIX(DB_POSITION));
#endif
    rb_define_const(bdb_mDb, "PREV", INT2FIX(DB_PREV));
#if DB_VERSION_MAJOR < 3
    rb_define_const(bdb_mDb, "PRIVATE", INT2FIX(0));
#else
    rb_define_const(bdb_mDb, "PRIVATE", INT2FIX(DB_PRIVATE));
#endif
    rb_define_const(bdb_mDb, "RDONLY", INT2FIX(DB_RDONLY));
    rb_define_const(bdb_mDb, "RECNUM", INT2FIX(DB_RECNUM));
    rb_define_const(bdb_mDb, "RECORDCOUNT", INT2FIX(DB_RECORDCOUNT));
    rb_define_const(bdb_mDb, "RECOVER", INT2FIX(DB_RECOVER));
    rb_define_const(bdb_mDb, "RECOVER_FATAL", INT2FIX(DB_RECOVER_FATAL));
    rb_define_const(bdb_mDb, "RENUMBER", INT2FIX(DB_RENUMBER));
    rb_define_const(bdb_mDb, "RMW", INT2FIX(DB_RMW));
    rb_define_const(bdb_mDb, "SET", INT2FIX(DB_SET));
    rb_define_const(bdb_mDb, "SET_RANGE", INT2FIX(DB_SET_RANGE));
    rb_define_const(bdb_mDb, "SET_RECNO", INT2FIX(DB_SET_RECNO));
    rb_define_const(bdb_mDb, "SNAPSHOT", INT2FIX(DB_SNAPSHOT));
#if DB_VERSION_MAJOR < 3
    rb_define_const(bdb_mDb, "SYSTEM_MEM", INT2FIX(0));
#else
    rb_define_const(bdb_mDb, "SYSTEM_MEM", INT2FIX(DB_SYSTEM_MEM));
#endif
    rb_define_const(bdb_mDb, "THREAD", INT2FIX(DB_THREAD));
    rb_define_const(bdb_mDb, "TRUNCATE", INT2FIX(DB_TRUNCATE));
    rb_define_const(bdb_mDb, "TXN_NOSYNC", INT2FIX(DB_TXN_NOSYNC));
    rb_define_const(bdb_mDb, "USE_ENVIRON", INT2FIX(DB_USE_ENVIRON));
    rb_define_const(bdb_mDb, "USE_ENVIRON_ROOT", INT2FIX(DB_USE_ENVIRON_ROOT));
#if DB_VERSION_MAJOR < 3
    rb_define_const(bdb_mDb, "TXN_NOWAIT", INT2FIX(0));
    rb_define_const(bdb_mDb, "TXN_SYNC", INT2FIX(0));
    rb_define_const(bdb_mDb, "WRITECURSOR", INT2FIX(0));
#else
    rb_define_const(bdb_mDb, "TXN_NOWAIT", INT2FIX(DB_TXN_NOWAIT));
    rb_define_const(bdb_mDb, "TXN_SYNC", INT2FIX(DB_TXN_SYNC));
    rb_define_const(bdb_mDb, "WRITECURSOR", INT2FIX(DB_WRITECURSOR));
#endif
    rb_define_const(bdb_mDb, "TXN_COMMIT", INT2FIX(BDB_TXN_COMMIT));
/* ENV */
    bdb_cEnv = rb_define_class_under(bdb_mDb, "Env", rb_cObject);
    rb_define_private_method(bdb_cEnv, "initialize", bdb_obj_init, -1);
    rb_define_singleton_method(bdb_cEnv, "new", bdb_env_s_new, -1);
    rb_define_singleton_method(bdb_cEnv, "create", bdb_env_s_new, -1);
    rb_define_singleton_method(bdb_cEnv, "open", bdb_env_s_new, -1);
    rb_define_method(bdb_cEnv, "open_db", bdb_env_open_db, -1);
    rb_define_method(bdb_cEnv, "remove", bdb_env_remove, 0);
    rb_define_method(bdb_cEnv, "unlink", bdb_env_remove, 0);
    rb_define_method(bdb_cEnv, "close", bdb_env_close, 0);
    rb_define_method(bdb_cEnv, "begin", bdb_env_begin, -1);
    rb_define_method(bdb_cEnv, "txn_begin", bdb_env_begin, -1);
    rb_define_method(bdb_cEnv, "transaction", bdb_env_begin, -1);
    rb_define_method(bdb_cEnv, "lock_id", bdb_env_lockid, 0);
    rb_define_method(bdb_cEnv, "lock", bdb_env_lockid, 0);
    rb_define_method(bdb_cEnv, "lock_stat", bdb_env_lockstat, 0);
    rb_define_method(bdb_cEnv, "lock_detect", bdb_env_lockdetect, -1);
    rb_define_method(bdb_cEnv, "checkpoint", bdb_env_check, -1);
    rb_define_method(bdb_cEnv, "txn_checkpoint", bdb_env_check, -1);
    rb_define_method(bdb_cEnv, "stat", bdb_env_stat, 0);
    rb_define_method(bdb_cEnv, "txn_stat", bdb_env_stat, 0);
    rb_define_method(bdb_cEnv, "set_flags", bdb_env_set_flags, -1);
/* DATABASE */
    bdb_cCommon = rb_define_class_under(bdb_mDb, "Common", rb_cObject);
    rb_define_private_method(bdb_cCommon, "initialize", bdb_obj_init, -1);
    rb_include_module(bdb_cCommon, rb_mEnumerable);
    rb_define_singleton_method(bdb_cCommon, "new", bdb_s_new, -1);
    rb_define_singleton_method(bdb_cCommon, "create", bdb_s_new, -1);
    rb_define_singleton_method(bdb_cCommon, "open", bdb_s_new, -1);
    rb_define_singleton_method(bdb_cCommon, "remove", bdb_s_remove, -1);
    rb_define_singleton_method(bdb_cCommon, "bdb_remove", bdb_s_remove, -1);
    rb_define_singleton_method(bdb_cCommon, "unlink", bdb_s_remove, -1);
    rb_define_singleton_method(bdb_cCommon, "upgrade", bdb_s_upgrade, -1);
    rb_define_singleton_method(bdb_cCommon, "bdb_upgrade", bdb_s_upgrade, -1);
    rb_define_method(bdb_cCommon, "close", bdb_close, -1);
    rb_define_method(bdb_cCommon, "db_close", bdb_close, -1);
    rb_define_method(bdb_cCommon, "put", bdb_put, -1);
    rb_define_method(bdb_cCommon, "db_put", bdb_put, -1);
    rb_define_method(bdb_cCommon, "[]=", bdb_put, -1);
    rb_define_method(bdb_cCommon, "store", bdb_put, -1);
    rb_define_method(bdb_cCommon, "env", bdb_env, 0);
    rb_define_method(bdb_cCommon, "has_env?", bdb_has_env, 0);
    rb_define_method(bdb_cCommon, "env?", bdb_has_env, 0);
    rb_define_method(bdb_cCommon, "count", bdb_count, 1);
    rb_define_method(bdb_cCommon, "dup_count", bdb_count, 1);
    rb_define_method(bdb_cCommon, "get_dup", bdb_get_dup, 1);
    rb_define_method(bdb_cCommon, "dup", bdb_get_dup, 1);
    rb_define_method(bdb_cCommon, "db_get_dup", bdb_get_dup, 1);
    rb_define_method(bdb_cCommon, "get", bdb_get, -1);
    rb_define_method(bdb_cCommon, "db_get", bdb_get, -1);
    rb_define_method(bdb_cCommon, "[]", bdb_get, -1);
    rb_define_method(bdb_cCommon, "fetch", bdb_get, -1);
    rb_define_method(bdb_cCommon, "delete", bdb_del, 1);
    rb_define_method(bdb_cCommon, "del", bdb_del, 1);
    rb_define_method(bdb_cCommon, "db_del", bdb_del, 1);
    rb_define_method(bdb_cCommon, "sync", bdb_sync, 0);
    rb_define_method(bdb_cCommon, "db_sync", bdb_sync, 0);
    rb_define_method(bdb_cCommon, "flush", bdb_sync, 0);
    rb_define_method(bdb_cCommon, "db_cursor", bdb_cursor, 0);
    rb_define_method(bdb_cCommon, "cursor", bdb_cursor, 0);
    rb_define_method(bdb_cCommon, "each", bdb_each_pair, 0);
    rb_define_method(bdb_cCommon, "each_value", bdb_each_value, 0);
    rb_define_method(bdb_cCommon, "reverse_each_value", bdb_each_eulav, 0);
    rb_define_method(bdb_cCommon, "each_key", bdb_each_key, 0);
    rb_define_method(bdb_cCommon, "reverse_each_key", bdb_each_yek, 0);
    rb_define_method(bdb_cCommon, "each_pair", bdb_each_pair, 0);
    rb_define_method(bdb_cCommon, "reverse_each", bdb_each_riap, 0);
    rb_define_method(bdb_cCommon, "reverse_each_pair", bdb_each_riap, 0);
    rb_define_method(bdb_cCommon, "keys", bdb_keys, 0);
    rb_define_method(bdb_cCommon, "values", bdb_values, 0);
    rb_define_method(bdb_cCommon, "delete_if", bdb_delete_if, 0);
    rb_define_method(bdb_cCommon, "reject!", bdb_delete_if, 0);
    rb_define_method(bdb_cCommon, "reject", bdb_reject, 0);
    rb_define_method(bdb_cCommon, "clear", bdb_clear, 0);
    rb_define_method(bdb_cCommon, "replace", bdb_replace, 1);
    rb_define_method(bdb_cCommon, "update", bdb_update, 1);
    rb_define_method(bdb_cCommon, "include?", bdb_has_key, 1);
    rb_define_method(bdb_cCommon, "has_key?", bdb_has_key, 1);
    rb_define_method(bdb_cCommon, "key?", bdb_has_key, 1);
    rb_define_method(bdb_cCommon, "member?", bdb_has_key, 1);
    rb_define_method(bdb_cCommon, "has_value?", bdb_has_value, 1);
    rb_define_method(bdb_cCommon, "value?", bdb_has_value, 1);
    rb_define_method(bdb_cCommon, "has_both?", bdb_has_both, 2);
    rb_define_method(bdb_cCommon, "both?", bdb_has_both, 2);
    rb_define_method(bdb_cCommon, "to_a", bdb_to_a, 0);
    rb_define_method(bdb_cCommon, "to_hash", bdb_to_hash, 0);
    rb_define_method(bdb_cCommon, "invert", bdb_to_hash, 0);
    rb_define_method(bdb_cCommon, "empty?", bdb_empty, 0);
    rb_define_method(bdb_cCommon, "length", bdb_length, 0);
    rb_define_alias(bdb_cCommon,  "size", "length");
    rb_define_method(bdb_cCommon, "index", bdb_index, 1);
    rb_define_method(bdb_cCommon, "indexes", bdb_indexes, -1);
    rb_define_method(bdb_cCommon, "indices", bdb_indexes, -1);
    rb_define_method(bdb_cCommon, "set_partial", bdb_set_partial, 2);
    rb_define_method(bdb_cCommon, "clear_partial", bdb_clear_partial, 0);
    rb_define_method(bdb_cCommon, "partial_clear", bdb_clear_partial, 0);
#if DB_VERSION_MAJOR > 2 || (DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR >= 6)
    rb_define_method(bdb_cCommon, "join", bdb_join, -1);
    rb_define_method(bdb_cCommon, "byteswapped?", bdb_byteswapp, 0);
    rb_define_method(bdb_cCommon, "get_byteswapped", bdb_byteswapp, 0);
#endif
    bdb_cBtree = rb_define_class_under(bdb_mDb, "Btree", bdb_cCommon);
    rb_define_method(bdb_cBtree, "stat", bdb_tree_stat, 0);
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1
    bdb_sKeyrange = rb_struct_define("Keyrange", "less", "equal", "greater", 0);
    rb_global_variable(&bdb_sKeyrange);
    rb_define_method(bdb_cBtree, "key_range", bdb_btree_key_range, 1);
#endif
    bdb_cHash = rb_define_class_under(bdb_mDb, "Hash", bdb_cCommon);
#if DB_VERSION_MAJOR == 3
    rb_define_method(bdb_cHash, "stat", bdb_hash_stat, 0);
#endif
    bdb_cRecno = rb_define_class_under(bdb_mDb, "Recno", bdb_cCommon);
    rb_define_method(bdb_cRecno, "shift", bdb_shift, 0);
    rb_define_method(bdb_cRecno, "push", bdb_append, -1);
    rb_define_method(bdb_cRecno, "pop", bdb_pop, 0);
    rb_define_method(bdb_cRecno, "stat", bdb_tree_stat, 0);
#if DB_VERSION_MAJOR == 3
    bdb_cQueue = rb_define_class_under(bdb_mDb, "Queue", bdb_cCommon);
    rb_define_singleton_method(bdb_cQueue, "new", bdb_queue_s_new, -1);
    rb_define_singleton_method(bdb_cQueue, "create", bdb_queue_s_new, -1);
    rb_define_singleton_method(bdb_cQueue, "open", bdb_queue_s_new, -1);
    rb_define_method(bdb_cQueue, "push", bdb_append, -1);
    rb_define_method(bdb_cQueue, "shift", bdb_consume, 0);
    rb_define_method(bdb_cQueue, "stat", bdb_queue_stat, 0);
    rb_define_method(bdb_cQueue, "pad", bdb_queue_padlen, 0);
#endif
    bdb_cUnknown = rb_define_class_under(bdb_mDb, "Unknown", bdb_cCommon);
/* TRANSACTION */
    bdb_cTxn = rb_define_class_under(bdb_mDb, "Txn", rb_cObject);
    bdb_cTxnCatch = rb_define_class("DBTxnCatch", bdb_cTxn);
    rb_undef_method(CLASS_OF(bdb_cTxn), "new");
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
    rb_define_method(bdb_cTxn, "prepare", bdb_txn_prepare, 0);
    rb_define_method(bdb_cTxn, "txn_prepare", bdb_txn_prepare, 0);
    rb_define_method(bdb_cTxn, "assoc", bdb_txn_assoc, -1);
    rb_define_method(bdb_cTxn, "txn_assoc", bdb_txn_assoc, -1);
    rb_define_method(bdb_cTxn, "associate", bdb_txn_assoc, -1);
    rb_define_method(bdb_cTxn, "open_db", bdb_env_open_db, -1);
/* CURSOR */
    bdb_cCursor = rb_define_class_under(bdb_mDb, "Cursor", rb_cObject);
    rb_undef_method(CLASS_OF(bdb_cCursor), "new");
    rb_define_method(bdb_cCursor, "close", bdb_cursor_close, 0);
    rb_define_method(bdb_cCursor, "c_close", bdb_cursor_close, 0);
    rb_define_method(bdb_cCursor, "c_del", bdb_cursor_del, 0);
    rb_define_method(bdb_cCursor, "del", bdb_cursor_del, 0);
    rb_define_method(bdb_cCursor, "delete", bdb_cursor_del, 0);
#if DB_VERSION_MAJOR == 3
    rb_define_method(bdb_cCursor, "dup", bdb_cursor_dup, -1);
    rb_define_method(bdb_cCursor, "c_dup", bdb_cursor_dup, -1);
    rb_define_method(bdb_cCursor, "clone", bdb_cursor_dup, -1);
    rb_define_method(bdb_cCursor, "c_clone", bdb_cursor_dup, -1);
#endif
    rb_define_method(bdb_cCursor, "count", bdb_cursor_count, 0);
    rb_define_method(bdb_cCursor, "c_count", bdb_cursor_count, 0);
    rb_define_method(bdb_cCursor, "get", bdb_cursor_get, -1);
    rb_define_method(bdb_cCursor, "c_get", bdb_cursor_get, -1);
    rb_define_method(bdb_cCursor, "put", bdb_cursor_put, -1);
    rb_define_method(bdb_cCursor, "c_put", bdb_cursor_put, -1);
    rb_define_method(bdb_cCursor, "c_next", bdb_cursor_next, 0);
    rb_define_method(bdb_cCursor, "next", bdb_cursor_next, 0);
    rb_define_method(bdb_cCursor, "c_first", bdb_cursor_first, 0);
    rb_define_method(bdb_cCursor, "first", bdb_cursor_first, 0);
    rb_define_method(bdb_cCursor, "c_last", bdb_cursor_last, 0);
    rb_define_method(bdb_cCursor, "last", bdb_cursor_last, 0);
    rb_define_method(bdb_cCursor, "c_current", bdb_cursor_current, 0);
    rb_define_method(bdb_cCursor, "current", bdb_cursor_current, 0);
    rb_define_method(bdb_cCursor, "c_prev", bdb_cursor_prev, 0);
    rb_define_method(bdb_cCursor, "prev", bdb_cursor_prev, 0);
    rb_define_method(bdb_cCursor, "c_set", bdb_cursor_set, 1);
    rb_define_method(bdb_cCursor, "set", bdb_cursor_set, 1);
    rb_define_method(bdb_cCursor, "c_set_range", bdb_cursor_set_range, 1);
    rb_define_method(bdb_cCursor, "set_range", bdb_cursor_set_range, 1);
    rb_define_method(bdb_cCursor, "c_set_recno", bdb_cursor_set_recno, 1);
    rb_define_method(bdb_cCursor, "set_recno", bdb_cursor_set_recno, 1);
/* LOCK */
    bdb_cLockid = rb_define_class_under(bdb_mDb, "Lockid", rb_cObject);
    rb_undef_method(CLASS_OF(bdb_cLockid), "new");
    rb_define_method(bdb_cLockid, "lock_get", bdb_lockid_get, -1);
    rb_define_method(bdb_cLockid, "get", bdb_lockid_get, -1);
    rb_define_method(bdb_cLockid, "lock_vec", bdb_lockid_vec, -1);
    rb_define_method(bdb_cLockid, "vec", bdb_lockid_vec, -1);
    bdb_cLock = rb_define_class_under(bdb_cLockid, "Lock", rb_cObject);
    rb_undef_method(CLASS_OF(bdb_cLock), "new");
    rb_define_method(bdb_cLock, "put", bdb_lock_put, 0);
    rb_define_method(bdb_cLock, "lock_put", bdb_lock_put, 0);
    rb_define_method(bdb_cLock, "release", bdb_lock_put, 0);
    rb_define_method(bdb_cLock, "delete", bdb_lock_put, 0);
    bdb_errstr = rb_tainted_str_new(0, 0);
    rb_global_variable(&bdb_errstr);
}
