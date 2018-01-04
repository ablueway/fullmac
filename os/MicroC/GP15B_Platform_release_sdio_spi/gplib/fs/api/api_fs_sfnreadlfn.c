/************************************************************************/
/* 
	open file
	
	zhangzha creat @2006.09.18
 */
/************************************************************************/
#include "fsystem.h"
#include "globals.h"

#if FIND_EXT == 1
INT16S sfn_readlfn(f_node_ptr Dir, tFatFind *Ff)
{
	INT16U				i;
	INT16S				BufferPos;
	INT16S				flag = 0;
	
	INT16U				*punicode;
	INT16S				*LongNameUtf16 = (INT16S *)Ff->LongName;	// 减少内存的申请

#if MALLOC_USE == 1
	tDirEntry  			*Buffer = NULL;  
#elif USE_GLOBAL_VAR == 1
	tDirEntry*			Buffer = (tDirEntry*)gFS_slots;
#else
	STATIC tDirEntry	Buffer[LFN_FETCH_SLOTS];
#endif
	
#if WITHEXFAT == 1
	INT8U NameLong=0;
	INT8U DirAmount=0;
	INT8U SecondFlag=0;
	if(ISEXFAT(Dir->f_dpb))
	{
		BufferPos=0;
	}
	else
#endif
		BufferPos = LFN_FETCH_SLOTS - 1; 		//先读到最后一项里面

#if MALLOC_USE == 1
		Buffer = (INT8S *)FS_MEM_ALLOC(LFN_FETCH_SLOTS * sizeof(tDirEntry));
		if (Buffer==NULL)
			return DE_NOMEM;
#endif

		
	while (dir_lfnread(Dir, &Buffer[BufferPos])== DIRENT_SIZE)
	{   	
		/* If the directory name starts with FAT_DIR_ENFOFDIR */
		/* we don't need to scan further.                     */
		if (Buffer[BufferPos].Name[0] == ENDOFDIR)
		{
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)Buffer); 
		#endif
			return FD32_ENMFILE;
		}
		
		//遇到删除项
		if(ISEXFAT(Dir->f_dpb))
		{
			if ((INT8U)(Buffer[BufferPos].Name[0])&0x80== ENDOFDIR)
			{
				Dir->f_diroff += 1;
				BufferPos = 0;
				flag = 0;	
				continue;
			}						
		}
		else
		{
			if ((INT8U)(Buffer[BufferPos].Name[0]) == FREEENT)
			{
				Dir->f_diroff += 1;
				BufferPos = 0;
				flag = 0;	
				continue;
			}
		}
				
		/* If we find a file without the Long Name attribute, it is a valid */
		/* short name. We then try to extract a long name before it.        */
		//短目录项
	
		#if WITHEXFAT == 1
		if(!ISEXFAT(Dir->f_dpb))
		{
		#endif	
			if (Buffer[BufferPos].Attr != FD32_ALNGNAM)
			{
				// 读到2次短目录项，只返回短目录项
				if (flag == 1)
				{
				#if MALLOC_USE == 1
					FS_MEM_FREE((INT16U *)Buffer); 
				#endif
					strcpy((CHAR *) Ff->LongName, (CHAR *) Ff->ShortName);		//长目录项有问题，只能用短目录项
					return 0;
				}
				
				flag = 1;
				
				Ff->SfnEntry = Buffer[BufferPos];
				
				//短文件名,且全部是小写,返回小写
				if (Buffer[BufferPos].NTRes & 0x8)
				{
					for (i = 0; i < 8; i++)
					{
						Buffer[BufferPos].Name[i] = fs_tolower(Buffer[BufferPos].Name[i]);
					}
				}
				if (Buffer[BufferPos].NTRes & 0x10)
				{
					for (i = 8; i < 11; i++)
						Buffer[BufferPos].Name[i] = fs_tolower(Buffer[BufferPos].Name[i]);
				}
				
				fd32_expand_fcb_name((INT8S*)Ff->ShortName, (INT8S*)Buffer[BufferPos].Name);
				
				if(Dir->f_diroff == 0) 
				{
				#if MALLOC_USE == 1
					FS_MEM_FREE((INT16U*)Buffer); 
				#endif
					strcpy((CHAR *)Ff->LongName , (CHAR *)Ff->ShortName);
					return 0; 
				}
				
				Dir->f_diroff--;
				BufferPos--;
				continue;	
			}
			//长目录项的结束项
			else if (Buffer[BufferPos].Name[0] & 0x40)
			{
				if (flag == 0)
				{
				#if MALLOC_USE == 1
					FS_MEM_FREE((INT16U *)Buffer); 
				#endif
					return FD32_ENMFILE;
				}
					
			#ifdef FATLFN
				Ff->LfnEntries = fetch_lfn(&Buffer[BufferPos], LFN_FETCH_SLOTS - BufferPos - 1, LongNameUtf16);
			
				if (!(Ff->LfnEntries))
				{
					strcpy((CHAR *) Ff->LongName, (CHAR *) Ff->ShortName);
				}
			#else
				Ff->LfnEntries  = 0;
				strcpy(Ff->LongName, Ff->ShortName);
			#endif 
							
				if (Ff->LfnEntries) 
				{
					punicode = (INT16U *)Buffer;
			
					for (i=0x00; ;i++)
					{
						punicode[i] = LongNameUtf16[i];
						if (LongNameUtf16[i] == 0) 
						{
							break;
						}
					}		
					fd32_utf16to8_limit((UTF16 *) Buffer, (UTF8 *) Ff->LongName, sizeof(Ff->LongName)/sizeof(Ff->LongName[0]));
				}
				
			#if MALLOC_USE == 1
				FS_MEM_FREE((INT16U *)Buffer); 
			#endif
				return 0; 
			}
			//普通的长目录项
			else
			{
				Dir->f_diroff--;
				BufferPos--;
				
				//目录超长，只返回短目录项
				if (BufferPos < 0)
				{
				#if MALLOC_USE == 1
					FS_MEM_FREE((INT16U *)Buffer); 
				#endif
					strcpy((CHAR *) Ff->LongName, (CHAR *) Ff->ShortName);		//长目录项有问题，只用短目录项
					return 0;
				}
			}
		#if WITHEXFAT == 1
		}
		else
		{
			if((INT8U)(Buffer[BufferPos].Name[0])==0x85)
			{
				ExFATMainDIR *MainDiR=(ExFATMainDIR *)&Buffer[BufferPos];				
				
				flag = 1;
				BufferPos++;
				DirAmount=MainDiR->SecondaryCount;
				Dir->f_diroff ++;				
				
			}
			else if((INT8U)(Buffer[BufferPos].Name[0])==0xC0)
			{	
				ExFATExtDIR *ExtDTIR=(ExFATExtDIR *)&Buffer[BufferPos];				
				if(flag == 1)
				{				
					BufferPos++;	
					NameLong=ExtDTIR->NameLength;
					SecondFlag=ExtDTIR->SecondaryFlags;
					DirAmount--;
					Ff->NameHash=ExtDTIR->NameHash;					
				}
				Dir->f_diroff ++;
			}
			else if((INT8U)(Buffer[BufferPos].Name[0])==0xC1)
			{
				ExFATNameDIR *NameDIR;//=(ExFATNameDIR *)Buffer[BufferPos];
				INT8U Tempbuff[8];
				if(flag == 1)
				{
					DirAmount--;					
					if(DirAmount==0)
					{
						Ff->SfnEntry = Buffer[1];
						//交换顺序  为了方便Atrr放下
						memcpy((void *)Tempbuff,(void *)&Ff->SfnEntry.Name[8],8);	
						memcpy((void *)&Ff->SfnEntry.NTRes,(void *)Tempbuff,8);					
						Ff->SfnEntry.Attr=Buffer[0].Name[4];
					    memcpy((void *)&Dir->extmain_dir,(void *)&Buffer[0],32);
						//copy file name
						for(i=2;i<=BufferPos;i++)
						{
							NameDIR=(ExFATNameDIR *)&Buffer[i];
							memcpy((void*)(LongNameUtf16+15*(i-2)),(void *)NameDIR->FileName,30);
						}			

						{
							punicode = (INT16U *)Buffer;							 
							for (i=0x00; ;i++)
							{
								punicode[i] = LongNameUtf16[i];
								if (LongNameUtf16[i] == 0) 
								{
									break;
								}
							}
							fd32_utf16to8_limit((UTF16 *) Buffer, (UTF8 *) Ff->LongName, sizeof(Ff->LongName)/sizeof(Ff->LongName[0]));
						}


							
						Dir->f_diroff=(Dir->f_diroff)-BufferPos+2;	
                        BufferPos = 0;
						flag = 0;
						SecondFlag=0;
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
				flag = 0;	
				DirAmount=0;
				SecondFlag=0;
				continue;			
			}		
			
		}
		#endif		
	} /* while (NumRead > 0) */

#if MALLOC_USE == 1
	FS_MEM_FREE((INT16U *)Buffer); 
#endif

	return FD32_ENMFILE;
}
#endif


