lib_LTLIBRARIES = libgpod.la

libgpod_la_SOURCES = \
	itdb.h           \
	itdb_artwork.c   \
	itdb_itunesdb.c  \
	itdb_playlist.c  \
	itdb_private.h   \
	itdb_photoalbum.c \
	itdb_track.c     \
	db-artwork-parser.c \
	db-artwork-parser.h \
	db-parse-context.c  \
	db-parse-context.h  \
	db-artwork-debug.c  \
	db-artwork-debug.h  \
	db-image-parser.c   \
	db-image-parser.h   \
	db-itunes-parser.h  \
	db-artwork-writer.c \
	ithumb-writer.c \
	itdb_device.c \
	itdb_device.h \
	itdb_endianness.h \
	glib-compat.h


libgpod_la_headers = itdb.h
libgpod_la_noinst_headers = itdb_private.h hal-common.h
libgpod_la_LDFLAGS = -version-info $(LIBGPOD_CURRENT):$(LIBGPOD_REVISION):$(LIBGPOD_AGE) \
        -no-undefined

libgpodincludedir = $(includedir)/gpod-1.0/gpod
libgpodinclude_HEADERS = $(libgpod_la_headers)

INCLUDES=$(LIBGPOD_CFLAGS)
LIBS=$(LIBGPOD_LIBS)
