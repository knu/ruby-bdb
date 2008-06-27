#include "bdb.h"

#if ! HAVE_CONST_DB_GET_BOTH
#define DB_GET_BOTH 9
#endif

static ID id_bt_compare, id_bt_prefix, id_h_hash;

#if HAVE_CONST_DB_DUPSORT
static ID id_dup_compare;
#endif

#if HAVE_ST_DB_SET_APPEND_RECNO
static ID id_append_recno;
#endif

#if HAVE_ST_DB_SET_H_COMPARE
static ID id_h_compare;
#endif

#if HAVE_ST_DB_SET_FEEDBACK
static ID id_feedback;
#endif

static void bdb_mark _((bdb_DB *));

#define GetIdDb(obj_, dbst_) do {				\
    VALUE th = rb_thread_current();				\
								\
    if (!RTEST(th) || !RBASIC(th)->flags) {			\
	rb_raise(bdb_eFatal, "invalid thread object");		\
    }								\
    (obj_) = rb_thread_local_aref(th, bdb_id_current_db);	\
    if (TYPE(obj_) != T_DATA ||					\
	RDATA(obj_)->dmark != (RUBY_DATA_FUNC)bdb_mark) {	\
	rb_raise(bdb_eFatal, "BUG : current_db not set");	\
    }								\
    Data_Get_Struct(obj_, bdb_DB, dbst_);			\
} while (0)

#if HAVE_ST_DB_APP_PRIVATE

#define GetIdDbSec(obj_, dbst_, dbbd_) do {		\
    (obj_) = (VALUE)dbbd_->app_private;			\
    if (!obj) {						\
	GetIdDb(obj_, dbst_);				\
    }							\
    else { 						\
	Data_Get_Struct(obj_, bdb_DB, dbst_);     	\
    }							\
} while (0)

#else

#define GetIdDbSec(obj_, dbst_, dbbd_) do {					\
    GetIdDb(obj_, dbst_);							\
    if (dbbd_ != NULL && dbst_->dbp != dbbd_) {					\
	if (TYPE(dbst_->secondary) == T_ARRAY) {				\
	    int i_;								\
	    VALUE tmp_;								\
										\
	    for (i_ = 0; i_ < RARRAY_LEN(dbst_->secondary); i_++) {		\
		tmp_ = RARRAY_PTR(dbst_->secondary)[i_];			\
		if (TYPE(tmp_) == T_ARRAY && RARRAY_LEN(tmp_) >= 1) {		\
                    VALUE tmp_db = RARRAY_PTR(tmp_)[0];				\
		    if (TYPE(tmp_db) == T_DATA &&				\
			RDATA(tmp_db)->dmark == (RUBY_DATA_FUNC)bdb_mark) {	\
			Data_Get_Struct(tmp_db, bdb_DB, dbst_);			\
			if (dbst_->dbp == dbbd_) {				\
			    (obj_) = tmp_db;					\
			    goto found;						\
			}							\
		    }								\
		}								\
	    }									\
	}									\
	rb_raise(bdb_eFatal, "Invalid reference");				\
    found:									\
	Data_Get_Struct(obj_, bdb_DB, dbst_);					\
    }										\
} while (0)

#endif

VALUE
bdb_respond_to(VALUE obj, ID meth)
{
    return rb_funcall(obj, rb_intern("respond_to?"), 2, ID2SYM(meth), Qtrue);
}

void
bdb_ary_push(struct ary_st *db_ary, VALUE obj)
{
    if (db_ary->mark) {
        rb_warning("db_ary in mark phase");
        return;
    }
    if (db_ary->len == db_ary->total) {
	if (db_ary->total) {
	    REALLOC_N(db_ary->ptr, VALUE, db_ary->total + 5);
	}
	else {
	    db_ary->ptr = ALLOC_N(VALUE, 5);
	}
	db_ary->total += 5;
    }
    db_ary->ptr[db_ary->len] = obj;
    db_ary->len++;
}

void
bdb_ary_unshift(struct ary_st *db_ary, VALUE obj)
{
    if (db_ary->mark) {
        rb_warning("db_ary in mark phase");
        return;
    }
    if (db_ary->len == db_ary->total) {
	if (db_ary->total) {
	    REALLOC_N(db_ary->ptr, VALUE, db_ary->total + 5);
	}
	else { 
	    db_ary->ptr = ALLOC_N(VALUE, 5);
	}
	db_ary->total += 5;
    }
    if (db_ary->len) {
        MEMMOVE(db_ary->ptr + 1, db_ary->ptr, VALUE, db_ary->len);
    }
    db_ary->len++;
    db_ary->ptr[0] = obj;
}

VALUE
bdb_ary_delete(struct ary_st *db_ary, VALUE val)
{
    int i, pos;

    if (!db_ary->ptr || db_ary->mark) return Qfalse;
    for (pos = 0; pos < db_ary->len; pos++) {
	if (db_ary->ptr[pos] == val) {
	    for (i = pos + 1; i < db_ary->len; i++, pos++) {
		db_ary->ptr[pos] = db_ary->ptr[i];
	    }
	    db_ary->len = pos;
	    return Qtrue;
	}
    }
    return Qfalse;
}

void
bdb_ary_mark(struct ary_st *db_ary)
{
    int i;

    for (i = 0; i < db_ary->len; i++) {
        rb_gc_mark(db_ary->ptr[i]);
    }
}

VALUE
bdb_test_dump(VALUE obj, DBT *key, VALUE a, int type_kv)
{
    bdb_DB *dbst;
    int is_nil = 0;
    VALUE tmp = a;

    Data_Get_Struct(obj, bdb_DB, dbst);
    if (dbst->filter[type_kv]) {
	if (FIXNUM_P(dbst->filter[type_kv])) {
	    tmp = rb_funcall(obj, NUM2INT(dbst->filter[type_kv]), 1, a);
	}
	else {
	    tmp = rb_funcall(dbst->filter[type_kv], bdb_id_call, 1, a);
	}
    }
    if (dbst->marshal) {
        if (rb_obj_is_kind_of(tmp, bdb_cDelegate)) {
	    tmp = bdb_deleg_to_orig(tmp);
        }
        tmp = rb_funcall(dbst->marshal, bdb_id_dump, 1, tmp);
        if (TYPE(tmp) != T_STRING) {
	    rb_raise(rb_eTypeError, "dump() must return String");
	}
    }
    else {
        tmp = rb_obj_as_string(tmp);
        if ((dbst->options & BDB_NIL) && a == Qnil) {
            is_nil = 1;
        }
    }
    key->data = StringValuePtr(tmp);
    key->flags &= ~DB_DBT_MALLOC;
    key->size = RSTRING_LEN(tmp) + is_nil;
    return tmp;
}

VALUE
bdb_test_ret(VALUE obj, VALUE tmp, VALUE a, int type_kv)
{
    bdb_DB *dbst;
    Data_Get_Struct(obj, bdb_DB, dbst);
    if (dbst->marshal || a == Qnil) {
	return a;
    }
    else {
        if (dbst->filter[type_kv]) {
            return rb_obj_as_string(a);
        }
        else {
	    return tmp;
	}
    }
}

VALUE
bdb_test_recno(VALUE obj, DBT *key, db_recno_t *recno, VALUE a)
{
    bdb_DB *dbst;
    Data_Get_Struct(obj, bdb_DB, dbst);
    if (RECNUM_TYPE(dbst)) {
        *recno = NUM2INT(a) + dbst->array_base;
        key->data = recno;
        key->size = sizeof(db_recno_t);
	return a;
    }
    else {
        return bdb_test_dump(obj, key, a, FILTER_KEY);
    }
}

VALUE
bdb_test_load(VALUE obj, DBT *a, int type_kv)
{
    VALUE res;
    int posi;
    bdb_DB *dbst;

    posi = type_kv & ~FILTER_FREE;
    Data_Get_Struct(obj, bdb_DB, dbst);
    if (dbst->marshal) {
	res = rb_str_new(a->data, a->size);
	if (dbst->filter[2 + posi]) {
	    if (FIXNUM_P(dbst->filter[2 + posi])) {
		res = rb_funcall(obj,
                                 NUM2INT(dbst->filter[2 + posi]), 1, res);
	    }
	    else {
		res = rb_funcall(dbst->filter[2 + posi], bdb_id_call, 1, res);
	    }
	}
	res = rb_funcall(dbst->marshal, bdb_id_load, 1, res);
    }
    else {
#if HAVE_CONST_DB_QUEUE
	int i;

	if (dbst->type == DB_QUEUE) {
	    for (i = a->size - 1; i >= 0; i--) {
		if (((char *)a->data)[i] != dbst->re_pad)
		    break;
	    }
	    a->size = i + 1;
	}
#endif
	if ((dbst->options & BDB_NIL) &&
            a->size == 1 && ((char *)a->data)[0] == '\000') {
	    res = Qnil;
	}
	else {
            if (a->size == 0 && !(dbst->options & BDB_NIL)) {
                res = Qnil;
            }
            else {
                res = rb_tainted_str_new(a->data, a->size);
                if (dbst->filter[2 + posi]) {
                    if (FIXNUM_P(dbst->filter[2 + posi])) {
                        res = rb_funcall(obj, NUM2INT(dbst->filter[2 + posi]),
                                         1, res);
                    }
                    else {
                        res = rb_funcall(dbst->filter[2 + posi],
                                         bdb_id_call, 1, res);
                    }
                }
            }
	}
    }
    if ((a->flags & DB_DBT_MALLOC) && !(type_kv & FILTER_FREE)) {
	free(a->data);
	a->data = 0;
        a->flags &= ~DB_DBT_MALLOC;
    }
    return res;
}

static VALUE
test_load_dyna1(VALUE obj, DBT *key, DBT *val)
{
    bdb_DB *dbst;
    VALUE del, res, tmp;
    struct deleg_class *delegst;
    
    Data_Get_Struct(obj, bdb_DB, dbst);
    res = bdb_test_load(obj, val, FILTER_VALUE);
    if (dbst->marshal && !SPECIAL_CONST_P(res)) {
	del = Data_Make_Struct(bdb_cDelegate, struct deleg_class, 
			       bdb_deleg_mark, free, delegst);
	delegst->db = obj;
	if (RECNUM_TYPE(dbst)) {
	    tmp = INT2NUM((*(db_recno_t *)key->data) - dbst->array_base);
	}
	else {
	    tmp = rb_str_new(key->data, key->size);
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
	    tmp = rb_funcall(dbst->marshal, bdb_id_load, 1, tmp);
	}
	delegst->key = tmp;
	delegst->obj = res;
	res = del;
    }
    return res;
}

static VALUE
test_load_dyna(VALUE obj, DBT *key, DBT *val)
{
    VALUE res = test_load_dyna1(obj, key, val);
    if (key->flags & DB_DBT_MALLOC) {
	free(key->data);
	key->data = 0;
        key->flags &= ~DB_DBT_MALLOC;
    }
    return res;
}

#define INT_COMPAR 1
#define NUM_COMPAR 2
#define STR_COMPAR 3
#define COMPAR_INT 5
#define COMPAR_NUM 6
#define COMPAR_STR 7

static int
compar_funcall(VALUE av, VALUE bv, int compar)
{
    long ai, bi;
    double ad, bd;

    switch (compar) {
    case INT_COMPAR:
	ai = NUM2INT(rb_Integer(av));
	bi = NUM2INT(rb_Integer(bv));
	if (ai == bi) return 0;
	if (ai > bi) return 1;
	return -1;
    case NUM_COMPAR:
	ad = NUM2DBL(rb_Float(av));
	bd = NUM2DBL(rb_Float(bv));
	if (ad == bd) return 0;
	if (ad > bd) return 1;
	return -1;
    case STR_COMPAR:
	av = rb_obj_as_string(av);
	bv = rb_obj_as_string(bv);
	return strcmp(StringValuePtr(av), StringValuePtr(bv));
    case COMPAR_INT:
	ai = NUM2INT(rb_Integer(av));
	bi = NUM2INT(rb_Integer(bv));
	if (bi == ai) return 0;
	if (bi > ai) return 1;
	return -1;
    case COMPAR_NUM:
	ad = NUM2DBL(rb_Float(av));
	bd = NUM2DBL(rb_Float(bv));
	if (bd == ad) return 0;
	if (bd > ad) return 1;
	return -1;
    case COMPAR_STR:
	av = rb_obj_as_string(av);
	bv = rb_obj_as_string(bv);
	return strcmp(StringValuePtr(bv), StringValuePtr(av));
    default:
	rb_raise(bdb_eFatal, "Invalid comparison function");
    }
    return 0;
}

static int
#if BDB_OLD_FUNCTION_PROTO
bdb_bt_compare(const DBT *a, const DBT *b)
#else
bdb_bt_compare(DB *dbbd, const DBT *a, const DBT *b)
#endif
{
    VALUE obj, av, bv, res;
    bdb_DB *dbst;
#if BDB_OLD_FUNCTION_PROTO
    DB *dbbd = NULL;
#endif

    GetIdDbSec(obj, dbst, dbbd);
    av = bdb_test_load(obj, (DBT *)a, FILTER_VALUE|FILTER_FREE);
    bv = bdb_test_load(obj, (DBT *)b, FILTER_VALUE|FILTER_FREE);
    if (dbst->bt_compare == 0) {
	res = rb_funcall(obj, id_bt_compare, 2, av, bv);
    }
    else {
	if (FIXNUM_P(dbst->bt_compare)) {
	    return compar_funcall(av, bv, NUM2INT(dbst->bt_compare));
	}
	res = rb_funcall(dbst->bt_compare, bdb_id_call, 2, av, bv);
    }
    return NUM2INT(res);
} 

static size_t
#if BDB_OLD_FUNCTION_PROTO
bdb_bt_prefix(const DBT *a, const DBT *b)
#else
bdb_bt_prefix(DB *dbbd, const DBT *a, const DBT *b)
#endif
{
    VALUE obj, av, bv, res;
    bdb_DB *dbst;
#if BDB_OLD_FUNCTION_PROTO
    DB *dbbd = NULL;
#endif

    GetIdDbSec(obj, dbst, dbbd);
    av = bdb_test_load(obj, (DBT *)a, FILTER_VALUE|FILTER_FREE);
    bv = bdb_test_load(obj, (DBT *)b, FILTER_VALUE|FILTER_FREE);
    if (dbst->bt_prefix == 0)
	res = rb_funcall(obj, id_bt_prefix, 2, av, bv);
    else
	res = rb_funcall(dbst->bt_prefix, bdb_id_call, 2, av, bv);
    return NUM2INT(res);
} 

#if HAVE_CONST_DB_DUPSORT

static int
#if BDB_OLD_FUNCTION_PROTO
bdb_dup_compare(const DBT *a, const DBT *b)
#else
bdb_dup_compare(DB *dbbd, const DBT *a, const DBT *b)
#endif
{
    VALUE obj, av, bv, res;
    bdb_DB *dbst;
#if BDB_OLD_FUNCTION_PROTO
    DB *dbbd = NULL;
#endif

    GetIdDbSec(obj, dbst, dbbd);
    av = bdb_test_load(obj, (DBT *)a, FILTER_VALUE|FILTER_FREE);
    bv = bdb_test_load(obj, (DBT *)b, FILTER_VALUE|FILTER_FREE);
    if (dbst->dup_compare == 0) {
	res = rb_funcall(obj, id_dup_compare, 2, av, bv);
    }
    else {
	if (FIXNUM_P(dbst->dup_compare)) {
	    return compar_funcall(av, bv, NUM2INT(dbst->dup_compare));
	}
	res = rb_funcall(dbst->dup_compare, bdb_id_call, 2, av, bv);
    }
    return NUM2INT(res);
}

#endif

static u_int32_t
#if BDB_OLD_FUNCTION_PROTO
bdb_h_hash(const void *bytes, u_int32_t length)
#else
bdb_h_hash(DB *dbbd, const void *bytes, u_int32_t length)
#endif
{
    VALUE obj, st, res;
    bdb_DB *dbst;
#if BDB_OLD_FUNCTION_PROTO
    DB *dbbd = NULL;
#endif

    GetIdDbSec(obj, dbst, dbbd);
    st = rb_tainted_str_new((char *)bytes, length);
    if (dbst->h_hash == 0)
	res = rb_funcall(obj, id_h_hash, 1, st);
    else
	res = rb_funcall(dbst->h_hash, bdb_id_call, 1, st);
    return NUM2UINT(res);
}

#if HAVE_ST_DB_SET_H_COMPARE

static int
bdb_h_compare(DB *dbbd, const DBT *a, const DBT *b)
{
    VALUE obj, av, bv, res;
    bdb_DB *dbst;

    GetIdDbSec(obj, dbst, dbbd);
    av = bdb_test_load(obj, (DBT *)a, FILTER_VALUE|FILTER_FREE);
    bv = bdb_test_load(obj, (DBT *)b, FILTER_VALUE|FILTER_FREE);
    if (dbst->h_compare == 0) {
	res = rb_funcall(obj, id_h_compare, 2, av, bv);
    }
    else {
	if (FIXNUM_P(dbst->h_compare)) {
	    return compar_funcall(av, bv, NUM2INT(dbst->h_compare));
	}
	res = rb_funcall(dbst->h_compare, bdb_id_call, 2, av, bv);
    }
    return NUM2INT(res);
}

#endif

#if HAVE_ST_DB_SET_APPEND_RECNO

static int
bdb_append_recno(DB *dbp, DBT *data, db_recno_t recno)
{
    VALUE res, obj, av, rec;
    bdb_DB *dbst;

    GetIdDbSec(obj, dbst, dbp);
    av = bdb_test_load(obj, data, FILTER_VALUE|FILTER_FREE);
    rec = INT2NUM(recno - dbst->array_base);
    if (dbst->append_recno == 0)
	res = rb_funcall(obj, id_append_recno, 2, rec, av);
    else
	res = rb_funcall(dbst->append_recno, bdb_id_call, 2, rec, av);
    if (!NIL_P(res)) {
	bdb_test_dump(obj, data, res, FILTER_VALUE);
    }
    return 0;
}

