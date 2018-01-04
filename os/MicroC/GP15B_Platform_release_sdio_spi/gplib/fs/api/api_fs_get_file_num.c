/************************************************************************/
/* 	
	zhangzha add��
	ͳ��ĳ�����͵��ļ���������
*/
/************************************************************************/
#include "fsystem.h"
#include "globals.h"


//�����������̵��ļ���ͳ�Ƴ�ĳ�����͵��ļ�������
int fs_StatFileNumByExtName(int dsk, INT8S * extname, INT32U *filenum)
{
	f_node_ptr fnp;
	struct f_node pre_f_node;
#if WITHEXFAT == 0
	tDirEntry  DirEntry;
#endif	
	INT16U level;

#if MALLOC_USE == 1
	INT16U *dir_off = FS_MEM_ALLOC(C_MAX_DIR_LEVEL);
  #if WITHEXFAT == 1
	INT32U *Clu_off = FS_MEM_ALLOC(C_MAX_DIR_LEVEL*2);
  #endif
#else
	INT16U dir_off[C_MAX_DIR_LEVEL];
	INT32U Clu_off[C_MAX_DIR_LEVEL];
#endif
	INT16S flag = 0;
	
	INT16S ret, i;
	INT32U count = 0;
	INT8S	localextname[3];	
#if WITHEXFAT == 1
	INT8S	fileextname[3];
#endif
	/* Allocate an fnode if possible - error return (0) if not.     */
	if ((fnp = get_f_node()) == (f_node_ptr) 0)
	{
	#if MALLOC_USE == 1
		FS_MEM_FREE(dir_off); 
	#endif	
		return DE_MAX_FILE_NUM;
	}
	
	/* Force the fnode into read-write mode                         */
	fnp->f_mode = RDWR;
	
	/* Select the default to help non-drive specified path          */
	/* searches...                                                  */
	fnp->f_dpb = get_dpb(dsk);
	if (media_check(fnp->f_dpb) < 0)
	{
		release_f_node(fnp);
	#if MALLOC_USE == 1
		FS_MEM_FREE(dir_off); 
	#endif	
		return DE_MAX_FILE_NUM;
	}
	
	for (i = 0; i < 3; i++)
	{
		if (extname[i] == '\0')
		{
			for (; i < 3; i++)
				localextname[i] = ' ';
			break;
		}
		else
			localextname[i] = extname[i];
	}
		
	fs_strupr(localextname);
	
	//start from the first of root directory
	level = 0;
	fnp->f_diroff = dir_off[level] = 0;
	dir_init_fnode(fnp, level);
	
	while (1)
	{   	
#if WITHEXFAT == 1
		ret = sfn_readdir(fnp);
		if (ret == FD32_ENMFILE)
		{
			if (level == 0)
			{
				release_f_node(fnp);
			  #if MALLOC_USE == 1
				FS_MEM_FREE(dir_off); 
			  #endif				
				*filenum = count;
				return 0;
			}
			else
			//�ص���һ��Ŀ¼
			{
				//������һ����־��������fnp->f_diroffΪ1��ȥ��".."Ŀ¼���Է�����һ��Ŀ¼
				if(!ISEXFAT(fnp->f_dpb))
				{
					flag = 1;
					fnp->f_diroff = 1;
					continue;
				}
			#if WITHEXFAT == 1
				else
				{
					//������һ��Ŀ¼
					level--;
					if (level == 0)
						dir_init_fnode(fnp, level);
					else
						//ָ����һ��Ŀ¼����ʼ��ַ
						fnp->f_cluster = fnp->f_dirstart = Clu_off[level];
					//�ָ���һ��Ŀ¼��ƫ����
					fnp->f_diroff = dir_off[level];
				}
			#endif
				
			}
		}
		else
		{			
			//�����Ŀ¼��������Ŀ¼���޸�f_dirstart����Ŀ¼ָ��ĵ�ַ��
			//����f_diroff������Ϊ0
			//��һ��Ŀ¼�ĵ�ַ�����һ�����ݣ����������ǰĿ¼������Ļ���û�취��ȥ��^_^
			if ((fnp->f_dir.dir_attrib& D_DIR) == D_DIR)		
			{
				//"."
				if ( (level != 0) && (fnp->f_diroff == 1)&&(!ISEXFAT(fnp->f_dpb)))
				{
					if ( !memcmp((void*)fnp->f_dir.dir_name, ".          ", 11) )
					//ɨ���Ŀ¼��ռ�����
					{
					
					}
					//�Ƿ���Ŀ¼���ص���һ��Ŀ¼�������"."��".."�ļ���
					else
					{
						//������һ��Ŀ¼
						level--;
						//�ָ���һ��Ŀ¼��f_node
						*fnp = pre_f_node;		// Tristan: Is this correct?
					}
				}
				//".."
				else if ( (level != 0) && (fnp->f_diroff == 2)&&(!ISEXFAT(fnp->f_dpb)))
				{
					if (!memcmp((void*)fnp->f_dir.dir_name,  "..         ", 11) )
					//�����һ��Ŀ¼��fnp���ص���һ��Ŀ¼
					{
						if (flag == 1)
						{
							flag = 0;
							//������һ��Ŀ¼
							level--;
							if (level == 0)
								dir_init_fnode(fnp, level);
							else
								//ָ����һ��Ŀ¼����ʼ��ַ
								fnp->f_cluster = fnp->f_dirstart = (((INT32U) fnp->f_dir.dir_start_high<< 16) + (INT32U) fnp->f_dir.dir_start);
							//�ָ���һ��Ŀ¼��ƫ����
							fnp->f_diroff = dir_off[level];
						}
					}
					else
					//�Ƿ���Ŀ¼�����ݱ��ݵ�fnp�ص���һ��Ŀ¼��
					{
						//������һ��Ŀ¼
						level--;
						//�ָ���һ��Ŀ¼��f_node
						*fnp = pre_f_node;		// Tristan: Is this correct?
					}
				}
				else
				{
					flag = 0;
				#if WITHEXFAT == 1
					if(ISEXFAT(fnp->f_dpb)) {					
						if(level>=(C_MAX_DIR_LEVEL-1)) {						
							continue;
						}
					}
				#endif
					//������һ��Ŀ¼��ƫ����
					dir_off[level] = fnp->f_diroff;
				#if WITHEXFAT == 1
					Clu_off[level] = fnp->f_dirstart;
				#endif
					//��һ��Ŀ¼��f_node��һ������
					pre_f_node = *fnp;		// Tristan: Is this correct?
					//������һ��Ŀ¼
					level++;
					//��һ��Ŀ¼��ͷ��ʼɨ��
					fnp->f_diroff = 0;
					//ָ����һ��Ŀ¼����ʼ��ַ
				
				#if WITHEXFAT == 1
					if(ISEXFAT(fnp->f_dpb))
					{
						fnp->f_cluster = fnp->f_dirstart = fnp->ext_dir->FirstCluster;
					}
					else
				#endif
					{
						fnp->f_cluster = fnp->f_dirstart = (((INT32U) fnp->f_dir.dir_start_high<< 16) + (INT32U) fnp->f_dir.dir_start);
					}
					continue;
				}
			}
			//������ļ�����ɨ����ļ���fat���ռ�������
			else
			{
				if ( !memcmp(localextname, "*", 1) )
				{
					count++;
				}
			  #if WITHEXFAT == 1
				else{
					if(ISEXFAT(fnp->f_dpb))
					{
						memcpy((void *)fileextname,(void*)(fnp->ShortName+8),3);
						for(i=0;i<3;i++){
							fileextname[i] = fs_toupper(fileextname[i]);
						}
					}
					else
					{
						memcpy((void *)fileextname,(void*)(fnp->f_dir.dir_name+8),3);
					}
					if(!memcmp((void*)fileextname, localextname, 3)){
						count++;
					}
				}
			  #else	
				else if ( !memcmp((void*)(fnp->f_dir.dir_name+8), localextname, 3) )
				{
					count++;
				}
			  #endif	
				
				
			}
			
		}


#else	
		ret = dir_lfnread(fnp, (tDirEntry *)&DirEntry);
		if (ret == DE_BLKINVLD)
		{
			release_f_node(fnp);
		#if MALLOC_USE == 1
			FS_MEM_FREE(dir_off); 
		#endif
			
			*filenum = count;
			return FD32_ENMFILE;
		}	
			
		/* whole directory scanned end		 						*/
		if ( (ret == DE_HNDLDSKFULL) || (DirEntry.Name[0] == ENDOFDIR) )
		{
			if (level == 0)
			{
				release_f_node(fnp);
			#if MALLOC_USE == 1
				FS_MEM_FREE(dir_off); 
			#endif
				
				*filenum = count;
				return 0;
			}
			else
			//�ص���һ��Ŀ¼
			{
				//������һ����־��������fnp->f_diroffΪ1��ȥ��".."Ŀ¼���Է�����һ��Ŀ¼
				flag = 1;
				fnp->f_diroff = 1;
				continue;
			}
		}
		//����ɾ����
		else if ((INT8U)(DirEntry.Name[0]) == FREEENT)
		{
			fnp->f_diroff += 1;
			continue;
		}
		//��Ŀ¼��
		else if (DirEntry.Attr == FD32_ALNGNAM) 

		{
			fnp->f_diroff++;
			continue;
		}		
		//��Ŀ¼��
		else
		{			
			//�����Ŀ¼��������Ŀ¼���޸�f_dirstart����Ŀ¼ָ��ĵ�ַ��
			//����f_diroff������Ϊ0
			//��һ��Ŀ¼�ĵ�ַ�����һ�����ݣ����������ǰĿ¼������Ļ���û�취��ȥ��^_^
			if ((DirEntry.Attr & D_DIR) == D_DIR)		
			{
				//"."
				if ( (level != 0) && (fnp->f_diroff == 0) )
				{
					if ( !memcmp((void*)DirEntry.Name, ".          ", 11) )
					//ɨ���Ŀ¼��ռ�����
					{

					}
					//�Ƿ���Ŀ¼���ص���һ��Ŀ¼�������"."��".."�ļ���
					else
					{
						//������һ��Ŀ¼
						level--;
						//�ָ���һ��Ŀ¼��f_node
						*fnp = pre_f_node;		// Tristan: Is this correct?
					}
				}
				//".."
				else if ( (level != 0) && (fnp->f_diroff == 1) )
				{
					if (!memcmp((void*)DirEntry.Name,  "..         ", 11) )
					//�����һ��Ŀ¼��fnp���ص���һ��Ŀ¼
					{
						if (flag == 1)
						{
							flag = 0;
							//������һ��Ŀ¼
							level--;
							if (level == 0)
								dir_init_fnode(fnp, level);
							else
								//ָ����һ��Ŀ¼����ʼ��ַ
								fnp->f_cluster = fnp->f_dirstart = FIRSTCLUSTER(DirEntry);
							//�ָ���һ��Ŀ¼��ƫ����
							fnp->f_diroff = dir_off[level];
						}
					}
					else
					//�Ƿ���Ŀ¼�����ݱ��ݵ�fnp�ص���һ��Ŀ¼��
					{
						//������һ��Ŀ¼
						level--;
						//�ָ���һ��Ŀ¼��f_node
						*fnp = pre_f_node;		// Tristan: Is this correct?
					}
				}
				else
				{
					flag = 0;
					//������һ��Ŀ¼��ƫ����
					dir_off[level] = fnp->f_diroff;
					//��һ��Ŀ¼��f_node��һ������
					pre_f_node = *fnp;		// Tristan: Is this correct?
					//������һ��Ŀ¼
					level++;
					//��һ��Ŀ¼��ͷ��ʼɨ��
					fnp->f_diroff = 0;
					//ָ����һ��Ŀ¼����ʼ��ַ
					fnp->f_cluster = fnp->f_dirstart = FIRSTCLUSTER(DirEntry);
					continue;
				}
			}
			//������ļ�����ɨ����ļ���fat���ռ�������
			else
			{
				if ( !memcmp(localextname, "*", 1) )
				{
					count++;
				}
				else if ( !memcmp((void*)(DirEntry.Name+8), localextname, 3) )
				{
					count++;
				}
			}
			fnp->f_diroff += 1;		
		}	
#endif		
	} 
}


