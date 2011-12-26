#include "bdbxml.h"

static VALUE xb_cEnv, xb_cRes, xb_cQue, xb_cMan;
static VALUE xb_cInd, xb_cUpd, xb_cMod, xb_cVal;
static VALUE xb_cCon, xb_cDoc, xb_cCxt, xb_mObs;
#if HAVE_DBXML_XML_INDEX_LOOKUP
static VALUE xb_cLook;
#endif
#if HAVE_DBXML_XML_EVENT_WRITER
static VALUE xb_cEwr;
#endif
#if HAVE_DBXML_XML_EVENT_READER
static VALUE xb_cErd;
#endif

static VALUE xb_io;
static ID id_current_env, id_close, id_read, id_write, id_pos;

#if 0
#define FREE_DEBUG(a, ...) fprintf(stderr, a, ##__VA_ARGS__)
#else
#define FREE_DEBUG(a, ...)
#endif

static void xb_val_mark(xval *val);
static void xb_val_free(xval *val);
static XmlValue xb_val_xml(VALUE);

static void
xb_io_push(VALUE obj)
{
    if (TYPE(xb_io) == T_ARRAY) {
        rb_ary_push(xb_io, obj);
    }
}

static void
xb_io_delete(VALUE obj, bool to_close)
{
    if (TYPE(xb_io) == T_ARRAY) {
        for (int i = 0; i < RARRAY_LEN(xb_io); i++) {
            if (RARRAY_PTR(xb_io)[i] == obj) {
                rb_ary_delete_at(xb_io, i);
                break;
            }
        }
    }
    if (to_close) {
        rb_funcall2(obj, id_close, 0, 0);
    }
}

namespace std {
    class xbWrite : public streambuf {
    public:
        xbWrite(VALUE obj, bool to_close):obj_(obj),close_(to_close) {
            xb_io_push(obj);
        }
        ~xbWrite() { 
            sync();
            xb_io_delete(obj_, close_);
        }
    protected:
        int_type overflow(int c) {
            if (c != traits_type::eof()) {
                buf_ += c;
                if (c == '\n') {
                    flush_line();
                }
            }
            return c;
        }
        int sync() {
            if (!buf_.empty()) {
                flush_line();
            }
            return 0;
        }
    private:
        VALUE obj_;
        string buf_;
        bool close_;

        void flush_line() {
            VALUE str = rb_str_new2(buf_.c_str());
            buf_.clear();
            rb_funcall(obj_, id_write, 1, str);
        }
            
    };

    class xbRead : public streambuf {
    public:
        xbRead(VALUE obj, bool to_close):obj_(obj),close_(to_close),
                                         eof_(false) {
            xb_io_push(obj);
        }
        ~xbRead() { 
            xb_io_delete(obj_, close_);
        }
    protected:
        int_type underflow() {
            if (gptr() < egptr()) return(traits_type::to_int_type(*gptr()));
            if (eof_) return traits_type::eof();
            VALUE str = rb_funcall(obj_, id_read, 1, INT2NUM(1024));
            if (NIL_P(str)) {
                eof_ = true;
                return traits_type::eof();
            }
            char *ptr = StringValuePtr(str);
            MEMCPY(buff_, ptr, char,  RSTRING(str)->len);
            setg(buff_, buff_, buff_ +  RSTRING(str)->len);
            return(traits_type::to_int_type(*gptr()));
        }
    private:
        VALUE obj_;
        bool close_, eof_;
        char buff_[1024];
    };
}

#include "dbxml/XmlInputStream.hpp"

class xbInput : public XmlInputStream {
public:
    xbInput(const VALUE obj):pos_(false),obj_(obj) {
        if (rb_respond_to(obj, id_pos)) {
            pos_ = true;
        }
        xb_io_push(obj);
    }
    ~xbInput() {
        xb_io_delete(obj_, false);
    }

    xbInput(const xbInput &);

    unsigned int curPos() const {
        if (pos_) {
            VALUE count = rb_funcall(obj_, id_pos, 0, 0);
            return NUM2ULONG(count);
        }
        return 0;
    }

    unsigned int readBytes(char* const fill, const unsigned int maxr) {
        VALUE line = rb_funcall(obj_, id_read, 1, INT2NUM(maxr));
        if (NIL_P(line)) return 0;
        char *str = StringValuePtr(line);
        MEMCPY(fill, str, char, RSTRING(line)->len);
        return RSTRING(line)->len;
    }

private:
    bool pos_;
    VALUE obj_;
    
};

class XbResolve : public XmlResolver {
public:
    XbResolve(VALUE man, VALUE obj):man_(man),obj_(obj) {}
    ~XbResolve() {}

    virtual bool 
    resolveCollection(XmlTransaction *xmltxn, XmlManager &xmlman,
                      const std::string &xmluri, XmlResults &xmlresult) 
        const {

	if (!rb_respond_to(obj_, rb_intern("resolve_collection"))) {
	    return false;
	}
        VALUE uri = rb_tainted_str_new2(xmluri.c_str());
        VALUE obj = retrieve_txn(xmltxn);
        VALUE rep = rb_funcall(obj_, rb_intern("resolve_collection"), 2,
                               obj, uri);
	if (NIL_P(rep)) {
	    return false;
	}
	if (TYPE(rep) == T_DATA && RDATA(rep)->dfree == (RDF)xb_res_free) {
	    xres *res = get_res(rep);
	    xmlresult = XmlResults(*res->res);
            return true;
        }
	else {
	    rb_raise(rb_eArgError, "object must an XML::Results");
	}
        return false;
    }

    virtual bool 
    resolveDocument(XmlTransaction *xmltxn, XmlManager &xmlman,
                      const std::string &xmluri, XmlValue &xmlval) 
        const {

	if (!rb_respond_to(obj_, rb_intern("resolve_document"))) {
	    return false;
	}
        VALUE uri = rb_tainted_str_new2(xmluri.c_str());
        VALUE obj = retrieve_txn(xmltxn);
        VALUE rep = rb_funcall(obj_, rb_intern("resolve_document"), 2,
                               obj, uri);
	if (NIL_P(rep)) {
	    return false;
	}
	xmlval = xb_val_xml(rep);
	return true;
    }

    virtual XmlInputStream *
    resolveEntity(XmlTransaction *xmltxn, XmlManager &xmlman,
                  const std::string &xmlsys, const std::string &xmlpub)
        const {

	if (!rb_respond_to(obj_, rb_intern("resolve_entity"))) {
	    return NULL;
	}
        VALUE sys = rb_tainted_str_new2(xmlsys.c_str());
        VALUE pub = rb_tainted_str_new2(xmlpub.c_str());
        VALUE obj = retrieve_txn(xmltxn);
        VALUE res = rb_funcall(obj_, rb_intern("resolve_entity"), 3,
                               obj, sys, pub);
        if (NIL_P(res)) {
            return NULL;
        }
        if (!rb_respond_to(res, id_read)) {
            rb_raise(rb_eArgError, "object must respond to #read");
        }
        return new xbInput(res);
    }

    virtual XmlInputStream *
    resolveSchema(XmlTransaction *xmltxn, XmlManager &xmlman,
                  const std::string &xmlschema, const std::string &xmlname)
        const {
	if (!rb_respond_to(obj_, rb_intern("resolve_schema"))) {
	    return NULL;
	}
        VALUE schema = rb_tainted_str_new2(xmlschema.c_str());
        VALUE name = rb_tainted_str_new2(xmlname.c_str());
        VALUE obj = retrieve_txn(xmltxn);
        VALUE res = rb_funcall(obj_, rb_intern("resolve_schema"), 3,
                               obj, schema, name);
        if (NIL_P(res)) {
            return NULL;
        }
        if (!rb_respond_to(res, id_read)) {
            rb_raise(rb_eArgError, "object must respond to #read");
        }
        return new xbInput(res);
    }

    virtual XmlInputStream *
    resolveModule(XmlTransaction *xmltxn, XmlManager &xmlman, 
		  const std::string &xmlmodule, const std::string &xmlname)
	const {
	if (!rb_respond_to(obj_, rb_intern("resolve_module"))) {
	    return NULL;
	}
        VALUE module = rb_tainted_str_new2(xmlmodule.c_str());
        VALUE name = rb_tainted_str_new2(xmlname.c_str());
        VALUE obj = retrieve_txn(xmltxn);
        VALUE res = rb_funcall(obj_, rb_intern("resolve_module"), 3,
                               obj, module, name);
        if (NIL_P(res)) {
            return NULL;
        }
        if (!rb_respond_to(res, id_read)) {
            rb_raise(rb_eArgError, "object must respond to #read");
        }
        return new xbInput(res);
    }

    virtual bool 
    resolveModuleLocation(XmlTransaction *xmltxn, XmlManager &xmlman,
                      const std::string &xmlname, XmlResults &xmlresult) 
        const {

	if (!rb_respond_to(obj_, rb_intern("resolve_module_location"))) {
	    return false;
	}
        VALUE name = rb_tainted_str_new2(xmlname.c_str());
        VALUE obj = retrieve_txn(xmltxn);
        VALUE rep = rb_funcall(obj_, rb_intern("resolve_collection"), 2,
                               obj, name);
	if (NIL_P(rep)) {
	    return false;
	}
	if (TYPE(rep) == T_DATA && RDATA(rep)->dfree == (RDF)xb_res_free) {
	    xres *res = get_res(rep);
	    xmlresult = XmlResults(*res->res);
            return true;
        }
	else {
	    rb_raise(rb_eArgError, "object must an XML::Results");
	}
        return false;
    }

private:
    VALUE man_;
    VALUE obj_;

    VALUE retrieve_txn(XmlTransaction *xmltxn) const {
        if (xmltxn == 0) return man_;
        xman *man = get_man(man_);
        bdb_ENV *envst;
        GetEnvDBErr(man->env, envst, id_current_env, xb_eFatal);
        if (!envst->db_ary.ptr) return man_;
        VALUE obj;
        for (int i = 0; i < envst->db_ary.len; i++) {
            obj = envst->db_ary.ptr[i];
            if (rb_obj_is_kind_of(obj, xb_cTxn)) {
                bdb_TXN *txnst;

                GetTxnDBErr(obj, txnst, xb_eFatal);
                if (xmltxn == (XmlTransaction *)txnst->txn_cxx) {
                    return obj;
                }
            }
        }
        return man_;
    }
                
};

static VALUE
xb_s_new(int argc, VALUE *argv, VALUE obj)
{
    VALUE res = rb_funcall2(obj, rb_intern("allocate"), 0, 0);
    rb_obj_call_init(res, argc, argv);
    return res;
}

static VALUE
xb_i_txn(VALUE obj, int *flags)
{
    VALUE key, value;
    char *options;

    key = rb_ary_entry(obj, 0);
    value = rb_ary_entry(obj, 1);
    key = rb_obj_as_string(key);
    options = StringValuePtr(key);
    if (strcmp(options, "flags") == 0) {
	*flags = NUM2INT(value);
    }
    return Qnil;
}

static VALUE
xb_protect_close(VALUE obj)
{
    return rb_funcall2(obj, id_close, 0, 0);
}

static void
xb_final(bdb_ENV *envst)
{
    VALUE *ary;
    int i;

    ary = envst->db_ary.ptr;
    if (ary) {
        envst->db_ary.mark = Qtrue;
        for (i = 0; i < envst->db_ary.len; i++) {
            if (rb_respond_to(ary[i], rb_intern("close"))) {
                xb_protect_close(ary[i]);
            }
        }
        envst->db_ary.mark = Qfalse;
        envst->db_ary.total = envst->db_ary.len = 0;
        envst->db_ary.ptr = 0;
        ::free(ary);
    }
    if (envst->envp) {
	if (!(envst->options & BDB_ENV_NOT_OPEN)) {
            DbEnv *env_cxx = static_cast<DbEnv *>(envst->envp->app_private);
            delete env_cxx;
	}
	envst->envp = NULL;
    }
}

static void
xb_env_free(bdb_ENV *envst)
{
    xb_final(envst);
    ::free(envst);
}

static VALUE
xb_env_close(VALUE obj)
{
    if (!OBJ_TAINTED(obj) && rb_safe_level() >= 4) {
	rb_raise(rb_eSecurityError, "Insecure: can't close the environnement");
    }
    bdb_ENV *envst;
    GetEnvDBErr(obj, envst, id_current_env, xb_eFatal);
    xb_final(envst);
    rset_obj(obj);
    return Qnil;
}

static VALUE
xb_env_s_alloc(VALUE obj)
{
    bdb_ENV *envst;
    return Data_Make_Struct(obj, bdb_ENV, 0, free, envst);
}

static VALUE
xb_env_s_i_options(VALUE obj, int *flags)
{
    VALUE key = rb_ary_entry(obj, 0);
    VALUE value = rb_ary_entry(obj, 1);
    key = rb_obj_as_string(key);
    char *options = StringValuePtr(key);
    if (strcmp(options, "env_flags") == 0) {
	*flags = NUM2INT(value);
    }
#if HAVE_DBXML_CONST_DB_CLIENT
    else if (strcmp(options, "set_rpc_server") == 0 ||
	     strcmp(options, "set_server") == 0) {
	*flags |= DB_CLIENT;
    }
#endif
    return Qnil;
}

static VALUE
xb_env_s_new(int argc, VALUE *argv, VALUE obj)
{
    VALUE res;
    bdb_ENV *envst;
    int flags = 0;

#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
    res = rb_obj_alloc(obj);
#else
    res = rb_funcall2(obj, rb_intern("allocate"), 0, 0);
#endif
    Data_Get_Struct(res, bdb_ENV, envst);
    if (argc && TYPE(argv[argc - 1]) == T_HASH) {
	rb_iterate(rb_each, argv[argc - 1], 
                   (VALUE(*)(ANYARGS))xb_env_s_i_options, (VALUE)&flags);
    }
    DbEnv *env = new DbEnv(flags);
    envst->envp = env->get_DB_ENV();
    envst->envp->app_private = static_cast<void *>(env);
    envst->envp->set_errpfx(envst->envp, "BDB::");
    bdb_test_error(envst->envp->set_alloc(envst->envp, malloc, realloc, free));
    RDATA(res)->dfree = (RDF)xb_env_free;
    rb_obj_call_init(res, argc, argv);
    return res;
}

static VALUE
xb_env_init(int argc, VALUE *argv, VALUE obj)
{
    bdb_env_init(argc, argv, obj);
    return obj;
}

static VALUE
xb_env_begin(int argc, VALUE *argv, VALUE obj)
{
    rb_raise(rb_eNoMethodError, "use a Manager to open a transaction");
}

static void
xb_man_mark(xman *man)
{
    rb_gc_mark(man->env);
    rb_gc_mark(man->rsv);
}

static VALUE
ent_each(VALUE klass)
{
    return rb_funcall(xb_mObs, rb_intern("each_object"), 1, klass);
}

static VALUE xb_res_close(VALUE obj);
static VALUE xb_val_close(VALUE obj);
static VALUE xb_doc_close(VALUE obj);
static VALUE xb_con_close(VALUE obj);

static VALUE
ent_release(VALUE obj, VALUE man)
{
    VALUE klass = CLASS_OF(obj);
    if (klass == xb_cRes) {
        xres *res;
        Data_Get_Struct(obj, xres, res);
        if (res->man == man) {
            FREE_DEBUG("res_release %x\n", obj);
            xb_res_close(obj);
        }
        return Qnil;
    }
    if (klass == xb_cVal) {
        xval *val;
        Data_Get_Struct(obj, xval, val);
        if (val->man == man) {
            FREE_DEBUG("val_release %x\n", obj);
            xb_val_close(obj);
        }
        return Qnil;
    }
    if (klass == xb_cDoc) {
        xdoc *doc;
        Data_Get_Struct(obj, xdoc, doc);
        if (doc->man == man) {
            FREE_DEBUG("doc_release %x\n", obj);
            xb_doc_close(obj);
        }
        return Qnil;
    }
    if (klass == xb_cCon) {
        xcon *con;
        Data_Get_Struct(obj, xcon, con);
        if (con->man == man) {
            FREE_DEBUG("con_release %x\n", obj);
            xb_con_close(obj);
        }
        return Qnil;
    }
    if (klass == xb_cQue) {
        xque *que;
        Data_Get_Struct(obj, xque, que);
        if (que->man == man) {
            FREE_DEBUG("que_release %x\n", obj);
            delete que->que;
            que->que = 0;
            rset_obj(obj);
        }
        return Qnil;
    }
    return Qnil;
}

static VALUE
clean_ent(VALUE ary)
{
    return rb_iterate(ent_each, RARRAY_PTR(ary)[1], 
                      (VALUE (*)(...))ent_release, RARRAY_PTR(ary)[0]);
}

static VALUE
xb_man_close(VALUE obj)
{
    xman *man = (xman *)DATA_PTR(obj);
    FREE_DEBUG("xb_man_close %x\n", man);
    if (man->env) {
        bdb_ENV *envst = (bdb_ENV *)DATA_PTR(man->env);
        bdb_ary_delete(&envst->db_ary, obj);
        man->env = 0;
    }
    VALUE ary = rb_ary_new2(2);
    rb_ary_push(ary, obj);
    rb_ary_push(ary, xb_cRes);
    rb_protect(clean_ent, ary, 0);
    rb_ary_store(ary, 1, xb_cVal);
    rb_protect(clean_ent, ary, 0);
    rb_ary_store(ary, 1, xb_cDoc);
    rb_protect(clean_ent, ary, 0);
    rb_ary_store(ary, 1, xb_cQue);
    rb_protect(clean_ent, ary, 0);
    rb_ary_store(ary, 1, xb_cCon);
    rb_protect(clean_ent, ary, 0);
    if (man->man) {
        delete man->man;
        man->man = 0;
    }
    return Qnil;
}

static void
xb_man_free(xman *man)
{
    FREE_DEBUG("xb_man_free %x\n", man);
    if (man->man) {
        xb_man_close(man->ori);
    }
    ::free(man);
}

static VALUE
xb_env_manager(int argc, VALUE *argv, VALUE obj)
{
    VALUE res, a;
    bdb_ENV *envst;
    DbEnv *env_cxx;
    xman *man;
    XmlManager *xmlman;
    int flags = 0;

    if (rb_scan_args(argc, argv, "01", &a)) {
        flags = NUM2INT(a) & ~DBXML_ADOPT_DBENV;
    }
    GetEnvDBErr(obj, envst, id_current_env, xb_eFatal);
    if (!(envst->options & BDB_ENV_NOT_OPEN)) {
        envst->options |= BDB_ENV_NOT_OPEN;
        flags |= DBXML_ADOPT_DBENV;
    }
    env_cxx = static_cast<DbEnv *>(envst->envp->app_private);
    PROTECT(xmlman = new XmlManager(env_cxx, flags));
    res = Data_Make_Struct(xb_cMan, xman, (RDF)xb_man_mark,
                           (RDF)xb_man_free, man);
    man->man = xmlman;
    man->env = obj;
    man->ori = res;
    return res;
}

static VALUE
xb_man_s_alloc(VALUE obj)
{
    xman *man;
    return Data_Make_Struct(obj, xman, 0, (RDF)free, man);
}

static VALUE
xb_man_init(int argc, VALUE *argv, VALUE obj)
{
    VALUE a = Qnil, b;
    xman *man;
    int flags = 0;

    switch (argc) {
    case 0:
        break;
    case 1:
        if (FIXNUM_P(a)) {
            flags = NUM2INT(a);
            a = Qnil;
        }
        else {
            a = argv[0];
        }
        break;
    default:
        if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
            flags = NUM2INT(b);
        }
    }
    if (NIL_P(a)) {
        Data_Get_Struct(obj, xman, man);
        PROTECT(man->man = new XmlManager(flags));
    }
    else {
        bdb_ENV *envst;
        DbEnv *env_cxx;

        GetEnvDBErr(a, envst, id_current_env, xb_eFatal);
        env_cxx = static_cast<DbEnv *>(envst->envp->app_private);
        Data_Get_Struct(obj, xman, man);
        PROTECT(man->man = new XmlManager(env_cxx, flags));
        man->env = a;
        bdb_ary_push(&envst->db_ary, obj);
    }
    RDATA(obj)->dfree = (RDF)xb_man_free;
    man->ori = obj;
    return obj;
}

static VALUE
xb_man_env(VALUE obj)
{
    xman *man = get_man(obj);
    if (RTEST(man->env)) {
        return man->env;
    }
    return Qnil;
}

static VALUE
xb_man_env_p(VALUE obj)
{
    xman *man = get_man(obj);
    if (RTEST(man->env)) {
        return Qtrue;
    }
    return Qfalse;
}

static VALUE
xb_man_home(VALUE obj)
{
    xman *man = get_man(obj);
    return rb_tainted_str_new2(man->man->getHome().c_str());
}

static VALUE
xb_man_page_get(VALUE obj)
{
    xman *man = get_man(obj);
    return INT2NUM(man->man->getDefaultPageSize());
}

static VALUE
xb_man_page_set(VALUE obj, VALUE a)
{
    xman *man = get_man(obj);
    u_int32_t page = NUM2INT(a);
    PROTECT(man->man->setDefaultPageSize(page));
    return a;
}

static VALUE
xb_man_flags_get(VALUE obj)
{
    xman *man = get_man(obj);
    return INT2NUM(man->man->getDefaultContainerFlags());
}

static VALUE
xb_man_flags_set(VALUE obj, VALUE a)
{
    xman *man = get_man(obj);
    u_int32_t flags = NUM2INT(a);
    PROTECT(man->man->setDefaultContainerFlags(flags));
    return a;
}

static VALUE
xb_man_type_get(VALUE obj)
{
    xman *man = get_man(obj);
    return INT2NUM(man->man->getDefaultContainerType());
}

static VALUE
xb_man_type_set(VALUE obj, VALUE a)
{
    xman *man = get_man(obj);
    XmlContainer::ContainerType type = XmlContainer::ContainerType(NUM2INT(a));
    PROTECT(man->man->setDefaultContainerType(type));
    return a;
}

static VALUE
xb_man_rename(VALUE obj, VALUE a, VALUE b)
{
    rb_secure(2);
    XmlTransaction *xmltxn = get_txn(obj);
    xman *man = get_man_txn(obj);
    char *oldname = StringValuePtr(a);
    char *newname = StringValuePtr(b);
    if (xmltxn) {
        PROTECT(man->man->renameContainer(*xmltxn, oldname, newname));
    }
    else {
        PROTECT(man->man->renameContainer(oldname, newname));
    }
    return obj;
}

static VALUE
xb_man_remove(VALUE obj, VALUE a)
{
    rb_secure(2);
    XmlTransaction *xmltxn = get_txn(obj);
    xman *man = get_man_txn(obj);
    char *name = StringValuePtr(a);
    if (xmltxn) {
        PROTECT(man->man->removeContainer(*xmltxn, name));
    }
    else {
        PROTECT(man->man->removeContainer(name));
    }
    return obj;
}

#if ! HAVE_RB_BLOCK_CALL

static VALUE
xb_each(VALUE *tmp)
{
    return rb_funcall2(tmp[0], (ID)tmp[1], (int)tmp[2], (VALUE *)tmp[3]);
}

#endif

static VALUE
xb_txn_missing(int argc, VALUE *argv, VALUE obj)
{
    bdb_TXN *txnst;

    GetTxnDBErr(obj, txnst, xb_eFatal);
    get_man(txnst->man);
    if (argc <= 0) { 
        rb_raise(rb_eArgError, "no id given");
    }
    ID id = SYM2ID(argv[0]);
    argc--; argv++;
    if (rb_block_given_p()) {
#if HAVE_RB_BLOCK_CALL
	return rb_block_call(txnst->man, id, argc, argv, 
			     (VALUE(*)(ANYARGS))rb_yield, 0);
#else
        VALUE tmp[4];

        tmp[0] = txnst->man;
        tmp[1] = (VALUE)id;
        tmp[2] = (VALUE)argc;
        tmp[3] = (VALUE)argv;
        return rb_iterate((VALUE(*)(VALUE))xb_each, (VALUE)tmp, 
                          (VALUE(*)(ANYARGS))rb_yield, 0);
#endif
    }
    return rb_funcall2(txnst->man, id, argc, argv);
}

static VALUE
xb_man_dump_con(int argc, VALUE *argv, VALUE obj)
{
    VALUE a, b, c;
    bool to_close = false;

    xman *man = get_man(obj);
    if (rb_scan_args(argc, argv, "21", &a, &b, &c) == 3) {
        to_close = RTEST(c);
    }
    char *name = StringValuePtr(a);
    if (rb_respond_to(b, id_write)) {
        std::xbWrite rbw(b, to_close);
        std::ostream out(&rbw);
        PROTECT(man->man->dumpContainer(name, &out));
    }
    else {    
        char *file = StringValuePtr(b);
        std::ofstream out(file);
        PROTECT2(man->man->dumpContainer(name, &out), out.close());
    }
    return obj;
}

static VALUE
xb_man_verify(int argc, VALUE *argv, VALUE obj)
{
    VALUE a, b, c, d;
    int flags = 0;

    rb_secure(4);
    switch (rb_scan_args(argc, argv, "12", &a, &b, &c, &d)) {
    case 4:
        flags = NUM2INT(d);
        /* ... */
    case 3:
        if (!rb_respond_to(b, id_write)) {
            flags = NUM2INT(c);
        }
    }
    xman *man = get_man(obj);
    char *name = StringValuePtr(a);
    if (flags & DB_SALVAGE) {
        if (rb_respond_to(b, id_write)) {
            std::xbWrite rbw(b, RTEST(c));
            std::ostream out(&rbw);
            PROTECT(man->man->verifyContainer(name, &out, flags));
        }
        else {
            char *file = StringValuePtr(b);
            std::ofstream out;
            out.open(file);
            PROTECT2(man->man->verifyContainer(name, &out, flags), 
                     out.close());
        }
    }
    else {
        std::ofstream out;
        PROTECT(man->man->verifyContainer(name, &out, flags));
    }
    return Qnil;
}

static VALUE
xb_man_load_con(int argc, VALUE *argv, VALUE obj)
{
    VALUE a, b, c = Qnil, d = Qnil;
    xupd *upd;
    XmlUpdateContext *xmlupd = 0;
    unsigned long lineno = 0;
    bool freeupd = true;

    rb_secure(2);
    xman *man = get_man(obj);
    switch (rb_scan_args(argc, argv, "22", &a, &b, &c, &d)) {
    case 4:
        upd = get_upd(d);
        xmlupd = upd->upd;
        freeupd = false;
        /* ... */
    case 3:
        if (!rb_respond_to(b, id_read)) {
            upd = get_upd(c);
            xmlupd = upd->upd;
            freeupd = false;
        }
    }
    char *name = StringValuePtr(a);
    if (rb_respond_to(b, id_read)) {
        std::xbRead rbs(b, RTEST(c));
        std::istream in(&rbs);
        if (freeupd) {
            xmlupd = new XmlUpdateContext(man->man->createUpdateContext());
        }
        PROTECT2(man->man->loadContainer(name, &in, &lineno, *xmlupd),
                 if (freeupd) delete xmlupd);
    }
    else {
        char *file = StringValuePtr(b);
        std::ifstream in(file);
        if (freeupd) {
            xmlupd = new XmlUpdateContext(man->man->createUpdateContext());
        }
        PROTECT2(man->man->loadContainer(name, &in, &lineno, *xmlupd),
                 in.close(); if (freeupd) delete xmlupd);
    }
    return obj;
}

