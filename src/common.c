#include "bdb.h"
#include <rubyio.h>

static ID id_bt_compare, id_bt_prefix, id_dup_compare, id_h_hash;
ID bdb_id_call;

VALUE
bdb_test_load(obj, a, type_kv)
    VALUE obj;
    DBT a;
    int type_kv;
{
    VALUE res;
    int i;
    bdb_DB *dbst;

    Data_Get_Struct(obj, bdb_DB, dbst);
    if (dbst->marshal) {
	res = rb_str_new(a.data, a.size);
	if (dbst->filter[2 + type_kv]) {
	    if (FIXNUM_P(dbst->filter[2 + type_kv])) {
		res = rb_funcall(obj, 
				 NUM2INT(dbst->filter[2 + type_kv]), 1, res);
	    }
	    else {
		res = rb_funcall(dbst->filter[2 + type_kv],
				 bdb_id_call, 1, res);
	    }
	}
	res = rb_funcall(dbst->marshal, id_load, 1, res);
    }
    else {
#if DB_VERSION_MAJOR >= 3
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
	    if (dbst->filter[2 + type_kv]) {
		if (FIXNUM_P(dbst->filter[2 + type_kv])) {
		    res = rb_funcall(obj, 
				     NUM2INT(dbst->filter[2 + type_kv]),
				     1, res);
		}
		else {
		    res = rb_funcall(dbst->filter[2 + type_kv],
				     bdb_id_call, 1, res);
		}
	    }
	}
    }
    if (a.flags & DB_DBT_MALLOC) {
	free(a.data);
    }
    return res;
}

static VALUE 
test_load_dyna1(obj, key, val)
    VALUE obj;
    DBT key, val;
{
    bdb_DB *dbst;
    VALUE del, res, tmp;
    struct deleg_class *delegst;
    
    Data_Get_Struct(obj, bdb_DB, dbst);
    res = bdb_test_load(obj, val, FILTER_VALUE);
    if (dbst->marshal && !SPECIAL_CONST_P(res)) {
	del = Data_Make_Struct(bdb_cDelegate, struct deleg_class, 
			       bdb_deleg_mark, bdb_deleg_free, delegst);
	delegst->db = obj;
	if (RECNUM_TYPE(dbst)) {
	    tmp = INT2NUM((*(db_recno_t *)key.data) - dbst->array_base);
	}
	else {
	    tmp = rb_str_new(key.data, key.size);
	    if (dbst->filter[2 + FILTER_VALUE]) {
		if (FIXNUM_P(dbst->filter[2 + FILTER_VALUE])) {
		    tmp = rb_funcall(obj, 
				     NUM2INT(dbst->filter[2 + FILTER_VALUE]),
				     1, tmp);
		}
		else {
		    tmp = rb_funcall(dbst->filter[2 + FILTER_VALUE],
				     bdb_id_call, 1, tmp);
		}
	    }
	}
	delegst->key = rb_funcall(dbst->marshal, id_load, 1, tmp);
	delegst->obj = res;
	res = del;
    }
    return res;
}

static VALUE
test_load_dyna(obj, key, val)
    VALUE obj;
    DBT key, val;
{
    VALUE res = test_load_dyna1(obj, key, val);
    if (key.flags & DB_DBT_MALLOC) {
	free(key.data);
    }
    return res;
}

static int
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 2) || DB_VERSION_MAJOR >= 4
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
    av = bdb_test_load(obj, *a, FILTER_VALUE);
    bv = bdb_test_load(obj, *b, FILTER_VALUE);
    if (dbst->bt_compare == 0)
	res = rb_funcall(obj, id_bt_compare, 2, av, bv);
    else
	res = rb_funcall(dbst->bt_compare, bdb_id_call, 2, av, bv);
    return NUM2INT(res);
} 

static size_t
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 2) || DB_VERSION_MAJOR >= 4
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
    av = bdb_test_load(obj, *a, FILTER_VALUE);
    bv = bdb_test_load(obj, *b, FILTER_VALUE);
    if (dbst->bt_prefix == 0)
	res = rb_funcall(obj, id_bt_prefix, 2, av, bv);
    else
	res = rb_funcall(dbst->bt_prefix, bdb_id_call, 2, av, bv);
    return NUM2INT(res);
} 

static int
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 2) || DB_VERSION_MAJOR >= 4
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
    av = bdb_test_load(obj, *a, FILTER_VALUE);
    bv = bdb_test_load(obj, *b, FILTER_VALUE);
    if (dbst->dup_compare == 0)
	res = rb_funcall(obj, id_dup_compare, 2, av, bv);
    else
	res = rb_funcall(dbst->dup_compare, bdb_id_call, 2, av, bv);
    return NUM2INT(res);
}

static u_int32_t
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 2) || DB_VERSION_MAJOR >= 4
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
	res = rb_funcall(dbst->h_hash, bdb_id_call, 1, st);
    return NUM2UINT(res);
}

static VALUE
bdb_i_options(obj, dbstobj)
    VALUE obj, dbstobj;
{
    VALUE key, value;
    char *options;
    DB *dbp;
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
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 2) || DB_VERSION_MAJOR >= 4
    else if (strcmp(options, "set_q_extentsize") == 0) {
	bdb_test_error(dbp->set_q_extentsize(dbp, NUM2INT(value)));
    }
#endif
    else if (strcmp(options, "marshal") == 0) {
        switch (value) {
        case Qtrue: 
	    dbst->marshal = bdb_mMarshal; 
	    dbst->options |= BDB_MARSHAL;
	    break;
        case Qfalse: 
	    dbst->marshal = Qfalse; 
	    dbst->options &= ~BDB_MARSHAL;
	    break;
        default: 
	    if (!rb_respond_to(value, id_load) ||
		!rb_respond_to(value, id_dump)) {
		rb_raise(bdb_eFatal, "marshal value must be true or false");
	    }
	    dbst->marshal = value;
	    dbst->options |= BDB_MARSHAL;
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
    else if (strcmp(options, "set_store_key") == 0) {
	if (!rb_obj_is_kind_of(value, rb_cProc)) 
	    rb_raise(bdb_eFatal, "expected a Proc object");
	dbst->filter[FILTER_KEY] = value;
    }
    else if (strcmp(options, "set_fetch_key") == 0) {
	if (!rb_obj_is_kind_of(value, rb_cProc)) 
	    rb_raise(bdb_eFatal, "expected a Proc object");
	dbst->filter[2 + FILTER_KEY] = value;
    }
    else if (strcmp(options, "set_store_value") == 0) {
	if (!rb_obj_is_kind_of(value, rb_cProc)) 
	    rb_raise(bdb_eFatal, "expected a Proc object");
	dbst->filter[FILTER_VALUE] = value;
    }
    else if (strcmp(options, "set_fetch_value") == 0) {
	if (!rb_obj_is_kind_of(value, rb_cProc)) 
	    rb_raise(bdb_eFatal, "expected a Proc object");
	dbst->filter[2 + FILTER_KEY] = value;
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
    int i;
    if (dbst->marshal) rb_gc_mark(dbst->marshal);
    if (dbst->env) rb_gc_mark(dbst->env);
    if (dbst->orig) rb_gc_mark(dbst->orig);
    if (dbst->secondary) rb_gc_mark(dbst->secondary);
    if (dbst->bt_compare) rb_gc_mark(dbst->bt_compare);
    if (dbst->bt_prefix) rb_gc_mark(dbst->bt_prefix);
    if (dbst->dup_compare) rb_gc_mark(dbst->dup_compare);
    for (i = 0; i < 4; i++) {
	if (dbst->filter[i]) rb_gc_mark(dbst->filter[i]);
    }
    if (dbst->h_hash) rb_gc_mark(dbst->h_hash);
    if (dbst->filename) rb_gc_mark(dbst->filename);
    if (dbst->database) rb_gc_mark(dbst->database);
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

#if !((DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1) || DB_VERSION_MAJOR >= 4)
static long
bdb_hard_count(dbp)
    DB *dbp;
{
    DBC *dbcp;
    DBT key, data;
    db_recno_t recno;
    long count = 0;
    int ret;

    MEMZERO(&key, DBT, 1);
    key.data = &recno;
    key.size = sizeof(db_recno_t);
    MEMZERO(&data, DBT, 1);
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
    DB_BTREE_STAT *bdb_stat;
    long count, hard;

#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1) || DB_VERSION_MAJOR >= 4
#if DB_VERSION_MINOR >= 3 || DB_VERSION_MAJOR >= 4
    bdb_test_error(dbp->stat(dbp, &bdb_stat, 0));
#else
    bdb_test_error(dbp->stat(dbp, &bdb_stat, 0, 0));
#endif
    count = (bdb_stat->bt_nkeys == bdb_stat->bt_ndata)?bdb_stat->bt_nkeys:-1;
    free(bdb_stat);
#else
    bdb_test_error(dbp->stat(dbp, &bdb_stat, 0, DB_RECORDCOUNT));
    count = bdb_stat->bt_nrecs;
    free(bdb_stat);
    hard = bdb_hard_count(dbp);
    count = (count == hard)?count:-1;
#endif
    return count;
}

static VALUE
bdb_recno_length(obj)
    VALUE obj;
{
    bdb_DB *dbst;
    DB_BTREE_STAT *bdb_stat;
    VALUE hash;

    GetDB(obj, dbst);
#if DB_VERSION_MAJOR >= 4
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &bdb_stat, DB_FAST_STAT));
#else
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &bdb_stat, DB_RECORDCOUNT));
#else
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &bdb_stat, 0, DB_RECORDCOUNT));
#endif
#endif
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1) || DB_VERSION_MAJOR >= 4
    hash = INT2NUM(bdb_stat->bt_nkeys);
