/************************************************************************/
/* 
	find file from disk ignore dir
	
	zhangzha creat @2006.09.18
 */
/************************************************************************/

#include "fsystem.h"
#include "globals.h"

#if FIND_EXT == 1

f_ppos fs_getfirstfile(STDiskFindControl *pstDiskFindControl, INT16S dsk, INT8S *extname, struct sfn_info* f_info, INT16S attr, INT8S speedup,INT32S (*filter)(INT8S *str))
{
	pstDiskFindControl->level = 0;
	pstDiskFindControl->find_begin_file = 0;
	pstDiskFindControl->find_begin_dir = 0;	
	
	pstDiskFindControl->sfn_cluster = 0;
	pstDiskFindControl->sfn_cluster_offset = 0;
	pstDiskFindControl->dir_change_flag = 0;
	pstDiskFindControl->root_dir_flag = 0;
	
	memset(pstDiskFindControl->dir_offset, 0, 16 * sizeof(INT16U));
	memset(pstDiskFindControl->cluster, 0, 16 * sizeof(INT32U));
	memset(pstDiskFindControl->cluster_offset, 0, 16 * sizeof(INT32U));
	
	sfn_changedisk(pstDiskFindControl, dsk);
	sfn_setcurdirpos(pstDiskFindControl, &pstDiskFindControl->cur_pos);
	return fs_getnextfile(pstDiskFindControl, dsk, extname, f_info, attr, speedup,filter);
}

f_ppos fs_getnextfile(STDiskFindControl *pstDiskFindControl, INT16S dsk, INT8S *extname, struct sfn_info* f_info, INT16S attr, INT8S speedup,INT32S (*filter)(INT8S *str))
{
	struct	sfn_info dir_info;
	f_pos	*p_file_pos, *p_dir_pos;
	f_pos	cur_pos, *p_cur_pos;
	
	while (1)		//find file
	{
		if (pstDiskFindControl->find_begin_file == 0)
		    p_file_pos=sfn_findfirst(pstDiskFindControl, dsk, (CHAR *) extname, f_info, attr,filter);
		else
		    p_file_pos=sfn_findnext(pstDiskFindControl, dsk, (CHAR *) extname, f_info, attr,filter);

		if (p_file_pos != NULL)	
		{
		    if (pstDiskFindControl->level == 0)
		    {
		    	pstDiskFindControl->root_dir_flag = 1;			// the root folder have found file
		    }
		    pstDiskFindControl->find_begin_file = 1;			//下一次调用findnext
		    if (f_info->f_name[0] == '.')		//070423 当查找的属性为D_ALL时，不返回"."和".."节点
		    	continue;
		    
		    return p_file_pos; 
		}
		else
		//该目录下文件都找完了，开始查找目录
		{
			while (1)		//find dir
			{
			    if (pstDiskFindControl->find_begin_dir == 0)  
			    	p_dir_pos=sfn_findfirst(pstDiskFindControl, dsk, "*", &dir_info, D_DIR,NULL);
			    else
			    	p_dir_pos=sfn_findnext(pstDiskFindControl, dsk, "*", &dir_info, D_DIR,NULL);
			      
			    //如果找到目录，则进入该目录
			    if (p_dir_pos != NULL)
			    {
			        //排除目录"."和".."的情况
			        if ( dir_info.f_name[0] == '.')
			        {
			        	pstDiskFindControl->find_begin_dir = 1;
			        	continue;
			        }

					//当大于程序限定的目录深度时，不进入该子目录
					if(pstDiskFindControl->level >= C_MAX_DIR_LEVEL - 1)
						continue;			        

					//获取当前目录的handle
					sfn_getcurpos(pstDiskFindControl, &cur_pos);
					sfn_setcurdirpos(pstDiskFindControl, p_dir_pos);				//保存当前文件所在的目录的f_pos

					p_dir_pos = sfn_changedir(pstDiskFindControl, p_dir_pos);		//进入子目录，应判断返回值
					//进入子目录失败，出错了，查找结束
					if (p_dir_pos == NULL)
					{
						//gFS_errno = EFLG_DIR; 					//目录有问题
						//return NULL;
						pstDiskFindControl->find_begin_dir = 1;
			        	continue;
					}
			        
			        pstDiskFindControl->dir_change_flag = 1;
			        //进入子目录成功，保存上层目录的偏移值，
			        //以保证下回返回上层目录时可以继续往下查找
					pstDiskFindControl->dir_offset[pstDiskFindControl->level] = cur_pos.f_offset;
				#if WITHEXFAT == 1
					pstDiskFindControl->FatherClu[pstDiskFindControl->level] = cur_pos.f_entry;
				#endif
					if(speedup)
					{
						pstDiskFindControl->cluster[pstDiskFindControl->level] = pstDiskFindControl->sfn_cluster;
						pstDiskFindControl->cluster_offset[pstDiskFindControl->level] = pstDiskFindControl->sfn_cluster_offset;
					}
					pstDiskFindControl->level++;
			        
			        // 080321 add for disk find speedup
				  	if(speedup)
				  	{
				  		pstDiskFindControl->sfn_cluster = pstDiskFindControl->cluster[pstDiskFindControl->level];
						pstDiskFindControl->sfn_cluster_offset = pstDiskFindControl->cluster_offset[pstDiskFindControl->level];
				  	}
				  	else
				  	{
				  		pstDiskFindControl->sfn_cluster = 0;
				  		pstDiskFindControl->sfn_cluster_offset = 0;
				  	}
				  	// 080321 add end
			        		        
			        //接着开始查找子目录下的文件
			        pstDiskFindControl->find_begin_file = 0;
			        pstDiskFindControl->find_begin_dir = 0;
			        break;
			    }    
			    else
			    //目录都找完了，则返回上一层目录
			    {
					//如果当前是根目录，则表示全部都查找完了
					if (pstDiskFindControl->level == 0)
					{
						gFS_errno = 0;
						return NULL;
					}
					
					//获取当前目录的handle
			        sfn_getcurpos(pstDiskFindControl, &cur_pos);			        
					p_cur_pos = sfn_fatherdir(pstDiskFindControl, &cur_pos);	//from current pos to father dir
					//无法返回上层目录，出错了，查找结束
					if (p_cur_pos == NULL)
					{
						gFS_errno = 0;
						return NULL;
					}
					
					//恢复上一层的目录深度
					pstDiskFindControl->level--;
					sfn_getcurpos(pstDiskFindControl, &cur_pos);
					cur_pos.f_offset = pstDiskFindControl->dir_offset[pstDiskFindControl->level];
					sfn_setcurpos(pstDiskFindControl, &cur_pos);
					pstDiskFindControl->dir_change_flag = 0;
					
					// 080321 add for disk find speedup
					pstDiskFindControl->sfn_cluster = 0;
					pstDiskFindControl->sfn_cluster_offset = 0;
				  	// 080321 add end
					
					pstDiskFindControl->find_begin_dir = 1;
			    }
			}//while (1)		//find dir
		}
	}//while (1)		//find file
}

