#include "bdb.h"

static ID id_bt_compare, id_bt_prefix, id_dup_compare, id_h_hash, id_proc_call;

VALUE
bdb_test_load(dbst, a)
    bdb_DB *dbst;
    DBT a;
{
    VALUE res;
    int i;

    if (dbst->marshal) {
	res = rb_funcall(dbst->marshal, id_load, 1, rb_str_new(a.data, a.size));
    }
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

static VALUE 
test_load_dyna(obj, key, val)
    VALUE obj;
    DBT key, val;
{
    bdb_DB *dbst;
    VALUE del, res, tmp;
    struct deleg_class *delegst;
    
    Data_Get_Struct(obj, bdb_DB, dbst);
    res = bdb_test_load(dbst, val);
    if (dbst->marshal && !SPECIAL_CONST_P(res)) {
	del = Data_Make_Struct(bdb_cDelegate, struct deleg_class, 
			       bdb_deleg_mark, bdb_deleg_free, delegst);
	delegst->db = obj;
	if (RECNUM_TYPE(dbst)) {
	    tmp = INT2NUM((*(db_recno_t *)key.data) - dbst->array_base);
	}
	else {
	    tmp = rb_str_new(key.data, key.size);
	}
	delegst->key = rb_funcall(dbst->marshal, id_load, 1, tmp);
	delegst->obj = res;
	res = del;
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
    av = bdb_test_load(dbst, *a);
    bv = bdb_test_load(dbst, *b);
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
    av = bdb_test_load(dbst, *a);
    bv = bdb_test_load(dbst, *b);
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
    av = bdb_test_load(dbst, *a);
    bv = bdb_test_load(dbst, *b);
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
        bdb_test_error(dbp->set_bt_minkey(dbp, NUM2INT(value)));
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
	bdb_test_error(dbp->set_bt_compare(dbp, bdb_bt_compare));
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
	bdb_test_error(dbp->set_bt_prefix(dbp, bdb_bt_prefix));
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
	bdb_test_error(dbp->set_dup_compare(dbp, bdb_dup_compare));
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
	bdb_test_error(dbp->set_h_hash(dbp, bdb_h_hash));
#endif
    }
    else if (strcmp(options, "set_cachesize") == 0) {
        Check_Type(value, T_ARRAY);
        if (RARRAY(value)->len < 3)
            rb_raise(bdb_eFatal, "expected 3 values for cachesize");
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->db_cachesize = NUM2INT(RARRAY(value)->ptr[1]);
#else
        bdb_test_error(dbp->set_cachesize(dbp, 
                                      NUM2INT(RARRAY(value)->ptr[0]),
                                      NUM2INT(RARRAY(value)->ptr[1]),
                                      NUM2INT(RARRAY(value)->ptr[2])));
#endif
    }
    else if (strcmp(options, "set_flags") == 0) {
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->flags = NUM2UINT(value);
#else
        bdb_test_error(dbp->set_flags(dbp, NUM2UINT(value)));
#endif
	dbst->flags |= NUM2UINT(value);
    }
    else if (strcmp(options, "set_h_ffactor") == 0) {
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->h_ffactor = NUM2INT(value);
#else
        bdb_test_error(dbp->set_h_ffactor(dbp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_h_nelem") == 0) {
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->h_nelem = NUM2INT(value);
#else
        bdb_test_error(dbp->set_h_nelem(dbp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_lorder") == 0) {
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->db_lorder = NUM2INT(value);
#else
        bdb_test_error(dbp->set_lorder(dbp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_pagesize") == 0) {
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->db_pagesize = NUM2INT(value);
#else
        bdb_test_error(dbp->set_pagesize(dbp, NUM2INT(value)));
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
        bdb_test_error(dbp->set_re_delim(dbp, ch));
#endif
    }
    else if (strcmp(options, "set_re_len") == 0) {
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->re_len = NUM2INT(value);
	dbst->dbinfo->flags |= DB_FIXEDLEN;
#else
        bdb_test_error(dbp->set_re_len(dbp, NUM2INT(value)));
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
        bdb_test_error(dbp->set_re_pad(dbp, ch));
#endif
    }
    else if (strcmp(options, "set_re_source") == 0) {
        if (TYPE(value) != T_STRING)
            rb_raise(bdb_eFatal, "re_source must be a filename");
#if DB_VERSION_MAJOR < 3
	dbst->dbinfo->re_source = RSTRING(value)->ptr;
#else
        bdb_test_error(dbp->set_re_source(dbp, RSTRING(value)->ptr));
#endif
	dbst->options |= BDB_RE_SOURCE;
    }
    else if (strcmp(options, "marshal") == 0) {
        switch (value) {
        case Qtrue: dbst->marshal = bdb_mMarshal; break;
        case Qfalse: dbst->marshal = Qfalse; break;
        default: 
	    if (!rb_respond_to(value, id_load) ||
		!rb_respond_to(value, id_dump)) {
		rb_raise(bdb_eFatal, "marshal value must be true or false");
	    }
	    dbst->marshal = value;
	    break;
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

void
bdb_free(dbst)
    bdb_DB *dbst;
{
    if (dbst->dbp && dbst->flags27 != -1 && !(dbst->options & BDB_TXN)) {
        bdb_test_error(dbst->dbp->close(dbst->dbp, 0));
        dbst->dbp = NULL;
    }
    free(dbst);
}

void
bdb_mark(dbst)
    bdb_DB *dbst;
{
    if (dbst->marshal) rb_gc_mark(dbst->marshal);
    if (dbst->env) rb_gc_mark(dbst->env);
    if (dbst->orig) rb_gc_mark(dbst->orig);
    if (dbst->secondary) rb_gc_mark(dbst->secondary);
    if (dbst->bt_compare) rb_gc_mark(dbst->bt_compare);
    if (dbst->bt_prefix) rb_gc_mark(dbst->bt_prefix);
    if (dbst->dup_compare) rb_gc_mark(dbst->dup_compare);
    if (dbst->h_hash) rb_gc_mark(dbst->h_hash);
}

static VALUE
bdb_env(obj)
    VALUE obj;
{
    bdb_DB *dbst;

    GetDB(obj, dbst);
    return dbst->env;
}

VALUE
bdb_has_env(obj)
    VALUE obj;
{
    bdb_DB *dbst;

    GetDB(obj, dbst);
    return (dbst->env == 0)?Qfalse:Qtrue;
}

VALUE
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
	bdb_test_error(dbst->dbp->close(dbst->dbp, flags));
	dbst->dbp = NULL;
    }
    return Qnil;
}

#if !(DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1)
static long
bdb_hard_count(dbp)
    DB *dbp;
{
    DBC *dbcp;
    DBT key, data;
    db_recno_t recno;
    long count = 0;
    int ret;

    memset(&key, 0, sizeof(key));
    key.data = &recno;
    key.size = sizeof(db_recno_t);
    memset(&data, 0, sizeof(data));
    data.flags = DB_DBT_MALLOC;
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    key.flags |= DB_DBT_MALLOC;
    bdb_test_error(dbp->cursor(dbp, 0, &dbcp));
#else
    bdb_test_error(dbp->cursor(dbp, 0, &dbcp, 0));
#endif
    do {
        ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, DB_NEXT));
        if (ret == DB_NOTFOUND) {
            bdb_test_error(dbcp->c_close(dbcp));
            return count;
        }
	if (ret == DB_KEYEMPTY) return -1;
	free(data.data);
        count++;
    } while (1);
    return count;
}
#endif

static long
bdb_is_recnum(dbp)
    DB *dbp;
{
    DB_BTREE_STAT *stat;
    long count, hard;

#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1
#if DB_VERSION_MINOR < 3
    bdb_test_error(dbp->stat(dbp, &stat, 0, 0));
#else
    bdb_test_error(dbp->stat(dbp, &stat, 0));
#endif
    count = (stat->bt_nkeys == stat->bt_ndata)?stat->bt_nkeys:-1;
    free(stat);
#else
    bdb_test_error(dbp->stat(dbp, &stat, 0, DB_RECORDCOUNT));
    count = stat->bt_nrecs;
    free(stat);
    hard = bdb_hard_count(dbp);
    count = (count == hard)?count:-1;
#endif
    return count;
}

static VALUE bdb_length __((VALUE));

static VALUE
bdb_recno_length(obj)
    VALUE obj;
{
    bdb_DB *dbst;
    DB_BTREE_STAT *stat;
    VALUE hash;

    GetDB(obj, dbst);
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &stat, DB_RECORDCOUNT));
#else
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &stat, 0, DB_RECORDCOUNT));
#endif
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1
    hash = INT2NUM(stat->bt_nkeys);
#else
    hash = INT2NUM(stat->bt_nrecs);
#endif
    free(stat);
    return hash;
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
    res = Data_Make_Struct(obj, bdb_DB, bdb_mark, bdb_free, dbst);
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
	    dbst->marshal = txnst->marshal;
	}
	else if ((v = rb_hash_aref(f, rb_str_new2("env"))) != RHASH(f)->ifnone) {
	    if (!rb_obj_is_kind_of(v, bdb_cEnv)) {
		rb_raise(bdb_eFatal, "argument of env must be an environnement");
	    }
	    Data_Get_Struct(v, bdb_ENV, dbenvst);
	    dbst->env = v;
	    dbenvp = dbenvst->dbenvp;
	    flags27 = dbenvst->flags27;
	    dbst->marshal = dbenvst->marshal;
	}
    }
#if DB_VERSION_MAJOR == 3
    bdb_test_error(db_create(&dbp, dbenvp, 0));
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
	    bdb_test_error(dbp->set_bt_compare(dbp, bdb_bt_compare));
#endif
    }
    if (dbst->bt_prefix == 0 && rb_method_boundp(obj, id_bt_prefix, 0) == Qtrue) {
	    dbst->options |= BDB_BT_PREFIX;
#if DB_VERSION_MAJOR < 3
	    dbst->dbinfo->bt_prefix = bdb_bt_prefix;
#else
	    bdb_test_error(dbp->set_bt_prefix(dbp, bdb_bt_prefix));
#endif
    }
