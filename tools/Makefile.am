bin_PROGRAMS=ipod-read-sysinfo-extended
ipod_read_sysinfo_extended_SOURCES = read-sysinfoextended.c
ipod_read_sysinfo_extended_CFLAGS=$(LIBGPOD_CFLAGS) -I$(top_srcdir)/src
ipod_read_sysinfo_extended_LDADD=$(LIBGPOD_LIBS) $(top_builddir)/src/libgpod.la
if HAVE_SGUTILS
ipod_read_sysinfo_extended_SOURCES+=ipod-scsi.c
ipod_read_sysinfo_extended_CFLAGS+=$(SGUTILS_CFLAGS)
ipod_read_sysinfo_extended_LDADD+=$(SGUTILS_LIBS)
endif
if HAVE_LIBIMOBILEDEVICE
ipod_read_sysinfo_extended_SOURCES+=ipod-lockdown.c
ipod_read_sysinfo_extended_CFLAGS+=$(LIBIMOBILEDEVICE_CFLAGS)
ipod_read_sysinfo_extended_LDADD+=$(LIBIMOBILEDEVICE_LIBS)
endif
if HAVE_LIBUSB
ipod_read_sysinfo_extended_SOURCES+=ipod-usb.c
ipod_read_sysinfo_extended_CFLAGS+=$(LIBUSB_CFLAGS)
ipod_read_sysinfo_extended_LDADD+=$(LIBUSB_LIBS)
endif

COMMON_SOURCES = 						\
	generic-callout.c					\
	$(top_srcdir)/src/itdb_sysinfo_extended_parser.c 	\
	$(top_srcdir)/src/itdb_plist.c

COMMON_CFLAGS = $(LIBGPOD_CFLAGS) -I$(top_srcdir)/src
COMMON_LIBS = $(top_builddir)/src/libgpod.la $(LIBGPOD_LIBS)

if HAVE_SGUTILS
COMMON_SOURCES += ipod-scsi.c
COMMON_CFLAGS  += $(SGUTILS_CFLAGS)
COMMON_LIBS    += $(SGUTILS_LIBS)
endif

if HAVE_LIBUSB
COMMON_SOURCES +=ipod-usb.c
COMMON_CFLAGS  +=$(LIBUSB_CFLAGS)
COMMON_LIBS    +=$(LIBUSB_LIBS)
endif

noinst_HEADERS = backends.h

if HAVE_HAL
haldir = $(HALCALLOUTSDIR)
hal_PROGRAMS = libgpod-callout
libgpod_callout_SOURCES = $(COMMON_SOURCES) hal-backend.c
libgpod_callout_CFLAGS  = $(HAL_CFLAGS) $(COMMON_CFLAGS)
libgpod_callout_LDADD   = $(HAL_LIBS) $(COMMON_LIBS)

fdidir = $(HALFDIDIR)/policy/20thirdparty
fdi_DATA = 20-libgpod-sysinfo-extended.fdi

EXTRA_DIST = $(fdi_DATA)
endif # HAVE_HAL

if USE_UDEV
libudevdir=@udevdir@
libudev_PROGRAMS=ipod-set-info
ipod_set_info_SOURCES = $(COMMON_SOURCES) udev-backend.c
ipod_set_info_CFLAGS  = $(COMMON_CFLAGS)
ipod_set_info_LDADD   = $(COMMON_LIBS)

rulesdir=${libudevdir}/rules.d
rules_DATA= 90-libgpod.rules
endif # USE_UDEV

if HAVE_LIBIMOBILEDEVICE
if HAVE_HAL
hal_PROGRAMS+=iphone-callout
iphone_callout_SOURCES = iphone-callout.c ipod-lockdown.c
iphone_callout_CFLAGS = $(COMMON_CFLAGS) $(HAL_LIBS) $(LIBIMOBILEDEVICE_CFLAGS)
iphone_callout_LDADD = $(COMMON_LIBS) $(HAL_CFLAGS) $(LIBIMOBILEDEVICE_LIBS)
endif

if USE_UDEV
libudev_PROGRAMS+=iphone-set-info
iphone_set_info_SOURCES = iphone-callout.c ipod-lockdown.c
iphone_set_info_CFLAGS = $(COMMON_CFLAGS) $(LIBIMOBILEDEVICE_CFLAGS)
iphone_set_info_LDADD = $(COMMON_LIBS) $(LIBIMOBILEDEVICE_LIBS)
iphone_set_info_CPPFLAGS = -DUSE_UDEV
endif
endif # HAVE_LIBIPHONE

if HAVE_SGUTILS
noinst_PROGRAMS=ipod-time-sync
ipod_time_sync_SOURCES =	\
	ipod-time-sync.c	\
	ipod-scsi.c
ipod_time_sync_CFLAGS=$(SGUTILS_CFLAGS) $(LIBGPOD_CFLAGS) 
ipod_time_sync_LDADD=$(SGUTILS_LIBS) $(LIBGPOD_LIBS) 
endif

if USE_UDEV
install-data-hook:
	$(MKDIR_P) "$(DESTDIR)$(TMPMOUNTDIR)"
endif
if HAVE_HAL
install-data-hook:
	$(MKDIR_P) "$(DESTDIR)$(TMPMOUNTDIR)"
endif

