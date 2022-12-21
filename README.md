# CDBALIB
Cross-platform C database abstraction library with support for prepared statements.

## Goal
The goal of CDBALIB is to have a cross-platform C library for connecting to different types SQL databases using a common programming interface.

An import feature is support for the use (and reuse) of prepared statements, which can speed up database access in cases where the same SQL statement is being performed multiple times with different values.

This can be done by having an SQL statement with values replaced by question marks (e.g. `UPDATE myvalues SET value = ? WHERE id = ?`), preparing this query and then execuring it multiple times if needed with different values bound to each question mark.

Besides speed this also helps preventing SQL injection attacks, which would otherwise require the programmer to make sure the values are properly quoted.

This library was written with portability across platforms in mind.

# Supported databases
The following databases are currently supported:
- [SQLite3](http://www.sqlite.org/)
- [MySQL](https://www.mysql.com/)/[MariaDB](https://mariadb.org/)

The following database support is under development:
- ODBC

## Platforms
CDBALIB can currently be built on at least the following platforms (others may work as well):
- Windows (using MinGW-w64 GCC compiler)
- Debian Linux

## License
CDBALIB is distributed under the LGPL license
