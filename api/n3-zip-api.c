
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
#include <sys/types.h>
#include <linux/types.h>


#include "n3-zip-drv-common.h"
#include "n3-zip-drv-ioctl.h"


#include "n3-zip-api.h"

int32_t n3_dev_count = -1;
int32_t n3z_driver_handle[MAX_N3_DEVICES + 1] = {-1, -1, -1, -1, -1};

int32_t n3z_open(int dev_id)
{

	if(n3z_driver_handle[dev_id]<0)
	{
		switch(dev_id){
			case 0:	n3z_driver_handle[dev_id] = open("/dev/n3-zip-drv0",O_RDWR); break;
			case 1:	n3z_driver_handle[dev_id] = open("/dev/n3-zip-drv1",O_RDWR); break;
			case 2:	n3z_driver_handle[dev_id] = open("/dev/n3-zip-drv2",O_RDWR); break;
			case 3:	n3z_driver_handle[dev_id] = open("/dev/n3-zip-drv3",O_RDWR); break;
			case MAX_N3_DEVICES:  n3z_driver_handle[MAX_N3_DEVICES] = open("/dev/n3-zip-round-robin",O_RDWR); break;
		}
		if(n3z_driver_handle[dev_id]<0)
		{
			if(dev_id < MAX_N3_DEVICES)
				fprintf(stderr,"Unable to open /dev/n3-zip-drv%d\n", dev_id);
			else
				fprintf(stderr,"Unable to open /dev/n3-zip-round-robin\n");
			return n3z_driver_handle[dev_id];
		}
	}
	return n3z_driver_handle[dev_id];
}

int32_t n3z_init(int32_t dev_id)
{
	int32_t ret = 0;
	int32_t device = dev_id;

	int n3_dev_count = 0;

	if(dev_id < 0)
		device = MAX_N3_DEVICES;
	else{
		get_n3z_dev_count(&n3_dev_count);

		if(dev_id >= n3_dev_count)
			return -ENODEV;
	}

	if (n3z_driver_handle[device] < 0)
		ret = n3z_open(device);
	else
		ret = n3z_driver_handle[device];

#if 0
	ret = ioctl(n3z_driver_handle, N3Z_IOCTL_VALID_INIT, NULL);
	if (ret < 0) ret = -errno;
#endif
	return ret;
}

int32_t n3z_shutdown(int32_t dev_id)
{
	int32_t device = dev_id;
	if(dev_id < 0){
		device = MAX_N3_DEVICES;
	}
	if (n3z_driver_handle[device] < 0)
		return -1;
	close (n3z_driver_handle[device]);
	return 0;
}

int32_t
zq_set_exec_mask(uint32_t mask)
{
	int32_t ret;
	ret = ioctl(n3z_driver_handle[0], N3Z_IOCTL_SET_EXEC_MASK, mask);
	if (ret < 0) 
		ret = -errno;
	return ret;
}

int32_t
get_n3z_dev_count(int32_t *count)
{
	int32_t ret;
	n3z_driver_handle[MAX_N3_DEVICES] = open("/dev/n3-zip-round-robin",O_RDWR);
	ret = ioctl(n3z_driver_handle[4], N3Z_IOCTL_GET_N3_COUNT, count);
	if (n3z_driver_handle[MAX_N3_DEVICES] < 0)
		close (n3z_driver_handle[MAX_N3_DEVICES]);
	if (ret < 0) 
		ret = -errno;
	return ret;
}

static int32_t
zqadd(cvmx_zip_command_t *cmd, int32_t driver_handle)
{
	int32_t ret;
	ret = ioctl(driver_handle, N3Z_IOCTL_SUBMIT_INSTR, cmd);
	if (ret < 0) 
		ret = -errno;
	return ret;
}

static int32_t
zqadd_speed(cvmx_zip_command_t *cmd, int32_t driver_handle)
{
	int32_t ret;
	ret = ioctl(driver_handle, N3Z_IOCTL_SPEED_INSTR, cmd);
	if (ret < 0) 
		ret = -errno;
	return ret;
}

