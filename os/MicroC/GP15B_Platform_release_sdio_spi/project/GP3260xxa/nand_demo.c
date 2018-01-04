#include "application.h"
#include "drv_l1_uart.h"
#include "console.h"
#include "drv_l2_nand_ext.h"
#include "usbd.h"
#undef gp_memcpy // undefine gp_memcpy in usbd.h that use standard memcpy
#undef gp_memset // undefine gp_memset in usbd.h that use standard memset
#undef gp_free	 // undefine gp_free that call no thinng 
#undef gp_malloc // undefine gp_free that call no thinng
#include "gp_stdlib.h"
#include "gplib_mm_gplus.h"

#if (defined(NAND1_EN) && (NAND1_EN == 1)) || \
	(defined(NAND2_EN) && (NAND2_EN == 1)) || \
	(defined(NAND3_EN) && (NAND3_EN == 1)) || \
	((defined NAND_APP_EN) && (NAND_APP_EN == 1))
	

extern void usbd_uninit(void);

static void nand_cmd_handler(int argc, char *argv[]);

#if (defined(NAND1_EN) && (NAND1_EN == 1)) || \
	    (defined(NAND2_EN) && (NAND2_EN == 1)) || \
	    (defined(NAND3_EN) && (NAND3_EN == 1)) 
static void nand_cmd_data_handler(int argc, char *argv[]);
#endif

#if ((defined NAND_APP_EN) && (NAND_APP_EN == 1))
static void nand_cmd_app_handler(int argc, char *argv[]);
#endif

static cmd_t nand_cmd_list[] = 
{
	{"nand",  nand_cmd_handler,  NULL },
	{NULL,    NULL,   NULL}
};

void nand_cmd_register(void)
{
	cmd_t *pcmd = &nand_cmd_list[0];

	while (pcmd->cmd != NULL) 
	{
		cmdRegister(pcmd);
		pcmd += 1;
	}
}

void Nand_Demo(void)
{
	#if defined(GPLIB_CONSOLE_EN) && (GPLIB_CONSOLE_EN == 1)
	nand_cmd_register();
	#endif
}

static void nand_cmd_help(void)
{
    DBG_PRINT(
    	"\r\nUsage:\r\n"
    	
 	#if (defined(NAND1_EN) && (NAND1_EN == 1)) || \
	    (defined(NAND2_EN) && (NAND2_EN == 1)) || \
	    (defined(NAND3_EN) && (NAND3_EN == 1)) 
    	"    nand data help\r\n"
    #endif
    
    #if (defined NAND_APP_EN) && (NAND_APP_EN == 1)
        "    nand app help\r\n"
    #endif
    
    );
}

static void nand_cmd_handler(int argc, char *argv[])
{
	if (STRCMP(argv[1],"help") == 0)
    {
    	nand_cmd_help();
    }
    
    #if (defined(NAND1_EN) && (NAND1_EN == 1)) || \
	    (defined(NAND2_EN) && (NAND2_EN == 1)) || \
	    (defined(NAND3_EN) && (NAND3_EN == 1)) 
    else if (STRCMP(argv[1],"data") == 0)
    {
        nand_cmd_data_handler(argc - 1, &argv[1]);
    }
    #endif
    
    #if (defined NAND_APP_EN) && (NAND_APP_EN == 1)
 	else if (STRCMP(argv[1],"app") == 0)
    {
        nand_cmd_app_handler(argc - 1, &argv[1]);
    }
    #endif
    
    else
    {
       	nand_cmd_help();
    }
}

static int nf_atoi(char * str)
{
	int num = -1;
	
	if (STRCMP(str, "0") == 0)
		num = 0;
	else if (STRCMP(str, "1") == 0)
		num = 1;
	else if (STRCMP(str, "2") == 0)
		num = 2;
	else if (STRCMP(str, "3") == 0)
		num = 3;	
	else if (STRCMP(str, "4") == 0)
		num = 4;	
	else if (STRCMP(str, "5") == 0)
		num = 5;
	else if (STRCMP(str, "6") == 0)
		num = 6;	
	else if (STRCMP(str, "7") == 0)
		num = 7;
	else if (STRCMP(str, "8") == 0)
		num = 8;
	else if (STRCMP(str, "9") == 0)
		num = 9;
	else if (STRCMP(str, "10") == 0)
		num = 10;
		
	return num;	
}

