#include "bdb.h"

static void lockid_mark(dblockid)
    bdb_LOCKID *dblockid;
{
    rb_gc_mark(dblockid->env);
}

static void lockid_free(dblockid)
    bdb_LOCKID *dblockid;
{
#if BDB_VERSION >= 40000
    bdb_ENV *envst;

    GetEnvDB(dblockid->env, envst);
    bdb_test_error(envst->envp->lock_id_free(envst->envp, dblockid->lock));
#endif
}

static VALUE
bdb_env_lockid(obj)
    VALUE obj;
{
    unsigned int idp;
    bdb_ENV *envst;
    bdb_LOCKID *dblockid;
    VALUE a;

    GetEnvDB(obj, envst);
#if BDB_VERSION < 30000
    if (!envst->envp->lk_info) {
	rb_raise(bdb_eLock, "lock region not open");
    }
    bdb_test_error(lock_id(envst->envp->lk_info, &idp));
#else
#if BDB_VERSION >= 40000
    bdb_test_error(envst->envp->lock_id(envst->envp, &idp));
#else
    bdb_test_error(lock_id(envst->envp, &idp));
#endif
#endif
    a = Data_Make_Struct(bdb_cLockid, bdb_LOCKID, lockid_mark, lockid_free, dblockid);
    dblockid->lock = idp;
    dblockid->env = obj;
    return a;
}

static VALUE
bdb_env_lockdetect(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    VALUE a, b;
    bdb_ENV *envst;
    int flags, atype, aborted;

    flags = atype = aborted = 0;
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	flags = NUM2INT(b);
    }
    atype = NUM2INT(a);
    GetEnvDB(obj, envst);
#if BDB_VERSION < 30000
    if (!envst->envp->lk_info) {
	rb_raise(bdb_eLock, "lock region not open");
    }
    bdb_test_error(lock_detect(envst->envp->lk_info, flags, atype));
#else
#if BDB_VERSION >= 40000
    bdb_test_error(envst->envp->lock_detect(envst->envp, flags, atype, &aborted));
#else
    bdb_test_error(lock_detect(envst->envp, flags, atype, &aborted));
#endif
#endif
    return INT2NUM(aborted);
}

static VALUE
bdb_env_lockstat(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    bdb_ENV *envst;
    DB_LOCK_STAT *statp;
    VALUE a, b;
    int flags;

    GetEnvDB(obj, envst);
#if BDB_VERSION < 30000
    if (!envst->envp->lk_info) {
	rb_raise(bdb_eLock, "lock region not open");
    }
    if (argc != 0) {
	rb_raise(rb_eArgError, "invalid number of arguments (%d for 0)", argc);
    }
    bdb_test_error(lock_stat(envst->envp->lk_info, &statp, 0));
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
#if BDB_VERSION >= 40000
    flags = 0;
    if (rb_scan_args(argc, argv, "01", &b) == 1) {
	flags = NUM2INT(b);
    }
    bdb_test_error(envst->envp->lock_stat(envst->envp, &statp, flags));
    a = rb_hash_new();
#if BDB_VERSION >= 40100
    rb_hash_aset(a, rb_tainted_str_new2("st_lastid"), INT2NUM(statp->st_id));
#else
    rb_hash_aset(a, rb_tainted_str_new2("st_lastid"), INT2NUM(statp->st_lastid));
#endif
    rb_hash_aset(a, rb_tainted_str_new2("st_nmodes"), INT2NUM(statp->st_nmodes));
    rb_hash_aset(a, rb_tainted_str_new2("st_maxlocks"), INT2NUM(statp->st_maxlocks));
    rb_hash_aset(a, rb_tainted_str_new2("st_maxlockers"), INT2NUM(statp->st_maxlockers));
    rb_hash_aset(a, rb_tainted_str_new2("st_maxobjects"), INT2NUM(statp->st_maxobjects));
    rb_hash_aset(a, rb_tainted_str_new2("st_nlocks"), INT2NUM(statp->st_nlocks));
    rb_hash_aset(a, rb_tainted_str_new2("st_maxnlocks"), INT2NUM(statp->st_maxnlocks));
    rb_hash_aset(a, rb_tainted_str_new2("st_nlockers"), INT2NUM(statp->st_nlockers));
    rb_hash_aset(a, rb_tainted_str_new2("st_maxnlockers"), INT2NUM(statp->st_maxnlockers));
    rb_hash_aset(a, rb_tainted_str_new2("st_nobjects"), INT2NUM(statp->st_nobjects));
    rb_hash_aset(a, rb_tainted_str_new2("st_maxnobjects"), INT2NUM(statp->st_maxnobjects));
    rb_hash_aset(a, rb_tainted_str_new2("st_nrequests"), INT2NUM(statp->st_nrequests));
    rb_hash_aset(a, rb_tainted_str_new2("st_nreleases"), INT2NUM(statp->st_nreleases));
    rb_hash_aset(a, rb_tainted_str_new2("st_nnowaits"), INT2NUM(statp->st_nnowaits));
    rb_hash_aset(a, rb_tainted_str_new2("st_nconflicts"), INT2NUM(statp->st_nconflicts));
    rb_hash_aset(a, rb_tainted_str_new2("st_ndeadlocks"), INT2NUM(statp->st_ndeadlocks));
    rb_hash_aset(a, rb_tainted_str_new2("st_nlocktimeouts"), INT2NUM(statp->st_nlocktimeouts));
    rb_hash_aset(a, rb_tainted_str_new2("st_ntxntimeouts"), INT2NUM(statp->st_ntxntimeouts));
    rb_hash_aset(a, rb_tainted_str_new2("st_regsize"), INT2NUM(statp->st_regsize));
    rb_hash_aset(a, rb_tainted_str_new2("st_region_wait"), INT2NUM(statp->st_region_wait));
    rb_hash_aset(a, rb_tainted_str_new2("st_region_nowait"), INT2NUM(statp->st_region_nowait));
#else
    if (argc != 0) {
	rb_raise(rb_eArgError, "invalid number of arguments (%d for 0)", argc);
    }
#if BDB_VERSION < 30300
    bdb_test_error(lock_stat(envst->envp, &statp, 0));
#else
    bdb_test_error(lock_stat(envst->envp, &statp));
#endif
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
#endif
    free(statp);
    return a;
}