#ifdef DB_DUPSORT
    if (dbst->dup_compare == 0 && rb_method_boundp(obj, id_dup_compare, 0) == Qtrue) {
	    dbst->options |= BDB_DUP_COMPARE;
#if DB_VERSION_MAJOR < 3
	    dbst->dbinfo->dup_compare = bdb_dup_compare;
#else
	    bdb_test_error(dbp->set_dup_compare(dbp, bdb_dup_compare));
#endif
    }
#endif
    if (dbst->h_hash == 0 && rb_method_boundp(obj, id_h_hash, 0) == Qtrue) {
	    dbst->options |= BDB_H_HASH;
#if DB_VERSION_MAJOR < 3
	    dbst->dbinfo->h_hash = bdb_h_hash;
#else
	    bdb_test_error(dbp->set_h_hash(dbp, bdb_h_hash));
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
        bdb_test_error(dbp->set_flags(dbp, DB_DUP | DB_DUPSORT));
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
        if (bdb_errcall) {
            bdb_errcall = 0;
            rb_raise(bdb_eFatal, "%s -- %s", RSTRING(bdb_errstr)->ptr, db_strerror(ret));
        }
        else
            rb_raise(bdb_eFatal, "%s", db_strerror(ret));
    }
#else
    if ((ret = dbp->open(dbp, name, subname, NUM2INT(c), flags, mode)) != 0) {
        dbp->close(dbp, 0);
        if (bdb_errcall) {
            bdb_errcall = 0;
            rb_raise(bdb_eFatal, "%s -- %s", RSTRING(bdb_errstr)->ptr, db_strerror(ret));
        }
        else
            rb_raise(bdb_eFatal, "%s", db_strerror(ret));
    }