static void nf_cmd_dump_buffer(unsigned char *addr, unsigned long size)
{
	unsigned long i;	
	
	for(i=0; i<size; i++) 
    {
		if (i%16 == 0) DBG_PRINT("\r\n[%08lx] ",i);
			DBG_PRINT("%02x ",addr[i]);
	}
	DBG_PRINT("\r\n\n");	
}
#else
void Nand_Demo(void)
{
}
#endif

/* ======================= NAND DATA ======================= */
#if (defined(NAND1_EN) && (NAND1_EN == 1)) || \
	(defined(NAND2_EN) && (NAND2_EN == 1)) || \
	(defined(NAND3_EN) && (NAND3_EN == 1))
	
static void nf_demo_data_mount_disk(INT8U drv)
{
	if(_devicemount(drv))					// Mount device nand
	{
		DBG_PRINT("Mount Disk Fail[%d]\r\n", drv);
		#if 0
		{
			INT16S ret;
			
		ret = _format(drv, FAT32_Type); 
		_deviceunmount(drv); 
                _devicemount(drv);		 
        DrvNand_flush_allblk();	 
		}
        #endif
	}
	else
	{
		DBG_PRINT("Mount Disk success[%d]\r\n", drv);
	}
}

static void nf_demo_data_freespace(INT8U drv)
{
	INT64U vfs_disk_free_size;
	
	vfs_disk_free_size = vfsFreeSpace(drv);
	
	DBG_PRINT("disk free size 0x%lx byte\r\n", vfs_disk_free_size);
}

static void nf_demo_data_flush_all(void)
{
	DrvNand_flush_allblk();
}

extern void FlushWorkbuffer(void);

static void nf_demo_data_flush(void)
{
	FlushWorkbuffer();
}

static void nf_demo_data_msdcon(INT8U drv)
{
	_deviceunmount(drv); // to avoid data inconsistency, unmount link with file system
	drv_l2_usbd_msdc_set_lun(LUN_NF_TYPE, LUN_NUM_0, USBD_STORAGE_NO_WPROTECT, &gp_msdc_nand0);
	usbd_register_class(&usbd_msdc_ctl_blk);
	usbd_init();
}

static void nf_demo_data_msdcoff(INT8U drv)
{
	usbd_uninit();
}

static void nf_demo_data_format_disk(INT8U drv)
{
	INT16S ret;
	
	_devicemount(drv); // to do format, disk must be mounted first
	ret = _format(drv, FAT32_Type); 
 	DrvNand_flush_allblk();

	DBG_PRINT("format disk[%d] %s\r\n", drv, (ret == 0 ? "success":"fail"));
}

INT32S nf_demo_data_lowformat_disk(INT8U drv)
{
	INT16S ret;
	
	_deviceunmount(drv); // to avoid data inconsistency, unmount link with file system
	
	ret = DrvNand_lowlevelformat();
	if (ret != 0)
		return ret;
	ret = DrvNand_initial();
	DrvNand_flush_allblk();
	
	if (ret != 0)
	{
		DBG_PRINT("initial fail\r\n");
		return ret;
	}	
	
	ret = _devicemount(drv);
	if (ret != 0)
		DBG_PRINT("mount fail\r\n"); // dont care fail, just printed
	
	ret = _format(drv, FAT32_Type); 
 	DrvNand_flush_allblk();
 	
	DBG_PRINT("low format disk[%d] %s\r\n", drv, ((ret == 0) ? "success":"fail"));
	
	return ret;
}

