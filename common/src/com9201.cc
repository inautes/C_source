/******************************************************************************
 *   서브시스템 : FUP서버
 *   프로그램명 : com9201.cc
 *         기능 : upload 시작
 *         설명 : 서버에 업로드하기 전에 호출되는 함수
 *       작성자 : JDP
 *       작성일 : 2004/10/02 토요일
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
#include "com9201.h"
//#define  _DEBUG_
// db 제어 서버
#include <unistd.h>     /* for close() getpid()*/
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include "comsock.h" //sock send recv
#include "comhead.h"

// db 제어 서버
//******************************************************************************
//  COM9201 main
//
//  input : pcom9201_r.proc_flag => 1=wedisk, 2=mydata
//
//  return:  1(정상)
//          -1(DB오류)
//******************************************************************************
long com9201(CCOM9201_R pcom9201_r,char* szIP, int nPort)
{
	//--------------------------------------------------------------------------
	// DB 중개 서버 연결 및 데이터 처리
	//--------------------------------------------------------------------------
	struct sockaddr_in dcmdSerAddr;
	int dcmd_socket;

	if ( ( dcmd_socket = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0 )
	{
		infLOG(ERROR, "com9201[ERR]: 소켓 생성 오류 입니다.\n");

       	return(-1);

	}


/*
	struct timeval tv;
	tv.tv_sec = 5; //5초

	int st = setsockopt(dcmd_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));
	if(st != 0)
		infLOG(ALWAY, " ]com9201[ERR] 소켓 recv time out 옵션 설정 실패 errno = ( %d )\n",errno);

	struct timeval tv2;
	tv2.tv_sec = 5; //5초

	st = setsockopt(dcmd_socket, SOL_SOCKET, SO_SNDTIMEO, &tv2, sizeof(struct timeval));
	if(st != 0)
		infLOG(ALWAY, " ]com9201[ERR] 소켓 send time out 옵션 설정 실패 errno = ( %d )\n",errno);

*/
		memset(&dcmdSerAddr,0x00,sizeof(dcmdSerAddr));
		dcmdSerAddr.sin_family      = AF_INET;
		dcmdSerAddr.sin_addr.s_addr = inet_addr(szIP);
		dcmdSerAddr.sin_port        = htons(nPort);

		if( connect(dcmd_socket,(struct sockaddr * )&dcmdSerAddr,sizeof(dcmdSerAddr)) < 0 )
		{
			 infLOG(ERROR, " ]com9201[ERR]중개서버 접속 오류 : [ %s ] [ %d ]  \n " ,szIP,nPort);

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
		dcmdHeader.nDataSize = sizeof(CCOM9201_R); //unsigend long
		dcmdHeader.nCmd = 9201;

		dwSendDataLen = HEADER_SIZE + dcmdHeader.nDataCnt * dcmdHeader.nDataSize;
		pSendData = new char[dwSendDataLen];

		memcpy( pSendData , &dcmdHeader,HEADER_SIZE);
		if( dcmdHeader.nDataCnt * dcmdHeader.nDataSize > 0 )
			memcpy( pSendData + HEADER_SIZE, &pcom9201_r , dcmdHeader.nDataCnt * dcmdHeader.nDataSize);

		if(ComSendData(dcmd_socket,pSendData, dwSendDataLen ) <= 0)
		{
			#ifdef __DEBUG
			printf(" ] com9201 요청 패킷 전송 \n");
			#endif
			infLOG(ERROR, "com9201 요청 패킷 전송 실패 \n");


			if(pSendData != NULL)
				delete[] pSendData;

			pSendData = NULL;

			infLOG(ERROR, "com9201[ERR]: 9201 통신 오류(S) : 서버와의 통신 오류 입니다. \n");


	       	return(-1);
		}


		if(pSendData != NULL )
			delete[] pSendData;
		//--------------------------------------------------------------------------
		// DB 중개 서버로 부터 데이터 받기  (  )
		//--------------------------------------------------------------------------
		if(ComRecvData(dcmd_socket,(char*)&dcmdHeader,HEADER_SIZE ) <= 0)
		{

			infLOG(ERROR, "com9201[ERR]: 9201 통신 오류(R) : 서버와의 통신 오류 입니다. 1 \n");

	       	return(-1);

		}


		if( dcmdHeader.nCmd == 9201 )
		{

		}
		else
		{
			infLOG(ERROR, "com9201[ERR]: 9201 통신 오류(R) : 서버와의 통신 오류 입니다. 2 \n");


			close(dcmd_socket);

	       	return(-1);
		}
		close(dcmd_socket);


	return 1;
}
