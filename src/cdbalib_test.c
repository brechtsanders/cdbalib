#include "cdbalib.h"
#include <stdlib.h>
#include <stdio.h>

void print_row (cdba_prep_handle stmt, int cols)
{
  while (cdba_prep_fetch_row(stmt) > 0) {
    printf("row:\t");
    for (int i = 0; i < cols; i++) {
      if (i > 0)
        printf("\t");
      switch (cdba_prep_get_column_type(stmt, i)) {
        case CDBA_TYPE_INT :
          {
            db_int val = cdba_prep_get_column_int(stmt, i);
            printf("%li", (long)val);
          }
          break;
        case CDBA_TYPE_FLOAT :
          {
            double val = cdba_prep_get_column_float(stmt, i);
            printf("%f", val);
          }
          break;
        case CDBA_TYPE_TEXT :
          {
            char* val = cdba_prep_get_column_text(stmt, i);
            if (!val) {
              printf("NULL");
            } else {
              printf("%s", val);
              cdba_free(val);
            }
          }
          break;
        case CDBA_TYPE_NULL :
          printf("NULL");
        default :
          break;
      }
    }
    printf("\n");
  }
}

int main (int argc, char *argv[], char *envp[])
{
  cdba_library_handle dblib;
  cdba_handle db;
  cdba_prep_handle stmt;

  //show version
  printf("cdbalib version %s\n", cdba_get_version_string());

  //open database library
  if ((dblib = cdba_library_initialize()) == NULL) {
    fprintf(stderr, "Error initializing database\n");
  }
  printf("Database driver name: %s\n", cdba_library_get_name(dblib));
  printf("Database driver version: %s\n", cdba_library_get_version(dblib));

  if ((db = cdba_open(dblib, "cdbalist_test.sq3")) == NULL) {
    fprintf(stderr, "Error opening database\n");
    return 1;
  }

  //cdba_begin_transaction(db);

  if (cdba_sql(db, "SELECT 1") != 0) {
    fprintf(stderr, "Error executing query: %s\n", cdba_get_error(db));
  }

  if (cdba_sql(db, "CREATE TABLE test1 ("
                   "  intval INTEGER,"
                   "  fltval FLOAT,"
                   "  txtval TEXT,"
                   "  PRIMARY KEY (intval)"
                   ");"
  ) != 0) {
    fprintf(stderr, "Error executing query: %s\n", cdba_get_error(db));
  }

  if (cdba_sql(db, "INSERT INTO test1 (intval,fltval,txtval) VALUES (1,1.001,'Test 1')") != 0) {
    fprintf(stderr, "Error executing query: %s\n", cdba_get_error(db));
  }
  if (cdba_multiple_sql(db, "INSERT INTO test1 (intval,fltval,txtval) VALUES (2,2.0002,'Test 2');"
                            "INSERT INTO test1 (intval,fltval,txtval) VALUES (3,3.00003,'Test 3');"
                            "INSERT INTO test1 (intval,fltval,txtval) VALUES (4,4.000004,'Test 4');"
  ) != 0) {
    fprintf(stderr, "Error executing query: %s\n", cdba_get_error(db));
  }

/*
  if (cdba_sql(db, "SELECT 1; SELECT 2") != 0) {
    fprintf(stderr, "Error executing query\n");
  }
*/

  if (cdba_multiple_sql(db, "SELECT 1; SELECT 2; ") != 0) {
    fprintf(stderr, "Error executing query: %s\n", cdba_get_error(db));
  }

  //if ((stmt = cdba_create_preparedstatement(db, "SELECT * FROM lastvalue")) == NULL) {
  //if ((stmt = cdba_create_preparedstatement(db, "SELECT 'Test' AS tst, * FROM lastvalue WHERE device=? AND epoch > ?")) == NULL) {
  //if ((stmt = cdba_create_preparedstatement(db, "SELECT * FROM lastvalue WHERE device=? AND FLOOR(obisid/10)=?")) == NULL) {
  //if ((stmt = cdba_create_preparedstatement(db, "SELECT * FROM test1 WHERE intval-2*FLOOR(intval/2)=? ORDER BY intval")) == NULL) {
  //if ((stmt = cdba_create_preparedstatement(db, "SELECT 123 AS intval, 3.141592 AS fltval, 'Test' AS txtval")) == NULL) {
  if ((stmt = cdba_create_preparedstatement(db, "SELECT 123 AS intval, 3.141592 AS fltval, 'Test' AS txtval, NULL AS nulval")) == NULL) {
    fprintf(stderr, "Error preparing SQL statement: %s\n", cdba_get_error(db));
  } else if (cdba_prep_execute(stmt, CDBA_TYPE_INT, (db_int)1) != 0) {
    fprintf(stderr, "Error executing SQL statement: %s\n", cdba_get_error(db));
    cdba_prep_close(stmt);
  } else {

    char* s;
    int n = cdba_prep_get_column_count(stmt);
    printf("# columns: %i\n", n);

    for (int i = 0; i < n; i++) {
      s = cdba_prep_get_column_name(stmt, i);
      printf("type[%i]: %i\tname[%i]: %s\n", i, (int)cdba_prep_get_column_type(stmt, i), i, (s ? s : "(no name)"));
      cdba_free(s);
    }

    printf("[Odd rows]\n");
    print_row(stmt, n);

    cdba_prep_reset(stmt);



    printf("[Even rows]\n");
    if (cdba_prep_execute(stmt, CDBA_TYPE_INT, (db_int)0) != 0) {
      fprintf(stderr, "Error executing SQL statement: %s\n", cdba_get_error(db));
      cdba_prep_close(stmt);
    } else {
      int n = cdba_prep_get_column_count(stmt);
      while (cdba_prep_fetch_row(stmt) > 0) {
        print_row(stmt, n);
      }
    }



  }


  if (cdba_sql(db, "DROP TABLE test1") != 0) {
    fprintf(stderr, "Error executing query: %s\n", cdba_get_error(db));
  }

  cdba_prep_close(stmt);

  cdba_commit_transaction(db);

  //close database
  cdba_close(db);

  //close database library
  cdba_library_cleanup(dblib);

  return 0;
}