static void nf_demo_data_mount_test(INT8U drv, UINT32 iteration)
{
	INT16S ret;
	UINT32 k = 0;
	INT16S fp; 
	CHAR *file_path = "A:\\gp326033_driver_test.mcp";
	
	DBG_PRINT("===== mount test start =====\r\n");
	ret = _devicemount(drv);
	if(ret)					// Mount device nand
	{
		DBG_PRINT("0 Mount Disk Fail[%d], iterarion[%d], ret[%d]\r\n", drv, k, ret);
		return;
	}
	else
	{
		DBG_PRINT("0 Mount Disk success[%d], iterarion[%d]\r\n", drv, k);
	}	
	OSTimeDly(20);
	for (k=1; k <= iteration; k++)
	{
		ret = _deviceunmount(drv);
		if(ret)					// Unmount device nand
		{
			DBG_PRINT("Unmount Disk Fail[%d], iterarion[%d], ret[%d]\r\n", drv, k, ret);
		 	return;
		}
		else
		{
			DBG_PRINT("Unmount Disk success[%d], iterarion[%d]\r\n", drv, k);
		}				
		ret = _devicemount(drv);
		if(ret)					// Mount device nand
		{
			DBG_PRINT("Mount Disk Fail[%d], iterarion[%d], ret[%d]\r\n", drv, k, ret);
		 	return;
		}
		else
		{
			DBG_PRINT("Mount Disk success[%d], iterarion[%d]\r\n", drv, k);
		}
		fp = open(file_path, O_RDONLY); 
		if (fp == -1)
		{
			DBG_PRINT("open file [%s] fail. fp[%d]\r\n", file_path, fp);
		}
		else
			DBG_PRINT("open file [%s] success. fp[%d]\r\n", file_path, fp);
		close(fp);
		DrvNand_flush_allblk(); 
	}
	ret = _deviceunmount(drv);
	DrvNand_flush_allblk(); 
	OSTimeDly(20);
	if(ret)					// Unmount device nand
	{
		DBG_PRINT("0 Unmount Disk Fail[%d], iterarion[%d], ret[%d]\r\n", drv, k, ret);
		return;
	}
	else
	{
		DBG_PRINT("0 Unmount Disk success[%d], iterarion[%d]\r\n", drv, k);
	}	
	DBG_PRINT("===== mount test end =====\r\n");
}

static void nand_cmd_data_mount(int argc, char *argv[])
{
	INT8U drv = 255;
	
	if (argc > 2)
		drv = nf_atoi(argv[2]);
		
	if (drv != FS_NAND1)
	{
		DBG_PRINT("illegal driver number [%s], should be %d\r\n", argv[2], FS_NAND1);
		return;
	}
	
	nf_demo_data_mount_disk(drv);
}

static void nand_cmd_data_disksize(int argc, char *argv[])
{
	INT8U drv = 255;
	
	if (argc > 2)
		drv = nf_atoi(argv[2]);
		
	if (drv != FS_NAND1)
	{
		DBG_PRINT("illegal driver number [%s], should be %d\r\n", argv[2], FS_NAND1);
		return;
	}
		
	nf_demo_data_freespace(drv);
}

static void nand_cmd_data_flush_all(int argc, char *argv[])
{
	nf_demo_data_flush_all();
}

static void nand_cmd_data_flush(int argc, char *argv[])
{
	nf_demo_data_flush();
}

static void nand_cmd_data_msdcon(int argc, char *argv[])
{
	INT8U drv = 255;
	
	if (argc > 2)
		drv = nf_atoi(argv[2]);
		
	if (drv != FS_NAND1)
	{
		DBG_PRINT("illegal driver number [%s], should be %d\r\n", argv[2], FS_NAND1);
		return;
	}
	nf_demo_data_msdcon(drv);
}

static void nand_cmd_data_msdcoff(int argc, char *argv[])
{
	INT8U drv = 255;
	
	if (argc > 2)
		drv = nf_atoi(argv[2]);
		
	if (drv != FS_NAND1)
	{
		DBG_PRINT("illegal driver number [%s], should be %d\r\n", argv[2], FS_NAND1);
		return;
	}
	nf_demo_data_msdcoff(drv);
}

static void nand_cmd_data_format(int argc, char *argv[])
{
	INT8U drv = 255;
	
	if (argc > 2)
		drv = nf_atoi(argv[2]);
		
	if (drv != FS_NAND1)
	{
		DBG_PRINT("illegal driver number [%s], should be %d\r\n", argv[2], FS_NAND1);
		return;
	}
	
	nf_demo_data_format_disk(drv);
}

static void nand_cmd_data_lowformat(int argc, char *argv[])
{
	INT8U drv = 255;
	
	if (argc > 2)
		drv = nf_atoi(argv[2]);
	if (drv != FS_NAND1)
	{
		DBG_PRINT("illegal driver number [%s], should be %d\r\n", argv[2], FS_NAND1);
		return;
	}
	
	nf_demo_data_lowformat_disk(drv);
}

