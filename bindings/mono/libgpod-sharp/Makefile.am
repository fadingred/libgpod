ASSEMBLY = libgpod-sharp
TARGET = library
LINK = $(GLIBSHARP_LIBS) $(GDKSHARP_LIBS)

SOURCES =  \
	Artwork.cs \
	AssemblyInfo.cs \
	Chapter.cs \
	ChapterData.cs \
	Device.cs \
	GPodBase.cs \
	GPodList.cs \
	IpodInfo.cs \
	ITDB.cs \
	PhotoAlbum.cs \
	PhotoDB.cs \
	Playlist.cs \
	Thumbnail.cs \
	Track.cs

dlname := $(shell grep "^dlname=" $(top_builddir)/src/libgpod.la | sed "s/^dlname=\(.*\)/\1/g")
libgpod-sharp.dll.config: $(top_srcdir)/bindings/mono/libgpod-sharp/libgpod-sharp.dll.config.in 
	sed "s/@DLNAME@/$(dlname)/g" $< >$@

all-local: libgpod-sharp.dll.config

EXTRAS = \
	libgpod-sharp.pc.in

if HAVE_MONO
include $(srcdir)/../Makefile.include
else
EXTRA_DIST = $(SOURCES)
endif