#else
    hash = INT2NUM(bdb_stat->bt_nrecs);
#endif
    free(bdb_stat);
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
		rb_raise(bdb_eFatal, "flags must be r, r+, w, w+, a or a+");
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
	    dbst->no_thread = dbenvst->no_thread;
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
	    dbst->no_thread = dbenvst->no_thread;
	    dbst->marshal = dbenvst->marshal;
	}
    }
#if DB_VERSION_MAJOR >= 3
    bdb_test_error(db_create(&dbp, dbenvp, 0));
    dbp->set_errpfx(dbp, "BDB::");
    dbp->set_errcall(dbp, bdb_env_errcall);
    dbst->dbp = dbp;
    dbst->flags27 = -1;
#else
    dbst->dbinfo = &dbinfo;
#endif
    if (rb_respond_to(obj, id_load) == Qtrue &&
	rb_respond_to(obj, id_dump) == Qtrue) {
	dbst->marshal = obj;
	dbst->options |= BDB_MARSHAL;
    }
    if (rb_method_boundp(obj, rb_intern("bdb_store_key"), 0) == Qtrue) {
	dbst->filter[FILTER_KEY] = INT2FIX(rb_intern("bdb_store_key"));
    }
    if (rb_method_boundp(obj, rb_intern("bdb_fetch_key"), 0) == Qtrue) {
	dbst->filter[2 + FILTER_KEY] = INT2FIX(rb_intern("bdb_fetch_key"));
    }
    if (rb_method_boundp(obj, rb_intern("bdb_store_value"), 0) == Qtrue) {
	dbst->filter[FILTER_VALUE] = INT2FIX(rb_intern("bdb_store_value"));
    }
    if (rb_method_boundp(obj, rb_intern("bdb_fetch_value"), 0) == Qtrue) {
	dbst->filter[2 + FILTER_VALUE] = INT2FIX(rb_intern("bdb_fetch_value"));
    }
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
    if (!dbst->no_thread && !(dbst->options & BDB_RE_SOURCE)) {
	flags |= DB_THREAD;
    }
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
#if DB_VERSION_MINOR >= 3 || DB_VERSION_MAJOR >= 4
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
#if  DB_VERSION_MAJOR >= 3
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
	dbst1->filename = dbst1->database = Qnil;
	if (name) {
	    dbst1->filename = rb_tainted_str_new2(name);
	    OBJ_FREEZE(dbst1->filename);
	}
#if DB_VERSION_MAJOR >= 3
	if (subname) {
	    dbst1->database = rb_tainted_str_new2(subname);
	    OBJ_FREEZE(dbst1->database);
	}
