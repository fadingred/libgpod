lib_LTLIBRARIES = libgpod.la

libgpod_la_SOURCES = \
	itdb.h           \
	itdb_itunesdb.c  \
	itdb_playlist.c  \
	itdb_private.h   \
	itdb_track.c 
libgpod_la_headers = itdb.h
libgpod_la_noinst_headers = itdb_private.h
libgpod_la_LDFLAGS = -version-info $(LIBGPOD_CURRENT):$(LIBGPOD_REVISION):$(LIBGPOD_AGE) \
        -no-undefined

libgpodincludedir = $(includedir)/gpod-1.0/gpod
libgpodinclude_HEADERS = $(libgpod_la_headers)

INCLUDES=$(LIBGPOD_CFLAGS)
LIBS=$(LIBGPOD_LIBS)
