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

EXTRAS = \
	libgpod-sharp.pc.in

if HAVE_MONO
include $(srcdir)/../Makefile.include
else
EXTRA_DIST = $(SOURCES)
endif
