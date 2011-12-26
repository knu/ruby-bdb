#include "bdb.h"

#if HAVE_TYPE_DB_SEQUENCE

static VALUE bdb_cSeq;

static void
bdb_seq_free(bdb_SEQ *seqst)
{
    if (seqst->seqp) {
        seqst->seqp->close(seqst->seqp, 0);
        seqst->seqp = NULL;
    }
    free(seqst);
}

static void
bdb_seq_mark(bdb_SEQ *seqst)
{
    rb_gc_mark(seqst->db);
    rb_gc_mark(seqst->txn);
    rb_gc_mark(seqst->orig);
}

static VALUE
bdb_seq_close(VALUE obj)
{
    bdb_SEQ *seqst;

    GetSEQ(obj, seqst);
    seqst->seqp->close(seqst->seqp, 0);
    seqst->seqp = NULL;
    return Qnil;
}

static VALUE
bdb_seq_txn_dup(VALUE obj, VALUE a)
{
    bdb_SEQ *seq0, *seq1;
    bdb_TXN *txnst;
    VALUE res;

    GetSEQ(obj, seq0);
    GetTxnDB(a, txnst);
    res = Data_Make_Struct(obj, bdb_SEQ, bdb_seq_mark, bdb_seq_free, seq1);
    MEMCPY(seq1, seq0, bdb_SEQ, 1);
    seq1->txn = a;
    seq1->txnid = txnst->txnid;
    seq1->orig = obj;
    return res;
}

static VALUE
bdb_seq_txn_close(VALUE obj, VALUE commit, VALUE real)
{
    bdb_SEQ *seqst;

    if (!real) {
        Data_Get_Struct(obj, bdb_SEQ, seqst);
        seqst->seqp = NULL;
    }
    else {
        bdb_seq_close(obj);
    }
    return Qnil;
}

static VALUE
bdb_seq_i_options(VALUE obj, VALUE seqobj)
{
    VALUE key, value;
    bdb_SEQ *seqst;
    char *options;

    key = rb_ary_entry(obj, 0);
    value = rb_ary_entry(obj, 1);
    key = rb_obj_as_string(key);
    options = StringValuePtr(key);
    Data_Get_Struct(seqobj, bdb_SEQ, seqst);
    if (strcmp(options, "set_cachesize") == 0) {
        if (seqst->seqp->set_cachesize(seqst->seqp, NUM2INT(value))) {
            seqst->seqp->remove(seqst->seqp, 0, 0);
            rb_raise(rb_eArgError, "Invalid value (%d) for set_cachesize",
                     NUM2INT(value));
        }
    }
    else if (strcmp(options, "set_flags") == 0) {
        if (seqst->seqp->set_flags(seqst->seqp, NUM2INT(value))) {
            seqst->seqp->remove(seqst->seqp, 0, 0);
            rb_raise(rb_eArgError, "Invalid value (%d) for set_flags",
                     NUM2INT(value));
        }
    }
    else if (strcmp(options, "set_range") == 0) {
        Check_Type(value, T_ARRAY);
        if (RARRAY_LEN(value) != 2) { 
            rb_raise(bdb_eFatal, "expected 2 values for range");
        }
        if (seqst->seqp->set_range(seqst->seqp, 
                                   NUM2LONG(RARRAY_PTR(value)[0]),
                                   NUM2LONG(RARRAY_PTR(value)[1]))) {
            seqst->seqp->remove(seqst->seqp, 0, 0);
            rb_raise(rb_eArgError, "Invalid value (%ld, %ld) for set_range",
                     NUM2LONG(RARRAY_PTR(value)[0]),
                     NUM2LONG(RARRAY_PTR(value)[1]));
        }
    }
    else {
        rb_warning("Unknown option %s", options);
    }
    return Qnil;
}

