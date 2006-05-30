if HAVE_GDKPIXBUF
TESTTHUMBS=test-thumbnails test-write-thumbnails test-photos
else
TESTTHUMBS=
endif

TESTMISC=test-init-ipod

if HAVE_TAGLIB
TESTTAGLIB=test-rebuild-db
else
TESTTAGLIB=
endif

noinst_PROGRAMS=test-itdb test-ls $(TESTTHUMBS) $(TESTTAGLIB) $(TESTMISC)

INCLUDES=$(LIBGPOD_CFLAGS) -I$(top_srcdir)/src -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"
LIBS=$(LIBGPOD_LIBS) $(top_builddir)/src/libgpod.la

test_itdb_SOURCES = itdb_main.c
test_itdb_LDADD = 

test_ls_SOURCES = test-ls.c
test_ls_LDADD = 

if HAVE_TAGLIB
test_rebuild_db_SOURCES = test-rebuild-db.cc
test_rebuild_db_LDADD = $(TAGLIB_LIBS)
test_rebuild_db_CXXFLAGS = $(TAGLIB_CFLAGS)
endif

if HAVE_GDKPIXBUF
test_thumbnails_SOURCES = test-covers.c
test_thumbnails_CFLAGS = $(AM_CFLAGS)

test_photos_SOURCES = test-photos.c
test_photos_CFLAGS = $(AM_CFLAGS)

test_init_ipod_SOURCES = test-init-ipod.c

test_write_thumbnails_SOURCES = test-write-covers.c
test_write_thumbnails_CFLAGS = $(AM_CFLAGS)
endif