#endif
	return res1;
    }
    else {
	dbst->dbp = dbp;
	dbst->flags27 = flags27;
	dbst->filename = dbst->database = Qnil;
	if (name) {
	    dbst->filename = rb_tainted_str_new2(name);
	    OBJ_FREEZE(dbst->filename);
	}
#if DB_VERSION_MAJOR >= 3
	if (subname) {
	    dbst->database = rb_tainted_str_new2(subname);
	    OBJ_FREEZE(dbst->database);
	}
#endif
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
#if DB_VERSION_MAJOR >= 3
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
#if DB_VERSION_MINOR >= 3 || DB_VERSION_MAJOR >= 4
    bdb_test_error(ret->dbp->get_type(ret->dbp, &ret->type));
#else
    ret->type = ret->dbp->get_type(ret->dbp);
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

#if DB_VERSION_MAJOR >= 3

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
    MEMZERO(&key, DBT, 1);
    recno = 1;
    key.data = &recno;
    key.size = sizeof(db_recno_t);
    for (i = 0, a = argv; i < argc; i++, a++) {
	MEMZERO(&data, DBT, 1);
	test_dump(obj, data, *a, FILTER_VALUE);
	set_partial(dbst, data);
#if DB_VERSION_MAJOR >= 3
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
    MEMZERO(&key, DBT, 1);
    MEMZERO(&data, DBT, 1);
    if (rb_scan_args(argc, argv, "21", &a, &b, &c) == 3) {
        flags = NUM2INT(c);
    }
    test_recno(obj, key, recno, a);
    test_dump(obj, data, b, FILTER_VALUE);
    set_partial(dbst, data);
#if DB_VERSION_MAJOR >= 3
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
test_load_key(obj, key)
    VALUE obj;
    DBT key;
{
    bdb_DB *dbst;
    Data_Get_Struct(obj, bdb_DB, dbst);
    if (RECNUM_TYPE(dbst))
        return INT2NUM((*(db_recno_t *)key.data) - dbst->array_base);
    return bdb_test_load(obj, key, FILTER_KEY);
}

VALUE
bdb_assoc(obj, key, data)
    VALUE obj;
    DBT key, data;
{
    return rb_assoc_new(test_load_key(obj, key), 
			bdb_test_load(obj, data, FILTER_VALUE));
}

VALUE
bdb_assoc_dyna(obj, key, data)
    VALUE obj;
    DBT key, data;
{
    VALUE v = test_load_dyna1(obj, key, data);
    return rb_assoc_new(test_load_key(obj, key), v);
}

static VALUE
bdb_assoc2(obj, skey, pkey, data)
    VALUE obj;
    DBT skey, pkey, data;
{
    return rb_assoc_new(
	rb_assoc_new(test_load_key(obj, skey), test_load_key(obj, pkey)),
	bdb_test_load(obj, data, FILTER_VALUE));
}

VALUE
bdb_assoc3(obj, skey, pkey, data)
    VALUE obj;
    DBT skey, pkey, data;
{
    return rb_ary_new3(3, test_load_key(obj, skey), 
		       test_load_key(obj, pkey),
		       bdb_test_load(obj, data, FILTER_VALUE));
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
    MEMZERO(&key, DBT, 1);
    MEMZERO(&data, DBT, 1);
    data.flags |= DB_DBT_MALLOC;
    switch(rb_scan_args(argc, argv, "12", &a, &b, &c)) {
    case 3:
        flags = NUM2INT(c);
        if ((flags & ~DB_RMW) == DB_GET_BOTH) {
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
	    return bdb_has_both_internal(obj, a, b, Qtrue);
#else
            test_dump(obj, data, b, FILTER_VALUE);
	    data.flags |= DB_DBT_MALLOC;
#endif
        }
	break;
    case 2:
	flagss = flags = NUM2INT(b);
	break;
    }
    test_recno(obj, key, recno, a);
    set_partial(dbst, data);
    flags |= test_init_lock(dbst);
    ret = bdb_test_error(dbst->dbp->get(dbst->dbp, txnid, &key, &data, flags));
    if (ret == DB_NOTFOUND || ret == DB_KEYEMPTY)
        return notfound;
    if (((flags & ~DB_RMW) == DB_GET_BOTH) ||
	((flags & ~DB_RMW) == DB_SET_RECNO)) {
        return bdb_assoc(obj, key, data);
    }
    if (dyna) {
	return test_load_dyna(obj, key, data);
    }
    else {
	return bdb_test_load(obj, data, FILTER_VALUE);
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

#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3) || DB_VERSION_MAJOR >= 4

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
    MEMZERO(&skey, DBT, 1);
    MEMZERO(&pkey, DBT, 1);
    MEMZERO(&data, DBT, 1);
    data.flags |= DB_DBT_MALLOC;
    pkey.flags |= DB_DBT_MALLOC;
    switch(rb_scan_args(argc, argv, "12", &a, &b, &c)) {
    case 3:
        flags = NUM2INT(c);
        if ((flags & ~DB_RMW) == DB_GET_BOTH) {
            test_dump(obj, data, b, FILTER_VALUE);
	    data.flags |= DB_DBT_MALLOC;
        }
	break;
    case 2:
	flagss = flags = NUM2INT(b);
	break;
    }
    test_recno(obj, skey, srecno, a);
    set_partial(dbst, data);
    flags |= test_init_lock(dbst);
    ret = bdb_test_error(dbst->dbp->pget(dbst->dbp, txnid, &skey, &pkey, &data, flags));
    if (ret == DB_NOTFOUND || ret == DB_KEYEMPTY)
        return Qnil;
    if (((flags & ~DB_RMW) == DB_GET_BOTH) ||
	((flags & ~DB_RMW) == DB_SET_RECNO)) {
        return bdb_assoc2(obj, skey, pkey, data);
    }
    return bdb_assoc(obj, pkey, data);
}

#endif

#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1) || DB_VERSION_MAJOR >= 4

static VALUE
bdb_btree_key_range(obj, a)
    VALUE obj, a;
{
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBT key;
    int ret;
    db_recno_t recno;
    DB_KEY_RANGE key_range;

    init_txn(txnid, obj, dbst);
    MEMZERO(&key, DBT, 1);
    test_recno(obj, key, recno, a);
    bdb_test_error(dbst->dbp->key_range(dbst->dbp, txnid, &key, &key_range, 0));
    return rb_struct_new(bdb_sKeyrange, 
			 rb_float_new(key_range.less),
			 rb_float_new(key_range.equal), 
			 rb_float_new(key_range.greater));
}
#endif

#if DB_VERSION_MAJOR > 2 || (DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR >= 6)
static VALUE
bdb_count(obj, a)
    VALUE obj, a;
{
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBC *dbcp;
    DBT key, data;
    int ret, flags27;
    db_recno_t recno;
    db_recno_t count;

    init_txn(txnid, obj, dbst);
    MEMZERO(&key, DBT, 1);
    test_recno(obj, key, recno, a);
    MEMZERO(&data, DBT, 1);
    data.flags |= DB_DBT_MALLOC;
    set_partial(dbst, data);
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
    flags27 = test_init_lock(dbst);
    ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, DB_SET | flags27));
    if (ret == DB_NOTFOUND) {
        bdb_test_error(dbcp->c_close(dbcp));
        return INT2NUM(0);
    }
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1) || DB_VERSION_MAJOR >= 4
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
}
#endif

#if DB_VERSION_MAJOR >= 3

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
    MEMZERO(&key, DBT, 1);
    MEMZERO(&data, DBT, 1);
    recno = 1;
    key.data = &recno;
    key.size = sizeof(db_recno_t);
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
    ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, DB_CONSUME));
    bdb_test_error(dbcp->c_close(dbcp));
    if (ret == DB_NOTFOUND) {
	return Qnil;
    }
    return bdb_assoc(obj, key, data);
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
    MEMZERO(&key, DBT, 1);
    MEMZERO(&data, DBT, 1);
    MEMZERO(&datas, DBT, 1);
    test_recno(obj, key, recno, a);
    test_dump(obj, datas, b, FILTER_VALUE);
    data.flags |= DB_DBT_MALLOC;
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
	    return bdb_assoc(obj, key, data);
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
#if DB_VERSION_MAJOR < 3 || (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR < 1)
    if (RECNUM_TYPE(dbst)) {
	free(data.data);
	dbcp->c_close(dbcp);
	return Qfalse;
    }
#endif
    while (1) {
	free_key(dbst, key);
	free(data.data);
	MEMZERO(&data, DBT, 1);
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
    int ret, flags;
    db_recno_t recno;

#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    return bdb_has_both_internal(obj, a, b, Qfalse);
#else
    init_txn(txnid, obj, dbst);
#if DB_VERSION_MAJOR < 3 || (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR < 1)
    if (RECNUM_TYPE(dbst)) {
	return bdb_has_both_internal(obj, a, b, Qfalse);
    }
#endif
    MEMZERO(&key, DBT, 1);
    MEMZERO(&data, DBT, 1);
    test_recno(obj, key, recno, a);
    key.flags |= DB_DBT_MALLOC;
    test_dump(obj, data, b, FILTER_VALUE);
    data.flags |= DB_DBT_MALLOC;
    set_partial(dbst, data);
    flags = DB_GET_BOTH | test_init_lock(dbst);
    ret = bdb_test_error(dbst->dbp->get(dbst->dbp, txnid, &key, &data, flags));
    if (ret == DB_NOTFOUND || ret == DB_KEYEMPTY)
        return Qfalse;
    free(data.data);
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
    MEMZERO(&key, DBT, 1);
    test_recno(obj, key, recno, a);
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
    MEMZERO(&key, DBT, 1);
    init_recno(dbst, key, recno);
    MEMZERO(&data, DBT, 1);
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
bdb_lgth_intern(obj, delete)
    VALUE obj, delete;
{
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBC *dbcp;
    DBT key, data;
    int ret, value, flags;
    db_recno_t recno;

    init_txn(txnid, obj, dbst);
    MEMZERO(&key, DBT, 1);
    init_recno(dbst, key, recno);
    MEMZERO(&data, DBT, 1);
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
	if (delete == Qtrue) {
	    bdb_test_error(dbcp->c_del(dbcp, 0));
	}
    } while (1);
    return INT2NUM(-1);
}

static VALUE
bdb_length(obj)
    VALUE obj;
{
    return bdb_lgth_intern(obj, Qfalse);
}

typedef struct {
    int sens;
    VALUE replace;
    VALUE db;
    VALUE set;
    DBC *dbcp;
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3) || DB_VERSION_MAJOR >= 4
    void *data;
    int len;
#endif
    int primary;
    int type;
} eachst;

static VALUE
bdb_each_ensure(st)
    eachst *st;
{
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3) || DB_VERSION_MAJOR >= 4
    if (st->len && st->data) {
	free(st->data);
    }
#endif
    st->dbcp->c_close(st->dbcp);
    return Qnil;
}