#if BDB_VERSION < 30000
#define GetLockid(obj, lockid, envst)		\
{						\
    Data_Get_Struct(obj, bdb_LOCKID, lockid);	\
    GetEnvDB(lockid->env, envst);		\
    if (envst->envp->lk_info == 0) {	\
        rb_raise(bdb_eLock, "closed lockid");	\
    }						\
}
#else
#define GetLockid(obj, lockid, envst)		\
{						\
    Data_Get_Struct(obj, bdb_LOCKID, lockid);	\
    GetEnvDB(lockid->env, envst);		\
}
#endif

static void
lock_mark(lock)
    bdb_LOCK *lock;
{
    rb_gc_mark(lock->env);
}

static void
lock_free(lock)
    bdb_LOCK *lock;
{
#if BDB_VERSION < 30000
    bdb_ENV *envst;

    GetEnvDB(lock->env, envst);
    if (envst->envp->lk_info) {
	lock_close(envst->envp->lk_info);
	envst->envp->lk_info = NULL;
    }
#endif
    free(lock);
}

static VALUE
bdb_lockid_get(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    bdb_LOCKID *lockid;
    bdb_ENV *envst;
    DB_LOCK lock;
    bdb_LOCK *lockst;
    DBT objet;
    unsigned int flags;
    int lock_mode;
    VALUE a, b, c, res;

    rb_secure(2);
    flags = 0;
    if (rb_scan_args(argc, argv, "21", &a, &b, &c) == 3) {
	if (c == Qtrue) {
	    flags = DB_LOCK_NOWAIT;
	}
	else {
	    flags = NUM2UINT(c);
	}
    }
    Check_SafeStr(a);
    MEMZERO(&objet, DBT, 1);
    objet.data = RSTRING(a)->ptr;
    objet.size = RSTRING(a)->len;
    lock_mode = NUM2INT(b);
    GetLockid(obj, lockid, envst);
#if BDB_VERSION < 30000
    if (!envst->envp->lk_info) {
	rb_raise(bdb_eLock, "lock region not open");
    }
    bdb_test_error(lock_get(envst->envp->lk_info, lockid->lock, flags,
			    &objet, lock_mode, &lock));
#else
#if BDB_VERSION >= 40000

    bdb_test_error(envst->envp->lock_get(envst->envp, lockid->lock,
					 flags, &objet, lock_mode, &lock));
#else
    bdb_test_error(lock_get(envst->envp, lockid->lock, flags,
			    &objet, lock_mode, &lock));
#endif
#endif
    res = Data_Make_Struct(bdb_cLock, bdb_LOCK, lock_mark, lock_free, lockst);
#if BDB_VERSION < 30000
    lockst->lock = lock;
#else
    lockst->lock = ALLOC(DB_LOCK);
    MEMCPY(lockst->lock, &lock, DB_LOCK, 1);
#endif
    lockst->env = lockid->env;
    return res;
} 

#if BDB_VERSION < 30000
#define GetLock(obj, lock, envst)		\
{						\
    Data_Get_Struct(obj, bdb_LOCK, lock);	\
    GetEnvDB(lock->env, envst);			\
    if (envst->envp->lk_info == 0)		\
        rb_raise(bdb_eLock, "closed lock");	\
}
#else
#define GetLock(obj, lock, envst)		\
{						\
    Data_Get_Struct(obj, bdb_LOCK, lock);	\
    GetEnvDB(lock->env, envst);			\
}
#endif

