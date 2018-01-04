#include "fsystem.h"
#include "globals.h"

//=========================================================
//函数功能：
//    将file2_handle文件连接到file1_handle文件之后
//=========================================================
INT16S file_cat(INT16S file1_handle, INT16S file2_handle)
{
	REG f_node_ptr fnp1, fnp2;
	INT16S disk; 
	
	INT32U file1_size, file2_size;
	INT32U file1_startclst, file2_startclst;
	INT16U secsize;
	INT16U	ClusterSize;
	INT32U  i, next_clus;
	INT16S  ret = 0;
	
	FS_OS_LOCK();	
	
	if((file1_handle<0)|(file2_handle<0)) {
		ret = DE_INVLDPARM;
		goto end;
	}
	
	fnp1 = xlt_fd(file1_handle);
	if( (fnp1 == (f_node_ptr)0)) {
		ret = DE_INVLDHNDL;
		goto end;
	}
	
	if (!fnp1->f_dpb ->dpb_mounted || !fnp1->f_count) {
		ret = DE_INVLDHNDL;
		goto end;
	}

	fnp2 = xlt_fd(file2_handle);
	if( (fnp2 == (f_node_ptr)0)) {
		ret = DE_INVLDHNDL;
		goto end;
	}
	
    if (!fnp2->f_dpb ->dpb_mounted || !fnp2->f_count) {
		ret = DE_INVLDHNDL;
		goto end;
	}

  
  	//获取cluster size
    secsize = fnp1->f_dpb->dpb_secsize;	// get sector size	
	ClusterSize = secsize << fnp1->f_dpb->dpb_shftcnt;
	disk = fnp1->f_dpb->dpb_unit;		// get disk internal number		
	
//	file1_startclst = fnp1->f_dir.dir_start;  //获取文件起始簇
//	file2_startclst = fnp2->f_dir.dir_start;  //获取文件起始簇
	
	file1_startclst = ((fnp1->f_dir.dir_start_high&0xffff)<<16)|fnp1->f_dir.dir_start;  //获取文件起始簇
	file2_startclst = ((fnp2->f_dir.dir_start_high&0xffff)<<16)|fnp2->f_dir.dir_start;  //获取文件起始簇
	
	file1_size = fnp1->f_dir.dir_size;
	file2_size = fnp2->f_dir.dir_size;
	//g_total_ClusterSize = file1_startclst;
	//(DBG_PRINT("flie1_stclus = %d\r\n", file1_startclst));	
	//(DBG_PRINT("flie2_stclus = %d\r\n", file2_startclst));					
	for(i=file1_startclst;;)
	{
		 next_clus = next_cluster(fnp1->f_dpb, i);
		 if(next_clus==LONG_LAST_CLUSTER) {
		   break;
		 } else if (next_clus == 1) {
			ret = 1;
			goto end;
		 }
		 
		 i = next_clus;
	}
	//DEBUG_MSG(DBG_PRINT("flie1_endclus = %d\r\n", i));	
  #if WITHEXFAT == 1	
	link_fat(fnp1->f_dpb,i,file2_startclst,fnp1->ext_dir->SecondaryFlags);
  #else
    link_fat(fnp1->f_dpb,i,file2_startclst,NULL);
  #endif	
	//if (!flush1(fnp1->f_dpb))
	//		return DE_INVLDBUF;
	//修改文件size
	fnp1->f_dir.dir_size = file1_size+file2_size;
	fnp1->f_flags |= F_DMOD;
	
	flush_buffers(disk);

end:
	FS_OS_UNLOCK();
	return ret;
}
INT16S get_file_node_info(INT16S file1_handle)
{
	REG f_node_ptr fnp1;
	INT32U file1_startclst;
	FS_OS_LOCK();	
	fnp1 = xlt_fd(file1_handle);
	if( (fnp1 == (f_node_ptr)0))
		return 	 DE_INVLDHNDL;
	
	file1_startclst = fnp1->f_dir.dir_start;  //获取文件起始簇
	
	DBG_PRINT("\r\nindex_node_stclus = %d\r\n", file1_startclst);
	FS_OS_UNLOCK();
	return 0;	
}


/*if file_handle1 is tail cluster access, we can use fast file cat*/
/*file fast cat, dominant add, 2011/10/03*/
INT16S file_fast_cat(INT16S file1_handle, INT16S file2_handle)
{
	REG f_node_ptr fnp1, fnp2;
    INT32U file2_startclst;
    INT16S disk;
    INT32U file1_size, file2_size;
    CLUSTER file1_tail_cluster;
  	INT16U secsize;
	INT16U	ClusterSize;
	INT32U  i, next_clus; 
   	INT16S  ret = 0;
    
	FS_OS_LOCK();

	fnp1 = xlt_fd(file1_handle);
	fnp2 = xlt_fd(file2_handle);

    secsize = fnp1->f_dpb->dpb_secsize;	// get sector size
	ClusterSize = secsize << fnp1->f_dpb->dpb_shftcnt;
	disk = fnp1->f_dpb->dpb_unit;		// get disk internal number
    //free_fat=fast_extend(fnp1,  1, OutLength);

    //free_fat = fnp1->f_cluster;

	//file1_startclst = ((fnp1->f_dir.dir_start_high&0xffff)<<16)|fnp1->f_dir.dir_start;  //获取文件起始簇    
	file2_startclst = ((fnp2->f_dir.dir_start_high&0xffff)<<16)|fnp2->f_dir.dir_start;  //获取文件起始簇

	file1_size = fnp1->f_dir.dir_size;
	file2_size = fnp2->f_dir.dir_size;

    //file1_tail_cluster=get_tail_cluster(file1_handle);
    file1_tail_cluster = fnp1->f_cluster;

    //DBG_PRINT ("Fast File1 current cluster:%d\r\n",fnp1->f_cluster);  // dominant add for test
    //DBG_PRINT ("Fast File2 current cluster:%d\r\n",fnp2->f_cluster);  // dominant add for test

    /*search from current tail cluster*/
	for(i=file1_tail_cluster;;)
	{
		 next_clus = next_cluster(fnp1->f_dpb, i);
		 if(next_clus==LONG_LAST_CLUSTER) {
		   break;
		 }
		 else if (next_clus == 1) {
			ret = 1;
			goto end;
		 }

		 i = next_clus;
	}
  #if WITHEXFAT == 1
    link_fat(fnp1->f_dpb,file1_tail_cluster,file2_startclst,fnp1->ext_dir->SecondaryFlags);
  #else
    link_fat(fnp1->f_dpb,file1_tail_cluster,file2_startclst,NULL);
  #endif  
	fnp1->f_dir.dir_size = file1_size+file2_size;
	fnp1->f_flags |= F_DMOD;
	flush_buffers(disk);

end:
    FS_OS_UNLOCK();
   	return ret;
} 



