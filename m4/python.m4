dnl copied from pygtk
dnl
dnl a macro to check for ability to create python extensions
dnl  AM_CHECK_PYTHON_HEADERS([ACTION-IF-POSSIBLE], [ACTION-IF-NOT-POSSIBLE])
dnl function also defines PYTHON_INCLUDES
AC_DEFUN([AM_CHECK_PYTHON_HEADERS],
[AC_REQUIRE([AM_PATH_PYTHON])
AC_MSG_CHECKING(for  development headers)
dnl deduce PYTHON_INCLUDES
py_prefix=`$PYTHON -c "import sys; print sys.prefix"`
py_exec_prefix=`$PYTHON -c "import sys; print sys.exec_prefix"`
PYTHON_INCLUDES="-I${py_prefix}/include/python${PYTHON_VERSION}"
if test "$py_prefix" != "$py_exec_prefix"; then
  PYTHON_INCLUDES="$PYTHON_INCLUDES -I${py_exec_prefix}/include/python${PYTHON_VERSION}"
fi
AC_SUBST(PYTHON_INCLUDES)
dnl check if the headers exist:
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $PYTHON_INCLUDES"
AC_TRY_CPP([#include <Python.h>],dnl
[AC_MSG_RESULT(found)
$1],dnl
[AC_MSG_RESULT(not found)
$2])
CPPFLAGS="$save_CPPFLAGS"
])

dnl check for python
AC_DEFUN([LIBGPOD_CHECK_PYTHON],
[
    AC_ARG_WITH(python,
        AC_HELP_STRING([--with-python=PATH],
            [build python bindings [[default=no]]]),
        [with_python=$withval],[with_python=no])

    AC_MSG_CHECKING(whether to build python bindings)
    if test "X$with_python" != Xno; then
        if test "X$with_python" != Xyes; then
            PYTHON=$with_python
        fi
        with_python=yes
    fi
    AC_MSG_RESULT($with_python)

    if test "X$with_python" == Xyes; then
        if test -z "$PYTHON"; then
            AC_PATH_PROG(PYTHON, python)
        fi
    
        if test -n "$PYTHON"; then
            AM_PATH_PYTHON($PYTHON_MIN_VERSION)
            AM_CHECK_PYTHON_HEADERS(with_python="yes",with_python="no")
    
            if test "X$with_python" == Xyes; then
                dnl test for python ldflags
                dnl copied from the Redland RDF bindings, http://librdf.org/
                if test `uname` = Darwin; then
                    PYTHON_LDFLAGS="-Wl,-F. -Wl,-F. -bundle"
                    if $PYTHON -c 'import sys, string; sys.exit(string.find(sys.prefix,"Framework")+1)'; then
                        :
                    else
                        PYTHON_LDFLAGS="$PYTHON_LDFLAGS -framework Python"
                    fi
                else
                    PYTHON_LDFLAGS="-shared"
                fi
                AC_SUBST(PYTHON_LDFLAGS)

                dnl check for swig
                AC_PROG_SWIG($SWIG_MIN_VERSION)
                with_python="$has_swig"
            fi
        else
            AC_MSG_WARN(python not found.  try --with-python=/path/to/python)
            with_python="no"
        fi
    fi
    AM_CONDITIONAL(HAVE_PYTHON, test x$with_python = xyes)
])dnl
