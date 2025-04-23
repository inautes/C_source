/******************************************************************************
 *   서브시스템 : CMD서버
 *   프로그램명 : fups4002.cc
 *         기능 : 무료자료실 등록
 *         설명 :
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
#include <unistd.h>
#include "comcomm.h"
#include "comconf.h"
#include "commydb.h"
#include "apdefine.h"
#include "../../fupsvr/inc/fups4002.h"
#include "dcmdfups4002.h"
//#define  _DEBUG_
#include "mysql_pool.h"

extern CMysqlPool * m_g_clMysqlPool;
/******************************************************************************
** nServerFlag -> 1 : We디스크
**                2 : 내디스크
*******************************************************************************/

long dcmdfups4002(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData) //CFUPS4002 pfups4002,char* pErrMsg)
{


	LPCFUPS4002 pfups4002 = (LPCFUPS4002)pRecvData;

	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);

	ERR_HEADER err_req_header;
	memset(&err_req_header , 0x00,ERR_HEADER_SIZE);




	char szQuery[10000], szQuery1[10000];
	MYSQL *con = NULL;
	MYSQL_RES   *res;
	MYSQL_ROW    row;
	char ErrMsg[256];    // error message
	memset(ErrMsg,0x00,sizeof(ErrMsg));

	int  ErrNum;         // error no
	unsigned long dwId=0;

	memset(&ErrMsg   , 0x00, sizeof(ErrMsg   )); // 오류메시지
