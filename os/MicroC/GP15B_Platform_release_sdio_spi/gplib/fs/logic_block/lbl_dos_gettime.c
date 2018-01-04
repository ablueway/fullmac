#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
/*                                                              */
/* dos_gettime for the file time                                */
/*                                                              */
dostime dos_gettime(void)
{
	dostime_t dt;
	
	/* First - get the system time set by either the user   */
	/* on start-up or the CMOS clock                        */
	FS_OS_GetTime(&dt);
	
	return time_encode(&dt);
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
void dostime_decode(INT16U dos_time, INT8U *phour, INT8U *pminute, INT8U *psecond)
{
	*phour = TM_HOUR(dos_time);
	*pminute = TM_MIN(dos_time);
	*psecond = TM_DEC(dos_time)*2;

	return;
}
#endif



