ifeq ($(OS),)
OS = $(shell uname -s)
endif
PREFIX = /usr/local
CC   = gcc
CPP  = g++
AR   = ar
LIBPREFIX = lib
LIBEXT = .a
ifeq ($(OS),Windows_NT)
BINEXT = .exe
SOLIBPREFIX =
SOEXT = .dll
else ifeq ($(OS),Darwin)
BINEXT =
SOLIBPREFIX = lib
SOEXT = .dylib
else
BINEXT =
SOLIBPREFIX = lib
SOEXT = .so
endif
INCS = -Iinclude -Isrc
CFLAGS = $(INCS) -Os
CPPFLAGS = $(INCS) -Os
STATIC_CFLAGS = -DSTATIC_CDBALIB
SHARED_CFLAGS = -DBUILD_CDBALIB_DLL
LIBS =
LDFLAGS =
ifeq ($(OS),Darwin)
STRIPFLAG =
else
STRIPFLAG = -s
endif
MKDIR = mkdir -p
RM = rm -f
RMDIR = rm -rf
CP = cp -f
CPDIR = cp -rf
DOXYGEN = $(shell which doxygen)
PKGCONFIG = $(shell which pkg-config)
MYSQLCONFIG = $(shell which mariadb_config || which mysql_config)

ifneq ($(OS),Windows_NT)
SHARED_CFLAGS += -fPIC
endif
ifeq ($(OS),Windows_NT)
SHARED_LDFLAGS += -Wl,--out-implib,$@$(LIBEXT) -Wl,--output-def,$(@:%$(SOEXT)=%.def)
endif
ifeq ($(OS),Darwin)
OS_LINK_FLAGS = -dynamiclib -o $@
else
OS_LINK_FLAGS = -shared -Wl,-soname,$@ $(STRIPFLAG)
endif

OSALIAS := $(OS)
ifeq ($(OS),Windows_NT)
ifneq (,$(findstring x86_64,$(shell gcc --version)))
OSALIAS := win64
else
OSALIAS := win32
endif
endif

SQLITE3_CFLAGS = -DDB_SQLITE3 $(shell $(PKGCONFIG) --cflags sqlite3)
SQLITE3_LIBS = $(shell $(PKGCONFIG) --libs sqlite3)
MYSQL_CFLAGS = -DDB_MYSQL $(shell $(MYSQLCONFIG) --cflags)
MYSQL_LIBS = $(shell $(MYSQLCONFIG) --libs)
ifeq ($(OS),Windows_NT)
ODBC_CFLAGS = -DDB_ODBC 
ODBC_LIBS = -lodbc32
else
ODBC_CFLAGS = -DDB_ODBC
ODBC_LIBS = 
endif

