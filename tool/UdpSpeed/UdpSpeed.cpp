// UdpSpeed.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "UdpSpeed.h"
#include <afxsock.h>
#include <stdio.h>
#include <windows.h>
#include <winbase.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

#define VERSION  "UdpSpeed 1.0.03"

CWinApp theApp;

using namespace std;

static m_sendTime = 60;//second
UINT SendDataThreadProc( LPVOID pParam )
{
	SOCKET sock = (SOCKET)pParam;
	struct sockaddr_in desAddr;
	UINT printCount=0;
	unsigned __int16 seq;
	char data[20];
	int dataLen;
	char * DATA_MSG="hello";
	int pkgCount=0;
	
	memset(&desAddr, 0, sizeof(desAddr));
	desAddr.sin_family = AF_INET;
	desAddr.sin_port = htons(4800);
	desAddr.sin_addr.s_addr =inet_addr("192.168.1.1");
	
	seq = 0;
	dataLen = 0;
	memcpy(data, &seq, sizeof(seq));
	dataLen = sizeof(seq);

	sprintf(data+dataLen, DATA_MSG);;
	dataLen +=strlen(DATA_MSG);
	int tick1 = GetTickCount();
	while(1)
	{
		sendto(sock, data,dataLen,0, (sockaddr *)&desAddr, sizeof(desAddr));
		Sleep(40);
		printCount ++;
		
		seq ++;
		pkgCount ++;
		memcpy(data, &seq, sizeof(seq));
		if (pkgCount % 100 ==0)
			printf("send %d pkg\n",pkgCount);
		int tick2 = GetTickCount();
		if (tick2 - tick1 >= m_sendTime * 1000)
			break;
		
	}

	printf("send end,total pkg amount=%d\n",pkgCount) ;
	return 0;
}


