/******************************************************************************
 *   서브시스템 : database command 서버
 *   프로그램명 : dcmd9203.cc
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
#include <errno.h>

#include "apdefine.h"
#include "comcomm.h"
#include "commydb.h"
#include "com9203.h"
#include "dcmd9203.h"

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
//  COM9203 main
//
//  input : pCom9203_r->proc_flag => 1=wedisk, 2=mydata
//
//  return:  1(정상)
//          -1(DB오류)
//******************************************************************************
long dcmd9203(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData)//CCOM9203_R pCom9203_r)
{

	LPCCOM9203_R pCom9203_r = (LPCCOM9203_R)pRecvData;

	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);


	char szSysErrMsg[255];
	memset(szSysErrMsg,0x00,sizeof(szSysErrMsg));

	char szQuery[10000];
	MYSQL       *con=NULL;
	MYSQL_RES *res;
	MYSQL_ROW  row;

	#ifdef _DEBUG_
	printf("com9203-> start-------------------------------\n");
	printf("com9203-> server_id  (%s)\n" , pCom9203_r->server_id  );
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
	// T_SERVER_INFO의 dn_user초기화
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));

	sprintf(szQuery, "update zangsi.T_SERVER_INFO set dn_user = 0 where server_id = '%s' "
	                 , pCom9203_r->server_id);


	if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery)){
		infLOG(ERROR, "com9203[ERR]: SELECT * FROM T_CONTENTS_UPDN error\n");
		infLOG(ERROR, "com9203[ERR]: [ %s ] [%d](%s)\n",szQuery, mysql_errno(con), mysql_error(con));
		if( bCloseDB )
			db_disconnect(con);
		infLOG(ERROR, "dcmd9203[SQL]: %s\n", szQuery);
		req_header.nCmd = -1 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);
		return HEADER_SIZE;

		return -1;
		*/
    }

	//--------------------------------------------------------------------------
	// T_CONTENTS_FILE_USER_CNT의 cur_user_cnt초기화
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery," update zangsi.T_CONTENTS_FILE a, zangsi.T_CONTENTS_FILE_USER_CNT b  "
					" set b.cur_user_cnt = 0	"
					" where a.id = b.id and a.server_id = '%s'  "
			, pCom9203_r->server_id);



	if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery)){
		infLOG(ERROR, "com9203[ERR]: SELECT * FROM T_CONTENTS_UPDN error\n");
		infLOG(ERROR, "com9203[ERR]: [ %s ] [%d](%s)\n",szQuery, mysql_errno(con), mysql_error(con));
		/*
		if( bCloseDB )
			db_disconnect(con);
		infLOG(ERROR, "dcmd9203[SQL]: %s\n", szQuery);
		req_header.nCmd = -1 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);
		return HEADER_SIZE;

		return -1;
		*/
    }


	mysql_free_result(res);
	if( bCloseDB )
		db_disconnect(con);

	#ifdef _DEBUG_
	printf("com9203-> end\n");
	#endif


	req_header.nCmd = 9203 ;
	pSendData = new char[HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

	return HEADER_SIZE;

}
