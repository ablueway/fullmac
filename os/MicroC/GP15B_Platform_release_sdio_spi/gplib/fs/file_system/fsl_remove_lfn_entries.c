#include "fsystem.h"
#include "globals.h"

/* Description.
 *  Remove entries with D_LFN attribute preceeding the directory entry
 *  pointed by fnp, fnode isn't modified (I hope).
 * Return value. 
 *  SUCCESS     - completed successfully.
 *  DE_BLKINVLD - error occured, fnode is released.
 * input: fnp with valid non-LFN directory entry, not equal to '..' or
 *  '.'
 */

#ifndef READ_ONLY
INT16S remove_lfn_entries(f_node_ptr fnp)
{
	INT16U original_diroff = fnp->f_diroff;
  #if WITHEXFAT == 1
  	INT8U BlackEntries[32],DirNum=0,DirCount=0;
	if(ISEXFAT(fnp->f_dpb))
	{	
		memcpy((void *)BlackEntries,(void *)fnp->ext_dir,32);
		fnp->f_diroff-=2;
	}
  #endif

	while (1) {
	  #if WITHEXFAT == 1
		if(ISEXFAT(fnp->f_dpb))
		{
			if(DirCount>DirNum)
			{
				break;
			}
			fnp->f_diroff++;	
			if (dir_read(fnp) <= 0) {
				fnp->f_diroff = original_diroff;		
				dir_close(fnp);
				return DE_BLKINVLD;
			}
			if(DirCount==0)
			{
				if(fnp->f_dir.dir_name[0]==0x85)
				{
					DirNum=fnp->f_dir.dir_name[1];
				}
				else
				{
					fnp->f_diroff = original_diroff;
					dir_close(fnp);
					return DE_BLKINVLD;
				}
			}
			else if(DirCount==1)
			{
				if(fnp->f_dir.dir_name[0]!=0xC0)
				{
					fnp->f_diroff = original_diroff;
					dir_close(fnp);
					return DE_BLKINVLD;
				}
			}
			else
			{
				if(fnp->f_dir.dir_name[0]!=0xC1)
				{
					fnp->f_diroff = original_diroff;
					dir_close(fnp);
					return DE_BLKINVLD;
				}
			}			
			DirCount++;
			fnp->f_dir.dir_name[0] &=0x7F;				
			
		}
		else
	  #endif
		{
			if (fnp->f_diroff == 0) {
				break;
			}
			fnp->f_diroff--;
			if (dir_read(fnp) <= 0) {
				fnp->f_diroff = original_diroff;		
				dir_close(fnp);
				return DE_BLKINVLD;
			}
			if (fnp->f_dir.dir_attrib != D_LFN) {
				break;
			}
			fnp->f_dir.dir_name[0] = DELETED;
		}
		
		fnp->f_flags |= F_DMOD;
	
		if (!dir_write(fnp)) {
			fnp->f_diroff = original_diroff;			
			return DE_BLKINVLD;				
		}
	}

	fnp->f_diroff = original_diroff;

#if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
		memcpy((void *)fnp->ext_dir,(void *)BlackEntries,32);
		fnp->ext_dir->EntryType&=0x7F;
	}
	else
#endif
	{
		if (dir_read(fnp) <= 0) {
			dir_close(fnp);
			return DE_BLKINVLD;
		}
	}	
	return SUCCESS;
}
#endif