/*
	infLOG(ALWAY,"fups4002-> fups4002 start\n");
	infLOG(ALWAY,"fups4002-> id        (%d)\n", pfups4002->id           );
	infLOG(ALWAY,"fups4002-> folder_yn (%s)\n", pfups4002->folder_yn    );
	infLOG(ALWAY,"fups4002-> server_id (%s)\n", pfups4002->server_id    );
	infLOG(ALWAY,"fups4002-> file_path2(%s)\n", pfups4002->sfile_path   );
	infLOG(ALWAY,"fups4002-> file_name1(%s)\n", pfups4002->sfile_name   );
	infLOG(ALWAY,"fups4002-> file_name2(%s)\n", pfups4002->lfile_name   );
	infLOG(ALWAY,"fups4002-> file_type (%s)\n", pfups4002->file_type    );
	infLOG(ALWAY,"fups4002-> file_size (%13.0f)\n", pfups4002->file_size);
	infLOG(ALWAY,"fups4002-> total_file_size (%13.0f)\n", pfups4002->total_file_size);
*/
	if (pfups4002->id == 0)
	{
		sprintf(ErrMsg, "등록번호가 없습니다.\n");

	    infLOG(ERROR, "fups4002-> [%d](%s)\n",ErrNum, ErrMsg);

		strcpyA(err_req_header.errmsg,ErrMsg);

		err_req_header.header.nCmd = -1 ;
		pSendData = new char [ERR_HEADER_SIZE];
		memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

       	return ERR_HEADER_SIZE;
	}

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	bool bCloseDB = false;

	CMysqlCon MysqlCon(m_g_clMysqlPool,m_g_clMysqlPool->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );

	con = MysqlCon.GetMysqlCon();


	if (con == NULL )
	{
		ErrNum = -400292;
		sprintf(ErrMsg, "FUPS4002 | DB에 접속하지 못 하였습니다.\n");

		infLOG(ERROR, "FUPS4002 | GetMysqlCon is null \n");
		int nRetry = 0;
		while (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			infLOG(ERROR, "FUPS4002 | GetMysqlCon is null [%d](%s)\n",ErrNum, ErrMsg);
			//infLOG(ERROR, "FUPS4002 | Mysql Error | Errno : %d | ErrMsg : %s \n\n", mysql_errno(con),mysql_error(con));
		}

		if( nRetry >= 5)
		{

			req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);

			infLOG(ERROR, "FUPS4002 | Cannot DB Connect \n");

	       	return HEADER_SIZE;
	    }
		bCloseDB = true;
		infLOG(ERROR,"FUPS4002 | Connect DB direct\n");
	}

	//--------------------------------------------------------------------------
	// 트렌젝션시작
	//--------------------------------------------------------------------------
	if (tran_begin(con)!=0)
	{
		ErrNum = -400292;
		sprintf(ErrMsg, "테이베이스 오류입니다.\n");

		goto fups4002_err;
	}
	//--------------------------------------------------------------------------
	// 무료자료실 등록
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "INSERT INTO T_BOARD_DATA               		"
	                 "     ( bid         , step        , level      "
	                 "     , title       , descript    , descript2 , descript3 , qury_cnt   "
	                 "     , memo_cnt    , secret_yn   , del_yn     "
	                 "     , reg_user    , reg_date    , reg_time   "
	                 "     , cate_cd     ) 							"
	                 "SELECT bid         , step        , level      "
	                 "     , title       , descript    , descript2 , descript3 , qury_cnt   "
	                 "     , memo_cnt    , secret_yn   , del_yn     "
	                 "	   , reg_user    , reg_date    , reg_time   "
	                 "     , cate_cd     							"
	                 "  FROM T_BOARD_DATA_T  						"
	                 " WHERE id  =  %lu         				    "
	                 , pfups4002->id);



	//infLOG(ALWAY,"Query : %s\n",szQuery);


	if (mysql_query(con, szQuery)){
		ErrNum = -400201;
		sprintf(ErrMsg, "컨텐츠정보를 생성 하지 못 하였습니다. ( %s ) \n",szQuery);

		goto fups4002_tran_err;
    }
	//--------------------------------------------------------------------------
	// 등록된 ID를 얻는다.
	//--------------------------------------------------------------------------
	dwId = 0;
	dwId = mysql_insert_id(con);

	#ifdef _DEBUG_
	printf("fups4002[LOG] : insert ID (%10ld)\n", dwId);
	#endif

	//--------------------------------------------------------------------------
	// 무료자료실 이미지 정보 업데이트
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, " update zangsi.T_BOARD_IMG "
	                 " set id = %lu, use_yn = 'Y' "
	                 " WHERE cont_cd = 'BD' and id  =  %lu "
	                 , dwId, pfups4002->id);
	//infLOG(ALWAY,"Query : %s\n",szQuery);

	if (mysql_query(con, szQuery)){
		ErrNum = -400201;
		sprintf(ErrMsg, "컨텐츠이미지정보를 생성 하지 못 하였습니다. ( %s ) \n",szQuery);

		goto fups4002_tran_err;
    }

	//--------------------------------------------------------------------------
	// 무료자료실 파일정보 INSERT
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));
	/*
	sprintf(szQuery, "INSERT INTO T_BOARD_DATA_F           "
	                 "     ( id         , folder_yn  , server_id  "
	                 "     , file_path  , file_name1 , file_name2 "
	                 "     , file_size  , file_type  , qury_cnt   "
	                 "     , down_cnt   , reg_user   , reg_date   "
	                 "     , reg_time   )                         "
	                 "SELECT %lu        , '%s'       , '%s'       "
	                 "     , '%s'       , '%s'       , local_file "
	                 "     , %13.0f     , '%s'       , 0          "
	                 "     , 0          , reg_user   , reg_date   "
	                 "     , reg_time                "
	                 "  FROM T_BOARD_DATA_T   "
	                 " WHERE id  =  %d               "
	                 , dwId
	                 , pfups4002->folder_yn
	                 , pfups4002->server_id
	                 , pfups4002->sfile_path
	                 , pfups4002->sfile_name
	                 , pfups4002->file_size
	                 , pfups4002->file_type
	                 , pfups4002->id
	                 );
	 */

	 sprintf(szQuery, "INSERT INTO T_BOARD_DATA_F           "
	                 "     ( id         , folder_yn  , server_id  "
	                 "     , file_spath  , file_sname , file_lname "
	                 "     , file_size  , file_type  , qury_cnt   "
	                 "     , down_cnt   , reg_user   , reg_date   "
	                 "     , reg_time   )                         "
	                 "SELECT %lu        , '%s'       , '%s'       "
	                 "     , '%s'       , '%s'       , local_file "
	                 "     , %13.0f     , '%s'       , 0          "
	                 "     , 0          , reg_user   , reg_date   "
	                 "     , reg_time                "
	                 "  FROM T_BOARD_DATA_T   "
	                 " WHERE id  =  %lu               "
	                 , dwId
	                 , pfups4002->folder_yn
	                 , pfups4002->server_id
	                 , pfups4002->sfile_path
	                 , pfups4002->sfile_name
	                 //, pfups4002->file_size
	                 , pfups4002->total_file_size
	                 , pfups4002->file_type
	                 , pfups4002->id
	                 );

	//infLOG(ALWAY,"Query : %s\n",szQuery);


	if (mysql_query(con, szQuery)){
		ErrNum = -400202;
		sprintf(ErrMsg, "컨텐츠정보를 생성 하지 못 하였습니다.\n");
		goto fups4002_tran_err;
    }

    #ifdef _DEBUG_
	printf("fups4002-> INSERT T_BOARD_DATA_F OK\n");
	#endif

	//--------------------------------------------------------------------------
	// 서버정보용량 UPDATE
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "UPDATE T_SERVER_INFO         "
	                 "   SET disk_use  = disk_use + %13.0f"
	                 " WHERE server_id = '%s'             "
	                 //,pfups4002->file_size
	                 , pfups4002->total_file_size
	                 ,pfups4002->server_id
	                 );
	if (mysql_query(con, szQuery)){
		ErrNum = -400204;
		sprintf(ErrMsg, "서버용량 처리를 하지 못 했습니다.\n");
		goto fups4002_tran_err;
    }

	//--------------------------------------------------------------------------
	// 임시컨턴츠정보
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "DELETE FROM T_BOARD_DATA_T  "
	                 " WHERE id = %lu                    "
	                 ,pfups4002->id
	                 );
	if (mysql_query(con, szQuery)){
		sprintf(ErrMsg, "임시컨턴츠정보를 삭제하지 못 했습니다.\n");
	    infLOG(ERROR, "fups4002-> [%d](%s)\n",ErrNum, ErrMsg);
		infLOG(ERROR, "fups4002-> [%d](%s)\n",mysql_errno(con), mysql_error(con));
    }

	if (tran_commit(con)!=0)
	{
		ErrNum = -400293;
		sprintf(ErrMsg, "테이베이스 오류입니다.\n");
		goto fups4002_tran_err;
	}

	if( bCloseDB )
		db_disconnect(con);



	req_header.nCmd = 1 ;
	pSendData = new char[HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

   	return HEADER_SIZE;



//------------------------------------------------------------------------------
fups4002_err:
    infLOG(ERROR, "fups4002-> [%d](%s)\n",ErrNum, ErrMsg);
	infLOG(ERROR, "fups4002-> [%d](%s)\n",mysql_errno(con), mysql_error(con));



	tran_end(con);
	if( bCloseDB )
		db_disconnect(con);
//	E_dump(ErrNum, ErrMsg, pSendData);

	strcpyA(err_req_header.errmsg,ErrMsg);

	err_req_header.header.nCmd = -1 ;
	pSendData = new char [ERR_HEADER_SIZE];
	memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

   	return ERR_HEADER_SIZE;


fups4002_tran_err:
    infLOG(ERROR, "fups4002-> [%d](%s)\n",ErrNum, ErrMsg);
	infLOG(ERROR, "fups4002-> [%d](%s)\n",mysql_errno(con), mysql_error(con));



	tran_rollback(con);
	tran_end(con);
	if( bCloseDB )
		db_disconnect(con);
//	E_dump(ErrNum, ErrMsg, pSendData);

	strcpyA(err_req_header.errmsg,ErrMsg);

	err_req_header.header.nCmd = -1 ;
	pSendData = new char [ERR_HEADER_SIZE];
	memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

   	return ERR_HEADER_SIZE;

}

