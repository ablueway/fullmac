#include "fsystem.h"
#include "globals.h"



#if WITHEXFAT == 1

INT8S ExFatMainDirCheckSum(f_node_ptr fnp)
{
	struct buffer *bp;
	REG INT16U secsize = fnp->f_dpb->dpb_secsize;
	INT16U Back_diroff=fnp->f_diroff;
	INT8U DirAmount=0xff,buffcount=0; 
	INT16U CheckSum=0;
	
#if MALLOC_USE == 1
	tDirEntry			*Buffer = NULL;  
#elif USE_GLOBAL_VAR == 1
	tDirEntry*			Buffer = (tDirEntry*)gFS_slots;
#else
	STATIC tDirEntry	Buffer[LFN_FETCH_SLOTS];
#endif

#if MALLOC_USE == 1
	Buffer = (tDirEntry*)FS_MEM_ALLOC(sizeof(tDirEntry));
	if (Buffer==NULL)
		return DE_NOMEM;
#endif

	/* can't have more than 65535 directory entries */
	if (fnp->f_diroff >= 65535U)
		return DE_HNDLDSKFULL;
	fnp->f_diroff--;
	
	while(DirAmount--)
	{
		/* Do a "seek" to the directory position        */
		fnp->f_offset = fnp->f_diroff * (INT32U)DIRENT_SIZE;
		
		/* Search through the FAT to find the block     */
		/* that this entry is in.                       */

		if (map_cluster(fnp, XFR_READ) != SUCCESS)
			return DE_HNDLDSKFULL; 	

		bp = getblock_from_off(fnp, secsize);
		/* Now that we have the block for our entry, get the    */
		/* directory entry.                                     */
		if (bp == NULL)
			return DE_BLKINVLD;

		getdirent((INT8U *) & bp->b_buffer[(fnp->f_diroff * DIRENT_SIZE>>WORD_PACK_VALUE) & ((fnp->f_dpb->dpb_secsize>>WORD_PACK_VALUE)-1)],
			(struct dirent *)&Buffer[buffcount++]);
		
		bp->b_flag &= ~(BFR_DATA | BFR_FAT);
		bp->b_flag |= BFR_DIR | BFR_VALID;
		fnp->f_flags &= ~F_DMOD;
		
		if(buffcount==1)
		{
			if((INT8U)(Buffer[0].Name[0])==0x85)
			{
				ExFATMainDIR *MainDiR=(ExFATMainDIR *)&Buffer[0];				
				DirAmount=MainDiR->SecondaryCount;
			}
			else
			{
				return -1;
			}
		}
		fnp->f_diroff++;

		if(DirAmount==0)
		{
		//»ñÈ¡ count	
			{
				ExFATMainDIR *MainDiR=(ExFATMainDIR *)&Buffer[0];				
				DirAmount=MainDiR->SecondaryCount;
			}
		
			{
				INT16U NumberOfBytes = ((INT16U) DirAmount + 1) * 32;
				INT16U index;
				INT8U *Entries=(INT8U *)&Buffer[0];
		
				for (index = 0; index < NumberOfBytes; index++) {
						if ((index == 2) || (index == 3)) {
								continue;
						}
						CheckSum = ((CheckSum & 1) ? 0x8000 : 0) + (CheckSum >> 1) + (INT16U) Entries[index];
				}
			}	

			fnp->f_diroff=Back_diroff;
			fnp->f_diroff--;
			DirAmount=0xff;
		}

		if(DirAmount==0xFE)
		{
			ExFATMainDIR *MainDiR=(ExFATMainDIR *)&bp->b_buffer[((fnp->f_diroff-1) * DIRENT_SIZE>>WORD_PACK_VALUE) & ((fnp->f_dpb->dpb_secsize>>WORD_PACK_VALUE)-1)];
			MainDiR->SetChecksum=CheckSum;
			DirAmount=0;
			bp->b_flag &= ~(BFR_DATA | BFR_FAT);
			fnp->f_flags |=(BFR_DIRTY | BFR_VALID);
			fnp->f_flags |=F_DMOD;			
			break;
		}
		
	}

	fnp->f_diroff=Back_diroff;
	return 0;
}



#endif







void dir_close(f_node_ptr fnp)
{
	if (!fnp || !(fnp->f_flags&F_DDIR) || !(fnp->f_dpb->dpb_mounted)) {
		return;
	}

  #if WITHFAT32 == 1
	if ((fnp->f_dpb->dpb_fsinfoflag & FSINFODIRTYFLAG)&&(!ISEXFAT(fnp->f_dpb))) {
		write_fsinfo(fnp->f_dpb);
	}
  #endif

#ifndef READ_ONLY
	dir_write(fnp);							// Write out the entry
  #if  WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
		ExFatMainDirCheckSum(fnp);		
	}
  #endif


#endif
	flush_buffers(fnp->f_dpb->dpb_unit);	// Clear buffers after release
	
	release_f_node(fnp);					// And release this instance of the fnode
}
