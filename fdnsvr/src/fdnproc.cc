#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <string.h>     /* for memset() */
#include <errno.h>
#include <pthread.h>        /* for POSIX threads */
#include <unistd.h>     /* for close() getpid()*/
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>

#include "fdndefine.h"
#include "fdnsock.h" //sock send recv
#include "fdnproc.h"
#include "fdncomlib.h"

#include "fdndownproc.h"
#include "fdnguruproc.h"
#include "fdncomproc.h"

#include "apstruct.h"
#include "comhead.h"
#include "apdefine.h" //for log
#include "comcomm.h" //for log
#include "comconf.h"


extern multimap<int,USERINFO>m_UserList;
extern char g_szOsp_id[10];
extern char g_szDcmdIP[16];
extern int  g_nDcmdPort; //  = 0;

extern char g_szSUB_DcmdIP[16];
extern int  g_nSUB_DcmdPort;
extern bool g_bConnect1 ;
extern bool g_bConnect2 ;

int nListenNum = 10; // 대기자수


void *ThreadMain(void *threadArgs)
{
    int clntSock;
    pthread_detach(pthread_self());
	LPUSERINFO pData = (LPUSERINFO) threadArgs;

	USERINFO UserData ;
	memset(&UserData,0x00,sizeof(USERINFO));
	UserData.thread.clntSock = pData->thread.clntSock;
	UserData.threadID = getpid();
	strcpyA(UserData.thread.userIP,pData->thread.userIP);


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

	infLOG(ALWAY, " ] 쓰레드 시작 ( %d )\n",pData->thread.clntSock);

    ///////////////

	//추가 하기전 삭제
	// 아이디 중복 채크 제거//
	/*
	multimap<int,USERINFO>::iterator mi;

	//mi = m_UserList.begin(); change
	mi = m_UserList.find(Socket);
	if(mi != m_UserList.end())
	{
		memcpy(mi->second.szUserID, pHeader->szUserID , sizeof(pHeader->szUserID));
		#ifdef __DEBUG
		printf(" ] insert id : %s\n",mi->second.szUserID);
		#endif
	}
	*/

	//추가
	m_UserList.insert(pair<int,USERINFO>(clntSock,UserData));

	SendRealCount(1,0);

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
		infLOG(ALWAY, " ] 소켓 recv time out 옵션 설정 실패 errno = ( %d )\n",errno);
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
		infLOG(ALWAY, " ]socket send time out 옵션 설정 실패 errno = ( %d )\n",errno);
	}

    HandleTCPClient(clntSock);

	//mutex 목록에서 삭제.

	multimap<int,USERINFO>::iterator mi;
	//mi = m_UserList.begin();
	mi = m_UserList.find(clntSock);
	if(mi != m_UserList.end())
	{
		infLOG(ALWAY, " ] socket delete ID(%s)\n",mi->second.szUserID);

		//if(mi->second.gCom9xxx.nStatus != 0)
		//{
		//	com9102(mi->second.gCom9xxx.com9102_R);
		//}

		m_UserList.erase(mi);
		close(clntSock);    /* Close client socket */
	}

	infLOG(ALWAY, " ] thread exit ( %d ) \n\n",clntSock);

	SendRealCount(1,0);


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
	char* pSendData ;
	pSendData = NULL;

	char* pRecvData;
	pRecvData = NULL;

	while(nStop == false)
	{
		memset(pHeader,0x00,HEADER_SIZE);
//		infLOG(ALWAY, " ]WaitProc: Recv Header\n");

		if(RecvData(Socket,(char*)pHeader,HEADER_SIZE ) <= 0) //HEADER_SIZE == sizeof( HEADER)
		{
			infLOG(ERROR, " ]WaitProc: ERROR Exception Recv Header\n");

			delete pHeader;
			pHeader=NULL;
			nStop = true;
			break;
		}

		infLOG(ALWAY, " ]WaitForRequest: Check ) pHeader->nCmd (%d) (%d) (%d) (%s) \n",pHeader->nCmd,pHeader->nDataCnt,pHeader->nDataSize,pHeader->szUserID);

		if(pHeader->nCmd <= 0)
		{
			infLOG(ERROR, " ]WaitProc: ERROR Exception pHeader->nCmd <=0 \n");

			delete pHeader;
			pHeader=NULL;
			nStop = true;
			break;
		}
		else if( pHeader->nCmd != RS_FILE_REQUEST_FILE 				 &&
				pHeader->nCmd != RS_FILE_DATA_TRANSFER 				 &&
				pHeader->nCmd != RS_FILE_REQUEST_NEXT_FILE 			 &&
				pHeader->nCmd != RS_FILE_REQUEST_NEXT_FILEINFO 		 &&
				pHeader->nCmd != RS_FILE_CHECK_ID_OK 				 &&
				pHeader->nCmd != RS_FILE_CHECK_ID_FAIL 				 &&
				pHeader->nCmd != RS_FILE_REQUEST_CONTINUE 			 &&
				pHeader->nCmd != RS_FILE_CHECK_ID					 &&
				pHeader->nCmd != RS_FILE_REQUEST_ID_DISCONNECT 		 &&
				pHeader->nCmd != RS_FILE_REQUEST_ID_DISCONNECT_OK 	 &&
				pHeader->nCmd != RS_FILE_REQUEST_LIST 				 &&
				pHeader->nCmd != RS_FDN_REQUEST_HOLD_TIME			 &&
				pHeader->nCmd != RS_FILE_REQUEST_FILE_WITH_HOLD_TIME &&
				pHeader->nCmd != RS_FILE_REQUEST_FILE_FILINFO 		 &&
				pHeader->nCmd != RS_CMD_REQUEST_USER_LIST 			 &&
				pHeader->nCmd != RS_ADMIN_REQUEST_ID_DISCONNECT		 &&
				pHeader->nCmd != RS_ADMIN_REQUEST_ID_DISCONNECT_OK	 &&
				pHeader->nCmd != RS_REQUEST_CYBER_MONEY 			 &&
				pHeader->nCmd != RS_REQUEST_CYBER_MONEY_READY 		 &&
				pHeader->nCmd != RS_GURU_FILE_DN 					 &&
				pHeader->nCmd != RS_GURU_FILE_DN_LIST 				 &&
				pHeader->nCmd != RS_EOL                               &&
				pHeader->nCmd != RS_GRID_DATA_TRANSFER	 			&&
				pHeader->nCmd != RS_GRID_STOP			 			&&
				pHeader->nCmd != RS_GRID_FAIL			 			&&
				pHeader->nCmd != RS_GRID_NEXT_FILE		 			&&
				pHeader->nCmd != RS_GRID_COMPLETION		 			&&
				pHeader->nCmd != RS_FILE_REQUEST_FILE_WITH_HOLD_TIME_GRID &&
				pHeader->nCmd != RS_FILE_REQUEST_FILE_GRID 			&&
				pHeader->nCmd != RS_GRID_KEEPALIVE_CHECK &&
				pHeader->nCmd != RS_GRID_DATA_TRANSFER_NEXT			&&
				pHeader->nCmd != RS_EOL )
		{
			infLOG(ERROR, " > ERROR > pHeader->Cmd 오류 \n");

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

			// client에서 data부을 수신

			if(RecvData(Socket, pRecvData, dwRecvLen )<= 0) //에러 나왔을때...
			{
				infLOG(ERROR, " ]WaitForRequest: ERROR Exception Recv Body\n");

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
		else
		{
			dwSendLen = Processed(Socket,(char *)pHeader, NULL, pSendData);	//서비스호출
		}

		if(dwSendLen > 0 )
		{

			if(pHeader->nCmd != RS_FILE_REQUEST_FILE &&
				pHeader->nCmd != RS_FILE_REQUEST_FILE_GRID &&
				pHeader->nCmd != RS_FILE_REQUEST_FILE_WITH_HOLD_TIME &&
				pHeader->nCmd != RS_FILE_REQUEST_FILE_WITH_HOLD_TIME_GRID &&
				pHeader->nCmd != RS_GURU_FILE_DN)
			{

				long ret = 0;
				ret = SendData(Socket,pSendData, dwSendLen );
				infLOG(ALWAY, " ]Send cmd(%d) data Len(%d)=(%ld)\n",((LPHEADER)pSendData)->nCmd, dwSendLen,ret);
				if(ret  <= 0)
				{
					infLOG(ERROR, " ]WaitProc: ERROR Exception Send Data\n");

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
		else
		{
			if( dwSendLen == END)
			{
				infLOG(ALWAY, " ]WaitProc: Send END cmd(%d)\n",pHeader->nCmd);
				if(SendData(Socket,pSendData, sizeof(HEADER) ) <= 0)
				{
					infLOG(ERROR, " ]WaitProc: ERROR Exception Send END\n");

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

	infLOG(ALWAY, " ]Processed: Cmd (%d) (%s)\n",pRHeader->nCmd,pRHeader->szUserID);

	switch(pRHeader->nCmd)
	{
	case RS_GURU_FILE_DN_LIST:
		{
			nErrcode = GuruFileRequestList(Socket,pRecvHead,pRecvData,pSendData);
			break;
		}
	case RS_GURU_FILE_DN:
		{
			nErrcode = GuruFileRequestFile(Socket,pRecvHead,pRecvData,pSendData);
			break;
		}

	case RS_FILE_REQUEST_ID_DISCONNECT:
		{
			nErrcode =  FileRequestIdDisconnect(Socket,pRecvHead,pRecvData,pSendData);
			break;
		}

	case RS_FILE_CHECK_ID:
		{
			nErrcode =   FileCheckID(Socket,pRecvHead,pRecvData,pSendData);
			break;
		}


	case RS_FILE_REQUEST_NEXT_FILE:
		{
			nErrcode =   FileRequestNextFile(Socket,pRecvHead,pRecvData,pSendData);
			break;

		}
	case RS_FILE_REQUEST_FILE_WITH_HOLD_TIME_GRID:
	case RS_FILE_REQUEST_FILE_WITH_HOLD_TIME:
		{
			nErrcode =   FileRequestFileWithHoldTime(Socket,pRecvHead,pRecvData,pSendData);
			break;
		}
	case RS_FILE_REQUEST_FILE_GRID:
	case RS_FILE_REQUEST_FILE:
		{
			nErrcode =   FileRequestFile(Socket,pRecvHead,pRecvData,pSendData);
			break;
		}

	case RS_FILE_REQUEST_LIST: // 목록 조회
		{
			nErrcode = FileRequestList(Socket,pRecvHead,pRecvData,pSendData);
			break;
		}
	case RS_FDN_REQUEST_HOLD_TIME: //정액제 시간 보내기
		{
			nErrcode = RequesetHoldTime(Socket,pRecvHead,pRecvData,pSendData);
			break;
		}
	case RS_GRID_KEEPALIVE_CHECK :
		{
			nErrcode =   FileRequestGridKeepAliveCheck(Socket,pRecvHead,pRecvData,pSendData);
			break;
		}
	case RS_GRID_FAIL :
		{
			nErrcode =   FileRequestGridFail(Socket,pRecvHead,pRecvData,pSendData);
			break;
		}
	case RS_GRID_DATA_TRANSFER_NEXT :
		{
			nErrcode =	FileRequestGridTransferNext(Socket,pRecvHead,pRecvData,pSendData);
			break;
		}
	case RS_GRID_COMPLETION:
	case RS_GRID_STOP:
	case RS_EOL:
		{
			nErrcode =   RequestEol(Socket,pRecvHead,pRecvData,pSendData);
			break;
		}

	default:
		{
			infLOG(ERROR, " ]Processed: 잘못된 Command 입니다. cmd ( %d )\n",pRHeader->nCmd);

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

	infLOG(ALWAY, " ]Processed: Cmd (%d) (%s) return code(%d) \n",pRHeader->nCmd,pRHeader->szUserID,nErrcode);

	if (nErrcode < 0)
	{
		dwSendLen = ERR_HEADER_SIZE;

		if( nErrcode == END)
		{
			dwSendLen = END;
		}
	}
	else
	{
		if(nErrcode ==0)
		{
			return 0;
		}
		if(pRHeader->nCmd == RS_FILE_REQUEST_FILE
			|| pRHeader->nCmd == RS_FILE_REQUEST_FILE_WITH_HOLD_TIME
			|| pRHeader->nCmd == RS_GURU_FILE_DN  )
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

	/*

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
	*/
}



void SendRealCount(int nAppand,unsigned long uFileid)
{

	int nRealCount=0;
	multimap<int,USERINFO>::iterator mi;
	mi = m_UserList.begin();

	while(mi != m_UserList.end())
	{
		if(mi->second.nRealDown == 1)
			nRealCount++;

		mi++;
	}

 	infLOG(ALWAY, "SendRealCount = TotalCount = %d , nRealCount= %d \n",m_UserList.size(),nRealCount);

	CCOM9009_R com9009_R;
	memset(&com9009_R , 0x00 , sizeof(CCOM9009_R));

	char* pHost = getenv("HOSTNAME");
	if( pHost == NULL || sizeof(pHost) < 3)
		pHost = Func_getHostname();

	strcpyA(com9009_R.server_id , pHost);
	com9009_R.nConnectCnt = m_UserList.size();
	com9009_R.nMode = 0; // 0 : down , 1: up

//	com9009(com9009_R);
/*
	strcpyA(com9009_R.server_id , pHost);
	strcpyA(com9009_R.osp_id , &g_szOsp_id);
	com9009_R.nConnectCnt = m_UserList.size();
	com9009_R.nMode = 0;
	com9009_R.nRealDownCnt = nRealCount;
	com9009_R.nAppand = nAppand;
	com9009_R.uFileid = uFileid;
	com9009_R.nPort = g_nPort;
*/
// 	infLOG(ALWAY, "insert Down User Count = %d \n",m_UserList.size());

	int nRet=-1;
	if(g_bConnect1 == true)
	{
		nRet = com9009(com9009_R,g_szDcmdIP,g_nDcmdPort) ;
	}
	else
	{	// 1번 안되면 2번 무조건 접속.
		nRet = com9009(com9009_R,g_szSUB_DcmdIP,g_nSUB_DcmdPort) ;
	}


	//com9009(com9009_R,&g_szDcmdIP,g_nDcmdPort); // server count update +
}
