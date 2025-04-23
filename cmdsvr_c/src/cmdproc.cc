/******************************************************************************
 *   서브시스템 : CMD서버
 *   프로그램명 : cmdproc.cc
 *         기능 : CMD서버의 Main
 *         설명 :
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
#include "cmdmain.h"
#include "comhead.h"
#include "comcomm.h"
#include "comconf.h"
using namespace std;

extern int m_nListenNum;
extern pthread_cond_t *mycond;

void *mon_thread(void  *data)
{
	//printf("moniter thread\n");
/*	while(1)
	{
		sleep(60);
		multimap<int,ph>::iterator mi;
		mi = s_info.phinfo.begin();
		//printf("size = %d\n",s_info.phinfo.size());
		while(mi != s_info.phinfo.end())
		{
			//printf("%d : %d : %d \n", mi->first , mi->second.index_num , mi->second.sockfd);
			mi++;
		}
	}
	
	
	*/
	int nMenu,nResult;
	char cMenu;
	int nCount=0;
	char szMenu[10];
	memset(szMenu,0x00,10);
	
	while(nMenu != 3)
	{
		printf("----------------------------------------------\n"
			   "	     몌뉴를 선택해 주세요 	 	  		  \n"
			   "1 (연결 상태 조회) : 2 (대기열 갯수) : 3 (종료)\n"
			   "----------------------------------------------\n");
		memset(szMenu,0x00,10);
		scanf("%s",szMenu);
		getchar();
		nMenu = atoi(szMenu);
		printf("%d\n",nMenu);
		switch(nMenu)
		{
			case 1:
			{
				nCount=0;
				
				printf("-----------------------------------------------\n"
					   "			 현재 쓰레드 상황               \n"
					   "-----------------------------------------------\n");
				multimap<int,USERINFO>::iterator mi;
				mi = m_UserList.begin();
				printf("Total Connect = %d\n",m_UserList.size());
				
				while(mi != m_UserList.end())
				{
					printf("%4d> [소켓번호 : %d] [주소 : %s] [아이디 : %s] \n"
								,nCount+1,mi->first,mi->second.thread.userIP,mi->second.szUserID);
					
					mi++;
					nCount++;
					
					if(nCount != 0 && nCount%30 == 0)
					{
						printf("---- 계속 (나가기 'n') ----\n");
						scanf("%c",&cMenu);
						getchar();
						if(cMenu == 'n' || cMenu == 'N')
							break;
					}
				}
					
				break;
			}
			case 2:
			{
				printf("대기할 갯수를 입력하세요 (0 ~ 10) : ");
				scanf("%d",&nResult);
				getchar();
				if(nResult < 0 || nResult >10)
				{
					printf("오류 < 0 ~ 10 사이의 숫자만 가능합니다.>\n");
					break;
				}
				printf("대기열이 %d 만큼 변경 되었습니다.\n",nResult);
				
				break;
			}
			case 3:
			{
				printf("서버를 종료 합니까?(y/n): ");
				scanf("%c",&cMenu);
				getchar();
				if(cMenu == 'y'  || cMenu == 'Y')
				{
					
					printf("서버를 종료 합니다.\n");
				//	infLOG(ALWAY, "================= 프로그램 종료 ==================\n"); 
				
					multimap<int,USERINFO>::iterator mi;
					mi = m_UserList.begin();
					while(mi != m_UserList.end())
					{
						 m_UserList.erase(mi);
						 close(mi->first);    
						 #ifdef __DEBUG
						 printf("in handletcpclient  closeSocket\n");
						 #endif
						 mi++;
						
					}

					exit(0);
					
				}
				else
					nMenu = 1000;
					
				break;
			}
			default:
				printf("not have command\n");
		}
		nMenu = 1000;
	}

}

void DErrorMsg(char* msg)
{
	#ifdef __DEBUG
	printf("%s",msg);
	#endif
}

