#include <ruby.h>
#include <db.h>
#include <errno.h>

#include "bdb.h"

VALUE bdb_cEnv;
VALUE bdb_eFatal;
VALUE bdb_eLock, bdb_eLockDead, bdb_eLockHeld, bdb_eLockGranted;
#if DB_VERSION_MAJOR >= 4
VALUE bdb_eRepUnavail;
#endif
VALUE bdb_mDb;
VALUE bdb_cCommon, bdb_cBtree, bdb_cRecnum, bdb_cHash, bdb_cRecno, bdb_cUnknown;
VALUE bdb_cDelegate;

#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1) || DB_VERSION_MAJOR >= 4
VALUE bdb_sKeyrange;
#endif

#if DB_VERSION_MAJOR >= 3
VALUE bdb_cQueue;
#endif

VALUE bdb_cTxn, bdb_cTxnCatch;
VALUE bdb_cCursor;
VALUE bdb_cLock, bdb_cLockid;
VALUE bdb_cLsn;

VALUE bdb_mMarshal;

ID id_dump, id_load;
ID id_current_db;

VALUE bdb_errstr;
int bdb_errcall = 0;

#if DB_VERSION_MAJOR < 3

char *
db_strerror(int err)
{
    if (err == 0)
        return "" ;

    if (err > 0)
        return strerror(err) ;

    switch (err) {
#ifdef DB_INCOMPLETE
    case DB_INCOMPLETE:
        return ("DB_INCOMPLETE: Sync was unable to complete");
#endif
    case DB_KEYEMPTY:
        return ("DB_KEYEMPTY: Non-existent key/data pair");
    case DB_KEYEXIST:
        return ("DB_KEYEXIST: Key/data pair already exists");
    case DB_LOCK_DEADLOCK:
        return ("DB_LOCK_DEADLOCK: Locker killed to resolve a deadlock");
    case DB_LOCK_NOTGRANTED:
        return ("DB_LOCK_NOTGRANTED: Lock not granted");
    case DB_LOCK_NOTHELD:
        return ("DB_LOCK_NOTHELD: Lock not held by locker");
    case DB_NOTFOUND:
        return ("DB_NOTFOUND: No matching key/data pair found");
#if DB_VERSION_MINOR >= 6
    case DB_RUNRECOVERY:
        return ("DB_RUNRECOVERY: Fatal error, run database recovery");
#endif
#ifdef DB_OLD_VERSION
    case DB_OLD_VERSION:
        return ("DB_OLD_VERSION: The database cannot be opened without being first upgraded");
#endif
    default:
        return "Unknown Error" ;
    }
}

#endif

