#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <mysql.h>

#include <unistd.h>     /* for close() getpid()*/ 
#include <sys/socket.h> /* for socket(), bind(), and connect() */ 
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */ 
#include "fupsock.h" //sock send recv 
#include "comhead.h"
#include "fups4005.h"
#include "fupcomlib.h"
#include "comcomm.h"
#include "commydb.h"
#include "apdefine.h"

long fupsflog4005( CFUPS4005 PFUPS4005, LPMUREKA_VINFO pMurekaVInfo )
{
	infLOG(ALWAY,"------------fupsflog4005 start-------------\n");
		
	//--------------------------------------------------------------------------
	// DB 중개 서버 연결 및 데이터 처리
	//--------------------------------------------------------------------------
	struct sockaddr_in dcmdSerAddr;
	int dcmd_socket;
	int nMurekaVCnt = PFUPS4005.mureka_cnt;
	
	if ( ( dcmd_socket = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0 )
	{
		infLOG(ERROR, "fupsflog4005 : socket() errno [ %d ] error msg [ %s ]\n",errno,strerror(errno));
		return(-2); 
	}
	
	memset(&dcmdSerAddr,0x00,sizeof(dcmdSerAddr));
	dcmdSerAddr.sin_family      = AF_INET;
	dcmdSerAddr.sin_addr.s_addr = inet_addr(DB_COMMAND_SERVER_IP);
	dcmdSerAddr.sin_port        = htons(DB_COMMAND_SERVER_PORT);

	if( connect(dcmd_socket,(struct sockaddr * )&dcmdSerAddr,sizeof(dcmdSerAddr)) < 0 )
	{
		infLOG(ERROR, "fupsflog4005 : connect() errno [ %d ] error msg [ %s ]\n",errno,strerror(errno));
		
		infLOG(ERROR, " fupsflog4005 : 중개서버 접속 오류 : [ %s ] [ %d ]  \n 다음 서버(%s)로 접속합니다. \n"
					,DB_COMMAND_SERVER_IP,DB_COMMAND_SERVER_PORT,DB_SUB_COMMAND_SERVER_IP);		
	
		dcmdSerAddr.sin_addr.s_addr = inet_addr(DB_SUB_COMMAND_SERVER_IP);			

		if( connect(dcmd_socket,(struct sockaddr * )&dcmdSerAddr,sizeof(dcmdSerAddr)) < 0 )
		{
			infLOG(ERROR, " fupsflog4005 : 중개서버 접속 오류 : [ %s ] [ %d ]  \n Network와 중계서버를 확인해주세요. \n"
						,DB_SUB_COMMAND_SERVER_IP,DB_COMMAND_SERVER_PORT);		
			return -2;
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
	
	if(nMurekaVCnt > 0 && pMurekaVInfo != NULL)
		dcmdHeader.nDataSize = sizeof(CFUPS4005) + (sizeof(MUREKA_VINFO) * nMurekaVCnt); // strlen(pData); //unsigend long
	else
		dcmdHeader.nDataSize = sizeof(CFUPS4005); // strlen(pData); //unsigend long
	dcmdHeader.nCmd = 40051;

	dwSendDataLen = HEADER_SIZE + dcmdHeader.nDataSize;
	pSendData = new char[dwSendDataLen];

	memcpy( pSendData , &dcmdHeader,HEADER_SIZE);

	memcpy( pSendData + HEADER_SIZE, &PFUPS4005 , sizeof(CFUPS4005));	

	if(nMurekaVCnt > 0 && pMurekaVInfo != NULL)
		memcpy( pSendData + HEADER_SIZE + sizeof(CFUPS4005), pMurekaVInfo , sizeof(MUREKA_VINFO) * nMurekaVCnt);	
	infLOG(ALWAY, "fupsflog4005 : 40051 전송 sizeof(CFUPS4005) = %ld  \n",sizeof(CFUPS4005));
	
	if(SendData( dcmd_socket, pSendData, dwSendDataLen ) <= 0)
	{
		infLOG(ERROR, "fupsflog4005 : 40051 전송 SendData() errno [ %d ] error msg [ %s ]\n",errno,strerror(errno));
		
		
		
		if(pSendData)
			delete[] pSendData;
		
		pSendData = NULL;
		
		infLOG(ERROR, "fupsflog4005: 40051 통신 오류(S) : 서버와의 통신 오류 입니다. 1분후 재시도 해주십시오. \n");
		
		
		close(dcmd_socket);
       	return(-2); 
	}
		
	
	if(pSendData)
		delete[] pSendData;	
	//--------------------------------------------------------------------------
	// DB 중개 서버로 부터 데이터 받기  (  )
	//--------------------------------------------------------------------------	
	if(RecvData(dcmd_socket,(char*)&dcmdHeader,HEADER_SIZE ) <= 0)
	{
			
		infLOG(ERROR, "fupsflog4005 : 40051 받기 RecvData() errno [ %d ] error msg [ %s ]\n",errno,strerror(errno));
		
		close(dcmd_socket);
       	return(-2); 
		
	}
	infLOG(ALWAY,"fupsflog4005 : result - dcmdHeader.nCmd [ %d ] \n",dcmdHeader.nCmd);
	
	
	if( dcmdHeader.nCmd > 0  )
	{
		close(dcmd_socket);
		return dcmdHeader.nCmd;
		
	}
	else if( dcmdHeader.nCmd == 0 )
	{
		close(dcmd_socket);
		return 0 ;
	}
	else
	{
		infLOG(ERROR, "fupsflog4005 : 40051 결과 오류 [ %d ] \n",dcmdHeader.nCmd );
		
		
		close(dcmd_socket);
		
       	return(-2); 		
	}
	
	close(dcmd_socket);
	
	
	return 0 ;
			
}