/*******************************************************************************
** 서비스 제어함수
*******************************************************************************/
long processed(int& Socket,char* pHead, char* pRecvData, char* &pSendData)
{

//				   012345678901234]
	infLOG(ALWAY, "processed	  ] In Process \n");	

	char  ErrMsg[256];
	int   nErrcode  = 0;
	long  dwSendLen = 0;

	LPHEADER pRHeader = (LPHEADER)pHead;
	switch(pRHeader->nCmd)
	{

		
	
		case RS_CMD_REQUEST_USER_LIST: //10051 사용자 리스트 조회 admin 
		{
			//			   012345678901234]			
			infLOG(ALWAY, "RequestUserList] (%s)\n",pRHeader->szUserID);
			nErrcode = RequestUserList(Socket,pHead, pRecvData, pSendData);
			//			   012345678901234]			
			infLOG(ALWAY, "RequestUserList] (%s)\n",pRHeader->szUserID);
			break;
		}
		
		case 1001:	// we 서버할당조회 RS_CMD_REQUEST_FUP_WESERVERINFO
		{
			
			//			   012345678901234]			
			infLOG(ALWAY, "cmds1001 	  ] (%s)\n",pRHeader->szUserID);
			nErrcode = cmds1001(pHead, pRecvData, pSendData);
			//			   012345678901234]			
			infLOG(ALWAY, "cmds1001 	  ] (%s)\n",pRHeader->szUserID);
			break;
		}
		
		case 1101: // 내자료실 서버 할당 및 사용자 용량 검사 RS_CMD_REQUEST_FUP_MYSERVERINFO
		{
			//			   012345678901234]			
			infLOG(ALWAY, "cmds1101 	  ] (%s)\n",pRHeader->szUserID);
			nErrcode = cmds1101(pHead, pRecvData, pSendData);
			//			   012345678901234]			
			infLOG(ALWAY, "cmds1101 	  ] (%s)\n",pRHeader->szUserID);
			break;	
		}
			
		case 1005: //서버 목록 조회 admin
		{
			//			   012345678901234]			
			infLOG(ALWAY, "cmds1005		  ] \n");
			nErrcode = cmds1005(pHead, pRecvData, pSendData);
			//			   012345678901234]			
			infLOG(ALWAY, "	cmds1005	  ] \n");
			
			break;
			
		}

		case RS_EOL:
		{	
			//printf("send RS_EOL\n");
			//			   012345678901234]		
			infLOG(ALWAY, "RS_EOL		  ] \n");		

			HEADER headers;
			
			headers.nCmd = RS_EOL;
			headers.nDataCnt=0;
			headers.nDataSize =0;
		
			pSendData = new char[HEADER_SIZE];
			memset(pSendData, 0x00, HEADER_SIZE);
			
			memcpy(pSendData, &headers, HEADER_SIZE); //header	
						
			nErrcode = RS_EOL;
			break;			
		}
		
        default:
		{
			//			   012345678901234]			
			infLOG(ALWAY, "Default 		  ] (not have Service) \n");
			memset (ErrMsg, 0x00, sizeof(ErrMsg));
			sprintf(ErrMsg, "Service: (%d)는 없는 서비스 입니다.", pRHeader->nCmd);
			E_dump(-900001, ErrMsg, pSendData);
			nErrcode = -900001;
			break;
		}
	}

	if (nErrcode < 0)
	{
		//			   012345678901234]		
		infLOG(ALWAY, "nErrcode 	  ] (%d)\n",nErrcode);		
		
		dwSendLen = ERR_HEADER_SIZE;
	}
	else
	{
		//			   012345678901234]		
		infLOG(ALWAY, " Send Data 	  ] \n");
		LPHEADER pSHeader = (LPHEADER)pSendData;
		pSHeader->nErrorCode = 0;
		dwSendLen = HEADER_SIZE + (pSHeader->nDataCnt * pSHeader->nDataSize);
	}
	#ifdef __DEBUG
	printf("processed end ]\n");
	#endif
	
//				   012345678901234]	
	infLOG(ALWAY, "processed End  ] (%d)\n",dwSendLen);
	return dwSendLen;		
	
		
							
		/*		


		case 1002:	// 컨텐츠등록 조회
		{
			infLOG(ALWAY, "Input ) cmds1002\n");			
			nErrcode = cmds1002(pHead, pRecvData, pSendData);
			infLOG(ALWAY, "Ouput ) cmds1002\n");			
			break;
		}
		
		case 1003:	// 친구목록조회
		{
			
			infLOG(ALWAY, "Input ) cmds1003\n");
			nErrcode = cmds1003(pHead, pRecvData, pSendData);
			infLOG(ALWAY, "Ouput ) cmds1003\n");
			break;
		}
		
		case 1004:	// 로그인인증
		{
			infLOG(ALWAY, "Input ) cmds1004\n");			
			nErrcode = cmds1004(pHead, pRecvData, pSendData);
			infLOG(ALWAY, "Ouput ) cmds1004\n");			
			break;
		}
		case 1011:	// 등록자료삭제
		{
			
			infLOG(ALWAY, "Input ) cmds1011\n");
			nErrcode = cmds1011(pHead, pRecvData, pSendData);
			infLOG(ALWAY, "Ouput ) cmds1011\n");			
			break;
		}
		case 1012:	// 다운로드자료삭제
		{
			infLOG(ALWAY, "Input ) cmds1012\n");						
			nErrcode = cmds1012(pHead, pRecvData, pSendData);			
			infLOG(ALWAY, "Ouput ) cmds1012\n");			
			break;
		}

		case 10041: // RS_CMD_REQUEST_ADMIN_LOGIN
		{
			infLOG(ALWAY, "Input ) cmds10041\n");
			nErrcode = cmds10041(pHead, pRecvData, pSendData);
			infLOG(ALWAY, "Ouput ) cmds10041\n");
			break;
		}
		*/		
		
		
		
		
		
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
	tv.tv_sec = 60; //2분
	
	int st = setsockopt(clntSock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));
	if(st != 0)
	{	
		char szErrorMsg[255];
		memset(szErrorMsg,0x00,sizeof(szErrorMsg));
		
		int nErrno = errno;
//			01234567890123456789]
		if(errno == EBADF )
			sprintf(szErrorMsg,"ThreadMain           ]  Socket setsockopt SO_RCVTIMEO Error CodeNum( %d ) CodeName( EBADF )\n",nErrno);
		if(errno == ENOTSOCK )
			sprintf(szErrorMsg,"ThreadMain           ] Socket setsockopt SO_RCVTIMEO Error CodeNum( %d ) CodeName( ENOTSOCK )\n",nErrno);
		if(errno == ENOPROTOOPT )
			sprintf(szErrorMsg,"ThreadMain           ] Socket setsockopt SO_RCVTIMEO Error CodeNum( %d ) CodeName( ENOPROTOOPT )\n",nErrno);
		if(errno == EFAULT)
			sprintf(szErrorMsg,"ThreadMain           ] Socket setsockopt SO_RCVTIMEO Error CodeNum( %d ) CodeName(  EFAULT )\n",nErrno);

		#ifdef __DEBUG
//				01234567890123456789]		
		printf("%s",szErrorMsg);
		#endif
		infLOG(ERROR, szErrorMsg); 
	}
	
	struct timeval tv2;
	tv2.tv_sec = 60; //2분
				
	st = setsockopt(clntSock, SOL_SOCKET, SO_SNDTIMEO, &tv2, sizeof(struct timeval));
	if(st != 0)
	{
		char szErrorMsg[255];
		memset(szErrorMsg,0x00,sizeof(szErrorMsg));
		
		int nErrno = errno;
		

		if(errno == EBADF )
			sprintf(szErrorMsg,"ThreadMain           ] Socket setsockopt SO_SNDTIMEO Error CodeNum( %d ) CodeName( EBADF )\n",nErrno);
		if(errno == ENOTSOCK )
			sprintf(szErrorMsg,"ThreadMain           ] Socket setsockopt SO_SNDTIMEO Error CodeNum( %d ) CodeName( ENOTSOCK )\n",nErrno);
		if(errno == ENOPROTOOPT )
			sprintf(szErrorMsg,"ThreadMain           ] Socket setsockopt SO_SNDTIMEO Error CodeNum( %d ) CodeName( ENOPROTOOPT )\n",nErrno);
		if(errno == EFAULT)
			sprintf(szErrorMsg,"ThreadMain           ] Socket setsockopt SO_SNDTIMEO Error CodeNum( %d ) CodeName(  EFAULT )\n",nErrno);
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
		printf("ThreadMain           ] ( %d ) ( %s )close thread and socket\n\n",clntSock, mi->second.szUserID);
		#endif
		infLOG(ALWAY, "ThreadMain           ] ( %d ) ( %s ) close thread and socket \n\n",clntSock,mi->second.szUserID);
				
		m_UserList.erase(mi);
		close(clntSock);    // Close client socket 

	}
	else
		infLOG(ALWAY, "ThreadMain           ] (unknown) close thread and socket\n\n\n\n\n");

    return (NULL);
}