static VALUE
bdb_seq_open(int argc, VALUE *argv, VALUE obj)
{
    VALUE a, b, c, res;
    int flags = 0, count;
    bdb_DB *dbst;
    DB_TXN *txnid;
    DBT key;
    db_seq_t value;
    db_recno_t recno;
    bdb_SEQ *seqst;
    VALUE options = Qnil;

    INIT_TXN(txnid, obj, dbst);
    res = Data_Make_Struct(bdb_cSeq, bdb_SEQ, bdb_seq_mark, bdb_seq_free, seqst);
    seqst->db = obj;
    if (argc && TYPE(argv[argc - 1]) == T_HASH) {
        options = argv[argc - 1];
        argc--;
    }
    count = rb_scan_args(argc, argv, "12", &a, &b, &c);
    bdb_test_error(db_sequence_create(&seqst->seqp, dbst->dbp, 0));
    switch (count) {
    case 3:
        value = NUM2LONG(c);
        if (seqst->seqp->initial_value(seqst->seqp, value)) {
            seqst->seqp->remove(seqst->seqp, 0, 0);
            rb_raise(rb_eArgError, "invalid initial value");
        }
        /* ... */
    case 2:
        if (!NIL_P(flags)) {
            flags = NUM2INT(b);
        }
        break;
    }
    if (!NIL_P(options)) {
        rb_iterate(rb_each, options, bdb_seq_i_options, res);
    }
    a = bdb_test_recno(obj, &key, &recno, a);
    if (seqst->seqp->open(seqst->seqp, txnid, &key, flags)) {
        seqst->seqp->remove(seqst->seqp, txnid, 0);
        rb_raise(rb_eArgError, "can't open the sequence");
    }
    seqst->txn = dbst->txn;
    seqst->txnid = txnid;
    if (rb_block_given_p()) {
        return rb_ensure(rb_yield, res, bdb_seq_close, res);
    }
    return res;
}

static VALUE
bdb_seq_s_open(int argc, VALUE *argv, VALUE obj)
{
    VALUE args[4];

    if (argc <= 0 || argc > 3) {
        rb_raise(rb_eArgError, "Invalid number of arguments %d", argc);
    }
    args[0] = argv[0];
    args[1] = INT2NUM(DB_CREATE | DB_EXCL);
    if (argc > 1) {
        args[2] = argv[1];
        if (argc > 2) {
            args[3] = argv[2];
        }
    }
    return bdb_seq_open(argc + 1, args, obj);
}

        
static VALUE
bdb_seq_remove(int argc, VALUE *argv, VALUE obj)
{
    bdb_SEQ *seqst;
    VALUE a;
    int flags = 0;

    GetSEQ(obj, seqst);
    if (rb_scan_args(argc, argv, "01", &a)) {
        flags = NUM2INT(a);
    }
    if (seqst->seqp->remove(seqst->seqp, seqst->txnid, flags)) {
        rb_raise(rb_eArgError, "invalid argument");
    }
    seqst->seqp = NULL;
    return Qnil;
}

static VALUE
bdb_seq_get(int argc, VALUE *argv, VALUE obj)
{
    bdb_SEQ *seqst;
    int delta = 1, flags = 0;
    VALUE a, b;
    db_seq_t val;

    GetSEQ(obj, seqst);
    switch (rb_scan_args(argc, argv, "02", &a, &b)) {
    case 2:
        flags = NUM2INT(b);
        /* ... */
    case 1:
        delta = NUM2INT(a);
    }
    bdb_test_error(seqst->seqp->get(seqst->seqp, seqst->txnid, delta, &val, flags));
    return LONG2NUM(val);
}

static VALUE
bdb_seq_cachesize(VALUE obj)
{

    bdb_SEQ *seqst;
    int size;

    GetSEQ(obj, seqst);
    bdb_test_error(seqst->seqp->get_cachesize(seqst->seqp, &size));
    return INT2NUM(size);
}

static VALUE
bdb_seq_flags(VALUE obj)
{

    bdb_SEQ *seqst;
    unsigned int flags;

    GetSEQ(obj, seqst);
    bdb_test_error(seqst->seqp->get_flags(seqst->seqp, &flags));
    return INT2NUM(flags);
}

