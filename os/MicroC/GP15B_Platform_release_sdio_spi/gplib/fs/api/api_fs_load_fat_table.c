#include "fsystem.h"
#include "globals.h"
 
#if _SEEK_SPEED_UP == 1

#if _CACHE_FAT_INFLEXION == 1

#if CHEZAI_MP3 == 1
	#define C_FAT_INFLEXION_BUFFER_SIZE	1024
#else
	#define C_FAT_INFLEXION_BUFFER_SIZE		512
#endif
#define C_FatTempBufferSize				256


INT16S fs_FastLoadFatTable(REG f_node_ptr fnp, INT16U *pBuffer);
#ifndef READ_ONLY
INT16S fs_LoadFatTable(REG f_node_ptr fnp);

INT16S LoadFatTable(INT16S fd)
{
	INT16S ret;
	REG f_node_ptr fnp;

	FS_OS_LOCK();
		
	fnp = xlt_fd(fd);
	if (fnp == (f_node_ptr) 0) {
		return DE_INVLDHNDL;
	}
	
	ret = fs_LoadFatTable(fnp);  
	
	FS_OS_UNLOCK();
	
	if (ret < 0) {
		handle_error_code(ret);
		return -1;
	} else {
		return ret;
	}
}
#endif

INT16S fs_LoadFatTable(REG f_node_ptr fnp)
{
#if 0
	CLUSTER	i;
	INT32U	total_cluster;
	INT16U	cluster_size;
#endif
	INT16S	ret;
	CHAR	*ptemp;
	
	if (!fnp->f_dpb->dpb_mounted || !fnp->f_count) {
		return DE_INVLDHNDL;		
	}

	// reset file link attribute
	fnp->f_link_attribute = 0x00;
	
	if ((O_RDONLY != (fnp->f_mode & O_ACCMODE))&&(!ISEXFAT(fnp->f_dpb))) {
		return -1;
	}

#if 0		// 2008.10.10 marked for not preciseness judgement
	// get cluster size
	cluster_size = fnp->f_dpb->dpb_secsize;
	cluster_size <<= fnp->f_dpb->dpb_shftcnt;	

	// get total cluster
	total_cluster = (fnp->f_dir.dir_size + cluster_size - 1) / cluster_size;

	// judge continue && compact link
	i = fnp ->f_cluster + total_cluster -1;

	total_cluster++;	// 考虑到有一个结束标志，多加一个

	//判断fat是否是连续的
	if ( (total_cluster > 512)
		&&(next_cluster(fnp->f_dpb, i-1) == i) 
		&& (next_cluster(fnp->f_dpb, i) == LONG_LAST_CLUSTER) ) 
	{
		fnp->f_start_cluster = fnp->f_cluster;
		fnp->f_link_attribute = FLINK_ATTR_LINER;
		return SUCCESS; 
	}
#endif

	ptemp = (CHAR *) FS_SeekSpeedup_Malloc(C_FAT_INFLEXION_BUFFER_SIZE);
	if (ptemp == 0) {
		return -1;
	}

	fnp->plink_key_point = (INT32U) ptemp;
	
	//保存拐点信息
	ret = fs_FastLoadFatTable(fnp, (INT16U *) ptemp);	// just for file O_RDONLY		 
	if (ret == 0) 
	{
		fnp->f_link_attribute = FLINK_ATTR_INFLEXION_CACHE;
		return SUCCESS; 
	}
	else if(ret == 1)
	{
		fnp->f_start_cluster = fnp->f_cluster;
		fnp->f_link_attribute = FLINK_ATTR_LINER;
		ret = 0;
	}
	FS_SeekSpeedup_Free((INT32U)ptemp);
	fnp->plink_key_point = 0;
	return ret;
}

//快速加载fat表
// if return -1, fail
// if return 0, file is store not in series
// if return 1, file is store in series
INT16S fs_FastLoadFatTable(REG f_node_ptr fnp, INT16U *buffer)
{
	INT16U	i, j;
	INT16U	idx;
	INT32U	total_cls = 1;
	INT16U	max_inflexion;
	INT8U 	*bp;
	ST_INFLEXION_HEADER	*p_header;
	
	if (!flush1(firstbuf)) {
		return -1;
	}
	
	firstbuf->b_flag &= ~BFR_VALID;
	
	bp = firstbuf->b_buffer;
	
	memset(buffer, 0, C_FAT_INFLEXION_BUFFER_SIZE * sizeof(INT16U));
	p_header = (ST_INFLEXION_HEADER*)buffer;

//先做标志判断，簇链接表是否有效
#if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
		INT64U clucount=0;				
		CLUSTER cluster;					//当前的cluster id
		struct ST_FAT32_INFLEXION *pBuffer = (struct ST_FAT32_INFLEXION *) (p_header + 1);
		
		cluster = fnp->f_cluster;			//文件的起始cluster地址
		if (cluster == 0)
			return -1;
		
		//保留3个word用于保存item num和total_cluster
		max_inflexion = (C_FAT_INFLEXION_BUFFER_SIZE * 2 - sizeof(ST_INFLEXION_HEADER)) / sizeof(struct ST_FAT32_INFLEXION);

		if(NOFatChain(fnp->ext_dir->SecondaryFlags))
		{
			clucount=fnp->f_dpb->dpb_secsize;
			clucount=(clucount<<(fnp->f_dpb->dpb_shftcnt))-1;
			clucount=((fnp->ext_dir->DataLength)+clucount)/(fnp->f_dpb->dpb_secsize);
			clucount=clucount>>(fnp->f_dpb->dpb_shftcnt);
			pBuffer[0].cls_id = cluster;
			pBuffer[0].length = (INT32U)clucount;

			p_header->total_cluster = (INT32U)clucount;		//保存总共有多少个cls
			p_header->inflexion_count = 1;				//保存item数

			return 1;
		}
	}
