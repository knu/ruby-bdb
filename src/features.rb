#!/usr/bin/ruby

unless "a".respond_to?(:tr_cpp)
   class String
      def tr_cpp
         strip.upcase.tr_s("^A-Z0-9_", "_")
      end
   end
end

def have_db_member(where, array)
   found = false
   array.each do |st|
      if have_struct_member(where, st, 'db.h')
         $defs.pop
         $defs << "-DHAVE_ST_#{where}_#{st.tr_cpp}"
         found = true
      else
         $defs << "-DNOT_HAVE_ST_#{where}_#{st.tr_cpp}"
      end
   end
   found
end

def have_db_type(type)
   if have_type(type, 'db.h')
      true
   else
      $defs << "-DNOT_HAVE_TYPE_#{type.tr_cpp}"
      false
   end
end

def try_db_compile(func, src, mess = nil)
   print "checking for #{func}... " unless mess
   if try_compile(src)
      $defs << "-DHAVE_#{func}"
      puts "yes"
      true
   else
      $defs << "-DNOT_HAVE_#{func}"
      puts "no"
      false
   end
end

def have_db_const(const)
   const.each do |c|
      print "checking for #{c} in db.h..."
      try_db_compile("CONST_#{c.tr_cpp}", <<EOT, "checking for #{c} in db.h... ")
#include <db.h>

int main()
{
   int x = #{c};
   return x;
}
EOT
   end
end

def try_db_link(func, src)
   if try_link(src)
      $defs << "-DHAVE_#{func}"
      true
   else
      $defs << "-DNOT_HAVE_#{func}"
      false
   end
end

def try_db_run(func, src)
   ENV['LD_LIBRARY_PATH'] = $bdb_libdir
   if try_run(src)
      $defs << "-DHAVE_#{func}"
      true
   else
      $defs << "-DNOT_HAVE_#{func}"
      false
   end
ensure
   ENV['LD_LIBRARY_PATH'] = ''
end