#endif

#if HAVE_ST_DB_SET_FEEDBACK

static void
bdb_feedback(DB *dbp, int opcode, int pct)
{
    VALUE obj;
    bdb_DB *dbst;

    GetIdDbSec(obj, dbst, dbp);
    if (NIL_P(dbst->feedback)) {
	return;
    }
    if (dbst->feedback == 0) {
	rb_funcall(obj, id_feedback, 2, INT2NUM(opcode), INT2NUM(pct));
    }
    else {
	rb_funcall(dbst->feedback, bdb_id_call, 2, INT2NUM(opcode), INT2NUM(pct));
    }
}

#endif

static VALUE
compar_func(VALUE value)
{
    value = rb_obj_as_string(value);
    char *compar = StringValuePtr(value);

    if (strcmp(compar, "int_compare") == 0) {
	value = INT2NUM(INT_COMPAR);
    }
    else if (strcmp(compar, "int_compare_desc") == 0) {
	value = INT2NUM(COMPAR_INT);
    }
    else if (strcmp(compar, "numeric_compare") == 0) {
	value = INT2NUM(NUM_COMPAR);
    }
    else if (strcmp(compar, "numeric_compare_desc") == 0) {
	value = INT2NUM(COMPAR_NUM);
    }
    else if (strcmp(compar, "string_compare") == 0) {
	value = INT2NUM(STR_COMPAR);
    }
    else if (strcmp(compar, "string_compare_desc") == 0) {
	value = INT2NUM(STR_COMPAR);
    }
    else {
	rb_raise(bdb_eFatal, "arg must respond to #call");
    }
    return value;
}