struct lockreq {
    DB_LOCKREQ *list;
};

static VALUE
bdb_lockid_each(obj, listobj)
    VALUE obj, listobj;
{
    VALUE key, value;
    DB_LOCKREQ *list;
    bdb_ENV *envst;
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
	GetLock(value, lockst, envst);
#if BDB_VERSION < 30000
	list->lock = lockst->lock;
#else
	MEMCPY(&list->lock, lockst->lock, DB_LOCK, 1);
#endif
    }
#if BDB_VERSION >= 40000
    else if (strcmp(options, "timeout") == 0) {
	list->timeout = rb_Integer(value);
    }
#endif
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
    bdb_ENV *envst;
    unsigned int flags;
    VALUE a, b, c, res;
    int i, n, err;
    VALUE listobj;
    struct lockreq *listst;

    flags = 0;
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	if (b == Qtrue) {
	    flags = DB_LOCK_NOWAIT;
	}
	else {
	    flags = NUM2UINT(b);
	}
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
    GetLockid(obj, lockid, envst);
#if BDB_VERSION < 30000
    if (!envst->envp->lk_info) {
	rb_raise(bdb_eLock, "lock region not open");
    }
    err = lock_vec(envst->envp->lk_info, lockid->lock, flags,
		   list, RARRAY(a)->len, NULL);
#else
#if BDB_VERSION >= 40000
    err = envst->envp->lock_vec(envst->envp, lockid->lock, flags,
				list, RARRAY(a)->len, NULL);
#else    
    err = lock_vec(envst->envp, lockid->lock, flags,
		   list, RARRAY(a)->len, NULL);
#endif
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
    res = rb_ary_new2(RARRAY(a)->len);
    n = 0;
    for (i = 0; i < RARRAY(a)->len; i++) {
	if (list[i].op == DB_LOCK_GET) {
	    c = Data_Make_Struct(bdb_cLock, bdb_LOCK, lock_mark, lock_free, lockst);
#if BDB_VERSION < 30000
	    lockst->lock = list[i].lock;
#else
	    lockst->lock = ALLOC(DB_LOCK);
	    MEMCPY(lockst->lock, &list[i].lock, DB_LOCK, 1);
#endif
	    lockst->env = lockid->env;
	    rb_ary_push(res, c);
	    n++;
	}
	else {
	    rb_ary_push(res, Qnil);
	}
    }
    return res;
}

static VALUE
bdb_lock_put(obj)
    VALUE obj;
{
    bdb_LOCK *lockst;
    bdb_ENV *envst;

    GetLock(obj, lockst, envst);
#if BDB_VERSION < 30000
    if (!envst->envp->lk_info) {
	rb_raise(bdb_eLock, "lock region not open");
    }
    bdb_test_error(lock_put(envst->envp->lk_info, lockst->lock));
#else
#if BDB_VERSION >= 40000
    bdb_test_error(envst->envp->lock_put(envst->envp, lockst->lock));
#else
    bdb_test_error(lock_put(envst->envp, lockst->lock));
#endif
#endif
    return Qnil;
} 

void bdb_init_lock()
{
    rb_define_method(bdb_cEnv, "lock_id", bdb_env_lockid, 0);
    rb_define_method(bdb_cEnv, "lock", bdb_env_lockid, 0);
    rb_define_method(bdb_cEnv, "lock_stat", bdb_env_lockstat, -1);
    rb_define_method(bdb_cEnv, "lock_detect", bdb_env_lockdetect, -1);
    bdb_cLockid = rb_define_class_under(bdb_mDb, "Lockid", rb_cObject);
    rb_undef_method(CLASS_OF(bdb_cLockid), "allocate");
    rb_undef_method(CLASS_OF(bdb_cLockid), "new");
    rb_define_method(bdb_cLockid, "lock_get", bdb_lockid_get, -1);
    rb_define_method(bdb_cLockid, "get", bdb_lockid_get, -1);
    rb_define_method(bdb_cLockid, "lock_vec", bdb_lockid_vec, -1);
    rb_define_method(bdb_cLockid, "vec", bdb_lockid_vec, -1);
    bdb_cLock = rb_define_class_under(bdb_cLockid, "Lock", rb_cObject);
    rb_undef_method(CLASS_OF(bdb_cLock), "allocate");
    rb_undef_method(CLASS_OF(bdb_cLock), "new");
    rb_define_method(bdb_cLock, "put", bdb_lock_put, 0);
    rb_define_method(bdb_cLock, "lock_put", bdb_lock_put, 0);
    rb_define_method(bdb_cLock, "release", bdb_lock_put, 0);
    rb_define_method(bdb_cLock, "delete", bdb_lock_put, 0);
}
