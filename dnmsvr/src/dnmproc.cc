/******************************************************************************
 *   서브시스템 : CMD서버
 *   프로그램명 : cmdproc.cc
 *         기능 : CMD서버의 Main
 *         설명 :
 *       작성자 : LEE
 *       작성일 : 2004/02/16
 *     수정이력 :
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#include <map>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>

#include "apdefine.h"
#include "apstruct.h"
#include "dnmmain.h"
#include "comhead.h"
#include "comcomm.h"
#include "comconf.h"
using namespace std;
//#define __DEBUG

extern int m_nListenNum;
extern pthread_cond_t *mycond;



void DErrorMsg(char* msg)
{
	#ifdef __DEBUG
	printf("%s",msg);
	#endif
}


long processed(int& Socket,char* DATA )
{

//				   012345678901234]
	infLOG(ALWAY, "processed	  ] In Process \n");	

	long  dwSendLen = 0;

	PACKET101* pPacket101 = (PACKET101*)DATA;
	
	
	switch(atoi(pPacket101->szCmd))
	{

		
		case RS_CMD_REQUEST_DISCONNECT_USER_CHECK :
		{
			infLOG(ALWAY, "RS_CMD_REQUEST_DISCONNECT_USER_CHECK (%s)\n",pPacket101->szUserID);
		 	RequestDisconnectUser(Socket,DATA);
			break;
		}
		
        default:
		{
			
			infLOG(ALWAY, "Service: (%s)는 없는 서비스 입니다.", pPacket101->szCmd);
		}
	}
	#ifdef __DEBUG
	printf("processed end ]\n");
	#endif
	
//				   012345678901234]	
	infLOG(ALWAY, "processed End  ] (%d)\n",dwSendLen);
	return 1;		
	


}


int RecvData(int nSockID,char* RecvBuffer,int nRecvLen)
{
	int iRet;
	memset(RecvBuffer,0x00,nRecvLen);
	
	if(nSockID <0)
		return(-1);
	
	int nCount=0;
	int nRecv = nRecvLen;

	
	while(nRecv > 0)
	{
		iRet = recv(nSockID,(char*)RecvBuffer+nCount,nRecv,0);
		if(iRet<0)
		{
/*
			if(errno == EINTR)
			{
				continue;
			}
*/			return(-1*errno);
		}
		else
		{
			if(iRet ==0)
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
/*
			if(errno == EINTR)
				continue;
*/
			return(-1*errno);
		}
		if(iRet ==0)
			return 0;
		nTempLen += iRet;
	}

	return(nTempLen);
}

int RequestUserList(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData) //cmd 사용자 목록 요청 10051
{
	#ifdef __DEBUG
	printf("RequestUserList     ] RequestUserList \n");
	#endif
	
	LPUSERLIST pData = new USERLIST[2000];
	memset(pData,0x00,sizeof(USERLIST)*2000);


	multimap<int,USERINFO>::iterator mi;
	mi = m_UserList.begin();
	
	int nCount =0;
	
	while(mi != m_UserList.end())
	{
	
		if(mi->first == Socket)
			strcpy(pData[nCount].szUserID,"jadmin");
		else
			strcpy(pData[nCount].szUserID,mi->second.szUserID);
		strcpy(pData[nCount].szUserIP,mi->second.thread.userIP);
		pData[nCount].nNumber = nCount ;
		strcpy(pData[nCount].szStartTime,mi->second.thread.startTime);
			
		#ifdef __DEBUG
		printf("%4d ] [소켓번호 : %d] [주소 : %s] [아이디 : %s] [접속시간 %s]\n"
					,nCount+1,mi->first,mi->second.thread.userIP,mi->second.szUserID,mi->second.thread.startTime);
		#endif
		
		mi++;
		nCount++;
		if(nCount >= 1999)
			break;
		
	}
	
	HEADER header;
	memset(&header,0x00,sizeof(header));
	
	header.nCmd = RS_CMD_REQUEST_USER_LIST;
	header.nDataCnt = nCount;
	header.nDataSize = sizeof(USERLIST);
	
	pSendData  = new char[HEADER_SIZE + sizeof(USERLIST)*nCount];
	memset(pSendData ,0x00,HEADER_SIZE + sizeof(USERLIST)*nCount);
	
	memcpy( pSendData , &header,HEADER_SIZE);
	
	if(sizeof(USERLIST)*nCount > 0)
		memcpy( pSendData + HEADER_SIZE, pData ,sizeof(USERLIST)*nCount);
	
	#ifdef __DEBUG
	printf("RequestUserList     ] end RequestUserList \n");
	#endif

	delete[] pData;
	return (HEADER_SIZE + (header.nDataCnt * header.nDataSize));	
	
				
	
}



