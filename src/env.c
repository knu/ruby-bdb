#include "bdb.h"

ID bdb_id_call;

#if BDB_VERSION >= 30000
static ID id_feedback;
#endif

ID bdb_id_current_env;
static void bdb_env_mark _((bdb_ENV *));

#define GetIdEnv(obj, envst) do {					   	\
    VALUE th = rb_thread_current();						\
										\
    if (!RTEST(th) || !RBASIC(th)->flags) {					\
	rb_raise(bdb_eFatal, "invalid thread object");				\
    }										\
    (obj) = rb_thread_local_aref(th, bdb_id_current_env); 			\
    if (TYPE(obj) != T_DATA ||						   	\
	RDATA(obj)->dmark != (RUBY_DATA_FUNC)bdb_env_mark) {		   	\
	rb_raise(bdb_eFatal, "BUG : current_env not set");		   	\
    }									   	\
    GetEnvDB(obj, envst);						   	\
} while (0)

#define GetIdEnv1(obj, envst) do {					\
    VALUE th = rb_thread_current();					\
									\
    if (!RTEST(th) || !RBASIC(th)->flags) {				\
	rb_raise(bdb_eFatal, "invalid thread object");			\
    }									\
    (obj) = rb_thread_local_aref(th, bdb_id_current_env);		\
    if (TYPE(obj) != T_DATA ||						\
	RDATA(obj)->dmark != (RUBY_DATA_FUNC)bdb_env_mark) {		\
	obj = Qnil;							\
    }									\
    else {								\
	GetEnvDB(obj, envst);						\
    }									\
} while (0)

#if BDB_VERSION >= 40100
static ID id_app_dispatch;
#endif

struct db_stoptions {
    bdb_ENV *env;
    VALUE config;
    int lg_max, lg_bsize;
};

#if BDB_VERSION >= 40000
#if BDB_VERSION >= 40200
static int
bdb_env_rep_transport(DB_ENV *env, const DBT *control, const DBT *rec,
		      const DB_LSN *lsn, int envid, u_int32_t flags)

{
    VALUE obj, av, bv, res;
    bdb_ENV *envst;
    struct dblsnst *lsnst;
    VALUE lsnobj;

    GetIdEnv(obj, envst);
    lsnobj = bdb_makelsn(obj);
    Data_Get_Struct(lsnobj, struct dblsnst, lsnst);
    MEMCPY(lsnst->lsn, lsn, DB_LSN, 1);
    av = rb_tainted_str_new(control->data, control->size);
    bv = rb_tainted_str_new(rec->data, rec->size);
    if (envst->rep_transport == 0) {
	res = rb_funcall(obj, rb_intern("bdb_rep_transport"), 5, av, bv, lsnobj,
			 INT2FIX(envid), INT2FIX(flags));
    }
    else {
	res = rb_funcall(envst->rep_transport, bdb_id_call, 5, 
			 av, bv, lsnobj, INT2FIX(envid), INT2FIX(flags));
    }
    return NUM2INT(res);
}
#else
static int
bdb_env_rep_transport(DB_ENV *env, const DBT *control, const DBT *rec,
		      int envid, u_int32_t flags)
{
    VALUE obj, av, bv, res;
    bdb_ENV *envst;

    GetIdEnv(obj, envst);
    av = rb_tainted_str_new(control->data, control->size);
    bv = rb_tainted_str_new(rec->data, rec->size);
    if (envst->rep_transport == 0) {
	res = rb_funcall(obj, rb_intern("bdb_rep_transport"), 4, av, bv,
			 INT2FIX(envid), INT2FIX(flags));
    }
    else {
	res = rb_funcall(envst->rep_transport, bdb_id_call, 4, 
			 av, bv, INT2FIX(envid), INT2FIX(flags));
    }
    return NUM2INT(res);
}
#endif

static VALUE
bdb_env_rep_elect(int argc, VALUE *argv, VALUE env)
{
    VALUE nb, pri, ti, nvo;
    bdb_ENV *envst;
    int envid = 0, nvotes = 0;

    GetEnvDB(env, envst);
#if BDB_VERSION >= 40500
    pri = ti = 0;
    if (rb_scan_args(argc, argv, "11", &nb, &nvo) == 2) {
        nvotes = NUM2INT(nvo);
    }
#else
    if (rb_scan_args(argc, argv, "31", &nb, &pri, &ti, &nvo) == 4) {
        nvotes = NUM2INT(nvo);
    }
#endif
    
#if BDB_VERSION >= 40300
#if BDB_VERSION >= 40500
#if BDB_VERSION >= 40600
    bdb_test_error(envst->envp->rep_elect(envst->envp, NUM2INT(nb), nvotes, 0));
#else
    bdb_test_error(envst->envp->rep_elect(envst->envp, NUM2INT(nb), nvotes, &envid, 0));
#endif
#else
    bdb_test_error(envst->envp->rep_elect(envst->envp, NUM2INT(nb), nvotes,
					  NUM2INT(pri), NUM2INT(ti), &envid, 0));
#endif
#else
    bdb_test_error(envst->envp->rep_elect(envst->envp, NUM2INT(nb),
					  NUM2INT(pri), NUM2INT(ti), &envid));
#endif
    return INT2NUM(envid);
}

static VALUE
bdb_env_rep_process_message(VALUE env, VALUE av, VALUE bv, VALUE ev)
{
    bdb_ENV *envst;
    DBT control, rec;
    int ret, envid;
    VALUE result;
#if BDB_VERSION >= 40200
    VALUE lsn;
    struct dblsnst *lsnst;
#endif


    GetEnvDB(env, envst);
    av = rb_str_to_str(av);
    bv = rb_str_to_str(bv);
    MEMZERO(&control, DBT, 1);
    MEMZERO(&rec, DBT, 1);
    control.size = RSTRING_LEN(av);
    control.data = StringValuePtr(av);
    rec.size = RSTRING_LEN(bv);
    rec.data = StringValuePtr(bv);
    envid = NUM2INT(ev);
#if BDB_VERSION >= 40200
    lsn = bdb_makelsn(env);
    Data_Get_Struct(lsn, struct dblsnst, lsnst);

#if BDB_VERSION >= 40600
    ret = envst->envp->rep_process_message(envst->envp, &control, &rec, 
					   envid, lsnst->lsn);
#else
    ret = envst->envp->rep_process_message(envst->envp, &control, &rec, 
					   &envid, lsnst->lsn);
#endif
#else
    ret = envst->envp->rep_process_message(envst->envp, &control, &rec, 
					   &envid);
#endif
    if (ret == DB_RUNRECOVERY) {
	bdb_test_error(ret);
    }
    result = rb_ary_new();
    rb_ary_push(result, INT2NUM(ret));
    rb_ary_push(result, rb_str_new(rec.data, rec.size));
    rb_ary_push(result, INT2NUM(envid));
#if BDB_VERSION >= 40200
    if (ret == DB_REP_NOTPERM || ret == DB_REP_ISPERM) {
	rb_ary_push(result, lsn);
    }
#endif
    return result;
}

static VALUE
bdb_env_rep_start(VALUE env, VALUE ident, VALUE flags)
{
    bdb_ENV *envst;
    DBT cdata;

    GetEnvDB(env, envst);
    if (!NIL_P(ident)) {
	ident = rb_str_to_str(ident);
	MEMZERO(&cdata, DBT, 1);
	cdata.size = RSTRING_LEN(ident);
	cdata.data = StringValuePtr(ident);
    }
    bdb_test_error(envst->envp->rep_start(envst->envp, 
					  NIL_P(ident)?NULL:&cdata,
					  NUM2INT(flags)));
    return Qnil;
}

#if BDB_VERSION >= 40100

static VALUE
bdb_env_rep_limit(int argc, VALUE *argv, VALUE obj)
{
    bdb_ENV *envst;
    VALUE a, b;
    u_int32_t gbytes, bytes;

    GetEnvDB(obj, envst);
    gbytes = bytes = 0;
    switch(rb_scan_args(argc, argv, "11", &a, &b)) {
    case 1:
	if (TYPE(a) == T_ARRAY) {
	    if (RARRAY_LEN(a) != 2) {
		rb_raise(bdb_eFatal, "Expected an Array with 2 values");
	    }
	    gbytes = NUM2UINT(RARRAY_PTR(a)[0]);
	    bytes = NUM2UINT(RARRAY_PTR(a)[1]);
	}
	else {
	    bytes = NUM2UINT(RARRAY_PTR(a)[1]);
	}
	break;
    case 2:
	gbytes = NUM2UINT(a);
	bytes = NUM2UINT(b);
	break;
    }
#if BDB_VERSION >= 40500
    bdb_test_error(envst->envp->rep_set_limit(envst->envp, gbytes, bytes));
#else
    bdb_test_error(envst->envp->set_rep_limit(envst->envp, gbytes, bytes));
#endif
    return obj;
}
#endif
 
#endif

#if BDB_VERSION >= 30000
static void
bdb_env_feedback(DB_ENV *envp, int opcode, int pct)
{
    VALUE obj;
    bdb_ENV *envst;

    GetIdEnv(obj, envst);
    if (NIL_P(envst->feedback)) {
	return;
    }
    if (envst->feedback == 0) {
	rb_funcall(obj, id_feedback, 2, INT2NUM(opcode), INT2NUM(pct));
    }
    else {
	rb_funcall(envst->feedback, bdb_id_call, 2, INT2NUM(opcode), INT2NUM(pct));
    }
}

#endif

#if BDB_VERSION >= 40100

static int
bdb_env_app_dispatch(DB_ENV *envp, DBT *log_rec, DB_LSN *lsn, db_recops op)
{
    VALUE obj, lsnobj, logobj, res;
    bdb_ENV *envst;
    struct dblsnst *lsnst;

    GetIdEnv(obj, envst);
    lsnobj = bdb_makelsn(obj);
    Data_Get_Struct(lsnobj, struct dblsnst, lsnst);
    MEMCPY(lsnst->lsn, lsn, DB_LSN, 1);
    logobj = rb_str_new(log_rec->data, log_rec->size);
    if (envst->app_dispatch == 0) {
	res = rb_funcall(obj, id_app_dispatch, 3, logobj, lsnobj, INT2NUM(op));
    }
    else {
	res = rb_funcall(envst->app_dispatch, bdb_id_call, 3, logobj, lsnobj, INT2NUM(op));
    }
    return NUM2INT(res);
}

#endif

#if BDB_VERSION >= 40416

static ID id_msgcall;

static void
bdb_env_msgcall(const DB_ENV *dbenv, const char *msg)
{
    VALUE obj;
    bdb_ENV *envst;

    GetIdEnv(obj, envst);
    if (NIL_P(envst->msgcall)) {
	return;
    }
    if (envst->msgcall == 0) {
	rb_funcall(obj, id_msgcall, 1, rb_tainted_str_new2(msg));
    }
    else {
	rb_funcall(envst->msgcall, bdb_id_call, 1, rb_tainted_str_new2(msg));
    }
}

static ID id_thread_id;

static void
bdb_env_thread_id(DB_ENV *dbenv, pid_t *pid, db_threadid_t *tid)
{
    VALUE obj;
    bdb_ENV *envst;
    VALUE res;

    GetIdEnv(obj, envst);
    if (NIL_P(envst->thread_id)) {
	*pid = 0;
	*tid = 0;
	return;
    }
    if (envst->thread_id == 0) {
	res = rb_funcall2(obj, id_thread_id, 0, 0);
    }
    else {
	res = rb_funcall2(envst->thread_id, bdb_id_call, 0, 0);
    }
    res = rb_Array(res);
    if (TYPE(res) != T_ARRAY || RARRAY_LEN(res) != 2) {
	rb_raise(bdb_eFatal, "expected [pid, threadid]");
    }
    *pid = NUM2INT(RARRAY_PTR(res)[0]);
    *tid = NUM2INT(RARRAY_PTR(res)[0]);
    return;
}

