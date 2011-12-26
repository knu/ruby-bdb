#include "bdb.h"

static void
mark_lsn(struct dblsnst *lsnst)
{
    rb_gc_mark(lsnst->env);
}

static void
free_lsn(struct dblsnst *lsnst)
{
    if (BDB_VALID(lsnst->env, T_DATA)) {
	bdb_clean_env(lsnst->env, lsnst->self);
    }
#if HAVE_TYPE_DB_LOGC
    if (lsnst->cursor && BDB_VALID(lsnst->env, T_DATA)) {
	bdb_ENV *envst;

	Data_Get_Struct(lsnst->env, bdb_ENV, envst);
	if (envst->envp) {
	    lsnst->cursor->close(lsnst->cursor, 0);
	}
	lsnst->cursor = 0;
    }
#endif
    if (lsnst->lsn) free(lsnst->lsn);
    free(lsnst);
}

VALUE
bdb_makelsn(VALUE env)
{
    bdb_ENV *envst;
    struct dblsnst *lsnst;
    VALUE res;

    GetEnvDB(env, envst);
    res = Data_Make_Struct(bdb_cLsn, struct dblsnst, mark_lsn, free_lsn, lsnst);
    lsnst->env = env;
    lsnst->lsn = ALLOC(DB_LSN);
    lsnst->self = res;
    return res;
}

static VALUE
bdb_s_log_put_internal(VALUE obj, VALUE a, int flag)
{
    bdb_ENV *envst;
    VALUE ret;
    DBT data;
    struct dblsnst *lsnst;

    GetEnvDB(obj, envst);
    if (TYPE(a) != T_STRING) a = rb_str_to_str(a);
    ret = bdb_makelsn(obj);
    Data_Get_Struct(ret, struct dblsnst, lsnst);
    data.data = StringValuePtr(a);
    data.size = RSTRING_LEN(a);
#if HAVE_ST_DB_ENV_LG_INFO
    if (!envst->envp->lg_info) {
	rb_raise(bdb_eFatal, "log region not open");
    }
    bdb_test_error(log_put(envst->envp->lg_info, lsnst->lsn, &data, flag));
#elif HAVE_ST_DB_ENV_LOG_PUT
    bdb_test_error(envst->envp->log_put(envst->envp, lsnst->lsn, &data, flag));
#else
    bdb_test_error(log_put(envst->envp, lsnst->lsn, &data, flag));
#endif
    return ret;
}

static VALUE
bdb_s_log_put(int argc, VALUE *argv, VALUE obj)
{
    VALUE a, b;
    int flag = 0;
    
    if (argc == 0 || argc > 2) {
	rb_raise(bdb_eFatal, "Invalid number of arguments");
    }
#if HAVE_CONST_DB_CHECKPOINT
    flag = DB_CHECKPOINT;
#endif
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	flag = NUM2INT(b);
    }
    return bdb_s_log_put_internal(obj, a, flag);
}

static VALUE
bdb_s_log_checkpoint(VALUE obj, VALUE a)
{
#if HAVE_CONST_DB_CHECKPOINT
    return bdb_s_log_put_internal(obj, a, DB_CHECKPOINT);
#else
    rb_warning("BDB::CHECKPOINT is obsolete");
    return bdb_s_log_put_internal(obj, a, 0);
#endif
}

static VALUE
bdb_s_log_flush(int argc, VALUE *argv, VALUE obj)
{
    bdb_ENV *envst;

    if (argc == 0) {
	GetEnvDB(obj, envst);
#if HAVE_ST_DB_ENV_LG_INFO
	if (!envst->envp->lg_info) {
	    rb_raise(bdb_eFatal, "log region not open");
	}
	bdb_test_error(log_flush(envst->envp->lg_info, NULL));
#elif HAVE_ST_DB_ENV_LOG_FLUSH
	bdb_test_error(envst->envp->log_flush(envst->envp, NULL));
#else
	bdb_test_error(log_flush(envst->envp, NULL));
#endif
	return obj;
    }
    else if (argc == 1) {
	return bdb_s_log_put_internal(obj, argv[0], DB_FLUSH);
    }
    else {
	rb_raise(bdb_eFatal, "Invalid number of arguments");
    }
}

