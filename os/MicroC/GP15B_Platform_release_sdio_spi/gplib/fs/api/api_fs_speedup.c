/************************************************************************/
//	
//	for unsp FS speed up
//	lanzhu @ zhuan'an
//                                                                     
/************************************************************************/
#include "fsystem.h"
#include "globals.h"

/************************************************************************/
/* internal function                                                    */
/************************************************************************/
#if _SEEK_SPEED_UP == 1
#if _CACHE_FAT_INFLEXION == 0
static void cache_all_cluster(f_node_ptr fnp, INT32U total_cluster);
static void cache_key_cluster(f_node_ptr fnp, INT32U total_cluster);

static INT32U unsp_fs_get_index_1(INT16U len);
static INT32U unsp_fs_get_index_2(INT16U divide_value);

static CLUSTER fat16_get_key_cluster(f_node_ptr fnp, CLUSTER relative_clus);
static CLUSTER fat32_get_key_cluster(f_node_ptr fnp, CLUSTER relative_clus);
#endif
/************************************************************************/
/*
	fast_next_cluster
	input:
			f_node_ptr	filenp
			INT16U		relative_clus
			
	output:	cluster number
	note:	NOT FOR FAT32
																		*/
/************************************************************************/
CLUSTER fast_next_cluster(f_node_ptr fnp, CLUSTER relative_clus)
{
  #if _CACHE_FAT_INFLEXION == 1
	// liner link 
	if (fnp->f_link_attribute & FLINK_ATTR_LINER) {
		return (CLUSTER)(fnp->f_start_cluster + relative_clus);
	} else if (fnp->f_link_attribute & FLINK_ATTR_INFLEXION_CACHE) {
		CLUSTER relcluster, total_cls;
		ST_INFLEXION_HEADER *p_header = (ST_INFLEXION_HEADER*)fnp->plink_key_point;
		INT16U	i;
		INT16U	nItem;
		CLUSTER	cur_cls = 0;
		
		total_cls = p_header->total_cluster;
		nItem = p_header->inflexion_count;
			
		relcluster = (CLUSTER) ((fnp->f_offset / fnp->f_dpb->dpb_secsize) >> fnp->f_dpb->dpb_shftcnt);
					
		if (relcluster >= total_cls) {
			return 1;
		}
				
		if (ISFAT16(fnp->f_dpb)) {
			struct ST_FAT16_INFLEXION *pBuffer = (struct ST_FAT16_INFLEXION *)(p_header + 1);
						
			for (i=0; i<nItem; i++) {
				cur_cls += pBuffer[i].length;
				if (relcluster < cur_cls) {
					cur_cls -= pBuffer[i].length;
					return pBuffer[i].cls_id + relcluster - cur_cls;
				}
			}
		} else if (ISFAT32(fnp->f_dpb)||ISEXFAT(fnp->f_dpb)) {
			struct ST_FAT32_INFLEXION *pBuffer = (struct ST_FAT32_INFLEXION *)(p_header + 1);
						
			for (i=0; i<nItem; i++) {
				cur_cls += pBuffer[i].length;
				if (relcluster < cur_cls) {
					cur_cls -= pBuffer[i].length;
					return pBuffer[i].cls_id + relcluster - cur_cls;
				}
			}
		}
	}
	return 1;
  #else	
	// liner link 
	if (fnp->f_link_attribute & FLINK_ATTR_LINER) {
		return (CLUSTER) (fnp->f_start_cluster + relative_clus);
	}

	// cache all 
	if (fnp->f_link_attribute & FLINK_ATTR_ALL_CACHE) {
		if (ISFAT32(fnp->f_dpb)) {
			return (CLUSTER) fs_read_dword(fnp->plink_key_point + (relative_clus<<1));
		} else {
			return (CLUSTER) fs_read_word(fnp->plink_key_point + relative_clus); 
		}
	}
	
	// cache key cluster
	if (ISFAT32(fnp->f_dpb)) {
		return (CLUSTER) fat32_get_key_cluster(fnp, relative_clus);
	} else {
		return (CLUSTER) fat16_get_key_cluster(fnp, relative_clus);
	}
  #endif
}