static ID id_thread_id_string;

static char *
bdb_env_thread_id_string(DB_ENV *dbenv, pid_t pid, db_threadid_t tid, char *buf)
{
    VALUE obj;
    bdb_ENV *envst;
    VALUE res, a, b;

    GetIdEnv(obj, envst);
    if (NIL_P(envst->thread_id_string)) {
	snprintf(buf, DB_THREADID_STRLEN, "%d/%d", pid, (int)tid);
	return buf;
    }
    a = INT2NUM(pid);
    b = INT2NUM(tid);
    if (envst->thread_id_string == 0) {
	res = rb_funcall(obj, id_thread_id_string, 2, a, b);
    }
    else {
	res = rb_funcall(envst->thread_id_string, bdb_id_call, 2, a, b);
    }
    snprintf(buf, DB_THREADID_STRLEN, "%s", StringValuePtr(res));
    return buf;
}

static ID id_isalive;

#if BDB_VERSION >= 40500

static int
bdb_env_isalive(DB_ENV *dbenv, pid_t pid, db_threadid_t tid, u_int32_t flags)
{
    VALUE obj;
    bdb_ENV *envst;
    VALUE res, a ,b, c;

    GetIdEnv(obj, envst);
    if (NIL_P(envst->isalive)) {
	return 0;
    }
    a = INT2NUM(pid);
    b = INT2NUM(tid);
    c = INT2NUM(flags);
    if (envst->isalive == 0) {
	res = rb_funcall(obj, id_isalive, 3, a, b, c);
    }
    else {
	res = rb_funcall(envst->isalive, bdb_id_call, 3, a, b, c);
    }
    if (RTEST(res)) {
	return 1;
    }
    return 0;
}

#else

static int
bdb_env_isalive(DB_ENV *dbenv, pid_t pid, db_threadid_t tid)
{
    VALUE obj;
    bdb_ENV *envst;
    VALUE res, a ,b;

    GetIdEnv(obj, envst);
    if (NIL_P(envst->isalive)) {
	return 0;
    }
    a = INT2NUM(pid);
    b = INT2NUM(tid);
    if (envst->isalive == 0) {
	res = rb_funcall(obj, id_isalive, 2, a, b);
    }
    else {
	res = rb_funcall(envst->isalive, bdb_id_call, 2, a, b);
    }
    if (RTEST(res)) {
	return 1;
    }
    return 0;
}

#endif

#endif

#if BDB_VERSION >= 40500

static ID id_event_notify;

static void
bdb_env_event_notify(DB_ENV *dbenv, u_int32_t event, void *event_info)
{
    VALUE obj;
    bdb_ENV *envst;
    VALUE res, a ,b;

    GetIdEnv(obj, envst);
    if (NIL_P(envst->event_notify)) {
	return;
    }
    a = INT2NUM(event);
    if (envst->event_notify == 0) {
	rb_funcall(obj, id_event_notify, 1, a);
    }
    else {
	rb_funcall(envst->event_notify, bdb_id_call, 1, a);
    }
}

static VALUE
bdb_env_set_notify(VALUE obj, VALUE a)
{
    bdb_ENV *envst;

    GetEnvDB(obj, envst);
    if (!NIL_P(a)) {
	if (!rb_respond_to(a, bdb_id_call)) {
	    rb_raise(rb_eArgError, "object must respond to #call");
	}
	envst->envp->set_event_notify(envst->envp, bdb_env_event_notify);
    }
    envst->event_notify = a;
    return a;
}

#endif


