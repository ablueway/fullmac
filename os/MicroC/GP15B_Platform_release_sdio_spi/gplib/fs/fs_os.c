#include "fsystem.h"


#if _OPERATING_SYSTEM != _OS_NONE
OS_EVENT *gFS_sem;
#endif

INT32S FS_OS_Init(void) 
{
  #if _OPERATING_SYSTEM != _OS_NONE
    gFS_sem = OSSemCreate(1);
  #endif

    return (0);
}

INT32S FS_OS_Exit(void) 
{
    return (0);
}

void FS_OS_LOCK(void)
{
  #if _OPERATING_SYSTEM != _OS_NONE
	INT8U err = NULL;
	
	OSSemPend(gFS_sem, 0, &err);
  #endif
}

void FS_OS_UNLOCK(void)
{
  #if _OPERATING_SYSTEM != _OS_NONE
	OSSemPost(gFS_sem);
  #endif
}

/****************************************************************/
/*																*/
/*			       seek speedup									*/
/*																*/
/*		  seek speedup memery management function               */
/*																*/
/****************************************************************/
INT32U FS_SeekSpeedup_Malloc(INT32U len)
{
	return (INT32U)gp_malloc(len << 1);
}

void FS_SeekSpeedup_Free(INT32U addr)
{
	gp_free((void*)addr);
}

/****************************************************************/
/*																*/
/*			       getdate function								*/
/*																*/
/*		  user write this file to assign the file system date   */
/*																*/
/****************************************************************/
void FS_OS_GetDate(dosdate_t *dd)
{
	dd->year = 2004;
	dd->month = 8;
	dd->monthday = 23;
}

/****************************************************************/
/*																*/
/*			       gettime function								*/
/*																*/
/*		  user write this file to assign the file system time   */
/*																*/
/****************************************************************/
void FS_OS_GetTime(dostime_t *dt)
{
	dt->hour = 16;
	dt->minute = 54;
	dt->second = 37;
	dt->hundredth = 0;
}


