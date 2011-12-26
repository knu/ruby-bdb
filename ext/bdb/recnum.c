#include "bdb.h"

static ID id_cmp;

static VALUE
bdb_recnum_init(int argc, VALUE *argv, VALUE obj)
{
    VALUE *nargv;
    VALUE array = rb_str_new2("array_base");
    VALUE sarray = rb_str_new2("set_array_base");

    if (!argc || TYPE(argv[argc - 1]) != T_HASH) {
	nargv = ALLOCA_N(VALUE, argc + 1);
	MEMCPY(nargv, argv, VALUE, argc);
	nargv[argc] = rb_hash_new();
	argv = nargv;
	argc++;
    }
    rb_hash_aset(argv[argc - 1], array, INT2FIX(0));
    if (rb_hash_aref(argv[argc - 1], sarray) != RHASH(argv[argc - 1])->ifnone) {
	rb_hash_aset(argv[argc - 1], sarray, INT2FIX(0));
    }
    rb_hash_aset(argv[argc - 1], rb_str_new2("set_flags"), INT2FIX(DB_RENUMBER));
    return bdb_init(argc, argv, obj);
} 

static VALUE
bdb_sary_subseq(VALUE obj, long beg, long len)
{
    VALUE ary2, a;
    bdb_DB *dbst;
    long i;

    GetDB(obj, dbst);
    if (beg > dbst->len) return Qnil;
    if (beg < 0 || len < 0) return Qnil;

    if (beg + len > dbst->len) {
	len = dbst->len - beg;
    }
    if (len <= 0) return rb_ary_new2(0);

    ary2 = rb_ary_new2(len);
    for (i = 0; i < len; i++) {
	a = INT2NUM(i + beg);
	rb_ary_push(ary2, bdb_get(1, &a, obj));
    }
    return ary2;
}

static VALUE
bdb_sary_entry(VALUE obj, VALUE position)
{
    bdb_DB *dbst;
    long offset;

    GetDB(obj, dbst);
    if (dbst->len == 0) return Qnil;
    offset = NUM2LONG(position);
    if (offset < 0) {
	offset += dbst->len;
    }
    if (offset < 0 || dbst->len <= offset) return Qnil;
    position = INT2NUM(offset);
    return bdb_get(1, &position, obj);
}

static VALUE
bdb_sary_aref(int argc, VALUE *argv, VALUE obj)
{
    VALUE arg1, arg2;
    long beg, len;
    bdb_DB *dbst;

    GetDB(obj, dbst);
    if (rb_scan_args(argc, argv, "11", &arg1, &arg2) == 2) {
	beg = NUM2LONG(arg1);
	len = NUM2LONG(arg2);
	if (beg < 0) {
	    beg = dbst->len + beg;
	}
	return bdb_sary_subseq(obj, beg, len);
    }

    if (FIXNUM_P(arg1)) {
	return bdb_sary_entry(obj, arg1);
    }
    else if (TYPE(arg1) == T_BIGNUM) {
	rb_raise(rb_eIndexError, "index too big");
    }
    else {
	switch (rb_range_beg_len(arg1, &beg, &len, dbst->len, 0)) {
	  case Qfalse:
	    break;
	  case Qnil:
	    return Qnil;
	  default:
	    return bdb_sary_subseq(obj, beg, len);
	}
    }
    return bdb_sary_entry(obj, arg1);
}

static VALUE
bdb_intern_shift_pop(VALUE obj, int depart, int len)
{
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBC *dbcp;
    DBT key, data;
    int i, ret, flags;
    db_recno_t recno;
    VALUE res;

    rb_secure(4);
    INIT_TXN(txnid, obj, dbst);
#if HAVE_DB_CURSOR_4
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#endif
    SET_PARTIAL(dbst, data);
    flags = TEST_INIT_LOCK(dbst);
    res = rb_ary_new2(len);
    for (i = 0; i < len; i++) {
	MEMZERO(&key, DBT, 1);
	INIT_RECNO(dbst, key, recno);
	MEMZERO(&data, DBT, 1);
	data.flags = DB_DBT_MALLOC;
	bdb_cache_error(dbcp->c_get(dbcp, &key, &data, depart | flags),
			dbcp->c_close(dbcp), ret);
	if (ret == DB_NOTFOUND) break;
	rb_ary_push(res, bdb_test_load(obj, &data, FILTER_VALUE));
	bdb_cache_error(dbcp->c_del(dbcp, 0), dbcp->c_close(dbcp), ret);
	if (dbst->len > 0) dbst->len--;
    }
    dbcp->c_close(dbcp);
    if (RARRAY_LEN(res) == 0) return Qnil;
    else if (RARRAY_LEN(res) == 1) return RARRAY_PTR(res)[0];
    else return res;
}

