/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5101.cc
 *         기능 : 사용자가입현황
 *         설명 : 1일 1회 작업한다.
 *                SYSTEM  (00000000) => sysdate - 1일 처리하며,
 *                직접입력(yyyymmdd) => yyyymmdd를  처리한다.
 *                적극회원 구하기위한 등록/구매 건수를 구한다.
 *       작성자 : JDP / LEE
 *       작성일 : 2004/02/16
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
#include <signal.h>
#include <sys/stat.h>

#include "daemcom.h"
#include "commydb.h"

#define  MAX_ROWS	1
#define _DEBUG_

int daem5101_init_process(int argc, char **argv);
int daem5101_main_process();
int daem5101_term_process();
int daem5101_delete_current();
int daem5101_select_user_id();
int daem5101_insert_deal();
int daem5101_insert_vip();
int daem5101_select_yesterday();
int daem5101_select_dnew_dcnt();
int daem5101_select_unew_dcnt();
int daem5101_select_vip_ncnt();
int daem5101_insert_data();
int daem5101_get_sysdate();
void daem5101_signal(int nSignal);

MYSQL     *con;
MYSQL_RES *res;
MYSQL_ROW  row;

int    gdnew_dcnt        ;	//당일가입회원
int    gdnew_ncnt        ;	//당일누적회원
int    gunew_ncnt        ;	//당일가입회원(주민번호)
int    gunew_dcnt        ;	//당일누적회원(주민번호)
int    gvip_ncnt         ;	//적극회원
int    gbreak_dcnt       ;	//탈퇴회원
int    gbreak_ncnt       ;	//탈퇴누적
char   gsys_date  [  8+1];	//처리일자(sysdate)
char   gpro1_date [  8+1];	//처리일자(sysdate-1)
char   gpro2_date [  8+1];	//처리일자(sysdate-2)
char   greg_date  [  8+1];	//등록일
char   greg_time  [  6+1];	//등록시간
char   guser_id   [ 12+1];  //사용자ID
unsigned long guse_coupon_cnt;
unsigned long nuse_coupon_cnt;

bool m_bTodayProc;

