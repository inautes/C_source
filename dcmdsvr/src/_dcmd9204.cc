/******************************************************************************
 *   서브시스템 : database command 서버
 *   프로그램명 : dcmd9204.cc
 *         기능 : 한시간마다 접속자수 업데이트
 *         설명 : 한시간마다 컨텐츠서버의 접속자수를 확인하여 업데이트
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
#include <errno.h>

#include "apdefine.h"
#include "comcomm.h"
#include "commydb.h"
#include "com9204.h"
#include "dcmd9204.h"

//#define  _DEBUG_

// db 제어 서버
#include <unistd.h>     /* for close() getpid()*/
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include "dcmdsock.h" //sock send recv
// db 제어 서버

#include "mysql_pool.h"

extern CMysqlPool * m_g_clMysqlPool;

//******************************************************************************
//  COM9204 main
//
//  input : pCom9204_r->proc_flag => 1=wedisk, 2=mydata
//
//  return:  1(정상)
//          -1(DB오류)
//******************************************************************************
long dcmd9204(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData)//CCOM9204_R pCom9204_r)
{

	LPCCOM9204_R pCom9204_r = (LPCCOM9204_R)pRecvData;

	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);


	char szSysErrMsg[255];
	memset(szSysErrMsg,0x00,sizeof(szSysErrMsg));

	char szQuery[10000];
	MYSQL       *con=NULL;
	MYSQL_RES *res;
	MYSQL_ROW  row;

	#ifdef _DEBUG_
	printf("com9204-> start-------------------------------\n");
	printf("com9204-> server_id  (%s)\n" , pCom9204_r->server_id  );
	#endif

	// DB 연결
	bool bCloseDB = false;


	CMysqlCon MysqlCon(m_g_clMysqlPool,getpid());

	con = MysqlCon.GetMysqlCon();

	if (con == NULL )
	{
    	infLOG(ERROR, "GetMysqlCon is null \n");
		req_header.nCmd = -1 ;


		int nRetry = 0;
		while (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			#ifdef __DEBUG_
			printf(" ] DB 접속 재시도 \n");
			#endif
		}

		if( nRetry >= 5)
		{

			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);

	       	return HEADER_SIZE;

	    }
	    bCloseDB = true;


	}




	//--------------------------------------------------------------------------
	// T_SERVER_INFO의 dn_user 업데이트
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));

	sprintf(szQuery, "update zangsi.T_SERVER_INFO set dn_user = %d where server_id = '%s' "
	                 , pCom9204_r->nCount, pCom9204_r->server_id);

	infLOG(ERROR,"------------------------->%s",szQuery);

	#ifdef _DEBUG_
	printf("----------------------->%s\n" , szQuery  );
	#endif

	if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery)){
		infLOG(ERROR, "com9204[ERR]: SELECT * FROM T_CONTENTS_UPDN error\n");
		infLOG(ERROR, "com9204[ERR]: [ %s ] [%d](%s)\n",szQuery, mysql_errno(con), mysql_error(con));
		if( bCloseDB )
			db_disconnect(con);
		infLOG(ERROR, "dcmd9204[SQL]: %s\n", szQuery);
		req_header.nCmd = -1 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);
		return HEADER_SIZE;

		return -1;
    }

	mysql_free_result(res);
	if( bCloseDB )
		db_disconnect(con);

	#ifdef _DEBUG_
	printf("com9204-> end\n");
	#endif


	req_header.nCmd = 9204 ;
	pSendData = new char[HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

	return HEADER_SIZE;

}