static void
xmltxn_free(void **xmltxn)
{
    if (!xmltxn) return;
    if (!*xmltxn) return;
    XmlTransaction *cxxtxn = reinterpret_cast<XmlTransaction *>(*xmltxn);
    delete cxxtxn;
    // tell the c layer we deallocated it
    *xmltxn = NULL;
}

static int
xmltxn_abort( void * xmltxn )
{
    return reinterpret_cast<XmlTransaction *>(xmltxn)->getDbTxn()->abort();
}

static int
xmltxn_commit( void * xmltxn, u_int32_t flags )
{
    return reinterpret_cast<XmlTransaction *>(xmltxn)->getDbTxn()->commit(flags);
}

static int
xmltxn_discard( void * xmltxn, u_int32_t flags )
{
    return reinterpret_cast<XmlTransaction *>(xmltxn)->getDbTxn()->discard(flags);
}



static VALUE
xb_man_begin(int argc, VALUE *argv, VALUE obj)
{
    struct txn_rslbl txnr;
    xman *man;
    VALUE rman;
    XmlTransaction *xmltxn, *xmlold = 0;
    int flags = 0;

    if (argc) {
	if (TYPE(argv[argc - 1]) == T_HASH) {
	    rb_iterate(RMFS(rb_each), argv[argc - 1], RMF(xb_i_txn), (VALUE)&flags);
	}
	if (FIXNUM_P(argv[0])) {
	    flags = NUM2INT(argv[0]);
	}
	flags &= ~BDB_TXN_COMMIT;
    }
    if (rb_obj_is_kind_of(obj, xb_cTxn)) {
	bdb_TXN *txnst;

	GetTxnDBErr(obj, txnst, xb_eFatal);
        rman = txnst->man;
        xmlold = (XmlTransaction *)txnst->txn_cxx;
        man = get_man(txnst->man);
    }
    else {
        rman = obj;
        man = get_man(obj);
        if (!RTEST(man->env)) {
            rb_raise(rb_eArgError, "no environnement");
        }
        obj = man->env;
    }
    if (xmlold) {
        PROTECT(xmltxn = new XmlTransaction(xmlold->createChild(flags)));
    }
    else {
        PROTECT(xmltxn = new XmlTransaction(man->man->createTransaction(flags)));
    }
    txnr.txn_cxx = xmltxn;
    txnr.txn_cxx_free    = xmltxn_free;
    txnr.txn_cxx_abort   = xmltxn_abort;
    txnr.txn_cxx_commit  = xmltxn_commit;
    txnr.txn_cxx_discard = xmltxn_discard;

    txnr.txn = xmltxn->getDbTxn()->get_DB_TXN(); // XXX
    txnr.man = rman;
    return bdb_env_rslbl_begin((VALUE)&txnr, argc, argv, obj);
}

#if HAVE_DBXML_MAN_EXISTS_CONTAINER

static VALUE
xb_man_con_version(VALUE obj, VALUE a)
{
    xman *man = get_man(obj);
    char *uri = StringValuePtr(a);
    return INT2NUM(man->man->existsContainer(uri));
}

#endif

#if HAVE_DBXML_MAN_REINDEX_CONTAINER

static VALUE
xb_man_reindex(int argc, VALUE *argv, VALUE obj)
{
    XmlUpdateContext *xmlupd = 0;
    xupd *upd;
    VALUE a, b, c;
    std::string name;
    bool freeupd = true;
    int flags = 0;

    rb_secure(2);
    XmlTransaction *xmltxn = get_txn(obj);
    xman *man = get_man_txn(obj);
    switch (rb_scan_args(argc, argv, "12", &a, &b, &c)) {
    case 3:
        flags = NUM2INT(c);
        /* ... */
    case 2:
        if (!NIL_P(b)) {
            upd = get_upd(b);
            xmlupd = upd->upd;
            freeupd = false;
        }
        /* ... */
    case 1:
        name = StringValuePtr(a);
        break;
    }
    if (freeupd) {
        xmlupd = new XmlUpdateContext(man->man->createUpdateContext());
    }
    if (!xmltxn) {
        PROTECT2(man->man->reindexContainer(name, *xmlupd, flags),
                 if (freeupd) delete xmlupd);
    }
    else {
        PROTECT2(man->man->reindexContainer(*xmltxn, name, *xmlupd, flags),
                 if (freeupd) delete xmlupd);
    }
    return obj;
}

#endif

#if HAVE_DBXML_MAN_DEFAULT_SEQUENCE_INCREMENT

static VALUE
xb_man_increment_get(VALUE obj)
{
    xman *man = get_man(obj);
    return INT2NUM(man->man->getDefaultSequenceIncrement());
}

static VALUE
xb_man_increment_set(VALUE obj, VALUE a)
{
    xman *man = get_man(obj);
    int incr = NUM2INT(a);
    PROTECT(man->man->setDefaultSequenceIncrement(incr));
    return a;
}

#endif

#if HAVE_DBXML_MAN_GET_FLAGS

static VALUE
xb_man_get_flags(VALUE obj)
{
    xman *man = get_man(obj);
    return INT2NUM(man->man->getFlags());
}

#endif

#if HAVE_DBXML_MAN_GET_IMPLICIT_TIMEZONE

static VALUE
xb_man_get_itz(VALUE obj)
{
  xman *man = get_man(obj);
  return INT2NUM(man->man->getImplicitTimezone());
}

static VALUE
xb_man_set_itz(VALUE obj, VALUE a)
{
  xman *man = get_man(obj);
  int tz = NUM2INT(a);
  PROTECT(man->man->setImplicitTimezone(tz));
  return a;
}

#endif

#if HAVE_DBXML_MAN_COMPACT_CONTAINER

static VALUE
xb_man_compact_con(int argc, VALUE *argv, VALUE obj)
{
    XmlUpdateContext *xmlupd = 0;
    xupd *upd;
    VALUE a, b;
    std::string name;
    bool freeupd = true;
    int flags = 0;

    rb_secure(2);
    XmlTransaction *xmltxn = get_txn(obj);
    xman *man = get_man_txn(obj);
    switch (rb_scan_args(argc, argv, "11", &a, &b)) {
    case 2:
        if (!NIL_P(b)) {
            upd = get_upd(b);
            xmlupd = upd->upd;
            freeupd = false;
        }
        /* ... */
    case 1:
        name = StringValuePtr(a);
        break;
    }
    if (freeupd) {
        xmlupd = new XmlUpdateContext(man->man->createUpdateContext());
    }
    if (!xmltxn) {
        PROTECT2(man->man->compactContainer(name, *xmlupd, flags),
                 if (freeupd) delete xmlupd);
    }
    else {
        PROTECT2(man->man->compactContainer(*xmltxn, name, *xmlupd, flags),
                 if (freeupd) delete xmlupd);
    }
    return obj;
}

#endif

#if HAVE_DBXML_MAN_TRUNCATE_CONTAINER

static VALUE
xb_man_truncate_con(int argc, VALUE *argv, VALUE obj)
{
    XmlUpdateContext *xmlupd = 0;
    xupd *upd;
    VALUE a, b;
    std::string name;
    bool freeupd = true;
    int flags = 0;

    rb_secure(2);
    XmlTransaction *xmltxn = get_txn(obj);
    xman *man = get_man_txn(obj);
    switch (rb_scan_args(argc, argv, "11", &a, &b)) {
    case 2:
        if (!NIL_P(b)) {
            upd = get_upd(b);
            xmlupd = upd->upd;
            freeupd = false;
        }
        /* ... */
    case 1:
        name = StringValuePtr(a);
        break;
    }
    if (freeupd) {
        xmlupd = new XmlUpdateContext(man->man->createUpdateContext());
    }
    if (!xmltxn) {
        PROTECT2(man->man->truncateContainer(name, *xmlupd, flags),
                 if (freeupd) delete xmlupd);
    }
    else {
        PROTECT2(man->man->truncateContainer(*xmltxn, name, *xmlupd, flags),
                 if (freeupd) delete xmlupd);
    }
    return obj;
}

#endif

static void
xb_con_mark(xcon *con)
{
    rb_gc_mark(con->man);
    rb_gc_mark(con->ind);
    rb_gc_mark(con->txn);
#if HAVE_DBXML_XML_INDEX_LOOKUP
    rb_gc_mark(con->look);
#endif
}

static VALUE
close_txn(VALUE txn)
{
    bdb_TXN *txnst;

    Data_Get_Struct(txn, bdb_TXN, txnst);
    if (txnst->txnid) {
        rb_funcall2(txn, rb_intern("abort"), 0, 0);
    }
    return Qnil;
}

static void 
delete_ind(VALUE obj)
{
    if (!RTEST(obj)) return;
    xind *ind = get_ind(obj);
    delete ind->ind;
    ind->ind = 0;
    RDATA(obj)->dfree = free;
    RDATA(obj)->dmark = 0;
}

static void 
delete_look(VALUE obj)
{
    if (!RTEST(obj)) return;
    xlook *look = get_look(obj);
    delete look->look;
    look->look = 0;
    RDATA(obj)->dfree = free;
    RDATA(obj)->dmark = 0;
}

static void
xb_con_free(xcon *con)
{
    FREE_DEBUG("xb_con_free %x\n", con);
    if (con->con) {
        if (con->man) {
            delete_ind(con->ind);
#if HAVE_DBXML_XML_INDEX_LOOKUP
            delete_look(con->look);
#endif
            if (con->txn) {
                close_txn(con->txn);
            }
        }
        delete con->con;
    }
    ::free(con);
}

static VALUE
xb_int_open_con(int argc, VALUE *argv, VALUE obj, VALUE orig)
{
    VALUE res, a, b;
    XmlContainer *xmlcon;
    xcon *con;
    int flags = 0;

    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
        flags = NUM2INT(b);
    }
    if (flags && DB_CREATE) {
        rb_secure(4);
    }
    char *name = StringValuePtr(a);
    xman *man = get_man_txn(obj);
    XmlTransaction *xmltxn = get_txn(obj);
    if (xmltxn) {
        PROTECT(xmlcon = new XmlContainer(man->man->openContainer(*xmltxn, name, flags)));
    }
    else {
        PROTECT(xmlcon = new XmlContainer(man->man->openContainer(name, flags)));
    }
    if (!orig) {
        res = Data_Make_Struct(xb_cCon, xcon, (RDF)xb_con_mark, 
                               (RDF)xb_con_free, con);
    }
    else {
        res = orig;
        Data_Get_Struct(res, xcon, con);
        RDATA(res)->dfree = (RDF)xb_con_free;
    }
    FREE_DEBUG("open_con %x man %x\n", con, man);
    con->con = xmlcon;
    if (rb_obj_is_kind_of(obj, xb_cTxn)) {
        con->txn = obj;
        con->man = get_txn_man(obj);
    }
    else {
        con->man = obj;
    }
    con->opened = 1;
    return res;
}

static VALUE
xb_int_create_con(int argc, VALUE *argv, VALUE obj, VALUE orig)
{
    VALUE res, a, b, c, d;
    XmlContainer *xmlcon;
    xcon *con;

    rb_secure(4);
    xman *man = get_man_txn(obj);
    XmlTransaction *xmltxn = get_txn(obj);
    if (argc == 1) {
        char *name = StringValuePtr(argv[0]);
        if (xmltxn) {
            PROTECT(xmlcon = new XmlContainer(man->man->createContainer(*xmltxn, name)));
        }
        else {
            PROTECT(xmlcon = new XmlContainer(man->man->createContainer(name)));
        }
    }
    else {
        int flags = 0, mode = 0;
        XmlContainer::ContainerType type = XmlContainer::NodeContainer;
        switch (rb_scan_args(argc, argv, "13", &a, &b, &c, &d)) {
        case 4:
            mode = NUM2INT(d);
            /* ... */
        case 3:
            if (!NIL_P(c)) {
                type = XmlContainer::ContainerType(NUM2INT(c));
            }
            /* ... */
        case 2:
            if (!NIL_P(b)) {
                flags = NUM2INT(b);
            }
        }
        char *name = StringValuePtr(a);
        if (xmltxn) {
            PROTECT(xmlcon = new XmlContainer(man->man->createContainer(*xmltxn, name, flags, type, mode)));
        }
        else {
            PROTECT(xmlcon = new XmlContainer(man->man->createContainer(name, flags, type, mode)));
        }
    }
    if (!orig) {
        res = Data_Make_Struct(xb_cCon, xcon, (RDF)xb_con_mark, 
                               (RDF)xb_con_free, con);
    }
    else {
        res = orig;
        Data_Get_Struct(res, xcon, con);
        RDATA(res)->dfree = (RDF)xb_con_free;
    }
    FREE_DEBUG("open_con %x man %x\n", con, man);
    con->con = xmlcon;
    if (rb_obj_is_kind_of(obj, xb_cTxn)) {
        con->txn = obj;
        con->man = get_txn_man(obj);
    }
    else {
        con->man = obj;
    }
    con->opened = 1;
    return res;
}

static VALUE
xb_man_create_con(int argc, VALUE *argv, VALUE obj)
{
    return xb_int_create_con(argc, argv, obj, 0);
}

static VALUE
xb_man_open_con(int argc, VALUE *argv, VALUE obj)
{
    return xb_int_open_con(argc, argv, obj, 0);
}

static VALUE
xb_con_s_alloc(VALUE obj)
{
    xcon *con;
    return Data_Make_Struct(obj, xcon, (RDF)xb_con_mark, (RDF)free, con);
}

static VALUE
xb_con_s_open(int argc, VALUE *argv, VALUE obj)
{
    if (argc <= 0) {
        rb_raise(rb_eArgError, "invalid number of arguments (%d)", argc);
    }
    VALUE tmp = argv[0];
    argc--; argv++;
    get_man(tmp);
    VALUE res = xb_con_s_alloc(obj);
    return xb_int_open_con(argc, argv, tmp, res);
}

static VALUE
xb_con_init(int argc, VALUE *argv, VALUE obj)
{
    if (argc <= 0) {
        rb_raise(rb_eArgError, "invalid number of arguments (%d)", argc);
    }
    VALUE tmp = argv[0];
    argc--; argv++;
    return xb_int_open_con(argc, argv, tmp, obj);
}

static void
xb_upd_mark(xupd *upd)
{
    rb_gc_mark(upd->man);
}

static void
xb_upd_free(xupd *upd)
{
    FREE_DEBUG("xb_upd_free %x\n", upd);
    if (upd->upd) {
        delete upd->upd;
    }
    ::free(upd);
}

static VALUE
xb_int_create_upd(VALUE obj, VALUE orig)
{
    VALUE res;
    XmlUpdateContext *xmlupd;
    xupd *upd;

    xman *man = get_man(obj);
    PROTECT(xmlupd = new XmlUpdateContext(man->man->createUpdateContext()));
    if (!orig) {
        res = Data_Make_Struct(xb_cUpd, xupd, (RDF)xb_upd_mark, 
                               (RDF)xb_upd_free, upd);
    }
    else {
        res = orig;
        Data_Get_Struct(res, xupd, upd);
        RDATA(res)->dfree = (RDF)xb_upd_free;
    }
    upd->man = obj;
    upd->upd = xmlupd;
    return res;
}

static VALUE
xb_man_create_upd(VALUE obj)
{
    return xb_int_create_upd(obj, 0);
}

static VALUE
xb_upd_s_alloc(VALUE obj)
{
    xupd *upd;
    return Data_Make_Struct(obj, xupd, (RDF)xb_upd_mark, (RDF)free, upd);
}

static VALUE
xb_upd_init(VALUE obj, VALUE a)
{
    return xb_int_create_upd(a, obj);
}

static VALUE
xb_upd_manager(VALUE obj)
{
    xupd *upd = get_upd(obj);
    return upd->man;
}

#if HAVE_DBXML_UPDATE_APPLY_CHANGES

static VALUE
xb_upd_changes_get(VALUE obj)
{
    xupd *upd = get_upd(obj);
    if (RTEST(upd->upd->getApplyChangesToContainers())) {
        return Qtrue;
    }
    return Qfalse;
}

static VALUE
xb_upd_changes_set(VALUE obj, VALUE a)
{
    xupd *upd = get_upd(obj);
    upd->upd->setApplyChangesToContainers(RTEST(a));
    return obj;
}

#endif

static void
xb_doc_mark(xdoc *doc)
{
    rb_gc_mark(doc->man);
}

static void
doc_free(xdoc *doc)
{
    if (doc->doc) {
        delete doc->doc;
        doc->doc = 0;
    }
}

static void
xb_doc_free(xdoc *doc)
{
    FREE_DEBUG("xb_doc_free %x\n", doc);
    doc_free(doc);
    ::free(doc);
}

static VALUE
xb_int_create_doc(VALUE obj, VALUE orig)
{
    XmlDocument *xmldoc;
    xdoc *doc;

    xman *man = get_man(obj);
    PROTECT(xmldoc = new XmlDocument(man->man->createDocument()));
    VALUE res;

    if (!orig) {
        res = Data_Make_Struct(xb_cDoc, xdoc, (RDF)xb_doc_mark, 
                               (RDF)xb_doc_free, doc);
    }
    else {
        res = orig;
        Data_Get_Struct(res, xdoc, doc);
        RDATA(res)->dfree = (RDF)xb_doc_free;
    }
    doc->man = obj;
    doc->doc = xmldoc;
    return res;
}

static VALUE
xb_man_create_doc(VALUE obj)
{
    return xb_int_create_doc(obj, 0);
}

static VALUE
xb_doc_s_alloc(VALUE obj)
{
    xdoc *doc;
    return Data_Make_Struct(obj, xdoc, (RDF)xb_doc_mark, (RDF)free, doc);
}

static VALUE
xb_doc_init(VALUE obj, VALUE a)
{
    return xb_int_create_doc(a, obj);
}

static VALUE
xb_doc_close(VALUE obj)
{
    xdoc *doc = get_doc(obj);
    FREE_DEBUG("xb_doc_close %x\n", doc);
    doc_free(doc);
    rset_obj(obj);
    return Qnil;
}

static void
xb_cxt_mark(xcxt *cxt)
{
    rb_gc_mark(cxt->man);
}

static void
xb_cxt_free(xcxt *cxt)
{
    FREE_DEBUG("xb_cxt_free %x\n", cxt);
    if (cxt->cxt) {
        delete cxt->cxt;
    }
    ::free(cxt);
}

static VALUE
xb_int_create_cxt(int argc, VALUE *argv, VALUE obj, VALUE orig)
{
    XmlQueryContext::ReturnType rt = XmlQueryContext::LiveValues;
    XmlQueryContext::EvaluationType et = XmlQueryContext::Eager;
    VALUE a, b;
    xcxt *cxt;
    XmlQueryContext *xmlcxt;

    switch (rb_scan_args(argc, argv, "02", &a, &b)) {
    case 2:
        et = XmlQueryContext::EvaluationType(NUM2INT(b));
        /* ... */
    case 1:
        rt = XmlQueryContext::ReturnType(NUM2INT(a));
    }
    xman *man = get_man(obj);
    PROTECT(xmlcxt = new XmlQueryContext(man->man->createQueryContext(rt, et)));
    VALUE res;
    if (!orig) {
        res = Data_Make_Struct(xb_cCxt, xcxt, (RDF)xb_cxt_mark,
                               (RDF)xb_cxt_free, cxt);
    }
    else {
        res = orig;
        Data_Get_Struct(res, xcxt, cxt);
        RDATA(res)->dfree = (RDF)xb_cxt_free;
    }
    cxt->cxt = xmlcxt;
    cxt->man = obj;
    return res;
}

static VALUE
xb_man_create_cxt(int argc, VALUE *argv, VALUE obj)
{
    return xb_int_create_cxt(argc, argv, obj, 0);
}

static VALUE
xb_cxt_s_alloc(VALUE obj)
{
    xcxt *cxt;
    return Data_Make_Struct(obj, xcxt, (RDF)xb_cxt_mark, (RDF)free, cxt);
}
 
static VALUE
xb_cxt_init(int argc, VALUE *argv, VALUE obj)
{
    if (argc <= 0) {
        rb_raise(rb_eArgError, "invalid number of arguments (%d)", argc);
    }
    VALUE tmp = argv[0];
    argc--; argv++;
    return xb_int_create_cxt(argc, argv, tmp, obj);
}

static void
xb_que_mark(xque *que)
{
    rb_gc_mark(que->man);
    rb_gc_mark(que->txn);
}

static void
xb_que_free(xque *que)
{
    FREE_DEBUG("xb_que_free %x\n", que);
    if (que->que) {
        delete que->que;
    }
    ::free(que);
}
       
static VALUE
xb_man_prepare(int argc, VALUE *argv, VALUE obj)
{
    VALUE res, a, b;
    XmlQueryExpression *xmlqe;
    XmlQueryContext *xmlcxt;
    bool freecxt = false;

    xman *man = get_man_txn(obj);
    XmlTransaction *xmltxn = get_txn(obj);
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
        xcxt *cxt = get_cxt(b);
	xmlcxt = cxt->cxt;
    }
    else {
        xmlcxt = new XmlQueryContext(man->man->createQueryContext());
        freecxt = true;
    }
    char *str = StringValuePtr(a);
    if (xmltxn) {
        PROTECT2(xmlqe = new XmlQueryExpression(man->man->prepare(*xmltxn, str, *xmlcxt)),
                 if (freecxt) delete xmlcxt);
    }
    else {
        PROTECT2(xmlqe = new XmlQueryExpression(man->man->prepare(str, *xmlcxt)),
                 if (freecxt) delete xmlcxt);
    }
    xque *que;
    res = Data_Make_Struct(xb_cQue, xque, (RDF)xb_que_mark,
                           (RDF)xb_que_free, que);
    que->que = xmlqe;
    if (rb_obj_is_kind_of(obj, xb_cTxn)) {
        que->txn = obj;
        que->man = get_txn_man(obj);
    }
    else {
        que->man = obj;
    }
    return res;
}

static void
xb_res_mark(xres *res)
{
    rb_gc_mark(res->man);
}

static void
res_free(xres *res)
{
    if (res->res) {
        delete res->res;
        res->res = 0;
    }
}

static void
xb_res_free(xres *res)
{
    FREE_DEBUG("xb_res_free %x\n", res);
    res_free(res);
    ::free(res);
}

static VALUE
xb_res_close(VALUE obj)
{
    xres *res = get_res(obj);
    res_free(res);
    rset_obj(obj);
    return Qnil;
}

static VALUE
xb_int_create_res(VALUE obj, VALUE orig)
{
    xres *res;
    XmlResults *xmlres;

    xman *man = get_man(obj);
    PROTECT(xmlres = new XmlResults(man->man->createResults()));
    VALUE result;
    if (!orig) {
        result = Data_Make_Struct(xb_cRes, xres, (RDF)xb_res_mark,
                                  (RDF)xb_res_free, res);
    }
    else {
        result = orig;
        Data_Get_Struct(result, xres, res);
        RDATA(result)->dfree = (RDF)xb_res_free;
    }
    res->res = xmlres;
    res->man = obj;
    return result;
}

static VALUE
xb_man_create_res(VALUE obj)
{
    return xb_int_create_res(obj, 0);
}

static VALUE
xb_res_s_alloc(VALUE obj)
{
    xres *res;
    return Data_Make_Struct(obj, xres, (RDF)xb_res_mark, (RDF)free, res);
}

static VALUE
xb_res_init(VALUE obj, VALUE a)
{
    return xb_int_create_res(a, obj);
}

static VALUE
xb_man_query(int argc, VALUE *argv, VALUE obj)
{
    VALUE a, b, c;
    XmlResults *xmlres;
    XmlQueryContext *xmlcxt;
    bool freecxt = false;
    xcxt *cxt;
    int flags = 0;

    xman *man = get_man_txn(obj);
    XmlTransaction *xmltxn = get_txn(obj);
    switch(rb_scan_args(argc, argv, "12", &a, &b, &c)) {
    case 3:
        flags = NUM2INT(c);
    case 2:
        cxt = get_cxt(b);
	xmlcxt = cxt->cxt;
        break;
    default:
	xmlcxt = new XmlQueryContext(man->man->createQueryContext());
        freecxt = true;
    }
    char *str = StringValuePtr(a);
    if (xmltxn) {
        PROTECT2(xmlres = new XmlResults(man->man->query(*xmltxn, str, *xmlcxt, flags)),
                 if (freecxt) delete xmlcxt);
    }
    else {
        PROTECT2(xmlres = new XmlResults(man->man->query(str, *xmlcxt, flags)),
                 if (freecxt) delete xmlcxt);
    }
    xres *res;
    VALUE result = Data_Make_Struct(xb_cRes, xres, (RDF)xb_res_mark,
                                    (RDF)xb_res_free, res);
    res->res = xmlres;
    if (rb_obj_is_kind_of(obj, xb_cTxn)) {
        res->man = get_txn_man(obj);
    }
    else {
        res->man = obj;
    }
    if (rb_block_given_p()) {
#if HAVE_RB_BLOCK_CALL
	return rb_block_call(result, rb_intern("each"), 0, 0,
			     (VALUE(*)(ANYARGS))rb_yield, Qnil);
#else
        return rb_iterate(rb_each, result, (VALUE(*)(ANYARGS))rb_yield, Qnil);
#endif
    }
    return result;
}

static VALUE
xb_man_upgrade(int argc, VALUE *argv, VALUE obj)
{
    VALUE a, b;
    xupd *upd;
    XmlUpdateContext *xmlupd;
    bool freeupd = false;

    xman *man = get_man(obj);
    char *name = StringValuePtr(a);
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
        upd = get_upd(b);
        xmlupd = upd->upd;
    }
    else {
        xmlupd = new XmlUpdateContext(man->man->createUpdateContext());
        freeupd = true;
    }
    PROTECT2(man->man->upgradeContainer(name, *xmlupd),
             if (freeupd) delete xmlupd);
    return obj;
}

static VALUE
xb_con_manager(VALUE obj)
{
    xcon *con = get_con(obj);
    return con->man;
}

static VALUE
xb_con_alias_set(VALUE obj, VALUE a)
{
    xcon *con = get_con(obj);
    char *alias = StringValuePtr(a);
    if (con->con->addAlias(alias)) {
        return a;
    }
    return Qnil;
}

static VALUE
xb_con_alias_del(VALUE obj, VALUE a)
{
    xcon *con = get_con(obj);
    char *alias = StringValuePtr(a);
    if (con->con->removeAlias(alias)) {
        return a;
    }
    return Qfalse;
}

static VALUE
xb_con_all(int argc, VALUE *argv, VALUE obj)
{
    xres *res;
    XmlResults *xmlres;
    VALUE a;
    int flags = 0;

    if (rb_scan_args(argc, argv, "01", &a)) {
        flags = NUM2INT(a);
    }
    xcon *con = get_con(obj);
    XmlTransaction *xmltxn = get_con_txn(con);
    if (xmltxn) {
        PROTECT(xmlres = new XmlResults(con->con->getAllDocuments(*xmltxn, flags)));
    }
    else {
        PROTECT(xmlres = new XmlResults(con->con->getAllDocuments(flags)));
    }
    VALUE result = Data_Make_Struct(xb_cRes, xres, (RDF)xb_res_mark,
                                    (RDF)xb_res_free, res);
    res->res = xmlres;
    res->man = con->man;
    return result;
}