static VALUE
bdb_env_i_options(VALUE obj, VALUE db_stobj)
{
    char *options;
    DB_ENV *envp;
    VALUE key, value;
    bdb_ENV *envst;
    struct db_stoptions *db_st;

    Data_Get_Struct(db_stobj, struct db_stoptions, db_st);
    envst = db_st->env;

    key = rb_ary_entry(obj, 0);
    value = rb_ary_entry(obj, 1);
    envp = envst->envp;
    key = rb_obj_as_string(key);
    options = StringValuePtr(key);
    if (strcmp(options, "set_cachesize") == 0) {
	switch (TYPE(value)) {
	case T_FIXNUM:
	case T_FLOAT:
	case T_BIGNUM:
#if BDB_VERSION < 30000
	    envp->mp_size = NUM2INT(value);
#else
	    bdb_test_error(envp->set_cachesize(envp, 0, NUM2UINT(value), 0));
#endif
	    break;
	default:
	    Check_Type(value, T_ARRAY);
	    if (RARRAY_LEN(value) < 3) {
		rb_raise(bdb_eFatal, "expected 3 values for cachesize");
	    }
#if BDB_VERSION < 30000
	    envp->mp_size = NUM2INT(RARRAY_PTR(value)[1]);
#else
	    bdb_test_error(envp->set_cachesize(envp, 
					       NUM2UINT(RARRAY_PTR(value)[0]),
					       NUM2UINT(RARRAY_PTR(value)[1]),
					       NUM2INT(RARRAY_PTR(value)[2])));
#endif
	    break;
	}
    }
#if DB_VERSION_MAJOR == 3
    else if (strcmp(options, "set_region_init") == 0) {
#if BDB_VERSION < 30114
        bdb_test_error(envp->set_region_init(envp, NUM2INT(value)));
#else
        bdb_test_error(db_env_set_region_init(NUM2INT(value)));
#endif
    }
#endif
#if BDB_VERSION >= 30000
    else if (strcmp(options, "set_tas_spins") == 0) {
#if BDB_VERSION < 30114 || BDB_VERSION >= 40000
#if BDB_VERSION >= 40416
	rb_warning("Invalid option :set_tas_spins");
#else
        bdb_test_error(envp->set_tas_spins(envp, NUM2INT(value)));
#endif
#else
        bdb_test_error(db_env_set_tas_spins(NUM2INT(value)));
#endif
    }
    else if (strcmp(options, "set_tx_max") == 0) {
        bdb_test_error(envp->set_tx_max(envp, NUM2INT(value)));
    }
#if BDB_VERSION >= 30100
    else if (strcmp(options, "set_tx_timestamp") == 0) {
	time_t ti;
	value = rb_Integer(value);
	ti = (time_t)NUM2INT(value);
        bdb_test_error(envp->set_tx_timestamp(envp, &ti));
    }
#endif
#endif
#if BDB_VERSION < 30000
    else if  (strcmp(options, "set_verbose") == 0) {
        envp->db_verbose = NUM2INT(value);
    }
#else
#if BDB_VERSION < 40300
    else if (strcmp(options, "set_verb_chkpoint") == 0) {
        bdb_test_error(envp->set_verbose(envp, DB_VERB_CHKPOINT, NUM2INT(value)));
    }
#endif
    else if (strcmp(options, "set_verb_deadlock") == 0) {
        bdb_test_error(envp->set_verbose(envp, DB_VERB_DEADLOCK, NUM2INT(value)));
    }
    else if (strcmp(options, "set_verb_recovery") == 0) {
        bdb_test_error(envp->set_verbose(envp, DB_VERB_RECOVERY, NUM2INT(value)));
    }
    else if (strcmp(options, "set_verb_waitsfor") == 0) {
        bdb_test_error(envp->set_verbose(envp, DB_VERB_WAITSFOR, NUM2INT(value)));
    }
#if BDB_VERSION >= 40000
    else if (strcmp(options, "set_verb_replication") == 0) {
        bdb_test_error(envp->set_verbose(envp, DB_VERB_REPLICATION, NUM2INT(value)));
    }
#endif
#endif
    else if (strcmp(options, "set_lk_detect") == 0) {
#if BDB_VERSION < 30000
	envp->lk_detect = NUM2INT(value);
#else
        bdb_test_error(envp->set_lk_detect(envp, NUM2INT(value)));
#endif
    }
#if BDB_VERSION < 40500
    else if (strcmp(options, "set_lk_max") == 0) {
#if BDB_VERSION < 30000
	envp->lk_max = NUM2INT(value);
#else
        bdb_test_error(envp->set_lk_max(envp, NUM2INT(value)));
#endif
    }
#endif
    else if (strcmp(options, "set_lk_conflicts") == 0) {
	int i, j, l, v;
	unsigned char *conflits, *p;

	Check_Type(value, T_ARRAY);
	l = RARRAY_LEN(value);
	p = conflits = ALLOC_N(unsigned char, l * l);
	for (i = 0; i < l; i++) {
	    if (TYPE(RARRAY_PTR(value)[i]) != T_ARRAY ||
		RARRAY_LEN(RARRAY_PTR(value)[i]) != l) {
		free(conflits);
		rb_raise(bdb_eFatal, "invalid array for lk_conflicts");
	    }
	    for (j = 0; j < l; j++, p++) {
		if (TYPE(RARRAY_PTR(RARRAY_PTR(value)[i])[j]) != T_FIXNUM) {
		    free(conflits);
		    rb_raise(bdb_eFatal, "invalid value for lk_conflicts");
		}
		v = NUM2INT(RARRAY_PTR(RARRAY_PTR(value)[i])[j]);
		if (v != 0 && v != 1) {
		    free(conflits);
		    rb_raise(bdb_eFatal, "invalid value for lk_conflicts");
		}
		*p = (unsigned char)v;
	    }
	}
#if BDB_VERSION < 30000
	envp->lk_modes = l;
	envp->lk_conflicts = conflits;
#else
        bdb_test_error(envp->set_lk_conflicts(envp, conflits, l));
#endif
    }
#if BDB_VERSION >= 30200
    else if (strcmp(options, "set_lk_max_locks") == 0) {
        bdb_test_error(envp->set_lk_max_locks(envp, NUM2INT(value)));
    }
    else if (strcmp(options, "set_lk_max_lockers") == 0) {
        bdb_test_error(envp->set_lk_max_lockers(envp, NUM2INT(value)));
    }
    else if (strcmp(options, "set_lk_max_objects") == 0) {
        bdb_test_error(envp->set_lk_max_objects(envp, NUM2INT(value)));
    }
#endif
    else if (strcmp(options, "set_lg_max") == 0) {
	db_st->lg_max = NUM2INT(value);
    }
#if BDB_VERSION >= 30000
    else if (strcmp(options, "set_lg_bsize") == 0) {
	db_st->lg_bsize = NUM2INT(value);
    }
#endif
    else if (strcmp(options, "set_data_dir") == 0) {
	SafeStringValue(value);
#if BDB_VERSION >= 30100
	bdb_test_error(envp->set_data_dir(envp, StringValuePtr(value)));
#else
	{
	    char *tmp;

	    tmp = ALLOCA_N(char, strlen("DB_DATA_DIR") + RSTRING_LEN(value) + 2);
	    sprintf(tmp, "DB_DATA_DIR %s", StringValuePtr(value));
	    rb_ary_push(db_st->config, rb_str_new2(tmp));
	}
#endif
    }
    else if (strcmp(options, "set_lg_dir") == 0) {
	SafeStringValue(value);
#if BDB_VERSION >= 30100
	bdb_test_error(envp->set_lg_dir(envp, StringValuePtr(value)));
#else
	{
	    char *tmp;

	    tmp = ALLOCA_N(char, strlen("DB_LOG_DIR") + RSTRING_LEN(value) + 2);
	    sprintf(tmp, "DB_LOG_DIR %s", StringValuePtr(value));
	    rb_ary_push(db_st->config, rb_str_new2(tmp));
	}
#endif
    }
    else if (strcmp(options, "set_tmp_dir") == 0) {
	SafeStringValue(value);
#if BDB_VERSION >= 30100
	bdb_test_error(envp->set_tmp_dir(envp, StringValuePtr(value)));
#else
	{
	    char *tmp;

	    tmp = ALLOCA_N(char, strlen("DB_TMP_DIR") + RSTRING_LEN(value) + 2);
	    sprintf(tmp, "DB_TMP_DIR %s", StringValuePtr(value));
	    rb_ary_push(db_st->config, rb_str_new2(tmp));
	}
#endif
    }
#if BDB_VERSION >= 30100
    else if (strcmp(options, "set_server") == 0 ||
	strcmp(options, "set_rpc_server") == 0) {
	char *host;
	long sv_timeout, cl_timeout;
	unsigned long flags;

	host = 0;
	sv_timeout = cl_timeout = 0;
	flags = 0;
	switch (TYPE(value)) {
	case T_STRING:
	    SafeStringValue(value);
	    host = StringValuePtr(value);
	    break;
	case T_ARRAY:
	    switch (RARRAY_LEN(value)) {
	    default:
	    case 3:
		sv_timeout = NUM2INT(RARRAY_PTR(value)[2]);
	    case 2:
		cl_timeout = NUM2INT(RARRAY_PTR(value)[1]);
	    case 1:
		SafeStringValue(RARRAY_PTR(value)[0]);
		host = StringValuePtr(RARRAY_PTR(value)[0]);
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
#if BDB_VERSION >= 30300
	bdb_test_error(envp->set_rpc_server(envp, NULL, host, cl_timeout, sv_timeout, flags));
#else
	bdb_test_error(envp->set_server(envp, host, cl_timeout, sv_timeout, flags));
#endif
    }
#endif
#if BDB_VERSION >= 30200
    else if (strcmp(options, "set_flags") == 0) {
        bdb_test_error(envp->set_flags(envp, NUM2UINT(value), 1));
    }
#endif
    else if (strcmp(options, "marshal") == 0) {
        switch (value) {
        case Qtrue: envst->marshal = bdb_mMarshal; break;
        case Qfalse: envst->marshal = Qfalse; break;
        default: 
	    if (!bdb_respond_to(value, bdb_id_load) ||
		!bdb_respond_to(value, bdb_id_dump)) {
		rb_raise(bdb_eFatal, "marshal value must be true or false");
	    }
	    envst->marshal = value;
	    break;
        }
    }
    else if (strcmp(options, "thread") == 0) {
	if (RTEST(value)) {
	    envst->options &= ~BDB_NO_THREAD;
	}
	else {
	    envst->options |= BDB_NO_THREAD;
	}
    }
#if BDB_VERSION >= 40000
    else if (strcmp(options, "set_rep_transport") == 0 ||
	strcmp(options, "rep_set_transport") == 0) {
	if (TYPE(value) != T_ARRAY || RARRAY_LEN(value) != 2) {
	    rb_raise(bdb_eFatal, "expected an Array of length 2 for set_rep_transport");
	}
	if (!FIXNUM_P(RARRAY_PTR(value)[0])) {
	    rb_raise(bdb_eFatal, "expected a Fixnum for the 1st arg of set_rep_transport");
	}
	if (!rb_respond_to(RARRAY_PTR(value)[1], bdb_id_call)) {
	    rb_raise(bdb_eFatal, "2nd arg must respond to #call");
	}
	envst->rep_transport = RARRAY_PTR(value)[1];
#if BDB_VERSION >= 40500
	bdb_test_error(envst->envp->rep_set_transport(envst->envp, NUM2INT(RARRAY_PTR(value)[0]), bdb_env_rep_transport));
#else
	bdb_test_error(envst->envp->set_rep_transport(envst->envp, NUM2INT(RARRAY_PTR(value)[0]), bdb_env_rep_transport));
#endif
	envst->options |= BDB_REP_TRANSPORT;
    }
    else if (strcmp(options, "set_timeout") == 0) {
	if (TYPE(value) == T_ARRAY) {
	    if (RARRAY_LEN(value) >= 1 && !NIL_P(RARRAY_PTR(value)[0])) {

		bdb_test_error(envst->envp->set_timeout(envst->envp, 
							NUM2UINT(RARRAY_PTR(value)[0]),
							DB_SET_TXN_TIMEOUT));
	    }
	    if (RARRAY_LEN(value) == 2 && !NIL_P(RARRAY_PTR(value)[1])) {
		bdb_test_error(envst->envp->set_timeout(envst->envp, 
							NUM2UINT(RARRAY_PTR(value)[0]),
							DB_SET_LOCK_TIMEOUT));
	    }
	}
	else {
	    bdb_test_error(envst->envp->set_timeout(envst->envp,
						    NUM2UINT(value),
						    DB_SET_TXN_TIMEOUT));
	}
    }
    else if (strcmp(options, "set_txn_timeout") == 0) {
	bdb_test_error(envst->envp->set_timeout(envst->envp,
						NUM2UINT(value),
						DB_SET_TXN_TIMEOUT));
    }
    else if (strcmp(options, "set_lock_timeout") == 0) {
	bdb_test_error(envst->envp->set_timeout(envst->envp,
						NUM2UINT(value),
						DB_SET_LOCK_TIMEOUT));
    }
#endif
#if BDB_VERSION >= 40100
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
	bdb_test_error(envst->envp->set_encrypt(envst->envp, passwd, flags));
	envst->options |= BDB_ENV_ENCRYPT;
    }
    else if (strcmp(options, "set_rep_limit") == 0 ||
	strcmp(options, "rep_set_limit") == 0) {
	u_int32_t gbytes, bytes;
	gbytes = bytes = 0;
	if (TYPE(value) == T_ARRAY) {
	    if (RARRAY_LEN(value) != 2) {
		rb_raise(bdb_eFatal, "Expected an Array with 2 values");
	    }
	    gbytes = NUM2UINT(RARRAY_PTR(value)[0]);
	    bytes = NUM2UINT(RARRAY_PTR(value)[1]);
	}
	else {
	    bytes = NUM2UINT(RARRAY_PTR(value)[1]);
	}
#if BDB_VERSION >= 40500
	bdb_test_error(envst->envp->rep_set_limit(envst->envp, gbytes, bytes));
#else
	bdb_test_error(envst->envp->set_rep_limit(envst->envp, gbytes, bytes));
#endif
    }
#endif
#if BDB_VERSION >= 30000
    else if (strcmp(options, "set_feedback") == 0) {
	if (!rb_respond_to(value, bdb_id_call)) {
	    rb_raise(bdb_eFatal, "arg must respond to #call");
	}
	envst->options |= BDB_FEEDBACK;
	envst->feedback = value;
	envst->envp->set_feedback(envst->envp, bdb_env_feedback);
    }
#endif
#if BDB_VERSION >= 40100
    else if (strcmp(options, "set_app_dispatch") == 0) {
	if (!rb_respond_to(value, bdb_id_call)) {
	    rb_raise(bdb_eFatal, "arg must respond to #call");
	}
	envst->options |= BDB_APP_DISPATCH;
	envst->app_dispatch = value;
	envst->envp->set_app_dispatch(envst->envp, bdb_env_app_dispatch);
    }
#endif
#if BDB_VERSION >= 40416
    else if (strcmp(options, "set_msgcall") == 0) {
	if (!rb_respond_to(value, bdb_id_call)) {
	    rb_raise(bdb_eFatal, "arg must respond to #call");
	}
	envst->msgcall = value;
	envst->envp->set_msgcall(envst->envp, bdb_env_msgcall);
    }
    else if (strcmp(options, "set_thread_id") == 0) {
	if (!rb_respond_to(value, bdb_id_call)) {
	    rb_raise(bdb_eFatal, "arg must respond to #call");
	}
	envst->thread_id = value;
	envst->envp->set_thread_id(envst->envp, bdb_env_thread_id);
    }
    else if (strcmp(options, "set_thread_id_string") == 0) {
	if (!rb_respond_to(value, bdb_id_call)) {
	    rb_raise(bdb_eFatal, "arg must respond to #call");
	}
	envst->thread_id_string = value;
	envst->envp->set_thread_id_string(envst->envp, bdb_env_thread_id_string);
    }
    else if (strcmp(options, "set_isalive") == 0) {
	if (!rb_respond_to(value, bdb_id_call)) {
	    rb_raise(bdb_eFatal, "arg must respond to #call");
	}
	envst->isalive = value;
	envst->envp->set_isalive(envst->envp, bdb_env_isalive);
    }
#endif
#if BDB_VERSION >= 40250
    else if (strcmp(options, "set_shm_key") == 0) {
	bdb_test_error(envst->envp->set_shm_key(envst->envp, NUM2INT(value)));
    }
#endif
#if BDB_VERSION >= 40500
    else if (strcmp(options, "set_rep_nsites") == 0 ||
	     strcmp(options, "rep_set_nsites") == 0) {
	bdb_test_error(envst->envp->rep_set_nsites(envst->envp, NUM2INT(value)));
    }
    else if (strcmp(options, "set_rep_priority") == 0 ||
	     strcmp(options, "rep_set_priority") == 0) {
	bdb_test_error(envst->envp->rep_set_priority(envst->envp, NUM2INT(value)));
    }
    else if (strcmp(options, "set_rep_config") == 0 ||
	     strcmp(options, "rep_set_config") == 0) {
	int onoff = 0;

	if (TYPE(value) != T_ARRAY || RARRAY_LEN(value) != 2) {
	    rb_raise(bdb_eFatal, "Expected an Array with 2 values");
	}
	if (RARRAY_PTR(value)[1] == Qtrue) {
	    onoff = 1;
	}
	else if (RARRAY_PTR(value)[1] == Qfalse ||
		 RARRAY_PTR(value)[1] == Qnil) {
	    onoff = 0;
	}
	else {
	    onoff = NUM2UINT(RARRAY_PTR(value)[1]);
	}
	bdb_test_error(envst->envp->rep_set_config(envst->envp, 
						   NUM2UINT(RARRAY_PTR(value)[0]),
						   onoff));
    }
    else if (strcmp(options, "set_rep_timeout") == 0 ||
	     strcmp(options, "rep_set_timeout") == 0) {
	if (TYPE(value) != T_ARRAY || RARRAY_LEN(value) != 2) {
	    rb_raise(bdb_eFatal, "Expected an Array with 2 values");
	}
	bdb_test_error(envst->envp->rep_set_timeout(envst->envp, 
						    NUM2UINT(RARRAY_PTR(value)[0]),
						    NUM2UINT(RARRAY_PTR(value)[1])));
    }
    else if (strcmp(options, "repmgr_set_local_site") == 0 ||
	     strcmp(options, "set_repmgr_local_site") == 0) {
	if (TYPE(value) != T_ARRAY || RARRAY_LEN(value) != 2) {
	    rb_raise(bdb_eFatal, "Expected an Array with 2 values");
	}
	bdb_test_error(envst->envp->repmgr_set_local_site(envst->envp,
							  StringValuePtr(RARRAY_PTR(value)[0]),
							  NUM2UINT(RARRAY_PTR(value)[1]),
							  0));
    }
    else if (strcmp(options, "repmgr_add_remote_site") == 0) {
	int flags = 0;

	if (TYPE(value) != T_ARRAY || RARRAY_LEN(value) < 2 || RARRAY_LEN(value) > 4) {
	    rb_raise(bdb_eFatal, "Expected an Array with 3 values");
	}
	if (RARRAY_LEN(value) == 3) {
	    flags = NUM2INT(RARRAY_PTR(value)[2]);
	}
	bdb_test_error(envst->envp->repmgr_add_remote_site(envst->envp,
							   StringValuePtr(RARRAY_PTR(value)[0]),
							   NUM2UINT(RARRAY_PTR(value)[1]),
							   NULL, flags));
    }
    else if (strcmp(options, "repmgr_set_ack_policy") == 0 ||
	     strcmp(options, "set_repmgr_ack_policy") == 0) {
	bdb_test_error(envst->envp->repmgr_set_ack_policy(envst->envp, NUM2UINT(value)));
    }

#endif
    return Qnil;
}

struct env_iv {
    bdb_ENV *envst;
    VALUE env;
};

#if BDB_VERSION >= 30000

static VALUE
bdb_env_aref()
{
    VALUE obj;
    bdb_ENV *envst;

    GetIdEnv1(obj, envst);
    return obj;
}

#endif

VALUE
bdb_protect_close(VALUE obj)
{
    return rb_funcall2(obj, rb_intern("close"), 0, 0);
}

static void
bdb_final(bdb_ENV *envst)
{
    VALUE obj, *ary;
    bdb_ENV *thst;
    int i;

    ary = envst->db_ary.ptr;
    if (ary) {
        envst->db_ary.mark = Qtrue;
        for (i = 0; i < envst->db_ary.len; i++) {
            if (rb_respond_to(ary[i], rb_intern("close"))) {
                rb_protect(bdb_protect_close, ary[i], 0);
            }
        }
        envst->db_ary.mark = Qfalse;
        envst->db_ary.total = envst->db_ary.len = 0;
        envst->db_ary.ptr = 0;
        free(ary);
    }
    if (envst->envp) {
	if (!(envst->options & BDB_ENV_NOT_OPEN)) {
#if BDB_VERSION < 30000
	    db_appexit(envst->envp);
	    free(envst->envp);
#else
	    envst->envp->close(envst->envp, 0);
#endif
	}
	envst->envp = NULL;
    }
#if BDB_VERSION >= 30000
    {
	int status;

	obj = rb_protect(bdb_env_aref, 0, &status);
	if (!status && !NIL_P(obj)) {
	    Data_Get_Struct(obj, bdb_ENV, thst);
	    if (thst == envst) {
		rb_thread_local_aset(rb_thread_current(), bdb_id_current_env, Qnil);
	    }
	}
    }
#endif
}

static void
bdb_env_free(bdb_ENV *envst)
{
    bdb_final(envst);
    free(envst);
}

static VALUE
bdb_env_close(VALUE obj)
{
    bdb_ENV *envst;

    if (!OBJ_TAINTED(obj) && rb_safe_level() >= 4) {
	rb_raise(rb_eSecurityError, "Insecure: can't close the environnement");
    }
    GetEnvDB(obj, envst);
    bdb_final(envst);
    RDATA(obj)->dfree = free;
    return Qnil;
}

static void
bdb_env_mark(bdb_ENV *envst)
{ 
    rb_gc_mark(envst->marshal);
#if BDB_VERSION >= 40000
    rb_gc_mark(envst->rep_transport);
#if BDB_VERSION >= 40100
    rb_gc_mark(envst->app_dispatch);
#endif
#endif
#if BDB_VERSION >= 40416
    rb_gc_mark(envst->msgcall);
    rb_gc_mark(envst->thread_id);
    rb_gc_mark(envst->thread_id_string);
    rb_gc_mark(envst->isalive);
#endif
#if BDB_VERSION >= 30000
    rb_gc_mark(envst->feedback);
#endif
#if BDB_VERSION >= 40500
    rb_gc_mark(envst->event_notify);
#endif
    rb_gc_mark(envst->home);
    bdb_ary_mark(&envst->db_ary);
}

VALUE
bdb_env_open_db(int argc, VALUE *argv, VALUE obj)
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
#if BDB_VERSION >= 30000
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
    return rb_funcall2(cl, rb_intern("new"), argc, argv);
}

