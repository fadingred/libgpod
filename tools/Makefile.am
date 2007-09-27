if HAVE_SGUTILS

bin_PROGRAMS=ipod-read-sysinfo-extended

ipod_read_sysinfo_extended_SOURCES = read-sysinfoextended-sgutils.c
ipod_read_sysinfo_extended_CFLAGS=$(LIBGPOD_CFLAGS) $(SGUTILS_CFLAGS) -I$(top_srcdir)/src
ipod_read_sysinfo_extended_LDADD=$(LIBGPOD_LIBS) $(SGUTILS_LIBS) $(top_builddir)/src/libgpod.la

endif