#include "bdb.h"

static void lockid_mark(dblockid)
    bdb_LOCKID *dblockid;
{
    rb_gc_mark(dblockid->env);
}

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
    if (!dbenvst->dbenvp->lk_info) {
	rb_raise(bdb_eLock, "lock region not open");
    }
    bdb_test_error(lock_id(dbenvst->dbenvp->lk_info, &idp));
#else
    bdb_test_error(lock_id(dbenvst->dbenvp, &idp));
#endif
    a = Data_Make_Struct(bdb_cLockid, bdb_LOCKID, lockid_mark, free, dblockid);
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
    bdb_ENV *dbenvst;
    int flags, atype, aborted;

    flags = atype = aborted = 0;
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	flags = NUM2INT(b);
    }
    atype = NUM2INT(a);
    GetEnvDB(obj, dbenvst);
#if DB_VERSION_MAJOR < 3
    if (!dbenvst->dbenvp->lk_info) {
	rb_raise(bdb_eLock, "lock region not open");
    }
    bdb_test_error(lock_detect(dbenvst->dbenvp->lk_info, flags, atype));
#else
    bdb_test_error(lock_detect(dbenvst->dbenvp, flags, atype, &aborted));
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
    if (!dbenvst->dbenvp->lk_info) {
	rb_raise(bdb_eLock, "lock region not open");
    }
    bdb_test_error(lock_stat(dbenvst->dbenvp->lk_info, &statp, 0));
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
#if DB_VERSION_MINOR < 3
    bdb_test_error(lock_stat(dbenvst->dbenvp, &statp, 0));
#else
    bdb_test_error(lock_stat(dbenvst->dbenvp, &statp));
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
    free(statp);
    return a;
}

#if DB_VERSION_MAJOR < 3
#define GetLockid(obj, lockid, dbenvst)		\
{						\
    Data_Get_Struct(obj, bdb_LOCKID, lockid);	\
    GetEnvDB(lockid->env, dbenvst);		\
    if (dbenvst->dbenvp->lk_info == 0) {	\
        rb_raise(bdb_eLock, "closed lockid");	\
    }						\
}
#else
#define GetLockid(obj, lockid, dbenvst)		\
{						\
    Data_Get_Struct(obj, bdb_LOCKID, lockid);	\
    GetEnvDB(lockid->env, dbenvst);		\
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
#if DB_VERSION_MAJOR < 3
    bdb_ENV *dbenvst;

    GetEnvDB(lock->env, dbenvst);
    if (dbenvst->dbenvp->lk_info) {
	lock_close(dbenvst->dbenvp->lk_info);
	dbenvst->dbenvp->lk_info = NULL;
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
    bdb_ENV *dbenvst;
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
    GetLockid(obj, lockid, dbenvst);
#if DB_VERSION_MAJOR < 3
    if (!dbenvst->dbenvp->lk_info) {
	rb_raise(bdb_eLock, "lock region not open");
    }
    bdb_test_error(lock_get(dbenvst->dbenvp->lk_info, lockid->lock, flags,
			&objet, lock_mode, &lock));
#else
    bdb_test_error(lock_get(dbenvst->dbenvp, lockid->lock, flags,
			&objet, lock_mode, &lock));
#endif
    res = Data_Make_Struct(bdb_cLock, bdb_LOCK, lock_mark, lock_free, lockst);
#if DB_VERSION_MAJOR < 3
    lockst->lock = lock;
#else
    lockst->lock = ALLOC(DB_LOCK);
    MEMCPY(lockst->lock, &lock, DB_LOCK, 1);
#endif
    lockst->env = lockid->env;
    return res;
} 

#if DB_VERSION_MAJOR < 3
#define GetLock(obj, lock, dbenvst)		\
{						\
    Data_Get_Struct(obj, bdb_LOCK, lock);	\
    GetEnvDB(lock->env, dbenvst);		\
    if (dbenvst->dbenvp->lk_info == 0)		\
        rb_raise(bdb_eLock, "closed lock");	\
}
#else
#define GetLock(obj, lock, dbenvst)		\
{						\
    Data_Get_Struct(obj, bdb_LOCK, lock);	\
    GetEnvDB(lock->env, dbenvst);		\
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
    bdb_ENV *dbenvst;
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
	GetLock(value, lockst, dbenvst);
#if DB_VERSION_MAJOR < 3
	list->lock = lockst->lock;
#else
	MEMCPY(&list->lock, lockst->lock, DB_LOCK, 1);
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
    bdb_ENV *dbenvst;
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
    GetLockid(obj, lockid, dbenvst);
#if DB_VERSION_MAJOR < 3
    if (!dbenvst->dbenvp->lk_info) {
	rb_raise(bdb_eLock, "lock region not open");
    }
    err = lock_vec(dbenvst->dbenvp->lk_info, lockid->lock, flags,
		   list, RARRAY(a)->len, NULL);
#else
    err = lock_vec(dbenvst->dbenvp, lockid->lock, flags,
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
    res = rb_ary_new2(RARRAY(a)->len);
    n = 0;
    for (i = 0; i < RARRAY(a)->len; i++) {
	if (list[i].op == DB_LOCK_GET) {
	    c = Data_Make_Struct(bdb_cLock, bdb_LOCK, lock_mark, lock_free, lockst);
#if DB_VERSION_MAJOR < 3
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
    bdb_ENV *dbenvst;

    GetLock(obj, lockst, dbenvst);
#if DB_VERSION_MAJOR < 3
    if (!dbenvst->dbenvp->lk_info) {
	rb_raise(bdb_eLock, "lock region not open");
    }
    bdb_test_error(lock_put(dbenvst->dbenvp->lk_info, lockst->lock));
#else
    bdb_test_error(lock_put(dbenvst->dbenvp, lockst->lock));
#endif
    return Qnil;
} 

void bdb_init_lock()
{
    rb_define_method(bdb_cEnv, "lock_id", bdb_env_lockid, 0);
    rb_define_method(bdb_cEnv, "lock", bdb_env_lockid, 0);
    rb_define_method(bdb_cEnv, "lock_stat", bdb_env_lockstat, 0);
    rb_define_method(bdb_cEnv, "lock_detect", bdb_env_lockdetect, -1);
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
}
