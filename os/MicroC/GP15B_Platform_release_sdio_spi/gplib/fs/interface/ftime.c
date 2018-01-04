#include "fsystem.h"
#include "globals.h"

INT8S * time_decode(INT16U *tp, CHAR *timec)
{
	dostime_t t;
	t.hour = (*tp)>>11;
	t.minute = ((*tp)>>5)& 0x3f;
	t.second = ((*tp)&0x1f)<<1;
	
	*(timec+0) = (t.hour/10) + '0';
	*(timec+1) = (t.hour%10) + '0';
	*(timec+2) = ':';
	*(timec+3) = (t.minute/10) + '0';
	*(timec+4) = (t.minute%10) + '0';
	*(timec+5) = ':';
	*(timec+6) = (t.second/10) + '0';
	*(timec+7) = (t.second%10) + '0';
	*(timec+8) = '\0';
	return (INT8S *)timec;
}

INT8S * date_decode(INT16U *dp, CHAR *datec)
{
	dosdate_t d;
	INT16U t ;

	d.year = ((*dp)>>9)&0x7f;
	d.year +=1980;
	d.month =((*dp)>>5)&0x0f;
	d.monthday = (*dp) &0x1f;

	t = d.year ;
	*(datec+0) = (t/1000) + '0';
	t = t - (t/1000)*1000;
	*(datec+1) = (t/100) + '0';
	t = t - (t/100)*100;
	*(datec+2) = (t/10) + '0';
	*(datec+3) = (t%10) + '0';
	*(datec+4) = '-';
	*(datec+5) = (d.month/10) + '0';
	*(datec+6) = (d.month%10) + '0';
	*(datec+7) = '-';
	*(datec+8) = (d.monthday/10) + '0';
	*(datec+9) = (d.monthday%10) + '0';
	*(datec+10) = '\0';
	return (INT8S *)datec;
}

INT8S *ctime(INT32U *clock, INT8S *ctime)
{
	INT16U dp;
	INT16U tp;
	CHAR datec[11], timec[11];
	
	dp = (*clock) & 0xffff;
	tp = ((*clock)>>16) & 0xffff;
	date_decode(&dp,datec);
	time_decode(&tp,timec);
	strcpy((CHAR *) ctime, (CHAR *) datec);
	strcat((CHAR *) ctime, " ");
	strcat((CHAR *) ctime, (CHAR *) timec);
	return (INT8S *)ctime;
}
