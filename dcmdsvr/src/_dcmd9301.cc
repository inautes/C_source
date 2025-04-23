/******************************************************************************
 *   서브시스템 : daem5003
 *   프로그램명 : dcmd9301.cc
 *         기능 : 컨텐츠 삭제 리스트 요청
 *         설명 : DB에서 삭제처리된 컨텐츠들의 리스트를 컨텐츠 삭제 데몬에 전달
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

#include "com9301.h"

#include "dcmd9301.h"

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
//  COM9301 main
//
//  input : pCom9301_r->proc_flag => 1=wedisk, 2=mydata
//
//  return:  1(정상)
//          -1(DB오류)
//******************************************************************************
long dcmd9301(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData)//CCOM9301_R pCom9301_r)
{

	LPCCOM9301_R pCom9301_r = (LPCCOM9301_R)pRecvData;

	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);

	char szQuery[10000];
	MYSQL       *con=NULL;
	MYSQL_RES *res;
	MYSQL_ROW  row;

	#ifdef _DEBUG_
	printf("com9301-> start-------------------------------\n");
	printf("com9301-> user_id  (%s)\n" , pCom9301_r->user_id  );
	printf("com9301-> conn_ip  (%s)\n" , pCom9301_r->conn_ip  );
	#endif

	// DB 연결
	bool bCloseDB = false;


	CMysqlCon MysqlCon(m_g_clMysqlPool,getpid());

	con = MysqlCon.GetMysqlCon();

	if (con == NULL )
	{
    	infLOG(ERROR, "GetMysqlCon is null \n");
		req_header.nCmd = -1 ;


		int nRetry = 0;
		while (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			#ifdef __DEBUG_
			printf(" ] DB 접속 재시도 \n");
			#endif
		}

		if( nRetry >= 5)
		{

			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);

	       	return HEADER_SIZE;

	    }
	    bCloseDB = true;


	}




	//--------------------------------------------------------------------------
	// 업로드사용자 처리
	//--------------------------------------------------------------------------
	char sztemp [100];      // query temp
	char proc_date [  8+1];	// 처리일자
	char server_id[6+1];

	memset(szQuery, 0x00, sizeof(szQuery));
	memset(proc_date, 0x00, sizeof(proc_date));
	memset(server_id, 0x00, sizeof(server_id));

	strcpy(proc_date, pCom9301_r->szProc_date);
	strcpy(server_id, pCom9301_r->szServer_Id);

	if (strcmp(gproc_date, "00000000") = 0)
	{
		strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d')");

		if (mysql_query(con, szQuery)){
		    ZzLOG(ERROR, "sysdate: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}

		if (!(res = mysql_store_result(con)))
		{
		    ZzLOG(ERROR, "sysdate: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
	 	if (mysql_num_rows(res)==0)
	 	{
		    ZzLOG(ERROR, "sysdate: mysql_num_rows error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			mysql_free_result(res);
			return -1;
		}
		row = mysql_fetch_row(res);
		memset(proc_date, 0x00, sizeof(proc_date));

		strcpy(proc_date,   getstr(row, 0));

	}


	char szQuery[1000];  // query string
	int i, j, count;
	int st_min, ed_min;
	int nRowcnt=0;

    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "gproc_date : [%s]\n", gproc_date);
    ZzLOG(ALWAY, "gserver_id : [%s]\n", gserver_id);
    ZzLOG(ALWAY, "---------------------------------------------------------\n");

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/
	count = 0;
	while(1) {

		memset (szQuery, 0x00, sizeof(szQuery));

		sprintf(szQuery,"SELECT file_path, file_name1, id, file_size, del_date, cont_gu , folder_yn"
                        "   FROM zangsi.T_CONTENTS_DEL									"
                        "  WHERE server_id  = '%s' 									"
                        "    AND del_date  >= '00000000' 								"
                        "    AND del_date  <= '%s' 									"
                        "  LIMIT 100 											"
                        , server_id
                        , proc_date
                        );

		ZzLOG(ERROR, "%s\n\n",szQuery);

		#ifdef __DEBUG
		printf("%s\n\n",szQuery);
		#endif

		if (mysql_query(con, szQuery)){
		    ZzLOG(ERROR, "daem5003_process: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}

		if (!(res = mysql_store_result(con))) {
		    ZzLOG(ERROR, "daem5003_process: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
	 	if (mysql_num_rows(res)==0)	{
			mysql_free_result(res);
		    ZzLOG(ALWAY, "daem5003_process: 처리할 자료가 없습니다.\n");
			break;
		}

	   	ZzLOG(ALWAY, "daem5003_process: select_cnt(%d)\n", mysql_num_rows(res));

		nRowcnt = 0;
		LPDELLLIST pDellList;
		pDellList = new DELLLIST[4096];
		while((row = mysql_fetch_row(res)))
		{
			memset(pDellList[nRowcnt].file_path , 0x00, sizeof(pDellList[nRowcnt].file_path));
			memset(pDellList[nRowcnt].file_name1, 0x00, sizeof(pDellList[nRowcnt].file_name1));
			memset(pDellList[nRowcnt].file_size, 0x00, sizeof(pDellList[nRowcnt].file_size));
			memset(pDellList[nRowcnt].del_date , 0x00, sizeof(pDellList[nRowcnt].del_date ));
			memset(pDellList[nRowcnt].cont_gu  , 0x00, sizeof(pDellList[nRowcnt].cont_gu ));
			memset(pDellList[nRowcnt].folder_yn  , 0x00, sizeof(pDellList[nRowcnt].folder_yn ));

			strcpy(pDellList[nRowcnt].file_path, getstr(row, 0));
			strcpy(pDellList[nRowcnt].file_name1, getstr(row, 1));
			pDellList[nRowcnt].id        = getint(row,2);
			pDellList[nRowcnt].file_size = getnum(row,3);
			strcpy(pDellList[nRowcnt].del_date, getstr(row, 4));
			strcpy(pDellList[nRowcnt].cont_gu , getstr(row, 5));
			strcpy(pDellList[nRowcnt].folder_yn , getstr(row, 6));

			ZzLOG(ERROR, " file_path  =%s file_name = %s \n"
						 " file_date = %s cont_gu = %s \n"
						 " id = %ld\n\n",pDellList[nRowcnt].file_path,pDellList[nRowcnt].file_name1,pDellList[nRowcnt].id,gcont_gu,gid);

			#ifdef __DEBUG
			printf(" gfile_path  =%s gile_name = %s \n"
				   " gfile_date = %s gcont_gu = %s \n"
				   " gid = %ld\n\n",gfile_path,gfile_name,gdel_date,gcont_gu,gid);

			#endif

			if (daem5003_process_db() != 0)
			{
				mysql_free_result(res);
				return -1;
			}
		}
		mysql_free_result(res);
	}


	req_header.nCmd = 9301 ;
	pSendData = new char[HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

	return HEADER_SIZE;

}
