SUBDIRS = examples

EXTRA_DIST =		\
	gpod.i 		\
	ipod.py 	\
	__init__.py	\
	gtkpod.py

CLEANFILES =		\
	*.py*		\
	_gpod.so	\
	gpod.py		\
	gpod_wrap.c	\
	gpod_wrap.o

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
	$(INSTALL_DATA) __init__.py  $(DESTDIR)$(pythondir)/gpod/__init__.py
	$(INSTALL_DATA) gpod.py      $(DESTDIR)$(pythondir)/gpod/gpod.py
	$(INSTALL_DATA) gtkpod.py    $(DESTDIR)$(pythondir)/gpod/gtkpod.py
	$(INSTALL_DATA) ipod.py      $(DESTDIR)$(pythondir)/gpod/ipod.py
	$(PYTHON) -c 'from py_compile import compile; compile("$(DESTDIR)$(pythondir)/gpod/gpod.py")'
	$(PYTHON) -O -c 'from py_compile import compile; compile("$(DESTDIR)$(pythondir)/gpod/gpod.py")'
	$(PYTHON) -c 'from py_compile import compile; compile("$(DESTDIR)$(pythondir)/gpod/gtkpod.py")'
	$(PYTHON) -O -c 'from py_compile import compile; compile("$(DESTDIR)$(pythondir)/gpod/gtkpod.py")'
	$(PYTHON) -c 'from py_compile import compile; compile("$(DESTDIR)$(pythondir)/gpod/ipod.py")'
	$(PYTHON) -O -c 'from py_compile import compile; compile("$(DESTDIR)$(pythondir)/gpod/ipod.py")'

uninstall-pythonDATA: $(python_DATA)
	rm -rf $(DESTDIR)$(pythondir)/gpod
endif
