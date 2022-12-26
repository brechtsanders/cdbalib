#include "cdbalib.h"

#include <stdlib.h>
#include <stdio.h>/////
#include <string.h>
#if defined(DB_MYSQL)
#ifdef _WIN32
#include <mariadb/mysql.h>
#else
#include <mysql.h>
#endif
#if defined(LIBMYSQL_VERSION_ID) && LIBMYSQL_VERSION_ID > 80000
#if __STDC_VERSION__ >= 199901L
#define my_bool bool
#else
#define my_bool int
#endif
#endif
#elif defined(DB_SQLITE3)
#include <sqlite3.h>
#elif defined(DB_ODBC)
#include <odbcinst.h>
#else
#endif

#define RETRY_ATTEMPTS 12
#define RETRY_WAIT_TIME 250
#ifdef _WIN32
#include <windows.h>
#define WAIT_BEFORE_RETRY(ms) Sleep(ms);
#else
#include <unistd.h>
#define WAIT_BEFORE_RETRY(ms) usleep(ms * 1000);
#endif

////////////////////////////////////////////////////////////////////////

struct cdba_library_handle_struct {
  const char* drivername;
#if defined(DB_MYSQL)
#elif defined(DB_SQLITE3)
#elif defined(DB_ODBC)
  SQLHENV odbc_env;
#else
#endif
};

DLL_EXPORT_CDBALIB cdba_library_handle cdba_library_initialize ()
{
  struct cdba_library_handle_struct* dblib;
  if ((dblib = (struct cdba_library_handle_struct*)malloc(sizeof(struct cdba_library_handle_struct))) == NULL)
    return NULL;
#if defined(DB_MYSQL)
  mysql_library_init(0, NULL, NULL);
  dblib->drivername = "MySQL";
#elif defined(DB_SQLITE3)
  sqlite3_initialize();
  dblib->drivername = "SQLite3";
#elif defined(DB_ODBC)
  if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &dblib->odbc_env) == SQL_ERROR) {
    free(dblib);
    return NULL;
  }
  SQLSetEnvAttr(dblib->odbc_env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC2, 0);
  //SQLSetEnvAttr(dblib->odbc_env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
  dblib->drivername = "ODBC";
#else
  free(dblib);
  dblib = NULL;
#endif
  return dblib;
}

DLL_EXPORT_CDBALIB void cdba_library_cleanup (cdba_library_handle dblib)
{
  if (!dblib)
    return;
#if defined(DB_MYSQL)
  mysql_library_end();
#elif defined(DB_SQLITE3)
  sqlite3_shutdown();
#elif defined(DB_ODBC)
  if (dblib->odbc_env)
    SQLFreeHandle(SQL_HANDLE_ENV, dblib->odbc_env);
#else
#endif
}

DLL_EXPORT_CDBALIB const char* cdba_library_get_name (cdba_library_handle dblib)
{
  if (!dblib)
    return NULL;
  return dblib->drivername;
}

DLL_EXPORT_CDBALIB char* cdba_library_get_version (cdba_library_handle dblib)
{
#if defined(DB_MYSQL)
  static char buf[12];
  unsigned long l;
  l = mysql_get_client_version();
  snprintf(buf, sizeof(buf), "%u.%u.%u", (unsigned int)(l / 10000), (unsigned int)((l / 100) % 100), (unsigned int)(l % 100));
  return strdup(buf);
#elif defined(DB_SQLITE3)
  return strdup(sqlite3_libversion());
#elif defined(DB_ODBC)
  static SQLCHAR buf[12];
  SQLHDBC odbc_conn;
  SQLSMALLINT buflen = 0;
  if (SQLAllocHandle(SQL_HANDLE_DBC, dblib->odbc_env, &odbc_conn) == SQL_ERROR) {
    return NULL;
  }
  if (SQLGetInfoA(odbc_conn, SQL_ODBC_VER, buf, sizeof(buf), &buflen) != SQL_SUCCESS) {
/*
    SQLCHAR sql_errmsg[SQL_MAX_MESSAGE_LENGTH + 1];
    SQLCHAR sql_state[6];
    SQLINTEGER sql_native_error = 0;
    SQLSMALLINT sql_errmsglen = 0;
    if (SQLErrorA(dblib->odbc_env, odbc_conn, SQL_NULL_HSTMT, sql_state, &sql_native_error, sql_errmsg, sizeof(sql_errmsg), &sql_errmsglen) == SQL_SUCCESS) {
      fprintf(stderr, "ODBC error: %s\n", sql_errmsg);
    }
*/
    SQLFreeHandle(SQL_HANDLE_DBC, odbc_conn);
    return NULL;
  }
  SQLFreeHandle(SQL_HANDLE_DBC, odbc_conn);
  return strdup((char*)buf);
#else
  return strdup(CDBALIB_VERSION_STRING);
#endif
}

////////////////////////////////////////////////////////////////////////

struct cdba_handle_struct
{
#if defined(DB_MYSQL)
  MYSQL* mysql_conn;
#elif defined(DB_SQLITE3)
  sqlite3* sqlite3_conn;
#elif defined(DB_ODBC)
  SQLHDBC odbc_conn;
  cdba_library_handle dblib;
#else
#endif
  char* errmsg;
};

DLL_EXPORT_CDBALIB cdba_handle cdba_open (cdba_library_handle dblib, const char* path)
{
  struct cdba_handle_struct* db;
  if (!dblib)
    return NULL;
  if ((db = (struct cdba_handle_struct*)malloc(sizeof(struct cdba_handle_struct))) == NULL)
    return NULL;
  db->errmsg = NULL;
#if defined(DB_MYSQL)
  my_bool reconnect = 1;
  if ((db->mysql_conn = mysql_init(NULL)) == NULL) {
    free(db);
    return NULL;
  }
  mysql_set_character_set(db->mysql_conn, "utf8");
  mysql_options(db->mysql_conn, MYSQL_OPT_RECONNECT, &reconnect);
  if (mysql_real_connect(db->mysql_conn, "127.0.0.1", "p1log2db", "TOPSECRET", "p1log2db", 0, NULL, 0) == NULL) {
    free(db);
    return NULL;
  }
#elif defined(DB_SQLITE3)
  if (sqlite3_open(path, &db->sqlite3_conn) != SQLITE_OK) {
    free(db);
    return NULL;
  }
#elif defined(DB_ODBC)
  db->dblib = dblib;
  if (SQLAllocHandle(SQL_HANDLE_DBC, dblib->odbc_env, &db->odbc_conn) == SQL_ERROR) {
    free(db);
    return NULL;
  }
  SQLSetConnectAttr(db->odbc_conn, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);
  //path = "DSN=cdbalib_test_msaccess;DBQ=\\\\SERVER\\Users\\brecht\\sources\\CPP\\cdbalib\\build\\test_msaccess.accdb;DriverId=25;FIL=MS Access;MaxBufferSize=2048;PageTimeout=5;UID=admin;";/////
  path = "DSN=cdbalib_test_msaccess";/////
  if (SQLDriverConnectA(db->odbc_conn, NULL, (SQLCHAR*)path, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT) == SQL_ERROR) {
    free(db);
    return NULL;
  }
#else
  free(db);
  db = NULL;
#endif
  return db;
}