int
bdb_test_error(comm)
    int comm;
{
    VALUE error;

    switch (comm) {
    case 0:
    case DB_NOTFOUND:
    case DB_KEYEMPTY:
    case DB_KEYEXIST:
	return comm;
        break;
#ifdef DB_INCOMPLETE
    case DB_INCOMPLETE:
	comm = 0;
	return comm;
        break;
#endif
#if DB_VERSION_MAJOR >= 4
    case DB_REP_UNAVAIL:
	error = bdb_eRepUnavail;
	break;
#endif
    case DB_LOCK_DEADLOCK:
    case EAGAIN:
	error = bdb_eLockDead;
	break;
    case DB_LOCK_NOTGRANTED:
	error = bdb_eLockGranted;
	break;
#if DB_VERSION_MAJOR < 3
    case DB_LOCK_NOTHELD:
	error = bdb_eLockHeld;
	break;
#endif
#if (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3) || DB_VERSION_MAJOR >= 4
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
	    rb_raise(error, "%s -- %s", RSTRING(bdb_errstr)->ptr, db_strerror(comm));
	}
	else {
	    rb_raise(error, "%s", RSTRING(bdb_errstr)->ptr);
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
    id_current_db = rb_intern("bdb_current_db");
    id_dump = rb_intern("dump");
    id_load = rb_intern("load");
    bdb_mDb = rb_define_module("BDB");
    bdb_eFatal = rb_define_class_under(bdb_mDb, "Fatal", rb_eStandardError);
    bdb_eLock = rb_define_class_under(bdb_mDb, "LockError", bdb_eFatal);
    bdb_eLockDead = rb_define_class_under(bdb_mDb, "LockDead", bdb_eLock);
    bdb_eLockHeld = rb_define_class_under(bdb_mDb, "LockHeld", bdb_eLock);
    bdb_eLockGranted = rb_define_class_under(bdb_mDb, "LockGranted",  bdb_eLock);
#if DB_VERSION_MAJOR >= 4
    bdb_eRepUnavail = rb_define_class_under(bdb_mDb, "RepUnavail", bdb_eFatal);
#endif
/* CONSTANT */
    rb_define_const(bdb_mDb, "VERSION", version);
    rb_define_const(bdb_mDb, "VERSION_MAJOR", INT2FIX(major));
    rb_define_const(bdb_mDb, "VERSION_MINOR", INT2FIX(minor));
    rb_define_const(bdb_mDb, "VERSION_PATCH", INT2FIX(patch));
    rb_define_const(bdb_mDb, "BTREE", INT2FIX(DB_BTREE));
    rb_define_const(bdb_mDb, "HASH", INT2FIX(DB_HASH));
    rb_define_const(bdb_mDb, "RECNO", INT2FIX(DB_RECNO));
#if DB_VERSION_MAJOR < 3
    rb_define_const(bdb_mDb, "QUEUE", INT2FIX(0));
#else
    rb_define_const(bdb_mDb, "QUEUE", INT2FIX(DB_QUEUE));
#endif
    rb_define_const(bdb_mDb, "UNKNOWN", INT2FIX(DB_UNKNOWN));
    rb_define_const(bdb_mDb, "AFTER", INT2FIX(DB_AFTER));
#ifdef DB_AGGRESSIVE
    rb_define_const(bdb_mDb, "AGGRESSIVE", INT2FIX(DB_AGGRESSIVE));
#endif
    rb_define_const(bdb_mDb, "APPEND", INT2FIX(DB_APPEND));
    rb_define_const(bdb_mDb, "ARCH_ABS", INT2FIX(DB_ARCH_ABS));
    rb_define_const(bdb_mDb, "ARCH_DATA", INT2FIX(DB_ARCH_DATA));
    rb_define_const(bdb_mDb, "ARCH_LOG", INT2FIX(DB_ARCH_LOG));
    rb_define_const(bdb_mDb, "BEFORE", INT2FIX(DB_BEFORE));
#ifdef DB_CACHED_COUNTS
    rb_define_const(bdb_mDb, "CACHED_COUNTS", INT2FIX(DB_CACHED_COUNTS));
#endif
#ifdef DB_CDB_ALLDB
    rb_define_const(bdb_mDb, "CDB_ALLDB", INT2FIX(DB_CDB_ALLDB));
#endif
#ifdef DB_CHECKPOINT
    rb_define_const(bdb_mDb, "CHECKPOINT", INT2FIX(DB_CHECKPOINT));
#endif
#ifdef DB_CLIENT
    rb_define_const(bdb_mDb, "CLIENT", INT2FIX(DB_CLIENT));
#endif
#ifdef DB_CONFIG
    rb_define_const(bdb_mDb, "CONFIG", INT2FIX(DB_CONFIG));
#endif
#if DB_VERSION_MAJOR < 3
    rb_define_const(bdb_mDb, "CONSUME", INT2FIX(0));
#else
    rb_define_const(bdb_mDb, "CONSUME", INT2FIX(DB_CONSUME));
#endif
#ifdef DB_CONSUME_WAIT
    rb_define_const(bdb_mDb, "CONSUME_WAIT", INT2FIX(DB_CONSUME_WAIT));
#endif
    rb_define_const(bdb_mDb, "CREATE", INT2FIX(DB_CREATE));
    rb_define_const(bdb_mDb, "CURLSN", INT2FIX(DB_CURLSN));
    rb_define_const(bdb_mDb, "CURRENT", INT2FIX(DB_CURRENT));
#ifdef DB_DIRTY_READ
    rb_define_const(bdb_mDb, "DIRTY_READ", INT2FIX(DB_DIRTY_READ));
#else
    rb_define_const(bdb_mDb, "DIRTY_READ", INT2FIX(0));
#endif
    rb_define_const(bdb_mDb, "DBT_MALLOC", INT2FIX(DB_DBT_MALLOC));
    rb_define_const(bdb_mDb, "DBT_PARTIAL", INT2FIX(DB_DBT_PARTIAL));
#if DB_VERSION_MAJOR < 3
    rb_define_const(bdb_mDb, "DBT_REALLOC", INT2FIX(0));
#else
    rb_define_const(bdb_mDb, "DBT_REALLOC", INT2FIX(DB_DBT_REALLOC));
#endif
    rb_define_const(bdb_mDb, "DBT_USERMEM", INT2FIX(DB_DBT_USERMEM));
#ifdef DB_DONOTINDEX
    rb_define_const(bdb_mDb, "DONOTINDEX", INT2FIX(DB_DONOTINDEX));
#endif
    rb_define_const(bdb_mDb, "DUP", INT2FIX(DB_DUP));
#ifdef DB_DUPSORT
    rb_define_const(bdb_mDb, "DUPSORT", INT2FIX(DB_DUPSORT));
#endif
#ifdef DB_EXCL
    rb_define_const(bdb_mDb, "EXCL", INT2FIX(DB_EXCL));
#endif
#ifdef DB_FAST_STAT
    rb_define_const(bdb_mDb, "FAST_STAT", INT2FIX(DB_FAST_STAT));
#endif
    rb_define_const(bdb_mDb, "FIRST", INT2FIX(DB_FIRST));
#ifdef DB_FIXEDLEN
    rb_define_const(bdb_mDb, "FIXEDLEN", INT2FIX(DB_FIXEDLEN));
#endif
    rb_define_const(bdb_mDb, "FLUSH", INT2FIX(DB_FLUSH));
#if DB_VERSION_MAJOR < 3
    rb_define_const(bdb_mDb, "FORCE", INT2FIX(1));
#else
    rb_define_const(bdb_mDb, "FORCE", INT2FIX(DB_FORCE));
#endif
#if DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6
    rb_define_const(bdb_mDb, "GET_BOTH", INT2FIX(9));
#else
    rb_define_const(bdb_mDb, "GET_BOTH", INT2FIX(DB_GET_BOTH));
#endif
    rb_define_const(bdb_mDb, "GET_RECNO", INT2FIX(DB_GET_RECNO));
#ifdef DB_HOME
    rb_define_const(bdb_mDb, "HOME", INT2FIX(DB_HOME));
#endif
#ifdef DB_INCOMPLETE
    rb_define_const(bdb_mDb, "INCOMPLETE", INT2FIX(DB_INCOMPLETE));
#endif
#ifdef DB_INIT_CDB
    rb_define_const(bdb_mDb, "INIT_CDB", INT2FIX(DB_INIT_CDB));
#endif
    rb_define_const(bdb_mDb, "INIT_LOCK", INT2FIX(DB_INIT_LOCK));
    rb_define_const(bdb_mDb, "INIT_LOG", INT2FIX(DB_INIT_LOG));
    rb_define_const(bdb_mDb, "INIT_MPOOL", INT2FIX(DB_INIT_MPOOL));
    rb_define_const(bdb_mDb, "INIT_TXN", INT2FIX(DB_INIT_TXN));
    rb_define_const(bdb_mDb, "INIT_TRANSACTION", INT2FIX(BDB_INIT_TRANSACTION));
    rb_define_const(bdb_mDb, "INIT_LOMP", INT2FIX(BDB_INIT_LOMP));
#ifdef DB_JOINENV
    rb_define_const(bdb_mDb, "JOINENV", INT2FIX(DB_JOINENV));
#endif
#ifdef DB_JOIN_ITEM
    rb_define_const(bdb_mDb, "JOIN_ITEM", INT2FIX(DB_JOIN_ITEM));
#endif
#ifdef DB_JOIN_NOSORT
    rb_define_const(bdb_mDb, "JOIN_NOSORT", INT2FIX(DB_JOIN_NOSORT));
#endif
    rb_define_const(bdb_mDb, "KEYFIRST", INT2FIX(DB_KEYFIRST));
    rb_define_const(bdb_mDb, "KEYLAST", INT2FIX(DB_KEYLAST));
    rb_define_const(bdb_mDb, "LAST", INT2FIX(DB_LAST));
#ifdef DB_LOCK_CONFLICT
    rb_define_const(bdb_mDb, "LOCK_CONFLICT", INT2FIX(DB_LOCK_CONFLICT));
#else
    rb_define_const(bdb_mDb, "LOCK_CONFLICT", INT2FIX(0));
#endif
    rb_define_const(bdb_mDb, "LOCK_DEADLOCK", INT2FIX(DB_LOCK_DEADLOCK));
    rb_define_const(bdb_mDb, "LOCK_DEFAULT", INT2FIX(DB_LOCK_DEFAULT));
    rb_define_const(bdb_mDb, "LOCK_GET", INT2FIX(DB_LOCK_GET));
    rb_define_const(bdb_mDb, "LOCK_NOTGRANTED", INT2FIX(DB_LOCK_NOTGRANTED));
    rb_define_const(bdb_mDb, "LOCK_NOWAIT", INT2FIX(DB_LOCK_NOWAIT));
    rb_define_const(bdb_mDb, "LOCK_OLDEST", INT2FIX(DB_LOCK_OLDEST));
    rb_define_const(bdb_mDb, "LOCK_PUT", INT2FIX(DB_LOCK_PUT));
    rb_define_const(bdb_mDb, "LOCK_PUT_ALL", INT2FIX(DB_LOCK_PUT_ALL));
    rb_define_const(bdb_mDb, "LOCK_PUT_OBJ", INT2FIX(DB_LOCK_PUT_OBJ));
    rb_define_const(bdb_mDb, "LOCK_RANDOM", INT2FIX(DB_LOCK_RANDOM));
    rb_define_const(bdb_mDb, "LOCK_YOUNGEST", INT2FIX(DB_LOCK_YOUNGEST));
    rb_define_const(bdb_mDb, "LOCK_NG", INT2FIX(DB_LOCK_NG));
    rb_define_const(bdb_mDb, "LOCK_READ", INT2FIX(DB_LOCK_READ));
    rb_define_const(bdb_mDb, "LOCK_WRITE", INT2FIX(DB_LOCK_WRITE));
    rb_define_const(bdb_mDb, "LOCK_IWRITE", INT2FIX(DB_LOCK_IWRITE));
    rb_define_const(bdb_mDb, "LOCK_IREAD", INT2FIX(DB_LOCK_IREAD));
    rb_define_const(bdb_mDb, "LOCK_IWR", INT2FIX(DB_LOCK_IWR));
#ifdef DB_LOCKDOWN
    rb_define_const(bdb_mDb, "LOCKDOWN", INT2FIX(DB_LOCKDOWN));
#else
    rb_define_const(bdb_mDb, "LOCKDOWN", INT2FIX(0));
#endif
#ifdef DB_LOCK_EXPIRE
    rb_define_const(bdb_mDb, "LOCK_EXPIRE", INT2FIX(DB_LOCK_EXPIRE));
#endif
#ifdef DB_LOCK_MAXLOCKS
    rb_define_const(bdb_mDb, "LOCK_MAXLOCKS", INT2FIX(DB_LOCK_MAXLOCKS));
#endif
#ifdef DB_LOCK_MINLOCKS
    rb_define_const(bdb_mDb, "LOCK_MINLOCKS", INT2FIX(DB_LOCK_MINLOCKS));
#endif
#ifdef DB_LOCK_MINWRITE
    rb_define_const(bdb_mDb, "LOCK_MINWRITE", INT2FIX(DB_LOCK_MINWRITE));
#endif
    rb_define_const(bdb_mDb, "MPOOL_CLEAN", INT2FIX(DB_MPOOL_CLEAN));
    rb_define_const(bdb_mDb, "MPOOL_CREATE", INT2FIX(DB_MPOOL_CREATE));
    rb_define_const(bdb_mDb, "MPOOL_DIRTY", INT2FIX(DB_MPOOL_DIRTY));
    rb_define_const(bdb_mDb, "MPOOL_DISCARD", INT2FIX(DB_MPOOL_DISCARD));
    rb_define_const(bdb_mDb, "MPOOL_LAST", INT2FIX(DB_MPOOL_LAST));
    rb_define_const(bdb_mDb, "MPOOL_NEW", INT2FIX(DB_MPOOL_NEW));
#ifdef DB_MPOOL_PRIVATE
    rb_define_const(bdb_mDb, "MPOOL_PRIVATE", INT2FIX(DB_MPOOL_PRIVATE));
#endif
#ifdef DB_OVERWRITE
    rb_define_const(bdb_mDb, "OVERWRITE", INT2FIX(DB_OVERWRITE));
#endif
#ifdef DB_PRINTABLE
    rb_define_const(bdb_mDb, "PRINTABLE", INT2FIX(DB_PRINTABLE));
#endif
    rb_define_const(bdb_mDb, "NEXT", INT2FIX(DB_NEXT));
#if DB_NEXT_DUP
    rb_define_const(bdb_mDb, "NEXT_DUP", INT2FIX(DB_NEXT_DUP));
#endif
#ifdef DB_NEXT_NODUP
    rb_define_const(bdb_mDb, "NEXT_NODUP", INT2FIX(DB_NEXT_NODUP));
#endif
#ifdef DB_NODUPDATA
    rb_define_const(bdb_mDb, "NODUPDATA", INT2FIX(DB_NODUPDATA));
#endif
    rb_define_const(bdb_mDb, "NOMMAP", INT2FIX(DB_NOMMAP));
#ifdef DB_NOORDERCHK
    rb_define_const(bdb_mDb, "NOORDERCHK", INT2FIX(DB_NOORDERCHK));
#endif
    rb_define_const(bdb_mDb, "NOOVERWRITE", INT2FIX(DB_NOOVERWRITE));
#ifdef DB_NOSERVER
    rb_define_const(bdb_mDb, "NOSERVER", INT2FIX(DB_NOSERVER));
#endif
#ifdef DB_NOSERVER_HOME
    rb_define_const(bdb_mDb, "NOSERVER_HOME", INT2FIX(DB_NOSERVER_HOME));
#endif
#ifdef DB_NOSERVER_ID
    rb_define_const(bdb_mDb, "NOSERVER_ID", INT2FIX(DB_NOSERVER_ID));
#endif
    rb_define_const(bdb_mDb, "NOSYNC", INT2FIX(DB_NOSYNC));
#ifdef DB_OLD_VERSION
    rb_define_const(bdb_mDb, "OLD_VERSION", INT2FIX(DB_OLD_VERSION));
#endif
#ifdef DB_ORDERCHKONLY
    rb_define_const(bdb_mDb, "ORDERCHKONLY", INT2FIX(DB_ORDERCHKONLY));
#endif
#ifdef DB_PAD
    rb_define_const(bdb_mDb, "PAD", INT2FIX(DB_PAD));
#endif
#if DB_VERSION_MAJOR < 3
    rb_define_const(bdb_mDb, "POSITION", INT2FIX(0));
#else
    rb_define_const(bdb_mDb, "POSITION", INT2FIX(DB_POSITION));
#endif
    rb_define_const(bdb_mDb, "PREV", INT2FIX(DB_PREV));
#ifdef DB_PREV_NODUP
    rb_define_const(bdb_mDb, "PREV_NODUP", INT2FIX(DB_PREV_NODUP));
#endif
#if DB_VERSION_MAJOR < 3
    rb_define_const(bdb_mDb, "PRIVATE", INT2FIX(0));
#else
    rb_define_const(bdb_mDb, "PRIVATE", INT2FIX(DB_PRIVATE));
#endif
    rb_define_const(bdb_mDb, "RDONLY", INT2FIX(DB_RDONLY));
    rb_define_const(bdb_mDb, "RECNUM", INT2FIX(DB_RECNUM));
    rb_define_const(bdb_mDb, "RECORDCOUNT", INT2FIX(DB_RECORDCOUNT));
    rb_define_const(bdb_mDb, "RECOVER", INT2FIX(DB_RECOVER));
    rb_define_const(bdb_mDb, "RECOVER_FATAL", INT2FIX(DB_RECOVER_FATAL));
    rb_define_const(bdb_mDb, "RENUMBER", INT2FIX(DB_RENUMBER));
    rb_define_const(bdb_mDb, "RMW", INT2FIX(DB_RMW));
#ifdef DB_SALVAGE
    rb_define_const(bdb_mDb, "SALVAGE", INT2FIX(DB_SALVAGE));
#endif
#ifdef DB_SECONDARY_BAD
    rb_define_const(bdb_mDb, "SECONDARY_BAD", INT2FIX(DB_SECONDARY_BAD));
#else
    rb_define_const(bdb_mDb, "SECONDARY_BAD", INT2FIX(0));
#endif
    rb_define_const(bdb_mDb, "SET", INT2FIX(DB_SET));
    rb_define_const(bdb_mDb, "SET_RANGE", INT2FIX(DB_SET_RANGE));
    rb_define_const(bdb_mDb, "SET_RECNO", INT2FIX(DB_SET_RECNO));
    rb_define_const(bdb_mDb, "SNAPSHOT", INT2FIX(DB_SNAPSHOT));
#ifdef DB_STAT_CLEAR
    rb_define_const(bdb_mDb, "STAT_CLEAR", INT2FIX(DB_STAT_CLEAR));
#endif
#if DB_VERSION_MAJOR < 3
    rb_define_const(bdb_mDb, "SYSTEM_MEM", INT2FIX(0));
#else
    rb_define_const(bdb_mDb, "SYSTEM_MEM", INT2FIX(DB_SYSTEM_MEM));
#endif
    rb_define_const(bdb_mDb, "THREAD", INT2FIX(DB_THREAD));
    rb_define_const(bdb_mDb, "TRUNCATE", INT2FIX(DB_TRUNCATE));
#if DB_VERSION_MAJOR > 3 || (DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 1)
    rb_define_const(bdb_mDb, "TXN_ABORT", INT2FIX(DB_TXN_ABORT));
#endif
    rb_define_const(bdb_mDb, "TXN_BACKWARD_ROLL", INT2FIX(DB_TXN_BACKWARD_ROLL));
    rb_define_const(bdb_mDb, "TXN_FORWARD_ROLL", INT2FIX(DB_TXN_FORWARD_ROLL));
    rb_define_const(bdb_mDb, "TXN_NOSYNC", INT2FIX(DB_TXN_NOSYNC));
#if DB_VERSION_MAJOR >= 4
    rb_define_const(bdb_mDb, "TXN_APPLY", INT2FIX(DB_TXN_APPLY));
#if DB_VERSION_MINOR >= 1
    rb_define_const(bdb_mDb, "TXN_PRINT", INT2FIX(DB_TXN_PRINT));
    rb_define_const(bdb_mDb, "TXN_WRITE_NOSYNC", INT2FIX(DB_TXN_WRITE_NOSYNC));
#endif
#endif
#ifdef DB_UPGRADE
    rb_define_const(bdb_mDb, "UPGRADE", INT2FIX(DB_UPGRADE));
#endif
    rb_define_const(bdb_mDb, "USE_ENVIRON", INT2FIX(DB_USE_ENVIRON));
    rb_define_const(bdb_mDb, "USE_ENVIRON_ROOT", INT2FIX(DB_USE_ENVIRON_ROOT));
#if DB_VERSION_MAJOR < 3
    rb_define_const(bdb_mDb, "TXN_NOWAIT", INT2FIX(0));
    rb_define_const(bdb_mDb, "TXN_SYNC", INT2FIX(0));
    rb_define_const(bdb_mDb, "VERB_CHKPOINT", INT2FIX(1));
    rb_define_const(bdb_mDb, "VERB_DEADLOCK", INT2FIX(1));
    rb_define_const(bdb_mDb, "VERB_RECOVERY", INT2FIX(1));
    rb_define_const(bdb_mDb, "VERB_WAITSFOR", INT2FIX(1));
    rb_define_const(bdb_mDb, "WRITECURSOR", INT2FIX(0));
#else
    rb_define_const(bdb_mDb, "TXN_NOWAIT", INT2FIX(DB_TXN_NOWAIT));
    rb_define_const(bdb_mDb, "TXN_SYNC", INT2FIX(DB_TXN_SYNC));
    rb_define_const(bdb_mDb, "VERB_CHKPOINT", INT2FIX(DB_VERB_CHKPOINT));
    rb_define_const(bdb_mDb, "VERB_DEADLOCK", INT2FIX(DB_VERB_DEADLOCK));
    rb_define_const(bdb_mDb, "VERB_RECOVERY", INT2FIX(DB_VERB_RECOVERY));
    rb_define_const(bdb_mDb, "VERB_WAITSFOR", INT2FIX(DB_VERB_WAITSFOR));
    rb_define_const(bdb_mDb, "WRITECURSOR", INT2FIX(DB_WRITECURSOR));
#if DB_VERSION_MAJOR >= 4
    rb_define_const(bdb_mDb, "VERB_REPLICATION", INT2FIX(DB_VERB_REPLICATION));
#endif
#endif
#ifdef DB_VERIFY
    rb_define_const(bdb_mDb, "VERIFY", INT2FIX(DB_VERIFY));
#endif
#ifdef DB_XA_CREATE
    rb_define_const(bdb_mDb, "XA_CREATE", INT2FIX(DB_XA_CREATE));
#endif
#ifdef DB_XIDDATASIZE
    rb_define_const(bdb_mDb, "XIDDATASIZE", INT2FIX(DB_XIDDATASIZE));
#endif
    rb_define_const(bdb_mDb, "TXN_COMMIT", INT2FIX(BDB_TXN_COMMIT));
#ifdef DB_REGION_INIT
    rb_define_const(bdb_mDb, "REGION_INIT", INT2FIX(DB_REGION_INIT));
#endif
#ifdef DB_AUTO_COMMIT
    rb_define_const(bdb_mDb, "AUTO_COMMIT", INT2FIX(DB_AUTO_COMMIT));
#endif
#if DB_VERSION_MAJOR >= 4
    rb_define_const(bdb_mDb, "REP_CLIENT", INT2FIX(DB_REP_CLIENT));
    rb_define_const(bdb_mDb, "REP_DUPMASTER", INT2FIX(DB_REP_DUPMASTER));
    rb_define_const(bdb_mDb, "REP_HOLDELECTION", INT2FIX(DB_REP_HOLDELECTION));
    rb_define_const(bdb_mDb, "REP_LOGSONLY", INT2FIX(DB_REP_LOGSONLY));
    rb_define_const(bdb_mDb, "REP_MASTER", INT2FIX(DB_REP_MASTER));
    rb_define_const(bdb_mDb, "REP_NEWMASTER", INT2FIX(DB_REP_NEWMASTER));
    rb_define_const(bdb_mDb, "REP_NEWSITE", INT2FIX(DB_REP_NEWSITE));
    rb_define_const(bdb_mDb, "REP_OUTDATED", INT2FIX(DB_REP_OUTDATED));
    rb_define_const(bdb_mDb, "REP_PERMANENT", INT2FIX(DB_REP_PERMANENT));
    rb_define_const(bdb_mDb, "REP_UNAVAIL", INT2FIX(DB_REP_UNAVAIL));
    rb_define_const(bdb_mDb, "EID_BROADCAST", INT2FIX(DB_EID_BROADCAST));
    rb_define_const(bdb_mDb, "EID_INVALID", INT2FIX(DB_EID_INVALID));
    rb_define_const(bdb_mDb, "SET_LOCK_TIMEOUT", INT2FIX(DB_SET_LOCK_TIMEOUT));
    rb_define_const(bdb_mDb, "SET_TXN_TIMEOUT", INT2FIX(DB_SET_TXN_TIMEOUT));
    rb_define_const(bdb_mDb, "LOCK_GET_TIMEOUT", INT2FIX(DB_LOCK_GET_TIMEOUT));
    rb_define_const(bdb_mDb, "LOCK_TIMEOUT", INT2FIX(DB_LOCK_TIMEOUT));
#endif
#ifdef DB_ENCRYPT_AES
    rb_define_const(bdb_mDb, "ENCRYPT_AES", INT2FIX(DB_ENCRYPT_AES));
#endif
#ifdef DB_ENCRYPT
    rb_define_const(bdb_mDb, "ENCRYPT", INT2FIX(DB_ENCRYPT));
#endif
#ifdef DB_CHKSUM_SHA1
    rb_define_const(bdb_mDb, "CHKSUM_SHA1", INT2FIX(DB_CHKSUM_SHA1));
#endif
#ifdef DB_DIRECT_DB
    rb_define_const(bdb_mDb, "DIRECT_DB", INT2FIX(DB_DIRECT_DB));
#endif
#ifdef DB_DIRECT_LOG
    rb_define_const(bdb_mDb, "DIRECT_LOG", INT2FIX(DB_DIRECT_LOG));
#endif
#if DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 1
    rb_define_const(bdb_mDb, "PRIORITY_VERY_LOW", INT2FIX(DB_PRIORITY_VERY_LOW));
    rb_define_const(bdb_mDb, "PRIORITY_LOW", INT2FIX(DB_PRIORITY_LOW));
    rb_define_const(bdb_mDb, "PRIORITY_DEFAULT", INT2FIX(DB_PRIORITY_DEFAULT));
    rb_define_const(bdb_mDb, "PRIORITY_HIGH", INT2FIX(DB_PRIORITY_HIGH));
    rb_define_const(bdb_mDb, "PRIORITY_VERY_HIGH", INT2FIX(DB_PRIORITY_VERY_HIGH));
#endif
#ifdef DB_GET_BOTH_RANGE
    rb_define_const(bdb_mDb, "GET_BOTH_RANGE", INT2FIX(DB_GET_BOTH_RANGE));
#endif
    bdb_init_env();
    bdb_init_common();
    bdb_init_recnum();
    bdb_init_transaction();
    bdb_init_cursor();
    bdb_init_lock();
    bdb_init_log();
    bdb_init_delegator();

    bdb_errstr = rb_tainted_str_new(0, 0);
    rb_global_variable(&bdb_errstr);
}