static VALUE
xb_con_type(VALUE obj)
{
    xcon *con = get_con(obj);
    return INT2NUM(con->con->getContainerType());
}

static VALUE
xb_con_sync(VALUE obj)
{
    xcon *con = get_con(obj);
    PROTECT(con->con->sync());
    return obj;
}

static VALUE
xb_con_close(VALUE obj)
{
    xcon *con = get_con(obj);
    FREE_DEBUG("xb_con_close %x\n", con);
    rset_obj(obj);
    delete_ind(con->ind);
#if HAVE_DBXML_XML_INDEX_LOOKUP
    delete_look(con->look);
#endif
    delete con->con;
    return Qnil;
}

static void
delete_doc(VALUE obj, xdoc *doc)
{
    if (TYPE(obj) != T_DATA || RDATA(obj)->dfree != (RDF)xb_doc_free) {
        rb_raise(rb_eArgError, "expected a Document object");
    }
    delete doc->doc;
    rset_obj(obj);
}

static VALUE
xb_con_add(int argc, VALUE *argv, VALUE obj)
{
    VALUE a, c, d;
    volatile VALUE b;
    xupd *upd;
    XmlUpdateContext *xmlupd = 0;
    bool freeupd = true;
    int flags = 0;
    
    rb_secure(4);
    xcon *con = get_con(obj);
    XmlTransaction *xmltxn = get_con_txn(con);
    rb_scan_args(argc, argv, "13", &a, &b, &c, &d);
    if (TYPE(a) == T_DATA && RDATA(a)->dmark == (RDF)xb_doc_mark) {
        if (argc == 4) {
            rb_raise(rb_eArgError, "invalid number of argument (4 for 3)");
        }
        if (argc >= 2 && !NIL_P(b)) {
            upd = get_upd(b);
            xmlupd = upd->upd;
            freeupd = false;
        }
        if (argc == 3) {
            flags = NUM2INT(c);
        }
        xdoc *doc = get_doc(a);
        if (freeupd) {
            xman *man = get_man(con->man);
            xmlupd = new XmlUpdateContext(man->man->createUpdateContext());
        }
        if (xmltxn) {
            PROTECT2(con->con->putDocument(*xmltxn, *doc->doc, *xmlupd, flags),
                     if (freeupd) delete xmlupd);
        }
        else {
            PROTECT2(con->con->putDocument(*doc->doc, *xmlupd, flags),
                     if (freeupd) delete xmlupd);
        }
        delete_doc(a, doc);
    }
    else {
	std::string new_id;

        if (argc == 1) {
            rb_raise(rb_eArgError, "invalid number of argument (1 for 2)");
        }
        char *name = StringValuePtr(a);
        if (argc >= 3 && !NIL_P(c)) {
            upd = get_upd(c);
            xmlupd = upd->upd;
            freeupd = false;
        }
        if (argc == 4) {
            flags = NUM2INT(d);
        }
        if (rb_respond_to(b, id_read)) {
            xbInput rbs(b);
            if (freeupd) {
                xman *man = get_man(con->man);
                xmlupd = new XmlUpdateContext(man->man->createUpdateContext());
            }
            if (xmltxn) {
                PROTECT2(new_id = con->con->putDocument(*xmltxn, name, &rbs, *xmlupd, flags),
                         if (freeupd) delete xmlupd);
            }
            else {
                PROTECT2(new_id = con->con->putDocument(name, &rbs, *xmlupd, flags),
                         if (freeupd) delete xmlupd);
            }
        }
        else {
            char *content = StringValuePtr(b);
            if (freeupd) {
                xman *man = get_man(con->man);
                xmlupd = new XmlUpdateContext(man->man->createUpdateContext());
            }
            if (xmltxn) {
                PROTECT2(new_id = con->con->putDocument(*xmltxn, name, content, *xmlupd, flags),
                         if (freeupd) delete xmlupd);
            }
            else {
                PROTECT2(new_id = con->con->putDocument(name, content, *xmlupd, flags),
                         if (freeupd) delete xmlupd);
            }
        }
	return rb_tainted_str_new2(new_id.c_str());
    }
    return obj;
}

static VALUE
xb_con_set(VALUE obj, VALUE a, VALUE b)
{
    VALUE tmp[2];
    tmp[0] = a;
    tmp[1] = b;
    return xb_con_add(2, tmp, obj);
}

static VALUE
xb_con_update(int argc, VALUE *argv, VALUE obj)
{
    VALUE a, b;
    XmlUpdateContext *xmlupd = 0;
    bool freeupd = true;
    
    rb_secure(4);
    xcon *con = get_con(obj);
    XmlTransaction *xmltxn = get_con_txn(con);
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
        xupd *upd = get_upd(b);
        xmlupd = upd->upd;
        freeupd = false;
    }
    if (TYPE(a) != T_DATA || RDATA(a)->dmark != (RDF)xb_doc_mark) {
        rb_raise(rb_eArgError, "expected an Document object");
    }
    xdoc *doc = get_doc(a);
    if (freeupd) {
        xman *man = get_man(con->man);
        xmlupd = new XmlUpdateContext(man->man->createUpdateContext());
    }
    if (xmltxn) {
        PROTECT2(con->con->updateDocument(*xmltxn, *doc->doc, *xmlupd),
                 if (freeupd) delete xmlupd);
    }
    else {
        PROTECT2(con->con->updateDocument(*doc->doc, *xmlupd),
                 if (freeupd) delete xmlupd);
    }
    delete_doc(a, doc);
    return obj;
}

static VALUE
xb_con_delete(int argc, VALUE *argv, VALUE obj)
{
    VALUE a, b;
    xupd *upd;
    XmlUpdateContext *xmlupd = 0;
    bool freeupd = true;
    
    rb_secure(4);
    xcon *con = get_con(obj);
    XmlTransaction *xmltxn = get_con_txn(con);
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
        upd = get_upd(b);
        xmlupd = upd->upd;
        freeupd = false;
    }
    if (TYPE(a) == T_DATA && RDATA(a)->dmark == (RDF)xb_doc_mark) {
        xdoc *doc = get_doc(a);
        if (freeupd) {
            xman *man = get_man(con->man);
            xmlupd = new XmlUpdateContext(man->man->createUpdateContext());
        }
        if (xmltxn) {
            PROTECT2(con->con->deleteDocument(*xmltxn, *doc->doc, *xmlupd),
                     if (freeupd) delete xmlupd);
        }
        else {
            PROTECT2(con->con->deleteDocument(*doc->doc, *xmlupd),
                     if (freeupd) delete xmlupd);
        }
    }
    else {
        char *name = StringValuePtr(a);
        if (freeupd) {
            xman *man = get_man(con->man);
            xmlupd = new XmlUpdateContext(man->man->createUpdateContext());
        }
        if (xmltxn) {
            PROTECT2(con->con->deleteDocument(*xmltxn, name, *xmlupd),
                     if (freeupd) delete xmlupd);
        }
        else {
            PROTECT2(con->con->deleteDocument(name, *xmlupd),
                     if (freeupd) delete xmlupd);
        }
    }
    return obj;
}

static VALUE
xb_con_txn_p(VALUE obj)
{
    xcon *con = get_con(obj);
    if (RTEST(con->txn)) {
        return Qtrue;
    }
    return Qfalse;
}

static VALUE
xb_con_txn(VALUE obj)
{
    xcon *con = get_con(obj);
    if (RTEST(con->txn)) {
        return con->txn;
    }
    return Qnil;
}

static VALUE
xb_con_name(VALUE obj)
{
    std::string name;
    xcon *con = get_con(obj);
    PROTECT(name = con->con->getName());
    return rb_tainted_str_new2(name.c_str());
}

static VALUE
xb_con_add_index(int argc, VALUE *argv, VALUE obj)
{
    xupd *upd;
    VALUE a, b, c, d, e;
    char *uri = 0, *name = 0, *index = 0;
    XmlUpdateContext *xmlupd = 0;
    
    xcon *con = get_con(obj);
    XmlTransaction *xmltxn = get_con_txn(con);

    switch(rb_scan_args(argc, argv, "32", &a, &b, &c, &d, &e)) {
    case 5:
    {
	uri = StringValuePtr(a);
	name = StringValuePtr(b);
	XmlIndexSpecification::Type type = XmlIndexSpecification::Type(NUM2INT(c));
	XmlValue::Type syntax = XmlValue::Type(NUM2INT(d));
	upd = get_upd(e);
	xmlupd = upd->upd;
	if (xmltxn) {
	    PROTECT(con->con->addIndex(*xmltxn, uri, name, type, syntax, *xmlupd));
	}
	else {
	    PROTECT(con->con->addIndex(uri, name, type, syntax, *xmlupd));
	}
	break;
    }
    case 4:
    {
	uri = StringValuePtr(a);
	name = StringValuePtr(b);
	if (TYPE(d) == T_DATA && RDATA(d)->dfree == (RDF)xb_upd_free) {
	    upd = get_upd(d);
	    xmlupd = upd->upd;
	    index = StringValuePtr(c);
	    if (xmltxn) {
		PROTECT(con->con->addIndex(*xmltxn, uri, name, index, *xmlupd));
	    }
	    else {
		PROTECT(con->con->addIndex(uri, name, index, *xmlupd));
	    }
	}
	else {
	    XmlValue::Type syntax = XmlValue::Type(NUM2INT(d));
	    XmlIndexSpecification::Type type = XmlIndexSpecification::Type(NUM2INT(c));
            xman *man = get_man(con->man);
            xmlupd = new XmlUpdateContext(man->man->createUpdateContext());
	    if (xmltxn) {
		PROTECT2(con->con->addIndex(*xmltxn, uri, name, type, syntax, *xmlupd),
			 delete xmlupd);
	    }
	    else {
		PROTECT2(con->con->addIndex(uri, name, type, syntax, *xmlupd),
			 delete xmlupd);
	    }
	}
	break;
    }
    case 3:
    {
	uri = StringValuePtr(a);
	name = StringValuePtr(b);
	index = StringValuePtr(c);
	xman *man = get_man(con->man);
	xmlupd = new XmlUpdateContext(man->man->createUpdateContext());
	if (xmltxn) {
	    PROTECT2(con->con->addIndex(*xmltxn, uri, name, index, *xmlupd),
		     delete xmlupd);
	}
	else {
	    PROTECT2(con->con->addIndex(uri, name, index, *xmlupd),
		     delete xmlupd);
	}
	break;
    }
    }
    return obj;
}
	
static VALUE
xb_con_add_def_index(int argc, VALUE *argv, VALUE obj)
{
    xupd *upd;
    VALUE a, b;
    char *index = 0;
    XmlUpdateContext *xmlupd = 0;
    
    xcon *con = get_con(obj);
    XmlTransaction *xmltxn = get_con_txn(con);

    switch(rb_scan_args(argc, argv, "11", &a, &b)) {
    case 2:
    {
	index = StringValuePtr(a);
	upd = get_upd(b);
	xmlupd = upd->upd;
	if (xmltxn) {
	    PROTECT(con->con->addDefaultIndex(*xmltxn, index, *xmlupd));
	}
	else {
	    PROTECT(con->con->addDefaultIndex(index, *xmlupd));
	}
	break;
    }
    case 1:
    {
	index = StringValuePtr(a);
	xman *man = get_man(con->man);
	xmlupd = new XmlUpdateContext(man->man->createUpdateContext());
	if (xmltxn) {
	    PROTECT2(con->con->addDefaultIndex(*xmltxn, index, *xmlupd),
		     delete xmlupd);
	}
	else {
	    PROTECT2(con->con->addDefaultIndex(index, *xmlupd),
		     delete xmlupd);
	}
	break;
    }
    }
    return obj;
}

static VALUE
xb_con_delete_index(int argc, VALUE *argv, VALUE obj)
{
    xupd *upd;
    VALUE a, b, c, d;
    char *uri = 0, *name = 0, *index = 0;
    XmlUpdateContext *xmlupd = 0;
    
    xcon *con = get_con(obj);
    XmlTransaction *xmltxn = get_con_txn(con);

    switch(rb_scan_args(argc, argv, "31", &a, &b, &c, &d)) {
    case 4:
    {
	uri = StringValuePtr(a);
	name = StringValuePtr(b);
	index = StringValuePtr(c);
	upd = get_upd(d);
	xmlupd = upd->upd;
	if (xmltxn) {
	    PROTECT(con->con->deleteIndex(*xmltxn, uri, name, index, *xmlupd));
	}
	else {
	    PROTECT(con->con->deleteIndex(uri, name, index, *xmlupd));
	}
	break;
    }
    case 3:
    {
	uri = StringValuePtr(a);
	name = StringValuePtr(b);
	index = StringValuePtr(c);
	xman *man = get_man(con->man);
	xmlupd = new XmlUpdateContext(man->man->createUpdateContext());
	if (xmltxn) {
	    PROTECT2(con->con->deleteIndex(*xmltxn, uri, name, index, *xmlupd),
		     delete xmlupd);
	}
	else {
	    PROTECT2(con->con->deleteIndex(uri, name, index, *xmlupd),
		     delete xmlupd);
	}
	break;
    }
    }
    return obj;
}
	
static VALUE
xb_con_delete_def_index(int argc, VALUE *argv, VALUE obj)
{
    xupd *upd;
    VALUE a, b;
    char *index = 0;
    XmlUpdateContext *xmlupd = 0;
    
    xcon *con = get_con(obj);
    XmlTransaction *xmltxn = get_con_txn(con);

    switch(rb_scan_args(argc, argv, "11", &a, &b)) {
    case 2:
    {
	index = StringValuePtr(a);
	upd = get_upd(b);
	xmlupd = upd->upd;
	if (xmltxn) {
	    PROTECT(con->con->deleteDefaultIndex(*xmltxn, index, *xmlupd));
	}
	else {
	    PROTECT(con->con->deleteDefaultIndex(index, *xmlupd));
	}
	break;
    }
    case 1:
    {
	index = StringValuePtr(a);
	xman *man = get_man(con->man);
	xmlupd = new XmlUpdateContext(man->man->createUpdateContext());
	if (xmltxn) {
	    PROTECT2(con->con->deleteDefaultIndex(*xmltxn, index, *xmlupd),
		     delete xmlupd);
	}
	else {
	    PROTECT2(con->con->deleteDefaultIndex(index, *xmlupd),
		     delete xmlupd);
	}
	break;
    }
    }
    return obj;
}
	
static VALUE
xb_con_replace_index(int argc, VALUE *argv, VALUE obj)
{
    xupd *upd;
    VALUE a, b, c, d;
    char *uri = 0, *name = 0, *index = 0;
    XmlUpdateContext *xmlupd = 0;
    
    xcon *con = get_con(obj);
    XmlTransaction *xmltxn = get_con_txn(con);

    switch(rb_scan_args(argc, argv, "31", &a, &b, &c, &d)) {
    case 4:
    {
	uri = StringValuePtr(a);
	name = StringValuePtr(b);
	index = StringValuePtr(c);
	upd = get_upd(d);
	xmlupd = upd->upd;
	if (xmltxn) {
	    PROTECT(con->con->replaceIndex(*xmltxn, uri, name, index, *xmlupd));
	}
	else {
	    PROTECT(con->con->replaceIndex(uri, name, index, *xmlupd));
	}
	break;
    }
    case 3:
    {
	uri = StringValuePtr(a);
	name = StringValuePtr(b);
	index = StringValuePtr(c);
	xman *man = get_man(con->man);
	xmlupd = new XmlUpdateContext(man->man->createUpdateContext());
	if (xmltxn) {
	    PROTECT2(con->con->replaceIndex(*xmltxn, uri, name, index, *xmlupd),
		     delete xmlupd);
	}
	else {
	    PROTECT2(con->con->replaceIndex(uri, name, index, *xmlupd),
		     delete xmlupd);
	}
	break;
    }
    }
    return obj;
}
	
static VALUE
xb_con_replace_def_index(int argc, VALUE *argv, VALUE obj)
{
    xupd *upd;
    VALUE a, b;
    char *index = 0;
    XmlUpdateContext *xmlupd = 0;
    
    xcon *con = get_con(obj);
    XmlTransaction *xmltxn = get_con_txn(con);

    switch(rb_scan_args(argc, argv, "11", &a, &b)) {
    case 2:
    {
	index = StringValuePtr(a);
	upd = get_upd(b);
	xmlupd = upd->upd;
	if (xmltxn) {
	    PROTECT(con->con->replaceDefaultIndex(*xmltxn, index, *xmlupd));
	}
	else {
	    PROTECT(con->con->replaceDefaultIndex(index, *xmlupd));
	}
	break;
    }
    case 1:
    {
	index = StringValuePtr(a);
	xman *man = get_man(con->man);
	xmlupd = new XmlUpdateContext(man->man->createUpdateContext());
	if (xmltxn) {
	    PROTECT2(con->con->replaceDefaultIndex(*xmltxn, index, *xmlupd),
		     delete xmlupd);
	}
	else {
	    PROTECT2(con->con->replaceDefaultIndex(index, *xmlupd),
		     delete xmlupd);
	}
	break;
    }
    }
    return obj;
}
	
static void 
xb_ind_mark(xind *ind)
{
    rb_gc_mark(ind->con);
}

static void 
xb_ind_free(xind *ind)
{
    FREE_DEBUG("xb_ind_free %x\n", ind);
    if (ind->ind) {
        delete ind->ind;
        if (RTEST(ind->con)) {
            xcon *con = get_con(ind->con);
            con->ind = 0;
        }
    }
   ::free(ind);
}

static VALUE
xb_con_index(int argc, VALUE *argv, VALUE obj)
{
    XmlIndexSpecification *xmlind;
    VALUE a;

    xcon *con = get_con(obj);
    XmlTransaction *xmltxn = get_con_txn(con);
    if (RTEST(con->ind)) return con->ind;
    if (xmltxn) {
        int flags = 0;

        if (rb_scan_args(argc, argv, "01", &a) == 1) {
            flags = NUM2INT(a);
        }
        PROTECT(xmlind = new XmlIndexSpecification(con->con->getIndexSpecification(*xmltxn, flags)));
    }
    else {
        if (argc) {
            rb_raise(xb_eFatal, "invalid number of arguments (%d for 0)", argc);
        }
        PROTECT(xmlind = new XmlIndexSpecification(con->con->getIndexSpecification()));
    }
    xind *ind;
    VALUE res = Data_Make_Struct(xb_cInd, xind, (RDF)xb_ind_mark,
                                 (RDF)xb_ind_free, ind); 
    ind->ind = xmlind;
    ind->con = obj;
    con->ind = res;
    return res;
}

static void
add_ind(VALUE obj, VALUE a)
{
    xcon *con = get_con(obj);
    if (con->ind == a) return;
    if (RTEST(con->ind)) {
        xind *ind = get_ind(con->ind);
        delete ind->ind;
        rset_obj(con->ind);
    }
    xind *ind = get_ind(a);
    if (ind->con != obj) {
        xcon *tmp = get_con(ind->con);
        tmp->ind = 0;
    }
    con->ind = a;
}

static VALUE
xb_con_index_set(int argc, VALUE *argv, VALUE obj)
{
    xupd *upd;
    XmlUpdateContext *xmlupd = 0;
    bool freeupd = true;
    VALUE a, b;
    
    rb_secure(4);
    xcon *con = get_con(obj);
    XmlTransaction *xmltxn = get_con_txn(con);
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
        upd = get_upd(b);
        xmlupd = upd->upd;
        freeupd = false;
    }
    xind *ind = get_ind(a);
    if (freeupd) {
        xman *man = get_man(con->man);
        xmlupd = new XmlUpdateContext(man->man->createUpdateContext());
    }
    if (xmltxn) {
        PROTECT2(con->con->setIndexSpecification(*xmltxn, *ind->ind, *xmlupd),
                 if (freeupd) delete xmlupd);
    }
    else {
        PROTECT2(con->con->setIndexSpecification(*ind->ind, *xmlupd),
                 if (freeupd) delete xmlupd);
    }
    add_ind(obj, a);
    return obj;
}

#if HAVE_DBXML_CON_INDEX_NODES

static VALUE
xb_con_index_p(VALUE obj)
{
    xcon *con = get_con(obj);
    if (con->con->getIndexNodes()) {
        return Qtrue;
    }
    return Qfalse;
}

#endif

#if HAVE_DBXML_CON_PAGESIZE

static VALUE
xb_con_pagesize(VALUE obj)
{
    xcon *con = get_con(obj);
    return INT2NUM(con->con->getPageSize());
}

#endif

static VALUE xb_xml_val(XmlValue *, VALUE);

#if HAVE_DBXML_CON_FLAGS

static VALUE
xb_con_flags(VALUE obj)
{
    xcon *con = get_con(obj);
    return INT2NUM(con->con->getFlags());
}

#endif

#if HAVE_DBXML_CON_NODE

static VALUE
xb_con_node(int argc, VALUE *argv, VALUE obj)
{
    VALUE a, b;
    XmlValue xmlval;
    int flags = 0;
    xcon *con = get_con(obj);
    XmlTransaction *xmltxn = get_con_txn(con);

    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
	flags = NUM2INT(b);
    }
    char *node = StringValuePtr(a);
    if (xmltxn) {
	PROTECT(xmlval = con->con->getNode(*xmltxn, node, flags));
    }
    else {
	PROTECT(xmlval = con->con->getNode(node, flags));
    }
    return xb_xml_val(&xmlval, con->man);
}

#endif

#if HAVE_DBXML_XML_EVENT_WRITER

static void
xb_ewr_mark(xewr *ewr)
{
    rb_gc_mark(ewr->con);
}

static void
xb_ewr_free(xewr *ewr)
{
    if (ewr->ewr) {
	PROTECT(ewr->ewr->close());
	ewr->ewr = 0;
    }
    ::free(ewr);
}

static VALUE
xb_con_ewr(int argc, VALUE *argv, VALUE obj)
{
    VALUE a, b, c;
    XmlEventWriter *xmlewr;
    xewr *ewr;
    int flags = 0;
    xcon *con = get_con(obj);
    XmlTransaction *xmltxn = get_con_txn(con);

    if (rb_scan_args(argc, argv, "21", &a, &b, &c) == 3) {
	flags = NUM2INT(b);
    }
    xdoc *doc = get_doc(a);
    xupd *upd = get_upd(b);
    if (xmltxn) {
	PROTECT(XmlEventWriter &rex = con->con->putDocumentAsEventWriter(*xmltxn, 
									 *doc->doc, 
									 *upd->upd, 
									 flags);
		xmlewr = (XmlEventWriter *)&rex);
    }
    else {
	PROTECT(XmlEventWriter &rex = con->con->putDocumentAsEventWriter(*doc->doc, 
									 *upd->upd, 
									 flags);
		xmlewr = (XmlEventWriter *)&rex);
    }
    VALUE res = Data_Make_Struct(xb_cEwr, xewr, (RDF)xb_ewr_mark, 
				 (RDF)xb_ewr_free, ewr);
    ewr->ewr = xmlewr;
    ewr->con = obj;
    return res;
}

#endif

static VALUE
xb_con_get(int argc, VALUE *argv, VALUE obj)
{
    XmlDocument *xmldoc;
    VALUE res;
    xdoc *doc;
    VALUE a, b;
    int flags = 0;

    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
        flags = NUM2INT(b);
    }
    char *name = StringValuePtr(a);
    xcon *con = get_con(obj);
    XmlTransaction *xmltxn = get_con_txn(con);
    if (xmltxn) {
        PROTECT(xmldoc = new XmlDocument(con->con->getDocument(*xmltxn, name, flags)));
    }
    else {
        PROTECT(xmldoc = new XmlDocument(con->con->getDocument(name, flags)));
    }
    res = Data_Make_Struct(xb_cDoc, xdoc, (RDF)xb_doc_mark, 
                           (RDF)xb_doc_free, doc);
    get_man(con->man);
    doc->doc = xmldoc;
    doc->man = con->man;
    return res;
}

static VALUE
xb_con_stat(int argc, VALUE *argv, VALUE obj)
{
    VALUE a, b, c, d, e, f;
    XmlValue xmlval = XmlValue();
    XmlStatistics *stats;
    char *index = 0, *p_uri = 0, *p_name = 0;
    bool has_parent = false;
    double total, uniq;

    switch (rb_scan_args(argc, argv, "33", &a, &b, &c, &d, &e, &f)) {
    case 6:
        xmlval = xb_val_xml(f);
        /* ... */
    case 5:
        p_uri = StringValuePtr(c);
        p_name = StringValuePtr(d);
        index = StringValuePtr(e);
        has_parent = true;
        break;
    case 4:
        xmlval = xb_val_xml(d);
        /* ... */
    case 3:
        index = StringValuePtr(c);
    }
    char *uri = StringValuePtr(a);
    char *name = StringValuePtr(b);
    xcon *con = get_con(obj);
    XmlTransaction *xmltxn = get_con_txn(con);
    if (xmltxn) {
        PROTECT(
            if (has_parent) {
                stats = new XmlStatistics(
                    con->con->lookupStatistics(*xmltxn, uri, name, p_uri, p_name,
                                               index, xmlval));
            }
            else {
                stats = new XmlStatistics(
                    con->con->lookupStatistics(*xmltxn, uri, name, index, xmlval));
            }
            total = stats->getNumberOfIndexedKeys();
            uniq = stats->getNumberOfUniqueKeys();
            delete stats;
            );
    }
    else {
        PROTECT(
            if (has_parent) {
                stats = new XmlStatistics(
                    con->con->lookupStatistics(uri, name, p_uri, p_name,
                                               index, xmlval));
            }
            else {
                stats = new XmlStatistics(
                    con->con->lookupStatistics(uri, name, index, xmlval));
            }
            total = stats->getNumberOfIndexedKeys();
            uniq = stats->getNumberOfUniqueKeys();
            delete stats;
            );
    }
    return rb_assoc_new(rb_float_new(total), rb_float_new(uniq));
}