static VALUE
bdb_i_options(VALUE obj, VALUE dbstobj)
{
    VALUE key, value;
    char *options, *str;
    DB *dbp;
    bdb_DB *dbst;

    Data_Get_Struct(dbstobj, bdb_DB, dbst);
    key = rb_ary_entry(obj, 0);
    value = rb_ary_entry(obj, 1);
    dbp = dbst->dbp;
    key = rb_obj_as_string(key);
    options = StringValuePtr(key);
    if (strcmp(options, "set_bt_minkey") == 0) {
#if HAVE_TYPE_DB_INFO
	dbst->dbinfo->bt_minkey = NUM2INT(value);
#else
        bdb_test_error(dbp->set_bt_minkey(dbp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_bt_compare") == 0) {
	if (!rb_respond_to(value, bdb_id_call)) {
	    value = compar_func(value);
	}
	dbst->options |= BDB_BT_COMPARE;
	dbst->bt_compare = value;
#if HAVE_TYPE_DB_INFO
	dbst->dbinfo->bt_compare = bdb_bt_compare;
#else
	bdb_test_error(dbp->set_bt_compare(dbp, bdb_bt_compare));
#endif
    }
    else if (strcmp(options, "set_bt_prefix") == 0) {
	if (!rb_respond_to(value, bdb_id_call)) {
	    rb_raise(bdb_eFatal, "arg must respond to #call");
	}
	dbst->options |= BDB_BT_PREFIX;
	dbst->bt_prefix = value;
#if HAVE_TYPE_DB_INFO
	dbst->dbinfo->bt_prefix = bdb_bt_prefix;
#else
	bdb_test_error(dbp->set_bt_prefix(dbp, bdb_bt_prefix));
#endif
    }
    else if (strcmp(options, "set_dup_compare") == 0) {
#if HAVE_CONST_DB_DUPSORT
	if (!rb_respond_to(value, bdb_id_call)) {
	    value = compar_func(value);
	}
	dbst->options |= BDB_DUP_COMPARE;
	dbst->dup_compare = value;
#if HAVE_TYPE_DB_INFO
	dbst->dbinfo->dup_compare = bdb_dup_compare;
#else
	bdb_test_error(dbp->set_dup_compare(dbp, bdb_dup_compare));
#endif
#else
	rb_warning("dup_compare need db >= 2.5.9");
#endif
    }
    else if (strcmp(options, "set_h_hash") == 0) {
	if (!rb_respond_to(value, bdb_id_call)) {
	    rb_raise(bdb_eFatal, "arg must respond to #call");
	}
	dbst->options |= BDB_H_HASH;
	dbst->h_hash = value;
#if HAVE_TYPE_DB_INFO
	dbst->dbinfo->h_hash = bdb_h_hash;
#else
	bdb_test_error(dbp->set_h_hash(dbp, bdb_h_hash));
#endif
    }
#if HAVE_ST_DB_SET_H_COMPARE
    else if (strcmp(options, "set_h_compare") == 0) {
	if (!rb_respond_to(value, bdb_id_call)) {
	    value = compar_func(value);
	}
	dbst->options |= BDB_H_COMPARE;
	dbst->h_compare = value;
	bdb_test_error(dbp->set_h_compare(dbp, bdb_h_compare));
    }
#endif
#if HAVE_ST_DB_SET_APPEND_RECNO
    else if (strcmp(options, "set_append_recno") == 0) {
	if (!rb_respond_to(value, bdb_id_call)) {
	    rb_raise(bdb_eFatal, "arg must respond to #call");
	}
	dbst->options |= BDB_APPEND_RECNO;
	dbst->append_recno = value;
	bdb_test_error(dbp->set_append_recno(dbp, bdb_append_recno));
    }
#endif
    else if (strcmp(options, "set_cachesize") == 0) {
	switch (TYPE(value)) {
	case T_FIXNUM:
	case T_FLOAT:
	case T_BIGNUM:
#if HAVE_TYPE_DB_INFO
	    dbst->dbinfo->db_cachesize = NUM2INT(value);
#else
	    bdb_test_error(dbp->set_cachesize(dbp, 0, NUM2UINT(value), 0));
#endif
	    break;
	default:
	    Check_Type(value, T_ARRAY);
	    if (RARRAY_LEN(value) < 3) { 
		rb_raise(bdb_eFatal, "expected 3 values for cachesize");
	    }
#if HAVE_TYPE_DB_INFO
	    dbst->dbinfo->db_cachesize = NUM2INT(RARRAY_PTR(value)[1]);
#else
	    bdb_test_error(dbp->set_cachesize(dbp, 
					      NUM2INT(RARRAY_PTR(value)[0]),
					      NUM2INT(RARRAY_PTR(value)[1]),
					      NUM2INT(RARRAY_PTR(value)[2])));
#endif
	    break;
	}
    }
    else if (strcmp(options, "set_flags") == 0) {
#if HAVE_TYPE_DB_INFO
	dbst->dbinfo->flags = NUM2UINT(value);
#else
        bdb_test_error(dbp->set_flags(dbp, NUM2UINT(value)));
#endif
	dbst->flags |= NUM2UINT(value);
    }
    else if (strcmp(options, "set_h_ffactor") == 0) {
#if HAVE_TYPE_DB_INFO
	dbst->dbinfo->h_ffactor = NUM2INT(value);
#else
        bdb_test_error(dbp->set_h_ffactor(dbp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_h_nelem") == 0) {
#if HAVE_TYPE_DB_INFO
	dbst->dbinfo->h_nelem = NUM2INT(value);
#else
        bdb_test_error(dbp->set_h_nelem(dbp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_lorder") == 0) {
#if HAVE_TYPE_DB_INFO
	dbst->dbinfo->db_lorder = NUM2INT(value);
#else
        bdb_test_error(dbp->set_lorder(dbp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_pagesize") == 0) {
#if HAVE_TYPE_DB_INFO
	dbst->dbinfo->db_pagesize = NUM2INT(value);
#else
        bdb_test_error(dbp->set_pagesize(dbp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_re_delim") == 0) {
	int ch;
	if (TYPE(value) == T_STRING) {
	    str = StringValuePtr(value);
	    ch = str[0];
	}
	else {
	    ch = NUM2INT(value);
	}
#if HAVE_TYPE_DB_INFO
	dbst->dbinfo->re_delim = ch;
	dbst->dbinfo->flags |= DB_DELIMITER;
#else
        bdb_test_error(dbp->set_re_delim(dbp, ch));
#endif
    }
    else if (strcmp(options, "set_re_len") == 0) {
#if HAVE_TYPE_DB_INFO
	dbst->dbinfo->re_len = NUM2INT(value);
	dbst->dbinfo->flags |= DB_FIXEDLEN;
#else
        bdb_test_error(dbp->set_re_len(dbp, NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_re_pad") == 0) {
	int ch;
	if (TYPE(value) == T_STRING) {
	    str = StringValuePtr(value);
	    ch = str[0];
	}
	else {
	    ch = NUM2INT(value);
	}
#if HAVE_TYPE_DB_INFO
	dbst->dbinfo->re_pad = ch;
	dbst->dbinfo->flags |= DB_PAD;
#else
        bdb_test_error(dbp->set_re_pad(dbp, ch));
#endif
    }
    else if (strcmp(options, "set_re_source") == 0) {
        if (TYPE(value) != T_STRING)
            rb_raise(bdb_eFatal, "re_source must be a filename");
#if HAVE_TYPE_DB_INFO
	dbst->dbinfo->re_source = StringValuePtr(value);
#else
        bdb_test_error(dbp->set_re_source(dbp, StringValuePtr(value)));
#endif
	dbst->options |= BDB_RE_SOURCE;
    }
#if HAVE_ST_DB_SET_Q_EXTENTSIZE
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
	    if (!bdb_respond_to(value, bdb_id_load) ||
		!bdb_respond_to(value, bdb_id_dump)) {
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
	if (RTEST(value)) {
	    dbst->options &= ~BDB_NO_THREAD;
	}
	else {
	    dbst->options |= BDB_NO_THREAD;
	}
    }
    else if (strcmp(options, "set_store_key") == 0) {
	if (!rb_respond_to(value, bdb_id_call)) {
	    rb_raise(bdb_eFatal, "arg must respond to #call");
	}
	dbst->filter[FILTER_KEY] = value;
    }
    else if (strcmp(options, "set_fetch_key") == 0) {
	if (!rb_respond_to(value, bdb_id_call)) {
	    rb_raise(bdb_eFatal, "arg must respond to #call");
	}
	dbst->filter[2 + FILTER_KEY] = value;
    }
    else if (strcmp(options, "set_store_value") == 0) {
	if (!rb_respond_to(value, bdb_id_call)) {
	    rb_raise(bdb_eFatal, "arg must respond to #call");
	}
	dbst->filter[FILTER_VALUE] = value;
    }
    else if (strcmp(options, "set_fetch_value") == 0) {
	if (!rb_respond_to(value, bdb_id_call)) {
	    rb_raise(bdb_eFatal, "arg must respond to #call");
	}
	dbst->filter[2 + FILTER_VALUE] = value;
    }
#if HAVE_ST_DB_SET_ENCRYPT
    else if (strcmp(options, "set_encrypt") == 0) {
	char *passwd;
	int flags = DB_ENCRYPT_AES;
	if (TYPE(value) == T_ARRAY) {
	    if (RARRAY_LEN(value) != 2) {
		rb_raise(bdb_eFatal, "Expected an Array with 2 values");
	    }
	    passwd = StringValuePtr(RARRAY_PTR(value)[0]);
	    flags = NUM2INT(RARRAY_PTR(value)[1]);
	}
	else {
	    passwd = StringValuePtr(value);
	}
	bdb_test_error(dbp->set_encrypt(dbp, passwd, flags));
    }
#endif
#if HAVE_ST_DB_SET_FEEDBACK
    else if (strcmp(options, "set_feedback") == 0) {
	if (!rb_respond_to(value, bdb_id_call)) {
	    rb_raise(bdb_eFatal, "arg must respond to #call");
	}
	dbst->options |= BDB_FEEDBACK;
	dbst->feedback = value;
	dbp->set_feedback(dbp, bdb_feedback);
    }
#endif
    else if (strcmp(options, "store_nil_as_null") == 0) {
        if (RTEST(value)) {
            dbst->options |= BDB_NIL;
        }
        else {
            dbst->options &= ~BDB_NIL;
        }
    }
    return Qnil;
}

struct bdb_eiv {
    VALUE bdb;
    bdb_DB *dbst;
};

static void
bdb_i_close(bdb_DB *dbst, int flags)
{
    bdb_ENV *envst;

    if (dbst->dbp) {
	if (RTEST(dbst->txn) && RBASIC(dbst->txn)->flags) {
	    bdb_TXN *txnst;
	    int opened = 0;

	    Data_Get_Struct(dbst->txn, bdb_TXN, txnst);
	    opened = bdb_ary_delete(&txnst->db_ary, dbst->ori_val);
	    if (!opened) {
		opened = bdb_ary_delete(&txnst->db_assoc, dbst->ori_val);
	    }
	    if (opened) {
		if (txnst->options & BDB_TXN_COMMIT) {
		    rb_funcall2(dbst->txn, rb_intern("commit"), 0, 0);
		}
		else {
		    rb_funcall2(dbst->txn, rb_intern("abort"), 0, 0);
		}
	    }
	    if (!(dbst->options & BDB_NOT_OPEN)) {
		bdb_test_error(dbst->dbp->close(dbst->dbp, flags));
	    }
	}
	else {
	    if (dbst->env && RBASIC(dbst->env)->flags) {
		Data_Get_Struct(dbst->env, bdb_ENV, envst);
		bdb_ary_delete(&envst->db_ary, dbst->ori_val);
	    }
	    if (!(dbst->options & BDB_NOT_OPEN)) {
		bdb_test_error(dbst->dbp->close(dbst->dbp, flags));
	    }
	}
    }
    dbst->dbp = NULL;
}

VALUE
bdb_local_aref()
{
    bdb_DB *dbst;
    VALUE obj;

    GetIdDb(obj, dbst);
    return obj;
}

static VALUE
i_close(bdb_DB *dbst)
{
    bdb_i_close(dbst, 0);
    return Qnil;
}

static VALUE
bdb_final_aref(bdb_DB *dbst)
{
    VALUE obj, th;

    th = rb_thread_current();
    if (RTEST(th) && RBASIC(th)->flags) {
	obj = rb_thread_local_aref(th, bdb_id_current_db);
	if (!NIL_P(obj) && RDATA(obj)->dmark == (RUBY_DATA_FUNC)bdb_mark &&
	    DATA_PTR(obj) == dbst) {
	    rb_thread_local_aset(th, bdb_id_current_db, Qnil);
	}
    }
    return Qnil;
}

static void
bdb_free(bdb_DB *dbst)
{
    if (dbst->dbp && !(dbst->options & BDB_NOT_OPEN)) {
	i_close(dbst);
	bdb_final_aref(dbst);
    }
    free(dbst);
}

static void
bdb_mark(bdb_DB *dbst)
{
    int i;
    rb_gc_mark(dbst->marshal);
    rb_gc_mark(dbst->env);
    rb_gc_mark(dbst->txn);
    rb_gc_mark(dbst->orig);
    rb_gc_mark(dbst->secondary);
    rb_gc_mark(dbst->bt_compare);
    rb_gc_mark(dbst->bt_prefix);
#if HAVE_CONST_DB_DUPSORT
    rb_gc_mark(dbst->dup_compare);
#endif
    for (i = 0; i < 4; i++) {
	rb_gc_mark(dbst->filter[i]);
    }
    rb_gc_mark(dbst->h_hash);
#if HAVE_ST_DB_SET_H_COMPARE
    rb_gc_mark(dbst->h_compare);
#endif
    rb_gc_mark(dbst->filename);
    rb_gc_mark(dbst->database);
#if HAVE_ST_DB_SET_APPEND_RECNO
    rb_gc_mark(dbst->append_recno);
#endif
#if HAVE_ST_DB_SET_FEEDBACK
    rb_gc_mark(dbst->feedback);
#endif
}

static VALUE
bdb_env(VALUE obj)
{
    bdb_DB *dbst;

    GetDB(obj, dbst);
    if (RTEST(dbst->env)) {
	return dbst->env;
    }
    return Qnil;
}

VALUE
bdb_env_p(VALUE obj)
{
    bdb_DB *dbst;

    GetDB(obj, dbst);
    return (RTEST(dbst->env)?Qtrue:Qfalse);
}

static VALUE
bdb_txn(VALUE obj)
{
    bdb_DB *dbst;

    GetDB(obj, dbst);
    if (RTEST(dbst->txn)) {
	return dbst->txn;
    }
    return Qnil;
}

VALUE
bdb_txn_p(VALUE obj)
{
    bdb_DB *dbst;

    GetDB(obj, dbst);
    return (RTEST(dbst->txn)?Qtrue:Qfalse);
}

static VALUE
bdb_close(int argc, VALUE *argv, VALUE obj)
{
    VALUE opt;
    bdb_DB *dbst;
    int flags = 0;

    if (!OBJ_TAINTED(obj) && rb_safe_level() >= 4) {
	rb_raise(rb_eSecurityError, "Insecure: can't close the database");
    }
    Data_Get_Struct(obj, bdb_DB, dbst);
    if (dbst->dbp != NULL) {
	if (rb_scan_args(argc, argv, "01", &opt)) {
	    flags = NUM2INT(opt);
	}
	bdb_i_close(dbst, flags);
    }
    if (RDATA(obj)->dfree != free) {
	dbst->options |= BDB_NOT_OPEN;
	bdb_final_aref(dbst);
	RDATA(obj)->dfree = free;
    }
    return Qnil;
}

#if ! HAVE_ST_DB_BTREE_STAT_BT_NKEYS

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
#if HAVE_DB_CURSOR_4
    bdb_test_error(dbp->cursor(dbp, 0, &dbcp, 0));
#else
    key.flags |= DB_DBT_MALLOC;
    bdb_test_error(dbp->cursor(dbp, 0, &dbcp));
#endif
    do {
	MEMZERO(&data, DBT, 1);
	data.flags = DB_DBT_MALLOC;
        bdb_cache_error(dbcp->c_get(dbcp, &key, &data, DB_NEXT),
			dbcp->c_close(dbcp), ret);
        if (ret == DB_NOTFOUND) {
            dbcp->c_close(dbcp);
            return count;
        }
	if (ret == DB_KEYEMPTY)  {
            dbcp->c_close(dbcp);
	    return -1;
	}
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
    long count;
#if ! HAVE_ST_DB_BTREE_STAT_BT_NKEYS
    long hard;
#endif

#if HAVE_ST_DB_BTREE_STAT_BT_NKEYS
#if HAVE_DB_STAT_4
#if HAVE_DB_STAT_4_TXN
    bdb_test_error(dbp->stat(dbp, NULL, &bdb_stat, 0));
#else
    bdb_test_error(dbp->stat(dbp, &bdb_stat, 0, 0));
#endif
#else
    bdb_test_error(dbp->stat(dbp, &bdb_stat, 0));
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
#if HAVE_DB_STAT_4 && HAVE_CONST_DB_FAST_STAT
    DB_TXN *txnid = NULL;
#endif

    GetDB(obj, dbst);
#if HAVE_CONST_DB_FAST_STAT
#if HAVE_DB_STAT_4
    if (RTEST(dbst->txn)) {
        bdb_TXN *txnst;

        GetTxnDB(dbst->txn, txnst);
        txnid = txnst->txnid;
    }
    bdb_test_error(dbst->dbp->stat(dbst->dbp, txnid, &bdb_stat, DB_FAST_STAT));
#else
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &bdb_stat, DB_FAST_STAT));
#endif
#else
#if HAVE_DB_STAT_4
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &bdb_stat, 0, DB_RECORDCOUNT));
#else
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &bdb_stat, DB_RECORDCOUNT));
#endif
#endif
#if HAVE_ST_DB_BTREE_STAT_BT_NKEYS
    hash = INT2NUM(bdb_stat->bt_nkeys);
#else
    hash = INT2NUM(bdb_stat->bt_nrecs);
#endif
    free(bdb_stat);
    return hash;
}

static VALUE
bdb_s_new(int argc, VALUE *argv, VALUE obj)
{
    VALUE res;
    bdb_TXN *txnst = NULL;
    bdb_ENV *envst = NULL;
    bdb_DB *dbst;
    DB_ENV *envp = 0;

#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
    res = rb_obj_alloc(obj);
#else
    res = rb_funcall2(obj, rb_intern("allocate"), 0, 0);
#endif
    Data_Get_Struct(res, bdb_DB, dbst);
    if (argc && TYPE(argv[argc - 1]) == T_HASH) {
	VALUE v, f = argv[argc - 1];

	if ((v = rb_hash_aref(f, rb_str_new2("txn"))) != RHASH(f)->ifnone) {
	    if (!rb_obj_is_kind_of(v, bdb_cTxn)) {
		rb_raise(bdb_eFatal, "argument of txn must be a transaction");
	    }
	    Data_Get_Struct(v, bdb_TXN, txnst);
	    dbst->txn = v;
	    dbst->env = txnst->env;
	    Data_Get_Struct(txnst->env, bdb_ENV, envst);
	    envp = envst->envp;
	    dbst->options |= envst->options & BDB_NO_THREAD;
	    dbst->marshal = txnst->marshal;
	}
	else if ((v = rb_hash_aref(f, rb_str_new2("env"))) != RHASH(f)->ifnone) {
	    if (!rb_obj_is_kind_of(v, bdb_cEnv)) {
		rb_raise(bdb_eFatal, "argument of env must be an environnement");
	    }
	    Data_Get_Struct(v, bdb_ENV, envst);
	    dbst->env = v;
	    envp = envst->envp;
	    dbst->options |= envst->options & BDB_NO_THREAD;
	    dbst->marshal = envst->marshal;
	}
#if HAVE_CONST_DB_ENCRYPT 
	if (envst && (envst->options & BDB_ENV_ENCRYPT)) {
	    VALUE tmp = rb_str_new2("set_flags");
	    if ((v = rb_hash_aref(f, rb_intern("set_flags"))) != RHASH(f)->ifnone) {
		rb_hash_aset(f, rb_intern("set_flags"), 
			     INT2NUM(NUM2INT(v) | DB_ENCRYPT));
	    }
	    else if ((v = rb_hash_aref(f, tmp)) != RHASH(f)->ifnone) {
		rb_hash_aset(f, tmp, INT2NUM(NUM2INT(v) | DB_ENCRYPT));
	    }
	    else {
		rb_hash_aset(f, tmp, INT2NUM(DB_ENCRYPT));
	    }
	}
#endif
    }
#if HAVE_ST_DB_SET_ERRCALL
    bdb_test_error(db_create(&(dbst->dbp), envp, 0));
    dbst->dbp->set_errpfx(dbst->dbp, "BDB::");
    dbst->dbp->set_errcall(dbst->dbp, bdb_env_errcall);
#endif
    if (bdb_respond_to(obj, bdb_id_load) == Qtrue &&
	bdb_respond_to(obj, bdb_id_dump) == Qtrue) {
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
    rb_obj_call_init(res, argc, argv);
    if (txnst) {
	bdb_ary_push(&txnst->db_ary, res);
    }
    else if (envst) {
	bdb_ary_push(&envst->db_ary, res);
    }
    return res;
}

VALUE
bdb_init(int argc, VALUE *argv, VALUE obj)
{
    bdb_DB *dbst;
    DB *dbp;
    int flags, mode, ret, nb;
    char *name, *subname;
    VALUE a, b, d, e;
    VALUE hash_arg = Qnil;
#if HAVE_TYPE_DB_INFO
    DB_INFO dbinfo;

    MEMZERO(&dbinfo, DB_INFO, 1);
#endif
    Data_Get_Struct(obj, bdb_DB, dbst);
    dbp = dbst->dbp;
#if HAVE_TYPE_DB_INFO
    dbst->dbinfo = &dbinfo;
#endif
#if HAVE_ST_DB_SET_ENCRYPT
    if (rb_const_defined(CLASS_OF(obj), rb_intern("BDB_ENCRYPT"))) {
	char *passwd;
	int flags = DB_ENCRYPT_AES;
	VALUE value = rb_const_get(CLASS_OF(obj), rb_intern("BDB_ENCRYPT"));
	if (TYPE(value) == T_ARRAY) {
	    if (RARRAY_LEN(value) != 2) {
		rb_raise(bdb_eFatal, "Expected an Array with 2 values");
	    }
	    passwd = StringValuePtr(RARRAY_PTR(value)[0]);
	    flags = NUM2INT(RARRAY_PTR(value)[1]);
	}
	else {
	    passwd = StringValuePtr(value);
	}
	bdb_test_error(dbp->set_encrypt(dbp, passwd, flags));
    }
#endif
    if (argc && TYPE(argv[argc - 1]) == T_HASH) {
	hash_arg = argv[argc - 1];
	rb_iterate(rb_each, argv[argc - 1], bdb_i_options, obj);
        argc--;
    }
    mode = flags = 0;
    if (argc) {
	flags = DB_RDONLY;
    }
    a = b = d = e = Qnil;
    switch(nb = rb_scan_args(argc, argv, "04", &a, &b, &d, &e)) {
    case 4:
	mode = NUM2INT(e);
    case 3:
	if (TYPE(d) == T_STRING) {
	    if (strcmp(StringValuePtr(d), "r") == 0)
		flags = DB_RDONLY;
	    else if (strcmp(StringValuePtr(d), "r+") == 0)
		flags = 0;
	    else if (strcmp(StringValuePtr(d), "w") == 0 ||
		     strcmp(StringValuePtr(d), "w+") == 0)
		flags = DB_CREATE | DB_TRUNCATE;
	    else if (strcmp(StringValuePtr(d), "a") == 0 ||
		     strcmp(StringValuePtr(d), "a+") == 0)
		flags = DB_CREATE;
	    else {
		rb_raise(bdb_eFatal, "flags must be r, r+, w, w+, a or a+");
	    }
	}
	else if (d == Qnil)
	    flags = DB_RDONLY;
	else
	    flags = NUM2INT(d);
    }
    name = subname = NULL;
    if (!NIL_P(a)) {
	SafeStringValue(a);
        name = StringValuePtr(a);
    }
    if (!NIL_P(b)) {
	SafeStringValue(b);
        subname = StringValuePtr(b);
    }
    if (dbst->bt_compare == 0 && rb_respond_to(obj, id_bt_compare) == Qtrue) {
	dbst->options |= BDB_BT_COMPARE;
#if HAVE_TYPE_DB_INFO
	dbst->dbinfo->bt_compare = bdb_bt_compare;
#else
	bdb_test_error(dbp->set_bt_compare(dbp, bdb_bt_compare));
#endif
    }
    if (dbst->bt_prefix == 0 && rb_respond_to(obj, id_bt_prefix) == Qtrue) {
	dbst->options |= BDB_BT_PREFIX;
#if HAVE_TYPE_DB_INFO
	dbst->dbinfo->bt_prefix = bdb_bt_prefix;
#else
	bdb_test_error(dbp->set_bt_prefix(dbp, bdb_bt_prefix));
#endif
    }
#if HAVE_CONST_DB_DUPSORT
    if (dbst->dup_compare == 0 && rb_respond_to(obj, id_dup_compare) == Qtrue) {
	dbst->options |= BDB_DUP_COMPARE;
#if HAVE_TYPE_DB_INFO
	dbst->dbinfo->dup_compare = bdb_dup_compare;
#else
	bdb_test_error(dbp->set_dup_compare(dbp, bdb_dup_compare));
#endif
    }
#endif
    if (dbst->h_hash == 0 && rb_respond_to(obj, id_h_hash) == Qtrue) {
	dbst->options |= BDB_H_HASH;
#if HAVE_TYPE_DB_INFO
	dbst->dbinfo->h_hash = bdb_h_hash;
#else
	bdb_test_error(dbp->set_h_hash(dbp, bdb_h_hash));
#endif
    }
#if HAVE_ST_DB_SET_H_COMPARE
    if (dbst->h_compare == 0 && rb_respond_to(obj, id_h_compare) == Qtrue) {
	dbst->options |= BDB_H_COMPARE;
	bdb_test_error(dbp->set_h_compare(dbp, bdb_h_compare));
    }
#endif
#if HAVE_ST_DB_SET_APPEND_RECNO
    if (dbst->append_recno == 0 && rb_respond_to(obj, id_append_recno) == Qtrue) {
	dbst->options |= BDB_APPEND_RECNO;
	bdb_test_error(dbp->set_append_recno(dbp, bdb_append_recno));
    }
#endif
#if HAVE_ST_DB_SET_FEEDBACK
    if (dbst->feedback == 0 && rb_respond_to(obj, id_feedback) == Qtrue) {
	dbp->set_feedback(dbp, bdb_feedback);
	dbst->options |= BDB_FEEDBACK;
    }
#endif
    if (flags & DB_TRUNCATE) {
	rb_secure(2);
    }
    if (flags & DB_CREATE) {
	rb_secure(4);
    }
    if (rb_safe_level() >= 4) {
	flags |= DB_RDONLY;
    }
#if HAVE_CONST_DB_DUPSORT
    if (dbst->options & BDB_DUP_COMPARE) {
#if HAVE_TYPE_DB_INFO
	dbst->dbinfo->flags = DB_DUP | DB_DUPSORT;
#else
        bdb_test_error(dbp->set_flags(dbp, DB_DUP | DB_DUPSORT));
#endif
    }
#endif
#ifndef BDB_NO_THREAD_COMPILE
    if (!(dbst->options & (BDB_RE_SOURCE | BDB_NO_THREAD))) {
	flags |= DB_THREAD;
    }
#endif
    if (dbst->options & BDB_NEED_CURRENT) {
	rb_thread_local_aset(rb_thread_current(), bdb_id_current_db, obj);
    }

#if ! HAVE_ST_DB_OPEN
    {
	bdb_ENV *envst;
	DB_ENV *envp = NULL;
	if (dbst->env) {
	    Data_Get_Struct(dbst->env, bdb_ENV, envst);
	    envp = envst->envp;
	}

	if (name == NULL && (flags & DB_RDONLY)) {
	    flags &= ~DB_RDONLY;
	}
	    
	if ((ret = db_open(name, dbst->type, flags, mode, envp,
			   dbst->dbinfo, &dbp)) != 0) {
	    if (bdb_errcall) {
		bdb_errcall = 0;
		rb_raise(bdb_eFatal, "%s -- %s", StringValuePtr(bdb_errstr), db_strerror(ret));
	    }
	    else
		rb_raise(bdb_eFatal, "%s", db_strerror(ret));
	}
	dbst->dbp = dbp;
    }
#else
    {
#if HAVE_DB_OPEN_7
	DB_TXN *txnid = NULL;
#endif

	if (name == NULL && subname == NULL) {
	    if (flags & DB_RDONLY) {
		flags &= ~DB_RDONLY;
	    }
#if HAVE_DB_OPEN_7 
#if HAVE_DB_OPEN_7_DB_CREATE
	    flags |= DB_CREATE;
#endif
#endif
	}
#if HAVE_DB_OPEN_7
	if (RTEST(dbst->txn)) {
	    bdb_TXN *txnst;

	    GetTxnDB(dbst->txn, txnst);
	    txnid = txnst->txnid;
	}
	else if (RTEST(dbst->env)) {
	    bdb_ENV *envst;

	    GetEnvDB(dbst->env, envst);
#if HAVE_CONST_DB_AUTO_COMMIT
	    if (envst->options & BDB_AUTO_COMMIT) {
		flags |= DB_AUTO_COMMIT;
		dbst->options |= BDB_AUTO_COMMIT;
	    }
	}
#endif
	ret = dbp->open(dbp, txnid, name, subname, dbst->type, flags, mode);
#else
	ret = dbp->open(dbp, name, subname, dbst->type, flags, mode);
#endif
	if (ret != 0) {
	    dbp->close(dbp, 0);
	    if (bdb_errcall) {
		bdb_errcall = 0;
		rb_raise(bdb_eFatal, "%s -- %s", StringValuePtr(bdb_errstr), db_strerror(ret));
	    }
	    else {
		rb_raise(bdb_eFatal, "%s", db_strerror(ret));
	    }
	}
    }
#endif
    dbst->options &= ~BDB_NOT_OPEN;
    if (dbst->env) {
	bdb_ENV *envst;
	Data_Get_Struct(dbst->env, bdb_ENV, envst);
	dbst->options |= envst->options & BDB_INIT_LOCK;
    }
    dbst->filename = dbst->database = Qnil;
    if (name) {
	dbst->filename = rb_tainted_str_new2(name);
	OBJ_FREEZE(dbst->filename);
    }
#if HAVE_ST_DB_OPEN
    if (subname) {
	dbst->database = rb_tainted_str_new2(subname);
	OBJ_FREEZE(dbst->database);
    }
#endif
    dbst->len = -2;
    if (dbst->type == DB_UNKNOWN) {
#if ! HAVE_ST_DB_GET_TYPE
	dbst->type = dbst->dbp->type;
#else
#if HAVE_TYPE_DBTYPE
	{
	    DBTYPE new_type;
#if HAVE_DB_GET_TYPE_2
	    bdb_test_error(dbst->dbp->get_type(dbst->dbp, &new_type));
#else
	    new_type = dbst->dbp->get_type(dbst->dbp);
#endif
	    dbst->type = (int)new_type;
	}
#else
	dbst->type = dbst->dbp->get_type(dbst->dbp);
#endif
#endif
	switch(dbst->type) {
	case DB_BTREE:
	    RBASIC(obj)->klass = bdb_cBtree;
	    break;
	case DB_HASH:
	    RBASIC(obj)->klass = bdb_cHash;
	    break;
	case DB_RECNO:
	{
	    long count;

	    rb_warning("It's hard to distinguish Recnum with Recno for all versions of Berkeley DB");
	    if ((count = bdb_is_recnum(dbst->dbp)) != -1) {
		RBASIC(obj)->klass = bdb_cRecnum;
		dbst->len = count;
	    }
	    else {
		RBASIC(obj)->klass = bdb_cRecno;
	    }
	    break;
	}
#if HAVE_CONST_DB_QUEUE
	case DB_QUEUE:
	    RBASIC(obj)->klass = bdb_cQueue;
	    break;
#endif
	default:
	    dbst->dbp->close(dbst->dbp, 0);
            dbst->dbp = NULL;
	    rb_raise(bdb_eFatal, "Unknown DB type");
	}
    }
    if (dbst->len == -2 && rb_obj_is_kind_of(obj, bdb_cRecnum)) {
	long count;

	if ((count = bdb_is_recnum(dbst->dbp)) != -1) {
	    VALUE len = bdb_recno_length(obj);
	    dbst->len = NUM2LONG(len);
	}
	else {
	    if (flags & DB_TRUNCATE) {
		dbst->len = 0;
	    }
	    else {
                dbst->dbp->close(dbst->dbp, 0);
                dbst->dbp = NULL;
		rb_raise(bdb_eFatal, "database is not a Recnum");
	    }
	}
    }
#if HAVE_ST_DB_APP_PRIVATE
    dbst->dbp->app_private = (void *)obj;
#endif
    return obj;
}

static VALUE
bdb_s_alloc(obj)
    VALUE obj;
{
    VALUE res, cl;
    bdb_DB *dbst;

    res = Data_Make_Struct(obj, bdb_DB, bdb_mark, bdb_free, dbst);
    dbst->options = BDB_NOT_OPEN;
    cl = obj;
    while (cl) {
	if (cl == bdb_cBtree || RCLASS(cl)->m_tbl == RCLASS(bdb_cBtree)->m_tbl) {
	    dbst->type = DB_BTREE; 
	    break;
	}
	if (cl == bdb_cRecnum || RCLASS(cl)->m_tbl == RCLASS(bdb_cRecnum)->m_tbl) {
	    dbst->type = DB_RECNO; 
	    break;
	}
	else if (cl == bdb_cHash || RCLASS(cl)->m_tbl == RCLASS(bdb_cHash)->m_tbl) {
	    dbst->type = DB_HASH; 
	    break;
	}
	else if (cl == bdb_cRecno || RCLASS(cl)->m_tbl == RCLASS(bdb_cRecno)->m_tbl) {
	    dbst->type = DB_RECNO;
	    break;
    }
#if HAVE_CONST_DB_QUEUE
	else if (cl == bdb_cQueue || RCLASS(cl)->m_tbl == RCLASS(bdb_cQueue)->m_tbl) {
	    dbst->type = DB_QUEUE;
	    break;
	}
#endif
	else if (cl == bdb_cUnknown || RCLASS(cl)->m_tbl == RCLASS(bdb_cUnknown)->m_tbl) {
	    dbst->type = DB_UNKNOWN;
	    break;
	}
	cl = RCLASS_SUPER(cl);
    }
    if (!cl) {
	rb_raise(bdb_eFatal, "unknown database type");
    }
    dbst->ori_val = res;
    return res;
}

static VALUE
bdb_i_s_create(obj, db)
    VALUE obj, db;
{
    VALUE tmp[2];
    tmp[0] = rb_ary_entry(obj, 0);
    tmp[1] = rb_ary_entry(obj, 1);
    bdb_put(2, tmp, db);
    return Qnil;
} 

static VALUE
bdb_s_create(int argc, VALUE *argv, VALUE obj)
{
    VALUE res;
    int i;

    res = rb_funcall2(obj, rb_intern("new"), 0, 0);
    if (argc == 1 && TYPE(argv[0]) == T_HASH) {
	rb_iterate(rb_each, argv[0], bdb_i_s_create, res);
	return res;
    }
    if (argc % 2 != 0) {
        rb_raise(rb_eArgError, "odd number args for %s", rb_class2name(obj));
    }
    for (i = 0; i < argc; i += 2) {
        bdb_put(2, argv + i, res);
    }
    return res;
}

static VALUE
bdb_s_open(int argc, VALUE *argv, VALUE obj)
{
    VALUE res = rb_funcall2(obj, rb_intern("new"), argc, argv);
    if (rb_block_given_p()) {
	return rb_ensure(rb_yield, res, bdb_protect_close, res);
    }
    return res;
}

#if HAVE_CONST_DB_QUEUE

struct re {
    int re_len, re_pad;
};

static VALUE
bdb_queue_i_search_re_len(obj, restobj)
    VALUE obj, restobj;
{
    VALUE key, value;
    char *str;
    struct re *rest;

    Data_Get_Struct(restobj, struct re, rest);
    key = rb_ary_entry(obj, 0);
    value = rb_ary_entry(obj, 1);
    key = rb_obj_as_string(key);
    if (strcmp(StringValuePtr(key), "set_re_len") == 0) {
	rest->re_len = NUM2INT(value);
    }
    else if (strcmp(StringValuePtr(key), "set_re_pad") == 0) {
	int ch;
	if (TYPE(value) == T_STRING) {
	    str = StringValuePtr(value);
	    ch = str[0];
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
bdb_queue_s_new(int argc, VALUE *argv, VALUE obj) 
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
bdb_append_internal(argc, argv, obj, flag, retval)
    int argc, flag;
    VALUE *argv, obj;
{
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBT key, data;
    db_recno_t recno;
    int i;
    VALUE *a, ary = Qnil;
    volatile VALUE res = Qnil;

    rb_secure(4);
    if (argc < 1)
	return obj;
    INIT_TXN(txnid, obj, dbst);
#if HAVE_CONST_DB_AUTO_COMMIT
    if (txnid == NULL && (dbst->options & BDB_AUTO_COMMIT)) {
      flag |= DB_AUTO_COMMIT;
    }
#endif
    MEMZERO(&key, DBT, 1);
    recno = 1;
    key.data = &recno;
    key.size = sizeof(db_recno_t);
#if DB_APPEND
    if (flag & DB_APPEND) {
	key.flags |= DB_DBT_MALLOC;
    }
#endif
    if (retval) {
	ary = rb_ary_new();
    }
    for (i = 0, a = argv; i < argc; i++, a++) {
	MEMZERO(&data, DBT, 1);
	res = bdb_test_dump(obj, &data, *a, FILTER_VALUE);
	SET_PARTIAL(dbst, data);
#if HAVE_CONST_DB_QUEUE
	if (dbst->type == DB_QUEUE && dbst->re_len < data.size) {
	    rb_raise(bdb_eFatal, "size > re_len for Queue");
	}
#endif
	bdb_test_error(dbst->dbp->put(dbst->dbp, txnid, &key, &data, flag));
	if (retval) {
	    rb_ary_push(ary, INT2NUM(*(db_recno_t *)key.data));
	}
    }
    if (retval) {
	return ary;
    }
    return obj;
}

static VALUE
bdb_append_m(int argc, VALUE *argv, VALUE obj)
{
    return bdb_append_internal(argc, argv, obj, DB_APPEND, Qtrue);
}

static VALUE
bdb_append(obj, val)
    VALUE val, obj;
{
    return bdb_append_internal(1, &val, obj, DB_APPEND, Qfalse);
}

static VALUE
bdb_unshift(int argc, VALUE *argv, VALUE obj)
{
    bdb_DB *dbst;
    DB_TXN *txnid;
    int flag;

    INIT_TXN(txnid, obj, dbst);
    if (dbst->flags & DB_RENUMBER) {
	flag = 0;
    }
    else {
	flag = DB_NOOVERWRITE;
    }
    return bdb_append_internal(argc, argv, obj, flag, Qtrue);
}

VALUE
bdb_put(int argc, VALUE *argv, VALUE obj)
{
    volatile VALUE a0 = Qnil;
    volatile VALUE b0 = Qnil;
    VALUE a, b, c;
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBT key, data;
    int ret, flags;
    db_recno_t recno;

    rb_secure(4);
    INIT_TXN(txnid, obj, dbst);
    flags = 0;
    a = b = c = Qnil;
    MEMZERO(&key, DBT, 1);
    MEMZERO(&data, DBT, 1);
    if (rb_scan_args(argc, argv, "21", &a, &b, &c) == 3) {
        flags = NUM2INT(c);
    }
    a0 = bdb_test_recno(obj, &key, &recno, a);
    b0 = bdb_test_dump(obj, &data, b, FILTER_VALUE);
    SET_PARTIAL(dbst, data);
#if HAVE_CONST_DB_QUEUE
    if (dbst->type == DB_QUEUE && dbst->re_len < data.size) {
	rb_raise(bdb_eFatal, "size > re_len for Queue");
    }
#endif
#if HAVE_CONST_DB_AUTO_COMMIT
    if (txnid == NULL && (dbst->options & BDB_AUTO_COMMIT)) {
      flags |= DB_AUTO_COMMIT;
    }
#endif
    ret = bdb_test_error(dbst->dbp->put(dbst->dbp, txnid, &key, &data, flags));
    if (ret == DB_KEYEXIST)
	return Qfalse;
    else {
	if (dbst->partial) {
	    if (flags & DB_APPEND) {
		a = INT2NUM((long)key.data);
	    }
	    return bdb_get(1, &a, obj);
	}
	else {
	    return bdb_test_ret(obj, b0, b, FILTER_VALUE);
	}
    }
}

static VALUE
bdb_aset(obj, a, b)
    VALUE obj, a, b;
{
    VALUE tmp[2];
    tmp[0] = a;
    tmp[1] = b;
    bdb_put(2, tmp, obj);
    return b;
}

VALUE
bdb_test_load_key(obj, key)
    VALUE obj;
    DBT *key;
{
    bdb_DB *dbst;
    Data_Get_Struct(obj, bdb_DB, dbst);
    if (RECNUM_TYPE(dbst))
        return INT2NUM((*(db_recno_t *)key->data) - dbst->array_base);
    return bdb_test_load(obj, key, FILTER_KEY);
}

VALUE
bdb_assoc(obj, key, data)
    VALUE obj;
    DBT *key, *data;
{
    return rb_assoc_new(bdb_test_load_key(obj, key), 
			bdb_test_load(obj, data, FILTER_VALUE));
}

VALUE
bdb_assoc_dyna(obj, key, data)
    VALUE obj;
    DBT *key, *data;
{
    VALUE k, v;
    int to_free = key->flags & DB_DBT_MALLOC;

    key->flags &= ~DB_DBT_MALLOC;
    k = bdb_test_load_key(obj, key);
    v = test_load_dyna1(obj, key, data);
    if (to_free) {
        free(key->data);
	key->data = 0;
    }
    return rb_assoc_new(k, v);
}

#if HAVE_ST_DB_PGET

static VALUE
bdb_assoc2(obj, skey, pkey, data)
    VALUE obj;
    DBT *skey, *pkey, *data;
{
    return rb_assoc_new(
	rb_assoc_new(bdb_test_load_key(obj, skey), bdb_test_load_key(obj, pkey)),
	bdb_test_load(obj, data, FILTER_VALUE));
}

#endif

VALUE
bdb_assoc3(obj, skey, pkey, data)
    VALUE obj;
    DBT *skey, *pkey, *data;
{
    return rb_ary_new3(3, bdb_test_load_key(obj, skey), 
		       bdb_test_load_key(obj, pkey),
		       bdb_test_load(obj, data, FILTER_VALUE));
}

static VALUE bdb_has_both _((VALUE, VALUE, VALUE));
#if (BDB_VERSION < 30100) || ! HAVE_CONST_DB_GET_BOTH
#define CANT_DB_CURSOR_GET_BOTH 1
static VALUE bdb_has_both_internal _((VALUE, VALUE, VALUE, VALUE));
#else
#define CANT_DB_CURSOR_GET_BOTH 0
#endif

static VALUE
bdb_get_internal(argc, argv, obj, notfound, dyna)
    int argc;
    VALUE *argv;
    VALUE obj;
    VALUE notfound;
    int dyna;
{
    VALUE a = Qnil;
    VALUE b = Qnil;
    VALUE c;
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBT key, data;
    int flagss;
    int ret, flags;
    db_recno_t recno;
    void *tmp_data = 0;

    INIT_TXN(txnid, obj, dbst);
    flagss = flags = 0;
    MEMZERO(&key, DBT, 1);
    MEMZERO(&data, DBT, 1);
    data.flags |= DB_DBT_MALLOC;
    switch(rb_scan_args(argc, argv, "12", &a, &b, &c)) {
    case 3:
        flags = NUM2INT(c);
        if ((flags & ~DB_RMW) == DB_GET_BOTH) {
#if ! HAVE_CONST_DB_GET_BOTH
	    return bdb_has_both_internal(obj, a, b, Qtrue);
#else
            b = bdb_test_dump(obj, &data, b, FILTER_VALUE);
	    data.flags |= DB_DBT_MALLOC;
	    tmp_data = data.data;
#endif
        }
	break;
    case 2:
	flagss = flags = NUM2INT(b);
	break;
    }
    a = bdb_test_recno(obj, &key, &recno, a);
    SET_PARTIAL(dbst, data);
    flags |= TEST_INIT_LOCK(dbst);
#if HAVE_ST_DB_OPEN
#if HAVE_DB_KEY_DBT_MALLOC
    {
	void *tmp_key = key.data;
	key.flags |= DB_DBT_MALLOC;
#endif
#endif
	ret = bdb_test_error(dbst->dbp->get(dbst->dbp, txnid, &key, &data, flags));
	if (ret == DB_NOTFOUND || ret == DB_KEYEMPTY)
	    return notfound;
#if HAVE_ST_DB_OPEN
#if HAVE_DB_KEY_DBT_MALLOC
	if (tmp_key == key.data) {
	    key.flags &= ~DB_DBT_MALLOC;
	}
    }
#endif
#endif
    if (((flags & ~DB_RMW) == DB_GET_BOTH) ||
	((flags & ~DB_RMW) == DB_SET_RECNO)) {
	if (tmp_data == data.data) {
	    data.flags &= ~DB_DBT_MALLOC;
	}
        return bdb_assoc(obj, &key, &data);
    }
    if (dyna) {
	return test_load_dyna(obj, &key, &data);
    }
    else {
	return bdb_test_load(obj, &data, FILTER_VALUE);
    }
}

VALUE
bdb_get(int argc, VALUE *argv, VALUE obj)
{
    return bdb_get_internal(argc, argv, obj, Qnil, 0);
}

static VALUE
bdb_get_dyna(int argc, VALUE *argv, VALUE obj)
{
    return bdb_get_internal(argc, argv, obj, Qnil, 1);
}

static VALUE
bdb_fetch(int argc, VALUE *argv, VALUE obj)
{
    VALUE key, if_none;
    VALUE val;

    rb_scan_args(argc, argv, "11", &key, &if_none);
    val = bdb_get_internal(1, argv, obj, Qundef, 1);
    if (val == Qundef) {
	if (rb_block_given_p()) {
	    if (argc > 1) {
		rb_raise(rb_eArgError, "wrong # of arguments");
	    }
	    return rb_yield(key);
	}
	if (argc == 1) {
	    rb_raise(rb_eIndexError, "key not found");
	}
	return if_none;
    }
    return val;
}

#if HAVE_ST_DB_PGET

static VALUE
bdb_pget(int argc, VALUE *argv, VALUE obj)
{
    VALUE a = Qnil;
    VALUE b = Qnil;
    VALUE c;
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBT pkey, data, skey;
    int flagss;
    int ret, flags;
    db_recno_t srecno;
    void *tmp_data = 0;

    INIT_TXN(txnid, obj, dbst);
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
            b = bdb_test_dump(obj, &data, b, FILTER_VALUE);
	    data.flags |= DB_DBT_MALLOC;
	    tmp_data = data.data;
        }
	break;
    case 2:
	flagss = flags = NUM2INT(b);
	break;
    }
    a = bdb_test_recno(obj, &skey, &srecno, a);
    SET_PARTIAL(dbst, data);
    flags |= TEST_INIT_LOCK(dbst);
    ret = bdb_test_error(dbst->dbp->pget(dbst->dbp, txnid, &skey, &pkey, &data, flags));
    if (ret == DB_NOTFOUND || ret == DB_KEYEMPTY)
        return Qnil;
    if (((flags & ~DB_RMW) == DB_GET_BOTH) ||
	((flags & ~DB_RMW) == DB_SET_RECNO)) {
	if ((data.flags & DB_DBT_MALLOC) && tmp_data == data.data) {
	    data.flags &= ~DB_DBT_MALLOC;
	}
        return bdb_assoc2(obj, &skey, &pkey, &data);
    }
    return bdb_assoc(obj, &pkey, &data);
}

#endif

#if HAVE_TYPE_DB_KEY_RANGE

static VALUE
bdb_btree_key_range(obj, a)
    VALUE obj, a;
{
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBT key;
    db_recno_t recno;
    DB_KEY_RANGE key_range;
    volatile VALUE b = Qnil;

    INIT_TXN(txnid, obj, dbst);
    MEMZERO(&key, DBT, 1);
    b = bdb_test_recno(obj, &key, &recno, a);
    bdb_test_error(dbst->dbp->key_range(dbst->dbp, txnid, &key, &key_range, 0));
    return rb_struct_new(bdb_sKeyrange, 
			 rb_float_new(key_range.less),
			 rb_float_new(key_range.equal), 
			 rb_float_new(key_range.greater));
}
#endif

#if HAVE_TYPE_DB_COMPACT

struct data_flags {
    DB_COMPACT *cdata;
    int flags;
};

static VALUE
bdb_compact_i(obj, dataobj)
    VALUE obj, dataobj;
{
    VALUE key, value;
    char *str;
    struct data_flags *dtf;

    Data_Get_Struct(dataobj, struct data_flags, dtf);
    key = rb_ary_entry(obj, 0);
    value = rb_ary_entry(obj, 1);
    key = rb_obj_as_string(key);
    str = StringValuePtr(key);
    if (strcmp(str, "compact_timeout") == 0) {
	dtf->cdata->compact_timeout = NUM2LONG(value);
    }
    else if (strcmp(str, "compact_fillpercent") == 0) {
	dtf->cdata->compact_fillpercent = NUM2INT(value);
    }
    else if (strcmp(str, "flags") == 0) {
	dtf->flags = NUM2INT(value);
    }
    else {
	rb_warning("Unknown option %s", str);
    }
    return Qnil;
}
    
static VALUE
bdb_treerec_compact(int argc, VALUE *argv, VALUE obj)
{
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBT start, stop, end;
    DBT *pstart, *pstop;
    DB_COMPACT cdata;
    db_recno_t recno_start, recno_stop;
    int flags;
    VALUE a, b, c, result;

    pstop = pstart = NULL;
    MEMZERO(&cdata, DB_COMPACT, 1);
    flags = 0;
    INIT_TXN(txnid, obj, dbst);
    switch (rb_scan_args(argc, argv, "03", &a, &b, &c)) {
    case 3:
	if (FIXNUM_P(c)) {
	    flags = NUM2INT(c);
	}
	else {
	    struct data_flags *dtf;
	    VALUE dtobj;
		
	    dtobj = Data_Make_Struct(rb_cData, struct data_flags, 0, free, dtf);
	    dtf->cdata = &cdata;
	    dtf->flags = 0;
	    rb_iterate(rb_each, c, bdb_compact_i, dtobj);
	    flags = dtf->flags;
	}
	/* ... */
    case 2:
	if (!NIL_P(b)) {
	    MEMZERO(&stop, DBT, 1);
	    b = bdb_test_recno(obj, &start, &recno_stop, b);
	    pstop = &stop;
	}
	/* ... */
    case 1:
	if (!NIL_P(a)) {
	    MEMZERO(&start, DBT, 1);
	    a = bdb_test_recno(obj, &start, &recno_start, a);
	    pstart = &start;
	}
    }
    MEMZERO(&end, DBT, 1);
    bdb_test_error(dbst->dbp->compact(dbst->dbp, txnid, pstart, pstop, &cdata,
				      flags, &end));
    result = rb_hash_new();
    rb_hash_aset(result, rb_tainted_str_new2("end"), bdb_test_load_key(obj, &end));
    rb_hash_aset(result, rb_tainted_str_new2("compact_deadlock"),
		 INT2NUM(cdata.compact_deadlock));
    rb_hash_aset(result, rb_tainted_str_new2("compact_levels"),
		 INT2NUM(cdata.compact_levels));
    rb_hash_aset(result, rb_tainted_str_new2("compact_pages_free"),
		 INT2NUM(cdata.compact_pages_free));
    rb_hash_aset(result, rb_tainted_str_new2("compact_pages_examine"),
		 INT2NUM(cdata.compact_pages_examine));
    rb_hash_aset(result, rb_tainted_str_new2("compact_pages_truncated"),
		 INT2NUM(cdata.compact_pages_truncated));
    return result;
}

#endif

#if HAVE_ST_DBC_C_GET

#if HAVE_CONST_DB_NEXT_DUP

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
    volatile VALUE b = Qnil;

    INIT_TXN(txnid, obj, dbst);
    MEMZERO(&key, DBT, 1);
    b = bdb_test_recno(obj, &key, &recno, a);
    MEMZERO(&data, DBT, 1);
    data.flags |= DB_DBT_MALLOC;
    SET_PARTIAL(dbst, data);
#if HAVE_DB_CURSOR_4
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#endif
    flags27 = TEST_INIT_LOCK(dbst);
    bdb_cache_error(dbcp->c_get(dbcp, &key, &data, DB_SET | flags27),
		    dbcp->c_close(dbcp), ret);
    if (ret == DB_NOTFOUND) {
        dbcp->c_close(dbcp);
        return INT2NUM(0);
    }
#if HAVE_ST_DBC_C_COUNT
    bdb_cache_error(dbcp->c_count(dbcp, &count, 0), dbcp->c_close(dbcp), ret);
    dbcp->c_close(dbcp);
    return INT2NUM(count);
#else
    count = 1;
    while (1) {
	MEMZERO(&data, DBT, 1);
	data.flags |= DB_DBT_MALLOC;
	SET_PARTIAL(dbst, data);
        bdb_cache_error(dbcp->c_get(dbcp, &key, &data, DB_NEXT_DUP | flags27),
			dbcp->c_close(dbcp), ret);
        if (ret == DB_NOTFOUND) {
            dbcp->c_close(dbcp);
            return INT2NUM(count);
        }
        if (ret == DB_KEYEMPTY) continue;
        FREE_KEY(dbst, key);
        if (data.flags & DB_DBT_MALLOC) {
            free(data.data);
	    data.data = 0;
	}
        count++;
    }
    return INT2NUM(-1);
#endif
}

#endif

#if HAVE_CONST_DB_CONSUME

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
    INIT_TXN(txnid, obj, dbst);
    MEMZERO(&key, DBT, 1);
    MEMZERO(&data, DBT, 1);
    recno = 1;
    key.data = &recno;
    key.size = sizeof(db_recno_t);
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
    bdb_cache_error(dbcp->c_get(dbcp, &key, &data, DB_CONSUME),
		    dbcp->c_close(dbcp), ret);
    dbcp->c_close(dbcp);
    if (ret == DB_NOTFOUND) {
	return Qnil;
    }
    return bdb_assoc(obj, &key, &data);
}

#endif

#endif

static VALUE
bdb_has_key(obj, key)
    VALUE obj, key;
{
    return (bdb_get_internal(1, &key, obj, Qundef, 0) == Qundef)?Qfalse:Qtrue;
}

#if CANT_DB_CURSOR_GET_BOTH

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
    volatile VALUE c = Qnil;
    volatile VALUE d = Qnil;

    INIT_TXN(txnid, obj, dbst);
    flagss = flags = 0;
    MEMZERO(&key, DBT, 1);
    MEMZERO(&data, DBT, 1);
    MEMZERO(&datas, DBT, 1);
    c = bdb_test_recno(obj, &key, &recno, a);
    d = bdb_test_dump(obj, &datas, b, FILTER_VALUE);
    data.flags |= DB_DBT_MALLOC;
    SET_PARTIAL(dbst, data);
    flags |= TEST_INIT_LOCK(dbst);
#if HAVE_DB_CURSOR_4
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#endif
    bdb_cache_error(dbcp->c_get(dbcp, &key, &data, DB_SET),
		    dbcp->c_close(dbcp), ret);
    if (ret == DB_NOTFOUND || ret == DB_KEYEMPTY) {
	dbcp->c_close(dbcp);
        return (flag == Qtrue)?Qnil:Qfalse;
    }
    if (datas.size == data.size &&
	memcmp(datas.data, data.data, data.size) == 0) {
	dbcp->c_close(dbcp);
	if (flag == Qtrue) {
	    return bdb_assoc(obj, &key, &data);
	}
	else {
	    FREE_KEY(dbst, key);
	    free(data.data);
	    return Qtrue;
	}
    }
#if ! HAVE_CONST_DB_NEXT_DUP
    FREE_KEY(dbst, key);
    free(data.data);
    dbcp->c_close(dbcp);
    return (flag == Qtrue)?Qnil:Qfalse;
#else
#if CANT_DB_CURSOR_GET_BOTH
    if (RECNUM_TYPE(dbst)) {
	free(data.data);
	dbcp->c_close(dbcp);
	return Qfalse;
    }
#endif
    while (1) {
	FREE_KEY(dbst, key);
	free(data.data);
	MEMZERO(&data, DBT, 1);
	data.flags |= DB_DBT_MALLOC;
	bdb_cache_error(dbcp->c_get(dbcp, &key, &data, DB_NEXT_DUP),
			dbcp->c_close(dbcp), ret);
	if (ret == DB_NOTFOUND || ret == DB_KEYEMPTY) {
	    dbcp->c_close(dbcp);
	    return Qfalse;
	}
	if (datas.size == data.size &&
	    memcmp(datas.data, data.data, data.size) == 0) {
	    FREE_KEY(dbst, key);
	    free(data.data);
	    dbcp->c_close(dbcp);
	    return Qtrue;
	}
    }
    return Qfalse;
#endif
}

#endif

static VALUE
bdb_has_both(obj, a, b)
    VALUE obj, a, b;
{
#if ! HAVE_CONST_DB_GET_BOTH
    return bdb_has_both_internal(obj, a, b, Qfalse);
#else
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBT key, data;
    int ret, flags;
    db_recno_t recno;
    volatile VALUE c = Qnil;
    volatile VALUE d = Qnil;
    void *tmp_key, *tmp_data;

    INIT_TXN(txnid, obj, dbst);
#if CANT_DB_CURSOR_GET_BOTH
    if (RECNUM_TYPE(dbst)) {
	return bdb_has_both_internal(obj, a, b, Qfalse);
    }
#endif
    MEMZERO(&key, DBT, 1);
    MEMZERO(&data, DBT, 1);
    c = bdb_test_recno(obj, &key, &recno, a);
    d = bdb_test_dump(obj, &data, b, FILTER_VALUE);
    data.flags |= DB_DBT_MALLOC;
    SET_PARTIAL(dbst, data);
    flags = DB_GET_BOTH | TEST_INIT_LOCK(dbst);
    tmp_key = key.data;
    tmp_data = data.data;
#if HAVE_ST_DB_OPEN
#if HAVE_DB_KEY_DBT_MALLOC
    key.flags |= DB_DBT_MALLOC;
#endif
#endif
    ret = bdb_test_error(dbst->dbp->get(dbst->dbp, txnid, &key, &data, flags));
    if (ret == DB_NOTFOUND || ret == DB_KEYEMPTY)
        return Qfalse;
    if (tmp_data != data.data) {
	free(data.data);
    }
    if ((key.flags & DB_DBT_MALLOC) && tmp_key != key.data) {
	free(key.data);
    }
    return Qtrue;
#endif
}

VALUE
bdb_del(obj, a)
    VALUE a, obj;
{
    bdb_DB *dbst;
    DB_TXN *txnid;
    int flag = 0;
    DBT key;
    int ret;
    db_recno_t recno;
    volatile VALUE b = Qnil;

    rb_secure(4);
    INIT_TXN(txnid, obj, dbst);
#if HAVE_CONST_DB_AUTO_COMMIT
    if (txnid == NULL && (dbst->options & BDB_AUTO_COMMIT)) {
        flag |= DB_AUTO_COMMIT;
    }
#endif
    MEMZERO(&key, DBT, 1);
    b = bdb_test_recno(obj, &key, &recno, a);
    ret = bdb_test_error(dbst->dbp->del(dbst->dbp, txnid, &key, flag));
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

    INIT_TXN(txnid, obj, dbst);
    MEMZERO(&key, DBT, 1);
    INIT_RECNO(dbst, key, recno);
    MEMZERO(&data, DBT, 1);
    data.flags = DB_DBT_MALLOC;
#if HAVE_DB_CURSOR_4
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#endif
    SET_PARTIAL(dbst, data);
    flags = TEST_INIT_LOCK(dbst);
    bdb_cache_error(dbcp->c_get(dbcp, &key, &data, DB_FIRST | flags),
		    dbcp->c_close(dbcp), ret);
    if (ret == DB_NOTFOUND) {
        dbcp->c_close(dbcp);
        return Qtrue;
    }
    FREE_KEY(dbst, key);
    free(data.data);
    dbcp->c_close(dbcp);
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

    INIT_TXN(txnid, obj, dbst);
    value = 0;
#if HAVE_DB_CURSOR_4
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#endif
    SET_PARTIAL(dbst, data);
    flags = TEST_INIT_LOCK(dbst);
    do {
	MEMZERO(&key, DBT, 1);
	INIT_RECNO(dbst, key, recno);
	MEMZERO(&data, DBT, 1);
	data.flags = DB_DBT_MALLOC;
	SET_PARTIAL(dbst, data);
        bdb_cache_error(dbcp->c_get(dbcp, &key, &data, DB_NEXT | flags),
			dbcp->c_close(dbcp), ret);
        if (ret == DB_NOTFOUND) {
            dbcp->c_close(dbcp);
            return INT2NUM(value);
        }
	if (ret == DB_KEYEMPTY) continue;
	FREE_KEY(dbst, key);
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
#if HAVE_CONST_DB_MULTIPLE_KEY
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
#if HAVE_CONST_DB_MULTIPLE_KEY
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
    VALUE res = Qnil;

    GetDB(st->db, dbst);
    dbcp = st->dbcp;
    switch (st->type) {
    case BDB_ST_DUPU:
	FREE_KEY(dbst, *key);
	res = bdb_test_load(st->db, data, FILTER_VALUE);
	if (TYPE(st->replace) == T_ARRAY) {
	    rb_ary_push(st->replace, res);
	}
	else {
	    rb_yield(res);
	}
	break;
    case BDB_ST_DUPKV:
	res = bdb_assoc_dyna(st->db, key, data);
	if (TYPE(st->replace) == T_ARRAY) {
	    rb_ary_push(st->replace, res);
	}
	else {
	    rb_yield(res);
	}
	break;
    case BDB_ST_DUPVAL:
	res = test_load_dyna(st->db, key, data);
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
            data->flags &= ~DB_DBT_MALLOC;
	    data->data = 0;
	}
	rb_yield(bdb_test_load_key(st->db, key));
	break;

    case BDB_ST_VALUE:
	FREE_KEY(dbst, *key);
	res = rb_yield(bdb_test_load(st->db, data, FILTER_VALUE));
	if (st->replace == Qtrue) {
	    MEMZERO(data, DBT, 1);
	    res = bdb_test_dump(st->db, data, res, FILTER_VALUE);
	    SET_PARTIAL(dbst, *data);
	    bdb_test_error(dbcp->c_put(dbcp, key, data, DB_CURRENT));
	}
	else if (st->replace != Qfalse) {
	    rb_ary_push(st->replace, res);
	}
	break;
     case BDB_ST_SELECT:
	res = bdb_assoc(st->db, key, data);
	if (RTEST(rb_yield(res))) {
	    rb_ary_push(st->replace, res);
	}
	break;
    case BDB_ST_KV:
	if (st->primary) {
	    rb_yield(bdb_assoc3(st->db, key, pkey, data));
	}
	else {
	    rb_yield(bdb_assoc_dyna(st->db, key, data));
	}
	break;
    case BDB_ST_REJECT:
    {
	VALUE ary = bdb_assoc(st->db, key, data);
	if (!RTEST(rb_yield(ary))) {
	    rb_hash_aset(st->replace, RARRAY_PTR(ary)[0], RARRAY_PTR(ary)[1]);
	}
	break;
    }
    case BDB_ST_DELETE:
	if (RTEST(rb_yield(bdb_assoc(st->db, key, data)))) {
	    bdb_test_error(dbcp->c_del(dbcp, 0));
	}
	break;
    }
}

static int
bdb_i_last_prefix(dbcp, key, pkey, data, orig, st)
    DBC *dbcp;
    DBT *key, *pkey, *data, *orig;
    eachst *st;
{
    int ret, flags = DB_LAST;

    while (1) {
#if HAVE_CONST_DB_MULTIPLE_KEY
	if (st->type == BDB_ST_KV && st->primary) {
	    ret = bdb_test_error(dbcp->c_pget(dbcp, key, pkey, data, DB_LAST));
	}
	else
#endif
	{
#if HAVE_CURSOR_C_GET_KEY_MALLOC
	    key->flags |= DB_DBT_MALLOC;
#endif
	    ret = bdb_test_error(dbcp->c_get(dbcp, key, data, flags));
#if HAVE_CURSOR_C_GET_KEY_MALLOC
	    key->flags &= ~DB_DBT_MALLOC;
#endif
	}
	flags = DB_PREV;
	if (ret == DB_NOTFOUND) {
	    return ret;
	}
	if (key->size >= orig->size &&
	    !memcmp(key->data, orig->data, orig->size)) {
	    return 0;
	}
	if (memcmp(key->data, orig->data, 
		   (key->size > orig->size)?orig->size:key->size) < 0) {
	    return DB_NOTFOUND;
	}
    }
}

static VALUE
bdb_i_each_kv(st)
    eachst *st;
{
    bdb_DB *dbst;
    DBC *dbcp;
    DBT pkey, key, data, orig;
    int ret, init = Qfalse, prefix = Qfalse;
    db_recno_t recno;
    volatile VALUE res = Qnil;
    
    prefix = st->type & BDB_ST_PREFIX;
    st->type &= ~BDB_ST_PREFIX;
    GetDB(st->db, dbst);
    dbcp = st->dbcp;
    MEMZERO(&key, DBT, 1);
    INIT_RECNO(dbst, key, recno);
    MEMZERO(&orig, DBT, 1);
    MEMZERO(&data, DBT, 1);
    data.flags = DB_DBT_MALLOC;
    SET_PARTIAL(dbst, data);
    MEMZERO(&pkey, DBT, 1);
    pkey.flags = DB_DBT_MALLOC;
    if (!NIL_P(st->set)) {
	res = bdb_test_recno(st->db, &key, &recno, st->set);
        if (prefix) {
            init = Qtrue;
            orig.size = key.size;
            orig.data = ALLOCA_N(char, key.size);
            MEMCPY(orig.data, key.data, char, key.size);
        }
	if (prefix && st->sens == DB_PREV) {
	    ret = bdb_i_last_prefix(dbcp, &key, &pkey, &data, &orig, st);
	}
	else {
#if HAVE_CONST_DB_MULTIPLE_KEY
	    if (st->type == BDB_ST_KV && st->primary) {
		ret = bdb_test_error(dbcp->c_pget(dbcp, &key, &pkey, &data, 
						  (st->type & BDB_ST_DUP)?DB_SET:
						  DB_SET_RANGE));
	    }
	    else
#endif
	    {
#if HAVE_CURSOR_C_GET_KEY_MALLOC
		key.flags |= DB_DBT_MALLOC;
#endif
		MEMZERO(&data, DBT, 1);
		data.flags = DB_DBT_MALLOC;
		SET_PARTIAL(dbst, data);
		ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, 
                                             (st->type & BDB_ST_DUP)?DB_SET:
                                             DB_SET_RANGE));
#if HAVE_CURSOR_C_GET_KEY_MALLOC
		key.flags &= ~DB_DBT_MALLOC;
#endif
	    }
	}
	if (ret == DB_NOTFOUND) {
	    return Qfalse;
	}
        if (prefix) {
            if (key.size >= orig.size &&
                !memcmp(key.data, orig.data, orig.size)) {
                bdb_treat(st, &pkey, &key, &data);
            }
	    else {
		return Qfalse;
	    }
	}
	else {
	    bdb_treat(st, &pkey, &key, &data);
	}
    }
    do {
#if HAVE_CONST_DB_MULTIPLE_KEY
	if (st->type == BDB_ST_KV && st->primary) {
	    ret = bdb_test_error(dbcp->c_pget(dbcp, &key, &pkey, &data, st->sens));
	}
	else
#endif
	{
#if HAVE_CURSOR_C_GET_KEY_MALLOC
	    key.flags |= DB_DBT_MALLOC;
#endif
	    MEMZERO(&data, DBT, 1);
	    data.flags = DB_DBT_MALLOC;
	    SET_PARTIAL(dbst, data);
	    ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, st->sens));
#if HAVE_CURSOR_C_GET_KEY_MALLOC
	    key.flags &= ~DB_DBT_MALLOC;
#endif
	}
        if (ret == DB_NOTFOUND) {
            return Qnil;
        }
	if (ret == DB_KEYEMPTY) continue;
        if (prefix) {
            if (!init) {
                init = Qtrue;
                orig.size = key.size;
                orig.data = ALLOCA_N(char, key.size);
                MEMCPY(orig.data, key.data, char, key.size);
            }
            if (key.size >= orig.size &&
                !memcmp(key.data, orig.data, orig.size)) {
                bdb_treat(st, &pkey, &key, &data);
            }
        }
        else {
            bdb_treat(st, &pkey, &key, &data);
        }
    } while (1);
    return Qnil;
}

#if HAVE_CONST_DB_MULTIPLE_KEY

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
    void *p;
    volatile VALUE res = Qnil;
    
    GetDB(st->db, dbst);
    dbcp = st->dbcp;
    MEMZERO(&key, DBT, 1);
    MEMZERO(&pkey, DBT, 1);
    MEMZERO(&rkey, DBT, 1);
    INIT_RECNO(dbst, key, recno);
    MEMZERO(&data, DBT, 1);
    MEMZERO(&rdata, DBT, 1);
    st->data = data.data = (void *)ALLOC_N(char, st->len);
    data.ulen = st->len;
    data.flags = DB_DBT_USERMEM;
    SET_PARTIAL(dbst, data);
    SET_PARTIAL(dbst, rdata);
    init = 1;
    do {
	if (init && !NIL_P(st->set)) {
	    res = bdb_test_recno(st->db, &key, &recno, st->set);
	    ret = bdb_test_error(dbcp->c_get(dbcp, &key, &data, 
                                             ((st->type & BDB_ST_DUP)?DB_SET:
                                              DB_SET_RANGE)|DB_MULTIPLE_KEY));
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
    int flags = 0;

    if (argc && TYPE(argv[argc - 1]) == T_HASH) {
	VALUE g, f = argv[argc - 1];
	if ((g = rb_hash_aref(f, rb_intern("flags"))) != RHASH(f)->ifnone ||
	    (g = rb_hash_aref(f, rb_str_new2("flags"))) != RHASH(f)->ifnone) {
	    flags = NUM2INT(g);
	}
	argc--;
    }
	
    MEMZERO(&st, eachst, 1);

#if HAVE_CONST_DB_MULTIPLE_KEY
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
    if ((type & ~BDB_ST_PREFIX) == BDB_ST_DELETE) {
	rb_secure(4);
    }
    INIT_TXN(txnid, obj, dbst);
#if HAVE_DB_CURSOR_4
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, flags));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#endif
    st.db = obj;
    st.dbcp = dbcp;
    st.sens = sens | TEST_INIT_LOCK(dbst);
    st.replace = replace;
    st.type = type & ~BDB_ST_ONE;
#if HAVE_CONST_DB_MULTIPLE_KEY
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

#if HAVE_CONST_DB_NEXT_DUP

static VALUE
bdb_get_dup(int argc, VALUE *argv, VALUE obj)
{
    VALUE result = Qfalse;
    if (!rb_block_given_p()) {
	result = rb_ary_new();
    }
    return bdb_each_kvc(argc, argv, obj, DB_NEXT_DUP, result, BDB_ST_DUPU);
}

static VALUE
bdb_common_each_dup(int argc, VALUE *argv, VALUE obj)
{
    if (!rb_block_given_p()) {
	rb_raise(bdb_eFatal, "each_dup called out of an iterator");
    }
    return bdb_each_kvc(argc, argv, obj, DB_NEXT_DUP, Qfalse, BDB_ST_DUPKV);
}

static VALUE
bdb_common_each_dup_val(int argc, VALUE *argv, VALUE obj)
{
    if (!rb_block_given_p()) {
	rb_raise(bdb_eFatal, "each_dup called out of an iterator");
    }
    return bdb_each_kvc(argc, argv, obj, DB_NEXT_DUP, Qfalse, BDB_ST_DUPVAL);
}

static VALUE
bdb_common_dups(int argc, VALUE *argv, VALUE obj)
{
    int flags = BDB_ST_DUPKV;
    VALUE result = rb_ary_new();
    if (argc > 1) {
	if (RTEST(argv[argc - 1])) {
	    flags = BDB_ST_DUPKV;
	}
	else {
	    flags = BDB_ST_DUPVAL;
	}
	argc--;
    }
    return bdb_each_kvc(argc, argv, obj, DB_NEXT_DUP, result, flags);
}

#endif

static VALUE
bdb_delete_if(int argc, VALUE *argv, VALUE obj)
{
    return bdb_each_kvc(argc, argv, obj, DB_NEXT, Qfalse, BDB_ST_DELETE | BDB_ST_ONE);
}

static VALUE
bdb_reject(int argc, VALUE *argv, VALUE obj)
{
    return bdb_each_kvc(argc, argv, obj, DB_NEXT, rb_hash_new(), BDB_ST_REJECT);
}

VALUE
bdb_each_value(int argc, VALUE *argv, VALUE obj)
{ 
    return bdb_each_kvc(argc, argv, obj, DB_NEXT, Qfalse, BDB_ST_VALUE);
}

VALUE
bdb_each_eulav(int argc, VALUE *argv, VALUE obj)
{
    return bdb_each_kvc(argc, argv, obj, DB_PREV, Qfalse, BDB_ST_VALUE | BDB_ST_ONE);
}

VALUE
bdb_each_key(int argc, VALUE *argv, VALUE obj)
{ 
    return bdb_each_kvc(argc, argv, obj, DB_NEXT, Qfalse, BDB_ST_KEY); 
}

static VALUE
bdb_each_yek(int argc, VALUE *argv, VALUE obj) 
{ 
    return bdb_each_kvc(argc, argv, obj, DB_PREV, Qfalse, BDB_ST_KEY | BDB_ST_ONE);
}

static VALUE
bdb_each_pair(int argc, VALUE *argv, VALUE obj) 
{
    return bdb_each_kvc(argc, argv, obj, DB_NEXT, Qfalse, BDB_ST_KV);
}

static VALUE
bdb_each_prefix(int argc, VALUE *argv, VALUE obj) 
{
    return bdb_each_kvc(argc, argv, obj, DB_NEXT, Qfalse, BDB_ST_KV | BDB_ST_PREFIX);
}

static VALUE
bdb_each_riap(int argc, VALUE *argv, VALUE obj) 
{
    return bdb_each_kvc(argc, argv, obj, DB_PREV, Qfalse, BDB_ST_KV | BDB_ST_ONE);
}

static VALUE
bdb_each_xiferp(int argc, VALUE *argv, VALUE obj) 
{
    return bdb_each_kvc(argc, argv, obj, DB_PREV, Qfalse, 
                        BDB_ST_KV | BDB_ST_ONE | BDB_ST_PREFIX);
}

static VALUE
bdb_each_pair_prim(int argc, VALUE *argv, VALUE obj) 
{
    VALUE tmp[2] = {Qnil, Qtrue};
    rb_scan_args(argc, argv, "01", tmp);
    return bdb_each_kvc(2, tmp, obj, DB_NEXT, Qfalse, BDB_ST_KV);
}

static VALUE
bdb_each_riap_prim(int argc, VALUE *argv, VALUE obj) 
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

    INIT_TXN(txnid, obj, dbst);
#if HAVE_DB_CURSOR_4
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#endif
    flags = ((flag == Qnil)?DB_PREV:DB_NEXT) | TEST_INIT_LOCK(dbst);
    do {
	MEMZERO(&key, DBT, 1);
	INIT_RECNO(dbst, key, recno);
	MEMZERO(&data, DBT, 1);
	data.flags = DB_DBT_MALLOC;
	SET_PARTIAL(dbst, data);
        bdb_cache_error(dbcp->c_get(dbcp, &key, &data, flags),
			dbcp->c_close(dbcp), ret);
        if (ret == DB_NOTFOUND) {
            dbcp->c_close(dbcp);
            return result;
        }
	if (ret == DB_KEYEMPTY) continue;
	switch (TYPE(result)) {
	case T_ARRAY:
	    if (flag == Qtrue) {
		rb_ary_push(result, bdb_assoc(obj, &key, &data));
	    }
	    else {
		rb_ary_push(result, bdb_test_load(obj, &data, FILTER_VALUE));
	    }
	    break;
	case T_HASH:
	    if (flag == Qtrue) {
		rb_hash_aset(result, bdb_test_load_key(obj, &key), 
			     bdb_test_load(obj, &data, FILTER_VALUE));
	    }
	    else {
		rb_hash_aset(result, bdb_test_load(obj, &data, FILTER_VALUE), 
			     bdb_test_load_key(obj, &key));
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
bdb_update_i(pair, obj)
    VALUE pair, obj;
{
    Check_Type(pair, T_ARRAY);
    if (RARRAY_LEN(pair) < 2) {
	rb_raise(rb_eArgError, "pair must be [key, value]");
    }
    bdb_put(2, RARRAY_PTR(pair), obj);
    return Qnil;
}

static VALUE
each_pair(obj)
    VALUE obj;
{
    return rb_funcall(obj, rb_intern("each_pair"), 0, 0);
}

static VALUE
bdb_update(obj, other)
    VALUE obj, other;
{
    rb_iterate(each_pair, other, bdb_update_i, obj);
    return obj;
}

VALUE
bdb_clear(int argc, VALUE *argv, VALUE obj)
{
#if HAVE_ST_DB_TRUNCATE
    bdb_DB *dbst;
    DB_TXN *txnid;
    unsigned int count = 0;
#endif
    int flags = 0;

    rb_secure(4);
#if HAVE_ST_DB_TRUNCATE
    INIT_TXN(txnid, obj, dbst);
#if HAVE_CONST_DB_AUTO_COMMIT
    if (txnid == NULL && (dbst->options & BDB_AUTO_COMMIT)) {
      flags |= DB_AUTO_COMMIT;
    }
#endif
    bdb_test_error(dbst->dbp->truncate(dbst->dbp, txnid, &count, flags));
    return INT2NUM(count);
#else
    flags = 0;
    if (argc && TYPE(argv[argc - 1]) == T_HASH) {
	VALUE g, f = argv[argc - 1];
	if ((g = rb_hash_aref(f, rb_intern("flags"))) != RHASH(f)->ifnone ||
	    (g = rb_hash_aref(f, rb_str_new2("flags"))) != RHASH(f)->ifnone) {
	    flags = NUM2INT(g);
	}
	argc--;
    }
    if (argc) {
	flags = NUM2INT(argv[0]);
    }
    return bdb_lgth_intern(obj, Qtrue, flags);
#endif
}

static VALUE
bdb_replace(int argc, VALUE *argv, VALUE obj)
{
    VALUE g;
    int flags;

    if (argc == 0 || argc > 2) {
	rb_raise(rb_eArgError, "invalid number of arguments (0 for 1)");
    }
    flags = 0;
    if (TYPE(argv[argc - 1]) == T_HASH) {
	VALUE f = argv[argc - 1];
	if ((g = rb_hash_aref(f, rb_intern("flags"))) != RHASH(f)->ifnone ||
	    (g = rb_hash_aref(f, rb_str_new2("flags"))) != RHASH(f)->ifnone) {
	    flags = NUM2INT(g);
	}
	argc--;
    }
    if (argc == 2) {
	flags = NUM2INT(argv[1]);
    }
    g = INT2FIX(flags);
    bdb_clear(1, &g, obj);
    rb_iterate(each_pair, argv[0], bdb_update_i, obj);
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
    INIT_TXN(txnid, obj, dbst);
#if HAVE_DB_CURSOR_4
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#endif
    flags = DB_NEXT | TEST_INIT_LOCK(dbst);
    do {
	MEMZERO(&key, DBT, 1);
	INIT_RECNO(dbst, key, recno);
	MEMZERO(&data, DBT, 1);
	data.flags = DB_DBT_MALLOC;
	SET_PARTIAL(dbst, data);
        bdb_cache_error(dbcp->c_get(dbcp, &key, &data, flags),
			dbcp->c_close(dbcp), ret);
        if (ret == DB_NOTFOUND) {
            dbcp->c_close(dbcp);
            return ary;
        }
	if (ret == DB_KEYEMPTY) continue;
	switch (type) {
	case BDB_ST_VALUE:
	    FREE_KEY(dbst, key);
	    rb_ary_push(ary, bdb_test_load(obj, &data, FILTER_VALUE));
	    break;
	case BDB_ST_KEY:
	    free(data.data);
	    rb_ary_push(ary, bdb_test_load_key(obj, &key));
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

    INIT_TXN(txnid, obj, dbst);
    MEMZERO(&key, DBT, 1);
    INIT_RECNO(dbst, key, recno);
#if HAVE_DB_CURSOR_4
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp, 0));
#else
    bdb_test_error(dbst->dbp->cursor(dbst->dbp, txnid, &dbcp));
#endif
    flags = sens | TEST_INIT_LOCK(dbst);
    do {
	MEMZERO(&data, DBT, 1);
	data.flags = DB_DBT_MALLOC;
	SET_PARTIAL(dbst, data);
        bdb_cache_error(dbcp->c_get(dbcp, &key, &data, flags),
			dbcp->c_close(dbcp), ret);
        if (ret == DB_NOTFOUND) {
            dbcp->c_close(dbcp);
            return (b == Qfalse)?Qfalse:Qnil;
        }
	if (ret == DB_KEYEMPTY) continue;
	if (rb_equal(a, bdb_test_load(obj, &data, FILTER_VALUE)) == Qtrue) {
	    VALUE d;
	    
	    dbcp->c_close(dbcp);
	    if (b == Qfalse) {
		d = Qtrue;
		FREE_KEY(dbst, key);
	    }
	    else {
		d = bdb_test_load_key(obj, &key);
	    }
	    return  d;
	}
	FREE_KEY(dbst, key);
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
bdb_indexes(int argc, VALUE *argv, VALUE obj)
{
    VALUE indexes;
    int i;

#if HAVE_RB_ARY_VALUES_AT
    rb_warn("Common#%s is deprecated; use Common#values_at",
#if HAVE_RB_FRAME_THIS_FUNC
	    rb_id2name(rb_frame_this_func())
#else
	    rb_id2name(rb_frame_last_func())
#endif
	    );
#endif
    indexes = rb_ary_new2(argc);
    for (i = 0; i < argc; i++) {
	rb_ary_push(indexes, bdb_get(1, &argv[i], obj));
    }
    return indexes;
}

static VALUE
bdb_values_at(int argc, VALUE *argv, VALUE obj)
{
    VALUE result = rb_ary_new2(argc);
    long i;
    for (i = 0; i < argc; i++) {
	rb_ary_push(result, bdb_get(1, &argv[i], obj));
    }
    return result;
}

static VALUE
bdb_select(int argc, VALUE *argv, VALUE obj)
{
    VALUE result = rb_ary_new();

    if (rb_block_given_p()) {
	if (argc > 0) {
	    rb_raise(rb_eArgError, "wrong number arguments(%d for 0)", argc);
	}
	return bdb_each_kvc(argc, argv, obj, DB_NEXT, result, BDB_ST_SELECT);
    }
    rb_warn("Common#select(index..) is deprecated; use Common#values_at");
    return bdb_values_at(argc, argv, obj);
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

#if HAVE_TYPE_DB_HASH_STAT

static VALUE
bdb_hash_stat(int argc, VALUE *argv, VALUE obj)
{
    bdb_DB *dbst;
    DB_HASH_STAT *bdb_stat;
    VALUE hash, flagv;
    int flags = 0;
#if HAVE_DB_STAT_4_TXN
    DB_TXN *txnid = NULL;
#endif

    if (rb_scan_args(argc, argv, "01", &flagv) == 1) {
	flags = NUM2INT(flagv);
    }
    GetDB(obj, dbst);
#if HAVE_DB_STAT_4
#if HAVE_DB_STAT_4_TXN
    if (RTEST(dbst->txn)) {
        bdb_TXN *txnst;

        GetTxnDB(dbst->txn, txnst);
        txnid = txnst->txnid;
    }
    bdb_test_error(dbst->dbp->stat(dbst->dbp, txnid, &bdb_stat, flags));
#else
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &bdb_stat, 0, flags));
#endif
#else
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &bdb_stat, flags));
#endif
    hash = rb_hash_new();
    rb_hash_aset(hash, rb_tainted_str_new2("hash_magic"), INT2NUM(bdb_stat->hash_magic));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_version"), INT2NUM(bdb_stat->hash_version));
    rb_hash_aset(hash, rb_tainted_str_new2("hash_pagesize"), INT2NUM(bdb_stat->hash_pagesize));
#if HAVE_ST_DB_HASH_STAT_HASH_NKEYS
    rb_hash_aset(hash, rb_tainted_str_new2("hash_nkeys"), INT2NUM(bdb_stat->hash_nkeys));
#if ! HAVE_ST_DB_HASH_STAT_HASH_NRECS
    rb_hash_aset(hash, rb_tainted_str_new2("hash_nrecs"), INT2NUM(bdb_stat->hash_nkeys));
#endif
#endif
#if HAVE_ST_DB_HASH_STAT_HASH_NRECS
    rb_hash_aset(hash, rb_tainted_str_new2("hash_nrecs"), INT2NUM(bdb_stat->hash_nrecs));
#endif
#if HAVE_ST_DB_HASH_STAT_HASH_NDATA
    rb_hash_aset(hash, rb_tainted_str_new2("hash_ndata"), INT2NUM(bdb_stat->hash_ndata));
#endif
#if HAVE_ST_DB_HASH_STAT_HASH_NELEM
    rb_hash_aset(hash, rb_tainted_str_new2("hash_nelem"), INT2NUM(bdb_stat->hash_nelem));
#endif
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
#if HAVE_ST_DB_HASH_STAT_HASH_PAGECNT
    rb_hash_aset(hash, rb_tainted_str_new2("hash_pagecnt"), INT2NUM(bdb_stat->hash_pagecnt));
#endif
    free(bdb_stat);
    return hash;
}

#endif

VALUE
bdb_tree_stat(int argc, VALUE *argv, VALUE obj)
{
    bdb_DB *dbst;
    DB_BTREE_STAT *bdb_stat;
    VALUE hash, flagv;
    char pad;
    int flags = 0;
#if HAVE_DB_STAT_4_TXN
    DB_TXN *txnid = NULL;
#endif

    if (rb_scan_args(argc, argv, "01", &flagv) == 1) {
	flags = NUM2INT(flagv);
    }
    GetDB(obj, dbst);
#if HAVE_DB_STAT_4
#if HAVE_DB_STAT_4_TXN
    if (RTEST(dbst->txn)) {
        bdb_TXN *txnst;

        GetTxnDB(dbst->txn, txnst);
        txnid = txnst->txnid;
    }
    bdb_test_error(dbst->dbp->stat(dbst->dbp, txnid, &bdb_stat, flags));
#else
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &bdb_stat, 0, flags));
#endif
#else
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &bdb_stat, flags));
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
#if HAVE_ST_DB_BTREE_STAT_BT_NRECS
    rb_hash_aset(hash, rb_tainted_str_new2("bt_nrecs"), INT2NUM(bdb_stat->bt_nrecs));