#if _CACHE_FAT_INFLEXION == 0
static CLUSTER fat16_get_key_cluster(f_node_ptr fnp, CLUSTER relative_clus)
{
	INT16U  i, j, k;
	CLUSTER key_cluster, ret_cls;
	INT16U  rel_start, rel_offset;
	INT16U  divide_value, divide_value2 = 0;
	INT16U	key_cluster_length;	
	INT16U  block_start = 0, block_offset = 0;
	INT32U	ptemp = fnp->plink_key_point;

	divide_value = fnp->link_divide_value;
	rel_start = relative_clus/divide_value;
	rel_offset = relative_clus%divide_value;
	
	//fnp->f_start_cluster保存fnp->plink_key_point buffer实际使用的大小
	if (rel_start+1 < fnp->f_start_cluster) {
		key_cluster = fs_read_word(ptemp+rel_start+1);
	} else if (rel_start == fnp->f_start_cluster-1) {
		key_cluster = fs_read_word(ptemp+rel_start);		//最后一个cls还没有进入cache
	} else {
		return 1;			//超出范围
	}
	
	// the cluster is not in key cache
	if (key_cluster == 0) {
		for (i=1; fs_read_word(ptemp+i); i++) ;		//第0个一定有值
		key_cluster = fs_read_word(ptemp+i-1);
		
		j = (rel_start - i + 2) * divide_value;
		for (k=1; k<=j; k++) {
			key_cluster = next_cluster(fnp->f_dpb, key_cluster);	 // the next
			if (key_cluster == LONG_LAST_CLUSTER) {
				break;
			}
			if (k % divide_value) {				
				continue;	// not store the cluster not in key poistion						
			}
			fs_write_word(ptemp+i, key_cluster);
			i++;
		}
	}

	// just at key point, just return 
	if (!rel_offset) {
		return (CLUSTER) (fs_read_word(ptemp+rel_start));
	}

	ptemp = fnp->plink_cache_buf;	
	// in case of memory alloc failure
	if (ptemp == 0) {
		key_cluster = fs_read_word(fnp->plink_key_point + rel_start);

		for (i=0x00; i<rel_offset; i++) {
			key_cluster = next_cluster(fnp->f_dpb, key_cluster);
		}

		return (CLUSTER) key_cluster;
	}
	
	key_cluster_length = KEY_CLUSTER_LENGTH >> 1;
	if (divide_value > key_cluster_length) {
		divide_value2 = (divide_value+key_cluster_length-1)/key_cluster_length;
		block_start = rel_offset/divide_value2;
		block_offset = rel_offset%divide_value2;
	}
	
	key_cluster = fs_read_word(fnp->plink_key_point + rel_start);	
	
	// found in the first cache buffer	
	if (key_cluster == fs_read_word(ptemp)) {
		if (divide_value > key_cluster_length) {
			key_cluster = fs_read_word(ptemp + block_start);

			for (i=0x00; i<block_offset; i++) {
				key_cluster = next_cluster(fnp->f_dpb, key_cluster);
			}
			return (CLUSTER) key_cluster;
		} else {
			return (CLUSTER) fs_read_word(ptemp + rel_offset);  
		}
	}
	
	// found in the second cache buffer 
	if (divide_value > key_cluster_length) {
		ret_cls = fs_read_word(ptemp + key_cluster_length);
	} else {
		ret_cls = fs_read_word(ptemp + divide_value);
	}
	
	if (key_cluster == ret_cls) {
		if (divide_value > key_cluster_length) {
			key_cluster = fs_read_word(ptemp + key_cluster_length + block_start);

			for (i=0x00; i<block_offset; i++) {
				key_cluster = next_cluster(fnp->f_dpb, key_cluster);
			}
			return (CLUSTER) key_cluster;
		} else {
			return (CLUSTER) fs_read_word(ptemp + divide_value + rel_offset);  
		}
	}
	
	//can not found in two buffers, abandon a buffer, and cache a new buffer
	if (divide_value > key_cluster_length) {
		for (i=0; i<key_cluster_length; i++) {
			fs_write_word(ptemp + key_cluster_length + i, fs_read_word(ptemp + i)); 
		}
	} else {
		for (i=0; i<divide_value; i++) {
			fs_write_word(ptemp + divide_value + i, fs_read_word(ptemp + i)); 
		}
	}
	
	//cache a new buffer of now point segment
	if (divide_value > key_cluster_length) {
		fs_write_word(ptemp, key_cluster);

		for (i=0x01,k=0x01; i<divide_value; i++) {
			key_cluster = next_cluster(fnp->f_dpb, key_cluster);	 // the next
			if (rel_offset == i) {
				ret_cls = key_cluster;
			}
			if (key_cluster == LONG_LAST_CLUSTER) {
				for (; i<divide_value; i++) {
					if ((i%divide_value2) == 0) {			// Added by Tristan, 2007/08/17
						fs_write_word(ptemp + k, 0xffff); 
						k++;
					}
				}
				break;
			}
			if (i % divide_value2) {				
				continue;	// not store the cluster not in key poistion						
			}
			fs_write_word(ptemp+k, key_cluster);
			k++;
		}
		
		return (CLUSTER) ret_cls;
	} else {
		fs_write_word(ptemp, key_cluster); 
		for (i=0x01; i<divide_value; i++) {
			key_cluster = next_cluster(fnp->f_dpb, key_cluster);
			if (key_cluster == LONG_LAST_CLUSTER) {
				for (; i<divide_value; i++) {
					fs_write_word(ptemp + i, 0xffff); 
				}
				break;
			}
			fs_write_word(ptemp + i, key_cluster); 
		}
		return (CLUSTER) fs_read_word(ptemp + rel_offset);	// return cluster number
	}
}

