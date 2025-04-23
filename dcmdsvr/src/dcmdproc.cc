/******************************************************************************
 *   서브시스템 : Dcmd Server
 *   프로그램명 :
 *         기능 :
 *         설명 :
 *     수정이력 :
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/

#include "dcmdproc.h"


void *ThreadMain(void *threadArgs)
{
    int clntSock;

    pthread_detach(pthread_self());

	char szUserIP[16];
	memset(szUserIP,0x00,sizeof(szUserIP));


	LPUSERINFO pData = (LPUSERINFO) threadArgs;
    clntSock = pData->thread.clntSock;
	sprintf(szUserIP, "%s", pData->thread.userIP);
	delete pData;



	struct timeval tv;
	tv.tv_sec = 60*1; //1분

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

		infLOG(ERROR, "setsockopt error %s",szErrorMsg);
	}

	struct timeval tv2;
	tv2.tv_sec = 60*1; //1분

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

		infLOG(ERROR, "setsockopt error %s",szErrorMsg);
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
		// infLOG(ERROR, "----------| Exceptin Recv Header( socket = %d ) ( %d ) ( %s )|----------\n",Socket,errno,strerror(errno));
	}
	else
	{
		if(pHeader->nCmd <= 0)
		{
			infLOG(ERROR, "----------| Exceptin Command(user_id=[%s], nCmd=[%d]) |----------\n",pHeader->szUserID,pHeader->nCmd);
		}
		else
		{
			dwRecvLen = (pHeader->nDataCnt) * (pHeader->nDataSize);

			if(dwRecvLen != 0)
			{
				pRecvData = new char[dwRecvLen];
				memset(pRecvData,0x00,dwRecvLen);

				// client에서 data부을 수신

				if(RecvData(Socket, pRecvData, dwRecvLen )<= 0)	{
					infLOG(ERROR, "----------| Exceptin Body Data(user_id=[%s], nCmd=[%d])( %d ) ( %s ) |----------\n",pHeader->szUserID,pHeader->nCmd,errno,strerror(errno));
					dwSendLen = -1;
				}
				else{
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
					infLOG(ERROR, "----------| Request Send Packet Error(user_id=[%s], nCmd=[%d]) ( %d ) ( %s ) |----------\n",pHeader->szUserID,pHeader->nCmd,errno,strerror(errno));
			}
		}
	}


	if(pRecvData != NULL)
	{
//		infLOG(ALWAY, "delete pRecvData %p\n",pRecvData);
		delete[] pRecvData;
		pRecvData = NULL;
	}

	if(pSendData != NULL)
	{
//		infLOG(ALWAY, "delete pSendData %p\n",pSendData);
		delete[] pSendData;
	}

	if(pHeader != NULL)
	{
//		infLOG(ALWAY, "delete pHeader %p\n",pHeader);
		delete pHeader;
	}

	infLOG(ALWAY, "--| End Job |--\n");

	return -1;
}





long processed(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{

	char  ErrMsg[256];
	int   nErrcode  = 0;
	long  dwSendLen = 0;

	LPHEADER pRHeader = (LPHEADER)pRecvHead;

	switch(pRHeader->nCmd)
	{
		case 900210:	// 사용안함
		case 90021:	//
		{
			nErrcode = dcmd9002(Socket,pRHeader, pRecvData, pSendData,1, MODE_NORMAL); // 다운로드
			break;
		}
		case 900200:	// 사용안함
		case 90020:	//
		{
			nErrcode = dcmd9002(Socket,pRHeader, pRecvData, pSendData,0, MODE_NORMAL); // 다운로드
			break;
		}
		case 91020:	//
		{
			nErrcode = dcmd9002(Socket,pRHeader, pRecvData, pSendData,1, MODE_DSP); // 다운로드
			break;
		}
		case 3004:	//
		{
			nErrcode = dcmdfdns3004(Socket,pRHeader, pRecvData, pSendData);
			break;
		}
		case 4001:	//
		{
			nErrcode = dcmdfups4001(Socket,pRHeader, pRecvData, pSendData);
			break;
		}
		case 4002:	//
		{
			nErrcode = dcmdfups4002(Socket,pRHeader, pRecvData, pSendData);
			break;
		}
		case 4003:	//
		{
			nErrcode = dcmdfups4003(Socket,pRHeader, pRecvData, pSendData);
			break;
		}
		case 4005:	//
		{
			nErrcode = dcmdfups4005(Socket,pRHeader, pRecvData, pSendData);
			break;
		}
		case 4006:	//
		{
			nErrcode = dcmdfups4006(Socket,pRHeader, pRecvData, pSendData);
			break;
		}
		//20190123 1mb hash
		case 40052:	//
		{
			nErrcode = dcmdfups4005(Socket,pRHeader, pRecvData, pSendData);
			break;
		}
		case 40062:	//
		{
			nErrcode = dcmdfups4006(Socket,pRHeader, pRecvData, pSendData);
			break;
		}
		//1mb hash end
		case 40051:	// 필로그 저작권 검사
		{
			nErrcode = dcmdfups40051(Socket,pRHeader, pRecvData, pSendData);
			break;
		}
		case 40061:	// 필로그 제휴 검사
		{
			nErrcode = dcmdfups40061(Socket,pRHeader, pRecvData, pSendData);
			break;
		}
		case 9001:	// 업로드 시작
		{
			nErrcode = dcmd9001(Socket,pRHeader, pRecvData, pSendData);
			break;
		}
		case 9004:	//
		{
			nErrcode = dcmd9004(Socket,pRHeader, pRecvData, pSendData);
			break;
		}
		case 9005:	//
		{
			nErrcode = dcmd9005(Socket,pRHeader, pRecvData, pSendData);
			break;
		}
		case 9006:	//
		{
			nErrcode = dcmd9006(Socket,pRHeader, pRecvData, pSendData);
			break;
		}
		case 9009:	//
		{
		//	nErrcode = dcmd9009(Socket,pRHeader, pRecvData, pSendData);
			break;
		}
		case 9101:	//
		{
			nErrcode = dcmd9101(Socket,pRHeader, pRecvData, pSendData);
			break;
		}
		case 9102:	//
		{
			nErrcode = dcmd9102(Socket,pRHeader, pRecvData, pSendData, MODE_NORMAL);
			break;
		}
		case 9103:	//
		{
			nErrcode = dcmd9103(Socket,pRHeader, pRecvData, pSendData);
			break;
		}
		case 9104:	//
		{
			nErrcode = dcmd9104(Socket,pRHeader, pRecvData, pSendData);
			break;
		}
		case 9105:	//
		{
			nErrcode = dcmd9105(Socket,pRHeader, pRecvData, pSendData);
			break;
		}
		case 9106:	// 통합 가라 업로드
		{
			//nErrcode = dcmd9106(Socket,pRHeader, pRecvData, pSendData);
			break;
		}
		case 9201:	// 다운로드 중복 접속 끊기
		{
			//nErrcode = dcmd9201(Socket,pRHeader, pRecvData, pSendData);
			break;
		}
		case 516 : // RS_FILE_REQUEST_LIST:	// 다운로드 목록 조회 , 통합 서버
		case 518 : // RS_FILE_REQUEST_LIST:	// 다운로드 목록 조회 , 통합 서버
		{
		//	nErrcode = dcmd5160(Socket,pRHeader, pRecvData, pSendData);
			break;
		}
		default:
		{
			//			   012345678901234]
			//infLOG(ERROR, "----------|  (not have Service) \n");
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
