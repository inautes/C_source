/******************************************************************************
 *   서브시스템 : File Upload 서버
 *   프로그램명 : fupmain.cc
 *         기능 : FUP서버의 Main
 *         설명 :
 *       작성자 : LEE
 *       작성일 : 2004/02/16
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


#include "fupdefine.h"
#include "fupsock.h" //sock send recv 
#include "fupproc.h"
#include "fupcomlib.h"

#include "fupweproc.h"
#include "fupmyproc.h"
#include "fupguruproc.h"
#include "fupcomproc.h"
#include "fupcomlib.h"



#include "apstruct.h"
#include "comhead.h"
#include "apdefine.h" //for log
#include "comcomm.h" //for log
#include "comconf.h"




long fups4001(CFUPS4001 PFUPS4001);
long fups4002(CFUPS4002 PFUPS4002,char *pErrMsg);
long fups4003(CFUPS4003 PFUPS4003,char *pErrMsg);

extern multimap<int,USERINFO>m_UserList;


////#define __DEBUG

int nListenNum = 5;





void DieWithError(char *errorMessage)
{
	
    printf(errorMessage);
    exit(1);
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
	tv.tv_sec = 60*3; //2분
	
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

		infLOG(ERROR, szErrorMsg); 
	}
	
	struct timeval tv2;
	tv2.tv_sec = 60*3; //2분
				
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

		infLOG(ERROR, szErrorMsg); 
	}
	
    HandleTCPClient(clntSock);
    
    //clear user info on UserMap
	multimap<int,USERINFO>::iterator mi;
	//mi = m_UserList.begin(); change
	mi = m_UserList.find(clntSock);
	if(mi != m_UserList.end())
	{
		infLOG(ALWAY, "ThreadMain           ] ( %s ) close thread and socket \n",mi->second.szUserID);
				
		m_UserList.erase(mi);
		close(clntSock);    // Close client socket 

	}
	else
		infLOG(ALWAY, "ThreadMain           ] (unknown) close thread and socket\n\n\n\n\n");



    return (NULL);
}



void HandleTCPClient(int clntSocket)
{
 
	WaitForRequest(clntSocket);	
	//CheckUser();
}


int WaitForRequest(int& Socket)
{
	
	LPHEADER pHeader = new HEADER;
	memset(pHeader,0x00,HEADER_SIZE);
	bool nStop = false;

	// client에서 header을 수신
	char* pSendData ;
	pSendData = NULL;
	char* pRecvData;
	pRecvData = NULL;
	
	#ifdef __DEBUG
//			01234567890123456789]	
	printf(" ] ----------| Waiting |----------\n");
	#endif	
	infLOG(ALWAY, " ] ----------| Waiting |----------\n");
	
	while(nStop == false)
	{
		#ifdef __DEBUG
	//			01234567890123456789]	
		printf(" ] Waiting Recv\n");
		#endif
		
		memset(pHeader,0x00,HEADER_SIZE);
		infLOG(ALWAY, " ] Waiting Recv size ( %d )\n",HEADER_SIZE);
		
		if(RecvData(Socket,(char*)pHeader,HEADER_SIZE ) <= 0)
		{
			#ifdef __DEBUG
		//			01234567890123456789]	
			printf(" ] (%s) Exceptin Recv\n",pHeader->szUserID);
			#endif
			infLOG(ERROR, " ] (%s) Exceptin Recv\n",pHeader->szUserID);			
			
			delete pHeader;
			pHeader=NULL;
			nStop = true;
			break;
		}
		infLOG(ALWAY, " ] cmd check ( %d )\n",pHeader->nCmd );
		if(pHeader->nCmd <= 0)
		{
			#ifdef __DEBUG
		//			01234567890123456789]	
			printf(" ] Exceptin Command (%d) \n",pHeader->nCmd);
			#endif
						
			infLOG(ERROR, " ] Exceptin Command (%d) \n",pHeader->nCmd);
	
	
			delete pHeader;
			pHeader=NULL;
			nStop = true;
			break;
		}
		
		long dwSendLen = 0;
		long dwRecvLen = (pHeader->nDataCnt) * (pHeader->nDataSize);
	
		infLOG(ALWAY, " ] recv data len  ( %d )\n",dwRecvLen );	
				
		if(pSendData != NULL)
		{
			delete[] pSendData;
			pSendData = NULL;
		}	
	
		if(dwRecvLen != 0)
		{		
			if(pRecvData != NULL)
			{

				delete[] pRecvData;
				pRecvData = NULL;
			}
			
			pRecvData = new char[dwRecvLen];
			memset(pRecvData,0x00,dwRecvLen);
	
			// client에서 data부을 수신
			infLOG(ALWAY, " ] recv body len  ( %d )\n",dwRecvLen );	
			if(RecvData(Socket, pRecvData, dwRecvLen )<= 0)
			{
				#ifdef __DEBUG
			//			01234567890123456789]	
				printf(" ] (%s) Exceptin Body Data\n",pHeader->szUserID);
				#endif
											
				infLOG(ERROR, " ] (%s) Exceptin Body Data\n",pHeader->szUserID);

				
				delete[] pRecvData;
				delete pHeader;
				pHeader=NULL;
				pRecvData = NULL;
				nStop = true;
				break;
			}
	
			dwSendLen = processed(Socket,(char*)pHeader,pRecvData, pSendData);	//서비스호출
			
		}
		else
		{

			dwSendLen = processed(Socket,(char *)pHeader, NULL, pSendData);	//서비스호출
		}
	
		
		
		if(dwSendLen > 0 )
		{

			if(dwSendLen != 1) // if dwSednLen equal 1 , all job did finished 
			//1 일때는 모든 작업을 각 해당 함수안에서 끝낸 상태이다.		
			{
 
				if(SendData(Socket,pSendData, dwSendLen ) <= 0)
				{
					#ifdef __DEBUG
				//			01234567890123456789]	
					printf(" ] (%s) Request Send Packet Error \n",pHeader->szUserID);
					#endif					
					infLOG(ERROR, " ] (%s) Request Send Packet Error \n",pHeader->szUserID);	
					
					delete pHeader;	
					delete[] pSendData;
					pHeader=NULL;
					pSendData = NULL;
					nStop = true;
					break;
				}
				#ifdef __DEBUG
			//			01234567890123456789]	
				printf(" ] (%s) Request Send Packet \n",pHeader->szUserID);
				#endif					
				infLOG(ALWAY, " ] (%s) Request Send Packet \n",pHeader->szUserID);	
							
			}
			else
			{
				#ifdef __DEBUG
			//			01234567890123456789]	
				printf(" ] (%s) All Job did Finished \n",pHeader->szUserID);
				#endif					
				infLOG(ALWAY, " ] (%s) All Job did Finished \n",pHeader->szUserID);
							
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
			
				if(SendData(Socket,pSendData, sizeof(HEADER) ) <= 0)
				{
					#ifdef __DEBUG
				//			01234567890123456789]	
					printf(" ] (%s) (END) Request Send Packet Error \n",pHeader->szUserID);
					#endif					
					infLOG(ERROR, " ] (END) (%s) Request Send Packet Error \n",pHeader->szUserID);	
					
					if(pHeader)
						delete pHeader;	
					if(pSendData)
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
				#ifdef __DEBUG
			//			01234567890123456789]	
				printf(" ] (%s) (END) Request Send Packet \n",pHeader->szUserID);
				#endif					
				infLOG(ALWAY, " ] (%s) (END) Request Send Packet \n",pHeader->szUserID);	
								
			}
			
			nStop = true;
			break;
		}
	}


	if(pRecvData != NULL)
	{

		delete[] pRecvData;
		pRecvData = NULL;
	}
	if(pSendData != NULL)
		delete[] pSendData;
	if(pHeader != NULL)
		delete pHeader;	
		
	#ifdef __DEBUG
//			01234567890123456789]	
	printf(" ] ----------| End |----------\n");
	#endif					
	infLOG(ALWAY, " ] ----------| End |----------\n");
		
	
	return -1;
}





long processed(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{	
	char  ErrMsg[256];
	int   nErrcode  = 0;
	long  dwSendLen = 0;

	LPHEADER pRHeader = (LPHEADER)pRecvHead;

	#ifdef __DEBUG
//			01234567890123456789]	
	printf("run processed		] code num ( %d ) user id ( %s )\n",pRHeader->nCmd,pRHeader->szUserID);
	#endif					
	infLOG(ALWAY, "run processed		] code num ( %d ) user id ( %s )\n",pRHeader->nCmd,pRHeader->szUserID);
	
	
	
	switch(pRHeader->nCmd)
	{
	
	//free command
	case RS_GURU_FILE_UP: // x
		{
			nErrcode = RequestGuruFilUp(Socket,pRecvHead,pRecvData,pSendData);
			break;
		}
	case RS_GURUFILE_REQUEST_LIST : // x
		{
			nErrcode = GuruFileRequestList(Socket,pRecvHead, pRecvData, pSendData);
			break;
		}
	//admin command	
	case RS_CMD_REQUEST_USER_LIST: // x
		{
			nErrcode = RequestUserList(Socket,pRecvHead,pRecvData,pSendData);
			break;
		}
	case RS_MYDISK_FILE_REQUEST_LIST: // up
		{	
			nErrcode = MyDiskFileRequestList(Socket,pRecvHead, pRecvData, pSendData);
			break;	
		}			
	case RS_FILE_REQUEST_LIST : // up, down
		{
			nErrcode = FileRequestList(Socket,pRecvHead, pRecvData, pSendData);
			break;
		}
	case RS_MYDISK_FILE_DATA_TRANSFER: // x
		{	
			nErrcode = MyDiskFileDataTransfer(Socket,pRecvHead, pRecvData, pSendData);
			break;					
		}	
	case RS_FILE_DATA_TRANSFER : // dn
		{
			nErrcode = FileDataTransfer(Socket,pRecvHead, pRecvData, pSendData);
			break;
		}
	case RS_FILE_REQUEST_ID_DISCONNECT: // x
		{
			nErrcode =  FileRequestIdDisconnect(Socket,pRecvHead, pRecvData, pSendData);
			break;
		}
	case RS_ADMIN_REQUEST_ID_DISCONNECT: // x
		{
			nErrcode =  AdminRequestIdDisconnect(Socket,pRecvHead, pRecvData, pSendData);
			break;
		}
	case RS_FILE_CHECK_ID: // dn,up
		{
			nErrcode =  FileCheckID(Socket,pRecvHead, pRecvData, pSendData);
			break;
		}
	case RS_EOL: // up,down
		{
			nErrcode =  RequestEol(Socket,pRecvHead,pRecvData,pSendData);
			break;
		}
	case RS_SPEED_CHECK: // x
		{
			nErrcode =  RequestSpeedCheck(Socket,pRecvHead, pRecvData,pSendData);
			break;			
	 	}
	case RS_MYDISK_FILE_REQUEST_NEXT_FILE: // x
		{
			nErrcode = MyDiskFileRequestNextFile(Socket,pRecvHead, pRecvData, pSendData);
			break;
		}				
	case RS_FILE_REQUEST_NEXT_FILE: // dn
		{
			nErrcode = FileRequestNextFile(Socket,pRecvHead, pRecvData, pSendData);
			break;
		}		
	case RS_MYDISK_FILE_REQUEST_FILE: // up
		{	
			nErrcode =  MyDiskFileRequestFile(Socket,pRecvHead, pRecvData,pSendData);
			break;
		}
	
	case RS_FILE_REQUEST_FILE:// up,down 
		{	
			nErrcode =  FileRequestFile(Socket,pRecvHead, pRecvData,pSendData);
			break;
		}

	default:
		{
			#ifdef __DEBUG
		//			01234567890123456789]	
			printf("run processed Defaul] Exception code num ( %d ) user id ( %s )\n",pRHeader->nCmd,pRHeader->szUserID);
			#endif					
			infLOG(ERROR, "run processed Defaul] Exception code num ( %d ) user id ( %s )\n",pRHeader->nCmd,pRHeader->szUserID);			
			
			ERR_HEADER errheader;
			memset(&errheader,0x00,sizeof(ERR_HEADER));
			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_ERR;
			strcat(errheader.errmsg,"지원 하지 않는 서비스 입니다.");
			// we dont have service
			//not have service
			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));
			nErrcode = -RS_ERR;
			break;
		}
	}

	#ifdef __DEBUG
//			01234567890123456789]	
	printf("end processed		] code num ( %d ) user id ( %s )\n",pRHeader->nCmd,pRHeader->szUserID);
	#endif					
	infLOG(ALWAY, "end processed		] code num ( %d ) user id ( %s )\n",pRHeader->nCmd,pRHeader->szUserID);
		//			01234567890123456789]		
	infLOG(ALWAY,  "checing processed	] errorcode (%d) ) \n",nErrcode);
	if (nErrcode < 0)
	{
		
		
		dwSendLen = ERR_HEADER_SIZE;
		if( nErrcode == END)
			dwSendLen = END;
			
	}
	else
	{

		if(nErrcode ==0)
		{
			
			return 0;
		}
		if(pRHeader->nCmd == RS_FILE_DATA_TRANSFER
		 ||pRHeader->nCmd == RS_GURU_FILE_UP
		 ||pRHeader->nCmd == RS_MYDISK_FILE_DATA_TRANSFER)
		{
			return 1;
		}
			
		LPHEADER pSHeader = (LPHEADER)pSendData;	
		pSHeader->nErrorCode = 0;	
		dwSendLen = HEADER_SIZE + (pSHeader->nDataCnt * pSHeader->nDataSize);
	
	}

	#ifdef __DEBUG
//			01234567890123456789]	
	printf("end processed		]  size (%d)code num ( %d ) user id ( %s )\n",dwSendLen,pRHeader->nCmd,pRHeader->szUserID);
	#endif					
	infLOG(ALWAY, "end processed		]  size (%d)code num ( %d ) user id ( %s )\n",dwSendLen,pRHeader->nCmd,pRHeader->szUserID);
	
	return dwSendLen;
}











void *mon_thread(void  *data)
{
	/*
	while(1)
	{
		sleep(120);
		
		multimap<int,USERINFO>::iterator mi;
		mi = m_UserList.begin();
		printf("============================moniter thread================================\n");
		printf("Total Connect = %d\n",m_UserList.size());
		
		while(mi != m_UserList.end())
		{
			printf("socket num = %d : IP Address  = %s : ID = %s\n",mi->first,mi->second.thread.userIP,mi->second.szUserID);
			
			mi++;
		}
		printf("============================moniter thread================================\n");
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