/*****************************************************************************
* SIGNAL HANDLING
* (I) int nSigNo : Signal Number
* (R) void
*****************************************************************************/
//void    infSigHandler(int nSigNo)
void SignalHandler(int nSigNo) 
{

}


int RequestDisconnectUser(int& Socket,char *DATA ) //cmd 사용자 목록 요청 10051
{
	#ifdef __DEBUG
	printf("     ]  \n");
	#endif	
	
	char* pSendData = NULL;
	
	PACKET101 Packet101_r ;
	memcpy(&Packet101_r,DATA,sizeof(PACKET101));

	struct sockaddr_in disSerAddr;
	int dis_socket;
	if ( ( dis_socket = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0 )
	{
		infLOG(ERROR, ": 소켓 생성 오류 입니다.   ( %s ) \n",Packet101_r.szUserID);
       	return(-1); 
	}
	
	memset(&disSerAddr,0x00,sizeof(disSerAddr));
	disSerAddr.sin_family      = AF_INET;
	disSerAddr.sin_addr.s_addr = inet_addr(Packet101_r.szRemoteIP);
	disSerAddr.sin_port        = htons(REMOTE_CLIENT_PORT);
	
	#ifdef __DEBUG
	printf("dn 끊기 소켓 생성 및 접속 ( %s ) ( %s ) ( %d )\n",Packet101_r.szRemoteIP,REMOTE_SERVER_IP,REMOTE_CLIENT_PORT);
	#endif
	infLOG(ALWAY, ": REMOTE_SERVER_IP 에서  ( %s ) IP  ( %s ) PORT  ( %d ) 접속을 시도합니다.\n",REMOTE_SERVER_IP,Packet101_r.szRemoteIP,REMOTE_CLIENT_PORT);
	
	if(  strcmp(Packet101_r.szRemoteIP,REMOTE_SERVER_IP) != 0)
	{
	
		#ifdef __DEBUG
		printf("dn 끊기 소켓  접속 ( %s ) ( %s )\n",Packet101_r.szRemoteIP,REMOTE_SERVER_IP);
		#endif		
		/*
	    signal(SIGALRM,SignalHandler);
	    alarm(2); // 5초 후 스스로에게 interrupt를 건다.
	 
	 	if( connect(dis_socket,(struct sockaddr * )&disSerAddr,sizeof(disSerAddr)) < 0 )
 		{
 			if( errno == EINTR)
			{
				errno = ETIMEDOUT;
				infLOG(ALWAY, "시간초과 입니다.\n");
			}
			
			close( dis_socket );
			infLOG(ALWAY, "접속 할 수 없습니다.\n");
			alarm(0);
			return -1;
 		}
	 	
	    alarm(0);
		*/
		
		/*
		
		org_flags = fcntl(sock, F_GETFL, 0);
	    flags = org_flags | O_NONBLOCK;
	    fcntl(sock, F_SETFL, flags);
	
		// connect client
		if( connect(dis_socket,(struct sockaddr * )&disSerAddr,sizeof(disSerAddr)) < 0 )
		{
			infLOG(ERROR, ": 아이디 ( %s ) : 접속 실패  \n",Packet101_r.szUserID);
			close( dis_socket );
	       	return(-1); 		
		}	
		//send data client
		
	   // fcntl(sock, F_SETFL, org_flags);
	    */
	    
	    
		if( connect_timeout(dis_socket,(struct sockaddr * )&disSerAddr,sizeof(disSerAddr) , 3) < 0 )
		{
			infLOG(ERROR, ": 아이디 ( %s ) : 접속 실패  \n",Packet101_r.szUserID);
			close( dis_socket );
	       	return(-1); 		
		}	
	
		#ifdef __DEBUG
		printf("dn 끊기 소켓 전송 ( %s )\n",Packet101_r.szRemoteIP);
		#endif

		#ifdef __DEBUG
		printf("데이터 확인 \n");
		printf("Header.nCmd ( %s )\nHeader.szUserID ( %s ) \nDATA.szRemoteIP(%s)\nDATA,szMsg( %s )\n"
				,Packet101_r.szCmd,Packet101_r.szUserID,Packet101_r.szRemoteIP,Packet101_r.szMsg);
		printf(" 사이즈 ( %d ) \n",sizeof(PACKET101));
		#endif
	
		
		int nSendLen = 0;
		infLOG(ALWAY, ": 아이디 ( %s ) : 접속을 끊기 위해 패킷을 전송합니다.  \n",Packet101_r.szUserID);
		
		if((nSendLen = SendData(dis_socket,(char*)&Packet101_r, sizeof(PACKET101) )) < 0)
		{
			//			   012345678901234]		
			int err  =errno;				
			infLOG(ERROR, " send ] sendData  (%d)\n",err);
			#ifdef __DEBUG
			printf(" send ] sendData  \n");
			#endif
				
	
			return -1;
			
		
		}
		infLOG(ALWAY, ": 아이디 ( %s ) : 패킷 전송 완료.  \n",Packet101_r.szUserID);

		
		close(dis_socket);				
		infLOG(ALWAY, ": 아이디 ( %s ) : 소켓 정리.  \n",Packet101_r.szUserID);
	}
	//close client
	#ifdef __DEBUG
	printf("dn 끊기 소켓 접속 종료 ( %s )\n",Packet101_r.szUserID);
	#endif
	return RS_EOL;
}



void *ThreadMain(void *threadArgs)
{
    int clntSock;                   /* Socket descriptor for client connection */

	
    /* Guarantees that thread resources are deallocated upon return */
    pthread_detach(pthread_self()); 


	LPUSERINFO pData = (LPUSERINFO) threadArgs;
	
	USERINFO UserData ;

	memset(&UserData,0x00,sizeof(USERINFO));
	
	
	UserData.thread.clntSock = pData->thread.clntSock;
	UserData.threadID = getpid();
	strcpy(UserData.thread.userIP,pData->thread.userIP);
//	printf("ip == %s",UserData.thread.userIP);

	time_t			curtime;
	struct tm		*stm;
	time( &curtime );
	stm = (struct tm *) localtime(&curtime);

	localtime_r(&curtime, stm);

	sprintf(UserData.thread.startTime  ,"%04d년%02d월%02d일%02d시%02d분"
								,stm->tm_year+1900
								,  stm->tm_mon + 1
								,  stm->tm_mday
								,  stm->tm_hour
								,  stm->tm_min
								);
    
    
    clntSock = pData->thread.clntSock;
    
    // 시간 넣기...
    
    ///////////////
	
	//추가
	m_UserList.insert(pair<int,USERINFO>(clntSock,UserData));
	
	
	
	
	//id 넣기
    //free(pData);              /* Deallocate memory for argument */
    delete pData;

	
	//set timer for dissconnect at 1 minutes
	struct timeval tv;
	tv.tv_sec = 5; //2분
	
	int st = 
	(clntSock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));
	if(st != 0)
	{	
		char szErrorMsg[255];
		memset(szErrorMsg,0x00,sizeof(szErrorMsg));
		
		int nErrno = errno;
//			01234567890123456789]
		if(errno == EBADF )
			sprintf(szErrorMsg,"ThreadDnMain           ]  Socket setsockopt SO_RCVTIMEO Error CodeNum( %d ) CodeName( EBADF )\n",nErrno);
		if(errno == ENOTSOCK )
			sprintf(szErrorMsg,"ThreadDnMain           ] Socket setsockopt SO_RCVTIMEO Error CodeNum( %d ) CodeName( ENOTSOCK )\n",nErrno);
		if(errno == ENOPROTOOPT )
			sprintf(szErrorMsg,"ThreadDnMain           ] Socket setsockopt SO_RCVTIMEO Error CodeNum( %d ) CodeName( ENOPROTOOPT )\n",nErrno);
		if(errno == EFAULT)
			sprintf(szErrorMsg,"ThreadDnMain           ] Socket setsockopt SO_RCVTIMEO Error CodeNum( %d ) CodeName(  EFAULT )\n",nErrno);

		#ifdef __DEBUG
//				01234567890123456789]		
		printf("%s",szErrorMsg);
		#endif
		infLOG(ERROR, szErrorMsg); 
	}
	
	struct timeval tv2;
	tv2.tv_sec = 5; //2분
				
	st = setsockopt(clntSock, SOL_SOCKET, SO_SNDTIMEO, &tv2, sizeof(struct timeval));
	if(st != 0)
	{
		char szErrorMsg[255];
		memset(szErrorMsg,0x00,sizeof(szErrorMsg));
		
		int nErrno = errno;
		

		if(errno == EBADF )
			sprintf(szErrorMsg,"ThreadDnMain           ] Socket setsockopt SO_SNDTIMEO Error CodeNum( %d ) CodeName( EBADF )\n",nErrno);
		if(errno == ENOTSOCK )
			sprintf(szErrorMsg,"ThreadDnMain           ] Socket setsockopt SO_SNDTIMEO Error CodeNum( %d ) CodeName( ENOTSOCK )\n",nErrno);
		if(errno == ENOPROTOOPT )
			sprintf(szErrorMsg,"ThreadDnMain           ] Socket setsockopt SO_SNDTIMEO Error CodeNum( %d ) CodeName( ENOPROTOOPT )\n",nErrno);
		if(errno == EFAULT)
			sprintf(szErrorMsg,"ThreadDnMain           ] Socket setsockopt SO_SNDTIMEO Error CodeNum( %d ) CodeName(  EFAULT )\n",nErrno);
		#ifdef __DEBUG
		printf("%s",szErrorMsg);
		#endif
		infLOG(ERROR, szErrorMsg); 
	}
	
    WaitForRequest(clntSock);
    
    //clear user info on UserMap
	multimap<int,USERINFO>::iterator mi;
	//mi = m_UserList.begin(); change
	mi = m_UserList.find(clntSock);
	if(mi != m_UserList.end())
	{
		#ifdef __DEBUG
//				01234567890123456789]		
		printf("ThreadDnMain           ] ( %d ) ( %s )close thread and socket\n\n",clntSock, mi->second.szUserID);
		#endif
		infLOG(ALWAY, "ThreadDnMain           ] ( %d ) ( %s ) close thread and socket \n\n",clntSock,mi->second.szUserID);
				
		m_UserList.erase(mi);
		close(clntSock);    // Close client socket 

	}
	else
		infLOG(ALWAY, "ThreadDnMain           ] (unknown) close thread and socket\n\n\n\n\n");

    return (NULL);	
}