DLL_EXPORT_CDBALIB void cdba_close (cdba_handle db)
{
  if (!db)
    return;
  if (db->errmsg)
    free(db->errmsg);
#if defined(DB_MYSQL)
  mysql_close(db->mysql_conn);
#elif defined(DB_SQLITE3)
  sqlite3_close(db->sqlite3_conn);
#elif defined(DB_ODBC)
  if (db->odbc_conn) {
    SQLDisconnect(db->odbc_conn);
    SQLFreeHandle(SQL_HANDLE_DBC, db->odbc_conn);
  }
#else
#endif
  free(db);
}

DLL_EXPORT_CDBALIB void cdba_set_error (cdba_handle db, const char* errmsg)
{
  if (db->errmsg)
    free(db->errmsg);
  db->errmsg = (errmsg ? strdup(errmsg) : NULL);
}

#if defined(DB_ODBC)
void cdba_set_odbc_error (cdba_handle db, SQLHSTMT stmt, SQLSMALLINT handletype)
{
  SQLSMALLINT i;
  SQLSMALLINT len;
  size_t pos;
  size_t errmsglen;
  if (db->errmsg)
    free(db->errmsg);
  db->errmsg = NULL;
  errmsglen = 0;
  pos = 0;
  i = 1;
  len = 0;
  while (SQLGetDiagFieldA(handletype, (handletype == SQL_HANDLE_STMT ? stmt : db->odbc_conn), i, SQL_DIAG_MESSAGE_TEXT, NULL, 0, &len) != SQL_NO_DATA) {
    errmsglen += (i > 1 ? 1 : 0) + len + 1;
    if ((db->errmsg = (char*)realloc(db->errmsg, errmsglen)) == NULL)
      break;
    if (i > 1)
      strcpy(db->errmsg + pos++, "\n");
    if (SQLGetDiagFieldA(handletype, (handletype == SQL_HANDLE_STMT ? stmt : db->odbc_conn), i, SQL_DIAG_MESSAGE_TEXT, db->errmsg + pos, len + 1, NULL) == SQL_NO_DATA)
      continue;
    pos += len;
    i++;
  }
}
#endif

DLL_EXPORT_CDBALIB const char* cdba_get_error (cdba_handle db)
{
  return db->errmsg;
}

DLL_EXPORT_CDBALIB int cdba_sql (cdba_handle db, const char* sql)
{
#if defined(DB_MYSQL)
  int status;
  MYSQL_RES* res;
  if ((status = mysql_query(db->mysql_conn, sql)) != 0) {
    cdba_set_error(db, mysql_error(db->mysql_conn));
    return -1;
  }
  res = mysql_use_result(db->mysql_conn);
  mysql_free_result(res);
  return 0;
#elif defined(DB_SQLITE3)
  int status;
  int i;
  const char* sqlnext;
  sqlite3_stmt* stmt;
	if ((status = sqlite3_prepare_v2(db->sqlite3_conn, sql, -1, &stmt, &sqlnext)) != SQLITE_OK) {
    cdba_set_error(db, sqlite3_errmsg(db->sqlite3_conn));
	  return -2;
	}
	if (sqlnext && *sqlnext) {
    cdba_set_error(db, "not a single SQL statement");
    sqlite3_finalize(stmt);
	  return -3;
	}
  status = SQLITE_ERROR;
  i = 0;
  while (i++ < RETRY_ATTEMPTS && ((status = sqlite3_step(stmt)) == SQLITE_BUSY || status == SQLITE_LOCKED)) {
    WAIT_BEFORE_RETRY(RETRY_WAIT_TIME)
  }
  sqlite3_finalize(stmt);
  if (status != SQLITE_DONE && status != SQLITE_ROW) {
    cdba_set_error(db, sqlite3_errmsg(db->sqlite3_conn));
    return -1;
  }
  return 0;
#elif defined(DB_ODBC)
  SQLHDBC stmt;
  SQLRETURN status;
  if (SQLAllocHandle(SQL_HANDLE_STMT, db->odbc_conn, &stmt) != SQL_SUCCESS) {
    cdba_set_error(db, "SQLAllocHandle() failed");
    /////TO DO: cdba_set_error(db, ???);
    return -1;
  }
  status = SQLExecDirectA(stmt, (SQLCHAR*)sql, SQL_NTS);
  if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO) {
    cdba_set_odbc_error(db, stmt, SQL_HANDLE_STMT);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return -1;
  }
  SQLFreeHandle(SQL_HANDLE_STMT, stmt);
  return 0;
#else
  return -1;
#endif
}

