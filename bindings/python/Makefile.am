SUBDIRS = examples

EXTRA_DIST =		\
	gpod.i 		\
	ipod.py 	\
	__init__.py	\
	gtkpod.py

CLEANFILES =		\
	*.pyc		\
	*.pyo		\
	_gpod.so	\
	gpod.py		\
	gpod_wrap.c	\
	gpod_wrap.o

MAINTAINERCLEANFILES =	\
	README

if HAVE_PYTHON
MODULE_CFLAGS	= `$(PKG_CONFIG) --cflags $(top_srcdir)/libgpod-1.0.pc` $(PYTHON_INCLUDES)
MODULE_LIBS	= `$(PKG_CONFIG) --libs $(top_srcdir)/libgpod-1.0.pc` -L$(top_srcdir)/src/.libs
python_DATA	= gpod.py _gpod.so

gpod_wrap.c: gpod.i
	$(SWIG) -python gpod.i

gpod.py: gpod.i
	$(SWIG) -python gpod.i

gpod_wrap.o: gpod_wrap.c
	$(CC) $(MODULE_CFLAGS) -c -fpic gpod_wrap.c

_gpod.so: gpod_wrap.o
	$(CC) $(MODULE_LIBS) @PYTHON_LDFLAGS@ gpod_wrap.o -o $@

install-pythonDATA: $(python_DATA)
	$(mkinstalldirs) $(DESTDIR)$(pythondir)/gpod
	$(INSTALL_PROGRAM) _gpod.so  $(DESTDIR)$(pythondir)/gpod/_gpod.so
	$(INSTALL_DATA) *.py         $(DESTDIR)$(pythondir)/gpod/
	$(PYTHON) -c 'from compileall import compile_dir; compile_dir("$(DESTDIR)$(pythondir)/gpod/", ddir="$(pythondir)/gpod/")'
	$(PYTHON) -O -c 'from compileall import compile_dir; compile_dir("$(DESTDIR)$(pythondir)/gpod/", ddir="$(pythondir)/gpod/")'

uninstall-pythonDATA: $(python_DATA)
	rm -rf $(DESTDIR)$(pythondir)/gpod

README: README.in gpod.i
	WRAPPER_LIST=`grep 'PyObject\* sw_[^_].*;' gpod.i | awk -F "[ (]" '{ print $$2 }' | tr '\n' ' ' | sort -u`; \
	sed \
	    -e "s/@WRAPPER_LIST@/$${WRAPPER_LIST}/" \
	    -e "/^sw_/ s/ $$//" \
	    -e "/^sw_/ s/ /\n/g" \
	    $< > $@

test:
	cd tests && python tests.py
endif