#endif
    if (NUM2INT(c) == DB_UNKNOWN) {
	VALUE res1;
	bdb_DB *dbst1;
	int type;

#if DB_VERSION_MAJOR < 3
	type = dbp->type;
#else
#if DB_VERSION_MINOR >= 3
	{
	    DBTYPE new_type;
	    bdb_test_error(dbp->get_type(dbp, &new_type));
	    type = (int)new_type;
	}
#else
	type = dbp->get_type(dbp);
#endif
#endif
	switch(type) {
	case DB_BTREE:
	    res1 = Data_Make_Struct(bdb_cBtree, bdb_DB, bdb_mark, bdb_free, dbst1);
	    break;
	case DB_HASH:
	    res1 = Data_Make_Struct(bdb_cHash, bdb_DB, bdb_mark, bdb_free, dbst1);
	    break;
	case DB_RECNO:
	{
	    long count;

	    rb_warning("It's hard to distinguish Recnum with Recno for all versions of Berkeley DB");
	    if ((count = bdb_is_recnum(dbp)) != -1) {
		VALUE len;
		res1 = Data_Make_Struct(bdb_cRecnum, bdb_DB, bdb_mark, bdb_free, dbst1);
		dbst->len = count;
	    }
	    else {
		res1 = Data_Make_Struct(bdb_cRecno, bdb_DB, bdb_mark, bdb_free, dbst1);
	    }
	    break;
	}
#if  DB_VERSION_MAJOR == 3
	case DB_QUEUE:
	    res1 = Data_Make_Struct(bdb_cQueue, bdb_DB, bdb_mark, bdb_free, dbst1);
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
	dbst->len = -2;
	if (rb_obj_is_kind_of(res, bdb_cRecnum)) {
	    long count;

	    if ((count = bdb_is_recnum(dbp)) != -1) {
		VALUE len = bdb_recno_length(res);
		dbst->len = count;
	    }
	    else {
		if (flags & DB_TRUNCATE) {
		    dbst->len = 0;
		}
		else {
		    rb_raise(bdb_eFatal, "database is not a Recnum");
		}
	    }
	}
	return res;
    }
}

VALUE
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
	if (cl == bdb_cRecnum || RCLASS(cl)->m_tbl == RCLASS(bdb_cRecnum)->m_tbl) {
	    bb = INT2NUM(DB_RECNO); 
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
#if DB_VERSION_MINOR < 3
    ret->type = ret->dbp->get_type(ret->dbp);
#else
    bdb_test_error(ret->dbp->get_type(ret->dbp, &ret->type));
#endif
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
    VALUE obj, restobj;
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
bdb_append_internal(argc, argv, obj, flag)
    int argc, flag;
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
	bdb_test_error(dbst->dbp->put(dbst->dbp, txnid, &key, &data, flag));
    }
    return obj;
}

static VALUE
bdb_append_m(argc, argv, obj)
    int argc;
    VALUE *argv, obj;
{
    return bdb_append_internal(argc, argv, obj, DB_APPEND);
}

static VALUE
bdb_append(obj, val)
    VALUE val, obj;
{
    return bdb_append_m(1, &val, obj);
}

static VALUE
bdb_unshift(argc, argv, obj)
    int argc;
    VALUE *argv, obj;
{
    bdb_DB *dbst;
    DB_TXN *txnid;
    int flag;

    init_txn(txnid, obj, dbst);
    if (dbst->flags & DB_RENUMBER) {
	flag = 0;
    }
    else {
	flag = DB_NOOVERWRITE;
    }
    return bdb_append_internal(argc, argv, obj, flag);
}