#if BDB_VERSION >= 40300
void
bdb_env_errcall(const DB_ENV *env, const char *errpfx, const char *msg)
{
    bdb_errcall = 1;
    bdb_errstr = rb_tainted_str_new2(msg);
}

#else

void
bdb_env_errcall(const char *errpfx, char *msg)
{
    bdb_errcall = 1;
    bdb_errstr = rb_tainted_str_new2(msg);
}

#endif

VALUE
bdb_return_err()
{
    if (bdb_errcall) {
	bdb_errcall = 0;
	return bdb_errstr;
    }
    return Qnil;
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
bdb_func_sleep(unsigned long sec, unsigned long usec)
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
bdb_func_malloc(size_t size)
{
    return malloc(size);
}

static VALUE
bdb_set_func(bdb_ENV *envst)
{
#if BDB_VERSION >= 30000
#if BDB_VERSION < 30114
    bdb_test_error(envst->envp->set_func_sleep(envst->envp, bdb_func_sleep));
    bdb_test_error(envst->envp->set_func_yield(envst->envp, bdb_func_yield));
#else
    bdb_test_error(db_env_set_func_sleep(bdb_func_sleep));
    bdb_test_error(db_env_set_func_yield(bdb_func_yield));
#endif
#else
    bdb_test_error(db_jump_set((void *)bdb_func_sleep, DB_FUNC_SLEEP));
    bdb_test_error(db_jump_set((void *)bdb_func_yield, DB_FUNC_YIELD));
#endif
    return Qtrue;
}

static VALUE
bdb_env_each_options(VALUE opt, VALUE stobj)
{
    VALUE res;
    DB_ENV *envp;
    struct db_stoptions *db_st;

    res = rb_iterate(rb_each, opt, bdb_env_i_options, stobj);
    Data_Get_Struct(stobj, struct db_stoptions, db_st);
    envp = db_st->env->envp;
#if BDB_VERSION >= 30000
    if (db_st->lg_bsize) {
	bdb_test_error(envp->set_lg_bsize(envp, db_st->lg_bsize));
    }
#endif
    if (db_st->lg_max) {
#if BDB_VERSION < 30000
	envp->lg_max = db_st->lg_max;
#else
	bdb_test_error(envp->set_lg_max(envp, db_st->lg_max));
#endif
    }
    return res;
}

static VALUE
bdb_env_s_i_options(VALUE obj, int *flags)
{
    char *options;
    VALUE key, value;

    key = rb_ary_entry(obj, 0);
    value = rb_ary_entry(obj, 1);
    key = rb_obj_as_string(key);
    options = StringValuePtr(key);
    if (strcmp(options, "env_flags") == 0) {
	*flags = NUM2INT(value);
    }
#ifdef DB_CLIENT
    else if (strcmp(options, "set_rpc_server") == 0 ||
	     strcmp(options, "set_server") == 0) {
	*flags |= DB_CLIENT;
    }
#endif
    return Qnil;
}

#if BDB_VERSION >= 40500

static VALUE
bdb_env_s_j_options(VALUE obj, VALUE *res)
{
    char *options;
    VALUE key, value;

    key = rb_ary_entry(obj, 0);
    value = rb_ary_entry(obj, 1);
    key = rb_obj_as_string(key);
    options = StringValuePtr(key);
    if (strcmp(options, "set_event_notify") == 0) {
	*res = value;
    }
    return Qnil;
}

#endif

static VALUE
bdb_env_s_alloc(VALUE obj)
{
    VALUE res;
    bdb_ENV *envst;

    res = Data_Make_Struct(obj, bdb_ENV, bdb_env_mark, bdb_env_free, envst);
    envst->options |= BDB_ENV_NOT_OPEN;
    return res;
}

static VALUE
bdb_env_s_new(int argc, VALUE *argv, VALUE obj)
{
    bdb_ENV *envst;
    VALUE res;
    int flags = 0;

#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
    res = rb_obj_alloc(obj);
#else
    res = rb_funcall2(obj, rb_intern("allocate"), 0, 0);
#endif
    Data_Get_Struct(res, bdb_ENV, envst);
#if BDB_VERSION < 30000
    envst->envp = ALLOC(DB_ENV);
    MEMZERO(envst->envp, DB_ENV, 1);
    envst->envp->db_errpfx = "BDB::";
    envst->envp->db_errcall = bdb_env_errcall;
#else
#if BDB_VERSION >= 30100
    if (argc && TYPE(argv[argc - 1]) == T_HASH) {
	rb_iterate(rb_each, argv[argc - 1], bdb_env_s_i_options, (VALUE)&flags);
    }
#endif
    bdb_test_error(db_env_create(&(envst->envp), flags));
    envst->envp->set_errpfx(envst->envp, "BDB::");
    envst->envp->set_errcall(envst->envp, bdb_env_errcall);
#if BDB_VERSION >= 30300
    bdb_test_error(envst->envp->set_alloc(envst->envp, malloc, realloc, free));
#endif
#if BDB_VERSION >= 40500
    if (argc && TYPE(argv[argc - 1]) == T_HASH) {
	VALUE value = Qnil;

	rb_iterate(rb_each, argv[argc - 1], bdb_env_s_j_options, (VALUE)&value);
	if (!NIL_P(value)) {
	    if (!rb_respond_to(value, bdb_id_call)) {
		rb_raise(bdb_eFatal, "arg must respond to #call");
	    }
	    envst->event_notify = value;
	    envst->envp->set_event_notify(envst->envp, bdb_env_event_notify);
	}
    }
#endif
#endif
    rb_obj_call_init(res, argc, argv);
    return res;
}


#if BDB_VERSION >= 40000

VALUE 
bdb_env_s_rslbl(int argc, VALUE *argv, VALUE obj, DB_ENV *env)
{
    bdb_ENV *envst;
    VALUE res;

#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
    res = rb_obj_alloc(obj);
#else
    res = rb_funcall2(obj, rb_intern("allocate"), 0, 0);
#endif
    Data_Get_Struct(res, bdb_ENV, envst);
    envst->envp = env;
    envst->envp->set_errpfx(envst->envp, "BDB::");
    envst->envp->set_errcall(envst->envp, bdb_env_errcall);
    bdb_test_error(envst->envp->set_alloc(envst->envp, malloc, realloc, free));
    rb_obj_call_init(res, argc, argv);
    return res;
}

#endif

VALUE
bdb_env_init(int argc, VALUE *argv, VALUE obj)
{
    DB_ENV *envp;
    bdb_ENV *envst;
    VALUE a, c, d;
    char *db_home, **db_config;
    int ret, mode, flags;
    VALUE st_config;
    VALUE envid;

    envid = 0;
    st_config = 0;
    db_config = 0;
    mode = flags = 0;
    if (!RDATA(obj)->dmark) {
        RDATA(obj)->dmark = (RUBY_DATA_FUNC)bdb_env_mark;
    }
    Data_Get_Struct(obj, bdb_ENV, envst);
#if BDB_VERSION >= 30000
    envst->envp->set_errcall(envst->envp, bdb_env_errcall);
#endif
    envp = envst->envp;
#if BDB_VERSION >= 40100
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
	bdb_test_error(envp->set_encrypt(envp, passwd, flags));
	envst->options |= BDB_ENV_ENCRYPT;
    }
#endif
    if (argc && TYPE(argv[argc - 1]) == T_HASH) {
	int i;
	VALUE db_stobj;
	struct db_stoptions *db_st;

	st_config = rb_ary_new();
	db_stobj = Data_Make_Struct(rb_cObject, struct db_stoptions, 0, free, db_st);
	db_st->env = envst;
	db_st->config = st_config;
	bdb_env_each_options(argv[argc - 1], db_stobj);
	if (RARRAY_LEN(st_config) > 0) {
	    db_config = ALLOCA_N(char *, RARRAY_LEN(st_config) + 1);
	    for (i = 0; i < RARRAY_LEN(st_config); i++) {
		db_config[i] = StringValuePtr(RARRAY_PTR(st_config)[i]);
	    }
	    db_config[RARRAY_LEN(st_config)] = 0;
	}
	argc--;
    }
    rb_scan_args(argc, argv, "12", &a, &c, &d);
    SafeStringValue(a);
    db_home = StringValuePtr(a);
    switch (argc) {
    case 3:
	mode = NUM2INT(d);
    case 2:
	flags = NUM2INT(c);
        break;
    }
    if (flags & DB_CREATE) {
	rb_secure(4);
    }
    if (flags & DB_USE_ENVIRON) {
	rb_secure(1);
    }
#ifndef BDB_NO_THREAD_COMPILE
    if (!(envst->options & BDB_NO_THREAD)) {
	bdb_set_func(envst);
	flags |= DB_THREAD;
    }
#endif
#if BDB_VERSION < 30000
    if ((ret = db_appinit(db_home, db_config, envp, flags)) != 0) {
	if (envst->envp) {
	    free(envst->envp);
	}
	envst->envp = NULL;
        if (bdb_errcall) {
            bdb_errcall = 0;
            rb_raise(bdb_eFatal, "%s -- %s", StringValuePtr(bdb_errstr), db_strerror(ret));
        }
        else
            rb_raise(bdb_eFatal, "%s", db_strerror(ret));
    }
#else
#if BDB_VERSION >= 40500
    if (envst->event_notify == 0 && rb_respond_to(obj, id_event_notify) == Qtrue) {
	envp->set_event_notify(envp, bdb_env_event_notify);
    }
#endif
#if BDB_VERSION >= 40000
    if (envst->rep_transport == 0 && rb_respond_to(obj, rb_intern("bdb_rep_transport")) == Qtrue) {
	if (!rb_const_defined(CLASS_OF(obj), rb_intern("ENVID"))) {
	    rb_raise(bdb_eFatal, "ENVID must be defined to use rep_transport");
	}
	envid = rb_const_get(CLASS_OF(obj), rb_intern("ENVID"));
#if BDB_VERSION >= 40500
	bdb_test_error(envp->rep_set_transport(envp, NUM2INT(envid),
						 bdb_env_rep_transport));
#else
	bdb_test_error(envp->set_rep_transport(envp, NUM2INT(envid),
						 bdb_env_rep_transport));
#endif
	envst->options |= BDB_REP_TRANSPORT;
    }
#endif
#if BDB_VERSION >= 30000
    if (envst->feedback == 0 && rb_respond_to(obj, id_feedback) == Qtrue) {
	envp->set_feedback(envp, bdb_env_feedback);
	envst->options |= BDB_FEEDBACK;
    }
#endif
#if BDB_VERSION >= 40100
    if (envst->app_dispatch == 0 && rb_respond_to(obj, id_app_dispatch) == Qtrue) {
	envp->set_app_dispatch(envp, bdb_env_app_dispatch);
	envst->options |= BDB_APP_DISPATCH;
    }
#endif
#if BDB_VERSION >= 40416
    if (envst->msgcall == 0 && rb_respond_to(obj, id_msgcall) == Qtrue) {
	envp->set_msgcall(envp, bdb_env_msgcall);
    }
    if (envst->thread_id == 0 && rb_respond_to(obj, id_thread_id) == Qtrue) {
	envp->set_thread_id(envp, bdb_env_thread_id);
    }
    if (envst->thread_id_string == 0 && rb_respond_to(obj, id_thread_id_string) == Qtrue) {
	envp->set_thread_id_string(envp, bdb_env_thread_id_string);
    }
    if (envst->isalive == 0 && rb_respond_to(obj, id_isalive) == Qtrue) {
	envp->set_isalive(envp, bdb_env_isalive);
    }
#endif
#if BDB_VERSION >= 30100
    if ((ret = envp->open(envp, db_home, flags, mode)) != 0)
#else
    if ((ret = envp->open(envp, db_home, db_config, flags, mode)) != 0)
#endif
    {
        envp->close(envp, 0);
	envst->envp = NULL;
        if (bdb_errcall) {
            bdb_errcall = 0;
            rb_raise(bdb_eFatal, "%s -- %s", StringValuePtr(bdb_errstr), db_strerror(ret));
        }
        else
            rb_raise(bdb_eFatal, "%s", db_strerror(ret));
    }
#endif
    envst->options &= ~BDB_ENV_NOT_OPEN;
    if (flags & DB_INIT_LOCK) {
	envst->options |= BDB_INIT_LOCK;
    }
    if (flags & DB_INIT_TXN) {
	envst->options |= BDB_AUTO_COMMIT;
    }
    envst->home = rb_tainted_str_new2(db_home);
    OBJ_FREEZE(envst->home);
#ifdef DB_INIT_REP
    if (flags & DB_INIT_REP) {
	envst->options |= BDB_REP_TRANSPORT;
    }
#endif
#if BDB_VERSION >= 30000
    if (envst->options & BDB_NEED_ENV_CURRENT) {
	rb_thread_local_aset(rb_thread_current(), bdb_id_current_env, obj);
    }
#endif
    return obj;
}