static VALUE
xb_con_lookup(int argc, VALUE *argv, VALUE obj)
{
    VALUE a, b, c, d, e, f;
    XmlValue xmlval = XmlValue();
    XmlResults *xmlres;
    char *index = 0, *p_uri = 0, *p_name = 0;
    bool has_parent = false;
    XmlQueryContext *xmlcxt;
    bool freecxt = false;

    if (argc && TYPE(argv[0]) == T_DATA &&
        RDATA(argv[0])->dfree == (RDF)xb_cxt_free) {
        xcxt *cxt = get_cxt(argv[0]);
	xmlcxt = cxt->cxt;
        argc--; argv++;
    }
    else {
        xcon *con = get_con(obj);
        xman *man = get_man(con->man);
        xmlcxt = new XmlQueryContext(man->man->createQueryContext());
        freecxt = true;
    }

    switch (rb_scan_args(argc, argv, "33", &a, &b, &c, &d, &e, &f)) {
    case 6:
        xmlval = xb_val_xml(f);
        /* ... */
    case 5:
        p_uri = StringValuePtr(c);
        p_name = StringValuePtr(d);
        index = StringValuePtr(e);
        has_parent = true;
        break;
    case 4:
        xmlval = xb_val_xml(d);
        /* ... */
    case 3:
        index = StringValuePtr(c);
    }
    char *uri = StringValuePtr(a);
    char *name = StringValuePtr(b);
    xcon *con = get_con(obj);
    XmlTransaction *xmltxn = get_con_txn(con);
    if (xmltxn) {
        PROTECT2(
            if (has_parent) {
                xmlres = new XmlResults(
                    con->con->lookupIndex(*xmltxn, *xmlcxt, uri, name,
                                          p_uri, p_name, index, xmlval));
            }
            else {
                xmlres = new XmlResults(
                    con->con->lookupIndex(*xmltxn, *xmlcxt, uri, name,
                                          index, xmlval));
            },
            if (freecxt) delete xmlcxt);
    }
    else {
        PROTECT2(
            if (has_parent) {
                xmlres = new XmlResults(
                    con->con->lookupIndex(*xmlcxt, uri, name,
                                          p_uri, p_name, index, xmlval));
            }
            else {
                xmlres = new XmlResults(
                    con->con->lookupIndex(*xmlcxt, uri, name,
                                          index, xmlval));
            },
            if (freecxt) delete xmlcxt);
    }
    xres *res;
    VALUE result = Data_Make_Struct(xb_cRes, xres, (RDF)xb_res_mark,
                                    (RDF)xb_res_free, res);
    res->res = xmlres;
    res->man = con->man;
    return result;
}

static VALUE
xb_ind_manager(VALUE obj)
{
    xind *ind = get_ind(obj);
    return xb_con_manager(ind->con);
}

static VALUE
xb_ind_container(VALUE obj)
{
    xind *ind = get_ind(obj);
    if (RTEST(ind->con)) return ind->con;
    return Qnil;
}

static VALUE
xb_ind_default(VALUE obj)
{
     std::string def;
     xind *ind = get_ind(obj);
     PROTECT(def = ind->ind->getDefaultIndex());
     return rb_tainted_str_new2(def.c_str());
}

static VALUE
xb_ind_add_default(int argc, VALUE *argv, VALUE obj)
{
     VALUE a, b;

     xind *ind = get_ind(obj);
     if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
         XmlIndexSpecification::Type type = XmlIndexSpecification::Type(NUM2INT(a));
         XmlValue::Type syntax = XmlValue::Type(NUM2INT(b));
         PROTECT(ind->ind->addDefaultIndex(type, syntax));
     }
     else {
         char *as = StringValuePtr(a);
         PROTECT(ind->ind->addDefaultIndex(as));
     }
     return obj;
}

static VALUE
xb_ind_add(int argc, VALUE *argv, VALUE obj)
{
     VALUE a, b, c, d;
     char *uri, *name;

     xind *ind = get_ind(obj);
     switch (rb_scan_args(argc, argv, "31", &a, &b, &c, &d)) {
     case 4:
     {
         XmlIndexSpecification::Type type = XmlIndexSpecification::Type(NUM2INT(c));
         XmlValue::Type syntax = XmlValue::Type(NUM2INT(d));
         uri = StringValuePtr(a);
         name = StringValuePtr(b);
         PROTECT(ind->ind->addIndex(uri, name, type, syntax));
         break;
     }
     case 3:
     {
         char *index = StringValuePtr(c);
         uri = StringValuePtr(a);
         name = StringValuePtr(b);
         PROTECT(ind->ind->addIndex(uri, name, index));
         break;
     }
     default:
         rb_raise(rb_eArgError, "invalid number of arguments");
     }
     return obj;
}

static VALUE
xb_ind_delete_default(int argc, VALUE *argv, VALUE obj)
{
     VALUE a, b;

     xind *ind = get_ind(obj);
     if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
         XmlIndexSpecification::Type type = XmlIndexSpecification::Type(NUM2INT(a));
         XmlValue::Type syntax = XmlValue::Type(NUM2INT(b));
         PROTECT(ind->ind->deleteDefaultIndex(type, syntax));
     }
     else {
         char *as = StringValuePtr(a);
         PROTECT(ind->ind->deleteDefaultIndex(as));
     }
     return obj;
}

static VALUE
xb_ind_delete(int argc, VALUE *argv, VALUE obj)
{
     VALUE a, b, c, d;
     char *uri, *name;

     xind *ind = get_ind(obj);
     switch (rb_scan_args(argc, argv, "22", &a, &b, &c, &d)) {
     case 4:
     {
         XmlIndexSpecification::Type type = XmlIndexSpecification::Type(NUM2INT(c));
         XmlValue::Type syntax = XmlValue::Type(NUM2INT(d));
         uri = StringValuePtr(a);
         name = StringValuePtr(b);
         PROTECT(ind->ind->deleteIndex(uri, name, type, syntax));
         break;
     }
     case 3:
     {
         char *index = StringValuePtr(c);
         uri = StringValuePtr(a);
         name = StringValuePtr(b);
         PROTECT(ind->ind->deleteIndex(uri, name, index));
         break;
     }
     default:
         rb_raise(rb_eArgError, "invalid number of arguments");
     }
     return obj;
}

static VALUE
xb_ind_replace_default(int argc, VALUE *argv, VALUE obj)
{
     VALUE a, b;

     xind *ind = get_ind(obj);
     if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
         XmlIndexSpecification::Type type = XmlIndexSpecification::Type(NUM2INT(a));
         XmlValue::Type syntax = XmlValue::Type(NUM2INT(b));
         PROTECT(ind->ind->replaceDefaultIndex(type, syntax));
     }
     else {
         char *as = StringValuePtr(a);
         PROTECT(ind->ind->replaceDefaultIndex(as));
     }
     return obj;
}

static VALUE
xb_ind_replace(int argc, VALUE *argv, VALUE obj)
{
     VALUE a, b, c, d;
     char *uri, *name;

     xind *ind = get_ind(obj);
     switch (rb_scan_args(argc, argv, "31", &a, &b, &c, &d)) {
     case 4:
     {
         XmlIndexSpecification::Type type = XmlIndexSpecification::Type(NUM2INT(c));
         XmlValue::Type syntax = XmlValue::Type(NUM2INT(d));
         uri = StringValuePtr(a);
         name = StringValuePtr(b);
         PROTECT(ind->ind->replaceIndex(uri, name, type, syntax));
         break;
     }
     case 3:
     {
         char *index = StringValuePtr(c);
         uri = StringValuePtr(a);
         name = StringValuePtr(b);
         PROTECT(ind->ind->replaceIndex(uri, name, index));
         break;
     }
     default:
         rb_raise(rb_eArgError, "invalid number of arguments");
     }
     return obj;
}

static VALUE
xb_ind_each(VALUE obj)
{
    std::string uri, name, index;
    VALUE ret;

    xind *ind = get_ind(obj);
    PROTECT(ind->ind->reset());
    while (ind->ind->next(uri, name, index)) {
	ret = rb_ary_new2(3);
	rb_ary_push(ret, rb_tainted_str_new2(uri.c_str()));
	rb_ary_push(ret, rb_tainted_str_new2(name.c_str()));
	rb_ary_push(ret, rb_tainted_str_new2(index.c_str()));
	rb_yield(ret);
    }
    return Qnil;
}

static VALUE
xb_ind_to_a(VALUE obj)
{
    std::string uri, name, index;
    VALUE ret, res;

    xind *ind = get_ind(obj);
    PROTECT(ind->ind->reset());
    res = rb_ary_new();
    while (ind->ind->next(uri, name, index)) {
	ret = rb_ary_new2(3);
	rb_ary_push(ret, rb_tainted_str_new2(uri.c_str()));
	rb_ary_push(ret, rb_tainted_str_new2(name.c_str()));
	rb_ary_push(ret, rb_tainted_str_new2(index.c_str()));
	rb_ary_push(res, ret);
    }
    return res;
}

static VALUE
xb_ind_each_type(VALUE obj)
{
    std::string uri, name;
    XmlIndexSpecification::Type type;
    XmlValue::Type syntax;
    VALUE ret;

    xind *ind = get_ind(obj);
    PROTECT(ind->ind->reset());
    while (ind->ind->next(uri, name, type, syntax)) {
	ret = rb_ary_new2(4);
	rb_ary_push(ret, rb_tainted_str_new2(uri.c_str()));
	rb_ary_push(ret, rb_tainted_str_new2(name.c_str()));
	rb_ary_push(ret, INT2NUM(type));
	rb_ary_push(ret, INT2NUM(syntax));
	rb_yield(ret);
    }
    return Qnil;
}

static VALUE
xb_ind_find(int argc, VALUE *argv, VALUE obj)
{
    char *uri, *name;
    std::string index;

    xind *ind = get_ind(obj);
    if (argc == 1) {
	name = StringValuePtr(argv[0]);
	uri = (char *)"";
    }
    else {
	if (argc != 2) {
	    rb_raise(xb_eFatal, "invalid number of arguments (%d for 2)", argc);
	}
	uri = StringValuePtr(argv[0]);
	name = StringValuePtr(argv[1]);
    }
    if (ind->ind->find(uri, name, index)) {
	return rb_tainted_str_new2(index.c_str());
    }
    return Qnil;
}

#if HAVE_DBXML_XML_INDEX_LOOKUP

static void
xb_look_mark(xlook *look)
{
    rb_gc_mark(look->txn);
    rb_gc_mark(look->man);
    rb_gc_mark(look->con);
}

static void
xb_look_free(xlook *look)
{
    FREE_DEBUG("xb_look_free %x\n", look);
    if (look->look) {
        delete look->look;
        if (RTEST(look->con)) {
            xcon *con = get_con(look->con);
            con->look = 0;
        }
    }
    ::free(look);
}

static void
add_look(VALUE obj, VALUE a)
{
    xcon *con = get_con(obj);
    if (con->look == a) return;
    if (RTEST(con->look)) {
        xlook *look = get_look(con->look);
        delete look->look;
        rset_obj(con->look);
    }
    xlook *look = get_look(a);
    if (look->con != obj) {
        xcon *tmp = get_con(look->con);
        tmp->look = 0;
    }
    con->look = a;
}

static VALUE
xb_man_create_look(int argc, VALUE *argv, VALUE obj)
{
    XmlIndexLookup *xmllook;
    xlook *look;
    XmlValue xmlval = XmlValue();
    XmlIndexLookup::Operation xmlop = XmlIndexLookup::EQ;
    VALUE a, b, c, d, e, f, res;

    rb_secure(4);
    xman *man = get_man_txn(obj);
    switch(rb_scan_args(argc, argv, "42", &a, &b, &c, &d, &e, &f)) {
    case 6:
        xmlop = XmlIndexLookup::Operation(NUM2INT(f));
        /* ... */
    case 5:
        if (!NIL_P(e)) {
            xmlval = xb_val_xml(e);
        }
        /* ... */
    }
    xcon *con = get_con(a);
    char *uri = StringValuePtr(b);
    char *name = StringValuePtr(c);
    char *index = StringValuePtr(d);

    PROTECT(xmllook = new XmlIndexLookup(man->man->createIndexLookup(*con->con, uri, name, index,
								     xmlval, xmlop)));

    res = Data_Make_Struct(xb_cLook, xlook, (RDF)xb_look_mark, (RDF)xb_look_free, look);
    look->look = xmllook;
    if (rb_obj_is_kind_of(obj, xb_cTxn)) {
        look->txn = obj;
        look->man = get_txn_man(obj);
    }
    else {
        look->man = obj;
    }
    look->con = a;
    add_look(a, res);
    return res;
}

static VALUE
xb_look_manager(VALUE obj)
{
    xlook *look = get_look(obj);
    return look->man;
}

static VALUE
xb_look_transaction(VALUE obj)
{
    xlook *look = get_look(obj);
    return look->txn;
}

static VALUE
xb_look_transaction_p(VALUE obj)
{
    xlook *look = get_look(obj);
    if (RTEST(look->txn)) {
        return Qtrue;
    }
    return Qfalse;
}

static VALUE
xb_look_container_set(VALUE obj, VALUE a)
{
    xlook *look = get_look(obj);
    xcon *con = get_con(a);
    PROTECT(look->look->setContainer(*con->con));
    look->con = a;
    add_look(a, obj);
    return a;
}

static VALUE
xb_look_container_get(VALUE obj)
{
    xlook *look = get_look(obj);
    return look->con;
}

static VALUE
xb_look_highbound_set(VALUE obj, VALUE a)
{
    xlook *look = get_look(obj);
    a = rb_Array(a);
    if (RARRAY_LEN(a) != 2) {
        rb_raise(rb_eArgError, "invalid argument [value, operation]");
    }
    XmlValue xmlval = xb_val_xml(RARRAY_PTR(a)[0]);
    XmlIndexLookup::Operation xmlop = XmlIndexLookup::Operation(NUM2INT(RARRAY_PTR(a)[1]));
    PROTECT(look->look->setHighBound(xmlval, xmlop));
    return a;
}

static VALUE
xb_look_highbound_get(VALUE obj)
{
    xlook *look = get_look(obj);
    XmlIndexLookup::Operation xmlop = look->look->getHighBoundOperation();
    XmlValue xmlval;

    PROTECT(xmlval = look->look->getHighBoundValue());
    VALUE a = xb_xml_val(&xmlval, look->man);
    return rb_assoc_new(a, INT2NUM(xmlop));
}

static VALUE
xb_look_lowbound_set(VALUE obj, VALUE a)
{
    xlook *look = get_look(obj);
    a = rb_Array(a);
    if (RARRAY_LEN(a) != 2) {
        rb_raise(rb_eArgError, "invalid argument [value, operation]");
    }
    XmlValue xmlval = xb_val_xml(RARRAY_PTR(a)[0]);
    XmlIndexLookup::Operation xmlop = XmlIndexLookup::Operation(NUM2INT(RARRAY_PTR(a)[1]));
    PROTECT(look->look->setLowBound(xmlval, xmlop));
    return a;
}

static VALUE
xb_look_lowbound_get(VALUE obj)
{
    xlook *look = get_look(obj);
    XmlIndexLookup::Operation xmlop = look->look->getLowBoundOperation();
    XmlValue xmlval;

    PROTECT(xmlval = look->look->getLowBoundValue());
    VALUE a = xb_xml_val(&xmlval, look->man);
    return rb_assoc_new(a, INT2NUM(xmlop));
}

static VALUE
xb_look_index_set(VALUE obj, VALUE a)
{
    xlook *look = get_look(obj);
    char *index = StringValuePtr(a);
    PROTECT(look->look->setIndex(index));
    return a;
}

static VALUE
xb_look_index_get(VALUE obj)
{
    xlook *look = get_look(obj);
    std::string index;
    PROTECT(index = look->look->getIndex());
    return rb_tainted_str_new2(index.c_str());
}

static VALUE
xb_look_node_set(VALUE obj, VALUE a)
{
    xlook *look = get_look(obj);
    a = rb_Array(a);
    if (RARRAY_LEN(a) != 2) {
        rb_raise(rb_eArgError, "invalid argument [uri, name]");
    }
    char *uri = StringValuePtr(RARRAY_PTR(a)[0]);
    char *name = StringValuePtr(RARRAY_PTR(a)[1]);
    PROTECT(look->look->setNode(uri, name));
    return a;
}

static VALUE
xb_look_node_uri_get(VALUE obj)
{
    xlook *look = get_look(obj);
    std::string uri;
    PROTECT(uri = look->look->getNodeURI());
    return rb_tainted_str_new2(uri.c_str());
}

static VALUE
xb_look_node_uri_set(VALUE obj, VALUE a)
{
    xlook *look = get_look(obj);
    char *uri = StringValuePtr(a);
    PROTECT(look->look->setNode(uri, look->look->getNodeName()));
    return a;
}

static VALUE
xb_look_node_name_get(VALUE obj)
{
    xlook *look = get_look(obj);
    std::string name;
    PROTECT(name = look->look->getNodeName());
    return rb_tainted_str_new2(name.c_str());
}

static VALUE
xb_look_node_name_set(VALUE obj, VALUE a)
{
    xlook *look = get_look(obj);
    char *name = StringValuePtr(a);
    PROTECT(look->look->setNode(look->look->getNodeURI(), name));
    return a;
}

static VALUE
xb_look_node_get(VALUE obj)
{
    return rb_assoc_new(xb_look_node_uri_get(obj), xb_look_node_name_get(obj));
}

static VALUE
xb_look_parent_set(VALUE obj, VALUE a)
{
    xlook *look = get_look(obj);
    a = rb_Array(a);
    if (RARRAY_LEN(a) != 2) {
        rb_raise(rb_eArgError, "invalid argument [uri, name]");
    }
    char *uri = StringValuePtr(RARRAY_PTR(a)[0]);
    char *name = StringValuePtr(RARRAY_PTR(a)[1]);
    PROTECT(look->look->setParent(uri, name));
    return a;
}

static VALUE
xb_look_parent_uri_get(VALUE obj)
{
    xlook *look = get_look(obj);
    std::string uri;
    PROTECT(uri = look->look->getParentURI());
    return rb_tainted_str_new2(uri.c_str());
}

static VALUE
xb_look_parent_uri_set(VALUE obj, VALUE a)
{
    xlook *look = get_look(obj);
    char *uri = StringValuePtr(a);
    PROTECT(look->look->setParent(uri, look->look->getParentName()));
    return a;
}

static VALUE
xb_look_parent_name_get(VALUE obj)
{
    xlook *look = get_look(obj);
    std::string name;
    PROTECT(name = look->look->getParentName());
    return rb_tainted_str_new2(name.c_str());
}

static VALUE
xb_look_parent_name_set(VALUE obj, VALUE a)
{
    xlook *look = get_look(obj);
    char *name = StringValuePtr(a);
    PROTECT(look->look->setParent(look->look->getParentURI(), name));
    return a;
}

static VALUE
xb_look_parent_get(VALUE obj)
{
    return rb_assoc_new(xb_look_parent_uri_get(obj), xb_look_parent_name_get(obj));
}

static VALUE
xb_look_execute(int argc, VALUE *argv, VALUE obj)
{
    xlook *look = get_look(obj);
    XmlQueryContext *xmlcxt;
    xcxt *cxt;
    VALUE a, b;
    int flags = 0;
    bool freecxt = true;

    switch(rb_scan_args(argc, argv, "02", &a, &b)) {
    case 2:
        flags = NUM2INT(b);
        /* ... */
    case 1:
        if (!NIL_P(a)) {
            cxt = get_cxt(a);
            xmlcxt = cxt->cxt;
            freecxt = false;
        }
    }
    if (freecxt) {
        xman *man = get_man(look->man);
        xmlcxt = new XmlQueryContext(man->man->createQueryContext());
    }
    XmlResults *xmlres;
    if (!RTEST(look->txn)) {
        PROTECT2(xmlres = new XmlResults(look->look->execute(*xmlcxt, flags)),
                 if (freecxt) delete xmlcxt;);
    }
    else {
        bdb_TXN *txnst;

        GetTxnDBErr(look->txn, txnst, xb_eFatal);
        XmlTransaction *xmltxn = (XmlTransaction *)txnst->txn_cxx;
        PROTECT2(xmlres = new XmlResults(look->look->execute(*xmltxn, *xmlcxt, flags)),
                 if (freecxt) delete xmlcxt;);
    }
    xres *res;
    VALUE result = Data_Make_Struct(xb_cRes, xres, (RDF)xb_res_mark,
                                    (RDF)xb_res_free, res);
    res->res = xmlres;
    res->man = look->man;
    if (rb_block_given_p()) {
#if HAVE_RB_BLOCK_CALL
	return rb_block_call(result, rb_intern("each"), 0, 0,
			     (VALUE(*)(ANYARGS))rb_yield, Qnil);
#else
        return rb_iterate(rb_each, result, (VALUE(*)(ANYARGS))rb_yield, Qnil);
#endif
    }
    return result;
}

#endif

static void
xb_val_mark(xval *val)
{
    rb_gc_mark(val->man);
}

static void
val_free(xval *val)
{
    if (val->val) {
        delete val->val;
        val->val = 0;
    }
}

static void
xb_val_free(xval *val)
{
    FREE_DEBUG("xb_val_free %x\n", val);
    val_free(val);
    ::free(val);
}

static VALUE
xb_val_close(VALUE obj)
{
    xval *val;

    Data_Get_Struct(obj, xval, val);
    val_free(val);
    rset_obj(obj);
    return Qnil;
}

static VALUE
xb_xml_val(XmlValue *xmlval, VALUE man)
{
    VALUE res;
    if (xmlval->isNull()) {
	return Qnil;
    }
    if (xmlval->isString()) {
        return rb_tainted_str_new2(xmlval->asString().c_str());
    }
    if (xmlval->isNumber()) {
        return rb_float_new(xmlval->asNumber());
    }
    if (xmlval->isBoolean()) {
        if (xmlval->asBoolean()) {
            return Qtrue;
        }
        return Qfalse;
    }
    xval *val;
    res = Data_Make_Struct(xb_cVal, xval, (RDF)xb_val_mark, 
                           (RDF)xb_val_free, val);
    val->val = new XmlValue(*xmlval);
    val->man = man;
    return res;
}

static XmlValue
xb_val_xml(VALUE a)
{
    XmlValue xmlval;

    if (NIL_P(a)) {
	return XmlValue();
    }
    switch (TYPE(a)) {
    case T_STRING:
	xmlval = XmlValue(StringValuePtr(a));
	break;
    case T_FIXNUM:
    case T_FLOAT:
    case T_BIGNUM:
	xmlval = XmlValue(NUM2DBL(a));
	break;
    case T_TRUE:
	xmlval = XmlValue(true);
	break;
    case T_FALSE:
	xmlval = XmlValue(false);
	break;
    case T_DATA:
	if (RDATA(a)->dmark == (RDF)xb_doc_mark) {
            xdoc *doc;
	    Data_Get_Struct(a, xdoc, doc);
	    xmlval = XmlValue(*(doc->doc));
            break;
	}
        if (RDATA(a)->dmark == (RDF)xb_val_mark) {
            xval *val;
	    Data_Get_Struct(a, xval, val);
            xmlval = XmlValue(*(val->val));
            break;
        }
	/* ... */
    default:
	xmlval = XmlValue(StringValuePtr(a));
	break;
    }
    return xmlval;
}

static VALUE
xb_doc_manager(VALUE obj)
{
    xdoc *doc = get_doc(obj);
    return doc->man;
}

static VALUE
xb_doc_name_get(VALUE obj)
{
    xdoc *doc = get_doc(obj);
    return rb_tainted_str_new2(doc->doc->getName().c_str());
}

static VALUE
xb_doc_name_set(VALUE obj, VALUE a)
{
    xdoc *doc = get_doc(obj);
    char *str = StringValuePtr(a);
    PROTECT(doc->doc->setName(str));
    return a;
}

static VALUE
xb_doc_content_str(VALUE obj)
{
    std::string str;
    xdoc *doc = get_doc(obj);
    PROTECT(str = doc->doc->getContent(str));
    return rb_tainted_str_new2(str.c_str());
}

static VALUE
xb_doc_content_set(VALUE obj, VALUE a)
{
    xdoc *doc = get_doc(obj);
#if HAVE_DBXML_XML_EVENT_READER
    if (TYPE(a) == T_DATA && RDATA(a)->dmark == (RDF)xb_erd_mark) {
	xerd *erd = get_erd(a);
	PROTECT(doc->doc->setContentAsEventReader(*erd->erd));
	erd->erd = 0;
	return a;
    }
#endif
    char *str = StringValuePtr(a);
    PROTECT(doc->doc->setContent(str));
    return a;
}

static VALUE
xb_doc_get(int argc, VALUE *argv, VALUE obj)
{
    std::string uri = "";
    std::string name;
    XmlValue val;
    VALUE a, b;

    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
        if (!NIL_P(a)) {
            uri = StringValuePtr(a);
        }
        name = StringValuePtr(b);
    }
    else {
        name = StringValuePtr(a);
    }
    xdoc *doc = get_doc(obj);
    if (doc->doc->getMetaData(uri, name, val)) {
        return xb_xml_val(&val, doc->man);
    }
    return Qnil;
}

static VALUE
xb_doc_set(int argc, VALUE *argv, VALUE obj)
{
    std::string uri = "";
    std::string name;
    XmlValue val;
    VALUE a, b, c;

    if (rb_scan_args(argc, argv, "21", &a, &b, &c) == 3) {
        uri = StringValuePtr(a);
        name = StringValuePtr(b);
        val = xb_val_xml(c);
    }
    else {
        name = StringValuePtr(a);
        val = xb_val_xml(b);
    }
    xdoc *doc = get_doc(obj);
    PROTECT(doc->doc->setMetaData(uri, name, val));
    return obj;
}

static VALUE
xb_doc_remove(int argc, VALUE *argv, VALUE obj)
{
    std::string uri = "";
    std::string name;
    VALUE a, b;

    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
        uri = StringValuePtr(a);
        name = StringValuePtr(b);
    }
    else {
        name = StringValuePtr(a);
    }
    xdoc *doc = get_doc(obj);
    PROTECT(doc->doc->removeMetaData(uri, name));
    return obj;
}

static VALUE
xb_doc_each(VALUE obj)
{
    std::string uri = "";
    std::string name;
    XmlValue val;
    VALUE ary;

    xdoc *doc = get_doc(obj);
    XmlMetaDataIterator iter = doc->doc->getMetaDataIterator();
    while (iter.next(uri, name, val)) {
        ary = rb_ary_new2(3);
        rb_ary_push(ary, rb_tainted_str_new2(uri.c_str()));
        rb_ary_push(ary, rb_tainted_str_new2(name.c_str()));
        rb_ary_push(ary, xb_xml_val(&val, doc->man));
        rb_yield(ary);
    }
    return obj;
}

static VALUE
xb_doc_fetch(VALUE obj)
{
    xdoc *doc = get_doc(obj);
    doc->doc->fetchAllData();
    return obj;
}

static VALUE
xb_cxt_manager(VALUE obj)
{
    xcxt *cxt = get_cxt(obj);
    return cxt->man;
}

static VALUE
xb_cxt_uri_set(VALUE obj, VALUE a)
{
    xcxt *cxt = get_cxt(obj);
    char *uri = StringValuePtr(a);
    PROTECT(cxt->cxt->setBaseURI(uri));
    return a;
}

static VALUE
xb_cxt_uri_get(VALUE obj)
{
    xcxt *cxt = get_cxt(obj);
    std::string uri;
    PROTECT(uri = cxt->cxt->getBaseURI());
    return rb_tainted_str_new2(uri.c_str());
}

static VALUE
xb_cxt_name_get(VALUE obj, VALUE a)
{
    std::string uri;
    xcxt *cxt = get_cxt(obj);
    char *prefix = StringValuePtr(a);
    PROTECT(uri = cxt->cxt->getNamespace(prefix));
    return rb_tainted_str_new2(uri.c_str());
}

static VALUE
xb_cxt_name_set(VALUE obj, VALUE a, VALUE b)
{
    xcxt *cxt = get_cxt(obj);
    char *prefix = StringValuePtr(a);
    char *uri = StringValuePtr(b);
    PROTECT(cxt->cxt->setNamespace(prefix, uri));
    return obj;
}