static CLUSTER fat32_get_key_cluster(f_node_ptr fnp, CLUSTER relative_clus)
{
	INT32U  i, j, k;
	CLUSTER	key_cluster, ret_cls;	
	INT32U  rel_start, rel_offset;
	INT32U  divide_value, divide_value2 = 0;
	INT32U  block_start = 0, block_offset = 0;
	INT16U	key_cluster_length;	
	INT32U	ptemp = fnp->plink_key_point;

	divide_value = fnp->link_divide_value;
	rel_start = relative_clus/divide_value;
	rel_offset = relative_clus%divide_value;

	//fnp->f_start_cluster保存fnp->plink_key_point buffer实际使用的大小
	if (rel_start + 1 < fnp->f_start_cluster) {
		key_cluster = fs_read_dword(ptemp+((rel_start+1)<<1));
	} else if (rel_start == fnp->f_start_cluster-1) {
		key_cluster = fs_read_dword(ptemp+(rel_start<<1));
	} else {
		return 1;			//超出范围
	}
	
	// the cluster is not in key cache
	if (key_cluster == 0) {
		for (i=2; fs_read_dword(ptemp+i); i+=2) ;		//第0个一定有值
		key_cluster = fs_read_dword(ptemp+i-2);
		j = (rel_start - (i>>1) + 2) * divide_value;
		for (k=1; k<=j; k++) {		//i <= rel_start+1;
			key_cluster = next_cluster(fnp->f_dpb, key_cluster);	 // the next
			if (key_cluster == LONG_LAST_CLUSTER) {
				break;
			}
			if (k % divide_value) {				
				continue;	// not store the cluster not in key poistion						
			}
			fs_write_dword(ptemp+i, key_cluster);
			i += 2;
		}
	}

	// just at key point, just return 
	if (!rel_offset) {
		return (CLUSTER) (fs_read_dword(ptemp + (rel_start<<1)));	
	}

	ptemp = fnp->plink_cache_buf;		
	// in case of memory alloc failure
	if (ptemp == 0) {
		key_cluster = fs_read_dword(fnp->plink_key_point + (rel_start<<1));

		for (i=0x00; i<rel_offset; i++) {
			key_cluster = next_cluster(fnp->f_dpb, key_cluster);
		}

		return (CLUSTER) key_cluster;
	}
	
	key_cluster_length = KEY_CLUSTER_LENGTH>>2;			// 128
	if (divide_value > key_cluster_length) {
		divide_value2 = (divide_value+key_cluster_length-1) / key_cluster_length;
		block_start = rel_offset / divide_value2;
		block_offset = rel_offset % divide_value2;
	}
	
	key_cluster = fs_read_dword(fnp->plink_key_point + (rel_start<<1));
		
	// found in the first cache buffer	
	if (key_cluster == fs_read_dword(ptemp)) {
		if (divide_value > key_cluster_length) {
			key_cluster = fs_read_dword(ptemp + (block_start<<1));

			for (i=0x00; i<block_offset; i++) {
				key_cluster = next_cluster(fnp->f_dpb, key_cluster);
			}
			return (CLUSTER) key_cluster;
		} else {
			return (CLUSTER) fs_read_dword(ptemp + (rel_offset<<1));  
		}
	}
	
	// found in the second cache buffer 
	if (divide_value > key_cluster_length) {
		ret_cls = fs_read_dword(ptemp + (key_cluster_length<<1));
	} else {
		ret_cls = fs_read_dword(ptemp + (divide_value<<1));
	}
	if (key_cluster == ret_cls) {
		if (divide_value > key_cluster_length) {
			key_cluster = fs_read_dword(ptemp + ((key_cluster_length + block_start)<<1));

			for (i=0x00; i<block_offset; i++) {
				key_cluster = next_cluster(fnp->f_dpb, key_cluster);
			}
			return (CLUSTER) key_cluster;
		} else {
			return (CLUSTER) fs_read_dword(ptemp + ((divide_value+ rel_offset)<<1));  
		}
	}

	// can not found in all two cache buffer, move the first cache buffer to second cache buffer
	if (divide_value > key_cluster_length) {
		for (i=0; i<key_cluster_length; i++) {
			fs_write_dword(ptemp + ((key_cluster_length + i)<<1), fs_read_dword(ptemp + (i<<1))); 
		}
	} else {
		for (i=0; i<divide_value; i++) {
			fs_write_dword(ptemp + ((divide_value + i)<<1), fs_read_dword(ptemp + (i<<1))); 
		}
	}
		
	// then fill the first cache buffer 
	if (divide_value > key_cluster_length) {
		fs_write_dword(ptemp, key_cluster);

		for (i=0x01,k=0x02; i<divide_value; i++) {
			key_cluster = next_cluster(fnp->f_dpb, key_cluster);	 // the next			
			if (rel_offset == i) {
				ret_cls = key_cluster;
			}
			if (key_cluster == LONG_LAST_CLUSTER) {
				for (; i<divide_value; i++) {
					if ((i%divide_value2) == 0) {		// Added by Tristan, 2007/08/17
						fs_write_dword(ptemp+k, LONG_LAST_CLUSTER);
						k += 2;
					}
				}
				break;
			}
			if (i % divide_value2) {				
				continue;	// not store the cluster not in key poistion						
			}
			fs_write_dword(ptemp+k, key_cluster);
			k += 2;
		}
		
		return (CLUSTER) ret_cls;
	} else {
		fs_write_dword(ptemp, key_cluster); 
		for (i=0x01; i<divide_value; i++){
			key_cluster = next_cluster(fnp->f_dpb, key_cluster);
			if (key_cluster == LONG_LAST_CLUSTER) {
				for (; i<divide_value; i++) {
					fs_write_dword(ptemp + (i<<1), LONG_LAST_CLUSTER); 
				}
				break;
			}
			fs_write_dword(ptemp + (i<<1), key_cluster); 
		}
		return (CLUSTER) fs_read_dword(ptemp + (rel_offset<<1));	// return cluster number
	}
}
#endif

/************************************************************************/
/*
	check_flink_sequence	
	input:	
			f_node_ptr filenp

	output: 
			FLINK_ATTR_LINER
			FLINK_ATTR_ALL_CACHE

	note:	just FOR FAT16, not for FAT32
  */
