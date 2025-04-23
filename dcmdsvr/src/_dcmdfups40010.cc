/******************************************************************************
 *   서브시스템 : CMD서버
 *   프로그램명 : dcmffups4001.cc
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
#include "/home/ezwon/zangsi_with_dcmd/fupsvr/inc/fups40010.h"
#include "dcmdfups40010.h"
#include "dcmdcomlib.h" //업로드 fupcomlib
//#define  _DEBUG_

#include "mysql_pool.h"

extern CMysqlPool * m_g_clMysqlPool;

long dcmdfups40010(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData)//CFUPS4001 pfups4001)
{

	LPCFUPS4001 pfups4001 = (LPCFUPS4001)pRecvData;

	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);

	char szQuery[10000], szQuery1[10000];
	MYSQL *con;
	MYSQL_RES   *res;
	MYSQL_ROW    row;
	char ErrMsg[256];    // error message
	int  ErrNum;         // error no
	long reg_seq = 1;
    char szOldFile[768];
    char szNewFile[768];

    memset(szOldFile,0x00,sizeof(szOldFile));
    memset(szNewFile,0x00,sizeof(szNewFile));

	unsigned long dwId=0;
	unsigned long bomul_id=0;
	unsigned long req_id=0;
	char szReqUser     [16];
	char req_title[512];
	char disp_end_date [ 8+1];
	char disp_end_time [ 6+1];
	char reg_user      [12+1];
	char reg_date      [ 8+1];
	char reg_time      [ 6+1];
	char up_st_date    [ 8+1];
	char up_st_time    [ 6+1];
	char file_name2   [255+1];  // 로컬파일이름
	char cont_gu   [2+1];  // 로컬파일이름
	char item_color[16];
	char item_bold_yn[2];

	char bomul_gu[2];
	unsigned int bomul_amt = 0;
	char bomul_item_code[2];
	unsigned int bomul_prize_num = 0;

	unsigned int item_bold_cnt = 0;
	unsigned int item_color_cnt = 0;
	unsigned int item_bomul_cnt = 0;

	memset(szReqUser,0x00,sizeof(szReqUser));
	memset(req_title,0x00,sizeof(req_title));
	memset(&disp_end_date,0x00,sizeof(disp_end_date));
	memset(&disp_end_time,0x00,sizeof(disp_end_time));
	memset(&reg_user     ,0x00,sizeof(reg_user));
	memset(&reg_date     ,0x00,sizeof(reg_date));
	memset(&reg_time     ,0x00,sizeof(reg_time));
	memset(&up_st_date   ,0x00,sizeof(up_st_date));
	memset(&up_st_time   ,0x00,sizeof(up_st_time));
	memset(&file_name2   ,0x00,sizeof(file_name2));
	memset(&cont_gu      ,0x00,sizeof(cont_gu));
	memset(&item_color      ,0x00,sizeof(item_color));
	memset(&item_bold_yn      ,0x00,sizeof(item_bold_yn));
	memset(bomul_gu ,0x00,sizeof(bomul_gu));
	memset(bomul_item_code ,0x00,sizeof(bomul_gu));

	memset(&ErrMsg   , 0x00, sizeof(ErrMsg   )); // 오류메시지

	#ifdef _DEBUG_
	printf("fups4001-> fups4001 start\n");
	printf("id           (%d)\n", pfups4001->id           );
	printf("title        (%s)\n", pfups4001->title        );
	printf("descript     (%s)\n", pfups4001->descript     );
	printf("keyword      (%s)\n", pfups4001->keyword      );
	printf("sect_code    (%s)\n", pfups4001->sect_code    );
	printf("share_meth   (%s)\n", pfups4001->share_meth   );
	printf("disp_end_date(%s)\n", pfups4001->disp_end_date);
	printf("disp_end_time(%s)\n", pfups4001->disp_end_time);
	printf("disp_stat    (%s)\n", pfups4001->disp_stat    );
	printf("file_del_yn  (%s)\n", pfups4001->file_del_yn  );
	printf("folder_yn    (%s)\n", pfups4001->folder_yn    );
	printf("server_id    (%s)\n", pfups4001->server_id    );
	printf("file_path    (%s)\n", pfups4001->file_path    );
	printf("file_name1   (%s)\n", pfups4001->file_name1   );
	printf("file_name2   (%s)\n", pfups4001->file_name2   );
	printf("file_type    (%s)\n", pfups4001->file_type    );
	printf("up_st_date   (%s)\n", pfups4001->up_st_date   );
	printf("up_st_time   (%s)\n", pfups4001->up_st_time   );
	printf("reg_user     (%s)\n", pfups4001->reg_user     );
	printf("reg_date     (%s)\n", pfups4001->reg_date     );
	printf("reg_time     (%s)\n", pfups4001->reg_time     );
    printf("price_amt    (%d)\n", pfups4001->price_amt    );
    printf("won_mega     (%d)\n", pfups4001->won_mega     );
	printf("file_size    (%13.0f)\n", pfups4001->file_size);
	printf("file_resoX   (%d)\n", pfups4001->file_resoX   );
	printf("file_resoY   (%d)\n", pfups4001->file_resoY   );
	printf("dsp_file_cnt (%d)\n", pfups4001->dsp_file_cnt );
	printf("down_cnt     (%d)\n", pfups4001->down_cnt     );
	#endif


	#ifdef _DEBUG_
	printf("pfups4001->file_name2   (%s) file_name2 (%s)\n", pfups4001->file_name2,file_name2   );
	#endif
	infLOG(ALWAY,"cfups4001 frist input ) pfups4001->file_name2   (%s) file_name2 (%s)\n", pfups4001->file_name2,file_name2   );



	if (pfups4001->id == 0)
	{
		sprintf(ErrMsg, "등록번호가 없습니다..\n");
	    infLOG(ERROR, "fups4001-> [%d](%s)\n",ErrNum, ErrMsg);
//		infLOG(ERROR, "fups4001-> [%d](%s)\n",mysql_errno(con), mysql_error(con));

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


	CMysqlCon MysqlCon(m_g_clMysqlPool,m_g_clMysqlPool->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );

	con = MysqlCon.GetMysqlCon();


	if (con == NULL )
	{
		ErrNum = -400192;
		sprintf(ErrMsg, "DB에 접속하지 못 하였습니다.\n");
		#ifdef __DEBUG
		printf("DB에 접속하지 못 하였습니다.\n");
		#endif
		infLOG(ERROR, "GetMysqlCon is null \n");

			int nRetry = 0;
		while (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			#ifdef __DEBUG_
			printf(" ] DB 접속 재시도 \n");
			#endif
			infLOG(ERROR, "FUPS40010 | GetMysqlCon is null [%d](%s)\n",ErrNum, ErrMsg);
			//infLOG(ERROR, "FUPS40010 | Mysql Error | Errno : %d | ErrMsg : %s \n\n", mysql_errno(con),mysql_error(con));
		}

		if( nRetry >= 5)
		{

			req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);

			infLOG(ERROR, "FUPS40010 | Cannot DB Connect \n");

	       	return HEADER_SIZE;
	    }
	    bCloseDB = true;

	}

	//--------------------------------------------------------------------------
	// 등록일자 얻기
	//--------------------------------------------------------------------------


	memset(szQuery,  0x00, sizeof(szQuery));
	memset(szQuery1, 0x00, sizeof(szQuery1));
	strcpyA(szQuery, "SELECT date_format(now(),'%Y%m%d')");
	strcat(szQuery, "     , date_format(now(),'%H%i%s')");
	/*
	strcat(szQuery, "     , if( sect_code = '04' or sect_code = '10' or sect_code = '12'  "
				"         , date_format(date_add(now(),INTERVAL 60 DAY),'%Y%m%d') "
				"		  , if( sect_code = '06' or sect_code = '09' or sect_code = '13' or sect_code = '14' "
				"		  , date_format(date_add(now(),INTERVAL 90 DAY),'%Y%m%d') "
				"		  , date_format(date_add(now(), INTERVAL 30 DAY),'%Y%m%d')))" );
	//strcat(szQuery, "     , date_format(date_add(now(), INTERVAL 30 DAY),'%Y%m%d')");
	*/
	strcat(szQuery, "	  , date_format(date_add(now(), INTERVAL notice_term DAY),'%Y%m%d')");
	strcat(szQuery, "     , date_format(now(),'%H%i%s')");
	strcat(szQuery, "     , reg_user   as reg_user   ");
	strcat(szQuery, "     , reg_date   as up_st_date ");
	strcat(szQuery, "     , reg_time   as up_st_time ");
	strcat(szQuery, "     , file_name2 as file_name2 ");
	strcat(szQuery, "     , bomul_id   as bomul_id   ");
	strcat(szQuery, "     , req_id   as req_id   ");
	strcat(szQuery, "     , cont_gu   as cont_gu   ");
	strcat(szQuery, "     , item_color   as item_color   ");
	strcat(szQuery, "     , item_bold_yn as item_bold_yn   ");
	strcat(szQuery, "     , bomul_gu ,bomul_amt, bomul_item_code , bomul_prize_num ");
	strcat(szQuery, "  FROM zangsi.T_CONTENTS_TEMP   ");
	sprintf(szQuery1, " WHERE id  =  %ld ", pfups4001->id);
	strcat(szQuery, szQuery1);

	if (mysql_query(con, szQuery)){
		ErrNum = -400101;
		sprintf(ErrMsg, "테이베이스 오류입니다.\n");
		goto fups4001_err;
	}

	if (!(res = mysql_store_result(con)))
	{
		ErrNum = -400102;
		sprintf(ErrMsg, "테이베이스 오류입니다.\n");
		goto fups4001_err;
	}
 	if (mysql_num_rows(res)==0)
 	{
		ErrNum = -400102;
		sprintf(ErrMsg, "검색된 자료가 없습니다.\n");
		mysql_free_result(res);
		goto fups4001_err;
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
	memset(&cont_gu    , 0x00, sizeof(cont_gu     ));
	memset(&item_color    , 0x00, sizeof(item_color     ));
	memset(&item_bold_yn    , 0x00, sizeof(item_bold_yn     ));
	memset(bomul_gu ,0x00,sizeof(bomul_gu));
	memset(bomul_item_code ,0x00,sizeof(bomul_gu));

	strcpyA(reg_date     , getstr(row, 0));
	strcpyA(reg_time     , getstr(row, 1));
	strcpyA(disp_end_date, getstr(row, 2));
	strcpyA(disp_end_time, getstr(row, 3));
	strcpyA(reg_user     , getstr(row, 4));
	strcpyA(up_st_date   , getstr(row, 5));
	strcpyA(up_st_time   , getstr(row, 6));
	strcpyA(file_name2   , getstr(row, 7));
	bomul_id = (unsigned long)(getnum(row, 8));
	req_id = (unsigned long)(getnum(row, 9));
	strcpyA(cont_gu   , getstr(row, 10));
	strcpyA(item_color   , getstr(row, 11));
	strcpyA(item_bold_yn   , getstr(row, 12));
	strcpyA(bomul_gu   , getstr(row, 13));
	bomul_amt   = (unsigned int)getint(row, 14);
	strcpyA(bomul_item_code   , getstr(row, 15));
	bomul_prize_num   = (unsigned int)getint(row, 16);

	#ifdef _DEBUG_
	printf("pfups4001->file_name2   (%s) file_name2 (%s)\n", pfups4001->file_name2,file_name2   );
	#endif
	infLOG(ALWAY,"cfups4001 second input ) pfups4001->file_name2   (%s) file_name2 (%s)\n", pfups4001->file_name2,file_name2   );


	mysql_free_result(res);





	if( strcmp(cont_gu , "WE") == 0)
	{
		infLOG(ALWAY,"위디스크 업로드 DB 등록 시작 \n");




		//--------------------------------------------------------------------------
		// 트렌젝션시작
		//--------------------------------------------------------------------------
		if (tran_begin(con)!=0)
		{
			ErrNum = -400192;
			sprintf(ErrMsg, "위디스크  트렌젝션 오류입니다.\n");
			goto fups4001_err;
		}







		//--------------------------------------------------------------------------
		// 컨텐츠 가상 아이디 생성
		//--------------------------------------------------------------------------
		memset(szQuery , 0x00, sizeof(szQuery ));
		sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_DSP ( id ,server_id ) "
						 " SELECT real_cont_id , '%s' from zangsi.T_CONTENTS_TEMP where id  =  %ld ",  pfups4001->server_id , pfups4001->id);

		if (mysql_query(con, szQuery))
		{
			ErrNum = -400100;
			strcpyA(ErrMsg, "분산 데이터 정보를 입력하지 못하였습니다.\n");
			goto fups4001_tran_err;
	    }

		#ifdef _DEBUG_
		printf("Query : %s \n",szQuery);
		#endif


		//--------------------------------------------------------------------------
		// 서버정보용량 UPDATE
		//--------------------------------------------------------------------------
		memset (szQuery , 0x00, sizeof(szQuery ));
		sprintf(szQuery, "UPDATE zangsi.T_SERVER_INFO         "
		                 "   SET disk_use  = disk_use + %13.0f"
	                 	"  ,real_disk_use  = real_disk_use + %13.0f"
		                 " WHERE server_id = '%s'             "
		                 ,pfups4001->file_size
		                 ,pfups4001->file_size
		                 ,pfups4001->server_id
		                 );
		if (mysql_query(con, szQuery))
		{
			ErrNum = -400104;
			sprintf(ErrMsg, "위디스크 서버용량 처리를 하지 못 했습니다.\n");
			infLOG(ERROR, "Exception ) fups4001-> [%d](%s)\n",ErrNum, ErrMsg);
			infLOG(ERROR, "Exception ) fups4001-> [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    }
		#ifdef _DEBUG_
		printf("Query : %s \n",szQuery);
		#endif
		//--------------------------------------------------------------------------
		// 컨텐츠정보 번호발번
		//--------------------------------------------------------------------------
		memset(szQuery , 0x00, sizeof(szQuery ));
		sprintf(szQuery, "UPDATE zangsi.T_CONTENTS_TEMP set end_dsp_file_cnt  = end_dsp_file_cnt + 1  WHERE id  =  %ld ", pfups4001->id);

		if (mysql_query(con, szQuery))
		{
			ErrNum = -400111;
			strcpyA(ErrMsg, "분산 데이터 정보를 처리하지 못하였습니다.\n");
			infLOG(ERROR, "Exception ) fups4001-> [%d](%s)\n",ErrNum, ErrMsg);
			infLOG(ERROR, "Exception ) fups4001-> [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    }
		// 등록된 ID를 얻는다.
		#ifdef _DEBUG_
		printf("Query : %s \n",szQuery);
		#endif

		//--------------------------------------------------------------------------
		// 임시컨턴츠정보
		//--------------------------------------------------------------------------
		memset (szQuery , 0x00, sizeof(szQuery ));
		sprintf(szQuery, "DELETE FROM zangsi.T_CONTENTS_TEMP  "
		                 " WHERE id = %ld   and dsp_file_cnt = end_dsp_file_cnt      "
		                 ,pfups4001->id
		                 );
		if (mysql_query(con, szQuery))
		{
			sprintf(ErrMsg, "임시컨턴츠정보를 삭제하지 못 했습니다.\n");
			infLOG(ERROR, "Exception ) (%ld) fups4001-> 임시컨턴츠정보를 삭제하지 못 했습니다.\n",dwId);
			infLOG(ERROR, "Exception ) fups4001-> [%d](%s)\n",ErrNum, ErrMsg);
			infLOG(ERROR, "Exception ) fups4001-> [%d](%s)\n",mysql_errno(con), mysql_error(con));
		}
		#ifdef _DEBUG_
		printf("Query : %s \n",szQuery);
		#endif
	    //infLOG(ERROR, "위디스크자료실 파일 이름 확인 : old file ( %s ) new file ( %s )  \n",szOldFile,szNewFile);

		/*

		if( rename(szOldFile,szNewFile) != 0)
		{
			ErrNum = -400105;
			sprintf(ErrMsg, " 파일 오류 [ %ld ]  ",pfups4001->id);
			goto fups4001_tran_err;
		}
		*/



		if (tran_commit(con)!=0)
		{
			ErrNum = -400193;
			sprintf(ErrMsg, "위디스크 테이베이스 전송 오류입니다.\n");
			goto fups4001_tran_err;
		}

	}
	else
	{
		ErrNum = -400194;
		sprintf(ErrMsg, "일치하는 코드가 없습니다.\n");
		infLOG(ALWAY,"cfups4001 ( %ld ) 일치 하는 cont_gu (%s) 가 없습니다.\n", pfups4001->id,cont_gu);
		goto fups4001_err;
	}
	if( bCloseDB )
		db_disconnect(con);

	infLOG(ALWAY," ] cfups4001 등록 성공 ( %d ) \n",dwId);


	req_header.nCmd = 40010 ;
	pSendData = new char[HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

   	return HEADER_SIZE;


//------------------------------------------------------------------------------
fups4001_err:
    infLOG(ERROR, "Exception ) fups4001-> [%d](%s)\n",ErrNum, ErrMsg);
	infLOG(ERROR, "fups4001-> [%d](%s)\n",mysql_errno(con), mysql_error(con));

	tran_end(con);

	if( bCloseDB )
		db_disconnect(con);


	req_header.nCmd = -1 ;
	pSendData = new char [HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

   	return HEADER_SIZE;



fups4001_tran_err:
    infLOG(ERROR, "Exception ) fups4001-> [%d](%s)\n",ErrNum, ErrMsg);
	infLOG(ERROR, "Exception ) fups4001-> [%d](%s)\n",mysql_errno(con), mysql_error(con));

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