static VALUE
bdb_env_internal_close(VALUE obj)
{
    return rb_funcall2(obj, rb_intern("close"), 0, 0);
}

static VALUE
bdb_env_s_open(int argc, VALUE *argv, VALUE obj)
{
    VALUE res = rb_funcall2(obj, rb_intern("new"), argc, argv);
    if (rb_block_given_p()) {
	return rb_ensure(rb_yield, res, bdb_env_internal_close, res);
    }
    return res;
}

static VALUE
bdb_env_s_remove(int argc, VALUE *argv, VALUE obj)
{
    DB_ENV *envp;
    VALUE a, b;
    char *db_home;
    int flag = 0;

    rb_secure(2);
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	flag = NUM2INT(b);
    }
    db_home = StringValuePtr(a);
#if BDB_VERSION < 30000
    envp = ALLOCA_N(DB_ENV, 1);
    MEMZERO(envp, DB_ENV, 1);
    envp->db_errpfx = "BDB::";
    envp->db_errcall = bdb_env_errcall;
    if (lock_unlink(db_home, flag, envp) == EBUSY) {
	rb_raise(bdb_eFatal, "The shared memory region was in use");
    }
    if (log_unlink(db_home, flag, envp) == EBUSY) {
	rb_raise(bdb_eFatal, "The shared memory region was in use");
    }
    if (memp_unlink(db_home, flag, envp) == EBUSY) {
	rb_raise(bdb_eFatal, "The shared memory region was in use");
    }
    if (txn_unlink(db_home, flag, envp) == EBUSY) {
	rb_raise(bdb_eFatal, "The shared memory region was in use");
    }
#else
    bdb_test_error(db_env_create(&envp, 0));
    envp->set_errpfx(envp, "BDB::");
    envp->set_errcall(envp, bdb_env_errcall);
#if BDB_VERSION < 30100
    bdb_test_error(envp->remove(envp, db_home, NULL, flag));
#else
    bdb_test_error(envp->remove(envp, db_home, flag));
#endif
#endif
    return Qtrue;
}

static VALUE
bdb_env_set_flags(int argc, VALUE *argv, VALUE obj)
{
#if BDB_VERSION >= 30200
    bdb_ENV *envst;
    VALUE opt, flag;
    int state = 1;

    GetEnvDB(obj, envst);
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
    bdb_test_error(envst->envp->set_flags(envst->envp, NUM2INT(flag), state));
#endif
    return Qnil;
}

static VALUE
bdb_env_home(VALUE obj)
{
    bdb_ENV *envst;
    GetEnvDB(obj, envst);
    return envst->home;
}

#if BDB_VERSION >= 40000

#ifndef HAVE_RB_BLOCK_CALL

static VALUE
bdb_env_iterate(VALUE *tmp)
{
    return rb_funcall2(tmp[0], rb_intern("__bdb_thread_init__"), 
		       (int)tmp[1], (VALUE *)tmp[2]);
}

#endif

static VALUE
bdb_thread_init(int argc, VALUE *argv, VALUE obj)
{
    VALUE env;

    if ((env = rb_thread_local_aref(rb_thread_current(), bdb_id_current_env)) != Qnil) {
	rb_thread_local_aset(obj, bdb_id_current_env, env);
    }
    if (rb_block_given_p()) {
#if HAVE_RB_BLOCK_CALL
	return rb_block_call(obj, rb_intern("__bdb_thread_init__"), argc, argv,
			     rb_yield, obj);
#else
	VALUE tmp[3];
	tmp[0] = obj;
	tmp[1] = (VALUE)argc;
	tmp[2] = (VALUE)argv;
	return rb_iterate((VALUE (*)(VALUE))bdb_env_iterate, (VALUE)tmp, rb_yield, obj);
#endif
    }
    return rb_funcall2(obj, rb_intern("__bdb_thread_init__"), argc, argv);
}

#endif

#if BDB_VERSION >= 30000

static VALUE
bdb_env_feedback_set(VALUE obj, VALUE a)
{
    bdb_ENV *envst;

    GetEnvDB(obj, envst);
    if (NIL_P(a)) {
	envst->feedback = a;
    }
    else {
	if (!rb_respond_to(a, bdb_id_call)) {
	    rb_raise(bdb_eFatal, "arg must respond to #call");
	}
	envst->feedback = a;
	if (!(envst->options & BDB_NEED_ENV_CURRENT)) {
	    envst->options |= BDB_FEEDBACK;
	    rb_thread_local_aset(rb_thread_current(), bdb_id_current_env, obj);
	}
    }
    return a;
}

#endif

#if BDB_VERSION >= 40250