DLL_EXPORT_CDBALIB int cdba_multiple_sql (cdba_handle db, const char* sql)
{
#if defined(DB_MYSQL)
  int status;
  MYSQL_RES* res;
  mysql_set_server_option(db->mysql_conn, MYSQL_OPTION_MULTI_STATEMENTS_ON);
  if ((status = mysql_query(db->mysql_conn, sql)) != 0) {
    cdba_set_error(db, mysql_error(db->mysql_conn));
  } else {
    res = mysql_use_result(db->mysql_conn);
    mysql_free_result(res);
    status = 0;
  }
  mysql_set_server_option(db->mysql_conn, MYSQL_OPTION_MULTI_STATEMENTS_OFF);
  return status;
#elif defined(DB_SQLITE3)
  int status;
  int i;
  const char* sqlnext;
  sqlite3_stmt* stmt;
  sqlnext = sql;
  while (sqlnext && *sqlnext) {
    while (*sqlnext == ' ' || *sqlnext == '\t' || *sqlnext == '\r' || *sqlnext == '\n')
      sqlnext++;
    if (!*sqlnext)
      break;
    if ((status = sqlite3_prepare_v2(db->sqlite3_conn, sqlnext, -1, &stmt, &sqlnext)) != SQLITE_OK) {
      cdba_set_error(db, sqlite3_errmsg(db->sqlite3_conn));
      return -2;
    }
    status = SQLITE_ERROR;
    i = 0;
    while (i++ < RETRY_ATTEMPTS && ((status = sqlite3_step(stmt)) == SQLITE_BUSY || status == SQLITE_LOCKED)) {
      WAIT_BEFORE_RETRY(RETRY_WAIT_TIME)
    }
    sqlite3_finalize(stmt);
    if (status != SQLITE_DONE && status != SQLITE_ROW) {
      cdba_set_error(db, sqlite3_errmsg(db->sqlite3_conn));
      return -1;
    }
  }
  return 0;
#elif defined(DB_ODBC)
  return cdba_sql(db, sql);/////
#else
  return -1;
#endif
}

DLL_EXPORT_CDBALIB void cdba_begin_transaction (cdba_handle db)
{
/*
#if defined(DB_MYSQL)
  mysql_autocommit(db->sqlite3_conn, 0);
#elif defined(DB_SQLITE3)
#elif defined(DB_ODBC)
#else
#endif
*/
#if DB_ODBC
  SQLSetConnectAttr(db->odbc_conn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER);
#else
  cdba_sql(db, "BEGIN");
#endif
}

DLL_EXPORT_CDBALIB void cdba_commit_transaction (cdba_handle db)
{
/*
#if defined(DB_MYSQL)
  mysql_commit(db->sqlite3_conn, 0);
  mysql_autocommit(db->sqlite3_conn, 1);
#elif defined(DB_SQLITE3)
#elif defined(DB_ODBC)
#else
#endif
*/
#if DB_ODBC
  SQLEndTran(SQL_HANDLE_DBC, db->odbc_conn, SQL_COMMIT);
  SQLSetConnectAttr(db->odbc_conn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_IS_UINTEGER);
#else
  cdba_sql(db, "COMMIT");
#endif
}

DLL_EXPORT_CDBALIB void cdba_rollback_transaction (cdba_handle db)
{
/*
#if defined(DB_MYSQL)
  mysql_rollback(db->sqlite3_conn, 0);
  mysql_autocommit(db->sqlite3_conn, 1);
#elif defined(DB_SQLITE3)
#else
#endif
*/
#if defined(DB_ODBC)
  SQLEndTran(SQL_HANDLE_DBC, db->odbc_conn, SQL_ROLLBACK);
  SQLSetConnectAttr(db->odbc_conn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_IS_UINTEGER);
#else
  cdba_sql(db, "ROLLBACK");
#endif
}

////////////////////////////////////////////////////////////////////////

#if defined(DB_MYSQL)
struct mysql_argbindinfo_struct {
  union {
    long long intval;
    double floatval;
  };
};

struct mysql_resultbindinfo_struct {
  struct mysql_argbindinfo_struct value;
  unsigned long length;
  my_bool is_null;
};
#endif

struct cdba_prep_handle_struct {
  union {
#if defined(DB_MYSQL)
    MYSQL_STMT* mysql_prepstat;
#elif defined(DB_SQLITE3)
    sqlite3_stmt* sqlite3_prepstat;
#elif defined(DB_ODBC)
    SQLHSTMT odbc_prepstat;
#else
#endif
  };
  char* errmsg;
#if defined(DB_MYSQL)
  MYSQL_RES* mysql_result_metadata;
  MYSQL_BIND* mysql_bind_result;
  struct mysql_resultbindinfo_struct* mysql_bind_vars;
#elif defined(DB_SQLITE3)
  int sqlite3_first_step_status;
#elif defined(DB_ODBC)
  cdba_handle db;
  SQLLEN* odbc_bind_len;
  int odbc_first_step_status;
#else
#endif
  int numargs;
  int numcols;
};

DLL_EXPORT_CDBALIB cdba_prep_handle cdba_create_preparedstatement (cdba_handle db, const char* sql)
{
  struct cdba_prep_handle_struct* stmt;
  if (!sql || !*sql)
    return NULL;
  if ((stmt = (struct cdba_prep_handle_struct*)malloc(sizeof(struct cdba_prep_handle_struct))) == NULL) {
    cdba_set_error(db, "Memory allocation error");
    return NULL;
  }
  stmt->errmsg = NULL;
#if defined(DB_MYSQL)
  if ((stmt->mysql_prepstat = mysql_stmt_init(db->mysql_conn)) == NULL) {
    cdba_set_error(db, "Memory allocation error");
	  return NULL;
  }
	if (mysql_stmt_prepare(stmt->mysql_prepstat, sql, strlen(sql)) != 0) {
	  cdba_set_error(db, mysql_stmt_error(stmt->mysql_prepstat));
	  mysql_stmt_close(stmt->mysql_prepstat);
	  free(stmt);
	  return NULL;
	}
  stmt->mysql_result_metadata = NULL;
  stmt->mysql_bind_result = NULL;
  stmt->mysql_bind_vars = NULL;
  stmt->numargs = mysql_stmt_param_count(stmt->mysql_prepstat);
  stmt->numcols = mysql_stmt_field_count(stmt->mysql_prepstat);
#elif defined(DB_SQLITE3)
  if (sqlite3_prepare_v2(db->sqlite3_conn, sql, -1, &(stmt->sqlite3_prepstat), NULL) != SQLITE_OK) {
    cdba_set_error(db, sqlite3_errmsg(db->sqlite3_conn));
    free(stmt);
    return NULL;
  }
  stmt->sqlite3_first_step_status = -1;
  stmt->numargs = sqlite3_bind_parameter_count(stmt->sqlite3_prepstat);
  stmt->numcols = sqlite3_column_count(stmt->sqlite3_prepstat);
#elif defined(DB_ODBC)
  SQLRETURN status;
  SQLSMALLINT n;
  status = SQLAllocHandle(SQL_HANDLE_STMT, db->odbc_conn, &stmt->odbc_prepstat);
  if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO) {
    cdba_set_error(db, "SQLAllocHandle() failed");
    free(stmt);
    return NULL;
  }
  status = SQLPrepare(stmt->odbc_prepstat, (SQLCHAR*)sql, SQL_NTS);
  if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO) {
    cdba_set_odbc_error(db, stmt->odbc_prepstat, SQL_HANDLE_STMT);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt->odbc_prepstat);
    free(stmt);
    return NULL;
  }
