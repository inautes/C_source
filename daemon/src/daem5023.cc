/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5023.cc
 *         기능 : 내디스크삭제처리
 *         설명 : 1. 프로세스 처리 예정시간 - 04:00이후 daem5002 완료 이후에 수행.
 *                2. 구매기간이 만료된 자료중(만료후+4일)에 해당되는 자료중
 *                   만료일자의 용량과 현시점의 용량을 비교하여 작을경우는
 *                   삭제 대상이 된다.
 *                3. 삭제 대상중 현재 사용량이 용량보다 초과하는 경우는 파일을
 *                   삭제하게 된다.
 *                4. 내디스크 정책 변경으로 현재 사용하지 않음.
 *     설치위치 : CMD01
 *
 *       작성자 : JDP
 *       작성일 : 2004/02/16
 *     수정이력 : 2009/07/31 - HCS
 					수정 내용 : 만료후 + 30일로 삭제일 변경
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#include <mysql.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>

#include "daemcom.h"
#include "commydb.h"

#define  MAX_ROWS	1
#define _DEBUG_

int    daem5023_init_process(int argc, char **argv);
int    daem5023_main_process();
int    daem5023_term_process();
int    daem5023_select_user();
int    daem5023_delete_data();
int    daem5023_delete_end();
int    daem5023_select_data_info();
int    daem5023_select_data_myinfo();
int    daem5023_delete_data_info();
int    daem5023_delete_data_myinfo();
int    daem5023_select_del_date();
void   daem5023_signal(int nSignal);

MYSQL     *con, *con2;
MYSQL_RES *res;
MYSQL_ROW  row;

char   gproc_date [  8+1];	// 처리일자
char   ged_date   [  8+1];	// 만료일자
char   greg_user  [ 12+1];	// 직접입력
char   guser_id   [ 12+1];	// 사용자ID
int    gseq_no           ;	// 디스크일련번호
double gdisk_use         ;	// 디스크사용량
double gdisk_size        ;  // 디스크보류량(D-4)
double gcur_disk_size    ;  // 디스크보류량(현재)
double gdelete_size      ;	// 삭제한 파일크기
char   gdel_date  [  8+1];	// 삭제예정일자

unsigned long gid     = 0;  // 등록ID
double   gfile_size   = 0;	// 파일크기
int      gdown_cnt    = 0;  // 다운로드수

//******************************************************************************
//* daem5023 main
//******************************************************************************
int daem5023_main_process()
{
	char szQuery[1000];  // query string
	int i, j, count;
	int nRowcnt=0;
	int ret = 0;

    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "daem5023_main_process: gproc_date[%s]\n", gproc_date);  
    ZzLOG(ALWAY, "daem5023_main_process: ged_date  [%s]\n", ged_date  );  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*  - 처리일자 -4일 의 자료를 검색한다.                                   */
	/*  - del_date is null 인자료는 아직 처리하지 않은 자료임.                */
	/*------------------------------------------------------------------------*/
	while(1) {
		
		//----------------------------------------------------------------------
		// 트렌젝션시작
		//----------------------------------------------------------------------
		if (tran_begin(con)!=0) {
		    ZzLOG(ERROR, "daem5023_main_process: tran_begin: 테이베이스 오류입니다.\n");  
			ZzLOG(ERROR, "daem5023_main_process: [%d](%s)\n", mysql_errno(con), mysql_error(con));
		    ret = -1;
			break;
		}

	    ZzLOG(ALWAY, "daem5023_main_process: tran_begin OK\n");

		//----------------------------------------------------------------------
		// 1명에 대한 변수초기화
		//----------------------------------------------------------------------
		gdelete_size = 0.0;		// 삭제처리한 size

		//----------------------------------------------------------------------
		// 처리할 사용자 1명 select
		//----------------------------------------------------------------------
		ret = daem5023_select_user();
		if (ret ==  9) 
			break;	// 처리할 자료가 없습니다.
		if (ret == -1) 
			break;	// 오류발생

		//----------------------------------------------------------------------
		// 자료실 삭제처리
		//----------------------------------------------------------------------
		ret = daem5023_delete_data();
		if (ret == -1) break;	// 오류발생

		//----------------------------------------------------------------------
		// 삭제처리 삭제완료처리
		//----------------------------------------------------------------------
		ret = daem5023_delete_end();
		if (ret == -1) break;	// 오류발생

		//----------------------------------------------------------------------
		// commit
		//----------------------------------------------------------------------
		if (commit(con)!=0){
		    ZzLOG(ERROR, "[daem5023_main_process] commit 1 error...\n");
			ZzLOG(ERROR, "[daem5023_main_process] [%d](%s)\n", mysql_errno(con), mysql_error(con));
		    ret = -1;
			break;
		}
	}		

	if (ret == -1) {
		tran_rollback(con);
		tran_end(con);
		return -1;	
	}

    ZzLOG(ALWAY, "daem5023_main_process: 정상적으로 처리 완료!!!\n");
	return  0;
}


