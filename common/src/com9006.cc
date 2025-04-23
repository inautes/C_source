/******************************************************************************
 *   서브시스템 : FDN서버
 *   프로그램명 : com9006.cc
 *         기능 : 구매정보에서 정액제로 구매했는지를 조사한다.
 *         설명 : 다운로드 전에 정액제에 관련 사항을 검색한다.
 *       작성자 : JDP
 *       작성일 : 2004/12/29 수요일
 *     수정이력 :
*******************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#include <mysql.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// db 제어 서버
#include <unistd.h>     /* for close() getpid()*/
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include "comsock.h" //sock send recv
#include "comhead.h"
// db 제어 서버


#include "./fdnsvr/inc/fdndefine.h"
#include "apdefine.h"
#include "comcomm.h"
#include "commydb.h"
//#define  _DEBUG_

long com9006(char* pData ,char* pResult,char* pErrMsg,char* szIP, int nPort)
{




	LPHOLDTIME pHoldTime = (LPHOLDTIME)pResult;
	memset(pHoldTime,0x00,sizeof(HOLDTIME));

	LPFILEINFO pFileInfo = (LPFILEINFO)pData;

	#ifdef _DEBUG_
	printf(" ] com9006 start\n");
	#endif

	//--------------------------------------------------------------------------
	// DB 중개 서버 연결 및 데이터 처리
	//--------------------------------------------------------------------------
	struct sockaddr_in dcmdSerAddr;
	int dcmd_socket;

	if ( ( dcmd_socket = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0 )
	{
		infLOG(ERROR, "com9006[ERR]: 소켓 생성 오류 입니다.\n");

		if(pErrMsg != NULL)
			strcpy(pErrMsg,"9006 소켓 오류 : 소켓 생성 오류 입니다. \n");
       	return(-1);

	}

	memset(&dcmdSerAddr,0x00,sizeof(dcmdSerAddr));
	dcmdSerAddr.sin_family      = AF_INET;
	dcmdSerAddr.sin_addr.s_addr = inet_addr(szIP);
	dcmdSerAddr.sin_port        = htons(nPort);

	if( connect(dcmd_socket,(struct sockaddr * )&dcmdSerAddr,sizeof(dcmdSerAddr)) < 0 )
	{
		 infLOG(ERROR, " ]com9006[ERR]중개서버 접속 오류 : [ %s ] [ %d ]  \n " ,szIP,nPort);

		// dcmdSerAddr.sin_addr.s_addr = inet_addr(DB_SUB_COMMAND_SERVER_IP);
		return -1;
	}

	//--------------------------------------------------------------------------
	// DB 중개 서버 해더 및 데이터 전송  ( deal_no 전송 ) - 2009/01/22 2.0 변경건으로 구조체 전부 넘겨줌(HCS).
	//--------------------------------------------------------------------------
	char* pSendData = NULL; //데이터 전송 버퍼
	long dwSendDataLen = 0;

	HEADER dcmdHeader;
	memset(&dcmdHeader,0x00,HEADER_SIZE);

	dcmdHeader.nDataCnt = 1;
	dcmdHeader.nDataSize = sizeof(FILEINFO);
	dcmdHeader.nCmd = 9006;

	dwSendDataLen = HEADER_SIZE + dcmdHeader.nDataCnt * dcmdHeader.nDataSize;
	pSendData = new char[dwSendDataLen];

	memcpy( pSendData , &dcmdHeader,HEADER_SIZE);
	if( dcmdHeader.nDataCnt * dcmdHeader.nDataSize > 0 )
		memcpy( pSendData + HEADER_SIZE, pFileInfo , dcmdHeader.nDataCnt * dcmdHeader.nDataSize);

	if(ComSendData(dcmd_socket,pSendData, dwSendDataLen ) <= 0)
	{
		#ifdef __DEBUG
		printf(" ] com9006 요청 패킷 전송 deal_type , deal_no(id) (%d)( %ld )\n",pFileInfo->nDealType, pFileInfo->dwDealID);
		#endif
		infLOG(ERROR, "com9006 요청 패킷 전송 deal_type, deal_no(id) (%d) ( %ld )\n",pFileInfo->nDealType, pFileInfo->dwDealID);


		if(pSendData)
			delete[] pSendData;

		pSendData = NULL;

		infLOG(ERROR, "com9006[ERR]: 9006 통신 오류(S) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");

		if(pErrMsg != NULL)
			strcpy(pErrMsg,"9006 통신 오류(S) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");
       	return(-1);
	}

	if(pSendData)
		delete[] pSendData;
	//--------------------------------------------------------------------------
	// DB 중개 서버로 부터 데이터 받기  (  )
	//--------------------------------------------------------------------------
	if(ComRecvData(dcmd_socket,(char*)&dcmdHeader,HEADER_SIZE ) <= 0)
	{

		infLOG(ERROR, "com9006[ERR]: 9006 통신 오류(R) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");

		if(pErrMsg != NULL)
			strcpy(pErrMsg,"9006 통신 오류(R) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오.  \n");
       	return(-1);

	}


	if( dcmdHeader.nCmd == 9006 )
	{
		if(dcmdHeader.nDataCnt * dcmdHeader.nDataSize > 0 )
		{
			if( ComRecvData(dcmd_socket,(char*)pHoldTime,dcmdHeader.nDataCnt*dcmdHeader.nDataSize) <= 0 )
			{
				infLOG(ERROR, "com9006[ERR]: 9006 통신 오류(R) 1: 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");

				if(pErrMsg != NULL)
					strcpy(pErrMsg,"9006 통신 오류(R) 2: 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오.  \n");
		       	return(-1);
			}
		}
	}
	else
	{
		infLOG(ERROR, "com9006[ERR]: 9006 통신 오류(R) 3: 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");

		if(pErrMsg != NULL)
			strcpy(pErrMsg,"9006 통신 오류(R) 4: 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오.  \n");
       	return(-1);
	}
	close(dcmd_socket);

	return 1;
}
