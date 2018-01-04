
#include "os.h"


#define	MESSAGE_DEGUG	0

#if _OPERATING_SYSTEM == 1
	#define MESSAGE_LOCK()		OSSchedLock();//{INT8U err = 0; OSSemPend(gMessage_sem , 0 , &err);}
	#define MESSAGE_UNLOCK()	OSSchedUnlock();//{OSSemPost(gMessage_sem);}
#else
	#define MESSAGE_LOCK()
	#define MESSAGE_UNLOCK()
#endif


#if	MESSAGE_DEGUG
	unsigned long gFreeCount = 0;
	unsigned long gMallocCount = 0;
#endif

STMsg gMsgContent[MESSAGE_SIZE];
OS_EVENT* hApMessageQ;

INT32U TransmitMSG(OS_EVENT* hevent,STMsg* pmsg)
{
	if(pmsg->Type == 0)
		return MSG_PARAM_ERR;

	if(OS_NO_ERR != OSQPost(hevent,pmsg)) {
		FreeMSG((STMsg*)pmsg);
		return MSG_SEND_ERR;
	}

	return MSG_NO_ERR;
}

INT32U SendMSG(OS_EVENT* hevent, INT32U type, INT32U param1)
{
	STMsg* pmsg;

	if(type == 0)
		return MSG_PARAM_ERR;

	pmsg =	MallocMSG();

	if(pmsg == 0)
		return MSG_NOMEM_ERR;

	pmsg->Type     = type;
	pmsg->Param1.Param1   = param1;
	pmsg->Param2.Param2   = 0x0;
	pmsg->Reserved = 0x0;

	if(OS_NO_ERR != OSQPost(hevent,pmsg)) {
		FreeMSG((STMsg*)pmsg);
		return MSG_SEND_ERR;
	}

	return MSG_NO_ERR;
}

INT32U SendTpMSG(OS_EVENT* hevent, INT32U type, INT32U param1, INT32U param2, INT32U tptype)
{
	STMsg* pmsg;

	if(type == 0)
		return MSG_PARAM_ERR;

	pmsg = MallocMSG();
	if(pmsg == 0)
		return MSG_NOMEM_ERR;

	pmsg->Type     = type;
	pmsg->Param1.Param1   = param1;
	pmsg->Param2.Param2   = param2;
	pmsg->Reserved = tptype;

	if(OS_NO_ERR != OSQPost(hevent,(void*)pmsg)) {
		FreeMSG(pmsg);
		return MSG_SEND_ERR;	
	}	
	
	return MSG_NO_ERR;
}

INT32U SendTpFrontMSG(OS_EVENT* hevent, INT32U type, INT32U param1, INT32U param2, INT32U tptype)
{
	STMsg* pmsg;
	
	if(type == 0)
		return MSG_SEND_ERR;
	
	pmsg = MallocMSG();
	if(pmsg == 0)
		return MSG_NOMEM_ERR;
		
	pmsg->Type     = type;
	pmsg->Param1.Param1   = param1;
	pmsg->Param2.Param2   = param2;
	pmsg->Reserved = tptype;	
	
	if(OS_NO_ERR != OSQPostFront(hevent,(void*)pmsg)) {
		FreeMSG(pmsg);
		return MSG_SEND_ERR;
	}

	return MSG_NO_ERR;
}

STMsg* GetMSG(OS_EVENT* hevent)
{
	int err = 0;	
	return (STMsg*)OSQPend (hevent,0, (INT8U*)&err);
}

/*
STMsg* GetMsgMainQ( void )
{
	return GetMSG(hMainQ);
}
*/

STMsg* MallocMSG()
{
	register STMsg *pmsg = gMsgContent;
	register INT32S i = 0;

	MESSAGE_LOCK();
	
	while( i++ < MESSAGE_SIZE ) {
		if( 0 == pmsg->Type ) {
#if	MESSAGE_DEGUG
			gMallocCount++;
#endif
			pmsg->Type = MSG_MALLOCED;
			MESSAGE_UNLOCK();
			return pmsg;
		}
		pmsg++;
	}
	
	MESSAGE_UNLOCK();
	return (STMsg*)0;
}

void FreeMSG(STMsg* pmsg)
{
	MESSAGE_LOCK();
	
#if	MESSAGE_DEGUG
	gFreeCount++;
#endif
	pmsg->Type = 0;
	
	MESSAGE_UNLOCK();
}

INT32U SendFrontMSG(OS_EVENT* hevent, INT32U type, INT32U param1)
{
	STMsg* pmsg;

	if(type == 0)
		return MSG_PARAM_ERR;

	pmsg =	MallocMSG();
	if(pmsg == 0)
		return MSG_NOMEM_ERR;

	pmsg->Type     = type;
	pmsg->Param1.Param1   = param1;
	pmsg->Param2.Param2   = 0x0;
	pmsg->Reserved = 0x0;	

	if(OS_NO_ERR != OSQPostFront(hevent,pmsg)) {
		FreeMSG((STMsg*)pmsg);
		return MSG_SEND_ERR;
	}

	return MSG_NO_ERR;

}

void MsgInit()
{
	int i;
	
	MESSAGE_LOCK();
	
	for( i=0 ; i<MESSAGE_SIZE ; i++ ) {
		gMsgContent[i].Type = 0;
	}
	
	MESSAGE_UNLOCK();
}


void FlushMsg(OS_EVENT* hevent)
{
	OS_Q_DATA data;
	STMsg* pmsg;
	INT32S i = 0;
	
	MESSAGE_LOCK();
	
	OSQQuery(hevent,&data);
	for(i = 0;i<data.OSNMsgs;i++)
	{
		pmsg = GetMSG(hevent);
		FreeMSG(pmsg);
	}
	
	MESSAGE_UNLOCK();
}
//--------------------------------------end of Message.c----------------------------------------