/************************************************************************/
void check_flink_sequence(f_node_ptr fnp)
{
  #if _CACHE_FAT_INFLEXION == 1
	fs_LoadFatTable(fnp);
  #else	
	CLUSTER	i;
	INT32U	total_cluster;
	INT32U	file_size;
	INT16U	cluster_size;
	INT16U	key_cluster_length;
	INT32U	ptemp;

	key_cluster_length = KEY_CLUSTER_LENGTH;		// 256
	if (ISFAT32(fnp->f_dpb)) {
		key_cluster_length >>= 1;
	}
	
	// reset file link attribute
	fnp->f_link_attribute = 0x00;

	// get cluster size
	cluster_size = fnp->f_dpb->dpb_secsize;
	cluster_size <<= fnp->f_dpb->dpb_shftcnt;	

	// get file size
	file_size = fnp->f_dir.dir_size;

	// get total cluster
	total_cluster = (file_size+cluster_size-1) / cluster_size;

	// judge continue && compact link
	i = fnp->f_cluster + total_cluster -1;

	total_cluster++;	// 考虑到有一个结束标志，多加一个

	//判断fat是否是连续的
	if ((total_cluster>key_cluster_length) &&
		(next_cluster(fnp->f_dpb, i-1)==i) &&
		(next_cluster(fnp->f_dpb, i)==LONG_LAST_CLUSTER)) 
	{
		fnp->f_start_cluster = fnp->f_cluster;
		fnp->f_link_attribute = FLINK_ATTR_LINER;
		return; 
	}

	// just memory allocate
	fnp->plink_key_point = 0x00;
	fnp->plink_cache_buf = 0x00;
	
	i = key_cluster_length;
	if (total_cluster < key_cluster_length) {
		i = total_cluster +1;
	}	
	
	if (ISFAT32(fnp->f_dpb)) {
		i <<= 1;
	}

	ptemp = unsp_fs_get_index_1(i);
	if (ptemp == 0) {
		return;
	}

	fnp->plink_key_point = ptemp;
	
	// if it's small, cache all cluster
	if (total_cluster <= key_cluster_length) {
		cache_all_cluster(fnp, total_cluster);
		return;
	}

	// so long chain, no space to store, just the key point
	cache_key_cluster(fnp, total_cluster);
	return;
  #endif
}

/************************************************************************/
/* 在UNSP 空间上获得地址空间值

input:	len 需要的空间长度
output:	NULL 没有内存空间
		其他值 有效的地址空间
                                                                      */
/************************************************************************/
#if _CACHE_FAT_INFLEXION == 0
static INT32U unsp_fs_get_index_1(INT16U len)
{
	return FS_SeekSpeedup_Malloc(len);
}

static INT32U unsp_fs_get_index_2(INT16U divide_value)
{
	INT32U ptemp;

	ptemp = FS_SeekSpeedup_Malloc(divide_value*2);
	
	fs_write_dword(ptemp, 0);
	fs_write_dword(ptemp+divide_value, 0);
	return ptemp;	
}

/************************************************************************/
/*

	input:	
			f_node_ptr			fnp 
			total_clusters		cluster count
	output:
			void 
																		*/
/************************************************************************/
static void cache_all_cluster(f_node_ptr fnp, INT32U total_cluster)
{
	CLUSTER i, j;
	INT32U	ptemp;

	ptemp = fnp->plink_key_point;
	j = fnp->f_cluster;
		
	if (ISFAT32(fnp->f_dpb)) {
		fs_write_dword(ptemp, j);
		for (i=0x01; i<total_cluster; i++) {
			j = next_cluster(fnp->f_dpb, j);	// get next 
			fs_write_dword(ptemp+(i<<1), j);
		}
	} else if (ISFAT16(fnp->f_dpb)) {
		fs_write_word(ptemp, (INT16U)j);
		for (i=0x01; i<total_cluster; i++) {
			j = next_cluster(fnp->f_dpb, j);	// get next 
			fs_write_word(ptemp+i, (INT16U) j);
		}
	}

	fnp->f_link_attribute = FLINK_ATTR_ALL_CACHE;

	return;
}

static void cache_key_cluster(f_node_ptr fnp, INT32U total_cluster)
{
	INT32U	divide_value;
	CLUSTER	i;
	INT16U	key_cluster_length;
	INT32U	ptmp;	

	key_cluster_length = KEY_CLUSTER_LENGTH;		// 256
	if (ISFAT32(fnp->f_dpb)) {
		key_cluster_length >>= 1;					// 128
	}

	// cluster count is variable, so we must calculate 
	divide_value = (total_cluster+key_cluster_length-1)/key_cluster_length;
	
	//如果divide_value大于256的话，最多只申请256 word的内存
	if (divide_value > (key_cluster_length>>1)) {
		ptmp = unsp_fs_get_index_2(KEY_CLUSTER_LENGTH>>1);
	} else if (ISFAT32(fnp->f_dpb)) {
		ptmp = unsp_fs_get_index_2(divide_value<<1);
	} else {
		ptmp = unsp_fs_get_index_2(divide_value);
	}
	if (ptmp == NULL) {
		return;
	}
		
	fnp->link_divide_value = divide_value;
	fnp->plink_cache_buf = ptmp;
	
	//用于保存fnp->plink_key_point实际使用的大小
	fnp->f_start_cluster = (total_cluster + divide_value - 1) / divide_value;

	ptmp = fnp->plink_key_point;
	fs_write_dword(ptmp, fnp->f_cluster);
	for (i=2; i<KEY_CLUSTER_LENGTH; i++) {
		fs_write_word(ptmp+i, 0);
	}
	
	// set link attribute
	fnp->f_link_attribute = FLINK_ATTR_KEY_CACHE;

	return;		
}
#endif		// _CACHE_FAT_INFLEXION == 0
#endif		// _SEEK_SPEED_UP == 1