static void
bdb_treat(st, pkey, key, data)
    eachst *st;
    DBT *pkey, *key, *data;
{
    bdb_DB *dbst;
    DBC *dbcp;
    VALUE res;

    GetDB(st->db, dbst);
    dbcp = st->dbcp;
    switch (st->type) {
    case BDB_ST_DUPU:
	free_key(dbst, *key);
	res = bdb_test_load(st->db, *data, FILTER_VALUE);
	if (TYPE(st->replace) == T_ARRAY) {
	    rb_ary_push(st->replace, res);
	}
	else {
	    rb_yield(res);
	}
	break;
    case BDB_ST_DUPKV:
	rb_yield(bdb_assoc_dyna(st->db, *key, *data));
	break;
    case BDB_ST_DUPVAL:
	res = test_load_dyna(st->db, *key, *data);
	if (TYPE(st->replace) == T_ARRAY) {
	    rb_ary_push(st->replace, res);
	}
	else {
	    rb_yield(res);
	}
	break;
    case BDB_ST_KEY:
	if (data->flags & DB_DBT_MALLOC) {
	    free(data->data);
	}
	rb_yield(test_load_key(st->db, *key));
	break;

    case BDB_ST_VALUE:
	free_key(dbst, *key);
	res = rb_yield(bdb_test_load(st->db, *data, FILTER_VALUE));
	if (st->replace == Qtrue) {
	    MEMZERO(data, DBT, 1);
	    test_dump(st->db, *data, res, FILTER_VALUE);
	    set_partial(dbst, *data);
	    bdb_test_error(dbcp->c_put(dbcp, key, data, DB_CURRENT));
	}
	else if (st->replace != Qfalse) {
	    rb_ary_push(st->replace, res);
	}
	break;
    case BDB_ST_KV:
	if (st->primary) {
	    rb_yield(bdb_assoc3(st->db, *key, *pkey, *data));
	}
	else {
	    rb_yield(bdb_assoc_dyna(st->db, *key, *data));
	}
	break;
    case BDB_ST_REJECT:
    {
	VALUE ary = bdb_assoc(st->db, *key, *data);
	if (!RTEST(rb_yield(ary))) {
	    rb_hash_aset(st->replace, RARRAY(ary)->ptr[0], RARRAY(ary)->ptr[1]);
	}
	break;
    }
    case BDB_ST_DELETE:
	if (RTEST(rb_yield(bdb_assoc(st->db, *key, *data)))) {
	    bdb_test_error(dbcp->c_del(dbcp, 0));
	}
	break;
    }
}

static VALUE
bdb_i_each_kv(st)
    eachst *st;
{
    bdb_DB *dbst;
    DBC *dbcp;
    DBT pkey, key, data;
    int ret;
    db_recno_t recno;
    VALUE res;
    
    GetDB(st->db, dbst);
    dbcp = st->dbcp;
    MEMZERO(&key, DBT, 1);
    init_recno(dbst, key, recno);
    MEMZERO(&data, DBT, 1);
    data.flags = DB_DBT_MALLOC;
    set_partial(dbst, data);
    MEMZERO(&pkey, DBT, 1);
    pkey.flags = DB_DBT_MALLOC;
    if (!NIL_P(st->set)) {
	test_recno(st->db, key, recno, st->set);
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3) || DB_VERSION_MAJOR >= 4
	if (st->type == BDB_ST_KV && st->primary) {
	    ret = bdb_test_error(dbcp->c_pget(dbcp, &key, &pkey, &data, DB_SET_RANGE));
	}
	else
#endif
	{
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
	    key.flags |= DB_DBT_MALLOC;
#endif
	    ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, DB_SET_RANGE));
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
	    key.flags &= ~DB_DBT_MALLOC;
#endif
	}
	if (ret == DB_NOTFOUND) {
	    return Qfalse;
	}
	bdb_treat(st, &pkey, &key, &data);
    }
    do {
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3) || DB_VERSION_MAJOR >= 4
	if (st->type == BDB_ST_KV && st->primary) {
	    ret = bdb_test_error(dbcp->c_pget(dbcp, &key, &pkey, &data, st->sens));
	}
	else
#endif
	{
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
	    key.flags |= DB_DBT_MALLOC;
#endif
	    ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, st->sens));
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
	    key.flags &= ~DB_DBT_MALLOC;
#endif
	}
        if (ret == DB_NOTFOUND) {
            return Qnil;
        }
	if (ret == DB_KEYEMPTY) continue;
	bdb_treat(st, &pkey, &key, &data);
    } while (1);
    return Qnil;
}

#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3) || DB_VERSION_MAJOR >= 4

static VALUE
bdb_i_each_kv_bulk(st)
    eachst *st;
{
    bdb_DB *dbst;
    DBC *dbcp;
    DBT key, data;
    DBT rkey, rdata;
    DBT pkey;
    int ret, init;
    db_recno_t recno;
    VALUE res;
    void *p;
    
    GetDB(st->db, dbst);
    dbcp = st->dbcp;
    MEMZERO(&key, DBT, 1);
    MEMZERO(&pkey, DBT, 1);
    MEMZERO(&rkey, DBT, 1);
    init_recno(dbst, key, recno);
    MEMZERO(&data, DBT, 1);
    MEMZERO(&rdata, DBT, 1);
    st->data = data.data = ALLOC_N(void, st->len);
    data.ulen = st->len;
    data.flags = DB_DBT_USERMEM;
    set_partial(dbst, data);
    set_partial(dbst, rdata);
    init = 1;
    do {
	if (init && !NIL_P(st->set)) {
	    test_recno(st->db, key, recno, st->set);
	    ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, DB_SET_RANGE | DB_MULTIPLE_KEY));
	    init = 0;
	}
	else {
	    ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, st->sens | DB_MULTIPLE_KEY));
	}
        if (ret == DB_NOTFOUND) {
            return Qnil;
        }
	if (ret == DB_KEYEMPTY) continue;
	for (DB_MULTIPLE_INIT(p, &data);;) {
	    if (RECNUM_TYPE(dbst)) {
		DB_MULTIPLE_RECNO_NEXT(p, &data, recno, 
				       rdata.data, rdata.size);
	    }
	    else {
		DB_MULTIPLE_KEY_NEXT(p, &data, rkey.data, rkey.size,
				     rdata.data, rdata.size);
	    }
	    if (p == NULL) break;
	    bdb_treat(st, 0, &rkey, &rdata);
	}
    } while (1);
    return Qnil;
}

#endif

VALUE
bdb_each_kvc(argc, argv, obj, sens, replace, type)
    VALUE obj, *argv;
    int argc, sens;
    VALUE replace;
    int type;
{
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBC *dbcp;
    eachst st;

    MEMZERO(&st, eachst, 1);

#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3) || DB_VERSION_MAJOR >= 4
    {
	VALUE bulkv = Qnil;

	st.set = Qnil;
	if (type & BDB_ST_ONE) {
	    rb_scan_args(argc, argv, "01", &st.set);
	}
	else {
	    if (type & BDB_ST_DUP) {
		rb_scan_args(argc, argv, "11", &st.set, &bulkv);
	    }
	    else {
		if (rb_scan_args(argc, argv, "02", &st.set, &bulkv) == 2) {
		    if (bulkv == Qtrue || bulkv == Qfalse) {
			st.primary = RTEST(bulkv);
			bulkv = Qnil;
		    }
		}
	    }
	}
	if (!NIL_P(bulkv)) {
	    st.len = 1024 * NUM2INT(bulkv);
	    if (st.len < 0) {
		rb_raise(bdb_eFatal, "negative value for bulk retrieval");
	    }
	}
    }
#else
    if (type & BDB_ST_DUP) {
	if (argc != 1) {
	    rb_raise(bdb_eFatal, "invalid number of arguments (%d for 1)", argc);
	}
	st.set = argv[0];
    }
    else {
	rb_scan_args(argc, argv, "01", &st.set);
    }
#endif
    type &= ~BDB_ST_ONE;
    if (type == BDB_ST_DELETE) {
	rb_secure(4);
    }
    init_txn(txnid, obj, dbst);
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    st.db = obj;
    st.dbcp = dbcp;
    st.sens = sens | test_init_lock(dbst);
    st.replace = replace;
    st.type = type & ~BDB_ST_ONE;
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3) || DB_VERSION_MAJOR >= 4
    if (st.len) {
	rb_ensure(bdb_i_each_kv_bulk, (VALUE)&st, bdb_each_ensure, (VALUE)&st);
    }
    else
