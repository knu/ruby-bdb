#include "bdb.h"

VALUE bdb_cEnv;
VALUE bdb_eFatal;
VALUE bdb_eLock, bdb_eLockDead, bdb_eLockHeld, bdb_eLockGranted;
#if HAVE_CONST_DB_REP_UNAVAIL
VALUE bdb_eRepUnavail;
#endif
VALUE bdb_mDb;
VALUE bdb_cCommon, bdb_cBtree, bdb_cRecnum, bdb_cHash, bdb_cRecno, bdb_cUnknown;
VALUE bdb_cDelegate;

#if HAVE_TYPE_DB_KEY_RANGE
VALUE bdb_sKeyrange;
#endif

#if HAVE_CONST_DB_QUEUE
VALUE bdb_cQueue;
#endif

VALUE bdb_cTxn, bdb_cTxnCatch;
VALUE bdb_cCursor;
VALUE bdb_cLock, bdb_cLockid;
VALUE bdb_cLsn;

VALUE bdb_mMarshal;

ID bdb_id_dump, bdb_id_load;
ID bdb_id_current_db;

VALUE bdb_errstr;
int bdb_errcall = 0;

#if ! HAVE_DB_STRERROR

char *
db_strerror(int err)
{
    if (err == 0)
        return "" ;

    if (err > 0)
        return strerror(err) ;

    switch (err) {
#if HAVE_CONST_DB_INCOMPLETE
    case DB_INCOMPLETE:
        return ("DB_INCOMPLETE: Sync was unable to complete");
#endif
#if HAVE_CONST_DB_KEYEMPTY
    case DB_KEYEMPTY:
        return ("DB_KEYEMPTY: Non-existent key/data pair");
#endif
#if HAVE_CONST_DB_KEYEXIST
    case DB_KEYEXIST:
        return ("DB_KEYEXIST: Key/data pair already exists");
#endif
#if HAVE_CONST_DB_LOCK_DEADLOCK
    case DB_LOCK_DEADLOCK:
        return ("DB_LOCK_DEADLOCK: Locker killed to resolve a deadlock");
#endif
#if HAVE_CONST_DB_LOCK_NOTGRANTED
    case DB_LOCK_NOTGRANTED:
        return ("DB_LOCK_NOTGRANTED: Lock not granted");
#endif
#if HAVE_CONST_DB_LOCK_NOTHELD
    case DB_LOCK_NOTHELD:
        return ("DB_LOCK_NOTHELD: Lock not held by locker");
#endif
#if HAVE_CONST_DB_NOTFOUND
    case DB_NOTFOUND:
        return ("DB_NOTFOUND: No matching key/data pair found");
#endif
#if HAVE_CONST_DB_RUNRECOVERY
    case DB_RUNRECOVERY:
        return ("DB_RUNRECOVERY: Fatal error, run database recovery");
#endif
#if HAVE_CONST_DB_OLD_VERSION
    case DB_OLD_VERSION:
        return ("DB_OLD_VERSION: The database cannot be opened without being first upgraded");
#endif
    default:
        return "Unknown Error" ;
    }
}

#endif

int
bdb_test_error(int comm)
{
    VALUE error;

    switch (comm) {
    case 0:
#if HAVE_CONST_DB_NOTFOUND
    case DB_NOTFOUND:
#endif
#if HAVE_CONST_DB_KEYEMPTY
    case DB_KEYEMPTY:
#endif
#if HAVE_CONST_DB_KEYEXIST
    case DB_KEYEXIST:
#endif
	return comm;
        break;
#if HAVE_CONST_DB_INCOMPLETE
    case DB_INCOMPLETE:
	comm = 0;
	return comm;
        break;
#endif
#if HAVE_CONST_DB_REP_UNAVAIL
    case DB_REP_UNAVAIL:
	error = bdb_eRepUnavail;
	break;
#endif
#if HAVE_CONST_DB_LOCK_DEADLOCK
    case DB_LOCK_DEADLOCK:
#endif
    case EAGAIN:
	error = bdb_eLockDead;
	break;
#if HAVE_CONST_DB_LOCK_NOTGRANTED
    case DB_LOCK_NOTGRANTED:
	error = bdb_eLockGranted;
	break;
#endif
#if HAVE_CONST_DB_LOCK_NOTHELD
    case DB_LOCK_NOTHELD:
	error = bdb_eLockHeld;
	break;
#endif
#if HAVE_ST_DB_ASSOCIATE
    case BDB_ERROR_PRIVATE:
	error = bdb_eFatal;
	bdb_errcall = 1;
	bdb_errstr = rb_inspect(rb_gv_get("$!"));
	comm = 0;
	break;
#endif
    default:
	error = bdb_eFatal;
	break;
    }
    if (bdb_errcall) {
	bdb_errcall = 0;
	if (comm) {
	    rb_raise(error, "%s -- %s", StringValuePtr(bdb_errstr), db_strerror(comm));
	}
	else {
	    rb_raise(error, "%s", StringValuePtr(bdb_errstr));
	}
    }
    else
	rb_raise(error, "%s", db_strerror(comm));
}