have_db_const([
 "DB_AFTER", "DB_AGGRESSIVE", "DB_APPEND", "DB_ARCH_ABS", "DB_ARCH_DATA",
 "DB_ARCH_LOG", "DB_AUTO_COMMIT", "DB_BEFORE", "DB_BTREE", "DB_CACHED_COUNTS",
 "DB_CDB_ALLDB", "DB_CHECKPOINT", "DB_CHKSUM", "DB_CHKSUM_SHA1", "DB_CLIENT",
 "DB_CONFIG", "DB_CONSUME", "DB_CONSUME_WAIT", "DB_CREATE", "DB_CURLSN", "DB_CURRENT",
 "DB_DBT_MALLOC", "DB_DBT_PARTIAL", "DB_DBT_REALLOC","DB_DBT_USERMEM", "DB_DIRECT_DB",
 "DB_DIRECT_LOG", "DB_DIRTY_READ", "DB_DONOTINDEX", "DB_DSYNC_LOG",
 "DB_DUP", "DB_DUPSORT", "DB_EID_BROADCAST", "DB_EID_INVALID", 
 "DB_ENCRYPT", "DB_ENCRYPT_AES", "DB_ENV_THREAD", "DB_EVENT_PANIC",
 "DB_EVENT_REP_CLIENT", "DB_EVENT_REP_ELECTED", "DB_EVENT_REP_MASTER",
 "DB_EVENT_REP_NEWMASTER", "DB_EVENT_REP_PERM_FAILED",
 "DB_EVENT_REP_STARTUPDONE", "DB_EVENT_WRITE_FAILED", "DB_EXCL",
 "DB_FAST_STAT", "DB_FIRST", "DB_FIXEDLEN", "DB_FLUSH", "DB_FORCE", "DB_GET_BOTH_RANGE",
 "DB_GET_RECNO", "DB_GET_BOTH", "DB_HASH", "DB_HOME", "DB_IGNORE_LEASE", "DB_INCOMPLETE",
 "DB_INIT_CDB", "DB_INIT_LOCK", "DB_INIT_LOG", "DB_INIT_MPOOL", "DB_INIT_REP",
 "DB_INIT_TXN", "DB_JOINENV", "DB_JOIN_ITEM", "DB_JOIN_NOSORT", "DB_KEYEMPTY",
 "DB_KEYEXIST", "DB_KEYFIRST", "DB_KEYLAST", "DB_LAST", "DB_LOCKDOWN",
 "DB_LOCK_CONFLICT", "DB_LOCK_DEADLOCK", "DB_LOCK_DEFAULT", "DB_LOCK_EXPIRE",
 "DB_LOCK_GET", "DB_LOCK_GET_TIMEOUT", "DB_LOCK_IREAD", "DB_LOCK_IWR",
 "DB_LOCK_IWRITE", "DB_LOCK_MAXLOCKS", "DB_LOCK_MINLOCKS", "DB_LOCK_MINWRITE",
 "DB_LOCK_NG", "DB_LOCK_NOTGRANTED", "DB_LOCK_NOTHELD", "DB_LOCK_NOWAIT",
 "DB_LOCK_OLDEST", "DB_LOCK_PUT", "DB_LOCK_PUT_ALL", "DB_LOCK_PUT_OBJ",
 "DB_LOCK_RANDOM", "DB_LOCK_READ", "DB_LOCK_TIMEOUT", "DB_LOCK_WRITE",
 "DB_LOCK_YOUNGEST", "DB_LOG_AUTOREMOVE", "DB_LOG_AUTO_REMOVE", "DB_LOG_DIRECT",
 "DB_LOG_DSYNC", "DB_LOG_INMEMORY", "DB_LOG_IN_MEMORY", "DB_LOG_ZERO",
 "DB_MPOOL_CLEAN", "DB_MPOOL_CREATE", "DB_MPOOL_DIRTY", "DB_MPOOL_DISCARD",
 "DB_MPOOL_LAST", "DB_MPOOL_NEW", "DB_MPOOL_PRIVATE", "DB_MULTIVERSION",
 "DB_MUTEX_PROCESS_ONLY", "DB_NEXT_DUP", "DB_NEXT_NODUP", "DB_NODUPDATA",
 "DB_NOORDERCHK", "DB_NOSERVER", "DB_NOSERVER_HOME", "DB_NOSERVER_ID",
 "DB_NOTFOUND", "DB_OLD_VERSION", "DB_ORDERCHKONLY", "DB_OVERWRITE",
 "DB_POSITION", "DB_PRIVATE", "DB_SYSTEM_MEM", "DB_RMW",
 "DB_PAD", "DB_PREV_DUP", "DB_PREV_NODUP", "DB_PRINTABLE", "DB_PRIORITY_DEFAULT",
 "DB_PRIORITY_HIGH", "DB_PRIORITY_LOW", "DB_PRIORITY_VERY_HIGH", "DB_QUEUE",
 "DB_RDONLY", "DB_READ_COMMITTED", "DB_READ_UNCOMMITTED", "DB_RECNO",
 "DB_RECNUM", "DB_RECORDCOUNT", "DB_RECOVER", "DB_RECOVER_FATAL",
 "DB_REGION_INIT", "DB_RENUMBER", "DB_REPFLAGS_MASK", "DB_REPMGR_ACKS_ALL",
 "DB_REPMGR_ACKS_ALL_PEERS", "DB_REPMGR_ACKS_NONE", "DB_REPMGR_ACKS_ONE",
 "DB_REPMGR_ACKS_ONE_PEER", "DB_REPMGR_ACKS_QUORUM", "DB_REPMGR_CONNECTED",
 "DB_REPMGR_DISCONNECTED", "DB_REPMGR_PEER", "DB_REP_ACK_TIMEOUT",
 "DB_REP_ANYWHERE", "DB_REP_BULKOVF", "DB_REP_CHECKPOINT_DELAY",
 "DB_REP_CLIENT", "DB_REP_CONF_BULK", "DB_REP_CONF_DELAYCLIENT",
 "DB_REP_CONF_NOAUTOINIT", "DB_REP_CONF_NOWAIT", "DB_REP_CONNECTION_RETRY",
 "DB_REP_DEFAULT_PRIORITY", "DB_REP_DUPMASTER", "DB_REP_EGENCHG",
 "DB_REP_ELECTION", "DB_REP_ELECTION_RETRY", "DB_REP_ELECTION_TIMEOUT",
 "DB_REP_FULL_ELECTION", "DB_REP_FULL_ELECTION_TIMEOUT", "DB_REP_HANDLE_DEAD",
 "DB_REP_HOLDELECTION", "DB_REP_IGNORE", "DB_REP_ISPERM", "DB_REP_JOIN_FAILURE",
 "DB_REP_LEASE_EXPIRED", "DB_REP_LEASE_TIMEOUT", "DB_REP_LOCKOUT",
 "DB_REP_LOGREADY", "DB_REP_LOGSONLY", "DB_REP_MASTER", "DB_REP_NEWMASTER",
 "DB_REP_NEWSITE", "DB_REP_NOBUFFER", "DB_REP_NOTPERM", "DB_REP_OUTDATED",
 "DB_REP_PAGEDONE", "DB_REP_PERMANENT", "DB_REP_REREQUEST", "DB_REP_UNAVAIL",
 "DB_RPCCLIENT", "DB_RUNRECOVERY", "DB_SALVAGE", "DB_SECONDARY_BAD", "DB_SET",
 "DB_SET_LOCK_TIMEOUT", "DB_SET_RANGE", "DB_SET_RECNO", "DB_SET_TXN_TIMEOUT",
 "DB_SNAPSHOT", "DB_STAT_ALL", "DB_STAT_CLEAR", "DB_STAT_SUBSYSTEM",
 "DB_THREAD", "DB_TRUNCATE", "DB_TXN_ABORT", "DB_TXN_APPLY",
 "DB_TXN_BACKWARD_ROLL", "DB_TXN_FORWARD_ROLL", "DB_TXN_NOSYNC",
 "DB_TXN_NOWAIT", "DB_TXN_PRINT", "DB_TXN_SNAPSHOT", "DB_TXN_SYNC",
 "DB_TXN_WRITE_NOSYNC", "DB_UNKNOWN", "DB_UPGRADE", "DB_USE_ENVIRON",
 "DB_USE_ENVIRON_ROOT", "DB_VERB_CHKPOINT", "DB_VERB_DEADLOCK",
 "DB_VERB_RECOVERY", "DB_VERB_REPLICATION", "DB_VERB_WAITSFOR",
 "DB_VERIFY", "DB_PRIORITY_VERY_LOW", "DB_WRITECURSOR", "DB_XA_CREATE",
 "DB_XIDDATASIZE", "DB_MULTIPLE_KEY", "DB_REP_CONF_LEASE",
 "DB_REP_HEARTBEAT_MONITOR", "DB_REP_HEARTBEAT_SEND", "DB_TXN_NOT_DURABLE"
               
              ])
