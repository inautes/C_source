/******************************************************************************
 *   서브시스템 : FDN
 *   프로그램명 : dcmd9009.cc
 *         기능 :
 *         설명 : 컨텐츠 서버 다운로드 카운트
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
#include "com9009.h"
#include "dcmd9009.h"
//#define  _DEBUG
//******************************************************************************
//  dcmd9009 main
//
//  return:  1(정상)
//          -1(DB오류)
//******************************************************************************
#include "mysql_pool.h"
#include "MysqlDB.h"
extern CMysqlPool * m_g_clMysqlPool;

long dcmd9009(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData)
{
	LPCCOM9009_R pcom9009_r = (LPCCOM9009_R)pRecvData;
	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);

	infLOG(ALWAY, "dcmd9009 MODE [ %d ] : %s : COUNT [%d] \n",pcom9009_r->nMode,	pcom9009_r->server_id,pcom9009_r->nConnectCnt);

	if(pcom9009_r->uDisksize != 0)
		infLOG(ALWAY, "uDisksize = %lu , uUsesize = %lu ,  uFreesize = %lu \n",pcom9009_r->uDisksize,	pcom9009_r->uUsesize ,pcom9009_r->uFreesize);

	CMysqlDB main_db;
	MYSQL       *con=NULL;
	main_db.setDebug(false);
	CMysqlCon MysqlCon;
	MysqlCon.ConnectPool(m_g_clMysqlPool,m_g_clMysqlPool->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );
	main_db.setCon(MysqlCon.GetMysqlCon());


	if ( main_db.getCon() == NULL )
	{
		infLOG(ERROR, "dcmd9009: GetMysqlCon is null\n");
		req_header.nCmd = -1 ;

		int nRetry = 0;
		while (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 ){
			nRetry++;sleep(1);
		}

		if( nRetry >= 5){
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);
			return HEADER_SIZE;
	    }

	    main_db.setCon(con);
	    main_db.setUseDisconnect(true);
	}

	int ret = 0;
	//--------------------------------------------------------------------------
	// 서버정보 다운로드 사용자 증가
	//--------------------------------------------------------------------------
	ret = main_db.num_rows( "SELECT * FROM  zangsi.T_CONTENTS_SERVER_CONNECT_CNT a where a.server_id " , pcom9009_r->server_id);
	main_db.free_result();

	if ( ret = 0 )
	{
		if( pcom9009_r->nMode == 0)
		{
			ret = main_db.query( "INSERT INTO zangsi.T_CONTENTS_SERVER_CONNECT_CNT (server_id,dn_user,udt_date,udt_time) VALUES ('%s', %d, DATE_FORMAT(NOW(),'%%Y%%m%%d'), DATE_FORMAT(NOW(),'%%H%%i%%s')) "
							 ,pcom9009_r->server_id ,pcom9009_r->nConnectCnt);
		}else if( pcom9009_r->nMode == 1)
		{
			ret = main_db.query( "INSERT INTO zangsi.T_CONTENTS_SERVER_CONNECT_CNT (server_id,up_user,udt_date,udt_time) VALUES ('%s', %d, DATE_FORMAT(NOW(),'%%Y%%m%%d'), DATE_FORMAT(NOW(),'%%H%%i%%s')) "
							 ,pcom9009_r->server_id ,pcom9009_r->nConnectCnt);
		}
	}
	else
	{
		if( pcom9009_r->nMode == 0)
		{
			ret = main_db.query( "UPDATE zangsi.T_CONTENTS_SERVER_CONNECT_CNT SET dn_user = %d , udt_date = DATE_FORMAT(NOW(),'%%Y%%m%%d'),udt_time = DATE_FORMAT(NOW(),'%%H%%i%%s') where server_id = '%s' "
							 ,pcom9009_r->nConnectCnt, pcom9009_r->server_id);
		}else if( pcom9009_r->nMode == 1)
		{
			ret = main_db.query( "UPDATE zangsi.T_CONTENTS_SERVER_CONNECT_CNT SET up_user = %d , udt_date = DATE_FORMAT(NOW(),'%%Y%%m%%d'),udt_time = DATE_FORMAT(NOW(),'%%H%%i%%s') where server_id = '%s' "
							 ,pcom9009_r->nConnectCnt, pcom9009_r->server_id);
		}
	}

	if( ret != 0 )
	{

		infLOG(ERROR, "dcmd9009 : query [ %s ] \nerror msg : [ %s ]\n",main_db.getQueryString(),main_db.getErrorMessage());
		main_db.disconnect_db();

		req_header.nCmd = -1 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);

		return HEADER_SIZE;
	}
	main_db.disconnect_db();

	req_header.nCmd = 9009 ;
	pSendData = new char[HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

	return HEADER_SIZE;
}