int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;
    SYSTEMTIME st;
    FILE *pFile=fopen(".\\log.txt","w+");
    if (!pFile)
    {
		cerr << _T("open log.txt failed") << endl;
        return -1;
    }
    
    char logstr[128];

    
    GetLocalTime(&st);
    sprintf(logstr,"\r\n[ %4d-%2d-%2d %2d:%2d:%2d.%d ] begin \r\n", st.wYear,st.wMonth,st.wDay, st.wHour,st.wMinute,st.wSecond, st.wMilliseconds);
    fwrite(logstr,1,strlen(logstr),pFile);
    fflush(pFile);
    
	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		cerr << _T("Fatal Error: MFC initialization failed") << endl;
		nRetCode = 1;
	}
	else
	{
		printf("%s\n\n", VERSION);

		if (argc >=2)
		{

			m_sendTime = atoi(argv[1]);
		}
		byte dataBuf[128*1024];
		int err;
		WORD wVer;
		WSADATA wsaData;
		wVer=MAKEWORD(1,1); 
		err=WSAStartup(wVer,&wsaData);//�ж�Windows sockets dll�汾



		struct sockaddr_in localAddr;
		struct sockaddr_in remoteAddr;
		int addrLen = sizeof(remoteAddr);
		int ret;

		//����socket , �ڷ����߳��ﶨʱ������
		SOCKET sendSock = socket(AF_INET,SOCK_DGRAM,0);
		if (sendSock == INVALID_SOCKET)
		{
			printf("socket error\n)");
			return -1;
			
		}
		
		memset(&localAddr, 0, sizeof(localAddr));
		localAddr.sin_family = AF_INET;
		localAddr.sin_port = htons(4800);
		localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		
		ret = bind(sendSock, (sockaddr *)&localAddr, sizeof(localAddr));
		if (ret !=0)
		{
			printf("bind error");
			closesocket(sendSock);
			return -1;
		}
		AfxBeginThread(SendDataThreadProc , (LPVOID)sendSock);

		SOCKET handle ;
		fd_set fd_in;
		struct timeval stTimeOut;
		int rcvLen;
		UINT tick1,tick2,tickBegin; 
		unsigned __int16 seq,preSeq;
		UINT  lossCount=0,lossSpeedCount=0,tolCount=0 , speedCount=0;
		UINT  errCount=0;




		while(1)
		{
			//�������ڽ��յ�socket
			handle = socket(AF_INET,SOCK_DGRAM,0);
			if (handle == INVALID_SOCKET)
			{
				printf("socket error\n)");
				return -1;
				
			}
			
			memset(&localAddr, 0, sizeof(localAddr));
			localAddr.sin_family = AF_INET;
			localAddr.sin_port = htons(4801);
			localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
			
			ret = bind(handle, (sockaddr *)&localAddr, sizeof(localAddr));
			if (ret !=0)
			{
				printf("bind error");
				closesocket(handle);
				return -1;
			}
			
			
			stTimeOut.tv_sec = 3;
			stTimeOut.tv_usec = 0;
			
			fd_in.fd_count = 1;
			fd_in.fd_array[0] = handle;

			tickBegin = tick2 = tick1 = GetTickCount();
			lossCount = lossSpeedCount = tolCount = speedCount=0;
			errCount=0;

			printf("begin receive:\n") ;
			for(int i=0 ;;i++)
			{
				if (i!=0)
				{
					ret = select(0,&fd_in,NULL,NULL,&stTimeOut);
					if (ret <=0)
					{
						printf("select error=%d\n",ret);
						break;
					}
				}
				rcvLen = recvfrom(handle,(char*)dataBuf,sizeof(dataBuf),0 ,(sockaddr *)&remoteAddr, &addrLen );
				if (rcvLen <=0)
				{
					printf("recv error\n");
					break;
				}
				//��ʼ������, ʱ�䲻׼ȷ
				if (i==0 )
				{
					tickBegin = tick2 = tick1 = GetTickCount();

				}
					
				tolCount ++;
				speedCount ++;
				memcpy(&seq,dataBuf, sizeof(seq));
                
                GetLocalTime(&st);
                
                sprintf(logstr,"[ %4d-%2d-%2d %2d:%2d:%2d.%d ] recv seq = %08u\r\n", st.wYear,st.wMonth,st.wDay, st.wHour,st.wMinute,st.wSecond, st.wMilliseconds, 
                    seq);
                fwrite(logstr,1,strlen(logstr),pFile);
                fflush(pFile);
				if (i!=0)
				{
					int diff = (seq - preSeq +65536) % 65536 ;
					if (diff >=60000) //����ȷ˳��İ�
					{
						errCount ++;
						printf("seq err.pre=%d,cur=%d\n",preSeq,seq);
						continue; //��������ȷ�İ�
							
					}else	if (diff > 1)
					{
						lossCount += diff-1;
						lossSpeedCount += diff-1;
						//printf("loss, cur=%d,pre=%d,l/r=%u/%u\n"
						//	,seq,preSeq, lossCount,tolCount);
						
                        sprintf(logstr,"[ %4d-%2d-%2d %2d:%2d:%2d.%d ] recv seq = %08u [last %08u], loss %u\r\n", st.wYear,st.wMonth,st.wDay, st.wHour,st.wMinute,st.wSecond, st.wMilliseconds, 
                            seq, preSeq, lossCount);
                        fwrite(logstr,1,strlen(logstr),pFile);
                        fflush(pFile);
                        
						printf("loss %u\n", lossCount);

					}
				}else
					printf("first packet,seq =%d\n",seq);

				tick2 = GetTickCount();
				if (tick2 - tick1 >=1000)
				{
					printf("r spd=%dk/s,loss=%f \n"
						, rcvLen / 1024 * speedCount /((tick2-tick1)/1000)
						, (double)(lossSpeedCount) / (lossSpeedCount+speedCount) ) ;
					tick1 = tick2;
					speedCount =0 ;
					lossSpeedCount =0;
				}
				if (tolCount % 1000 ==0)
				{
					printf("r %d\n",tolCount);
				}
				preSeq = seq;

			}//for end

			closesocket(handle);
			if (tick2 - tickBegin >=1000)
				printf("exit, time=%u(ms) , loss pkg =%u , unnormal pkg(skipped) = %u , receive pkg=%u "
					   " , speed=%uk/s ,loss=%f\n"
					,tick2-tickBegin, lossCount,errCount,tolCount
					,  rcvLen / 1024 * tolCount /((tick2 - tickBegin) /1000)
					, (double)lossCount / (lossCount + tolCount)   );
			else
				printf("exit fast!\n");
		}//while end


	}

    GetLocalTime(&st);
    sprintf(logstr,"\r\n[ %4d-%2d-%2d %2d:%2d:%2d.%d ] end === \r\n", st.wYear,st.wMonth,st.wDay, st.wHour,st.wMinute,st.wSecond, st.wMilliseconds);
    fwrite(logstr,1,strlen(logstr),pFile);
    fflush(pFile);

    fclose(pFile);

	return nRetCode;
}


