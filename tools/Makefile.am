if HAVE_SGUTILS

bin_PROGRAMS=ipod-read-sysinfo-extended

ipod_read_sysinfo_extended_SOURCES =	\
	read-sysinfoextended-sgutils.c	\
	ipod-scsi.c
ipod_read_sysinfo_extended_CFLAGS=$(LIBGPOD_CFLAGS) $(SGUTILS_CFLAGS) -I$(top_srcdir)/src
ipod_read_sysinfo_extended_LDADD=$(LIBGPOD_LIBS) $(SGUTILS_LIBS) $(top_builddir)/src/libgpod.la

if HAVE_HAL
haldir = $(HALCALLOUTSDIR)
hal_PROGRAMS = libgpod-callout
noinst_PROGRAMS=ipod-time-sync

libgpod_callout_SOURCES =		\
	hal-callout.c 		\
	ipod-scsi.c
libgpod_callout_CFLAGS=$(LIBGPOD_CFLAGS) $(HAL_CFLAGS) $(SGUTILS_CFLAGS) -I$(top_srcdir)/src
libgpod_callout_LDADD=$(top_builddir)/src/libgpod.la $(LIBGPOD_LIBS) $(HAL_LIBS) $(SGUTILS_LIBS) 

ipod_time_sync_SOURCES =	\
	ipod-time-sync.c	\
	ipod-scsi.c
ipod_time_sync_CFLAGS=$(SGUTILS_CFLAGS) $(LIBGPOD_CFLAGS) 
ipod_time_sync_LDADD=$(SGUTILS_LIBS) $(LIBGPOD_LIBS) 

fdidir = $(HALFDIDIR)/policy/20thirdparty
fdi_DATA = 20-libgpod-sysinfo-extended.fdi

EXTRA_DIST = $(fdi_DATA)

endif

endif