COMMON_PACKAGE_FILES = README.md LICENSE Changelog.txt
SOURCE_PACKAGE_FILES = $(COMMON_PACKAGE_FILES) Makefile doc/Doxyfile include/*.h src/*.c src/*.h build/*.workspace build/*.cbp build/*.depend



OBJDIR =
BINDIR = 

default: all

all: static-libs shared-libs pkg-config-files

static-libs: $(BINDIR)libcdba-sqlite3$(LIBEXT) $(BINDIR)libcdba-mysql$(LIBEXT) $(BINDIR)libcdba-odbc$(LIBEXT)

shared-libs: $(BINDIR)libcdba-sqlite3$(SOEXT) $(BINDIR)libcdba-mysql$(SOEXT) $(BINDIR)libcdba-odbc$(SOEXT)


$(OBJDIR)cdbaconfig-static.o: src/cdbaconfig.c
	$(CC) -c -o $@ $< $(STATIC_CFLAGS) $(CFLAGS)

$(OBJDIR)cdbaconfig-shared.o: src/cdbaconfig.c
	$(CC) -c -o $@ $< $(SHARED_CFLAGS) $(CFLAGS)

$(OBJDIR)libcdba-sqlite3-static.o: src/cdbalib.c
	$(CC) -c -o $@ $< $(STATIC_CFLAGS) $(CFLAGS) $(SQLITE3_CFLAGS)

$(BINDIR)libcdba-sqlite3$(LIBEXT): $(OBJDIR)libcdba-sqlite3-static.o $(OBJDIR)cdbaconfig-static.o
	$(AR) cr $@ $^

$(OBJDIR)libcdba-sqlite3-shared.o: src/cdbalib.c
	$(CC) -c -o $@ $< $(SHARED_CFLAGS) $(CFLAGS) $(SQLITE3_CFLAGS)

$(BINDIR)libcdba-sqlite3$(SOEXT): $(OBJDIR)libcdba-sqlite3-shared.o $(OBJDIR)cdbaconfig-shared.o
	$(CC) -o $@ $(OS_LINK_FLAGS) $^ $(SHARED_LDFLAGS) $(LDFLAGS) $(SQLITE3_LIBS) $(LIBS)


$(OBJDIR)libcdba-mysql-static.o: src/cdbalib.c
	$(CC) -c -o $@ $< $(STATIC_CFLAGS) $(CFLAGS) $(MYSQL_CFLAGS)

$(BINDIR)libcdba-mysql$(LIBEXT): $(OBJDIR)libcdba-mysql-static.o $(OBJDIR)cdbaconfig-static.o
	$(AR) cr $@ $^

$(OBJDIR)libcdba-mysql-shared.o: src/cdbalib.c
	$(CC) -c -o $@ $< $(SHARED_CFLAGS) $(CFLAGS) $(MYSQL_CFLAGS)

$(BINDIR)libcdba-mysql$(SOEXT): $(OBJDIR)libcdba-mysql-shared.o $(OBJDIR)cdbaconfig-shared.o
	$(CC) -o $@ $(OS_LINK_FLAGS) $^ $(SHARED_LDFLAGS) $(LDFLAGS) $(MYSQL_LIBS) $(LIBS)


$(OBJDIR)libcdba-odbc-static.o: src/cdbalib.c
	$(CC) -c -o $@ $< $(STATIC_CFLAGS) $(CFLAGS) $(ODBC_CFLAGS)

$(BINDIR)libcdba-odbc$(LIBEXT): $(OBJDIR)libcdba-odbc-static.o $(OBJDIR)cdbaconfig-static.o
	$(AR) cr $@ $^

$(OBJDIR)libcdba-odbc-shared.o: src/cdbalib.c
	$(CC) -c -o $@ $< $(SHARED_CFLAGS) $(CFLAGS) $(ODBC_CFLAGS)

$(BINDIR)libcdba-odbc$(SOEXT): $(OBJDIR)libcdba-odbc-shared.o $(OBJDIR)cdbaconfig-shared.o
	$(CC) -o $@ $(OS_LINK_FLAGS) $^ $(SHARED_LDFLAGS) $(LDFLAGS) $(ODBC_LIBS) $(LIBS)


.PHONY: pkg-config-files
pkg-config-files: $(OBJDIR)cdbalib-sqlite3.pc $(OBJDIR)cdbalib-mysql.pc $(OBJDIR)cdbalib-odbc.pc


define CDBALIB_SQLITE3_PC
prefix=$(PREFIX)
exec_prefix=$${prefix}
includedir=$${prefix}/include
libdir=$${exec_prefix}/lib

Name: CDBALIB SQLite3
Description: CDBALIB - C database abstraction library with support for prepared statements - SQLite3 library
Version: $(shell cat version)
Cflags: -I$${includedir} $(SQLITE3_CFLAGS)
Libs: -L$${libdir} -lcdbalib-sqlite3 $(SQLITE3_LIBS)
endef

$(OBJDIR)cdbalib-sqlite3.pc: version
	$(file > $@,$(CDBALIB_SQLITE3_PC))


define CDBALIB_MYSQL_PC
prefix=$(PREFIX)
exec_prefix=$${prefix}
includedir=$${prefix}/include
libdir=$${exec_prefix}/lib

Name: CDBALIB MySQL
Description: CDBALIB - C database abstraction library with support for prepared statements - MySQL library
Version: $(shell cat version)
Cflags: -I$${includedir} $(MYSQL_CFLAGS)
Libs: -L$${libdir} -lcdbalib-mysql $(MYSQL_LIBS)
endef


$(OBJDIR)cdbalib-mysql.pc: version
	$(file > $@,$(CDBALIB_MYSQL_PC))

define CDBALIB_ODBC_PC
prefix=$(PREFIX)
exec_prefix=$${prefix}
includedir=$${prefix}/include
libdir=$${exec_prefix}/lib

Name: CDBALIB ODBC
Description: CDBALIB - C database abstraction library with support for prepared statements - ODBC library
Version: $(shell cat version)
Cflags: -I$${includedir} $(ODBC_CFLAGS)
Libs: -L$${libdir} -lcdbalib-odbc $(ODBC_LIBS)
endef

$(OBJDIR)cdbalib-odbc.pc: version
	$(file > $@,$(CDBALIB_ODBC_PC))


.PHONY: doc
doc:
ifdef DOXYGEN
	$(DOXYGEN) doc/Doxyfile
endif

install: all doc
	$(MKDIR) $(PREFIX)/include $(PREFIX)/lib/pkgconfig $(PREFIX)/bin
	$(CP) include/*.h $(PREFIX)/include/
	$(CP) $(BINDIR)*$(LIBEXT) $(PREFIX)/lib/
	$(CP) $(OBJDIR)*.pc $(PREFIX)/lib/pkgconfig/
	#$(CP) $(UTILS_BIN) $(PREFIX)/bin/
ifeq ($(OS),Windows_NT)
	$(CP) $(BINDIR)*$(SOEXT) $(PREFIX)/bin/
	$(CP) $(BINDIR)*.def $(PREFIX)/lib/
else
	$(CP) $(BINDIR)*$(SOEXT) $(PREFIX)/lib/
endif
ifdef DOXYGEN
	$(CPDIR) doc/generated/man/* $(PREFIX)/man/
endif


.PHONY: version
version:
	sed -ne "s/^#define\s*CDBALIB_VERSION_[A-Z]*\s*\([0-9]*\)\s*$$/\1./p" include/cdbalib.h | tr -d "\n" | sed -e "s/\.$$//" > version

.PHONY: package
package: version
	tar cfJ cdbalib-$(shell cat version).tar.xz --transform="s?^?cdbalib-$(shell cat version)/?" $(SOURCE_PACKAGE_FILES)

.PHONY: package
binarypackage: version
	$(MKDIR) build_$(OSALIAS)/obj build_$(OSALIAS)/bin
ifneq ($(OS),Windows_NT)
	$(MAKE) PREFIX=binarypackage_temp_$(OSALIAS) OBJDIR=build_$(OSALIAS)/obj/ BINDIR=build_$(OSALIAS)/bin/ install
	tar cfJ cdbalib-$(shell cat version)-$(OSALIAS).tar.xz --transform="s?^binarypackage_temp_$(OSALIAS)/??" $(COMMON_PACKAGE_FILES) binarypackage_temp_$(OSALIAS)/*
else
	$(MAKE) PREFIX=binarypackage_temp_$(OSALIAS) OBJDIR=build_$(OSALIAS)/obj/ BINDIR=build_$(OSALIAS)/bin/ install DOXYGEN=
	cp -f $(COMMON_PACKAGE_FILES) binarypackage_temp_$(OSALIAS)
	rm -f cdbalib-$(shell cat version)-$(OSALIAS).zip
	cd binarypackage_temp_$(OSALIAS) && zip -r9 ../cdbalib-$(shell cat version)-$(OSALIAS).zip $(COMMON_PACKAGE_FILES) * && cd ..
endif
	$(RMDIR) build_$(OSALIAS) binarypackage_temp_$(OSALIAS)

.PHONY: clean
clean:
	$(RM) $(OBJDIR)*.o $(OBJDIR)*.pc $(BINDIR)*$(LIBEXT) $(BINDIR)*$(SOEXT) $(BINDIR)*.def $(UTILS_BIN) version cdbalib-*.tar.xz doc/doxygen_sqlite3.db
ifeq ($(OS),Windows_NT)
	$(RM) *.def
endif
	$(RMDIR) doc/generated

