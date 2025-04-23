/******************************************************************************
 *   서브시스템 : CMD서버
 *   프로그램명 : fups4001.cc
 *         기능 : 컨텐츠 등록
 *         설명 :
 *       작성자 : LEE
 *       작성일 : 2004/02/16
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
#include <unistd.h>

#include "fupcomlib.h"
#include "comcomm.h"
#include "commydb.h"
#include "apdefine.h"
#include "fups4001.h"
//#define  _DEBUG_
#include <unistd.h>     /* for close() getpid()*/
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include "fupsock.h" //sock send recv
#include "comhead.h"


/******************************************************************************
** nServerFlag -> 1 : We디스크
**                2 : 내디스크
*******************************************************************************/

long fups4001(CFUPS4001 pfups4001 )
{
	//--------------------------------------------------------------------------
	// DB 중개 서버 연결 및 데이터 처리
	//--------------------------------------------------------------------------
	struct sockaddr_in dcmdSerAddr;
	int dcmd_socket;

	if ( ( dcmd_socket = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0 )
	{
		infLOG(ERROR, "fups4001[ERR]: 소켓 생성 오류 입니다.\n");

       	return(-1);
	}
	memset(&dcmdSerAddr,0x00,sizeof(dcmdSerAddr));
	dcmdSerAddr.sin_family      = AF_INET;
	dcmdSerAddr.sin_addr.s_addr = inet_addr(DB_COMMAND_SERVER_IP);
	dcmdSerAddr.sin_port        = htons(DB_COMMAND_SERVER_PORT);

	struct timeval tv;
	tv.tv_sec = 5; //5초

	int st = setsockopt(dcmd_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));
	if(st != 0)
		infLOG(ALWAY, " ] 소켓 recv time out 옵션 설정 실패 errno = ( %d )\n",errno);

	struct timeval tv2;
	tv2.tv_sec = 5; //5초

	st = setsockopt(dcmd_socket, SOL_SOCKET, SO_SNDTIMEO, &tv2, sizeof(struct timeval));
	if(st != 0)
		infLOG(ALWAY, " ]소켓 send time out 옵션 설정 실패 errno = ( %d )\n",errno);


	if( connect(dcmd_socket,(struct sockaddr * )&dcmdSerAddr,sizeof(dcmdSerAddr)) < 0 )
	{
		infLOG(ERROR, " 중개서버 접속 오류 : [ %s ] [ %d ]  \n 다음 서버(%s)로 접속합니다. \n"
					,DB_COMMAND_SERVER_IP,DB_COMMAND_SERVER_PORT,DB_SUB_COMMAND_SERVER_IP);

		dcmdSerAddr.sin_addr.s_addr = inet_addr(DB_SUB_COMMAND_SERVER_IP);

		if( connect(dcmd_socket,(struct sockaddr * )&dcmdSerAddr,sizeof(dcmdSerAddr)) < 0 )
		{
			infLOG(ERROR, " 중개서버 접속 오류 : [ %s ] [ %d ]  \n 다음 서버로 접속합니다. \n"
						,DB_SUB_COMMAND_SERVER_IP,DB_SUB_COMMAND_SERVER_PORT);
			return -1;
		}
	}


	//--------------------------------------------------------------------------
	// DB 중개 서버 해더 및 데이터 전송  ( deal_no 전송 )
	//--------------------------------------------------------------------------
	char* pSendData = NULL; //데이터 전송 버퍼
	long dwSendDataLen = 0;

	HEADER dcmdHeader;
	memset(&dcmdHeader,0x00,HEADER_SIZE);

	dcmdHeader.nDataCnt = 1;
	dcmdHeader.nDataSize = sizeof(CFUPS4001 ); //unsigend long
	dcmdHeader.nCmd = 4001;

	dwSendDataLen = HEADER_SIZE + dcmdHeader.nDataCnt * dcmdHeader.nDataSize;
	pSendData = new char[dwSendDataLen];

	memcpy( pSendData , &dcmdHeader,HEADER_SIZE);
	if( dcmdHeader.nDataCnt * dcmdHeader.nDataSize > 0 )
		memcpy( pSendData + HEADER_SIZE, &pfups4001 , dcmdHeader.nDataCnt * dcmdHeader.nDataSize);

	infLOG(ALWAY, " 4001 Packet Send [ %d ] ( %d )  \n",HEADER_SIZE,dwSendDataLen);
	infLOG(ALWAY,"4001 copyright    (%s)\n", pfups4001.copyright_yn );

	if(SendData(dcmd_socket,pSendData, dwSendDataLen ) <= 0)
	{
		#ifdef __DEBUG
		printf(" ] fups4001 요청 패킷 전송 \n");
		#endif
		infLOG(ERROR, " ] fups4001 요청 패킷 전송 \n");
		if(pSendData)
			delete[] pSendData;

		pSendData = NULL;
		infLOG(ERROR, "fups4001[ERR]: 4001 통신 오류(S) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");
		close(dcmd_socket);
       	return(-1);
	}


	if(pSendData)
		delete[] pSendData;
	//-------------------------------------------ㅏ------------------------------
	// DB 중개 서버로 부터 데이터 받기  (  )
	//--------------------------------------------------------------------------

	if(RecvData(dcmd_socket,(char*)&dcmdHeader,HEADER_SIZE ) <= 0)
	{
		infLOG(ERROR, "fups4001[ERR]: 4001 통신 오류(R) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");
		close(dcmd_socket);
       	return(-1);
	}

	if( dcmdHeader.nCmd > 0  )
	{

	}
	else
	{
		infLOG(ERROR, "fups4001[ERR]: 4001 관련 오류(R) [ %d ]  \n",dcmdHeader.nCmd);

		close(dcmd_socket);

		if( dcmdHeader.nCmd  == -400199)
			return (-2);

		return(-1);
	}
	close(dcmd_socket);
	return 1 ;

}