#endif
    {
	rb_ensure(bdb_i_each_kv, (VALUE)&st, bdb_each_ensure, (VALUE)&st);
    }
    if (replace == Qtrue || replace == Qfalse) {
	return obj;
    }
    else {
	return st.replace;
    }
}

#if DB_VERSION_MAJOR > 2 || (DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR >= 6)

static VALUE
bdb_get_dup(argc, argv, obj)
    VALUE obj, *argv;
    int argc;
{
    VALUE result = Qfalse;
    if (!rb_block_given_p()) {
	result = rb_ary_new();
    }
    return bdb_each_kvc(argc, argv, obj, DB_NEXT_DUP, result, BDB_ST_DUPU);
}

static VALUE
bdb_common_each_dup(argc, argv, obj)
    VALUE obj, *argv;
    int argc;
{
    if (!rb_block_given_p()) {
	rb_raise(bdb_eFatal, "each_dup called out of an iterator");
    }
    return bdb_each_kvc(argc, argv, obj, DB_NEXT_DUP, Qfalse, BDB_ST_DUPKV);
}

static VALUE
bdb_common_each_dup_val(argc, argv, obj)
    VALUE obj, *argv;
    int argc;
{
    if (!rb_block_given_p()) {
	rb_raise(bdb_eFatal, "each_dup called out of an iterator");
    }
    return bdb_each_kvc(argc, argv, obj, DB_NEXT_DUP, Qfalse, BDB_ST_DUPVAL);
}

static VALUE
bdb_common_dups(argc, argv, obj)
    VALUE obj, *argv;
    int argc;
{
    VALUE result = rb_ary_new();
    return bdb_each_kvc(argc, argv, obj, DB_NEXT_DUP, result, BDB_ST_DUPVAL);
}

#endif

static VALUE
bdb_delete_if(argc, argv, obj)
    VALUE obj, *argv;
    int argc;
{
    return bdb_each_kvc(argc, argv, obj, DB_NEXT, Qfalse, BDB_ST_DELETE | BDB_ST_ONE);
}

static VALUE
bdb_reject(argc, argv, obj)
    VALUE obj, *argv;
    int argc;
{
    return bdb_each_kvc(argc, argv, obj, DB_NEXT, rb_hash_new(), BDB_ST_REJECT);
}

VALUE
bdb_each_value(argc, argv, obj)
    VALUE obj, *argv;
    int argc;
{ 
    return bdb_each_kvc(argc, argv, obj, DB_NEXT, Qfalse, BDB_ST_VALUE);
}

VALUE
bdb_each_eulav(argc, argv, obj)
    VALUE obj, *argv;
    int argc;
{
    return bdb_each_kvc(argc, argv, obj, DB_PREV, Qfalse, BDB_ST_VALUE | BDB_ST_ONE);
}

VALUE
bdb_each_key(argc, argv, obj)
    int argc;
    VALUE *argv, obj;
{ 
    return bdb_each_kvc(argc, argv, obj, DB_NEXT, Qfalse, BDB_ST_KEY); 
}

static VALUE
bdb_each_yek(argc, argv, obj) 
    int argc;
    VALUE *argv, obj;
{ 
    return bdb_each_kvc(argc, argv, obj, DB_PREV, Qfalse, BDB_ST_KEY | BDB_ST_ONE);
}

static VALUE
bdb_each_pair(argc, argv, obj) 
    int argc;
    VALUE obj, *argv;
{
    return bdb_each_kvc(argc, argv, obj, DB_NEXT, Qfalse, BDB_ST_KV);
}

static VALUE
bdb_each_riap(argc, argv, obj) 
    int argc;
    VALUE obj, *argv;
{
    return bdb_each_kvc(argc, argv, obj, DB_PREV, Qfalse, BDB_ST_KV | BDB_ST_ONE);
}

static VALUE
bdb_each_pair_prim(argc, argv, obj) 
    int argc;
    VALUE obj, *argv;
{
    VALUE tmp[2] = {Qnil, Qtrue};
    rb_scan_args(argc, argv, "01", tmp);
    return bdb_each_kvc(2, tmp, obj, DB_NEXT, Qfalse, BDB_ST_KV);
}

static VALUE
bdb_each_riap_prim(argc, argv, obj) 
    int argc;
    VALUE obj, *argv;
{
    VALUE tmp[2] = {Qnil, Qtrue};
    rb_scan_args(argc, argv, "01", tmp);
    return bdb_each_kvc(2, tmp, obj, DB_PREV, Qfalse, BDB_ST_KV);
}

VALUE
bdb_to_type(obj, result, flag)
    VALUE obj, result, flag;
{
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBC *dbcp;
    DBT key, data;
    int ret, flags;
    db_recno_t recno;

    init_txn(txnid, obj, dbst);
    MEMZERO(&key, DBT, 1);
    init_recno(dbst, key, recno);
    MEMZERO(&data, DBT, 1);
    data.flags = DB_DBT_MALLOC;
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#endif
    set_partial(dbst, data);
    flags = ((flag == Qnil)?DB_PREV:DB_NEXT) | test_init_lock(dbst);
    do {
        ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, flags));
        if (ret == DB_NOTFOUND) {
            bdb_test_error(dbcp->c_close(dbcp));
            return result;
        }
	if (ret == DB_KEYEMPTY) continue;
	switch (TYPE(result)) {
	case T_ARRAY:
	    if (flag == Qtrue) {
		rb_ary_push(result, bdb_assoc(obj, key, data));
	    }
	    else {
		rb_ary_push(result, bdb_test_load(obj, data, FILTER_VALUE));
	    }
	    break;
	case T_HASH:
	    if (flag == Qtrue) {
		rb_hash_aset(result, test_load_key(obj, key), 
			     bdb_test_load(obj, data, FILTER_VALUE));
	    }
	    else {
		rb_hash_aset(result, bdb_test_load(obj, data, FILTER_VALUE), 
			     test_load_key(obj, key));
	    }
	}

    } while (1);
    return result;
}

static VALUE
bdb_to_a(obj)
    VALUE obj;
{
    return bdb_to_type(obj, rb_ary_new(), Qtrue);
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
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3) || DB_VERSION_MAJOR >= 4
    bdb_DB *dbst;
    DB_TXN *txnid;
    int count;
#endif

    rb_secure(4);
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3) || DB_VERSION_MAJOR >= 4
    init_txn(txnid, obj, dbst);
    bdb_test_error(dbst->dbp->truncate(dbst->dbp, txnid, &count, 0));
    return INT2NUM(count);
#else
    return bdb_lgth_intern(obj, Qtrue);
#endif
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
bdb_invert(obj)
    VALUE obj;
{
    return bdb_to_type(obj, rb_hash_new(), Qfalse);
}

static VALUE
bdb_to_hash(obj)
    VALUE obj;
{
    return bdb_to_type(obj, rb_hash_new(), Qtrue);
}
 