void cndbgcmd(cvmx_zip_command_t *cmd,char *caption)
{
	printf("[%s] cmd=%p\n",caption,cmd);
	printf("IWORD0:\n");
	printf("totaloutputlength=%u\n",cmd->s.totaloutputlength);
	printf("exnum=%x\n",cmd->s.exnum);
	printf("exbits=%x\n",cmd->s.exbits);
	printf("forcefixed=%u\n",cmd->s.forcefixed);
	printf("forcedynamic=%u\n",cmd->s.forcedynamic);
	printf("eof=%u\n",cmd->s.eof);
	printf("bof=%u\n",cmd->s.bof);
	printf("speed=%d\n",(int)cmd->s.speed);
	printf("speed=%d\n",(int)cmd->s.speed);
	printf("compress=%u\n",cmd->s.compress);
	printf("dscatter=%u\n",cmd->s.dscatter);
	printf("dgather=%u\n",cmd->s.dgather);
	printf("hgather=%u\n",cmd->s.hgather);

	printf("IWORD1:\n");
	printf("history length=%u\n",cmd->s.historylength);

	printf("adler32=%x\n",(unsigned)cmd->s.adler32);
	printf("IWORD2=%lx\n",(long)cmd->s.ctx_ptr.u64);
	printf("IWORD3=%lx\n",(long)cmd->s.hist_ptr.u64);
	printf("IWORD4=%lx\n",(long)cmd->s.in_ptr.u64);
	printf("IWORD5=%lx\n",(long)cmd->s.out_ptr.u64);
	printf("IWORD6=%lx\n",(long)cmd->s.result_ptr.u64);
	printf("IWORD7=%lx\n",(long)cmd->s.prefix_grp_ptr.u64);
}

static uint64_t
sgtotal(cvmx_zip_ptr_t sg)
{
  uint64_t total       = 0;
  cvmx_zip_ptr_t *zptr = (cvmx_zip_ptr_t *)((unsigned long)(uint64_t)(sg.s.ptr));

  uint64_t i;
  for(i=0; i<sg.s.length; i++)
  {
    total += zptr[i].s.length;
  }
  return total; 
}

int32_t 
zdecompress(
  int32_t device_handle,
  cvmx_zip_ptr_t      sgin,
  cvmx_zip_ptr_t      sgout,
  cvmx_zip_result_t   *r,
  int32_t             wbits,
  int32_t             btype)
{
  cvmx_zip_command_t cmd;

  memset (&cmd, 0, sizeof(cmd));

  cmd.s.bof = 1;
  cmd.s.eof = 1;

  cmd.s.speed             = 0;
  cmd.s.compress          = 0;

  if(btype == BTYPE_LZS) {
   cmd.s.forcefixed = 1;
   cmd.s.forcedynamic = 1;
  }
  else
  {
   cmd.s.forcefixed        = 0;
   cmd.s.forcedynamic      = 0;
  }
  cmd.s.totaloutputlength = sgtotal(sgout);

  if(sgin.s.length > 1)
  {
    cmd.s.in_ptr          = sgin;
    cmd.s.dgather         = 1;
  }
  else
  {
    cvmx_zip_ptr_t *sglist = (void*)(unsigned long)(uint64_t)(sgin.s.ptr);
    cmd.s.in_ptr          = sglist[0];
    cmd.s.dgather         = 0;
  }

  if(sgout.s.length > 1)
  {
    cmd.s.out_ptr         = sgout;
    cmd.s.dscatter        = 1;
  }
  else
  {
    cvmx_zip_ptr_t *sglist = (void*)(unsigned long)(uint64_t)(sgout.s.ptr);
    cmd.s.out_ptr         = sglist[0];
    cmd.s.dscatter        = 0;
  }


  cmd.s.result_ptr.s.ptr  = (uint64_t)(unsigned long)(r);
  cmd.s.adler32           = (wbits == WBITS_ZLIB) ? 1 : 0;
  cmd.s.prefix_grp_ptr.u64    = 0;
  r->s.completioncode      = 0;
  //cndbgcmd(&cmd, "User Space");

  return zqadd(&cmd, device_handle); 
}