#endif
#if HAVE_ST_DB_BTREE_STAT_BT_NKEYS
    rb_hash_aset(hash, rb_tainted_str_new2("bt_nkeys"), INT2NUM(bdb_stat->bt_nkeys));
#if ! HAVE_ST_DB_BTREE_STAT_BT_NRECS
    rb_hash_aset(hash, rb_tainted_str_new2("bt_nrecs"), INT2NUM(bdb_stat->bt_nkeys));
#endif
#endif
#if HAVE_ST_DB_BTREE_STAT_BT_NDATA
    rb_hash_aset(hash, rb_tainted_str_new2("bt_ndata"), INT2NUM(bdb_stat->bt_ndata));
#endif
    rb_hash_aset(hash, rb_tainted_str_new2("bt_over_pg"), INT2NUM(bdb_stat->bt_over_pg));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_over_pgfree"), INT2NUM(bdb_stat->bt_over_pgfree));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_pagesize"), INT2NUM(bdb_stat->bt_pagesize));
    rb_hash_aset(hash, rb_tainted_str_new2("bt_re_len"), INT2NUM(bdb_stat->bt_re_len));
    pad = (char)bdb_stat->bt_re_pad;
    rb_hash_aset(hash, rb_tainted_str_new2("bt_re_pad"), rb_tainted_str_new(&pad, 1));
