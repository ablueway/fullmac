#include "fsystem.h"
#include "globals.h"

/************************************************************************/
/* 进行逻辑盘的剩余空间统计
input:	逻辑盘的内部代号
output:	可用簇总数	
function:	                  
history:
			[2/7/2006] 瀚海反映当空间小于8K时，
				创建文件，创建文件夹， 写入数据时会有问题。

				经检查发现： 
					空间统计程序和 find_fat_free 函数对 0 1 号簇的解释不一致造成的。

			lanzhu @ [1/22/2006]
			测试发现在某些状况下，使用该函数会影响后续FS函数的正常运行。
			
			测试看到的现象：
				有2个逻辑盘，先进行B 盘的空间统计，然后再打开A盘的文件，不能成功!!!

			跟踪发现是由于 直接使用 firstbuf 造成的!!!

*/
/************************************************************************/
CLUSTER fs_free( INT8U disk)
{
	/* There's an unwritten rule here. All fs       */
	/* cluster start at 2 and run to max_cluster+2  */
	REG CLUSTER i, k, Actual_FAT_size;
	REG CLUSTER cnt = 0;
	struct dpb *dpbp = CDS[disk].cdsDpb; 
	INT8U idx = 0, last = 0;
	INT16U TheLastCluster;
	
	#if FS_FORMAT_SPEEDUP == 1	
		INT32U *p_format_temp_buffer;
		INT32U sector_num; 
	#endif
	
//	CLUSTER max_cluster = dpbp->dpb_size+1; 		
	CLUSTER max_cluster = dpbp->dpb_size+2; 		// 2008.6.23,zurong modify for free space
 
#if WITHFAT32 == 1 || WITHEXFAT == 1
	if (ISFAT32(dpbp)||ISEXFAT(dpbp))
	{
		if (dpbp->dpb_xnfreeclst != XUNKNCLSTFREE)
		{
			return dpbp->dpb_xnfreeclst;
		}			 
	}
	else
#endif
	if (dpbp->dpb_nfreeclst != UNKNCLSTFREE)
	{
		return dpbp->dpb_nfreeclst;		
	}

	flush1 (firstbuf);

	// lanzhu add @ [1/22/2006] 将该缓冲标记为无效!!!	
	firstbuf->b_flag &= FALSE;
	 
	clear_buffers(disk); /* by jk 2005/02/15 */
	if (ISFAT32 (dpbp)) 	 
	{
		max_cluster = dpbp->dpb_xsize +2;
		Actual_FAT_size = (dpbp->dpb_xsize+129) / 128;			
	}
	else if(ISEXFAT (dpbp))
	{
		max_cluster = dpbp->dpb_xsize;
		#if WITHEXFAT == 1
	    Actual_FAT_size = (dpbp->dpb_exmlength+511)/512;
	    #endif
	} 
	else if (ISFAT16 (dpbp)) 
	{
		Actual_FAT_size = (dpbp->dpb_size+257)/256;		 		 			
	}
	else
	{
		Actual_FAT_size = ((dpbp->dpb_size*3 + 1)/2 + 511) / 512;
	}
	
#if FS_FORMAT_SPEEDUP == 1
	if(ISFAT12(dpbp))
	{
		p_format_temp_buffer = 0;
	}
	else
	{
		#if FS_FORMAT_SPEEDUP_BUFFER_SOURCE == ALLOCATE_BUFFER
		p_format_temp_buffer = (INT32U*)gp_malloc_align(C_FS_FORMAT_BUFFER_SIZE,4);
		#elif FS_FORMAT_SPEEDUP_BUFFER_SOURCE == GLOBAL_BUFFER
		p_format_temp_buffer =(INT32U*)gFS_format_speedup_buffer;
		#else
		p_format_temp_buffer = 0;
		#endif
	}
	if(p_format_temp_buffer)
	{
//		gp_memset((INT8S*)p_format_temp_buffer, 0, C_FS_FORMAT_BUFFER_SIZE>>WORD_PACK_VALUE);		
		
		for(i=0;i < Actual_FAT_size; )
		{
		
			if((i+C_FS_FORMAT_BUFFER_SECTOR_SIZE)>Actual_FAT_size)
			{				
				sector_num = Actual_FAT_size - i;					
			}
			else
			{				
				sector_num = C_FS_FORMAT_BUFFER_SECTOR_SIZE;
			}	
			
			if (ISEXFAT (dpbp))
			{
			    #if WITHEXFAT == 1
			    if (dskxfer (dpbp ->dpb_unit,dpbp ->dpb_exmtable + i,(INT32U)p_format_temp_buffer, sector_num, DSKREAD)) 
    			{
    				#if FS_FORMAT_SPEEDUP_BUFFER_SOURCE == ALLOCATE_BUFFER
    					gp_free(p_format_temp_buffer);
    				#endif
    				gFS_errno = DE_INVLDDATA;			 
    				return 0;
    			}
    			#endif
			}
			else
			{
			    if (dskxfer (dpbp ->dpb_unit,dpbp ->dpb_fatstrt + i,(INT32U)p_format_temp_buffer, sector_num, DSKREAD)) 
    			{
    				#if FS_FORMAT_SPEEDUP_BUFFER_SOURCE == ALLOCATE_BUFFER
    					gp_free(p_format_temp_buffer);
    				#endif
    				gFS_errno = DE_INVLDDATA;			 
    				return 0;
    			}
			}
			
			
			i = i + sector_num;
			
			if (ISFAT32 (dpbp))
			{
				// FAT32 FAT free space balance
				INT32U *p;
				p = ( INT32U *)p_format_temp_buffer;
				
				if (i==Actual_FAT_size) 	 //20051027 modify by zhangyan
				{
					INT16U TheLastCluster;
					
					TheLastCluster = max_cluster%128;
					if (!TheLastCluster) 
					{
						//TheLastCluster = 127;	
						TheLastCluster = 128;	// 2008.6.25,zurong modify for free space
					}
					
					TheLastCluster = (sector_num -1)*128 + TheLastCluster;

					//for(k = 0; k<=TheLastCluster; k ++) 	
					for(k = 0; k<TheLastCluster; k ++) 	// 2008.6.25,zurong modify for free space
					{
						if (*p == 0) 
						{ 
							cnt ++;
						}
						p ++;
					}
				}
				else
				{				
					for (k = 0; k < sector_num*128; k ++) 	 
					{
						if (*p == 0)
						 { 
						 	cnt ++; 
						 }
						p ++;
					}
				}
			}
			else if(ISEXFAT (dpbp))
			{

				// FAT32 FAT free space balance
				INT32U *p;
				p = ( INT32U *)p_format_temp_buffer;
				
				if (i==Actual_FAT_size) 	 //20051027 modify by zhangyan
				{
					INT32U TheLastCluster=0;
					INT16U ThelastBit=0;

					TheLastCluster = max_cluster%4096;//512*8
					if (TheLastCluster) 
					{
						//TheLastCluster = 128;	// 2008.6.25,zurong modify for free space
						ThelastBit=TheLastCluster%32;
						TheLastCluster=TheLastCluster/32;
						
					}					
					TheLastCluster = (sector_num -1)*128 + TheLastCluster;
					//for(k = 0; k<=TheLastCluster; k ++) 	
					for(k = 0; k<TheLastCluster; k ++) 	// 2008.6.25,zurong modify for free space
					{
						if (*p == 0) 
						{ 
							cnt +=32;
						}
						else
						{
						   INT8U n;
						   INT32U m=1;
						   for(n=0;n<32;n++)
						   {
						        if(((*p)&m)==0)
						        {
						            cnt++;						           
    					        }
								m=m<<1;
						   }					        
						}
						p ++;
					}
					if(ThelastBit)
					{
						INT32U m=1;
						for(k=0;k<ThelastBit;k++)
						{
							 if(((*p)&m)==0)
					        {
					            cnt++;					           
					        }
							m=m<<1;
						}
					}
				}
				else
				{		
					for (k = 0; k < sector_num*128; k ++) 	 
					{
						if (*p == 0)
						{ 
						 	cnt +=32; 
						}
						else
						{
						   INT8U n;
						   INT32U m=1;
						   for(n=0;n<32;n++)
						   {
						        if(((*p)&m)==0)
						        {
						            cnt++;						           
    					        }
								m=m<<1;
						   }					        
						}
						p ++;
					}
				}   
			}
			else if (ISFAT16(dpbp)) 	 
			{
				// FAT16 FAT free space balance
				INT16U *p;
				p = (INT16U *)p_format_temp_buffer;
				
				if (i==Actual_FAT_size) 	 //20051027 modify by zhangyan
				{
					INT16U TheLastCluster;
					
					TheLastCluster = max_cluster%256;
					if (!TheLastCluster) 
					{
						//TheLastCluster = 255;
						TheLastCluster = 256;// 2008.6.25,zurong modify for free space
					}
					
					TheLastCluster = (sector_num -1)*256 + TheLastCluster;

					//for (k = 0; k<=TheLastCluster; k ++)
					for (k = 0; k<TheLastCluster; k ++) // 2008.6.25,zurong modify for free space
					{
						if (*p == 0)
						{ 
							cnt ++; 
						}
						p ++;
					}
				}
				else
				{
					for (k = 0; k < sector_num*256; k ++) 	 
					{
						if (*p == 0)
						{ 
							cnt ++;
						}
						p ++;
					}
				}
			}
			else if (ISFAT12(dpbp)) 	 
			{ 	 //bugness for unSP
				// FAT12 FAT free space balance 	
				INT8U *p, idx = 0, last = 0;
				p = ( INT8U *)p_format_temp_buffer;
				for (k = 0; k < sector_num*512; k ++)
				{
					switch (idx) 	 
					{
						case 0:
							last = *p;
							idx = 1;
							break;
						case 1:
							if (last == 0 && (*p & 0xf0) == 0)
								cnt ++;
							last = *p & 0xf;
							idx = 2;
							break;
						case 2:
							if (last == 0 && *p == 0)
								cnt ++;
							last = 0;
							idx = 0;
							break;
					}
					p ++;
				}
			}
			else
			{
				#if FS_FORMAT_SPEEDUP_BUFFER_SOURCE == ALLOCATE_BUFFER
					gp_free(p_format_temp_buffer);
				#endif			
				return cnt; 	 // invalid format
			}			
					
		}	
	}
	else
#endif	
	{	
		// read FAT sectors one by one and check them
		for (i = 0; i < Actual_FAT_size; i ++) 	 
		{
			if (ISEXFAT (dpbp))
			{
			    #if WITHEXFAT == 1
			    if (dskxfer(dpbp ->dpb_unit, dpbp ->dpb_exmtable + i, (INT32U)firstbuf ->b_buffer, 1, DSKREAD)) 
				{
					gFS_errno = DE_INVLDDATA;			 
					return 0;
				}
				#endif
			}
			else
			{
			    if (dskxfer(dpbp ->dpb_unit, dpbp ->dpb_fatstrt + i, (INT32U)firstbuf ->b_buffer, 1, DSKREAD)) 
				{
					gFS_errno = DE_INVLDDATA;			 
					return 0;
				}
			}

			if (ISFAT32 (dpbp)) 	 
			{
				// FAT32 FAT free space balance
				INT32U *p;
				p = ( INT32U *)(firstbuf ->b_buffer);
				
				if (i==(Actual_FAT_size-1)) 	 //20051027 modify by zhangyan
				{
					TheLastCluster = max_cluster%128;
					if (!TheLastCluster) {
						//TheLastCluster = 127;	
						TheLastCluster = 128;	// 2008.6.25,zurong modify for free space
					}

					//for(k = 0; k<=TheLastCluster; k ++) 	
					for(k = 0; k<TheLastCluster; k ++) 	// 2008.6.25,zurong modify for free space
					{
						if (*p == 0) { cnt ++; }
						p ++;
					}
				}
				else
				{
					for (k = 0; k < 128; k ++) 	 
					{
						if (*p == 0) 	 { cnt ++; }
						p ++;
					}
				}
			}
			else if(ISEXFAT (dpbp))
			{
				// FAT32 FAT free space balance
				INT32U *p; 
				p = (INT32U *)(firstbuf ->b_buffer);
				if (i==(Actual_FAT_size-1)) 	 //20051027 modify by zhangyan
				{
					INT32U TheLastCluster=0;
					INT16U ThelastBit=0;	
					
					TheLastCluster = max_cluster%4096;
					if (TheLastCluster) {
						ThelastBit=TheLastCluster%32;					
						TheLastCluster = TheLastCluster/32;	// 2008.6.25,zurong modify for free space
					}
					//for(k = 0; k<=TheLastCluster; k ++)	
					for(k = 0; k<TheLastCluster; k ++)	// 2008.6.25,zurong modify for free space
					{
						if (*p == 0)
						{ 
							cnt +=32; 
						}
						else
						{
						   INT8U n;
						   INT32U m=1;
						   for(n=0;n<32;n++)
						   {
								if(((*p)&m)==0)
								{
									cnt++;								   
								}
								m=m<<1;
						   }							
						}
						p ++;
					}					
					if(ThelastBit)
					{
						INT32U m=1;
						for(k=0;k<ThelastBit;k++)
						{
							 if(((*p)&m)==0)
					        {
					            cnt++;					           
					        }
							m=m<<1;
						}
					}				
				}
				else
				{
					for (k = 0; k < 128; k ++)	 
					{
						if (*p == 0)
						{ 
							cnt +=32; 
						}
						else
						{
						   INT8U n;
						   INT32U m=1;
						   for(n=0;n<32;n++)
						   {
								if(((*p)&m)==0)
								{
									cnt++;								   
								}
								m=m<<1;
						   }							
						}
						p ++;
					}
				}				
			}
				
			
			else if (ISFAT16(dpbp)) 	 {
				// FAT16 FAT free space balance
				INT16U *p;
				p = (INT16U *)(firstbuf ->b_buffer);
				
				if (i==(Actual_FAT_size-1)) 	 //20051027 modify by zhangyan
				{				
					TheLastCluster = max_cluster%256;
					if (!TheLastCluster) 
					{
						//TheLastCluster = 255;
						TheLastCluster = 256;// 2008.6.25,zurong modify for free space
					}

					//for (k = 0; k<=TheLastCluster; k ++)
					for (k = 0; k<TheLastCluster; k ++) // 2008.6.25,zurong modify for free space
					{
						if (*p == 0){ cnt ++; }
						p ++;
					}
				}
				else
				{
					for (k = 0; k < 256; k ++) 	 
					{
						if (*p == 0){ cnt ++; }
						p ++;
					}
				}
			}
			else if (ISFAT12(dpbp)) 	 {
				// FAT12 FAT free space balance 	
				INT8U *p;
				
				p = (INT8U *)(firstbuf ->b_buffer);
				if (i==(Actual_FAT_size-1))
				{
					TheLastCluster = max_cluster * 3;
					TheLastCluster = (TheLastCluster >> 1) + (TheLastCluster & 1);
					TheLastCluster %= 512;
					if (!TheLastCluster)
					{
						TheLastCluster = 512;
					}
				}
				else
				{
					TheLastCluster = 512;
				}
				
				for (k = 0; k < TheLastCluster; k ++) 	 {
					switch (idx) 	 {
					case 0:
						last = *p;
						idx = 1;
						break;
					case 1:
						if (last == 0 && (*p & 0x0f) == 0)
							cnt ++;
						last = *p & 0xf0;
						idx = 2;
						break;
					case 2:
						if (last == 0 && *p == 0)
							cnt ++;
						last = 0;
						idx = 0;
						break;
					}
					p ++;
				}
			}
			else 	 {
				return cnt; 	 // invalid format
			}
		}
	}// if .else.

#if WITHFAT32 == 1 || WITHEXFAT == 1
	if (ISFAT32(dpbp)||ISEXFAT(dpbp))
	{
		dpbp->dpb_xnfreeclst = cnt;
		dpbp->dpb_fsinfoflag |= FSINFODIRTYFLAG;  //add by matao
		#if WITHEXFAT == 1
		if(ISEXFAT(dpbp))
		{
			dpbp->dpb_fsinfoflag=0;
		}
		#endif
	#ifndef READ_ONLY	// zurong TBD	
		write_fsinfo(dpbp);
	#endif	
	#if FS_FORMAT_SPEEDUP_BUFFER_SOURCE == ALLOCATE_BUFFER
			gp_free(p_format_temp_buffer);
	#endif
		return cnt;
	}
#endif
	dpbp->dpb_nfreeclst = cnt;
	#if FS_FORMAT_SPEEDUP_BUFFER_SOURCE == ALLOCATE_BUFFER
		gp_free(p_format_temp_buffer);
	#endif	
	return cnt;
}