/*
  status = SQLExecute(stmt->odbc_prepstat);
  if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO) {
    cdba_set_odbc_error(db, stmt->odbc_prepstat, SQL_HANDLE_STMT);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt->odbc_prepstat);
    free(stmt);
    return NULL;
  }
*/
  stmt->db = db;
  stmt->odbc_first_step_status = -1;
  stmt->numargs = -1;
  status = SQLNumParams(stmt->odbc_prepstat, &n);
  if (status == SQL_SUCCESS || status == SQL_SUCCESS_WITH_INFO)
    stmt->numargs = n;
  stmt->numcols = 0;
  status = SQLNumResultCols(stmt->odbc_prepstat, &n);
  if (status == SQL_SUCCESS || status == SQL_SUCCESS_WITH_INFO)
    stmt->numcols = n;
  if (stmt->numargs <= 0) {
    stmt->odbc_bind_len = NULL;
  } else if ((stmt->odbc_bind_len = (SQLLEN*)malloc(sizeof(SQLLEN) * stmt->numargs)) == NULL) {
    cdba_set_error(db, "Memory allocation error");
    SQLFreeHandle(SQL_HANDLE_STMT, stmt->odbc_prepstat);
    free(stmt);
    return NULL;
  } else {
    memset(stmt->odbc_bind_len, 0, sizeof(SQLLEN) * stmt->numargs);
  }
#else
  free(stmt);
  stmt = NULL;
#endif
  return stmt;
}

DLL_EXPORT_CDBALIB int cdba_prep_get_argument_count (cdba_prep_handle stmt)
{
  return stmt->numargs;
}

DLL_EXPORT_CDBALIB void cdba_prep_reset (cdba_prep_handle stmt)
{
  if (!stmt)
    return;
#if defined(DB_MYSQL)
  mysql_stmt_reset(stmt->mysql_prepstat);
  if (stmt->mysql_result_metadata) {
    mysql_free_result(stmt->mysql_result_metadata);
    stmt->mysql_result_metadata = NULL;
  }
  if (stmt->mysql_bind_result) {
    free(stmt->mysql_bind_result);
    stmt->mysql_bind_result = NULL;
  }
  if (stmt->mysql_bind_vars) {
    free(stmt->mysql_bind_vars);
    stmt->mysql_bind_vars = NULL;
  }
#elif defined(DB_SQLITE3)
  sqlite3_reset(stmt->sqlite3_prepstat);
  sqlite3_clear_bindings(stmt->sqlite3_prepstat);
  stmt->sqlite3_first_step_status = -1;
#elif defined(DB_ODBC)
  stmt->odbc_first_step_status = -1;
  SQLCancel(stmt->odbc_prepstat);
#else
#endif
}

DLL_EXPORT_CDBALIB void cdba_prep_close (cdba_prep_handle stmt)
{
  if (!stmt)
    return;
  if (stmt->errmsg)
    free(stmt->errmsg);
#if defined(DB_MYSQL)
  if (stmt->mysql_prepstat)
    mysql_stmt_close(stmt->mysql_prepstat);
  if (stmt->mysql_result_metadata)
    mysql_free_result(stmt->mysql_result_metadata);
  if (stmt->mysql_bind_result)
    free(stmt->mysql_bind_result);
  if (stmt->mysql_bind_vars)
    free(stmt->mysql_bind_vars);
#elif defined(DB_SQLITE3)
  if (stmt->sqlite3_prepstat)
    sqlite3_finalize(stmt->sqlite3_prepstat);
#elif defined(DB_ODBC)
  if (stmt->odbc_bind_len)
    free(stmt->odbc_bind_len);
  if (stmt->odbc_prepstat) {
    SQLCancel(stmt->odbc_prepstat);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt->odbc_prepstat);
  }
#else
#endif
  free(stmt);
}

DLL_EXPORT_CDBALIB void cdba_prep_set_error (cdba_prep_handle stmt, const char* errmsg)
{
  if (stmt->errmsg)
    free(stmt->errmsg);
  stmt->errmsg = (errmsg ? strdup(errmsg) : NULL);
}

DLL_EXPORT_CDBALIB const char* cdba_prep_get_error (cdba_prep_handle stmt)
{
  return stmt->errmsg;
}