static void
bdb_sary_replace(VALUE obj, VALUE beg, long len, long rpl)
{
    long i, j, rlen;
    VALUE tmp[2];
    bdb_DB *dbst;

    GetDB(obj, dbst);
    if (len < 0) rb_raise(rb_eIndexError, "negative length %ld", len);
    if (beg < 0) {
	beg += dbst->len;
    }
    if (beg < 0) {
	beg -= dbst->len;
	rb_raise(rb_eIndexError, "index %ld out of array", beg);
    }
    if (beg + len > dbst->len) {
	len = dbst->len - beg;
    }

    if (NIL_P(rpl)) {
	rpl = rb_ary_new2(0);
    }
    else if (TYPE(rpl) != T_ARRAY) {
	rpl = rb_ary_new3(1, rpl);
    }
    rlen = RARRAY_LEN(rpl);

    tmp[1] = Qnil;
    if (beg >= dbst->len) {
	for (i = dbst->len; i < beg; i++) {
	    tmp[0] = INT2NUM(i);
	    bdb_put(2, tmp, obj);
	    dbst->len++;
	}
	for (i = beg, j = 0; j < RARRAY_LEN(rpl); i++, j++) {
	    tmp[0] = INT2NUM(i);
	    tmp[1] = RARRAY_PTR(rpl)[j];
	    bdb_put(2, tmp, obj);
	    dbst->len++;
	}
    }
    else {
	if (len < rlen) {
	    tmp[1] = Qnil;
	    for (i = dbst->len - 1; i >= (beg + len); i--) {
		tmp[0] = INT2NUM(i);
		tmp[1] = bdb_get(1, tmp, obj);
		tmp[0] = INT2NUM(i + rlen - len);
		bdb_put(2, tmp, obj);
	    }
	    dbst->len += rlen - len;
	}
	for (i = beg, j = 0; j < rlen; i++, j++) {
	    tmp[0] = INT2NUM(i);
	    tmp[1] = RARRAY_PTR(rpl)[j];
	    bdb_put(2, tmp, obj);
	}
	if (len > rlen) {
	    for (i = beg + len; i < dbst->len; i++) {
		tmp[0] = INT2NUM(i);
		tmp[1] = bdb_get(1, tmp, obj);
		tmp[0] = INT2NUM(i + rlen - len);
		bdb_put(2, tmp, obj);
	    }
	    bdb_intern_shift_pop(obj, DB_LAST, len - rlen);
	}
    }
}

static VALUE
bdb_sary_aset(int argc, VALUE *argv, VALUE obj)
{
    long  beg, len;
    bdb_DB *dbst;

    GetDB(obj, dbst);
    if (argc == 3) {
	bdb_sary_replace(obj, NUM2LONG(argv[0]), NUM2LONG(argv[1]), argv[2]);
	return argv[2];
    }
    if (argc != 2) {
	rb_raise(rb_eArgError, "wrong # of arguments(%d for 2)", argc);
    }
    if (FIXNUM_P(argv[0])) {
	beg = FIX2LONG(argv[0]);
	goto fixnum;
    }
    else if (rb_range_beg_len(argv[0], &beg, &len, dbst->len, 1)) {
	bdb_sary_replace(obj, beg, len, argv[1]);
	return argv[1];
    }
    if (TYPE(argv[0]) == T_BIGNUM) {
	rb_raise(rb_eIndexError, "index too big");
    }

    beg = NUM2LONG(argv[0]);
  fixnum:
    if (beg < 0) {
	beg += dbst->len;
	if (beg < 0) {
	    rb_raise(rb_eIndexError, "index %ld out of array",
		     beg - dbst->len);
	}
    }
    if (beg > dbst->len) {
	VALUE nargv[2];
	int i;

	nargv[1] = Qnil;
	for (i = dbst->len; i < beg; i++) {
	    nargv[0] = INT2NUM(i);
	    bdb_put(2, nargv, obj);
	    dbst->len++;
	}
    }
    argv[0] = INT2NUM(beg);
    bdb_put(2, argv, obj);
    dbst->len++;
    return argv[1];
}