static VALUE
bdb_kv(obj, type)
    VALUE obj;
    int type;
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
    MEMZERO(&key, DBT, 1);
    init_recno(dbst, key, recno);
    MEMZERO(&data, DBT, 1);
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
	switch (type) {
	case BDB_ST_VALUE:
	    free_key(dbst, key);
	    rb_ary_push(ary, bdb_test_load(obj, data, FILTER_VALUE));
	    break;
	case BDB_ST_KEY:
	    free(data.data);
	    rb_ary_push(ary, test_load_key(obj, key));
	    break;
	}
    } while (1);
    return ary;
}

static VALUE
bdb_values(obj)
    VALUE obj;
{
    return bdb_kv(obj, BDB_ST_VALUE);
}

static VALUE
bdb_keys(obj)
    VALUE obj;
{
    return bdb_kv(obj, BDB_ST_KEY);
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
    MEMZERO(&key, DBT, 1);
    init_recno(dbst, key, recno);
    MEMZERO(&data, DBT, 1);
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
	if (rb_equal(a, bdb_test_load(obj, data, FILTER_VALUE)) == Qtrue) {
	    VALUE d;
	    
	    bdb_test_error(dbcp->c_close(dbcp));
	    if (b == Qfalse) {
		d = Qtrue;
		free_key(dbst, key);
	    }
	    else {
		d = test_load_key(obj, key);
	    }
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

#if DB_VERSION_MAJOR >= 3
static VALUE
bdb_hash_stat(argc, argv, obj)
    int argc;
    VALUE *argv, obj;
{
    bdb_DB *dbst;
    DB_HASH_STAT *bdb_stat;
    VALUE hash, flagv;
    int ret;
    int flags = 0;

    if (rb_scan_args(argc, argv, "01", &flagv) == 1) {
	flags = NUM2INT(flagv);
    }
    GetDB(obj, dbst);
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR < 3
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &bdb_stat, 0, flags));
#else
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &bdb_stat, flags));
#endif
    hash = rb_hash_new();
    rb_hash_aset(hash, rb_tainted_str_new2("hash_magic"), INT2NUM(bdb_stat->hash_magic));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_version"), INT2NUM(bdb_stat->hash_version));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_pagesize"), INT2NUM(bdb_stat->hash_pagesize));
#if (DB_VERSION_MAJOR == 3 &&					\
        ((DB_VERSION_MINOR >= 1 && DB_VERSION_PATCH >= 14) ||	\
         DB_VERSION_MINOR >= 2)) ||				\
    DB_VERSION_MAJOR >= 4
    rb_hash_aset(hash, rb_tainted_str_new2("hash_nkeys"), INT2NUM(bdb_stat->hash_nkeys));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_nrecs"), INT2NUM(bdb_stat->hash_nkeys));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_ndata"), INT2NUM(bdb_stat->hash_ndata));
#else
    rb_hash_aset(hash, rb_tainted_str_new2("hash_nrecs"), INT2NUM(bdb_stat->hash_nrecs));
#endif
    rb_hash_aset(hash, rb_tainted_str_new2("hash_nelem"), INT2NUM(bdb_stat->hash_nelem));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_ffactor"), INT2NUM(bdb_stat->hash_ffactor));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_buckets"), INT2NUM(bdb_stat->hash_buckets));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_free"), INT2NUM(bdb_stat->hash_free));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_bfree"), INT2NUM(bdb_stat->hash_bfree));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_bigpages"), INT2NUM(bdb_stat->hash_bigpages));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_big_bfree"), INT2NUM(bdb_stat->hash_big_bfree));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_overflows"), INT2NUM(bdb_stat->hash_overflows));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_ovfl_free"), INT2NUM(bdb_stat->hash_ovfl_free));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_dup"), INT2NUM(bdb_stat->hash_dup));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_dup_free"), INT2NUM(bdb_stat->hash_dup_free));
    free(bdb_stat);
    return hash;
}
#endif

VALUE
bdb_tree_stat(argc, argv, obj)
    int argc;
    VALUE *argv, obj;
{
    bdb_DB *dbst;
    DB_BTREE_STAT *bdb_stat;
    VALUE hash, flagv;
    char pad;
    int flags = 0;

    if (rb_scan_args(argc, argv, "01", &flagv) == 1) {
	flags = NUM2INT(flagv);
    }
    GetDB(obj, dbst);
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3) || DB_VERSION_MAJOR >= 4
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &bdb_stat, flags));
#else
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &bdb_stat, 0, flags));
#endif
    hash = rb_hash_new();
    rb_hash_aset(hash, rb_tainted_str_new2("bt_magic"), INT2NUM(bdb_stat->bt_magic));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_version"), INT2NUM(bdb_stat->bt_version));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_dup_pg"), INT2NUM(bdb_stat->bt_dup_pg));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_dup_pgfree"), INT2NUM(bdb_stat->bt_dup_pgfree));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_free"), INT2NUM(bdb_stat->bt_free));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_int_pg"), INT2NUM(bdb_stat->bt_int_pg));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_int_pgfree"), INT2NUM(bdb_stat->bt_int_pgfree));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_leaf_pg"), INT2NUM(bdb_stat->bt_leaf_pg));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_leaf_pgfree"), INT2NUM(bdb_stat->bt_leaf_pgfree));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_levels"), INT2NUM(bdb_stat->bt_levels));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_minkey"), INT2NUM(bdb_stat->bt_minkey));
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1) || DB_VERSION_MAJOR >= 4
    rb_hash_aset(hash, rb_tainted_str_new2("bt_nrecs"), INT2NUM(bdb_stat->bt_nkeys));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_nkeys"), INT2NUM(bdb_stat->bt_nkeys));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_ndata"), INT2NUM(bdb_stat->bt_ndata));
#else
    rb_hash_aset(hash, rb_tainted_str_new2("bt_nrecs"), INT2NUM(bdb_stat->bt_nrecs));
#endif
    rb_hash_aset(hash, rb_tainted_str_new2("bt_over_pg"), INT2NUM(bdb_stat->bt_over_pg));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_over_pgfree"), INT2NUM(bdb_stat->bt_over_pgfree));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_pagesize"), INT2NUM(bdb_stat->bt_pagesize));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_re_len"), INT2NUM(bdb_stat->bt_re_len));
    pad = (char)bdb_stat->bt_re_pad;
    rb_hash_aset(hash, rb_tainted_str_new2("bt_re_pad"), rb_tainted_str_new(&pad, 1));
    free(bdb_stat);
    return hash;
}

#if DB_VERSION_MAJOR >= 3
static VALUE
bdb_queue_stat(argc, argv, obj)
    int argc;
    VALUE *argv, obj;
{
    bdb_DB *dbst;
    DB_QUEUE_STAT *bdb_stat;
    VALUE hash, flagv;
    char pad;
    int flags = 0;

    if (rb_scan_args(argc, argv, "01", &flagv) == 1) {
	flags = NUM2INT(flagv);
    }
    GetDB(obj, dbst);
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR < 3
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &bdb_stat, 0, flags));
#else
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &bdb_stat, flags));
#endif
    hash = rb_hash_new();
    rb_hash_aset(hash, rb_tainted_str_new2("qs_magic"), INT2NUM(bdb_stat->qs_magic));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_version"), INT2NUM(bdb_stat->qs_version));
#if (DB_VERSION_MAJOR == 3 &&					\
        ((DB_VERSION_MINOR >= 1 && DB_VERSION_PATCH >= 14) ||	\
         DB_VERSION_MINOR >= 2)) ||				\
    DB_VERSION_MAJOR >= 4
    rb_hash_aset(hash, rb_tainted_str_new2("qs_nrecs"), INT2NUM(bdb_stat->qs_nkeys));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_nkeys"), INT2NUM(bdb_stat->qs_nkeys));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_ndata"), INT2NUM(bdb_stat->qs_ndata));
#else
    rb_hash_aset(hash, rb_tainted_str_new2("qs_nrecs"), INT2NUM(bdb_stat->qs_nrecs));
