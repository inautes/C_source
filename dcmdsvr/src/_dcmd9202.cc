/******************************************************************************
 *   서브시스템 : FUP서버 // database command 서버
 *   프로그램명 : dcmd9202.cc
 *         기능 : fupserver 실행시 초기화
 *         설명 : T_SERVER_INFO 의 up_user와 up_size를 0으로 초기화
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
#include "com9202.h"
#include "dcmd9202.h"

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
//  COM9202 main
//
//  input : pCom9202_r->proc_flag => 1=wedisk, 2=mydata
//
//  return:  1(정상)
//          -1(DB오류)
//******************************************************************************
long dcmd9202(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData)//CCOM9202_R pCom9202_r)
{

	LPCCOM9202_R pCom9202_r = (LPCCOM9202_R)pRecvData;


	char szSysErrMsg[255];
	memset(szSysErrMsg,0x00,sizeof(szSysErrMsg));

	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);

	char szQuery[10000];
	MYSQL       *con=NULL;
	MYSQL_RES *res;
	MYSQL_ROW  row;

	#ifdef _DEBUG_
	printf("com9202-> start-------------------------------\n");
	printf("com9202-> server_id  (%s)\n" , pCom9202_r->server_id  );
	#endif

	// DB 연결
	bool bCloseDB = false;


	CMysqlCon MysqlCon(m_g_clMysqlPool,m_g_clMysqlPool->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );

	con = MysqlCon.GetMysqlCon();

	if (con == NULL )
	{
    	infLOG(ERROR, "DCMD9202 | GetMysqlCon is null \n");



		int nRetry = 0;
		while (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);

			infLOG(ERROR, "DCMD9202 | Cannot DB Connect - GetMysqlCon is null\n");
			//infLOG(ERROR, "DCMD9202 | Mysql Error | Errno : %d | ErrMsg : %s \n\n", mysql_errno(con),mysql_error(con));


		}

		if( nRetry >= 5)
		{
	    	req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);

			infLOG(ERROR, "DCMD9202 | Cannot DB Connect\n");

	       	return HEADER_SIZE;

	    }
	    bCloseDB = true;

	    infLOG(ERROR, "DCMD9202 | GetMysqlCon Direct Connect \n");



	}




	//--------------------------------------------------------------------------
	// T_SERVER_INFO의 up_user, up_size초기화
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));

	sprintf(szQuery, "update zangsi.T_SERVER_INFO set up_user = 0, up_size = 0 where server_id = '%s' "
	                 , pCom9202_r->server_id);


	if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery)){
		infLOG(ERROR, "com9202[ERR]: [ %s ] [%d](%s)\n",szQuery, mysql_errno(con), mysql_error(con));
		if( bCloseDB )
			db_disconnect(con);
		infLOG(ERROR, "dcmd9202[SQL]: %s\n", szQuery);
		req_header.nCmd = -1 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);
		return HEADER_SIZE;

		return -1;
    }


	if( bCloseDB )
		db_disconnect(con);

	#ifdef _DEBUG_
	printf("com9202-> end\n");
	#endif


	req_header.nCmd = 9202 ;
	pSendData = new char[HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

	return HEADER_SIZE;

}