static VALUE
bdb_s_log_curlsn(VALUE obj, VALUE a)
{
#if HAVE_CONST_DB_CURLSN
    return bdb_s_log_put_internal(obj, a, DB_CURLSN);
#else
    rb_warning("BDB::CURLSN is obsolete");
    return bdb_s_log_put_internal(obj, a, 0);
#endif
}
  

static VALUE
bdb_env_log_stat(int argc, VALUE *argv, VALUE obj)
{
    DB_LOG_STAT *bdb_stat;
    bdb_ENV *envst;
    VALUE res, b;
    int flags;

    GetEnvDB(obj, envst);
#if HAVE_ST_DB_ENV_LG_INFO
    if (!envst->envp->lg_info) {
	rb_raise(bdb_eFatal, "log region not open");
    }
    if (argc != 0) {
	rb_raise(rb_eArgError, "invalid number of arguments (%d for 0)", argc);
    }
    bdb_test_error(log_stat(envst->envp->lg_info, &bdb_stat, 0));
#elif HAVE_ST_DB_ENV_LOG_STAT
    flags = 0;
    if (rb_scan_args(argc, argv, "01", &b) == 1) {
	flags = NUM2INT(b);
    }
    bdb_test_error(envst->envp->log_stat(envst->envp, &bdb_stat, flags));
#else
    if (argc != 0) {
	rb_raise(rb_eArgError, "invalid number of arguments (%d for 0)", argc);
    }
#if HAVE_DB_LOG_STAT_3
    bdb_test_error(log_stat(envst->envp, &bdb_stat, 0));
#else
    bdb_test_error(log_stat(envst->envp, &bdb_stat));
#endif
#endif
    res = rb_hash_new();
    rb_hash_aset(res, rb_tainted_str_new2("st_magic"), INT2NUM(bdb_stat->st_magic));
    rb_hash_aset(res, rb_tainted_str_new2("st_version"), INT2NUM(bdb_stat->st_version));
    rb_hash_aset(res, rb_tainted_str_new2("st_regsize"), INT2NUM(bdb_stat->st_regsize));
    rb_hash_aset(res, rb_tainted_str_new2("st_mode"), INT2NUM(bdb_stat->st_mode));
#if HAVE_ST_DB_LOG_STAT_ST_REFCNT
    rb_hash_aset(res, rb_tainted_str_new2("st_refcnt"), INT2NUM(bdb_stat->st_refcnt));
#endif
#if HAVE_ST_DB_LOG_STAT_ST_LG_BSIZE
    rb_hash_aset(res, rb_tainted_str_new2("st_lg_bsize"), INT2NUM(bdb_stat->st_lg_bsize));
#endif
#if HAVE_ST_DB_LOG_STAT_ST_LG_SIZE
    rb_hash_aset(res, rb_tainted_str_new2("st_lg_size"), INT2NUM(bdb_stat->st_lg_size));
    rb_hash_aset(res, rb_tainted_str_new2("st_lg_max"), INT2NUM(bdb_stat->st_lg_size));
#else
    rb_hash_aset(res, rb_tainted_str_new2("st_lg_max"), INT2NUM(bdb_stat->st_lg_max));
#endif
    rb_hash_aset(res, rb_tainted_str_new2("st_w_mbytes"), INT2NUM(bdb_stat->st_w_mbytes));
    rb_hash_aset(res, rb_tainted_str_new2("st_w_bytes"), INT2NUM(bdb_stat->st_w_bytes));
    rb_hash_aset(res, rb_tainted_str_new2("st_wc_mbytes"), INT2NUM(bdb_stat->st_wc_mbytes));
    rb_hash_aset(res, rb_tainted_str_new2("st_wc_bytes"), INT2NUM(bdb_stat->st_wc_bytes));
    rb_hash_aset(res, rb_tainted_str_new2("st_wcount"), INT2NUM(bdb_stat->st_wcount));
#if HAVE_ST_DB_LOG_STAT_ST_WCOUNT_FILL
    rb_hash_aset(res, rb_tainted_str_new2("st_wcount_fill"), INT2NUM(bdb_stat->st_wcount_fill));
#endif
    rb_hash_aset(res, rb_tainted_str_new2("st_scount"), INT2NUM(bdb_stat->st_scount));
    rb_hash_aset(res, rb_tainted_str_new2("st_cur_file"), INT2NUM(bdb_stat->st_cur_file));
    rb_hash_aset(res, rb_tainted_str_new2("st_cur_offset"), INT2NUM(bdb_stat->st_cur_offset));
    rb_hash_aset(res, rb_tainted_str_new2("st_region_wait"), INT2NUM(bdb_stat->st_region_wait));
    rb_hash_aset(res, rb_tainted_str_new2("st_region_nowait"), INT2NUM(bdb_stat->st_region_nowait));
#if HAVE_ST_DB_LOG_STAT_ST_DISK_FILE
    rb_hash_aset(res, rb_tainted_str_new2("st_disk_file"), INT2NUM(bdb_stat->st_disk_file));
#endif
#if HAVE_ST_DB_LOG_STAT_ST_DISK_OFFSET
    rb_hash_aset(res, rb_tainted_str_new2("st_disk_offset"), INT2NUM(bdb_stat->st_disk_offset));
#if HAVE_ST_DB_LOG_STAT_ST_FLUSHCOMMIT
    rb_hash_aset(res, rb_tainted_str_new2("st_flushcommit"), INT2NUM(bdb_stat->st_flushcommit));
#endif
#if HAVE_ST_DB_LOG_STAT_ST_MAXCOMMITPERFLUSH
    rb_hash_aset(res, rb_tainted_str_new2("st_maxcommitperflush"), INT2NUM(bdb_stat->st_maxcommitperflush));
#endif
#if HAVE_ST_DB_LOG_STAT_ST_MINCOMMITPERFLUSH
    rb_hash_aset(res, rb_tainted_str_new2("st_mincommitperflush"), INT2NUM(bdb_stat->st_mincommitperflush));
#endif
#endif
    free(bdb_stat);
    return res;
}