#if HAVE_RB_ARY_INSERT

static VALUE
bdb_sary_insert(int argc, VALUE *argv, VALUE obj)
{
    long pos;

    if (argc < 2) {
	rb_raise(rb_eArgError, "wrong number of arguments(at least 2)");
    }
    pos = NUM2LONG(argv[0]);
    if (pos == -1) {
	bdb_DB *dbst;

	GetDB(obj, dbst);
 	pos = dbst->len;
    }
    else if (pos < 0) {
	pos++;
    }

    bdb_sary_replace(obj, pos, 0, rb_ary_new4(argc-1, argv+1));
    return obj;
}

#endif

static VALUE
bdb_sary_at(VALUE obj, VALUE pos)
{
    return bdb_sary_entry(obj, pos);
}

static VALUE
bdb_sary_first(VALUE obj)
{
    bdb_DB *dbst;
    VALUE tmp;

    GetDB(obj, dbst);
    tmp = INT2NUM(0);
    return bdb_get(1, &tmp, obj);
}

static VALUE
bdb_sary_last(VALUE obj)
{
    bdb_DB *dbst;
    VALUE tmp;

    GetDB(obj, dbst);
    if (!dbst->len) return Qnil;
    tmp = INT2NUM(dbst->len);
    return bdb_get(1, &tmp, obj);
}

static VALUE
bdb_sary_fetch(int argc, VALUE *argv, VALUE obj)
{
    VALUE pos, ifnone;
    bdb_DB *dbst;
    long idx;

    GetDB(obj, dbst);
    rb_scan_args(argc, argv, "11", &pos, &ifnone);
    idx = NUM2LONG(pos);

    if (idx < 0) {
	idx +=  dbst->len;
    }
    if (idx < 0 || dbst->len <= idx) {
	return ifnone;
    }
    pos = INT2NUM(idx);
    return bdb_get(1, &pos, obj);
}
    

static VALUE
bdb_sary_concat(VALUE obj, VALUE y)
{
    bdb_DB *dbst;
    long i;
    VALUE tmp[2];

    y = rb_convert_type(y, T_ARRAY, "Array", "to_ary");
    GetDB(obj, dbst);
    for (i = 0; i < RARRAY_LEN(y); i++) {
	tmp[0] = INT2NUM(dbst->len);
	tmp[1] = RARRAY_PTR(y)[i];
	bdb_put(2, tmp, obj);
	dbst->len++;
    }
    return obj;
}
    
static VALUE
bdb_sary_push(VALUE obj, VALUE y)
{
    bdb_DB *dbst;
    VALUE tmp[2];

    GetDB(obj, dbst);
    tmp[0] = INT2NUM(dbst->len);
    tmp[1] = y;
    bdb_put(2, tmp, obj);
    dbst->len++;
    return obj;
}

static VALUE
bdb_sary_push_m(int argc, VALUE *argv, VALUE obj)
{
    bdb_DB *dbst;
    long i;
    VALUE tmp[2];

    if (argc == 0) {
	rb_raise(rb_eArgError, "wrong # of arguments(at least 1)");
    }
    if (argc > 0) {
	GetDB(obj, dbst);
	for (i = 0; i < argc; i++) {
	    tmp[0] = INT2NUM(dbst->len);
	    tmp[1] = argv[i];
	    bdb_put(2, tmp, obj);
	    dbst->len++;
	}
    }
    return obj;
}

static VALUE
bdb_sary_s_create(int argc, VALUE *argv, VALUE obj)
{
    VALUE res;

    res = rb_funcall2(obj, rb_intern("new"), 0, 0);
    if (argc < 0) {
        rb_raise(rb_eArgError, "negative number of arguments");
    }
    if (argc > 0) {
	bdb_sary_push_m(argc, argv, res);
    }
    return res;
}
    
static VALUE
bdb_sary_shift(VALUE obj)
{
    VALUE res;
    bdb_DB *dbst;

    GetDB(obj, dbst);
    if (dbst->len == 0) return Qnil;
    res = bdb_intern_shift_pop(obj, DB_FIRST, 1);
    return res;
}

