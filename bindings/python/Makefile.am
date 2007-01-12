SUBDIRS = examples

EXTRA_DIST =                \
        README.in           \
        __init__.py         \
        gpod.i.in           \
        gtkpod.py           \
        ipod.py

CLEANFILES =                \
        *.pyc               \
        *.pyo               \
        _gpod.so            \
        gpod.py             \
        gpod_wrap.*

DISTCLEANFILES =            \
        README              \
        gpod.i

README: README.in gpod.i
	WRAPPER_LIST=`grep 'PyObject\* sw_[^_].*;' gpod.i | awk -F "[ (]" '{ print $$2 }' | sort -u | tr '\n' ' '`; \
	sed \
	    -e "s/@WRAPPER_LIST@/$${WRAPPER_LIST}/" \
	    -e "/^sw_/ s/ $$//" \
	    -e "/^sw_/ s/ /\n/g" \
	    $< > $@

if HAVE_PYTHON
BUILT_SOURCES = gpod_wrap.c
SWIG_SOURCES = gpod.i
LIBGPOD_CFLAGS += -fno-strict-aliasing
INCLUDES = -I$(top_srcdir)/src

gpoddir = $(pyexecdir)/gpod
gpod_PYTHON =  __init__.py gpod.py gtkpod.py ipod.py
gpod_LTLIBRARIES = _gpod.la
_gpod_la_SOURCES = gpod_wrap.c $(SWIG_SOURCES)
_gpod_la_CPPFLAGS = $(PYTHON_INCLUDES) $(LIBGPOD_CFLAGS) $(INCLUDES)
_gpod_la_LDFLAGS = -module -avoid-version
_gpod_la_LIBADD = $(LIBGPOD_LIBS) $(top_builddir)/src/libgpod.la

gpod_wrap.c: $(SWIG_SOURCES)
	$(SWIG) -python $(INCLUDES) -o $@ $<

gpod.py: $(SWIG_SOURCES)
	$(SWIG) -python -o $@ $<

test:
	cd tests && python tests.py
endif
