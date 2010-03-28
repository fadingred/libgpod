ASSEMBLY = libgpod-sharp-test
TARGET = exe
LINK = -r:../libgpod-sharp/libgpod-sharp.dll

SOURCES =  \
	AssemblyInfo.cs \
	Main.cs

EXTRAS = \
	libgpod-sharp-test.in

if HAVE_MONO
include $(srcdir)/../Makefile.include
else
EXTRA_DIST = $(SOURCES)
endif