#endif

	
	if (ISFAT16(fnp->f_dpb)) {
		INT16U	length = 1;					//各个拐点间的距离
		INT16U	clussec, cur_clussec;		//cluster id对应在fat表里面的sector地址
		INT16U	cluster;					//当前的cluster id
		struct ST_FAT16_INFLEXION *pBuffer = (struct ST_FAT16_INFLEXION *)(p_header + 1);//(struct ST_FAT16_INFLEXION *) (buffer + 3);
		
		cluster = (INT16U)fnp->f_cluster;			//文件的起始cluster地址
		if (cluster == 0)
			return -1;
		
		//保留3个word用于保存item num和total_cluster
		max_inflexion = (C_FAT_INFLEXION_BUFFER_SIZE * 2 - sizeof(ST_INFLEXION_HEADER)) / sizeof(struct ST_FAT16_INFLEXION);
		
		j = 0;
		pBuffer[j].cls_id = cluster;
		
		while (1)
		{
			//该cluster在fat表里面对应的sector地址和偏移量
			idx = cluster & 0xff;
			clussec = cluster >> 8;
			
			//读4个sector的fat表
			//if (dskxfer(fnp->f_dpb->dpb_unit, clussec + fnp->f_dpb->dpb_fatstrt, gFatTempBuffer, C_FatTempBufferSize / 256, DSKREAD)) 
			if (dskxfer(fnp->f_dpb->dpb_unit, clussec + fnp->f_dpb->dpb_fatstrt, (INT32U) bp, C_FatTempBufferSize / 256, DSKREAD)) 
			{
				gFS_errno = DE_INVLDDATA;
				return -1; 
			}	
			
			for (i = idx; i < C_FatTempBufferSize; i++)
			{
				INT16U	res = getword(&bp[i << (1 - WORD_PACK_VALUE)]);
				
				if (cluster + 1 == res)
				{
					//fat表是连续的
					length++;
					total_cls++;
					cluster = res;
				}
				else 
				{
					//fat表不连续
					if (res >= MASK16)
					{
						//到文件结束位置了
						pBuffer[j++].length = length;
						p_header->total_cluster = total_cls;		//保存总共有多少个cls
						p_header->inflexion_count = j;				//保存item数
						if(j == 1 && length == total_cls)
						{
							return 1;
						}
						return 0;
					}
					else
					{
						//if(j == max_inflexion)
						if(j == (max_inflexion-1))		// zurong fix buffer slop over bug. 2008.6.24
						{
							//buffer已满
							pBuffer[j++].length = length;
							p_header->total_cluster = total_cls;	//保存总共有多少个cls
							p_header->inflexion_count = j;			//保存item数
							return 0;
						}
						pBuffer[j++].length = length;
						pBuffer[j].cls_id = res;
						length = 1;
						total_cls++;
						
						cluster = res;
						cur_clussec = cluster >> 8;
						if (( cur_clussec > clussec + C_FatTempBufferSize / 256 - 1) || (cur_clussec < clussec) )
						{
							break;
						}
						else
						{
							i = (cluster - (clussec << 8)) & (C_FatTempBufferSize - 1);
							i--;
						}
					}
				}
			}
		}
	}