//******************************************************************************
//* daem5101 main
//******************************************************************************
int daem5101_main_process()
{
	gdnew_dcnt  = 0;	//당일가입회원
	gdnew_ncnt  = 0;	//당일누적회원
	gunew_dcnt  = 0;	//당일가입회원(주민번호)
	gunew_ncnt  = 0;	//당일누적회원(주민번호)
	gvip_ncnt   = 0;	//적극회원
	gbreak_dcnt = 0;	//탈퇴회원
	gbreak_ncnt = 0;	//탈퇴누적
	guse_coupon_cnt = 0;
	nuse_coupon_cnt = 0;
	
	//--------------------------------------------------------------------------
	// 트렌젝션시작
	//--------------------------------------------------------------------------
	if (tran_begin(con)!=0) {
	    ZzLOG(ERROR, "tran_begin: 테이베이스 오류입니다.\n");  
		ZzLOG(ERROR, "daem5101_main_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
	}

	// 처리일자의 자료삭제
	if (daem5101_delete_current() != 0)
		goto daem5101_main_process_err;

	// 거래건수 집계처리
	if (daem5101_insert_deal() != 0)
		goto daem5101_main_process_err;

	// 전일자의 누적건수 조회
	if (daem5101_select_yesterday() != 0)
		goto daem5101_main_process_err;

	// 신규회원 건수
	if (daem5101_select_dnew_dcnt() != 0)
		goto daem5101_main_process_err;

	// 신규회원 건수
/*	if (daem5101_select_unew_dcnt() !=0)
		goto daem5101_main_process_err;
*/
	// 적극회원 건수
	if (daem5101_select_vip_ncnt() !=0)
		goto daem5101_main_process_err;

	// 계산한 자료 insert
	if (daem5101_insert_data() !=0)
		goto daem5101_main_process_err;

	if (tran_commit(con)!=0){
	    ZzLOG(ERROR, "daem5101_main_process: tran_commit error...\n");
		ZzLOG(ERROR, "daem5101_main_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    goto daem5101_main_process_err;
	}

	return (0);

daem5101_main_process_err:
	tran_rollback(con);
    return -1;
}


//******************************************************************************
//* daem5101_delete_current()
//* 처리일자의 자료를 삭제처리 한다.
//******************************************************************************
int daem5101_delete_current()
{
	char szQuery[1000];		// query string

	//--------------------------------------------------------------------------
	// 처리일자 해당되는 자료삭제(재작업을 위해)
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi_sum.T_AUD_ENTER"
	                 " WHERE enter_date    = '%s'   "
	                 ,gpro1_date
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5101_delete_current: DELETE zangsi_sum.T_AUD_ENTER error...\n");
		ZzLOG(ERROR, "daem5101_delete_current: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
    }

	//--------------------------------------------------------------------------
	// 사용자별 거래건수 삭제
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi_sum.T_USER_DEAL_DD"
	                 " WHERE deal_date    = '%s'   "
	                 ,gpro1_date
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5101_delete_current: DELETE zangsi_sum.T_USER_DEAL_DD error...\n");
		ZzLOG(ERROR, "daem5101_delete_current: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
    }

	//--------------------------------------------------------------------------
	// 처리일자 해당되는 자료삭제(재작업을 위해)
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi_sum.T_ANALY_ACCT"
	                 " WHERE acct_date    = '%s'   "
	                 ,gpro1_date
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5101_delete_current: DELETE zangsi.T_AUD_ENTER error...\n[ %s ] \n",szQuery);
		ZzLOG(ERROR, "daem5101_delete_current: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
    }


	if (commit(con)!=0){
	    ZzLOG(ERROR, "daem5101_delete_current: commit error...\n");
		ZzLOG(ERROR, "daem5101_delete_current: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}

    return 0;

}

//******************************************************************************
//* daem5101_select_yesterday()
//* 처리일자의 전일까지의 누적count를 조회한다.
//******************************************************************************
int daem5101_select_yesterday()
{
	char szQuery[1000];		// query string

	//--------------------------------------------------------------------------
	gdnew_ncnt  = 0;	//신규누적
	gunew_ncnt  = 0;	//신규누적(유니크사용자)
	gbreak_ncnt = 0;	//탈퇴누적
	//--------------------------------------------------------------------------
	// 전일자료 누적건수 조회
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT a.dnew_ncnt, a.unew_ncnt, a.break_ncnt"
	                 "  FROM zangsi_sum.T_AUD_ENTER a "
	                 " WHERE a.enter_date = '%s'  "
	                 ,gpro2_date
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5101_select_yesterday: mysql_query error...\n");
		ZzLOG(ERROR, "daem5101_select_yesterday: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem5101_select_yesterday: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem5101_select_yesterday: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
 	if (mysql_num_rows(res)==0)	{
		mysql_free_result(res);
		return 0;
	}
	if (row = mysql_fetch_row(res))
	{
		gdnew_ncnt  = getint(row, 0);
		gunew_ncnt  = getint(row, 1);
		gbreak_ncnt = getint(row, 2);
	}
	mysql_free_result(res);
	return 0;
}


//******************************************************************************
//* daem5101_insert_deal()
//* 일일 사용자별 거래집계처리
//******************************************************************************
int daem5101_insert_deal()
{
	char szQuery[2000];		// query string
	int ret=0;
	int nRowcnt = 0;

	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE from zangsi_sum.T_TEMP_DEAL ");
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5101_insert_deal: DROP T_TEMP_DEAL error...\n");
		ZzLOG(ERROR, "daem5101_insert_deal: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    ret = -1;
    }
    if (ret != 0) return ret;
    	
    
	if (commit(con)!=0){
	    ZzLOG(ERROR, "daem5101_insert_deal: commit 1 error...\n");
		ZzLOG(ERROR, "daem5101_insert_deal: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
		
	//--------------------------------------------------------------------------
	// 건수를 TEMPORARY에 생성한다.
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "INSERT INTO zangsi_sum.T_TEMP_DEAL     "
	                 "SELECT reg_user         as user_id   "
	                 "     , reg_date         as deal_date "
	                 "     , count(reg_user)  as reg_cnt   "
	                 "     , 0                as sale_cnt  "
	                 "     , 0                as buy_cnt   "
	                 "  FROM zangsi.T_CONTENTS_INFO        "
	                 " WHERE reg_date = '%s'               "
	                 " GROUP BY reg_user                   "
	                 "UNION ALL                            "
	                 "SELECT sale_user        as user_id   "
	                 "     , deal_date        as deal_date "
	                 "     , 0                as reg_cnt   "
	                 "     , count(sale_user) as sale_cnt  "
	                 "     , 0                as buy_cnt   "
	                 "  FROM zangsi.T_DEAL_INFO            "
	                 " WHERE deal_date = '%s'              "
	                 " GROUP BY sale_user                  "
	                 "UNION ALL                            "
	                 "SELECT buy_user         as user_id   "
	                 "     , deal_date        as deal_date "
	                 "     , 0                as reg_cnt   "
	                 "     , 0                as sale_cnt  "
	                 "     , count(buy_user)  as buy_cnt   "
	                 "  FROM zangsi.T_DEAL_INFO            "
	                 " WHERE deal_date = '%s'              "
	                 " GROUP BY buy_user                   "
	                 ,gpro1_date
	                 ,gpro1_date
	                 ,gpro1_date
	                 );
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5101_insert_deal: CREATE T_TEMP_DEAL reg_cnt error...\n");
		ZzLOG(ERROR, "daem5101_insert_deal: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
    }

	if (commit(con)!=0){
	    ZzLOG(ERROR, "daem5101_insert_deal: commit 1 error...\n");
		ZzLOG(ERROR, "daem5101_insert_deal: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
		
		    
	//--------------------------------------------------------------------------
	// 해당일자의 자료를 생성한다.
	//--------------------------------------------------------------------------
	ret = 0;
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "INSERT INTO zangsi_sum.T_USER_DEAL_DD "
	                 "     ( user_id    , deal_date     "
	                 "     , reg_cnt    , sale_cnt      "
	                 "     , buy_cnt)                   "
	                 "SELECT user_id    , deal_date     "
	                 "     , sum(reg_cnt)               "
	                 "     , sum(sale_cnt)              "
	                 "     , sum(buy_cnt)               "
	                 "  FROM zangsi_sum.T_TEMP_DEAL                  "
	                 " GROUP BY user_id    , deal_date  ");
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5101_insert_deal: INSERT T_USER_DEAL_DD error...\n");
		ZzLOG(ERROR, "daem5101_insert_deal: [ %s ] [%d](%s)\n",szQuery,mysql_errno(con), mysql_error(con));
	    ret = -1;
    }
	//--------------------------------------------------------------------------
	// TEMPORARY 테이블을 삭제 한다.
	//--------------------------------------------------------------------------
	/*
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DROP TABLE zangsi_sum.T_TEMP_DEAL ");
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5101_insert_deal: DROP T_TEMP_DEAL error...\n");
		ZzLOG(ERROR, "daem5101_insert_deal: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    ret = -1;
    }
	if (ret != 0) return ret;
	*/
	if (commit(con)!=0){
	    ZzLOG(ERROR, "daem5101_insert_deal: commit 1 error...\n");
		ZzLOG(ERROR, "daem5101_insert_deal: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
		



	//--------------------------------------------------------------------------
	// 처리할 대상 사용자 clean(vip회원)
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi_sum.T_USER_TEMP");

	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5101_insert_deal: DELETE zangsi.T_USER_TEMP error...\n");
		ZzLOG(ERROR, "daem5101_insert_deal: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
    }

	if (commit(con)!=0){
	    ZzLOG(ERROR, "daem5101_insert_deal: commit 1 error...\n");
		ZzLOG(ERROR, "daem5101_insert_deal: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}

	//--------------------------------------------------------------------------
	// 3. 처리할 대상 사용자 설정(vip회원)
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "INSERT INTO zangsi_sum.T_USER_TEMP"
	                 "     ( user_id )              "
	                 "SELECT user_id                "
	                 "  FROM zangsi_sum.T_USER_DEAL_DD  "
	                 " WHERE deal_date = '%s'       "
	                 ,gpro1_date
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5102_delete_current: INSERT zangsi.T_USER_TEMP error...\n");
		ZzLOG(ERROR, "daem5102_delete_current: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
    }
	nRowcnt = mysql_affected_rows(con);

	if (commit(con)!=0){
	    ZzLOG(ERROR, "daem5102_delete_current: commit 2 error...\n");
		ZzLOG(ERROR, "daem5102_delete_current: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}

	if (nRowcnt == 0) return 0;
	//--------------------------------------------------------------------------
	// 처리할 사용자를 한명씩 읽어 거래내역을 계산한다.
	//--------------------------------------------------------------------------
	/*
	while(1) {

		// 처리대상 사용자 1명 select
		ret = daem5101_select_user_id();
		if (ret != 0) break;

		// 처리대상 사용자의 전일잔액을 구한다
		ret = daem5101_insert_vip();
		if (ret != 0) break;
	}
	
	// 더이상 처리할 자료 없음
	if (ret != 9) return -1;
	*/
	
	return 0;
}

//******************************************************************************
// daem5101_select_user_id
// - 사용자를 1건 읽는다.
//******************************************************************************
int daem5101_select_user_id()
{
	char szQuery[1000];  // query string

	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT user_id           "
	                 "  FROM zangsi_sum.T_USER_TEMP"
	                 " LIMIT 1                 "
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5101_select_user_id: mysql_query error...\n");
		ZzLOG(ERROR, "daem5101_select_user_id: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem5101_select_user_id: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem5101_select_user_id: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)	{
		mysql_free_result(res);
		return 9;
	}
	if (!(row = mysql_fetch_row(res)))	{
	    ZzLOG(ERROR, "daem5101_select_user_id: mysql_fetch_row error...\n");
		ZzLOG(ERROR, "daem5101_select_user_id: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}

	memset(&guser_id, 0x00, sizeof(guser_id));
	strcpy(guser_id, getstr(row, 0));

	mysql_free_result(res);
	return 0;
}

//******************************************************************************
// daem5101_insert_vip
// - vip 회원 자료 생성
//******************************************************************************
int daem5101_insert_vip()
{
	char szQuery[1000];  // query string
	int  nRowcnt   = 0;
	int  reg_cnt   = 0;
	int  sale_cnt  = 0;
	int  buy_cnt   = 0;
	int  ret       = 0;

	//--------------------------------------------------------------------------
	//일일 거래건수에서 해당일자의 거래가 있는 사용자의 전체건수를 조회한다.
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT sum(reg_cnt)  reg_cnt  "
	                 "     , sum(sale_cnt) sale_cnt "
	                 "     , sum(buy_cnt)  buy_cnt  "
	                 "  FROM zangsi_sum.T_USER_DEAL_DD  "
	                 " WHERE user_id = '%s'         "
	                 ,guser_id
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5101_insert_vip: mysql_query error...\n");
		ZzLOG(ERROR, "daem5101_insert_vip: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem5101_insert_vip: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem5101_insert_vip: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)	{
		mysql_free_result(res);
		return 9;
	}
	if (!(row = mysql_fetch_row(res)))	{
	    ZzLOG(ERROR, "daem5101_insert_vip: mysql_fetch_row error...\n");
		ZzLOG(ERROR, "daem5101_insert_vip: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}

	reg_cnt   = 0;
	sale_cnt  = 0;
	buy_cnt   = 0;

	reg_cnt   = getint(row, 0);
	sale_cnt  = getint(row, 1);
	buy_cnt   = getint(row, 2);

	mysql_free_result(res);

	//적극회원 = 등록건수 + 구매건수 > 2
	if (reg_cnt + buy_cnt > 2)
	{
		//--------------------------------------------------------------------------
		// 1. 수수료내역 생성
		//--------------------------------------------------------------------------
		nRowcnt   = 0;
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "UPDATE zangsi_sum.T_USER_DEAL "
		                 "   SET reg_cnt  =  %d     "
		                 "     , sale_cnt =  %d     "
		                 "     , buy_cnt  =  %d     "
		                 " WHERE user_id  = '%s'    "
		                 ,reg_cnt
		                 ,sale_cnt
		                 ,buy_cnt
		                 ,guser_id
		                 );
		if (mysql_query(con, szQuery)){
		    ZzLOG(ERROR, "daem5101_insert_vip: UPDATE T_USER_DEAL error...(%s)\n", guser_id);
			ZzLOG(ERROR, "daem5101_insert_vip: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
	    }
		nRowcnt = mysql_affected_rows(con);
		
		if (nRowcnt == 0)
		{
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "INSERT INTO zangsi_sum.T_USER_DEAL"
			                 "     ( user_id  , reg_cnt     " 
			                 "     , sale_cnt , buy_cnt     "
			                 "     ) VALUES                 "
			                 "     ( '%s'     , %d          "
			                 "     ,  %d      , %d)         "
			                 ,guser_id
			                 ,reg_cnt
			                 ,sale_cnt
			                 ,buy_cnt
			                 );
			if (mysql_query(con, szQuery)){
				ret = mysql_errno(con);
				if (ret != 1062) {
					// 중복키 OK처리			                 
			    	ZzLOG(ERROR, "daem5101_insert_vip: INSERT T_USER_DEAL error...(%s)\n", guser_id);
					ZzLOG(ERROR, "daem5101_insert_vip: [%d](%s)\n",mysql_errno(con), mysql_error(con));
					return -1;
		    	}
			}
		}
	}

	//--------------------------------------------------------------------------
	// 해당 사용자 리스트에서 삭제
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi_sum.T_USER_TEMP"
	                 " WHERE user_id = '%s'         "
	                 ,guser_id
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5101_insert_vip: (%s)처리한 사용자 삭제 error...\n", guser_id);
		ZzLOG(ERROR, "daem5101_insert_vip: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
    }

	if (commit(con) != 0){
	    ZzLOG(ERROR, "daem5102_main_process: commit error...\n");
		ZzLOG(ERROR, "daem5102_main_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	return 0;
}


//******************************************************************************
//* daem5101_select_dnew_dcnt()
//* 처리일자의 신규가입회원을 구한다.
//******************************************************************************
int daem5101_select_dnew_dcnt()
{
	char szQuery[1000];		// query string

	gdnew_dcnt  = 0;		//신규가입회원
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT count(user_id)"
	                 "  FROM zangsi.T_USER_INFO a "
	                 " WHERE a.reg_date = '%s'  "
	                 ,gpro1_date
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5101_select_dnew_dcnt: mysql_query error...\n");
		ZzLOG(ERROR, "daem5101_select_dnew_dcnt: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem5101_select_dnew_dcnt: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem5101_select_dnew_dcnt: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
 	if (mysql_num_rows(res)==0)	{
		mysql_free_result(res);
		return 0;
	}
	if (row = mysql_fetch_row(res))	{
		gdnew_dcnt  = getint(row, 0);
	}
	mysql_free_result(res);
	return 0;
}

//******************************************************************************
//* daem5101_select_unew_dcnt()
//* 처리일자의 신규가입회원을 구한다.
//******************************************************************************
int daem5101_select_unew_dcnt()
{
	char szQuery[1000];		// query string

	gunew_dcnt  = 0;		//신규가입회원(중복제거)
	gunew_ncnt  = 0;		//신규누적회원(중보제거)

	//--------------------------------------------------------------------------
	// 누적 가입회원(중복제거)
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT count(jumn_no)"
	                 "  FROM zangsi.T_USER_JUMN a "
	                 " WHERE a.enter_date  < '%s' "
	                 ,gpro1_date
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5101_select_unew_dcnt-1: mysql_query error...\n");
		ZzLOG(ERROR, "daem5101_select_unew_dcnt-1: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem5101_select_unew_dcnt-1: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem5101_select_unew_dcnt-1: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
 	if (mysql_num_rows(res)==0)	{
		mysql_free_result(res);
		return 0;
	}
	if (row = mysql_fetch_row(res))	{
		gunew_ncnt  = getint(row, 0);
	}
	mysql_free_result(res);

	//--------------------------------------------------------------------------
	// 당일 가입회원(중복제거)
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT count(jumn_no)"
	                 "  FROM zangsi.T_USER_JUMN a "
	                 " WHERE a.enter_date = '%s'  "
	                 ,gpro1_date
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5101_select_unew_dcnt-0: mysql_query error...\n");
		ZzLOG(ERROR, "daem5101_select_unew_dcnt-0: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem5101_select_unew_dcnt-0: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem5101_select_unew_dcnt-0: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
 	if (mysql_num_rows(res)==0)	{
		mysql_free_result(res);
		return 0;
	}
	if (row = mysql_fetch_row(res))	{
		gunew_dcnt  = getint(row, 0);
	}
	mysql_free_result(res);
	
	return 0;
}

//******************************************************************************
//* daem5101_select_vip()
//* 처리일자의 신규가입회원을 구한다.
//******************************************************************************
int daem5101_select_vip_ncnt()
{
	char szQuery[1000];		// query string

	gvip_ncnt  = 0;			//적극회원
	//--------------------------------------------------------------------------
	// 누적 가입회원(중복제거)
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT count(user_id)     "
	                 "  FROM zangsi_sum.T_USER_DEAL ");

	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5101_select_vip_ncnt: mysql_query error...\n");
		ZzLOG(ERROR, "daem5101_select_vip_ncnt: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem5101_select_vip_ncnt: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem5101_select_vip_ncnt: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
 	if (mysql_num_rows(res)==0)	{
		mysql_free_result(res);
		return 0;
	}
	if (row = mysql_fetch_row(res))	{
		gvip_ncnt  = getint(row, 0);
	}
	mysql_free_result(res);

	return 0;
}


//******************************************************************************
//* daem5101_insert_date()
//  - 회원가입현황 isert
//******************************************************************************
int daem5101_insert_data()
{
	char szQuery[3000];		// query string
	//--------------------------------------------------------------------------
	//  insert
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "INSERT INTO zangsi_sum.T_AUD_ENTER"
	                 "     ( enter_date             "
	                 "     , dnew_dcnt  , dnew_ncnt "
	                 "     , unew_dcnt  , unew_ncnt "
	                 "     , vip_ncnt               "
	                 "     , break_dcnt , break_ncnt"
	                 "     , reg_date   , reg_time  "
	                 "     ) VALUES       "
	                 "     ( '%s'         "
	                 "     ,  %d   ,  %d  "
	                 "     ,  %d   ,  %d  "
	                 "     ,  %d          "
	                 "     ,  %d   ,  %d  "
	                 "     , '%s'  , '%s')"
	                 ,gpro1_date
	                 ,gdnew_dcnt
	                 ,gdnew_ncnt + gdnew_dcnt
	                 ,gunew_dcnt
	                 ,gunew_ncnt + gunew_dcnt
	                 ,gvip_ncnt
	                 ,gbreak_dcnt
	                 ,gbreak_ncnt + gbreak_dcnt
	                 ,greg_date
	                 ,greg_time
	                 );
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5101_insert_data: INSERT zangsi. error...\n");
		ZzLOG(ERROR, "daem5101_insert_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;	
    }


	//--------------------------------------------------------------------------
	//  insert
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	
	/*
	sprintf(szQuery, "INSERT INTO zangsi.T_ANALY_ACCT "
	                 "SELECT a.inout_date             "
	                 "     , count(distinct(if(a.inout_date = b.reg_date, a.user_id, null ))) "
	                 "     , sum(if(a.inout_date = b.reg_date, in_amt,0)) "
	                 "     , count(distinct(if(a.inout_date = b.reg_date, null,a.user_id ))) "
	                 "     , sum(if(a.inout_date = b.reg_date, 0,in_amt)) "
	                 "     , '%s', '%s' "
	                 "  FROM zangsi.T_INOUT_AMOUNT a "
	                 "     , zangsi.T_USER_INFO    b "
	                 " WHERE a.inout_date >= '%s'    "
	                 "   AND a.inout_date <= '%s'    "
	                 "   AND a.inout_code in ('31','32','33','34','35','36') "
	                 "   AND a.user_id    = b.user_id "
	                 " GROUP BY a.inout_date "
	                 ,greg_date
	                 ,greg_time
	                 ,gpro1_date
	                 ,gpro1_date
	                 );
	*/


	/********** add by ychahn *****************/
	/* 전체 쿠폰사용건수 조회*/
	/******************************************/
	sprintf(szQuery, "SELECT  count(user_id) as use_coupon_cnt "
	                 "  FROM zangsi.T_COUPON_HIST "
	                 " WHERE use_date >= '%s'    "
	                 "   AND use_date <= '%s'    "
	                 "   AND use_yn = 'Y' "
	                 " GROUP BY use_date "
	                 ,gpro1_date
	                 ,gpro1_date
	                 );
	                 
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5101_insert_data: INSERT zangsi.T_ANALY_ACCT error...\n");
		ZzLOG(ERROR, "daem5101_insert_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;	
    }
    
    
    if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem5501_process: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)	
 	{
		mysql_free_result(res);
	    ZzLOG(ALWAY, "daem5501_process2: 처리할 자료가 없습니다.\nQuery %s\n",szQuery);
		guse_coupon_cnt = 0;
	}
	else
	{
		if((row = mysql_fetch_row(res)))
		{
			
			guse_coupon_cnt = (unsigned long)getnum(row,0);
		}
		else
		{
			//예외상황 
			ZzLOG(ALWAY, "daem5501_process: 결과 값을 가져오는데 실패함..\nQuery %s\n",szQuery);
			ZzLOG(ERROR, "daem5101_insert_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));			
			guse_coupon_cnt = 0;
		}
		
	}





	// 신규 졀제사 쿠폰 사용 건수

	//충전(띠앗)추가 3I.20080828 - HCS
	//충전(T_cash, 틴캐쉬) 추가 3G, 3J. 20090512 - HCS
	sprintf(szQuery,  "SELECT a.inout_date    "
		      		  "    , count(b.user_id) "
		      		  " FROM zangsi.T_INOUT_AMOUNT a "
		      		  "    , zangsi.T_COUPON_HIST b "
		      		  "WHERE a.inout_date >= '%s'"
		      		  "  AND a.inout_date <= '%s'"   
		      		  "  AND a.inout_code >= '31' "
		      		  "  AND a.inout_code <= '3Z' "
		      		  "  AND a.cnl_yn ='N'	       "
		      		  "  AND a.user_id    = b.user_id "
		      		  "  and a.inout_date = b.use_date"
		      		  "  AND b.use_yn = 'Y'    "
		      		  "GROUP BY a.inout_date"
	                 ,gpro1_date
	                 ,gpro1_date
		              );
		                 
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5101_insert_data: INSERT zangsi.T_ANALY_ACCT error...\n");
		ZzLOG(ERROR, "daem5101_insert_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;	
    }
    
    
    if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem5501_process: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)	
 	{
		mysql_free_result(res);
	    ZzLOG(ALWAY, "daem5501_process2: 처리할 자료가 없습니다.\nQuery %s\n",szQuery);
		nuse_coupon_cnt = 0;
	}
	else
	{
		if((row = mysql_fetch_row(res)))
		{
			
			nuse_coupon_cnt = (unsigned long)getnum(row,1);
		}
		else
		{
			//예외상황 
			ZzLOG(ALWAY, "daem5501_process: 결과 값을 가져오는데 실패함..\nQuery %s\n",szQuery);
			ZzLOG(ERROR, "daem5101_insert_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));			
			nuse_coupon_cnt = 0;
		}
		
	}
    mysql_free_result(res);

	/********** add by ychahn *****************/
	/* 신규가입 신규 결제비 쿠폰사용건수 추가 */
	/******************************************/
	memset (szQuery, 0x00, sizeof(szQuery));
  		
	//충전(띠앗)추가 3I.20080828 - HCS
	//충전(T_cash, 틴캐쉬) 추가 3G, 3J. 20090512 - HCS
	sprintf(szQuery, "INSERT INTO zangsi_sum.T_ANALY_ACCT "
	                 "SELECT a.inout_date             "
	                 "     , count(distinct(if(a.inout_date = b.reg_date, a.user_id, null ))) "
	                 "     , sum(if(a.inout_date = b.reg_date, in_amt,0)) "
	                 "     , count(distinct(if(a.inout_date = b.reg_date, null,a.user_id ))) "
	                 "     , sum(if(a.inout_date = b.reg_date, 0,in_amt)) "
					 "     , %ld ,%ld , '%s', '%s' "
	                 "  FROM zangsi.T_INOUT_AMOUNT a "
	                 "     , zangsi.T_USER_INFO    b "
	                 " WHERE a.inout_date >= '%s'    "
	                 "   AND a.inout_date <= '%s'    "
		      		  "  AND a.inout_code >= '31' "
		      		  "  AND a.inout_code <= '3Z' "
	                 "   AND a.cnl_yn ='N'	       "
	                 "   AND a.user_id    = b.user_id "
	                 " GROUP BY a.inout_date "
	                 ,nuse_coupon_cnt
	                 ,guse_coupon_cnt 
	                 ,greg_date
	                 ,greg_time
	                 ,gpro1_date
	                 ,gpro1_date
	                 );


	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5101_insert_data: INSERT zangsi.T_ANALY_ACCT error...\n");
		ZzLOG(ERROR, "daem5101_insert_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;	
    }
    
	if(m_bTodayProc)
	{	
		//20100217 -- HCS : 신규, 가입 후 첫 결제자 금액별 집계 추가
		/******************************************/
		/* 신규가입 신규 결제금액별 건수 */
		/******************************************/
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " INSERT INTO zangsi_sum.T_ANALY_FIRST_ACCT "
						 " (acct_date, case_code, acct_code, acct_amt, acct_cnt) "
		                 " SELECT "
		                 " a.inout_date, 'NA', a.inout_code, a.in_amt, count(b.user_id) "
		                 "  FROM zangsi.T_INOUT_AMOUNT a "
		                 "     , zangsi.T_USER_INFO    b "
		                 " WHERE "
		                 " a.user_id = b.user_id and a.inout_date = b.reg_date "
		                 "   AND a.inout_date = '%s' "
	   		      		 "  AND a.inout_code >= '31' "
	   		      		 "  AND a.inout_code <= '3Z' "
		                 "   AND a.cnl_yn ='N' "
		                 " GROUP BY a.inout_code, a.in_amt "
		                 ,gpro1_date);
	
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5101_insert_data: INSERT zangsi.T_ANALY_ACCT error...\n");
			ZzLOG(ERROR, "daem5101_insert_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;	
	    }
	    
	    
		/******************************************/
		/* 기존 회원중 첫 결제 금액별 건수 */
		/******************************************/
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " INSERT INTO zangsi_sum.T_ANALY_FIRST_ACCT "
						 " (acct_date, case_code, acct_code, acct_amt, acct_cnt) "
		                 " SELECT "
		                 " a.inout_date, 'FA', a.inout_code, a.in_amt, count(b.user_id) "
		                 " from zangsi.T_INOUT_AMOUNT a, zangsi.T_USER_INFO b, zangsi_sum.T_ACCT_PERIOD c "
		                 " WHERE "
		                 " a.user_id = b.user_id and b.user_id = c.user_id  "
			      		 "  AND a.inout_code >= '31' "
			      		 "  AND a.inout_code <= '3Z' "
		                 " and a.inout_date = '%s' and b.reg_date < '%s' and c.acct_cnt = 1 "
		                 " and a.cnl_yn ='N' "
		                 " GROUP BY a.inout_code, a.in_amt "
		                 ,gpro1_date
		                 ,gpro1_date);
	
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5101_insert_data: INSERT zangsi.T_ANALY_ACCT error...\n");
			ZzLOG(ERROR, "daem5101_insert_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;	
	    }

		//20100304 -- HCS : 전체 회원 대상 결제자 금액별 집계 추가
		/******************************************/
		/* 전체 회원 대상 결제자 금액별 건수 */
		/******************************************/
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " INSERT INTO zangsi_sum.T_ANALY_ALL_ACCT "
						 " (acct_date, acct_code, acct_amt, acct_cnt) "
		                 " SELECT "
		                 " inout_date, inout_code, in_amt, count(user_id) "
		                 " FROM zangsi.T_INOUT_AMOUNT  "
		                 " WHERE "
		                 " inout_date = '%s' "
			      		 "  AND inout_code >= '31' "
			      		 "  AND inout_code <= '3Z' "
		                 " and cnl_yn ='N' "
		                 " GROUP BY inout_code, in_amt "
		                 ,gpro1_date);
	
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5101_insert_data: INSERT zangsi.T_ANALY_ACCT error...\n");
			ZzLOG(ERROR, "daem5101_insert_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;	
	    }
	}


	return 0;
}

/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5101_get_sysdate()
{
	char szQuery[1000];		// query string
	char sztemp [100];      // query temp

	memset(szQuery, 0x00, sizeof(szQuery));
	if (strcmp(gsys_date, "00000000")==0)
	{
		m_bTodayProc = true;
		strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d')");
		strcat(szQuery, "     , date_format(now(),'%H%i%s')");
		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -1 DAY),'%Y%m%d')");
		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -2 DAY),'%Y%m%d')");
	}
	else
	{
		strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d')");
		strcat(szQuery, "     , date_format(now(),'%H%i%s')");
		strcat(szQuery, "     , date_format(date_add('");
		strcat(szQuery, gsys_date);
		strcat(szQuery, "',INTERVAL 0 DAY),'%Y%m%d')");
		strcat(szQuery, "     , date_format(date_add('");
		strcat(szQuery, gsys_date);
		strcat(szQuery, "',INTERVAL -1 DAY),'%Y%m%d')");
	}
	if (mysql_query(con, szQuery))
	{
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
	memset(greg_date , 0x00, sizeof(greg_date ));
	memset(greg_time , 0x00, sizeof(greg_time ));
	memset(gpro1_date, 0x00, sizeof(gpro1_date));
	memset(gpro2_date, 0x00, sizeof(gpro2_date));

	strcpy(greg_date ,   getstr(row, 0));
	strcpy(greg_time ,   getstr(row, 1));
	strcpy(gpro1_date,   getstr(row, 2));
	strcpy(gpro2_date,   getstr(row, 3));
	
	mysql_free_result(res);


	return 0;
}

/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5101_init_process(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("d_", "/logs/daemon"); 
    
    m_bTodayProc = false;

    ZzLOG(ALWAY, "[daem5101]***************프로그램 시작***************\n");  

    // 파라미터 값 설정 및 초기화
    if (argc != 2){
    	goto arg_error;
    }

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(con=db_connect_nodb("")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1); 
	}

	/* 처리일자 */
	memset(gsys_date, 0x00, sizeof(gsys_date));
	strcpy(gsys_date, argv[1]);
	ret=daem5101_get_sysdate();
	if (ret < 0){
		db_disconnect(con);
		return -1;
	}
	
    return (0);


arg_error:
    ZzLOG(ERROR, "usage : %s YYYYMMDD\n", argv[0]);
    ZzLOG(ERROR, "        YYYYMMDD(처리일자): 00000000 = 시스템일자\n", argv[0]);
    ZzPRT(ERROR, "usage : %s YYYYMMDD\n", argv[0]);
    ZzPRT(ERROR, "        YYYYMMDD(처리일자): 00000000 = 시스템일자\n", argv[0]);
    return -1;
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daem5101_term_process()
{
    // DB close
	db_disconnect(con);
    ZzLOG(ALWAY, "[daem5101]***************프로그램 종료***************\n\n");  

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5101_signal(int nSignal)
{
    daem5101_term_process();
}

/*****************************************************************************
*  프로그램 메인 
*****************************************************************************/
int main(int argc, char **argv)
{                
	char    szTemp[1024];
	int     rc;
                 
	/*       
	** SIGNAL 정의
	*/       
	signal(SIGTERM, daem5101_signal);
	signal(SIGINT,  daem5101_signal);
	signal(SIGQUIT, daem5101_signal);
	signal(SIGKILL, daem5101_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daem5101_init_process(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daem5101_main_process();
	
		/* 프로그램 종료루틴 */                    
		daem5101_term_process();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
