/******************************************************************************
 *   서브시스템 : FUP서버
 *   프로그램명 : com9104.cc
 *         기능 : upload 오류처리
 *         설명 : wedisk, mydata에 upload시 오류발생하면 호출된다. 업로드 사이즈 줄이기
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
#include "com9104.h"
#include "dcmd9104.h"
//#define  _DEBUG_

#include "mysql_pool.h"

extern CMysqlPool * m_g_clMysqlPool;

//******************************************************************************
//  COM9104 main
//
//  input : pcom9104_r->proc_flag => 1=wedisk, 2=mydata
//
//  return:  1(정상)
//          -1(DB오류)
//******************************************************************************
long dcmd9104(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData)//CCOM9104_R pcom9104_r)
{
	LPCCOM9104_R pcom9104_r = (LPCCOM9104_R)pRecvData;

	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);


	char szQuery[10000];
	MYSQL       *con =NULL;
	MYSQL_RES *res;
	MYSQL_ROW  row;

	#ifdef _DEBUG_
	printf("com9104-> start\n");
	printf("com9104-> proc_flag(%d)\n" , pcom9104_r->proc_flag);
	printf("com9104-> temp_id  (%lu)\n", pcom9104_r->id       );
	printf("com9104-> user_id  (%s)\n" , pcom9104_r->user_id  );
	printf("com9104-> file_size(%15.0f)\n" , pcom9104_r->file_size  );
	#endif
/*	infLOG(ALWAY, "com9104 : start\n"
		   		  "com9104 : proc_flag(%d)\n"
				  "com9104 : temp_id  (%lu)\n"
                  "com9104 : user_id  (%s)\n"
                  "com9104 : file_size(%15.0f)\n" ,
                  pcom9104_r->proc_flag,
                  pcom9104_r->id       ,
                  pcom9104_r->user_id  ,
                  pcom9104_r->file_size  );
*/	// DB 연결

	bool bCloseDB = false;

	CMysqlCon MysqlCon(m_g_clMysqlPool,m_g_clMysqlPool->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );

	con = MysqlCon.GetMysqlCon();


	if (con == NULL )
	{
		infLOG(ERROR, "DCMD9104 | GetMysqlCon is null \n");




		int nRetry = 0;

		while (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			infLOG(ERROR, "CMD9104 | Cannot DB Connect - GetMysqlCon is null\n");
			//infLOG(ERROR, "CMD9104 | Mysql Error | Errno : %d | ErrMsg : %s \n\n", mysql_errno(con),mysql_error(con));
		}
		if( nRetry >= 5)
		{
			req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);
			infLOG(ERROR, "CMD9104 | Cannot DB Connect \n");
			return HEADER_SIZE;
	    }


	    bCloseDB = true;
	    infLOG(ERROR, "DCMD9104 | GetMysqlCon Direct Connect \n");

	}

	//--------------------------------------------------------------------------
	// T_CONTENTS_TEMP 삭제
	//--------------------------------------------------------------------------
/*
	memset (szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "DELETE FROM T_CONTENTS_TEMP "
	                 " WHERE id  = '%lu'                 "
	                 ,pcom9104_r->id
	                 );

	#ifdef _DEBUG_
	printf( "DELETE FROM T_CONTENTS_TEMP \n"
			" WHERE id  =  %lu                  \n"
	        ,pcom9104_r->id
			);
	#endif

	if (mysql_query(con, szQuery)){
		infLOG(ERROR, "com9104[ERR]: DELETE FROM T_CONTENTS_TEMP error\n");
		infLOG(ERROR, "com9104[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		if( bCloseDB )
			db_disconnect(con);

		return -1;
    }
*/
	//--------------------------------------------------------------------------
	// 내자료실인경우 upload_size clear한다.
	//--------------------------------------------------------------------------
	if ((pcom9104_r->proc_flag == 2) || (pcom9104_r->proc_flag == 3))
	{
		memset (szQuery , 0x00, sizeof(szQuery ));
		sprintf(szQuery, "UPDATE zangsi.T_MYDATA_INFO "
		                 "   SET upload_size = upload_size - %15.0f "
		                 " WHERE user_id  = '%s'      "
		                 ,pcom9104_r->file_size
		                 ,pcom9104_r->user_id
		                 );

		#ifdef _DEBUG_
		printf( "UPDATE zangsi.T_MYDATA_INFO \n"
		        "   SET upload_size = upload_size - %15.0f\n"
				" WHERE user_id  = '%s'      \n"
		        ,pcom9104_r->file_size
				,pcom9104_r->user_id
				);
		#endif

		if (mysql_query(con, szQuery))
		{
			infLOG(ERROR, "com9104[ERR]: UPDATE T_MYDATA_INFO error\n");
			infLOG(ERROR, "com9104[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			if( bCloseDB )
				db_disconnect(con);

			infLOG(ERROR, "com9104[SQL]: %s\n", szQuery);


			req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);

	       	return HEADER_SIZE;
	    }
	}
	else if( pcom9104_r->proc_flag == 4 )
	{
		memset (szQuery , 0x00, sizeof(szQuery ));
		sprintf(szQuery, "UPDATE zangsi.T_PERM_UPLOAD_AUTH "
		                 "   SET upload_size = upload_size - %15.0f "
		                 " WHERE user_id  = '%s'      "
		                 ,pcom9104_r->file_size
		                 ,pcom9104_r->user_id
		                 );

		#ifdef _DEBUG_
		printf( "UPDATE zangsi.T_PERM_UPLOAD_AUTH \n"
		        "   SET upload_size = upload_size - %15.0f\n"
				" WHERE user_id  = '%s'      \n"
		        ,pcom9104_r->file_size
				,pcom9104_r->user_id
				);
		#endif

		if (mysql_query(con, szQuery))
		{
			infLOG(ERROR, "com9104[ERR]: UPDATE T_MYDATA_INFO error\n");
			infLOG(ERROR, "com9104[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			if( bCloseDB )
				db_disconnect(con);

			infLOG(ERROR, "com9104[SQL]: %s\n", szQuery);


			req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);

	       	return HEADER_SIZE;
	    }
	}

	if( bCloseDB )
		db_disconnect(con);

	#ifdef _DEBUG_
	printf("com9104-> end\n");
	#endif
/*	infLOG(ALWAY, "com9104 : end\n"
		   		  "com9104 : proc_flag(%d)\n"
				  "com9104 : temp_id  (%lu)\n"
                  "com9104 : user_id  (%s)\n"
                  "com9104 : file_size(%15.0f)\n" ,
                  pcom9104_r->proc_flag,
                  pcom9104_r->id       ,
                  pcom9104_r->user_id  ,
                  pcom9104_r->file_size  );
*/
	req_header.nCmd = 9104 ;
	pSendData = new char[HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

	return HEADER_SIZE;
}