/*******************************************************************************
** Client 송수신
*******************************************************************************/

void WaitForRequest(int& Socket)
{

	HEADER Header ;
	memset(&Header,0x00,HEADER_SIZE);

	// client에서 header을 수신

	//			   012345678901234]
	infLOG(ALWAY, "Wait Recv Head ] \n");	
	
	if(RecvData(Socket,(char*)&Header,HEADER_SIZE )<= 0)
	{
		//			   012345678901234]		
		infLOG(ERROR, "Wait Recv Head ] Recv Header Exception \n"); 
		#ifdef __DEBUG
		printf("Wait Recv Head ] Recv Header Exception \n"); 
		#endif
		
		return;
	}

    //아이디 셋팅
	multimap<int,USERINFO>::iterator mi;    
	mi = m_UserList.find(Socket);
	if(mi != m_UserList.end())
	{
		memcpy(mi->second.szUserID,Header.szUserID,min(12,(int)sizeof(Header.szUserID)));
	}
	
	
	char* pSendData = NULL;
	char* pRecvData = NULL;	
	
	long dwSendLen = 0;
	long dwRecvLen = (Header.nDataCnt) * (Header.nDataSize);
	
	if(dwRecvLen > 0)
	{
		//			   012345678901234]
		infLOG(ALWAY, "ready Body Data] \n");		

		
		if(dwRecvLen > 0)
		{
			if(pRecvData)
				delete[] pRecvData;
			pRecvData = new char[dwRecvLen];
			memset(pRecvData, 0x00,dwRecvLen);
		}

		//			   012345678901234]
		infLOG(ALWAY, "Recv Body Data ] Size (%d) \n",dwRecvLen);			
		// client에서 data부을 수신
		if(RecvData(Socket, pRecvData, dwRecvLen )< 0)
		{
			//			   012345678901234]			
			infLOG(ERROR, "Recv Body Data ] Recv Body Data Exception \n");					
			
			#ifdef __DEBUG
			printf("Recv Body Data ] Recv Body Data Exception \n");				
			#endif
			
			if(pRecvData)
				delete[] pRecvData;
				
			return;
		}
		
		//			   012345678901234]		
		infLOG(ALWAY, "processed start]\n");			
		dwSendLen = processed(Socket, (char *)&Header, pRecvData, pSendData);	//서비스호출
		infLOG(ALWAY, "processed end  ]\n");
					
		if(pRecvData)
			delete[] pRecvData;
	}
	else
	{
		//			   012345678901234]		
		infLOG(ALWAY, "processed start] with not body \n");			
		dwSendLen = processed(Socket, (char *)&Header, NULL, pSendData);	//서비스호출
		infLOG(ALWAY, "processed end  ] with not body \n");					
		
	}


	if(dwSendLen > 0)
	{
		//			   012345678901234]				
		infLOG(ALWAY, "processed send ] send data len > 0  \n");			
		if(SendData(Socket,(char*)pSendData, dwSendLen ) < 0)
		{
			//			   012345678901234]		
			int err  =errno;				
			infLOG(ERROR, "processed send ] sendData ( %d) (%d)\n",dwSendLen,err);
			#ifdef __DEBUG
			printf("processed send ] sendData ( %d) \n",dwSendLen);
			#endif
				
			if(pSendData)
			{
				delete[] pSendData;
				pSendData = NULL;
			}
			return;
			
		
		}

	}


	memset(&Header,0x00,HEADER_SIZE);
	//			   012345678901234]	
	infLOG(ALWAY, "recv Last Head ] EOL Recv  \n");
	
	//sleep(500);
	
	if(RecvData(Socket,(char*)&Header,HEADER_SIZE )< 0)
	{
		//			   012345678901234]		
		int err  =errno;
		infLOG(ERROR, "WairForRequest ] 응답 대기중 에러 3 - (%d): <client 죽음>\n",err); 
		#ifdef __DEBUG
		printf("WairForRequest ] 응답 대기중 에러 3: <client 죽음>\n");
		#endif
		if(pSendData)
		{
			delete[] pSendData;
			pSendData = NULL;
		}
			
		return;
	}
	
	
	
	if(Header.nCmd == RS_EOL)
	{	

		#ifdef __DEBUG
		printf("WairForRequest ]  RS_EOL\n");
		#endif
				//printf("send RS_EOL\n");
		//			   012345678901234]		
		infLOG(ALWAY, "WairForRequest ]  RS_EOL \n");		
		Header.nCmd = RS_EOL;
		Header.nDataCnt=0;
		Header.nDataSize =0;
		if(SendData(Socket,(char*)&Header, HEADER_SIZE ) < 0)
		{
			//			   012345678901234]			
			infLOG(ERROR, "Exception      ] Send RS_EOL error\n");
			
			#ifdef __DEBUG
			printf("WairForRequest ] EOL 전송 에러 4: <client 강제 종료>\n");
			#endif
			if(pSendData)
			{
				delete[] pSendData;
				pSendData = NULL;
			}

			return;
		}
	}
	else
	{
		//			   012345678901234]		
		infLOG(ERROR, "recv Last Head ] Recv Eol Error\n");	

		#ifdef __DEBUG
		printf("WairForRequest ] 에러 5: <처리 없음>\n");
		#endif
		
		//printf("RS_EOL not receive \n");
	}
		
	
	//			   012345678901234]	
	infLOG(ALWAY, "WaitForRequest ] End\n");
			
	if(pSendData)
	{
		delete[] pSendData;
		pSendData = NULL;
	}
}


