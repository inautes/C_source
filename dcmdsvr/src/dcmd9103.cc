/******************************************************************************
 *   서브시스템 : FUP서버
 *   프로그램명 : com9103.cc
 *         기능 : 내자료실 사용량UPDATE
 *         설명 : 내자료실에 파일upload, 복사, 삭제시 디스크사용량을 UPDATE한다
 *     수정이력 :
 *     수정내용 :
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
#include "com9103.h"
#include "dcmd9103.h"
//#define  _DEBUG_

// db 제어 서버
#include <unistd.h>     /* for close() getpid()*/
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include "dcmdsock.h" //sock send recv

#include "mysql_pool.h"
extern CMysqlPool * m_g_clMysqlPool;


// db 제어 서버

//******************************************************************************
//* COM9103 main
//* error 발생시 pSendData를 사용하고 정상인경우 seq_no를 return한다.
//  return:  1(정상)
//          -1(DB오류)
//          -2(minus처리시 사이즈가 너무작음)
//          -3(남아있는 용량보다 사이즈가 큼)
//******************************************************************************
long dcmd9103(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData)//CCOM9103_R pcom9103_r,char* pErrMsg)
{

	LPCCOM9103_R pcom9103_r = (LPCCOM9103_R)pRecvData;

	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);

	ERR_HEADER err_req_header;
	memset(&err_req_header , 0x00,ERR_HEADER_SIZE);

	char szQuery[10000];
	MYSQL       *con=NULL;

	MYSQL_RES *res;
	MYSQL_ROW  row;
	double  disk_use                ;  // 파일크기
	double  disk_rem                ;  // 디스크남은용량

	#ifdef _DEBUG_
	printf("com9103-> start\n");
	printf("com9103-> user_id  (%s)\n"    , pcom9103_r->user_id   );
	printf("com9103-> file_size(%13.0f)\n", pcom9103_r->file_size );
	#endif

	// DB 연결
	bool bCloseDB = false;

	CMysqlCon MysqlCon(m_g_clMysqlPool,m_g_clMysqlPool->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );

	con = MysqlCon.GetMysqlCon();

	if (con == NULL )
	{
		infLOG(ERROR, "DCMD9103 | GetMysqlCon is null \n");

		int nRetry = 0;
		while (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			infLOG(ERROR, "DCMD9103 | Cannot DB Connect - GetMysqlCon is null\n");
			//infLOG(ERROR, "DCMD9103 | Mysql Error | Errno : %d | ErrMsg : %s \n\n", mysql_errno(con),mysql_error(con));
		}
		if( nRetry >= 5)
		{
			strcpyA(err_req_header.errmsg,"COM9103 : DB에 접속하지 못 하였습니다.\n");
			err_req_header.header.nCmd = -1;

			infLOG(ERROR, "DCMD9103 | Cannot DB Connect \n");

			pSendData = new char [ERR_HEADER_SIZE];
			memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);
	       	return ERR_HEADER_SIZE;
	    }

	    bCloseDB = true;
	    infLOG(ERROR, "DCMD9103 | GetMysqlCon Direct Connect \n");
	}

	//1=wedisk, 2=mydisk, 3=mydata 4=filog disk
	if ((pcom9103_r->proc_flag == 2) || (pcom9103_r->proc_flag == 3))
	{
		//--------------------------------------------------------------------------
		// 서버정보용량 UPDATE
		//--------------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery ));
		sprintf(szQuery, "SELECT disk_size - disk_use - upload_size "
		                 "  FROM zangsi.T_MYDATA_INFO    "
		                 " WHERE user_id = '%s'         "
		                 ,pcom9103_r->user_id
		                 );
		if (mysql_query(con, szQuery))
		{
			infLOG(ERROR, "com9103[ERR]: 내자료실 검색시 오류가 발생했습니다.\n");
			infLOG(ERROR, "com9103[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			if( bCloseDB )
			{
				db_disconnect(con);
			}

			infLOG(ERROR, "com9103[SQL]: %s\n", szQuery);

			strcpyA(err_req_header.errmsg,"COM9103 : 내자료실 용량 검색시 오류가 발생했습니다.\n");

			err_req_header.header.nCmd = -1 ;
			pSendData = new char [ERR_HEADER_SIZE];
			memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

	       	return ERR_HEADER_SIZE;

		}

		if (!(res = mysql_store_result(con)))
		{
			infLOG(ERROR, "com9103[ERR]: 내자료실 용량 검색시 오류가 발생했습니다.\n");
			infLOG(ERROR, "com9103[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			if( bCloseDB )
			{
				db_disconnect(con);
			}
			infLOG(ERROR, "com9103[SQL]: %s\n", szQuery);

			strcpyA(err_req_header.errmsg,"COM9103 : 내자료실 용량 검색시 오류가 발생했습니다.\n");

			err_req_header.header.nCmd = -1 ;
			pSendData = new char [ERR_HEADER_SIZE];
			memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

	       	return ERR_HEADER_SIZE;
		}

	 	if (mysql_num_rows(res)==0)
	 	{
			infLOG(ERROR, "com9103[ERR]: 내자료실 용량 검색시 오류가 발생하였습니다.\n");
			infLOG(ERROR, "com9103[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));

			mysql_free_result(res);

			if( bCloseDB )
			{
				db_disconnect(con);
			}
			infLOG(ERROR, "com9103[SQL]: %s\n", szQuery);
			strcpyA(err_req_header.errmsg,"COM9103 : 내자료실 용량 검색시 오류가 발생했습니다.\n");

			err_req_header.header.nCmd = -1 ;
			pSendData = new char [ERR_HEADER_SIZE];
			memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

	       	return ERR_HEADER_SIZE;
		}

		memset(&disk_rem, 0x00, sizeof(disk_rem));
		if (row = mysql_fetch_row(res))
		{
			disk_rem = getnum(row, 0);
		}
		mysql_free_result(res);

		if (pcom9103_r->file_size > disk_rem)
		{
			infLOG(ERROR, "com9103[ERR]: 내자료실 용량부족 에러 : user_id(%s) upload_size(%15.0f) > disk_rem(%15.0f)\n",  pcom9103_r->user_id, pcom9103_r->file_size, disk_rem);
			infLOG(ERROR, "com9103[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));

			if( bCloseDB )
			{
				db_disconnect(con);
			}
			infLOG(ERROR, "com9103[SQL]: %s\n", szQuery);

			strcpyA(err_req_header.errmsg,"내자료실 공간이 부족합니다.\n");

			err_req_header.header.nCmd = -3 ;
			pSendData = new char [ERR_HEADER_SIZE];
			memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

	       	return ERR_HEADER_SIZE;

		}
		//--------------------------------------------------------------------------
		// 서버정보용량 UPDATE
		//--------------------------------------------------------------------------
		memset (szQuery , 0x00, sizeof(szQuery ));
		sprintf(szQuery, "UPDATE zangsi.T_MYDATA_INFO  "
		                 "   SET upload_size = upload_size + %15.0f "
		                 " WHERE user_id  = '%s'       "
		                 ,pcom9103_r->file_size
		                 ,pcom9103_r->user_id
		                 );

		#ifdef _DEBUG_
		printf( "dcmd9103 > szQuery : %s\n", szQuery);
		#endif

		if (mysql_query(con, szQuery))
		{
			infLOG(ERROR, "com9103[ERR]: UPDATE zangsi.T_MYDATA_INFO (%s)\n", mysql_error(con));
			infLOG(ERROR, "com9103[SQL]: %s\n", szQuery);

			if( bCloseDB )
				db_disconnect(con);

			infLOG(ERROR, "com9103[SQL]: %s\n", szQuery);
			strcpyA(err_req_header.errmsg,"COM9103 : 내자료실 용량 DB 오류입니다.\n");

			err_req_header.header.nCmd = -1 ;
			pSendData = new char [ERR_HEADER_SIZE];
			memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

	       	return ERR_HEADER_SIZE;
	    }
	}
	else if( pcom9103_r->proc_flag == 4 ) // filog
	{
		//20100218 -- HCS : 필로그 용량 제한 없앰.
		/*
		//--------------------------------------------------------------------------
		// 서버정보용량 UPDATE
		//--------------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery ));
		sprintf(szQuery, "SELECT disk_size - disk_use - upload_size "
		                 "  FROM zangsi.T_PERM_UPLOAD_AUTH    "
		                 " WHERE user_id = '%s'         "
		                 ,pcom9103_r->user_id
		                 );
		if (mysql_query(con, szQuery))
		{
			infLOG(ERROR, "com9103[ERR]: 필로그 자료실 검색시 오류가 발생했습니다.\n");
			infLOG(ERROR, "com9103[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			if( bCloseDB )
				db_disconnect(con);

			infLOG(ERROR, "com9103[SQL]: %s\n", szQuery);

			strcpyA(err_req_header.errmsg,"COM9103 : 필로그 자료실 용량 검색시 오류가 발생했습니다.\n");

			err_req_header.header.nCmd = -1 ;
			pSendData = new char [ERR_HEADER_SIZE];
			memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

	       	return ERR_HEADER_SIZE;
		}

		if (!(res = mysql_store_result(con)))
		{
			infLOG(ERROR, "com9103[ERR]: 필로그 자료실 용량 검색시 오류가 발생했습니다.\n");
			infLOG(ERROR, "com9103[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			if( bCloseDB )
				db_disconnect(con);

			infLOG(ERROR, "com9103[SQL]: %s\n", szQuery);

			strcpyA(err_req_header.errmsg,"COM9103 : 필로그 자료실 용량 검색시 오류가 발생했습니다.\n");

			err_req_header.header.nCmd = -1 ;
			pSendData = new char [ERR_HEADER_SIZE];
			memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

	       	return ERR_HEADER_SIZE;
		}

	 	if (mysql_num_rows(res)==0)
	 	{

			infLOG(ERROR, "com9103[ERR]: 필로그 자료실 용량 검색시 오류가 발생하였습니다.\n");
			infLOG(ERROR, "com9103[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));

			mysql_free_result(res);

			if( bCloseDB )
				db_disconnect(con);

			infLOG(ERROR, "com9103[SQL]: %s\n", szQuery);
			strcpyA(err_req_header.errmsg,"COM9103 : 필로그 자료실 용량 검색시 오류가 발생했습니다.\n");

			err_req_header.header.nCmd = -1 ;
			pSendData = new char [ERR_HEADER_SIZE];
			memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

	       	return ERR_HEADER_SIZE;
		}

		memset(&disk_rem, 0x00, sizeof(disk_rem));

		if (row = mysql_fetch_row(res))
		{
			disk_rem = getnum(row, 0);
		}
		mysql_free_result(res);

		if (pcom9103_r->file_size > disk_rem)
		{
			infLOG(ERROR, "com9103[ERR]: 필로그 자료실 용량부족 에러 : user_id(%s) upload_size(%15.0f) > disk_rem(%15.0f)\n",  pcom9103_r->user_id, pcom9103_r->file_size, disk_rem);
			infLOG(ERROR, "com9103[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));

			if( bCloseDB )
				db_disconnect(con);

			infLOG(ERROR, "com9103[SQL]: %s\n", szQuery);

			strcpyA(err_req_header.errmsg,"필로그 자료실 공간이 부족합니다.\n");

			err_req_header.header.nCmd = -3 ;
			pSendData = new char [ERR_HEADER_SIZE];
			memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

	       	return ERR_HEADER_SIZE;
		}
		//--------------------------------------------------------------------------
		// 서버정보용량 UPDATE
		//--------------------------------------------------------------------------
		memset (szQuery , 0x00, sizeof(szQuery ));
		sprintf(szQuery, "UPDATE zangsi.T_PERM_UPLOAD_AUTH  "
		                 "   SET upload_size = upload_size + %15.0f "
		                 " WHERE user_id  = '%s'       "
		                 ,pcom9103_r->file_size
		                 ,pcom9103_r->user_id
		                 );

		#ifdef _DEBUG_
		printf( "dcmd9103 > szQuery : %s\n", szQuery);
		#endif

		if (mysql_query(con, szQuery))
		{
			infLOG(ERROR, "com9103[ERR]: UPDATE zangsi.T_PERM_UPLOAD_AUTH (%s)\n", mysql_error(con));
			infLOG(ERROR, "com9103[SQL]: %s\n", szQuery);
			if( bCloseDB )
				db_disconnect(con);

			infLOG(ERROR, "com9103[SQL]: %s\n", szQuery);
			strcpyA(err_req_header.errmsg,"COM9103 : 내자료실 용량 DB 오류입니다.\n");

			err_req_header.header.nCmd = -1 ;
			pSendData = new char [ERR_HEADER_SIZE];
			memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

	       	return ERR_HEADER_SIZE;
	    }
	    */
	}
	if( bCloseDB )
		db_disconnect(con);

	req_header.nCmd = 9103 ;
	pSendData = new char[HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

	#ifdef _DEBUG_
	printf("com9103-> end\n");
	#endif
	return HEADER_SIZE;
}