static VALUE
bdb_sary_pop(VALUE obj)
{
    VALUE res;
    bdb_DB *dbst;

    GetDB(obj, dbst);
    if (dbst->len == 0) return Qnil;
    res = bdb_intern_shift_pop(obj, DB_LAST, 1);
    return res;
}

static VALUE
bdb_sary_unshift_m(int argc, VALUE *argv, VALUE obj)
{
    bdb_DB *dbst;
    VALUE tmp[2];
    long i;

    if (argc == 0) {
	rb_raise(rb_eArgError, "wrong # of arguments(at least 1)");
    }
    if (argc > 0) {
/* ++ */
	GetDB(obj, dbst);
	for (i = dbst->len - 1; i >= 0; i++) {
	    tmp[0] = INT2NUM(i);
	    tmp[1] = bdb_get(1, tmp, obj);
	    tmp[0] = INT2NUM(i + argc);
	    bdb_put(2, tmp, obj);
	}
	for (i = 0; i < argc; i++) {
	    tmp[0] = INT2NUM(i);
	    tmp[1] = argv[i];
	    bdb_put(2, tmp, obj);
	    dbst->len++;
	}
    }
    return obj;
}

static VALUE
bdb_sary_length(VALUE obj)
{
    bdb_DB *dbst;

    GetDB(obj, dbst);
    if (dbst->len < 0) rb_raise(bdb_eFatal, "Invalid BDB::Recnum");
    return INT2NUM(dbst->len);
}

static VALUE
bdb_sary_empty_p(VALUE obj)
{
    bdb_DB *dbst;

    GetDB(obj, dbst);
    if (dbst->len < 0) rb_raise(bdb_eFatal, "Invalid BDB::Recnum");
    return (dbst->len)?Qfalse:Qtrue;
}

static VALUE
bdb_sary_rindex(VALUE obj, VALUE a)
{
    return bdb_internal_value(obj, a, Qtrue, DB_PREV);
}

static VALUE
bdb_sary_to_a(VALUE obj)
{
    return bdb_to_type(obj, rb_ary_new(), Qfalse);
}

static VALUE
bdb_sary_reverse_m(VALUE obj)
{
    return bdb_to_type(obj, rb_ary_new(), Qnil);
}

static VALUE
bdb_sary_reverse_bang(VALUE obj)
{
    long i, j;
    bdb_DB *dbst;
    VALUE tmp[2], interm;

    GetDB(obj, dbst);
    if (dbst->len <= 1) return obj;
    i = 0;
    j = dbst->len - 1;
    while (i < j) {
	tmp[0] = INT2NUM(i);
	interm = bdb_get(1, tmp, obj);
	tmp[0] = INT2NUM(j);
	tmp[1] = bdb_get(1, tmp, obj);
	tmp[0] = INT2NUM(i);
	bdb_put(2, tmp, obj);
	tmp[0] = INT2NUM(j);
	tmp[1] = interm;
	bdb_put(2, tmp, obj);
	i++; j--;
    }
    return obj;
}

static VALUE
bdb_sary_collect_bang(int argc, VALUE *argv, VALUE obj)
{
    return bdb_each_kvc(argc, argv, obj, DB_NEXT, Qtrue, BDB_ST_VALUE);
}

static VALUE
bdb_sary_collect(int argc, VALUE *argv, VALUE obj)
{
    if (!rb_block_given_p()) {
	return bdb_sary_to_a(obj);
    }
    return bdb_each_kvc(argc, argv, obj, DB_NEXT, rb_ary_new(), BDB_ST_VALUE);
}

static VALUE
bdb_sary_select(int argc, VALUE *argv, VALUE obj)
{
    VALUE result;
    long i;

    if (rb_block_given_p()) {
	if (argc > 0) {
	    rb_raise(rb_eArgError, "wrong number arguments(%d for 0)", argc);
	}
	return bdb_each_kvc(argc, argv, obj, DB_NEXT, rb_ary_new(), BDB_ST_SELECT);
    }
#if HAVE_RB_ARY_VALUES_AT
	rb_warn("Recnum#select(index..) is deprecated; use Recnum#values_at");
#endif
    result = rb_ary_new();
    for (i = 0; i < argc; i++) {
	rb_ary_push(result, bdb_sary_fetch(1, argv + i, obj));
    }
    return result;
}