#if HAVE_ST_DB_BTREE_STAT_BT_PAGECNT
    rb_hash_aset(hash, rb_tainted_str_new2("bt_pagecnt"), INT2NUM(bdb_stat->bt_pagecnt));
#endif
    free(bdb_stat);
    return hash;
}

#if HAVE_TYPE_DB_QUEUE_STAT

static VALUE
bdb_queue_stat(int argc, VALUE *argv, VALUE obj)
{
    bdb_DB *dbst;
    DB_QUEUE_STAT *bdb_stat;
    VALUE hash, flagv;
    char pad;
    int flags = 0;
#if HAVE_DB_STAT_4_TXN
    DB_TXN *txnid = NULL;
#endif

    if (rb_scan_args(argc, argv, "01", &flagv) == 1) {
	flags = NUM2INT(flagv);
    }
    GetDB(obj, dbst);
#if HAVE_DB_STAT_4
#if HAVE_DB_STAT_4_TXN
    if (RTEST(dbst->txn)) {
        bdb_TXN *txnst;

        GetTxnDB(dbst->txn, txnst);
        txnid = txnst->txnid;
    }
    bdb_test_error(dbst->dbp->stat(dbst->dbp, txnid, &bdb_stat, flags));
#else
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &bdb_stat, 0, flags));
#endif
#else
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &bdb_stat, flags));
#endif
    hash = rb_hash_new();
    rb_hash_aset(hash, rb_tainted_str_new2("qs_magic"), INT2NUM(bdb_stat->qs_magic));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_version"), INT2NUM(bdb_stat->qs_version));
