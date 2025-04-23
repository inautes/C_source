/******************************************************************************
 *   서브시스템 : CMD서버
 *   프로그램명 : fups4004.cc
 *         기능 : 컨텐츠 등록
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
#include "commydb.h"
#include "apdefine.h"
//#include "/home/jindogg/zangsi/fupsvr/inc/fupcomlib.h"
#include "/home/filenori/zangsi_with_dcmd/fupsvr/inc/fups4004.h"
// fups4004 와 4004 는 같은 구조체를 가지고 있음.

#include "dcmdcomlib.h" //업로드 fupcomlib
//#define  _DEBUG_
#include "mysql_pool.h"

extern CMysqlPool * m_g_clMysqlPool;


long dcmdfups4004(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData)//CFUPS4004 pfups4004)
{

	LPCFUPS4004 pfups4004 = (LPCFUPS4004)pRecvData;


	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);

	char szQuery[10000], szQuery1[10000];
	MYSQL *con = NULL;
	MYSQL_RES   *res;
	MYSQL_ROW    row;
	char ErrMsg[256];    // error message
	int  ErrNum;         // error no

	unsigned long dwId=0;
	unsigned long bomul_id=0;
	unsigned long req_id=0;
	char disp_end_date [ 8+1];
	char disp_end_time [ 6+1];
	char reg_user      [12+1];
	char reg_date      [ 8+1];
	char reg_time      [ 6+1];
	char up_st_date    [ 8+1];
	char up_st_time    [ 6+1];
	char file_name2   [255+1];  // 로컬파일이름

	memset(&ErrMsg   , 0x00, sizeof(ErrMsg   )); // 오류메시지
	#ifdef _DEBUG_
	printf("fups4004-> fups4004 start\n");
	printf("id           (%d)\n", pfups4004->id           );
	printf("title        (%s)\n", pfups4004->title        );
	printf("descript     (%s)\n", pfups4004->descript     );
	printf("keyword      (%s)\n", pfups4004->keyword      );
	printf("sect_code    (%s)\n", pfups4004->sect_code    );
	printf("share_meth   (%s)\n", pfups4004->share_meth   );
	printf("disp_end_date(%s)\n", pfups4004->disp_end_date);
	printf("disp_end_time(%s)\n", pfups4004->disp_end_time);
	printf("disp_stat    (%s)\n", pfups4004->disp_stat    );
	printf("file_del_yn  (%s)\n", pfups4004->file_del_yn  );
	printf("folder_yn    (%s)\n", pfups4004->folder_yn    );
	printf("server_id    (%s)\n", pfups4004->server_id    );
	printf("file_path    (%s)\n", pfups4004->file_path    );
	printf("file_name1   (%s)\n", pfups4004->file_name1   );
	printf("file_name2   (%s)\n", pfups4004->file_name2   );
	printf("file_type    (%s)\n", pfups4004->file_type    );
	printf("up_st_date   (%s)\n", pfups4004->up_st_date   );
	printf("up_st_time   (%s)\n", pfups4004->up_st_time   );
	printf("reg_user     (%s)\n", pfups4004->reg_user     );
	printf("reg_date     (%s)\n", pfups4004->reg_date     );
	printf("reg_time     (%s)\n", pfups4004->reg_time     );
    printf("price_amt    (%d)\n", pfups4004->price_amt    );
    printf("won_mega     (%d)\n", pfups4004->won_mega     );
	printf("file_size    (%13.0f)\n", pfups4004->file_size);
	printf("file_resoX   (%d)\n", pfups4004->file_resoX   );
	printf("file_resoY   (%d)\n", pfups4004->file_resoY   );
	printf("qury_cnt     (%d)\n", pfups4004->qury_cnt     );
	printf("down_cnt     (%d)\n", pfups4004->down_cnt     );
	#endif


#ifdef _DEBUG_
printf("pfups4004->file_name2   (%s) file_name2 (%s)\n", pfups4004->file_name2,file_name2   );
#endif
//infLOG(ALWAY,"cfups4004 frist input ) pfups4004->file_name2   (%s) file_name2 (%s)\n", pfups4004->file_name2,file_name2   );



	if (pfups4004->id == 0)
	{
		sprintf(ErrMsg, "등록번호가 없습니다..\n");
	    infLOG(ERROR, "fups4004-> [%d](%s)\n",ErrNum, ErrMsg);
//		infLOG(ERROR, "fups4004-> [%d](%s)\n",mysql_errno(con), mysql_error(con));

		req_header.nCmd = -1 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);

       	return HEADER_SIZE;

		//return -1;
	}

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------

	bool bCloseDB = false;

	CMysqlCon MysqlCon(m_g_clMysqlPool,getpid());

	con = MysqlCon.GetMysqlCon();

	if ( con == NULL )
	{
		ErrNum = -400492;

		infLOG(ERROR, "GetMysqlCon is null \n");

		sprintf(ErrMsg, "DB에 접속하지 못 하였습니다.\n");
		#ifdef __DEBUG
		printf("DB에 접속하지 못 하였습니다.\n");
		#endif



		int nRetry = 0;
		while (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			#ifdef __DEBUG_
			printf(" ] DB 접속 재시도 \n");
			#endif
			infLOG(ERROR, "Exception ) fups4004-> [%d](%s)\n",ErrNum, ErrMsg);
		}

		if( nRetry >= 5)
		{

			req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);

	       	return HEADER_SIZE;
	    }
	    bCloseDB = true;

	}

	//--------------------------------------------------------------------------
	// 트렌젝션시작
	//--------------------------------------------------------------------------
	if (tran_begin(con)!=0)
	{
		ErrNum = -400492;
		sprintf(ErrMsg, "테이베이스 오류입니다.\n");
		goto fups4004_err;
	}

	//--------------------------------------------------------------------------
	// 등록일자 얻기
	//--------------------------------------------------------------------------

	memset(szQuery,  0x00, sizeof(szQuery));
	memset(szQuery1, 0x00, sizeof(szQuery1));
	strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d')");
	strcat(szQuery, "     , date_format(now(),'%H%i%s')");
	strcat(szQuery, "     ,  if(sect_code = 13 or sect_code = 14 , date_format(date_add(now(),INTERVAL 60 DAY),"
					"'%Y%m%d'),date_format(date_add(now(), INTERVAL 30 DAY),'%Y%m%d'))");
	//strcat(szQuery, "     , date_format(date_add(now(), INTERVAL 30 DAY),'%Y%m%d')");
	strcat(szQuery, "     , date_format(now(),'%H%i%s')");
	strcat(szQuery, "     , reg_user   as reg_user   ");
	strcat(szQuery, "     , reg_date   as up_st_date ");
	strcat(szQuery, "     , reg_time   as up_st_time ");
	strcat(szQuery, "     , file_name2 as file_name2 ");
	strcat(szQuery, "     , bomul_id   as bomul_id   ");
	strcat(szQuery, "     , req_id   as req_id   ");
	strcat(szQuery, "  FROM T_CONTENTS_TEMP   ");
	sprintf(szQuery1, " WHERE id  =  %ld ", pfups4004->id);
	strcat(szQuery, szQuery1);

	if (mysql_query(con, szQuery)){
		ErrNum = -400401;
		sprintf(ErrMsg, "테이베이스 오류입니다.\n");
		goto fups4004_err;
	}

	if (!(res = mysql_store_result(con)))
	{
		ErrNum = -400402;
		sprintf(ErrMsg, "테이베이스 오류입니다.\n");
		goto fups4004_err;
	}
 	if (mysql_num_rows(res)==0)
 	{
		ErrNum = -400402;
		sprintf(ErrMsg, "검색된 자료가 없습니다.\n");
		mysql_free_result(res);
		goto fups4004_err;
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
	memset(&bomul_id    , 0x00, sizeof(bomul_id     ));
	memset(&req_id    , 0x00, sizeof(req_id     ));


	strcpy(reg_date     , getstr(row, 0));
	strcpy(reg_time     , getstr(row, 1));
	strcpy(disp_end_date, getstr(row, 2));
	strcpy(disp_end_time, getstr(row, 3));
	strcpy(reg_user     , getstr(row, 4));
	strcpy(up_st_date   , getstr(row, 5));
	strcpy(up_st_time   , getstr(row, 6));
	strcpy(file_name2   , getstr(row, 7));
	bomul_id = (unsigned long)(getnum(row, 8));
	req_id = (unsigned long)(getnum(row, 9));


	#ifdef _DEBUG_
	printf("pfups4004->file_name2   (%s) file_name2 (%s)\n", pfups4004->file_name2,file_name2   );
	#endif
	//infLOG(ALWAY,"cfups4004 second input ) pfups4004->file_name2   (%s) file_name2 (%s)\n", pfups4004->file_name2,file_name2   );


	mysql_free_result(res);
	//--------------------------------------------------------------------------
	// 컨텐츠정보 번호발번
	//--------------------------------------------------------------------------
	memset(szQuery , 0x00, sizeof(szQuery ));
	strcpy(szQuery, "INSERT INTO T_CONTENTS_ID (cont_gu) values ('01')");

	// 컨텐츠정보 번호발번(T_CONTENTS_ID)
	if (mysql_query(con, szQuery)){
		ErrNum = -400411;
		sprintf(ErrMsg, "컨텐츠정보를 생성 하지 못 하였습니다.\n");
		goto fups4004_tran_err;
    }
	// 등록된 ID를 얻는다.
	dwId = 0;
	dwId = mysql_insert_id(con);
	#ifdef _DEBUG_
	printf("fups4004[LOG] : insert ID (%10ld)\n", dwId);
	#endif

	if (commit(con)!=0)
	{
		ErrNum = -400493;
		sprintf(ErrMsg, "테이베이스 오류입니다.\n");
		goto fups4004_tran_err;
	}


	//--------------------------------------------------------------------------
	// 컨텐츠정보 INSERT
	//--------------------------------------------------------------------------
	memset(szQuery , 0x00, sizeof(szQuery ));


    #ifdef _DEBUG_
    printf( "INSERT INTO T_CONTENTS_INFO              "
            "     ( id,  title   , descript   , descript2 , descript3 , keyword       "
            "     , sect_code    , sect_sub   , adult_yn     "
            "     , share_meth   , price_amt    , won_mega   "
            "     , reg_user     , reg_date     , reg_time   "
            "     , disp_end_date, disp_end_time,item_bold_yn"
            "     , item_color   , bomul_id     , bomul_stat "
            "     , req_id ) "
            "SELECT %ld,  title  , descript     , descript2 , descript3  , keyword      "
            "     , sect_code    , sect_sub     , adult_yn     "
            "     , share_meth   , price_amt    , won_mega    "
            "     , reg_user     ,'%s'          , '%s'          "
            "     , '%s'         , '%s'         ,item_bold_yn "
            "     , item_color   , bomul_id     , bomul_stat  "
            "     , req_id  "
            "  FROM T_CONTENTS_TEMP "
            " WHERE id  =  %ld              \n\n"
            ,dwId
            ,reg_date
            ,reg_time
            ,disp_end_date
            ,disp_end_time
            ,pfups4004->id);
    #endif

			    sprintf(szQuery, "INSERT INTO T_CONTENTS_INFO              "
			            "     ( id,  title   , descript  , descript2 , descript3  , keyword       "
			            "     , sect_code    , sect_sub   , adult_yn     "
			            "     , share_meth   , price_amt    , won_mega   "
			            "     , reg_user     , reg_date     , reg_time   "
			            "     , disp_end_date, disp_end_time,item_bold_yn"
			            "     , item_color   , bomul_id     , bomul_stat "
			            "     , req_id ) "
			            "SELECT %ld,  title  , descript     , descript2 , descript3  , keyword      "
			            "     , sect_code    , sect_sub     , adult_yn     "
			            "     , share_meth   , price_amt    , won_mega    "
			            "     , reg_user     ,'%s'          , '%s'          "
			            "     , '%s'         , '%s'         ,item_bold_yn "
			            "     , item_color   , bomul_id     , bomul_stat  "
			            "     , req_id  "
			            "  FROM T_CONTENTS_TEMP "
			            " WHERE id  =  %ld              "
	                 ,dwId
	                 ,reg_date
	                 ,reg_time
	                 ,disp_end_date
	                 ,disp_end_time
	                 ,pfups4004->id);


	// 컨텐츠정보 등록(T_CONTENTS_INFO)
	if (mysql_query(con, szQuery)){
		ErrNum = -400401;
		sprintf(ErrMsg, "컨텐츠정보를 생성 하지 못 하였습니다.\n");
		goto fups4004_tran_err;
    }

	//--------------------------------------------------------------------------
	// 요청자료 처리
	//--------------------------------------------------------------------------
	#ifdef _DEBUG_
	printf("req_id = %ld \n\n",req_id);
	#endif

	if( req_id > 0 )
	{
		memset(szQuery , 0x00, sizeof(szQuery ));

		#ifdef _DEBUG_
	    printf( " UPDATE T_CONTENTS_REQ "
	            "    set upload_yn = 'Y'"
	            "  where id = %ld \n\n"
	            ,req_id );
		#endif

	    sprintf(szQuery," UPDATE T_CONTENTS_REQ "
			            "    set upload_yn = 'Y'"
			            "  where id = %ld "
	            		,req_id );


		// 컨텐츠정보 등록(T_CONTENTS_INFO)
		if (mysql_query(con, szQuery)){
			ErrNum = -400423;
			sprintf(ErrMsg, "요청자료를 갱신하지 못 하였습니다.\n");
		}


	}

	//--------------------------------------------------------------------------
	// 내가 보유한 아이템에 업로드중 상태를 사용상태로 정보 기록
	//--------------------------------------------------------------------------
	memset(szQuery , 0x00, sizeof(szQuery ));

	#ifdef _DEBUG_
    printf( " UPDATE T_ITEM_PAYMENT "
            "    set use_code = '1' "
            "      , use_id   = %ld "
            "  where use_id   = %ld "
            "    and user_id  = '%s'"
            "    and use_code = '3' \n\n"
            ,dwId , pfups4004->id , reg_user  );
	#endif

	sprintf(szQuery, " UPDATE T_ITEM_PAYMENT "
		             "    set use_code = '1' "
		             "      , use_id   = %ld "
		             "  where use_id   = %ld "
		             "    and user_id  = '%s'"
		             "    and use_code = '3' "
		             ,dwId , pfups4004->id , reg_user  );

	// 컨텐츠정보 등록(T_CONTENTS_INFO)
	if (mysql_query(con, szQuery)){
		ErrNum = -400424;
		sprintf(ErrMsg, "아이템 정보를 갱신하지 못 하였습니다.\n");
		goto fups4004_tran_err;
    }

	//--------------------------------------------------------------------------
	// 보물 테이블
	//--------------------------------------------------------------------------
	if (bomul_id>0) {
		memset(szQuery , 0x00, sizeof(szQuery ));

		#ifdef _DEBUG_
        printf( " UPDATE T_CONTENTS_BOMUL        "
	            "    set id     = %ld "
	            "  where seq_no = %ld \n\n"
	            ,dwId , bomul_id);
        #endif

		sprintf(szQuery, " UPDATE T_CONTENTS_BOMUL        "
			             "    set id     = %ld "
			             "  where seq_no = %ld "
			             ,dwId , bomul_id);


		// 컨텐츠정보 등록(T_CONTENTS_INFO)
		if (mysql_query(con, szQuery)){
			ErrNum = -400425;
			sprintf(ErrMsg, "보물 아이템 정보를 갱신하지 못 하였습니다.\n");
			goto fups4004_tran_err;
		}
	}


	//--------------------------------------------------------------------------
	// 컨텐츠파일의 이름을 결정한다.
	//--------------------------------------------------------------------------


    char szOldFile[768];
    char szNewFile[768];

    memset(szOldFile,0x00,sizeof(szOldFile));
    memset(szNewFile,0x00,sizeof(szNewFile));

    char szFilename[60];
    char szFileType[10];

    memset(szFilename,0x00,sizeof(szFilename));
    memset(szFileType,0x00,sizeof(szFileType));


	if( strcmp(pfups4004->folder_yn,"Y") == 0 )
	{

			sprintf(szFilename,"%ld",dwId);


			sprintf(szOldFile,"%s/%s",pfups4004->file_path,pfups4004->file_name1);
			sprintf(szNewFile,"%s/%s",pfups4004->file_path,szFilename);

			#ifdef __DEBUG
			printf("FileDataTransfer     ] old file ( %s ) old file ( %s )  \n",szOldFile,szNewFile);
			#endif

			if( rename(szOldFile,szNewFile) != 0)
			{
			    sprintf(szFilename,"%s",pfups4004->file_name1);
			}
		}
		else
		{


	    	int nLen = GetReverseIndex(pfups4004->file_name1 , '.');

			if(nLen < 0)
			{
				#ifdef __DEBUG
				printf("FileDataTransfer  ERR] Error client file name  %s\n",pfups4004->file_name1);
				#endif

				infLOG(ERROR, "FileDataTransfer  ERR]: File 이름 에러 (확장자 없음): < 통 과 >\n");

			}
			else
				GetRightString(pfups4004->file_name1,strlen(pfups4004->file_name1)-nLen,szFileType);

			sprintf(szFilename,"%ld%s",dwId,szFileType);

			sprintf(szOldFile,"%s/%s",pfups4004->file_path,pfups4004->file_name1);
			sprintf(szNewFile,"%s/%s",pfups4004->file_path,szFilename);

			#ifdef __DEBUG
			printf("FileDataTransfer     ] old file ( %s ) old file ( %s )  \n",szOldFile,szNewFile);
			#endif

			if( rename(szOldFile,szNewFile) != 0)
			{
			    sprintf(szFilename,"%s",pfups4004->file_name1);
			}

    }


	//--------------------------------------------------------------------------
	// 컨텐츠파일정보 INSERT
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));

	#ifdef _DEBUG_
	printf( "INSERT INTO T_CONTENTS_FILE               "
            "       (id            ,folder_yn     ,server_id  "
            "       ,file_path     ,file_name1    ,file_name2 "
            "       ,file_size     ,file_type     ,file_resoX "
            "       ,file_resoY    ,qury_cnt      ,down_cnt   "
            "       ,up_st_date    ,up_st_time    ,reg_user   "
            "       ,reg_date      ,reg_time)                 "
            "SELECT  %ld           ,'%s'          ,'%s'       "
            "       ,'%s'          ,'%s'          ,file_name2 "
            "       , %13.0f  ,'%s' , %d  , %d  , %d  , %d    "
            "       ,'%s'     ,'%s' ,'%s' , '%s' ,'%s'        "
            "  FROM T_CONTENTS_TEMP "
            " WHERE id  =  %ld              \n\n"
            ,dwId
            ,pfups4004->folder_yn
            ,pfups4004->server_id
            ,pfups4004->file_path
            ,szFilename
//            ,pfups4004->file_name1
            ,pfups4004->file_size
            ,pfups4004->file_type
            ,pfups4004->file_resoX
            ,pfups4004->file_resoY
            ,pfups4004->qury_cnt
            ,pfups4004->down_cnt
            ,up_st_date
            ,up_st_time
            ,reg_user
            ,reg_date
            ,reg_time
            ,pfups4004->id);
    #endif


	sprintf(szQuery, "INSERT INTO T_CONTENTS_FILE               "
	                 "       (id            ,folder_yn     ,server_id  "
	                 "       ,file_path     ,file_name1    ,file_name2 "
	                 "       ,file_size     ,file_type     ,file_resoX "
	                 "       ,file_resoY    ,qury_cnt      ,down_cnt   "
	                 "       ,up_st_date    ,up_st_time    ,reg_user   "
	                 "       ,reg_date      ,reg_time)                 "
	                 "SELECT  %ld           ,'%s'          ,'%s'       "
	                 "       ,'%s'          ,'%s'          ,file_name2 "
	                 "       , %13.0f  ,'%s' , %d  , %d  , %d  , %d    "
	                 "       ,'%s'     ,'%s' ,'%s' , '%s' ,'%s'        "
	                 "  FROM T_CONTENTS_TEMP "
	                 " WHERE id  =  %ld              "
	                 ,dwId
	                 ,pfups4004->folder_yn
	                 ,pfups4004->server_id
	                 ,pfups4004->file_path
	                 ,szFilename
	                 ,pfups4004->file_size
	                 ,pfups4004->file_type
	                 ,pfups4004->file_resoX
	                 ,pfups4004->file_resoY
	                 ,pfups4004->qury_cnt
	                 ,pfups4004->down_cnt
	                 ,up_st_date
	                 ,up_st_time
	                 ,reg_user
	                 ,reg_date
	                 ,reg_time
	                 ,pfups4004->id);

	// 컨텐츠파일정보 등록(T_CONTENTS_FILE)
	if (mysql_query(con, szQuery)){
		ErrNum = -400402;
		sprintf(ErrMsg, "컨텐츠정보를 생성 하지 못 하였습니다.\n");
		goto fups4004_tran_err;
    }
    #ifdef _DEBUG_
	printf("fups4004-> INSERT T_CONTENTS_FILE OK\n");
	#endif

	//--------------------------------------------------------------------------
	// 컨텐츠파일 LIST정보 INSERT
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));

	#ifdef _DEBUG_
    printf( "INSERT INTO T_CONTENTS_FILELIST           "
            "       (id            ,seq_no        ,folder_yn  "
            "       ,file_name     ,file_size     ,file_type  "
            "       ,reg_user      ,reg_date      ,reg_time)  "
            "SELECT  %ld           ,seq_no        ,folder_yn  "
            "       ,file_name     ,file_size     ,file_type  "
            "       ,'%s'          ,'%s'          ,'%s'       "
            "  FROM T_CONTENTS_TEMPLIST "
            " WHERE id  =  %ld              \n\n"
            ,dwId
            ,reg_user
            ,reg_date
            ,reg_time
            ,pfups4004->id);

	#endif
	sprintf(szQuery, "INSERT INTO T_CONTENTS_FILELIST           "
	                 "       (id            ,seq_no        ,folder_yn  "
	                 "       ,file_name     ,file_size     ,file_type  "
	                 "       ,reg_user      ,reg_date      ,reg_time)  "
	                 "SELECT  %ld           ,seq_no        ,folder_yn  "
	                 "       ,file_name     ,file_size     ,file_type  "
	                 "       ,'%s'          ,'%s'          ,'%s'       "
	                 "  FROM T_CONTENTS_TEMPLIST "
	                 " WHERE id  =  %ld              "
	                 ,dwId
	                 ,reg_user
	                 ,reg_date
	                 ,reg_time
	                 ,pfups4004->id);

	// 컨텐츠파일리스트 등록(T_CONTENTS_FILELIST)
	if (mysql_query(con, szQuery)){
		ErrNum = -400422;
		sprintf(ErrMsg, "컨텐츠파일리스트를 생성 하지 못 하였습니다.\n");
		goto fups4004_tran_err;
    }
    #ifdef _DEBUG_
	printf("fups4004-> INSERT T_CONTENTS_FILELIST OK\n");
	#endif

	//--------------------------------------------------------------------------
	// 성인컨텐츠정보(T_CONTENTS_LIMIT) INSERT
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));

    #ifdef _DEBUG_

	printf( "INSERT INTO T_CONTENTS_LIMIT              "
            "       (id            ,title         ,sect_code  "
            "       ,adult_yn      ,reg_user                  "
            "       ,reg_date      ,reg_time               )  "
            "SELECT  %ld           ,title         ,sect_code  "
            "       ,adult_yn      ,'%s'                      "
            "       ,'%s'          ,'%s'                      "
            "  FROM T_CONTENTS_TEMP                    "
            " WHERE id  =  %ld                                "
            "   and limit_yn = 'Y'              \n\n  "
            ,dwId
            ,reg_user
            ,reg_date
            ,reg_time
            ,pfups4004->id);


    #endif

	sprintf(szQuery, "INSERT INTO T_CONTENTS_LIMIT              "
                     "       (id            ,title         ,sect_code  "
                     "       ,adult_yn      ,reg_user                  "
                     "       ,reg_date      ,reg_time               )  "
                     "SELECT  %ld           ,title         ,sect_code  "
                     "       ,adult_yn      ,'%s'                      "
                     "       ,'%s'          ,'%s'                      "
                     "  FROM T_CONTENTS_TEMP                    "
                     " WHERE id  =  %ld                                "
                     "   and limit_yn = 'Y'                            "

                     ,dwId
                     ,reg_user
                     ,reg_date
                     ,reg_time
                     ,pfups4004->id);




	// 컨텐츠파일리스트 등록(T_CONTENTS_LIMIT)
	if (mysql_query(con, szQuery)){
		ErrNum = -400423;
		sprintf(ErrMsg, "성인텐츠 제안갯수를 생성 하지 못 하였습니다.\n");
		goto fups4004_tran_err;
    }
    #ifdef _DEBUG_
	printf("fups4004-> INSERT T_CONTENTS_LIMIT OK\n");
	#endif

	//--------------------------------------------------------------------------
	// 컨텐츠이미지정보(T_CONTENTS_IMG) INSERT
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));
/*
	sprintf(szQuery, "INSERT INTO T_CONTENTS_IMG  "
	                 "       (cont_gu       ,id          "
	                 "       ,img_spath     ,img_sname   "
	                 "       ,img_lname     ,reg_user    "
	                 "       ,reg_date      ,reg_time)   "
	                 "SELECT  cont_gu       ,%ld         "
	                 "       ,img_spath     ,img_sname   "
	                 "       ,img_lname     ,'%s'        "
	                 "       ,'%s'          ,'%s'        "
	                 "  FROM T_CONTENTS_TEMPIMG   "
	                 " WHERE id  =  %ld                  "
	                 ,dwId
	                 ,reg_user
	                 ,reg_date
	                 ,reg_time
	                 ,pfups4004->id);
*/

	sprintf(szQuery, "UPDATE T_CONTENTS_IMG  	"
					 "set id = %ld,tmp_yn='N'		"
					 "where id = %ld and cont_gu = 'WE' "
	                 ,dwId
	                 ,pfups4004->id);


	if (mysql_query(con, szQuery))
	{
		ErrNum = -400424;
		sprintf(ErrMsg, "컨텐츠이미지정보를 생성 하지 못 하였습니다.\n");
		goto fups4004_tran_err;
    }

	//--------------------------------------------------------------------------
	// 컨텐츠아이템정보(T_CONTENTS_ITEM) INSERT
	//--------------------------------------------------------------------------
	/*
	memset (szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "INSERT INTO T_CONTENTS_INFO  "
	                 "       (cont_gu       ,id          "
	                 "       ,seq_no        ,item_code   "
	                 "       ,item_color				 "
	                 "       ,reg_date      ,reg_time)   "
	                 "SELECT  cont_gu       ,%ld         "
	                 "       ,seq_no        ,item_code   "
	                 "       ,item_color				 "
	                 "       ,'%s'          ,'%s'        "
	                 "  FROM T_CONTENTS_TEMP  "
	                 " WHERE id  =  %d              "
	                 ,dwId
	                 //seq_no
	                 //item_code
	                 //item_color
	                 ,reg_date
	                 ,reg_time
	                 ,pfups4004->id);

	if (mysql_query(con, szQuery))
	{
		ErrNum = -400424;
		sprintf(ErrMsg, "컨텐츠아이템정보를 생성 하지 못 하였습니다.\n");
		goto fups4004_tran_err;
    }
    */
