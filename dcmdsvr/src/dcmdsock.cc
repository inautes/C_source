#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <string.h>     /* for memset() */
#include <errno.h>
#include <unistd.h>     /* for close() sleep()*/


#include "apdefine.h" //for log
#include "comcomm.h" //for log
#include "dcmdsock.h"


int CreateTCPServerSocket(unsigned short port)
{
    int sock;                        /* socket to create */
    struct sockaddr_in echoServAddr; /* Local address */

    /* Create socket for incoming connections */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
    	return -1;
    }

	int value = 1;
    int st = setsockopt(sock,SOL_SOCKET,SO_REUSEADDR ,&value,sizeof(value));
    if(st != 0)
	{
		infLOG(ERROR, "CreateServerSocket	] setsockopt error SOL_SOCKET errornum ( %d )\n",errno);
	}

    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(port);              /* Local port */

    /* Bind to the local address */
    if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
    {
    	return -1;
    }

    /* Mark the socket so it will listen for incoming connections */
    if (listen(sock, SOMAXCONN) < 0)
    {
    	return -1;
    }

    return sock;
}


int AcceptTCPConnection(int servSock)
{
    int clntSock;                    /* Socket descriptor for client */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned int clntLen;            /* Length of client address data structure */

    clntLen = sizeof(echoClntAddr);

    if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr, &clntLen)) < 0)
    {
		infLOG(ERROR, "AcceptTCPConnection	] Accept Failed ( %d )\n",errno);
    }

	//infLOG(ALWAY, "AcceptTCPConnection	] Accept  ( %s )\n",inet_ntoa(echoClntAddr.sin_addr));
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
		if(iRet<0)
		{
			int err = errno;
/*			if(err == EINTR ) //|| err == EAGAIN) //linux 에서만...
			{
				sleep(10);
				continue;
			}
*/
			infLOG(ERROR, "RecvData error no 1 [ %d ] \n",errno);
			return(-1*err); //에러 return
		}
		else
		{
			if(iRet == 0) //0이면 접속이 끊김...즉 인터넷 불통 또는 강제 종료
			{
				//infLOG(ERROR, "RecvData error no 2 [ %d ] \n",0);
				return 0;
			}
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
			infLOG(ERROR, "SendData error no ( %d )\n",err);
			return(-1*err);
		}
		else
		{
			if(iRet == 0)
			{
				infLOG(ERROR, "SendData error no [ %d ] \n",0);
				return 0;
			}

			nTempLen += iRet;
		}
	}

	return(nTempLen);
}



int Connect(char* pServerIP, int nPort)
{
	int sock;
	struct sockaddr_in ServAddr;

    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
    	infLOG(ERROR,"create socket error");
    	return -1;
    }

    int value = 1;
    int ret = setsockopt(sock,SOL_SOCKET,SO_REUSEADDR ,&value,sizeof(value));


    memset(&ServAddr, 0, sizeof(ServAddr));   /* Zero out structure */
    ServAddr.sin_family = AF_INET;                /* Internet address family */
    ServAddr.sin_addr.s_addr = inet_addr(pServerIP); /* Any incoming interface */
    ServAddr.sin_port = htons(nPort);              /* Local port */

	if(connect(sock, (struct sockaddr*)&ServAddr, sizeof(ServAddr)) == -1)
	{
		infLOG(ERROR,"socket connect error ");
		return -1;
	}

	if( SetSockTimeOut(sock, 10 , 0 ) != 0)
	{
		infLOG(ERROR,"socket recv timeout error ");
	}
	if( SetSockTimeOut(sock, 10 , 1 ) != 0 )
	{
		infLOG(ERROR,"socket send timeout error ");
	}

	return (sock);
}



int SetSockTimeOut(int nSock, unsigned int nSec,int nMode  )
{
	int st = 0;
	struct timeval tv;
	tv.tv_sec = nSec;

	if( nSock < 0 )
		return -1;


	if(nMode == 0)
	{


		st = setsockopt(nSock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));
		if(st != 0)
		{
			#ifdef __DEBUG
			if(errno == EBADF )
				printf("Socket Opt Erro = EBADF\n");
			if(errno == ENOTSOCK )
				printf("Socket Opt Erro = ENOTSOCK\n");
			if(errno == ENOPROTOOPT )
				printf("Socket Opt Erro = ENOPROTOOPT\n");
			if(errno == EFAULT)
				printf("Socket Opt Erro = EFAULT\n");
			printf("errno number == %d\n , st = %d\n",errno,st);
			printf("소켓 recv time out 옵션 설정 실패\n");
			#endif

			return -1*errno;
		}

	}
	else if( nMode == 1)
	{
		st = setsockopt(nSock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(struct timeval));
		if(st != 0)
		{
			#ifdef __DEBUG
			printf("recv timeout error\n");
			if(errno == EBADF )
				printf("Socket Opt Erro = EBADF\n");
			if(errno == ENOTSOCK )
				printf("Socket Opt Erro = ENOTSOCK\n");
			if(errno == ENOPROTOOPT )
				printf("Socket Opt Erro = ENOPROTOOPT\n");
			if(errno == EFAULT)
				printf("Socket Opt Erro = EFAULT\n");
			printf("errno number == %d\n , st = %d\n",errno,st);
			printf("소켓 send time out 옵션 설정 실패\n");
			#endif

			return -1*errno;
		}

	}
	else
		return -1;
	return 0;
}