static VALUE
bdb_sary_values_at(int argc, VALUE *argv, VALUE obj)
{
    VALUE result;
    long i;

    result = rb_ary_new();
    for (i = 0; i < argc; i++) {
	rb_ary_push(result, bdb_sary_fetch(1, argv + i, obj));
    }
    return result;
}

static VALUE
bdb_sary_indexes(int argc, VALUE *argv, VALUE obj)
{
#if HAVE_RB_ARY_VALUES_AT
    rb_warn("Recnum#%s is deprecated; use Recnum#values_at",
#if HAVE_RB_FRAME_THIS_FUNC
	    rb_id2name(rb_frame_this_func()));
#else
	    rb_id2name(rb_frame_last_func()));
#endif
#endif
    return bdb_sary_values_at(argc, argv, obj);
}

static VALUE
bdb_sary_filter(int argc, VALUE *argv, VALUE obj)
{
    rb_warn("BDB::Recnum#filter is deprecated; use BDB::Recnum#collect!");
    return bdb_sary_collect_bang(argc, argv, obj);
}

static VALUE
bdb_sary_delete(VALUE obj, VALUE item)
{
    bdb_DB *dbst;
    long i1, i2;
    VALUE tmp, a;

    GetDB(obj, dbst);
    i2 = dbst->len;
    for (i1 = 0; i1 < dbst->len;) {
	tmp = INT2NUM(i1);
	a = bdb_get(1, &tmp, obj);
	if (rb_equal(a, item)) {
	    bdb_del(obj, INT2NUM(i1));
	    dbst->len--;
	}
	else {
	    i1++;
	}
    }
    if (dbst->len == i2) {
	if (rb_block_given_p()) {
	    return rb_yield(item);
	}
	return Qnil;
    }
    return item;
}

static VALUE
bdb_sary_delete_at_m(VALUE obj, VALUE a)
{
    bdb_DB *dbst;
    long pos;
    VALUE tmp;
    VALUE del = Qnil;

    GetDB(obj, dbst);
    pos = NUM2INT(a);
    if (pos >= dbst->len) return Qnil;
    if (pos < 0) pos += dbst->len;
    if (pos < 0) return Qnil;

    tmp = INT2NUM(pos);
    del = bdb_get(1, &tmp, obj);
    bdb_del(obj, tmp);
    dbst->len--;
    return del;
}

static VALUE
bdb_sary_reject_bang(VALUE obj)
{
    bdb_DB *dbst;
    long i1, i2;
    VALUE tmp, a;

    GetDB(obj, dbst);
    i2 = dbst->len;
    for (i1 = 0; i1 < dbst->len;) {
	tmp = INT2NUM(i1);
	a = bdb_get(1, &tmp, obj);
	if (!RTEST(rb_yield(a))) {
	    i1++;
	    continue;
	}
	bdb_del(obj, tmp);
	dbst->len--;
    }
    if (dbst->len == i2) return Qnil;
    return obj;
}

static VALUE
bdb_sary_delete_if(VALUE obj)
{
    bdb_sary_reject_bang(obj);
    return obj;
}

static VALUE
bdb_sary_replace_m(VALUE obj, VALUE obj2)
{
    bdb_DB *dbst;

    GetDB(obj, dbst);
    obj2 = rb_convert_type(obj2, T_ARRAY, "Array", "to_ary");
    bdb_sary_replace(obj, 0, dbst->len, obj2);
    return obj;
}

static VALUE
bdb_sary_clear(int argc, VALUE *argv, VALUE obj)
{
    bdb_DB *dbst;
    VALUE g;
    int flags = 0;

    if (argc && TYPE(argv[argc - 1]) == T_HASH) {
	VALUE f = argv[argc - 1];
	if ((g = rb_hash_aref(f, rb_intern("flags"))) != RHASH(f)->ifnone ||
	    (g = rb_hash_aref(f, rb_str_new2("flags"))) != RHASH(f)->ifnone) {
	    flags = NUM2INT(g);
	}
	argc--;
    }
    if (argc == 1) {
	flags = NUM2INT(argv[0]);
    }
    g = INT2FIX(flags);
    bdb_clear(1, &g, obj);
    GetDB(obj, dbst);
    dbst->len = 0;
    return obj;
}

