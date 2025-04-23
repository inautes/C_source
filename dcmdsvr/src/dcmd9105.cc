/******************************************************************************
 *   서브시스템 : FUP서버
 *   프로그램명 : com9105.cc
 *         기능 : upload 시 T_CONTENTS_TEMP 에 서버 파일 정보 저장
 *         설명 : wedisk, mydata 업로드 시작시 temp 에 서버쪽 파일 정보 저장
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
#include "com9105.h"
#include "dcmd9105.h"
//#define  _DEBUG_

#include "mysql_pool.h"

extern CMysqlPool * m_g_clMysqlPool;
//******************************************************************************
//  COM9105 main
//
//  input : pcom9105_r->proc_flag => 1=wedisk, 2=mydata
//
//  return:  1(정상)
//          -1(DB오류)
//******************************************************************************
long dcmd9105(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData)//CCOM9105_R pcom9105_r)
{
	LPCCOM9105_R pcom9105_r = (LPCCOM9105_R)pRecvData;

	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);

	char szQuery[10000];
	MYSQL       *con=NULL;
	MYSQL_RES *res;
	MYSQL_ROW  row;

	#ifdef _DEBUG_
	printf(" com9105-> start\n com9105-> temp_id  (%lu)\n com9105-> server_id (%s)"
		   " com9105-> user_id  (%s)\n com9105-> sfile_path(%s)\n com9105-> sfile_name(%s)\n"
	, pcom9105_r->id  , pcom9105_r->server_id , pcom9105_r->user_id , pcom9105_r->sfile_path , pcom9105_r->sfile_name  );
	#endif

	// DB 연결
	bool bCloseDB = false;
	CMysqlCon MysqlCon(m_g_clMysqlPool,m_g_clMysqlPool->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );

	con = MysqlCon.GetMysqlCon();

	if (con == NULL )
	{
		infLOG(ERROR, "DCMD9105 | GetMysqlCon is null \n");
		int nRetry = 0;

		while(!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			infLOG(ERROR, "CMD9105 | Cannot DB Connect - GetMysqlCon is null\n");
			//infLOG(ERROR, "CMD9105 | Mysql Error | Errno : %d | ErrMsg : %s \n\n", mysql_errno(con),mysql_error(con));
		}
		if( nRetry >= 5)
		{
			req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);
			infLOG(ERROR, "CMD9105 | Cannot DB Connect \n");
			return HEADER_SIZE;
	    }
	    bCloseDB = true;
	    infLOG(ERROR, "DCMD9105 | GetMysqlCon Direct Connect \n");
	}


	//--------------------------------------------------------------------------
	// T_CONTENTS_TEMP 에 서버 파일 정보 저장 //path , name
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "UPDATE T_CONTENTS_TEMP set sfile_path = '%s',sfile_name='%s',server_id='%s'"
	                 " WHERE id  = %lu                 "
	                 ,pcom9105_r->sfile_path
	                 ,pcom9105_r->sfile_name
	                 ,pcom9105_r->server_id
	                 ,pcom9105_r->id
	                 );

	#ifdef _DEBUG_
	printf( "dcmd9105 > szQuery : %s\n", szQuery);
	#endif

	if (mysql_query(con, szQuery))
	{
		infLOG(ERROR, "com9105[ERR]: UPDATE T_CONTENTS_TEMP set error\n");
		infLOG(ERROR, "com9105[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		infLOG(ERROR, "com9105[SQL]: %s\n", szQuery);

		if( bCloseDB )
			db_disconnect(con);

		req_header.nCmd = -1 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);
		return HEADER_SIZE;
    }


	if( bCloseDB )
		db_disconnect(con);

	#ifdef _DEBUG_
	printf("com9105-> end\n");
	#endif

	req_header.nCmd = 9105 ;
	pSendData = new char[HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

	return HEADER_SIZE;
}
