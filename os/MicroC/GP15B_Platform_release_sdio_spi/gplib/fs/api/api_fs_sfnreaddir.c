/************************************************************************/
/* 
	读出一个短目录项的内容
 */
/************************************************************************/
#include "fsystem.h"
#include "globals.h"

#if FIND_EXT == 1
INT16S sfn_readdir(f_node_ptr Dir)
{
	tDirEntry	DirEntry;	
//Don't find a better way.(- -)
#if WITHEXFAT == 1
	INT8U 		BufferPos=0,DirAmount=0,Fflag=0,NameLong,SecondFlag;

	#if MALLOC_USE == 1
		tFatFind*		Ff;
	#elif USE_GLOBAL_VAR == 1
		tFatFind		*Ff = &gFS_sFindData;
	#else
		STATIC tFatFind sFindData;
		tFatFind*		Ff = &sFindData;
	#endif
	
	INT16U				*LongNameUtf16;
	#if MALLOC_USE == 1
		tDirEntry			*Buffer = NULL;  
		tDirEntry			*pBuffer = NULL; 
	#elif USE_GLOBAL_VAR == 1
		tDirEntry*			Buffer = (tDirEntry*)gFS_slots;
	#else
		STATIC tDirEntry	Buffer[LFN_FETCH_SLOTS];
	#endif


	#if MALLOC_USE == 1
		Buffer = (tDirEntry*)FS_MEM_ALLOC(LFN_FETCH_SLOTS*sizeof(tDirEntry));
		if (Buffer==NULL)
			return DE_NOMEM;
		
		Ff = (INT8S *)FS_MEM_ALLOC(sizeof(tFatFind));	
		if (Ff ==NULL){
			return DE_NOMEM;
			FS_MEM_FREE(Buffer);
		}
	#endif
	LongNameUtf16 = (INT16U *)Ff->LongName;
	memset(Ff->LongName,0,FD32_LFNPMAX << (1 - WORD_PACK_VALUE));
#endif


	while (dir_lfnread(Dir, &DirEntry)== DIRENT_SIZE)
	{   	
		/* If the directory name starts with FAT_DIR_ENFOFDIR */
		/* we don't need to scan further.                     */
		if (DirEntry.Name[0] == ENDOFDIR)
		{
			//在非跟目录的情况下，当遇到目录的结束项的时候，需要
			//跳的上一层目录
			//可以判断一个flag，以决定是否返回上一层目录
			return FD32_ENMFILE;
		}
		
	#if WITHEXFAT == 1
		if(ISEXFAT(Dir->f_dpb))
		{
			if ((INT8U)(DirEntry.Name[0])&0x80== ENDOFDIR)
			{
				Dir->f_diroff += 1;	
				continue;
			}				

			memcpy((void *)&Buffer[BufferPos],(void *)&DirEntry,32);

			if((INT8U)(Buffer[BufferPos].Name[0])==0x85)
			{
				ExFATMainDIR *MainDiR = (ExFATMainDIR *)&Buffer[BufferPos]; 
					
				Fflag = 1;
				BufferPos++;
				DirAmount = MainDiR->SecondaryCount;
				Dir->f_diroff ++;
			}
			else if((INT8U)(Buffer[BufferPos].Name[0])==0xC0)
			{	
				ExFATExtDIR *ExtDTIR=(ExFATExtDIR *)&Buffer[BufferPos]; 			
				if(Fflag == 1)
				{				
					BufferPos++;	
					NameLong=ExtDTIR->NameLength;
					SecondFlag=ExtDTIR->SecondaryFlags;
					DirAmount--;
				}
				Dir->f_diroff ++;
			}
			else if((INT8U)(Buffer[BufferPos].Name[0])==0xC1)
			{
				ExFATNameDIR *NameDIR;
				INT8U Tempbuff[8],i;
				if(Fflag == 1)
				{
					DirAmount--;					
					if(DirAmount==0)
					{
						memcpy((void *)&Dir->f_dir,(void *)&Buffer[1],32);
						//交换顺序	为了方便Atrr放下
						memcpy((void *)Tempbuff,(void *)&Dir->f_dir.dir_name[8],8);	
						memcpy((void *)&Dir->f_dir.dir_case,(void *)Tempbuff,8); 				
						Dir->f_dir.dir_attrib=Buffer[0].Name[4];
						memcpy((void *)&Dir->extmain_dir,(void *)&Buffer[0],32);
						Dir->ext_dir = (ExFATExtDIR *)&Dir->f_dir;
						//目录文件
						Dir->ShortName[11] = 0;
						for(i=2;i<=BufferPos;i++)
						{
							NameDIR=(ExFATNameDIR *)&Buffer[i];
							memcpy((void*)(LongNameUtf16+15*(i-2)),(void *)NameDIR->FileName,30);
						}			
						memset(Dir->ShortName,' ',8);
						fd32_utf16to8_limit((UTF16 *) Ff->LongName,(UTF8 * ) Ff->ShortName,  36);
						utf8_to_oemcp((UTF8 *) Ff->ShortName, -1, (INT8S *)Dir->ShortName, 11);
						
						//过滤 . .. 
						if(Dir->ShortName[0] == '.')
						{
							if(LongNameUtf16[1] != 0)
							{
								if(!((LongNameUtf16[1]==(INT16U)'.')&&(LongNameUtf16[2]==0)))
								{
									Dir->ShortName[0] = '_';
								}
							}
						}
					
						
						for(i=0;i<11;i++)
						{
							if(Dir->ShortName[i] == 0)
							{
								memset((void *)&Dir->ShortName[i],' ',11-i);
								break;
							}
						}
						
						if((Dir->f_dir.dir_attrib&D_ARCHIVE) == 0)
						{
							memset(Dir->ShortName+8,' ',3);
						}
						else
						{
							
							for(i=NameLong;i>0;i--)
							{
								if((INT8U)(*(LongNameUtf16+i-1)) == '.')
								{
									break;
								}								
							}
							if(i==0)
							{
								memset(Dir->ShortName+8,' ',3);
							}
							else
							{
								Dir->ShortName[8] = (INT8U)(*(LongNameUtf16+i))?(INT8U)(*(LongNameUtf16+i)):' ';
								Dir->ShortName[9] = (INT8U)(*(LongNameUtf16+i+1))?(INT8U)(*(LongNameUtf16+i+1)):' ';
								Dir->ShortName[10] = (INT8U)(*(LongNameUtf16+i+2))?(INT8U)(*(LongNameUtf16+i+2)):' ';
							}
						}
						Dir->f_diroff=(Dir->f_diroff)-BufferPos+2;	
						BufferPos = 0;
						Fflag = 0;
						SecondFlag=0;

						#if WITHEXFAT == 1
						  #if MALLOC_USE == 1
							FS_MEM_FREE((INT16U *)Buffer); 		 //add by zhangyan
							FS_MEM_FREE((INT16U *)Ff);	
						  #endif
						#endif
							return 0;
					}
					BufferPos++;
				}
				Dir->f_diroff ++;				
			}
			else
			{
				Dir->f_diroff += 1;
				BufferPos = 0;
				Fflag = 0;	
				DirAmount=0;
				SecondFlag=0;
				continue;			
			}	
		}
		else
	#endif
		{
			//遇到删除项
			if ((INT8U)(DirEntry.Name[0]) == FREEENT)
			{
				Dir->f_diroff += 1;
				continue;
			}
					
			//短目录项
			if (DirEntry.Attr != FD32_ALNGNAM)
			{						
				Dir->f_diroff += 1;
				memcpy(&Dir->f_dir, (void*)&DirEntry, sizeof(tDirEntry));
				return 0;
			}
			//其它目录项（长目录项等）
			else
			{
				Dir->f_diroff++;
			}
		}

		
	} /* while (dir_lfnread(Dir, &DirEntry)== DIRENT_SIZE) */

#if WITHEXFAT == 1
  #if MALLOC_USE == 1
	FS_MEM_FREE((INT16U *)Buffer); 		 //add by zhangyan
	FS_MEM_FREE((INT16U *)Ff);	
  #endif
#endif
	return FD32_ENMFILE;
}
#endif
