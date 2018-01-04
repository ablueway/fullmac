#include "fsystem.h"
#include "globals.h"

/************************************************************************/
/*                                                                      */
/************************************************************************/
void fs_init(void)
{
	INT16S i;

	FS_OS_Init();
	
	FS_OS_LOCK();						//2006.4.25 防止fs_init还没有结束，就因为OS调度，有地方调用read或write之类的fs 函数

	gFS_errno = 0x00;				// reset FS error number

	f_nodes = f_nodes_array;
	for (i=0; i<MAX_FILE_NUM; i++)
	{
		memset(&f_nodes_array[i], 0x00, sizeof(f_nodes_array[0]));
	}

	default_drive = 0;

	{
		INT16S i;
		firstbuf = &buffers[0];
		
		for (i = 0; i < NUMBUFF; i++)
		{
			buffers[i].b_flag = FALSE;
			if (i == (NUMBUFF-1)){
				buffers[i].b_next = buffers;
			}
			else{ 	
				buffers[i].b_next = buffers+i+1;
			}
			if (i > 0){
				buffers[i].b_prev = buffers+i-1;
			}
			else{
				buffers[0].b_prev = buffers+NUMBUFF-1;
			}
		}
	}

	{
		struct dpb *dpb = &DPB[0];
		
		/* Initialize the current directory structures    */
		for (i = 0; i < MAX_DISK_NUM; i++)
		{
			struct cds *pcds_table = &CDS[i];
			
		#if WITH_CDS_PATHSTR == 1
			fmemcpy(pcds_table->cdsCurrentPath, "A:\\\0", 4);  
			pcds_table->cdsCurrentPath[0] += i;    
		#endif
			pcds_table->cdsDpb = &dpb[i]; 
			pcds_table->cdsDpb->dpb_unit = i;
			pcds_table->cdsFlags = CDSPHYSDRV;
		#if WITHFAT32 == 1
			pcds_table->cdsDpb->dpb_fsinfoflag = 0;
		#endif
			pcds_table->cdsStrtClst = 0; 
		}
	}

	for (i=0;i < MAX_DISK_NUM;i++)
	{
		if (FileSysDrv[i] == NULL)
			continue;
		
		DPB[i].dpb_mounted = 0;
		FileSysDrv[i]->Status = 0;
	}

#if FIND_EXT == 1
// 2008.05.28 marked
//	sfn_changedisk(pstDiskFindControl, 0);
#endif
	
	ChangeUnitab((struct nls_table *) &nls_ascii_table);
	gUnicodePage = UNI_ENGLISH;

  #if 0		// Tristan: it is not a good idea to call this function
	FS_LoadDriver();
  #endif
	
	FS_OS_UNLOCK();						//2006.4.25 防止fs_init还没有结束，就因为OS调度，有地方调用read或write之类的fs 函数
}
