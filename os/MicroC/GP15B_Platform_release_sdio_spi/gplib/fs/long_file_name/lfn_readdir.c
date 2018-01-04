/**************************************************************************
 * FreeDOS 32 FAT Driver                                                  *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2003, Salvatore Isaja                               *
 *                                                                        *
 * This is "readdir.c" - Services to find a specified directory entry     *
 *                                                                        *
 *                                                                        *
 * This file is part of the FreeDOS 32 FAT Driver.                        *
 *                                                                        *
 * The FreeDOS 32 FAT Driver is free software; you can redistribute it    *
 * and/or modify it under the terms of the GNU General Public License     *
 * as published by the Free Software Foundation; either version 2 of the  *
 * License, or (at your option) any later version.                        *
 *                                                                        *
 * The FreeDOS 32 FAT Driver is distributed in the hope that it will be   *
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with the FreeDOS 32 FAT Driver; see the file COPYING;            *
 * if not, write to the Free Software Foundation, Inc.,                   *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/
#include "fsystem.h"
#include "globals.h"

#if LFN_FLAG == 1
/**************************************************************************/
/* Searches an open directory for files matching the file specification */
/* and the search flags.                                                */
/* On success, returns 0, fills the FindData structure.                 */
/* Returns a negative error code on failure.                            */
/**************************************************************************/

/************************************************************************/
/* 读出一个目录项的内容
input:
output:
history:
	[1/16/2005]
	在UNSP 这样的体系上，这样的程序，太消耗RAM了
	使用Ff->LongName 空间来存放 中间得到的UNICODE文件名， 减少内存的申请

	使用动态判断的方法，计算出长文件名目录项的个数，再进行内存的申请

 */
