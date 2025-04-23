#include <stdio.h>      /* for printf() and fprintf() */ 
#include <sys/socket.h> /* for socket(), bind(), and connect() */ 
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */ 
#include <string.h>     /* for memset() */ 
#include <errno.h> 
#include <unistd.h>     /* for close() sleep()*/ 


#include "fupdefine.h"
#include "fupsock.h"
#include "apdefine.h" //for log
#include "comcomm.h" //for log


int RecvFileData(int nSockID,char* RecvBuffer,long nRecvLen,double dTotalLen)
{
	memset(RecvBuffer,0x00,nRecvLen);
	
	if(nSockID <0)
		return(-1);
	
	int nCount=0;
	int nRecv = nRecvLen;
	int iRet=0;

	if(dTotalLen < (double)RECVBUF)
		nRecv =(long)dTotalLen ;	
		
	while(nRecv > 0 )
	{
		if(nRecv < SOCKBUF)
			iRet = recv(nSockID,(char*)RecvBuffer+nCount,nRecv,0);
		else
			iRet = recv(nSockID,(char*)RecvBuffer+nCount,SOCKBUF ,0);

		
		if(iRet<0)
		{
			int err = errno;
								
			if( err == EINTR )//|| err == EAGAIN )
			{								
				infLOG(ERROR,"RecvFileData : EINTR - Sleep(10)\n");
				sleep(10);
				continue;
			}
			
			infLOG(ERROR,"RecvFileData : errno [ %d ] error msg [ %s ]\n",errno,strerror(errno));
			return(-1*err);
		}
		else
		{
			if(iRet == 0)
				return 0;
			nRecv = nRecv - iRet;
			nCount = nCount + iRet;
		}

	}

	return(nCount);

}

int RecvData(int nSockID,char* RecvBuffer,int nRecvLen)
{
	
	int iRet;
	memset(RecvBuffer,0x00,nRecvLen);
	
	if(nSockID <0)
		return(-1);
	
	int nCount=0;
	int nRecv = nRecvLen;

	
	while(nRecv > 0 )
	{
		iRet = recv(nSockID,(char*)RecvBuffer+nCount,nRecv,0);
		if(iRet<0)
		{
			int err = errno;
			if( err == EINTR )//|| err == EAGAIN )
			{
				infLOG(ERROR,"RecvData : EINTR - Sleep(10)\n");
				sleep(10);
				continue;
			}
			infLOG(ERROR,"RecvData : errno [ %d ] error msg [ %s ]\n",errno,strerror(errno));
			return(-1*err);
		}
		else
		{
			if(iRet == 0)
				return 0;
			nRecv = nRecv - iRet;
			nCount = nCount + iRet;
		}
	
	
	}


	return(nCount);
	
	
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
			if(err == EINTR )// || err == EAGAIN)
			{
				infLOG(ERROR,"SendData : EINTR - Sleep(10)\n");
				sleep(10);
				continue;	
			}
			infLOG(ERROR,"SendData : errno [ %d ] error msg [ %s ]\n",errno,strerror(errno));				
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



//#define MAXPENDING 5    /* Maximum outstanding connection requests */

int CreateTCPServerSocket(unsigned short port)
{
    int sock;                        /* socket to create */
    struct sockaddr_in ServAddr; /* Local address */

    /* Create socket for incoming connections */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
    	infLOG(ERROR,"CreateTCPServerSocket : socket() error - errno [ %d ] error msg [ %s ]\n",errno,strerror(errno));				
    	return -1;	
    }
      
     int value = 1;
    
    int st = setsockopt(sock,SOL_SOCKET,SO_REUSEADDR ,&value,sizeof(value));
     
    if(st != 0)
	{
		infLOG(ERROR,"CreateTCPServerSocket : setsockopt(SO_REUSEADDR) error - errno [ %d ] error msg [ %s ]\n",errno,strerror(errno));				
	}
    
		
    /* Construct local address structure */
    memset(&ServAddr, 0, sizeof(ServAddr));   /* Zero out structure */
    ServAddr.sin_family = AF_INET;                /* Internet address family */
    ServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    ServAddr.sin_port = htons(port);              /* Local port */
	
	
    if (bind(sock, (struct sockaddr *) &ServAddr, sizeof(ServAddr)) < 0)
    {
    	infLOG(ERROR,"CreateTCPServerSocket : bind() error - errno [ %d ] error msg [ %s ]\n",errno,strerror(errno));				
    	return -1;
    }

    /* Mark the socket so it will listen for incoming connections */
    if (listen(sock, 1024) < 0)
    {
    	infLOG(ERROR,"CreateTCPServerSocket : listen() error - errno [ %d ] error msg [ %s ]\n",errno,strerror(errno));				
    	return -1;    	
    }
        

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
    if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr, 
           &clntLen)) < 0)
    {
  		infLOG(ERROR,"AcceptTCPConnection : accept() error - errno [ %d ] error msg [ %s ]\n",errno,strerror(errno));				
    }
    else
    	infLOG(ERROR,"AcceptTCPConnection : accept( sock : %d - ip : %s )\n",clntSock,inet_ntoa(echoClntAddr.sin_addr));				
 	
    return clntSock;
}

