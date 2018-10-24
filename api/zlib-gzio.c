/*
 * zlib.h -- interface of the 'zlib' general purpose compression library
 * version 1.2.2, October 3rd, 2004
 *
 * Copyright (C) 1995-2004 Jean-loup Gailly and Mark Adler
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Jean-loup Gailly jloup@gzip.org
 * Mark Adler madler@alumni.caltech.edu
 */

#include "zlib.h"
#include <errno.h>
#include <string.h>


typedef struct {
	z_stream stream;
	int32_t z_err;
	int32_t z_eof;			
	FILE *file;				/* .gz file */
	uint8_t *inbuf;
	uint8_t *outbuf;
	int8_t mode;
	int32_t in;
	int32_t out;
	uint8_t *msg;
	int32_t back;
	int32_t last;
	int32_t transparent;
	uint32_t crc;
}gz_stream_t;

static int destroy(gz_stream_t *s)
{
	int err;
	if (s->stream.state != Z_NULL) {
		if (s->mode == 'w') {
			err = deflateEnd(&(s->stream));
		} else if (s->mode == 'r') {
		   err = inflateEnd(&(s->stream));
		}
	}
	if (s->file != NULL)
		fclose(s->file);

	if (s->outbuf != NULL)
		free(s->outbuf);
	
	if (s)
		free(s);
	
	return err;
}