DLL_EXPORT_CDBALIB int cdba_prep_execute (cdba_prep_handle stmt, ...)
{
  int i;
  int type;
  int status = 0;
  va_list argp;
  va_start(argp, stmt);
#if defined(DB_MYSQL)
  MYSQL_BIND* bindarg;
  struct mysql_argbindinfo_struct* argcopy;
  //bind arguments
  if (stmt->numargs == 0) {
    bindarg = NULL;
  } else {
    if ((bindarg = (MYSQL_BIND*)malloc(sizeof(MYSQL_BIND) * stmt->numargs)) == NULL) {
      cdba_prep_set_error(stmt, "Memory allocation error");
      va_end(argp);
      return -1;
    }
  }
  if (stmt->numargs == 0) {
    argcopy = NULL;
  } else {
    if ((argcopy = (struct mysql_argbindinfo_struct*)malloc(sizeof(struct mysql_argbindinfo_struct) * stmt->numargs)) == NULL) {
      cdba_prep_set_error(stmt, "Memory allocation error");
      free(bindarg);
      va_end(argp);
      return -1;
    }
  }
  memset(bindarg, 0, sizeof(MYSQL_BIND) * stmt->numargs);
  for (i = 0; i < stmt->numargs; i++) {
    type = va_arg(argp, int);
    switch (type) {
      case CDBA_TYPE_NULL :
        bindarg[i].buffer_type = MYSQL_TYPE_NULL;
        bindarg[i].buffer = NULL;
        break;
      case CDBA_TYPE_INT :
        argcopy[i].intval = (long long)va_arg(argp, db_int);
        bindarg[i].buffer_type = MYSQL_TYPE_LONGLONG;
        bindarg[i].buffer = &(argcopy[i].intval);
        break;
      case CDBA_TYPE_FLOAT :
        argcopy[i].floatval = va_arg(argp, db_flt);
        bindarg[i].buffer_type = MYSQL_TYPE_DOUBLE;
        bindarg[i].buffer = &(argcopy[i].intval);
        break;
      case CDBA_TYPE_TEXT :
        bindarg[i].buffer = va_arg(argp, char*);
        if (bindarg[i].buffer) {
          bindarg[i].buffer_type = MYSQL_TYPE_STRING;
          bindarg[i].buffer_length = strlen(bindarg[i].buffer);
        } else {
          bindarg[i].buffer_type = MYSQL_TYPE_NULL;
        }
        break;
      default :
        cdba_prep_set_error(stmt, "Unknown database type");
        cdba_prep_reset(stmt);
        va_end(argp);
        return -1;
    }
  }
  mysql_stmt_bind_param(stmt->mysql_prepstat, bindarg);
  //execute statement
  if ((status = mysql_stmt_execute(stmt->mysql_prepstat)) != 0) {
    cdba_prep_set_error(stmt, mysql_stmt_error(stmt->mysql_prepstat));
    if (bindarg)
      free(bindarg);
    if (argcopy)
      free(argcopy);
    cdba_prep_reset(stmt);
    va_end(argp);
    return -1;
  }
  //bind results
  if (stmt->numcols > 0) {
    //allocate space for bind data
    if (stmt->mysql_bind_result)
      free(stmt->mysql_bind_result);
    if ((stmt->mysql_bind_result = (MYSQL_BIND*)malloc(sizeof(MYSQL_BIND) * stmt->numcols)) == NULL) {
      cdba_prep_set_error(stmt, "Memory allocation error");
      if (bindarg)
        free(bindarg);
      if (argcopy)
        free(argcopy);
      cdba_prep_reset(stmt);
      va_end(argp);
      return -1;
    }
    memset(stmt->mysql_bind_result, 0, sizeof(MYSQL_BIND) * stmt->numcols);
    if (stmt->mysql_bind_vars )
      free(stmt->mysql_bind_vars );
    if ((stmt->mysql_bind_vars = (struct mysql_resultbindinfo_struct*)malloc(sizeof(struct mysql_resultbindinfo_struct) * stmt->numcols)) == NULL) {
      cdba_prep_set_error(stmt, "Memory allocation error");
      if (bindarg)
        free(bindarg);
      if (argcopy)
        free(argcopy);
      free(stmt->mysql_bind_result);
      cdba_prep_reset(stmt);
      va_end(argp);
      return -1;
    }
    /////memset(stmt->mysql_bind_result, 0, sizeof(struct mysql_resultbindinfo_struct) * stmt->numcols);
    //populate bind data
    if (stmt->mysql_result_metadata)
      mysql_free_result(stmt->mysql_result_metadata);
    stmt->mysql_result_metadata = mysql_stmt_result_metadata(stmt->mysql_prepstat);
    for (i = 0; i < stmt->numcols; i++) {
      switch (cdba_prep_get_column_type(stmt, i)) {
        case CDBA_TYPE_NULL :
          stmt->mysql_bind_result[i].buffer_type = MYSQL_TYPE_NULL;
          stmt->mysql_bind_result[i].buffer = NULL;
          stmt->mysql_bind_result[i].buffer_length = 0;
          break;
        case CDBA_TYPE_INT :
          stmt->mysql_bind_result[i].buffer_type = MYSQL_TYPE_LONGLONG;
          stmt->mysql_bind_result[i].buffer = &(stmt->mysql_bind_vars[i].value.intval);
          stmt->mysql_bind_result[i].buffer_length = sizeof(stmt->mysql_bind_vars[i].value.intval);
          break;
        case CDBA_TYPE_FLOAT :
          stmt->mysql_bind_result[i].buffer_type = MYSQL_TYPE_DOUBLE;
          stmt->mysql_bind_result[i].buffer = &(stmt->mysql_bind_vars[i].value.floatval);
          stmt->mysql_bind_result[i].buffer_length = sizeof(stmt->mysql_bind_vars[i].value.floatval);
          break;
        case CDBA_TYPE_TEXT :
          stmt->mysql_bind_result[i].buffer_type = MYSQL_TYPE_STRING;
          stmt->mysql_bind_result[i].buffer = NULL;
          stmt->mysql_bind_result[i].buffer_length = 0;
          break;
/*
        case CDBA_TYPE_BLOB :
*/
        default :
          stmt->mysql_bind_result[i].buffer_type = MYSQL_TYPE_NULL;
          stmt->mysql_bind_result[i].buffer = NULL;
          stmt->mysql_bind_result[i].buffer_length = 0;
         break;
      }
      stmt->mysql_bind_result[i].length = &(stmt->mysql_bind_vars[i].length);
      stmt->mysql_bind_result[i].is_null = &(stmt->mysql_bind_vars[i].is_null);
    }
    mysql_stmt_bind_result(stmt->mysql_prepstat, stmt->mysql_bind_result);
  }
  if (bindarg)
    free(bindarg);
  if (argcopy)
    free(argcopy);
#elif defined(DB_SQLITE3)
  //bind arguments
  for (i = 0; i < stmt->numargs; i++) {
    type = va_arg(argp, int);
    switch (type) {
      case CDBA_TYPE_NULL :
        sqlite3_bind_null(stmt->sqlite3_prepstat, i + 1);
        break;
      case CDBA_TYPE_INT :
        {
          int64_t val = va_arg(argp, db_int);
          sqlite3_bind_int64(stmt->sqlite3_prepstat, i + 1, val);
        }
        break;
      case CDBA_TYPE_FLOAT :
        {
          double val = va_arg(argp, db_flt);
          sqlite3_bind_double(stmt->sqlite3_prepstat, i + 1, val);
        }
        break;
      case CDBA_TYPE_TEXT :
        {
          const char* val = va_arg(argp, const char*);
          if (val)
            sqlite3_bind_text(stmt->sqlite3_prepstat, i + 1, val, -1, SQLITE_STATIC);
          else
            sqlite3_bind_null(stmt->sqlite3_prepstat, i + 1);
        }
        break;
      default :
        cdba_prep_set_error(stmt, "Unknown database type");
        sqlite3_reset(stmt->sqlite3_prepstat);
        sqlite3_clear_bindings(stmt->sqlite3_prepstat);
        va_end(argp);
        return -1;
    }
  }
  //fetch first row
  stmt->sqlite3_first_step_status = -1;
  i = 0;
  while (i++ < RETRY_ATTEMPTS && ((stmt->sqlite3_first_step_status = sqlite3_step(stmt->sqlite3_prepstat)) == SQLITE_BUSY || stmt->sqlite3_first_step_status == SQLITE_LOCKED)) {
    WAIT_BEFORE_RETRY(RETRY_WAIT_TIME)
  }
  switch (stmt->sqlite3_first_step_status) {
    case SQLITE_OK :
    case SQLITE_DONE :
    case SQLITE_ROW :
      status = 0;
      break;
    default :
      status = -1;
      cdba_prep_set_error(stmt, sqlite3_errmsg(sqlite3_db_handle(stmt->sqlite3_prepstat)));
      break;
  }
#elif defined(DB_ODBC)
  SQLRETURN odbcstatus;
  //bind arguments
  for (i = 0; i < stmt->numargs; i++) {
    type = va_arg(argp, int);
    switch (type) {
      case CDBA_TYPE_NULL :
        stmt->odbc_bind_len[i] = SQL_NULL_DATA;
        odbcstatus = SQLBindParameter(stmt->odbc_prepstat, i + 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARCHAR, 1, 1, NULL, 0, &stmt->odbc_bind_len[i]);
        break;
      case CDBA_TYPE_INT :
        {
          //SQLBIGINT val = va_arg(argp, db_int);
          SQLINTEGER val = va_arg(argp, db_int);
          //stmt->odbc_bind_len[i] = sizeof(val);
          //odbcstatus = SQLBindParameter(stmt->odbc_prepstat, i + 1, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0, &val, 0, NULL);
          odbcstatus = SQLBindParameter(stmt->odbc_prepstat, i + 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &val, 0, NULL);
        }
        break;
      case CDBA_TYPE_FLOAT :
        {
          SQLDOUBLE val = va_arg(argp, db_flt);
          //stmt->odbc_bind_len[i] = sizeof(val);
          odbcstatus = SQLBindParameter(stmt->odbc_prepstat, i + 1, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DOUBLE, 0, 0, &val, 0, NULL);
        }
        break;
      case CDBA_TYPE_TEXT :
        {
          SQLCHAR* val = va_arg(argp, char*);
          stmt->odbc_bind_len[i] = (val ? SQL_NTS : SQL_NULL_DATA);
          odbcstatus = SQLBindParameter(stmt->odbc_prepstat, i + 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARCHAR, 12, 12, (val ? val : NULL), (val ? strlen(val) : 0), &stmt->odbc_bind_len[i]);
        }
        break;
      default :
        cdba_prep_set_error(stmt, "Unknown database type");
        va_end(argp);
        return -1;
    }
printf("odbcstatus[%i](type:%i): %i\n", (int)i, (int)type, (int)odbcstatus);/////
  }
  odbcstatus = SQLExecute(stmt->odbc_prepstat);
printf("odbcstatus: %i\n", (int)odbcstatus);/////
  if (odbcstatus != SQL_SUCCESS && odbcstatus != SQL_SUCCESS_WITH_INFO) {
    cdba_set_odbc_error(stmt->db, stmt->odbc_prepstat, SQL_HANDLE_STMT);
    /////cdba_set_odbc_error(NULL, stmt->odbc_prepstat, SQL_HANDLE_STMT);
    return -2;
  }
  stmt->odbc_first_step_status = 1;
  status = odbcstatus;
#else
#endif
  va_end(argp);
  return status;
}