static VALUE
bdb_seq_range(VALUE obj)
{
    bdb_SEQ *seqst;
    db_seq_t deb, fin;

    GetSEQ(obj, seqst);
    bdb_test_error(seqst->seqp->get_range(seqst->seqp, &deb, &fin));
    return rb_assoc_new(LONG2NUM(deb), LONG2NUM(fin));
}

static VALUE
bdb_seq_stat(int argc, VALUE *argv, VALUE obj)
{
    bdb_SEQ *seqst;
    int  flags = 0;
    VALUE a, res;
    DB_SEQUENCE_STAT sta;

    GetSEQ(obj, seqst);
    if (rb_scan_args(argc, argv, "01", &a)) {
        flags = NUM2INT(a);
    }
    bdb_test_error(seqst->seqp->stat(seqst->seqp, (void *)&sta, flags));
    res = rb_hash_new();
    rb_hash_aset(res, rb_str_new2("wait"), INT2NUM(sta.st_wait));
    rb_hash_aset(res, rb_str_new2("nowait"), INT2NUM(sta.st_nowait));
    rb_hash_aset(res, rb_str_new2("current"), INT2NUM(sta.st_current));
    rb_hash_aset(res, rb_str_new2("value"), INT2NUM(sta.st_value));
    rb_hash_aset(res, rb_str_new2("last_value"), INT2NUM(sta.st_last_value));
    rb_hash_aset(res, rb_str_new2("min"), INT2NUM(sta.st_min));
    rb_hash_aset(res, rb_str_new2("max"), INT2NUM(sta.st_max));
    rb_hash_aset(res, rb_str_new2("cache_size"), INT2NUM(sta.st_cache_size));
    rb_hash_aset(res, rb_str_new2("flags"), INT2NUM(sta.st_flags));
    return res;
}

static VALUE
bdb_seq_db(VALUE obj)
{
    bdb_SEQ *seqst;

    GetSEQ(obj, seqst);
    return seqst->db;
}

static VALUE
bdb_seq_key(VALUE obj)
{
    bdb_SEQ *seqst;
    DBT key;

    GetSEQ(obj, seqst);
    bdb_test_error(seqst->seqp->get_key(seqst->seqp, &key));
    return bdb_test_load_key(seqst->db, &key);
}

#endif

void bdb_init_sequence()
{
#if HAVE_TYPE_DB_SEQUENCE
    bdb_cSeq = rb_define_class_under(bdb_mDb, "Sequence", rb_cObject);
#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
    rb_undef_alloc_func(bdb_cSeq);
#else
    rb_undef_method(CLASS_OF(bdb_cSeq), "allocate");
#endif
    rb_undef_method(CLASS_OF(bdb_cSeq), "new");
    rb_define_method(bdb_cCommon, "open_sequence", bdb_seq_open, -1);
    rb_define_method(bdb_cCommon, "create_sequence", bdb_seq_s_open, -1);
    rb_define_method(bdb_cSeq, "get", bdb_seq_get, -1);
    rb_define_method(bdb_cSeq, "stat", bdb_seq_stat, -1);
    rb_define_method(bdb_cSeq, "close", bdb_seq_close, 0);
    rb_define_method(bdb_cSeq, "remove", bdb_seq_remove, -1);
    rb_define_method(bdb_cSeq, "range", bdb_seq_range, 0);
    rb_define_method(bdb_cSeq, "cachesize", bdb_seq_cachesize, 0);
    rb_define_method(bdb_cSeq, "flags", bdb_seq_flags, 0);
    rb_define_method(bdb_cSeq, "db", bdb_seq_db, 0);
    rb_define_method(bdb_cSeq, "key", bdb_seq_key, 0);
    rb_define_private_method(bdb_cSeq, "__txn_close__", bdb_seq_txn_close, 2);
    rb_define_private_method(bdb_cSeq, "__txn_dup__", bdb_seq_txn_dup, 1);
#endif
}