int32_t 
zcompress(
  int32_t device_handle,
  cvmx_zip_ptr_t      sgin,
  cvmx_zip_ptr_t      sgout,
  cvmx_zip_result_t   *r,
  int32_t             wbits,
  int32_t             btype)
{
    cvmx_zip_command_t cmd;

    memset (&cmd, 0, sizeof(cmd));
    cmd.s.bof = 1;
    cmd.s.eof = 1;
    cmd.s.compress = 1;

    if(btype == BTYPE_AUTO)
    {
      cmd.s.forcefixed   = 0;
      cmd.s.forcedynamic = 0;
    }
    else if(btype == BTYPE_FIXEDHUFFMAN)
    {
      cmd.s.forcefixed   = 1;
      cmd.s.forcedynamic = 0;
    }
    else if(btype == BTYPE_LZS)
    {
      cmd.s.forcefixed   = 1;
      cmd.s.forcedynamic = 1;
    }
    else
    {
      cmd.s.forcefixed   = 0;
      cmd.s.forcedynamic = 1;
    }

    #if DEFLATE_INIT_LEVEL == Z_BEST_SPEED
      cmd.s.speed             = 1;
    #endif
    cmd.s.totaloutputlength = sgtotal(sgout);
    cmd.s.adler32           = (wbits == WBITS_ZLIB) ? 1 : 0;

    if(sgin.s.length > 1)
    {
      cmd.s.in_ptr          = sgin;
      cmd.s.dgather         = 1;
    }
    else
    {
      cvmx_zip_ptr_t *sglist = (void*)(unsigned long)(uint64_t)(sgin.s.ptr);
      cmd.s.in_ptr          = sglist[0];
      cmd.s.dgather         = 0;
    }

    if(sgout.s.length > 1)
    {
      cmd.s.out_ptr         = sgout;
      cmd.s.dscatter        = 1;
    }
    else
    {
      cvmx_zip_ptr_t *sglist = (void*)(unsigned long)(uint64_t)(sgout.s.ptr);
      cmd.s.out_ptr         = sglist[0];
      cmd.s.dscatter        = 0;
    }

    cmd.s.result_ptr.s.ptr  = (uint64_t)(unsigned long)(r);
    cmd.s.prefix_grp_ptr.u64    = 0;
    r->s.completioncode     = 0;
    //cndbgcmd(&cmd, "User Space");
    return zqadd(&cmd, device_handle);
}

int32_t
zstatus(
  cvmx_zip_result_t *r,
  int32_t           wbits,
  int32_t           op,
  uint32_t          *tbw)
{
  int32_t ret = Z_OK;
  /* command status */
  //printf("completion code %d \n", r->s.completioncode);
  int i;
#if 0
  uint8_t *ptr = r;
  printf("response buffer\n");
  for (i = 0; i < sizeof(cvmx_zip_result_t); i++) {
	if (i && (i%8 == 0)) printf("\n");
	printf("%02x", ptr[i]);
  }
#endif
  switch(r->s.completioncode)
  {
    case 0: 
    { 
      ret = Z_OK;
      break;
    }
    case 1: 
    {
      if((op == OP_COMPRESSION) || (op == OP_LZS_DECOMPRESSION))
      {
        ret = Z_STREAM_END;
      }
      else
      {
        if(r->s.eof != 1)
          return Z_DATA_ERROR;
  
        if((((r->s.totalbitsprocessed+7)/8) > r->s.totalbytesread))
          return Z_DATA_ERROR;

        ret = Z_STREAM_END;
      }
      break;
    }
    case 2:  return Z_BUF_ERROR;
    default: return Z_DATA_ERROR;
  }
 //printf("ret %d \n", ret);


#if 0
  if(op == OP_COMPRESSION)
  {
    /* tbw stats */
    if(wbits == WBITS_RAW_RFC1951)
      tbw[0] = r->s.totalbyteswritten;
    else if(wbits == WBITS_ZLIB)
      tbw[0] = r->s.totalbyteswritten + ZLIB_HDR_SIZE + ZLIB_FTR_SIZE;
    else
      tbw[0] = r->s.totalbyteswritten + GZIP_HDR_SIZE + GZIP_FTR_SIZE;

    /* generate footer */
#if 0
    if(wbits != WBITS_RAW_RFC1951)
    {
      /* hdr ptr is already updated */
      if(genftr(sg,r,wbits))
        return Z_BUF_ERROR; 
    }
#endif
  }
  else
  {
    /* tbw stats */
    tbw[0] = r->s.totalbyteswritten;
     
    #ifdef FTR_CHECKS
    /* chksum detection stats */
    if(wbits != WBITS_RAW_RFC1951)
    {
      uint32_t chksum = getchksum(sg,r,wbits);
      if(chksum != ((wbits == WBITS_ZLIB)?r->s.adler:r->s.crc32))
        ret = Z_DATA_ERROR;
    } 
    #endif
  }
    #endif
  return ret;
}