#

try_db_link('DB_STRERROR', <<EOT)
#include <db.h>

int main()
{
   db_strerror(2);
   return 1;
}
EOT

#


have_db_type('DB_KEY_RANGE')
have_db_type('DB_INFO')
have_db_type('DB_SEQUENCE')
have_db_type('DB_LOGC')
have_db_type('DBTYPE')
have_db_type('DB_COMPACT')
if have_db_type('DB_HASH_STAT')
   have_db_member('DB_HASH_STAT',
               ['hash_nkeys', 'hash_nrecs', 'hash_ndata', 'hash_nelem', 
                'hash_pagecnt'])
end
if have_db_type('DB_QUEUE_STAT')
   have_db_member('DB_QUEUE_STAT',
               ['qs_nkeys', 'qs_nrecs', 'qs_ndata', 'qs_start'])
end
if have_db_type('DB_REP_STAT')
   have_db_member('DB_REP_STAT',
                  ['st_bulk_fills', 'st_bulk_overflows', 'st_bulk_records', 
                   'st_bulk_transfers', 'st_client_rerequests', 'st_client_svc_miss',
                   'st_client_svc_req', 'st_egen', 'st_election_nvotes',
                   'st_election_sec', 'st_election_usec', 'st_next_pg', 
                   'st_pg_duplicated', 'st_pg_records', 
                   'st_pg_requested', 'st_startup_complete', 'st_waiting_pg',])
end
#
have_db_member('DBC',
            ['c_close', 'c_count', 'c_del', 'c_dup', 'c_get', 'c_pget', 'c_put',
             'close', 'count', 'del', 'dup', 'get', 'pget', 'put',
             'get_priority'])