#if ! HAVE_ST_DB_ENV_LOG_CURSOR

static VALUE
bdb_env_log_get(VALUE obj, VALUE a)
{
    bdb_ENV *envst;
    DBT data;
    struct dblsnst *lsnst;
    VALUE res, lsn;
    int ret, flag;

    GetEnvDB(obj, envst);
    flag = NUM2INT(a);
    MEMZERO(&data, DBT, 1);
    data.flags |= DB_DBT_MALLOC;
    lsn = bdb_makelsn(obj);
    Data_Get_Struct(lsn, struct dblsnst, lsnst);
#if HAVE_ST_DB_ENV_LG_INFO
    if (!envst->envp->lg_info) {
	rb_raise(bdb_eFatal, "log region not open");
    }
    ret = bdb_test_error(log_get(envst->envp->lg_info, lsnst->lsn, &data, flag));
#else
    ret = bdb_test_error(log_get(envst->envp, lsnst->lsn, &data, flag));
#endif
    if (ret == DB_NOTFOUND) {
	return Qnil;
    }
    res = rb_tainted_str_new(data.data, data.size);
    free(data.data);
    return rb_assoc_new(res, lsn);
}

#else

static VALUE bdb_log_cursor _((VALUE));

#endif

#define BDB_LOG_INIT 0
#define BDB_LOG_SET 1
#define BDB_LOG_NEXT 2

