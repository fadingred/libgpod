noinst_PROGRAMS=test-itdb

INCLUDES=$(LIBGPOD_CFLAGS) -I$(top_srcdir)/src -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"
LIBS=$(LIBGPOD_LIBS)

test_itdb_SOURCES = itdb_main.c
test_itdb_LDADD = $(top_builddir)/src/libgpod.la