static void nand_cmd_data_mounttest(int argc, char *argv[])
{
	INT8U drv = 255;
	
	if (argc > 2)
		drv = nf_atoi(argv[2]);
	if (drv != FS_NAND1)
	{
		DBG_PRINT("illegal driver number [%s], should be %d\r\n", argv[2], FS_NAND1);
		return;
	}
	
	nf_demo_data_mount_test(drv, 100);
}

static void nand_cmd_data_help(void)
{
    DBG_PRINT(
    	"\r\nUsage: nand data help\r\n"
    	"\r\n       nand data mount <drive>\r\n"
    	"\r\n       nand data disksize <drive>\r\n"
        "\r\n       nand data flushall\r\n"
        "\r\n       nand data flush\r\n"
        "\r\n       nand data msdcon <drive>\r\n"
        "\r\n       nand data msdcoff <drive>\r\n"
        "\r\n       nand data format <drive>\r\n"
        "\r\n       nand data lowformat <drive>\r\n"
        "\r\n       nand data mounttest <drive>\r\n"
    	);
}

static void nand_cmd_data_handler(int argc, char *argv[])
{
	if (STRCMP(argv[1],"help") == 0)
    {
    	nand_cmd_data_help();
    }
 	else if (STRCMP(argv[1],"mount") == 0)
    {
    	nand_cmd_data_mount(argc, argv);
    }
	else if (STRCMP(argv[1],"disksize") == 0)
    {
    	nand_cmd_data_disksize(argc, argv);
    }
   	else if (STRCMP(argv[1],"flushall") == 0)
    {
    	nand_cmd_data_flush_all(argc, argv);
    }
	else if (STRCMP(argv[1],"flush") == 0)
    {
    	nand_cmd_data_flush(argc, argv);
    }    
   	else if (STRCMP(argv[1],"msdcon") == 0)
    {
    	nand_cmd_data_msdcon(argc, argv);
    }
    else if (STRCMP(argv[1],"msdcoff") == 0)
    {
    	nand_cmd_data_msdcoff(argc, argv);
    }
	else if (STRCMP(argv[1],"format") == 0)
    {
    	nand_cmd_data_format(argc, argv);
    } 
	else if (STRCMP(argv[1],"lowformat") == 0)
    {
    	nand_cmd_data_lowformat(argc, argv);
    }  
	else if (STRCMP(argv[1],"mounttest") == 0)
    {
    	nand_cmd_data_mounttest(argc, argv);
    }            
    else
    {
    	nand_cmd_data_help();
    }
}
#endif

/* ======================= NAND APP ======================= */
#if (defined NAND_APP_EN) && (NAND_APP_EN == 1)

static void nand_cmd_app_init(int argc, char *argv[])
{
	SINT32 ret;
	
	ret = NandBootInit();
	if (ret == 0)
		DBG_PRINT("app init ok!\r\n");
	else
		DBG_PRINT("app init fail! ret=%d\r\n", ret);
}

static void showAppPartInfo(UINT8 partNum, NF_APP_PART_INFO *partInfo)
{
	DBG_PRINT("App Partition %d Header's Info:\r\n", partNum);
	DBG_PRINT("->Size 0x%x Sectors\r\n", partInfo->part_size);
	DBG_PRINT("->CheckSum 0x%x \r\n", partInfo->checkSum);
	DBG_PRINT("->StartSector 0x%x \r\n", partInfo->startSector);
	DBG_PRINT("->ImageSize 0x%x Sectors\r\n", partInfo->imageSize);
	DBG_PRINT("->DestAddress 0x%x Sectors\r\n", partInfo->destAddress);
	DBG_PRINT("->Type 0x%x Sectors\r\n", partInfo->type);
}

static void nand_cmd_app_partinfo(int argc, char *argv[])
{
	SINT32 ret;
	UINT16 partTotal, k;
	UINT32 partTotalSector;
	NF_APP_PART_INFO partInfo;
	
	ret = NandAppInitParts(&partTotal, &partTotalSector);
	if (ret == 0)
		DBG_PRINT("app parts init ok! part#=%d, size=%d sectors\r\n", partTotal, partTotalSector);
	else
	{
		DBG_PRINT("app parts init fail! ret=%d, part#=%d, size=%d sectors\r\n", ret, partTotal, partTotalSector);
		return;
	}
	for (k = 0; k < partTotal; k++)
	{
		ret = NandAppGetPartInfo(k, &partInfo);
		if (ret == 0)
		{
			showAppPartInfo(k, &partInfo);
		}
		else
			DBG_PRINT("app part info fail. k=%d ret=%d\r\n", k, ret);
	}
}