VALUE
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
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    if (rb_scan_args(argc, argv, "21", &a, &b, &c) == 3) {
        flags = NUM2INT(c);
    }
    test_recno(dbst, key, recno, a);
    test_dump(dbst, data, b);
    set_partial(dbst, data);
#if DB_VERSION_MAJOR == 3
    if (dbst->type == DB_QUEUE && dbst->re_len < data.size) {
	rb_raise(bdb_eFatal, "size > re_len for Queue");
    }
#endif
    ret = bdb_test_error(dbst->dbp->put(dbst->dbp, txnid, &key, &data, flags));
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
    if (RECNUM_TYPE(dbst))
        return INT2NUM((*(db_recno_t *)key.data) - dbst->array_base);
    return bdb_test_load(dbst, key);
}

VALUE
bdb_assoc(dbst, recno, key, data)
    bdb_DB *dbst;
    int recno;
    DBT key, data;
{
    return rb_assoc_new(test_load_key(dbst, key), bdb_test_load(dbst, data));
}

static VALUE
bdb_assoc2(dbst, recno, skey, pkey, data)
    bdb_DB *dbst;
    int recno;
    DBT skey, pkey, data;
{
    return rb_assoc_new(
	rb_assoc_new(test_load_key(dbst, skey), test_load_key(dbst, pkey)),
	bdb_test_load(dbst, data));
}

static VALUE bdb_has_both(VALUE, VALUE, VALUE);
static VALUE bdb_has_both_internal(VALUE, VALUE, VALUE, VALUE);

static VALUE
bdb_get_internal(argc, argv, obj, notfound, dyna)
    int argc;
    VALUE *argv;
    VALUE obj;
    VALUE notfound;
    int dyna;
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
    ret = bdb_test_error(dbst->dbp->get(dbst->dbp, txnid, &key, &data, flags));
    if (ret == DB_NOTFOUND || ret == DB_KEYEMPTY)
        return notfound;
    if (((flags & ~DB_RMW) == DB_GET_BOTH) ||
	((flags & ~DB_RMW) == DB_SET_RECNO)) {
        return bdb_assoc(dbst, recno, key, data);
    }
    if (dyna) {
	return test_load_dyna(obj, key, data);
    }
    else {
	return bdb_test_load(dbst, data);
    }
}

VALUE
bdb_get(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    return bdb_get_internal(argc, argv, obj, Qnil, 0);
}

static VALUE
bdb_get_dyna(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    return bdb_get_internal(argc, argv, obj, Qnil, 1);
}

#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3

static VALUE
bdb_pget(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    VALUE a, b, c, d;
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBT pkey, data, skey;
    DBT datas;
    int flagss;
    int ret, flags;
    db_recno_t srecno, precno;

    init_txn(txnid, obj, dbst);
    flagss = flags = 0;
    memset(&pkey, 0, sizeof(DBT));
    memset(&skey, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
    data.flags |= DB_DBT_MALLOC;
    pkey.flags |= DB_DBT_MALLOC;
    switch(rb_scan_args(argc, argv, "12", &a, &b, &c)) {
    case 3:
        flags = NUM2INT(c);
        if ((flags & ~DB_RMW) == DB_GET_BOTH) {
            test_dump(dbst, data, b);
        }
	break;
    case 2:
	flagss = flags = NUM2INT(b);
	break;
    }
    test_recno(dbst, skey, srecno, a);
    set_partial(dbst, data);
    flags |= test_init_lock(dbst);
    ret = bdb_test_error(dbst->dbp->pget(dbst->dbp, txnid, &skey, &pkey, &data, flags));
    if (ret == DB_NOTFOUND || ret == DB_KEYEMPTY)
        return Qnil;
    if (((flags & ~DB_RMW) == DB_GET_BOTH) ||
	((flags & ~DB_RMW) == DB_SET_RECNO)) {
        return bdb_assoc2(dbst, srecno, skey, pkey, data);
    }
    return bdb_assoc(dbst, srecno, pkey, data);
}

#endif

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
    bdb_test_error(dbst->dbp->key_range(dbst->dbp, txnid, &key, &key_range, flags));
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
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
    set_partial(dbst, data);
    flags27 = test_init_lock(dbst);
    ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, DB_SET | flags27));
    if (ret == DB_NOTFOUND) {
        bdb_test_error(dbcp->c_close(dbcp));
        return INT2NUM(0);
    }
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1
    bdb_test_error(dbcp->c_count(dbcp, &count, 0));
    bdb_test_error(dbcp->c_close(dbcp));
    return INT2NUM(count);