DLL_EXPORT_CDBALIB db_int cdba_prep_get_rows_affected (cdba_prep_handle stmt)
{
#if defined(DB_MYSQL)
  return mysql_stmt_affected_rows(stmt->mysql_prepstat);
#elif defined(DB_SQLITE3)
  return sqlite3_changes(sqlite3_db_handle(stmt->sqlite3_prepstat));
#elif defined(DB_ODBC)
  SQLLEN rows = 0;
  SQLRowCount(stmt->odbc_prepstat, &rows);
  return (db_int)rows;
#else
#endif
}

DLL_EXPORT_CDBALIB db_int cdba_prep_get_insert_id (cdba_prep_handle stmt)
{
#if defined(DB_MYSQL)
  return mysql_stmt_insert_id(stmt->mysql_prepstat);
#elif defined(DB_SQLITE3)
  return sqlite3_last_insert_rowid(sqlite3_db_handle(stmt->sqlite3_prepstat));
#elif defined(DB_ODBC)
#else
#endif
}

DLL_EXPORT_CDBALIB int cdba_prep_fetch_row (cdba_prep_handle stmt)
{
#if defined(DB_MYSQL)
  int status;
	status = mysql_stmt_fetch(stmt->mysql_prepstat);
	if (status == 0 || status == MYSQL_DATA_TRUNCATED)
    return 1;
  return (status == MYSQL_NO_DATA ? 0 : -1);
#elif defined(DB_SQLITE3)
  int status;
  int i;
  if (stmt->sqlite3_first_step_status != -1) {
    status = stmt->sqlite3_first_step_status;
    stmt->sqlite3_first_step_status = -1;
  } else {
    status = SQLITE_ERROR;
    i = 0;
    while (i++ < RETRY_ATTEMPTS && ((status = sqlite3_step(stmt->sqlite3_prepstat)) == SQLITE_BUSY || status == SQLITE_LOCKED)) {
      WAIT_BEFORE_RETRY(RETRY_WAIT_TIME)
    }
  }
  if (status == SQLITE_ROW)
    return 1;
  return (status == SQLITE_DONE || status == SQLITE_OK ? 0 : -1);
#elif defined(DB_ODBC)
  if (stmt->odbc_first_step_status == 1) {
    stmt->odbc_first_step_status = 0;       /////TO DO: what id query returns no results?
    return 1;
  }
  SQLRETURN status;
  status = SQLFetch(stmt->odbc_prepstat);
  return (status == SQL_SUCCESS || status == SQL_SUCCESS_WITH_INFO ? 1 : 0);
#else
#endif
}

DLL_EXPORT_CDBALIB int cdba_prep_get_column_count (cdba_prep_handle stmt)
{
  return stmt->numcols;
}

