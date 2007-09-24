lib_LTLIBRARIES = libgpod.la

libgpod_la_SOURCES = \
	db-artwork-debug.c  \
	db-artwork-debug.h  \
	db-artwork-parser.c \
	db-artwork-parser.h \
	db-artwork-writer.c \
	db-image-parser.c   \
	db-image-parser.h   \
	db-itunes-parser.h  \
	db-parse-context.c  \
	db-parse-context.h  \
	glib-compat.h \
	itdb.h           \
	itdb_artwork.c   \
	itdb_device.c  \
	itdb_device.h  \
	itdb_endianness.h \
	itdb_itunesdb.c  \
	itdb_photoalbum.c \
	itdb_playlist.c  \
	itdb_private.h   \
	itdb_sha1.c \
	itdb_sha1.h \
	itdb_sysinfo.c \
	itdb_track.c     \
	ithumb-writer.c \
	pixmaps.c \
	pixmaps.h \
	sha1.c


libgpod_la_headers = itdb.h
libgpod_la_noinst_headers = itdb_private.h hal-common.h sha1.h itdb-sha1.h
libgpod_la_LDFLAGS = -version-info $(LIBGPOD_SO_VERSION) -no-undefined

libgpodincludedir = $(includedir)/gpod-1.0/gpod
libgpodinclude_HEADERS = $(libgpod_la_headers)

INCLUDES=$(LIBGPOD_CFLAGS)
LIBS=$(LIBGPOD_LIBS)
