/******************************************************************************
 *   서브시스템 : CMD서버
 *   프로그램명 : fups4003.cc
 *         기능 : 내자료실 컨텐츠 등록
 *         설명 :
 *     수정이력 : 디비 경량화 작업으로 T_CONTDATA_INFO, T_CONTDATA_FILE
 				  , T_CONTDATA_FILELIST 부분 삭제
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
#include "commydb.h"
#include "apdefine.h"
#include "../../fupsvr/inc/fups4003.h" //CFUPS4003 구조체 사용
#include "dcmdfups4003.h"
#include "dcmdcomlib.h" //업로드 fupcomlib와 동기화
//#define  _DEBUG_
#include "mysql_pool.h"

extern CMysqlPool * m_g_clMysqlPool;

long dcmdfups4003(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData)//CFUPS4003 pfups4003)
{
	LPCFUPS4003 pfups4003 = (LPCFUPS4003)pRecvData;

	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);

	char szQuery[10000], szQuery1[10000];
	MYSQL *con = NULL;
	MYSQL_RES   *res;
	MYSQL_ROW    row;
	char ErrMsg[256];    // error message
	int  ErrNum;         // error no
    char szOldFile[768];
    char szNewFile[768];

    memset(szOldFile,0x00,sizeof(szOldFile));
    memset(szNewFile,0x00,sizeof(szNewFile));

	unsigned long dwId=0;
	char disp_end_date [ 8+1];
	char disp_end_time [ 6+1];
	char reg_user      [12+1];
	char reg_date      [ 8+1];
	char reg_time      [ 6+1];
	char up_st_date    [ 8+1];
	char up_st_time    [ 6+1];
	char file_name2   [255+1];  // 로컬파일이름
	char cont_gu       [ 2+1];
	char search_yn     [ 1+1];	// 검색허용여부
	int  cate_no             ;

	memset(&ErrMsg   , 0x00, sizeof(ErrMsg   )); // 오류메시지
	#ifdef _DEBUG_
	printf("fups4003-> fups4003 start\n");
	printf("id           (%d)\n", pfups4003 ->id           );
	printf("title        (%s)\n", pfups4003 ->title        );
	printf("descript     (%s)\n", pfups4003 ->descript     );
	printf("keyword      (%s)\n", pfups4003 ->keyword      );
	printf("sect_code    (%s)\n", pfups4003 ->sect_code    );
	printf("share_meth   (%s)\n", pfups4003 ->share_meth   );
	printf("disp_end_date(%s)\n", pfups4003 ->disp_end_date);
	printf("disp_end_time(%s)\n", pfups4003 ->disp_end_time);
	printf("disp_stat    (%s)\n", pfups4003 ->disp_stat    );
	printf("file_del_yn  (%s)\n", pfups4003 ->file_del_yn  );
	printf("folder_yn    (%s)\n", pfups4003 ->folder_yn    );
	printf("server_id    (%s)\n", pfups4003 ->server_id    );
	printf("file_path    (%s)\n", pfups4003 ->file_path    );
	printf("file_name1   (%s)\n", pfups4003 ->file_name1   );
	printf("file_name2   (%s)\n", pfups4003 ->file_name2   );
	printf("file_type    (%s)\n", pfups4003 ->file_type    );
	printf("up_st_date   (%s)\n", pfups4003 ->up_st_date   );
	printf("up_st_time   (%s)\n", pfups4003 ->up_st_time   );
	printf("reg_user     (%s)\n", pfups4003 ->reg_user     );
	printf("reg_date     (%s)\n", pfups4003 ->reg_date     );
	printf("reg_time     (%s)\n", pfups4003 ->reg_time     );
	printf("price_amt    (%d)\n", pfups4003 ->price_amt    );
	printf("won_mega     (%d)\n", pfups4003 ->won_mega     );
	printf("file_size    (%13.0f)\n", pfups4003 ->file_size);
	printf("file_resoX   (%d)\n", pfups4003 ->file_resoX   );
	printf("file_resoY   (%d)\n", pfups4003 ->file_resoY   );
	printf("qury_cnt     (%d)\n", pfups4003 ->qury_cnt     );
	printf("down_cnt     (%d)\n", pfups4003 ->down_cnt     );
	#endif

	//infLOG(ALWAY," ] 내 자료실 파일 등록 DB (%s)\n",pfups4003 ->file_name2);
	#ifdef __DEBUG
	printf("pfups4003 ->file_name2   (%s) file_name2 (%s)\n", pfups4003 ->file_name2,file_name2   );
	#endif


	if (pfups4003 ->id == 0)
	{
		sprintf(ErrMsg, "등록번호가 없습니다..\n");
	  	infLOG(ERROR, " ] DB fups4003-> [%d](%s)\n",ErrNum, ErrMsg);
//		infLOG(ERROR, "fups4003-> [%d](%s)\n",mysql_errno(con), mysql_error(con));

		req_header.nCmd = -1 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);

       	return HEADER_SIZE;

	}

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	bool bCloseDB = false;


	CMysqlCon MysqlCon(m_g_clMysqlPool,m_g_clMysqlPool->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );

	con = MysqlCon.GetMysqlCon();

	if (con == NULL )
	{
		ErrNum = -400392;
		sprintf(ErrMsg, "DB에 접속하지 못 하였습니다.\n");
		infLOG(ERROR, "FUPS4003 | GetMysqlCon is null \n");


		int nRetry = 0;
		while (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			infLOG(ERROR, "FUPS4003 | GetMysqlCon is null [%d](%s)\n",ErrNum, ErrMsg);
			infLOG(ERROR, "FUPS4003 | Mysql Error | Errno : %d | ErrMsg : %s \n\n", mysql_errno(con),mysql_error(con));
		}


		if( nRetry >= 5)
		{

			req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);

			infLOG(ERROR, "FUPS4003 | Cannot DB Connect \n");

	       	return HEADER_SIZE;
	    }
	    bCloseDB = true;
	    infLOG(ERROR,"FUPS4003 | Connect DB direct\n");

	}

	//--------------------------------------------------------------------------
	// 트렌젝션시작
	//--------------------------------------------------------------------------
	if (tran_begin(con)!=0)
	{
		ErrNum = -400392;
		sprintf(ErrMsg, "테이베이스 오류입니다.\n");
		goto fups4003_err;
	}

	//--------------------------------------------------------------------------
	// 등록일자 얻기
	//--------------------------------------------------------------------------
	memset(szQuery,  0x00, sizeof(szQuery));
	memset(szQuery1, 0x00, sizeof(szQuery1));
	strcpyA(szQuery, "SELECT date_format(now(),'%Y%m%d')");
	strcat(szQuery, "     , date_format(now(),'%H%i%s')");
	strcat(szQuery, "     , date_format(date_add(now(), INTERVAL 30 DAY),'%Y%m%d')");
	strcat(szQuery, "     , date_format(now(),'%H%i%s')");
	strcat(szQuery, "     , reg_user   as reg_user   ");
	strcat(szQuery, "     , reg_date   as up_st_date ");
	strcat(szQuery, "     , reg_time   as up_st_time ");
	strcat(szQuery, "     , file_name2 as file_name2 ");
	strcat(szQuery, "     , cont_gu                  ");
	strcat(szQuery, "     , search_yn                ");
	strcat(szQuery, "     , cate_no                  ");
	strcat(szQuery, "  FROM T_CONTENTS_TEMP   ");
	sprintf(szQuery1, " WHERE id  =  %lu ", pfups4003 ->id);
	strcat(szQuery, szQuery1);

	#ifdef __DEBUG
	printf("%s\n",szQuery);
	#endif

	if (mysql_query(con, szQuery)){
		ErrNum = -400301;
		sprintf(ErrMsg, "테이베이스 오류입니다.\n");
		goto fups4003_err;
	}

	if (!(res = mysql_store_result(con)))
	{
		ErrNum = -400302;
		sprintf(ErrMsg, "테이베이스 오류입니다.\n");
		goto fups4003_err;
	}
 	if (mysql_num_rows(res)==0)
 	{
		ErrNum = -400302;
		sprintf(ErrMsg, "검색된 자료가 없습니다.\n");
		mysql_free_result(res);
		goto fups4003_err;
	}
	row = mysql_fetch_row(res);
	memset(reg_user     , 0x00, sizeof(reg_user     ));
	memset(reg_date     , 0x00, sizeof(reg_date     ));
	memset(disp_end_date, 0x00, sizeof(disp_end_date));
	memset(disp_end_time, 0x00, sizeof(disp_end_time));
	memset(reg_time     , 0x00, sizeof(reg_time     ));
	memset(up_st_date   , 0x00, sizeof(up_st_date   ));
	memset(up_st_time   , 0x00, sizeof(up_st_time   ));
	memset(file_name2   , 0x00, sizeof(file_name2   ));
	memset(cont_gu      , 0x00, sizeof(cont_gu      ));
	memset(search_yn    , 0x00, sizeof(search_yn    ));
	memset(&cate_no     , 0x00, sizeof(cate_no      ));

	strcpyA(reg_date     , getstr(row, 0));
	strcpyA(reg_time     , getstr(row, 1));
	strcpyA(disp_end_date, getstr(row, 2));
	strcpyA(disp_end_time, getstr(row, 3));
	strcpyA(reg_user     , getstr(row, 4));
	strcpyA(up_st_date   , getstr(row, 5));
	strcpyA(up_st_time   , getstr(row, 6));
	strcpyA(file_name2   , getstr(row, 7));
	strcpyA(cont_gu      , getstr(row, 8));
	strcpyA(search_yn    , getstr(row, 9));
	cate_no   = getint(row,10);

	#ifdef __DEBUG
	printf(" ] pfups4003 ->file_name2   (%s) file_name2 (%s)\n", pfups4003 ->file_name2,file_name2   );
	#endif
	//infLOG(ALWAY, " ] 등록 - pfups4003 ->file_name2   (%s) file_name2 (%s)\n", pfups4003 ->file_name2,file_name2  );

	mysql_free_result(res);


	//--------------------------------------------------------------------------
	// 나만의 자료실 - 컨텐츠정보 INSERT
	//--------------------------------------------------------------------------
	memset(szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "INSERT INTO T_CONTDATA_MYINFO           "
	                 "     ( title   , descript  , keyword           "
	                 "     , sect_code    , adult_yn  , share_meth   "
	                 "     , price_amt    , won_mega  , reg_user     "
	                 "     , reg_date     , reg_time  , disp_end_date"
	                 "     , disp_end_time, open_meth , open_pass)   "
	                 "SELECT title   , descript  , keyword      "
	                 "     , sect_code    , adult_yn  , share_meth   "
	                 "     , price_amt    , won_mega  , reg_user     "
	                 "     , '%s'         , '%s'      , '%s'         "
	                 "     , '%s'         , open_meth , open_pass    "
	                 "  FROM T_CONTENTS_TEMP "
	                 " WHERE id  =  %lu              "
	                 ,reg_date
	                 ,reg_time
	                 ,disp_end_date
	                 ,disp_end_time
	                 ,pfups4003 ->id);

		#ifdef __DEBUG
		printf("%s\n",szQuery);
		#endif

	if (mysql_query(con, szQuery)){
		ErrNum = -400301;
		sprintf(ErrMsg, "컨텐츠정보를 생성 하지 못 하였습니다.\n");
		goto fups4003_tran_err;
    }

	//--------------------------------------------------------------------------
	// 생성된 컨텐츠번호 받기
	//--------------------------------------------------------------------------
	dwId = 0;
	dwId = mysql_insert_id(con);
	#ifdef _DEBUG_
	printf("fups4003[LOG] : insert ID (%10ld)\n", dwId);
	#endif


	//--------------------------------------------------------------------------
	// 나만의 자료실 - 컨텐츠파일정보 INSERT
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "INSERT INTO T_CONTDATA_MYFILE             "
	                 "       (id            ,folder_yn     ,server_id  "
	                 "       ,file_path     ,file_name1    ,file_name2 "
	                 "       ,file_size     ,file_type     ,file_resoX "
	                 "       ,file_resoY    ,qury_cnt      ,down_cnt   "
	                 "       ,up_st_date    ,up_st_time    ,reg_user   "
	                 "       ,reg_date      ,reg_time)                 "
	                 "SELECT  %lu           ,'%s'          ,'%s'       "
	                 "       ,'%s'          ,'%s'          ,file_name2 "
	                 "       , %13.0f  ,'%s' , %d  , %d  , %d  , %d    "
	                 "       ,'%s'     ,'%s' ,'%s' , '%s' ,'%s'        "
	                 "  FROM T_CONTENTS_TEMP "
	                 " WHERE id  =  %lu              "
	                 ,dwId
	                 ,pfups4003 ->folder_yn
	                 ,pfups4003 ->server_id
	                 ,pfups4003 ->file_path
	                 ,pfups4003 ->file_name1
	                 ,pfups4003 ->file_size
	                 ,pfups4003 ->file_type
	                 ,pfups4003 ->file_resoX
	                 ,pfups4003 ->file_resoY
	                 ,pfups4003 ->qury_cnt
	                 ,pfups4003 ->down_cnt
	                 ,up_st_date
	                 ,up_st_time
	                 ,reg_user
	                 ,reg_date
	                 ,reg_time
	                 ,pfups4003 ->id);
		#ifdef __DEBUG
		printf("%s\n",szQuery);
		#endif
	if (mysql_query(con, szQuery)){
		ErrNum = -400302;
		sprintf(ErrMsg, "컨텐츠정보를 생성 하지 못 하였습니다.\n");
		goto fups4003_tran_err;
    }
	if( strcmp(pfups4003->folder_yn,"Y") == 0 )
   	{

		//--------------------------------------------------------------------------
		// 나만의 자료실 - 컨텐츠파일 LIST정보 INSERT
		//--------------------------------------------------------------------------
		memset (szQuery , 0x00, sizeof(szQuery ));
		sprintf(szQuery, "INSERT INTO T_CONTDATA_MYFILELIST           "
		                 "       (id            ,seq_no        ,folder_yn  "
		                 "       ,file_name     ,file_size     ,file_type  "
		                 "       ,reg_user      ,reg_date      ,reg_time)  "
		                 "SELECT  %lu           ,seq_no        ,folder_yn  "
		                 "       ,file_name     ,file_size     ,file_type  "
		                 "       ,'%s'          ,'%s'          ,'%s'       "
		                 "  FROM T_CONTENTS_TEMPLIST "
		                 " WHERE id  =  %lu              "
		                 ,dwId
		                 ,reg_user
		                 ,reg_date
		                 ,reg_time
		                 ,pfups4003 ->id);

		#ifdef __DEBUG
		printf("%s\n",szQuery);
		#endif

		// 컨텐츠파일리스트 등록(T_CONTENTS_FILELIST)
		if (mysql_query(con, szQuery)){
			ErrNum = -400322;
			sprintf(ErrMsg, "컨텐츠파일리스트를 생성 하지 못 하였습니다.\n");
			goto fups4003_tran_err;
		}
		#ifdef _DEBUG_
		printf("fups4003-> INSERT T_CONTENTS_MYFILELIST OK\n");
		#endif


		//--------------------------------------------------------------------------
		// 임시컨턴츠파일리스트정보
		//--------------------------------------------------------------------------
		memset (szQuery , 0x00, sizeof(szQuery ));
		sprintf(szQuery, "DELETE FROM T_CONTENTS_TEMPLIST "
		                 " WHERE id = %lu                     "
		                 ,pfups4003 ->id
		                 );
		#ifdef __DEBUG
		printf("%s\n",szQuery);
		#endif
		if (mysql_query(con, szQuery))
		{
			sprintf(ErrMsg, "임시컨턴츠파일리스트를 삭제 하지 못 했습니다.\n");
			infLOG(ERROR, "Exception ) (%lu) fups4003-> 임시컨턴츠파일리스트를 삭제하지 못 했습니다.\n",dwId);
			infLOG(ERROR, "Exception ) fups4003-> [%d](%s)\n",ErrNum, ErrMsg);
			infLOG(ERROR, "Exception ) fups4003-> [%d](%s)\n",mysql_errno(con), mysql_error(con));
		}
	}


	//--------------------------------------------------------------------------
	// 내자료실 사용량 UPDATE
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));

	sprintf(szQuery, "UPDATE T_MYDATA_INFO              "
	                 "   SET disk_use    = disk_use    + %15.0f"
	                 "     , upload_size = upload_size - %15.0f"
	                 " WHERE user_id   = '%s'                  "
	                 ,pfups4003 ->file_size
	                 ,pfups4003 ->file_size
	                 ,reg_user
	                 );


			#ifdef __DEBUG
			printf("%s\n",szQuery);
			#endif
	if (mysql_query(con, szQuery)){
		ErrNum = -400304;
		sprintf(ErrMsg, "서버용량 처리를 하지 못 했습니다.\n");
		goto fups4003_tran_err;
    }

	//--------------------------------------------------------------------------
	// 서버정보용량 UPDATE
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "UPDATE T_SERVER_INFO         "
	                 "   SET disk_use  = disk_use + %15.0f"
	                 "  ,real_disk_use  = real_disk_use + %13.0f"
	                 " WHERE server_id = '%s'             "
	                 ,pfups4003 ->file_size
	                 ,pfups4003 ->file_size
	                 ,pfups4003 ->server_id
	                 );
			#ifdef __DEBUG
			printf("%s\n",szQuery);
			#endif
	if (mysql_query(con, szQuery)){
		ErrNum = -400304;
		sprintf(ErrMsg, "서버용량 처리를 하지 못 했습니다.\n");
		goto fups4003_tran_err;
    }


	//--------------------------------------------------------------------------
	// 컨텐츠 정보에 추가 신규
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "INSERT into zangsi.T_CONTENTS_FILE_USER_CNT "
	                 " ( id , cont_gu ) select %lu , 'MD' "
	                 ,dwId
	                 );
	if (mysql_query(con, szQuery))
	{
		ErrNum = -400104;
		sprintf(ErrMsg, "파일 사용자수 정보를 처리를 하지 못 했습니다.\n");
		infLOG(ERROR, "Exception ) fups4001-> [%d](%s)\n",ErrNum, ErrMsg);
		infLOG(ERROR, "Exception ) fups4001-> [%d](%s)\n",mysql_errno(con), mysql_error(con));
  	}




	//--------------------------------------------------------------------------
	// 임시컨턴츠정보
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "DELETE FROM zangsi.T_CONTENTS_TEMP  "
	                 " WHERE id = %lu                     "
	                 ,pfups4003->id
	                 );
			#ifdef __DEBUG
			printf("%s\n",szQuery);
			#endif
	if (mysql_query(con, szQuery)){
		ErrNum = -400305;
		sprintf(ErrMsg, "임시컨턴츠정보를 삭제하지 못 했습니다.\n");
		infLOG(ERROR, "fups4003-> Exception !!! [%d]temp_id(%lu)임시컨턴츠정보를 삭제하지 못 했습니다.\n", pfups4003 ->id);
		infLOG(ERROR, "fups4003-> Exception !!! [%d](%s)\n",mysql_errno(con), mysql_error(con));
    }


	//--------------------------------------------------------------------------
	// 임시컨턴츠정보
	//--------------------------------------------------------------------------
	/* 2009/04/06 - HCS
		디비 경량화 작업 중 안쓰는 테이블 삭제.
	memset (szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "DELETE FROM T_CONTENTS_TEMPIMG "
	                 " WHERE id = %lu                     "
	                 ,pfups4003 ->id
	                 );

			#ifdef __DEBUG
			printf("%s\n",szQuery);
			#endif
	if (mysql_query(con, szQuery))
	{
		sprintf(ErrMsg, "임시컨턴츠이미지정보를 삭제하지 못 했습니다.\n");
		infLOG(ERROR, "Exception ) (%lu) fups4001-> 임시컨턴츠정보를 삭제하지 못 했습니다.\n",dwId);
		infLOG(ERROR, "Exception ) fups4003-> [%d](%s)\n",ErrNum, ErrMsg);
		infLOG(ERROR, "Exception ) fups4003-> [%d](%s)\n",mysql_errno(con), mysql_error(con));
	}
	*/
	/*
	if( rename(szOldFile,szNewFile) != 0)
	{
		ErrNum = -400306;
		sprintf(ErrMsg, " 파일 오류 [ %lu ]  ",pfups4003->id);
		goto fups4003_tran_err;
	}
	*/



	if (tran_commit(con)!=0)
	{
		ErrNum = -400393;
		sprintf(ErrMsg, "테이베이스 오류입니다.\n");
		goto fups4003_tran_err;
	}

	if( bCloseDB )
		db_disconnect(con);

	//db_disconnect(con);
	req_header.nCmd = 1 ;
	pSendData = new char[HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

   	return HEADER_SIZE;
//------------------------------------------------------------------------------
fups4003_err:
	#ifdef __DEBUG
	printf("fups4003_err ) fups4003-> [%d](%s)\n",ErrNum, ErrMsg);
	printf("fups4003_err ) fups4003-> [%d](%s)\n",mysql_errno(con), mysql_error(con));
	#endif
	infLOG(ERROR, "fups4003-> [%d](%s)\n",ErrNum, ErrMsg);
	infLOG(ERROR, "fups4003-> [%d](%s)\n",mysql_errno(con), mysql_error(con));

	tran_end(con);
	if( bCloseDB )
		db_disconnect(con);

//	E_dump(ErrNum, ErrMsg, pSendData);

	req_header.nCmd = -1 ;
	pSendData = new char [HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

   	return HEADER_SIZE;

fups4003_tran_err:
	#ifdef __DEBUG
	printf("fups4003_tran_err ) fups4003-> [%d](%s)\n",ErrNum, ErrMsg);
	printf("fups4003_tran_err ) fups4003-> [%d](%s)\n",mysql_errno(con), mysql_error(con));
	#endif

	infLOG(ERROR, "fups4003-> [%d](%s)\n",ErrNum, ErrMsg);
	infLOG(ERROR, "fups4003-> [%d](%s)\n",mysql_errno(con), mysql_error(con));

	tran_rollback(con);
	tran_end(con);
	if( bCloseDB )
		db_disconnect(con);

//	E_dump(ErrNum, ErrMsg, pSendData);

	req_header.nCmd = -1 ;
	pSendData = new char [HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

   	return HEADER_SIZE;
}