static VALUE
xb_cxt_name_del(VALUE obj, VALUE a)
{
    xcxt *cxt = get_cxt(obj);
    char *prefix = StringValuePtr(a);
    PROTECT(cxt->cxt->removeNamespace(prefix));
    return obj;
}

static VALUE
xb_cxt_clear(VALUE obj)
{
    xcxt *cxt = get_cxt(obj);
    PROTECT(cxt->cxt->clearNamespaces());
    return obj;
}

static VALUE
xb_cxt_get(VALUE obj, VALUE a)
{
    xcxt *cxt = get_cxt(obj);
    char *name = StringValuePtr(a);
    XmlValue xmlval;
    PROTECT(cxt->cxt->getVariableValue(name, xmlval));
    return xb_xml_val(&xmlval, cxt->man);
}

#if HAVE_DBXML_CXT_VARIABLE_VALUE

static VALUE
xb_cxt_get_results(VALUE obj, VALUE a)
{
    xcxt *cxt = get_cxt(obj);
    char *name = StringValuePtr(a);
    XmlResults xmlres;
    PROTECT(cxt->cxt->getVariableValue(name, xmlres));
    xres *res;
    VALUE result = Data_Make_Struct(xb_cRes, xres, (RDF)xb_res_mark,
                                    (RDF)xb_res_free, res);
    res->res = new XmlResults(xmlres);
    res->man = cxt->man;
    return result;
}

#endif

static VALUE
xb_cxt_set(VALUE obj, VALUE a, VALUE b)
{
    xcxt *cxt = get_cxt(obj);
    char *name = StringValuePtr(a);
    if (TYPE(b) == T_DATA && RDATA(b)->dfree == (RDF)xb_res_free) {
        xres *res = get_res(b);
        PROTECT(cxt->cxt->setVariableValue(name, res->res));
    }
    else {
        XmlValue xmlval = xb_val_xml(b);
        PROTECT(cxt->cxt->setVariableValue(name, xmlval));
    }
    return obj;
}

static VALUE
xb_cxt_eval_set(VALUE obj, VALUE a)
{
    xcxt *cxt = get_cxt(obj);
    XmlQueryContext::EvaluationType type = XmlQueryContext::EvaluationType(NUM2INT(a));
    PROTECT(cxt->cxt->setEvaluationType(type));
    return a;
}

static VALUE
xb_cxt_eval_get(VALUE obj)
{
    xcxt *cxt = get_cxt(obj);
    XmlQueryContext::EvaluationType type;
    PROTECT(type = cxt->cxt->getEvaluationType());
    return INT2NUM(type);
}

static VALUE
xb_cxt_return_set(VALUE obj, VALUE a)
{
    xcxt *cxt = get_cxt(obj);
    XmlQueryContext::ReturnType type = XmlQueryContext::ReturnType(NUM2INT(a));
    PROTECT(cxt->cxt->setReturnType(type));
    return a;
}

static VALUE
xb_cxt_return_get(VALUE obj)
{
    xcxt *cxt = get_cxt(obj);
    XmlQueryContext::ReturnType type;
    PROTECT(type = cxt->cxt->getReturnType());
    return INT2NUM(type);
}

#if HAVE_DBXML_CXT_COLLECTION

static VALUE
xb_cxt_coll_set(VALUE obj, VALUE a)
{
    xcxt *cxt = get_cxt(obj);
    char *uri = StringValuePtr(a);
    PROTECT(cxt->cxt->setDefaultCollection(uri));
    return a;
}

static VALUE
xb_cxt_coll_get(VALUE obj)
{
    xcxt *cxt = get_cxt(obj);
    std::string uri;
    PROTECT(uri = cxt->cxt->getDefaultCollection());
    return rb_tainted_str_new2(uri.c_str());
}

#endif

#if HAVE_DBXML_CXT_INTERRUPT

static VALUE
xb_cxt_interrupt(VALUE obj)
{
    xcxt *cxt = get_cxt(obj);
    cxt->cxt->interruptQuery();
    return Qnil;
}

#endif

#if HAVE_DBXML_CXT_TIMEOUT

static VALUE
xb_cxt_get_timeout(VALUE obj)
{
    xcxt *cxt = get_cxt(obj);
    int to;
    PROTECT(to = cxt->cxt->getQueryTimeoutSeconds());
    return INT2NUM(to);
}

static VALUE
xb_cxt_set_timeout(VALUE obj, VALUE a)
{
    xcxt *cxt = get_cxt(obj);
    PROTECT(cxt->cxt->setQueryTimeoutSeconds(NUM2INT(a)));
    return a;
}

#endif

static VALUE
xb_que_manager(VALUE obj)
{
    xque *que = get_que(obj);
    return que->man;
}

static VALUE
xb_que_txn_p(VALUE obj)
{
    xque *que = get_que(obj);
    if (RTEST(que->txn)) {
        return Qtrue;
    }
    return Qfalse;
}

static VALUE
xb_que_txn(VALUE obj)
{
    xque *que = get_que(obj);
    if (RTEST(que->txn)) {
        return que->txn;
    }
    return Qnil;
}

static VALUE
xb_que_to_str(VALUE obj)
{
    xque *que = get_que(obj);
    std::string qe;
    PROTECT(qe = que->que->getQuery());
    return rb_tainted_str_new2(qe.c_str());
}

static VALUE
xb_que_exec(int argc, VALUE *argv, VALUE obj)
{
    XmlTransaction *xmltxn = 0;
    XmlQueryContext *xmlcxt;
    xcxt *cxt;
    XmlValue xmlval;
    bool has_cxt = true;
    bool has_val = false;
    int flags = 0;

    xque *que = get_que(obj);
    switch (argc) {
    case 0:
    {
        xman *man = get_man(que->man);
        xmlcxt = new XmlQueryContext(man->man->createQueryContext());
        has_cxt = false;
    }
    break;
    case 1:
        if (TYPE(argv[0]) == T_DATA &&
            RDATA(argv[0])->dfree == (RDF)xb_cxt_free) {
            cxt = get_cxt(argv[0]);
            xmlcxt = cxt->cxt;
        }
        else {
            xmlval = xb_val_xml(argv[0]);
            has_val = true;
            xman *man = get_man(que->man);
            xmlcxt = new XmlQueryContext(man->man->createQueryContext());
            has_cxt = false;
        }
        break;
    case 2:
        if (TYPE(argv[0]) == T_DATA &&
            RDATA(argv[0])->dfree == (RDF)xb_cxt_free) {
            cxt = get_cxt(argv[0]);
            xmlcxt = cxt->cxt;
            flags = NUM2INT(argv[1]);
        }
        else {
            cxt = get_cxt(argv[1]);
            xmlcxt = cxt->cxt;
            xmlval = xb_val_xml(argv[0]);
            has_val = true;
        }
        break;
    case 3:
        cxt = get_cxt(argv[1]);
        xmlcxt = cxt->cxt;
        xmlval = xb_val_xml(argv[0]);
        has_val = true;
        flags = NUM2INT(argv[2]);
        break;
    default:
        rb_raise(rb_eArgError, "invalid number of arguments");
        break;
    }
    if (RTEST(que->txn)) {
        bdb_TXN *txnst;

        GetTxnDBErr(que->txn, txnst, xb_eFatal);
        xmltxn = (XmlTransaction *)txnst->txn_cxx;
    }
    XmlResults *xmlres;
    if (has_val) {
        if (xmltxn) {
            PROTECT2(xmlres = new XmlResults(que->que->execute(*xmltxn, xmlval,
                                                              *xmlcxt, flags)),
                     if (!has_cxt) delete xmlcxt;);
        }
        else {
            PROTECT2(xmlres = new XmlResults(que->que->execute(xmlval, 
                                                               *xmlcxt, flags)),
                     if (!has_cxt) delete xmlcxt;);
        }
    }
    else {
        if (xmltxn) {
            PROTECT2(xmlres = new XmlResults(que->que->execute(*xmltxn, *xmlcxt, flags)),
                     if (!has_cxt) delete xmlcxt;);
        }
        else {
            PROTECT2(xmlres = new XmlResults(que->que->execute(*xmlcxt, flags)),
                     if (!has_cxt) delete xmlcxt;);
        }
    }
    xres *res;
    VALUE result = Data_Make_Struct(xb_cRes, xres, (RDF)xb_res_mark,
                                    (RDF)xb_res_free, res);
    res->res = xmlres;
    res->man = que->man;
    if (rb_block_given_p()) {
#if HAVE_RB_BLOCK_CALL
	return rb_block_call(result, rb_intern("each"), 0, 0,
			     (VALUE(*)(ANYARGS))rb_yield, Qnil);
#else
        return rb_iterate(rb_each, result, (VALUE(*)(ANYARGS))rb_yield, Qnil);
#endif
    }
    return result;
}

#if HAVE_DBXML_QUE_UPDATE

static VALUE
xb_que_update(VALUE obj)
{
  xque *que = get_que(obj);
  if (que->que->isUpdateExpression()) {
    return Qtrue;
  }
  return Qfalse;
}

#endif

static VALUE
xb_res_manager(VALUE obj)
{
    xres *res = get_res(obj);
    return res->man;
}

static VALUE
xb_res_add(VALUE obj, VALUE a)
{
    xres *res = get_res(obj);
    XmlValue val = xb_val_xml(a);
    PROTECT(res->res->add(val));
    return a;
}

static VALUE
xb_res_size(VALUE obj)
{
    size_t size;
    xres *res = get_res(obj);
    PROTECT(size = res->res->size());
    return INT2NUM(size);
}

#if HAVE_DBXML_RES_EVAL

static VALUE
xb_res_eval(VALUE obj)
{
    XmlQueryContext::EvaluationType eval;
    xres *res = get_res(obj);
    PROTECT(eval = res->res->getEvaluationType());
    return INT2NUM(eval);
}

#endif

static VALUE
xb_res_each(VALUE obj)
{
    XmlValue xmlval;
    if (!rb_block_given_p()) {
        rb_raise(rb_eArgError, "block not supplied");
    }
    xres *res = get_res(obj);
    PROTECT(res->res->reset());
    while (res->res->next(xmlval)) {
        rb_yield(xb_xml_val(&xmlval, res->man));
    }
    return obj;
}

static VALUE
xb_con_each(VALUE obj)
{
    if (!rb_block_given_p()) {
        rb_raise(rb_eArgError, "block not supplied");
    }
    return xb_res_each(xb_con_all(0, NULL, obj));
}

static void
xb_mod_mark(xmod *mod)
{
    rb_gc_mark(mod->man);
}

static void
xb_mod_free(xmod *mod)
{
    FREE_DEBUG("xb_mod_free %x\n", mod);
    if (mod->mod) {
        delete mod->mod;
    }
    ::free(mod);
}

static VALUE
xb_int_create_mod(VALUE obj, VALUE orig)
{
    XmlModify *xmlmod;
    xmod *mod;

    xman *man = get_man(obj);
    PROTECT(xmlmod = new XmlModify(man->man->createModify()));
    VALUE res;
    if (!orig) {
        res = Data_Make_Struct(xb_cMod, xmod, (RDF)xb_mod_mark,
                               (RDF)xb_mod_free, mod);
    }
    else {
        res = orig;
        Data_Get_Struct(res, xmod, mod);
        RDATA(res)->dfree = (RDF)xb_mod_free;
    }
    mod->man = obj;
    mod->mod = xmlmod;
    return res;
}

static VALUE
xb_man_create_mod(VALUE obj)
{
    return xb_int_create_mod(obj, 0);
}

static VALUE
xb_mod_s_alloc(VALUE obj)
{
    xmod *mod;
    return Data_Make_Struct(obj, xmod, (RDF)xb_mod_mark, (RDF)free, mod);
}

static VALUE
xb_mod_init(VALUE obj, VALUE a)
{
    return xb_int_create_mod(a, obj);
}

static VALUE
xb_mod_manager(VALUE obj)
{
    xmod *mod = get_mod(obj);
    return mod->man;
}

static VALUE
xb_mod_txn_p(VALUE obj)
{
    xmod *mod = get_mod(obj);
    if (RTEST(mod->txn)) {
        return Qtrue;
    }
    return Qfalse;
}

static VALUE
xb_mod_txn(VALUE obj)
{
    xmod *mod = get_mod(obj);
    if (RTEST(mod->txn)) {
        return mod->txn;
    }
    return Qnil;
}

#if HAVE_DBXML_MOD_ENCODING

static VALUE
xb_mod_encoding(VALUE obj, VALUE a)
{
    char *encoding = StringValuePtr(a);
    xmod *mod = get_mod(obj);
    PROTECT(mod->mod->setNewEncoding(encoding));
    return a;
}

#endif
    
static VALUE
xb_mod_append(int argc, VALUE *argv, VALUE obj)
{
    VALUE a, b, c, d, e;
#if HAVE_DBXML_MOD_APPEND_STEP_RES
    xres *res = NULL;
#endif
    char *content = 0;
    int location = -1;

    if (rb_scan_args(argc, argv, "41", &a, &b, &c, &d, &e) == 5) {
        location = NUM2INT(e);
    }
    xque *que = get_que(a);
    XmlModify::XmlObject type = XmlModify::XmlObject(NUM2INT(b));
    char *name = StringValuePtr(c);
#if HAVE_DBXML_MOD_APPEND_STEP_RES
    if (TYPE(d) == T_DATA && RDATA(obj)->dfree == (RDF)xb_res_free) {
	res = get_res(d);
    }
    else
#endif
    {
	content = StringValuePtr(d);
    }
    xmod *mod = get_mod(obj);
#if HAVE_DBXML_MOD_APPEND_STEP_RES
    if (res != NULL) {
	PROTECT(mod->mod->addAppendStep(*que->que, type, name, *res->res, location));
    }
    else
#endif
    {
	PROTECT(mod->mod->addAppendStep(*que->que, type, name, content, location));
    }
    return obj;
}

static VALUE
xb_mod_insert_after(VALUE obj, VALUE a, VALUE b, VALUE c, VALUE d)
{
#if HAVE_DBXML_MOD_APPEND_STEP_RES
    xres *res = NULL;
#endif
    char *content;
    xmod *mod = get_mod(obj);
    xque *que = get_que(a);
    XmlModify::XmlObject type = XmlModify::XmlObject(NUM2INT(b));
    char *name = StringValuePtr(c);
#if HAVE_DBXML_MOD_APPEND_STEP_RES
    if (TYPE(d) == T_DATA && RDATA(obj)->dfree == (RDF)xb_res_free) {
	res = get_res(d);
    }
    else
#endif
    {
	content = StringValuePtr(d);
    }
#if HAVE_DBXML_MOD_APPEND_STEP_RES
    if (res != NULL) {
	PROTECT(mod->mod->addInsertAfterStep(*que->que, type, name, *res->res));
    }
    else
#endif
    {
	PROTECT(mod->mod->addInsertAfterStep(*que->que, type, name, content));
    }
    return obj;
}

static VALUE
xb_mod_insert_before(VALUE obj, VALUE a, VALUE b, VALUE c, VALUE d)
{
#if HAVE_DBXML_MOD_APPEND_STEP_RES
    xres *res = NULL;
#endif
    char *content;
    xmod *mod = get_mod(obj);
    xque *que = get_que(a);
    XmlModify::XmlObject type = XmlModify::XmlObject(NUM2INT(b));
    char *name = StringValuePtr(c);
#if HAVE_DBXML_MOD_APPEND_STEP_RES
    if (TYPE(d) == T_DATA && RDATA(obj)->dfree == (RDF)xb_res_free) {
	res = get_res(d);
    }
    else
#endif
    {
	content = StringValuePtr(d);
    }
#if HAVE_DBXML_MOD_APPEND_STEP_RES
    if (res != NULL) {
	PROTECT(mod->mod->addInsertBeforeStep(*que->que, type, name, *res->res));
    }
    else
#endif
    {
	PROTECT(mod->mod->addInsertBeforeStep(*que->que, type, name, content));
    }
    return obj;
}

static VALUE
xb_mod_remove(VALUE obj, VALUE a)
{
    xmod *mod = get_mod(obj);
    xque *que = get_que(a);
    PROTECT(mod->mod->addRemoveStep(*que->que));
    return obj;
}

static VALUE
xb_mod_rename(VALUE obj, VALUE a, VALUE b)
{
    xmod *mod = get_mod(obj);
    xque *que = get_que(a);
    char *name = StringValuePtr(b);
    PROTECT(mod->mod->addRenameStep(*que->que, name));
    return obj;
}

static VALUE
xb_mod_update(VALUE obj, VALUE a, VALUE b)
{
    xmod *mod = get_mod(obj);
    xque *que = get_que(a);
    char *name = StringValuePtr(b);
    PROTECT(mod->mod->addUpdateStep(*que->que, name));
    return obj;
}

static VALUE
xb_mod_execute(int argc, VALUE *argv, VALUE obj)
{
    xmod *mod = get_mod(obj);
    XmlValue xmlval;
    xres *res = 0;
    bool has_val = false;
    XmlUpdateContext *xmlupd = 0;
    XmlQueryContext *xmlcxt = 0;
    bool freeupd = true, freecxt = true;
    VALUE a, b, c;
    
    rb_secure(4);
    switch (rb_scan_args(argc, argv, "12", &a, &b, &c)) {
    case 3:
    {
        xupd *upd = get_upd(c);
        xmlupd = upd->upd;
        freeupd = false;
    }
        /* ... */
    case 2:
    {
        xcxt *cxt = get_cxt(b);
        xmlcxt = cxt->cxt;
        freecxt = false;
    }
    }
    if (TYPE(a) == T_DATA && RDATA(a)->dfree == (RDF)xb_res_free) {
        res = get_res(a);
    }
    else {
        xmlval = xb_val_xml(a);
        has_val = true;
    }
    XmlTransaction *xmltxn = get_txn(mod->txn);
    if (freeupd) {
        xman *man = get_man(mod->man);
        xmlupd = new XmlUpdateContext(man->man->createUpdateContext());
    }
    if (freecxt) {
        xman *man = get_man(mod->man);
        xmlcxt = new XmlQueryContext(man->man->createQueryContext());
    }
    if (has_val) {
        if (xmltxn) {
            PROTECT2(mod->mod->execute(*xmltxn, xmlval, *xmlcxt, *xmlupd),
                     if (freeupd) delete xmlupd; if (freecxt) delete xmlcxt);
        }
        else {
            PROTECT2(mod->mod->execute(xmlval, *xmlcxt, *xmlupd),
                     if (freeupd) delete xmlupd; if (freecxt) delete xmlcxt);
        }
    }
    else {
        if (xmltxn) {
            PROTECT2(mod->mod->execute(*xmltxn, *res->res, *xmlcxt, *xmlupd),
                     if (freeupd) delete xmlupd; if (freecxt) delete xmlcxt);
        }
        else {
            PROTECT2(mod->mod->execute(*res->res, *xmlcxt, *xmlupd),
                     if (freeupd) delete xmlupd; if (freecxt) delete xmlcxt);
        }
    }
    return obj;
}

static VALUE
xb_val_s_alloc(VALUE obj)
{
    xval *val;

    VALUE res = Data_Make_Struct(xb_cVal, xval, (RDF)xb_val_mark, 
                                 (RDF)xb_val_free, val);
    val->val = new XmlValue();
    return res;
}

static VALUE
xb_val_init(int argc, VALUE *argv, VALUE obj)
{
    xval *val;
    VALUE a, b;

    Data_Get_Struct(obj, xval, val);
    if (rb_scan_args(argc, argv, "11", &a, &b) == 2) {
      XmlValue::Type type = XmlValue::Type(NUM2INT(a));
      b = rb_obj_as_string(b);
      char *str = StringValuePtr(b);
      delete val->val;
      val->val = new XmlValue(type, str);
      return obj;
    }
    if (TYPE(a) == T_DATA && RDATA(obj)->dmark != (RDF)xb_doc_mark) {
      xdoc *doc = get_doc(a);
      delete val->val;
      val->val = new XmlValue(*doc->doc);
    }
    else if (a == Qtrue || a == Qfalse) {
      delete val->val;
      val->val = new XmlValue(a == Qtrue);
    }
    else if (FIXNUM_P(a) || TYPE(a) == T_FLOAT || TYPE(a) == T_BIGNUM) {
      a = rb_funcall2(a, rb_intern("to_f"), 0, 0);
      delete val->val;
      val->val = new XmlValue(RFLOAT(a)->value);
    }
    else {
      a = rb_obj_as_string(a);
      char *str = StringValuePtr(b);
      delete val->val;
      val->val = new XmlValue(str);
    }
    return obj;
}

static VALUE
xb_val_to_doc(VALUE obj)
{
    xval *val;
    xdoc *doc;

    Data_Get_Struct(obj, xval, val);
    XmlDocument *xmldoc;
    PROTECT(xmldoc = new XmlDocument(val->val->asDocument()));
    VALUE res = Data_Make_Struct(xb_cDoc, xdoc, (RDF)xb_doc_mark, 
                                 (RDF)xb_doc_free, doc);
    get_man(val->man);
    doc->doc = xmldoc;
    doc->man = val->man;
    return res;
}

static VALUE
xb_val_to_str(VALUE obj)
{
    xval *val;

    Data_Get_Struct(obj, xval, val);
    std::string str;
    PROTECT(str = val->val->asString());
    return rb_tainted_str_new2(str.c_str());
}

static VALUE
xb_val_to_f(VALUE obj)
{
    xval *val;

    Data_Get_Struct(obj, xval, val);
    double dbl;
    PROTECT(dbl = val->val->asNumber());
    return rb_float_new(dbl);
}

static VALUE
xb_val_type(VALUE obj)
{
    xval *val;

    Data_Get_Struct(obj, xval, val);
    return INT2NUM(val->val->getType());
}

static VALUE
xb_val_type_p(VALUE obj, VALUE a)
{
    xval *val;

    Data_Get_Struct(obj, xval, val);
    XmlValue::Type type = XmlValue::Type(NUM2INT(a));
    if (val->val->isType(type)) {
        return Qtrue;
    }
    return Qfalse;
}

static VALUE
xb_val_nil_p(VALUE obj)
{
    xval *val;

    Data_Get_Struct(obj, xval, val);
    if (val->val->isNull()) {
        return Qtrue;
    }
    return Qfalse;
}

static VALUE
xb_val_number_p(VALUE obj)
{
    xval *val;

    Data_Get_Struct(obj, xval, val);
    if (val->val->isNumber()) {
        return Qtrue;
    }
    return Qfalse;
}

static VALUE
xb_val_string_p(VALUE obj)
{
    xval *val;

    Data_Get_Struct(obj, xval, val);
    if (val->val->isString()) {
        return Qtrue;
    }
    return Qfalse;
}

static VALUE
xb_val_boolean_p(VALUE obj)
{
    xval *val;

    Data_Get_Struct(obj, xval, val);
    if (val->val->isBoolean()) {
        return Qtrue;
    }
    return Qfalse;
}

static VALUE
xb_val_node_p(VALUE obj)
{
    xval *val;

    Data_Get_Struct(obj, xval, val);
    if (val->val->isNode()) {
        return Qtrue;
    }
    return Qfalse;
}

static VALUE
xb_val_node_name(VALUE obj)
{
    xval *val;

    Data_Get_Struct(obj, xval, val);
    std::string str;
    PROTECT(str = val->val->getNodeName());
    return rb_tainted_str_new2(str.c_str());
}

static VALUE
xb_val_node_value(VALUE obj)
{
    xval *val;

    Data_Get_Struct(obj, xval, val);
    std::string str;
    PROTECT(str = val->val->getNodeValue());
    return rb_tainted_str_new2(str.c_str());
}

static VALUE
xb_val_node_type(VALUE obj)
{
    xval *val;

    Data_Get_Struct(obj, xval, val);
    short type;
    PROTECT(type = val->val->getNodeType());
    return INT2NUM(type);
}

static VALUE
xb_val_namespace(VALUE obj)
{
    xval *val;

    Data_Get_Struct(obj, xval, val);
    std::string str;
    PROTECT(str = val->val->getNamespaceURI());
    return rb_tainted_str_new2(str.c_str());
}

static VALUE
xb_val_prefix(VALUE obj)
{
    xval *val;

    Data_Get_Struct(obj, xval, val);
    std::string str;
    PROTECT(str = val->val->getPrefix());
    return rb_tainted_str_new2(str.c_str());
}

static VALUE
xb_val_local_name(VALUE obj)
{
    xval *val;

    Data_Get_Struct(obj, xval, val);
    std::string str;
    PROTECT(str = val->val->getLocalName());
    return rb_tainted_str_new2(str.c_str());
}

static VALUE
xb_val_parent_node(VALUE obj)
{
    xval *val, *node;
    XmlValue *xmlnode, tmp;

    Data_Get_Struct(obj, xval, val);
    PROTECT(tmp = val->val->getParentNode();
            if (!tmp) return Qnil;
            xmlnode = new XmlValue(tmp));
    VALUE res = Data_Make_Struct(CLASS_OF(obj), xval, (RDF)xb_val_mark,
                                 (RDF)free, node);
    node->val = xmlnode;
    node->man = val->man;
    return res;
}
    
static VALUE
xb_val_first_child(VALUE obj)
{
    xval *val, *node;
    XmlValue *xmlnode, tmp;

    Data_Get_Struct(obj, xval, val);
    PROTECT(tmp = val->val->getFirstChild();
            if (!tmp) return Qnil;
            xmlnode = new XmlValue(tmp));
    VALUE res = Data_Make_Struct(CLASS_OF(obj), xval, (RDF)xb_val_mark,
                                 (RDF)free, node);
    node->val = xmlnode;
    node->man = val->man;
    return res;
}

static VALUE
xb_val_last_child(VALUE obj)
{
    xval *val, *node;
    XmlValue *xmlnode, tmp;

    Data_Get_Struct(obj, xval, val);
    PROTECT(tmp = val->val->getLastChild();
            if (!tmp) return Qnil;
            xmlnode = new XmlValue(tmp));
    VALUE res = Data_Make_Struct(CLASS_OF(obj), xval, (RDF)xb_val_mark,
                                 (RDF)free, node);
    node->val = xmlnode;
    node->man = val->man;
    return res;
}
    
static VALUE
xb_val_previous_sibling(VALUE obj)
{
    xval *val, *node;
    XmlValue *xmlnode, tmp;

    Data_Get_Struct(obj, xval, val);
    PROTECT(tmp = val->val->getPreviousSibling();
            if (!tmp) return Qnil;
            xmlnode = new XmlValue(tmp));
    VALUE res = Data_Make_Struct(CLASS_OF(obj), xval, (RDF)xb_val_mark,
                                 (RDF)free, node);
    node->val = xmlnode;
    node->man = val->man;
    return res;
}
    
static VALUE
xb_val_next_sibling(VALUE obj)
{
    xval *val, *node;
    XmlValue *xmlnode, tmp;

    Data_Get_Struct(obj, xval, val);
    PROTECT(tmp = val->val->getNextSibling();
            if (!tmp) return Qnil;
            xmlnode = new XmlValue(tmp));
    VALUE res = Data_Make_Struct(CLASS_OF(obj), xval, (RDF)xb_val_mark,
                                 (RDF)free, node);
    node->val = xmlnode;
    node->man = val->man;
    return res;
}

