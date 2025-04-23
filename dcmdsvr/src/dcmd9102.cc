/******************************************************************************
 *   서브시스템 : FDN서버
 *   프로그램명 : com9102.cc
 *         기능 :
 *         설명 : 중복 접속 처리, 다운로드 사용자수 감소, 컨텐츠 다운로드 수 감소
 *     수정이력 :
 *     수정내용 : 2.0다운로드로 인증코드가 존재할 경우 중복접속제거를 인증코드기준으로 함.
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
#include "com9102.h"
#include "dcmd9102.h"
//#define  _DEBUG_
// db 제어 서버
#include <unistd.h>     /* for close() getpid()*/
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include "dcmdsock.h" //sock send recv

#include "mysql_pool.h"

extern CMysqlPool * m_g_clMysqlPool;
extern CMysqlPool* m_g_clMysqlPoolDnLog;

// db 제어 서버
//******************************************************************************
//  com9102 main
//
//  input : pcom9102_r->proc_flag => 1=wedisk, 2=mydata
//
//  return:  1(정상)
//          -1(DB오류)
//******************************************************************************
long dcmd9102(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData,int nMode)//CCOM9102_R pcom9102_r)
{

	LPCCOM9102_R pcom9102_r = (LPCCOM9102_R)pRecvData;

	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);




	char szQuery[10000];
	MYSQL       *con=NULL;

	#ifdef _DEBUG_
	printf("com9102-> start-------------------------------\n");
	printf("com9102-> server_id(%s)\n" , pcom9102_r->server_id);
	printf("com9102-> user_id  (%s)\n" , pcom9102_r->user_id  );
	printf("com9102-> cer_key  (%s)\n" , pcom9102_r->cer_key  );
	#endif
/*
	infLOG(ALWAY,  "com9102-> start-------------------------------\n"
	               "com9102-> server_id(%s)\n"
	               "com9102-> user_id  (%s)\n"
	               "com9102-> cer_key  (%s)\n"
	               , pcom9102_r->server_id  , pcom9102_r->user_id, pcom9102_r->cer_key  );
*/



