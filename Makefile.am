SUBDIRS=src tests po

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgpod-1.0.pc

# the bindings/ below are just a hack until a better way is found
EXTRA_DIST =                    \
        intltool-merge.in       \
        intltool-update.in      \
        intltool-extract.in     \
        libgpod-1.0.pc.in	\
	bindings/python/examples/play.py \
	bindings/python/examples/tag-genre-from-audioscrobber.py \
        bindings/python/Makefile \
        bindings/python/gpod.i


DISTCLEANFILES =                \
        intltool-extract        \
        intltool-merge          \
        intltool-update