void WaitForRequest(int& Socket)
{
	PACKET101 DATA ;
	memset(&DATA,0x00,sizeof(PACKET101));

	// client에서 header을 수신

	//			   012345678901234]
	infLOG(ALWAY, "Wait Recv Head ] \n");	

	//해더 받기	
	if(RecvData(Socket,(char*)&DATA,sizeof(PACKET101) )<= 0)
	{
		//			   012345678901234]		
		infLOG(ERROR, "Wait Recv Head ] Recv Header Exception \n"); 
		#ifdef __DEBUG
		printf("Wait Recv Head ] Recv Header Exception \n"); 
		#endif
		
		return;
	}
	
	#ifdef __DEBUG
	printf("데이터 확인 \n");
	printf("Header.nCmd ( %s )\nHeader.szUserID ( %s ) \nDATA.szRemoteIP(%s)\nDATA,szMsg( %s )\n"
			,DATA.szCmd,DATA.szUserID,DATA.szRemoteIP,DATA.szMsg);
	#endif

    //아이디 셋팅
	multimap<int,USERINFO>::iterator mi;    
	mi = m_UserList.find(Socket);
	if(mi != m_UserList.end())
	{
		memcpy(mi->second.szUserID,DATA.szUserID,min(12,(int)sizeof(DATA.szUserID)));
	}
	
	
	char* pRecvData = NULL;	
	
	long dwSendLen = 0;
	

	//			   012345678901234]		
	infLOG(ALWAY, "processed start] with not body \n");			
	dwSendLen = processed(Socket, (char *)&DATA );	//서비스호출

	infLOG(ALWAY, "WaitForRequest ] End : 20001 \n");
		
	
	return ;

	
}