static VALUE
bdb_sary_fill(int argc, VALUE *argv, VALUE obj)
{
    VALUE item, arg1, arg2, tmp[2];
    long beg, len, i;
    bdb_DB *dbst;

    GetDB(obj, dbst);
    rb_scan_args(argc, argv, "12", &item, &arg1, &arg2);
    switch (argc) {
      case 1:
	  len = dbst->len;
	  beg = 0;
	  break;
      case 2:
	if (rb_range_beg_len(arg1, &beg, &len, dbst->len, 1)) {
	    break;
	}
	/* fall through */
      case 3:
	beg = NIL_P(arg1)?0:NUM2LONG(arg1);
	if (beg < 0) {
	    beg += dbst->len;
	    if (beg < 0) beg = 0;
	}
	len = NIL_P(arg2)?dbst->len - beg:NUM2LONG(arg2);
	break;
    }
    tmp[1] = item;
    for (i = 0; i < len; i++) {
	tmp[0] = INT2NUM(i + beg);
	bdb_put(2, tmp, obj);
	if ((i + beg) >= dbst->len) dbst->len++;
    }
    return obj;
}

static VALUE
bdb_sary_cmp(VALUE obj, VALUE obj2)
{
    bdb_DB *dbst, *dbst2 = 0;
    VALUE a, a2, tmp, ary;
    long i, len;

    if (obj == obj2) return INT2FIX(0);
    GetDB(obj, dbst);
    len = dbst->len;
    if (!rb_obj_is_kind_of(obj2, bdb_cRecnum)) {
	obj2 = rb_convert_type(obj2, T_ARRAY, "Array", "to_ary");
	if (len > RARRAY_LEN(obj2)) {
	    len = RARRAY_LEN(obj2);
	}
	ary = Qtrue;
    }
    else {
	GetDB(obj2, dbst2);
	len = dbst->len;
	if (len > dbst2->len) {
	    len = dbst2->len;
	}
	ary = Qfalse;
    }
    for (i = 0; i < len; i++) {
	tmp = INT2NUM(i);
	a = bdb_get(1, &tmp, obj);
	if (ary) {
	    a2 = RARRAY_PTR(obj2)[i];
	}
	else {
	    a2 = bdb_get(1, &tmp, obj2);
	}
	tmp = rb_funcall(a, id_cmp, 1, a2);
	if (tmp != INT2FIX(0)) {
	    return tmp;
	}
    }
    len = dbst->len - ary?RARRAY_LEN(obj2):dbst2->len;
    if (len == 0) return INT2FIX(0);
    if (len > 0) return INT2FIX(1);
    return INT2FIX(-1);
}

static VALUE
bdb_sary_slice_bang(int argc, VALUE *argv, VALUE obj)
{
    VALUE arg1, arg2;
    long pos, len;
    bdb_DB *dbst;

    GetDB(obj, dbst);
    if (rb_scan_args(argc, argv, "11", &arg1, &arg2) == 2) {
	pos = NUM2LONG(arg1);
	len = NUM2LONG(arg2);
      delete_pos_len:
	if (pos < 0) {
	    pos = dbst->len + pos;
	}
	arg2 = bdb_sary_subseq(obj, pos, len);
	bdb_sary_replace(obj, pos, len, Qnil);
	return arg2;
    }

    if (!FIXNUM_P(arg1) && rb_range_beg_len(arg1, &pos, &len, dbst->len, 1)) {
	goto delete_pos_len;
    }

    pos = NUM2LONG(arg1);
    if (pos >= dbst->len) return Qnil;
    if (pos < 0) pos += dbst->len;
    if (pos < 0) return Qnil;

    arg1 = INT2NUM(pos);
    arg2 = bdb_sary_at(obj, arg1);
    if (bdb_del(obj, arg1) != Qnil) dbst->len--;
    return arg2;
}

static VALUE
bdb_sary_plus(VALUE obj, VALUE y)
{
    return rb_ary_plus(bdb_sary_to_a(obj), y);
}

static VALUE
bdb_sary_times(VALUE obj, VALUE y)
{
    return rb_funcall(bdb_sary_to_a(obj), rb_intern("*"), 1, y);
}

static VALUE
bdb_sary_diff(VALUE obj, VALUE y)
{
    return rb_funcall(bdb_sary_to_a(obj), rb_intern("-"), 1, y);
}

static VALUE
bdb_sary_and(VALUE obj, VALUE y)
{
    return rb_funcall(bdb_sary_to_a(obj), rb_intern("&"), 1, y);
}