//******************************************************************************
//* daem5023_select_user
//******************************************************************************
int daem5023_select_user()
{
	char szQuery[2000];		// query string

    //ZzLOG(ALWAY, "daem5023_select_user: start!!!\n");

	//--------------------------------------------------------------------------
	// 디스크 종료일자가 처리일자 -4일 인 자료중 1건을 select한다.
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	if (strcmp(greg_user, "")==0) 
	{
	    ZzLOG(ALWAY, "daem5023_select_user: reg_user is null!!!\n");
		sprintf(szQuery, "SELECT a.user_id   , a.seq_no    "
		                 "     , b.disk_size , b.disk_use  "
		                 "  FROM zangsi.T_MYDISK_PAYMENT a "
		                 "     , zangsi.T_MYDATA_INFO    b "
		                 " WHERE a.ed_date   = '%s'        "
		                 "   AND a.del_date  is null       "
		                 "   AND a.user_id   = b.user_id   "
		                 "  LIMIT 1                        "
	                     , ged_date
	                     );
	}
	else 
	{
	    ZzLOG(ALWAY, "daem5023_select_user: reg_user is (%s)!!!\n", greg_user);
		sprintf(szQuery, "SELECT a.user_id   , a.seq_no    "
		                 "     , b.disk_size , b.disk_use  "
		                 "  FROM zangsi.T_MYDISK_PAYMENT a "
		                 "     , zangsi.T_MYDATA_INFO    b "
		                 " WHERE a.user_id   = '%s'        "
		                 "   AND a.ed_date   = '%s'        "
		                 "   AND a.del_date  is null       "
		                 "   AND a.user_id   = b.user_id   "
		                 "  LIMIT 1                        "
		                 , greg_user
	                     , ged_date
	                     );
	}
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5023_select_user: mysql_query error...\n");
		ZzLOG(ERROR, "daem5023_select_user: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) 
	{
	    ZzLOG(ERROR, "daem5023_select_user: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem5023_select_user: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
 	if (mysql_num_rows(res)==0)	
 	{
		mysql_free_result(res);
		return 9;
	}

	memset(&guser_id      , 0x00, sizeof(guser_id      ));
	memset(&gseq_no       , 0x00, sizeof(gseq_no       ));
	memset(&gcur_disk_size, 0x00, sizeof(gcur_disk_size));
	memset(&gdisk_use     , 0x00, sizeof(gdisk_use     ));
	if (row = mysql_fetch_row(res))
	{
		strcpy(guser_id  , getstr(row, 0));
		gseq_no          = getint(row, 1);
		gcur_disk_size   = getnum(row, 2);
		gdisk_use        = getnum(row, 3);
	}
	mysql_free_result(res);
	
	//--------------------------------------------------------------------------
	// now() - 4일의 용량기준으로 삭제처리를 한다.
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT ifnull(sum(buy_size),0) + 31457280 "
	                 "  FROM zangsi.T_MYDISK_PAYMENT  "
	                 " WHERE user_id = '%s'           "
	                 , guser_id
	                 );
	strcat (szQuery, "  AND ed_date > date_format(date_add(now(), INTERVAL -4 DAY),'%Y%m%d')");

	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5023_select_user: mysql_query error...\n");
		ZzLOG(ERROR, "daem5023_select_user: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem5023_select_user: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem5023_select_user: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
 	if (mysql_num_rows(res)==0)	{
		mysql_free_result(res);
		return 9;
	}

	memset(&gdisk_size, 0x00, sizeof(gdisk_size));
	if (row = mysql_fetch_row(res))
	{
		gdisk_size       = getnum(row, 0);
	}
	mysql_free_result(res);
	
    ZzLOG(ALWAY, "daem5023_select_user: guser_id  [%s]\n", guser_id);
    ZzLOG(ALWAY, "daem5023_select_user: gseq_no   [%d]\n", gseq_no);
    ZzLOG(ALWAY, "daem5023_select_user: gdisk_size[%15.0f][%15.0f]\n", gdisk_size, gcur_disk_size);
    ZzLOG(ALWAY, "daem5023_select_user: gdisk_use [%15.0f]\n", gdisk_use);
	
	return 0;
}


//******************************************************************************
//* daem5023_delete_data (공유자료실 삭제처리)
//******************************************************************************
int daem5023_delete_data()
{
	int ret = 0;

    //ZzLOG(ALWAY, "daem5023_delete_data: start!!!\n");

	//--------------------------------------------------------------------------
	// 공개자료실 삭제	
	//--------------------------------------------------------------------------
/*
	while(1) 
	{
		
		// 디스크보유량이 디스크사용량이 보다 크거나 같으면 cancel

		// 삭제할 대상 1건 select
		ret = daem5023_select_data_info();
		if (ret != 0) break;
		
		// 삭제처리
		ret = daem5023_delete_data_info();
		if (ret != 0) break;
		
		//gdelete_size = gdelete_size + gfile_size;
	}
	
	if (ret == -1) 
		return ret;
*/	
	//--------------------------------------------------------------------------
	// 내자료실 삭제	
	//--------------------------------------------------------------------------
	while(1) 
	{
		
		// 디스크보유량이 디스크사용량이 보다 크거나 같으면 cancel
		if ( gdisk_size >= (gdisk_use - gdelete_size) )
			break;

		// 삭제할 대상 1건 select
		ret = daem5023_select_data_myinfo();
		if (ret != 0) break;
		
		// 삭제처리
		ret = daem5023_delete_data_myinfo();
		if (ret != 0) break;
		
		gdelete_size = gdelete_size + gfile_size;
	}
	
	return ret;	
}


//******************************************************************************
//* daem5023_select_data(공개자료실 삭제할 대상 1건 select)
//******************************************************************************
int daem5023_select_data_info()
{
	char szQuery[1000];		// query string

    //ZzLOG(ALWAY, "daem5023_select_data_info: start!!!\n");
	memset(&gid       , 0x00, sizeof(gid       ));
	memset(&gfile_size, 0x00, sizeof(gfile_size));
	memset(&gdown_cnt , 0x00, sizeof(gdown_cnt ));
	//--------------------------------------------------------------------------
	// 공개자료 1건 select
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT a.id, a.file_size, fix_down_cnt + down_cnt"
	                 "  FROM zangsi.T_CONTDATA_FILE a "
	                 " WHERE a.reg_user = '%s'        "
	                 " ORDER BY a.id "
	                 " LIMIT 1 "
	                 , guser_id);

	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5023_select_data_info mysql_query error...\n");
		ZzLOG(ERROR, "daem5023_select_data_info: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem5023_select_data_info: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem5023_select_data_info: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
 	if (mysql_num_rows(res)==0)	{
		mysql_free_result(res);
		return 9;
	}
	if (row = mysql_fetch_row(res))
	{
		gid        = getint(row, 0);
		gfile_size = getnum(row, 1);
		gdown_cnt  = getint(row, 2);
		
	    ZzLOG(ALWAY, "daem5023_select_data_info: gid       [%ld]\n"   , gid);
	    ZzLOG(ALWAY, "daem5023_select_data_info: gfile_size[%15.0f]\n", gfile_size);
	    ZzLOG(ALWAY, "daem5023_select_data_info: gdown_cnt [%ld]\n"   , gdown_cnt);

		
	}
	mysql_free_result(res);
	return 0;
}

//******************************************************************************
//* daem5023_select_data_myinfo(내자료실 삭제할 대상 1건 select)
//******************************************************************************
int daem5023_select_data_myinfo()
{
	char szQuery[1000];		// query string

    //ZzLOG(ALWAY, "daem5023_select_data_myinfo: start!!!\n");

	memset(&gid       , 0x00, sizeof(gid       ));
	memset(&gfile_size, 0x00, sizeof(gfile_size));
	//--------------------------------------------------------------------------
	// 공개자료 1건 select
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT a.id, a.file_size          "
	                 "  FROM zangsi.T_CONTDATA_MYFILE a "
	                 " WHERE a.reg_user = '%s'          "
	                 " ORDER BY a.id "
	                 " LIMIT 1 "
	                 ,guser_id);

	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5023_select_data_myinfo: mysql_query error...\n");
		ZzLOG(ERROR, "daem5023_select_data_myinfo: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem5023_select_data_myinfo: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem5023_select_data_myinfo: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
 	if (mysql_num_rows(res)==0)	{
		mysql_free_result(res);
		return 9;
	}
	if (row = mysql_fetch_row(res))
	{
	
		gid        = getint(row, 0);
		gfile_size = getnum(row, 1);
	}
	mysql_free_result(res);
	return 0;
}


//******************************************************************************
//* daem5023_delete_data(공개자료실 삭제처리)
//******************************************************************************
int daem5023_delete_data_info()
{
	char szQuery[5000];		// query string

	//ZzLOG(ALWAY, "daem5023_delete_data_info: start!!!\n");


	if (gdown_cnt > 0) {
		if (daem5023_select_del_date() != 0)
			return  -1;
	}
	else {
		memset(&gdel_date, 0x00, sizeof(gdel_date));
		strcpy( gdel_date, "00000000");
	}

    ZzLOG(ALWAY, "daem5023_delete_data_info: gdel_date(%s)\n", gdel_date);


	//--------------------------------------------------------------------------
	// T_CONTENTS_DEL insert
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_DEL "
	                 "     ( cont_gu                    "
	                 "     , id           , folder_yn   "
	                 "     , server_id    , del_date    "
	                 "     , file_path    , file_name1  "
	                 "     , file_name2   , file_size   "
	                 "     , file_type    , reg_user    "
	                 "     , reg_date     , reg_time)   "
	                 "SELECT 'MY'                       "
	                 "     , a.id         , a.folder_yn "
	                 "     , a.server_id  , '%s'        "
	                 "     , a.file_path  , a.file_name1"
	                 "     , a.file_name2 , a.file_size "
	                 "     , a.file_type  , a.reg_user  "
	                 "     , a.reg_date   , a.reg_time  "
	                 "  FROM zangsi.T_CONTDATA_FILE a   "
	                 " WHERE a.id            = %ld      "
	                 ,gdel_date
	                 ,gid
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5023_delete_data_info: INSERT T_CONTENTS_DEL error...\n");
		ZzLOG(ERROR, "daem5023_delete_data_info: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
    }

	//--------------------------------------------------------------------------
	// 공개자료실 컨텐츠정보 삭제
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi.T_CONTDATA_INFO WHERE id = %ld ", gid);
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5023_delete_data_info: UPDATE T_CONTENTS_INFO error...\n");
		ZzLOG(ERROR, "daem5023_delete_data_info: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
    }

	//--------------------------------------------------------------------------
	// 공개자료실 파일정보 삭제
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi.T_CONTDATA_FILE WHERE id = %ld ", gid);
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5023_delete_data_info: UPDATE T_CONTDATA_FILE error...\n");
		ZzLOG(ERROR, "daem5023_delete_data_info: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
    }
	/*
	//--------------------------------------------------------------------------
	// 공개자료실 메모정보 삭제
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi.T_CONTDATA_MEMO WHERE id = %ld ", gid);
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5023_delete_data_info: UPDATE T_CONTDATA_MEMO error...\n");
		ZzLOG(ERROR, "daem5023_delete_data_info: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
    }
    */
    ZzLOG(ALWAY, "daem5023_delete_data_info: DELETE OK\n");

	return 0;
}


//******************************************************************************
//* daem5023_delete_mydata(내자료실 삭제처리)
//******************************************************************************
int daem5023_delete_data_myinfo()
{
	char szQuery[1000];		// query string

	//ZzLOG(ALWAY, "daem5023_delete_data_myinfo: start!!!\n");

	//--------------------------------------------------------------------------
	// T_CONTENTS_DEL insert
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_DEL "
	                 "     ( cont_gu                    "
	                 "     , id           , folder_yn   "
	                 "     , server_id    , del_date    "
	                 "     , file_path    , file_name1  "
	                 "     , file_name2   , file_size   "
	                 "     , file_type    , reg_user    "
	                 "     , reg_date     , reg_time)   "
	                 "SELECT 'MD'                       "
	                 "     , a.id         , a.folder_yn "
	                 "     , a.server_id  , '00000000'  "
	                 "     , a.file_path  , a.file_name1"
	                 "     , a.file_name2 , a.file_size "
	                 "     , a.file_type  , a.reg_user  "
	                 "     , a.reg_date   , a.reg_time  "
	                 "  FROM zangsi.T_CONTDATA_MYFILE a   "
	                 " WHERE a.id            = %ld      "
	                 ,gid);
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5023_delete_data_myinfo: INSERT T_CONTENTS_DEL error...\n");
		ZzLOG(ERROR, "daem5023_delete_data_myinfo: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
    }

	//--------------------------------------------------------------------------
	// 공개자료실 컨텐츠정보 삭제
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi.T_CONTDATA_MYINFO WHERE id = %ld ", gid);
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5023_delete_data_myinfo: UPDATE T_CONTDATA_MYINFO error...\n");
		ZzLOG(ERROR, "daem5023_delete_data_myinfo: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
    }

	//--------------------------------------------------------------------------
	// 공개자료실 파일정보 삭제
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi.T_CONTDATA_MYFILE WHERE id = %ld ", gid);
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5023_delete_data_myinfo: UPDATE T_CONTDATA_MYFILE error...\n");
		ZzLOG(ERROR, "daem5023_delete_data_myinfo: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
    }

    ZzLOG(ALWAY, "daem5023_delete_data_myinfo: DELETE OK\n");

	return 0;
}


//******************************************************************************
//* daem5023_select_del_date(삭제예정일 select)
//******************************************************************************
int daem5023_select_del_date()
{
	char szQuery[1000];		// query string
	char szTemp [1000];		// query string

    //ZzLOG(ALWAY, "daem5023_select_del_date: start!!!\n");
	
	//--------------------------------------------------------------------------
	// 최종거래일자 + 3
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	memset (szTemp , 0x00, sizeof(szTemp ));
	
	strcpy (szQuery, "SELECT date_format(date_add(ifnull(max(deal_date),date_format(now(),'%Y%m%d')), INTERVAL 3 DAY),'%Y%m%d')");
	strcat (szQuery, "  FROM zangsi.T_DEAL_INFO ");
	strcat (szQuery, " WHERE cont_gu = 'MY'");
	sprintf(szTemp , "   AND id      = %ld ", gid);
	strcat (szQuery, szTemp);
	
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5023_select_del_date: mysql_query error...\n");
		ZzLOG(ERROR, "daem5023_select_del_date: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem5023_select_del_date: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem5023_select_del_date: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
 	if (mysql_num_rows(res)==0)	{
		mysql_free_result(res);
		memset(&gdel_date, 0x00, sizeof(gdel_date));
		strcpy(gdel_date , "00000000");
		return 0;
	}
	if (row = mysql_fetch_row(res))
	{
		memset(&gdel_date, 0x00, sizeof(gdel_date));
		strcpy(gdel_date  , getstr(row, 0));
	}
	mysql_free_result(res);
	return 0;
}



//******************************************************************************
//* daem5023 db 처리로직
//******************************************************************************
int daem5023_delete_end()
{
	char szQuery[1000];		// query string
	int  nRowcnt;			// select row count
	int  ret;

    //ZzLOG(ALWAY, "daem5023_delete_end: start!!!\n");

	//디스크 정리대상
	//----------------------------------------------------------------------
	// Update T_MYDATA_INFO(사용량 Update)
	//----------------------------------------------------------------------
	if(gdelete_size > 0) {
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "UPDATE zangsi.T_MYDATA_INFO"
		                 "   SET disk_use = disk_use - %15.0f "
		                 " WHERE user_id  = '%s'     "
		                 ,gdelete_size
		                 ,guser_id
		                 );
		if (mysql_query(con, szQuery)){
		    ZzLOG(ERROR, "daem5023_delete_end: UPDATE zangsi.T_MYDISK_INFO error...\n");
			ZzLOG(ERROR, "daem5023_delete_end: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;	
	    }
	}
	
	//----------------------------------------------------------------------
	// Update T_MYDISK_PAYMENT (삭제처리정보)
	//----------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "UPDATE zangsi.T_MYDISK_PAYMENT"
	                 "   SET del_date = '%s'        "
	                 "     , del_size =  %15.0f     "
	                 "     , del_yn   = 'Y'         "
	                 " WHERE user_id  = '%s'        "
	                 "   AND seq_no   =  %d         "
	                 ,gproc_date
	                 ,gdelete_size
	                 ,guser_id
	                 ,gseq_no
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5023_delete_end: UPDATE zangsi.T_MYDISK_PAYMENT error...\n");
		ZzLOG(ERROR, "daem5023_delete_end: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;	
	}

    ZzLOG(ALWAY, "daem5023_delete_end: delete_size(%15.0f)\n", gdelete_size);

	return 0;
}

/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5023_get_sysdate()
{
	char szQuery[1000];		// query string
	char sztemp [100];      // query temp

	if (strcmp(gproc_date, "00000000") == 0)
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d')");
		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -30 DAY),'%Y%m%d')");
	}
	else
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		strcpy(szQuery, "SELECT date_format('");
		strcat(szQuery, gproc_date);
		strcat(szQuery, "','%Y%m%d')");
		strcat(szQuery, "     , date_format(date_add('");
		strcat(szQuery, gproc_date);
		strcat(szQuery, "', INTERVAL -30 DAY),'%Y%m%d')");
	}

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
	memset(gproc_date, 0x00, sizeof(gproc_date));
	memset(ged_date  , 0x00, sizeof(ged_date  ));

	strcpy(gproc_date,   getstr(row, 0));
	strcpy(ged_date  ,   getstr(row, 1));
	
	mysql_free_result(res);

	return 0;
}