#
have_db_member('DB',
            ['app_private', 'set_h_compare', 'set_append_recno',
             'set_feedback', 'set_q_extendsize', 'set_encrypt', 'set_errcall',
             'get_type', 'pget', 'fd', 'set_priority', 'byteswapped',
             'get_bt_minkey', 'get_cachesize', 'get_dbname', 'get_env', 
             'get_h_ffactor', 'get_h_nelem', 'get_lorder', 'get_pagesize',
             'get_q_extentsize', 'get_re_delim', 'get_re_len', 
             'get_re_pad', 'get_re_source', 'get_flags', 'get_open_flags',
             'verify', 'truncate', 'upgrade', 'remove', 'rename', 'join', 
             'get_byteswapped' ,'set_cache_priority'])

if $defs.include?('-DHAVE_ST_DB_GET_TYPE')
   try_db_compile('DB_GET_TYPE_2', <<EOT)
#include <db.h>

int main()
{
   DB *db = 0;
   DBTYPE dt = 0;
   db->get_type(db, &dt);
   return 1;
}
EOT
end
   
if have_db_member('DB', ['open'])
   if try_db_compile('DB_OPEN_7', <<EOT)
#include <db.h>

int main()
{
   DB *db = 0;
   db->open(db, NULL, "", "", DB_BTREE, 0, 0);
   return 1;
}
EOT
      try_db_run('DB_OPEN_7_DB_CREATE', <<EOT)
#include <db.h>

int main()
{
    DB *db = 0;
    db_create(&db, 0, 0);
    if (db->open(db, 0, NULL, NULL, DB_BTREE, 0, 0)) {
        return 0;
    }
    return 1;
}
EOT
      args = if $defs.include?('-DHAVE_DB_OPEN_7_DB_CREATE')
                'db, NULL, NULL, NULL, DB_BTREE, DB_CREATE|DB_THREAD, 0'
             else
                'db, NULL, NULL, DB_BTREE, DB_CREATE|DB_THREAD, 0'
             end
      try_db_run('DB_KEY_DBT_MALLOC', <<EOT)
#include <db.h>
#include <string.h>

int
a_compare(DB *db, const DBT *dbt1, const DBT *dbt2)
{
    return 1;
}