INT32S fast_rdblock(INT16S fd, INT32U lpBuf, INT32U count, INT16S mode) 	 
{
	REG f_node_ptr fnp;
	REG struct buffer *bp;
	INT32U xfr_cnt = 0;
	INT32U ret_cnt = 0;
	INT32U to_xfer;
	INT16U secsize;
	INT32U currentblock;
	INT16S ret;
	INT16S disk; 
	INT32U fast_num;
	INT32U	fast_startblock;
	INT32U	fast_preblock;
	INT8U	fast_flag = 0;
	INT16U 	sector, boff;
	INT8U  begin_add_flag;
	#if WITHEXFAT == 1 
	INT64U filesize;
	#else
	INT32U filesize;
	#endif

	
	fnp = xlt_fd(fd);
	if (fnp == (f_node_ptr) 0) {
		return DE_INVLDHNDL;
	}

	if (!fnp->f_dpb->dpb_mounted || !fnp->f_count) {						
		return (INT32S) DE_INVLDHNDL;		
	}

	#if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
		filesize=fnp->ext_dir->ValidDataLength;
	else
	#endif
		filesize=fnp->f_dir.dir_size;



	if ((fnp->f_offset + count) > filesize) {
		count = (INT32U)(filesize- fnp->f_offset);		
	}		

	
	to_xfer = count;
	secsize = fnp->f_dpb->dpb_secsize;	// get sector size	
	disk = fnp->f_dpb->dpb_unit;		// get disk internal number
	
	//if( (fnp->f_offset & 0x01) || (lpBuf & 0x1) )		
	if( (fnp->f_offset & 0x03) || (lpBuf & 0x3) )				// 08/05/14 modify for 4 byte alignment
		begin_add_flag = 1;
	else 
		begin_add_flag = 0;

#if _SEEK_SPEED_UP == 1
  #if _CACHE_FAT_INFLEXION == 1
	if (fnp->f_link_attribute & FLINK_ATTR_INFLEXION_CACHE) {
		INT32U relcluster;
		
		relcluster = (CLUSTER) ((fnp->f_offset / fnp->f_dpb->dpb_secsize) >> fnp->f_dpb->dpb_shftcnt);
		
		if (relcluster < fast_GetTotalSpeedupCluster(fnp)) {
			fast_flag = 1;
		}
	} else 
  #endif
	if (fnp->f_link_attribute & FLINK_ATTR_LINER) {
		fast_flag = 1;
	}	
	
	if (!begin_add_flag && fast_flag) {
		while (ret_cnt < count) {
			ret = map_cluster(fnp, XFR_READ);
			if (ret != SUCCESS) {
				 gFS_errno = ret;
				 save_far_f_node(fnp);
				 return DE_HNDLDSKFULL;
			}
			
			sector = (INT16U) (fnp->f_offset >> 9) & fnp->f_dpb->dpb_clsmask; 
			boff = (INT16U) (fnp->f_offset & 0x1ff);	
			currentblock = clus2phys(fnp->f_cluster, fnp->f_dpb) + sector; 
			
			if (!boff) {
				fast_num = to_xfer / fnp->f_dpb->dpb_secsize;
			  #if _CACHE_FAT_INFLEXION == 1
				if (fnp->f_link_attribute & FLINK_ATTR_INFLEXION_CACHE) {
					INT32U	length;
					
					length = fast_GetNextInflexion(fnp, fnp->f_cluster_offset);
					if (length == 0) {
						goto Normal_Read;
					}
					
					length <<= fnp->f_dpb->dpb_shftcnt;		//连续的 sector number
					length = length - sector;				//表示从当前的 sector 开始的连续 sector 数量
					
					if (fast_num > length) {
						fast_num = length;
					}
				}
			  #endif	
				if (fast_num) {
					//把currentblock对应的buffer设置为无效
					DeleteBlockInBufferCache(currentblock, currentblock + fast_num - 1, disk, XFR_READ);		
					
					if (dskxfer(disk, currentblock, lpBuf>>WORD_PACK_VALUE, fast_num, DSKREAD)) {
						gFS_errno = DE_INVLDDATA;
						return gFS_errno;
					}	
					
					xfr_cnt = fast_num * secsize;
					
					to_xfer -= xfr_cnt;
					ret_cnt += xfr_cnt;
					fnp->f_offset += xfr_cnt;
					
					// update dest address pointer
					lpBuf = (INT32U) lpBuf + xfr_cnt;
					
					continue;
				}
			}
			
			bp = getblock(currentblock, disk);			
			if (bp == NULL) {
				save_far_f_node(fnp);
				return DE_BLKINVLD;
			}
			
			xfr_cnt = min(to_xfer, secsize-boff);
			
			if (!(fnp->f_flags & F_DDIR)) {
				xfr_cnt = min(xfr_cnt, filesize - fnp->f_offset);		
			}
			
			MemPackByteCopyFar(((INT32U)bp->b_buffer<<WORD_PACK_VALUE)+boff, lpBuf, xfr_cnt);
			
			if (xfr_cnt==(sizeof(bp->b_buffer)<<WORD_PACK_VALUE) ||
			    ((mode&XFR_READ) && (fnp->f_offset+xfr_cnt==filesize)))
			{
				bp->b_flag |= BFR_UNCACHE;
			}
			
			fnp->f_offset += xfr_cnt;
			ret_cnt += xfr_cnt;
			to_xfer -= xfr_cnt;
					
			lpBuf = ((INT32U) lpBuf + xfr_cnt);
		}
		
		save_far_f_node(fnp);
		return (INT32S) ret_cnt;
	}
#endif

	// Do the data transfer. Use block transfer methods so that we can utilize memory management in future
#if _SEEK_SPEED_UP==1 && _CACHE_FAT_INFLEXION==1
Normal_Read:
#endif
	fast_flag = 0;
	while (ret_cnt < count) {
		// Do an EOF test and return whatever was transferred but only for regular files.
		if (!(fnp->f_flags & F_DDIR) && (fnp->f_offset >= filesize)) {
			save_far_f_node(fnp);
			return ret_cnt;
		}
		
		ret = map_cluster(fnp, mode & (XFR_READ|XFR_WRITE));
		if (ret != SUCCESS) {
			 gFS_errno = ret;
			 save_far_f_node(fnp);
			 return DE_HNDLDSKFULL;
		}
		
		// Compute the block within the cluster and the offset within the block.
		sector = (INT16U) (fnp->f_offset >> 9) & fnp->f_dpb->dpb_clsmask;
		boff = (INT16U) (fnp->f_offset & 0x1ff);
		currentblock = clus2phys(fnp->f_cluster, fnp->f_dpb) + sector; 
		
		if (!begin_add_flag && !boff) {						//满足读一个sector的条件，直接读到user buffer
			if ((count-ret_cnt) >= secsize) {
				if (!fast_flag) {
					//开始计数
					fast_startblock = currentblock;
					fast_preblock = currentblock;
					fast_flag = 1;
				} else {
					if (fast_preblock+1 == currentblock) {
						//连续的sector，继续统计
						fast_preblock = currentblock;		
					} else {
						fast_flag = 3;		//sector地址不连续标志
					}
				}
				
				if(fast_flag != 3)
				{
					fnp->f_offset += secsize;
					ret_cnt += secsize;
					to_xfer -= secsize;
				}
			}
			
			if ((count-ret_cnt < secsize) && fast_flag) {
				fast_flag = 2;
			}
		}
		
		if (fast_flag) {			
			if (fast_flag == 1)	{	//连续的sector地址，继续计数
				continue;
			}

			fast_num = fast_preblock - fast_startblock + 1;
		
			//把currentblock对应的buffer设置为无效
			DeleteBlockInBufferCache(fast_startblock, fast_preblock, disk, XFR_READ);		
						
			if (dskxfer(disk, fast_startblock, lpBuf>>WORD_PACK_VALUE, fast_num, DSKREAD)) {
				gFS_errno = DE_INVLDDATA;
				return gFS_errno;
			}	
				
			// update dest address pointer
			lpBuf = (INT32U) lpBuf + (INT32U) fast_num * secsize;
			
			//if (fast_flag == 3) {			//不连续的sector地址，重新开始计数
				//开始计数
				//fast_startblock = currentblock;
				//fast_preblock = currentblock;
				//fast_flag = 1;
			//} else {
			fast_flag = 0;	//结束了
			//}
			continue;
		}
		
		bp = getblock(currentblock, disk);			
		if (bp == NULL) {
			save_far_f_node(fnp);
			return DE_BLKINVLD;
		}
		
		xfr_cnt = min(to_xfer, secsize - boff);
		
		if (!(fnp->f_flags & F_DDIR)) {
			xfr_cnt = (INT16U) min(xfr_cnt, filesize - fnp->f_offset);		
		}
		
		MemPackByteCopyFar(((INT32U)bp->b_buffer<<WORD_PACK_VALUE)+boff, lpBuf, xfr_cnt);
		
		if (xfr_cnt==(sizeof(bp->b_buffer)<<WORD_PACK_VALUE) ||
		    ((mode&XFR_READ) && (fnp->f_offset+xfr_cnt==filesize)))
		{
			bp->b_flag |= BFR_UNCACHE;
		}
		
		fnp->f_offset += xfr_cnt;
		ret_cnt += xfr_cnt;
		to_xfer -= xfr_cnt;
				
		lpBuf = ((INT32U) lpBuf + xfr_cnt);
	}

	save_far_f_node(fnp);
	return (INT32S) ret_cnt;
}