static VALUE
xb_val_owner_element(VALUE obj)
{
    xval *val, *node;
    XmlValue *xmlnode;

    Data_Get_Struct(obj, xval, val);
    PROTECT(xmlnode = new XmlValue(val->val->getOwnerElement()));
    VALUE res = Data_Make_Struct(CLASS_OF(obj), xval, (RDF)xb_val_mark,
                                 (RDF)free, node);
    node->val = xmlnode;
    node->man = val->man;
    return res;
}

static VALUE
xb_val_attributes(VALUE obj)
{
    xval *val;
    xres *res;
    XmlResults *xmlres;

    Data_Get_Struct(obj, xval, val);
    PROTECT(xmlres = new XmlResults(val->val->getAttributes()));
    VALUE result = Data_Make_Struct(xb_cRes, xres, (RDF)xb_res_mark,
                                    (RDF)xb_res_free, res);
    res->res = xmlres;
    res->man = val->man;
    return result;
}

#if HAVE_DBXML_VAL_TYPE_URI

static VALUE
xb_val_type_uri(VALUE obj)
{
    xval *val;

    Data_Get_Struct(obj, xval, val);
    std::string str;
    PROTECT(str = val->val->getTypeURI());
    return rb_tainted_str_new2(str.c_str());
}

#endif

#if HAVE_DBXML_VAL_TYPE_NAME

static VALUE
xb_val_type_name(VALUE obj)
{
    xval *val;

    Data_Get_Struct(obj, xval, val);
    std::string str;
    PROTECT(str = val->val->getTypeName());
    return rb_tainted_str_new2(str.c_str());
}

#endif

#if HAVE_DBXML_VAL_NODE_HANDLE

static VALUE
xb_val_node_handle(VALUE obj)
{
    xval *val;

    Data_Get_Struct(obj, xval, val);
    std::string str;
    PROTECT(str = val->val->getNodeHandle());
    return rb_tainted_str_new2(str.c_str());
}

#endif

#if HAVE_DBXML_XML_EVENT_WRITER

#if HAVE_DBXML_XML_EVENT_WRITER_ALLOC

static VALUE
xb_ewr_s_alloc(VALUE obj)
{
    xewr *ewr;

    VALUE res = Data_Make_Struct(xb_cEwr, xewr, (RDF)xb_ewr_mark, 
                                 (RDF)xb_ewr_free, ewr);
    ewr->ewr = new XmlEventWriter();
    return res;
}

static VALUE
xb_ewr_init(VALUE obj)
{
    return obj;
}

#endif

static VALUE
xb_ewr_close(VALUE obj)
{
    xewr *ewr = get_ewr(obj);
    PROTECT(ewr->ewr->close());
    ewr->ewr = 0;
    return Qnil;
}

static VALUE
xb_ewr_attribute(VALUE obj, VALUE a, VALUE b, VALUE c, VALUE d, VALUE e)
{
    xewr *ewr = get_ewr(obj);
    unsigned char *prefix = NULL;
    unsigned char *uri = NULL;
    unsigned char *localName = (unsigned char *)StringValuePtr(a);
    if (!NIL_P(b)) {
	prefix = (unsigned char *)StringValuePtr(b);
    }
    if (!NIL_P(c)) {
	uri = (unsigned char *)StringValuePtr(c);
    }
    unsigned char *value =  (unsigned char *)StringValuePtr(d);   
    PROTECT(ewr->ewr->writeAttribute(localName, prefix, uri, value, RTEST(e)));
    return obj;
}

static VALUE
xb_ewr_dtd(VALUE obj, VALUE a)
{
    xewr *ewr = get_ewr(obj);
    unsigned char *dtd = (unsigned char *)StringValuePtr(a);
    PROTECT(ewr->ewr->writeDTD(dtd, strlen((char *)dtd)));
    return obj;
}

static VALUE
xb_ewr_end_doc(VALUE obj)
{
    xewr *ewr = get_ewr(obj);
    PROTECT(ewr->ewr->writeEndDocument());
    return obj;
}

static VALUE
xb_ewr_end_ele(int argc, VALUE *argv, VALUE obj)
{
    xewr *ewr = get_ewr(obj);
    unsigned char *prefix = NULL;
    unsigned char *uri = NULL;
    unsigned char *localName = NULL;
    VALUE a, b, c;
    switch (rb_scan_args(argc, argv, "12", &a, &b, &c)) {
    case 3:
	if (!NIL_P(c)) {
	    uri = (unsigned char *)StringValuePtr(c);
	}
	/* ... */
    case 2:
	if (!NIL_P(b)) {
	    prefix = (unsigned char *)StringValuePtr(b);
	}
    }
    localName = (unsigned char *)StringValuePtr(a);
    PROTECT(ewr->ewr->writeEndElement(localName, prefix, uri));
    return obj;
}

static VALUE
xb_ewr_end_ent(VALUE obj, VALUE a)
{
    xewr *ewr = get_ewr(obj);
    unsigned char *name = (unsigned char *)StringValuePtr(a);
    PROTECT(ewr->ewr->writeEndEntity(name));
    return obj;
}

static VALUE
xb_ewr_pi(VALUE obj, VALUE a, VALUE b)
{
    xewr *ewr = get_ewr(obj);
    unsigned char *target = (unsigned char *)StringValuePtr(a);
    unsigned char *data = (unsigned char *)StringValuePtr(b);
    PROTECT(ewr->ewr->writeProcessingInstruction(target, data));
    return obj;
}

static VALUE
xb_ewr_start_ele(VALUE obj, VALUE a, VALUE b, VALUE c, VALUE d, VALUE e)
{
    xewr *ewr = get_ewr(obj);
    unsigned char *prefix = NULL;
    unsigned char *uri = NULL;
    unsigned char *localName = (unsigned char *)StringValuePtr(a);
    if (!NIL_P(b)) {
	prefix = (unsigned char *)StringValuePtr(b);
    }
    if (!NIL_P(c)) {
	uri = (unsigned char *)StringValuePtr(c);
    }
    int numAttributes = NUM2INT(d);   
    PROTECT(ewr->ewr->writeStartElement(localName, prefix, uri, numAttributes, RTEST(e)));
    return obj;
}

static VALUE
xb_ewr_start_ent(VALUE obj, VALUE a, VALUE b)
{
    xewr *ewr = get_ewr(obj);
    unsigned char *name = (unsigned char *)StringValuePtr(a);
    PROTECT(ewr->ewr->writeStartEntity(name, RTEST(b)));
    return obj;
}

static VALUE
xb_ewr_start_doc(int argc, VALUE *argv, VALUE obj)
{
    xewr *ewr = get_ewr(obj);
    unsigned char *version = NULL;
    unsigned char *encoding = NULL;
    unsigned char *standalone = NULL;
    VALUE a, b, c;
    switch (rb_scan_args(argc, argv, "03", &a, &b, &c)) {
    case 3:
	if (!NIL_P(c)) {
	    standalone = (unsigned char *)StringValuePtr(c);
	}
        /* ... */
    case 2:
	if (!NIL_P(b)) {
	    encoding = (unsigned char *)StringValuePtr(b);
	}
	/* ... */
    case 1:
	if (!NIL_P(a)) {
	    version = (unsigned char *)StringValuePtr(a);
	}
    }
    PROTECT(ewr->ewr->writeStartDocument(version, encoding, standalone));
    return obj;
}

static VALUE
xb_ewr_txt(VALUE obj, VALUE a, VALUE b)
{
    xewr *ewr = get_ewr(obj);
    XmlEventReader::XmlEventType type = XmlEventReader::XmlEventType(NUM2INT(a));
    unsigned char *txt = (unsigned char *)StringValuePtr(b);
    PROTECT(ewr->ewr->writeText(type, txt, strlen((char *)txt)));
    return obj;
}

#endif

#if HAVE_DBXML_XML_EVENT_READER

static void
xb_erd_mark(xerd *erd)
{
    rb_gc_mark(erd->doc);
}

static void
xb_erd_free(xerd *erd)
{
    if (erd->erd) {
	PROTECT(erd->erd->close());
	erd->erd = 0;
    }
    ::free(erd);
}

#if HAVE_DBXML_XML_EVENT_READER_ALLOC

static VALUE
xb_erd_s_alloc(VALUE obj)
{
    xerd *erd;

    VALUE res = Data_Make_Struct(xb_cErd, xerd, (RDF)xb_erd_mark, 
                                 (RDF)xb_erd_free, erd);
    erd->erd = new XmlEventReader();
    return res;
}

static VALUE
xb_erd_init(VALUE obj)
{
    return obj;
}

#endif

static VALUE
xb_erd_close(VALUE obj)
{
    xerd *erd = get_erd(obj);
    PROTECT(erd->erd->close());
    erd->erd = 0;
    return Qnil;
}

static VALUE
xb_doc_erd(VALUE obj)
{
    XmlEventReader *xmlerd;
    xerd *erd;
    xdoc *doc = get_doc(obj);
    PROTECT(XmlEventReader &rex = doc->doc->getContentAsEventReader();
	    xmlerd = (XmlEventReader *) &rex);
    VALUE res = Data_Make_Struct(xb_cErd, xerd, (RDF)xb_erd_mark, 
				 (RDF)xb_erd_free, erd);
    erd->erd = xmlerd;
    erd->doc = obj;
    return res;
}

static VALUE
xb_erd_ev(VALUE obj)
{
    xerd *erd = get_erd(obj);
    int xmlt;
    PROTECT(xmlt = erd->erd->getEventType());
    return INT2NUM(xmlt);
}

static VALUE
xb_erd_uri(VALUE obj)
{
    xerd *erd = get_erd(obj);
    const unsigned char *uri;
    PROTECT(uri = erd->erd->getNamespaceURI());
    return rb_tainted_str_new2((char *)uri);
}

static VALUE
xb_erd_name(VALUE obj)
{
    xerd *erd = get_erd(obj);
    const unsigned char *uri;
    PROTECT(uri = erd->erd->getLocalName());
    return rb_tainted_str_new2((char *)uri);
}

static VALUE
xb_erd_prefix(VALUE obj)
{
    xerd *erd = get_erd(obj);
    const unsigned char *uri;
    PROTECT(uri = erd->erd->getPrefix());
    return rb_tainted_str_new2((char *)uri);
}

static VALUE
xb_erd_value(VALUE obj)
{
    xerd *erd = get_erd(obj);
    const unsigned char *uri;
    size_t len;
    PROTECT(uri = erd->erd->getValue(len));
    return rb_tainted_str_new((char *)uri, len);
}

static VALUE
xb_erd_attr_count(VALUE obj)
{
    xerd *erd = get_erd(obj);
    int nb;
    PROTECT(nb = erd->erd->getAttributeCount());
    return INT2NUM(nb);
}

static VALUE
xb_erd_attr_p(VALUE obj, VALUE a)
{
    xerd *erd = get_erd(obj);
    int nb = NUM2INT(a);
    bool res;
    PROTECT(res = erd->erd->isAttributeSpecified(nb));
    if (res) return Qtrue;
    return Qfalse;
}

static VALUE
xb_erd_attr_uri(VALUE obj, VALUE a)
{
    xerd *erd = get_erd(obj);
    int nb = NUM2INT(a);
    const unsigned char *uri;
    PROTECT(uri = erd->erd->getAttributeNamespaceURI(nb));
    return rb_tainted_str_new2((char *)uri);
}

static VALUE
xb_erd_attr_name(VALUE obj, VALUE a)
{
    xerd *erd = get_erd(obj);
    int nb = NUM2INT(a);
    const unsigned char *uri;
    PROTECT(uri = erd->erd->getAttributeLocalName(nb));
    return rb_tainted_str_new2((char *)uri);
}

static VALUE
xb_erd_attr_prefix(VALUE obj, VALUE a)
{
    xerd *erd = get_erd(obj);
    int nb = NUM2INT(a);
    const unsigned char *uri;
    PROTECT(uri = erd->erd->getAttributePrefix(nb));
    return rb_tainted_str_new2((char *)uri);
}

static VALUE
xb_erd_attr_value(VALUE obj, VALUE a)
{
    xerd *erd = get_erd(obj);
    int nb = NUM2INT(a);
    const unsigned char *uri;
    PROTECT(uri = erd->erd->getAttributeValue(nb));
    return rb_tainted_str_new2((char *)uri);
}

static VALUE
xb_erd_info_p(VALUE obj)
{
    xerd *erd = get_erd(obj);
    if (erd->erd->hasEmptyElementInfo()) {
	return Qtrue;
    }
    return Qfalse;
}

static VALUE
xb_erd_escape_p(VALUE obj)
{
    xerd *erd = get_erd(obj);
    if (erd->erd->hasEntityEscapeInfo()) {
	return Qtrue;
    }
    return Qfalse;
}

static VALUE
xb_erd_next_p(VALUE obj)
{
    xerd *erd = get_erd(obj);
    bool res;
    PROTECT(res = erd->erd->hasNext());
    if (res) {
	return Qtrue;
    }
    return Qfalse;
}

static VALUE
xb_erd_empty_p(VALUE obj)
{
    xerd *erd = get_erd(obj);
    bool res;
    PROTECT(res = erd->erd->isEmptyElement());
    if (res) {
	return Qtrue;
    }
    return Qfalse;
}

static VALUE
xb_erd_white_p(VALUE obj)
{
    xerd *erd = get_erd(obj);
    bool res;
    PROTECT(res = erd->erd->isWhiteSpace());
    if (res) {
	return Qtrue;
    }
    return Qfalse;
}

static VALUE
xb_erd_nescape_p(int argc, VALUE *argv, VALUE obj)
{
    VALUE a;
    int nb = 0;
    bool res;
    xerd *erd = get_erd(obj);
    if (rb_scan_args(argc, argv, "01", &a) == 1) {
	nb = NUM2INT(a);
    }
    PROTECT(res = erd->erd->needsEntityEscape(nb));
    if (res) {
	return Qtrue;
    }
    return Qfalse;
}

static VALUE
xb_erd_next(VALUE obj)
{
    int res;
    xerd *erd = get_erd(obj);
    PROTECT(res = erd->erd->next());
    return INT2NUM(res);
}

static VALUE
xb_erd_next_tag(VALUE obj)
{
    int res;
    xerd *erd = get_erd(obj);
    PROTECT(res = erd->erd->nextTag());
    return INT2NUM(res);
}

static VALUE
xb_erd_get_expand(VALUE obj)
{
    xerd *erd = get_erd(obj);
    if (erd->erd->getExpandEntities()) {
	return Qtrue;
    }
    return Qfalse;
}

static VALUE
xb_erd_set_expand(VALUE obj, VALUE a)
{
    xerd *erd = get_erd(obj);
    if (RTEST(a)) {
	erd->erd->setExpandEntities(true);
    }
    else {
	erd->erd->setExpandEntities(false);
    }
    return a;
}

static VALUE
xb_erd_get_info(VALUE obj)
{
    xerd *erd = get_erd(obj);
    if (erd->erd->getReportEntityInfo()) {
	return Qtrue;
    }
    return Qfalse;
}

static VALUE
xb_erd_set_info(VALUE obj, VALUE a)
{
    xerd *erd = get_erd(obj);
    if (RTEST(a)) {
	erd->erd->setReportEntityInfo(true);
    }
    else {
	erd->erd->setReportEntityInfo(false);
    }
    return a;
}

static VALUE
xb_erd_encoding(VALUE obj)
{
    xerd *erd = get_erd(obj);
    const unsigned char *uri;
    PROTECT(uri = erd->erd->getEncoding());
    return rb_tainted_str_new2((char *)uri);
}

static VALUE
xb_erd_encoding_p(VALUE obj)
{
    xerd *erd = get_erd(obj);
    bool res;
    PROTECT(res = erd->erd->encodingSet());
    if (res) return Qtrue;
    return Qfalse;
}

static VALUE
xb_erd_version(VALUE obj)
{
    xerd *erd = get_erd(obj);
    const unsigned char *uri;
    PROTECT(uri = erd->erd->getVersion());
    return rb_tainted_str_new2((char *)uri);
}

static VALUE
xb_erd_system(VALUE obj)
{
    xerd *erd = get_erd(obj);
    const unsigned char *uri;
    PROTECT(uri = erd->erd->getSystemId());
    return rb_tainted_str_new2((char *)uri);
}

static VALUE
xb_erd_doc_stand_p(VALUE obj)
{
    xerd *erd = get_erd(obj);
    bool res;
    PROTECT(res = erd->erd->isStandalone());
    if (res) return Qtrue;
    return Qfalse;
}

static VALUE
xb_erd_attr_stand_p(VALUE obj)
{
    xerd *erd = get_erd(obj);
    bool res;
    PROTECT(res = erd->erd->standaloneSet());
    if (res) return Qtrue;
    return Qfalse;
}

static VALUE
xb_val_to_erd(VALUE obj)
{
    xval *val;
    xerd *erd;

    Data_Get_Struct(obj, xval, val);
    XmlEventReader *xmlerd;
    PROTECT(XmlEventReader &rex = val->val->asEventReader();
	    xmlerd = (XmlEventReader *)&rex);
    VALUE res = Data_Make_Struct(xb_cErd, xerd, (RDF)xb_erd_mark, 
                                 (RDF)xb_erd_free, erd);
    erd->erd = xmlerd;
    erd->doc = obj;
    return res;
}

#endif

static VALUE
xb_man_resolver(VALUE obj, VALUE a)
{
    xman *man = get_man(obj);
    XbResolve *rsv = new XbResolve(obj, a);
    PROTECT(man->man->registerResolver(*rsv));
    man->rsv = a;
    return obj;
}