#else
    count = 1;
    while (1) {
	ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, DB_NEXT_DUP | flags27));
	if (ret == DB_NOTFOUND) {
	    bdb_test_error(dbcp->c_close(dbcp));
	    return INT2NUM(count);
	}
	if (ret == DB_KEYEMPTY) continue;
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
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
    flags27 = test_init_lock(dbst);
    result = rb_ary_new2(0);
    ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, DB_SET | flags27));
    if (ret == DB_NOTFOUND) {
        bdb_test_error(dbcp->c_close(dbcp));
	return result;
    }
    rb_ary_push(result, bdb_test_load(dbst, data));
    while (1) {
	ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, DB_NEXT_DUP | flags27));
	if (ret == DB_NOTFOUND) {
	    bdb_test_error(dbcp->c_close(dbcp));
	    return result;
	}
	if (ret == DB_KEYEMPTY) continue;
	free_key(dbst, key);
	rb_ary_push(result, bdb_test_load(dbst, data));
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
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
    ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, DB_CONSUME));
    bdb_test_error(dbcp->c_close(dbcp));
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
    return bdb_get_internal(1, &key, obj, Qfalse, 0);
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
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, DB_SET));
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
    if (RECNUM_TYPE(dbst)) {
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
	ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, DB_NEXT_DUP));
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
    if (RECNUM_TYPE(dbst)) {
	return bdb_has_both_internal(obj, a, b, Qfalse);
    }
#endif
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
    data.flags |= DB_DBT_MALLOC;
    test_dump(dbst, data, b);
    test_recno(dbst, key, recno, a);
    set_partial(dbst, data);
    key.flags |= DB_DBT_MALLOC;
    flags = DB_GET_BOTH | test_init_lock(dbst);
    ret = bdb_test_error(dbst->dbp->get(dbst->dbp, txnid, &key, &data, flags));
    if (ret == DB_NOTFOUND || ret == DB_KEYEMPTY)
        return Qfalse;
    free(data.data);
/*
    if (!RECNUM_TYPE(dbst)) {
	free(key.data);
    }
*/
    return Qtrue;
#endif
}

VALUE
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
    ret = bdb_test_error(dbst->dbp->del(dbst->dbp, txnid, &key, 0));
    if (ret == DB_NOTFOUND || ret == DB_KEYEMPTY)
        return Qnil;
    else
        return obj;
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
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    set_partial(dbst, data);
    flags = test_init_lock(dbst);
    ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, DB_FIRST | flags));
    if (ret == DB_NOTFOUND) {
        bdb_test_error(dbcp->c_close(dbcp));
        return Qtrue;
    }
    free_key(dbst, key);
    free(data.data);
    bdb_test_error(dbcp->c_close(dbcp));
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
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    set_partial(dbst, data);
    flags = test_init_lock(dbst);
    do {
        ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, DB_NEXT | flags));
        if (ret == DB_NOTFOUND) {
            bdb_test_error(dbcp->c_close(dbcp));
            return obj;
        }
	if (ret == DB_KEYEMPTY) continue;
        if (RTEST(rb_yield(bdb_assoc(dbst, recno, key, data)))) {
            bdb_test_error(dbcp->c_del(dbcp, 0));
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
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    set_partial(dbst, data);
    flags = test_init_lock(dbst);
    do {
        ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, DB_NEXT | flags));
        if (ret == DB_NOTFOUND) {
            bdb_test_error(dbcp->c_close(dbcp));
            return INT2NUM(value);
        }
	if (ret == DB_KEYEMPTY) continue;
	free_key(dbst, key);
	free(data.data);
        value++;
    } while (1);
    return INT2NUM(value);
}

typedef struct {
    int sens;
    VALUE replace;
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
    VALUE res;
    
    dbst = st->dbst;
    dbcp = st->dbcp;
    memset(&key, 0, sizeof(key));
    init_recno(dbst, key, recno);
    memset(&data, 0, sizeof(data));
    data.flags = DB_DBT_MALLOC;
    set_partial(dbst, data);
    do {
        ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, st->sens));
        if (ret == DB_NOTFOUND) {
            return Qnil;
        }
	if (ret == DB_KEYEMPTY) continue;
	free_key(dbst, key);
	res = rb_yield(bdb_test_load(dbst, data));
	if (st->replace == Qtrue) {
	    memset(&data, 0, sizeof(data));
	    test_dump(dbst, data, res);
	    set_partial(dbst, data);
	    bdb_test_error(dbcp->c_put(dbcp, &key, &data, DB_CURRENT));
	}
	else if (st->replace != Qfalse) {
	    rb_ary_push(st->replace, res);
	}
    } while (1);
}

VALUE
bdb_each_valuec(obj, sens, replace)
    VALUE obj;
    int sens;
    VALUE replace;
{
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBC *dbcp;
    eachst st;

    init_txn(txnid, obj, dbst);
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    st.dbst = dbst;
    st.dbcp = dbcp;
    st.sens = sens | test_init_lock(dbst);
    st.replace = replace;
    rb_ensure(bdb_i_each_value, (VALUE)&st, bdb_each_ensure, (VALUE)&st);
    if (replace == Qtrue || replace == Qfalse) {
	return obj;
    }
    else {
	return replace;
    }
}

VALUE
bdb_each_value(obj)
    VALUE obj;
{ 
    return bdb_each_valuec(obj, DB_NEXT, Qfalse);
}

