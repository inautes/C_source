	/******************************************************************************
 *   서브시스템 : FDN서버
 *   프로그램명 : fdns3004.cc
 *         기능 : 무료자료실 다운로드
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

#include "comcomm.h"
#include "commydb.h"
#include "apdefine.h"
#include "fdns3004.h"
#include <unistd.h>     /* for close() getpid()*/
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include "fdnsock.h" //sock send recv
#include "comhead.h"
#include <errno.h>
//#define  _DEBUG_

//extern multimap<int,USERINFO> m_UserList;
extern char g_szOsp_id[10];
extern char g_szDcmdIP[16];
extern int  g_nDcmdPort; //  = 0;

extern char g_szSUB_DcmdIP[16];
extern int  g_nSUB_DcmdPort;
extern bool g_bConnect1 ;
extern bool g_bConnect2 ;

//******************************************************************************
//* fdns3004 main
//* return: 성공:0   오류:-1 (pErrMsg에 오류메시지를 리턴한다.)
//*         1. input   id
//*         2. output  CFDNS3004_R
//*         3. output  pErrMsg
//******************************************************************************
//int fdns3004(unsigned long ul_id, CFDNS3004_R* Pfdns3004,char* pErrMsg)

int fdns3004(unsigned long ul_id, CFDNS3004_R *Pfdns3004, char* pErrMsg)
{


		//--------------------------------------------------------------------------
		// DB 중개 서버 연결 및 데이터 처리
		//--------------------------------------------------------------------------
		struct sockaddr_in dcmdSerAddr;
		int dcmd_socket;

		#ifdef __DEBUG
		printf(" ] fdns3004 : 소켓 생성 \n");
		infLOG(ALWAY, " ] fdns3004 : 소켓 생성 \n");
		#endif

		if ( ( dcmd_socket = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0 )
		{
			#ifdef __DEBUG
			printf(" ] fdns3004[ERR]: 소켓 생성 오류 입니다.\n");
			#endif
			infLOG(ERROR, "] fdns3004[ERR]: 소켓 생성 오류 입니다.\n");


		return(-1);

	}

	#ifdef __DEBUG
	printf(" ] fdns3004 : DB ( %s ) ( %d ) 서버 접속  \n",g_szDcmdIP,g_nDcmdPort);
	infLOG(ALWAY, " ] fdns3004 : DB ( %s ) ( %d ) 서버 접속  \n",g_szDcmdIP,g_nDcmdPort);
	#endif

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


	memset(&dcmdSerAddr,0x00,sizeof(dcmdSerAddr));
	dcmdSerAddr.sin_family      = AF_INET;

	if(g_bConnect1 == true)
	{
		dcmdSerAddr.sin_addr.s_addr = inet_addr(g_szDcmdIP);
		dcmdSerAddr.sin_port        = htons(g_nDcmdPort);
		infLOG(ALWAY, " 중개서버 접속  : [ %s ] [ %d ] \n",g_szDcmdIP,g_nDcmdPort);
	}
	else if(g_bConnect2 == true)
	{
		dcmdSerAddr.sin_addr.s_addr = inet_addr(g_szSUB_DcmdIP);
		dcmdSerAddr.sin_port        = htons(g_nSUB_DcmdPort);
		infLOG(ALWAY, " 중개서버 접속  2 : [ %s ] [ %d ] \n",g_szSUB_DcmdIP,g_nSUB_DcmdPort);
	}
	else
	{
		infLOG(ERROR, " fdns3004 : 중개서버 접속 오류 g_bConnect1 = false, g_bConnect2 = false \n" );
		return -1;
	}

	if( connect(dcmd_socket,(struct sockaddr * )&dcmdSerAddr,sizeof(dcmdSerAddr)) < 0 )
	{
		infLOG(ERROR, " fdns3004 : 중개서버 접속 오류 \n" );
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
		dcmdHeader.nDataSize = sizeof(unsigned long ); //unsigend long
		dcmdHeader.nCmd = 3004;

		dwSendDataLen = HEADER_SIZE + dcmdHeader.nDataCnt * dcmdHeader.nDataSize;
		pSendData = new char[dwSendDataLen];

		memcpy( pSendData , &dcmdHeader,HEADER_SIZE);
		if( dcmdHeader.nDataCnt * dcmdHeader.nDataSize > 0 )
			memcpy( pSendData + HEADER_SIZE, &ul_id , dcmdHeader.nDataCnt * dcmdHeader.nDataSize);


		#ifdef __DEBUG
		printf(" ] fdns3004 : 데이터 전송 ( %ld ) \n",ul_id);
		infLOG(ALWAY, " ] fdns3004 : 데이터 전송 ( %ld ) \n",ul_id);
		#endif

		if(SendData(dcmd_socket,pSendData, dwSendDataLen ) <= 0)
		{


			if(pSendData)
				delete[] pSendData;

			pSendData = NULL;

			#ifdef __DEBUG
			printf(" ] fdns3004[ERR]: 3004 통신 오류(S) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");
			#endif



			if(pErrMsg)
				strcpy(pErrMsg,"3004 통신 오류S) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오.  \n");

			infLOG(ERROR, " ] fdns3004[ERR]: 3004 통신 오류(S) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");

			close(dcmd_socket);
	       	return(-1);
		}


		if(pSendData)
			delete[] pSendData;
		//--------------------------------------------------------------------------
		// DB 중개 서버로 부터 데이터 받기  (  )
		//--------------------------------------------------------------------------
		if(RecvData(dcmd_socket,(char*)&dcmdHeader,HEADER_SIZE ) <= 0)
		{
			#ifdef __DEBUG
			printf(" ] fdns3004[ERR]: 3004 통신 오류(R) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");
			#endif
			infLOG(ERROR, " ] fdns3004[ERR]: 3004 통신 오류(R) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");

			if(pErrMsg)
				strcpy(pErrMsg,"3004 통신 오류(R) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오.  \n");

			close(dcmd_socket);
	       	return(-1);

		}


		if( dcmdHeader.nCmd > 0 )
		{
			if(dcmdHeader.nDataCnt * dcmdHeader.nDataSize > 0 )
			{

				if( RecvData(dcmd_socket,(char*)Pfdns3004,dcmdHeader.nDataCnt * dcmdHeader.nDataSize) <= 0 )
				{
					#ifdef __DEBUG
					printf(" ] fdns3004[ERR]: 3004 통신 오류(DR) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");
					#endif
					infLOG(ERROR, "] fdns3004[ERR]: 3004 통신 오류(DR) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");


					if(pErrMsg)
						strcpy(pErrMsg,"3004 통신 오류(DR) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오.  \n");
					close(dcmd_socket);
			       	return(-1);
				}
			}


		}
		else
		{
			if( RecvData(dcmd_socket,pErrMsg,ERR_HEADER_SIZE - HEADER_SIZE) <= 0 )
			{
				infLOG(ERROR, " ] fdns3004[ERR]: 3004 통신 오류(R) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");


				if(pErrMsg)
					strcpy(pErrMsg,"fdns3004 통신 오류(R) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오.  \n");
				close(dcmd_socket);
		       	return(-1);
			}


			infLOG(ERROR, "] fdns3004[ERR]: 3004 통신 오류(DR) : 서버와의 통신 오류 입니다. \n");


			close(dcmd_socket);

	       	return(-1);
		}
		close(dcmd_socket);


	return 0;

}
