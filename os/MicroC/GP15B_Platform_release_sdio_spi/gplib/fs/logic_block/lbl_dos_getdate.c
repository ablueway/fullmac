#include "fsystem.h"
#include "globals.h"

#ifndef READ_ONLY
dosdate dos_getdate(void)
{
  	dosdate_t dd;
 	
  	FS_OS_GetDate(&dd);						// Get the system date
  
  	return DT_ENCODE(dd.month, dd.monthday, dd.year - EPOCH_YEAR);
}

void dosdate_decode(INT16U dos_date, INT16U *pyear, INT8U *pmonth, INT8U *pday)
{	
	*pyear = DT_YEAR(dos_date);
	*pmonth = DT_MONTH(dos_date);
	*pday = DT_DAY(dos_date);
}

#if WITHEXFAT == 1
INT32U dostimeget_ExFat(void)
{
	INT32U dostime_exfat=0;
	dosdate_t dd;
	dostime_t dt;	
	FS_OS_GetDate(&dd);						
	FS_OS_GetTime(&dt);
	
	dostime_exfat=(dt.second)/2;
	dostime_exfat|=(dt.minute)<<5;
	dostime_exfat|=(dt.hour)<<0xb;
	dostime_exfat|=(dd.monthday)<<0x10;
	dostime_exfat|=(dd.month)<<0x15;
	dostime_exfat|=(dd.year)<<0x19;

	return dostime_exfat;
}

INT8U dosms_Exfat(void)
{
	INT8U dosms_exfat=0;
	dostime_t dt;	
	FS_OS_GetTime(&dt);

	dosms_exfat=(dt.second%0x02)*100; 		//10ms

	return dosms_exfat;
}
#endif
#endif
