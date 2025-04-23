/******************************************************************************
 *   서브시스템 : FUP서버
 *   프로그램명 : dcmd9001.cc
 *         기능 : upload 시작
 *         설명 : 서버에 업로드하기 전에 호출되는 함수
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
#include "com9001.h"
#include "dcmd9001.h"
//#define  _DEBUG_

// db 제어 서버
#include <unistd.h>     /* for close() getpid()*/
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include "dcmdsock.h" //sock send recv
// db 제어 서버

#include "mysql_pool.h"

extern CMysqlPool * m_g_clMysqlPool;
extern CMysqlPool* m_g_clMysqlPoolDnLog;

//******************************************************************************
//  COM9001 main
//
//  input : pCom9001_r->proc_flag => 1=wedisk, 2=mydata
//
//  return:  1(정상)
//          -1(DB오류)
//******************************************************************************
long dcmd9001(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData)//CCOM9001_R pcom9001_r)
{
	LPCCOM9001_R pCom9001_r = (LPCCOM9001_R)pRecvData;

	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);

	char szQuery[10000];
	MYSQL       *con=NULL;

	#ifdef _DEBUG_
	printf("dcmd9001-> start-------------------------------\n");
	printf("dcmd9001-> server_id(%s)\n" , pCom9001_r->server_id);
	printf("dcmd9001-> user_id  (%s)\n" , pCom9001_r->user_id  );
	printf("dcmd9001-> conn_ip  (%s)\n" , pCom9001_r->conn_ip  );
	printf("dcmd9001-> temp_id  (%lu)\n", pCom9001_r->temp_id  );
	printf("dcmd9001-> upload_size  (%15.0f )\n", pCom9001_r->upload_size  );
	#endif

	// 다중 다운로드 관련 하여 수정 - 20081224
	//--------------------------------------------------------------------------
	// 업로드사용자 처리
	//--------------------------------------------------------------------------
	//char* pDBName, char* pDBIp, char* pDBUser, char* pDBPass )

	CMysqlCon MysqlConDnLog;
	MysqlConDnLog.ConnectPool(m_g_clMysqlPoolDnLog,m_g_clMysqlPoolDnLog->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );

	MYSQL* pDCon = MysqlConDnLog.GetMysqlCon();

	if( pDCon == NULL )
	{
		int nRetry = 0;
		infLOG(ERROR, "dcmd9001: m_g_clMysqlPoolDnLog is null\n");
		req_header.nCmd = -1 ;
		while (!(pDCon=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			infLOG(ERROR, "DCMD9001 | Cannot DB Connect - GetMysqlCon is null\n");
		}

		if( nRetry >= 5){
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);
			return HEADER_SIZE;
	    }
	}
	if( pDCon == NULL )
	{
		infLOG(ERROR,"9001 Error 오류 입니다.\n");
	}
	else
	{
		memset (szQuery , 0x00, sizeof(szQuery ));
		sprintf(szQuery, "REPLACE INTO zangsi.T_CONTENTS_UPDN "
		                 "     ( updn_flag, user_id, cont_gu, server_id, conn_ip, id, reg_date, reg_time ) VALUES "
		                 "     ( 'UP'     , '%s'   , '%s'   , '%s'     , '%s'   , %lu ,date_format(now(),'%%Y%%m%%d') ,date_format(now(),'%%H%%i%%s'))"
		                 , pCom9001_r->user_id
		                 , pCom9001_r->cont_gu
		                 , pCom9001_r->server_id
		                 , pCom9001_r->conn_ip
		                 , pCom9001_r->temp_id
		                 );


		if (mysql_query(pDCon, szQuery))
		{
			infLOG(ERROR, "dcmd9001[SQL]: [ %s ] [%d](%s)\n",szQuery,mysql_errno(pDCon), mysql_error(pDCon));
	    }
		//DCon.CloseDB();
	}
	bool bCloseDB = false;

	CMysqlCon MysqlCon(m_g_clMysqlPool,m_g_clMysqlPool->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );

	con = MysqlCon.GetMysqlCon();

	if (con == NULL )
	{
		infLOG(ERROR, "DCMD9001 | GetMysqlCon is null\n");

		int nRetry = 0;
		while(!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			infLOG(ERROR, "DCMD9001 | Cannot DB Connect - GetMysqlCon is null\n");
			//infLOG(ERROR, "DCMD9001 | Mysql Error | Errno : %d | ErrMsg : %s \n\n", mysql_errno(con),mysql_error(con));
		}
		if( nRetry >= 5)
		{
			req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);
			infLOG(ERROR, "DCMD9001 | Cannot DB Connect \n");
			return HEADER_SIZE;
	    }

	    bCloseDB = true;
	    infLOG(ERROR,"DCMD9001 | Connect DB direct\n");
	}

	//--------------------------------------------------------------------------
	// 서버정보 업로드 사용자 증가
	//--------------------------------------------------------------------------
	memset(szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "UPDATE T_SERVER_INFO      	"
	                 "   SET up_user = up_user + 1 		"
	                 "     , up_size = up_size + %15.0f "
	                 " WHERE server_id  = '%s'         	"
	                 ,pCom9001_r->upload_size
	                 ,pCom9001_r->server_id);

	if (mysql_query(con, szQuery))
	{
		infLOG(ERROR, "dcmd9001[ERR]: UPDATE T_SERVER_INFO  SET up_user = up_user + 1 error\n");
		infLOG(ERROR, "dcmd9001[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		if( bCloseDB )
			db_disconnect(con);
		infLOG(ERROR, "dcmd9001[SQL]: %s\n", szQuery);
		req_header.nCmd = -1 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);
		return HEADER_SIZE;
    }

	if( bCloseDB )
		db_disconnect(con);

	#ifdef _DEBUG_
	printf("dcmd9001-> end\n");
	#endif

	req_header.nCmd = 9001 ;
	pSendData = new char[HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

	return HEADER_SIZE;
}
