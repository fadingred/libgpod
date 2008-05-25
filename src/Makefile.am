lib_LTLIBRARIES = libgpod.la

libgpod_la_SOURCES = 		\
	db-artwork-debug.c  	\
	db-artwork-parser.c 	\
	db-artwork-writer.c 	\
	db-image-parser.c   	\
	db-parse-context.c  	\
	itdb_artwork.c   	\
	itdb_chapterdata.c   	\
	itdb_device.c  		\
	itdb_itunesdb.c 	\
	itdb_photoalbum.c 	\
	itdb_playlist.c  	\
	itdb_plist.c		\
	itdb_sha1.c 		\
	itdb_sysinfo_extended_parser.c \
	itdb_track.c     	\
	ithumb-writer.c 	\
	pixmaps.c 		\
	sha1.c

libgpod_la_LDFLAGS = -version-info $(LIBGPOD_SO_VERSION) -no-undefined

libgpodincludebase = $(includedir)/gpod-1.0
libgpodincludedir = $(libgpodincludebase)/gpod
libgpodinclude_HEADERS = itdb.h
noinst_HEADERS = 		\
	db-artwork-debug.h	\
	db-artwork-parser.h 	\
	db-image-parser.h   	\
	db-itunes-parser.h  	\
	db-parse-context.h  	\
	itdb_device.h  		\
	itdb_endianness.h 	\
	itdb_plist.h		\
	itdb_private.h   	\
	itdb_sha1.h 		\
	itdb_sysinfo_extended_parser.h \
	pixmaps.h		\
	sha1.h

INCLUDES=$(LIBGPOD_CFLAGS) $(LIBXML_CFLAGS)
LIBS=$(LIBGPOD_LIBS) $(LIBXML_LIBS)

uninstall-hook:
	-rmdir --ignore-fail-on-non-empty $(DESTDIR)$(libgpodincludedir)
	-rmdir --ignore-fail-on-non-empty $(DESTDIR)$(libgpodincludebase)
