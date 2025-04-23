/******************************************************************************
 *   서브시스템 : FUP서버
 *   프로그램명 : com9106.h
 *         기능 : upload 시 nori.T_CONTENTS_TEMPLIST 에 이미 중복 처리 결과가 들어가므로 file_id,server_group_id 를 여기서 조회한다.
 *         설명 : 가라 업로드를 위한 처리.
 *       작성자 : 김일오
 *       작성일 : 2015.03.12
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

#include "apstruct.h"
#include "comconf.h"

#include "apdefine.h"
#include "comcomm.h"
#include "commydb.h"
#include "com9106.h"
//#define  _DEBUG_
// db 제어 서버
#include <unistd.h>     /* for close() getpid()*/
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include "comsock.h" //sock send recv
#include "comhead.h"
// db 제어 서버
//******************************************************************************
//  COM9106 main
//
//  input : pcom9106_r
//
//  return:  pcom9106_r (정상)
//          -1(DB오류)
//******************************************************************************
CCOM9106_R com9106(CCOM9106_R pcom9106_r,char* szIP, int nPort)
{
	//--------------------------------------------------------------------------
	// DB 중개 서버 연결 및 데이터 처리
	//--------------------------------------------------------------------------

	CCOM9106_R pcom9106_return;
	memset(&pcom9106_return,0x00,sizeof(CCOM9106_R));


	struct sockaddr_in dcmdSerAddr;
	int dcmd_socket;

	if ( ( dcmd_socket = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0 )
	{
		infLOG(ERROR, "com9106[ERR]: 소켓 생성 오류 입니다.\n");


		pcom9106_return.result_val = -91061;
		return pcom9106_return;

	}

	memset(&dcmdSerAddr,0x00,sizeof(dcmdSerAddr));
	dcmdSerAddr.sin_family      = AF_INET;
	dcmdSerAddr.sin_addr.s_addr = inet_addr(szIP);
	dcmdSerAddr.sin_port        = htons(nPort);

	if( connect(dcmd_socket,(struct sockaddr * )&dcmdSerAddr,sizeof(dcmdSerAddr)) < 0 )
	{
		 infLOG(ERROR, " ]com9106[ERR]중개서버 접속 오류 : [ %s ] [ %d ]  \n " ,szIP,nPort);

		pcom9106_return.result_val = -1;
		return pcom9106_return;
	}


		//--------------------------------------------------------------------------
		// DB 중개 서버 해더 및 데이터 전송  ( deal_no 전송 )
		//--------------------------------------------------------------------------
		char* pSendData = NULL; //데이터 전송 버퍼
		long dwSendDataLen = 0;

		HEADER dcmdHeader;
		memset(&dcmdHeader,0x00,HEADER_SIZE);

		dcmdHeader.nDataCnt = 1;
		dcmdHeader.nDataSize = sizeof(CCOM9106_R); //unsigend long
		dcmdHeader.nCmd = 9106;

		dwSendDataLen = HEADER_SIZE + dcmdHeader.nDataCnt * dcmdHeader.nDataSize;
		pSendData = new char[dwSendDataLen];

		memcpy( pSendData , &dcmdHeader,HEADER_SIZE);


		if( dcmdHeader.nDataCnt * dcmdHeader.nDataSize > 0 )
			memcpy( pSendData + HEADER_SIZE, &pcom9106_r , dcmdHeader.nDataCnt * dcmdHeader.nDataSize);

		if(ComSendData(dcmd_socket,pSendData, dwSendDataLen ) <= 0)
		{
			#ifdef __DEBUG
			printf(" ] com9106 요청 패킷 전송 ID \n");
			#endif
			infLOG(ERROR, "com9106 요청 패킷 전송 ID \n");


			if(pSendData)
				delete[] pSendData;

			pSendData = NULL;

			infLOG(ERROR, "com9106[ERR]: 1 9106 통신 오류(S) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");

			//close(dcmd_socket);
			pcom9106_return.result_val = -1;
			return pcom9106_return;
		}


		if(pSendData)
			delete[] pSendData;
		//--------------------------------------------------------------------------
		// DB 중개 서버로 부터 데이터 받기  (  )
		//--------------------------------------------------------------------------
		if(ComRecvData(dcmd_socket,(char*)&dcmdHeader,HEADER_SIZE ) <= 0)
		{
			infLOG(ERROR, "com9106[ERR]: 2 9106 통신 오류(R) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. 1\n");

			pcom9106_return.result_val = -1;
			return pcom9106_return;
		}

		if(dcmdHeader.nDataCnt * dcmdHeader.nDataSize > 0 )
		{
			infLOG(ALWAY, "ComRecvData()+ DB 중개 서버로 부터 데이터 받기\n");
			printf("ComRecvData()+ DB 중개 서버로 부터 데이터 받기\n");

			if(ComRecvData(dcmd_socket,(char*)&pcom9106_return,dcmdHeader.nDataCnt * dcmdHeader.nDataSize ) <= 0)
			{

				infLOG(ERROR, "com9106[ERR]: 3- 9106 통신 오류(R) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");
				printf("com9106[ERR]: 3- 9106 통신 오류(R) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");

				pcom9106_return.result_val = -1;
				return pcom9106_return;
			}

			infLOG(ALWAY,"pcom9106_return 확인 : \n"
										" temp_id = %lu      \n"
										" char b_duplicate  = %d        \n"
						, pcom9106_return.temp_id  ,pcom9106_return.b_duplicate);
		}

		if( dcmdHeader.nCmd == 9106 )
		{
			pcom9106_return.result_val = 1;
		}
		else
		{
			infLOG(ERROR, "com9106[ERR]: 5 9106 통신 오류(R) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오.2 \n");

			close(dcmd_socket);

			pcom9106_return.result_val = -1;
	    return pcom9106_return;
		}
		close(dcmd_socket);

		return pcom9106_return;
}