int32_t 
zcompress_speed(
  int32_t device_handle,
  cvmx_zip_ptr_t      sgin,
  cvmx_zip_ptr_t      sgout,
  cvmx_zip_result_t   *r,
  int32_t             wbits,
  int32_t             btype)
{
    cvmx_zip_command_t cmd;

    memset (&cmd, 0, sizeof(cmd));
    cmd.s.bof = 1;
    cmd.s.eof = 1;
    cmd.s.compress = 1;

    if(btype == BTYPE_AUTO)
    {
      cmd.s.forcefixed   = 0;
      cmd.s.forcedynamic = 0;
    }
    else if(btype == BTYPE_FIXEDHUFFMAN)
    {
      cmd.s.forcefixed   = 1;
      cmd.s.forcedynamic = 0;
    }
    else if(btype == BTYPE_LZS)
    {
      cmd.s.forcefixed   = 1;
      cmd.s.forcedynamic = 1;
    }
    else
    {
      cmd.s.forcefixed   = 0;
      cmd.s.forcedynamic = 1;
    }

    cmd.s.speed             = 3;
    cmd.s.totaloutputlength = sgtotal(sgout);
    cmd.s.adler32           = (wbits == WBITS_ZLIB) ? 1 : 0;

    if(sgin.s.length > 1)
    {
      cmd.s.in_ptr          = sgin;
      cmd.s.dgather         = 1;
    }
    else
    {
      cvmx_zip_ptr_t *sglist = (void*)(unsigned long)(uint64_t)(sgin.s.ptr);
      cmd.s.in_ptr          = sglist[0];
      cmd.s.dgather         = 0;
    }

    if(sgout.s.length > 1)
    {
      cmd.s.out_ptr         = sgout;
      cmd.s.dscatter        = 1;
    }
    else
    {
      cvmx_zip_ptr_t *sglist = (void*)(unsigned long)(uint64_t)(sgout.s.ptr);
      cmd.s.out_ptr         = sglist[0];
      cmd.s.dscatter        = 0;
    }

    cmd.s.result_ptr.s.ptr  = (uint64_t)(unsigned long)(r);
    cmd.s.prefix_grp_ptr.u64    = 0;
    r->s.completioncode     = 0;
    return zqadd_speed(&cmd, device_handle);
}

int32_t
zstatus_speed(cvmx_zip_result_t *res)
{
	double ops;
	uint64_t total_time = res->u64[0];
	uint32_t totalbytesread = res->s.totalbytesread;
	uint32_t totalbyteswritten = res->s.totalbyteswritten;
	uint32_t reqs = res->u64[2];
	double  inmbps, outmbps;
	
	ops = (double) (((double) reqs * (double) 1000 * (double) 1000)/ ((double) (total_time)));
	//printf("ops %-10.0lf\n", ops);
	inmbps = (((double)totalbytesread*ops*8)/(1024*1024));
	outmbps = (((double)totalbyteswritten*ops*8)/(1024*1024));
	//printf("InMbps %-10.0lf \n", inmbps);
	//printf("OutMbps %-10.0lf \n", outmbps);
	res->u64[0] = ops;
	res->u64[1] = inmbps;
	res->u64[2] = outmbps;
	return 0;
}
int32_t 
zdecompress_speed(
		int32_t device_handle,
		cvmx_zip_ptr_t      sgin,
		cvmx_zip_ptr_t      sgout,
		cvmx_zip_result_t   *r,
		int32_t             wbits,
		int32_t             btype)
{
	cvmx_zip_command_t cmd;

	memset (&cmd, 0, sizeof(cmd));
	cmd.s.bof = 1;
	cmd.s.eof = 1;
	cmd.s.compress = 0;
#if 1
	if(btype == BTYPE_AUTO)
	{
		cmd.s.forcefixed   = 0;
		cmd.s.forcedynamic = 0;
	}
	else if(btype == BTYPE_FIXEDHUFFMAN)
	{
		cmd.s.forcefixed   = 1;
		cmd.s.forcedynamic = 0;
	}
	else if(btype == BTYPE_LZS)
	{
		cmd.s.forcefixed   = 1;
		cmd.s.forcedynamic = 1;
	}
	else
	{
		cmd.s.forcefixed   = 0;
		cmd.s.forcedynamic = 1;
	}
#endif
	cmd.s.speed             = 1;
	cmd.s.totaloutputlength = sgtotal(sgout);
	cmd.s.adler32           = (wbits == WBITS_ZLIB) ? 1 : 0;

	if(sgin.s.length > 1)
	{
		cmd.s.in_ptr          = sgin;
		cmd.s.dgather         = 1;
	}
	else
	{
		cvmx_zip_ptr_t *sglist = (void*)(unsigned long)(uint64_t)(sgin.s.ptr);
		cmd.s.in_ptr          = sglist[0];
		cmd.s.dgather         = 0;
	}

	if(sgout.s.length > 1)
	{
		cmd.s.out_ptr         = sgout;
		cmd.s.dscatter        = 1;
	}
	else
	{
		cvmx_zip_ptr_t *sglist = (void*)(unsigned long)(uint64_t)(sgout.s.ptr);
		cmd.s.out_ptr         = sglist[0];
		cmd.s.dscatter        = 0;
	}

	cmd.s.result_ptr.s.ptr  = (uint64_t)(unsigned long)(r);
	cmd.s.prefix_grp_ptr.u64    = 0;
	r->s.completioncode     = 0;
	return zqadd_speed(&cmd, device_handle);
}
