/******************************************************************************
 *   서브시스템 : FUP서버
 *   프로그램명 : com9101.cc
 *         기능 : upload 종료
 *         설명 : 서버에 업로드 종료 시점에 업로드 사용자 정보 삭제 및 카운트 감소
 *       작성일 :
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
#include "com9101.h"
#include "dcmd9101.h"
//#define  _DEBUG_
#include "mysql_pool.h"

extern CMysqlPool * m_g_clMysqlPool;
extern CMysqlPool* m_g_clMysqlPoolDnLog;

//******************************************************************************
//  COM9101 main
//
//  input : pcom9101_r->proc_flag => 1=wedisk, 2=mydata
//
//  return:  1(정상)
//          -1(DB오류)
//******************************************************************************
long dcmd9101(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData)//CCOM9101_R pcom9101_r)
{

	LPCCOM9101_R pcom9101_r = (LPCCOM9101_R)pRecvData;

	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);



	char szQuery[10000];
	MYSQL       *con=NULL;


	#ifdef _DEBUG_
	printf("com9101-> start-------------------------------\n");
	printf("com9101-> server_id(%s)\n" , pcom9101_r->server_id);
	printf("com9101-> user_id  (%s)\n" , pcom9101_r->user_id  );
	printf("com9001-> upload_size  (%15.0f )\n", pcom9101_r->upload_size  );
	#endif

	// 다중 다운로드 관련 하여 수정 - 20081224
	CMysqlCon MysqlConDnLog;
	MysqlConDnLog.ConnectPool(m_g_clMysqlPoolDnLog,m_g_clMysqlPoolDnLog->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );

	MYSQL* pDCon = MysqlConDnLog.GetMysqlCon();

	if( pDCon == NULL )
	{
		infLOG(ERROR,"9001 치명적인(Critical Error) 오류 입니다.\n");
	}
	else
	{
		//--------------------------------------------------------------------------
		// 업로드사용자 처리
		//--------------------------------------------------------------------------
		memset (szQuery , 0x00, sizeof(szQuery ));
		sprintf(szQuery, "DELETE FROM zangsi.T_CONTENTS_UPDN where updn_flag = 'UP' and user_id = '%s'"
		                 , pcom9101_r->user_id
		                 );

		if (mysql_query(pDCon, szQuery)){
			infLOG(ERROR, "com9101[ERR]: [ %s ] [%d](%s)\n",szQuery,mysql_errno(pDCon), mysql_error(pDCon));
	    }

		//DCon.CloseDB();
	}

	// DB 연결
	bool bCloseDB = false;


	CMysqlCon MysqlCon(m_g_clMysqlPool,m_g_clMysqlPool->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );

	con = MysqlCon.GetMysqlCon();

	if (con == NULL )
	{
		infLOG(ERROR, "DCMD9101 | GetMysqlCon is null \n");

		int nRetry = 0;
		while (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			infLOG(ERROR, "DCMD9101 | Cannot DB Connect - GetMysqlCon is null\n");
			//infLOG(ERROR, "DCMD9101 | Mysql Error | Errno : %d | ErrMsg : %s \n\n", mysql_errno(con),mysql_error(con));
		}
		if( nRetry >= 5)
		{
			req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);
			infLOG(ERROR, "DCMD9101 | Cannot DB Connect \n");
			return HEADER_SIZE;
	    }
		infLOG(ERROR, "DCMD9101 | GetMysqlCon Direct Connect \n");

	    bCloseDB = true;
	}

	//--------------------------------------------------------------------------
	// 서버정보 업로드 사용자 감소
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "UPDATE T_SERVER_INFO      	"
	                 "   SET up_user = if(up_user<1, 0, up_user-1)"
	                 "     , up_size = if(up_size - %15.0f < 0, 0, up_size - %15.0f)"
	                 " WHERE server_id  = '%s'         	"
	                 ,pcom9101_r->upload_size
	                 ,pcom9101_r->upload_size
	                 ,pcom9101_r->server_id
	                 );


	if (mysql_query(con, szQuery))
	{
		infLOG(ERROR, "com9101[ERR]: T_SERVER_INFO error\n");
		infLOG(ERROR, "com9101[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		if( bCloseDB )
			db_disconnect(con);

		infLOG(ERROR, "com9101[SQL]: %s\n", szQuery);
		req_header.nCmd = -1 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);

       	return HEADER_SIZE;
    }

	if( bCloseDB )
		db_disconnect(con);

	#ifdef _DEBUG_
	printf("com9101-> end\n");
	#endif




	req_header.nCmd = 9101 ;
	pSendData = new char[HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

	return HEADER_SIZE;

}
