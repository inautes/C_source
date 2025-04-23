	/******************************************************************************
 *   서브시스템 : CMD서버
 *   프로그램명 : fdns3004.cc
 *         기능 : 무료자료실 다운로드
 *         설명 : 무료자료실 파일정보조회, 무료자료실 다운로드 검수증가
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

#include "comcomm.h"
#include "comconf.h"
#include "commydb.h"
#include "apdefine.h"
//#include "/home/ezwon/zangsi_with_dcmd/fdnsvr/inc/fdns3004.h"
#include "../../fdnsvr/inc/fdns3004.h"
#include "dcmdfdns3004.h"

#define  MAX_ROWS	1
//#define  _DEBUG_
// db 제어 서버
#include <unistd.h>     /* for close() getpid()*/
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */


#include "mysql_pool.h"

extern CMysqlPool * m_g_clMysqlPool;

// db 제어 서버
//******************************************************************************
//* fdns3004 main
//* return: 성공:0   오류:-1 (pErrMsg에 오류메시지를 리턴한다.)
//*         1. input   id
//*         2. output  CFDNS3004_R
//*         3. output  pErrMsg
//******************************************************************************
int dcmdfdns3004(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData)//unsigned long ul_id, CFDNS3004_R *fdns3004s, char* pErrMsg)
{
	unsigned long ul_id;
	memcpy(&ul_id,pRecvData,sizeof(unsigned long));


	CFDNS3004_R fdns3004s;
	memset(&fdns3004s,0x00,sizeof(CFDNS3004_R));


	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);

	ERR_HEADER err_req_header;
	memcpy(&err_req_header , pHeader,ERR_HEADER_SIZE);


	MYSQL       *con=NULL;
	MYSQL_RES   *res;
	MYSQL_ROW    row;

	//CFDNS3004_S	*fdns3004s;

	char szQuery[1000];  // query string
	char sztemp [1000];  // query string
	char ErrMsg[256];    // error message
	int  ErrNum;         // error no
	/*--------------------------------------------------------------------*/
	/* start!!!                                                           */
	/*--------------------------------------------------------------------*/
	#ifdef _DEBUG_
	printf("fdns3004-> start...\n");
	#endif

	// DB 연결
	bool bCloseDB = false;

	CMysqlCon MysqlCon(m_g_clMysqlPool,m_g_clMysqlPool->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );

	con = MysqlCon.GetMysqlCon();

	if (con == NULL )
	{

		ErrNum = -888888;
		sprintf(ErrMsg, "DB에 접속하지 못 하였습니다...\n");


		strcpyA(err_req_header.errmsg,"3004 : DB에 접속하지 못 하였습니다.\n");

		infLOG(ERROR, "DCMD3004 | GetMysqlCon is null \n");
		int nRetry = 0;
		while (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			#ifdef __DEBUG_
			printf(" ] DB 접속 재시도 \n");
			#endif
			infLOG(ERROR, "DCMD3004 | GetMysqlCon is null [%d](%s)\n",ErrNum, ErrMsg);
		}

		if( nRetry >= 5)
		{

			err_req_header.header.nCmd = -1 ;
			pSendData = new char [ERR_HEADER_SIZE];
			memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

			infLOG(ERROR, "DCMD3004 | Cannot DB Connect \n");

	       	return ERR_HEADER_SIZE;
	    }
	    bCloseDB = true;

	    infLOG(ERROR,"DCMD3004 | Connect DB direct\n");



	}
	//--------------------------------------------------------------------------
	// 무료자료실 파일정보조회
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "SELECT id         , folder_yn  , file_type  "
	                 "     , server_id  , file_spath , file_sname "
	                 "     , file_lname , file_size               "
	                 "  FROM zangsi.T_BOARD_DATA_F   "
	                 " WHERE id  =  %lu               "
	                 , ul_id
	                 );
	if (mysql_query(con, szQuery)){
		ErrNum = -300401;
		sprintf(ErrMsg, "검색시 오류가 발생하였습니다.\n");

	    infLOG(ERROR, "fdns3004-> [%d](%s)\n",ErrNum, ErrMsg);
		infLOG(ERROR, "fdns3004-> [%d](%s)\n",mysql_errno(con), mysql_error(con));
		#ifdef _DEBUG_
	    printf("fdns3004-> [%d](%s)\n",ErrNum, ErrMsg);
		printf("fdns3004-> [%d](%s)\n",mysql_errno(con), mysql_error(con));
		#endif

		if( bCloseDB )
			db_disconnect(con);

		strcpyA(err_req_header.errmsg,ErrMsg);
		err_req_header.header.nCmd = ErrNum ;

		pSendData = new char [ERR_HEADER_SIZE];
		memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

	   	return ERR_HEADER_SIZE;

	}
	if (!(res = mysql_store_result(con)))
	{
		ErrNum = -300402;
		sprintf(ErrMsg, "검색시 오류가 발생하였습니다.\n");

	    infLOG(ERROR, "fdns3004-> [%d](%s)\n",ErrNum, ErrMsg);
		infLOG(ERROR, "fdns3004-> [%d](%s)\n",mysql_errno(con), mysql_error(con));
		#ifdef _DEBUG_
	    printf("fdns3004-> [%d](%s)\n",ErrNum, ErrMsg);
		printf("fdns3004-> [%d](%s)\n",mysql_errno(con), mysql_error(con));
		#endif

		if( bCloseDB )
			db_disconnect(con);

		strcpyA(err_req_header.errmsg,ErrMsg);
		err_req_header.header.nCmd = ErrNum ;

		pSendData = new char [ERR_HEADER_SIZE];
		memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

	   	return ERR_HEADER_SIZE;

	}
 	if (mysql_num_rows(res)==0)
 	{
		ErrNum = -300403;
		sprintf(ErrMsg, "다운로드할 자료가 없습니다.\n");

		mysql_free_result(res);

		infLOG(ERROR, "fdns3004-> [%d](%s)\n",ErrNum, ErrMsg);
		infLOG(ERROR, "fdns3004-> [%d](%s)\n",mysql_errno(con), mysql_error(con));
		#ifdef _DEBUG_
	    printf("fdns3004-> [%d](%s)\n",ErrNum, ErrMsg);
		printf("fdns3004-> [%d](%s)\n",mysql_errno(con), mysql_error(con));
		#endif

		if( bCloseDB )
			db_disconnect(con);

		strcpyA(err_req_header.errmsg,ErrMsg);
		err_req_header.header.nCmd = ErrNum ;

		pSendData = new char [ERR_HEADER_SIZE];
		memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

	   	return ERR_HEADER_SIZE;
	}
	row = mysql_fetch_row(res);


	fdns3004s.id  = (unsigned long)getnum(row, 0);

	strcpyA(fdns3004s.folder_yn , getstr(row, 1));
	strcpyA(fdns3004s.file_type , getstr(row, 2));
	strcpyA(fdns3004s.server_id , getstr(row, 3));
	strcpyA(fdns3004s.sfile_path, getstr(row, 4));
	strcpyA(fdns3004s.sfile_name, getstr(row, 5));
	strcpyA(fdns3004s.lfile_name, getstr(row, 6));

	fdns3004s.file_size = getnum(row, 7);

	mysql_free_result(res);

	//--------------------------------------------------------------------------
	// 무료자료실 다운로드 검수증가
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery ));
	sprintf(szQuery, "UPDATE zangsi.T_BOARD_DATA_F   "
	                 "   SET down_cnt = down_cnt +1  "
	                 " WHERE id       = %d           "
	                 ,ul_id
	                 );
	#ifdef __DEBUG
	printf("%s\n",szQuery);
	#endif
	if (mysql_query(con, szQuery)){
		ErrNum = -300405;
		sprintf(ErrMsg, "데이타 UPDATE하지 못했습니다.\n");

		infLOG(ERROR, "fdns3004-> [%d](%s)\n",ErrNum, ErrMsg);
		infLOG(ERROR, "fdns3004-> [%d](%s)\n",mysql_errno(con), mysql_error(con));
		#ifdef _DEBUG_
	    printf("fdns3004-> [%d](%s)\n",ErrNum, ErrMsg);
		printf("fdns3004-> [%d](%s)\n",mysql_errno(con), mysql_error(con));
		#endif

		//tran_rollback(con);
		if( bCloseDB )
			db_disconnect(con);


		strcpyA(err_req_header.errmsg,ErrMsg);
		err_req_header.header.nCmd = ErrNum ;

		pSendData = new char [ERR_HEADER_SIZE];
		memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

	   	return ERR_HEADER_SIZE;
    }

	if( bCloseDB )
		db_disconnect(con);
	//strcpyA(pDnDate, st_date);
	#ifdef _DEBUG_
	printf("fdns3004->seq_no =(%d)\n", ul_id);
	#endif


	req_header.nCmd = 3004;
	req_header.nDataCnt = 1;
	req_header.nDataSize = sizeof(CFDNS3004_R);

	unsigned long dwSendLen = HEADER_SIZE + req_header.nDataCnt * req_header.nDataSize ;


	pSendData = new char[dwSendLen];

	memcpy( pSendData , &req_header,HEADER_SIZE);
	if( req_header.nDataCnt * req_header.nDataSize > 0 )
		memcpy( pSendData + HEADER_SIZE,(char*)&fdns3004s , req_header.nDataCnt * req_header.nDataSize);


	return dwSendLen;



}