static VALUE
bdb_i_each_log_get(VALUE obj, int flag)
{
#if ! HAVE_ST_DB_ENV_LOG_CURSOR
    bdb_ENV *envst;
#endif
    struct dblsnst *lsnst, *lsnst1;
    DBT data;
    VALUE lsn, lsn1;
    VALUE res;
    int ret, init, flags;

    init = BDB_LOG_INIT;
#if ! HAVE_ST_DB_ENV_LOG_CURSOR
    GetEnvDB(obj, envst);
#else
    lsn = obj;
    Data_Get_Struct(obj, struct dblsnst, lsnst);
    flag = lsnst->flags;
    if (lsnst->cursor == 0) {
	DB_LSN *lsn1;

	init = BDB_LOG_SET;
	lsn1 = lsnst->lsn;
	lsn = bdb_makelsn(lsnst->env);
	Data_Get_Struct(lsn, struct dblsnst, lsnst);
	MEMCPY(lsnst->lsn, lsn1, DB_LSN, 1);
	bdb_log_cursor(lsn);
    }
#endif

    do {
#if ! HAVE_ST_DB_ENV_LOG_CURSOR
	lsn = bdb_makelsn(obj);
	Data_Get_Struct(lsn, struct dblsnst, lsnst);
#endif
	MEMZERO(&data, DBT, 1);
	data.flags |= DB_DBT_MALLOC;
	switch (init) {
	case BDB_LOG_INIT:
	    flags = (flag == DB_NEXT)?DB_FIRST:DB_LAST;
	    break;
	case BDB_LOG_SET:
	    flags = DB_SET;
	    break;
	default:
	    flags = flag;
	    break;
	}
	init = BDB_LOG_NEXT;
#if HAVE_ST_DB_ENV_LG_INFO
	if (!envst->envp->lg_info) {
	    rb_raise(bdb_eFatal, "log region not open");
	}
	ret = bdb_test_error(log_get(envst->envp->lg_info, lsnst->lsn, &data, flags));
#elif HAVE_ST_DB_ENV_LOG_CURSOR
	ret = bdb_test_error(lsnst->cursor->get(lsnst->cursor, lsnst->lsn, &data, flags));
	lsn1 = bdb_makelsn(lsnst->env);
	Data_Get_Struct(lsn1, struct dblsnst, lsnst1);
	MEMCPY(lsnst1->lsn, lsnst->lsn, DB_LSN, 1);
#else
	ret = bdb_test_error(log_get(envst->envp, lsnst->lsn, &data, flags));
	lsn1 = lsn;
#endif
	if (ret == DB_NOTFOUND) {
	    return Qnil;
	}
	res = rb_tainted_str_new(data.data, data.size);
	free(data.data);
	rb_yield(rb_assoc_new(res, lsn));
    } while (1);
    return Qnil;
}

#if ! HAVE_ST_DB_ENV_LOG_CURSOR
 
static VALUE
bdb_env_log_each(VALUE obj) 
{ 
    return bdb_i_each_log_get(obj, DB_NEXT);
}

static VALUE
bdb_env_log_hcae(VALUE obj)
{ 
    return bdb_i_each_log_get(obj, DB_PREV);
}

#else

static VALUE
log_cursor_close(VALUE obj)
{
    struct dblsnst *lsnst;

    Data_Get_Struct(obj, struct dblsnst, lsnst);
    if (lsnst->cursor) {
	bdb_test_error(lsnst->cursor->close(lsnst->cursor, 0));
	lsnst->cursor = 0;
    }
    return Qnil;
}

static VALUE
bdb_log_cursor_close(VALUE obj)
{
    struct dblsnst *lsnst;

    Data_Get_Struct(obj, struct dblsnst, lsnst);
    bdb_clean_env(lsnst->env, obj);
    
    return log_cursor_close(obj);
}

static VALUE
bdb_log_cursor(VALUE lsn)
{
    bdb_ENV *envst;
    struct dblsnst *lsnst;

    Data_Get_Struct(lsn, struct dblsnst, lsnst);
    if (!lsnst->cursor) {
	GetEnvDB(lsnst->env, envst);
	bdb_test_error(envst->envp->log_cursor(envst->envp, &lsnst->cursor, 0));
	bdb_ary_push(&envst->db_ary, lsn);
    }
    return lsn;
}
    
static VALUE
bdb_env_log_cursor(VALUE obj)
{
    return bdb_log_cursor(bdb_makelsn(obj));
}

static VALUE
bdb_env_i_get(VALUE obj)
{
    bdb_ENV *envst;
    struct dblsnst *lsnst;

    log_cursor_close(obj);
    Data_Get_Struct(obj, struct dblsnst, lsnst);
    GetEnvDB(lsnst->env, envst);
    bdb_test_error(envst->envp->log_cursor(envst->envp, &lsnst->cursor, 0));
    return bdb_i_each_log_get(obj, lsnst->flags);
}


static VALUE
bdb_env_log_each(VALUE obj)
{
    VALUE lsn;
    struct dblsnst *lsnst;

    lsn = bdb_makelsn(obj);
    Data_Get_Struct(lsn, struct dblsnst, lsnst);
    lsnst->flags = DB_NEXT;
    return rb_ensure(bdb_env_i_get, lsn, bdb_log_cursor_close, lsn);
}

static VALUE
bdb_env_log_hcae(VALUE obj)
{
    VALUE lsn;
    struct dblsnst *lsnst;

    lsn = bdb_makelsn(obj);
    Data_Get_Struct(lsn, struct dblsnst, lsnst);
    lsnst->flags = DB_PREV;
    return rb_ensure(bdb_env_i_get, lsn, bdb_log_cursor_close, lsn);
}