/************************************************************************/
INT16S lfn_readdir(f_node_ptr Dir, tFatFind *Ff)
{
	INT16U				i;
	INT16S				BufferPos = 0;
	INT16S				flag = 0;
	INT16U				*punicode;
	INT16S				*LongNameUtf16 = (INT16S *)Ff->LongName;	// 减少内存的申请
	INT16S				lfn_entries = LFN_FETCH_SLOTS;
#if MALLOC_USE == 1
	tDirEntry  			*Buffer = NULL;  
	tDirEntry  			*pBuffer = NULL; 
#elif USE_GLOBAL_VAR == 1
	tDirEntry*			Buffer = (tDirEntry*)gFS_slots;
#else
	STATIC tDirEntry	Buffer[LFN_FETCH_SLOTS];
#endif

#if ADD_ENTRY == 1
	REG dmatch	*dmp = &sda_tmp_dm;	//2006.7.12 add
#endif
	
#if MALLOC_USE == 1
	Buffer = (tDirEntry*)FS_MEM_ALLOC(LFN_FETCH_SLOTS*sizeof(tDirEntry));
	if (Buffer==NULL)
		return DE_NOMEM;
	lfn_entries = 1;
#endif
	
#if WITHEXFAT == 1
	INT8U NameLong=0;
	INT8U DirAmount=0;
	INT8S	AllowableAttr = (INT8S) Ff->DIRAttr;
	INT8S	RequiredAttr  = (INT8S) ((Ff->DIRAttr) >> 8);
	INT8U SecondFlag=0;
#endif
	memset(Ff->LongName,0,FD32_LFNPMAX << (1 - WORD_PACK_VALUE));

	while (dir_lfnread(Dir, &Buffer[BufferPos])== DIRENT_SIZE)
	{   	

		/* If the directory name starts with FAT_DIR_ENFOFDIR */
		/* we don't need to scan further.                     */
		if (Buffer[BufferPos].Name[0] == ENDOFDIR)
		{
		#if MALLOC_USE == 1
			FS_MEM_FREE((INT16U *)Buffer); 		 //add by zhangyan
		#endif
			return FD32_ENMFILE;
		}
		
		if ((Buffer[BufferPos].Name[0] == 0xff) &&
			(Buffer[BufferPos].Attr == 0xff) &&
			(Buffer[BufferPos].FileSize == 0xffffffff))
		{/*
		#if MALLOC_USE == 1                                   //20120712 modify by duxiatang
			FS_MEM_FREE((INT16U *) Buffer); 
		#endif
			return FD32_ENMFILE;
			*/
			Dir->f_diroff += 1;
			BufferPos = 0;
			flag = 0;	
			continue;
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
		#if WITHEXFAT == 1
		if(!ISEXFAT(Dir->f_dpb))
		{
		#endif
			/* If we find a file without the Long Name attribute, it is a valid */
			/* short name. We then try to extract a long name before it.        */
			//短目录项
			if ((Buffer[BufferPos].Attr & FD32_ALNGNAM) != FD32_ALNGNAM)
			{
				Ff->SfnEntry    = Buffer[BufferPos];
				
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
				
			#ifdef FATLFN
				Ff->LfnEntries = fetch_lfn(Buffer, BufferPos, LongNameUtf16);
			
				if (!(Ff->LfnEntries))
				{
					strcpy((CHAR *) Ff->LongName, (CHAR *) Ff->ShortName);			
				}
			#else
				Ff->LfnEntries  = 0;
				strcpy(Ff->LongName, Ff->ShortName);
			#endif
				
			#if ADD_ENTRY == 1
				if (!(Ff->LfnEntries))					//2006.7.12 add start
				{
					dmp->dm_currentfileentry=Dir->f_diroff;
				}										////2006.7.12 add end
	        #endif
				
				Dir->f_diroff += 1;				

				if (Ff->LfnEntries) 
				{
	//			#ifdef unSP
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
			//长目录项的结束项
			else if (Buffer[BufferPos].Name[0]&0x40)
			{
			#if MALLOC_USE == 1
				lfn_entries = Buffer[BufferPos].Name[0] & ~0x40;
				lfn_entries++;		// 为短文件名也保留一个	
				
				//重新申请内存，并把原来buffer的值拷贝到新buffer
				pBuffer = Buffer;
				Buffer = (tDirEntry*)FS_MEM_ALLOC(lfn_entries*sizeof(tDirEntry));
				if (Buffer==NULL)
				{
					FS_MEM_FREE((INT16U *)pBuffer); 
					return DE_NOMEM;
				}
				memcpy(Buffer, pBuffer, sizeof(tDirEntry));
				FS_MEM_FREE((INT16U *)pBuffer); 		
			#endif
				
			#if ADD_ENTRY == 1
				dmp->dm_currentfileentry=Dir->f_diroff;	//2006.7.12 add
	        #endif	

				Dir->f_diroff++;			
				if (flag == 1)
				{
					BufferPos = 0;
					flag = 0;
				}
				else
				{
					BufferPos++;
					if (BufferPos > lfn_entries-1)
					{
						BufferPos = 0;
						continue;
					}
					flag = 1;		
				}
			}
			//普通的长目录项
			else
			{
				Dir->f_diroff++;
				if(flag == 1)
				{
					BufferPos++;
					if (BufferPos > lfn_entries-1) {
						BufferPos = 0;
						continue;
					}
				}
			}
		#if WITHEXFAT == 1	
		} /* while (NumRead > 0) */
		else	//ExFAT
		{
			if((INT8U)(Buffer[BufferPos].Name[0])==0x85)
			{
				ExFATMainDIR *MainDiR=(ExFATMainDIR *)&Buffer[BufferPos];				
				if (((AllowableAttr | MainDiR->FileAttributes) == AllowableAttr)
					&& ((RequiredAttr & MainDiR->FileAttributes) == RequiredAttr))
				{
					flag = 1;
					BufferPos++;
					DirAmount=MainDiR->SecondaryCount;
					Dir->f_diroff ++;				
				}	
				else			
				{
					Dir->f_diroff += DirAmount;
					DirAmount=0;
					BufferPos = 0;
					flag = 0;
					continue;
				}
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
					if((Ff->NameHash!=ExtDTIR->NameHash)&&(Ff->NameHash!=0xffff))
					{
						Dir->f_diroff += DirAmount;
						DirAmount=0;
						BufferPos = 0;
						flag = 0;
						SecondFlag=0;
						continue;					
					}
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
					#if ADD_ENTRY == 1
						dmp->dm_currentfileentry=Dir->f_diroff-BufferPos;
					#endif
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
						
					#if MALLOC_USE == 1
                    	FS_MEM_FREE((INT16U *)Buffer); 		 //add by zhangyan
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
				flag = 0;	
				DirAmount=0;
				SecondFlag=0;
				continue;			
			}
		}
	    #endif
	}
	#if MALLOC_USE == 1
	FS_MEM_FREE((INT16U *)Buffer); 		 //add by zhangyan
	#endif

	return FD32_ENMFILE;
}

/**************************************************************************/
#endif // LFN_FLAG

