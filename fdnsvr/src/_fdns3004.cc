
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

//******************************************************************************
//* fdns3004 main
//* return: 성공:0   오류:-1 (pErrMsg에 오류메시지를 리턴한다.)
//*         1. input   id
//*         2. output  CFDNS3004_R
//*         3. output  pErrMsg
//******************************************************************************
int fdns3004(unsigned long ul_id, CFDNS3004_R *p_fdns3004s, char* pErrMsg)
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
	memset(&dcmdSerAddr,0x00,sizeof(dcmdSerAddr));
	dcmdSerAddr.sin_family      = AF_INET;
	dcmdSerAddr.sin_addr.s_addr = inet_addr(DB_COMMAND_SERVER_IP);
	dcmdSerAddr.sin_port        = htons(DB_COMMAND_SERVER_PORT);
	
	#ifdef __DEBUG
	printf(" ] fdns3004 : DB ( %s ) ( %d ) 서버 접속  \n",DB_COMMAND_SERVER_IP,DB_COMMAND_SERVER_PORT);
	infLOG(ALWAY, " ] fdns3004 : DB ( %s ) ( %d ) 서버 접속  \n",DB_COMMAND_SERVER_IP,DB_COMMAND_SERVER_PORT);
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


	if( connect(dcmd_socket,(struct sockaddr * )&dcmdSerAddr,sizeof(dcmdSerAddr)) < 0 )
	{
		infLOG(ERROR, " 중개서버 접속 오류 : [ %s ] [ %d ]  \n 다음 서버로 접속합니다. \n",DB_COMMAND_SERVER_IP,DB_COMMAND_SERVER_PORT);		

		dcmdSerAddr.sin_addr.s_addr = inet_addr(DB_SUB_COMMAND_SERVER_IP);			
		
		if( connect(dcmd_socket,(struct sockaddr * )&dcmdSerAddr,sizeof(dcmdSerAddr)) < 0 )
		{
			infLOG(ERROR, " 중개서버 접속 오류 : [ %s ] [ %d ]  \n Network과 중계서버를 확인하세요. \n",DB_SUB_COMMAND_SERVER_IP,DB_COMMAND_SERVER_PORT);		
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
		dcmdHeader.nDataSize = sizeof(unsigned long ); //unsigend long
		dcmdHeader.nCmd = 3004;
		
		dwSendDataLen = HEADER_SIZE + dcmdHeader.nDataCnt * dcmdHeader.nDataSize;
		pSendData = new char[dwSendDataLen];
	
		memcpy( pSendData , &dcmdHeader,HEADER_SIZE);
		if( dcmdHeader.nDataCnt * dcmdHeader.nDataSize > 0 )
			memcpy( pSendData + HEADER_SIZE, &ul_id , dcmdHeader.nDataCnt * dcmdHeader.nDataSize);	


		#ifdef __DEBUG
		printf(" ] fdns3004 : 데이터 전송 ( %lu ) \n",ul_id);
		infLOG(ALWAY, " ] fdns3004 : 데이터 전송 ( %lu ) \n",ul_id);
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
				strcpyA(pErrMsg,"3004 통신 오류S) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오.  \n");

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
				strcpyA(pErrMsg,"3004 통신 오류(R) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오.  \n");
									
			close(dcmd_socket);
	       	return(-1); 
			
		}
		
		
		if( dcmdHeader.nCmd > 0 )
		{
			if(dcmdHeader.nDataCnt * dcmdHeader.nDataSize > 0 )
			{
	
				if( RecvData(dcmd_socket,(char*)p_fdns3004s,dcmdHeader.nDataCnt * dcmdHeader.nDataSize) <= 0 )
				{
					#ifdef __DEBUG
					printf(" ] fdns3004[ERR]: 3004 통신 오류(DR) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");
					#endif	
					infLOG(ERROR, "] fdns3004[ERR]: 3004 통신 오류(DR) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");
					
					
					if(pErrMsg)
						strcpyA(pErrMsg,"3004 통신 오류(DR) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오.  \n");
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
					strcpyA(pErrMsg,"fdns3004 통신 오류(R) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오.  \n");
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
