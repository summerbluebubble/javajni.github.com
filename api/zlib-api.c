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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include <stdint.h>
#include "zlib.h"
#include "deflate.h"
#include "inflate.h"
#include <sys/types.h>
#include <linux/types.h>


#include "n3-zip-drv-common.h"
#include "n3-zip-drv-ioctl.h"

#undef printk
#undef printf
int32_t zip_driver_handle = -1;


#define Z_OPEN_ERROR	Z_MEM_ERROR
int32_t zip_open()
{
	if(zip_driver_handle<0)
	{
		zip_driver_handle = open("/dev/n3-zip-round-robin",O_RDWR);
		if(zip_driver_handle<0)
		{
			fprintf(stderr,"Unable to open /dev/n3-zip-round-robin\n");
			return zip_driver_handle;
		}
	}
	return zip_driver_handle;
}

int32_t  deflateInit(z_streamp strm,int32_t level)
{
	if(zip_open()<0) return -1;
	n3_zip_operation_t op;
	op.sizeofptr = sizeof(void*);
	op.sizeofstrm = sizeof(z_stream);
	op.strm = strm;
	op.arg1 = level;
	int32_t ret = ioctl(zip_driver_handle,N3_ZIP_DRV_IOCTL_DEFLATE_INIT,&op);
	if(ret<0) ret = -errno;
	return ret;
}
int32_t  inflateInit(z_streamp strm)
{
	if(zip_open()<0) return -1;
	n3_zip_operation_t op;
	op.sizeofptr = sizeof(void*);
	op.sizeofstrm = sizeof(z_stream);
	op.strm = strm;
	int32_t ret=ioctl(zip_driver_handle,N3_ZIP_DRV_IOCTL_INFLATE_INIT,&op);
	if(ret<0) ret = -errno;
	return ret;
}
int32_t  deflateInit2(z_streamp strm,int32_t level,int32_t method,
					int32_t windowbits,int32_t memlevel,int32_t strategy)
{
	if(zip_open()<0) return -1;
	n3_zip_operation_t op;
	op.sizeofptr = sizeof(void*);
	op.sizeofstrm = sizeof(z_stream);
	op.strm = strm;
	op.arg1 = level;
	op.arg2 = windowbits;
	op.arg3 = strategy;
	int32_t ret=ioctl(zip_driver_handle,N3_ZIP_DRV_IOCTL_DEFLATE_INIT2,&op);
	if(ret<0) ret = -errno;
	return ret;
}
int32_t  inflateInit2(z_streamp strm,int32_t windowbits)
{
	if(zip_open()<0) return -1;
	n3_zip_operation_t op;
	op.sizeofptr = sizeof(void*);
	op.sizeofstrm = sizeof(z_stream);
	op.strm = strm;
	op.arg2 = windowbits;
	int32_t ret=ioctl(zip_driver_handle,N3_ZIP_DRV_IOCTL_INFLATE_INIT2,&op);
	if(ret<0) ret=-errno;
	return ret;
}
int32_t  deflate_offload(z_streamp strm, int32_t flush)
{
	if(zip_open()<0) return Z_OPEN_ERROR;
	n3_zip_operation_t op;
	op.sizeofptr = sizeof(void*);
	op.sizeofstrm = sizeof(z_stream);
	op.strm = strm;
	op.arg1 = flush;
	int32_t ret = ioctl(zip_driver_handle,N3_ZIP_DRV_IOCTL_DEFLATE,&op);
	if(ret<0) ret=-errno;
	return ret;
}
int32_t  deflate(z_streamp strm,int32_t flush)
{
	int32_t ret;
	if(strm->scatter || strm->gather)
	{
		fprintf(stderr,"Deflate - Scatter/Gather feature is not supported\n");
		return Z_STREAM_N3_ERROR;
	}
	#if 0000  //jijin - as of now, 64k-1 is the biggest size supported - the xfrm apis haven't been tested
	if((strm->avail_in < 2*N3Z_UNSAFE_AVAIL_IN) &&
		(strm->avail_out < 2*N3Z_UNSAFE_AVAIL_IN))
	#else 
	if((strm->avail_in < N3Z_UNSAFE_AVAIL_IN) &&
		(strm->avail_out < N3Z_UNSAFE_AVAIL_IN))
	#endif /* 0000 */ 
	{
		ret = deflate_offload(strm,flush);
	}
	else
	{
		uint32_t pending_avail_in = strm->avail_in;
		uint32_t pending_avail_out = strm->avail_out;
		uint32_t deepbreak=0;
		uint32_t total_avail_out = strm->avail_out;
		uint32_t total_avail_in = strm->avail_in;
		uint32_t iflush = Z_FULL_FLUSH;
		uint32_t total_bytes_written=0;
		strm->avail_in = 0;
		strm->avail_out = 0;
		do
		{
			deepbreak=0;
			if(!strm->avail_in)
			{
				if(pending_avail_in > N3Z_SAFE_AVAIL_IN)
				{
					strm->avail_in = N3Z_SAFE_AVAIL_IN;
					pending_avail_in -= N3Z_SAFE_AVAIL_IN;
				}
				else
				{
					strm->avail_in = pending_avail_in;
					pending_avail_in = 0;
				}
			}
			if(!strm->avail_out)
			{
				if(pending_avail_out > N3Z_SAFE_AVAIL_OUT)
				{
					strm->avail_out = N3Z_SAFE_AVAIL_OUT;
					pending_avail_out -= N3Z_SAFE_AVAIL_OUT;
				}
				else
				{
					strm->avail_out = pending_avail_out;
					pending_avail_out = 0;
				}
			}
			do
			{
				uint32_t old_avail_in = strm->avail_in;
				uint32_t old_avail_out = strm->avail_out;
				ret = deflate_offload(strm,iflush);
				total_bytes_written += (old_avail_out-strm->avail_out);
				if(ret==Z_BUF_ERROR) {deepbreak=4;break;}
				else if(ret<0) {deepbreak = 1;break;}
				else if(ret==Z_STREAM_END) { deepbreak =2;break;}
				else if((old_avail_in == strm->avail_in) && (old_avail_out == strm->avail_out)) {
					deepbreak =3;
					if(pending_avail_in>N3Z_SAFE_AVAIL_IN) 
						deepbreak=4;
					break;
				}
			}while(strm->avail_in || strm->avail_out);
			if(deepbreak==4) continue;
			if(deepbreak) break;
		}while((pending_avail_in && pending_avail_out)||((deepbreak==4)&&(strm->avail_in)));

		strm->avail_in += pending_avail_in;
		strm->avail_out = total_avail_out - total_bytes_written;

		if((!deepbreak)||(deepbreak==3))
		{
			if(flush==Z_FINISH)
			{
				uint32_t pending_avail_out = strm->avail_out;
				uint32_t total_avail_out = strm->avail_out;
				uint32_t total_bytes_written = 0;
				uint32_t deepbreak=0;
				if(pending_avail_out>N3Z_SAFE_AVAIL_OUT)
				{
					strm->avail_out = 0;
					do
					{
						if(!strm->avail_out)
						{
							if(pending_avail_out > N3Z_SAFE_AVAIL_OUT)
							{
								strm->avail_out = N3Z_SAFE_AVAIL_OUT;
								pending_avail_out -= N3Z_SAFE_AVAIL_OUT;
							}
							else
							{
								strm->avail_out = pending_avail_out;
								pending_avail_out = 0;
							}
						}
						do
						{
							uint32_t old_avail_in = strm->avail_in;
							uint32_t old_avail_out = strm->avail_out;
							ret = deflate_offload(strm,Z_FINISH);
							total_bytes_written += (old_avail_out - strm->avail_out);
							if(ret==Z_BUF_ERROR) {deepbreak=5;break;}
							if(ret<0) {deepbreak=1;break;}
							else if(ret == Z_STREAM_END) { deepbreak=2;break;}
							else if((old_avail_in==strm->avail_in) && (old_avail_out == strm->avail_out)) { deepbreak=3;break;}
						}while(strm->avail_out);
						if(deepbreak==5) {deepbreak=0;continue;}
						if(deepbreak) break;
					}while(pending_avail_out&&(ret!=Z_STREAM_END)&&(!deepbreak));
					strm->avail_out = total_avail_out - total_bytes_written;
				}
				else
				{
					do
					{
						ret = deflate_offload(strm,Z_FINISH);
					}while((ret!=Z_STREAM_END)&&(ret>=0)&&(strm->avail_out));
				}
			}
		}
	}/* 64K handling */
	return ret;

}
int32_t  inflate_offload(z_streamp strm, int32_t flush)
{
	if(zip_open()<0) return Z_OPEN_ERROR;
	n3_zip_operation_t op;
	op.sizeofptr = sizeof(void*);
	op.sizeofstrm = sizeof(z_stream);
	op.strm = strm;
	op.arg1 = flush;
	int32_t ret = ioctl(zip_driver_handle,N3_ZIP_DRV_IOCTL_INFLATE,&op);
	if(ret<0) ret = -errno;
	return ret;
}
int32_t  inflate(z_streamp strm,int32_t flush)
{
	int32_t ret;
	if(strm->scatter || strm->gather)
	{
		fprintf(stderr,"Inflate - Scatter/Gather feature is not supported\n");
		return Z_STREAM_N3_ERROR;
	}

	#if 0000  //as of now, 64k-1 is the biggest size supported - the xfrm apis haven't been tested
	if((strm->avail_in < 2*N3Z_UNSAFE_AVAIL_IN) &&
		(strm->avail_out < 2*N3Z_UNSAFE_AVAIL_IN))
	#else 
	if((strm->avail_in < N3Z_UNSAFE_AVAIL_IN) &&
		(strm->avail_out < N3Z_UNSAFE_AVAIL_IN))
	#endif /* 0000 */ 
	{
		ret = inflate_offload(strm,flush);
	}
	else
	{
		if(strm->avail_in >= N3Z_UNSAFE_AVAIL_IN){
			fprintf(stderr, "Inflate with avail_in > 64k -1 is not supported as of now\n");
			return Z_STREAM_N3_ERROR;
		}
		uint32_t pending_avail_in = strm->avail_in;
		uint32_t pending_avail_out = strm->avail_out;
		uint32_t deepbreak=0;
		uint32_t total_avail_out = strm->avail_out;
		uint32_t total_avail_in = strm->avail_in;
		uint32_t iflush = Z_FULL_FLUSH;
		uint32_t total_bytes_written=0;
		strm->avail_in = 0;
		strm->avail_out = 0;
		do
		{
			deepbreak=0;
			if(!strm->avail_in)
			{
				if(pending_avail_in > N3Z_SAFE_AVAIL_IN)
				{
					strm->avail_in = N3Z_SAFE_AVAIL_IN;
					pending_avail_in -= N3Z_SAFE_AVAIL_IN;
				}
				else
				{
					strm->avail_in = pending_avail_in;
					pending_avail_in = 0;
				}
			}
			if(!strm->avail_out)
			{
				if(pending_avail_out > N3Z_SAFE_AVAIL_OUT)
				{
					strm->avail_out = N3Z_SAFE_AVAIL_OUT;
					pending_avail_out -= N3Z_SAFE_AVAIL_OUT;
				}
				else
				{
					strm->avail_out = pending_avail_out;
					pending_avail_out = 0;
				}
			}
			do
			{
				uint32_t old_avail_in = strm->avail_in;
				uint32_t old_avail_out = strm->avail_out;
				ret = inflate_offload(strm,iflush);
				total_bytes_written += (old_avail_out-strm->avail_out);
				if(ret==Z_BUF_ERROR) {deepbreak=4;break;}
				else if(ret<0) {deepbreak = 1;break;}
				else if(ret==Z_STREAM_END) { deepbreak =2;break;}
				else if(ret==Z_NEED_DICT) {deepbreak=5;break;}
				else if((old_avail_in == strm->avail_in) && (old_avail_out == strm->avail_out)) {	
					deepbreak =3;
					if(pending_avail_in) 
						deepbreak=4;
					break;
				}
			}while(strm->avail_in || strm->avail_out);
			if(deepbreak==4) continue;
			if(deepbreak) break;
		}while((pending_avail_in && pending_avail_out) || ((deepbreak==4)&&(strm->avail_in)) || (deepbreak==4) &&(pending_avail_out));

		strm->avail_in += pending_avail_in;
		strm->avail_out = total_avail_out - total_bytes_written;

		if((!deepbreak)||(deepbreak==3))
		{
			if(flush==Z_FINISH)
			{
				uint32_t pending_avail_out = strm->avail_out;
				uint32_t total_avail_out = strm->avail_out;
				uint32_t total_bytes_written = 0;
				uint32_t deepbreak=0;
				if(pending_avail_out>N3Z_SAFE_AVAIL_OUT)
				{
					strm->avail_out = 0;
					do
					{
						if(!strm->avail_out)
						{
							if(pending_avail_out > N3Z_SAFE_AVAIL_OUT)
							{
								strm->avail_out = N3Z_SAFE_AVAIL_OUT;
								pending_avail_out -= N3Z_SAFE_AVAIL_OUT;
							}
							else
							{
								strm->avail_out = pending_avail_out;
								pending_avail_out = 0;
							}
						}
						do
						{
							uint32_t old_avail_in = strm->avail_in;
							uint32_t old_avail_out = strm->avail_out;
							ret = inflate_offload(strm,Z_FINISH);
							total_bytes_written += (old_avail_out - strm->avail_out);
							if(ret==Z_BUF_ERROR) {deepbreak=5;break;}
							if(ret<0) {deepbreak=1;break;}
							else if(ret == Z_STREAM_END) { deepbreak=2;break;}
							else if((old_avail_in==strm->avail_in) && (old_avail_out == strm->avail_out)) { deepbreak=3;break;}
						}while(strm->avail_out);
						if(deepbreak==5){deepbreak=0;continue;}	
						if(deepbreak) break;
					}while(pending_avail_out&&(ret!=Z_STREAM_END)&&(!deepbreak));
					strm->avail_out = total_avail_out - total_bytes_written;
				}
				else
				{
					do
					{
						ret = inflate_offload(strm,Z_FINISH);
					}while((ret!=Z_STREAM_END)&&(ret>=0)&&(strm->avail_out));
				}
			}
		}
	}/* 64K handling */
	return ret;
}/* end inflate */
int32_t  deflate_nb(z_streamp strm,int32_t flush)
{
	if(zip_open()<0) return Z_OPEN_ERROR;
	fprintf(stderr, "Non-Blocking mode not supported\n");
	return Z_STREAM_N3_ERROR;
	if(strm->scatter || strm->gather)
	{
		fprintf(stderr,"Deflate_nb - Scatter/Gather feature is not supported\n");
		return Z_STREAM_N3_ERROR;
	}

	n3_zip_operation_t op;
	op.sizeofptr = sizeof(void*);
	op.sizeofstrm = sizeof(z_stream);
	op.strm = strm;
	op.arg1 = flush;
	int32_t ret = ioctl(zip_driver_handle,N3_ZIP_DRV_IOCTL_DEFLATE_NB,&op);
	if(ret<0) ret = -errno;
	return ret;
}
int32_t  inflate_nb(z_streamp strm, int32_t flush)
{
	if(zip_open()<0) return Z_OPEN_ERROR;
	fprintf(stderr, "Non-Blocking mode not supported\n");
	return Z_STREAM_N3_ERROR;
	if(strm->scatter || strm->gather)
	{
		fprintf(stderr,"Inflate_nb - Scatter/Gather feature is not supported\n");
		return Z_STREAM_N3_ERROR;
	}

	n3_zip_operation_t op;
	op.sizeofptr = sizeof(void*);
	op.sizeofstrm = sizeof(z_stream);
	op.strm = strm;
	op.arg1 = flush;
	int32_t ret = ioctl(zip_driver_handle,N3_ZIP_DRV_IOCTL_INFLATE_NB,&op);
	if(ret<0) ret = -errno;
	return ret;
}

