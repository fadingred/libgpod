AC_DEFUN([SHAMROCK_EXPAND_LIBDIR],
[
        expanded_libdir=`(
                case $prefix in
                        NONE) prefix=$ac_default_prefix ;;
                        *) ;;
                esac
                case $exec_prefix in
                        NONE) exec_prefix=$prefix ;;
                        *) ;;
                esac
                eval echo $libdir
        )`
        AC_SUBST(expanded_libdir)
])

AC_DEFUN([SHAMROCK_FIND_PROGRAM],
[
	AC_PATH_PROG($1, $2, $3)
	AC_SUBST($1)
])

AC_DEFUN([SHAMROCK_FIND_PROGRAM_OR_BAIL],
[
	SHAMROCK_FIND_PROGRAM($1, $2, no)
	if test "x$$1" = "xno"; then
		AC_MSG_ERROR([You need to install '$2'])
	fi
])

AC_DEFUN([SHAMROCK_FIND_MONO_2_0_COMPILER],
[
	SHAMROCK_FIND_PROGRAM_OR_BAIL(MCS, gmcs)
])

AC_DEFUN([SHAMROCK_FIND_MONO_RUNTIME],
[
	SHAMROCK_FIND_PROGRAM_OR_BAIL(MONO, mono)
])

AC_DEFUN([SHAMROCK_CHECK_MONO_MODULE],
[
	PKG_CHECK_MODULES(MONO_MODULE, mono >= $1)
])

AC_DEFUN([CHECK_GLIB_GTK_SHARP],
[
        found_gtksharp="yes"
	PKG_CHECK_MODULES(GDKSHARP,
		gtk-sharp-2.0 >= $GTK_SHARP_MIN_VERSION, [], found_gtksharp="no")
	AC_SUBST(GDKSHARP_LIBS)

	PKG_CHECK_MODULES(GLIBSHARP,
		glib-sharp-2.0 >= $GTK_SHARP_MIN_VERSION, [], found_gtksharp="no")
	AC_SUBST(GLIBSHARP_LIBS)
])

dnl check for mono and required dependencies
AC_DEFUN([LIBGPOD_CHECK_MONO],
[
    AC_ARG_WITH(mono,
        AC_HELP_STRING([--with-mono],
            [build mono bindings [[default=yes]]]),
        [with_mono=$withval],[with_mono=yes])

    AC_MSG_CHECKING(whether to build mono bindings)
    if test "X$with_mono" != Xno; then
        with_mono=yes
    fi
    AC_MSG_RESULT($with_mono)

    if test "X$with_mono" = Xyes; then
        SHAMROCK_EXPAND_LIBDIR
        SHAMROCK_CHECK_MONO_MODULE($MONO_MIN_VERSION)
        SHAMROCK_FIND_MONO_2_0_COMPILER
        SHAMROCK_FIND_MONO_RUNTIME
        CHECK_GLIB_GTK_SHARP
        if test "X$found_gtksharp" != "Xyes"; then
                with_mono="no"
        fi
    fi
    AM_CONDITIONAL(HAVE_MONO, test x$with_mono = xyes)
])
