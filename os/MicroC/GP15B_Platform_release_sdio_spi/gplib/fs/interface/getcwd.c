#include "fsystem.h"
#include "globals.h"

//-----------------------------------------------------
INT32U getcwd(CHAR *buffer, INT16S maxlen) 	 
{
	INT16S		i;
	CHAR		*ret;
#if MALLOC_USE == 1
	CHAR		*tmp_buf;
#elif USE_GLOBAL_VAR == 1
	CHAR		*tmp_buf = (CHAR *) gFS_path1;
#else
	CHAR		tmp_buf[FD32_LFNPMAX];	
#endif	 
	
	if (buffer && !maxlen) 	 
	{
		gFS_errno = DE_INVLDPARM;		 
		return 0;
	}	
	FS_OS_LOCK();

#if MALLOC_USE == 1
	tmp_buf = (CHAR *) FS_MEM_ALLOC(FD32_LFNPMAX); 
	if (tmp_buf == NULL)
		return DE_NOMEM;
#endif

	ret = fat_getcwd(tmp_buf, maxlen);
	if (ret != NULL) 
	{
		i = strlen(tmp_buf)+1;
		MemPackByteCopyFar((INT32U) tmp_buf<<WORD_PACK_VALUE, (INT32U) buffer<<WORD_PACK_VALUE, i<<WORD_PACK_VALUE);	 			
	}

#if MALLOC_USE == 1
	FS_MEM_FREE((INT16U *) tmp_buf); 	
#endif
	
	FS_OS_UNLOCK();	

	if (ret == NULL) 	 
	{
		gFS_errno = DE_INVLDPARM;		 
		return 0;
	}

	return (INT32U) ret;
}
