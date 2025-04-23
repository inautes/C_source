/******************************************************************************
 *   서브시스템 : Install파일 전송 서버
 *   프로그램명 : instmain.cc
 *         기능 : installserver의 Main
 *         설명 :
 *       작성자 : HCS
 *       작성일 : 2009/04/23
 *     수정이력 :
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/

#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <string.h>     /* for memset() */
#include <errno.h>
#include <pthread.h>        /* for POSIX threads */
#include <unistd.h>     /* for close() getpid()*/
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */

#include "installdefine.h"
#include "installsock.h" //sock send recv
#include "installproc.h"
#include "installcomlib.h"

#include "installsendproc.h"

#include "apstruct.h"
#include "apdefine.h" //for log
#include "comcomm.h" //for log
#include "comhead.h" //for log

extern multimap<int,USERINFO>m_UserList;
extern int gnMode;


int nListenNum = 5;

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

    infLOG(ALWAY, "]%d 쓰레드 (%d)(%s)\n", UserData.threadID, clntSock, pData->thread.userIP);

	//추가
	m_UserList.insert(pair<int,USERINFO>(clntSock,UserData));

	//id 넣기
    //free(pData);              /* Deallocate memory for argument */
    delete pData;

	struct timeval tv;
	tv.tv_sec = 60*2; //5초

	int st = setsockopt(clntSock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));
	if(st != 0)
	{
		//printf("recv timeout error\n");
		#ifdef __DEBUG
		if(errno == EBADF )
			printf("Socket Opt Erro = EBADF\n");
		if(errno == ENOTSOCK )
			printf("Socket Opt Erro = ENOTSOCK\n");
		if(errno == ENOPROTOOPT )
			printf("Socket Opt Erro = ENOPROTOOPT\n");
		if(errno == EFAULT)
			printf("Socket Opt Erro = EFAULT\n");
		printf(" ]errno number == %d\n , st = %d\n",errno,st);
		printf(" ]소켓 recv time out 옵션 설정 실패\n");
		#endif
		infLOG(ERROR, " ] 소켓 recv time out 옵션 설정 실패 errno = ( %d )\n",errno);
	}

	struct timeval tv2;
	tv2.tv_sec = 60*2; //5초

	st = setsockopt(clntSock, SOL_SOCKET, SO_SNDTIMEO, &tv2, sizeof(struct timeval));
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
		infLOG(ERROR, " ]소켓 send time out 옵션 설정 실패 errno = ( %d )\n",errno);
	}

    HandleTCPClient(clntSock);

	multimap<int,USERINFO>::iterator mi;
	mi = m_UserList.find(clntSock);
	if(mi != m_UserList.end())
	{
		m_UserList.erase(mi);
		close(clntSock);    /* Close client socket */
	}

	#ifdef __DEBUG
	printf(" ] 쓰레드 종료 ( %d ) \n\n\n\n",clntSock);
	#endif

    return (NULL);
}

void HandleTCPClient(int clntSocket)
{
	WaitForRequest(clntSocket);
}

int WaitForRequest(int& Socket)
{
	LPHEADER pHeader = new (nothrow) HEADER;
	memset(pHeader,0x00,HEADER_SIZE);
	bool nStop = false;

	// client에서 header을 수신
	char* pSendData;
	pSendData = NULL;

	char* pRecvData;
	pRecvData = NULL;

	while(nStop == false)
	{
		memset(pHeader,0x00,HEADER_SIZE);
		if(RecvData(Socket,(char*)pHeader,HEADER_SIZE ) <= 0) //HEADER_SIZE == sizeof( HEADER)
		{
			infLOG(ALWAY, " > Exception ) Recv Header 응답 대기중 응답없음으로 대기 종료. 소켓:(%d)\n", Socket);
			delete pHeader;
			pHeader=NULL;
			nStop = true;
			break;
		}

		if(pHeader->nCmd <= 0)
		{
			infLOG(ERROR, " > ERROR Exception ) pHeader->nCmd = %d 해더 오류 <client 죽음>\n", pHeader->nCmd);
			delete pHeader;
			pHeader=NULL;
			nStop = true;
			break;
		}
		else if( pHeader->nCmd != RS_INSTALL_FILE_DN 					 &&
				pHeader->nCmd != RS_INSTALL_FILE_DN_LIST 				 &&
				pHeader->nCmd != RS_EOL                               )
		{
			infLOG(ALWAY, " > ERROR > 정의 되지 않은 서비스 pHeader->Cmd = %d\n", pHeader->nCmd);
			delete pHeader;
			pHeader=NULL;
			nStop = true;
			break;
		}

		long dwSendLen = 0;
		long dwRecvLen = (pHeader->nDataCnt) * (pHeader->nDataSize);

		if(pSendData != NULL)
		{
			delete[] pSendData;
			pSendData = NULL;
		}

		if(dwRecvLen != 0) //  body 부분이 있으면...
		{
			pRecvData = new (nothrow) char[dwRecvLen];
			memset(pRecvData,0x00,dwRecvLen);

			// client에서 data부를수신
			if(RecvData(Socket, pRecvData, dwRecvLen )<= 0) //에러 나왔을때...
			{
				infLOG(ERROR, "> ERROR Exception ) Recv Body <=0 응답 대기중 에러[데이터부 수신에러] 2: <client 죽음>\n");
				if(pRecvData != NULL)
				{
					delete[] pRecvData;
					pRecvData = NULL;
				}
				delete pHeader;
				pHeader=NULL;

				nStop = true;
				break; //빠져 나가기..
			}
			dwSendLen = Processed(Socket,(char*)pHeader,pRecvData, pSendData);	//서비스호출
			if(pRecvData != NULL)
			{
				delete[] pRecvData;
				pRecvData = NULL;
			}
		}
		else //body 없으면
		{
			dwSendLen = Processed(Socket,(char *)pHeader, NULL, pSendData);	//서비스호출
		}

		if(dwSendLen > 0 ) //전송할 데이터가 있으면
		{
			if(pHeader->nCmd != RS_INSTALL_FILE_DN)
			{
				if(SendData(Socket,pSendData, dwSendLen ) <= 0)
				{
					infLOG(ERROR, " > ERROR Exception ) SendData <=0 결과 전송중 에러 1: <client 강제 종료 > \n");
					delete pHeader;
					delete[] pSendData;
					pHeader=NULL;
					pSendData = NULL;
					nStop = true;
					break;
				}
			}

			if(pSendData != NULL)
			{
				delete[] pSendData;
				pSendData = NULL;
			}
		}
		else //전송할 데이터가 없으면
		{
			if( dwSendLen == END)
			{
				if(SendData(Socket,pSendData, sizeof(HEADER) ) <= 0)
				{
					infLOG(ERROR, " > ERROR ) SendData <= 0 END 전송 에러 : <client 강제 종료 >\n");

					delete pHeader;
					delete[] pSendData;
					pHeader=NULL;
					pSendData = NULL;
					nStop = true;
					break;
				}
				if(pSendData != NULL)
				{
					delete[] pSendData;
					pSendData = NULL;
				}
			}
			nStop = true;
			break;
		}
	}

	if(pSendData != NULL)
		delete[] pSendData;
	if(pHeader != NULL)
		delete pHeader;
}