VALUE
bdb_each_eulav(obj)
    VALUE obj;
{
    return bdb_each_valuec(obj, DB_PREV, Qfalse);
}

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
        ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, st->sens));
        if (ret == DB_NOTFOUND) {
            return Qnil;
        }
	if (ret == DB_KEYEMPTY) continue;
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
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    st.dbst = dbst;
    st.dbcp = dbcp;
    st.sens = sens | test_init_lock(dbst);
    rb_ensure(bdb_i_each_key, (VALUE)&st, bdb_each_ensure, (VALUE)&st); 
    return obj;
}

VALUE bdb_each_key(obj) VALUE obj;{ return bdb_each_keyc(obj, DB_NEXT); }
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
        ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, st->sens));
        if (ret == DB_NOTFOUND) {
            return Qnil;
        }
	if (ret == DB_KEYEMPTY) continue;
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
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    st.dbst = dbst;
    st.dbcp = dbcp;
    st.sens = sens | test_init_lock(dbst);
    rb_ensure(bdb_i_each_common, (VALUE)&st, bdb_each_ensure, (VALUE)&st); 
    return obj;
}

static VALUE bdb_each_pair(obj) VALUE obj;{ return bdb_each_common(obj, DB_NEXT); }
static VALUE bdb_each_riap(obj) VALUE obj;{ return bdb_each_common(obj, DB_PREV); }

VALUE
bdb_to_a_intern(obj, flag)
    VALUE obj;
    int flag;
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
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    set_partial(dbst, data);
    flags = ((flag==-1)?DB_PREV:DB_NEXT) | test_init_lock(dbst);
    do {
        ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, flags));
        if (ret == DB_NOTFOUND) {
            bdb_test_error(dbcp->c_close(dbcp));
            return ary;
        }
	if (ret == DB_KEYEMPTY) continue;
	if (flag > 0) {
	    rb_ary_push(ary, bdb_assoc(dbst, recno, key, data));
	}
	else {
	    rb_ary_push(ary, bdb_test_load(dbst, data));
	}
    } while (1);
    return ary;
}

static VALUE
bdb_to_a(obj)
    VALUE obj;
{
    return bdb_to_a_intern(obj, 1);
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
    bdb_put(2, RARRAY(pair)->ptr, obj);
    return Qnil;
}

static VALUE
bdb_update(obj, other)
    VALUE obj, other;
{
    rb_iterate(each_pair, other, bdb_update_i, obj);
    return obj;
}

VALUE
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
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    set_partial(dbst, data);
    flags = DB_NEXT | test_init_lock(dbst);
    do {
        ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, flags));
        if (ret == DB_NOTFOUND) {
            bdb_test_error(dbcp->c_close(dbcp));
            return obj;
        }
	if (ret == DB_KEYEMPTY) continue;
	bdb_test_error(dbcp->c_del(dbcp, 0));
    } while (1);
    return obj;
}

static VALUE
bdb_replace(obj, other)
    VALUE obj, other;
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
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    set_partial(dbst, data);
    flags = DB_NEXT | test_init_lock(dbst);
    do {
        ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, flags));
        if (ret == DB_NOTFOUND) {
            bdb_test_error(dbcp->c_close(dbcp));
            return hash;
        }
	if (ret == DB_KEYEMPTY) continue;
	if (tes == Qtrue) {
	    rb_hash_aset(hash, test_load_key(dbst, key), bdb_test_load(dbst, data));
	}
	else {
	    rb_hash_aset(hash, bdb_test_load(dbst, data), test_load_key(dbst, key));
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
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    set_partial(dbst, data);
    flags = DB_NEXT | test_init_lock(dbst);
    do {
        ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, flags));
        if (ret == DB_NOTFOUND) {
            bdb_test_error(dbcp->c_close(dbcp));
            return ary;
        }
	if (ret == DB_KEYEMPTY) continue;
	free_key(dbst, key);
        rb_ary_push(ary, bdb_test_load(dbst, data));
    } while (1);
    return ary;
}

VALUE
bdb_internal_value(obj, a, b, sens)
    VALUE obj, a, b;
    int sens;
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
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    set_partial(dbst, data);
    flags = sens | test_init_lock(dbst);
    do {
        ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, flags));
        if (ret == DB_NOTFOUND) {
            bdb_test_error(dbcp->c_close(dbcp));
            return (b == Qfalse)?Qfalse:Qnil;
        }
	if (ret == DB_KEYEMPTY) continue;
        if (rb_equal(a, bdb_test_load(dbst, data)) == Qtrue) {
	    VALUE d;

            bdb_test_error(dbcp->c_close(dbcp));
	    d = (b == Qfalse)?Qtrue:test_load_key(dbst, key);
	    free_key(dbst, key);
	    return  d;
        }
	free_key(dbst, key);
    } while (1);
    return (b == Qfalse)?Qfalse:Qnil;
}

VALUE
bdb_index(obj, a)
    VALUE obj, a;
{
    return bdb_internal_value(obj, a, Qtrue, DB_NEXT);
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

VALUE
bdb_has_value(obj, a)
    VALUE obj, a;
{
    return bdb_internal_value(obj, a, Qfalse, DB_NEXT);
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
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    set_partial(dbst, data);
    flags = DB_NEXT | test_init_lock(dbst);
    do {
        ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, flags));
        if (ret == DB_NOTFOUND) {
            bdb_test_error(dbcp->c_close(dbcp));
            return ary;
        }
	if (ret == DB_KEYEMPTY) continue;
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
    bdb_test_error(dbst->dbp->sync(dbst->dbp, 0));
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
#if DB_VERSION_MINOR < 3
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &stat, 0, 0));
#else
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &stat, 0));
#endif
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

