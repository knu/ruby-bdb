#include "bdb.h"

static ID id_send;

void
bdb_deleg_mark(delegst)
    struct deleg_class *delegst;
{
    if (delegst->db) rb_gc_mark(delegst->db);
    if (delegst->key) rb_gc_mark(delegst->key);
    if (delegst->obj) rb_gc_mark(delegst->obj);
}

extern VALUE bdb_put _((int, VALUE *, VALUE));

static VALUE
bdb_deleg_each(tmp)
    VALUE *tmp;
{
    return rb_funcall2(tmp[0], id_send, (int)tmp[1], (VALUE *)tmp[2]);
}

static VALUE
bdb_deleg_missing(argc, argv, obj)
    int argc;
    VALUE *argv, obj;
{
    struct deleg_class *delegst, *newst;
    bdb_DB *dbst;
    VALUE res, new;

    Data_Get_Struct(obj, struct deleg_class, delegst);
    if (rb_block_given_p()) {
	VALUE tmp[3];

	tmp[0] = delegst->obj;
	tmp[1] = (VALUE)argc;
	tmp[2] = (VALUE)argv;
	res = rb_iterate((VALUE(*)(VALUE))bdb_deleg_each, (VALUE)tmp, rb_yield, 0);
    }
    else {
	res = rb_funcall2(delegst->obj, id_send, argc, argv);
    }
    Data_Get_Struct(delegst->db, bdb_DB, dbst);
    if (dbst->dbp) {
	VALUE nargv[2];

	if (!SPECIAL_CONST_P(res) &&
	    (TYPE(res) != T_DATA || 
	     RDATA(res)->dmark != (RUBY_DATA_FUNC)bdb_deleg_mark)) {
	    new = Data_Make_Struct(bdb_cDelegate, struct deleg_class, 
				   bdb_deleg_mark, free, newst);
	    newst->db = delegst->db;
	    newst->obj = res;
	    newst->key = (!delegst->type)?obj:delegst->key;
	    newst->type = 1;
	    res = new;
	}
	if (!delegst->type) {
	    nargv[0] = delegst->key;
	    nargv[1] = delegst->obj;
	}
	else {
	    Data_Get_Struct(delegst->key, struct deleg_class, newst);
	    nargv[0] = newst->key;
	    nargv[1] = newst->obj;
	}
	bdb_put(2, nargv, delegst->db);
    }
    return res;
}

#define DELEG_0(id)						\
   VALUE obj;							\
   {								\
       struct deleg_class *delegst;				\
       Data_Get_Struct(obj, struct deleg_class, delegst);	\
       return rb_funcall2(delegst->obj, id, 0, 0);		\
   }

static VALUE bdb_deleg_inspect(obj) DELEG_0(rb_intern("inspect"))
static VALUE bdb_deleg_to_s(obj)    DELEG_0(rb_intern("to_s"))
static VALUE bdb_deleg_to_str(obj)  DELEG_0(rb_intern("to_str"))
static VALUE bdb_deleg_to_a(obj)    DELEG_0(rb_intern("to_a"))
static VALUE bdb_deleg_to_ary(obj)  DELEG_0(rb_intern("to_ary"))
static VALUE bdb_deleg_to_i(obj)    DELEG_0(rb_intern("to_i"))
static VALUE bdb_deleg_to_int(obj)  DELEG_0(rb_intern("to_int"))
static VALUE bdb_deleg_to_f(obj)    DELEG_0(rb_intern("to_f"))
static VALUE bdb_deleg_to_hash(obj) DELEG_0(rb_intern("to_hash"))
static VALUE bdb_deleg_to_io(obj)   DELEG_0(rb_intern("to_io"))
static VALUE bdb_deleg_to_proc(obj) DELEG_0(rb_intern("to_proc"))

VALUE
bdb_deleg_to_orig(obj)
    VALUE obj;
{
    struct deleg_class *delegst;
    Data_Get_Struct(obj, struct deleg_class, delegst);
    return delegst->obj;
}

static VALUE
bdb_deleg_orig(obj)
    VALUE obj;
{
    return obj;
}

static VALUE
bdb_deleg_dump(obj, limit)
    VALUE obj, limit;
{
    struct deleg_class *delegst;
    bdb_DB *dbst;
    Data_Get_Struct(obj, struct deleg_class, delegst);
    Data_Get_Struct(delegst->db, bdb_DB, dbst);
    return rb_funcall(dbst->marshal, bdb_id_dump, 1, delegst->obj);
}

static VALUE
bdb_deleg_load(obj, str)
    VALUE obj, str;
{
    bdb_DB *dbst;

    obj = bdb_local_aref();
    Data_Get_Struct(obj, bdb_DB, dbst);
    return rb_funcall(dbst->marshal, bdb_id_load, 1, str);
}



void bdb_init_delegator()
{
    id_send = rb_intern("send");
    bdb_cDelegate = rb_define_class_under(bdb_mDb, "Delegate", rb_cObject);
    {
	VALUE ary = Qfalse;
	char *method;
	int i;

	ary = rb_class_instance_methods(1, &ary, rb_mKernel);
	for (i = 0; i < RARRAY(ary)->len; i++) {
	    method = StringValuePtr(RARRAY(ary)->ptr[i]);
	    if (!strcmp(method, "==") ||
		!strcmp(method, "===") || !strcmp(method, "=~")) continue;
	    rb_undef_method(bdb_cDelegate, method);
	}
    }
    rb_define_method(bdb_cDelegate, "method_missing", bdb_deleg_missing, -1);
    rb_define_method(bdb_cDelegate, "inspect", bdb_deleg_inspect, 0);
    rb_define_method(bdb_cDelegate, "to_s", bdb_deleg_to_s, 0);
    rb_define_method(bdb_cDelegate, "to_str", bdb_deleg_to_str, 0);
    rb_define_method(bdb_cDelegate, "to_a", bdb_deleg_to_a, 0);
    rb_define_method(bdb_cDelegate, "to_ary", bdb_deleg_to_ary, 0);
    rb_define_method(bdb_cDelegate, "to_i", bdb_deleg_to_i, 0);
    rb_define_method(bdb_cDelegate, "to_int", bdb_deleg_to_int, 0);
    rb_define_method(bdb_cDelegate, "to_f", bdb_deleg_to_f, 0);
    rb_define_method(bdb_cDelegate, "to_hash", bdb_deleg_to_hash, 0);
    rb_define_method(bdb_cDelegate, "to_io", bdb_deleg_to_io, 0);
    rb_define_method(bdb_cDelegate, "to_proc", bdb_deleg_to_proc, 0);
    rb_define_method(bdb_cDelegate, "_dump", bdb_deleg_dump, 1);
    rb_define_singleton_method(bdb_cDelegate, "_load", bdb_deleg_load, 1);
    /* don't use please */
    rb_define_method(bdb_cDelegate, "to_orig", bdb_deleg_to_orig, 0);
    rb_define_method(rb_mKernel, "to_orig", bdb_deleg_orig, 0);
}
