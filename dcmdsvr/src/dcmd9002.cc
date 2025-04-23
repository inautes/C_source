/******************************************************************************
 *   서브시스템 : FDN서버
 *   프로그램명 : dcmd9002.cc
 *         기능 : 다운로드 시작
 *         설명 : 다운로드 시작시, 관련 정보 처리
 *       작성자 :
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
#include "com9002.h"
#include "dcmd9002.h"
//#define  _DEBUG_
//******************************************************************************
//  dcmd9002 main
//
//  input : pcom9002_r->proc_flag => 1=wedisk, 2=mydata
//
//  return:  1(정상)
//          -1(DB오류)
//******************************************************************************
#include "mysql_pool.h"

extern CMysqlPool * m_g_clMysqlPool;
extern CMysqlPool* m_g_clMysqlPoolDnLog;

long dcmd9002(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData,int nFlag,int nMode)//Cdcmd9002_R pcom9002_r,int nFlag)
{
	LPCCOM9002_R pcom9002_r = (LPCCOM9002_R)pRecvData;
	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);

	char szQuery[10000];
	MYSQL       *con=NULL;


	#ifdef _DEBUG_
	printf("dcmd9002-> start-------------------------------\n");
	printf("dcmd9002-> server_id(%s)\n" , pcom9002_r->server_id);
	printf("dcmd9002-> user_id  (%s)\n" , pcom9002_r->user_id  );
	printf("dcmd9002-> cer_key  (%s)\n" , pcom9002_r->cer_key  );
	#endif
/*
	infLOG(ALWAY, "dcmd9002 - start\n"
	              "dcmd9002 - server_id(%s)\n"
	              "dcmd9002 - user_id  (%s)\n"
	              "dcmd9002 - cer_key  (%s)\n"
	              , pcom9002_r->server_id, pcom9002_r->user_id, pcom9002_r->cer_key);
*/