/*
	 다중 다운로드 관련 하여 수정 - 20081224.
	 2009-01-28 2.0 기능 추가 -- HCS
	 2009-01-29 9002와 동일하게 변경.
	dnmserver 서버와 연동
*/

	CMysqlCon MysqlConDnLog;
	MysqlConDnLog.ConnectPool(m_g_clMysqlPoolDnLog,m_g_clMysqlPoolDnLog->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );

	MYSQL* pDCon = MysqlConDnLog.GetMysqlCon();

	if( pDCon == NULL )
	{

		int nRetry = 0;
		infLOG(ERROR, "com9102: m_g_clMysqlPoolDnLog is null\n");
		req_header.nCmd = -1 ;
		while (!(pDCon=db_connect(OSP_LOG_DB_NAME		,OSP_LOG_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			infLOG(ERROR, "DCMD9102 | Cannot DB Connect - GetMysqlCon is null\n");
			infLOG(ERROR, "DCMD9102 | Mysql Error | Errno : %d | ErrMsg : %s \n\n", mysql_errno(pDCon),mysql_error(pDCon));
		}
		
				if( nRetry >= 5){
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);
			return HEADER_SIZE;
	    }
	}

	if( pDCon != NULL )
	{
		//--------------------------------------------------------------------------
		// 다운사용자 종료 처리
		//--------------------------------------------------------------------------
		if(strlen(pcom9102_r->cer_key) > 0 || strcmp(pcom9102_r->cer_key, "") != 0)
		{
			memset (szQuery , 0x00, sizeof(szQuery ));

			if( strcmp(pcom9102_r->conn_ip,"") == 0)
			{
				sprintf(szQuery, "DELETE FROM T_CONTENTS_CERKEY_UPDN where updn_flag = 'DN' and cer_key = '%s'  "
			                 , pcom9102_r->cer_key
			                 );
			}
			else
			{
				sprintf(szQuery, "DELETE FROM T_CONTENTS_CERKEY_UPDN where updn_flag = 'DN' and cer_key = '%s' and conn_ip = '%s' "
			                 , pcom9102_r->cer_key , pcom9102_r->conn_ip
			                 );
			}

			#ifdef _DEBUG_
			printf("com9102 ] \n%s\n",szQuery);
			#endif

			if (mysql_query(pDCon, szQuery))
			{
				infLOG(ERROR, "com9102[ERR]: [ %s ] [%d](%s)\n",szQuery,mysql_errno(pDCon), mysql_error(pDCon));
		    }

		}
		else if(strlen(pcom9102_r->user_id) > 0 || strcmp(pcom9102_r->user_id, "") != 0)
		{
			memset (szQuery , 0x00, sizeof(szQuery ));

			if( strcmp(pcom9102_r->conn_ip,"") == 0)
			{
				sprintf(szQuery, "DELETE FROM zangsi.T_CONTENTS_UPDN where updn_flag = 'DN' and user_id = '%s'  "
			                 , pcom9102_r->user_id
			                 );
			}
			else
			{
				sprintf(szQuery, "DELETE FROM zangsi.T_CONTENTS_UPDN where updn_flag = 'DN' and user_id = '%s' and conn_ip = '%s' "
			                 , pcom9102_r->user_id , pcom9102_r->conn_ip
			                 );
			}

			#ifdef _DEBUG_
			printf("com9102 ] \n%s\n",szQuery);
			#endif

			if (mysql_query(pDCon, szQuery))
			{
				infLOG(ERROR, "com9102[ERR]: [ %s ] [%d](%s)\n",szQuery,mysql_errno(pDCon), mysql_error(pDCon));
		    }

		}
		//DCon.CloseDB();

	}

	// DB 연결
	bool bCloseDB = false;


	CMysqlCon MysqlCon(m_g_clMysqlPool,m_g_clMysqlPool->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );

	if ( (con = MysqlCon.GetMysqlCon() ) == NULL )
	{
		infLOG(ERROR, "DCMD9102 | GetMysqlCon is null \n");

		int nRetry = 0;
		while (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			infLOG(ERROR, "CMD9102 | Cannot DB Connect - GetMysqlCon is null\n");
			//infLOG(ERROR, "CMD9102 | Mysql Error | Errno : %d | ErrMsg : %s \n\n", mysql_errno(con),mysql_error(con));
		}
		if( nRetry >= 5)
		{
			req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);
			infLOG(ERROR, "CMD9102 | Cannot DB Connect \n");
			return HEADER_SIZE;
	    }

	    bCloseDB = true;
	    infLOG(ERROR, "DCMD9102 | GetMysqlCon Direct Connect \n");

	}

	#ifdef __DEBUG
	printf(" ] 다운로드 사용자 감소 -- \n");
	#endif
	//--------------------------------------------------------------------------
	// 서버정보 다운로드 사용자 종료처리
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "UPDATE T_SERVER_INFO    "
	                 "   SET dn_user   = if(dn_user<1, 0, dn_user-1)"
	                 " WHERE server_id = '%s'        "
	                 ,pcom9102_r->server_id
	                 );


	#ifdef _DEBUG_
	printf("com9102 ]\n%s\n",szQuery);
	#endif

	if (mysql_query(con, szQuery))
	{
		infLOG(ERROR, "com9102[ERR]: UPDATE T_SERVER_INFO error\n");
		infLOG(ERROR, "com9102[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		if( bCloseDB )
			db_disconnect(con);
		infLOG(ERROR, "com9102[SQL]: %s\n", szQuery);
		req_header.nCmd = -1 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);

       	return HEADER_SIZE;
    }
    //infLOG(ALWAY, "com9102 : %s  \n",szQuery);

	memset (szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "UPDATE T_CONTENTS_FILE_USER_CNT    "
	                 "   SET cur_user_cnt   = if(cur_user_cnt<1, 0, cur_user_cnt-1) "
	                 " WHERE id = %lu  and cont_gu = '%s'   "
	                 ,pcom9102_r->temp_id , pcom9102_r->cont_gu
	                 );

	//infLOG(ALWAY, "com9102 : %s  \n",szQuery);

	if (mysql_query(con, szQuery))
	{
			infLOG(ERROR, "com9102[ERR]: [ %s ]  error\n",szQuery);
			infLOG(ERROR, "com9102[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
    }

	if( bCloseDB )
		db_disconnect(con);

	#ifdef _DEBUG_
	printf("com9102-> end\n");
	#endif

	req_header.nCmd = 9102 ;
	pSendData = new char[HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

	return HEADER_SIZE;
}