DLL_EXPORT_CDBALIB db_int cdba_prep_get_column_type (cdba_prep_handle stmt, int col)
{
#if defined(DB_MYSQL)
  switch (stmt->mysql_result_metadata->fields[col].type) {
    case MYSQL_TYPE_NULL :
      return CDBA_TYPE_NULL;
    case MYSQL_TYPE_BIT :
#ifdef MYSQL_TYPE_BOOL
    case MYSQL_TYPE_BOOL :
#endif
    case MYSQL_TYPE_DECIMAL :
    case MYSQL_TYPE_NEWDECIMAL :
    case MYSQL_TYPE_TINY :
    case MYSQL_TYPE_SHORT :
    case MYSQL_TYPE_INT24 :
    case MYSQL_TYPE_LONG :
    case MYSQL_TYPE_LONGLONG :
    case MYSQL_TYPE_TIMESTAMP :
    case MYSQL_TYPE_TIMESTAMP2 :
    case MYSQL_TYPE_DATE :
    case MYSQL_TYPE_TIME :
    case MYSQL_TYPE_DATETIME :
    case MYSQL_TYPE_YEAR :
      return CDBA_TYPE_INT;
    case MYSQL_TYPE_FLOAT :
    case MYSQL_TYPE_DOUBLE :
      return CDBA_TYPE_FLOAT;
    case MYSQL_TYPE_VARCHAR :
    case MYSQL_TYPE_VAR_STRING :
    case MYSQL_TYPE_STRING :
    case MYSQL_TYPE_TINY_BLOB :
    case MYSQL_TYPE_MEDIUM_BLOB :
    case MYSQL_TYPE_LONG_BLOB :
    case MYSQL_TYPE_BLOB :
      return CDBA_TYPE_TEXT;
      //return CDBA_TYPE_BLOB;
    default :
      return CDBA_TYPE_NULL;
  }
#elif defined(DB_SQLITE3)
  int type;
  type = sqlite3_column_type(stmt->sqlite3_prepstat, col);
  switch (type) {
    case SQLITE_NULL :
      return CDBA_TYPE_NULL;
    case SQLITE_INTEGER :
      return CDBA_TYPE_INT;
    case SQLITE_FLOAT :
      return CDBA_TYPE_FLOAT;
    case SQLITE_TEXT :
      return CDBA_TYPE_TEXT;
/*
    case SQLITE_BLOB :
      return CDBA_TYPE_BLOB;
*/
    default :
      return CDBA_TYPE_NULL;
  }
#elif defined(DB_ODBC)
  SQLRETURN status;
  SQLSMALLINT type;
  status = SQLDescribeCol(stmt->odbc_prepstat, col + 1, NULL, 0, NULL, &type, NULL, NULL, NULL);
  if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO)
    return CDBA_TYPE_NULL;
  switch (type) {
    case SQL_DECIMAL :
    case SQL_SMALLINT :
    case SQL_INTEGER :
    case SQL_BIT :
    case SQL_TINYINT :
    case SQL_BIGINT :
      return CDBA_TYPE_INT;
    case SQL_NUMERIC :
    case SQL_REAL :
    case SQL_FLOAT :
    case SQL_DOUBLE :
      return CDBA_TYPE_FLOAT;
    case SQL_CHAR :
    case SQL_VARCHAR :
    case SQL_LONGVARCHAR :
      return CDBA_TYPE_TEXT;
/*
    case SQL_BINARY :
    case SQL_VARBINARY :
    case SQL_LONGVARBINARY :
      return CDBA_TYPE_BLOB;
*/
/*
    case SQL_TYPE_TIME :
    case SQL_TYPE_TIMESTAMP :
    case SQL_TYPE_UTCDATETIME :
    case SQL_TYPE_UTCTIME :
*/
/*
    case SQL_WCHAR :
    case SQL_WVARCHAR :
    case SQL_WLONGVARCHAR :
*/
    default :
      return CDBA_TYPE_NULL;
  }
#else
#endif
}

DLL_EXPORT_CDBALIB char* cdba_prep_get_column_name (cdba_prep_handle stmt, int col)
{
#if defined(DB_MYSQL)
  const char* colname = stmt->mysql_result_metadata->fields[col].name;
  return (colname ? strdup(colname) : NULL);
#elif defined(DB_SQLITE3)
  const char* colname = sqlite3_column_name(stmt->sqlite3_prepstat, col);
  return (colname ? strdup(colname) : NULL);
#elif defined(DB_ODBC)
  SQLRETURN status;
  SQLCHAR* colname;
  SQLSMALLINT colnamelen;
  colnamelen = 0;
  status = SQLDescribeCol(stmt->odbc_prepstat, col + 1, NULL, 0, &colnamelen, NULL, NULL, NULL, NULL);
  if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO)
    return NULL;
  if ((colname = (SQLCHAR*)malloc(sizeof(SQLCHAR) * ++colnamelen)) == NULL)
    return NULL;
  status = SQLDescribeCol(stmt->odbc_prepstat, col + 1, colname, colnamelen, &colnamelen, NULL, NULL, NULL, NULL);
  if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO) {
    free(colname);
    return NULL;
  }
  return colname;
#else
  return NULL;
#endif
  return NULL;
}

DLL_EXPORT_CDBALIB db_int cdba_prep_get_column_int (cdba_prep_handle stmt, int col)
{
#if defined(DB_MYSQL)
  MYSQL_BIND bindarg;
  long long result = 0;
  memset(&bindarg, 0, sizeof(MYSQL_BIND));
  bindarg.buffer_type = MYSQL_TYPE_LONGLONG;
  bindarg.buffer = &result;
  if (mysql_stmt_fetch_column(stmt->mysql_prepstat, &bindarg, col, 0) != 0)
    return 0;
  return result;
#elif defined(DB_SQLITE3)
  return sqlite3_column_int64(stmt->sqlite3_prepstat, col);
#elif defined(DB_ODBC)
  SQLLEN value_size;
  SQLINTEGER value = -1;
  if (SQLGetData(stmt->odbc_prepstat, (SQLUSMALLINT)(col + 1), SQL_INTEGER, (SQLCHAR*)&value, (SQLLEN)sizeof(value), &value_size) == SQL_SUCCESS) {
    if (value_size == SQL_NULL_DATA || value_size == SQL_NO_TOTAL)
      value = 0;
  }
  return value;
#else
  return 0;
#endif
}