/*
	 다중 다운로드 관련 하여 수정 - 20081224.
	dnmserver 서버와 연동
*/

	CMysqlCon MysqlConDnLog;
	MysqlConDnLog.ConnectPool(m_g_clMysqlPoolDnLog,m_g_clMysqlPoolDnLog->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );

	MYSQL* pDCon = MysqlConDnLog.GetMysqlCon();

	if( pDCon == NULL )
	{
		infLOG(ERROR, "DCMD9002 | GetMysqlCon is null\n");

		int nRetry = 0;
		while (!(pDCon=db_connect(OSP_LOG_DB_NAME		,OSP_LOG_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			infLOG(ERROR, "DCMD9002 | Cannot DB Connect - GetMysqlCon is null\n");
		}
		
		if( nRetry >= 5){
			req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);
			return HEADER_SIZE;
	    }
	}

	if( pDCon != NULL )
	{

		if(strlen(pcom9002_r->cer_key) > 0 || strcmp(pcom9002_r->cer_key, "") != 0)
		{
			memset (szQuery , 0x00, sizeof(szQuery ));
			sprintf(szQuery, "REPLACE INTO T_CONTENTS_CERKEY_UPDN "
			                 "     ( updn_flag, cer_key, cont_gu, server_id, conn_ip, id, reg_date, reg_time ) VALUES "
			                 "     ( 'DN'     , '%s'   ,  '%s'  , '%s'     , '%s'   , %lu ,date_format(now(),'%%Y%%m%%d'),date_format(now(),'%%H%%i%%s')) "
			                 , pcom9002_r->cer_key
			                 , pcom9002_r->cont_gu
			                 , pcom9002_r->server_id
			                 , pcom9002_r->conn_ip
			                 , pcom9002_r->temp_id
			                 );

			#ifdef _DEBUG_
			printf("dcmd9002 ->\n%s\n",szQuery);
			#endif

			//infLOG(ALWAY,"dcmd9002 ->\n%s\n",szQuery);

			if (mysql_query(pDCon, szQuery))
			{
				infLOG(ERROR, "dcmd9002[ERR]: [ %s ] [%d](%s)\n",szQuery,mysql_errno(pDCon), mysql_error(pDCon));
		    }
		}
		else if(strlen(pcom9002_r->user_id) > 0 || strcmp(pcom9002_r->user_id, "") != 0)
		{
			memset (szQuery , 0x00, sizeof(szQuery ));
			sprintf(szQuery, "REPLACE INTO zangsi.T_CONTENTS_UPDN "
			                 "     ( updn_flag, user_id, cont_gu, server_id, conn_ip, id, reg_date, reg_time ) VALUES "
			                 "     ( 'DN'     , '%s'   ,  '%s'  , '%s'     , '%s'   , %lu ,date_format(now(),'%%Y%%m%%d') ,date_format(now(),'%%H%%i%%s')) "
			                 , pcom9002_r->user_id
			                 , pcom9002_r->cont_gu
			                 , pcom9002_r->server_id
			                 , pcom9002_r->conn_ip
			                 , pcom9002_r->temp_id
			                 );
			#ifdef _DEBUG_
			printf("dcmd9002 ->\n%s\n",szQuery);
			#endif


			if (mysql_query(pDCon, szQuery))
			{
				infLOG(ERROR, "dcmd9002[ERR]: [ %s ] [%d](%s)\n",szQuery,mysql_errno(pDCon), mysql_error(pDCon));
		    }
		}
		//DCon.CloseDB();
	}


	bool bCloseDB = false;


	CMysqlCon MysqlCon;
	MysqlCon.ConnectPool(m_g_clMysqlPool,m_g_clMysqlPool->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );

	con = MysqlCon.GetMysqlCon();

	if (con == NULL )
	{
		infLOG(ERROR, "DCMD9002 | GetMysqlCon is null\n");


		int nRetry = 0;
		while (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			infLOG(ERROR, "DCMD9002 | Cannot DB Connect - GetMysqlCon is null\n");
			//infLOG(ERROR, "DCMD9002 | Mysql Error | Errno : %d | ErrMsg : %s \n\n", mysql_errno(con),mysql_error(con));
		}
		if( nRetry >= 5)
		{
			req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);
			infLOG(ERROR, "DCMD9002 | Cannot DB Connect \n");
			return HEADER_SIZE;
	    }

	    bCloseDB = true;
	    infLOG(ERROR,"DCMD9002 | Connect DB direct\n");

	}


	//--------------------------------------------------------------------------
	// 서버정보 다운로드 사용자 증가
	//--------------------------------------------------------------------------
	if( nFlag == 1)
	{
		memset (szQuery , 0x00, sizeof(szQuery ));
		sprintf(szQuery, "UPDATE T_SERVER_INFO    "
		                 "   SET dn_user   = dn_user + 1 "
		                 " WHERE server_id = '%s'        "
		                 ,pcom9002_r->server_id
		                 );

		//infLOG(ALWAY, "dcmd9002 : %s  \n",szQuery);
		if (mysql_query(con, szQuery))
		{
			infLOG(ERROR, "dcmd9002[ERR]: UPDATE  T_SERVER_INFO error\n");
			infLOG(ERROR, "dcmd9002[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			if( bCloseDB )
				db_disconnect(con);
			infLOG(ERROR, "dcmd9002[SQL]: %s\n", szQuery);
			req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);

	       	return HEADER_SIZE;
	    }


		memset (szQuery , 0x00, sizeof(szQuery ));
		sprintf(szQuery, "replace into T_CONTENTS_FILE_USER_CNT (id,cont_gu,cur_user_cnt )    "
		                 "   select id , cont_gu , cur_user_cnt + 1 from T_CONTENTS_FILE_USER_CNT "
		                 " WHERE id = %lu  and cont_gu = '%s'   "
		                 ,pcom9002_r->temp_id , pcom9002_r->cont_gu
		                 );

		//infLOG(ALWAY, "dcmd9002 : %s  \n",szQuery);

		if (mysql_query(con, szQuery))
		{
				infLOG(ERROR, "com9002[ERR]: [ %s ]  error\n",szQuery);
				infLOG(ERROR, "com9002[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    }



	}
/*	else
	{



	}
*/
	if( bCloseDB )
		db_disconnect(con);

	req_header.nCmd = 9002 ;
	pSendData = new char[HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

	#ifdef _DEBUG_
	printf("dcmd9002-> end\n");
	#endif

	return HEADER_SIZE;
}