/*	memset (szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "INSERT INTO T_CONTENTS_ITEM  "
	                 "       (cont_gu       ,id          "
	                 "       ,seq_no        ,item_code   "
	                 "       ,item_color				 "
	                 "       ,reg_date      ,reg_time)   "
	                 "SELECT  cont_gu       ,%ld         "
	                 "       ,seq_no        ,item_code   "
	                 "       ,item_color				 "
	                 "       ,'%s'          ,'%s'        "
	                 "  FROM T_CONTENTS_TEMPITEM  "
	                 " WHERE id  =  %d              "
	                 ,dwId
	                 //seq_no
	                 //item_code
	                 //item_color
	                 ,reg_date
	                 ,reg_time
	                 ,pfups4004->id);

	if (mysql_query(con, szQuery)){
		ErrNum = -400424;
		sprintf(ErrMsg, "컨텐츠아이템정보를 생성 하지 못 하였습니다.\n");
		goto fups4004_tran_err;
    }
*/
	//--------------------------------------------------------------------------
	// 컨텐츠생성정보 INSERT
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "INSERT INTO T_CONTENTS_CREATE  "
	                 "       ( id,  cont_gu, udt_cd )       "
	                 "VALUES ( %d,  '01'   , 'I'    )       "
	                 ,dwId
	                 );
	if (mysql_query(con, szQuery)){
		ErrNum = -400403;
		sprintf(ErrMsg, "컨텐츠정보를 생성 하지 못 하였습니다.\n");
		goto fups4004_tran_err;
    }
    #ifdef _DEBUG_
	printf("fups4004-> INSERT T_CONTENTS_CREATE OK\n");
	#endif

	//--------------------------------------------------------------------------
	// 서버정보용량 UPDATE
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "UPDATE T_SERVER_INFO         "
	                 "   SET disk_use  = disk_use + %13.0f"
	                 " WHERE server_id = '%s'             "
	                 ,pfups4004->file_size
	                 ,pfups4004->server_id
	                 );
	if (mysql_query(con, szQuery)){
		ErrNum = -400404;
		sprintf(ErrMsg, "서버용량 처리를 하지 못 했습니다.\n");
		goto fups4004_tran_err;
    }

	//--------------------------------------------------------------------------
	// 임시컨턴츠정보
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "DELETE FROM T_CONTENTS_TEMP  "
	                 " WHERE id = %ld                     "
	                 ,pfups4004->id
	                 );
	if (mysql_query(con, szQuery))
	{
		sprintf(ErrMsg, "임시컨턴츠정보를 삭제하지 못 했습니다.\n");
		infLOG(ERROR, "Exception ) (%ld) fups4004-> 임시컨턴츠정보를 삭제하지 못 했습니다.\n",dwId);
		infLOG(ERROR, "Exception ) fups4004-> [%d](%s)\n",ErrNum, ErrMsg);
		infLOG(ERROR, "Exception ) fups4004-> [%d](%s)\n",mysql_errno(con), mysql_error(con));
	}

	//--------------------------------------------------------------------------
	// 임시컨턴츠파일리스트정보
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "DELETE FROM T_CONTENTS_TEMPLIST "
	                 " WHERE id = %ld                     "
	                 ,pfups4004->id
	                 );
	if (mysql_query(con, szQuery))
	{
		sprintf(ErrMsg, "임시컨턴츠파일리스트를 삭제 하지 못 했습니다.\n");
		infLOG(ERROR, "Exception ) (%ld) fups4004-> 임시컨턴츠파일리스트를 삭제하지 못 했습니다.\n",dwId);
		infLOG(ERROR, "Exception ) fups4004-> [%d](%s)\n",ErrNum, ErrMsg);
		infLOG(ERROR, "Exception ) fups4004-> [%d](%s)\n",mysql_errno(con), mysql_error(con));
	}

	//--------------------------------------------------------------------------
	// 임시컨턴츠정보
	//--------------------------------------------------------------------------
	/*
	memset (szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery, "DELETE FROM T_CONTENTS_TEMPIMG "
	                 " WHERE id = %ld                     "
	                 ,pfups4004->id
	                 );
	if (mysql_query(con, szQuery))
	{
		sprintf(ErrMsg, "임시컨턴츠이미지정보를 삭제하지 못 했습니다.\n");
		infLOG(ERROR, "Exception ) (%ld) fups4004-> 임시컨턴츠정보를 삭제하지 못 했습니다.\n",dwId);
		infLOG(ERROR, "Exception ) fups4004-> [%d](%s)\n",ErrNum, ErrMsg);
		infLOG(ERROR, "Exception ) fups4004-> [%d](%s)\n",mysql_errno(con), mysql_error(con));
	}
*/
	if (tran_commit(con)!=0)
	{
		ErrNum = -400493;
		sprintf(ErrMsg, "테이베이스 오류입니다.\n");
		goto fups4004_tran_err;
	}

	//db_disconnect(con);

	//infLOG(ALWAY," ] cfups4004 등록 성공\n");


	req_header.nCmd = 1 ;
	pSendData = new char[HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

   	return HEADER_SIZE;


//------------------------------------------------------------------------------
fups4004_err:
    infLOG(ERROR, "Exception ) fups4004-> [%d](%s)\n",ErrNum, ErrMsg);
	infLOG(ERROR, "fups4004-> [%d](%s)\n",mysql_errno(con), mysql_error(con));

	tran_end(con);

	if( bCloseDB )
		db_disconnect(con);


	req_header.nCmd = -1 ;
	pSendData = new char [HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

   	return HEADER_SIZE;



fups4004_tran_err:
    infLOG(ERROR, "Exception ) fups4004-> [%d](%s)\n",ErrNum, ErrMsg);
	infLOG(ERROR, "Exception ) fups4004-> [%d](%s)\n",mysql_errno(con), mysql_error(con));

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