int Processed(int& Socket,char* pRecvHead, char* pRecvData, char* &pSendData)
{
	char  ErrMsg[256];
	int   nErrcode  = 0;
	long  dwSendLen = 0;

	LPHEADER pRHeader = (LPHEADER)pRecvHead;

	switch(pRHeader->nCmd)
	{
	case RS_INSTALL_FILE_DN_LIST:
		{
			nErrcode = FileRequestList(Socket,pRecvHead,pRecvData,pSendData);
			break;
		}
	case RS_INSTALL_FILE_DN:
		{
			nErrcode = FileRequestFile(Socket,pRecvHead,pRecvData,pSendData);
			break;
		}
	case RS_EOL:
		{
			nErrcode =   RequestEol(Socket,pRecvHead,pRecvData,pSendData);
			break;
		}
	default:
		{
			infLOG(ERROR, " ] 잘못된 Command 입니다. cmd ( %d )\n",pRHeader->nCmd);

			#ifdef __DEBUG
			printf(" ] 잘못된 Command 입니다. cmd ( %d )\n",pRHeader->nCmd);
			#endif

			ERR_HEADER errheader;
			memset(&errheader,0x00,sizeof(ERR_HEADER));
			pSendData = new (nothrow) char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));

			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_ERR;
			strcat(errheader.errmsg,"지원 하지 않는 서비스 입니다.");

			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

			nErrcode = -RS_ERR;
			break;

		}
	}

	if (nErrcode < 0)
	{
		if( nErrcode == END) // 정상
		{
			dwSendLen = END;
		}
		else //에러
		{
			infLOG(ERROR, "Exception ) nErrcode < 0 (%d) CMD : [%d]\n",nErrcode, pRHeader->nCmd);
			#ifdef __DEBUG
			printf("Exception ) nErrcode < 0 (%d) CMD : [%d]\n",nErrcode, pRHeader->nCmd);
			#endif
			dwSendLen = ERR_HEADER_SIZE;
		}
	}
	else
	{
		if(nErrcode ==0)
		{
			return 0;
		}
		if(pRHeader->nCmd == RS_INSTALL_FILE_DN )
		{
			return 1;
		}
		if(pSendData != NULL)
		{
			LPHEADER pSHeader = (LPHEADER)pSendData;
			pSHeader->nErrorCode = 0;
			dwSendLen = HEADER_SIZE + (pSHeader->nDataCnt * pSHeader->nDataSize);
		}
		else
		{
			dwSendLen = 0;
		}
	}
	return dwSendLen;
}

void *mon_thread(void  *data)
{
/*	printf("============================moniter thread================================\n");
	while(1)
	{
		sleep(60);

		multimap<int,USERINFO>::iterator mi;
		mi = m_UserList.begin();
		printf("size = %d\n",m_UserList.size());

		while(mi != m_UserList.end())
		{
			printf("Thread num = %d socket num = %d : IP Address  = %s : ID = %s\n",mi->second.threadID,mi->first,mi->second.thread.userIP,mi->second.szUserID);

			mi++;
		}
	}
	printf("============================moniter thread================================\n");
	*/

	int nMenu,nResult;
	char cMenu;
	int nCount=0;
	char szMenu[10];
	memset(szMenu,0x00,10);

	while(nMenu != 3)
	{
		printf("----------------------------------------------\n"
			   "      FDN SERVER 몌뉴를 선택해 주세요 	 	  \n"
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

				nListenNum = nResult ;

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