static VALUE
bdb_log_i_get(VALUE obj)
{
    struct dblsnst *lsnst;

    log_cursor_close(obj);
    Data_Get_Struct(obj, struct dblsnst, lsnst);
    return bdb_i_each_log_get(obj, lsnst->flags);
}

static VALUE
bdb_log_each(VALUE lsn)
{
    struct dblsnst *lsnst;

    Data_Get_Struct(lsn, struct dblsnst, lsnst);
    lsnst->flags = DB_NEXT;
    return rb_ensure(bdb_log_i_get, lsn, bdb_log_cursor_close, lsn);
}

static VALUE
bdb_log_hcae(VALUE lsn)
{
    struct dblsnst *lsnst;

    Data_Get_Struct(lsn, struct dblsnst, lsnst);
    lsnst->flags = DB_PREV;
    return rb_ensure(bdb_log_i_get, lsn, bdb_log_cursor_close, lsn);
 }
 

#endif
 
static VALUE
bdb_env_log_archive(int argc, VALUE *argv, VALUE obj)
{
    char **list, **file;
    bdb_ENV *envst;
    int flag;
    VALUE res;

    GetEnvDB(obj, envst);
    flag = 0;
    list = NULL;
    if (rb_scan_args(argc, argv, "01", &res)) {
	flag = NUM2INT(res);
    }
#if HAVE_ST_DB_ENV_LG_INFO
    if (!envst->envp->lg_info) {
	rb_raise(bdb_eFatal, "log region not open");
    }
    bdb_test_error(log_archive(envst->envp->lg_info, &list, flag, NULL));
#elif HAVE_ST_DB_ENV_LOG_ARCHIVE
    bdb_test_error(envst->envp->log_archive(envst->envp, &list, flag));
#else
#if HAVE_DB_LOG_ARCHIVE_4
    bdb_test_error(log_archive(envst->envp, &list, flag, NULL));
#else
    bdb_test_error(log_archive(envst->envp, &list, flag));
#endif
#endif
    res = rb_ary_new();
    for (file = list; file != NULL && *file != NULL; file++) {
	rb_ary_push(res, rb_tainted_str_new2(*file));
    }
    if (list != NULL) free(list);
    return res;
}

#define GetLsn(obj, lsnst, envst)			\
{							\
    Data_Get_Struct(obj, struct dblsnst, lsnst);	\
    GetEnvDB(lsnst->env, envst);			\
}

static VALUE
bdb_lsn_env(VALUE obj)
{
    struct dblsnst *lsnst;
    bdb_ENV *envst;
    GetLsn(obj, lsnst, envst);
    return lsnst->env;
}

static VALUE
bdb_lsn_log_file(VALUE obj)
{
    struct dblsnst *lsnst;
    bdb_ENV *envst;
    char name[2048];

    GetLsn(obj, lsnst, envst);
#if HAVE_ST_DB_ENV_LG_INFO
    if (!envst->envp->lg_info) {
	rb_raise(bdb_eFatal, "log region not open");
    }
    bdb_test_error(log_file(envst->envp->lg_info, lsnst->lsn, name, 2048));
#elif HAVE_ST_DB_ENV_LOG_FILE
    bdb_test_error(envst->envp->log_file(envst->envp, lsnst->lsn, name, 2048));
#else
    bdb_test_error(log_file(envst->envp, lsnst->lsn, name, 2048));
#endif
    return rb_tainted_str_new2(name);
}

static VALUE
bdb_lsn_log_flush(VALUE obj)
{
    struct dblsnst *lsnst;
    bdb_ENV *envst;

    GetLsn(obj, lsnst, envst);
#if HAVE_ST_DB_ENV_LG_INFO
    if (!envst->envp->lg_info) {
	rb_raise(bdb_eFatal, "log region not open");
    }
    bdb_test_error(log_flush(envst->envp->lg_info, lsnst->lsn));
#elif HAVE_ST_DB_ENV_LOG_FLUSH
    bdb_test_error(envst->envp->log_flush(envst->envp, lsnst->lsn));
#else
    bdb_test_error(log_flush(envst->envp, lsnst->lsn));
#endif
    return obj;
}