static void nand_cmd_app_findpart(int argc, char *argv[])
{
	SINT32 ret;
	UINT16 index = 0;
	UINT8 type = 0;
	NF_APP_PART_INFO partInfo;

	if (argc > 2)
	{
		index = nf_atoi(argv[2]);
		if (index > 15)
		{
			DBG_PRINT("part index must between 0~15. %s\r\n", argv[2]);
			return;
		}
	}
	if (argc > 3)
	{
		type = nf_atoi(argv[3]);	
		if (type > APP_KIND_MAX)
		{
			DBG_PRINT("part index must between %d~%d. %s\r\n", APP_KIND_MIN, APP_KIND_MAX, argv[3]);
			return;
		}
	}	
	
	DBG_PRINT("part to be find: index=%d type=%d\r\n", index, type);
	
	ret = NandAppFindPart(index, type, &partInfo);
	if (ret == 0)
		showAppPartInfo(index, &partInfo);
	else
		DBG_PRINT("part not found. index=%d type=%d ret=%d\r\n", index, type, ret);
}

static void nand_cmd_app_readpart(int argc, char *argv[])
{
	SINT32 ret;
	UINT16 index = 0;
	UINT8 type = 0;
	NF_APP_PART_INFO partInfo;

	if (argc > 2)
	{
		index = nf_atoi(argv[2]);
		if (index > 15)
		{
			DBG_PRINT("part index must between 0~15. %s\r\n", argv[2]);
			return;
		}
	}
	if (argc > 3)
	{
		type = nf_atoi(argv[3]);	
		if (type > APP_KIND_MAX)
		{
			DBG_PRINT("part index must between %d~%d. %s\r\n", APP_KIND_MIN, APP_KIND_MAX, argv[3]);
			return;
		}
	}	
	
	DBG_PRINT("part to be find: index=%d type=%d\r\n", index, type);
	
	ret = NandAppFindPart(index, type, &partInfo);
	if (ret == 0)
	{
		void *buf = NULL;
		UINT32 buf_size = 32; // in sector unit, 32 mean 16K
		UINT32 imageSize = partInfo.imageSize;
		UINT16 req_size;
		UINT32 startSector = partInfo.startSector;
		
		showAppPartInfo(index, &partInfo);
		
		buf = gp_malloc_align(buf_size*512, 32);
		
		if (buf != NULL)
		{
			do
			{
				req_size = (imageSize < buf_size) ? imageSize : buf_size;
				DBG_PRINT("read app sector = %d\r\n", startSector);
				ret = NandBootReadSector(startSector, req_size, (UINT32)buf);
				DBG_PRINT("read app sector done = %d, ret=%d\r\n", startSector, ret);
				DBG_PRINT("dump app sector start = %d, nr = %d\r\n", startSector, req_size);
				nf_cmd_dump_buffer(buf, req_size*512);
				if (ret != 0)
					break;
				DBG_PRINT("\r\n");
				startSector += req_size;
				imageSize -= req_size;
			} while (imageSize != 0);
			gp_free(buf);
			NandBootFlush();
		}
	}
	else
		DBG_PRINT("part not found. index=%d type=%d ret=%d\r\n", index, type, ret);
}

static void nand_cmd_app_writepart(int argc, char *argv[])
{
	SINT32 ret;
	UINT16 index = 0;
	UINT8 type = 0;
	NF_APP_PART_INFO partInfo;

	if (argc > 2)
	{
		index = nf_atoi(argv[2]);
		if (index > 15)
		{
			DBG_PRINT("part index must between 0~15. %s\r\n", argv[2]);
			return;
		}
	}
	if (argc > 3)
	{
		type = nf_atoi(argv[3]);	
		if (type > APP_KIND_MAX)
		{
			DBG_PRINT("part index must between %d~%d. %s\r\n", APP_KIND_MIN, APP_KIND_MAX, argv[3]);
			return;
		}
	}	
	
	DBG_PRINT("part to be find: index=%d type=%d\r\n", index, type);
	
	ret = NandAppFindPart(index, type, &partInfo);
	if (ret == 0)
	{
		void *buf = NULL;
		UINT32 buf_size = 32; // in sector unit, 32 mean 16K
		UINT32 imageSize = partInfo.imageSize;
		UINT16 req_size;
		UINT32 startSector = partInfo.startSector;
		
		showAppPartInfo(index, &partInfo);
		
		buf = gp_malloc_align(buf_size*512, 32);
		
		if (buf != NULL)
		{
			gp_memset(buf, 0xAA, buf_size*512);
			
			do
			{
				req_size = (imageSize < buf_size) ? imageSize : buf_size;
				DBG_PRINT("write app sector = %d\r\n", startSector);
				ret = NandBootWriteSector(startSector, req_size, (UINT32)buf);
				DBG_PRINT("write app sector done= %d, ret=%d\r\n", startSector, ret);
				if (ret != 0)
					break;
				startSector += req_size;
				imageSize -= req_size;
			} while (imageSize != 0);
			gp_free(buf);
			NandBootFlush();
		}
	}
	else
		DBG_PRINT("part not found. index=%d type=%d ret=%d\r\n", index, type, ret);
}