extern "C" {
  
    static VALUE
    xb_con_txn_dup(VALUE obj, VALUE a)
    {
	xcon *xres;
	VALUE res;
	bdb_TXN *txnst;

	GetTxnDBErr(a, txnst, xb_eFatal);
	xcon * con = get_con(obj);
        res = Data_Make_Struct(CLASS_OF(obj), xcon, (RDF)xb_con_mark,
                               (RDF)free, xres);
        xres->con = con->con;
        xres->opened = con->opened;
        xres->txn = a;
        xres->man = txnst->man;
	return res;
    }

    static VALUE
    xb_con_txn_close(VALUE obj, VALUE commit, VALUE real)
    {
        xcon *con;

        Data_Get_Struct(obj, xcon, con);
        con->man = con->txn = Qfalse;
        con->con = 0;
        con->opened = 0;
        delete_ind(con->ind);
#if HAVE_DBXML_XML_INDEX_LOOKUP
        delete_look(con->look);
#endif
	return Qnil;
    }

    static VALUE
    xb_que_txn_dup(VALUE obj, VALUE a)
    {
        xque *que, *xres;
        VALUE res;
	bdb_TXN *txnst;

	GetTxnDBErr(a, txnst, xb_eFatal);
	Data_Get_Struct(obj, xque, que);
        res = Data_Make_Struct(CLASS_OF(obj), xque, (RDF)xb_que_mark,
                               (RDF)free, xres);
        xres->que = que->que;
        xres->txn = a;
        xres->man = txnst->man;
        return res;
    }

    static VALUE
    xb_que_txn_close(VALUE obj, VALUE commit, VALUE real)
    {
        rset_obj(obj);
	return Qnil;
    }

    static VALUE
    xb_mod_txn_dup(VALUE obj, VALUE a)
    {
        xmod *mod, *xres;
        VALUE res;
	bdb_TXN *txnst;

	GetTxnDBErr(a, txnst, xb_eFatal);
	Data_Get_Struct(obj, xmod, mod);
        res = Data_Make_Struct(CLASS_OF(obj), xmod, (RDF)xb_mod_mark,
                               (RDF)free, xres);
        xres->mod = mod->mod;
        xres->txn = a;
        xres->man = txnst->man;
        return res;
    }

    static VALUE
    xb_mod_txn_close(VALUE obj, VALUE commit, VALUE real)
    {
        rset_obj(obj);
	return Qnil;
    }

    static void xb_const_set(VALUE hash, const char *cvalue, const char *ckey)
    {
	VALUE key, value;

	key = rb_str_new2(ckey);
	rb_obj_freeze(key);
	value = rb_str_new2(cvalue);
	rb_obj_freeze(value);
	rb_hash_aset(hash, key, value);
    }

    void Init_bdbxml()
    {
	int major, minor, patch;
	VALUE version;
#ifdef BDB_LINK_OBJ
	extern void Init_bdb();
#endif

	static VALUE xb_mDb,  xb_mXML;

	if (rb_const_defined_at(rb_cObject, rb_intern("BDB"))) {
	    rb_raise(rb_eNameError, "module already defined");
	}
#ifdef BDB_LINK_OBJ
	Init_bdb();
#else
	rb_require("bdb");
#endif
	id_current_env = rb_intern("bdb_current_env");
        id_read = rb_intern("read");
        id_write = rb_intern("write");
        id_pos = rb_intern("pos");
        id_close = rb_intern("close");

	xb_mDb = rb_const_get(rb_cObject, rb_intern("BDB"));
	major = NUM2INT(rb_const_get(xb_mDb, rb_intern("VERSION_MAJOR")));
	minor = NUM2INT(rb_const_get(xb_mDb, rb_intern("VERSION_MINOR")));
	patch = NUM2INT(rb_const_get(xb_mDb, rb_intern("VERSION_PATCH")));
	if (major != DB_VERSION_MAJOR || minor != DB_VERSION_MINOR
	    || patch != DB_VERSION_PATCH) {
	    rb_raise(rb_eNotImpError, "\nBDB::XML needs compatible versions of BDB\n\tyou have BDB::XML version %d.%d.%d and BDB version %d.%d.%d\n",
		     DB_VERSION_MAJOR, DB_VERSION_MINOR, DB_VERSION_PATCH,
		     major, minor, patch);
	}
	version = rb_tainted_str_new2(dbxml_version(&major, &minor, &patch));
	if (major != DBXML_VERSION_MAJOR || minor != DBXML_VERSION_MINOR
	    || patch != DBXML_VERSION_PATCH) {
	    rb_raise(rb_eNotImpError, "\nBDB::XML needs compatible versions of DbXml\n\tyou have DbXml.hpp version %d.%d.%d and libdbxml version %d.%d.%d\n",
		     DB_VERSION_MAJOR, DB_VERSION_MINOR, DB_VERSION_PATCH,
		     major, minor, patch);
	}

        xb_mObs = rb_const_get(rb_cObject, rb_intern("ObjectSpace"));
	xb_eFatal = rb_const_get(xb_mDb, rb_intern("Fatal"));
	xb_cEnv = rb_const_get(xb_mDb, rb_intern("Env"));
#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
	rb_define_alloc_func(xb_cEnv, RMFS(xb_env_s_alloc));
#else
	rb_define_singleton_method(xb_cEnv, "allocate", RMFS(xb_env_s_alloc), 0);
#endif
        rb_define_singleton_method(xb_cEnv, "new", RMF(xb_env_s_new), -1);
        rb_define_singleton_method(xb_cEnv, "create", RMF(xb_env_s_new), -1);
	rb_define_private_method(xb_cEnv, "initialize", RMF(xb_env_init), -1);
        rb_define_method(xb_cEnv, "close", RMF(xb_env_close), 0);
        rb_define_method(xb_cEnv, "manager", RMF(xb_env_manager), -1);
	rb_define_method(xb_cEnv, "begin", RMF(xb_env_begin), -1);
	rb_define_method(xb_cEnv, "txn_begin", RMF(xb_env_begin), -1);
	rb_define_method(xb_cEnv, "transaction", RMF(xb_env_begin), -1);

	xb_cTxn = rb_const_get(xb_mDb, rb_intern("Txn"));
	rb_define_method(xb_cTxn, "begin", RMF(xb_man_begin), -1);
	rb_define_method(xb_cTxn, "txn_begin", RMF(xb_man_begin), -1);
	rb_define_method(xb_cTxn, "transaction", RMF(xb_man_begin), -1);
	rb_define_method(xb_cTxn, "create_container", RMF(xb_man_create_con), -1);
	rb_define_method(xb_cTxn, "open_container", RMF(xb_man_open_con), -1);
	rb_define_method(xb_cTxn, "rename_container", RMF(xb_man_rename), 2);
	rb_define_method(xb_cTxn, "remove_container", RMF(xb_man_remove), 1);
        rb_define_method(xb_cTxn, "prepare", RMF(xb_man_prepare), -1);
        rb_define_method(xb_cTxn, "query", RMF(xb_man_query), -1);
#if HAVE_DBXML_XML_INDEX_LOOKUP
        rb_define_method(xb_cTxn, "create_index_lookup", RMF(xb_man_create_look), -1);
#endif
#if HAVE_DBXML_MAN_EXISTS_CONTAINER
        rb_define_method(xb_cTxn, "container_version", RMF(xb_man_con_version), 1);
#endif
#if HAVE_DBXML_MAN_REINDEX_CONTAINER
	rb_define_method(xb_cTxn, "reindex_container", RMF(xb_man_reindex), -1);
#endif
#if HAVE_DBXML_MAN_COMPACT_CONTAINER
	rb_define_method(xb_cTxn, "compact_container", RMF(xb_man_compact_con), -1);
#endif
#if HAVE_DBXML_MAN_TRUNCATE_CONTAINER
	rb_define_method(xb_cTxn, "truncate_container", RMF(xb_man_truncate_con), -1);
#endif
        rb_define_method(xb_cTxn, "method_missing", RMF(xb_txn_missing), -1);

	DbXml::setLogLevel(DbXml::LEVEL_ALL, false);
        DbXml::setLogCategory(DbXml::CATEGORY_ALL, false);

        xb_io = rb_ary_new();
        rb_global_variable(&xb_io);

	xb_mXML = rb_define_module_under(xb_mDb, "XML");
	rb_define_const(xb_mXML, "VERSION", version);
	rb_define_const(xb_mXML, "VERSION_MAJOR", INT2FIX(major));
	rb_define_const(xb_mXML, "VERSION_MINOR", INT2FIX(minor));
	rb_define_const(xb_mXML, "VERSION_PATCH", INT2FIX(patch));
#if HAVE_DBXML_CONST_DBXML_ADOPT_DBENV
        rb_define_const(xb_mXML, "ADOPT_DBENV", INT2NUM(DBXML_ADOPT_DBENV));
#endif
#if HAVE_DBXML_CONST_DBXML_ALLOW_EXTERNAL_ACCESS
        rb_define_const(xb_mXML, "ALLOW_EXTERNAL_ACCESS", INT2NUM(DBXML_ALLOW_EXTERNAL_ACCESS));
#endif
#if HAVE_DBXML_CONST_DBXML_ALLOW_AUTO_OPEN
        rb_define_const(xb_mXML, "ALLOW_AUTO_OPEN", INT2NUM(DBXML_ALLOW_AUTO_OPEN));
#endif
#if HAVE_DBXML_CONST_DBXML_ALLOW_VALIDATION
        rb_define_const(xb_mXML, "ALLOW_VALIDATION", INT2NUM(DBXML_ALLOW_VALIDATION));
#endif
#if HAVE_DBXML_CONST_DBXML_TRANSACTIONAL
        rb_define_const(xb_mXML, "TRANSACTIONAL", INT2NUM(DBXML_TRANSACTIONAL));
#endif
#if HAVE_DBXML_CONST_DBXML_GEN_NAME
        rb_define_const(xb_mXML, "GEN_NAME", INT2NUM(DBXML_GEN_NAME));
#endif
#if HAVE_DBXML_CONST_DBXML_LAZY_DOCS
        rb_define_const(xb_mXML, "LAZY_DOCS", INT2NUM(DBXML_LAZY_DOCS));
#endif
#if HAVE_DBXML_CONST_DBXML_INDEX_NODES
        rb_define_const(xb_mXML, "INDEX_NODES", INT2NUM(DBXML_INDEX_NODES));
#endif
#if HAVE_DBXML_CONST_LEVEL_NONE
        rb_define_const(xb_mXML, "LEVEL_NONE", INT2NUM(LEVEL_NONE));
#endif
#if HAVE_DBXML_CONST_LEVEL_DEBUG
        rb_define_const(xb_mXML, "LEVEL_DEBUG", INT2NUM(LEVEL_DEBUG));
#endif
#if HAVE_DBXML_CONST_LEVEL_INFO
        rb_define_const(xb_mXML, "LEVEL_INFO", INT2NUM(LEVEL_INFO));
#endif
#if HAVE_DBXML_CONST_LEVEL_WARNING
        rb_define_const(xb_mXML, "LEVEL_WARNING", INT2NUM(LEVEL_WARNING));
#endif
#if HAVE_DBXML_CONST_LEVEL_ERROR
        rb_define_const(xb_mXML, "LEVEL_ERROR", INT2NUM(LEVEL_ERROR));
#endif
#if HAVE_DBXML_CONST_LEVEL_ALL
        rb_define_const(xb_mXML, "LEVEL_ALL", INT2NUM(LEVEL_ALL));
#endif
#if HAVE_DBXML_CONST_CATEGORY_NONE
        rb_define_const(xb_mXML, "CATEGORY_NONE", INT2NUM(CATEGORY_NONE));
#endif
#if HAVE_DBXML_CONST_CATEGORY_INDEXER
        rb_define_const(xb_mXML, "CATEGORY_INDEXER", INT2NUM(CATEGORY_INDEXER));
#endif
#if HAVE_DBXML_CONST_CATEGORY_QUERY
        rb_define_const(xb_mXML, "CATEGORY_QUERY", INT2NUM(CATEGORY_QUERY));
#endif
#if HAVE_DBXML_CONST_CATEGORY_OPTIMIZER
        rb_define_const(xb_mXML, "CATEGORY_OPTIMIZER", INT2NUM(CATEGORY_OPTIMIZER));
#endif
#if HAVE_DBXML_CONST_CATEGORY_DICTIONARY
        rb_define_const(xb_mXML, "CATEGORY_DICTIONARY", INT2NUM(CATEGORY_DICTIONARY));
#endif
#if HAVE_DBXML_CONST_CATEGORY_CONTAINER
        rb_define_const(xb_mXML, "CATEGORY_CONTAINER", INT2NUM(CATEGORY_CONTAINER));
#endif
#if HAVE_DBXML_CONST_CATEGORY_NODESTORE
        rb_define_const(xb_mXML, "CATEGORY_NODESTORE", INT2NUM(CATEGORY_NODESTORE));
#endif
#if HAVE_DBXML_CONST_CATEGORY_MANAGER
        rb_define_const(xb_mXML, "CATEGORY_MANAGER", INT2NUM(CATEGORY_MANAGER));
#endif
#if HAVE_DBXML_CONST_CATEGORY_ALL
        rb_define_const(xb_mXML, "CATEGORY_ALL", INT2NUM(CATEGORY_ALL));
#endif
#if HAVE_DBXML_CONST_DBXML_CHKSUM_SHA1
	rb_define_const(xb_mXML, "CHKSUM_SHA1", INT2NUM(DBXML_CHKSUM_SHA1));
#endif
#if HAVE_DBXML_CONST_DBXML_ENCRYPT
	rb_define_const(xb_mXML, "ENCRYPT", INT2NUM(DBXML_ENCRYPT));
#endif
#if HAVE_DBXML_CONST_DBXML_WELL_FORMED_ONLY
	rb_define_const(xb_mXML, "WELL_FORMED_ONLY", INT2NUM(DBXML_WELL_FORMED_ONLY));
#endif
#if HAVE_DBXML_CONST_DBXML_DOCUMENT_PROJECTION
	rb_define_const(xb_mXML, "DOCUMENT_PROJECTION", INT2NUM(DBXML_DOCUMENT_PROJECTION));
#endif
#if HAVE_DBXML_CONST_DBXML_NO_AUTO_COMMIT
	rb_define_const(xb_mXML, "NO_AUTO_COMMIT", INT2NUM(DBXML_NO_AUTO_COMMIT));
#endif
#if HAVE_DBXML_CONST_DBXML_STATISTICS
	rb_define_const(xb_mXML, "STATISTICS", INT2NUM(DBXML_STATISTICS));
#endif
#if HAVE_DBXML_CONST_DBXML_NO_STATISTICS
	rb_define_const(xb_mXML, "NO_STATISTICS", INT2NUM(DBXML_NO_STATISTICS));
#endif
#if HAVE_DBXML_CONST_DBXML_REVERSE_ORDER
	rb_define_const(xb_mXML, "REVERSE_ORDER", INT2NUM(DBXML_REVERSE_ORDER));
#endif
#if HAVE_DBXML_CONST_DBXML_CACHE_DOCUMENTS
	rb_define_const(xb_mXML, "CACHE_DOCUMENTS", INT2NUM(DBXML_CACHE_DOCUMENTS));
#endif
#if HAVE_DBXML_CONST_DBXML_INDEX_VALUES
	rb_define_const(xb_mXML, "INDEX_VALUES", INT2NUM(DBXML_INDEX_VALUES));
#endif
#if HAVE_DBXML_CONST_DBXML_NO_INDEX_NODES
	rb_define_const(xb_mXML, "NO_INDEX_NODES", INT2NUM(DBXML_NO_INDEX_NODES));
#endif

	{
	    VALUE name = rb_hash_new();
	    rb_define_const(xb_mXML, "Name", name);
	    xb_const_set(name, metaDataName_name, "name");
	    xb_const_set(name, metaDataName_root, "root");
	    xb_const_set(name, "default", "default");
	    rb_obj_freeze(name);
	    name = rb_hash_new();
	    rb_define_const(xb_mXML, "Namespace", name);
	    xb_const_set(name, metaDataNamespace_uri, "uri");
	    xb_const_set(name, metaDataNamespace_prefix, "prefix");
	    rb_obj_freeze(name);
	}

        xb_cMan = rb_define_class_under(xb_mXML, "Manager", rb_cObject);
#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
	rb_define_alloc_func(xb_cMan, RMFS(xb_man_s_alloc));
#else
	rb_define_singleton_method(xb_cMan, "allocate", RMFS(xb_man_s_alloc), 0);
#endif
	rb_define_singleton_method(xb_cMan, "new", RMF(xb_s_new), -1);
	rb_define_private_method(xb_cMan, "initialize", RMF(xb_man_init), -1);
	rb_define_method(xb_cMan, "environment", RMF(xb_man_env), 0);
	rb_define_method(xb_cMan, "env", RMF(xb_man_env), 0);
	rb_define_method(xb_cMan, "environment?", RMF(xb_man_env_p), 0);
	rb_define_method(xb_cMan, "env?", RMF(xb_man_env_p), 0);
	rb_define_method(xb_cMan, "has_environment?", RMF(xb_man_env_p), 0);
	rb_define_method(xb_cMan, "has_env?", RMF(xb_man_env_p), 0);
        rb_define_method(xb_cMan, "home", RMF(xb_man_home), 0);
        rb_define_method(xb_cMan, "pagesize", RMF(xb_man_page_get), 0);
        rb_define_method(xb_cMan, "pagesize=", RMF(xb_man_page_set), 1);
        rb_define_method(xb_cMan, "container_flags", RMF(xb_man_flags_get), 0);
        rb_define_method(xb_cMan, "container_flags=", RMF(xb_man_flags_set), 1);
        rb_define_method(xb_cMan, "container_type", RMF(xb_man_type_get), 0);
        rb_define_method(xb_cMan, "container_type=", RMF(xb_man_type_set), 1);
	rb_define_method(xb_cMan, "begin", RMF(xb_man_begin), -1);
	rb_define_method(xb_cMan, "txn_begin", RMF(xb_man_begin), -1);
	rb_define_method(xb_cMan, "transaction", RMF(xb_man_begin), -1);
	rb_define_method(xb_cMan, "create_container", RMF(xb_man_create_con), -1);
	rb_define_method(xb_cMan, "open_container", RMF(xb_man_open_con), -1);
	rb_define_method(xb_cMan, "open", RMF(xb_man_open_con), -1);
	rb_define_method(xb_cMan, "rename_container", RMF(xb_man_rename), 2);
	rb_define_method(xb_cMan, "rename", RMF(xb_man_rename), 2);
	rb_define_method(xb_cMan, "remove_container", RMF(xb_man_remove), 1);
	rb_define_method(xb_cMan, "remove", RMF(xb_man_remove), 1);
	rb_define_method(xb_cMan, "upgrade_container", RMF(xb_man_upgrade), 1);
        rb_define_method(xb_cMan, "upgrade", RMF(xb_man_upgrade), 1);
	rb_define_method(xb_cMan, "dump_container", RMF(xb_man_dump_con), -1);
	rb_define_method(xb_cMan, "dump", RMF(xb_man_dump_con), -1);
	rb_define_method(xb_cMan, "load_container", RMF(xb_man_load_con), -1);
	rb_define_method(xb_cMan, "load", RMF(xb_man_load_con), -1);

	rb_define_method(xb_cMan, "verify_container", RMF(xb_man_verify), 1);
	rb_define_method(xb_cMan, "verify", RMF(xb_man_verify), 1);
	rb_define_method(xb_cMan, "create_update_context", RMF(xb_man_create_upd), 0);
	rb_define_method(xb_cMan, "create_query_context", RMF(xb_man_create_cxt), -1);
	rb_define_method(xb_cMan, "create_results", RMF(xb_man_create_res), 0);
	rb_define_method(xb_cMan, "create_document", RMF(xb_man_create_doc), 0)	;
        rb_define_method(xb_cMan, "create_modify", RMF(xb_man_create_mod), 0);
        rb_define_method(xb_cMan, "prepare", RMF(xb_man_prepare), -1);
        rb_define_method(xb_cMan, "query", RMF(xb_man_query), -1);
        rb_define_method(xb_cMan, "resolver=", RMF(xb_man_resolver), 1);
        rb_define_method(xb_cMan, "close", RMF(xb_man_close), 0);
#if HAVE_DBXML_XML_INDEX_LOOKUP
        rb_define_method(xb_cMan, "create_index_lookup", RMF(xb_man_create_look), -1);
#endif
#if HAVE_DBXML_MAN_EXISTS_CONTAINER
        rb_define_method(xb_cMan, "container_version", RMF(xb_man_con_version), 1);
#endif
#if HAVE_DBXML_MAN_REINDEX_CONTAINER
	rb_define_method(xb_cMan, "reindex_container", RMF(xb_man_reindex), -1);
#endif
#if HAVE_DBXML_MAN_DEFAULT_SEQUENCE_INCREMENT
        rb_define_method(xb_cMan, "sequence_incr", RMF(xb_man_increment_get), 0);
        rb_define_method(xb_cMan, "sequence_increment", RMF(xb_man_increment_get), 0);
        rb_define_method(xb_cMan, "sequence_incr=", RMF(xb_man_increment_set), 0);
        rb_define_method(xb_cMan, "sequence_increment=", RMF(xb_man_increment_set), 0);
#endif
#if HAVE_DBXML_MAN_GET_FLAGS
	rb_define_method(xb_cMan, "flags", RMF(xb_man_get_flags), 0);
#endif
#if HAVE_DBXML_MAN_GET_IMPLICIT_TIMEZONE
	rb_define_method(xb_cMan, "implicit_timezone", RMF(xb_man_get_itz), 0);
	rb_define_method(xb_cMan, "implicit_timezone=", RMF(xb_man_set_itz), 1);
#endif
#if HAVE_DBXML_MAN_COMPACT_CONTAINER
	rb_define_method(xb_cMan, "compact_container", RMF(xb_man_compact_con), -1);
#endif
#if HAVE_DBXML_MAN_TRUNCATE_CONTAINER
	rb_define_method(xb_cMan, "truncate_container", RMF(xb_man_truncate_con), -1);
#endif

	xb_cCon = rb_define_class_under(xb_mXML, "Container", rb_cObject);
#if HAVE_DBXML_CONST_XMLCONTAINER_NODECONTAINER
        rb_define_const(xb_cCon, "NodeContainer", INT2NUM(XmlContainer::NodeContainer));
#endif
#if HAVE_DBXML_CONST_XMLCONTAINER_WHOLEDOCCONTAINER
        rb_define_const(xb_cCon, "WholedocContainer", INT2NUM(XmlContainer::WholedocContainer));
#endif
#if HAVE_DBXML_CONST_XMLCONTAINER_NODECONTAINER
        rb_define_const(xb_cCon, "Node", INT2NUM(XmlContainer::NodeContainer));
#endif
#if HAVE_DBXML_CONST_XMLCONTAINER_WHOLEDOCCONTAINER
        rb_define_const(xb_cCon, "Wholedoc", INT2NUM(XmlContainer::WholedocContainer));
#endif
	rb_include_module(xb_cCon, rb_mEnumerable);
#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
	rb_define_alloc_func(xb_cCon, RMFS(xb_con_s_alloc));
#else
	rb_define_singleton_method(xb_cCon, "allocate", RMFS(xb_con_s_alloc), 0);
#endif
	rb_define_singleton_method(xb_cCon, "new", RMF(xb_s_new), -1);
	rb_define_singleton_method(xb_cCon, "open", RMF(xb_con_s_open), -1);
	rb_define_private_method(xb_cCon, "initialize", RMF(xb_con_init), -1);
	rb_define_private_method(xb_cCon, "__txn_dup__", RMF(xb_con_txn_dup), 1);
	rb_define_private_method(xb_cCon, "__txn_close__", RMF(xb_con_txn_close), 2);
	rb_define_method(xb_cCon, "each", RMF(xb_con_each), 0);
	rb_define_method(xb_cCon, "manager", RMF(xb_con_manager), 0);
	rb_define_method(xb_cCon, "close", RMF(xb_con_close), 0);
	rb_define_method(xb_cCon, "transaction", RMF(xb_con_txn), 0);
	rb_define_method(xb_cCon, "txn", RMF(xb_con_txn), 0);
	rb_define_method(xb_cCon, "transaction?", RMF(xb_con_txn_p), 0);
	rb_define_method(xb_cCon, "txn?", RMF(xb_con_txn_p), 0);
	rb_define_method(xb_cCon, "in_transaction?", RMF(xb_con_txn_p), 0);
	rb_define_method(xb_cCon, "in_txn?", RMF(xb_con_txn_p), 0);
	rb_define_method(xb_cCon, "index", RMF(xb_con_index), -1);
	rb_define_method(xb_cCon, "index=", RMF(xb_con_index_set), -1);
#if HAVE_DBXML_CON_INDEX_NODES
        rb_define_method(xb_cCon, "index?", RMF(xb_con_index_p), 0);
#endif
#if HAVE_DBXML_CON_PAGESIZE
        rb_define_method(xb_cCon, "pagesize", RMF(xb_con_pagesize), 0);
#endif
	rb_define_method(xb_cCon, "name", RMF(xb_con_name), 0);
        rb_define_method(xb_cCon, "alias=", RMF(xb_con_alias_set), 1);
        rb_define_method(xb_cCon, "delete_alias", RMF(xb_con_alias_del), 1);
        rb_define_method(xb_cCon, "all_documents", RMF(xb_con_all), -1);
	rb_define_method(xb_cCon, "type", RMF(xb_con_type), 0);
	rb_define_method(xb_cCon, "[]", RMF(xb_con_get), -1);
	rb_define_method(xb_cCon, "get", RMF(xb_con_get), -1);
	rb_define_method(xb_cCon, "<<", RMF(xb_con_add), -1);
	rb_define_method(xb_cCon, "put", RMF(xb_con_add), -1);
	rb_define_method(xb_cCon, "push", RMF(xb_con_add), -1);
	rb_define_method(xb_cCon, "update", RMF(xb_con_update), -1);
	rb_define_method(xb_cCon, "[]=", RMF(xb_con_set), 2);
	rb_define_method(xb_cCon, "delete", RMF(xb_con_delete), -1);
	rb_define_method(xb_cCon, "sync", RMF(xb_con_sync), 0);
	rb_define_method(xb_cCon, "statistics", RMF(xb_con_stat), -1);
	rb_define_method(xb_cCon, "lookup_index", RMF(xb_con_lookup), -1);
	rb_define_method(xb_cCon, "add_index", RMF(xb_con_add_index), -1);
	rb_define_method(xb_cCon, "delete_index", RMF(xb_con_delete_index), -1);
	rb_define_method(xb_cCon, "replace_index", RMF(xb_con_replace_index), -1);
	rb_define_method(xb_cCon, "add_default_index", RMF(xb_con_add_def_index), -1);
	rb_define_method(xb_cCon, "delete_default_index", RMF(xb_con_delete_def_index), -1);
	rb_define_method(xb_cCon, "replace_default_index", RMF(xb_con_replace_def_index), -1);
#if HAVE_DBXML_CON_FLAGS
	rb_define_method(xb_cCon, "flags", RMF(xb_con_flags), 0);
#endif
#if HAVE_DBXML_CON_NODE
	rb_define_method(xb_cCon, "node", RMF(xb_con_node), -1);
#endif
#if HAVE_DBXML_XML_EVENT_WRITER
	rb_define_method(xb_cCon, "put_document_as_event_writer", RMF(xb_con_ewr), -1);
	rb_define_method(xb_cCon, "event_writer", RMF(xb_con_ewr), -1);
#endif

	xb_cInd = rb_define_class_under(xb_mXML, "Index", rb_cObject);
	rb_undef_method(CLASS_OF(xb_cInd), "allocate");
	rb_undef_method(CLASS_OF(xb_cInd), "new");
#if HAVE_DBXML_CONST_XMLINDEXSPECIFICATION_UNIQUE_OFF
        rb_define_const(xb_cInd, "UNIQUE_OFF", INT2NUM(XmlIndexSpecification::UNIQUE_OFF));
#endif
#if HAVE_DBXML_CONST_XMLINDEXSPECIFICATION_UNIQUE_ON
        rb_define_const(xb_cInd, "UNIQUE_ON", INT2NUM(XmlIndexSpecification::UNIQUE_ON));
#endif
#if HAVE_DBXML_CONST_XMLINDEXSPECIFICATION_PATH_NONE
        rb_define_const(xb_cInd, "PATH_NONE", INT2NUM(XmlIndexSpecification::PATH_NONE));
#endif
#if HAVE_DBXML_CONST_XMLINDEXSPECIFICATION_PATH_NODE
        rb_define_const(xb_cInd, "PATH_NODE", INT2NUM(XmlIndexSpecification::PATH_NODE));
#endif
#if HAVE_DBXML_CONST_XMLINDEXSPECIFICATION_PATH_EDGE
        rb_define_const(xb_cInd, "PATH_EDGE", INT2NUM(XmlIndexSpecification::PATH_EDGE));
#endif
#if HAVE_DBXML_CONST_XMLINDEXSPECIFICATION_NODE_NONE
        rb_define_const(xb_cInd, "NODE_NONE", INT2NUM(XmlIndexSpecification::NODE_NONE));
#endif
#if HAVE_DBXML_CONST_XMLINDEXSPECIFICATION_NODE_ELEMENT
        rb_define_const(xb_cInd, "NODE_ELEMENT", INT2NUM(XmlIndexSpecification::NODE_ELEMENT));
#endif
#if HAVE_DBXML_CONST_XMLINDEXSPECIFICATION_NODE_ATTRIBUTE
        rb_define_const(xb_cInd, "NODE_ATTRIBUTE", INT2NUM(XmlIndexSpecification::NODE_ATTRIBUTE));
#endif
#if HAVE_DBXML_CONST_XMLINDEXSPECIFICATION_NODE_METADATA
        rb_define_const(xb_cInd, "NODE_METADATA", INT2NUM(XmlIndexSpecification::NODE_METADATA));
#endif
#if HAVE_DBXML_CONST_XMLINDEXSPECIFICATION_KEY_NONE
        rb_define_const(xb_cInd, "KEY_NONE", INT2NUM(XmlIndexSpecification::KEY_NONE));
#endif
#if HAVE_DBXML_CONST_XMLINDEXSPECIFICATION_KEY_PRESENCE
        rb_define_const(xb_cInd, "KEY_PRESENCE", INT2NUM(XmlIndexSpecification::KEY_PRESENCE));
#endif
#if HAVE_DBXML_CONST_XMLINDEXSPECIFICATION_KEY_EQUALITY
        rb_define_const(xb_cInd, "KEY_EQUALITY", INT2NUM(XmlIndexSpecification::KEY_EQUALITY));
#endif
#if HAVE_DBXML_CONST_XMLINDEXSPECIFICATION_KEY_SUBSTRING
        rb_define_const(xb_cInd, "KEY_SUBSTRING", INT2NUM(XmlIndexSpecification::KEY_SUBSTRING));
#endif
	rb_define_method(xb_cInd, "manager", RMF(xb_ind_manager), 0);
	rb_define_method(xb_cInd, "container", RMF(xb_ind_container), 0);
	rb_define_method(xb_cInd, "default", RMF(xb_ind_default), 0);
	rb_define_method(xb_cInd, "add", RMF(xb_ind_add), -1);
	rb_define_method(xb_cInd, "add_default", RMF(xb_ind_add_default), -1);
	rb_define_method(xb_cInd, "delete", RMF(xb_ind_delete), -1);
	rb_define_method(xb_cInd, "delete_default", RMF(xb_ind_delete_default), -1);
	rb_define_method(xb_cInd, "replace", RMF(xb_ind_replace), -1);
	rb_define_method(xb_cInd, "replace_default", RMF(xb_ind_replace_default), -1);
	rb_define_method(xb_cInd, "each", RMF(xb_ind_each), 0);
	rb_define_method(xb_cInd, "each_type", RMF(xb_ind_each_type), 0);
	rb_define_method(xb_cInd, "find", RMF(xb_ind_find), -1);
	rb_define_method(xb_cInd, "to_a", RMF(xb_ind_to_a), 0);

#if HAVE_DBXML_XML_INDEX_LOOKUP
        xb_cLook = rb_define_class_under(xb_mXML, "IndexLookup", rb_cObject);
	rb_undef_method(CLASS_OF(xb_cLook), "allocate");
	rb_undef_method(CLASS_OF(xb_cLook), "new");
#if HAVE_DBXML_CONST_XMLINDEXLOOKUP_NONE
        rb_define_const(xb_cLook, "NONE", INT2NUM(XmlIndexLookup::NONE));
#endif
#if HAVE_DBXML_CONST_XMLINDEXLOOKUP_EQ
        rb_define_const(xb_cLook, "EQ", INT2NUM(XmlIndexLookup::EQ));
#endif
#if HAVE_DBXML_CONST_XMLINDEXLOOKUP_GT
        rb_define_const(xb_cLook, "GT", INT2NUM(XmlIndexLookup::GT));
#endif
#if HAVE_DBXML_CONST_XMLINDEXLOOKUP_GTE
        rb_define_const(xb_cLook, "GTE", INT2NUM(XmlIndexLookup::GTE));
#endif
#if HAVE_DBXML_CONST_XMLINDEXLOOKUP_LT
        rb_define_const(xb_cLook, "LT", INT2NUM(XmlIndexLookup::LT));
#endif
#if HAVE_DBXML_CONST_XMLINDEXLOOKUP_LTE
        rb_define_const(xb_cLook, "LTE", INT2NUM(XmlIndexLookup::LTE));
#endif
	rb_define_method(xb_cLook, "manager", RMF(xb_look_manager), 0);
	rb_define_method(xb_cLook, "transaction", RMF(xb_look_transaction), 0);
	rb_define_method(xb_cLook, "transaction?", RMF(xb_look_transaction_p), 0);
        rb_define_method(xb_cLook, "execute", RMF(xb_look_execute), -1);
        rb_define_method(xb_cLook, "container", RMF(xb_look_container_get), 0);
        rb_define_method(xb_cLook, "container=", RMF(xb_look_container_set), 1);
        rb_define_method(xb_cLook, "high_bound", RMF(xb_look_highbound_get), 0);
        rb_define_method(xb_cLook, "high_bound=", RMF(xb_look_highbound_set), 1);
        rb_define_method(xb_cLook, "low_bound", RMF(xb_look_lowbound_get), 0);
        rb_define_method(xb_cLook, "low_bound=", RMF(xb_look_lowbound_set), 1);
        rb_define_method(xb_cLook, "index", RMF(xb_look_index_get), 0);
        rb_define_method(xb_cLook, "index=", RMF(xb_look_index_set), 1);
        rb_define_method(xb_cLook, "node", RMF(xb_look_node_get), 0);
        rb_define_method(xb_cLook, "node_uri", RMF(xb_look_node_uri_get), 0);
        rb_define_method(xb_cLook, "node_name", RMF(xb_look_node_name_get), 0);
        rb_define_method(xb_cLook, "node=", RMF(xb_look_node_set), 1);
        rb_define_method(xb_cLook, "node_uri=", RMF(xb_look_node_uri_set), 1);
        rb_define_method(xb_cLook, "node_name=", RMF(xb_look_node_name_set), 1);
        rb_define_method(xb_cLook, "parent", RMF(xb_look_parent_get), 0);
        rb_define_method(xb_cLook, "parent_uri", RMF(xb_look_parent_uri_get), 0);
        rb_define_method(xb_cLook, "parent_name", RMF(xb_look_parent_name_get), 0);
        rb_define_method(xb_cLook, "parent=", RMF(xb_look_parent_set), 1);
        rb_define_method(xb_cLook, "parent_uri=", RMF(xb_look_parent_uri_set), 1);
        rb_define_method(xb_cLook, "parent_name=", RMF(xb_look_parent_name_set), 1);
#endif

	xb_cUpd = rb_define_class_under(xb_mXML, "UpdateContext", rb_cObject);
#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
	rb_define_alloc_func(xb_cUpd, RMFS(xb_upd_s_alloc));
#else
	rb_define_singleton_method(xb_cUpd, "allocate", RMFS(xb_upd_s_alloc), 0);
#endif
	rb_define_singleton_method(xb_cUpd, "new", RMF(xb_s_new), -1);
	rb_define_private_method(xb_cUpd, "initialize", RMF(xb_upd_init), 1);
	rb_define_method(xb_cUpd, "manager", RMF(xb_upd_manager), 0);
#if HAVE_DBXML_UPDATE_APPLY_CHANGES
        rb_define_method(xb_cUpd, "apply_changes", RMF(xb_upd_changes_get), 0);
        rb_define_method(xb_cUpd, "apply_changes=", RMF(xb_upd_changes_set), 1);
#endif
	xb_cDoc = rb_define_class_under(xb_mXML, "Document", rb_cObject);
	rb_include_module(xb_cDoc, rb_mEnumerable);
#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
	rb_define_alloc_func(xb_cDoc, RMFS(xb_doc_s_alloc));
#else
	rb_define_singleton_method(xb_cDoc, "allocate", RMFS(xb_doc_s_alloc), 0);
#endif
	rb_define_singleton_method(xb_cDoc, "new", RMF(xb_s_new), -1);
	rb_define_private_method(xb_cDoc, "initialize", RMF(xb_doc_init), 1);
	rb_define_method(xb_cDoc, "manager", RMF(xb_doc_manager), 0);
	rb_define_method(xb_cDoc, "name", RMF(xb_doc_name_get), 0);
	rb_define_method(xb_cDoc, "name=", RMF(xb_doc_name_set), 1);
	rb_define_method(xb_cDoc, "content", RMF(xb_doc_content_str), 0);
	rb_define_method(xb_cDoc, "content=", RMF(xb_doc_content_set), 1);
	rb_define_method(xb_cDoc, "[]", RMF(xb_doc_get), -1);
	rb_define_method(xb_cDoc, "[]=", RMF(xb_doc_set), -1);
	rb_define_method(xb_cDoc, "get", RMF(xb_doc_get), -1);
	rb_define_method(xb_cDoc, "set", RMF(xb_doc_set), -1);
	rb_define_method(xb_cDoc, "get_metadata", RMF(xb_doc_get), -1);
	rb_define_method(xb_cDoc, "set_metadata", RMF(xb_doc_set), -1);
	rb_define_method(xb_cDoc, "remove_metadata", RMF(xb_doc_remove), -1);
	rb_define_method(xb_cDoc, "each", RMF(xb_doc_each), 0);
	rb_define_method(xb_cDoc, "each_metadata", RMF(xb_doc_each), 0);
	rb_define_method(xb_cDoc, "fetch_alldata", RMF(xb_doc_fetch), 0);
	rb_define_method(xb_cDoc, "fetch", RMF(xb_doc_fetch), 0);
	rb_define_method(xb_cDoc, "to_s", RMF(xb_doc_content_str), 0);
	rb_define_method(xb_cDoc, "to_str", RMF(xb_doc_content_str), 0);
	rb_define_method(xb_cDoc, "release", RMF(xb_doc_close), 0);
#if HAVE_DBXML_XML_EVENT_READER
	rb_define_method(xb_cDoc, "event_reader", RMF(xb_doc_erd), 0);
	rb_define_method(xb_cDoc, "get_content_as_event_reader", RMF(xb_doc_erd), 0);
#endif

	xb_cCxt = rb_define_class_under(xb_mXML, "Context", rb_cObject);
	rb_const_set(xb_mXML, rb_intern("QueryContext"), xb_cCxt);
#if HAVE_DBXML_CONST_XMLQUERYCONTEXT_LIVEVALUES
	rb_define_const(xb_cCxt, "LiveValues", INT2NUM(XmlQueryContext::LiveValues));
#endif
#if HAVE_DBXML_CONST_XMLQUERYCONTEXT_DEADVALUES
	rb_define_const(xb_cCxt, "DeadValues", INT2NUM(XmlQueryContext::DeadValues));
#endif
#if HAVE_DBXML_CONST_XMLQUERYCONTEXT_EAGER
	rb_define_const(xb_cCxt, "Eager", INT2NUM(XmlQueryContext::Eager));
#endif
#if HAVE_DBXML_CONST_XMLQUERYCONTEXT_LAZY
	rb_define_const(xb_cCxt, "Lazy", INT2NUM(XmlQueryContext::Lazy));
#endif
#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
	rb_define_alloc_func(xb_cCxt, RMFS(xb_cxt_s_alloc));
#else
	rb_define_singleton_method(xb_cCxt, "allocate", RMFS(xb_cxt_s_alloc), 0);
#endif
	rb_define_singleton_method(xb_cCxt, "new", RMF(xb_s_new), -1);
	rb_define_private_method(xb_cCxt, "initialize", RMF(xb_cxt_init), -1);
	rb_define_method(xb_cCxt, "manager", RMF(xb_cxt_manager), 0);
	rb_define_method(xb_cCxt, "set_namespace", RMF(xb_cxt_name_set), 2);
	rb_define_method(xb_cCxt, "get_namespace", RMF(xb_cxt_name_get), 1);
	rb_define_method(xb_cCxt, "del_namespace", RMF(xb_cxt_name_del), 1);
	rb_define_method(xb_cCxt, "remove_namespace", RMF(xb_cxt_name_del), 1);
	rb_define_method(xb_cCxt, "clear", RMF(xb_cxt_clear), 0);
	rb_define_method(xb_cCxt, "clear_namespaces", RMF(xb_cxt_clear), 0);
	rb_define_method(xb_cCxt, "[]", RMF(xb_cxt_get), 1);
	rb_define_method(xb_cCxt, "[]=", RMF(xb_cxt_set), 2);
	rb_define_method(xb_cCxt, "returntype", RMF(xb_cxt_return_get), 0);
	rb_define_method(xb_cCxt, "returntype=", RMF(xb_cxt_return_set), 1);
	rb_define_method(xb_cCxt, "evaltype", RMF(xb_cxt_eval_get), 0);
	rb_define_method(xb_cCxt, "evaltype=", RMF(xb_cxt_eval_set), 1);
	rb_define_method(xb_cCxt, "uri", RMF(xb_cxt_uri_get), 0);
	rb_define_method(xb_cCxt, "uri=", RMF(xb_cxt_uri_set), 1);
	rb_define_method(xb_cCxt, "base_uri", RMF(xb_cxt_uri_get), 0);
	rb_define_method(xb_cCxt, "base_uri=", RMF(xb_cxt_uri_set), 1);
#if HAVE_DBXML_CXT_VARIABLE_VALUE
        rb_define_method(xb_cCxt, "get_results", RMF(xb_cxt_get_results), 1);
#endif
#if HAVE_DBXML_CXT_COLLECTION
        rb_define_method(xb_cCxt, "collection", RMF(xb_cxt_coll_get), 0);
        rb_define_method(xb_cCxt, "collection=", RMF(xb_cxt_coll_set), 1);
#endif
#if HAVE_DBXML_CXT_INTERRUPT
        rb_define_method(xb_cCxt, "interrupt_query", RMF(xb_cxt_interrupt), 0);
#endif
#if HAVE_DBXML_CXT_TIMEOUT
        rb_define_method(xb_cCxt, "query_timeout", RMF(xb_cxt_get_timeout), 0);
        rb_define_method(xb_cCxt, "query_timeout=", RMF(xb_cxt_set_timeout), 1);
#endif

	xb_cQue = rb_define_class_under(xb_mXML, "Query", rb_cObject);
	rb_const_set(xb_mXML, rb_intern("QueryExpression"), xb_cQue);
	rb_undef_method(CLASS_OF(xb_cQue), "allocate");
	rb_undef_method(CLASS_OF(xb_cQue), "new");
	rb_define_private_method(xb_cQue, "__txn_dup__", RMF(xb_que_txn_dup), 1);
	rb_define_private_method(xb_cQue, "__txn_close__", RMF(xb_que_txn_close), 2);
	rb_define_method(xb_cQue, "manager", RMF(xb_que_manager), 0);
	rb_define_method(xb_cQue, "transaction", RMF(xb_que_txn), 0);
	rb_define_method(xb_cQue, "txn", RMF(xb_que_txn), 0);
	rb_define_method(xb_cQue, "transaction?", RMF(xb_que_txn_p), 0);
	rb_define_method(xb_cQue, "txn?", RMF(xb_que_txn_p), 0);
	rb_define_method(xb_cQue, "in_transaction?", RMF(xb_que_txn_p), 0);
	rb_define_method(xb_cQue, "in_txn?", RMF(xb_que_txn_p), 0);
	rb_define_method(xb_cQue, "execute", RMF(xb_que_exec), -1);
	rb_define_method(xb_cQue, "to_s", RMF(xb_que_to_str), 0);
	rb_define_method(xb_cQue, "to_str", RMF(xb_que_to_str), 0);
#if HAVE_DBXML_QUE_UPDATE
	rb_define_method(xb_cQue, "update?", RMF(xb_que_update), 0);
#endif

	xb_cRes = rb_define_class_under(xb_mXML, "Results", rb_cObject);
	rb_include_module(xb_cRes, rb_mEnumerable);
#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
	rb_define_alloc_func(xb_cRes, RMFS(xb_res_s_alloc));
#else
	rb_define_singleton_method(xb_cRes, "allocate", RMFS(xb_res_s_alloc), 0);
#endif
	rb_define_singleton_method(xb_cRes, "new", RMF(xb_s_new), -1);
	rb_define_private_method(xb_cRes, "initialize", RMF(xb_res_init), 1);
	rb_define_method(xb_cRes, "manager", RMF(xb_res_manager), 0);
	rb_define_method(xb_cRes, "add", RMF(xb_res_add), 1);
	rb_define_method(xb_cRes, "each", RMF(xb_res_each), 0);
	rb_define_method(xb_cRes, "size", RMF(xb_res_size), 0);
#if HAVE_DBXML_RES_EVAL
	rb_define_method(xb_cRes, "evaluation_type", RMF(xb_res_eval), 0);
#endif

	xb_cMod = rb_define_class_under(xb_mXML, "Modify", rb_cObject);
#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
	rb_define_alloc_func(xb_cMod, RMFS(xb_mod_s_alloc));
#else
	rb_define_singleton_method(xb_cMod, "allocate", RMFS(xb_mod_s_alloc), 0);
#endif
	rb_define_singleton_method(xb_cMod, "new", RMF(xb_s_new), -1);
	rb_define_private_method(xb_cMod, "initialize", RMF(xb_mod_init), 1);
#if HAVE_DBXML_CONST_XMLMODIFY_ELEMENT
        rb_define_const(xb_cMod, "Element", INT2NUM(XmlModify::Element));
#endif
#if HAVE_DBXML_CONST_XMLMODIFY_ATTRIBUTE
        rb_define_const(xb_cMod, "Attribute", INT2NUM(XmlModify::Attribute));
#endif
#if HAVE_DBXML_CONST_XMLMODIFY_TEXT
        rb_define_const(xb_cMod, "Text", INT2NUM(XmlModify::Text));
#endif
#if HAVE_DBXML_CONST_XMLMODIFY_COMMENT
        rb_define_const(xb_cMod, "Comment", INT2NUM(XmlModify::Comment));
#endif
#if HAVE_DBXML_CONST_XMLMODIFY_PROCESSINGINSTRUCTION
        rb_define_const(xb_cMod, "PI", INT2NUM(XmlModify::ProcessingInstruction));
#endif
#if HAVE_DBXML_CONST_XMLMODIFY_PROCESSINGINSTRUCTION
        rb_define_const(xb_cMod, "ProcessingInstruction", INT2NUM(XmlModify::ProcessingInstruction));
#endif
	rb_define_private_method(xb_cMod, "__txn_dup__", RMF(xb_mod_txn_dup), 1);
	rb_define_private_method(xb_cMod, "__txn_close__", RMF(xb_mod_txn_close), 2);
	rb_define_method(xb_cMod, "manager", RMF(xb_mod_manager), 0);
	rb_define_method(xb_cMod, "transaction", RMF(xb_mod_txn), 0);
	rb_define_method(xb_cMod, "txn", RMF(xb_mod_txn), 0);
	rb_define_method(xb_cMod, "transaction?", RMF(xb_mod_txn_p), 0);
	rb_define_method(xb_cMod, "txn?", RMF(xb_mod_txn_p), 0);
	rb_define_method(xb_cMod, "in_transaction?", RMF(xb_mod_txn_p), 0);
	rb_define_method(xb_cMod, "in_txn?", RMF(xb_mod_txn_p), 0);
#if HAVE_DBXML_MOD_ENCODING
	rb_define_method(xb_cMod, "encoding=", RMF(xb_mod_encoding), 1);
#endif
        rb_define_method(xb_cMod, "append", RMF(xb_mod_append), -1);
        rb_define_method(xb_cMod, "insert_after", RMF(xb_mod_insert_after), 4);
        rb_define_method(xb_cMod, "insert_before", RMF(xb_mod_insert_before), 4);
        rb_define_method(xb_cMod, "remove", RMF(xb_mod_remove), 1);
        rb_define_method(xb_cMod, "rename", RMF(xb_mod_rename), 2);
        rb_define_method(xb_cMod, "update", RMF(xb_mod_update), 2);
        rb_define_method(xb_cMod, "execute", RMF(xb_mod_execute), -1);

        xb_cVal = rb_define_class_under(xb_mXML, "Value", rb_cObject);
#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
	rb_define_alloc_func(xb_cVal, RMFS(xb_val_s_alloc));
#else
	rb_define_singleton_method(xb_cVal, "allocate", RMFS(xb_val_s_alloc), 0);
#endif
	rb_define_singleton_method(xb_cVal, "new", RMF(xb_s_new), -1);
	rb_define_private_method(xb_cVal, "initialize", RMF(xb_val_init), -1);
#if HAVE_DBXML_CONST_XMLVALUE_NONE
        rb_define_const(xb_cVal, "NONE", INT2NUM(XmlValue::NONE));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_NODE
        rb_define_const(xb_cVal, "NODE", INT2NUM(XmlValue::NODE));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_ANY_SIMPLE_TYPE
        rb_define_const(xb_cVal, "ANY_SIMPLE_TYPE", INT2NUM(XmlValue::ANY_SIMPLE_TYPE));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_ANY_URI
        rb_define_const(xb_cVal, "ANY_URI", INT2NUM(XmlValue::ANY_URI));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_BASE_64_BINARY
        rb_define_const(xb_cVal, "BASE_64_BINARY", INT2NUM(XmlValue::BASE_64_BINARY));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_BOOLEAN
        rb_define_const(xb_cVal, "BOOLEAN", INT2NUM(XmlValue::BOOLEAN));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_DATE
        rb_define_const(xb_cVal, "DATE", INT2NUM(XmlValue::DATE));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_DATE_TIME
        rb_define_const(xb_cVal, "DATE_TIME", INT2NUM(XmlValue::DATE_TIME));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_DAY_TIME_DURATION
        rb_define_const(xb_cVal, "DAY_TIME_DURATION", INT2NUM(XmlValue::DAY_TIME_DURATION));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_DECIMAL
        rb_define_const(xb_cVal, "DECIMAL", INT2NUM(XmlValue::DECIMAL));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_DOUBLE
        rb_define_const(xb_cVal, "DOUBLE", INT2NUM(XmlValue::DOUBLE));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_DURATION
        rb_define_const(xb_cVal, "DURATION", INT2NUM(XmlValue::DURATION));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_FLOAT
        rb_define_const(xb_cVal, "FLOAT", INT2NUM(XmlValue::FLOAT));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_G_DAY
        rb_define_const(xb_cVal, "G_DAY", INT2NUM(XmlValue::G_DAY));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_G_MONTH
        rb_define_const(xb_cVal, "G_MONTH", INT2NUM(XmlValue::G_MONTH));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_G_MONTH_DAY
        rb_define_const(xb_cVal, "G_MONTH_DAY", INT2NUM(XmlValue::G_MONTH_DAY));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_G_YEAR
        rb_define_const(xb_cVal, "G_YEAR", INT2NUM(XmlValue::G_YEAR));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_G_YEAR_MONTH
        rb_define_const(xb_cVal, "G_YEAR_MONTH", INT2NUM(XmlValue::G_YEAR_MONTH));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_HEX_BINARY
        rb_define_const(xb_cVal, "HEX_BINARY", INT2NUM(XmlValue::HEX_BINARY));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_NOTATION
        rb_define_const(xb_cVal, "NOTATION", INT2NUM(XmlValue::NOTATION));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_QNAME
        rb_define_const(xb_cVal, "QNAME", INT2NUM(XmlValue::QNAME));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_STRING
        rb_define_const(xb_cVal, "STRING", INT2NUM(XmlValue::STRING));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_TIME
        rb_define_const(xb_cVal, "TIME", INT2NUM(XmlValue::TIME));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_YEAR_MONTH_DURATION
        rb_define_const(xb_cVal, "YEAR_MONTH_DURATION", INT2NUM(XmlValue::YEAR_MONTH_DURATION));
#endif
#if HAVE_DBXML_CONST_XMLVALUE_UNTYPED_ATOMIC 
        rb_define_const(xb_cVal, "UNTYPED_ATOMIC", INT2NUM(XmlValue::UNTYPED_ATOMIC ));
#endif
        rb_define_method(xb_cVal, "to_document", RMF(xb_val_to_doc), 0);
        rb_define_method(xb_cVal, "to_f", RMF(xb_val_to_f), 0);
        rb_define_method(xb_cVal, "to_s", RMF(xb_val_to_str), 0);
        rb_define_method(xb_cVal, "to_str", RMF(xb_val_to_str), 0);
        rb_define_method(xb_cVal, "type", RMF(xb_val_type), 0);
        rb_define_method(xb_cVal, "type?", RMF(xb_val_type_p), 1);
        rb_define_method(xb_cVal, "nil?", RMF(xb_val_nil_p), 1);
        rb_define_method(xb_cVal, "number?", RMF(xb_val_number_p), 1);
        rb_define_method(xb_cVal, "string?", RMF(xb_val_string_p), 1);
        rb_define_method(xb_cVal, "boolean?", RMF(xb_val_boolean_p), 1);
        rb_define_method(xb_cVal, "node?", RMF(xb_val_node_p), 1);
        rb_define_method(xb_cVal, "node_name", RMF(xb_val_node_name), 0);
        rb_define_method(xb_cVal, "node_value", RMF(xb_val_node_value), 0);
        rb_define_method(xb_cVal, "node_type", RMF(xb_val_node_type), 0);
        rb_define_method(xb_cVal, "namespace", RMF(xb_val_namespace), 0);
        rb_define_method(xb_cVal, "namespace_uri", RMF(xb_val_namespace), 0);
        rb_define_method(xb_cVal, "prefix", RMF(xb_val_prefix), 0);
        rb_define_method(xb_cVal, "local_name", RMF(xb_val_local_name), 0);
        rb_define_method(xb_cVal, "parent_node", RMF(xb_val_parent_node), 0);
        rb_define_method(xb_cVal, "first_child", RMF(xb_val_first_child), 0);
        rb_define_method(xb_cVal, "last_child", RMF(xb_val_last_child), 0);
        rb_define_method(xb_cVal, "previous_sibling", RMF(xb_val_previous_sibling), 0);
        rb_define_method(xb_cVal, "next_sibling", RMF(xb_val_next_sibling), 0);
        rb_define_method(xb_cVal, "owner_element", RMF(xb_val_owner_element), 0);
        rb_define_method(xb_cVal, "attributes", RMF(xb_val_attributes), 0);
#if HAVE_DBXML_VAL_TYPE_URI
        rb_define_method(xb_cVal, "type_uri", RMF(xb_val_type_uri), 0);
#endif
#if HAVE_DBXML_VAL_TYPE_NAME
        rb_define_method(xb_cVal, "type_name", RMF(xb_val_type_name), 0);
#endif
#if HAVE_DBXML_VAL_NODE_HANDLE
        rb_define_method(xb_cVal, "node_handle", RMF(xb_val_node_handle), 0);
#endif

#if HAVE_DBXML_XML_EVENT_WRITER
        xb_cEwr = rb_define_class_under(xb_mXML, "EventWriter", rb_cObject);
#if HAVE_DBXML_XML_EVENT_WRITER_ALLOC
#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
	rb_define_alloc_func(xb_cEwr, RMFS(xb_ewr_s_alloc));
#else
	rb_define_singleton_method(xb_cEwr, "allocate", RMFS(xb_ewr_s_alloc), 0);
#endif
	rb_define_singleton_method(xb_cEwr, "new", RMF(xb_s_new), -1);
	rb_define_private_method(xb_cEwr, "initialize", RMF(xb_ewr_init), 0);
#else
	rb_undef_alloc_func(xb_cEwr);
#endif
	rb_define_method(xb_cEwr, "close", RMF(xb_ewr_close), 0);
	rb_define_method(xb_cEwr, "attribute", RMF(xb_ewr_attribute), 5);
	rb_define_method(xb_cEwr, "dtd", RMF(xb_ewr_dtd), 1);
	rb_define_method(xb_cEwr, "end_document", RMF(xb_ewr_end_doc), 0);
	rb_define_method(xb_cEwr, "end_element", RMF(xb_ewr_end_ele), -1);
	rb_define_method(xb_cEwr, "end_entity", RMF(xb_ewr_end_ent), 1);
	rb_define_method(xb_cEwr, "processing_instruction", RMF(xb_ewr_pi), 2);
	rb_define_method(xb_cEwr, "start_element", RMF(xb_ewr_start_ele), 5);
	rb_define_method(xb_cEwr, "start_entity", RMF(xb_ewr_start_ent), 2);
	rb_define_method(xb_cEwr, "start_document", RMF(xb_ewr_start_doc), -1);
	rb_define_method(xb_cEwr, "text", RMF(xb_ewr_txt), 2);
#endif

#if HAVE_DBXML_XML_EVENT_READER
       xb_cErd = rb_define_class_under(xb_mXML, "EventReader", rb_cObject);
#if HAVE_DBXML_XML_EVENT_READER_ALLOC
#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
	rb_define_alloc_func(xb_cErd, RMFS(xb_erd_s_alloc));
#else
	rb_define_singleton_method(xb_cErd, "allocate", RMFS(xb_erd_s_alloc), 0);
#endif
	rb_define_singleton_method(xb_cErd, "new", RMF(xb_s_new), -1);
	rb_define_private_method(xb_cErd, "initialize", RMF(xb_erd_init), 0);
#else
	rb_undef_alloc_func(xb_cErd);
#endif
#if HAVE_DBXML_CONST_XMLEVENTREADER_STARTELEMENT
	rb_define_const(xb_cErd, "StartElement", INT2NUM(XmlEventReader::StartElement));
#endif
#if HAVE_DBXML_CONST_XMLEVENTREADER_ENDELEMENT
	rb_define_const(xb_cErd, "EndElement", INT2NUM(XmlEventReader::EndElement));
#endif
#if HAVE_DBXML_CONST_XMLEVENTREADER_CHARACTERS
	rb_define_const(xb_cErd, "Characters", INT2NUM(XmlEventReader::Characters));
#endif
#if HAVE_DBXML_CONST_XMLEVENTREADER_CDATA
	rb_define_const(xb_cErd, "CDATA", INT2NUM(XmlEventReader::CDATA));
#endif
#if HAVE_DBXML_CONST_XMLEVENTREADER_COMMENT
	rb_define_const(xb_cErd, "Comment", INT2NUM(XmlEventReader::Comment));
#endif
#if HAVE_DBXML_CONST_XMLEVENTREADER_WHITESPACE
	rb_define_const(xb_cErd, "Whitespace", INT2NUM(XmlEventReader::Whitespace));
#endif
#if HAVE_DBXML_CONST_XMLEVENTREADER_STARTDOCUMENT
	rb_define_const(xb_cErd, "StartDocument", INT2NUM(XmlEventReader::StartDocument));
#endif
#if HAVE_DBXML_CONST_XMLEVENTREADER_ENDDOCUMENT
	rb_define_const(xb_cErd, "EndDocument", INT2NUM(XmlEventReader::EndDocument));
#endif
#if HAVE_DBXML_CONST_XMLEVENTREADER_STARTENTITYREFERENCE
	rb_define_const(xb_cErd, "StartEntityReference", INT2NUM(XmlEventReader::StartEntityReference));
#endif
#if HAVE_DBXML_CONST_XMLEVENTREADER_ENDENTITYREFERENCE
	rb_define_const(xb_cErd, "EndEntityReference", INT2NUM(XmlEventReader::EndEntityReference));
#endif
#if HAVE_DBXML_CONST_XMLEVENTREADER_PROCESSINGINSTRUCTION
	rb_define_const(xb_cErd, "ProcessingInstruction", INT2NUM(XmlEventReader::ProcessingInstruction));
#endif
#if HAVE_DBXML_CONST_XMLEVENTREADER_DTD
	rb_define_const(xb_cErd, "DTD", INT2NUM(XmlEventReader::DTD));
#endif
	rb_define_method(xb_cErd, "close", RMF(xb_erd_close), 0);
	rb_define_method(xb_cErd, "event_type", RMF(xb_erd_ev), 0);
	rb_define_method(xb_cErd, "namespace_uri", RMF(xb_erd_uri), 0);
	rb_define_method(xb_cErd, "local_name", RMF(xb_erd_name), 0);
	rb_define_method(xb_cErd, "prefix", RMF(xb_erd_prefix), 0);
	rb_define_method(xb_cErd, "value", RMF(xb_erd_value), 0);
	rb_define_method(xb_cErd, "attribute_count", RMF(xb_erd_attr_count), 0);
	rb_define_method(xb_cErd, "attribute_specified?", RMF(xb_erd_attr_p), 1);
	rb_define_method(xb_cErd, "attribute_namespace_uri", RMF(xb_erd_attr_uri), 1);
	rb_define_method(xb_cErd, "attribute_local_name", RMF(xb_erd_attr_name), 1);
	rb_define_method(xb_cErd, "attribute_prefix", RMF(xb_erd_attr_prefix), 1);
	rb_define_method(xb_cErd, "attribute_value", RMF(xb_erd_attr_value), 1);
	rb_define_method(xb_cErd, "empty_element_info?", RMF(xb_erd_info_p), 0);
	rb_define_method(xb_cErd, "entity_escape_info?", RMF(xb_erd_escape_p), 0);
	rb_define_method(xb_cErd, "next?", RMF(xb_erd_next_p), 0);
	rb_define_method(xb_cErd, "empty_element?", RMF(xb_erd_empty_p), 0);
	rb_define_method(xb_cErd, "whitespace?", RMF(xb_erd_white_p), 0);
	rb_define_method(xb_cErd, "entity_escape?", RMF(xb_erd_nescape_p), -1);
	rb_define_method(xb_cErd, "next", RMF(xb_erd_next), 0);
	rb_define_method(xb_cErd, "next_tag", RMF(xb_erd_next_tag), 0);
	rb_define_method(xb_cErd, "expand_entities?", RMF(xb_erd_get_expand), 0);
	rb_define_method(xb_cErd, "expand_entities=", RMF(xb_erd_set_expand), 1);
	rb_define_method(xb_cErd, "entity_info?", RMF(xb_erd_get_info), 0);
	rb_define_method(xb_cErd, "entity_info=", RMF(xb_erd_set_info), 1);
	rb_define_method(xb_cErd, "encoding", RMF(xb_erd_encoding), 0);
	rb_define_method(xb_cErd, "encoding?", RMF(xb_erd_encoding_p), 0);
	rb_define_method(xb_cErd, "version", RMF(xb_erd_version), 0);
	rb_define_method(xb_cErd, "system_id", RMF(xb_erd_system), 0);
	rb_define_method(xb_cErd, "document_standalone?", RMF(xb_erd_doc_stand_p), 0);
	rb_define_method(xb_cErd, "standalone?", RMF(xb_erd_attr_stand_p), 0);
        rb_define_method(xb_cVal, "to_event_reader", RMF(xb_val_to_erd), 0);
        rb_define_method(xb_cVal, "event_reader", RMF(xb_val_to_erd), 0);
#endif

    }
}
    
