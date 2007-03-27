EXTRA_DIST = resources

dist-hook:
	rm -rf `find $(distdir)/resources -name CVS`

if HAVE_PYTHON
test:
	LD_LIBRARY_PATH=$(top_builddir)/src/.libs \
	PYTHONPATH=$(top_builddir)/bindings/python:$(top_builddir)/bindings/python/.libs  \
	$(PYTHON) tests.py
endif
