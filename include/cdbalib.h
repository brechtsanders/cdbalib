/*!
 * \file cdbalib.h
 * \brief CDBALIB - C database abstraction library with support for prepared statements - header file with main functions
 * \details CDBALIB is a C database abstraction library with support for prepared statements. This is the header file with the main functions.
 * \author Brecht Sanders
 * \copyright LGPL License
 *
 * Header file for the C database abstraction library with support for prepared statements (CDBALIB)
 */

#ifndef INCLUDED_CDBALIB_H
#define INCLUDED_CDBALIB_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>



/*! \cond PRIVATE */
#if !defined(DLL_EXPORT_CDBALIB)
# if defined(_WIN32) && (defined(BUILD_CDBALIB_DLL) || defined(CDBALIB_EXPORTS))
#  define DLL_EXPORT_CDBALIB __declspec(dllexport)
# elif /*defined(_WIN32)*/defined(__MINGW32__) && !defined(STATIC) && !defined(STATIC_CDBALIB) && !defined(BUILD_CDBALIB)
#  define DLL_EXPORT_CDBALIB __declspec(dllimport)
# else
#  define DLL_EXPORT_CDBALIB
# endif
#endif
/*! \endcond */



#ifdef __cplusplus
extern "C" {
#endif



/*! \brief integer type used by CDBALIB */
typedef int64_t db_int;
typedef double db_flt;

/*! \brief database data types
 * \sa     cdba_prep_get_column_type()
 * \sa     cdba_prep_execute()
 * \name   CDBA_TYPE_*
 * \{
 */
#define CDBA_TYPE_NULL  ((db_int)0)      /**< NULL type */
#define CDBA_TYPE_INT   ((db_int)1)      /**< integer number type */
#define CDBA_TYPE_FLOAT ((db_int)2)      /**< floating point number type */
#define CDBA_TYPE_TEXT  ((db_int)3)      /**< text type */
//#define CDBA_TYPE_BLOB  ((db_int)4)      /**< binary large object type type */



/*! \brief database handle type
 * \sa     cdba_library_initialize()
 * \sa     cdba_library_cleanup()
 * \sa     cdba_open()
 */
typedef struct cdba_library_handle_struct* cdba_library_handle;

/*! \brief initialize library (must be called once at the beginning of the program)
 * \return database library handle on success or NULL on error
 * \sa     cdba_library_cleanup()
 * \sa     cdba_open()
 */
DLL_EXPORT_CDBALIB cdba_library_handle cdba_library_initialize ();

/*! \brief clean up library (must be called once at the end of the program)
 * \param  dblib                 database library handle
 * \sa     cdba_library_initialize()
 */
DLL_EXPORT_CDBALIB void cdba_library_cleanup (cdba_library_handle dblib);

/*! \brief get name of database driver
 * \param  dblib                 database library handle
 * \return name of database driver
 * \sa     cdba_library_initialize()
 */
DLL_EXPORT_CDBALIB const char* cdba_library_get_name (cdba_library_handle dblib);

/*! \brief get version of database driver
 * \param  dblib                 database library handle
 * \return version of database driver, the caller must call cdba_free() on the result when it is no longer needed
 * \sa     cdba_library_initialize()
 */
DLL_EXPORT_CDBALIB char* cdba_library_get_version (cdba_library_handle dblib);



/*! \brief database handle type
 * \sa     cdba_open()
 * \sa     cdba_close()
 * \sa     cdba_set_error()
 * \sa     cdba_get_error()
 * \sa     cdba_sql()
 * \sa     cdba_begin_transaction()
 * \sa     cdba_commit_transaction()
 * \sa     cdba_rollback_transaction()
 * \sa     cdba_create_preparedstatement()
 */
typedef struct cdba_handle_struct* cdba_handle;

/*! \brief open new database connection
 * \param  dblib                 database library handle
 * \param  configtext            database settings (key=value pairs separated by spaces or semicolons, double quotes are supported and backslash can be used inside double quotes to escape characters)
 * \return database handle on success or NULL on error
 * \sa     cdba_close()
 * \sa     cdba_set_error()
 * \sa     cdba_get_error()
 * \sa     cdba_sql()
 * \sa     cdba_begin_transaction()
 * \sa     cdba_commit_transaction()
 * \sa     cdba_rollback_transaction()
 * \sa     cdba_create_preparedstatement()
 */
DLL_EXPORT_CDBALIB cdba_handle cdba_open (cdba_library_handle dblib, const char* configtext);

/*! \brief close database connection
 * \param  db                    database handle
 * \sa     cdba_open()
 */
DLL_EXPORT_CDBALIB void cdba_close (cdba_handle db);

/*! \brief set database error message (normally only for internal use)
 * \param  db                    database handle
 * \param  errmsg                database handle
 * \sa     cdba_open()
 * \sa     cdba_get_error()
 */
DLL_EXPORT_CDBALIB void cdba_set_error (cdba_handle db, const char* errmsg);

/*! \brief get last database error message
 * \param  db                    database handle
 * \return error message if any, otherwise NULL
 * \sa     cdba_open()
 * \sa     cdba_sql()
 * \sa     cdba_create_preparedstatement()
 * \sa     cdba_set_error()
 */
DLL_EXPORT_CDBALIB const char* cdba_get_error (cdba_handle db);

/*! \brief execute a database SQL statement
 * \param  db                    database handle
 * \param  sql                   SQL statement
 * \return zero on success, non-zero on error
 * \sa     cdba_sql_with_length()
 * \sa     cdba_multiple_sql()
 * \sa     cdba_open()
 * \sa     cdba_close()
 * \sa     cdba_get_error()
 */
DLL_EXPORT_CDBALIB int cdba_sql (cdba_handle db, const char* sql);

/*! \brief execute a database SQL statement
 * \param  db                    database handle
 * \param  sql                   SQL statement
 * \param  sqllen                length of SQL statement
 * \return zero on success, non-zero on error
 * \sa     cdba_sql()
 * \sa     cdba_multiple_sql()
 * \sa     cdba_open()
 * \sa     cdba_close()
 * \sa     cdba_get_error()
 */
DLL_EXPORT_CDBALIB int cdba_sql_with_length (cdba_handle db, const char* sql, size_t sqllen);

/*! \brief execute a database SQL statement
 * \param  db                    database handle
 * \param  sql                   SQL statements (can be multiple separated by semicolon)
 * \return zero on success, non-zero on error
 * \sa     cdba_sql()
 * \sa     cdba_sql_with_length()
 * \sa     cdba_open()
 * \sa     cdba_close()
 * \sa     cdba_get_error()
 */
DLL_EXPORT_CDBALIB int cdba_multiple_sql (cdba_handle db, const char* sql);

/*! \brief begin transaction
 * \param  db                    database handle
 * \sa     cdba_commit_transaction()
 * \sa     cdba_rollback_transaction()
 * \sa     cdba_open()
 * \sa     cdba_sql()
 * \sa     cdba_create_preparedstatement()
 */
DLL_EXPORT_CDBALIB void cdba_begin_transaction (cdba_handle db);

/*! \brief end transaction and commit changes
 * \param  db                    database handle
 * \sa     cdba_begin_transaction()
 * \sa     cdba_rollback_transaction()
 * \sa     cdba_open()
 * \sa     cdba_sql()
 * \sa     cdba_create_preparedstatement()
 */
DLL_EXPORT_CDBALIB void cdba_commit_transaction (cdba_handle db);

/*! \brief end transaction and rollback changes
 * \param  db                    database handle
 * \sa     cdba_begin_transaction()
 * \sa     cdba_commit_transaction()
 * \sa     cdba_open()
 * \sa     cdba_sql()
 * \sa     cdba_create_preparedstatement()
 */
DLL_EXPORT_CDBALIB void cdba_rollback_transaction (cdba_handle db);



/*! \brief database handle type
 * \sa     cdba_create_preparedstatement()
 * \sa     cdba_prep_get_argument_count()
 * \sa     cdba_prep_reset()
 * \sa     cdba_prep_close()
 * \sa     cdba_prep_set_error()
 * \sa     cdba_prep_get_error()
 * \sa     cdba_prep_execute()
 * \sa     cdba_prep_get_rows_affected()
 * \sa     cdba_prep_get_insert_id()
 * \sa     cdba_prep_fetch_row()
 * \sa     cdba_prep_get_column_count()
 * \sa     cdba_prep_get_column_type()
 * \sa     cdba_prep_get_column_name()
 * \sa     cdba_prep_get_column_int()
 * \sa     cdba_prep_get_column_float()
 * \sa     cdba_prep_get_column_text()
 */
typedef struct cdba_prep_handle_struct* cdba_prep_handle;

/*! \brief prepare an SQL statement
 * \param  db                    database handle
 * \param  sql                   SQL statement
 * \return prepared statement handle on success or NULL on error
 * \sa     cdba_open()
 * \sa     cdba_get_error()
 * \sa     cdba_prep_reset()
 * \sa     cdba_prep_close()
 * \sa     cdba_prep_set_error()
 * \sa     cdba_prep_get_error()
 * \sa     cdba_prep_execute()
 * \sa     cdba_prep_get_rows_affected()
 * \sa     cdba_prep_get_insert_id()
 * \sa     cdba_prep_fetch_row()
 * \sa     cdba_prep_get_column_count()
 * \sa     cdba_prep_get_column_type()
 * \sa     cdba_prep_get_column_name()
 * \sa     cdba_prep_get_column_int()
 * \sa     cdba_prep_get_column_float()
 * \sa     cdba_prep_get_column_text()
 */
DLL_EXPORT_CDBALIB cdba_prep_handle cdba_create_preparedstatement (cdba_handle db, const char* sql);

/*! \brief get number of arguments in prepared statement
 * \param  stmt                  prepared statement handle
 * \return number of arguments in prepared statement or < 0 if not supported
 * \sa     cdba_create_preparedstatement()
 */
DLL_EXPORT_CDBALIB int cdba_prep_get_argument_count (cdba_prep_handle stmt);

/*! \brief reset prepared statement so it can be reused
 * \param  stmt                  prepared statement handle
 * \sa     cdba_create_preparedstatement()
 * \sa     cdba_prep_execute()
 * \sa     cdba_prep_close()
 */
DLL_EXPORT_CDBALIB void cdba_prep_reset (cdba_prep_handle stmt);

/*! \brief close prepared statement
 * \param  stmt                  prepared statement handle
 * \sa     cdba_create_preparedstatement()
 */
DLL_EXPORT_CDBALIB void cdba_prep_close (cdba_prep_handle stmt);

/*! \brief set prepared statement (normally only for internal use)
 * \param  stmt                  prepared statement handle
 * \param  errmsg                database handle
 * \sa     cdba_create_preparedstatement()
 * \sa     cdba_prep_get_error()
 */
DLL_EXPORT_CDBALIB void cdba_prep_set_error (cdba_prep_handle stmt, const char* errmsg);

/*! \brief get last prepared statement error message
 * \param  stmt                  prepared statement handle
 * \return error message if any, otherwise NULL
 * \sa     cdba_create_preparedstatement()
 * \sa     cdba_prep_execute()
 * \sa     cdba_prep_set_error()
 */
DLL_EXPORT_CDBALIB const char* cdba_prep_get_error (cdba_prep_handle stmt);

/*! \brief execute a database SQL prepared statement
 * \param  stmt                  prepared statement handle
 * \param  ...                   arguments defined as pairs of CDBALIB_VERSION_* and a value of the corresponding type
 * \return zero on success, non-zero on error
 * \sa     cdba_create_preparedstatement()
 * \sa     CDBALIB_VERSION_*
 * \sa     cdba_prep_get_error()
 * \sa     cdba_prep_get_argument_count()
 * \sa     cdba_prep_get_rows_affected()
 * \sa     cdba_prep_get_insert_id()
 * \sa     cdba_prep_get_column_count()
 * \sa     cdba_prep_get_column_name()
 * \sa     cdba_prep_fetch_row()
 */
DLL_EXPORT_CDBALIB int cdba_prep_execute (cdba_prep_handle stmt, ...);

/*! \brief get number of rows affected after prepared statement was executed
 * \param  stmt                  prepared statement handle
 * \return number of rows affected
 * \sa     cdba_prep_execute()
 */
DLL_EXPORT_CDBALIB db_int cdba_prep_get_rows_affected (cdba_prep_handle stmt);

/*! \brief get automatically assigned row ID after prepared statement was executed (only valid for INSERT statements)
 * \param  stmt                  prepared statement handle
 * \return automatically assigned row ID (only valid for INSERT statements)
 * \sa     cdba_prep_execute()
 */
DLL_EXPORT_CDBALIB db_int cdba_prep_get_insert_id (cdba_prep_handle stmt);

/*! \brief fetch next row of data from executed prepared statement
 * \param  stmt                  prepared statement handle
 * \return 0 on success but no more data to fetch, positive when data was fetched or negative on error
 * \sa     cdba_prep_execute()
 */
DLL_EXPORT_CDBALIB int cdba_prep_fetch_row (cdba_prep_handle stmt);

/*! \brief get number of columns in result of executed prepared statement
 * \param  stmt                  prepared statement handle
 * \return number of columns
 * \sa     cdba_prep_execute()
 * \sa     cdba_prep_get_column_type()
 * \sa     cdba_prep_get_column_name()
 * \sa     cdba_prep_get_column_int()
 * \sa     cdba_prep_get_column_float()
 * \sa     cdba_prep_get_column_text()
 */
DLL_EXPORT_CDBALIB int cdba_prep_get_column_count (cdba_prep_handle stmt);

/*! \brief get column type in result of executed prepared statement
 * \param  stmt                  prepared statement handle
 * \param  col                   column number (first column is 0)
 * \return column type (one of CDBALIB_VERSION_*)
 * \sa     cdba_prep_execute()
 * \sa     cdba_prep_get_column_count()
 * \sa     CDBALIB_VERSION_*
 */
DLL_EXPORT_CDBALIB db_int cdba_prep_get_column_type (cdba_prep_handle stmt, int col);

/*! \brief get column name in result of executed prepared statement
 * \param  stmt                  prepared statement handle
 * \param  col                   column number (first column is 0)
 * \return column name, the caller must call cdba_free() on the result when it is no longer needed
 * \sa     cdba_prep_execute()
 * \sa     cdba_prep_get_column_count()
 */
DLL_EXPORT_CDBALIB char* cdba_prep_get_column_name (cdba_prep_handle stmt, int col);

/*! \brief get integer value of column from result of executed prepared statement
 * \param  stmt                  prepared statement handle
 * \param  col                   column number (first column is 0)
 * \return integer column value
 * \sa     cdba_prep_execute()
 * \sa     cdba_prep_get_column_count()
 * \sa     cdba_prep_get_column_type()
 * \sa     cdba_prep_get_column_float()
 * \sa     cdba_prep_get_column_text()
 */
DLL_EXPORT_CDBALIB db_int cdba_prep_get_column_int (cdba_prep_handle stmt, int col);

/*! \brief get floating point value of column from result of executed prepared statement
 * \param  stmt                  prepared statement handle
 * \param  col                   column number (first column is 0)
 * \return floating point column value
 * \sa     cdba_prep_execute()
 * \sa     cdba_prep_get_column_count()
 * \sa     cdba_prep_get_column_type()
 * \sa     cdba_prep_get_column_int()
 * \sa     cdba_prep_get_column_text()
 */
DLL_EXPORT_CDBALIB double cdba_prep_get_column_float (cdba_prep_handle stmt, int col);

/*! \brief get text value of column from result of executed prepared statement
 * \param  stmt                  prepared statement handle
 * \param  col                   column number (first column is 0)
 * \return text column value, the caller must call cdba_free() on the result when it is no longer needed
 * \sa     cdba_prep_execute()
 * \sa     cdba_prep_get_column_count()
 * \sa     cdba_prep_get_column_type()
 * \sa     cdba_prep_get_column_int()
 * \sa     cdba_prep_get_column_float()
 */
DLL_EXPORT_CDBALIB char* cdba_prep_get_column_text (cdba_prep_handle stmt, int col);



/*! \brief free memory allocated by DCBALIB
 * \param  data                  pointer to memory to be freed
 * \sa     cdba_library_get_version()
 * \sa     cdba_prep_get_column_name()
 * \sa     cdba_prep_get_column_text()
 */
DLL_EXPORT_CDBALIB void cdba_free (void* data);



/*! \brief version number constants
 * \sa     cdba_get_version()
 * \sa     cdba_get_version_string()
 * \name   CDBALIB_VERSION_*
 * \{
 */
/*! \brief major version number */
#define CDBALIB_VERSION_MAJOR 0
/*! \brief minor version number */
#define CDBALIB_VERSION_MINOR 2
/*! \brief micro version number */
#define CDBALIB_VERSION_MICRO 0
/*! @} */

/*! \brief packed version number */
#define CDBALIB_VERSION (CDBALIB_VERSION_MAJOR * 0x01000000 + CDBALIB_VERSION_MINOR * 0x00010000 + CDBALIB_VERSION_MICRO * 0x00000100)

/*! \cond PRIVATE */
#define CDBALIB_VERSION_STRINGIZE_(major, minor, micro) #major"."#minor"."#micro
#define CDBALIB_VERSION_STRINGIZE(major, minor, micro) CDBALIB_VERSION_STRINGIZE_(major, minor, micro)
/*! \endcond */

/*! \brief string with dotted version number \hideinitializer */
#define CDBALIB_VERSION_STRING CDBALIB_VERSION_STRINGIZE(CDBALIB_VERSION_MAJOR, CDBALIB_VERSION_MINOR, CDBALIB_VERSION_MICRO)

/*! \brief string with name of cdbalib library */
#define CDBALIB_NAME "cdbalib"

/*! \brief string with name and version of cdbalib library \hideinitializer */
#define CDBALIB_FULLNAME CDBALIB_NAME " " CDBALIB_VERSION_STRING

/*! \brief get cdbalib library version string
 * \param  pmajor        pointer to integer that will receive major version number
 * \param  pminor        pointer to integer that will receive minor version number
 * \param  pmicro        pointer to integer that will receive micro version number
 * \sa     cdba_get_version_string()
 */
DLL_EXPORT_CDBALIB void cdba_get_version (int* pmajor, int* pminor, int* pmicro);

/*! \brief get cdbalib library version string
 * \return version string
 * \sa     cdba_get_version()
 */
DLL_EXPORT_CDBALIB const char* cdba_get_version_string ();



#ifdef __cplusplus
}
#endif

#endif
