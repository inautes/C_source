/******************************************************************************
 *   서브시스템 : FUP서버
 *   프로그램명 : com9203.cc
 *         기능 : fdnserver 시작시 초기화
 *         설명 : fdnserver 실행(재실행)시에 T_SERVER_INFO의 dn_user, T_CONTENTS_FILE_USER_CNT의 cur_user_cnt를 초기화
 *       작성자 : HCS
 *       작성일 : 2007/09/28 금요일
 *     수정이력 :
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#include <mysql.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apdefine.h"
#include "comcomm.h"
#include "commydb.h"
#include "com9203.h"
//#define  _DEBUG_
// db 제어 서버
#include <unistd.h>     /* for close() getpid()*/
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <errno.h>
#include "comsock.h" //sock send recv
#include "comhead.h"

// db 제어 서버
//******************************************************************************
//  COM9203 main
//
//  input : pcom9203_r.proc_flag => 1=wedisk, 2=mydata
//
//  return:  1(정상)
//          -1(DB오류)
//******************************************************************************
long com9203(CCOM9203_R pcom9203_r,char* szIP, int nPort)
{
	infLOG(ALWAY, "com9203\n");
	/*
	//--------------------------------------------------------------------------
	// DB 중개 서버 연결 및 데이터 처리
	//--------------------------------------------------------------------------
	struct sockaddr_in dcmdSerAddr;
	int dcmd_socket;

	if ( ( dcmd_socket = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0 )
	{
		infLOG(ERROR, "com9101[ERR]: 소켓 생성 오류 입니다.\n");


       	return(-1);

	}

	memset(&dcmdSerAddr,0x00,sizeof(dcmdSerAddr));
	dcmdSerAddr.sin_family      = AF_INET;
	dcmdSerAddr.sin_addr.s_addr = inet_addr(szIP);
	dcmdSerAddr.sin_port        = htons(nPort);

	if( connect(dcmd_socket,(struct sockaddr * )&dcmdSerAddr,sizeof(dcmdSerAddr)) < 0 )
	{
		 infLOG(ERROR, " ]com9203[ERR]중개서버 접속 오류 : [ %s ] [ %d ]  \n " ,szIP,nPort);

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
	dcmdHeader.nDataSize = sizeof(CCOM9203_R); //unsigend long
	dcmdHeader.nCmd = 9203;

	dwSendDataLen = HEADER_SIZE + dcmdHeader.nDataCnt * dcmdHeader.nDataSize;
	pSendData = new char[dwSendDataLen];

	memcpy( pSendData , &dcmdHeader,HEADER_SIZE);
	if( dcmdHeader.nDataCnt * dcmdHeader.nDataSize > 0 )
		memcpy( pSendData + HEADER_SIZE, &pcom9203_r , dcmdHeader.nDataCnt * dcmdHeader.nDataSize);

	if(ComSendData(dcmd_socket,pSendData, dwSendDataLen ) <= 0)
	{
		#ifdef __DEBUG
		printf(" ] com9203 요청 패킷 전송 \n");
		#endif
		infLOG(ERROR, "com9203 요청 패킷 전송 실패 \n");


		if(pSendData)
			delete[] pSendData;

		pSendData = NULL;

		infLOG(ERROR, "com9203[ERR]: 9203 통신 오류(S) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");


       	return(-1);
	}


	if(pSendData)
		delete[] pSendData;
	//--------------------------------------------------------------------------
	// DB 중개 서버로 부터 데이터 받기  (  )
	//--------------------------------------------------------------------------
	if(ComRecvData(dcmd_socket,(char*)&dcmdHeader,HEADER_SIZE ) <= 0)
	{

		infLOG(ERROR, "com9203[ERR]: 9203 통신 오류(R) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오.1 \n");


       	return(-1);

	}


	if( dcmdHeader.nCmd == 9203 )
	{

	}
	else
	{
		infLOG(ERROR, "com9203[ERR]: 9203 통신 오류(R) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오.2 \n");


		close(dcmd_socket);

       	return(-1);
	}
	close(dcmd_socket);

	infLOG(ALWAY, "com9203 성공\n");
	*/
	return 1;
}