static VALUE
bdb_lsn_log_compare(VALUE obj, VALUE a)
{
    struct dblsnst *lsnst1, *lsnst2;
    bdb_ENV *envst1, *envst2;

    if (!rb_obj_is_kind_of(a, bdb_cLsn)) {
	rb_raise(bdb_eFatal, "invalid argument for <=>");
    }
    GetLsn(obj, lsnst1, envst1);
    GetLsn(a, lsnst2, envst2);
    return INT2NUM(log_compare(lsnst1->lsn, lsnst2->lsn));
}

static VALUE
bdb_lsn_log_get(int argc, VALUE *argv, VALUE obj)
{
    struct dblsnst *lsnst;
    DBT data;
    VALUE res, a;
    int ret, flags;
    bdb_ENV *envst;
#if HAVE_ST_DB_ENV_LOG_CURSOR
    DB_LOGC *cursor;
#endif

    flags = DB_SET;
    if (rb_scan_args(argc, argv, "01", &a) == 1) {
	flags = NUM2INT(a);
    }
    GetLsn(obj, lsnst, envst);
#if HAVE_ST_DB_ENV_LOG_CURSOR
    bdb_test_error(envst->envp->log_cursor(envst->envp, &cursor, 0));
#endif
    MEMZERO(&data, DBT, 1);
    data.flags |= DB_DBT_MALLOC;
#if HAVE_ST_DB_ENV_LG_INFO
    if (!envst->envp->lg_info) {
	rb_raise(bdb_eFatal, "log region not open");
    }
    ret = bdb_test_error(log_get(envst->envp->lg_info, lsnst->lsn, &data, flags));
#elif HAVE_ST_DB_ENV_LOG_CURSOR
    ret = cursor->get(cursor, lsnst->lsn, &data, flags);
    cursor->close(cursor, 0);
    ret = bdb_test_error(ret);
#else
    ret = bdb_test_error(log_get(envst->envp, lsnst->lsn, &data, flags));
#endif
    if (ret == DB_NOTFOUND) {
	return Qnil;
    }
    res = rb_tainted_str_new(data.data, data.size);
    free(data.data);
    return res;
}

static VALUE
bdb_log_register(VALUE obj, VALUE a)
{
#if ! (HAVE_ST_DB_ENV_LOG_REGISTER || HAVE_DB_LOG_REGISTER || HAVE_ST_DB_ENV_LG_INFO)
    rb_warn("log_register is obsolete");
    return Qnil;
#else
    bdb_DB *dbst;
    bdb_ENV *envst;

    if (TYPE(a) != T_STRING) {
	rb_raise(bdb_eFatal, "Need a filename");
    }
    if (bdb_env_p(obj) == Qfalse) {
	rb_raise(bdb_eFatal, "Database must be open in an Env");
    }
    Data_Get_Struct(obj, bdb_DB, dbst);
    Data_Get_Struct(dbst->env, bdb_ENV, envst);
#if HAVE_ST_DB_ENV_LG_INFO
    if (!envst->envp->lg_info) {
	rb_raise(bdb_eFatal, "log region not open");
    }
    bdb_test_error(log_register(envst->envp->lg_info, dbst->dbp, StringValuePtr(a), dbst->type, &envst->fidp));
#elif HAVE_ST_DB_ENV_LOG_REGISTER
    bdb_test_error(envst->envp->log_register(envst->envp, dbst->dbp, StringValuePtr(a)));
#else
#if HAVE_DB_LOG_REGISTER_4
    bdb_test_error(log_register(envst->envp, dbst->dbp, StringValuePtr(a), &envst->fidp));
#else
    bdb_test_error(log_register(envst->envp, dbst->dbp, StringValuePtr(a)));
#endif
#endif
    return obj;
#endif
}

