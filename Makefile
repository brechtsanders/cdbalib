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
INCS = -Iinclude
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
MYSQLCONFIG = $(shell which mysql_config)

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

SQLITE3_CFLAGS = $(shell $(PKGCONFIG) --cflags sqlite3)
SQLITE3_LIBS = $(shell $(PKGCONFIG) --libs sqlite3)
MYSQL_CFLAGS = $(shell $(MYSQLCONFIG) --cflags)
MYSQL_LIBS = $(shell $(MYSQLCONFIG) --libs)
ifeq ($(OS),Windows_NT)
ODBC_CFLAGS = 
ODBC_LIBS = -lodbc32
else
ODBC_CFLAGS = 
ODBC_LIBS = 
endif

COMMON_PACKAGE_FILES = README.md LICENSE Changelog.txt
SOURCE_PACKAGE_FILES = $(COMMON_PACKAGE_FILES) Makefile doc/Doxyfile include/*.h src/*.c build/*.workspace build/*.cbp build/*.depend



OBJDIR =
BINDIR = 

default: all

all: static-libs shared-libs

static-libs: $(BINDIR)libcdba-sqlite3$(LIBEXT) $(BINDIR)libcdba-mysql$(LIBEXT) $(BINDIR)libcdba-odbc$(LIBEXT)

shared-libs: $(BINDIR)libcdba-sqlite3$(SOEXT) $(BINDIR)libcdba-mysql$(SOEXT) $(BINDIR)libcdba-odbc$(SOEXT)


$(OBJDIR)libcdba-sqlite3-static.o: src/cdbalib.c
	$(CC) -c -o $@ $< $(STATIC_CFLAGS) $(SQLITE3_CFLAGS) $(CFLAGS)

$(BINDIR)libcdba-sqlite3$(LIBEXT): $(OBJDIR)libcdba-sqlite3-static.o
	$(AR) cr $@ $^

$(OBJDIR)libcdba-sqlite3-shared.o: src/cdbalib.c
	$(CC) -c -o $@ $< $(SHARED_CFLAGS) $(SQLITE3_CFLAGS) $(CFLAGS)

$(BINDIR)libcdba-sqlite3$(SOEXT): $(OBJDIR)libcdba-sqlite3-shared.o
	$(CC) -o $@ $(OS_LINK_FLAGS) $^ $(SHARED_LDFLAGS) $(LDFLAGS) $(SQLITE3_LIBS) $(LIBS)


$(OBJDIR)libcdba-mysql-static.o: src/cdbalib.c
	$(CC) -c -o $@ $< $(STATIC_CFLAGS) $(MYSQL_CFLAGS) $(CFLAGS)

$(BINDIR)libcdba-mysql$(LIBEXT): $(OBJDIR)libcdba-mysql-static.o
	$(AR) cr $@ $^

$(OBJDIR)libcdba-mysql-shared.o: src/cdbalib.c
	$(CC) -c -o $@ $< $(SHARED_CFLAGS) $(MYSQL_CFLAGS) $(CFLAGS)

$(BINDIR)libcdba-mysql$(SOEXT): $(OBJDIR)libcdba-mysql-shared.o
	$(CC) -o $@ $(OS_LINK_FLAGS) $^ $(SHARED_LDFLAGS) $(LDFLAGS) $(MYSQL_LIBS) $(LIBS)


$(OBJDIR)libcdba-odbc-static.o: src/cdbalib.c
	$(CC) -c -o $@ $< $(STATIC_CFLAGS) $(ODBC_CFLAGS) $(CFLAGS)

$(BINDIR)libcdba-odbc$(LIBEXT): $(OBJDIR)libcdba-odbc-static.o
	$(AR) cr $@ $^

$(OBJDIR)libcdba-odbc-shared.o: src/cdbalib.c
	$(CC) -c -o $@ $< $(SHARED_CFLAGS) $(ODBC_CFLAGS) $(CFLAGS)

$(BINDIR)libcdba-odbc$(SOEXT): $(OBJDIR)libcdba-odbc-shared.o
	$(CC) -o $@ $(OS_LINK_FLAGS) $^ $(SHARED_LDFLAGS) $(LDFLAGS) $(ODBC_LIBS) $(LIBS)


.PHONY: doc
doc:
ifdef DOXYGEN
	$(DOXYGEN) doc/Doxyfile
endif

install: all doc
	$(MKDIR) $(PREFIX)/include $(PREFIX)/lib $(PREFIX)/bin
	$(CP) include/*.h $(PREFIX)/include/
	$(CP) $(BINDIR)*$(LIBEXT) $(PREFIX)/lib/
	#$(CP) $(UTILS_BIN) $(PREFIX)/bin/
ifeq ($(OS),Windows_NT)
	$(CP) $(BINDIR)*$(SOEXT) $(PREFIX)/bin/
	$(CP) $(BINDIR)*.def $(PREFIX)/lib/
else
	$(CP) $(BINDIR)*$(SOEXT) $(PREFIX)/lib/
endif
ifdef DOXYGEN
	$(CPDIR) doc/generated/man $(PREFIX)/
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
	$(RM) $(OBJDIR)*.o $(BINDIR)*$(LIBEXT) $(BINDIR)*$(SOEXT) $(BINDIR)*.def $(UTILS_BIN) version cdbalib-*.tar.xz doc/doxygen_sqlite3.db
ifeq ($(OS),Windows_NT)
	$(RM) *.def
endif
	$(RMDIR) doc/generated