void
Init_bdb()
{
    int major, minor, patch;
    VALUE version;

    if (rb_const_defined_at(rb_cObject, rb_intern("BDB"))) {
	rb_raise(rb_eNameError, "module already defined");
    }
    version = rb_tainted_str_new2(db_version(&major, &minor, &patch));
    if (major != DB_VERSION_MAJOR || minor != DB_VERSION_MINOR
	|| patch != DB_VERSION_PATCH) {
        rb_raise(rb_eNotImpError, "\nBDB needs compatible versions of libdb & db.h\n\tyou have db.h version %d.%d.%d and libdb version %d.%d.%d\n",
		 DB_VERSION_MAJOR, DB_VERSION_MINOR, DB_VERSION_PATCH,
		 major, minor, patch);
    }
    bdb_mMarshal = rb_const_get(rb_cObject, rb_intern("Marshal"));
    bdb_id_current_db = rb_intern("__bdb_current_db__");
    bdb_id_dump = rb_intern("dump");
    bdb_id_load = rb_intern("load");
    bdb_mDb = rb_define_module("BDB");
    bdb_eFatal = rb_define_class_under(bdb_mDb, "Fatal", rb_eStandardError);
    bdb_eLock = rb_define_class_under(bdb_mDb, "LockError", bdb_eFatal);
    bdb_eLockDead = rb_define_class_under(bdb_mDb, "LockDead", bdb_eLock);
    bdb_eLockHeld = rb_define_class_under(bdb_mDb, "LockHeld", bdb_eLock);
    bdb_eLockGranted = rb_define_class_under(bdb_mDb, "LockGranted",  bdb_eLock);
#if HAVE_CONST_DB_REP_UNAVAIL
    bdb_eRepUnavail = rb_define_class_under(bdb_mDb, "RepUnavail", bdb_eFatal);
#endif
/* CONSTANT */
    rb_define_const(bdb_mDb, "VERSION", version);
    rb_define_const(bdb_mDb, "VERSION_MAJOR", INT2FIX(major));
    rb_define_const(bdb_mDb, "VERSION_MINOR", INT2FIX(minor));
    rb_define_const(bdb_mDb, "VERSION_PATCH", INT2FIX(patch));
    rb_define_const(bdb_mDb, "VERSION_NUMBER", INT2NUM(BDB_VERSION));
#if HAVE_CONST_DB_BTREE
    rb_define_const(bdb_mDb, "BTREE", INT2FIX(DB_BTREE));
#endif
#if HAVE_CONST_DB_HASH
    rb_define_const(bdb_mDb, "HASH", INT2FIX(DB_HASH));
#endif
#if HAVE_CONST_DB_RECNO
    rb_define_const(bdb_mDb, "RECNO", INT2FIX(DB_RECNO));
#endif
#if HAVE_CONST_DB_QUEUE
    rb_define_const(bdb_mDb, "QUEUE", INT2FIX(DB_QUEUE));
#else
    rb_define_const(bdb_mDb, "QUEUE", INT2FIX(0));
#endif
#if HAVE_CONST_DB_UNKNOWN
    rb_define_const(bdb_mDb, "UNKNOWN", INT2FIX(DB_UNKNOWN));
#endif
#if HAVE_CONST_DB_AFTER
    rb_define_const(bdb_mDb, "AFTER", INT2FIX(DB_AFTER));
#endif
#if HAVE_CONST_DB_AGGRESSIVE
    rb_define_const(bdb_mDb, "AGGRESSIVE", INT2FIX(DB_AGGRESSIVE));
#endif
#if HAVE_CONST_DB_APPEND
    rb_define_const(bdb_mDb, "APPEND", INT2FIX(DB_APPEND));
#endif
#if HAVE_CONST_DB_ARCH_ABS
    rb_define_const(bdb_mDb, "ARCH_ABS", INT2FIX(DB_ARCH_ABS));
#endif
#if HAVE_CONST_DB_ARCH_DATA
    rb_define_const(bdb_mDb, "ARCH_DATA", INT2FIX(DB_ARCH_DATA));
#endif
#if HAVE_CONST_DB_ARCH_LOG
    rb_define_const(bdb_mDb, "ARCH_LOG", INT2FIX(DB_ARCH_LOG));
#endif
#if HAVE_CONST_DB_BEFORE
    rb_define_const(bdb_mDb, "BEFORE", INT2FIX(DB_BEFORE));
#endif
#if HAVE_CONST_DB_CACHED_COUNTS
    rb_define_const(bdb_mDb, "CACHED_COUNTS", INT2FIX(DB_CACHED_COUNTS));
#endif
#if HAVE_CONST_DB_CDB_ALLDB
    rb_define_const(bdb_mDb, "CDB_ALLDB", INT2FIX(DB_CDB_ALLDB));
#endif
#if HAVE_CONST_DB_CHECKPOINT
    rb_define_const(bdb_mDb, "CHECKPOINT", INT2FIX(DB_CHECKPOINT));
#endif
#if HAVE_CONST_DB_CLIENT
    rb_define_const(bdb_mDb, "CLIENT", INT2FIX(DB_CLIENT));
#endif
#if HAVE_CONST_DB_RPCCLIENT
    rb_define_const(bdb_mDb, "RPCCLIENT", INT2FIX(DB_RPCCLIENT));
#endif
#if HAVE_CONST_DB_CONFIG
    rb_define_const(bdb_mDb, "CONFIG", INT2FIX(DB_CONFIG));
#endif
#if ! HAVE_CONST_DB_CONSUME
    rb_define_const(bdb_mDb, "CONSUME", INT2FIX(0));
#else
    rb_define_const(bdb_mDb, "CONSUME", INT2FIX(DB_CONSUME));
#endif
#if HAVE_CONST_DB_CONSUME_WAIT
    rb_define_const(bdb_mDb, "CONSUME_WAIT", INT2FIX(DB_CONSUME_WAIT));
#endif
#if HAVE_CONST_DB_CREATE
    rb_define_const(bdb_mDb, "CREATE", INT2FIX(DB_CREATE));
#endif
#if HAVE_CONST_DB_CURLSN
    rb_define_const(bdb_mDb, "CURLSN", INT2FIX(DB_CURLSN));
#endif
#if HAVE_CONST_DB_CURRENT
    rb_define_const(bdb_mDb, "CURRENT", INT2FIX(DB_CURRENT));
#endif
#if HAVE_CONST_DB_DIRTY_READ
    rb_define_const(bdb_mDb, "DIRTY_READ", INT2FIX(DB_DIRTY_READ));
#else
    rb_define_const(bdb_mDb, "DIRTY_READ", INT2FIX(0));
#endif
#if HAVE_CONST_DB_READ_COMMITTED
    rb_define_const(bdb_mDb, "READ_COMMITTED", INT2FIX(DB_READ_COMMITTED));
#endif
#if HAVE_CONST_DB_READ_UNCOMMITTED
    rb_define_const(bdb_mDb, "READ_UNCOMMITTED", INT2FIX(DB_READ_UNCOMMITTED));
#endif
#if HAVE_CONST_DB_STAT_ALL
    rb_define_const(bdb_mDb, "STAT_ALL", INT2FIX(DB_STAT_ALL));
#endif
#if HAVE_CONST_DB_STAT_SUBSYSTEM
    rb_define_const(bdb_mDb, "STAT_SUBSYSTEM", INT2FIX(DB_STAT_SUBSYSTEM));
#endif
#if HAVE_CONST_DB_DBT_MALLOC
    rb_define_const(bdb_mDb, "DBT_MALLOC", INT2FIX(DB_DBT_MALLOC));
#endif
#if HAVE_CONST_DB_DBT_PARTIAL
    rb_define_const(bdb_mDb, "DBT_PARTIAL", INT2FIX(DB_DBT_PARTIAL));
#endif
#if ! HAVE_CONST_DB_DBT_REALLOC
    rb_define_const(bdb_mDb, "DBT_REALLOC", INT2FIX(0));
#else
    rb_define_const(bdb_mDb, "DBT_REALLOC", INT2FIX(DB_DBT_REALLOC));
#endif
#if HAVE_CONST_DB_DBT_USERMEM
    rb_define_const(bdb_mDb, "DBT_USERMEM", INT2FIX(DB_DBT_USERMEM));
#endif
#if HAVE_CONST_DB_DONOTINDEX
    rb_define_const(bdb_mDb, "DONOTINDEX", INT2FIX(DB_DONOTINDEX));
#endif
#if HAVE_CONST_DB_DUP
    rb_define_const(bdb_mDb, "DUP", INT2FIX(DB_DUP));
#endif
#if HAVE_CONST_DB_DUPSORT
    rb_define_const(bdb_mDb, "DUPSORT", INT2FIX(DB_DUPSORT));
#endif
#if HAVE_CONST_DB_EXCL
    rb_define_const(bdb_mDb, "EXCL", INT2FIX(DB_EXCL));
#endif
#if HAVE_CONST_DB_MULTIVERSION
    rb_define_const(bdb_mDb, "MULTIVERSION", INT2FIX(DB_MULTIVERSION));
#endif
#if HAVE_CONST_DB_FAST_STAT
    rb_define_const(bdb_mDb, "FAST_STAT", INT2FIX(DB_FAST_STAT));
#endif
#if HAVE_CONST_DB_FIRST
    rb_define_const(bdb_mDb, "FIRST", INT2FIX(DB_FIRST));
#endif
#if HAVE_CONST_DB_FIXEDLEN
    rb_define_const(bdb_mDb, "FIXEDLEN", INT2FIX(DB_FIXEDLEN));
#endif
#if HAVE_CONST_DB_FLUSH
    rb_define_const(bdb_mDb, "FLUSH", INT2FIX(DB_FLUSH));
#endif
#if ! HAVE_CONST_DB_FORCE
    rb_define_const(bdb_mDb, "FORCE", INT2FIX(1));
#else
    rb_define_const(bdb_mDb, "FORCE", INT2FIX(DB_FORCE));
#endif
#if ! HAVE_CONST_DB_GET_BOTH
    rb_define_const(bdb_mDb, "GET_BOTH", INT2FIX(9));
#else
    rb_define_const(bdb_mDb, "GET_BOTH", INT2FIX(DB_GET_BOTH));
#endif
#if HAVE_CONST_DB_GET_RECNO
    rb_define_const(bdb_mDb, "GET_RECNO", INT2FIX(DB_GET_RECNO));
#endif
#if HAVE_CONST_DB_HOME
    rb_define_const(bdb_mDb, "HOME", INT2FIX(DB_HOME));
#endif
#if HAVE_CONST_DB_INCOMPLETE
    rb_define_const(bdb_mDb, "INCOMPLETE", INT2FIX(DB_INCOMPLETE));
#endif
#if HAVE_CONST_DB_INIT_CDB
    rb_define_const(bdb_mDb, "INIT_CDB", INT2FIX(DB_INIT_CDB));
#endif
#if HAVE_CONST_DB_INIT_LOCK
    rb_define_const(bdb_mDb, "INIT_LOCK", INT2FIX(DB_INIT_LOCK));
#endif
#if HAVE_CONST_DB_INIT_LOG
    rb_define_const(bdb_mDb, "INIT_LOG", INT2FIX(DB_INIT_LOG));
#endif
#if HAVE_CONST_DB_INIT_MPOOL
    rb_define_const(bdb_mDb, "INIT_MPOOL", INT2FIX(DB_INIT_MPOOL));
#endif
#if HAVE_CONST_DB_INIT_TXN
    rb_define_const(bdb_mDb, "INIT_TXN", INT2FIX(DB_INIT_TXN));
#endif
    rb_define_const(bdb_mDb, "INIT_TRANSACTION", INT2FIX(BDB_INIT_TRANSACTION));
    rb_define_const(bdb_mDb, "INIT_LOMP", INT2FIX(BDB_INIT_LOMP));
#if HAVE_CONST_DB_JOINENV
    rb_define_const(bdb_mDb, "JOINENV", INT2FIX(DB_JOINENV));
#endif
#if HAVE_CONST_DB_JOIN_ITEM
    rb_define_const(bdb_mDb, "JOIN_ITEM", INT2FIX(DB_JOIN_ITEM));
#endif
#if HAVE_CONST_DB_JOIN_NOSORT
    rb_define_const(bdb_mDb, "JOIN_NOSORT", INT2FIX(DB_JOIN_NOSORT));
#endif
#if HAVE_CONST_DB_KEYFIRST
    rb_define_const(bdb_mDb, "KEYFIRST", INT2FIX(DB_KEYFIRST));
#endif
#if HAVE_CONST_DB_KEYLAST
    rb_define_const(bdb_mDb, "KEYLAST", INT2FIX(DB_KEYLAST));
#endif
#if HAVE_CONST_DB_LAST
    rb_define_const(bdb_mDb, "LAST", INT2FIX(DB_LAST));
#endif
#if HAVE_CONST_DB_LOCK_CONFLICT
    rb_define_const(bdb_mDb, "LOCK_CONFLICT", INT2FIX(DB_LOCK_CONFLICT));
#else
    rb_define_const(bdb_mDb, "LOCK_CONFLICT", INT2FIX(0));
#endif
#if HAVE_CONST_DB_LOCK_DEADLOCK
    rb_define_const(bdb_mDb, "LOCK_DEADLOCK", INT2FIX(DB_LOCK_DEADLOCK));
#endif
#if HAVE_CONST_DB_LOCK_DEFAULT
    rb_define_const(bdb_mDb, "LOCK_DEFAULT", INT2FIX(DB_LOCK_DEFAULT));
#endif
#if HAVE_CONST_DB_LOCK_GET
    rb_define_const(bdb_mDb, "LOCK_GET", INT2FIX(DB_LOCK_GET));
#endif
#if HAVE_CONST_DB_LOCK_NOTGRANTED
    rb_define_const(bdb_mDb, "LOCK_NOTGRANTED", INT2FIX(DB_LOCK_NOTGRANTED));
#endif
#if HAVE_CONST_DB_LOCK_NOWAIT
    rb_define_const(bdb_mDb, "LOCK_NOWAIT", INT2FIX(DB_LOCK_NOWAIT));
#endif
#if HAVE_CONST_DB_LOCK_OLDEST
    rb_define_const(bdb_mDb, "LOCK_OLDEST", INT2FIX(DB_LOCK_OLDEST));
#endif
#if HAVE_CONST_DB_LOCK_PUT
    rb_define_const(bdb_mDb, "LOCK_PUT", INT2FIX(DB_LOCK_PUT));
#endif
#if HAVE_CONST_DB_LOCK_PUT_ALL
    rb_define_const(bdb_mDb, "LOCK_PUT_ALL", INT2FIX(DB_LOCK_PUT_ALL));
#endif
#if HAVE_CONST_DB_LOCK_PUT_OBJ
    rb_define_const(bdb_mDb, "LOCK_PUT_OBJ", INT2FIX(DB_LOCK_PUT_OBJ));
#endif
#if HAVE_CONST_DB_LOCK_RANDOM
    rb_define_const(bdb_mDb, "LOCK_RANDOM", INT2FIX(DB_LOCK_RANDOM));
#endif
#if HAVE_CONST_DB_LOCK_YOUNGEST
    rb_define_const(bdb_mDb, "LOCK_YOUNGEST", INT2FIX(DB_LOCK_YOUNGEST));
#endif
#if HAVE_CONST_DB_LOCK_NG
    rb_define_const(bdb_mDb, "LOCK_NG", INT2FIX(DB_LOCK_NG));
#endif
#if HAVE_CONST_DB_LOCK_READ
    rb_define_const(bdb_mDb, "LOCK_READ", INT2FIX(DB_LOCK_READ));
#endif
#if HAVE_CONST_DB_LOCK_WRITE
    rb_define_const(bdb_mDb, "LOCK_WRITE", INT2FIX(DB_LOCK_WRITE));
#endif
#if HAVE_CONST_DB_LOCK_IWRITE
    rb_define_const(bdb_mDb, "LOCK_IWRITE", INT2FIX(DB_LOCK_IWRITE));
#endif
#if HAVE_CONST_DB_LOCK_IREAD
    rb_define_const(bdb_mDb, "LOCK_IREAD", INT2FIX(DB_LOCK_IREAD));
#endif
#if HAVE_CONST_DB_LOCK_IWR
    rb_define_const(bdb_mDb, "LOCK_IWR", INT2FIX(DB_LOCK_IWR));
#endif
#if HAVE_CONST_DB_LOCKDOWN
    rb_define_const(bdb_mDb, "LOCKDOWN", INT2FIX(DB_LOCKDOWN));
#else
    rb_define_const(bdb_mDb, "LOCKDOWN", INT2FIX(0));
#endif
#if HAVE_CONST_DB_LOCK_EXPIRE
    rb_define_const(bdb_mDb, "LOCK_EXPIRE", INT2FIX(DB_LOCK_EXPIRE));
#endif
#if HAVE_CONST_DB_LOCK_MAXLOCKS
    rb_define_const(bdb_mDb, "LOCK_MAXLOCKS", INT2FIX(DB_LOCK_MAXLOCKS));
#endif
#if HAVE_CONST_DB_LOCK_MINLOCKS
    rb_define_const(bdb_mDb, "LOCK_MINLOCKS", INT2FIX(DB_LOCK_MINLOCKS));
#endif
#if HAVE_CONST_DB_LOCK_MINWRITE
    rb_define_const(bdb_mDb, "LOCK_MINWRITE", INT2FIX(DB_LOCK_MINWRITE));
#endif
#if HAVE_CONST_DB_MPOOL_CLEAN
    rb_define_const(bdb_mDb, "MPOOL_CLEAN", INT2FIX(DB_MPOOL_CLEAN));
#endif
#if HAVE_CONST_DB_MPOOL_CREATE
    rb_define_const(bdb_mDb, "MPOOL_CREATE", INT2FIX(DB_MPOOL_CREATE));
#endif
#if HAVE_CONST_DB_MPOOL_DIRTY
    rb_define_const(bdb_mDb, "MPOOL_DIRTY", INT2FIX(DB_MPOOL_DIRTY));
#endif
#if HAVE_CONST_DB_MPOOL_DISCARD
    rb_define_const(bdb_mDb, "MPOOL_DISCARD", INT2FIX(DB_MPOOL_DISCARD));
#endif
#if HAVE_CONST_DB_MPOOL_LAST
    rb_define_const(bdb_mDb, "MPOOL_LAST", INT2FIX(DB_MPOOL_LAST));
#endif
#if HAVE_CONST_DB_MPOOL_NEW
    rb_define_const(bdb_mDb, "MPOOL_NEW", INT2FIX(DB_MPOOL_NEW));
#endif
#if HAVE_CONST_DB_MPOOL_PRIVATE
    rb_define_const(bdb_mDb, "MPOOL_PRIVATE", INT2FIX(DB_MPOOL_PRIVATE));
#endif
#if HAVE_CONST_DB_OVERWRITE
    rb_define_const(bdb_mDb, "OVERWRITE", INT2FIX(DB_OVERWRITE));
#endif
#if HAVE_CONST_DB_PRINTABLE
    rb_define_const(bdb_mDb, "PRINTABLE", INT2FIX(DB_PRINTABLE));
#endif
    rb_define_const(bdb_mDb, "NEXT", INT2FIX(DB_NEXT));
#if DB_NEXT_DUP
    rb_define_const(bdb_mDb, "NEXT_DUP", INT2FIX(DB_NEXT_DUP));
#endif
#if HAVE_CONST_DB_NEXT_NODUP
    rb_define_const(bdb_mDb, "NEXT_NODUP", INT2FIX(DB_NEXT_NODUP));
#endif
#if HAVE_CONST_DB_PREV_DUP
    rb_define_const(bdb_mDb, "PREV_DUP", INT2FIX(DB_PREV_DUP));
#endif
#if HAVE_CONST_DB_PREV_NODUP
    rb_define_const(bdb_mDb, "PREV_NODUP", INT2FIX(DB_PREV_NODUP));
#endif
#if HAVE_CONST_DB_NODUPDATA
    rb_define_const(bdb_mDb, "NODUPDATA", INT2FIX(DB_NODUPDATA));
#endif
    rb_define_const(bdb_mDb, "NOMMAP", INT2FIX(DB_NOMMAP));
#if HAVE_CONST_DB_NOORDERCHK
    rb_define_const(bdb_mDb, "NOORDERCHK", INT2FIX(DB_NOORDERCHK));
#endif
    rb_define_const(bdb_mDb, "NOOVERWRITE", INT2FIX(DB_NOOVERWRITE));
#if HAVE_CONST_DB_NOSERVER
    rb_define_const(bdb_mDb, "NOSERVER", INT2FIX(DB_NOSERVER));
#endif
#if HAVE_CONST_DB_NOSERVER_HOME
    rb_define_const(bdb_mDb, "NOSERVER_HOME", INT2FIX(DB_NOSERVER_HOME));
#endif
#if HAVE_CONST_DB_NOSERVER_ID
    rb_define_const(bdb_mDb, "NOSERVER_ID", INT2FIX(DB_NOSERVER_ID));
#endif
    rb_define_const(bdb_mDb, "NOSYNC", INT2FIX(DB_NOSYNC));
#if HAVE_CONST_DB_OLD_VERSION
    rb_define_const(bdb_mDb, "OLD_VERSION", INT2FIX(DB_OLD_VERSION));
#endif
#if HAVE_CONST_DB_ORDERCHKONLY
    rb_define_const(bdb_mDb, "ORDERCHKONLY", INT2FIX(DB_ORDERCHKONLY));
#endif
#if HAVE_CONST_DB_PAD
    rb_define_const(bdb_mDb, "PAD", INT2FIX(DB_PAD));
#endif
#if ! HAVE_CONST_DB_POSITION
    rb_define_const(bdb_mDb, "POSITION", INT2FIX(0));
#else
    rb_define_const(bdb_mDb, "POSITION", INT2FIX(DB_POSITION));
#endif
    rb_define_const(bdb_mDb, "PREV", INT2FIX(DB_PREV));
#if ! HAVE_CONST_DB_PRIVATE
    rb_define_const(bdb_mDb, "PRIVATE", INT2FIX(0));
#else
    rb_define_const(bdb_mDb, "PRIVATE", INT2FIX(DB_PRIVATE));
#endif
#if HAVE_CONST_DB_RDONLY
    rb_define_const(bdb_mDb, "RDONLY", INT2FIX(DB_RDONLY));
#endif
#if HAVE_CONST_DB_RECNUM
    rb_define_const(bdb_mDb, "RECNUM", INT2FIX(DB_RECNUM));
#endif
#if HAVE_CONST_DB_RECORDCOUNT
    rb_define_const(bdb_mDb, "RECORDCOUNT", INT2FIX(DB_RECORDCOUNT));
#endif
#if HAVE_CONST_DB_RECOVER
    rb_define_const(bdb_mDb, "RECOVER", INT2FIX(DB_RECOVER));
#endif
#if HAVE_CONST_DB_RECOVER_FATAL
    rb_define_const(bdb_mDb, "RECOVER_FATAL", INT2FIX(DB_RECOVER_FATAL));
#endif
#if HAVE_CONST_DB_RENUMBER
    rb_define_const(bdb_mDb, "RENUMBER", INT2FIX(DB_RENUMBER));
#endif
#if HAVE_CONST_DB_RMW
    rb_define_const(bdb_mDb, "RMW", INT2NUM(DB_RMW));
#else
    rb_define_const(bdb_mDb, "RMW", INT2NUM(0));
#endif
#if HAVE_CONST_DB_SALVAGE
    rb_define_const(bdb_mDb, "SALVAGE", INT2FIX(DB_SALVAGE));
#endif
#if HAVE_CONST_DB_SECONDARY_BAD
    rb_define_const(bdb_mDb, "SECONDARY_BAD", INT2FIX(DB_SECONDARY_BAD));
#else
    rb_define_const(bdb_mDb, "SECONDARY_BAD", INT2FIX(0));
#endif
#if HAVE_CONST_DB_SET
    rb_define_const(bdb_mDb, "SET", INT2FIX(DB_SET));
#endif
#if HAVE_CONST_DB_SET_RANGE
    rb_define_const(bdb_mDb, "SET_RANGE", INT2FIX(DB_SET_RANGE));
#endif
#if HAVE_CONST_DB_SET_RECNO
    rb_define_const(bdb_mDb, "SET_RECNO", INT2FIX(DB_SET_RECNO));
#endif
#if HAVE_CONST_DB_SNAPSHOT
    rb_define_const(bdb_mDb, "SNAPSHOT", INT2FIX(DB_SNAPSHOT));
#endif
#if HAVE_CONST_DB_STAT_CLEAR
    rb_define_const(bdb_mDb, "STAT_CLEAR", INT2FIX(DB_STAT_CLEAR));
#endif
#if ! HAVE_CONST_DB_SYSTEM_MEM
    rb_define_const(bdb_mDb, "SYSTEM_MEM", INT2FIX(0));
#else
    rb_define_const(bdb_mDb, "SYSTEM_MEM", INT2FIX(DB_SYSTEM_MEM));
#endif
#if HAVE_CONST_DB_THREAD
    rb_define_const(bdb_mDb, "THREAD", INT2FIX(DB_THREAD));
#endif
#if HAVE_CONST_DB_ENV_THREAD
    rb_define_const(bdb_mDb, "ENV_THREAD", INT2FIX(DB_ENV_THREAD));
#endif
#if HAVE_CONST_DB_TRUNCATE
    rb_define_const(bdb_mDb, "TRUNCATE", INT2FIX(DB_TRUNCATE));
#endif
#if HAVE_CONST_DB_TXN_ABORT
    rb_define_const(bdb_mDb, "TXN_ABORT", INT2FIX(DB_TXN_ABORT));
#endif
#if HAVE_CONST_DB_TXN_BACKWARD_ROLL
    rb_define_const(bdb_mDb, "TXN_BACKWARD_ROLL", INT2FIX(DB_TXN_BACKWARD_ROLL));
#endif
#if HAVE_CONST_DB_TXN_FORWARD_ROLL
    rb_define_const(bdb_mDb, "TXN_FORWARD_ROLL", INT2FIX(DB_TXN_FORWARD_ROLL));
#endif
#if HAVE_CONST_DB_TXN_NOSYNC
    rb_define_const(bdb_mDb, "TXN_NOSYNC", INT2FIX(DB_TXN_NOSYNC));
#endif
#if HAVE_CONST_DB_TXN_NOT_DURABLE
    rb_define_const(bdb_mDb, "TXN_NOT_DURABLE", INT2FIX(DB_TXN_NOT_DURABLE));
#endif
#if HAVE_CONST_DB_TXN_APPLY
    rb_define_const(bdb_mDb, "TXN_APPLY", INT2FIX(DB_TXN_APPLY));
#endif
#if HAVE_CONST_DB_TXN_PRINT
    rb_define_const(bdb_mDb, "TXN_PRINT", INT2FIX(DB_TXN_PRINT));
#endif
#if HAVE_CONST_DB_TXN_WRITE_NOSYNC
    rb_define_const(bdb_mDb, "TXN_WRITE_NOSYNC", INT2FIX(DB_TXN_WRITE_NOSYNC));
#endif
#if HAVE_CONST_DB_TXN_SNAPSHOT
    rb_define_const(bdb_mDb, "TXN_SNAPSHOT", INT2FIX(DB_TXN_SNAPSHOT));
#endif
#if HAVE_CONST_DB_UPGRADE
    rb_define_const(bdb_mDb, "UPGRADE", INT2FIX(DB_UPGRADE));
#endif
#if HAVE_CONST_DB_USE_ENVIRON
    rb_define_const(bdb_mDb, "USE_ENVIRON", INT2FIX(DB_USE_ENVIRON));
#endif
#if HAVE_CONST_DB_USE_ENVIRON_ROOT
    rb_define_const(bdb_mDb, "USE_ENVIRON_ROOT", INT2FIX(DB_USE_ENVIRON_ROOT));
#endif
#if HAVE_CONST_DB_TXN_NOWAIT
    rb_define_const(bdb_mDb, "TXN_NOWAIT", INT2FIX(DB_TXN_NOWAIT));
#else
    rb_define_const(bdb_mDb, "TXN_NOWAIT", INT2FIX(0));
#endif
#if HAVE_CONST_DB_TXN_SYNC
    rb_define_const(bdb_mDb, "TXN_SYNC", INT2FIX(DB_TXN_SYNC));
#else
    rb_define_const(bdb_mDb, "TXN_SYNC", INT2FIX(0));
#endif
#if HAVE_CONST_DB_VERB_CHKPOINT
    rb_define_const(bdb_mDb, "VERB_CHKPOINT", INT2FIX(DB_VERB_CHKPOINT));
#else
    rb_define_const(bdb_mDb, "VERB_CHKPOINT", INT2FIX(1));
#endif
#if HAVE_CONST_DB_VERB_DEADLOCK
    rb_define_const(bdb_mDb, "VERB_DEADLOCK", INT2FIX(DB_VERB_DEADLOCK));
#else
    rb_define_const(bdb_mDb, "VERB_DEADLOCK", INT2FIX(1));
#endif
#if HAVE_CONST_DB_VERB_RECOVERY
    rb_define_const(bdb_mDb, "VERB_RECOVERY", INT2FIX(DB_VERB_RECOVERY));
#else
    rb_define_const(bdb_mDb, "VERB_RECOVERY", INT2FIX(1));
#endif
#if HAVE_CONST_DB_VERB_WAITSFOR
    rb_define_const(bdb_mDb, "VERB_WAITSFOR", INT2FIX(DB_VERB_WAITSFOR));
#else
    rb_define_const(bdb_mDb, "VERB_WAITSFOR", INT2FIX(1));
#endif
#if HAVE_CONST_DB_WRITECURSOR
    rb_define_const(bdb_mDb, "WRITECURSOR", INT2FIX(DB_WRITECURSOR));
#else
    rb_define_const(bdb_mDb, "WRITECURSOR", INT2NUM(DB_RMW));
#endif
#if HAVE_CONST_DB_VERB_REPLICATION
    rb_define_const(bdb_mDb, "VERB_REPLICATION", INT2FIX(DB_VERB_REPLICATION));
#endif
#if HAVE_CONST_DB_VERIFY
    rb_define_const(bdb_mDb, "VERIFY", INT2FIX(DB_VERIFY));
#endif
#if HAVE_CONST_DB_XA_CREATE
    rb_define_const(bdb_mDb, "XA_CREATE", INT2FIX(DB_XA_CREATE));
#endif
#if HAVE_CONST_DB_XIDDATASIZE
    rb_define_const(bdb_mDb, "XIDDATASIZE", INT2FIX(DB_XIDDATASIZE));
#endif
#if HAVE_CONST_BDB_TXN_COMMIT
    rb_define_const(bdb_mDb, "TXN_COMMIT", INT2FIX(BDB_TXN_COMMIT));
#endif
#if HAVE_CONST_DB_REGION_INIT
    rb_define_const(bdb_mDb, "REGION_INIT", INT2FIX(DB_REGION_INIT));
#endif
#if HAVE_CONST_DB_AUTO_COMMIT
    rb_define_const(bdb_mDb, "AUTO_COMMIT", INT2FIX(DB_AUTO_COMMIT));
#endif
#if HAVE_CONST_DB_REP_CONF_LEASE
    rb_define_const(bdb_mDb, "REP_CONF_LEASE", INT2FIX(DB_REP_CONF_LEASE));
#endif
#if HAVE_CONST_DB_REP_HEARTBEAT_MONITOR
    rb_define_const(bdb_mDb, "DB_REP_HEARTBEAT_MONITOR", INT2FIX(DB_REP_HEARTBEAT_MONITOR));
#endif
#if HAVE_CONST_DB_REP_HEARTBEAT_SEND
    rb_define_const(bdb_mDb, "DB_REP_HEARTBEAT_SEND", INT2FIX(DB_REP_HEARTBEAT_SEND));
#endif
#if HAVE_CONST_DB_REP_CLIENT
    rb_define_const(bdb_mDb, "REP_CLIENT", INT2FIX(DB_REP_CLIENT));
#endif
#if HAVE_CONST_DB_REP_DUPMASTER
    rb_define_const(bdb_mDb, "REP_DUPMASTER", INT2FIX(DB_REP_DUPMASTER));
#endif
#if HAVE_CONST_DB_REP_HOLDELECTION
    rb_define_const(bdb_mDb, "REP_HOLDELECTION", INT2FIX(DB_REP_HOLDELECTION));
#endif
#if HAVE_CONST_DB_REP_MASTER
    rb_define_const(bdb_mDb, "REP_MASTER", INT2FIX(DB_REP_MASTER));
#endif
#if HAVE_CONST_DB_REP_NEWMASTER
    rb_define_const(bdb_mDb, "REP_NEWMASTER", INT2FIX(DB_REP_NEWMASTER));
#endif
#if HAVE_CONST_DB_REP_NEWSITE
    rb_define_const(bdb_mDb, "REP_NEWSITE", INT2FIX(DB_REP_NEWSITE));
#endif
#if HAVE_CONST_DB_REP_LOGSONLY
    rb_define_const(bdb_mDb, "REP_LOGSONLY", INT2FIX(DB_REP_LOGSONLY));
#endif
#if HAVE_CONST_DB_REP_OUTDATED
    rb_define_const(bdb_mDb, "REP_OUTDATED", INT2FIX(DB_REP_OUTDATED));
#endif
#if HAVE_CONST_DB_REP_PERMANENT
    rb_define_const(bdb_mDb, "REP_PERMANENT", INT2FIX(DB_REP_PERMANENT));
#endif
#if HAVE_CONST_DB_REP_UNAVAIL
    rb_define_const(bdb_mDb, "REP_UNAVAIL", INT2FIX(DB_REP_UNAVAIL));
#endif
#if HAVE_CONST_DB_REP_ISPERM
    rb_define_const(bdb_mDb, "REP_ISPERM", INT2FIX(DB_REP_ISPERM));
#endif
#if HAVE_CONST_DB_REP_NOTPERM
    rb_define_const(bdb_mDb, "REP_NOTPERM", INT2FIX(DB_REP_NOTPERM));
#endif
#if HAVE_CONST_DB_REP_IGNORE
    rb_define_const(bdb_mDb, "REP_IGNORE", INT2FIX(DB_REP_IGNORE));
#endif
#if HAVE_CONST_DB_REP_JOIN_FAILURE
    rb_define_const(bdb_mDb, "REP_JOIN_FAILURE", INT2FIX(DB_REP_JOIN_FAILURE));
#endif
#if HAVE_CONST_DB_EID_BROADCAST
    rb_define_const(bdb_mDb, "EID_BROADCAST", INT2FIX(DB_EID_BROADCAST));
#endif
#if HAVE_CONST_DB_EID_INVALID
    rb_define_const(bdb_mDb, "EID_INVALID", INT2FIX(DB_EID_INVALID));
#endif
#if HAVE_CONST_DB_SET_LOCK_TIMEOUT
    rb_define_const(bdb_mDb, "SET_LOCK_TIMEOUT", INT2FIX(DB_SET_LOCK_TIMEOUT));
#endif
#if HAVE_CONST_DB_SET_TXN_TIMEOUT
    rb_define_const(bdb_mDb, "SET_TXN_TIMEOUT", INT2FIX(DB_SET_TXN_TIMEOUT));
#endif
#if HAVE_CONST_DB_LOCK_GET_TIMEOUT
    rb_define_const(bdb_mDb, "LOCK_GET_TIMEOUT", INT2FIX(DB_LOCK_GET_TIMEOUT));
#endif
#if HAVE_CONST_DB_LOCK_TIMEOUT
    rb_define_const(bdb_mDb, "LOCK_TIMEOUT", INT2FIX(DB_LOCK_TIMEOUT));
#endif
#if HAVE_CONST_DB_ENCRYPT_AES
    rb_define_const(bdb_mDb, "ENCRYPT_AES", INT2FIX(DB_ENCRYPT_AES));
#endif
#if HAVE_CONST_DB_ENCRYPT
    rb_define_const(bdb_mDb, "ENCRYPT", INT2FIX(DB_ENCRYPT));
#endif
#if HAVE_CONST_DB_CHKSUM_SHA1
    rb_define_const(bdb_mDb, "CHKSUM_SHA1", INT2FIX(DB_CHKSUM_SHA1));
#endif
#if HAVE_CONST_DB_CHKSUM
    rb_define_const(bdb_mDb, "CHKSUM", INT2FIX(DB_CHKSUM));
#if ! HAVE_CONST_DB_CHKSUM_SHA1
    rb_define_const(bdb_mDb, "CHKSUM_SHA1", INT2FIX(DB_CHKSUM));
#endif
#endif
#if HAVE_CONST_DB_DIRECT_DB
    rb_define_const(bdb_mDb, "DIRECT_DB", INT2FIX(DB_DIRECT_DB));
#endif
#if HAVE_CONST_DB_DIRECT_LOG
    rb_define_const(bdb_mDb, "DIRECT_LOG", INT2FIX(DB_DIRECT_LOG));
#endif
#if HAVE_CONST_DB_DSYNC_LOG
    rb_define_const(bdb_mDb, "DSYNC_LOG", INT2FIX(DB_DSYNC_LOG));
#endif
#if HAVE_CONST_DB_LOG_INMEMORY
    rb_define_const(bdb_mDb, "LOG_INMEMORY", INT2FIX(DB_LOG_INMEMORY));
#endif
#if HAVE_CONST_DB_LOG_IN_MEMORY
    rb_define_const(bdb_mDb, "LOG_IN_MEMORY", INT2FIX(DB_LOG_IN_MEMORY));
#endif
#if HAVE_CONST_DB_LOG_AUTOREMOVE
    rb_define_const(bdb_mDb, "LOG_AUTOREMOVE", INT2FIX(DB_LOG_AUTOREMOVE));
#endif
#if HAVE_CONST_DB_LOG_AUTO_REMOVE
    rb_define_const(bdb_mDb, "LOG_AUTO_REMOVE", INT2FIX(DB_LOG_AUTO_REMOVE));
#endif
#if HAVE_CONST_DB_GET_BOTH_RANGE
    rb_define_const(bdb_mDb, "GET_BOTH_RANGE", INT2FIX(DB_GET_BOTH_RANGE));
#endif
#if HAVE_CONST_DB_INIT_REP
    rb_define_const(bdb_mDb, "INIT_REP", INT2FIX(DB_INIT_REP));
#endif
#if HAVE_CONST_DB_REP_NOBUFFER
    rb_define_const(bdb_mDb, "REP_NOBUFFER", INT2FIX(DB_REP_NOBUFFER));
#endif
#if HAVE_CONST_DB_MUTEX_PROCESS_ONLY
    rb_define_const(bdb_mDb, "MUTEX_PROCESS_ONLY", INT2FIX(DB_MUTEX_PROCESS_ONLY));
#endif
#if HAVE_CONST_DB_EVENT_PANIC
    rb_define_const(bdb_mDb, "EVENT_PANIC", INT2FIX(DB_EVENT_PANIC));
#endif
#if HAVE_CONST_DB_EVENT_REP_STARTUPDONE
    rb_define_const(bdb_mDb, "EVENT_REP_STARTUPDONE", INT2FIX(DB_EVENT_REP_STARTUPDONE));
#endif
#if HAVE_CONST_DB_EVENT_REP_CLIENT
    rb_define_const(bdb_mDb, "EVENT_REP_CLIENT", INT2FIX(DB_EVENT_REP_CLIENT));
#endif
#if HAVE_CONST_DB_EVENT_REP_ELECTED
    rb_define_const(bdb_mDb, "EVENT_REP_ELECTED", INT2FIX(DB_EVENT_REP_ELECTED));
#endif
#if HAVE_CONST_DB_EVENT_REP_MASTER
    rb_define_const(bdb_mDb, "EVENT_REP_MASTER", INT2FIX(DB_EVENT_REP_MASTER));
#endif
#if HAVE_CONST_DB_EVENT_REP_NEWMASTER
    rb_define_const(bdb_mDb, "EVENT_REP_NEWMASTER", INT2FIX(DB_EVENT_REP_NEWMASTER));
#endif
#if HAVE_CONST_DB_EVENT_REP_PERM_FAILED
    rb_define_const(bdb_mDb, "EVENT_REP_PERM_FAILED", INT2FIX(DB_EVENT_REP_PERM_FAILED));
#endif
#if HAVE_CONST_DB_EVENT_WRITE_FAILED
    rb_define_const(bdb_mDb, "EVENT_WRITE_FAILED", INT2FIX(DB_EVENT_WRITE_FAILED));
#endif
#if HAVE_CONST_DB_REP_CONF_BULK
    rb_define_const(bdb_mDb, "REP_CONF_BULK", INT2FIX(DB_REP_CONF_BULK));
#endif
#if HAVE_CONST_DB_REP_CONF_DELAYCLIENT
    rb_define_const(bdb_mDb, "REP_CONF_DELAYCLIENT", INT2FIX(DB_REP_CONF_DELAYCLIENT));
#endif
#if HAVE_CONST_DB_REP_CONF_NOAUTOINIT
    rb_define_const(bdb_mDb, "REP_CONF_NOAUTOINIT", INT2FIX(DB_REP_CONF_NOAUTOINIT));
#endif
#if HAVE_CONST_DB_REP_CONF_NOWAIT
    rb_define_const(bdb_mDb, "REP_CONF_NOWAIT", INT2FIX(DB_REP_CONF_NOWAIT));
#endif
#if HAVE_CONST_DB_REP_ACK_TIMEOUT
    rb_define_const(bdb_mDb, "REP_ACK_TIMEOUT", INT2FIX(DB_REP_ACK_TIMEOUT));
#endif
#if HAVE_CONST_DB_REP_ANYWHERE
    rb_define_const(bdb_mDb, "REP_ANYWHERE", INT2FIX(DB_REP_ANYWHERE));
#endif
#if HAVE_CONST_DB_REP_BULKOVF
    rb_define_const(bdb_mDb, "REP_BULKOVF", INT2FIX(DB_REP_BULKOVF));
#endif
#if HAVE_CONST_DB_REP_DEFAULT_PRIORITY
    rb_define_const(bdb_mDb, "REP_DEFAULT_PRIORITY", INT2FIX(DB_REP_DEFAULT_PRIORITY));
#endif
#if HAVE_CONST_DB_REP_EGENCHG
    rb_define_const(bdb_mDb, "REP_EGENCHG", INT2FIX(DB_REP_EGENCHG));
#endif
#if HAVE_CONST_DB_REPFLAGS_MASK
    rb_define_const(bdb_mDb, "REPFLAGS_MASK", INT2FIX(DB_REPFLAGS_MASK));
#endif
#if HAVE_CONST_DB_REP_FULL_ELECTION_TIMEOUT
    rb_define_const(bdb_mDb, "REP_FULL_ELECTION_TIMEOUT", INT2FIX(DB_REP_FULL_ELECTION_TIMEOUT));
#endif
#if HAVE_CONST_DB_REP_HANDLE_DEAD
    rb_define_const(bdb_mDb, "REP_HANDLE_DEAD", INT2FIX(DB_REP_HANDLE_DEAD));
#endif
#if HAVE_CONST_DB_REP_LEASE_EXPIRED
    rb_define_const(bdb_mDb, "REP_LEASE_EXPIRED", INT2FIX(DB_REP_LEASE_EXPIRED));
#endif
#if HAVE_CONST_DB_REP_LEASE_TIMEOUT
    rb_define_const(bdb_mDb, "REP_LEASE_TIMEOUT", INT2FIX(DB_REP_LEASE_TIMEOUT));
#endif
#if HAVE_CONST_DB_REP_LOCKOUT
    rb_define_const(bdb_mDb, "REP_LOCKOUT", INT2FIX(DB_REP_LOCKOUT));
#endif
#if HAVE_CONST_DB_REP_LOGREADY
    rb_define_const(bdb_mDb, "REP_LOGREADY", INT2FIX(DB_REP_LOGREADY));
#endif
#if HAVE_CONST_DB_REPMGR_CONNECTED
    rb_define_const(bdb_mDb, "REPMGR_CONNECTED", INT2FIX(DB_REPMGR_CONNECTED));
#endif
#if HAVE_CONST_DB_REPMGR_DISCONNECTED
    rb_define_const(bdb_mDb, "REPMGR_DISCONNECTED", INT2FIX(DB_REPMGR_DISCONNECTED));
#endif
#if HAVE_CONST_DB_REPMGR_PEER
    rb_define_const(bdb_mDb, "REPMGR_PEER", INT2FIX(DB_REPMGR_PEER));
#endif
#if HAVE_CONST_DB_REP_PAGEDONE
    rb_define_const(bdb_mDb, "REP_PAGEDONE", INT2FIX(DB_REP_PAGEDONE));
#endif
#if HAVE_CONST_DB_REP_REREQUEST
    rb_define_const(bdb_mDb, "REP_REREQUEST", INT2FIX(DB_REP_REREQUEST));
#endif
#if HAVE_CONST_DB_REPMGR_ACKS_ALL
    rb_define_const(bdb_mDb, "REPMGR_ACKS_ALL", INT2FIX(DB_REPMGR_ACKS_ALL));
#endif
#if HAVE_CONST_DB_REPMGR_ACKS_ALL_PEERS
    rb_define_const(bdb_mDb, "REPMGR_ACKS_ALL_PEERS", INT2FIX(DB_REPMGR_ACKS_ALL_PEERS));
#endif
#if HAVE_CONST_DB_REPMGR_ACKS_NONE
    rb_define_const(bdb_mDb, "REPMGR_ACKS_NONE", INT2FIX(DB_REPMGR_ACKS_NONE));
#endif
#if HAVE_CONST_DB_REPMGR_ACKS_ONE
    rb_define_const(bdb_mDb, "REPMGR_ACKS_ONE", INT2FIX(DB_REPMGR_ACKS_ONE));
#endif
#if HAVE_CONST_DB_REPMGR_ACKS_ONE_PEER
    rb_define_const(bdb_mDb, "REPMGR_ACKS_ONE_PEER", INT2FIX(DB_REPMGR_ACKS_ONE_PEER));
#endif
#if HAVE_CONST_DB_REPMGR_ACKS_QUORUM
    rb_define_const(bdb_mDb, "REPMGR_ACKS_QUORUM", INT2FIX(DB_REPMGR_ACKS_QUORUM));
#endif
#if HAVE_CONST_DB_REP_ELECTION
    rb_define_const(bdb_mDb, "REP_ELECTION", INT2FIX(DB_REP_ELECTION));
#endif
#if HAVE_CONST_DB_REP_FULL_ELECTION
    rb_define_const(bdb_mDb, "REP_FULL_ELECTION", INT2FIX(DB_REP_FULL_ELECTION));
#endif
#if HAVE_CONST_DB_REP_ELECTION_TIMEOUT
    rb_define_const(bdb_mDb, "REP_ELECTION_TIMEOUT", INT2FIX(DB_REP_ELECTION_TIMEOUT));
#endif
#if HAVE_CONST_DB_REP_ELECTION_RETRY
    rb_define_const(bdb_mDb, "REP_ELECTION_RETRY", INT2FIX(DB_REP_ELECTION_RETRY));
#endif
#if HAVE_CONST_DB_REP_CONNECTION_RETRY
    rb_define_const(bdb_mDb, "REP_CONNECTION_RETRY", INT2FIX(DB_REP_CONNECTION_RETRY));
#endif
#if HAVE_CONST_DB_REP_CHECKPOINT_DELAY
    rb_define_const(bdb_mDb, "REP_CHECKPOINT_DELAY", INT2FIX(DB_REP_CHECKPOINT_DELAY));
#endif
#if HAVE_CONST_DB_IGNORE_LEASE
    rb_define_const(bdb_mDb, "IGNORE_LEASE", INT2FIX(DB_IGNORE_LEASE));
#endif
#if HAVE_CONST_DB_PRIORITY_VERY_LOW
    rb_define_const(bdb_mDb, "PRIORITY_VERY_LOW", INT2FIX(DB_PRIORITY_VERY_LOW));
#endif
#if HAVE_CONST_DB_PRIORITY_LOW
    rb_define_const(bdb_mDb, "PRIORITY_LOW", INT2FIX(DB_PRIORITY_LOW));
#endif
#if HAVE_CONST_DB_PRIORITY_DEFAULT
    rb_define_const(bdb_mDb, "PRIORITY_DEFAULT", INT2FIX(DB_PRIORITY_DEFAULT));
#endif
#if HAVE_CONST_DB_PRIORITY_HIGH
    rb_define_const(bdb_mDb, "PRIORITY_HIGH", INT2FIX(DB_PRIORITY_HIGH));
#endif
#if HAVE_CONST_DB_PRIORITY_VERY_HIGH
    rb_define_const(bdb_mDb, "PRIORITY_VERY_HIGH", INT2FIX(DB_PRIORITY_VERY_HIGH));
#endif

#if HAVE_CONST_DB_LOG_DIRECT
    rb_define_const(bdb_mDb, "LOG_DIRECT", INT2FIX(DB_LOG_DIRECT));
#endif
#if HAVE_CONST_DB_LOG_DSYNC
    rb_define_const(bdb_mDb, "LOG_DSYNC", INT2FIX(DB_LOG_DSYNC));
#endif
#if HAVE_CONST_DB_LOG_ZERO
    rb_define_const(bdb_mDb, "LOG_ZERO", INT2FIX(DB_LOG_ZERO));
#endif

    bdb_init_env();
    bdb_init_common();
    bdb_init_recnum();
    bdb_init_transaction();
    bdb_init_cursor();
    bdb_init_lock();
    bdb_init_log();
    bdb_init_delegator();
    bdb_init_sequence();

    bdb_errstr = rb_tainted_str_new(0, 0);
    rb_global_variable(&bdb_errstr);
    
}
