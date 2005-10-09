if HAVE_GDKPIXBUF
TESTTHUMBS=test-thumbnails test-write-thumbnails
else
TESTTHUMBS=
endif

noinst_PROGRAMS=test-ipod-device test-itdb $(TESTTHUMBS)

INCLUDES=$(LIBGPOD_CFLAGS) -I$(top_srcdir)/src -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"
LIBS=$(LIBGPOD_LIBS) $(top_builddir)/src/libgpod.la

test_itdb_SOURCES = itdb_main.c
test_itdb_LDADD = 

if HAVE_GDKPIXBUF
test_thumbnails_SOURCES = test-covers.c
test_thumbnails_CFLAGS = $(GDKPIXBUF_CFLAGS) $(AM_CFLAGS)
test_thumbnails_LDADD = $(GDKPIXBUF_LIBS)

test_write_thumbnails_SOURCES = test-write-covers.c
test_write_thumbnails_CFLAGS = $(GDKPIXBUF_CFLAGS) $(AM_CFLAGS)
test_write_thumbnails_LDADD = $(GDKPIXBUF_LIBS)
endif