#if 0
f_ppos fs_getfirstfile_1(STDiskFindControl *pstDiskFindControl, INT16S dsk, INT8S *extname, struct sfn_info* f_info, INT16S attr,INT32S (*filter)(INT8S *str))
{
	pstDiskFindControl->level = 0;							// 从根目录开始找起
	pstDiskFindControl->find_begin_file = 0;					// 第一次先调用findfirst
	pstDiskFindControl->find_begin_dir = 0;						// 第一次先调用findfirst
	
	pstDiskFindControl->sfn_cluster = 0;
	pstDiskFindControl->sfn_cluster_offset = 0;
	
	memset(pstDiskFindControl->dir_offset, 0, 16 * sizeof(INT16U));
	memset(pstDiskFindControl->cluster, 0, 16 * sizeof(INT32U));
	memset(pstDiskFindControl->cluster_offset, 0, 16 * sizeof(INT32U));
	
	sfn_changedisk(pstDiskFindControl, dsk);
	sfn_setcurdirpos(pstDiskFindControl, &pstDiskFindControl->cur_pos);			// 当前所在目录为根目录;		
	return fs_getnextfile_1(pstDiskFindControl, dsk, extname, f_info, attr,filter);
}

// 080321 add for disk find speedup
f_ppos fs_getnextfile_1(STDiskFindControl *pstDiskFindControl, INT16S dsk, INT8S * extname, struct sfn_info* f_info, INT16S attr,INT32S (*filter)(INT8S *str))
{	
	struct	sfn_info dir_info;
	f_pos	*p_file_pos, *p_dir_pos;
	f_pos	cur_pos, *p_cur_pos;
	
	while (1)		//find file
	{
		//先查找文件
		if (pstDiskFindControl->find_begin_file == 0)
		    p_file_pos=sfn_findfirst(pstDiskFindControl, dsk, (CHAR *) extname, f_info, attr, filter);
		else
		    p_file_pos=sfn_findnext(pstDiskFindControl, dsk, (CHAR *) extname, f_info, attr, filter);
		    
		//如果找到文件，则返回此文件的句柄
		if (p_file_pos != NULL)	
		{    
		    pstDiskFindControl->find_begin_file = 1;			//下一次调用findnext
		    //if ( (f_info->f_attrib & D_DIR) == D_DIR )	//070423 当查找的属性为D_ALL时，不返回目录节点
		    if ((CHAR)f_info->f_name[0] == '.')		//070423 当查找的属性为D_ALL时，不返回"."和".."节点
		    	continue;
		    
		    return p_file_pos; 
		}
		else
		//该目录下文件都找完了，开始查找目录
		{
			while (1)		//find dir
			{ 
			    if (pstDiskFindControl->find_begin_dir == 0)  
			    	p_dir_pos=sfn_findfirst(pstDiskFindControl, dsk, "*", &dir_info, D_DIR,NULL);
			    else
			    	p_dir_pos=sfn_findnext(pstDiskFindControl, dsk, "*", &dir_info, D_DIR,NULL);
			      
			    //如果找到目录，则进入该目录
			    if (p_dir_pos != NULL)
			    {
			        //排除目录"."和".."的情况
			        if ( dir_info.f_name[0] == '.' )
			        {
			        	pstDiskFindControl->find_begin_dir = 1;
			        	continue;
			        }

					//当大于程序限定的目录深度时，不进入该子目录
					if(pstDiskFindControl->level >= C_MAX_DIR_LEVEL - 1)
						continue;			        

					//获取当前目录的handle
					sfn_getcurpos(pstDiskFindControl, &cur_pos);
					sfn_setcurdirpos(pstDiskFindControl, p_dir_pos);				//保存当前文件所在的目录的f_pos

					p_dir_pos = sfn_changedir(pstDiskFindControl, p_dir_pos);		//进入子目录，应判断返回值
					//进入子目录失败，出错了，查找结束
					if (p_dir_pos == NULL)
					{
						gFS_errno = EFLG_DIR; 					//目录有问题
						return NULL;
					}
			        
			        //进入子目录成功，保存上层目录的偏移值，
			        //以保证下回返回上层目录时可以继续往下查找
					pstDiskFindControl->dir_offset[pstDiskFindControl->level] = cur_pos.f_offset;
					pstDiskFindControl->cluster[pstDiskFindControl->level] = pstDiskFindControl->sfn_cluster;
					pstDiskFindControl->cluster_offset[pstDiskFindControl->level] = pstDiskFindControl->sfn_cluster_offset;
					pstDiskFindControl->level++;	
			        
			        // 080321 add for disk find speedup
				  	pstDiskFindControl->sfn_cluster = 0;
				  	pstDiskFindControl->sfn_cluster_offset = 0;
				  	// 080321 add end
			        		        
			        //接着开始查找子目录下的文件
			        pstDiskFindControl->find_begin_file = 0;
			        pstDiskFindControl->find_begin_dir = 0;
			        break;
			    }    
			    else
			    //目录都找完了，则返回上一层目录
			    {
					//如果当前是根目录，则表示全部都查找完了
					if (pstDiskFindControl->level == 0)
					{
						gFS_errno = 0;
						return NULL;
					}
					
					//获取当前目录的handle
			        sfn_getcurpos(pstDiskFindControl, &cur_pos);			        
					p_cur_pos = sfn_fatherdir(pstDiskFindControl, &cur_pos);	//from current pos to father dir
					//无法返回上层目录，出错了，查找结束
					if (p_cur_pos == NULL)
					{
						gFS_errno = 0;
						return NULL;
					}
					
					//恢复上一层的目录深度
					pstDiskFindControl->level--;
					sfn_getcurpos(pstDiskFindControl, &cur_pos);
					cur_pos.f_offset = pstDiskFindControl->dir_offset[pstDiskFindControl->level];
					sfn_setcurpos(pstDiskFindControl, &cur_pos);
					
					// 080321 add for disk find speedup
					pstDiskFindControl->sfn_cluster = pstDiskFindControl->cluster[pstDiskFindControl->level];
					pstDiskFindControl->sfn_cluster_offset = pstDiskFindControl->cluster_offset[pstDiskFindControl->level];
				  	// 080321 add end
					
					pstDiskFindControl->find_begin_dir = 1;
			    }
			}//while (1)		//find dir
		}
	}//while (1)		//find file
}
#endif
#endif
