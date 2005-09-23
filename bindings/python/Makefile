all: _gpod.so

gpod_wrap.c: gpod.i
	swig -python gpod.i

gpod.py: gpod.i
	swig -python gpod.i

gpod_wrap.o: gpod_wrap.c
	gcc -c -fpic gpod_wrap.c -I /usr/include/python2.3/ `pkg-config glib-2.0 --cflags`

_gpod.so: gpod_wrap.o
	gcc -shared gpod_wrap.o -o _gpod.so -L../../src/.libs -lgpod `pkg-config glib-2.0 --libs`


clean:
	rm gpod.py  gpod.pyc _gpod.so gpod_wrap.c gpod_wrap.o

test: _gpod.so play.py
	LD_LIBRARY_PATH=../../src/.libs ./play.py 
