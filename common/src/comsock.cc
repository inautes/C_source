
#include <stdio.h>      /* for printf() and fprintf() */ 
#include <sys/socket.h> /* for socket(), bind(), and connect() */ 
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */ 
#include <string.h>     /* for memset() */ 
#include <errno.h> 
#include <unistd.h>     /* for close() sleep()*/ 


#include "apdefine.h" //for log
#include "comcomm.h" //for log

//여기서 부터 recvdata ..함수


long ComRecvData(int nSockID,char* RecvBuffer,long nRecvLen)
{
	
	int iRet;
	memset(RecvBuffer,0x00,nRecvLen); //<-- 버퍼 초기화
	
	if(nSockID <0)
		return(-1);
	
	long nCount=0;
	long nRecv = nRecvLen;

	
	while(nRecv > 0) //recv 받은 데이터가 다 받을때까정..
	{
		iRet = recv(nSockID,(char*)RecvBuffer+nCount,nRecv,0); // 받기
		if(iRet<0) // 0보다 작으면 에러 ..win32에서는 getlasterror 로 채크 할수  있음
		{
			int err = errno;
/*			if(err == EINTR ) //|| err == EAGAIN) //linux 에서만...
			{
				sleep(10);
				continue;
			}
*/			
			close(nSockID);
			return(-1*err); //에러 return
		}
		else
		{
			if(iRet == 0) //0이면 접속이 끊김...즉 인터넷 불통 또는 강제 종료
			{
				close(nSockID);
				return 0;
			}
			nRecv = nRecv - iRet; //nRecv(받아야 할 크기 ) - iRet(받은 크기)
			//다 받으면 nRecv 가 0...그렇치 않으면 ..에러..
			nCount = nCount + iRet; // 받은 사이즈 
		}

	}

	return(nCount); //받은 사이즈 return;
	
	
}



long ComSendData(int nSockID,char* SendBuffer,long nSendLen)
{

	long nTempLen,iRet;
	
	if(nSockID < 0)
		return(-1);
	
	nTempLen = 0;
	
	
	while(nTempLen < nSendLen)
	{
		if( (nSendLen - nTempLen ) < 1024 )
			iRet = send(nSockID,&SendBuffer[nTempLen],(nSendLen - nTempLen),0);
		else
			iRet = send(nSockID,&SendBuffer[nTempLen], 1024  ,0);
				
		if(iRet <0)
		{
			int err = errno;	
/*			if(err == EINTR )//|| err == EAGAIN)
			{
				sleep(10);
				continue;	
			}
*/			
			close(nSockID);			
			return(-1*err);
		}
		else
		{
			if(iRet == 0)
			{
				close(nSockID);
				return 0;
			}
			nTempLen += iRet;
		}
	}
	
	return(nTempLen);
}