gzFile gzopen(const char *path, const char *mode)
{
	gz_stream_t *s = (gz_stream_t *)malloc(sizeof(gz_stream_t));
	if (!s)
		return Z_NULL;
	
	s->stream.zalloc = (alloc_func)0;
   s->stream.zfree = (free_func)0;
   s->stream.opaque = (void *)0;
   s->stream.next_in = s->inbuf = Z_NULL;
   s->stream.next_out = s->outbuf = Z_NULL;
   s->stream.avail_in = s->stream.avail_out = 0;
   s->file = NULL;
	//s->crc = 0;
	s->mode = mode[0]; /* Unused; so not set */

	if (s->mode == 'w') {
		int err;
		err = deflateInit2(&(s->stream), 1, Z_DEFLATED,
		                   MAX_WBITS+16, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
		s->stream.next_out = s->outbuf = malloc(Z_BUFSIZE);
		if (err != Z_OK || s->outbuf == Z_NULL) {
			destroy(s);
			return Z_NULL;
		}
	} else if (s->mode == 'r') {
		int err;
		s->stream.next_in = s->inbuf = malloc(Z_BUFSIZE);

		err = inflateInit2(&(s->stream), MAX_WBITS+16);
      if (err != Z_OK || s->inbuf == Z_NULL) {
			destroy(s);
			return (gzFile)Z_NULL;
		}
	}
	s->stream.avail_out = Z_BUFSIZE;

	s->file = fopen(path, mode); 
	if (s->file == NULL) {
		destroy(s);
		return Z_NULL;
	}
	return(s);
}



int gzwrite(gzFile file, char *buf, int32_t len)
{
	gz_stream_t *s = (gz_stream_t *)file;
	if (s == NULL || s->mode != 'w')
		return Z_STREAM_ERROR;
	
	s->stream.next_in = (uint8_t *)buf;
	s->stream.avail_in = len;

	while (s->stream.avail_in != 0) {
		if (s->stream.avail_out == 0) {
			s->stream.next_out = s->outbuf;
			if (fwrite(s->outbuf, 1, Z_BUFSIZE, s->file) != Z_BUFSIZE) {
				s->z_err = Z_ERRNO;
				break;
			}
			s->stream.avail_out = Z_BUFSIZE;
		}
		s->in += s->stream.avail_in;
		s->out += s->stream.avail_out;
		s->z_err = deflate(&(s->stream), Z_NO_FLUSH);
		s->in -= s->stream.avail_in;
		s->out -= s->stream.avail_out;
      if (s->z_err != Z_OK) break;
	}
	return (len - s->stream.avail_in);
}

static int
do_flush(gzFile file, int flush)
{
	int done = 0;
	gz_stream_t *s = (gz_stream_t *)file;
    if (s == NULL || s->mode != 'w') return Z_STREAM_ERROR;

    s->stream.avail_in = 0; /* should be zero */
    for (;;) {
        uint32_t len = Z_BUFSIZE - s->stream.avail_out;

        if (len != 0) {
            if ((uint32_t)fwrite(s->outbuf, 1, len, s->file) != len) {
                s->z_err = Z_ERRNO;
                return Z_ERRNO;
            }
            s->stream.next_out = s->outbuf;
            s->stream.avail_out = Z_BUFSIZE;
        }
        if (done) 
		  	break;
        s->out += s->stream.avail_out;
        s->z_err = deflate(&(s->stream), flush);
        s->out -= s->stream.avail_out;

		/* Resetting error when the error returned by Zlib is
		   No buffer space. (We always have buffer space) */
        if (len == 0 && s->z_err == Z_BUF_ERROR) s->z_err = Z_OK;

		/* Done only when error is Z_STREAM_END */
        done = (s->z_err == Z_STREAM_END);

		/* Error cases breaking away here (Last output not written) */
        if (s->z_err != Z_OK && s->z_err != Z_STREAM_END) 
		  	break;
    }
    return  (s->z_err == Z_STREAM_END ? Z_OK : s->z_err);
}

int gzclose(gzFile file)
{
	gz_stream_t *s = (gz_stream_t *)file;

	if (s == NULL) 
		return Z_STREAM_ERROR;

	if (s->mode == 'w') {
		(void)do_flush(file, Z_FINISH);
	}
	return destroy((gz_stream_t *)file);
}

int gzputs(gzFile file, const char *s)
{
	return gzwrite(file, (char *)s, (unsigned)strlen(s));
}

int gzputc(gzFile file, int c)
{
    unsigned char cc = (unsigned char) c; 
	 return ((gzwrite(file, (char *)&cc, 1) == 1) ? (int)cc : -1);
}

#if 0
const char * gzerror(gzFile file, int *errnum)
{
	char *m;
	gz_stream_t *s = (gz_stream_t *)file;

   if (s == NULL) {
        *errnum = Z_STREAM_ERROR;
        return (const char*)ERR_MSG(Z_STREAM_ERROR);
   }
    *errnum = s->z_err;
    if (*errnum == Z_OK) return (const char*)"";

    m = (char*)(*errnum == Z_ERRNO ? strerror(errno) : s->stream.msg);

    if (m == NULL || *m == '\0') m = (char*)ERR_MSG(s->z_err);

	 if (s->msg)
	 	free(s->msg);
	
	s->msg = (uint8_t *)malloc(strlen(m) + 3);
	if (s->msg == Z_NULL)
		return ((const char *)ERR_MSG(Z_MEM_ERROR));
	strcpy((char *)s->msg, m);
	return (const char *)s->msg;
}
#endif

int gzread(gzFile file, char *buf, int32_t len)
{
    gz_stream_t *s = (gz_stream_t *)file;

    if (s == NULL || s->mode != 'r') return Z_STREAM_ERROR;
    if (s->z_err == Z_DATA_ERROR || s->z_err == Z_ERRNO) return -1;
    if (s->z_err == Z_STREAM_END) return 0;  /* EOF */

	 s->stream.next_out = (uint8_t *)buf;
	 s->stream.avail_out = len;

	 while (s->stream.avail_out != 0) {
	 	if (s->stream.avail_in == 0 && !s->z_eof) {
			s->stream.avail_in = (uint32_t)fread(s->inbuf, 1, Z_BUFSIZE, s->file);
			if (s->stream.avail_in == 0) {
				s->z_eof = 1;
				if (ferror(s->file)) {
					s->z_err = Z_ERRNO;
					break;
				}
			}
         s->stream.next_in = s->inbuf;
		}
		s->in += s->stream.avail_in;
		s->out += s->stream.avail_out;
		s->z_err = inflate(&(s->stream), Z_NO_FLUSH);
		if ((s->z_err != Z_OK) && (s->z_err != Z_STREAM_END))
			break;
		s->in -= s->stream.avail_in;
		s->out -= s->stream.avail_out;
		if (s->z_err != Z_OK || s->z_eof) break;
	 }
	 if (s->z_eof) {
	 	do {
			if((s->stream.avail_in==0) && (s->stream.avail_out==0)) break;
			s->z_err = inflate(&(s->stream), Z_FINISH);
			if ((s->z_err != Z_OK) && (s->z_err != Z_STREAM_END))
				break;
			s->in += s->stream.avail_in;
			s->out -= s->stream.avail_out;
		} while (s->z_err != Z_STREAM_END);
	 }
	 return (int)(len - s->stream.avail_out);
}

int gzgetc(gzFile file)
{
	char c;
	return gzread(file, &c, 1) == 1 ? c : -1;
}

char * gzgets(gzFile file, char *buf, int len)
{
	char *b = buf;
	if (buf == Z_NULL || len <= 0) return Z_NULL;

	while (--len > 0 && gzread(file, buf, 1) == 1 && *buf++ != '\n');
   *buf = '\0';
   return b == buf && len > 0 ? Z_NULL : b;
}