#endif
    rb_hash_aset(hash, rb_tainted_str_new2("qs_pages"), INT2NUM(bdb_stat->qs_pages));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_pagesize"), INT2NUM(bdb_stat->qs_pagesize));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_pgfree"), INT2NUM(bdb_stat->qs_pgfree));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_re_len"), INT2NUM(bdb_stat->qs_re_len));
    pad = (char)bdb_stat->qs_re_pad;
    rb_hash_aset(hash, rb_tainted_str_new2("qs_re_pad"), rb_tainted_str_new(&pad, 1));
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR < 2
    rb_hash_aset(hash, rb_tainted_str_new2("qs_start"), INT2NUM(bdb_stat->qs_start));
#endif
    rb_hash_aset(hash, rb_tainted_str_new2("qs_first_recno"), INT2NUM(bdb_stat->qs_first_recno));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_cur_recno"), INT2NUM(bdb_stat->qs_cur_recno));
    free(bdb_stat);
    return hash;
}

static VALUE
bdb_queue_padlen(obj)
    VALUE obj;
{
    bdb_DB *dbst;
    DB_QUEUE_STAT *bdb_stat;
    VALUE hash;
    char pad;

    GetDB(obj, dbst);
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR < 3
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &bdb_stat, 0, 0));
#else
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &bdb_stat, 0));
#endif
    pad = (char)bdb_stat->qs_re_pad;
    hash = rb_assoc_new(rb_tainted_str_new(&pad, 1), INT2NUM(bdb_stat->qs_re_len));
    free(bdb_stat);
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

#if DB_VERSION_MAJOR >= 3
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
#if DB_VERSION_MAJOR >= 3
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
#if DB_VERSION_MAJOR >= 3
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
#else
    rb_raise(bdb_eFatal, "You can't remove a database with this version of DB");
#endif
    return Qtrue;
}

static VALUE
bdb_s_rename(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
#if (DB_VERSION_MAJOR == 3 &&					\
        ((DB_VERSION_MINOR >= 1 && DB_VERSION_PATCH >= 14) ||	\
         DB_VERSION_MINOR >= 2)) ||				\
    DB_VERSION_MAJOR >= 4
    bdb_DB *dbst;
    int ret;
    VALUE a, b, c;
    char *name, *subname, *newname;

    rb_secure(2);
    c = bdb_i_create(obj);
    GetDB(c, dbst);
    name = subname = NULL;
    a = b = c = Qnil;
    rb_scan_args(argc, argv, "30", &a, &b, &c);
    if (!NIL_P(b)) {
	Check_SafeStr(b);
	subname = RSTRING(b)->ptr;
    }
    Check_SafeStr(a);
    Check_SafeStr(c);
    name = RSTRING(a)->ptr;
    newname = RSTRING(c)->ptr;
    bdb_test_error(dbst->dbp->rename(dbst->dbp, name, subname, newname, 0));
#else
    rb_raise(bdb_eFatal, "You can't rename a database with this version of DB");
#endif
    return Qtrue;
}

#if DB_VERSION_MAJOR > 2 || (DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR >= 6)

