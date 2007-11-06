if HAVE_SGUTILS

bin_PROGRAMS=ipod-read-sysinfo-extended

ipod_read_sysinfo_extended_SOURCES =	\
	read-sysinfoextended-sgutils.c	\
	ipod-scsi-inquiry.c
ipod_read_sysinfo_extended_CFLAGS=$(LIBGPOD_CFLAGS) $(SGUTILS_CFLAGS) -I$(top_srcdir)/src
ipod_read_sysinfo_extended_LDADD=$(LIBGPOD_LIBS) $(SGUTILS_LIBS) $(top_builddir)/src/libgpod.la

if HAVE_HAL
haldir = $(HALCALLOUTSDIR)
hal_PROGRAMS = libgpod-callout

libgpod_callout_SOURCES =		\
	hal-callout.c 		\
	ipod-scsi-inquiry.c
libgpod_callout_CFLAGS=$(LIBGPOD_CFLAGS) $(SGUTILS_CFLAGS) -I$(top_srcdir)/src
libgpod_callout_LDADD=$(LIBGPOD_LIBS) $(SGUTILS_LIBS) $(top_builddir)/src/libgpod.la

fdidir = $(datadir)/hal/fdi/policy/20thirdparty
fdi_DATA = 20-libgpod-sysinfo-extended.fdi

EXTRA_DIST = $(fdi_DATA) 

endif

endif