static VALUE
bdb_sary_or(VALUE obj, VALUE y)
{
    return rb_funcall(bdb_sary_to_a(obj), rb_intern("|"), 1, y);
}

static VALUE
bdb_sary_compact(VALUE obj)
{
    return rb_funcall(bdb_sary_to_a(obj), rb_intern("compact!"), 0, 0);
}

static VALUE
bdb_sary_compact_bang(VALUE obj)
{
    bdb_DB *dbst;
    long i, j;
    VALUE tmp;

    GetDB(obj, dbst);
    j = dbst->len;
    for (i = 0; i < dbst->len; ) {
	tmp = INT2NUM(i);
	tmp = bdb_get(1, &tmp, obj);
	if (NIL_P(tmp)) {
	    bdb_del(obj, INT2NUM(i));
	    dbst->len--;
	}
	else {
	    i++;
	}
    }
    if (dbst->len == j) return Qnil;
    return obj;
}

static VALUE
bdb_sary_nitems(VALUE obj)
{
    bdb_DB *dbst;
    long i, j;
    VALUE tmp;

    GetDB(obj, dbst);
    j = 0;
    for (i = 0; i < dbst->len; ) {
	tmp = INT2NUM(i);
	tmp = bdb_get(1, &tmp, obj);
	if (!NIL_P(tmp)) j++;
    }
    return INT2NUM(j);
}