static VALUE
bdb_log_unregister(VALUE obj)
{
#if ! (HAVE_ST_DB_ENV_LOG_UNREGISTER || HAVE_DB_LOG_UNREGISTER || HAVE_ST_DB_ENV_LG_INFO)
    rb_warn("log_unregister is obsolete");
    return Qnil;
#else
    bdb_DB *dbst;
    bdb_ENV *envst;

    if (bdb_env_p(obj) == Qfalse) {
	rb_raise(bdb_eFatal, "Database must be open in an Env");
    }
    Data_Get_Struct(obj, bdb_DB, dbst);
    Data_Get_Struct(dbst->env, bdb_ENV, envst);
#if HAVE_ST_DB_ENV_LG_INFO
    if (!envst->envp->lg_info) {
	rb_raise(bdb_eFatal, "log region not open");
    }
    bdb_test_error(log_unregister(envst->envp->lg_info, envst->fidp));
#elif HAVE_ST_DB_ENV_LOG_UNREGISTER
    bdb_test_error(envst->envp->log_unregister(envst->envp, dbst->dbp));
#else
#if HAVE_DB_LOG_REGISTER_4
    bdb_test_error(log_unregister(envst->envp, envst->fidp));
#else
    bdb_test_error(log_unregister(envst->envp, dbst->dbp));
#endif
#endif
    return obj;
#endif
}

void bdb_init_log()
{
    rb_define_method(bdb_cEnv, "log_put", bdb_s_log_put, -1);
    rb_define_method(bdb_cEnv, "log_curlsn", bdb_s_log_curlsn, 0);
    rb_define_method(bdb_cEnv, "log_checkpoint", bdb_s_log_checkpoint, 1);
    rb_define_method(bdb_cEnv, "log_flush", bdb_s_log_flush, -1);
    rb_define_method(bdb_cEnv, "log_stat", bdb_env_log_stat, -1);
    rb_define_method(bdb_cEnv, "log_archive", bdb_env_log_archive, -1);
#if ! HAVE_ST_DB_ENV_LOG_CURSOR 
    rb_define_method(bdb_cEnv, "log_get", bdb_env_log_get, 1);
#else
    rb_define_method(bdb_cEnv, "log_cursor", bdb_env_log_cursor, 0);
#endif
    rb_define_method(bdb_cEnv, "log_each", bdb_env_log_each, 0);
    rb_define_method(bdb_cEnv, "log_reverse_each", bdb_env_log_hcae, 0);
    rb_define_method(bdb_cCommon, "log_register", bdb_log_register, 1);
    rb_define_method(bdb_cCommon, "log_unregister", bdb_log_unregister, 0);
    bdb_cLsn = rb_define_class_under(bdb_mDb, "Lsn", rb_cObject);
    rb_include_module(bdb_cLsn, rb_mComparable);
#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
    rb_undef_alloc_func(bdb_cLsn);
#else
    rb_undef_method(CLASS_OF(bdb_cLsn), "allocate");
#endif
    rb_undef_method(CLASS_OF(bdb_cLsn), "new");
    rb_define_method(bdb_cLsn, "env", bdb_lsn_env, 0);
#if HAVE_ST_DB_ENV_LOG_CURSOR
    rb_define_method(bdb_cLsn, "log_cursor", bdb_log_cursor, 0);
    rb_define_method(bdb_cLsn, "cursor", bdb_log_cursor, 0);
    rb_define_method(bdb_cLsn, "log_close", bdb_log_cursor_close, 0);
    rb_define_method(bdb_cLsn, "close", bdb_log_cursor_close, 0);
    rb_define_method(bdb_cLsn, "log_each", bdb_log_each, 0);
    rb_define_method(bdb_cLsn, "each", bdb_log_each, 0);
    rb_define_method(bdb_cLsn, "log_reverse_each", bdb_log_hcae, 0);
    rb_define_method(bdb_cLsn, "reverse_each", bdb_log_hcae, 0);
#endif
    rb_define_method(bdb_cLsn, "log_get", bdb_lsn_log_get, -1);
    rb_define_method(bdb_cLsn, "get", bdb_lsn_log_get, -1);
    rb_define_method(bdb_cLsn, "log_compare", bdb_lsn_log_compare, 1);
    rb_define_method(bdb_cLsn, "compare", bdb_lsn_log_compare, 1);
    rb_define_method(bdb_cLsn, "<=>", bdb_lsn_log_compare, 1);
    rb_define_method(bdb_cLsn, "log_file", bdb_lsn_log_file, 0);
    rb_define_method(bdb_cLsn, "file", bdb_lsn_log_file, 0);
    rb_define_method(bdb_cLsn, "log_flush", bdb_lsn_log_flush, 0);
    rb_define_method(bdb_cLsn, "flush", bdb_lsn_log_flush, 0);
}
