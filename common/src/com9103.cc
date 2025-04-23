/******************************************************************************
 *   서브시스템 : FUP서버
 *   프로그램명 : com9103.cc
 *         기능 : 내자료실 사용량UPDATE
 *         설명 : 내자료실에 파일upload, 복사, 삭제시 디스크사용량을 UPDATE한다
 *       작성자 : JDP
 *       작성일 : 2004/07/28 엄청 더운날
 *     수정이력 :
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#include <mysql.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apdefine.h"
#include "comcomm.h"
#include "commydb.h"
#include "com9103.h"
//#define  _DEBUG_
// db 제어 서버
#include <unistd.h>     /* for close() getpid()*/
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include "comsock.h" //sock send recv
#include "comhead.h"
// db 제어 서버
//******************************************************************************
//* COM9103 main
//* error 발생시 pSendData를 사용하고 정상인경우 seq_no를 return한다.
//  return:  1(정상)
//          -1(DB오류)
//          -2(minus처리시 사이즈가 너무작음)
//          -3(남아있는 용량보다 사이즈가 큼)
//******************************************************************************
long com9103(CCOM9103_R pcom9103_r,char* pErrMsg,char* szIP, int nPort)
{

	//--------------------------------------------------------------------------
	// DB 중개 서버 연결 및 데이터 처리
	//--------------------------------------------------------------------------
	struct sockaddr_in dcmdSerAddr;
	int dcmd_socket;

	if ( ( dcmd_socket = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0 )
	{
		infLOG(ERROR, "com9103[ERR]: 소켓 생성 오류 입니다.\n");

		if(pErrMsg != NULL )
			strcpy(pErrMsg,"9103 소켓 오류 : 소켓 생성 오류 입니다. \n");
       	return(-1);

	}

	memset(&dcmdSerAddr,0x00,sizeof(dcmdSerAddr));
	dcmdSerAddr.sin_family      = AF_INET;
	dcmdSerAddr.sin_addr.s_addr = inet_addr(szIP);
	dcmdSerAddr.sin_port        = htons(nPort);

	if( connect(dcmd_socket,(struct sockaddr * )&dcmdSerAddr,sizeof(dcmdSerAddr)) < 0 )
	{
		 infLOG(ERROR, " ]com9103[ERR]중개서버 접속 오류 : [ %s ] [ %d ]  \n " ,szIP,nPort);

		// dcmdSerAddr.sin_addr.s_addr = inet_addr(DB_SUB_COMMAND_SERVER_IP);
		return -1;
	}


	//--------------------------------------------------------------------------
	// DB 중개 서버 해더 및 데이터 전송  ( deal_no 전송 )
	//--------------------------------------------------------------------------
	char* pSendData = NULL; //데이터 전송 버퍼
	long dwSendDataLen = 0;

	HEADER dcmdHeader;
	memset(&dcmdHeader,0x00,HEADER_SIZE);

	dcmdHeader.nDataCnt = 1;
	dcmdHeader.nDataSize = sizeof(CCOM9103_R); //unsigend long
	dcmdHeader.nCmd = 9103;

	dwSendDataLen = HEADER_SIZE + dcmdHeader.nDataCnt * dcmdHeader.nDataSize;
	pSendData = new char[dwSendDataLen];

	memcpy( pSendData , &dcmdHeader,HEADER_SIZE);
	if( dcmdHeader.nDataCnt * dcmdHeader.nDataSize > 0 )
		memcpy( pSendData + HEADER_SIZE, &pcom9103_r , dcmdHeader.nDataCnt * dcmdHeader.nDataSize);

	if(ComSendData(dcmd_socket,pSendData, dwSendDataLen ) <= 0)
	{
		#ifdef __DEBUG
		printf(" ] com9103 요청 패킷 전송 \n");
		#endif
		infLOG(ERROR, "com9103 요청 패킷 전송 \n");


		if(pSendData != NULL )
			delete[] pSendData;

		pSendData = NULL;

		infLOG(ERROR, "com9103[ERR]: 9103 통신 오류(S) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");

		if(pErrMsg != NULL )
			strcpy(pErrMsg,"9103 통신 오류(S) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");
       	return(-1);
	}


	if(pSendData != NULL )
		delete[] pSendData;
	//--------------------------------------------------------------------------
	// DB 중개 서버로 부터 데이터 받기  (  )
	//--------------------------------------------------------------------------
	if(ComRecvData(dcmd_socket,(char*)&dcmdHeader,HEADER_SIZE ) <= 0)
	{

		infLOG(ERROR, "com9103[ERR]: 9103 통신 오류(R) 1: 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");

		if(pErrMsg != NULL )
			strcpy(pErrMsg,"9103 통신 오류(R) 1: 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오.  \n");
       	return(-1);

	}




	if( dcmdHeader.nCmd == 9103 )
	{

	}
	else
	{
		char szErrMsg[1024];
		memset(szErrMsg,0x00,sizeof(szErrMsg));

		if( ComRecvData(dcmd_socket,szErrMsg,ERR_HEADER_SIZE - HEADER_SIZE) <= 0 )
		{
			infLOG(ERROR, "com9103[ERR]: 9103 통신 오류(R) 2: 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");


			if(pErrMsg != NULL)
				strcpy(pErrMsg,"9103 통신 오류(R) 2: 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오.  \n");
	       	return(-1);
		}
		if( pErrMsg != NULL )
		{
			strcpy( pErrMsg , szErrMsg);
		}

		infLOG(ERROR, "com9103[ERR]: 9103 통신 오류(R) Cmd is Bad ( %ld )  \n", dcmdHeader.nCmd);

		close(dcmd_socket);

		return dcmdHeader.nCmd ;

	}
	close(dcmd_socket);

	return 1;

}