DLL_EXPORT_CDBALIB double cdba_prep_get_column_float (cdba_prep_handle stmt, int col)
{
#if defined(DB_MYSQL)
  MYSQL_BIND bindarg;
  double result = 0;
  memset(&bindarg, 0, sizeof(MYSQL_BIND));
  bindarg.buffer_type = MYSQL_TYPE_DOUBLE;
  bindarg.buffer = &result;
  if (mysql_stmt_fetch_column(stmt->mysql_prepstat, &bindarg, col, 0) != 0)
    return 0;
  return result;
#elif defined(DB_SQLITE3)
  return sqlite3_column_double(stmt->sqlite3_prepstat, col);
#elif defined(DB_ODBC)
  SQLLEN value_size;
  SQLDOUBLE value = -1;
  if (SQLGetData(stmt->odbc_prepstat, (SQLUSMALLINT)(col + 1), SQL_DOUBLE, (SQLCHAR*)&value, (SQLLEN)sizeof(value), &value_size) == SQL_SUCCESS) {
    if (value_size == SQL_NULL_DATA || value_size == SQL_NO_TOTAL)
      value = 0;
  }
  return value;
#else
  return 0;
#endif
}

DLL_EXPORT_CDBALIB char* cdba_prep_get_column_text (cdba_prep_handle stmt, int col)
{
  char* result = NULL;
#if defined(DB_MYSQL)
  MYSQL_BIND mysql_bind_var;
  if (stmt->mysql_bind_result[col].buffer_type != MYSQL_TYPE_STRING)
    return strdup("ERROR (not a string)");
    //return NULL;
  if (stmt->mysql_bind_vars[col].is_null || (result = (char*)malloc(stmt->mysql_bind_vars[col].length + 1)) == NULL)
    return NULL;
  memset(&mysql_bind_var, 0, sizeof(mysql_bind_var));
  mysql_bind_var.buffer_type = MYSQL_TYPE_STRING;
  mysql_bind_var.buffer = result;
  mysql_bind_var.buffer_length = stmt->mysql_bind_vars[col].length + 1;
  mysql_bind_var.length = &(stmt->mysql_bind_vars[col].length);
  mysql_bind_var.is_null = &(stmt->mysql_bind_vars[col].is_null);
  if (mysql_stmt_fetch_column(stmt->mysql_prepstat, &mysql_bind_var, col, 0) != 0) {
    free(result);
    return NULL;
  }
  //result[stmt->mysql_bind_vars[col].length] = 0;
#elif defined(DB_SQLITE3)
  if ((result = (char*)sqlite3_column_text(stmt->sqlite3_prepstat, col)) != NULL)
    result = strdup(result);
#elif defined(DB_ODBC)
#define STRING_ALLOCATE_INITIAL 256 *0+2
#define STRING_ALLOCATE_STEP 2048
  RETCODE status;
  SQLLEN len;
  status = SQLGetData(stmt->odbc_prepstat, (SQLUSMALLINT)(col + 1), SQL_C_CHAR, (SQLCHAR*)&result, 0, &len);
  if (status == SQL_SUCCESS) {
    if (len == SQL_NULL_DATA) {
      free(result);
      return NULL;
    }
    return strdup("");
  } else if (status == SQL_SUCCESS_WITH_INFO) {
    if ((result = (char*)malloc(++len)) == NULL)
      return NULL;
    status = SQLGetData(stmt->odbc_prepstat, (SQLUSMALLINT)(col + 1), SQL_C_CHAR, (SQLCHAR*)result, len, &len);
    if (status != SQL_SUCCESS) {
      free(result);
      return NULL;
    }
  } else {
    free(result);
    return NULL;
  }
/*
  RETCODE status;
  SQLLEN len;
  int pos = 0;
  if ((result = (char*)malloc(STRING_ALLOCATE_INITIAL)) == NULL) {
    return NULL;
  }
  status = SQLGetData(stmt->odbc_prepstat, (SQLUSMALLINT)(col + 1), SQL_C_CHAR, (SQLCHAR*)result, STRING_ALLOCATE_INITIAL, &len);
  if (status == SQL_SUCCESS) {
    if (len == SQL_NULL_DATA) {
      free(result);
      return NULL;
    }
  } else if (status == SQL_SUCCESS_WITH_INFO) {
    do {
      pos += (len != SQL_NO_TOTAL ? len : strlen(result + pos));
      if ((result = (char*)realloc(result, pos + STRING_ALLOCATE_STEP)) == NULL) {
        free(result);
        return NULL;
      }
    } while ((status = SQLGetData(stmt->odbc_prepstat, (SQLUSMALLINT)(col + 1), SQL_C_CHAR, (SQLCHAR*)result + pos, STRING_ALLOCATE_STEP, &len)) == SQL_SUCCESS_WITH_INFO);
  } else {
    free(result);
    return NULL;
  }
*/
#else
#endif
  return result;
}

////////////////////////////////////////////////////////////////////////

DLL_EXPORT_CDBALIB void cdba_free (void* data)
{
  if (data)
    free(data);
}

////////////////////////////////////////////////////////////////////////

DLL_EXPORT_CDBALIB void cdba_get_version (int* pmajor, int* pminor, int* pmicro)
{
  if (pmajor)
    *pmajor = CDBALIB_VERSION_MAJOR;
  if (pminor)
    *pminor = CDBALIB_VERSION_MINOR;
  if (pmicro)
    *pmicro = CDBALIB_VERSION_MICRO;
}

DLL_EXPORT_CDBALIB const char* cdba_get_version_string ()
{
  return CDBALIB_VERSION_STRING;
}



/*
  List tables:
    SQLite3
      SELECT name FROM sqlite_master WHERE type='table'
  MySQL
      SHOW TABLES WHERE Table_type = 'BASE TABLE'
      SELECT TABLE_NAME FROM information_schema.TABLES WHERE TABLE_SCHEMA LIKE '<table_name>' AND TABLE_TYPE LIKE 'BASE_TABLE';
*/

//ODBC: see also: https://learn.microsoft.com/en-us/sql/connect/odbc/cpp-code-example-app-connect-access-sql-db?view=sql-server-ver16
//ODBC: see also: https://learn.microsoft.com/en-us/sql/relational-databases/native-client-odbc-how-to/execute-queries/prepare-and-execute-a-statement-odbc?view=sql-server-ver16
//ODBC: see also: https://www.easysoft.com/developer/languages/c/examples/ListDBTables.html
//ODBC: see also: https://www.easysoft.com/developer/languages/c/examples/Transactions.html

