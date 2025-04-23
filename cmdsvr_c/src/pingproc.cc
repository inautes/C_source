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
#include "cmdmain.h"
#include "comhead.h"
#include "comcomm.h"
#include "comconf.h"
using namespace std;
//#define __DEBUG

extern int m_nListenNum;
extern pthread_cond_t *mycond;



void *thread_func(void *data)
{

	int mysocket;
	int mynum = *((int *)data);
	//printf("thread create -->  %d\n",mynum);
	multimap<int,ph>::iterator mi;

	pthread_mutex_lock(&async_mutex);
	pthread_cond_signal(&async_cond);
	pthread_mutex_unlock(&async_mutex);

	//printf("thread create %d\n",mynum);


	while(1)
	{
		pthread_mutex_lock(&mutex_lock);
		pthread_cond_wait(&mycond[mynum],&mutex_lock);
		mysocket = s_info.current_sockfd;
		pthread_mutex_unlock(&mutex_lock);
		
		struct timeval tv;
		tv.tv_sec = 60*60; //5분
	
	
	//	if (setsockopt(mysocket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(struct timeval)) != 0)
	//		//printf("send timeout error\n");
			
			
		int st = setsockopt(mysocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));
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
			printf("errno number == %d\n , st = %d\n",errno,st);	
			printf("소켓 recv time out 옵션 설정 실패\n");
			#endif
			infLOG(ERROR, "소켓 recv time out 옵션 설정 실패\n"); 
		}
		
		struct timeval tv2;
		tv2.tv_sec = 60*60; //5초
					
		st = setsockopt(mysocket, SOL_SOCKET, SO_SNDTIMEO, &tv2, sizeof(struct timeval));
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
			infLOG(ERROR, "소켓 send time out 옵션 설정 실패\n");
		}
			
			
		infLOG(ALWAY, "WaitForRequest(mysocket)\n");			
			
		WaitForRequest(mysocket);	
		
		infLOG(ALWAY, "end WaitForRequest(mysocket)\n");			
		
		//mi = s_info.phinfo.begin(); change
		mi = s_info.phinfo.find(1);
		
		while(mi != s_info.phinfo.end())
		{
		
			if(mi->second.index_num == mynum)
			{
				ph tmpph;
				tmpph.index_num = mynum;
				tmpph.sockfd = 0;
				s_info.phinfo.erase(mi);
				s_info.phinfo.insert(pair<int,ph>(0,tmpph));
				s_info.client_num --;
			
				#ifdef __DEBUG
				printf(" close socket %d \n",mysocket);
				#endif
				
				close(mysocket);
								
				break;
			}
			mi++;
		}
	}
}


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
			   "1 (쓰레드풀 조회) : 2 (대기열 갯수) : 3 (종료)\n"
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
				multimap<int,ph>::iterator mi;
				mi = s_info.phinfo.begin();
				
				printf("-----------------------------------------------\n"
					   "			 현재 쓰레드 풀 상황               \n"
					   "-----------------------------------------------\n");
						   
				while(mi != s_info.phinfo.end())
				{
					if(mi->first == 1)
					{
						printf("%4d> [쓰레드 번호: : %d] [연결 상황  : %d]  : [주소 : %s] \n"
								,nCount +1,  mi->second.index_num ,mi->first   , mi->second.szIP);
						nCount++;
					}
					mi++;
					
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
				m_nListenNum = nResult ;
				
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
				
					delete[] mycond;
					
					multimap<int,ph>::iterator mi;
					mi = s_info.phinfo.begin();
				
					while(mi != s_info.phinfo.end())
					{
						
						if(mi->first == 1)
						{
							#ifdef __DEBUG
							printf("close socket %d\n",mi->second.sockfd);
							#endif
							close(mi->second.sockfd);
						}
						mi++;
					}
				
					
					exit(1);
					
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
** Client 송수신
*******************************************************************************/
void WaitForRequest(int& Socket)
{
	LPHEADER pHeader = new HEADER;
	memset(pHeader,0x00,HEADER_SIZE);
	
//printf("WaitForRequest-> start header size(%d)\n", HEADER_SIZE);
	
	// client에서 header을 수신
	multimap<int,ph>::iterator mi;
infLOG(ALWAY, "Input ) WaitForRequest\n");	
infLOG(ALWAY, "Input ) Recv Header\n");	
	
	if(RecvData(Socket,(char*)pHeader,HEADER_SIZE )<= 0)
	{
		infLOG(ERROR, "WaitForRequst : 응답 대기중 에러 1: <client 죽음>\n"); 
		#ifdef __DEBUG
		printf("WaitForRequst : 응답 대기중 에러 1: <client 죽음>\n");
		#endif
		delete pHeader;
		return;
	}
infLOG(ALWAY, "->Input Thread info\n");	

	int nLen = sizeof(pHeader->szUserID);
	//mi = s_info.phinfo.begin(); change
	mi = s_info.phinfo.find(1);
	while(mi != s_info.phinfo.end())
	{
		
		if(mi->first == 1 && mi->second.sockfd == Socket)
		{
			memcpy(mi->second.szID,pHeader->szUserID,min(  nLen ,12 )  );
			#ifdef __DEBUG
			printf("--> [쓰레드 번호: : %d] [연결 상황  : %d]  : [주소 : %s] :[아이디 : %s]\n"
								,  mi->second.index_num ,mi->first   , mi->second.szIP,mi->second.szID);
			#endif
			break;
		}
		mi++;
	}
	
	//char* pSendData = new char[1*1024*1024]; //send buffer
	char* pSendData = NULL;
	long dwSendLen = 0;
	long dwRecvLen = (pHeader->nDataCnt) * (pHeader->nDataSize);

	//memset(pSendData, 0x00, 1*1024*1024);
	

	if(dwRecvLen != 0)
	{

infLOG(ALWAY, "-> Recv Body\n");		
		char* pRecvData = NULL;
		
		if(dwRecvLen > 0)
		{
			if( pRecvData)
				delete[] pRecvData;
			pRecvData = new char[dwRecvLen];
			memset(pRecvData, 0x00,dwRecvLen);
		}
	
		// client에서 data부을 수신
		if(RecvData(Socket, pRecvData, dwRecvLen )< 0)
		{
infLOG(ERROR, "Exception ) Recv Body Error\n");					
			
			#ifdef __DEBUG
			printf("WaitForRequst : 응답 대기중 에러 2: <client 죽음>\n");
			#endif
			
			if(pRecvData)
				delete[] pRecvData;
			if(pHeader)
				delete pHeader;
			return;
		}
		infLOG(ALWAY, "processed start \n");			
		
		dwSendLen = processed((char *)pHeader, pRecvData, pSendData);	//서비스호출
		if(pRecvData)
			delete[] pRecvData;
	}
	else
	{
infLOG(ALWAY, "->Not Have Body \n");		
		dwSendLen = processed((char *)pHeader, NULL, pSendData);	//서비스호출
		
	}


	if(dwSendLen > 0)
	{
		infLOG(ALWAY, "dwSendLen > 0 \n");			
		if(SendData(Socket,(char*)pSendData, dwSendLen ) < 0)
		{
			infLOG(ERROR, "WaitForRequst : 결과 전송중 에러 2: <client 강제 종료 >\n"); 
			#ifdef __DEBUG
			printf("WaitForRequst : 결과 전송중 에러 2: <client 강제 종료 >\n");
			#endif
			if(pHeader)
				delete pHeader;
			if(pSendData)
				delete[] pSendData;
			return;
			
		
		}

	}


	memset(pHeader,0x00,HEADER_SIZE);
	
	infLOG(ALWAY, "Line 386 \n");			
	
	if(RecvData(Socket,(char*)pHeader,HEADER_SIZE )< 0)
	{
		infLOG(ERROR, "WaitForRequst : 응답 대기중 에러 3: <client 죽음>\n"); 
		#ifdef __DEBUG
		printf("WaitForRequst : 응답 대기중 에러 3: <client 죽음>\n");
		#endif
		if(pSendData)
			delete[] pSendData;
		if(pHeader)
			delete pHeader;
		return;
	}
	
	
	
	if(pHeader->nCmd == RS_EOL)
	{	
		//printf("send RS_EOL\n");
		
infLOG(ALWAY, "->Check Cmd (RS_EOL) \n");		
		pHeader->nCmd = RS_EOL;
		pHeader->nDataCnt=0;
		pHeader->nDataSize =0;
		if(SendData(Socket,(char*)pHeader, HEADER_SIZE ) < 0)
		{
infLOG(ERROR, "Exception ) Send RS_EOL error\n");
			
			#ifdef __DEBUG
			printf("WaitForRequst : EOL 전송 에러 4: <client 강제 종료>\n");
			#endif
			if(pSendData)
				delete[] pSendData;
			if(pHeader)
				delete pHeader;
			return;
		}
	}
	else
	{
infLOG(ALWAY, "-> Check Cmd != RS_EOL\n");	

		infLOG(ERROR, "WaitForRequst : 에러 5: <처리 없음>\n"); 
		#ifdef __DEBUG
		printf("WaitForRequst : 에러 5: <처리 없음>\n");
		#endif
		
		//printf("RS_EOL not receive \n");
	}
		
	
infLOG(ALWAY, "Ouput ) WaitForRequest\n");
	
	if(pHeader)
		delete pHeader;	
	if(pSendData)
		delete[] pSendData;
}


/*******************************************************************************
** 서비스 제어함수
*******************************************************************************/
long processed(char* pHead, char* pRecvData, char* &pSendData)
{

infLOG(ALWAY, "Input ) processed\n");	

	char  ErrMsg[256];
	int   nErrcode  = 0;
	long  dwSendLen = 0;

	LPHEADER pRHeader = (LPHEADER)pHead;
	switch(pRHeader->nCmd)
	{
		/*
		case RS_SET_CONNECT_COUNT:
		{
			infLOG(ALWAY, "-->RequestSetListenCount\n");			
			nErrcode = RequestSetListenCount(pHead, pRecvData, pSendData);
			infLOG(ALWAY, "-->end RequestSetListenCount\n");
			
			break;
		}*/
		
		
	
		case RS_CMD_REQUEST_USER_LIST: //10051
		{
			infLOG(ALWAY, "Input ) RequestUserList (%s)\n",pRHeader->szUserID);
			nErrcode = RequestUserList(pHead, pRecvData, pSendData);
			infLOG(ALWAY, "Ouput ) RequestUserList (%s)\n",pRHeader->szUserID);
			break;
		}
		case 1001:	// 서버할당조회
		{
			
			infLOG(ALWAY, "Input ) cmds1001 (%s)\n",pRHeader->szUserID);
			nErrcode = cmds1001(pHead, pRecvData, pSendData);
			infLOG(ALWAY, "Ouput ) cmds1001 (%s)\n",pRHeader->szUserID);
			break;
		}
		case 1101: // 내자료실 서버 할당 및 사용자 용량 검사
		{
			infLOG(ALWAY, "Input ) cmds1101 (%s)\n",pRHeader->szUserID);
			nErrcode = cmds1101(pHead, pRecvData, pSendData);
			infLOG(ALWAY, "Ouput ) cmds1101 (%s)\n",pRHeader->szUserID);
			break;	
		}









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
		case 10041: // RS_CMD_REQUEST_ADMIN_LOGIN
		{
infLOG(ALWAY, "Input ) cmds10041\n");
			nErrcode = cmds10041(pHead, pRecvData, pSendData);
infLOG(ALWAY, "Ouput ) cmds10041\n");
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
		case 1005:
		{
infLOG(ALWAY, "Input ) cmds1005\n");
			nErrcode = cmds1005(pHead, pRecvData, pSendData);
infLOG(ALWAY, "Ouput ) cmds1005\n");
			
			break;
			
		}
		
		
		
		
		
		
		default:
		{
infLOG(ALWAY, "->Default (not have Service) \n");
			memset (ErrMsg, 0x00, sizeof(ErrMsg));
			sprintf(ErrMsg, "Service: (%d)는 없는 서비스 입니다.", pRHeader->nCmd);
			E_dump(-900001, ErrMsg, pSendData);
			nErrcode = -900001;
			break;
		}
	}

	if (nErrcode < 0)
	{
infLOG(ALWAY, "-> Exception ) nErrcode (%d)\n",nErrcode);		
		
		dwSendLen = ERR_HEADER_SIZE;
	}
	else
	{
infLOG(ALWAY, "-> Send Data \n");
		LPHEADER pSHeader = (LPHEADER)pSendData;
		pSHeader->nErrorCode = 0;
		dwSendLen = HEADER_SIZE + (pSHeader->nDataCnt * pSHeader->nDataSize);
	}
	#ifdef __DEBUG
	printf("--> end process\n");
	#endif
infLOG(ALWAY, "Ouput ) processed (%d)\n",dwSendLen);
	return dwSendLen;
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
			if(errno == EINTR)
			{
				continue;
			}
			return(-1*errno);
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
			if(errno == EINTR)
				continue;
			return(-1*errno);
		}
		if(iRet ==0)
			return 0;
		nTempLen += iRet;
	}

	return(nTempLen);
}
int RequestSetListenCount(char *pRecvHead, char *pRecvData, char* &pSendData) //cmd 사용자 목록 요청 10051
{
	#ifdef __DEBUG
	printf("--> RequestSetListenCount \n");
	#endif
	
	int nListen = atoi(pRecvData);
	
	#ifdef __DEBUG
	printf("--> listen count : ( %d )\n",nListen);
	#endif
	m_nListenNum = nListen;
	
	HEADER header;
	memset(&header,0x00,sizeof(header));
	
	header.nCmd = RS_SET_CONNECT_COUNT;
	header.nDataCnt = 0;
	header.nDataSize = 0;
	

	pSendData = new char[HEADER_SIZE + header.nDataCnt*header.nDataSize];
	memset(pSendData, 0x00, HEADER_SIZE + header.nDataCnt*header.nDataSize);
	
	memcpy( pSendData , &header,HEADER_SIZE);
	
	#ifdef __DEBUG
	printf("--> end RequesetUserList \n");
	#endif
	
	return 1;	
}
int RequestUserList(char *pRecvHead, char *pRecvData, char* &pSendData) //cmd 사용자 목록 요청 10051
{
	#ifdef __DEBUG
	printf("--> RequesetUserList \n");
	#endif
	

	int nCount=0;	
	
	LPUSERLIST pData = new USERLIST[1000];
	memset(pData,0x00,sizeof(USERLIST)*1000);

	multimap<int,ph>::iterator mi;
	//mi = s_info.phinfo.begin(); change
	mi = s_info.phinfo.find(1);
	
	while(mi != s_info.phinfo.end())
	{
		
		strcpy(pData[nCount].szUserID,mi->second.szID);
		strcpy(pData[nCount].szUserIP,mi->second.szIP);
		pData[nCount].nNumber = nCount ;
		
		
		#ifdef __DEBUG
		printf("%4d> [쓰레드 번호: : %d] [연결 상황  : %d]  : [주소 : %s] : [아이디 : %s] \n"
				,nCount ,  mi->second.index_num ,mi->first   , pData[nCount].szUserIP , pData[nCount].szUserID);
		#endif		
		mi++;
		
		nCount++;
		if(nCount >= 999)
			break;
	}
	
	/*			   
	while(mi != s_info.phinfo.end())
	{
		if(mi->first == 1)
		{
			

			strcpy(pData[nCount].szUserID,mi->second.szID);
			strcpy(pData[nCount].szUserIP,mi->second.szIP);
			pData[nCount].nNumber = nCount ;
			
			
			#ifdef __DEBUG
			printf("%4d> [쓰레드 번호: : %d] [연결 상황  : %d]  : [주소 : %s] : [아이디 : %s] \n"
					,nCount ,  mi->second.index_num ,mi->first   , pData[nCount].szUserIP , pData[nCount].szUserID);
			#endif
			
			nCount++;
			
		}
		
		mi++;
		if(nCount >=1000)
			break;
	}
	*/
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
	printf("--> end RequesetUserList \n");
	#endif
	
	delete[] pData;
	return 1;
	
				
	
}

//////////////////////////////////////////////////////////////////////////////////////
// request hdd info
// int RequestDriveInfo(int& Socket,int nCmd,char* strPath)
// return value     ,socket index, command , file or folder path
// return valu have 1 is success , -1 is failed 
 //////////////////////////////////////////////////////////////////////////////////////
/*int CmdRequestEol(int& Socket,LPPACKET pPacket) //int nCmd,char* pBuffer)
{
	
	#ifdef __DEBUG
	printf("CmdRequestEol 시작\n");
	#endif
		
	LPPACKET Packet = new struct _PACKET;
	memset(Packet,0x00,sizeof(struct _PACKET));
	Packet->nCmd = RS_EOL;
	if(SendData(Socket,(char*)Packet, sizeof(struct _PACKET)) < 0)
	{
		infLOG(ERROR, "CmdRequestEol : EOL 전송 에러 : <client 강제 종료>\n"); 
		#ifdef __DEBUG
		printf("CmdRequestEol : EOL 전송 에러 : <client 강제 종료>\n");
		#endif
		delete Packet;		
		return -1;
	
	}

	delete Packet;

	return -1;
}*/