VALUE
bdb_tree_stat(obj)
    VALUE obj;
{
    bdb_DB *dbst;
    DB_BTREE_STAT *stat;
    VALUE hash;
    char pad;

    GetDB(obj, dbst);
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &stat, 0));
#else
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &stat, 0, 0));
#endif
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
#if DB_VERSION_MINOR < 3
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &stat, 0, 0));
#else
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &stat, 0));
#endif
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
#if DB_VERSION_MINOR < 3
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &stat, 0, 0));
#else
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &stat, 0));
#endif
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
    if (dbst->marshal) {
	rb_raise(bdb_eFatal, "set_partial is not implemented with Marshal");
    }
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
    if (dbst->marshal) {
	rb_raise(bdb_eFatal, "set_partial is not implemented with Marshal");
    }
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

    dbenvp = NULL;
    env = 0;
    if (rb_obj_is_kind_of(obj, bdb_cEnv)) {
        GetEnvDB(obj, dbenvst);
        dbenvp = dbenvst->dbenvp;
        env = obj;
    }
    bdb_test_error(db_create(&dbp, dbenvp, 0));
    dbp->set_errpfx(dbp, "BDB::");
    dbp->set_errcall(dbp, bdb_env_errcall);
    ret = Data_Make_Struct(bdb_cCommon, bdb_DB, bdb_mark, bdb_free, dbst);
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
    bdb_test_error(dbst->dbp->upgrade(dbst->dbp, RSTRING(a)->ptr, flags));
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
    bdb_test_error(dbst->dbp->remove(dbst->dbp, name, subname, 0));
#endif
    return Qtrue;
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
	ret = bdb_test_error(st->dbcp->c_get(st->dbcp, &key, &data, st->sens));
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
    bdb_test_error(dbst->dbp->join(dbst->dbp, dbcarr, 0, &dbc));
#else
    bdb_test_error(dbst->dbp->join(dbst->dbp, dbcarr, &dbc, 0));
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
    int byteswap = 0;
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 5
    rb_raise(bdb_eFatal, "byteswapped needs Berkeley DB 2.5 or later");
#endif
    GetDB(obj, dbst);
#if DB_VERSION_MAJOR < 3
    return dbst->dbp->byteswapped?Qtrue:Qfalse;
#else
#if DB_VERSION_MINOR < 3
    return dbst->dbp->get_byteswapped(dbst->dbp)?Qtrue:Qfalse;
#else
    dbst->dbp->get_byteswapped(dbst->dbp, &byteswap);
    return byteswap?Qtrue:Qfalse;
#endif
#endif
}

#endif

#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3

static ID id_primary_db;
static VALUE bdb_global_second;

static VALUE
bdb_internal_second_call(tmp)
    VALUE *tmp;
{
    return rb_funcall2(tmp[0], id_proc_call, 3, tmp + 1);
}

static int
bdb_call_secondary(secst, pkey, pdata, skey)
    DB *secst;
    const DBT *pkey;
    const DBT *pdata;
    DBT *skey;
{
    VALUE obj, ary, second;
    bdb_DB *dbst, *secondst;
    VALUE key, value, result;
    int i, inter;

    if ((obj = rb_thread_local_aref(rb_thread_current(), id_primary_db)) == Qnil) {
	if (rb_thread_current() != rb_thread_main() && 
	    bdb_global_second != Qnil) {
	    obj = bdb_global_second;
	    
	}
	else {
	    rb_gv_set("$!", rb_str_new2("primary index not found"));
	    return BDB_ERROR_PRIVATE;
	}
    }
 retry:
    Data_Get_Struct(obj, bdb_DB, dbst);
    if (!dbst->dbp || !dbst->secondary) return DB_DONOTINDEX;
    for (i = 0; i < RARRAY(dbst->secondary)->len; i++) {
	ary = RARRAY(dbst->secondary)->ptr[i];
	if (RARRAY(ary)->len != 2) continue;
	second = RARRAY(ary)->ptr[0];
	Data_Get_Struct(second, bdb_DB, secondst);
	if (!secondst->dbp) continue;
	if (secondst->dbp == secst) {
	    VALUE tmp[4];

	    tmp[0] = RARRAY(ary)->ptr[1];
	    tmp[1] = second;
	    tmp[2] = test_load_key(dbst, *pkey);
	    tmp[3] = bdb_test_load(dbst, *pdata);
	    inter = 0;
	    result = rb_protect(bdb_internal_second_call, (VALUE)tmp, &inter);
	    if (inter) return BDB_ERROR_PRIVATE;
	    if (result == Qfalse) return DB_DONOTINDEX;
	    memset(skey, 0, sizeof(DBT));
	    if (result == Qtrue) {
		skey->data = pkey->data;
		skey->size = pkey->size;
	    }
	    else {
		DBT stmp;
		memset(&stmp, 0, sizeof(DBT));
		test_dump(secondst, stmp, result);
		skey->data = stmp.data;
		skey->size = stmp.size;
	    }
	    return 0;
	}
    }
    if (obj != bdb_global_second && bdb_global_second != Qnil &&
	rb_thread_current() != rb_thread_main()) {
	obj = bdb_global_second;
	goto retry;
    }
    rb_gv_set("$!", rb_str_new2("secondary index not found ?"));
    return BDB_ERROR_PRIVATE;
}