static VALUE
bdb_env_i_conf(VALUE obj, VALUE a)
{
    bdb_ENV *envst;
    u_int32_t bytes, gbytes, value, lk_detect;
    int i, ncache;
    char *str;
    const char *strval, **dirs;
    db_timeout_t timeout;
    long shm_key;
    time_t timeval;
    size_t size;
    VALUE res;

    GetEnvDB(obj, envst);
    str = StringValuePtr(a);
    if (strcmp(str, "cachesize") == 0) {
	bdb_test_error(envst->envp->get_cachesize(envst->envp, &gbytes, &bytes, &ncache));
	res = rb_ary_new2(3);
	rb_ary_push(res, INT2NUM(gbytes));
	rb_ary_push(res, INT2NUM(bytes));
	rb_ary_push(res, INT2NUM(ncache));
	return res;
    }
    if (strcmp(str, "data_dirs") == 0) {
	bdb_test_error(envst->envp->get_data_dirs(envst->envp, &dirs));
	res = rb_ary_new();
	if (dirs) {
	    for (i = 0; dirs[i] != NULL; i++) {
		rb_ary_push(res, rb_tainted_str_new2(dirs[i]));
	    }
	}
	return res;
    }
    if (strcmp(str, "flags") == 0) {
	bdb_test_error(envst->envp->get_flags(envst->envp, &value));
	return INT2NUM(value);
    }
    if (strcmp(str, "home") == 0) {
	bdb_test_error(envst->envp->get_home(envst->envp, &strval));
	if (strval && strlen(strval)) {
	    return rb_tainted_str_new2(strval);
	}
	return Qnil;
    }
    if (strcmp(str, "lg_bsize") == 0) {
	bdb_test_error(envst->envp->get_lg_bsize(envst->envp, &value));
	return INT2NUM(value);
    }
    if (strcmp(str, "lg_dir") == 0) {
	bdb_test_error(envst->envp->get_lg_dir(envst->envp, &strval));
	if (strval && strlen(strval)) {
	    return rb_tainted_str_new2(strval);
	}
	return Qnil;
    }
    if (strcmp(str, "lg_max") == 0) {
	bdb_test_error(envst->envp->get_lg_max(envst->envp, &value));
	return INT2NUM(value);
    }
    if (strcmp(str, "lg_regionmax") == 0) {
	bdb_test_error(envst->envp->get_lg_regionmax(envst->envp, &value));
	return INT2NUM(value);
    }
    if (strcmp(str, "lk_detect") == 0) {
	bdb_test_error(envst->envp->get_lk_detect(envst->envp, &lk_detect));
	return INT2NUM(lk_detect);
    }
    if (strcmp(str, "lk_max_lockers") == 0) {
	bdb_test_error(envst->envp->get_lk_max_lockers(envst->envp, &value));
	return INT2NUM(value);
    }
    if (strcmp(str, "lk_max_locks") == 0) {
	bdb_test_error(envst->envp->get_lk_max_locks(envst->envp, &value));
	return INT2NUM(value);
    }
    if (strcmp(str, "lk_max_objects") == 0) {
	bdb_test_error(envst->envp->get_lk_max_objects(envst->envp, &value));
	return INT2NUM(value);
    }
    if (strcmp(str, "mp_mmapsize") == 0) {
	bdb_test_error(envst->envp->get_mp_mmapsize(envst->envp, &size));
	return INT2NUM(size);
    }
    if (strcmp(str, "open_flags") == 0) {
	bdb_test_error(envst->envp->get_open_flags(envst->envp, &value));
	return INT2NUM(value);
    }
    if (strcmp(str, "rep_limit") == 0) {
#if BDB_VERSION >= 40500
	bdb_test_error(envst->envp->rep_get_limit(envst->envp, &gbytes, &bytes));
#else
	bdb_test_error(envst->envp->get_rep_limit(envst->envp, &gbytes, &bytes));
#endif
	res = rb_ary_new2(2);
	rb_ary_push(res, INT2NUM(gbytes));
	rb_ary_push(res, INT2NUM(bytes));
	return res;
    }
    if (strcmp(str, "shm_key") == 0) {
	bdb_test_error(envst->envp->get_shm_key(envst->envp, &shm_key));
	return INT2NUM(shm_key);
    }
    if (strcmp(str, "tas_spins") == 0) {
#if BDB_VERSION >= 40416
	rb_warn("Invalid option :tas_spins");
	return Qfalse;
#else
	bdb_test_error(envst->envp->get_tas_spins(envst->envp, &value));
	return INT2NUM(value);
#endif
    }
    if (strcmp(str, "txn_timeout") == 0) {
	bdb_test_error(envst->envp->get_timeout(envst->envp, &timeout, DB_SET_TXN_TIMEOUT));
	return INT2NUM(timeout);
    }
    if (strcmp(str, "lock_timeout") == 0) {
	bdb_test_error(envst->envp->get_timeout(envst->envp, &timeout, DB_SET_LOCK_TIMEOUT));
	return INT2NUM(timeout);
    }
    if (strcmp(str, "tmp_dir") == 0) {
	bdb_test_error(envst->envp->get_tmp_dir(envst->envp, &strval));
	if (strval && strlen(strval)) {
	    return rb_tainted_str_new2(strval);
	}
	return Qnil;
    }
    if (strcmp(str, "tx_max") == 0) {
	bdb_test_error(envst->envp->get_tx_max(envst->envp, &value));
	return INT2NUM(value);
    }
    if (strcmp(str, "tx_timestamp") == 0) {
	bdb_test_error(envst->envp->get_tx_timestamp(envst->envp, &timeval));
	return INT2NUM(timeval);
    }
#if BDB_VERSION >= 40500
    if (strcmp(str, "rep_priority") == 0) {
	bdb_test_error(envst->envp->rep_get_priority(envst->envp, &size));
	return INT2NUM(size);
    }
    if (strcmp(str, "rep_nsites") == 0) {
	bdb_test_error(envst->envp->rep_get_nsites(envst->envp, &size));
	return INT2NUM(size);
    }
#endif
    rb_raise(rb_eArgError, "Unknown option %s", str);
    return obj;
}

static char *
options[] = {
    "cachesize", "data_dirs", "flags", "home", "lg_bsize", "lg_dir",
    "lg_max", "lg_regionmax", "lk_detect", "lk_max_lockers", "lk_max_locks",
    "lk_max_objects", "mp_mmapsize", "open_flags", "rep_limit", "shm_key",
    "tas_spins", "txn_timeout", "lock_timeout", "tmp_dir", "tx_max",
#if BDB_VERSION >= 40500
    "rep_priority", "rep_nsites",
#endif
    "tx_timestamp", 0
};

struct optst {
    VALUE obj, str;
};

static VALUE
bdb_env_intern_conf(struct optst *optp)
{
    return bdb_env_i_conf(optp->obj, optp->str);
}

static VALUE
bdb_env_conf(int argc, VALUE *argv, VALUE obj)
{
    int i, state;
    VALUE res, val;
    struct optst opt;

    if (argc > 1) {
	rb_raise(rb_eArgError, "invalid number of arguments (%d for 1)", argc);
    }
    if (argc == 1) {
	return bdb_env_i_conf(obj, argv[0]);
    }
    res = rb_hash_new();
    opt.obj = obj;
    for (i = 0; options[i] != NULL; i++) {
	opt.str = rb_str_new2(options[i]);
	val = rb_protect((VALUE (*)(ANYARGS))bdb_env_intern_conf, (VALUE)&opt, &state);
	if (state == 0) {
	    rb_hash_aset(res, opt.str, val);
	}
    }
    return res;
}

#endif 

#if BDB_VERSION >= 40416

static VALUE
bdb_env_lsn_reset(int argc, VALUE *argv, VALUE obj)
{  
    char *file;
    int flags;
    VALUE a, b;
    bdb_ENV *envst;

    GetEnvDB(obj, envst);
    flags = 0;
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	flags = NUM2INT(b);
    }
    file = StringValuePtr(a);
    bdb_test_error(envst->envp->lsn_reset(envst->envp, file, flags));
    return obj;
}

static VALUE
bdb_env_fileid_reset(int argc, VALUE *argv, VALUE obj)
{  
    char *file;
    int flags;
    VALUE a, b;
    bdb_ENV *envst;

    GetEnvDB(obj, envst);
    flags = 0;
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	flags = NUM2INT(b);
    }
    file = StringValuePtr(a);
    bdb_test_error(envst->envp->fileid_reset(envst->envp, file, flags));
    return obj;
}

static VALUE
bdb_env_set_msgcall(VALUE obj, VALUE a)
{
    bdb_ENV *envst;

    GetEnvDB(obj, envst);
    if (NIL_P(a)) {
	envst->msgcall = Qnil;
	envst->envp->set_msgcall(envst->envp, NULL);
	return obj;
    }
    if (!rb_respond_to(a, bdb_id_call)) {
	rb_raise(rb_eArgError, "object must respond to #call");
    }
    if (!RTEST(envst->msgcall)) {
	envst->envp->set_msgcall(envst->envp, bdb_env_msgcall);
    }
    envst->msgcall = a;
    return obj;
}

static VALUE
bdb_env_set_thread_id(VALUE obj, VALUE a)
{
    bdb_ENV *envst;

    GetEnvDB(obj, envst);
    if (!rb_respond_to(a, bdb_id_call)) {
	rb_raise(rb_eArgError, "object must respond to #call");
    }
    if (!RTEST(envst->thread_id)) {
	envst->envp->set_thread_id(envst->envp, bdb_env_thread_id);
    }
    envst->thread_id = a;
    return obj;
}

static VALUE
bdb_env_set_thread_id_string(VALUE obj, VALUE a)
{
    bdb_ENV *envst;

    GetEnvDB(obj, envst);
    if (!rb_respond_to(a, bdb_id_call)) {
	rb_raise(rb_eArgError, "object must respond to #call");
    }
    if (!RTEST(envst->thread_id_string)) {
	envst->envp->set_thread_id_string(envst->envp, bdb_env_thread_id_string);
    }
    envst->thread_id_string = a;
    return obj;
}

static VALUE
bdb_env_set_isalive(VALUE obj, VALUE a)
{
    bdb_ENV *envst;

    GetEnvDB(obj, envst);
    if (!rb_respond_to(a, bdb_id_call)) {
	rb_raise(rb_eArgError, "object must respond to #call");
    }
    if (!RTEST(envst->isalive)) {
	envst->envp->set_isalive(envst->envp, bdb_env_isalive);
    }
    envst->isalive = a;
    return obj;
}

static VALUE
bdb_env_failcheck(int argc, VALUE *argv, VALUE obj)
{
    bdb_ENV *envst;
    int flags = 0;
    VALUE a;

    GetEnvDB(obj, envst);
    if (rb_scan_args(argc, argv, "01", &a)) {
	flags = NUM2INT(a);
    }
    bdb_test_error(flags = envst->envp->failchk(envst->envp, flags));
    return INT2NUM(flags);
}


#endif

#if BDB_VERSION >= 40500

static VALUE
bdb_env_repmgr_add_remote(int argc, VALUE *argv, VALUE obj)
{
    bdb_ENV *envst;
    VALUE a, b, c;
    int eid, flags;

    flags = 0;
    if (rb_scan_args(argc, argv, "21", &a, &b, &c) == 3) {
	flags = NUM2INT(c);
    }
    GetEnvDB(obj, envst);
    bdb_test_error(envst->envp->repmgr_add_remote_site(envst->envp, 
						       StringValuePtr(a),
						       NUM2UINT(b),
						       &eid, flags));
    return INT2NUM(eid);
}

static VALUE
bdb_env_repmgr_set_ack_policy(VALUE obj, VALUE a)
{
    bdb_ENV *envst;

    GetEnvDB(obj, envst);
    bdb_test_error(envst->envp->repmgr_set_ack_policy(envst->envp, 
						      NUM2UINT(a)));
    return a;
}

static VALUE
bdb_env_repmgr_get_ack_policy(VALUE obj)
{
    bdb_ENV *envst;
    int policy;

    GetEnvDB(obj, envst);
    bdb_test_error(envst->envp->repmgr_get_ack_policy(envst->envp, &policy));
    return INT2NUM(policy);
}

static VALUE
bdb_env_repmgr_set_local_site(int argc, VALUE *argv, VALUE obj)
{
    bdb_ENV *envst;
    VALUE a, b, c;
    int flags;

    flags = 0;
    if (rb_scan_args(argc, argv, "21", &a, &b, &c) == 3) {
	flags = NUM2INT(c);
    }
    GetEnvDB(obj, envst);
    bdb_test_error(envst->envp->repmgr_set_local_site(envst->envp, 
						      StringValuePtr(a),
						      NUM2UINT(b),
						      flags));
    return obj;
}