static VALUE
bdb_i_joinclose(st)
    eachst *st;
{
    bdb_DB *dbst;

    GetDB(st->db, dbst);
    if (st->dbcp && dbst && dbst->dbp) {
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
    bdb_DB *dbst;

    GetDB(st->db, dbst);
    MEMZERO(&key, DBT, 1);
    init_recno(dbst, key, recno);
    MEMZERO(&data, DBT, 1);
    data.flags |= DB_DBT_MALLOC;
    set_partial(dbst, data);
    do {
	ret = bdb_test_error(st->dbcp->c_get(st->dbcp, &key, &data, st->sens));
	if (ret  == DB_NOTFOUND || ret == DB_KEYEMPTY)
	    return Qnil;
	rb_yield(bdb_assoc(st->db, key, data));
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
	bdb_DB *tmp;

	for (dbs = dbcarr, i = 0; i < RARRAY(a)->len; i++, dbs++) {
	    if (!rb_obj_is_kind_of(RARRAY(a)->ptr[i], bdb_cCursor)) {
		rb_raise(bdb_eFatal, "element %d is not a cursor");
	    }
	    GetCursorDB(RARRAY(a)->ptr[i], dbcst, tmp);
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
    st.db = obj;
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
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR < 3
    return dbst->dbp->get_byteswapped(dbst->dbp)?Qtrue:Qfalse;
#else
    dbst->dbp->get_byteswapped(dbst->dbp, &byteswap);
    return byteswap?Qtrue:Qfalse;
#endif
#endif
}

#endif

#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3) || DB_VERSION_MAJOR >= 4

static VALUE
bdb_internal_second_call(tmp)
    VALUE *tmp;
{
    return rb_funcall2(tmp[0], bdb_id_call, 3, tmp + 1);
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

    if ((obj = rb_thread_local_aref(rb_thread_current(), id_current_db)) == Qnil) {
	rb_raise(bdb_eFatal, "BUG (secondary index) : current_db not set");
    }
    Data_Get_Struct(obj, bdb_DB, dbst);
    if (!dbst->dbp || !RTEST(dbst->secondary)) return DB_DONOTINDEX;
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
	    tmp[2] = test_load_key(obj, *pkey);
	    tmp[3] = bdb_test_load(obj, *pdata, FILTER_VALUE);
	    inter = 0;
	    result = rb_protect(bdb_internal_second_call, (VALUE)tmp, &inter);
	    if (inter) return BDB_ERROR_PRIVATE;
	    if (result == Qfalse) return DB_DONOTINDEX;
	    MEMZERO(skey, DBT, 1);
	    if (result == Qtrue) {
		skey->data = pkey->data;
		skey->size = pkey->size;
	    }
	    else {
		DBT stmp;
		MEMZERO(&stmp, DBT, 1);
		test_dump(second, stmp, result, FILTER_KEY);
		skey->data = stmp.data;
		skey->size = stmp.size;
	    }
	    return 0;
	}
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
    if (RTEST(secondst->secondary)) {
	rb_raise(bdb_eFatal, "associate with a primary index");
    }
    GetDB(obj, dbst);
    bdb_test_error(dbst->dbp->associate(dbst->dbp, secondst->dbp, 
					bdb_call_secondary, flags));
    dbst->options |= BDB_NEED_CURRENT;
    if (!dbst->secondary) {
	dbst->secondary = rb_ary_new();
    }
    rb_ary_push(dbst->secondary, rb_assoc_new(second, rb_f_lambda()));
    secondst->secondary = Qnil;
    return obj;
}

#endif

static VALUE
bdb_filename(obj)
    VALUE obj;
{
    bdb_DB *dbst;
    GetDB(obj, dbst);
    return dbst->filename;
}

static VALUE
bdb_database(obj)
    VALUE obj;
{
    bdb_DB *dbst;
    GetDB(obj, dbst);
    return dbst->database;
}

#if DB_VERSION_MAJOR >= 4 || (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3)
static VALUE
bdb_verify(argc, argv, obj)
    int argc;
    VALUE *argv, obj;
{
    bdb_DB *dbst;
    char *file, *database;
    VALUE flagv, iov;
    int flags = 0;
    OpenFile *fptr;
    FILE *io = NULL;

    rb_secure(4);
    file = database = NULL;
    switch(rb_scan_args(argc, argv, "02", iov, flagv)) {
    case 2:
	flags = NUM2INT(flagv);
    case 1:
	if (!NIL_P(iov)) {
	    iov = rb_convert_type(iov, T_FILE, "IO", "to_io");
	    GetOpenFile(iov, fptr);
	    rb_io_check_writable(fptr);
	    io = GetWriteFile(fptr);
	}
	break;
    case 0:
	break;
    }
    GetDB(obj, dbst);
    if (!NIL_P(dbst->filename)) {
	file = RSTRING(dbst->filename)->ptr;
    }
    if (!NIL_P(dbst->database)) {
	database = RSTRING(dbst->database)->ptr;
    }
    bdb_test_error(dbst->dbp->verify(dbst->dbp, file, database, io, flags));
    return Qnil;
}
#endif

void bdb_init_common()
{
    id_bt_compare = rb_intern("bdb_bt_compare");
    id_bt_prefix = rb_intern("bdb_bt_prefix");
    id_dup_compare = rb_intern("bdb_dup_compare");
    id_h_hash = rb_intern("bdb_h_hash");
    bdb_id_call = rb_intern("call");
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
    rb_define_singleton_method(bdb_cCommon, "rename", bdb_s_rename, -1);
    rb_define_singleton_method(bdb_cCommon, "bdb_rename", bdb_s_rename, -1);
    rb_define_method(bdb_cCommon, "filename", bdb_filename, 0);
    rb_define_method(bdb_cCommon, "subname", bdb_database, 0);
    rb_define_method(bdb_cCommon, "database", bdb_database, 0);
#if DB_VERSION_MAJOR >= 4 || (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3)
    rb_define_method(bdb_cCommon, "verify", bdb_verify, -1);
#endif
    rb_define_method(bdb_cCommon, "close", bdb_close, -1);
    rb_define_method(bdb_cCommon, "db_close", bdb_close, -1);
    rb_define_method(bdb_cCommon, "put", bdb_put, -1);
    rb_define_method(bdb_cCommon, "db_put", bdb_put, -1);
    rb_define_method(bdb_cCommon, "[]=", bdb_put, -1);
    rb_define_method(bdb_cCommon, "store", bdb_put, -1);
    rb_define_method(bdb_cCommon, "env", bdb_env, 0);
    rb_define_method(bdb_cCommon, "has_env?", bdb_has_env, 0);
    rb_define_method(bdb_cCommon, "env?", bdb_has_env, 0);
#if DB_VERSION_MAJOR > 2 || (DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR >= 6)
    rb_define_method(bdb_cCommon, "count", bdb_count, 1);
    rb_define_method(bdb_cCommon, "dup_count", bdb_count, 1);
    rb_define_method(bdb_cCommon, "each_dup", bdb_common_each_dup, -1);
    rb_define_method(bdb_cCommon, "each_dup_value", bdb_common_each_dup_val, -1);
    rb_define_method(bdb_cCommon, "dups", bdb_common_dups, -1);
    rb_define_method(bdb_cCommon, "duplicates", bdb_common_dups, -1);
    rb_define_method(bdb_cCommon, "get_dup", bdb_get_dup, -1);
#endif
    rb_define_method(bdb_cCommon, "get", bdb_get_dyna, -1);
    rb_define_method(bdb_cCommon, "db_get", bdb_get_dyna, -1);
    rb_define_method(bdb_cCommon, "[]", bdb_get_dyna, -1);
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3) || DB_VERSION_MAJOR >= 4
    rb_define_method(bdb_cCommon, "pget", bdb_pget, -1);
    rb_define_method(bdb_cCommon, "primary_get", bdb_pget, -1);
    rb_define_method(bdb_cCommon, "db_pget", bdb_pget, -1);
#endif
    rb_define_method(bdb_cCommon, "fetch", bdb_get_dyna, -1);
    rb_define_method(bdb_cCommon, "delete", bdb_del, 1);
    rb_define_method(bdb_cCommon, "del", bdb_del, 1);
    rb_define_method(bdb_cCommon, "db_del", bdb_del, 1);
    rb_define_method(bdb_cCommon, "sync", bdb_sync, 0);
    rb_define_method(bdb_cCommon, "db_sync", bdb_sync, 0);
    rb_define_method(bdb_cCommon, "flush", bdb_sync, 0);
    rb_define_method(bdb_cCommon, "each", bdb_each_pair, -1);
    rb_define_method(bdb_cCommon, "each_primary", bdb_each_pair_prim, -1);
    rb_define_method(bdb_cCommon, "each_value", bdb_each_value, -1);
    rb_define_method(bdb_cCommon, "reverse_each_value", bdb_each_eulav, -1);
    rb_define_method(bdb_cCommon, "each_key", bdb_each_key, -1);
    rb_define_method(bdb_cCommon, "reverse_each_key", bdb_each_yek, -1);
    rb_define_method(bdb_cCommon, "each_pair", bdb_each_pair, -1);
    rb_define_method(bdb_cCommon, "reverse_each", bdb_each_riap, -1);
    rb_define_method(bdb_cCommon, "reverse_each_pair", bdb_each_riap, -1);
    rb_define_method(bdb_cCommon, "reverse_each_primary", bdb_each_riap_prim, -1);
    rb_define_method(bdb_cCommon, "keys", bdb_keys, 0);
    rb_define_method(bdb_cCommon, "values", bdb_values, 0);
    rb_define_method(bdb_cCommon, "delete_if", bdb_delete_if, -1);
    rb_define_method(bdb_cCommon, "reject!", bdb_delete_if, -1);
    rb_define_method(bdb_cCommon, "reject", bdb_reject, -1);
    rb_define_method(bdb_cCommon, "clear", bdb_clear, 0);
    rb_define_method(bdb_cCommon, "truncate", bdb_clear, 0);
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
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3) || DB_VERSION_MAJOR >= 4
    rb_define_method(bdb_cCommon, "associate", bdb_associate, -1);
#endif
    bdb_cBtree = rb_define_class_under(bdb_mDb, "Btree", bdb_cCommon);
    rb_define_method(bdb_cBtree, "stat", bdb_tree_stat, -1);
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1) || DB_VERSION_MAJOR >= 4
    bdb_sKeyrange = rb_struct_define("Keyrange", "less", "equal", "greater", 0);
    rb_global_variable(&bdb_sKeyrange);
    rb_define_method(bdb_cBtree, "key_range", bdb_btree_key_range, 1);
#endif
    bdb_cHash = rb_define_class_under(bdb_mDb, "Hash", bdb_cCommon);
#if DB_VERSION_MAJOR >= 3
    rb_define_method(bdb_cHash, "stat", bdb_hash_stat, -1);
#endif
    bdb_cRecno = rb_define_class_under(bdb_mDb, "Recno", bdb_cCommon);
    rb_define_method(bdb_cRecno, "each_index", bdb_each_key, -1);
    rb_define_method(bdb_cRecno, "unshift", bdb_unshift, -1);
    rb_define_method(bdb_cRecno, "<<", bdb_append, 1);
    rb_define_method(bdb_cRecno, "push", bdb_append_m, -1);
    rb_define_method(bdb_cRecno, "stat", bdb_tree_stat, -1);
#if DB_VERSION_MAJOR >= 3
    bdb_cQueue = rb_define_class_under(bdb_mDb, "Queue", bdb_cCommon);
    rb_define_singleton_method(bdb_cQueue, "new", bdb_queue_s_new, -1);
    rb_define_singleton_method(bdb_cQueue, "create", bdb_queue_s_new, -1);
    rb_define_singleton_method(bdb_cQueue, "open", bdb_queue_s_new, -1);
    rb_define_method(bdb_cQueue, "each_index", bdb_each_key, -1);
    rb_define_method(bdb_cQueue, "<<", bdb_append, 1);
    rb_define_method(bdb_cQueue, "push", bdb_append_m, -1);
    rb_define_method(bdb_cQueue, "shift", bdb_consume, 0);
    rb_define_method(bdb_cQueue, "stat", bdb_queue_stat, -1);
    rb_define_method(bdb_cQueue, "pad", bdb_queue_padlen, 0);
#endif
    bdb_cUnknown = rb_define_class_under(bdb_mDb, "Unknown", bdb_cCommon);
}