/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5023_init_process(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("d_", "/logs/daemon"); 

    ZzLOG(ALWAY, "[daem5023]*****************프로그램 시작*****************\n");  
    ZzLOG(ALWAY, "[daem5023] 내디스크 삭제처리\n");  

    // 파라미터 값 설정 및 초기화
    if (argc < 2){
    	goto arg_error;
    }

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(con=db_connect("zangsi")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1); 
	}

	/* 처리일자 */
	memset(gproc_date, 0x00, sizeof(gproc_date));
	memset(greg_user,  0x00, sizeof(greg_user ));

	strcpy(gproc_date, argv[1]);
    if (argc > 2){
		strcpy(greg_user , argv[2]);
	}
	
	ret=daem5023_get_sysdate();
	if (ret < 0){
		db_disconnect(con);
		return -1;
	}

    return (0);


arg_error:
    ZzLOG(ERROR, "usage : %s YYYYMMDD XXXXX\n", argv[0]);
    ZzLOG(ERROR, "        YYYYMMDD(처리일자)-> '00000000'인경우 system일자로처리\n");

    ZzPRT(ERROR, "usage : %s YYYYMMDD XXXXX\n", argv[0]);
    ZzPRT(ERROR, "        YYYYMMDD(처리일자)-> '00000000'인경우 system일자로처리\n");
    return -1;
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daem5023_term_process()
{
    // DB close
	db_disconnect(con);
	db_disconnect(con2);
    ZzLOG(ALWAY, "[daem5023]*****************프로그램 종료*****************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5023_signal(int nSignal)
{
    daem5023_term_process();
}

/*****************************************************************************
*  프로그램 메인 
*****************************************************************************/
int main(int argc, char **argv)
{                
	
	int     rc;
                 
	/*       
	** SIGNAL 정의
	*/       
/*
	signal(SIGTERM, daem5023_signal);
	signal(SIGINT,  daem5023_signal);
	signal(SIGQUIT, daem5023_signal);
	signal(SIGKILL, daem5023_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
*/
	if ( daem5023_init_process(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daem5023_main_process();
		/* 프로그램 종료루틴 */
	}
	daem5023_term_process();
	return(0);
}
/*****************************************************************************
*  End of file...
*****************************************************************************/