int main()
{
    DB *db = 0;
    DBT k, v;
    char *s = "a";
    db_create(&db, 0, 0);
    db->set_bt_compare(db, a_compare);
    if (db->open(#{args})) {
        return 1;
    }
    memset(&k, 0, sizeof(DBT));
    k.data = s;
    k.size = 1;
    memset(&v, 0, sizeof(DBT));
    v.data = s;
    v.size = 1;
    v.flags |= DB_DBT_MALLOC;
    if (db->get(db, 0, &k, &v, 0)) {
       db->close(db, 0);
       return 0;
    }
    db->close(db, 0);
    return 1;
}
EOT
   end
end

if have_db_member('DB', ['associate'])
   try_db_compile('DB_ASSOCIATE_TXN', <<EOT)
#include <db.h>

int main()
{
   DB *db = 0;
   DB_TXN *txn = 0;
   db->associate(db, txn, db, 0, 0);
   return 1;
}
EOT
end

if $defs.include?("-DHAVE_ST_DB_JOIN")
   try_db_compile('DB_JOIN_FLAG_DBC', <<EOT)
#include <db.h>

int main()
{
   DB *db = 0;
   DBC **dbcarr = 0, *dbc;
   db->join(db, dbcarr, 0, &dbc);
   return 1;
}
EOT
end

if $defs.include?("-DHAVE_ST_DB_GET_BYTESWAPPED")
   try_db_compile('DB_GET_BYTESWAPPED_2', <<EOT)
#include <db.h>

int main()
{
   DB *db = 0;
   int swap;
   db->get_byteswapped(db, &swap);
   return 1;
}
EOT
end
#
have_db_member('DB_ENV',
               ["get_cachesize", "get_data_dirs", "get_flags", "get_home",
                "get_lg_bsize", "get_lg_dir", "get_lg_max", "get_lg_regionmax",
                "get_lk_detect", "get_lk_max_lockers", "get_lk_max_locks",
                "get_lk_max_objects", "get_mp_mmapsize", 
                "get_open_flags", "get_rep_limit", "get_rep_nsites", "get_shm_key",
                "get_tas_spins", "get_tmp_dir", "get_timeout", "get_tx_max",
                "get_tx_timestamp", "rep_get_nsites", "rep_get_priority"])

have_db_member('DB_ENV',
            ['lg_info', 'log_put', 'log_flush', 'log_cursor', 'log_file',
             'log_flush', 'set_feedback', 'set_app_dispatch', 
             'set_rep_transport', 'set_timeout', 'set_txn_timeout',
             'set_lock_timeout', 'set_encrypt', 'set_rep_limit',
             'rep_elect', 'rep_start', 'rep_process_message',
             'rep_set_limit', 'set_msgcall', 'set_thread_id',
             'set_thread_id_string', 'set_isalive', 'set_shm_key',
             'rep_set_nsites', 'rep_set_priority', 'rep_set_config',
             'rep_set_timeout', 'rep_set_transport', 'repmgr_set_local_site',
             'repmgr_add_remote_site', 'repmgr_set_ack_policy',
             'repmgr_set_site_list', 'repmgr_start',
             'set_intermediate_dir_mode', 'set_event_notify',
             'set_cachesize', 'set_region_init', 'set_tas_spins',
             'set_tx_timestamp', 'db_verbose', 'set_verbose',
             'lk_detect', 'set_lk_detect', 'lk_max', 'set_lk_max',
             'lk_conflicts', 'set_lk_conflicts', 'set_timeout',
             'set_lk_max_locks', 'set_lk_max_lockers', 'lg_max',
             'set_lk_max_objects', 'set_lg_bsize', 'set_data_dir',
             'set_lg_dir', 'set_tmp_dir', 'set_server', 'set_rpc_server',
             'set_flags', 'close', 'set_func_sleep', 'set_func_yield',
             'set_alloc', 'set_errcall' , 'lsn_reset',
             'fileid_reset', 'failchk', 'rep_sync', 'rep_stat',
             'log_set_config', 'fidp', 'rep_set_clockskew',
             'rep_set_request'])

if ! $defs.include?("-DHAVE_ST_DB_ENV_SET_FUNC_SLEEP")
   ['db_env_set_func_yield', 'db_env_set_func_sleep'].each do |func|
      try_db_link(func.tr_cpp, <<EOT)
#include <db.h>

static int a_func() { return 0; }

int main()
{
   #{func}(a_func);
   return 1;
}
EOT
   end
end

try_db_link('DB_ENV_CREATE', <<EOT)
#include <db.h>

int main()
{
   db_env_create((DB_ENV **)0, 0);
   return 1;
}
EOT

if ! $defs.include?("-DHAVE_ST_DB_ENV_SET_REGION_INIT")
   try_db_link('DB_ENV_SET_REGION_INIT', <<EOT)
#include <db.h>

int main()
{
  db_env_set_region_init(0);
  return 1;
}
EOT
end

try_db_link('DB_APPINIT', <<EOT)
#include <db.h>

int main()
{
   db_appinit((char *)0, (char **)0, (DB_ENV *)0, 0);
   return 0;
}
EOT

try_db_link('DB_JUMP_SET', <<EOT)
#include <db.h>

static int a_func() { return 0; }

int main()
{
   db_jump_set(a_func, 0);
   return 1;
}
EOT

if have_db_member('DB_ENV', ['open'])
   try_db_compile('ENV_OPEN_DB_CONFIG', <<EOT)
#include <db.h>

int main()
{
   DB_ENV *env = 0;
   char **db_config = 0;
   env->open(env, (char *)0, db_config, 0, 0);
   return 1;
}
EOT
end

db_env_create = "#define HAVE_DB_ENV_CREATE "
db_env_create << if $defs.include?('-DHAVE_DB_ENV_CREATE')
                    "1"
                 else
                    "0"
                 end

db_appinit = "#define HAVE_DB_APPINIT "
db_appinit << if $defs.include?('-DHAVE_DB_APPINIT')
                    "1"
                 else
                    "0"
                 end
env_open_db_config = "#define HAVE_ENV_OPEN_DB_CONFIG "
env_open_db_config << if $defs.include?('-DHAVE_ENV_OPEN_DB_CONFIG')
                    "1"
                 else
                    "0"
                 end
try_db_run('DB_ENV_ERRCALL_3', <<EOT)
#include <db.h>
#include <string.h>

#{db_env_create}
#{db_appinit}
#{env_open_db_config}

static int value = 0;

void
bdb_env_errcall(const DB_ENV *env, const char *errpfx, const char *msg)
{
    if (!strncmp((void *)env, "BDB::", 5)) {
	value = 1;
    }
    else {
        value = 0;
   }
}

int main() 
{ 
    DB_ENV *envp;

#if ! HAVE_DB_ENV_CREATE
    envp = malloc(sizeof(DB_ENV));
    memset(envp, 0, sizeof(DB_ENV));
    envp->db_errpfx = "BDB::";
    envp->db_errcall = bdb_env_errcall;
#else
    db_env_create(&envp, 0);
    envp->set_errpfx(envp, "BDB::");
    envp->set_errcall(envp, bdb_env_errcall);
#endif
#if HAVE_DB_APPINIT
    db_appinit("", 0, envp, 123456789);
#else
#if HAVE_ENV_OPEN_DB_CONFIG
    envp->open(envp, "", 0, 123456789, 123456789);
#else
    envp->open(envp, "", 123456789, 123456789);
#endif
#endif
    return value;
}
EOT

try_db_compile('ENV_REMOVE_4', <<EOT)
#include <db.h>

int main()
{
   DB_ENV *env = 0;
   env->remove(env, (char *)0, NULL, 0);
   return 1;
}
EOT

if $defs.include?('-DHAVE_ST_DB_ENV_LG_INFO')
   $defs << '-DNOT_HAVE_DB_LOG_REGISTER'
   $defs << '-DNOT_HAVE_DB_LOG_UNREGISTER'
else
   if !have_db_member('DB_ENV', ['log_stat'])
      try_db_link('DB_LOG_STAT_3', <<EOT)
#include <db.h>
int main()
{
   DB_ENV *env = 0;
   DB_LOG_STAT *bdb_stat;
   log_stat(env, &bdb_stat, 0);
   return 1;
}
EOT
   end

   if !have_db_member('DB_ENV', ['log_archive'])
      try_db_link('DB_LOG_ARCHIVE_4', <<EOT)
#include <db.h>
int main()
{
   char **list = 0;
   int flag = 0;
   DB_ENV *env = 0;
   log_archive(env, &list, flag, NULL);
   return 1;
}
EOT
   end

   if have_db_member('DB_ENV', ['log_register'])
      $defs << '-DNOT_HAVE_DB_LOG_REGISTER'
   else
      if try_db_link('DB_LOG_REGISTER_4', <<EOT)
#include <db.h>

int main()
{
   DB_ENV *env = 0;
   DB *db = 0;
   u_int32_t fidp;
   log_register(env, db, "", &fidp);
   return 1;
}
EOT
      $defs << '-DHAVE_DB_LOG_REGISTER'
      else
         try_db_link('DB_LOG_REGISTER', <<EOT)
#include <db.h>

int main()
{
   DB_ENV *env = 0;
   DB *db = 0;
   log_register(env, db, "");
   return 1;
}
EOT
      end
   end

   if have_db_member('DB_ENV', ['log_unregister'])
      $defs << '-DNOT_HAVE_DB_LOG_UNREGISTER'
   else
      try_db_link('DB_LOG_UNREGISTER', <<EOT)
#include <db.h>

int main()
{
   DB_ENV *env = 0;
   log_register(env, (void *)0);
   return 1;
}
EOT
   end

   try_db_compile('DB_CURSOR_4', <<EOT)
#include <db.h>

int main()
{
   DB *db = 0;
   DB_TXN *txn = 0;
   DBC *dbc;
   db->cursor(db, txn, &dbc, 0);
   return 1;
}
EOT

end

if $defs.include?("-DHAVE_ST_DB_ENV_REP_ELECT")
   if try_db_compile('DB_ENV_REP_ELECT_7', <<EOT)
#include <db.h>

int main()
{
   DB_ENV *env = 0;
   int envid;
   env->rep_elect(env, 0, 0, 0, 0, &envid, 0);
   return 1;
}
EOT
      $defs << "-DNOT_HAVE_DB_ENV_REP_ELECT_5"
      $defs << "-DNOT_HAVE_DB_ENV_REP_ELECT_4"
   elsif try_db_compile('DB_ENV_REP_ELECT_5', <<EOT)
#include <db.h>

int main()
{
   DB_ENV *env = 0;
   int envid;
   env->rep_elect(env, 0, 0, 0, &envid);
   return 1;
}
EOT
      $defs << "-DNOT_HAVE_DB_ENV_REP_ELECT_4"
   else
      $defs << "-DHAVE_DB_ENV_REP_ELECT_4"
   end
end

if $defs.include?("-DHAVE_ST_DB_ENV_REP_PROCESS_MESSAGE")
   if try_db_compile('DB_ENV_REP_PROCESS_MESSAGE_5', <<EOT)
#include <db.h>

int main()
{
   DB_ENV *env = 0;
   DBT control, rec;
   int envid = 0;
   env->rep_process_message(env, &control, &rec, envid, (DB_LSN *)0);
   return 1;
}
EOT
      $defs << "-DNOT_HAVE_DB_ENV_REP_PROCESS_MESSAGE_4"
   else
      $defs << "-DHAVE_DB_ENV_REP_PROCESS_MESSAGE_4"
   end
end
#

#
have_db_member('DB_LOG_STAT',
            ['st_refcnt' ,'st_lg_bsize', 'st_lg_size', 'st_lg_max',
             'st_wcount_fill', 'st_disk_file', 'st_disk_offset',
             'st_flushcommit', 'st_maxcommitperflush', 'st_mincommitperflush'])
#

have_db_member('DB_BTREE_STAT', 
            ['bt_nkeys', 'bt_nrecs', 'bt_ndata', 'bt_pagecnt'])

try_db_compile('DB_STAT_4', <<EOT)
#include <db.h>

int main()
{
    DB *db = 0;
    db->stat(db, (DB_TXN *)0, (void *)0, 0);
    return 1;
}
EOT

try_db_compile('DB_CURSOR_4', <<EOT)
#include <db.h>

int main()
{
    DB *db = 0;
    DBC *st = 0;
    db->cursor(db, (DB_TXN *)0, &st, 0);
    return 1;
}
EOT

#

#
#
# lock
have_db_member('DB_ENV', 
               ['lock_id_free', 'lock_id', 'lk_info', 'lock_detect', 'lock_stat',
                'lock_get', 'lock_vec', 'lock_put'])
have_db_member('DB_LOCK_STAT',
               ['st_id', 'st_lastid', 'st_lock_nowait', 'st_lock_wait',
                'st_nnowaits', 'st_nconflicts', 'st_objs_nowait', 'st_objs_wait',
                'st_lockers_nowait', 'st_lockers_wait', 'st_locks_nowait', 
                'st_locks_wait'])
have_db_member('DB_LOCKREQ', ['timeout'])
if ! $defs.include?("-DHAVE_ST_DB_ENV_LK_INFO") &&
      ! $defs.include?("-DHAVE_ST_DB_ENV_LOCK_STAT")
   unless try_db_link('LOCK_STAT_3', <<EOT)
#include <db.h>

int main()
{
    lock_stat((DB_ENV *)0, 0, 0);
    return 1;
}
EOT
      $defs << '-DHAVE_LOCK_STAT_2'
   end
end
#
have_db_member('DB_TXN',
               ['abort', 'commit', 'id', 'prepare', 'discard', 'set_timeout',
                'set_name'])
have_db_member('DB_ENV',
               ['tx_info', 'txn_begin', 'txn_checkpoint', 'txn_recover',
                'txn_stat', 'dbremove', 'dbrename', 'set_tx_max'])
have_db_member('DB_TXN_STAT',
               ['st_maxnactive', 'st_regsize', 'st_region_wait', 'st_region_nowait',
                'st_last_ckp', 'st_pending_ckp', 'st_nrestores'])
have_db_member('DB_TXN_ACTIVE', ['tid', 'name', 'parentid'])
if ! $defs.include?("-DHAVE_ST_DB_TXN_COMMIT")
   try_db_link('TXN_COMMIT_2', <<EOT)
#include <db.h>

int main()
{
    txn_commit((DB_TXN *)0, 0);
    return 1;
}
EOT
end
if ! $defs.include?("-DHAVE_ST_DB_TXN_PREPARE")
   if try_db_link('TXN_PREPARE_2', <<EOT)
#include <db.h>

int main()
{
    txn_prepare((DB_TXN *)0, (unsigned char *)0);
    return 1;
}
EOT
      $defs << '-DHAVE_TXN_PREPARE'
   elsif try_db_link('TXN_PREPARE', <<EOT)
#include <db.h>

int main()
{
    txn_prepare((DB_TXN *)0);
    return 1;
}
EOT
   end
end
if ! $defs.include?("-DHAVE_ST_DB_ENV_TX_INFO") &&
      ! $defs.include?("-DHAVE_ST_DB_ENV_TXN_CHECKPOINT")
   try_db_link('TXN_CHECKPOINT_4', <<EOT)
#include <db.h>

int main()
{
   txn_checkpoint((DB_ENV *)0, 0, 0, 0);
   return 1;
}
EOT
end

try_db_link('TXN_RECOVER', <<EOT)
#include <db.h>

int main()
{
   txn_recover((DB_TXN *)0, (DB_PREPLIST *)0, 0, 0);
   return 1;
}
EOT

if ! $defs.include?("-DHAVE_ST_DB_TXN_DISCARD")
   try_db_link('TXN_DISCARD', <<EOT)
#include <db.h>

int main()
{
   txn_discard((DB_TXN *)0, 0);
   return 1;
}
EOT
end

if ! $defs.include?("-DHAVE_ST_DB_ENV_TX_INFO") &&
      ! $defs.include?("-DHAVE_ST_DB_ENV_TXN_STAT")
   try_db_link('TXN_STAT_3', <<EOT)
#include <db.h>

int main()
{
   txn_stat((DB_ENV *)0, (DB_TXN_STAT *)0, 0);
   return 1;
}
EOT
end
#
#

begin
   conftest = CONFTEST_C.dup
   class Object
      remove_const('CONFTEST_C')
   end

   CONFTEST_C = 'conftest.cxx'

   if $defs.include?('-DHAVE_ST_DB_ENV_SET_REP_TRANSPORT') ||
         $defs.include?('-DHAVE_ST_DB_ENV_REP_SET_TRANSPORT')
      set_rep = if $defs.include?('-DHAVE_ST_DB_ENV_SET_REP_TRANSPORT')
                   'set_rep_transport'
                else
                   'rep_set_transport'
                end
      try_db_compile('ENV_REP_TRANSPORT_6', <<EOT)
#include <db.h>

static int
bdb_env_rep_transport(DB_ENV *env, const DBT *control, const DBT *rec,
		      const DB_LSN *lsn, int envid, u_int32_t flags)
{
   return 0;
}

int main()
{
   DB_ENV *env = 0;
   env->#{set_rep}(env, 0, bdb_env_rep_transport);
   return 1;
}
EOT
   end

   if $defs.include?('-DHAVE_DB_ENV_REP_ELECT_5')
      try_db_compile('DB_ENV_REP_ELECT_PRIO', <<EOT)
#include <db.h>

int main()
{
   DB_ENV *env = 0;
   int envid = 0;
   env->rep_elect(env, 0, 0, 0, &envid);
   return 1;
}
EOT
   end


   if $defs.include?('-DHAVE_DB_ENV_REP_PROCESS_MESSAGE_5')
      try_db_compile('DB_ENV_REP_PROCESS_MESSAGE_ENVID', <<EOT)
#include <db.h>

int main()
{
   DB_ENV *env = 0;
   DBT control, rec;
   DB_LSN lsn;
   int envid = 0;
   env->rep_process_message(env, &control, &rec, envid, &lsn);
   return 1;
}
EOT
   end


ensure
   class Object
      remove_const('CONFTEST_C')
   end

   CONFTEST_C = conftest
end


open('bdb_features.h', 'w') do |gen|
   $defs.delete_if do |k|
      if /\A-DHAVE/ =~ k
         k.sub!(/\A-D/, '#define ')
         k << ' 1'
         gen.puts k
         true
      end
   end
   $defs.delete_if do |k|
      if /\A-DNOT_HAVE/ =~ k
         k.sub!(/\A-DNOT_/, '#define ')
         k << ' 0'
         gen.puts k
         true
      end
   end
end

