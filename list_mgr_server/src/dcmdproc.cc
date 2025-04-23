/******************************************************************************
 *   프로그램명 : 다운로드 리스트 생성 (통합 db 참조)
 *       작성자 : 김일오
 *       작성일 : 
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/

#include "dcmdproc.h"

void *ThreadMain(void *threadArgs)
{
    int clntSock;                   /* Socket descriptor for client connection */

	pthread_detach(pthread_self()); 

	LPUSERINFO pData = (LPUSERINFO) threadArgs;
    clntSock = pData->thread.clntSock;
    char szClientIp[16+1];
    memset(szClientIp, 0x00, sizeof(szClientIp));
    sprintf(szClientIp, "%s", pData->thread.userIP);
    delete pData;
		
	struct timeval tv;
	tv.tv_sec = 30; //
	
	int st = setsockopt(clntSock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));
	if(st != 0)
	{	
		char szErrorMsg[255];
		memset(szErrorMsg,0x00,sizeof(szErrorMsg));
		
		int nErrno = errno;
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
	tv2.tv_sec = 30; 
				
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
	
    HandleTCPClient(clntSock);
    
	close(clntSock);    // Close client socket 

    return (NULL);
}



void HandleTCPClient(int clntSocket)
{
 
	WaitForRequest(clntSocket);	
}


int WaitForRequest(int& Socket)
{
	
	#ifdef __DEBUG 
	printf(" ] new pHeader\n");
	#endif
	LPHEADER pHeader = new HEADER;
	memset(pHeader,0x00,HEADER_SIZE);
	bool nStop = false;

	// client에서 header을 수신
	char* pSendData ;
	pSendData = NULL;
	char* pRecvData;
	pRecvData = NULL;

	long dwSendLen = 0;
	long dwRecvLen = 0;
		

	memset(pHeader,0x00,HEADER_SIZE);
	
	if(RecvData(Socket,(char*)pHeader,HEADER_SIZE ) <= 0)
	{
		infLOG(ERROR, "Wait Client Request  ] (%s) Exceptin Recv\n",pHeader->szUserID);			
	}
	else
	{

		if(pHeader->nCmd <= 0)
		{
			infLOG(ERROR, "Wait Client Request  ] Exceptin Command (%d) \n",pHeader->nCmd);
		}
		else
		{
			dwRecvLen = (pHeader->nDataCnt) * (pHeader->nDataSize);
		
			if(dwRecvLen != 0)
			{		
				
				#ifdef __DEBUG 
				printf(" ] new pRecvData\n");
				#endif
				pRecvData = new char[dwRecvLen];
				memset(pRecvData,0x00,dwRecvLen);
		
				// client에서 data부을 수신
				
				if(RecvData(Socket, pRecvData, dwRecvLen )<= 0)
				{
					infLOG(ERROR, "Wait Client Request  ] (%s) Exceptin Body Data\n",pHeader->szUserID);
					dwSendLen = -1;
				}
				else
				{
					dwSendLen = processed(Socket,(char*)pHeader,pRecvData, pSendData);	//서비스호출
				}
				
			}
			else
			{
				dwSendLen = processed(Socket,(char *)pHeader, NULL, pSendData);	//서비스호출
			}
		
			
			
			if(dwSendLen > 0 )
			{
	
				if(SendData(Socket,pSendData, dwSendLen ) <= 0)
				{
					#ifdef __DEBUG
					printf("Wait Client Request  ] (%s) Request Send Packet Error \n",pHeader->szUserID);
					#endif					
					infLOG(ERROR, "Wait Client Request  ] (%d)(%s) Request Send Packet Error \n", pHeader->nCmd, pHeader->szUserID);	
				}
				#ifdef __DEBUG
				printf("Wait Client Request  ] (%s) Request Send Packet \n",pHeader->szUserID);
				#endif					
				infLOG(ALWAY, "Wait Client Request  ] (%d)(%s) Request Send Packet \n", pHeader->nCmd, pHeader->szUserID);
			}
		}
	}
		
		
	if(pRecvData != NULL)
	{
			
		#ifdef __DEBUG 
		printf(" ] delete pRecvData\n");
		#endif

		delete[] pRecvData;
		pRecvData = NULL;
	}
	if(pSendData != NULL)
	{
		#ifdef __DEBUG 
		printf(" ] delete pSendData\n");
		#endif
		delete[] pSendData;
	}
	if(pHeader != NULL)
	{
		#ifdef __DEBUG 
		printf(" ] delete pHeader\n");
		#endif		
		delete pHeader;	
	}
	
		
	#ifdef __DEBUG
	printf("Wait Client Request  ] ----------| End |----------\n");
	#endif					
	infLOG(ALWAY, "----------| End Job |----------\n");
		
	
	return -1;
}





long processed(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{	
	char  ErrMsg[256];
	int   nErrcode  = 0;
	long  dwSendLen = 0;

	LPHEADER pRHeader = (LPHEADER)pRecvHead;

	infLOG(ALWAY, "----------| pRHeader->nCmd = %d |----------\n",pRHeader->nCmd);

	switch(pRHeader->nCmd)
	{
		case 516 : // RS_FILE_REQUEST_LIST:	// 다운로드 목록 조회 , 통합 서버
		case 518 : // RS_FILE_REQUEST_LIST:	// 다운로드 목록 조회 , 통합 서버
		{			
			infLOG(ALWAY, "----------| 516,518 dcmd5160 |----------\n");
			nErrcode = dcmd5160(Socket,pRHeader, pRecvData, pSendData);
			infLOG(ALWAY, "----------| 516,518 dcmd5160 end = %d |----------\n",nErrcode);
			break;
		}				
		case 30041 : // RS_GURU_FILE_DN_LIST: // 자료실?
		{
			infLOG(ALWAY, "----------| 30041 |----------\n");
			//nErrcode = dcmd30041(Socket,pRecvHead,pRecvData,pSendData);
			break;
		}
        default:
		{
			//			   012345678901234]			
			infLOG(ERROR, "Default 		  ] (not have Service) \n");
			memset (ErrMsg, 0x00, sizeof(ErrMsg));
			sprintf(ErrMsg, "Service: (%d)는 없는 서비스 입니다.", pRHeader->nCmd);
			E_dump(-900001, ErrMsg, pSendData);
			nErrcode = -900001;
			break;
		}
	}
	return nErrcode;

	
	
}




void GetErrMsg(int nErrno,char* szErrMsg)	
{
	int nTempErrNo = nErrno;
	strcpy(szErrMsg,strerror(nTempErrNo));	
	
			
}