#if HAVE_ST_DB_QUEUE_STAT_QS_NKEYS
    rb_hash_aset(hash, rb_tainted_str_new2("qs_nkeys"), INT2NUM(bdb_stat->qs_nkeys));
#if ! HAVE_ST_DB_QUEUE_STAT_QS_NRECS
    rb_hash_aset(hash, rb_tainted_str_new2("qs_nrecs"), INT2NUM(bdb_stat->qs_nkeys));
#endif
#endif
#if HAVE_ST_DB_QUEUE_STAT_QS_NDATA
    rb_hash_aset(hash, rb_tainted_str_new2("qs_ndata"), INT2NUM(bdb_stat->qs_ndata));
#endif
#if HAVE_ST_DB_QUEUE_STAT_QS_NRECS
    rb_hash_aset(hash, rb_tainted_str_new2("qs_nrecs"), INT2NUM(bdb_stat->qs_nrecs));
#endif
    rb_hash_aset(hash, rb_tainted_str_new2("qs_pages"), INT2NUM(bdb_stat->qs_pages));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_pagesize"), INT2NUM(bdb_stat->qs_pagesize));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_pgfree"), INT2NUM(bdb_stat->qs_pgfree));
    rb_hash_aset(hash, rb_tainted_str_new2("qs_re_len"), INT2NUM(bdb_stat->qs_re_len));
    pad = (char)bdb_stat->qs_re_pad;
    rb_hash_aset(hash, rb_tainted_str_new2("qs_re_pad"), rb_tainted_str_new(&pad, 1));
