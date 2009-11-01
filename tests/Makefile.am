if HAVE_GDKPIXBUF
TESTTHUMBS=test-thumbnails test-write-thumbnails test-photos get-timezone 

test_thumbnails_SOURCES = test-covers.c
test_thumbnails_CFLAGS = $(AM_CFLAGS)

test_write_thumbnails_SOURCES = test-write-covers.c
test_write_thumbnails_CFLAGS = $(AM_CFLAGS)

test_photos_SOURCES = test-photos.c
test_photos_CFLAGS = $(AM_CFLAGS)
else
TESTTHUMBS=
endif

TESTMISC=test-init-ipod

if HAVE_TAGLIB
TESTCP=test-cp
test_cp_SOURCES = test-cp.cc
test_cp_LDADD = $(TAGLIB_LIBS)
test_cp_CXXFLAGS = $(TAGLIB_CFLAGS)
TESTTAGLIB=test-rebuild-db
test_rebuild_db_SOURCES = test-rebuild-db.cc
test_rebuild_db_LDADD = $(TAGLIB_LIBS)
test_rebuild_db_CXXFLAGS = $(TAGLIB_CFLAGS)
else
TESTTAGLIB=
TESTCP=
endif

test_itdb_SOURCES = itdb_main.c
test_itdb_LDADD = 

test_ls_SOURCES = test-ls.c
test_ls_LDADD = 

test_init_ipod_SOURCES = test-init-ipod.c
test_init_ipod_LDADD = 

test_firewire_id_SOURCES = test-fw-id.c

test_sysinfo_extended_parsing_SOURCES = \
	test-sysinfo-extended-parsing.c \
	$(top_srcdir)/src/itdb_plist.c  \
	$(top_srcdir)/src/itdb_sysinfo_extended_parser.c

test_sysinfo_extended_parsing_LDADD =

get_timezone_SOURCES = get-timezone.c

noinst_PROGRAMS=test-itdb test-ls test-firewire-id \
		test-sysinfo-extended-parsing			 \
	        $(TESTTHUMBS) $(TESTTAGLIB) $(TESTCP) $(TESTMISC)

INCLUDES=$(LIBGPOD_CFLAGS) -I$(top_srcdir)/src -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"
LIBS=$(LIBGPOD_LIBS) $(top_builddir)/src/libgpod.la
