AC_PREREQ(2.52)
AC_INIT(src)
AC_CONFIG_SRCDIR(configure.ac)
AC_CONFIG_MACRO_DIR(m4)

AM_CONFIG_HEADER(config.h)


# Making releases:
#   LIBGPOD_MICRO_VERSION += 1;
#   LIBGPOD_SO_VERSION - see comments
#
LIBGPOD_MAJOR_VERSION=0
LIBGPOD_MINOR_VERSION=8
LIBGPOD_MICRO_VERSION=0
# If you need a modifier for the version number. 
# Normally empty, but can be used to make "fixup" releases.
LIBGPOD_EXTRAVERSION=

LIBGPOD_VERSION=$LIBGPOD_MAJOR_VERSION.$LIBGPOD_MINOR_VERSION.$LIBGPOD_MICRO_VERSION$LIBGPOD_EXTRAVERSION

dnl libtool versioning
# +1 : 0 : +1  == adds new functions to the interface
# +1 : 0 : 0   == changes or removes functions (changes include both
#                 changes to the signature and the semantic)
#  ? :+1 : ?   == just internal changes
# CURRENT : REVISION : AGE
LIBGPOD_SO_VERSION=7:1:3

AC_SUBST(LIBGPOD_SO_VERSION)
AC_SUBST(LIBGPOD_MAJOR_VERSION)
AC_SUBST(LIBGPOD_MINOR_VERSION)
AC_SUBST(LIBGPOD_MICRO_VERSION)
AC_SUBST(LIBGPOD_VERSION)

AM_INIT_AUTOMAKE(libgpod, $LIBGPOD_VERSION)

dnl make sure we keep ACLOCAL_FLAGS around for maintainer builds to work
AC_SUBST(ACLOCAL_AMFLAGS, "$ACLOCAL_FLAGS")

AM_MAINTAINER_MODE

AC_GNU_SOURCE
AC_PROG_CC
AM_PROG_CC_C_O

# AC_PROG_CXX is only needed for the test program tests/test-rebuild-db.cc.
# You can safely remove AC_PROG_CXX and the test program (edit tests/Makefile.am)
# if you should run into problems.
AC_PROG_CXX

AC_PROG_LD
AC_STDC_HEADERS
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_INTLTOOL([0.21])

