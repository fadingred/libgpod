SUBDIRS = examples tests

EXTRA_DIST =                \
        README.in           \
        __init__.py         \
        gpod.i.in           \
        gpod_doc.i.in       \
        gtkdoc-to-swig.xsl  \
        gtkpod.py           \
        ipod.py

CLEANFILES =                \
        *.pyc               \
        *.pyo               \
        _gpod.so            \
        gpod.py             \
        gpod_doc.i          \
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
SWIG_INTERFACES = gpod.i gpod_doc.i
LIBGPOD_CFLAGS += -fno-strict-aliasing
INCLUDES = -I$(top_srcdir)/src

gpoddir = $(pyexecdir)/gpod
gpod_PYTHON =  __init__.py gtkpod.py ipod.py
nodist_gpod_PYTHON = gpod.py
gpod_LTLIBRARIES = _gpod.la
nodist__gpod_la_SOURCES = gpod_wrap.c
_gpod_la_CPPFLAGS = $(PYTHON_INCLUDES) $(LIBGPOD_CFLAGS) $(PYGOBJECT_CFLAGS) $(INCLUDES)
_gpod_la_LDFLAGS = -module -avoid-version -shared
_gpod_la_LIBADD = $(LIBGPOD_LIBS) $(PYGOBJECT_LIBS) $(top_builddir)/src/libgpod.la

gpod_doc.i: $(srcdir)/gpod_doc.i.in $(srcdir)/gtkdoc-to-swig.xsl
	cat $< > $@
	-if test -x "`which xsltproc 2>/dev/null`"; then \
	    for xml in $(top_srcdir)/docs/reference/xml/*.xml; do \
		xsltproc $(srcdir)/gtkdoc-to-swig.xsl $$xml; \
	    done >> $@; \
	fi

gpod_wrap.c: $(SWIG_INTERFACES) $(nodist_gpod_PYTHON)
	$(SWIG) -python $(INCLUDES) -o $@ $<

gpod.py: $(SWIG_INTERFACES)
	$(SWIG) -python -o $@ $<

test:
	$(MAKE) -C tests test

uninstall-hook:
	-rmdir --ignore-fail-on-non-empty $(DESTDIR)$(gpoddir)
endif