static VALUE
bdb_associate(argc, argv, obj)
    VALUE obj, *argv;
    int argc;
{
    bdb_DB *dbst, *secondst;
    VALUE second, flag;
    int flags = 0;

    if (!rb_block_given_p()) {
	rb_raise(bdb_eFatal, "call out of an iterator");
    }
    if (rb_scan_args(argc, argv, "11", &second, &flag) == 2) {
	flags = NUM2INT(flag);
    }
    if (!rb_obj_is_kind_of(second, bdb_cCommon)) {
	rb_raise(bdb_eFatal, "associate expect a BDB object");
    }
    GetDB(second, secondst);
    GetDB(obj, dbst);
    bdb_test_error(dbst->dbp->associate(dbst->dbp, secondst->dbp, 
					bdb_call_secondary, flags));
    if (rb_thread_current() == rb_thread_main()) {
	if (bdb_global_second != Qnil && bdb_global_second != obj) {
	    rb_warning("re-defining a secondary index for another BDB object");
	}
	bdb_global_second = obj;
    }
    if (!dbst->secondary) {
	dbst->secondary = rb_ary_new();
    }
    rb_ary_push(dbst->secondary, rb_assoc_new(second, rb_f_lambda()));
    rb_thread_local_aset(rb_thread_current(), id_primary_db, obj);
    return obj;
}

#endif

void bdb_init_common()
{
    id_bt_compare = rb_intern("bdb_bt_compare");
    id_bt_prefix = rb_intern("bdb_bt_prefix");
    id_dup_compare = rb_intern("bdb_dup_compare");
    id_h_hash = rb_intern("bdb_h_hash");
    id_proc_call = rb_intern("call");
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3
    id_primary_db = rb_intern("bdb_primary_db");
    bdb_global_second = Qnil;
    rb_global_variable(&bdb_global_second);
#endif
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
    rb_define_method(bdb_cCommon, "get", bdb_get_dyna, -1);
    rb_define_method(bdb_cCommon, "db_get", bdb_get_dyna, -1);
    rb_define_method(bdb_cCommon, "[]", bdb_get_dyna, -1);
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3
    rb_define_method(bdb_cCommon, "pget", bdb_pget, -1);
    rb_define_method(bdb_cCommon, "db_pget", bdb_pget, -1);
#endif
    rb_define_method(bdb_cCommon, "fetch", bdb_get_dyna, -1);
    rb_define_method(bdb_cCommon, "delete", bdb_del, 1);
    rb_define_method(bdb_cCommon, "del", bdb_del, 1);
    rb_define_method(bdb_cCommon, "db_del", bdb_del, 1);
    rb_define_method(bdb_cCommon, "sync", bdb_sync, 0);
    rb_define_method(bdb_cCommon, "db_sync", bdb_sync, 0);
    rb_define_method(bdb_cCommon, "flush", bdb_sync, 0);
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
    rb_define_method(bdb_cCommon, "invert", bdb_invert, 0);
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
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3
    rb_define_method(bdb_cCommon, "associate", bdb_associate, -1);
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
    rb_define_method(bdb_cRecno, "each_index", bdb_each_key, 0);
    rb_define_method(bdb_cRecno, "unshift", bdb_unshift, -1);
    rb_define_method(bdb_cRecno, "<<", bdb_append, 1);
    rb_define_method(bdb_cRecno, "push", bdb_append_m, -1);
    rb_define_method(bdb_cRecno, "stat", bdb_tree_stat, 0);
#if DB_VERSION_MAJOR == 3
    bdb_cQueue = rb_define_class_under(bdb_mDb, "Queue", bdb_cCommon);
    rb_define_singleton_method(bdb_cQueue, "new", bdb_queue_s_new, -1);
    rb_define_singleton_method(bdb_cQueue, "create", bdb_queue_s_new, -1);
    rb_define_singleton_method(bdb_cQueue, "open", bdb_queue_s_new, -1);
    rb_define_method(bdb_cQueue, "each_index", bdb_each_key, 0);
    rb_define_method(bdb_cQueue, "<<", bdb_append, 1);
    rb_define_method(bdb_cQueue, "push", bdb_append_m, -1);
    rb_define_method(bdb_cQueue, "shift", bdb_consume, 0);
    rb_define_method(bdb_cQueue, "stat", bdb_queue_stat, 0);
    rb_define_method(bdb_cQueue, "pad", bdb_queue_padlen, 0);
#endif
    bdb_cUnknown = rb_define_class_under(bdb_mDb, "Unknown", bdb_cCommon);
}