static VALUE
bdb_env_repmgr_site_list(VALUE obj)
{
    bdb_ENV *envst;
    VALUE res, tmp;
    u_int count, i;
    DB_REPMGR_SITE *list;

    GetEnvDB(obj, envst);
    bdb_test_error(envst->envp->repmgr_site_list(envst->envp, 
						 &count, &list));
    res = rb_ary_new();
    for (i = 0; i < count; i++) {
	tmp = rb_hash_new();
	rb_hash_aset(tmp, rb_tainted_str_new2("eid"), INT2NUM(list[i].eid));
	rb_hash_aset(tmp, rb_tainted_str_new2("host"), rb_tainted_str_new2(list[i].host));
	rb_hash_aset(tmp, rb_tainted_str_new2("port"), INT2NUM(list[i].port));
	rb_hash_aset(tmp, rb_tainted_str_new2("status"), INT2NUM(list[i].status));
	rb_ary_push(res, tmp);
    }
    free(list);
    return res;
}

static VALUE
bdb_env_repmgr_start(VALUE obj, VALUE a, VALUE b)
{
    bdb_ENV *envst;

    GetEnvDB(obj, envst);
    bdb_test_error(envst->envp->repmgr_start(envst->envp, NUM2INT(a), NUM2INT(b)));
    return obj;
}

static VALUE bdb_env_rep_set_transport(VALUE obj, VALUE a, VALUE b)
{
    bdb_ENV *envst;

    GetEnvDB(obj, envst);
    if (!FIXNUM_P(a)) {
	rb_raise(bdb_eFatal, "expected a Fixnum for the 1st arg of set_rep_transport");
    }
    if (!rb_respond_to(b, bdb_id_call)) {
	rb_raise(bdb_eFatal, "2nd arg must respond to #call");
    }
    envst->rep_transport = b;
    bdb_test_error(envst->envp->rep_set_transport(envst->envp, NUM2INT(a), 
						  bdb_env_rep_transport));
    return obj;
}

static VALUE
bdb_env_rep_set_config(VALUE obj, VALUE a, VALUE b)
{
    bdb_ENV *envst;
    int onoff = 0;

    if (b == Qtrue) {
	onoff = 1;
    }
    else if (b == Qfalse || b == Qnil) {
	onoff = 0;
    }
    else {
	onoff = NUM2INT(b);
    }
    GetEnvDB(obj, envst);
    bdb_test_error(envst->envp->rep_set_config(envst->envp, NUM2UINT(a), onoff));
    return obj;
}

static VALUE
bdb_env_rep_get_config(VALUE obj, VALUE a)
{
    bdb_ENV *envst;
    int offon;

    GetEnvDB(obj, envst);
    bdb_test_error(envst->envp->rep_get_config(envst->envp, NUM2UINT(a), &offon));
    if (offon) {
	return Qtrue;
    }
    return Qfalse;
}

static VALUE
bdb_env_rep_set_nsites(VALUE obj, VALUE a)
{
    bdb_ENV *envst;

    GetEnvDB(obj, envst);
    bdb_test_error(envst->envp->rep_set_nsites(envst->envp, NUM2UINT(a)));
    return a;
}

static VALUE
bdb_env_rep_get_nsites(VALUE obj, VALUE a)
{
    bdb_ENV *envst;
    int offon;

    GetEnvDB(obj, envst);
    bdb_test_error(envst->envp->rep_get_nsites(envst->envp, &offon));
    return INT2NUM(offon);
}

static VALUE
bdb_env_rep_set_priority(VALUE obj, VALUE a)
{
    bdb_ENV *envst;

    GetEnvDB(obj, envst);
    bdb_test_error(envst->envp->rep_set_priority(envst->envp, NUM2UINT(a)));
    return a;
}

static VALUE
bdb_env_rep_get_priority(VALUE obj, VALUE a)
{
    bdb_ENV *envst;
    int offon;

    GetEnvDB(obj, envst);
    bdb_test_error(envst->envp->rep_get_priority(envst->envp, &offon));
    return INT2NUM(offon);
}

static VALUE
bdb_env_rep_set_timeout(VALUE obj, VALUE a, VALUE b)
{
    bdb_ENV *envst;

    GetEnvDB(obj, envst);
    bdb_test_error(envst->envp->rep_set_timeout(envst->envp,
					       NUM2UINT(a),
					       NUM2INT(b)));
    return obj;
}

static VALUE
bdb_env_rep_get_timeout(VALUE obj, VALUE a)
{
    bdb_ENV *envst;
    int offon;

    GetEnvDB(obj, envst);
    bdb_test_error(envst->envp->rep_get_timeout(envst->envp, NUM2UINT(a), &offon));
    return INT2NUM(offon);
}

static VALUE
bdb_env_rep_get_limit(VALUE obj)
{
    bdb_ENV *envst;
    u_int32_t gbytes, bytes;
    VALUE res;

    GetEnvDB(obj, envst);
    bdb_test_error(envst->envp->rep_get_limit(envst->envp, &gbytes, &bytes));
    res = rb_ary_new2(2);
    rb_ary_push(res, INT2NUM(gbytes));
    rb_ary_push(res, INT2NUM(bytes));
    return res;
}

static VALUE
bdb_env_rep_sync(int argc, VALUE *argv, VALUE obj)
{
    bdb_ENV *envst;
    VALUE a;
    int flags;

    flags = 0;
    if (rb_scan_args(argc, argv, "01", &a) == 1) {
	flags = NUM2INT(a);
    }
    GetEnvDB(obj, envst);
    bdb_test_error(envst->envp->rep_sync(envst->envp, flags));
    return obj;
}

static VALUE
bdb_env_rep_stat(int argc, VALUE *argv, VALUE obj)
{
    bdb_ENV *envst;
    VALUE a, lsn;
    int flags;
    struct dblsnst *lsnst;
    DB_REP_STAT *bs;

    flags = 0;
    if (rb_scan_args(argc, argv, "01", &a) == 1) {
	flags = NUM2INT(a);
    }
    GetEnvDB(obj, envst);
    bdb_test_error(envst->envp->rep_stat(envst->envp, &bs, flags));
    a = rb_hash_new();
    rb_hash_aset(a, rb_tainted_str_new2("st_bulk_fills"), INT2NUM(bs->st_bulk_fills));
    rb_hash_aset(a, rb_tainted_str_new2("st_bulk_overflows"), INT2NUM(bs->st_bulk_overflows));
    rb_hash_aset(a, rb_tainted_str_new2("st_bulk_records"), INT2NUM(bs->st_bulk_records));
    rb_hash_aset(a, rb_tainted_str_new2("st_bulk_transfers"), INT2NUM(bs->st_bulk_transfers));
    rb_hash_aset(a, rb_tainted_str_new2("st_client_rerequests"), INT2NUM(bs->st_client_rerequests));
    rb_hash_aset(a, rb_tainted_str_new2("st_client_svc_miss"), INT2NUM(bs->st_client_svc_miss));
    rb_hash_aset(a, rb_tainted_str_new2("st_client_svc_req"), INT2NUM(bs->st_client_svc_req));
    rb_hash_aset(a, rb_tainted_str_new2("st_dupmasters"), INT2NUM(bs->st_dupmasters));
    rb_hash_aset(a, rb_tainted_str_new2("st_egen"), INT2NUM(bs->st_egen));
    rb_hash_aset(a, rb_tainted_str_new2("st_election_cur_winner"), INT2NUM(bs->st_election_cur_winner));
    rb_hash_aset(a, rb_tainted_str_new2("st_election_gen"), INT2NUM(bs->st_election_gen));

    lsn = bdb_makelsn(obj);
    Data_Get_Struct(lsn, struct dblsnst, lsnst);
    MEMCPY(lsnst->lsn, &bs->st_election_lsn, DB_LSN, 1);
    rb_hash_aset(a, rb_tainted_str_new2("st_election_lsn"), lsn);

    rb_hash_aset(a, rb_tainted_str_new2("st_election_nsites"), INT2NUM(bs->st_election_nsites));
    rb_hash_aset(a, rb_tainted_str_new2("st_election_nvotes"), INT2NUM(bs->st_election_nvotes));
    rb_hash_aset(a, rb_tainted_str_new2("st_election_priority"), INT2NUM(bs->st_election_priority));
    rb_hash_aset(a, rb_tainted_str_new2("st_election_sec"), INT2NUM(bs->st_election_sec));
    rb_hash_aset(a, rb_tainted_str_new2("st_election_status"), INT2NUM(bs->st_election_status));
    rb_hash_aset(a, rb_tainted_str_new2("st_election_tiebreaker"), INT2NUM(bs->st_election_tiebreaker));
    rb_hash_aset(a, rb_tainted_str_new2("st_election_usec"), INT2NUM(bs->st_election_usec));
    rb_hash_aset(a, rb_tainted_str_new2("st_election_votes"), INT2NUM(bs->st_election_votes));
    rb_hash_aset(a, rb_tainted_str_new2("st_elections"), INT2NUM(bs->st_elections));
    rb_hash_aset(a, rb_tainted_str_new2("st_elections_won"), INT2NUM(bs->st_elections_won));
    rb_hash_aset(a, rb_tainted_str_new2("st_env_id"), INT2NUM(bs->st_env_id));
    rb_hash_aset(a, rb_tainted_str_new2("st_env_priority"), INT2NUM(bs->st_env_priority));
    rb_hash_aset(a, rb_tainted_str_new2("st_gen"), INT2NUM(bs->st_gen));
    rb_hash_aset(a, rb_tainted_str_new2("st_log_duplicated"), INT2NUM(bs->st_log_duplicated));
    rb_hash_aset(a, rb_tainted_str_new2("st_log_queued"), INT2NUM(bs->st_log_queued));
    rb_hash_aset(a, rb_tainted_str_new2("st_log_queued_max"), INT2NUM(bs->st_log_queued_max));
    rb_hash_aset(a, rb_tainted_str_new2("st_log_queued_total"), INT2NUM(bs->st_log_queued_total));
    rb_hash_aset(a, rb_tainted_str_new2("st_log_records"), INT2NUM(bs->st_log_records));
    rb_hash_aset(a, rb_tainted_str_new2("st_log_requested"), INT2NUM(bs->st_log_requested));
    rb_hash_aset(a, rb_tainted_str_new2("st_master"), INT2NUM(bs->st_master));
    rb_hash_aset(a, rb_tainted_str_new2("st_master_changes"), INT2NUM(bs->st_master_changes));
    rb_hash_aset(a, rb_tainted_str_new2("st_msgs_badgen"), INT2NUM(bs->st_msgs_badgen));
    rb_hash_aset(a, rb_tainted_str_new2("st_msgs_processed"), INT2NUM(bs->st_msgs_processed));
    rb_hash_aset(a, rb_tainted_str_new2("st_msgs_recover"), INT2NUM(bs->st_msgs_recover));
    rb_hash_aset(a, rb_tainted_str_new2("st_msgs_send_failures"), INT2NUM(bs->st_msgs_send_failures));
    rb_hash_aset(a, rb_tainted_str_new2("st_msgs_sent"), INT2NUM(bs->st_msgs_sent));
    rb_hash_aset(a, rb_tainted_str_new2("st_newsites"), INT2NUM(bs->st_newsites));

    lsn = bdb_makelsn(obj);
    Data_Get_Struct(lsn, struct dblsnst, lsnst);
    MEMCPY(lsnst->lsn, &bs->st_next_lsn, DB_LSN, 1);
    rb_hash_aset(a, rb_tainted_str_new2("st_next_lsn"), lsn);
    rb_hash_aset(a, rb_tainted_str_new2("st_next_pg"), INT2NUM(bs->st_next_pg));
    rb_hash_aset(a, rb_tainted_str_new2("st_nsites"), INT2NUM(bs->st_nsites));
    rb_hash_aset(a, rb_tainted_str_new2("st_nthrottles"), INT2NUM(bs->st_nthrottles));
    rb_hash_aset(a, rb_tainted_str_new2("st_outdated"), INT2NUM(bs->st_outdated));
    rb_hash_aset(a, rb_tainted_str_new2("st_pg_duplicated"), INT2NUM(bs->st_pg_duplicated));
    rb_hash_aset(a, rb_tainted_str_new2("st_pg_records"), INT2NUM(bs->st_pg_records));
    rb_hash_aset(a, rb_tainted_str_new2("st_pg_requested"), INT2NUM(bs->st_pg_requested));
    rb_hash_aset(a, rb_tainted_str_new2("st_startup_complete"), INT2NUM(bs->st_startup_complete));
    rb_hash_aset(a, rb_tainted_str_new2("st_status"), INT2NUM(bs->st_status));
    rb_hash_aset(a, rb_tainted_str_new2("st_txns_applied"), INT2NUM(bs->st_txns_applied));
    lsn = bdb_makelsn(obj);
    Data_Get_Struct(lsn, struct dblsnst, lsnst);
    MEMCPY(lsnst->lsn, &bs->st_waiting_lsn, DB_LSN, 1);
    rb_hash_aset(a, rb_tainted_str_new2("st_waiting_lsn"), lsn);
    rb_hash_aset(a, rb_tainted_str_new2("st_waiting_pg"), INT2NUM(bs->st_waiting_pg));
    free(bs);
    return a;
}