#if HAVE_ST_DB_QUEUE_STAT_QS_START
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
#if HAVE_DB_STAT_4_TXN
    DB_TXN *txnid = NULL;
#endif

    GetDB(obj, dbst);
#if HAVE_DB_STAT_4
#if HAVE_DB_STAT_4_TXN
    if (RTEST(dbst->txn)) {
        bdb_TXN *txnst;

        GetTxnDB(dbst->txn, txnst);
        txnid = txnst->txnid;
    }
    bdb_test_error(dbst->dbp->stat(dbst->dbp, txnid, &bdb_stat, 0));
#else
    bdb_test_error(dbst->dbp->stat(dbst->dbp, &bdb_stat, 0, 0));
#endif
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

#if HAVE_ST_DB_SET_ERRCALL

static VALUE
bdb_i_create(obj)
    VALUE obj;
{
    DB *dbp;
    bdb_ENV *envst = 0;
    DB_ENV *envp;
    bdb_DB *dbst;
    VALUE ret, env;

    envp = NULL;
    env = 0;
    if (rb_obj_is_kind_of(obj, bdb_cEnv)) {
        GetEnvDB(obj, envst);
        envp = envst->envp;
        env = obj;
    }
    bdb_test_error(db_create(&dbp, envp, 0));
    dbp->set_errpfx(dbp, "BDB::");
    dbp->set_errcall(dbp, bdb_env_errcall);
    ret = Data_Make_Struct(bdb_cCommon, bdb_DB, bdb_mark, bdb_free, dbst);
    rb_obj_call_init(ret, 0, 0);
    dbst->env = env;
    dbst->dbp = dbp;
    if (envp) {
	dbst->options |= envst->options & BDB_INIT_LOCK;
    }
    return ret;
}

#endif

static VALUE
bdb_s_upgrade(int argc, VALUE *argv, VALUE obj)
{
#if HAVE_ST_DB_UPGRADE
    bdb_DB *dbst;
    VALUE a, b;
    int flags;
    VALUE val;

    rb_secure(4);
    flags = 0;
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	flags = NUM2INT(b);
    }
    SafeStringValue(a);
    val = bdb_i_create(obj);
    GetDB(val, dbst);
    bdb_test_error(dbst->dbp->upgrade(dbst->dbp, StringValuePtr(a), flags));
    return val;
#else
    rb_raise(bdb_eFatal, "You can't upgrade a database with this version of DB");
#endif
}

#if HAVE_ST_DB_REMOVE

static VALUE
bdb_s_remove(int argc, VALUE *argv, VALUE obj)
{
    bdb_DB *dbst;
    VALUE a, b, c;
    char *name, *subname;

    rb_secure(2);
    c = bdb_i_create(obj);
    GetDB(c, dbst);
    name = subname = NULL;
    a = b = Qnil;
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
        if (!NIL_P(b)) {
	    SafeStringValue(b);
            subname = StringValuePtr(b);
        }
    }
    SafeStringValue(a);
    name = StringValuePtr(a);
    bdb_test_error(dbst->dbp->remove(dbst->dbp, name, subname, 0));
    return Qtrue;
}

#endif

#if HAVE_ST_DB_RENAME

static VALUE
bdb_s_rename(int argc, VALUE *argv, VALUE obj)
{
    bdb_DB *dbst;
    VALUE a, b, c;
    char *name, *subname, *newname;

    rb_secure(2);
    c = bdb_i_create(obj);
    GetDB(c, dbst);
    name = subname = NULL;
    a = b = c = Qnil;
    rb_scan_args(argc, argv, "30", &a, &b, &c);
    if (!NIL_P(b)) {
	SafeStringValue(b);
	subname = StringValuePtr(b);
    }
    SafeStringValue(a);
    SafeStringValue(c);
    name = StringValuePtr(a);
    newname = StringValuePtr(c);
    bdb_test_error(dbst->dbp->rename(dbst->dbp, name, subname, newname, 0));
    return Qtrue;
}

#endif

#if HAVE_ST_DB_JOIN

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
    INIT_RECNO(dbst, key, recno);
    do {
	MEMZERO(&data, DBT, 1);
	data.flags |= DB_DBT_MALLOC;
	SET_PARTIAL(dbst, data);
	ret = bdb_test_error(st->dbcp->c_get(st->dbcp, &key, &data, st->sens));
	if (ret  == DB_NOTFOUND || ret == DB_KEYEMPTY)
	    return Qnil;
	rb_yield(bdb_assoc(st->db, &key, &data));
    } while (1);
    return Qnil;
}
 
static VALUE
bdb_join(int argc, VALUE *argv, VALUE obj)
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
    if (RARRAY_LEN(a) == 0) {
        rb_raise(bdb_eFatal, "empty array");
    }
    dbcarr = ALLOCA_N(DBC *, RARRAY_LEN(a) + 1);
    {
	DBC **dbs;
	bdb_DB *tmp;

	for (dbs = dbcarr, i = 0; i < RARRAY_LEN(a); i++, dbs++) {
	    if (!rb_obj_is_kind_of(RARRAY_PTR(a)[i], bdb_cCursor)) {
		rb_raise(bdb_eFatal, "element %d is not a cursor", i);
	    }
	    GetCursorDB(RARRAY_PTR(a)[i], dbcst, tmp);
	    *dbs = dbcst->dbc;
	}
	*dbs = 0;
    }
    dbc = 0;
#if HAVE_TYPE_DB_INFO
    bdb_test_error(dbst->dbp->join(dbst->dbp, dbcarr, 0, &dbc));
#else
    bdb_test_error(dbst->dbp->join(dbst->dbp, dbcarr, &dbc, 0));
#endif
    st.db = obj;
    st.dbcp = dbc;
    st.sens = flags | TEST_INIT_LOCK(dbst);
    rb_ensure(bdb_i_join, (VALUE)&st, bdb_i_joinclose, (VALUE)&st);
    return obj;
}

#endif

#if HAVE_ST_DB_BYTESWAPPED || HAVE_ST_DB_GET_BYTESWAPPED

static VALUE
bdb_byteswapp(obj)
    VALUE obj;
{
    bdb_DB *dbst;
    int byteswap = 0;

    GetDB(obj, dbst);
#if HAVE_ST_DB_BYTESWAPPED
    return dbst->dbp->byteswapped?Qtrue:Qfalse;
#elif HAVE_DB_GET_BYTESWAPPED_2
    dbst->dbp->get_byteswapped(dbst->dbp, &byteswap);
    return byteswap?Qtrue:Qfalse;
#else
    return dbst->dbp->get_byteswapped(dbst->dbp)?Qtrue:Qfalse;
#endif
}

#endif

#if HAVE_ST_DB_ASSOCIATE

static VALUE
bdb_internal_second_call(tmp)
    VALUE *tmp;
{
    return rb_funcall2(tmp[0], bdb_id_call, 3, tmp + 1);
}