static void nf_demo_app_msdcon(INT8U drv)
{
	_deviceunmount(drv); // to avoid data inconsistency, unmount link with file system
	#if (defined NAND_APP_WRITE_EN) && (NAND_APP_WRITE_EN == 1)
	drv_l2_usbd_msdc_set_lun(LUN_NF_TYPE, LUN_NUM_0, USBD_STORAGE_NO_WPROTECT, &gp_msdc_nandapp0);
	#else
	drv_l2_usbd_msdc_set_lun(LUN_NF_TYPE, LUN_NUM_0, USBD_STORAGE_WPROTECT, &gp_msdc_nandapp0);
	#endif
	usbd_register_class(&usbd_msdc_ctl_blk);
	usbd_init();
}

static void nand_cmd_app_msdcon(int argc, char *argv[])
{
	INT8U drv = 255;
	
	if (argc > 2)
		drv = nf_atoi(argv[2]);
		
	if (drv != FS_NAND_APP)
	{
		DBG_PRINT("illegal driver number [%s], should be %d\r\n", argv[2], FS_NAND_APP);
		return;
	}
	nf_demo_app_msdcon(drv);
}

static void nf_demo_app_msdcoff(INT8U drv)
{
	usbd_uninit();
}

static void nand_cmd_app_msdcoff(int argc, char *argv[])
{
	INT8U drv = 255;
	
	if (argc > 2)
		drv = nf_atoi(argv[2]);
		
	if (drv != FS_NAND_APP)
	{
		DBG_PRINT("illegal driver number [%s], should be %d\r\n", argv[2], FS_NAND_APP);
		return;
	}
	nf_demo_app_msdcoff(drv);
}

static void nand_cmd_app_help(void)
{
    DBG_PRINT(
    	"\r\nUsage: nand app help\r\n"
    	"\r\n       nand app msdcon <drive>\r\n"
        "\r\n       nand app msdcoff <drive>\r\n"
    	"\r\n       nand app init\r\n"
    	"\r\n       nand app partinfo\r\n"
    	"\r\n       nand app findpart <index> <type>\r\n"
    	"\r\n       nand app readpart <index> <type>\r\n"
    	"\r\n       nand app writepart <index> <type>\r\n"
    	);
}


static void nand_cmd_app_handler(int argc, char *argv[])
{
	if (STRCMP(argv[1],"help") == 0)
    {
    	nand_cmd_app_help();
    }
    else if (STRCMP(argv[1],"init") == 0)
    {
    	nand_cmd_app_init(argc, argv);
    }
	else if (STRCMP(argv[1],"msdcon") == 0)
    {
    	nand_cmd_app_msdcon(argc, argv);
    }
    else if (STRCMP(argv[1],"msdcoff") == 0)
    {
    	nand_cmd_app_msdcoff(argc, argv);
    }    
	else if (STRCMP(argv[1],"partinfo") == 0)
    {
    	nand_cmd_app_partinfo(argc, argv);
    }    
	else if (STRCMP(argv[1],"findpart") == 0)
    {
    	nand_cmd_app_findpart(argc, argv);
    }
 	else if (STRCMP(argv[1],"readpart") == 0)
    {
    	nand_cmd_app_readpart(argc, argv);
    } 
 	else if (STRCMP(argv[1],"writepart") == 0)
    {
    	nand_cmd_app_writepart(argc, argv);
    }          
    else
    {
    	nand_cmd_app_help();
    }
}
#endif
