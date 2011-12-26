#include "bdb.h"

static ID id_send;

void
bdb_deleg_mark(struct deleg_class *delegst)
{
    rb_gc_mark(delegst->db);
    rb_gc_mark(delegst->key);
    rb_gc_mark(delegst->obj);
}

extern VALUE bdb_put _((int, VALUE *, VALUE));

#if ! HAVE_RB_BLOCK_CALL

static VALUE
bdb_deleg_each(VALUE *tmp)
{
    return rb_funcall2(tmp[0], id_send, (int)tmp[1], (VALUE *)tmp[2]);
}

#endif

static VALUE
bdb_deleg_missing(int argc, VALUE *argv, VALUE obj)
{
    struct deleg_class *delegst, *newst;
    bdb_DB *dbst;
    VALUE res, new;

    Data_Get_Struct(obj, struct deleg_class, delegst);
    if (rb_block_given_p()) {
#if HAVE_RB_BLOCK_CALL
	res = rb_block_call(delegst->obj, id_send, argc, argv, rb_yield, 0);
#else
	VALUE tmp[3];

	tmp[0] = delegst->obj;
	tmp[1] = (VALUE)argc;
	tmp[2] = (VALUE)argv;
	res = rb_iterate((VALUE(*)(VALUE))bdb_deleg_each, (VALUE)tmp, rb_yield, 0);
#endif
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
   {								\
       struct deleg_class *delegst;				\
       Data_Get_Struct(obj, struct deleg_class, delegst);	\
       return rb_funcall2(delegst->obj, id, 0, 0);		\
   }

static VALUE bdb_deleg_inspect(VALUE obj) DELEG_0(rb_intern("inspect"))
static VALUE bdb_deleg_to_s(VALUE obj)    DELEG_0(rb_intern("to_s"))
static VALUE bdb_deleg_to_str(VALUE obj)  DELEG_0(rb_intern("to_str"))
static VALUE bdb_deleg_to_a(VALUE obj)    DELEG_0(rb_intern("to_a"))
static VALUE bdb_deleg_to_ary(VALUE obj)  DELEG_0(rb_intern("to_ary"))
static VALUE bdb_deleg_to_i(VALUE obj)    DELEG_0(rb_intern("to_i"))
static VALUE bdb_deleg_to_int(VALUE obj)  DELEG_0(rb_intern("to_int"))
static VALUE bdb_deleg_to_f(VALUE obj)    DELEG_0(rb_intern("to_f"))
static VALUE bdb_deleg_to_hash(VALUE obj) DELEG_0(rb_intern("to_hash"))
static VALUE bdb_deleg_to_io(VALUE obj)   DELEG_0(rb_intern("to_io"))
static VALUE bdb_deleg_to_proc(VALUE obj) DELEG_0(rb_intern("to_proc"))

VALUE
bdb_deleg_to_orig(VALUE obj)
{
    struct deleg_class *delegst;
    Data_Get_Struct(obj, struct deleg_class, delegst);
    return delegst->obj;
}

static VALUE
bdb_deleg_orig(VALUE obj)
{
    return obj;
}

static VALUE
bdb_deleg_dump(VALUE obj, VALUE limit)
{
    struct deleg_class *delegst;
    bdb_DB *dbst;
    Data_Get_Struct(obj, struct deleg_class, delegst);
    Data_Get_Struct(delegst->db, bdb_DB, dbst);
    return rb_funcall(dbst->marshal, bdb_id_dump, 1, delegst->obj);
}

static VALUE
bdb_deleg_load(VALUE obj, VALUE str)
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
	VALUE ary = Qfalse, tmp;
	char *method;
	int i;

	ary = rb_class_instance_methods(1, &ary, rb_mKernel);
	for (i = 0; i < RARRAY_LEN(ary); i++) {
	    tmp = rb_obj_as_string(RARRAY_PTR(ary)[i]);
	    method = StringValuePtr(tmp);
	    if (!strcmp(method, "==") ||
		!strcmp(method, "===") ||
		!strcmp(method, "=~") ||
		!strcmp(method, "respond_to?")
		) continue;
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
    rb_define_method(bdb_cDelegate, "_dump_data", bdb_deleg_dump, 1);
    rb_define_singleton_method(bdb_cDelegate, "_load", bdb_deleg_load, 1);
    rb_define_singleton_method(bdb_cDelegate, "_load_data", bdb_deleg_load, 1);
    /* don't use please */
    rb_define_method(bdb_cDelegate, "to_orig", bdb_deleg_to_orig, 0);
    rb_define_method(rb_mKernel, "to_orig", bdb_deleg_orig, 0);
}