#if WITHFAT32 == 1 || WITHEXFAT == 1
	else if (ISFAT32(fnp->f_dpb)||ISEXFAT(fnp->f_dpb))
	{
		INT32U	length = 1;					//各个拐点间的距离
		CLUSTER	clussec, cur_clussec;		//cluster id对应在fat表里面的sector地址
		CLUSTER	cluster;					//当前的cluster id
		struct ST_FAT32_INFLEXION *pBuffer = (struct ST_FAT32_INFLEXION *) (p_header + 1);
		
		cluster = fnp->f_cluster;			//文件的起始cluster地址
		if (cluster == 0)
			return -1;
		
		//保留3个word用于保存item num和total_cluster
		max_inflexion = (C_FAT_INFLEXION_BUFFER_SIZE * 2 - sizeof(ST_INFLEXION_HEADER)) / sizeof(struct ST_FAT32_INFLEXION);
		
		j = 0;
		pBuffer[j].cls_id = cluster;
		
		while (1)
		{
			//该cluster在fat表里面对应的sector地址和偏移量
			idx = cluster & 0x7f;
			clussec = cluster >> 7;
			
			//读4个sector的fat表
			//if (dskxfer(fnp->f_dpb->dpb_unit, clussec + fnp->f_dpb->dpb_fatstrt, gFatTempBuffer, C_FatTempBufferSize / 256, DSKREAD)) 
			if (dskxfer(fnp->f_dpb->dpb_unit, clussec + fnp->f_dpb->dpb_fatstrt, (INT32U) bp, C_FatTempBufferSize / 256, DSKREAD)) 
			{
				gFS_errno = DE_INVLDDATA;
				return -1; 
			}	
			
			for (i = idx; i < C_FatTempBufferSize / 2; i++)
			{
				CLUSTER res;
				
				//res = getlong(&gFatTempBuffer[i << (2 - WORD_PACK_VALUE)]);
				res = getlong(&bp[i << (2 - WORD_PACK_VALUE)]);
				if (cluster + 1 == res)
				{
					//fat表是连续的
					cluster = res;
					
				#if 0
					if (length == 0xffff)
					{
						pBuffer[j++].length = length;
						if (j == max_inflexion)
						{
							//buffer已满
							p_header->total_cluster = total_cls;	//保存总共有多少个cls
							p_header->inflexion_count = j;			//保存item数
							return 0;
						}
						pBuffer[j].cls_id = res;					//下一个cluster的起始地址
						length = 1;
						total_cls++;
					}
					else
					{
				#endif
					length++;
					total_cls++;
				//	}
				}
				else
				{
					//fat表不连续
					if (res >= LONG_BAD)
					{
						//到文件结束位置了
						pBuffer[j++].length = length;
						p_header->total_cluster = total_cls;		//保存总共有多少个cls
						p_header->inflexion_count = j;				//保存item数
						if(j == 1 && length == total_cls)
						{
							return 1;
						}
						return 0;
					}
					else
					{
						//if(j == max_inflexion)
						if(j == (max_inflexion-1))		// zurong fix buffer slop over bug. 2008.6.24
						{
							//buffer已满
							pBuffer[j++].length = length;
							p_header->total_cluster = total_cls;	//保存总共有多少个cls
							p_header->inflexion_count = j;			//保存item数
							return 0;
						}
						pBuffer[j++].length = length;
						pBuffer[j].cls_id = res;					//下一个cluster的起始地址
						length = 1;
						total_cls++;
						
						cluster = res;
						cur_clussec = cluster >> 7;
						//if (( cur_clussec > clussec + 3) || (cur_clussec < clussec) )
						if (( cur_clussec > clussec + C_FatTempBufferSize / 256 - 1) || (cur_clussec < clussec) )	// 512
						{
							break;
						}
						else
						{
							i = (cluster - (clussec << 7)) & (C_FatTempBufferSize / 2 - 1);
							i--;
						}
					}
				}
			}
		}
	}
#endif	




	return -1;
}

INT32U fast_GetTotalSpeedupCluster(REG f_node_ptr fnp)
{
	ST_INFLEXION_HEADER *buffer = (ST_INFLEXION_HEADER*)fnp->plink_key_point;
	
	//return getlong(buffer);
	return buffer->total_cluster;
}

//计算从当前 cluster 开始的连续的 cluster 数量
INT32U fast_GetNextInflexion(REG f_node_ptr fnp, CLUSTER cluster)
{
	CLUSTER total_cls;
	ST_INFLEXION_HEADER *p_header = (ST_INFLEXION_HEADER*)fnp->plink_key_point;
	INT16U	i;
	INT16U	nItem;
	
	total_cls = p_header->total_cluster;
	nItem = p_header->inflexion_count;

	if (cluster >= total_cls) {
		return 0;
	}
					
	if (ISFAT16(fnp->f_dpb)) {
		INT16U	cur_cls = 0;
		struct ST_FAT16_INFLEXION *pBuffer = (struct ST_FAT16_INFLEXION *) (p_header + 1);
					
		for (i=0; i<nItem; i++) {
			cur_cls += pBuffer[i].length;
			if (cluster < cur_cls) {
				return cur_cls - cluster;
			}
		}
	} else if (ISFAT32(fnp->f_dpb)) {
		CLUSTER	cur_cls = 0;
		struct ST_FAT32_INFLEXION *pBuffer = (struct ST_FAT32_INFLEXION *) (p_header + 1);
					
		for (i=0; i<nItem; i++) {
			cur_cls += pBuffer[i].length;
			if (cluster < cur_cls) {
				return cur_cls - cluster;
			}
		}
	}
	return 0;
}
#endif

#endif
