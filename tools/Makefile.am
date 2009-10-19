bin_PROGRAMS=ipod-read-sysinfo-extended
ipod_read_sysinfo_extended_SOURCES = read-sysinfoextended.c
ipod_read_sysinfo_extended_CFLAGS=$(LIBGPOD_CFLAGS) -I$(top_srcdir)/src
ipod_read_sysinfo_extended_LDADD=$(LIBGPOD_LIBS) $(top_builddir)/src/libgpod.la
if HAVE_SGUTILS
ipod_read_sysinfo_extended_SOURCES+=ipod-scsi.c
ipod_read_sysinfo_extended_CFLAGS+=$(SGUTILS_CFLAGS)
ipod_read_sysinfo_extended_LDADD+=$(SGUTILS_LIBS)
endif
if HAVE_LIBIPHONE
ipod_read_sysinfo_extended_SOURCES+=ipod-lockdown.c
ipod_read_sysinfo_extended_CFLAGS+=$(LIBIPHONE_CFLAGS)
ipod_read_sysinfo_extended_LDADD+=$(LIBIPHONE_LIBS)
endif

if HAVE_HAL
haldir = $(HALCALLOUTSDIR)
hal_PROGRAMS = libgpod-callout
libgpod_callout_SOURCES =					\
	hal-callout.c 						\
	$(top_srcdir)/src/itdb_sysinfo_extended_parser.c 	\
	$(top_srcdir)/src/itdb_plist.c

libgpod_callout_CFLAGS =						\
	$(LIBGPOD_CFLAGS)					\
	$(HAL_CFLAGS)						\
	-I$(top_srcdir)/src

#libgpod_callout_LDADD=$(LIBGPOD_LIBS) $(HAL_LIBS) $(SGUTILS_LIBS) 
libgpod_callout_LDADD =						\
	$(top_builddir)/src/libgpod.la				\
	$(LIBGPOD_LIBS)						\
	$(HAL_LIBS)

fdidir = $(HALFDIDIR)/policy/20thirdparty
fdi_DATA = 20-libgpod-sysinfo-extended.fdi

EXTRA_DIST = $(fdi_DATA)

if HAVE_LIBIPHONE
libgpod_callout_SOURCES += ipod-lockdown.c
libgpod_callout_CFLAGS += $(LIBIPHONE_CFLAGS)
libgpod_callout_LDADD += $(LIBIPHONE_LIBS)
endif

if HAVE_SGUTILS
libgpod_callout_SOURCES += ipod-scsi.c
libgpod_callout_CFLAGS += $(SGUTILS_CFLAGS)
libgpod_callout_LDADD += $(SGUTILS_LIBS)
endif

endif

if HAVE_SGUTILS
noinst_PROGRAMS=ipod-time-sync
ipod_time_sync_SOURCES =	\
	ipod-time-sync.c	\
	ipod-scsi.c
ipod_time_sync_CFLAGS=$(SGUTILS_CFLAGS) $(LIBGPOD_CFLAGS) 
ipod_time_sync_LDADD=$(SGUTILS_LIBS) $(LIBGPOD_LIBS) 
endif

