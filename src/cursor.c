#include "bdb.h"

static void 
bdb_cursor_free(dbcst)
    bdb_DBC *dbcst;
{
    if (dbcst->dbc && dbcst->dbst && dbcst->dbst->dbp) {
        bdb_test_error(dbcst->dbc->c_close(dbcst->dbc));
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
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbc));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbc, flags));
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
    bdb_test_error(dbcst->dbc->c_close(dbcst->dbc));
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
    bdb_test_error(dbcst->dbc->c_del(dbcst->dbc, flags));
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
    bdb_test_error(dbcst->dbc->c_dup(dbcst->dbc, &dbcdup, flags));
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
    bdb_test_error(dbcst->dbc->c_count(dbcst->dbc, &count, 0));
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
    ret = bdb_test_error(dbcst->dbc->c_get(dbcst->dbc, &key_o, &data_o, DB_CURRENT));
    if (ret == DB_NOTFOUND || ret == DB_KEYEMPTY)
        return INT2NUM(0);
    count = 1;
    while (1) {
	ret = bdb_test_error(dbcst->dbc->c_get(dbcst->dbc, &key, &data, DB_NEXT_DUP));
	if (ret == DB_NOTFOUND) {
	    bdb_test_error(dbcst->dbc->c_get(dbcst->dbc, &key_o, &data_o, DB_SET));

	    free_key(dbcst->dbst, key_o);
	    free(data_o.data);
	    return INT2NUM(count);
	}
	if (ret == DB_KEYEMPTY) continue;
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
    ret = bdb_test_error(dbcst->dbc->c_get(dbcst->dbc, &key, &data, flags));
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

#if DB_VERSION_MAJOR == 3 || DB_VERSION_MINOR >= 6

static VALUE
bdb_cursor_next_dup(obj)
    VALUE obj;
{
    return bdb_cursor_xxx(obj, DB_NEXT_DUP);
}

struct iea_st {
    VALUE cursor;
    VALUE key;
    VALUE ret;
    int assoc;
};

static VALUE
bdb_i_each_dup_ensure(iea)
    struct iea_st *iea;
{
    return bdb_cursor_close(iea->cursor);
}

static VALUE
bdb_i_each_dup(iea)
    struct iea_st *iea;
{
    VALUE res;
    
    res = bdb_cursor_set(iea->cursor, iea->key);
    if (res == Qnil) return Qnil;
    if (!iea->assoc) {
	res = RARRAY(res)->ptr[1];
    }
    if (iea->ret != Qnil) {
	rb_ary_push(iea->ret, res);
    }
    else {
	rb_yield(res);
	    
    }
    while ((res = bdb_cursor_next_dup(iea->cursor)) != Qnil) {
	if (!iea->assoc) {
	    res = RARRAY(res)->ptr[1];
	}
	if (iea->ret != Qnil) {
	    rb_ary_push(iea->ret, res);
	}
	else {
	    rb_yield(res);
	}
    }
    return Qtrue;
}

static VALUE
bdb_common_dups(argc, argv, obj)
    int argc;
    VALUE obj, *argv;
{
    VALUE a, b, ret;
    struct iea_st iea;

    iea.assoc = 1;
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	iea.assoc = RTEST(b);
    }
    iea.ret = Qnil;
    if (!rb_block_given_p()) {
	iea.ret = rb_ary_new();
    }
    iea.cursor = bdb_cursor(obj);
    iea.key = a;
    rb_ensure(bdb_i_each_dup, (VALUE)&iea, bdb_i_each_dup_ensure, (VALUE)&iea);
    if (!rb_block_given_p()) return iea.ret;
    return obj;
}

static VALUE
bdb_common_each_dup(obj, a)
    VALUE obj, a;
{
    return bdb_common_dups(1, &a, obj);
}

static VALUE
bdb_common_each_dup_val(obj, a)
    VALUE obj, a;
{
    VALUE tmp[2];
    tmp[0] = a;
    tmp[1] = Qfalse;
    return bdb_common_dups(2, tmp, obj);
}

#endif

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
    ret = bdb_test_error(dbcst->dbc->c_put(dbcst->dbc, &key, &data, flags));
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

void bdb_init_cursor()
{
    rb_define_method(bdb_cCommon, "db_cursor", bdb_cursor, 0);
    rb_define_method(bdb_cCommon, "cursor", bdb_cursor, 0);
#if DB_VERSION_MAJOR == 3
    rb_define_method(bdb_cCommon, "each_dup", bdb_common_each_dup, 1);
    rb_define_method(bdb_cCommon, "each_dup_value", bdb_common_each_dup_val, 1);
    rb_define_method(bdb_cCommon, "dups", bdb_common_dups, -1);
    rb_define_method(bdb_cCommon, "duplicates", bdb_common_dups, -1);
#endif
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
#if DB_VERSION_MAJOR == 3 || DB_VERSION_MINOR >= 6
    rb_define_method(bdb_cCursor, "c_next_dup", bdb_cursor_next_dup, 0);
    rb_define_method(bdb_cCursor, "next_dup", bdb_cursor_next_dup, 0);
#endif
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
}
