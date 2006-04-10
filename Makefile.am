SUBDIRS=src bindings tests po m4 docs

# build documentation when doing a distcheck.
DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgpod-1.0.pc

EXTRA_DIST =                    \
        intltool-merge.in       \
        intltool-update.in      \
        intltool-extract.in     \
        libgpod-1.0.pc.in


DISTCLEANFILES =                \
        intltool-extract        \
        intltool-merge          \
        intltool-update

