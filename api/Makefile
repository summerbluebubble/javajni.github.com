#
# zlib.h -- interface of the 'zlib' general purpose compression library
# version 1.2.2, October 3rd, 2004
#
# Copyright (C) 1995-2004 Jean-loup Gailly and Mark Adler
#
# This software is provided 'as-is', without any express or implied
# warranty.  In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.
#
# Jean-loup Gailly jloup@gzip.org
# Mark Adler madler@alumni.caltech.edu
#

CC=gcc
#LIBRARY=n3linuxzlib.a
#LIBRARY=libn3linuxz.so
LIBRARY=libgaiqiz.so
LIBRARY_N32=n3linuxzlib_n32.a
SRC_ROOT=../..

EXTRA_CFLAGS += -g
EXTRA_CFLAGS += -fPIC
EXTRA_CFLAGS += -I ./
EXTRA_CFLAGS += -I../include/ 
EXTRA_CFLAGS += -I$(SRC_ROOT)/core/src
EXTRA_CFLAGS += -I../driver/kzlib
EXTRA_CFLAGS += -DZLIB_ZSTREAM_COMPATIBILITY

all: $(LIBRARY) $(LIBRARY_N32)

$(LIBRARY): zlib-api.c  n3-zip-api.c
	$(CC)  $(EXTRA_CFLAGS)  -c -o zlib-api.o zlib-api.c
	$(CC)  $(EXTRA_CFLAGS) -c -o n3-zip-api.o n3-zip-api.c
	$(CC)  $(EXTRA_CFLAGS)  -c -o NativeCompressCall.o NativeCompressCall.c
	$(CC) -shared -o $(LIBRARY) zlib-api.o n3-zip-api.o NativeCompressCall.o

$(LIBRARY_N32): zlib-api.c 
	$(CC)  $(EXTRA_CFLAGS) -static -c -o $(LIBRARY_N32) zlib-api.c

clean:
	rm -rf $(LIBRARY) $(LIBRARY_N32) *.o
	
