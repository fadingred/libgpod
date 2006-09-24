AC_PREREQ(2.52)
AC_INIT(src)
AC_CONFIG_SRCDIR(configure.ac)
AC_CONFIG_MACRO_DIR(m4)

AM_CONFIG_HEADER(config.h)

# Making releases:
#   LIBGPOD_MICRO_VERSION += 1;
#   LIBGPOD_INTERFACE_AGE += 1;
# if any functions have been added, set LIBGPOD_INTERFACE_AGE to 0.
# if backwards compatibility has been broken,
# set LIBGPOD_BINARY_AGE and LIBGPOD_INTERFACE_AGE to 0.
#
LIBGPOD_MAJOR_VERSION=0
LIBGPOD_MINOR_VERSION=4
LIBGPOD_MICRO_VERSION=0
LIBGPOD_INTERFACE_AGE=0
# If you need a modifier for the version number. 
# Normally empty, but can be used to make "fixup" releases.
LIBGPOD_EXTRAVERSION=

dnl libtool versioning from libgnome

LIBGPOD_CURRENT=`expr 100 '*' $LIBGPOD_MINOR_VERSION + $LIBGPOD_MICRO_VERSION - $LIBGPOD_INTERFACE_AGE`
LIBGPOD_BINARY_AGE=`expr 100 '*' $LIBGPOD_MINOR_VERSION + $LIBGPOD_MICRO_VERSION`
LIBGPOD_REVISION=$LIBGPOD_INTERFACE_AGE
LIBGPOD_AGE=`expr $LIBGPOD_BINARY_AGE - $LIBGPOD_INTERFACE_AGE`
LIBGPOD_VERSION=$LIBGPOD_MAJOR_VERSION.$LIBGPOD_MINOR_VERSION.$LIBGPOD_MICRO_VERSION$LIBGPOD_EXTRAVERSION

AC_SUBST(LIBGPOD_CURRENT)
AC_SUBST(LIBGPOD_REVISION)
AC_SUBST(LIBGPOD_AGE)
AC_SUBST(LIBGPOD_VERSION)

AM_INIT_AUTOMAKE(libgpod, $LIBGPOD_VERSION)

dnl make sure we keep ACLOCAL_FLAGS around for maintainer builds to work
AC_SUBST(ACLOCAL_AMFLAGS, "$ACLOCAL_FLAGS")

AM_MAINTAINER_MODE

AC_GNU_SOURCE
AC_PROG_CC
AC_PROG_LD
AC_STDC_HEADERS
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_INTLTOOL([0.21])
AC_CHECK_FUNCS(mremap)

PKG_CHECK_MODULES(LIBGPOD, glib-2.0 >= 2.4.0 gobject-2.0)
LIBGPOD_CFLAGS="$LIBGPOD_CFLAGS -Wall"

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
dnl * GDKPIXBUF is optional
dnl **************************************************

AC_ARG_ENABLE(gdk-pixbuf, [AC_HELP_STRING([--disable-gdk-pixbuf],[ArtworkDB will be disabled without gdk-pixbuf])], 
[case "${enableval}" in
  no) have_gdkpixbuf=no ;;
  *) have_gdkpixbuf=yes;;
esac], have_gdkpixbuf=yes)
AH_TEMPLATE([HAVE_GDKPIXBUF], [Whether gdk-pixbuf is installed, ArtworkDB writing support will be disabled if it can't be found])
if test x$have_gdkpixbuf = xyes; then
   PKG_CHECK_MODULES(GDKPIXBUF, gdk-pixbuf-2.0, have_gdkpixbuf=yes, have_gdkpixbuf=no)
   if test x"$have_gdkpixbuf" = xyes; then
      AC_DEFINE_UNQUOTED(HAVE_GDKPIXBUF, 1)
   fi
   LIBGPOD_CFLAGS="$LIBGPOD_CFLAGS $GDKPIXBUF_CFLAGS"
   LIBGPOD_LIBS="$LIBGPOD_LIBS $GDKPIXBUF_LIBS"
fi
AM_CONDITIONAL(HAVE_GDKPIXBUF, test x"$have_gdkpixbuf" = xyes)

dnl **************************************************
dnl * internationalization support
dnl **************************************************
ALL_LINGUAS="de es fr he it ja sv"

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
PYTHON_EYED3_MIN_VERSION=0.6.6
SWIG_MIN_VERSION=1.3.24
LIBGPOD_CHECK_PYTHON($PYTHON_MIN_VERSION)


dnl warnings bits, copied from gnome-keyring configure.in
dnl Turn on the additional warnings last, so -Werror doesn't affect other tests.

AC_ARG_ENABLE(more-warnings,
[  --enable-more-warnings  Maximum compiler warnings],
set_more_warnings="$enableval",[
if test -d "$srcdir/{arch}" || test -d "$srcdir/CVS"; then
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
	-Werror -std=c89 \
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

AC_OUTPUT([
Makefile
bindings/Makefile
bindings/python/Makefile
bindings/python/examples/Makefile
docs/Makefile
docs/reference/Makefile
docs/reference/version.xml
m4/Makefile
po/Makefile.in
src/Makefile
tests/Makefile
libgpod-1.0.pc
])

echo "
Configuration for $PACKAGE $VERSION :
--------------------------------

 Host System Type .....: $host
 Install path .........: $prefix
 Preprocessor .........: $CC $CPPFLAGS
 Compiler .............: $CC $CFLAGS $PACKAGE_CFLAGS
 Linker ...............: $CC $LDFLAGS $LIBS $PACKAGE_LIBS
 ArtworkDB support ....: $have_gdkpixbuf
 Python bindings ......: $with_python

 Now type 'make' to build $PACKAGE $VERSION,
 and then 'make install' for installation.
"