static VALUE bdb_cInt;

struct bdb_intern {
    VALUE obj;
    int sstype;
};

#define BDB_INTERN_CONFIG 1
#define BDB_INTERN_TIMEOUT 2

#define REP_INTERN(str_, sstype_)				\
static VALUE							\
bdb_env_rep_intern_##str_(VALUE obj)				\
{								\
    struct bdb_intern *st_intern;				\
    VALUE res;							\
								\
    res = Data_Make_Struct(bdb_cInt, struct bdb_intern, 0, free, st_intern);	\
    st_intern->obj = obj;					\
    st_intern->sstype = sstype_;				\
    return res;							\
}

REP_INTERN(config, BDB_INTERN_CONFIG)
REP_INTERN(timeout, BDB_INTERN_TIMEOUT)

static VALUE
bdb_intern_set(VALUE obj, VALUE a, VALUE b)
{
    struct bdb_intern *st_intern;

    Data_Get_Struct(obj, struct bdb_intern, st_intern);
    if (st_intern->sstype == BDB_INTERN_TIMEOUT) {
	return bdb_env_rep_set_timeout(st_intern->obj, a, b);
    }
    if (st_intern->sstype == BDB_INTERN_CONFIG) {
	return bdb_env_rep_set_config(st_intern->obj, a, b);
    }
    rb_raise(rb_eArgError, "Invalid argument for Intern__#[]=");
    return Qnil;
}

static VALUE
bdb_intern_get(VALUE obj, VALUE a)
{
    struct bdb_intern *st_intern;

    Data_Get_Struct(obj, struct bdb_intern, st_intern);
    if (st_intern->sstype == BDB_INTERN_TIMEOUT) {
	return bdb_env_rep_get_timeout(st_intern->obj, a);
    }
    if (st_intern->sstype == BDB_INTERN_CONFIG) {
	return bdb_env_rep_get_config(st_intern->obj, a);
    }
    rb_raise(rb_eArgError, "Invalid argument for Intern__#[]");
    return Qnil;
}

#endif

void bdb_init_env()
{
    bdb_id_call = rb_intern("call");
#if BDB_VERSION >= 30000
    id_feedback = rb_intern("bdb_feedback");
#endif
    bdb_id_current_env = rb_intern("bdb_current_env");
#if BDB_VERSION >= 40100
    id_app_dispatch = rb_intern("bdb_app_dispatch");
#if BDB_VERSION >= 40416
    id_msgcall = rb_intern("bdb_msgcall");
    id_thread_id = rb_intern("bdb_thread_id");
    id_thread_id_string = rb_intern("bdb_thread_id_string");
    id_isalive = rb_intern("bdb_isalive");
#endif
#if BDB_VERSION >= 40500
    id_event_notify = rb_intern("bdb_event_notify");
#endif
#endif
    bdb_cEnv = rb_define_class_under(bdb_mDb, "Env", rb_cObject);
    rb_define_private_method(bdb_cEnv, "initialize", bdb_env_init, -1);
#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
    rb_define_alloc_func(bdb_cEnv, bdb_env_s_alloc);
#else
    rb_define_singleton_method(bdb_cEnv, "allocate", bdb_env_s_alloc, 0);
#endif
    rb_define_singleton_method(bdb_cEnv, "new", bdb_env_s_new, -1);
    rb_define_singleton_method(bdb_cEnv, "create", bdb_env_s_new, -1);
    rb_define_singleton_method(bdb_cEnv, "open", bdb_env_s_open, -1);
    rb_define_singleton_method(bdb_cEnv, "remove", bdb_env_s_remove, -1);
    rb_define_singleton_method(bdb_cEnv, "unlink", bdb_env_s_remove, -1);
    rb_define_method(bdb_cEnv, "open_db", bdb_env_open_db, -1);
    rb_define_method(bdb_cEnv, "close", bdb_env_close, 0);
    rb_define_method(bdb_cEnv, "set_flags", bdb_env_set_flags, -1);
    rb_define_method(bdb_cEnv, "home", bdb_env_home, 0);
#if BDB_VERSION >= 40000
    rb_define_method(bdb_cEnv, "rep_elect", bdb_env_rep_elect, -1);
    rb_define_method(bdb_cEnv, "elect", bdb_env_rep_elect, -1);
    rb_define_method(bdb_cEnv, "rep_process_message", bdb_env_rep_process_message, 3);
    rb_define_method(bdb_cEnv, "process_message", bdb_env_rep_process_message, 3);
    rb_define_method(bdb_cEnv, "rep_start", bdb_env_rep_start, 2);
    if (!rb_method_boundp(rb_cThread, rb_intern("__bdb_thread_init__"), 1)) {
	rb_alias(rb_cThread, rb_intern("__bdb_thread_init__"), rb_intern("initialize"));
	rb_define_method(rb_cThread, "initialize", bdb_thread_init, -1);
    }
#if BDB_VERSION >= 40100
    rb_define_method(bdb_cEnv, "rep_limit=", bdb_env_rep_limit, -1);
#endif
#endif
#if BDB_VERSION >= 30000
    rb_define_method(bdb_cEnv, "feedback=", bdb_env_feedback_set, 1);
#endif
#if BDB_VERSION >= 40250
    rb_define_method(bdb_cEnv, "configuration", bdb_env_conf, -1);
    rb_define_method(bdb_cEnv, "conf", bdb_env_conf, -1);
#endif
#if BDB_VERSION >= 40416
    rb_define_method(bdb_cEnv, "lsn_reset", bdb_env_lsn_reset, -1);
    rb_define_method(bdb_cEnv, "fileid_reset", bdb_env_fileid_reset, -1);
    rb_define_method(bdb_cEnv, "msgcall=", bdb_env_set_msgcall, 1);
    rb_define_method(bdb_cEnv, "thread_id=", bdb_env_set_thread_id, 1);
    rb_define_method(bdb_cEnv, "thread_id_string=", bdb_env_set_thread_id_string, 1);
    rb_define_method(bdb_cEnv, "isalive=", bdb_env_set_isalive, 1);
    rb_define_method(bdb_cEnv, "failcheck", bdb_env_failcheck, -1);
#endif
#if BDB_VERSION >= 40500
    rb_define_method(bdb_cEnv, "event_notify=", bdb_env_set_notify, 1);
    bdb_cInt = rb_define_class_under(bdb_mDb, "Intern__", rb_cObject);
#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
    rb_undef_alloc_func(bdb_cInt);
#else
    rb_undef_method(CLASS_OF(bdb_cInt), "allocate");
#endif
    rb_undef_method(CLASS_OF(bdb_cInt), "new");
    rb_define_method(bdb_cInt, "[]", bdb_intern_get, 1);
    rb_define_method(bdb_cInt, "[]=", bdb_intern_set, 2);

    rb_define_method(bdb_cEnv, "repmgr_add_remote_site", 
		     bdb_env_repmgr_add_remote, -1);
    rb_define_method(bdb_cEnv, "repmgr_set_ack_policy",
		     bdb_env_repmgr_set_ack_policy, 1);
    rb_define_method(bdb_cEnv, "repmgr_ack_policy=",
		     bdb_env_repmgr_set_ack_policy, 1);
    rb_define_method(bdb_cEnv, "repmgr_get_ack_policy",
		     bdb_env_repmgr_get_ack_policy, 0);
    rb_define_method(bdb_cEnv, "repmgr_ack_policy",
		     bdb_env_repmgr_get_ack_policy, 0);
    rb_define_method(bdb_cEnv, "repmgr_set_local_site",
		     bdb_env_repmgr_set_local_site, -1);
    rb_define_method(bdb_cEnv, "repmgr_site_list", bdb_env_repmgr_site_list, 0);
    rb_define_method(bdb_cEnv, "repmgr_start", bdb_env_repmgr_start, 2);


    rb_define_method(bdb_cEnv, "rep_set_config", bdb_env_rep_set_config, 2);
    rb_define_method(bdb_cEnv, "rep_get_config", bdb_env_rep_get_config, 1);
    rb_define_method(bdb_cEnv, "rep_config", bdb_env_rep_intern_config, 0);
    rb_define_method(bdb_cEnv, "rep_config?", bdb_env_rep_intern_config, 0);
    rb_define_method(bdb_cEnv, "rep_set_nsites", bdb_env_rep_set_nsites, 1);
    rb_define_method(bdb_cEnv, "rep_nsites=", bdb_env_rep_set_nsites, 1);
    rb_define_method(bdb_cEnv, "rep_get_nsites", bdb_env_rep_get_nsites, 0);
    rb_define_method(bdb_cEnv, "rep_nsites", bdb_env_rep_get_nsites, 0);
    rb_define_method(bdb_cEnv, "rep_set_priority", bdb_env_rep_set_priority, 1);
    rb_define_method(bdb_cEnv, "rep_priority=", bdb_env_rep_set_priority, 1);
    rb_define_method(bdb_cEnv, "rep_get_priority", bdb_env_rep_get_priority, 0);
    rb_define_method(bdb_cEnv, "rep_priority", bdb_env_rep_get_priority, 0);
    rb_define_method(bdb_cEnv, "rep_get_limit", bdb_env_rep_get_limit, 0);
    rb_define_method(bdb_cEnv, "rep_limit", bdb_env_rep_get_limit, 0);
    rb_define_method(bdb_cEnv, "rep_set_timeout", bdb_env_rep_set_timeout, 2);
    rb_define_method(bdb_cEnv, "rep_get_timeout", bdb_env_rep_get_timeout, 1);
    rb_define_method(bdb_cEnv, "rep_timeout", bdb_env_rep_intern_timeout, 1);
    rb_define_method(bdb_cEnv, "rep_stat", bdb_env_rep_stat, 0);
    rb_define_method(bdb_cEnv, "rep_sync", bdb_env_rep_sync, -1);
    rb_define_method(bdb_cEnv, "rep_set_transport", bdb_env_rep_set_transport, 2);
    
#endif
}

