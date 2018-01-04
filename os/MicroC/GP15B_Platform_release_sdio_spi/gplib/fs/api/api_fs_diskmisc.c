#include "fsystem.h"
#include "globals.h"

/************************************************************************/
/* 
	disk misc info；such as free size；free area cluster 
 */
/************************************************************************/

INT32U  fs_get_freeclu_misc(INT8U disk)
{
	struct dpb * dpbp;
	INT32U freeclu_ms = 0;
	struct buffer *bp;
	
	dpbp = get_dpb(disk);
	
	if (media_check(dpbp) < 0)
	{
		gFS_errno = DE_INVLDDRV;
		return NULL;
	}
#if WITHFAT32 == 1	
	if(ISFAT32(dpbp))
	{
		bp = getblock(dpbp->dpb_xfsinfosec, dpbp->dpb_unit);
	    if (bp == NULL) {
		    return 0;
	    }
		freeclu_ms = getlong(&bp->b_buffer[0x1e8>>WORD_PACK_VALUE]);
	}
#endif
	return freeclu_ms;
}


INT32U fs_get_freearea_misc(INT8U disk)
{
	struct dpb * dpbp;
	INT32U freearea_ms = 0;
	struct buffer *bp;
	
	dpbp = get_dpb(disk);
	
	if (media_check(dpbp) < 0)
	{
		gFS_errno = DE_INVLDDRV;
		return NULL;
	}
#if WITHFAT32 == 1	
	if(ISFAT32(dpbp))
	{
		bp = getblock(dpbp->dpb_xfsinfosec, dpbp->dpb_unit);
	    if (bp == NULL) {
		    return 0;
	    }
		freearea_ms = getlong(&bp->b_buffer[0x1ec>>WORD_PACK_VALUE]);
	}
#endif	
	return freearea_ms;
}


//获取fat表大小
INT32U fs_get_fatsize(INT8U disk)
{
	struct dpb * dpbp;
	INT32U fatsize;
	
	dpbp = get_dpb(disk);
	
	if (media_check(dpbp) < 0)
	{
		gFS_errno = DE_INVLDDRV;
		return NULL;
	}
#if WITHFAT32 == 1
	if(ISFAT32(dpbp))
		fatsize =  dpbp->dpb_xfatsize;
	else		
#endif
		fatsize =  dpbp->dpb_fatsize;

#if WITHEXFAT == 1
	if(ISEXFAT(dpbp))
		fatsize =  (dpbp->dpb_exmlength + 511) >> 9;
#endif
	fatsize *= dpbp->dpb_secsize;	
	return fatsize;
}


INT32U remain_size, remain_read;
INT32U fs_read_fat_frist(INT32U disk, INT32U buf, INT32U size)
{
	INT32U len, fatsize;
	struct dpb *dpbp = get_dpb(disk);
	
	
	#if WITHFAT32 == 1
		if(ISFAT32(dpbp))
			fatsize = dpbp->dpb_xfatsize;
		else		
	#endif
			fatsize = dpbp->dpb_fatsize;

	#if WITHEXFAT == 1
		if(ISEXFAT(dpbp))
			fatsize = (dpbp->dpb_exmlength + 511) >> 9;
	#endif
		fatsize *= dpbp->dpb_secsize;
	
	remain_read = 0;
	remain_size = fatsize;
	if(size > remain_size)
		len = remain_size;
	else
		len = size;
	len >>= 9;
	
	if(len == 0)
	{
		return 0;
	}
	
	if(dskxfer (dpbp ->dpb_unit,dpbp ->dpb_fatstrt+remain_read,(INT32U)buf, len, DSKREAD))
	{
		return 0;
	}
	remain_read += len;
	len <<= 9;
	remain_size -= len;
	
	return len;
}


INT32U fs_read_fat_next(INT32U disk, INT32U buf, INT32U size)
{
	INT32U len;
	struct dpb *dpbp = get_dpb(disk);

	if(size > remain_size)
		len = remain_size;
	else
		len = size;
	len >>= 9;
	
	if(len == 0)
	{
		return 0;
	}
	
	if(dskxfer (dpbp ->dpb_unit,dpbp ->dpb_fatstrt+remain_read,(INT32U)buf, len, DSKREAD))
	{
		return 0;
	}
	remain_read += len;
	len <<= 9;
	remain_size -= len;

	return len;
}
 