int32_t  deflate_poll(z_streamp strm)
{
	if(zip_open()<0) return Z_OPEN_ERROR;
	n3_zip_operation_t op;
	op.sizeofptr = sizeof(void*);
	op.sizeofstrm = sizeof(z_stream);
	op.strm = strm;
	int32_t ret = ioctl(zip_driver_handle,N3_ZIP_DRV_IOCTL_DEFLATE_POLL,&op);
	if(ret<0) ret = -errno;
	return ret;
}

int32_t  inflate_poll(z_streamp strm)
{
	if(zip_open()<0) return Z_OPEN_ERROR;
	n3_zip_operation_t op;
	op.sizeofptr = sizeof(void*);
	op.sizeofstrm = sizeof(z_stream);
	op.strm = strm;
	int32_t ret = ioctl(zip_driver_handle,N3_ZIP_DRV_IOCTL_INFLATE_POLL,&op);
	if(ret<0) ret = -errno;
	return ret;
}

int32_t  deflateEnd(z_streamp strm)
{
	//printf("Inside Function %s\n",__FUNCTION__);
	if(zip_open()<0) return Z_OPEN_ERROR;

	n3_zip_operation_t op;
	op.sizeofptr = sizeof(void*);
	op.sizeofstrm = sizeof(z_stream);
	op.strm = strm;
	int32_t ret = ioctl(zip_driver_handle,N3_ZIP_DRV_IOCTL_DEFLATE_END,&op);
	if(ret<0) ret = -errno;
	return ret;
}
int32_t  inflateEnd(z_streamp strm)
{
	if(zip_open()<0) return Z_OPEN_ERROR;

	n3_zip_operation_t op;
	op.sizeofptr = sizeof(void*);
	op.sizeofstrm = sizeof(z_stream);
	op.strm = strm;
	int32_t ret = ioctl(zip_driver_handle,N3_ZIP_DRV_IOCTL_INFLATE_END,&op);
	if(ret<0) ret = -errno;
	return ret;
}
int32_t  deflateSetDictionary (z_streamp strm,const Bytef *dictionary,uInt  dictLength)
{
	if(zip_open()<0) return Z_OPEN_ERROR;

	n3_zip_operation_t op;
	op.sizeofptr = sizeof(void*);
	op.sizeofstrm = sizeof(z_stream);
	op.strm = strm;
	/* _MIPS_SIM comes from asm/unistd.h */
	//#if _MIPS_SIM == _MIPS_SIM_NABI32
	//op.arg1 = (uint64_t)(uint32_t)dictionary;
	//#else
	op.arg1 = (uint64_t)(unsigned long)dictionary;
	//#endif
	op.arg2 = dictLength;
	int32_t ret = ioctl(zip_driver_handle,N3_ZIP_DRV_IOCTL_DEFLATE_SET_DICTIONARY,&op);
	if(ret<0) ret=-errno;
	return ret;
}
int32_t  inflateSetDictionary (z_streamp strm,const Bytef *dictionary,uInt  dictLength)
{
	if(zip_open()<0) return Z_OPEN_ERROR;

	n3_zip_operation_t op;
	op.sizeofptr = sizeof(void*);
	op.sizeofstrm = sizeof(z_stream);
	op.strm = strm;
	//#if _MIPS_SIM == _MIPS_SIM_NABI32
	//op.arg1 = (uint64_t)(uint32_t)dictionary;
	//#else
	op.arg1 = (uint64_t)(unsigned long)dictionary;
	//#endif
	op.arg2 = dictLength;
	int32_t ret = ioctl(zip_driver_handle,N3_ZIP_DRV_IOCTL_INFLATE_SET_DICTIONARY,&op);
	if(ret<0) ret = -errno;
	return ret;
}

int32_t deflateReset (z_streamp strm)
{
#if 0
	deflate_state *s;
	if(strm==Z_NULL || strm->state==Z_NULL)
		return Z_STREAM_ERROR;

	strm->total_in = 0;
	strm->total_out = 0;
	strm->msg = Z_NULL;
	strm->data_type = Z_UNKNOWN;

	s = ABICAST2ptr(strm->state);
	s->pending = 0;
	s->pending_out = s->pending_buf;

	strm->adler = (s->wrap==2) ? 0 : 1;
	s->last_flush = Z_NO_FLUSH;
#endif

	return Z_OK;
}

int32_t inflateReset(z_streamp strm)
{
	inflate_state *s;
	if(strm==Z_NULL || strm->state==Z_NULL)
		return Z_STREAM_ERROR;
	strm->total_in = strm->total_out = 0;
	strm->msg = Z_NULL;
	strm->data_type = Z_UNKNOWN;
	
	s = ABICAST2ptr(strm->state);

	s->pending = 0;
	s->pending_out = s->pending_buf;
	
	if(s->wrap<0)
	s->wrap = -s->wrap;
	
	s->last_flush = Z_NO_FLUSH;	

	return Z_OK;
}


#include "zlib-gzio.c"