static int
bdb_call_secondary(secst, pkey, pdata, skey)
    DB *secst;
    DBT *pkey;
    DBT *pdata;
    DBT *skey;
{
    VALUE obj, ary, second;
    bdb_DB *dbst, *secondst;
    VALUE result = Qnil;
    int i, inter;

    GetIdDb(obj, dbst);
    if (!dbst->dbp || !RTEST(dbst->secondary)) return DB_DONOTINDEX;
    for (i = 0; i < RARRAY_LEN(dbst->secondary); i++) {
	ary = RARRAY_PTR(dbst->secondary)[i];
	if (RARRAY_LEN(ary) != 2) continue;
	second = RARRAY_PTR(ary)[0];
	Data_Get_Struct(second, bdb_DB, secondst);
	if (!secondst->dbp) continue;
	if (secondst->dbp == secst) {
	    VALUE tmp[4];

	    tmp[0] = RARRAY_PTR(ary)[1];
	    tmp[1] = second;
	    tmp[2] = bdb_test_load_key(obj, pkey);
	    tmp[3] = bdb_test_load(obj, pdata, FILTER_VALUE|FILTER_FREE);
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
		result = bdb_test_dump(second, &stmp, result, FILTER_KEY);
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
bdb_associate(int argc, VALUE *argv, VALUE obj)
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

    dbst->options |= BDB_NEED_CURRENT;
    if (!dbst->secondary) {
	dbst->secondary = rb_ary_new();
    }
    rb_thread_local_aset(rb_thread_current(), bdb_id_current_db, obj);
#if HAVE_RB_BLOCK_PROC
    rb_ary_push(dbst->secondary, rb_assoc_new(second, rb_block_proc()));
#else
    rb_ary_push(dbst->secondary, rb_assoc_new(second, rb_f_lambda()));
#endif
    secondst->secondary = Qnil;

#if HAVE_DB_ASSOCIATE_TXN
    {
	DB_TXN *txnid = NULL;
	if (RTEST(dbst->txn)) {
	    bdb_TXN *txnst;

	    GetTxnDB(dbst->txn, txnst);
	    txnid = txnst->txnid;
	}
#if HAVE_CONST_DB_AUTO_COMMIT
	else if (dbst->options & BDB_AUTO_COMMIT) {
	  flags |= DB_AUTO_COMMIT;
	}
#endif
	bdb_test_error(dbst->dbp->associate(dbst->dbp, txnid, secondst->dbp, 
					    bdb_call_secondary, flags));
    }
#else
    bdb_test_error(dbst->dbp->associate(dbst->dbp, secondst->dbp, 
					bdb_call_secondary, flags));
#endif
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

#if HAVE_ST_DB_VERIFY

static VALUE
bdb_verify(int argc, VALUE *argv, VALUE obj)
{
    bdb_DB *dbst;
    char *file, *database;
    VALUE flagv = Qnil, iov = Qnil;
    int flags = 0;
#if HAVE_TYPE_RB_IO_T
    rb_io_t *fptr;
#else
    OpenFile *fptr;
#endif
    FILE *io = NULL;

    rb_secure(4);
    file = database = NULL;
    switch(rb_scan_args(argc, argv, "02", &iov, &flagv)) {
    case 2:
	flags = NUM2INT(flagv);
    case 1:
	if (!NIL_P(iov)) {
	    iov = rb_convert_type(iov, T_FILE, "IO", "to_io");
	    GetOpenFile(iov, fptr);
	    rb_io_check_writable(fptr);
#if HAVE_RB_IO_STDIO_FILE
	    io = rb_io_stdio_file(fptr);
#else
	    io = GetWriteFile(fptr);
#endif
	}
	break;
    case 0:
	break;
    }
    GetDB(obj, dbst);
    if (!NIL_P(dbst->filename)) {
	file = StringValuePtr(dbst->filename);
    }
    if (!NIL_P(dbst->database)) {
	database = StringValuePtr(dbst->database);
    }
    bdb_test_error(dbst->dbp->verify(dbst->dbp, file, database, io, flags));
    return Qnil;
}

#endif

static VALUE
bdb__txn__dup(VALUE obj, VALUE a)
{
    bdb_DB *dbp, *dbh;
    bdb_TXN *txnst;
    VALUE res;
 
    GetDB(obj, dbp);
    GetTxnDB(a, txnst);
    res = Data_Make_Struct(CLASS_OF(obj), bdb_DB, bdb_mark, bdb_free, dbh);
    MEMCPY(dbh, dbp, bdb_DB, 1);
    dbh->txn = a;
    dbh->orig = obj;
    dbh->ori_val = res;
    dbh->options |= (txnst->options & BDB_INIT_LOCK) | BDB_NOT_OPEN;
    return res;
}

static VALUE
bdb__txn__close(VALUE obj, VALUE commit, VALUE real)
{
    bdb_DB *dbst, *dbst1;

    if (!real) {
	Data_Get_Struct(obj, bdb_DB, dbst);
	dbst->dbp = NULL;
    }
    else {
	if (commit) {
	    Data_Get_Struct(obj, bdb_DB, dbst);
	    if (dbst->orig) {
		Data_Get_Struct(dbst->orig, bdb_DB, dbst1);
		dbst1->len = dbst->len;
	    }
	}
//	bdb_close(0, 0, obj);
    }
    return Qnil;
}

#if HAVE_ST_DB_SET_CACHE_PRIORITY

static VALUE
bdb_cache_priority_set(VALUE obj, VALUE a)
{
    int priority;
    bdb_DB *dbst;
    GetDB(obj, dbst);
    priority = dbst->priority;
    bdb_test_error(dbst->dbp->set_cache_priority(dbst->dbp, NUM2INT(a)));
    dbst->priority = NUM2INT(a);
    return INT2FIX(priority);
}

static VALUE
bdb_cache_priority_get(VALUE obj)
{
    bdb_DB *dbst;
    GetDB(obj, dbst);
    return INT2FIX(dbst->priority);
}

#endif

#if HAVE_ST_DB_SET_FEEDBACK

static VALUE
bdb_feedback_set(VALUE obj, VALUE a)
{
    bdb_DB *dbst;

    GetDB(obj, dbst);
    if (NIL_P(a)) {
	dbst->feedback = a;
    }
    else {
	if (!rb_respond_to(a, bdb_id_call)) {
	    rb_raise(bdb_eFatal, "arg must respond to #call");
	}
	dbst->feedback = a;
	if (!(dbst->options & BDB_FEEDBACK)) {
	    dbst->options |= BDB_FEEDBACK;
	    rb_thread_local_aset(rb_thread_current(), bdb_id_current_db, obj); 
	}
    }
    return a;
}

#endif

static VALUE
bdb_i_conf(obj, a)
    VALUE obj, a;
{
    bdb_DB *dbst;
    u_int32_t value;
#if HAVE_ST_DB_GET_CACHESIZE
    u_int32_t bytes, gbytes;
    int ncache;
#endif
#if HAVE_ST_DB_GET_RE_PAD || HAVE_ST_DB_GET_LORDER || HAVE_ST_DB_GET_RE_DELIM
    int intval;
#endif
#if HAVE_ST_DB_GET_DBNAME
    const char *filename, *dbname;
#endif
    const char *str;

    GetDB(obj, dbst);
    str = StringValuePtr(a);
#if HAVE_ST_DB_GET_BT_MINKEY
    if (strcmp(str, "bt_minkey") == 0) {
	bdb_test_error(dbst->dbp->get_bt_minkey(dbst->dbp, &value));
	return INT2NUM(value);
    }
#endif
#if HAVE_ST_DB_GET_CACHESIZE
    if (strcmp(str, "cachesize") == 0) {
	VALUE res;

	bdb_test_error(dbst->dbp->get_cachesize(dbst->dbp, &gbytes, &bytes, &ncache));
	res = rb_ary_new2(3);
	rb_ary_push(res, INT2NUM(gbytes));
	rb_ary_push(res, INT2NUM(bytes));
	rb_ary_push(res, INT2NUM(ncache));
	return res;
    }
#endif
#if HAVE_ST_DB_GET_DBNAME
    if (strcmp(str, "dbname") == 0) {
	VALUE res;

	bdb_test_error(dbst->dbp->get_dbname(dbst->dbp, &filename, &dbname));
	res = rb_ary_new2(3);
	if (filename && strlen(filename)) {
	    rb_ary_push(res, rb_tainted_str_new2(filename));
	}
	else {
	    rb_ary_push(res, Qnil);
	}
	if (dbname && strlen(dbname)) {
	    rb_ary_push(res, rb_tainted_str_new2(dbname));
	}
	else {
	    rb_ary_push(res, Qnil);
	}
	return res;
    }
#endif
#if HAVE_ST_DB_GET_ENV
    if (strcmp(str, "env") == 0) {
	return bdb_env(obj);
    }
#endif
#if HAVE_ST_DB_GET_H_FFACTOR
    if (strcmp(str, "h_ffactor") == 0) {
	bdb_test_error(dbst->dbp->get_h_ffactor(dbst->dbp, &value));
	return INT2NUM(value);
    }
#endif
#if HAVE_ST_DB_GET_H_NELEM
    if (strcmp(str, "h_nelem") == 0) {
	bdb_test_error(dbst->dbp->get_h_nelem(dbst->dbp, &value));
	return INT2NUM(value);
    }
#endif
#if HAVE_ST_DB_GET_LORDER
    if (strcmp(str, "lorder") == 0) {
	bdb_test_error(dbst->dbp->get_lorder(dbst->dbp, &intval));
	return INT2NUM(intval);
    }
#endif
#if HAVE_ST_DB_GET_PAGESIZE
    if (strcmp(str, "pagesize") == 0) {
	bdb_test_error(dbst->dbp->get_pagesize(dbst->dbp, &value));
	return INT2NUM(value);
    }
#endif
#if HAVE_ST_DB_GET_Q_EXTENTSIZE
    if (strcmp(str, "q_extentsize") == 0) {
	bdb_test_error(dbst->dbp->get_q_extentsize(dbst->dbp, &value));
	return INT2NUM(value);
    }
#endif
#if HAVE_ST_DB_GET_RE_DELIM
    if (strcmp(str, "re_delim") == 0) {
	bdb_test_error(dbst->dbp->get_re_delim(dbst->dbp, &intval));
	return INT2NUM(intval);
    }
#endif
#if HAVE_ST_DB_GET_RE_LEN
    if (strcmp(str, "re_len") == 0) {
	bdb_test_error(dbst->dbp->get_re_len(dbst->dbp, &value));
	return INT2NUM(value);
    }
#endif
#if HAVE_ST_DB_GET_RE_PAD
    if (strcmp(str, "re_pad") == 0) {
	bdb_test_error(dbst->dbp->get_re_pad(dbst->dbp, &intval));
	return INT2NUM(intval);
    }
#endif
#if HAVE_ST_DB_GET_RE_SOURCE
    if (strcmp(str, "re_source") == 0) {
	const char *strval;

	bdb_test_error(dbst->dbp->get_re_source(dbst->dbp, &strval));
	if (strval && strlen(strval)) {
	    return rb_tainted_str_new2(strval);
	}
	return Qnil;
    }
#endif
#if HAVE_ST_DB_GET_FLAGS
    if (strcmp(str, "flags") == 0) {
	bdb_test_error(dbst->dbp->get_flags(dbst->dbp, &value));
	return INT2NUM(value);
    }
#endif
#if HAVE_ST_DB_GET_OPEN_FLAGS
    if (strcmp(str, "open_flags") == 0) {
	bdb_test_error(dbst->dbp->get_open_flags(dbst->dbp, &value));
	return INT2NUM(value);
    }
#endif
    rb_raise(rb_eArgError, "Unknown option %s", str);
    return obj;
}

static char *
options[] = {
#if HAVE_ST_DB_GET_BT_MINKEY
    "bt_minkey",
#endif
#if HAVE_ST_DB_GET_CACHESIZE
    "cachesize",
#endif
#if HAVE_ST_DB_GET_DBNAME
    "dbname",
#endif
#if HAVE_ST_DB_GET_ENV
    "env",
#endif
#if HAVE_ST_DB_GET_H_FFACTOR
    "h_ffactor",
#endif
#if HAVE_ST_DB_GET_H_NELEM
    "h_nelem",
#endif
#if HAVE_ST_DB_GET_LORDER
    "lorder",
#endif
#if HAVE_ST_DB_GET_PAGESIZE
    "pagesize",
#endif
#if HAVE_ST_DB_GET_Q_EXTENTSIZE
    "q_extentsize",
#endif
#if HAVE_ST_DB_GET_RE_DELIM
    "re_delim",
#endif
#if HAVE_ST_DB_GET_RE_LEN
    "re_len",
#endif
#if HAVE_ST_DB_GET_RE_PAD
    "re_pad",
#endif
#if HAVE_ST_DB_GET_RE_SOURCE
    "re_source",
#endif
#if HAVE_ST_DB_GET_FLAGS
    "flags",
#endif
#if HAVE_ST_DB_GET_OPEN_FLAGS
    "open_flags",
#endif
    0
};

struct optst {
    VALUE obj, str;
};

static VALUE
bdb_intern_conf(optp)
    struct optst *optp;
{
    return bdb_i_conf(optp->obj, optp->str);
}

static VALUE
bdb_conf(int argc, VALUE *argv, VALUE obj)
{
    int i, state;
    VALUE res, val;
    struct optst opt;

    if (argc > 1) {
	rb_raise(rb_eArgError, "invalid number of arguments (%d for 1)", argc);
    }
    if (argc == 1) {
	return bdb_i_conf(obj, argv[0]);
    }
    res = rb_hash_new();
    opt.obj = obj;
    for (i = 0; options[i] != NULL; i++) {
	opt.str = rb_str_new2(options[i]);
	val = rb_protect(bdb_intern_conf, (VALUE)&opt, &state);
	if (state == 0) {
	    rb_hash_aset(res, opt.str, val);
	}
    }
    return res;
}

#if HAVE_ST_DB_FD

static VALUE
bdb_fd(VALUE obj)
{
    int fd = 0;
    bdb_DB *dbst;
    VALUE ary[2];

    GetDB(obj, dbst);
    if (dbst->dbp->fd(dbst->dbp, &fd)) {
	rb_raise(rb_eArgError, "invalid database handler");
    }
    ary[0] = INT2FIX(fd);
    ary[1] = rb_str_new2("r");
    return rb_class_new_instance(2, ary, rb_cIO);
}

#endif

#if HAVE_ST_DB_SET_PRIORITY

static VALUE
bdb_set_priority(VALUE obj, VALUE a)
{
    bdb_DB *dbst;

    GetDB(obj, dbst);
    if (dbst->dbp->set_priority(dbst->dbp, NUM2INT(a))) {
	rb_raise(rb_eArgError, "invalid argument");
    }
    return a;
}

static VALUE
bdb_priority(VALUE obj)
{
    bdb_DB *dbst;
    DB_CACHE_PRIORITY prio = 0;

    GetDB(obj, dbst);
    if (dbst->dbp->get_priority(dbst->dbp, &prio)) {
	rb_raise(rb_eArgError, "invalid argument");
    }
    return INT2FIX(prio);
}

#endif

void bdb_init_common()
{
    id_bt_compare = rb_intern("bdb_bt_compare");
    id_bt_prefix = rb_intern("bdb_bt_prefix");
#if HAVE_CONST_DB_DUPSORT
    id_dup_compare = rb_intern("bdb_dup_compare");
#endif
    id_h_hash = rb_intern("bdb_h_hash");
#if HAVE_ST_DB_SET_H_COMPARE
    id_h_compare = rb_intern("bdb_h_compare");
#endif
#if HAVE_ST_DB_SET_APPEND_RECNO
    id_append_recno = rb_intern("bdb_append_recno");
#endif
#if HAVE_ST_DB_SET_FEEDBACK
    id_feedback = rb_intern("bdb_feedback");
#endif
    bdb_cCommon = rb_define_class_under(bdb_mDb, "Common", rb_cObject);
    rb_define_private_method(bdb_cCommon, "initialize", bdb_init, -1);
    rb_include_module(bdb_cCommon, rb_mEnumerable);
#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
    rb_define_alloc_func(bdb_cCommon, bdb_s_alloc);
#else
    rb_define_singleton_method(bdb_cCommon, "allocate", bdb_s_alloc, 0);
#endif
    rb_define_singleton_method(bdb_cCommon, "new", bdb_s_new, -1);
    rb_define_singleton_method(bdb_cCommon, "create", bdb_s_new, -1);
    rb_define_singleton_method(bdb_cCommon, "open", bdb_s_open, -1);
    rb_define_singleton_method(bdb_cCommon, "[]", bdb_s_create, -1);
#if HAVE_ST_DB_REMOVE
    rb_define_singleton_method(bdb_cCommon, "remove", bdb_s_remove, -1);
    rb_define_singleton_method(bdb_cCommon, "bdb_remove", bdb_s_remove, -1);
    rb_define_singleton_method(bdb_cCommon, "unlink", bdb_s_remove, -1);
#endif
    rb_define_singleton_method(bdb_cCommon, "upgrade", bdb_s_upgrade, -1);
    rb_define_singleton_method(bdb_cCommon, "bdb_upgrade", bdb_s_upgrade, -1);
#if HAVE_ST_DB_RENAME
    rb_define_singleton_method(bdb_cCommon, "rename", bdb_s_rename, -1);
    rb_define_singleton_method(bdb_cCommon, "bdb_rename", bdb_s_rename, -1);
#endif
    rb_define_private_method(bdb_cCommon, "__txn_close__", bdb__txn__close, 2);
    rb_define_private_method(bdb_cCommon, "__txn_dup__", bdb__txn__dup, 1);
    rb_define_method(bdb_cCommon, "filename", bdb_filename, 0);
    rb_define_method(bdb_cCommon, "subname", bdb_database, 0);
    rb_define_method(bdb_cCommon, "database", bdb_database, 0);
#if HAVE_ST_DB_VERIFY
    rb_define_method(bdb_cCommon, "verify", bdb_verify, -1);
#endif
    rb_define_method(bdb_cCommon, "close", bdb_close, -1);
    rb_define_method(bdb_cCommon, "db_close", bdb_close, -1);
    rb_define_method(bdb_cCommon, "put", bdb_put, -1);
    rb_define_method(bdb_cCommon, "db_put", bdb_put, -1);
    rb_define_method(bdb_cCommon, "[]=", bdb_aset, 2);
    rb_define_method(bdb_cCommon, "store", bdb_put, -1);
    rb_define_method(bdb_cCommon, "env", bdb_env, 0);
    rb_define_method(bdb_cCommon, "environment", bdb_env, 0);
    rb_define_method(bdb_cCommon, "has_env?", bdb_env_p, 0);
    rb_define_method(bdb_cCommon, "has_environment?", bdb_env_p, 0);
    rb_define_method(bdb_cCommon, "env?", bdb_env_p, 0);
    rb_define_method(bdb_cCommon, "environment?", bdb_env_p, 0);
    rb_define_method(bdb_cCommon, "txn", bdb_txn, 0);
    rb_define_method(bdb_cCommon, "transaction", bdb_txn, 0);
    rb_define_method(bdb_cCommon, "txn?", bdb_txn_p, 0);
    rb_define_method(bdb_cCommon, "transaction?", bdb_txn_p, 0);
    rb_define_method(bdb_cCommon, "in_txn?", bdb_txn_p, 0);
    rb_define_method(bdb_cCommon, "in_transaction?", bdb_txn_p, 0);
#if HAVE_CONST_DB_NEXT_DUP
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
#if HAVE_ST_DB_PGET
    rb_define_method(bdb_cCommon, "pget", bdb_pget, -1);
    rb_define_method(bdb_cCommon, "primary_get", bdb_pget, -1);
    rb_define_method(bdb_cCommon, "db_pget", bdb_pget, -1);
#endif
    rb_define_method(bdb_cCommon, "fetch", bdb_fetch, -1);
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
    rb_define_method(bdb_cCommon, "clear", bdb_clear, -1);
    rb_define_method(bdb_cCommon, "truncate", bdb_clear, -1);
    rb_define_method(bdb_cCommon, "replace", bdb_replace, -1);
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
    rb_define_method(bdb_cCommon, "select", bdb_select, -1);
    rb_define_method(bdb_cCommon, "values_at", bdb_values_at, -1);
    rb_define_method(bdb_cCommon, "set_partial", bdb_set_partial, 2);
    rb_define_method(bdb_cCommon, "clear_partial", bdb_clear_partial, 0);
    rb_define_method(bdb_cCommon, "partial_clear", bdb_clear_partial, 0);
#if HAVE_ST_DB_JOIN
    rb_define_method(bdb_cCommon, "join", bdb_join, -1);
#endif
#if HAVE_ST_DB_BYTESWAPPED || HAVE_ST_DB_GET_BYTESWAPPED
    rb_define_method(bdb_cCommon, "byteswapped?", bdb_byteswapp, 0);
    rb_define_method(bdb_cCommon, "get_byteswapped", bdb_byteswapp, 0);
#endif
#if HAVE_ST_DB_ASSOCIATE
    rb_define_method(bdb_cCommon, "associate", bdb_associate, -1);
#endif
#if HAVE_ST_DB_SET_CACHE_PRIORITY
    rb_define_method(bdb_cCommon, "cache_priority=", bdb_cache_priority_set, 1);
    rb_define_method(bdb_cCommon, "cache_priority", bdb_cache_priority_get, 0);
#endif
#if HAVE_ST_DB_SET_FEEDBACK
    rb_define_method(bdb_cCommon, "feedback=", bdb_feedback_set, 1);
#endif
    bdb_cBtree = rb_define_class_under(bdb_mDb, "Btree", bdb_cCommon);
    rb_define_method(bdb_cBtree, "stat", bdb_tree_stat, -1);
    rb_define_method(bdb_cBtree, "each_by_prefix", bdb_each_prefix, -1);
    rb_define_method(bdb_cBtree, "reverse_each_by_prefix", bdb_each_xiferp, -1);
#if HAVE_TYPE_DB_COMPACT
    rb_define_method(bdb_cBtree, "compact", bdb_treerec_compact, -1);
#endif
#if HAVE_TYPE_DB_KEY_RANGE
    bdb_sKeyrange = rb_struct_define("Keyrange", "less", "equal", "greater", 0);
    rb_global_variable(&bdb_sKeyrange);
    rb_define_method(bdb_cBtree, "key_range", bdb_btree_key_range, 1);
#endif
    bdb_cHash = rb_define_class_under(bdb_mDb, "Hash", bdb_cCommon);
#if HAVE_TYPE_DB_HASH_STAT
    rb_define_method(bdb_cHash, "stat", bdb_hash_stat, -1);
#endif
    bdb_cRecno = rb_define_class_under(bdb_mDb, "Recno", bdb_cCommon);
    rb_define_method(bdb_cRecno, "each_index", bdb_each_key, -1);
    rb_define_method(bdb_cRecno, "unshift", bdb_unshift, -1);
    rb_define_method(bdb_cRecno, "<<", bdb_append, 1);
    rb_define_method(bdb_cRecno, "push", bdb_append_m, -1);
    rb_define_method(bdb_cRecno, "stat", bdb_tree_stat, -1);
#if HAVE_TYPE_DB_COMPACT
    rb_define_method(bdb_cRecno, "compact", bdb_treerec_compact, -1);
#endif
#if HAVE_CONST_DB_QUEUE
    bdb_cQueue = rb_define_class_under(bdb_mDb, "Queue", bdb_cCommon);
    rb_define_singleton_method(bdb_cQueue, "new", bdb_queue_s_new, -1);
    rb_define_singleton_method(bdb_cQueue, "create", bdb_queue_s_new, -1);
    rb_define_method(bdb_cQueue, "each_index", bdb_each_key, -1);
    rb_define_method(bdb_cQueue, "<<", bdb_append, 1);
    rb_define_method(bdb_cQueue, "push", bdb_append_m, -1);
#if HAVE_CONST_DB_CONSUME
    rb_define_method(bdb_cQueue, "shift", bdb_consume, 0);
#endif
    rb_define_method(bdb_cQueue, "stat", bdb_queue_stat, -1);
    rb_define_method(bdb_cQueue, "pad", bdb_queue_padlen, 0);
#endif
    rb_define_method(bdb_cCommon, "configuration", bdb_conf, -1);
    rb_define_method(bdb_cCommon, "conf", bdb_conf, -1);
#if HAVE_ST_DB_FD
    rb_define_method(bdb_cCommon, "fd", bdb_fd, 0);
#endif
#if HAVE_ST_DB_SET_PRIORITY
    rb_define_method(bdb_cCommon, "priority", bdb_priority, 0);
    rb_define_method(bdb_cCommon, "priority=", bdb_set_priority, 1);
#endif
    bdb_cUnknown = rb_define_class_under(bdb_mDb, "Unknown", bdb_cCommon);
}
