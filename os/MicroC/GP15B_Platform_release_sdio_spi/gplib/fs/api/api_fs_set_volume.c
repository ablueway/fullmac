/************************************************************************/
/* 
	set volume
	
	zhangzha creat @2009.03.10
 */
/************************************************************************/

#include "fsystem.h"
#include "globals.h"

#if SUPPORT_VOLUME_FUNCTION == 1
INT16S fat_creat_volume_name(f_node_ptr fnp, INT8S *Name, INT8S Attr);
INT16S fd32_build_volume_name(CHAR *Dest, CHAR *Source);

INT16S fs_set_volume(INT8U disk_id, INT8U *p_volume)
{
	INT16U		len;	
	f_node_ptr	fnp;
	CHAR		Dir[4] = "A:\\";
	tFatFind	*psFind = &gFS_sFindData;
	INT16S		res;

	if (disk_id >= MAX_DISK_NUM) 
	{
		return DE_INVLDPARM;
	}
	Dir[0] = 'A' + disk_id;
	
	len = gp_strlen((INT8S*)p_volume);
	if(len > 11)
	{
		return DE_NAMETOOLONG;
	}
	
	if(check_lfn((CHAR *)p_volume) != 0)
    {
	    return DE_INVLDPARM;
    }
	
	if((fnp = descend_path((CHAR *) Dir, psFind)) == NULL)
    {
        return gFS_errno;
    }

#if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{
	   fnp->f_cluster = fnp->f_dirstart;
	   fnp->f_diroff = fnp->f_cluster_offset = 0;
	   while (dir_lfnread(fnp, &psFind->SfnEntry)== DIRENT_SIZE)
	   {
		   
		   if (psFind->SfnEntry.Name[0] == ENDOFDIR)
		   {
			   dir_close(fnp);
			   return FD32_ENMFILE;
		   }				   
		   //volume
		   if((INT8U)(psFind->SfnEntry.Name[0]) == 0x83)
		   {
			  break;
		   }   

		   fnp->f_diroff += 1;		   
	   }
	}
   else
 #endif
 	{
	    /* Loop through the directory                                   */
		while (lfn_readdir(fnp, psFind) == 0)
		{
			memcpy((void*)&fnp->f_dir, (void*)&psFind->SfnEntry, sizeof(tDirEntry));
			if ( (fnp ->f_dir.dir_attrib & D_VOLID) )
		    {
				fnp ->f_diroff -= 1;
				break;
			}
		}
 	}
	res = fat_creat_volume_name(fnp, (INT8S*)p_volume, D_VOLID);
	if (res!=0) 	
	{
		release_f_node(fnp);
		return res;
	}
    
    fnp->f_flags |= F_DMOD;
    dir_close(fnp);

    return SUCCESS;
}

INT16S fat_creat_volume_name(f_node_ptr fnp, INT8S *Name, INT8S Attr)
{
	INT16S		Res;
	tDirEntry	D;
	
	/* Initialize the directory entry */
	memset((void*)&D, 0, sizeof(tDirEntry));

	init_direntry((struct dirent *)&D, Attr, FREE);
	
	/* Get the name in FCB format, wildcards not allowed */
	if (fd32_build_volume_name((CHAR *) D.Name, (CHAR *)Name)) 
		return FD32_EFORMAT;

  #if WITHEXFAT == 1
	if(ISEXFAT(fnp->f_dpb))
	{	
		INT8U NameBuff[32];
        INT8S i;
		memset(NameBuff,0,32);
		memcpy((void *)NameBuff,(void *)D.Name,11);
		memset((void*)&D, 0, 32);	
		ChartToUTF8((INT8S *)NameBuff, (UTF8 *)&D);
		memset(NameBuff,0,32);		
		fd32_utf8to16((UTF8 *)&D, (UTF16 *)&NameBuff[2]);
		
		{
			INT16U *Value = (INT16U *)&NameBuff[2];
			for(i = 10;i >= 0;i--)
			{
				if(Value[i] == 0x20)
				{
					Value[i] = 0;
				}
				else
				{
					break;
				}
			}
			NameBuff[0] = 0x83;
			NameBuff[1] = i+1; 
		}

		memcpy((void *)&D,(void *)NameBuff,32);
	}	
  #endif
	/* Write the new directory entry into the free entry */
	fnp->f_flags |= F_DMOD;

	Res = dir_lfnwrite(fnp, &D);
	if (Res < 0) return Res;
	
	memcpy((void*)&fnp->f_dir, (void*)&D, sizeof(tDirEntry));
	return 0;
}

INT16S fd32_build_volume_name(CHAR *Dest, CHAR *Source)
{
	INT16S   WildCards = 0;
	INT16S   k;

	/* Name and extension have to be padded with spaces */
  	memset(Dest, 0x20, 11); 
	k = 0;
	while(k < 11 && *(Source + k))
	{
		*(Dest + k) = *(Source + k);
		k++;
	}
	
	if (Dest[0] == ' ')
	{
		return FD32_EFORMAT;
	}
	if (Dest[0] ==(CHAR) 0xE5) 
		Dest[0] = (CHAR) 0x05;
		
	for (k = 0; k < 11;)
	{
		if (Dest[k] < 0x20)		// If Dest is singed char, a value larger than 0x7F is a negative value
		{
			return FD32_EFORMAT;
		}
		switch (Dest[k])
		{
			case '"': 
			case '+': 
			case ',': 
			case '.':
			case '/': 
			case ':':
			case ';':
			case '<': 
			case '=':
			case '>': 
			case '[': 
			case '\\':
			case ']':
			case '|':
			{
				return FD32_EFORMAT;
			}
			case '?': 
				WildCards = 1;
				k++;
				break;
				
			case '*': WildCards = 1;
				if (k < 8) 
					for (; k < 8; k++) 
						Dest[k] = '?';
				else 
					for (; k < 11; k++) 
						Dest[k] = '?';
				break;
				
			default : 
				k += 1; // zza oemcp_skipchar((INT8S *) &Dest[k]);
		}
	}
	return WildCards;
}
#endif