/************************************************************************/
/*
	history:
		zhangzha add @[5/12/2006]
		添加对句柄有效性的判断
                                                                      */
/************************************************************************/
#if 0
INT32S fast_wrblock(INT16S fd, INT32U lpBuf, INT32U count, INT16S mode) 	 
{
	REG f_node_ptr fnp;
	REG struct buffer *bp;
	INT16U xfr_cnt = 0;
	INT32U ret_cnt = 0;
	INT32U to_xfer;
	INT16U secsize;
	INT32U currentblock;
	INT16S ret;
	INT16S disk; 
	INT16U	fast_num;
	INT32U	fast_startblock;
	INT32U	fast_preblock;
	INT8U	fast_flag = 0;	

	fnp = xlt_fd(fd);
	if ((fnp == (f_node_ptr)0))
	{
		return 	 DE_INVLDHNDL;
	}

	if (!fnp ->f_dpb ->dpb_mounted || !fnp->f_count)
	{						
		return (INT32S) DE_INVLDHNDL;		
	}

	if ((fnp->f_dir.dir_attrib & D_RDONLY) || ((fnp->f_mode & O_ACCMODE) == O_RDONLY)) 	 
	{
		return DE_ACCESS; 
	}

	fnp->f_dir.dir_attrib |= D_ARCHIVE;
	fnp->f_flags |= F_DMOD;       /* mark file as modified */
	fnp->f_flags &= ~F_DDATE;     /* set date not valid any more */
	
	if (dos_extend(fnp) != SUCCESS) 	 
	{
		save_far_f_node(fnp);
		return DE_HNDLDSKFULL; 
	}	

	to_xfer = count;
	secsize = fnp->f_dpb->dpb_secsize;	// get sector size	
	disk = fnp->f_dpb->dpb_unit;		// get disk internal number

	/* Do the data transfer. Use block transfer methods so that we  */
	/* can utilize memory management in future DOS-C versions.      */
	while (ret_cnt < count)
	{
	    INT16U sector, boff;

		ret = map_cluster(fnp, mode & (XFR_READ|XFR_WRITE));

		if (ret != SUCCESS) 
		{
			gFS_errno = ret;
			save_far_f_node(fnp);
			return DE_HNDLDSKFULL;
		}

		/* Compute the block within the cluster and the offset  */
		/* within the block.                                    */
		sector = (INT8U)(fnp->f_offset >> 9) & fnp->f_dpb->dpb_clsmask;
		boff = (INT16U)(fnp->f_offset & 0x1ff);
		currentblock = clus2phys(fnp->f_cluster, fnp->f_dpb) + sector;

		if (!boff) //满足写一个sector的条件，直接写到disk
		{
			if ((count - ret_cnt) >= secsize)
			{
				if (!fast_flag)
				{
					//开始计数
					fast_startblock = currentblock;
					fast_preblock = currentblock;
					fast_flag = 1;
				}
				else
				{
					if (fast_preblock + 1 == currentblock)
					{
						//连续的sector，继续统计
						fast_preblock = currentblock;		
					}
					else
					{
						//sector地址不连续
						fast_flag = 3;
					}
				}
				
				fnp->f_offset += secsize;
				ret_cnt += secsize;
				to_xfer -= secsize;
				
				if (fnp->f_offset > fnp->f_dir.dir_size)
				{
					fnp->f_dir.dir_size = fnp->f_offset;
				}
				merge_file_changes(fnp, FALSE); 
			}
			
			if ((count - ret_cnt < secsize) && fast_flag)
			{
				fast_flag = 2;
			}
		}
		
		if (fast_flag)
		{			
			if (fast_flag == 1)		//连续的sector地址，继续计数
				continue;
			
			fast_num = fast_preblock - fast_startblock + 1;
		
			//把currentblock对应的buffer设置为无效
			SetWirteBufferType(BFR_DATA);
			DeleteBlockInBufferCache(fast_startblock, fast_preblock, disk, XFR_WRITE);		
			if (dskxfer(disk, fast_startblock, lpBuf>>WORD_PACK_VALUE, fast_num, DSKWRITE)) 
			{
				gFS_errno = DE_INVLDDATA;
				return gFS_errno;
			}				
	
			// update dest address pointer
			lpBuf = (INT32U)lpBuf + (INT32U)fast_num * secsize;
			
			if (fast_flag == 3)	//不连续的sector地址，重新开始计数
			{
				//开始计数
				fast_startblock = currentblock;
				fast_preblock = currentblock;
				fast_flag = 1;
			}
			else
			{
				fast_flag = 0;	//结束了
			}
			continue;
		}

		bp = getblock(currentblock, disk);			
		if (bp == NULL)             /* (struct buffer *)0 --> DS:0 !! */
		{
			save_far_f_node(fnp);
			return DE_BLKINVLD;
		}
	
		xfr_cnt = min(to_xfer, secsize - boff);
				
		MemPackByteCopyFar(lpBuf, ((INT32U)bp->b_buffer<<WORD_PACK_VALUE)+boff, xfr_cnt);
				
		bp->b_flag |= (BFR_DIRTY | BFR_VALID);
		
		// 果buffer已经写满了，把buffer回写到disk。这样可以保证接下来的写是连续的
		if (xfr_cnt + boff == (sizeof(bp->b_buffer)<<WORD_PACK_VALUE))
		{
			if (!flush1(bp))
				return DE_INVLDBUF;
		}
		
		fnp->f_offset += xfr_cnt;
		ret_cnt += xfr_cnt;
		to_xfer -= xfr_cnt;
				
		lpBuf = ((INT32U)lpBuf + xfr_cnt);
		
		if (fnp->f_offset > fnp->f_dir.dir_size)
		{
			fnp->f_dir.dir_size = fnp->f_offset;
		}
		merge_file_changes(fnp, FALSE);     /* /// Added - Ron Cemer */
	}

	save_far_f_node(fnp);
	return (INT32S) ret_cnt;
}
#endif