//Rechard stevens
int connect_timeout(int sockfd, const struct sockaddr * saptr, socklen_t salen, int nsec)
{
	int				flags, n, error;
	socklen_t		len;
	fd_set			rset, wset;
	struct timeval	tval;

	flags =fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

	error = 0;
	if ( (n = connect(sockfd, (struct sockaddr *) saptr, salen)) < 0)
		if (errno != EINPROGRESS)
			return(-1);

	/* Do whatever we want while the connect is taking place. */

	if (n == 0)
		goto done;	/* connect completed immediately */

	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);
	wset = rset;
	tval.tv_sec = nsec;
	tval.tv_usec = 0;

	if ( (n = select(sockfd+1, &rset, &wset, NULL, nsec ? &tval : NULL)) == 0) 
	{
		close(sockfd);		/* timeout */
		errno = ETIMEDOUT;
		return(-1);
	}

	if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) 
	{
		len = sizeof(error);
		if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
			return(-1);			/* Solaris pending error */
	} else
		printf("select error: sockfd not set");

done:
	fcntl(sockfd, F_SETFL, flags);	/* restore file status flags */

	if (error) 
	{
		close(sockfd);		/* just in case */
		errno = error;
		return(-1);
	}
	return(0);
}


/*
int connect_timeout(int sockfd, const struct sockaddr *saptr, socklen_t salen, struct timeval tval)
{
    int                flags, n, error;
    socklen_t        len;
    fd_set            rset, wset;

#ifdef WIN32
    int nonblocking =1;
    ioctlsocket(sockfd, FIONBIO, (unsigned long*) &nonblocking);
#else
    flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
#endif

    error = 0;
    if ( (n = connect(sockfd, (struct sockaddr *) saptr, salen)) < 0)
    {
#ifdef WIN32
        errno = WSAGetLastError();
#endif
        if (errno != EINPROGRESS && errno!= EWOULDBLOCK)
        {
            return(-1);
        }
    }

    // Do whatever we want while the connect is taking place. 

    if (n == 0)
        goto done;    // connect completed immediately 

    FD_ZERO(&rset);
    FD_SET(sockfd, &rset);
    wset = rset;

    if ( (n = select(sockfd+1, &rset, &wset, NULL,
                     ((tval.tv_sec>0) || (tval.tv_usec>0))? &tval : NULL)) == 0)
    {
        close(sockfd);        // timeout 
        errno = ETIMEDOUT;
        return(-1);
    }

    if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset))
    {
#ifndef WIN32
        len = sizeof(error);
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
            return(-1);            // Solaris pending error 
#endif
    } else
        return(-1); //err_quit("select error: sockfd not set");

done:
#ifdef WIN32
    nonblocking =0;
    ioctlsocket(sockfd, FIONBIO, (unsigned long*) &nonblocking);
#else
    fcntl(sockfd, F_SETFL, flags);    // restore file status flags 
#endif

    if (error) {
        close(sockfd);        // just in case 
        errno = error;
        return(-1);
    }
    return(0);
}
*/
