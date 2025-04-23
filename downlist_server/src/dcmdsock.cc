#include <stdio.h>      /* for printf() and fprintf() */ 
#include <sys/socket.h> /* for socket(), bind(), and connect() */ 
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */ 
#include <string.h>     /* for memset() */ 
#include <errno.h> 
#include <unistd.h>     /* for close() sleep()*/ 


#include "apdefine.h" //for log
#include "comcomm.h" //for log

int CreateTCPServerSocket(unsigned short port)
{
    int sock;                        /* socket to create */
    struct sockaddr_in echoServAddr; /* Local address */

	infLOG(ALWAY,"CreateTCPServerSocket | TCP socket을 생성합니다.");
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
    	return -1;	
    }
    
    infLOG(ALWAY,"CreateTCPServerSocket | SO_REUSEADDR 을 설정합니다.");
    
	int value = 1;
    int st = setsockopt(sock,SOL_SOCKET,SO_REUSEADDR ,&value,sizeof(value));
     
    if(st != 0)
	{				
		infLOG(ERROR, "CreateTCPServerSocket | setsockopt error SOL_SOCKET errornum ( %d )\n",errno);
	}
	
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(port);              /* Local port */

    infLOG(ALWAY,"CreateTCPServerSocket | bind 을 설정합니다.");
    
    if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
    {
    	infLOG(ERROR,"CreateTCPServerSocket | bind 설정 중 오류입니다.서버가 이미 뛰워져 있거나 포트가 사용 중인지 확인하세요.");
    	return -1;
    }

    /* Mark the socket so it will listen for incoming connections */
    infLOG(ALWAY,"CreateTCPServerSocket | listen 을 %d 갯수만큼 설정 합니다.",SOMAXCONN);
    if (listen(sock, SOMAXCONN) < 0)
    {
    	infLOG(ERROR,"CreateTCPServerSocket | listen 설정 중 오류입니다.");
    	return -1;   	
    }

	infLOG(ALWAY,"CreateTCPServerSocket | TCP socket을 생성합니다.");
    return sock;
}


int AcceptTCPConnection(int servSock)
{
    int clntSock;                    /* Socket descriptor for client */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned int clntLen;            /* Length of client address data structure */

    /* Set the size of the in-out parameter */
    clntLen = sizeof(echoClntAddr);
    
    /* Wait for a client to connect */
    if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr, &clntLen)) < 0)
    {
		infLOG(ERROR, "AcceptTCPConnection | Accept 오류입니다. ( %d )\n",errno); 
    }
    /* clntSock is connected to a client! */
  	
	
	infLOG(ERROR, "AcceptTCPConnection | Accept 를 시작합니다. \n"); 

    return clntSock;
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
		if( (nSendLen - nTempLen ) < 4096 )
			iRet = send(nSockID,&SendBuffer[nTempLen],(nSendLen - nTempLen),0);
		else
			iRet = send(nSockID,&SendBuffer[nTempLen], 4096  ,0);
				
		if(iRet <0)
		{
			int err = errno;	
/*			if(err == EINTR )//|| err == EAGAIN)
			{
				sleep(10);
				continue;	
			}
*/						
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
