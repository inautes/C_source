#include <stdio.h>      /* for printf() and fprintf() */ 
#include <sys/socket.h> /* for socket(), bind(), and connect() */ 
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */ 
#include <string.h>     /* for memset() */ 
#include <unistd.h>     /* for close() sleep()*/ 
#include <errno.h> 


#include "chkdaem_socket.h"
#include "apdefine.h" //for log
#include "comcomm.h" //for log

int Connect(char* pServerIP, unsigned int nPort)
{
	int SockFD = 0;
	if(pServerIP == NULL)
	{
		#ifdef __DEBUG
		printf("Error : Connect Error , ServerIP is NULL\n");
		#endif
		
		return -2;
	}
	if(nPort <= 0 )
	{
		#ifdef __DEBUG
		printf("Error : Connect Error , Port is small than Zero\n");
		#endif
		
		return -2;		
	}
	
	
	struct sockaddr_in servaddr;
	
	memset(&servaddr,0,sizeof(servaddr));
	int nConnectStop = 0;
	
	if( (SockFD = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0)
	{
		#ifdef __DEBUG
		printf("Error : Socket Failed\n");
		#endif
		return -2;
	}
	
	
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(pServerIP);
	servaddr.sin_port        = htons(nPort);
	
	
	while(nConnectStop <= 2)
	{
		#ifdef __DEBUG
		printf("try connect\n");
		#endif
		if( connect( SockFD, (struct sockaddr * ) &servaddr,sizeof(servaddr) ) < 0 )
		{
			if( nConnectStop >= 2)
			{
				
				
				#ifdef __DEBUG
				printf("Error : Connect Failed [ %s ] [ %d ] \n",pServerIP,nPort);
				#endif 
			
				return -1;
			}
		}
		else
		{
		    break;
		}
		sleep(1);
		nConnectStop++;
		
	}

	
	if( nConnectStop >= 2)
	{
		
		#ifdef __DEBUG
		printf("Error : Connect Failed\n");
		#endif 
	
		return -1;
	}
		
	return SockFD;
}
//여기서 부터 recvdata ..함수
int RecvData(int nSockID,char* RecvBuffer,int nRecvLen)
{
	
	int iRet;
	memset(RecvBuffer,0x00,nRecvLen); //<-- 버퍼 초기화
	
	if(nSockID <0)
		return(-1);
	
	int nCount=0;
	int nRecv = nRecvLen;

	
	while(nRecv > 0) //recv 받은 데이터가 다 받을때까정..
	{
		iRet = recv(nSockID,(char*)RecvBuffer+nCount,nRecv,0); // 받기

		
		if(iRet<0) // 0보다 작으면 에러 ..win32에서는 getlasterror 로 채크 할수  있음
		{
			int err = errno;
			if(err == EINTR ) //|| err == EAGAIN) //linux 에서만...
			{
				sleep(10);
				continue;
			}
			return(-1*err); //에러 return
		}
		else
		{
			if(iRet == 0) //0이면 접속이 끊김...즉 인터넷 불통 또는 강제 종료
				return 0;
			nRecv = nRecv - iRet; //nRecv(받아야 할 크기 ) - iRet(받은 크기)
			//다 받으면 nRecv 가 0...그렇치 않으면 ..에러..
			nCount = nCount + iRet; // 받은 사이즈 
		}

	}

	return(nCount); //받은 사이즈 return;
	
	
}



int SendData(int nSockID,char* SendBuffer,int nSendLen)
{

	int nTempLen,iRet;
	
	if(nSockID < 0)
		return(-1);
	
	nTempLen = 0;
	
	
	while(nTempLen < nSendLen)
	{
		
		iRet = send(nSockID,&SendBuffer[nTempLen],(nSendLen - nTempLen),0);
		
		if(iRet <0)
		{
			int err = errno;	
			if(err == EINTR )//|| err == EAGAIN)
			{
				sleep(10);
				continue;	
			}
					
			return(-1*err);
		}
		else
		{
			if(iRet == 0)
				return 0;
			nTempLen += iRet;
		}
	}
	
	return(nTempLen);
}