#ifndef READ_ONLY
INT32S fast_wrblock(INT16S fd, INT32U lpBuf, INT32U count, INT16S mode) 		 
{
	REG f_node_ptr fnp;
	REG struct buffer *bp;
	INT32U xfr_cnt;
	INT32U ret_cnt = 0;
	INT32U to_xfer;
	INT16U secsize;
	INT32U currentblock;
	INT16S ret;
	INT16S disk; 

	INT8U	begin_add_flag;		
	INT16U	ClusterSize;
	INT32U	nCluster;
	INT16U	nLength;
	INT16U	nSector = 0;
	INT16U	fast_num;
	INT32U	sector_cnt;

	fnp = xlt_fd(fd);
	if( (fnp == (f_node_ptr)0))
		return 	 DE_INVLDHNDL;

	if (!fnp->f_dpb ->dpb_mounted || !fnp->f_count)
		return (INT32S) DE_INVLDHNDL;		

	if((fnp->f_dir.dir_attrib & D_RDONLY) || ((fnp->f_mode & O_ACCMODE) == O_RDONLY)) 	 
		return DE_ACCESS; 

	fnp->f_dir.dir_attrib |= D_ARCHIVE;
	fnp->f_flags |= F_DMOD;       /* mark file as modified */
	fnp->f_flags &= ~F_DDATE;     /* set date not valid any more */
	
	if (dos_extend(fnp) != SUCCESS) 	 
	{
		save_far_f_node(fnp);
		return DE_HNDLDSKFULL; 
	}	

	to_xfer = count;
	secsize = fnp->f_dpb->dpb_secsize;	// get sector size	
	ClusterSize = secsize << fnp->f_dpb->dpb_shftcnt;
	disk = fnp->f_dpb->dpb_unit;		// get disk internal number		
	
	/* Do the data transfer. Use block transfer methods so that we  */
	/* can utilize memory management in future DOS-C versions.      */
	while (ret_cnt < count)
	{
	    INT16U sector, boff, end_boff;
		
		//if( (fnp->f_offset & 0x01) || (lpBuf & 0x1) )	
		if( (fnp->f_offset & 0x03) || (lpBuf & 0x3) )		// 08/05/14 modify for 4 byte alignment
			begin_add_flag = 1;
		else 
			begin_add_flag = 0;
		
		if(nSector == 0)
		{
			//计算要操作的cluster的数量
			nCluster = ((fnp->f_offset & (ClusterSize - 1)) + to_xfer + ClusterSize - 1) >> 
						(fnp->f_dpb->dpb_shftcnt + 9);
			
			sector_cnt = ( (fnp->f_offset & 0x1ff) + to_xfer + 0x1ff ) >> 9;
			//因为只在一个sector中查找连续的cluster id，所以nLength值一定小于256(fat16)或128(fat32)
			//获取起始f_offset对应的cluster起始cluster id及从该id开始的连续的cluster数量
			ret = fast_map_cluster(fnp, nCluster, &nLength, XFR_WRITE);
			if (ret != SUCCESS) 
			{
				gFS_errno = ret;
				save_far_f_node(fnp);
				return DE_HNDLDSKFULL;
			}
	
			/* Compute the block within the cluster and the offset  */
			/* within the block.                                    */
			sector = (INT8U)(fnp->f_offset >> 9) & fnp->f_dpb->dpb_clsmask;
			currentblock = clus2phys(fnp->f_cluster, fnp->f_dpb) + sector;
	
			nSector = nLength << fnp->f_dpb->dpb_shftcnt;
			nSector -= sector;				//从当前sector开始的连续的sector数量
			
			if(nSector > sector_cnt)
				nSector = (INT16U)sector_cnt;
		}
		
		boff = (INT16U)(fnp->f_offset & 0x1ff);	
		end_boff = to_xfer & 0x1ff;
		
		if( !begin_add_flag && !boff && !((nSector == 1) && end_boff) ) //满足写一个sector的条件，直接写到disk
		{
			//最后一个sector不是写满一个sector，减去一个sector
			if(end_boff)
				fast_num = nSector - 1;
			else
				fast_num = nSector;

			//把currentblock对应的buffer设置为无效
			SetWirteBufferType(BFR_DATA);
			DeleteBlockInBufferCache(currentblock, currentblock + fast_num - 1, disk, XFR_WRITE);		
			if(dskxfer(disk, currentblock, lpBuf>>WORD_PACK_VALUE, fast_num, DSKWRITE)) 
			{
				gFS_errno = DE_INVLDDATA;
				return gFS_errno;
			}				
	
			xfr_cnt = (INT32U)fast_num * secsize;
		}
		else
		{
			fast_num = 1;
			
			bp = getblock(currentblock, disk);			
			if (bp == NULL)             /* (struct buffer *)0 --> DS:0 !! */
			{
				save_far_f_node(fnp);
				return DE_BLKINVLD;
			}
		
			xfr_cnt = min(to_xfer , secsize - boff);
					
			MemPackByteCopyFar(lpBuf , ((INT32U)bp->b_buffer<<WORD_PACK_VALUE)+boff , xfr_cnt);
					
			bp->b_flag |= (BFR_DIRTY | BFR_VALID);
			
			//如果buffer已经写满了，把buffer回写到disk。这样可以保证接下来的写是连续的
			if( xfr_cnt + boff == (sizeof(bp->b_buffer)<<WORD_PACK_VALUE) )
			{
				if (!flush1(bp))
					return DE_INVLDBUF;
			}
			
			//begin_add_flag = 0;
		}
		
		nSector -= fast_num;
		currentblock += fast_num;
		
		ret_cnt += xfr_cnt;
		to_xfer -= xfr_cnt;	
		fnp->f_offset += xfr_cnt;
		lpBuf = (INT32U)lpBuf + xfr_cnt;	

		#if WITHEXFAT == 1
		if(ISEXFAT(fnp->f_dpb))
		{
			if (fnp->f_offset > fnp->ext_dir->ValidDataLength)
			{
				fnp->ext_dir->ValidDataLength = fnp->f_offset;
				fnp->ext_dir->DataLength=fnp->f_offset;
			}	
		}
		else 
		#endif 	
		if (fnp->f_offset > fnp->f_dir.dir_size)
		{
			fnp->f_dir.dir_size = fnp->f_offset;
		}
		
		merge_file_changes(fnp , FALSE);     /* /// Added - Ron Cemer */
		
		if(nSector == 0)
		{
			fast_map_cluster_end(fnp, nLength);
		}
	}

	save_far_f_node(fnp);
	return (INT32S) ret_cnt;
}
#endif