AC_CHECK_FUNCS([localtime_r])
AC_CHECK_MEMBERS([struct tm.tm_gmtoff],,,[#include <time.h>])
dnl sqlite3 is needed for newer ipod models (nano5g), and libplist is needed 
dnl by libgpod sqlite code
PKG_CHECK_MODULES(LIBGPOD, glib-2.0 >= 2.8.0 gobject-2.0 sqlite3 libplist >= 1.0)

dnl **************************************************
dnl we've copied gchecksum from glib 2.16. Only use the
dnl copy if the version of glib on the system does not
dnl provide it.
dnl **************************************************

AC_ARG_WITH(internal-gchecksum,
            AC_HELP_STRING([--with-internal-gchecksum],
                           [Build using internal copy of gchecksum]),,
                           with_internal_gchecksum=no)

if test "x$with_internal_gchecksum" = "xno"; then
        PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.16.0, with_internal_gchecksum=no,
                          with_internal_gchecksum=yes)
fi

AM_CONDITIONAL(WITH_INTERNAL_GCHECKSUM, test "x$with_internal_gchecksum" = "xyes")

dnl **************************************************
dnl * sgutils is necessary to get the xml device file
dnl * from older ipods
dnl **************************************************
AC_CHECK_LIB(sgutils2, sg_ll_inquiry,
             [SGUTILS_LIBS="-lsgutils2"; have_sgutils=yes],
             have_sgutils=no)
if test "x$have_sgutils" != xyes; then
    AC_CHECK_LIB(sgutils, sg_ll_inquiry,
                 [SGUTILS_LIBS="-lsgutils"; have_sgutils=yes],
                 have_sgutils=no)
fi
AC_SUBST(SGUTILS_LIBS)
if test x"$have_sgutils" = xyes; then
        AC_DEFINE(HAVE_SGUTILS, 1, [Define if you have sgutils support])
fi
AM_CONDITIONAL(HAVE_SGUTILS, test x"$have_sgutils" = xyes)

dnl **************************************************
dnl * libusb is necessary to get the xml device file
dnl * from newer ipods (nano5g)
dnl **************************************************
PKG_CHECK_MODULES(LIBUSB, libusb-1.0, have_libusb=yes, have_libusb=no)
if test x"$have_libusb" = xyes; then
   AH_TEMPLATE([HAVE_LIBUSB], [Whether libusb is installed])
   AC_DEFINE_UNQUOTED(HAVE_LIBUSB, 1)
fi
AM_CONDITIONAL(HAVE_LIBUSB, test x"$have_libusb" = xyes)

dnl **************************************************
dnl * zlib is neeeded for handling compressed iTunesCDB files
dnl **************************************************
AC_CHECK_LIB(z, inflate,
             [Z_LIBS="-lz"; have_zlib=yes],
             have_zlib=no)
if test "x$have_zlib" != xyes; then
    AC_CHECK_LIB(zlib, inflate,
                 [Z_LIBS="-lzlib"; have_zlib=yes],
                 have_zlib=no)
fi
if test x"$have_zlib" = xyes; then
   AH_TEMPLATE([HAVE_ZLIB], [Whether zlib is installed, it's used for compressed iTunesCDB])
   AC_DEFINE_UNQUOTED(HAVE_ZLIB, 1)
else
   AC_MSG_ERROR([couldn't find zlib, please install zlib development headers])
fi
AC_SUBST(Z_LIBS)
AM_CONDITIONAL(HAVE_ZLIB, test x"$have_zlib" = xyes)

dnl ***********************************************************************
dnl * HAL is optional, but is required if you want things to "just work"
dnl * when a recent iPod is plugged in
dnl * libgpod by itself doesn't use HAL at all, HAL is just used to build a 
dnl * helper program which is then installed as a HAL callout
dnl ***********************************************************************
AC_ARG_WITH(hal, AC_HELP_STRING([--without-hal],
        [Disable HAL support]))

hal_callouts_dir="`$PKG_CONFIG --variable libdir hal`/hal/scripts"
AC_ARG_WITH(hal_callouts_dir, AC_HELP_STRING([--with-hal-callouts-dir=DIR],
	[Directory where HAL expects its callout scripts to be located]),,
	with_hal_callouts_dir="$hal_callouts_dir")
HALCALLOUTSDIR="$with_hal_callouts_dir"
AC_SUBST(HALCALLOUTSDIR)

hal_fdi_dir="`$PKG_CONFIG --variable prefix hal`/share/hal/fdi"
AC_ARG_WITH(hal_fdi_dir, AC_HELP_STRING([--with-hal-fdi-dir=DIR],
	[Directory where HAL expects its fdi files to be located]),,
	with_hal_fdi_dir="$hal_fdi_dir")
HALFDIDIR="$with_hal_fdi_dir"
AC_SUBST(HALFDIDIR)

if test "x$with_hal" != "xno"; then
  PKG_CHECK_MODULES(HAL, hal >= 0.5 hal < 0.6, enable_hal=yes, enable_hal=no)
  if test "x$enable_hal" != "xyes" -a "x$with_hal" = "xyes"; then
      AC_MSG_ERROR([HAL support explicitly requested but HAL couldn't be found])
  fi

  if test "x$enable_hal" = "xyes"; then
        AC_DEFINE(HAVE_HAL, 1, [Define if you have HAL support])
        AC_SUBST(HAL_CFLAGS)
	AC_SUBST(HAL_LIBS)
  fi
fi
AM_CONDITIONAL(HAVE_HAL, test x"$enable_hal" = xyes)

dnl ***********************************************************************
dnl * libimobiledevice is optional, but is required for iPhone/iPod Touch support
dnl ***********************************************************************
AC_ARG_WITH(libimobiledevice, AC_HELP_STRING([--without-libimobiledevice],
        [Disable iPhone/iPod Touch support]))

if test "x$with_libimobiledevice" != "xno"; then
  PKG_CHECK_MODULES(LIBIMOBILEDEVICE, libimobiledevice-1.0 >= 0.9.7, enable_libimobiledevice=yes, enable_libimobiledevice=no)
  if test "x$enable_libimobiledevice" != "xyes" -a "x$with_libimobiledevice" = "xyes"; then
      AC_MSG_ERROR([iPhone/iPod Touch support explicitly requested but libimobiledevice couldn't be found])
  fi

  if test "x$enable_libimobiledevice" = "xyes"; then
      AC_DEFINE(HAVE_LIBIMOBILEDEVICE, 1, [Define if you have libimobiledevice support])
      AC_SUBST(LIBIMOBILEDEVICE_CFLAGS)
      AC_SUBST(LIBIMOBILEDEVICE_LIBS)
  fi
fi
AM_CONDITIONAL(HAVE_LIBIMOBILEDEVICE, test x"$enable_libimobiledevice" = xyes)

dnl ***********************************************************************
dnl * udev can be used to run a callout as an alternative to the hal callout
dnl * nowadays (2009), it's the recommended way of having callouts, so
dnl * it should be preferred over HAL
dnl ***********************************************************************
AC_ARG_ENABLE(udev,
             [AC_HELP_STRING([--enable-udev],[Enable udev callout])],
             [if test $enableval = "yes" ; then
                 enable_udev=yes
              else
                enable_udev=no
              fi],
              [enable_udev=no])
AM_CONDITIONAL(USE_UDEV, test x"$enable_udev" = xyes)
AC_SUBST([udevdir], [/lib/udev])

dnl ***********************************************************************
dnl * for the udev and hal callouts, we need a temporary directory to
dnl * mount the iPod to write out its extended info file.
dnl ***********************************************************************

temp_mount_dir="/tmp/"
AC_ARG_WITH(temp_mount_dir, AC_HELP_STRING([--with-temp-mount-dir=DIR],
	[Directory where HAL/udev will create a sub-directory to mount iPods]),,
	with_temp_mount_dir="$temp_mount_dir")
TMPMOUNTDIR="$with_temp_mount_dir"
AC_SUBST(TMPMOUNTDIR)
AH_TEMPLATE([TMPMOUNTDIR], [Directory where HAL/udev will create a sub-directory to mount iPods])
AC_DEFINE_UNQUOTED(TMPMOUNTDIR, "$with_temp_mount_dir", [Directory where HAL/udev will create a sub-directory to mount iPods])

dnl **************************************************
dnl * TagLib is only used by test-rebuild-db 
dnl **************************************************
PKG_CHECK_MODULES(TAGLIB, taglib, have_taglib=yes, have_taglib=no)
if test x"$have_taglib" = xyes; then
   AH_TEMPLATE([HAVE_TAGLIB], [Whether TagLib is installed, it's only used in a test program])
   AC_DEFINE_UNQUOTED(HAVE_TAGLIB, 1)
fi
AC_SUBST(TAGLIB_CFLAGS)
AC_SUBST(TAGLIB_LIBS)
AM_CONDITIONAL(HAVE_TAGLIB, test x"$have_taglib" = xyes)

dnl **************************************************
dnl * libxml is used to parse the plist files (aka SysInfoExtended)
dnl **************************************************
AC_ARG_ENABLE(libxml, [AC_HELP_STRING([--disable-libxml],[SysInfoExtended won't be parsed])], 
[case "${enableval}" in
  no) enable_libxml=no ;;
  *) enable_libxml=yes;;
esac])
AH_TEMPLATE([HAVE_LIBXML], [Whether libxml is installed, it's used to parse SysInfoExtended])
if test x$enable_libxml != xno; then
   PKG_CHECK_MODULES(LIBXML, libxml-2.0)
   AC_DEFINE_UNQUOTED(HAVE_LIBXML, 1)
   LIBGPOD_CFLAGS="$LIBGPOD_CFLAGS $LIBXML_CFLAGS"
   LIBGPOD_LIBS="$LIBGPOD_LIBS $LIBXML_LIBS"
fi

dnl **************************************************
dnl * GDKPIXBUF is optional
dnl **************************************************
AC_ARG_ENABLE(gdk-pixbuf, [AC_HELP_STRING([--disable-gdk-pixbuf],[ArtworkDB will be disabled without gdk-pixbuf])], 
[case "${enableval}" in
  no) enable_gdkpixbuf=no ;;
  *) enable_gdkpixbuf=yes;;
esac])
AH_TEMPLATE([HAVE_GDKPIXBUF], [Whether gdk-pixbuf is installed, ArtworkDB writing support will be disabled if it can't be found])
if test x$enable_gdkpixbuf != xno; then
   GDKPIXBUF_REQ="gdk-pixbuf-2.0"
   PKG_CHECK_MODULES(GDKPIXBUF, $GDKPIXBUF_REQ >= 2.6.0, have_gdkpixbuf=yes, have_gdkpixbuf=no)
   if test x"$enable_gdkpixbuf" = xyes -a x"$have_gdkpixbuf" = xno; then
      AC_MSG_ERROR([gdk-pixbuf support explicitly requested but gdk-pixbuf couldn't be found])
   fi
fi
if test x"$have_gdkpixbuf" = xyes; then
   AC_DEFINE_UNQUOTED(HAVE_GDKPIXBUF, 1)
   LIBGPOD_CFLAGS="$LIBGPOD_CFLAGS $GDKPIXBUF_CFLAGS"
   LIBGPOD_LIBS="$LIBGPOD_LIBS $GDKPIXBUF_LIBS"
else
   have_gdkpixbuf=no
   GDKPIXBUF_REQ=""
fi
AM_CONDITIONAL(HAVE_GDKPIXBUF, test x"$have_gdkpixbuf" = xyes)
AC_SUBST(GDKPIXBUF_REQ)

dnl **************************************************
dnl * PYGOBJECT is optional
dnl **************************************************

AC_ARG_ENABLE(pygobject, [AC_HELP_STRING([--disable-pygobject],[Python API will lack GdkPixbuf support without PyGOBJECT])], 
[case "${enableval}" in
  no) enable_pygobject=no ;;
  *) enable_pygobject=yes;;
esac])
AH_TEMPLATE([HAVE_PYGOBJECT], [Whether pygobject is installed, Python API will lack GdkPixbuf support without PyGOBJECT])
if test x$enable_pygobject != xno; then
   PKG_CHECK_MODULES(PYGOBJECT, pygobject-2.0 >= 2.8.0, have_pygobject=yes, have_pygobject=no)
   if test x"$enable_pygobject" = xyes -a x"$have_pygobject" = xno; then
      AC_MSG_ERROR([pygobject support explicitly requested but pygobject couldn't be found])
   fi
fi
if test x"$have_pygobject" = xyes; then
   AC_DEFINE_UNQUOTED(HAVE_PYGOBJECT, 1)
else
   have_pygobject=no
fi
AM_CONDITIONAL(HAVE_PYGOBJECT, test x"$have_pygobject" = xyes)
AC_SUBST(PYGOBJECT_CFLAGS)
AC_SUBST(PYGOBJECT_LIBS)

dnl **************************************************
dnl * internationalization support
dnl **************************************************
ALL_LINGUAS="de es fr he it ja ro sv zh_CN"

GETTEXT_PACKAGE=libgpod
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])

AM_GLIB_GNU_GETTEXT


dnl **************************************************
dnl * gtk-doc
dnl **************************************************
GTK_DOC_CHECK(1.0)

dnl **************************************************
dnl * optional python bindings
dnl **************************************************

PYTHON_MIN_VERSION=2.1.1
PYTHON_MUTAGEN_MIN_VERSION=1.8
SWIG_MIN_VERSION=1.3.24
LIBGPOD_CHECK_PYTHON($PYTHON_MIN_VERSION)

dnl **************************************************
dnl * optional mono bindings
dnl **************************************************

MONO_MIN_VERSION=1.9.1
GTK_SHARP_MIN_VERSION=2.12
LIBGPOD_CHECK_MONO

dnl warnings bits, copied from gnome-keyring configure.in
dnl Turn on the additional warnings last, so -Werror doesn't affect other tests.

AC_ARG_ENABLE(more-warnings,
[  --enable-more-warnings  Maximum compiler warnings],
set_more_warnings="$enableval",[
if test -e "$srcdir/autogen.sh"; then
	set_more_warnings=yes
else
	set_more_warnings=no
fi
])
AC_MSG_CHECKING(for more warnings, including -Werror)
if test "$GCC" = "yes" -a "$set_more_warnings" != "no"; then
	AC_MSG_RESULT(yes)
	CFLAGS="\
	-Wall \
	-Wchar-subscripts -Wmissing-declarations -Wmissing-prototypes \
	-Wnested-externs -Wpointer-arith \
	-Wcast-align -Wsign-compare \
	-Werror \
	$CFLAGS"

	for option in -Wno-strict-aliasing -Wno-sign-compare -Wdeclaration-after-statement; do
		SAVE_CFLAGS="$CFLAGS"
		CFLAGS="$CFLAGS $option"
		AC_MSG_CHECKING([whether gcc understands $option])
		AC_TRY_COMPILE([], [],
			has_option=yes,
			has_option=no,)
		if test $has_option = no; then
			CFLAGS="$SAVE_CFLAGS"
		fi
		AC_MSG_RESULT($has_option)
		unset has_option
		unset SAVE_CFLAGS
	done
	unset option
else
	AC_MSG_RESULT(no)
fi

AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(LIBGPOD_CFLAGS)
AC_SUBST(LIBGPOD_LIBS)

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_CONFIG_FILES([
Makefile
bindings/Makefile
bindings/mono/Makefile
bindings/mono/libgpod-sharp/libgpod-sharp.pc
bindings/mono/libgpod-sharp/Makefile
bindings/mono/libgpod-sharp-test/Makefile
bindings/mono/libgpod-sharp-test/libgpod-sharp-test
bindings/python/gpod.i
bindings/python/Makefile
bindings/python/examples/Makefile
bindings/python/tests/Makefile
docs/Makefile
docs/reference/Makefile
docs/reference/version.xml
m4/Makefile
po/Makefile.in
src/Makefile
tools/Makefile
tests/Makefile
libgpod-1.0.pc
tools/90-libgpod.rules
])
AC_OUTPUT

echo "
Configuration for $PACKAGE $VERSION :
--------------------------------

 Host System Type .........: $host
 Install path .............: $prefix
 Preprocessor .............: $CC $CPPFLAGS
 Compiler .................: $CC $CFLAGS $LIBGPOD_CFLAGS
 Linker ...................: $CC $LDFLAGS $LIBS $LIBGPOD_LIBS
 Artwork support ..........: $have_gdkpixbuf
 Mono bindings ............: $with_mono
 Python bindings ..........: $with_python
 PyGObject support ........: $have_pygobject
 iPhone/iPod Touch support.: $enable_libimobiledevice
 Temporary mount directory.: $with_temp_mount_dir

 Now type 'make' to build $PACKAGE $VERSION,
 and then 'make install' for installation.
"

if test x"$enable_libxml" = xno; then
        echo "
**WARNING** libxml support is disabled, libgpod won't be able to 
parse SysInfoExtended. This means it can't take advantage of the 
extensive iPod description (image formats, features, ...) 
available from that file
"
fi

if test x"$have_gdkpixbuf" = xno; then
        echo "
**WARNING** gdkpixbuf support is disabled. libgpod won't be able
to read or write artwork (covers, photos, ...) from/to the iPod
"
fi

if test x"$enable_udev" = xyes -a x"$enable_hal" = xyes; then
	echo "
**WARNING** HAL and udev callouts have been enabled at the same time.
While this should be harmless, this isn't recommended. The udev
callout is the recommended one.
"
fi