void bdb_init_recnum()
{
    id_cmp = rb_intern("<=>");
    bdb_cRecnum = rb_define_class_under(bdb_mDb, "Recnum", bdb_cCommon);
    rb_define_singleton_method(bdb_cRecnum, "[]", bdb_sary_s_create, -1);
    rb_define_private_method(bdb_cRecnum, "initialize", bdb_recnum_init, -1);
    rb_define_method(bdb_cRecnum, "[]", bdb_sary_aref, -1);
    rb_define_method(bdb_cRecnum, "get", bdb_sary_aref, -1);
    rb_define_method(bdb_cRecnum, "db_get", bdb_sary_aref, -1);
    rb_define_method(bdb_cRecnum, "[]=", bdb_sary_aset, -1);
    rb_define_method(bdb_cRecnum, "put", bdb_sary_aset, -1);
    rb_define_method(bdb_cRecnum, "db_put", bdb_sary_aset, -1);
    rb_define_method(bdb_cRecnum, "store", bdb_sary_aset, -1);
    rb_define_method(bdb_cRecnum, "at", bdb_sary_at, 1);
    rb_define_method(bdb_cRecnum, "fetch", bdb_sary_fetch, -1);
    rb_define_method(bdb_cRecnum, "first", bdb_sary_first, 0);
    rb_define_method(bdb_cRecnum, "last", bdb_sary_last, 0);
    rb_define_method(bdb_cRecnum, "concat", bdb_sary_concat, 1);
    rb_define_method(bdb_cRecnum, "<<", bdb_sary_push, 1);
    rb_define_method(bdb_cRecnum, "push", bdb_sary_push_m, -1);
    rb_define_method(bdb_cRecnum, "pop", bdb_sary_pop, 0);
    rb_define_method(bdb_cRecnum, "shift", bdb_sary_shift, 0);
    rb_define_method(bdb_cRecnum, "unshift", bdb_sary_unshift_m, -1);
#if HAVE_RB_ARY_INSERT
    rb_define_method(bdb_cRecnum, "insert", bdb_sary_insert, -1);
#endif
    rb_define_method(bdb_cRecnum, "each", bdb_each_value, -1);
    rb_define_method(bdb_cRecnum, "each_index", bdb_each_key, -1);
    rb_define_method(bdb_cRecnum, "reverse_each", bdb_each_eulav, -1);
    rb_define_method(bdb_cRecnum, "length", bdb_sary_length, 0);
    rb_define_alias(bdb_cRecnum,  "size", "length");
    rb_define_method(bdb_cRecnum, "empty?", bdb_sary_empty_p, 0);
    rb_define_method(bdb_cRecnum, "index", bdb_index, 1);
    rb_define_method(bdb_cRecnum, "rindex", bdb_sary_rindex, 1);
    rb_define_method(bdb_cRecnum, "indexes", bdb_sary_indexes, -1);
    rb_define_method(bdb_cRecnum, "indices", bdb_sary_indexes, -1);
    rb_define_method(bdb_cRecnum, "reverse", bdb_sary_reverse_m, 0);
    rb_define_method(bdb_cRecnum, "reverse!", bdb_sary_reverse_bang, 0);
    rb_define_method(bdb_cRecnum, "collect", bdb_sary_collect, -1);
    rb_define_method(bdb_cRecnum, "collect!", bdb_sary_collect_bang, -1);
#if HAVE_RB_ARY_VALUES_AT
    rb_define_method(bdb_cRecnum, "map", bdb_sary_collect, 0);
    rb_define_method(bdb_cRecnum, "select", bdb_sary_select, -1);
    rb_define_method(bdb_cRecnum, "values_at", bdb_sary_values_at, -1);
#endif
    rb_define_method(bdb_cRecnum, "map!", bdb_sary_collect_bang, -1);
    rb_define_method(bdb_cRecnum, "filter", bdb_sary_filter, -1);
    rb_define_method(bdb_cRecnum, "delete", bdb_sary_delete, 1);
    rb_define_method(bdb_cRecnum, "delete_at", bdb_sary_delete_at_m, 1);
    rb_define_method(bdb_cRecnum, "delete_if", bdb_sary_delete_if, 0);
    rb_define_method(bdb_cRecnum, "reject!", bdb_sary_reject_bang, 0);
    rb_define_method(bdb_cRecnum, "replace", bdb_sary_replace_m, 1);
    rb_define_method(bdb_cRecnum, "clear", bdb_sary_clear, -1);
    rb_define_method(bdb_cRecnum, "fill", bdb_sary_fill, -1);
    rb_define_method(bdb_cRecnum, "include?", bdb_has_value, 1);
    rb_define_method(bdb_cRecnum, "<=>", bdb_sary_cmp, 1);
    rb_define_method(bdb_cRecnum, "slice", bdb_sary_aref, -1);
    rb_define_method(bdb_cRecnum, "slice!", bdb_sary_slice_bang, -1);
/*
    rb_define_method(bdb_cRecnum, "assoc", bdb_sary_assoc, 1);
    rb_define_method(bdb_cRecnum, "rassoc", bdb_sary_rassoc, 1);
*/
    rb_define_method(bdb_cRecnum, "+", bdb_sary_plus, 1);
    rb_define_method(bdb_cRecnum, "*", bdb_sary_times, 1);

    rb_define_method(bdb_cRecnum, "-", bdb_sary_diff, 1);
    rb_define_method(bdb_cRecnum, "&", bdb_sary_and, 1);
    rb_define_method(bdb_cRecnum, "|", bdb_sary_or, 1);

/*
    rb_define_method(bdb_cRecnum, "uniq", bdb_sary_uniq, 0);
    rb_define_method(bdb_cRecnum, "uniq!", bdb_sary_uniq_bang, 0);
*/
    rb_define_method(bdb_cRecnum, "compact", bdb_sary_compact, 0);
    rb_define_method(bdb_cRecnum, "compact!", bdb_sary_compact_bang, 0);
/*
    rb_define_method(bdb_cRecnum, "flatten", bdb_sary_flatten, 0);
    rb_define_method(bdb_cRecnum, "flatten!", bdb_sary_flatten_bang, 0);
*/
    rb_define_method(bdb_cRecnum, "nitems", bdb_sary_nitems, 0);
    rb_define_method(bdb_cRecnum, "stat", bdb_tree_stat, -1);
    rb_define_method(bdb_cRecnum, "to_a", bdb_sary_to_a, 0);
    rb_define_method(bdb_cRecnum, "to_ary", bdb_sary_to_a, 0);
    /* RECNO */
    rb_define_method(bdb_cRecno, "shift", bdb_sary_shift, 0);
    rb_define_method(bdb_cRecno, "to_a", bdb_sary_to_a, 0);
    rb_define_method(bdb_cRecno, "to_ary", bdb_sary_to_a, 0);
    rb_define_method(bdb_cRecno, "pop", bdb_sary_pop, 0);
    /* QUEUE */
#if HAVE_CONST_DB_QUEUE
    rb_define_method(bdb_cQueue, "to_a", bdb_sary_to_a, 0);
    rb_define_method(bdb_cQueue, "to_ary", bdb_sary_to_a, 0);
#endif
}
