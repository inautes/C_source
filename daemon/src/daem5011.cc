/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5011.cc(수수료계산)
 *         기능 : 수수료를 계산한다.
 *         설명 : 1. 프로세스 처리 예정시간 : 00:00 이후에 처리한다.
 *     설치위치 : cmdsvr에 위치한다.
 *
 *       작성자 : LEE
 *       작성일 : 2005/12/05
 *     수정이력 : 2007/11/30
 *			      HCS
 *			      -포인트 추가. (무료충전소 관련, line283)
 				  -출금수수료책정시 소득세를 회사이익분을 포함(line 329)
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

#include "daemcom.h"
#include "commydb.h"

#define  MAX_ROWS	1
#define _DEBUG_

int daem5011_get_sysdate();
int daem5011_init_process(int argc, char **argv);
int daem5011_main_process();
int daem5011_select_inout_code();
int daem5011_delete_current();
int daem5011_insert_comis_dd();
int daem5011_insert_buz_amt_dd();
int daem5011_insert_buz_cyb_dd();
int daem5011_insert_free_coupon();
int daem5011_term_process();
void daem5011_signal(int nSignal);

MYSQL     *con;
MYSQL_RES *res;
MYSQL_ROW  row;

char gproc_date [ 8+1];	// 처리일자
char greg_date  [ 8+1];	// 등록일자
char greg_time  [ 6+1];	// 등록시간
char gminor_code[ 2+1];	// 등록일자
char gminor_name[80+1];	// 등록시간

int  gmin_add         ;	// 증가값(분)
char gst_time   [ 6+1];	// 처리시작시간
char ged_time   [ 6+1];	// 처리마지막시간
char gproc_log  [80+1];	// 로그메시지버퍼
int  nProcCnt         ;
//******************************************************************************
//* daem5011 main
//******************************************************************************
int daem5011_main_process()
{
	int ret;

	//--------------------------------------------------------------------------
	// 트렌젝션시작
	//--------------------------------------------------------------------------
	if (tran_begin(con)!=0)	
	{
	    ZzLOG(ERROR, "daem5011_main_process: tran_begin error...\n");
		return -1;
	}

	//--------------------------------------------------------------------------
	// 트렌젝션시작
	//--------------------------------------------------------------------------
	if (daem5011_delete_current() != 0)
		return -1;

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*   - 입출금코드 while 만클 while loop
	/*------------------------------------------------------------------------*/
	memset(&gminor_code, 0x00, sizeof(gminor_code));
	memset(&gminor_name, 0x00, sizeof(gminor_name));
	strcpy( gminor_code, "00");
	while(1) {
		// 1. 입출금코드를 1건 조회
		ret = daem5011_select_inout_code();
		if (ret != 0) break;

		// 2. 해당일자, 해당입출금코드의 합계를 계산하여 Insert한다.
		ret = daem5011_insert_comis_dd();
		if (ret != 0) 
		{
			tran_rollback(con);
			break;
		}
	}
	// 자료가 없는경우 9를 return 한다.
	if (ret != 9) 
		return -1;
	
	//--------------------------------------------------------------------------
	// 일일캐쉬현황 삭제
	//--------------------------------------------------------------------------
	if (daem5011_insert_buz_amt_dd() != 0) 
	{
		tran_rollback(con);
		return -1;
	}
	//--------------------------------------------------------------------------
	// 일일사이버머니현황 삭제
	//--------------------------------------------------------------------------
	if (daem5011_insert_buz_cyb_dd() != 0) 
	{
		tran_rollback(con);
		return -1;
	}
	
	if( daem5011_insert_free_coupon() != 0 )
	{
		tran_rollback(con);
		return -1;
	}
	
	// 20060525 일 무료 충전소 
	
	ZzLOG(ALWAY, "daem5011_main_process: sucess !!!\n");

	tran_end(con);
	return 0;
}

//******************************************************************************
//  daem5011_delete_current()
//  - 현재 처리할 일자의 자료를 삭제한다.
//******************************************************************************
int daem5011_delete_current()
{
	char szQuery[1000];	// query string

	//--------------------------------------------------------------------------
	// 1. 수수료현황 삭제
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi_sum.T_COMIS_DD"
	                 " WHERE cmt_date = '%s'       "
	                 ,gproc_date
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5011_delete_current: DELETE T_COMIS_DD error...\n");
		ZzLOG(ERROR, "daem5011_delete_current: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
    }
    
	//--------------------------------------------------------------------------
	// 2. 일일캐쉬현황 삭제
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi_sum.T_BUZ_AMT_DD"
	                 " WHERE io_date = '%s'          "
	                 ,gproc_date
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5011_delete_current: DELETE T_BUZ_AMT_DD error...\n");
		ZzLOG(ERROR, "daem5011_delete_current: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
    }

	//--------------------------------------------------------------------------
	// 3. 일일사이버머니현황 삭제
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi_sum.T_BUZ_CYB_DD"
	                 " WHERE io_date = '%s'          "
	                 ,gproc_date
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5011_delete_current: DELETE T_BUZ_AMT_DD error...\n");
		ZzLOG(ERROR, "daem5011_delete_current: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
    }

	if (commit(con)!=0){
	    ZzLOG(ERROR, "daem5011_delete_current: commit error...\n");
		ZzLOG(ERROR, "daem5011_delete_current: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}

	return 0;
}

//******************************************************************************
// daem5011_select_inout_code()
// - 입출력코드 조회
//******************************************************************************
int daem5011_select_inout_code()
{
	char szQuery[1000];  // query string

	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT minor_code, minor_name"
	                 "  FROM zangsi.T_MINOR_CODE   "
	                 " WHERE major_code = '03'     "
	                 "   AND minor_code > '%s'     "
	                 "ORDER BY minor_code          "
	                 " LIMIT 1                     "
	                 , gminor_code
	                 );


		                 
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5011_select_inout_code: mysql_query error...\n");
		ZzLOG(ERROR, "daem5011_select_inout_code: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem5011_select_inout_code: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem5011_select_inout_code: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)	{
		mysql_free_result(res);
		return 9;
	}
	if (!(row = mysql_fetch_row(res)))
	{
	    ZzLOG(ERROR, "daem5011_select_inout_code: mysql_fetch_row error...\n");
		ZzLOG(ERROR, "daem5011_select_inout_code: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}

	memset(&gminor_code, 0x00, sizeof(gminor_code));
	memset(&gminor_name, 0x00, sizeof(gminor_name));

	strcpy(gminor_code, getstr(row, 0));
	strcpy(gminor_name, getstr(row, 1));

	mysql_free_result(res);
	
	return 0;
}


int daem5011_insert_free_coupon()
{
	char szQuery[2000];	// query string

	memset (szQuery, 0x00, sizeof(szQuery));
/*	sprintf(szQuery, "INSERT INTO zangsi_sum.T_COMIS_DD"
	                 "     ( cmt_date ,cmt_code    "
	                 "     , io_cnt   ,io_amt      "
	                 "     , cmt_amt  ,reg_date    "
	                 "     , reg_time )            "
	                 "SELECT reg_date  , '3Z' "
	                 "     , count(reg_date)             as io_cnt "
	                 "     , ifnull(sum(coupon_amt),0) as io_amt "
	                 "     , 0      as cmt_amt	   "
	                 "     , '%s'    , '%s'        "
	                 "  FROM zangsi.T_COUPON_ISSUE_SUCC "
	                 " where reg_date = '%s' and  coupon_code = '02' "                                            
	                 " GROUP BY reg_date "
	                 ,greg_date            
	                 ,greg_time            
	                 ,gproc_date
	                 );
*/
	sprintf(szQuery, "INSERT INTO zangsi_sum.T_COMIS_DD"
	                 "     ( cmt_date ,cmt_code    "
	                 "     , io_cnt   ,io_amt      "
	                 "     , cmt_amt  ,reg_date    "
	                 "     , reg_time )            "
	                 "SELECT reg_date  , '3Z' "
	                 "     , count(reg_date)             as io_cnt "
	                 "     , ifnull(sum(coupon_amt),0) as io_amt "
	                 "     , 0      as cmt_amt	   "
	                 "     , '%s'    , '%s'        "
	                 "  FROM zangsi.T_COUPON_ISSUE_SUCC "
	                 " where reg_date = '%s' and  coupon_code = '02' "                                            
	                 " 	  or reg_date = '%s' and  coupon_code = 'P' " //포인트제 시행으로 변경.
	                 " GROUP BY reg_date "
	                 ,greg_date
	                 ,greg_time
	                 ,gproc_date
	                 ,gproc_date
	                 );
	#ifdef __DEBUG
	printf("%s", szQuery);                 
	#endif
	
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5011_insert_comis_dd: mysql_query error...\n");
		ZzLOG(ERROR, "daem5011_insert_comis_dd: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (commit(con)!=0){
	    ZzLOG(ERROR, "daem5011_insert_comis_dd: commit error...\n");
		ZzLOG(ERROR, "daem5011_insert_comis_dd: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}

	return 0;	
}

//******************************************************************************
//  daem5011_insert_comis_dd()
//  - 해당일자의 해당입출금코드를 계산하여 insert한다.
//******************************************************************************
int daem5011_insert_comis_dd()
{
	char szQuery[2000];	// query string

	memset (szQuery, 0x00, sizeof(szQuery));
	
	
	
	sprintf(szQuery, "INSERT INTO zangsi_sum.T_COMIS_DD"
	                 "     ( cmt_date ,cmt_code    "
	                 "     , io_cnt   ,io_amt      "
	                 "     , cmt_amt  ,reg_date    "
	                 "     , reg_time )            "
	                 "SELECT inout_date  , inout_code "
	                 "     , count(inout_date)             as io_cnt "
	                 "     , ifnull(sum(in_amt+out_amt),0) as io_amt "
//	                 "     , if( inout_code = '21' ,ifnull(sum(out_amt - inout_cmt),0)  , ifnull(sum(inout_cmt),0))      as cmt_amt" //출금 수수료 계산시 소득세 포함
	                 "     , if( inout_code = '21' ,ifnull(sum(out_amt/2),0)  , ifnull(sum(inout_cmt),0))      as cmt_amt"  //출금 수수료 계산시 소득세 제외
	                 "     , '%s'    , '%s'        "
	                 "  FROM zangsi.T_INOUT_AMOUNT "   
	                 " WHERE inout_code  = '%s'    "   
	                 "   AND inout_date  = '%s'    "                                              
					 "   AND cnl_yn ='N'	       "                                              
	                 " GROUP BY inout_date, inout_code"
	                 ,greg_date            
	                 ,greg_time            
	                 ,gminor_code
	                 ,gproc_date
	                 );



	
                 
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5011_insert_comis_dd: mysql_query error...\n");
		ZzLOG(ERROR, "daem5011_insert_comis_dd: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (commit(con)!=0){
	    ZzLOG(ERROR, "daem5011_insert_comis_dd: commit error...\n");
		ZzLOG(ERROR, "daem5011_insert_comis_dd: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}

	return 0;
}


//******************************************************************************
//  daem5011_insert_buz_amt_dd()
//  - 해당일자의 캐쉬현황을 계산하여 insert한다.
//******************************************************************************
int daem5011_insert_buz_amt_dd()
{
	char szQuery[2000];	// query string
	double bef_amt;
	double cmt_amt;
	double sale_amt;
	double in_amt;
	double out_amt;
	double tot_amt;

	bef_amt  = 0;
	cmt_amt  = 0;
	sale_amt = 0;
	in_amt   = 0;
	out_amt  = 0;
	tot_amt  = 0;
	//--------------------------------------------------------------------------
	// 전일잔액을 구한다.
	//--------------------------------------------------------------------------

	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT tot_amt             "
	                 "  FROM zangsi_sum.T_BUZ_AMT_DD "
                     " WHERE io_date = date_format(date_add('%s',"
	                 ,gproc_date
	                 );
	strcat (szQuery, "INTERVAL -1 DAY),'%Y%m%d')");

	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5011_insert_buz_amt_dd: 전일잔액 SELECT mysql_query error...\n");
		ZzLOG(ERROR, "daem5011_insert_buz_amt_dd: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem5011_insert_buz_amt_dd: 전일잔액 SELECT mysql_store_result error...\n");
		ZzLOG(ERROR, "daem5011_insert_buz_amt_dd: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)!=0)	{
		if (row = mysql_fetch_row(res))	{
			bef_amt = getnum(row, 0);
		}
	}
	mysql_free_result(res);
	//--------------------------------------------------------------------------
	// 수수료/매출를 구한다.
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT IFNULL( "
	                 "       SUM(CASE cmt_code"
	                 "           WHEN '12' THEN cmt_amt"
	                 "           WHEN '13' THEN cmt_amt"
	                 "           WHEN '14' THEN cmt_amt"
	                 "           WHEN '21' THEN cmt_amt"
	                 "           ELSE 0"
	                 "           END),0) as '수수료' "
	                 "     , IFNULL("
	                 "       SUM(CASE cmt_code"
	                 "           WHEN '25' THEN cmt_amt"
	                 "           WHEN '26' THEN cmt_amt"
	                 "           WHEN '27' THEN cmt_amt"
	                 "           ELSE 0"
	                 "           END),0) as '매출' "
	                 "  FROM zangsi_sum.T_COMIS_DD     "
	                 " WHERE cmt_date = '%s'       "
	                 ,gproc_date
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5011_insert_buz_amt_dd: 수수료 SELECT mysql_query error...\n");
		ZzLOG(ERROR, "daem5011_insert_buz_amt_dd: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem5011_insert_buz_amt_dd: 수수료 SELECT mysql_store_result error...\n");
		ZzLOG(ERROR, "daem5011_insert_buz_amt_dd: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)!=0)	{
		if (row = mysql_fetch_row(res))	{
			cmt_amt  = getnum(row, 0);
			sale_amt = getnum(row, 1);
		}
	}
	mysql_free_result(res);
	//--------------------------------------------------------------------------
	// 입금/출금을 구한다.
	//--------------------------------------------------------------------------



	//--------------------------------------------------------------------------
	// 계산하여 Insert한다.
	//--------------------------------------------------------------------------
	tot_amt = bef_amt + cmt_amt + sale_amt + in_amt - out_amt;
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "INSERT INTO zangsi_sum.T_BUZ_AMT_DD"
	                 "     ( io_date   , bef_amt  "
	                 "     , cmt_amt   , sale_amt "
	                 "     , in_amt    , out_amt  "
	                 "     , tot_amt   ) VALUES   "
	                 "     ( '%s'      , %12.0f   "
	                 "     , %12.0f    , %12.0f   "
	                 "     , %12.0f    , %12.0f   "
	                 "     , %12.0f    )          "
	                 ,gproc_date
	                 ,bef_amt
	                 ,cmt_amt
	                 ,sale_amt
	                 ,in_amt
	                 ,out_amt
	                 ,tot_amt
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5011_insert_buz_amt_dd: mysql_query error...\n");
		ZzLOG(ERROR, "daem5011_insert_buz_amt_dd: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (commit(con)!=0){
	    ZzLOG(ERROR, "daem5011_insert_buz_amt_dd: commit error...\n");
		ZzLOG(ERROR, "daem5011_insert_buz_amt_dd: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}

	return 0;
}

//******************************************************************************
//  daem5011_insert_buz_cyb_dd()
//  - 해당일자의 사이버머니현황을 계산하여 insert한다.
//******************************************************************************
int daem5011_insert_buz_cyb_dd()
{
	char szQuery[2000];	// query string
	double bef_amt;		//전일잔액
	double inc_amt;		//충전
	double cash_amt;	//캐귀입금
	double cmt_amt;		//수수료
	double sale_amt;	//매출
	double out_amt;		//출금
	double tot_amt;		//합계

	bef_amt  = 0;
	inc_amt  = 0;
	cash_amt = 0;
	cmt_amt  = 0;
	sale_amt = 0;
	out_amt  = 0;
	tot_amt  = 0;
	//--------------------------------------------------------------------------
	// 전일잔액을 구한다.
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT tot_amt             "
	                 "  FROM zangsi_sum.T_BUZ_CYB_DD "
                     " WHERE io_date = date_format(date_add('%s',"
	                 ,gproc_date
	                 );
	strcat (szQuery, "INTERVAL -1 DAY),'%Y%m%d')");

	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5011_insert_buz_cyb_dd: 전일잔액 SELECT mysql_query error...\n");
		ZzLOG(ERROR, "daem5011_insert_buz_cyb_dd: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem5011_insert_buz_cyb_dd: 전일잔액 SELECT mysql_store_result error...\n");
		ZzLOG(ERROR, "daem5011_insert_buz_cyb_dd: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)!=0)	{
		if (row = mysql_fetch_row(res))	{
			bef_amt = getnum(row, 0);
		}
	}
	mysql_free_result(res);
	//--------------------------------------------------------------------------
	// 수수료/매출를 구한다.(코드 변경시 반드시 추가해야함!!!!)
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT IFNULL("
	                 "       SUM(CASE cmt_code"
	                 "           WHEN '31' THEN io_amt"
	                 "           WHEN '32' THEN io_amt"
	                 "           WHEN '33' THEN io_amt"
	                 "           WHEN '34' THEN io_amt"
	                 "           WHEN '35' THEN io_amt"
	                 "           WHEN '36' THEN io_amt"
	                 "           WHEN '37' THEN io_amt"
	                 "           WHEN '38' THEN io_amt"
	                 "           WHEN '39' THEN io_amt"
	                 "           WHEN '3A' THEN io_amt"	                 
	                 "           WHEN '3B' THEN io_amt"	                 
					 "           WHEN '3C' THEN io_amt"	                 
					 "           WHEN '3D' THEN io_amt"
					 "           WHEN '3E' THEN io_amt" 
					 "           WHEN '3F' THEN io_amt" 
					 "           WHEN '3G' THEN io_amt" 
					 "           WHEN '3H' THEN io_amt" 
					 "           WHEN '3I' THEN io_amt" 
					 "           WHEN '3J' THEN io_amt" 
					 "           WHEN '3K' THEN io_amt" 
					 "           WHEN '3L' THEN io_amt" 
					 "           WHEN '3Z' THEN io_amt" 
	                 "           ELSE 0"
	                 "           END),0) as '충전' "
	                 "     , SUM(if(cmt_code='19', io_amt,0)) as '캐쉬입금' "
	                 "     , IFNULL( "
	                 "       SUM(CASE cmt_code"
	                 "           WHEN '12' THEN cmt_amt"
	                 "           WHEN '13' THEN cmt_amt"
	                 "           WHEN '14' THEN cmt_amt"
	                 "           WHEN '21' THEN cmt_amt"
	                 "           WHEN '99' THEN cmt_amt"
	                 "           ELSE 0"
	                 "           END),0) as '수수료' "
	                 "     , IFNULL("
	                 "       SUM(CASE cmt_code"
	                 "           WHEN '25' THEN cmt_amt"
	                 "           WHEN '26' THEN cmt_amt"
	                 "           WHEN '27' THEN cmt_amt"
	                 "           ELSE 0"
	                 "           END),0) as '매출' "
	                 "  FROM zangsi_sum.T_COMIS_DD     "
	                 " WHERE cmt_date = '%s'       "
	                 ,gproc_date
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5011_insert_buz_cyb_dd: 수수료 SELECT mysql_query error...\n");
		ZzLOG(ERROR, "daem5011_insert_buz_cyb_dd: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem5011_insert_buz_cyb_dd: 수수료 SELECT mysql_store_result error...\n");
		ZzLOG(ERROR, "daem5011_insert_buz_cyb_dd: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)!=0)	{
		if (row = mysql_fetch_row(res))	{
			inc_amt  = getnum(row, 0);
			cash_amt = getnum(row, 1);
			cmt_amt  = getnum(row, 2);
			sale_amt = getnum(row, 3);
		}
	}
	mysql_free_result(res);
	//--------------------------------------------------------------------------
	// 계산하여 Insert한다.
	//--------------------------------------------------------------------------
	tot_amt = bef_amt + inc_amt + cash_amt - cmt_amt + sale_amt + out_amt;

	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "INSERT INTO zangsi_sum.T_BUZ_CYB_DD"
	                 "     ( io_date   , bef_amt  "
	                 "     , inc_amt   , cash_amt "
	                 "     , cmt_amt   , sale_amt "
	                 "     , out_amt   , tot_amt) VALUES"
	                 "     ( '%s'      , %12.0f   "
	                 "     , %12.0f    , %12.0f   "
	                 "     , %12.0f    , %12.0f   "
	                 "     , %12.0f    , %12.0f ) "
	                 ,gproc_date
	                 ,bef_amt
	                 ,inc_amt
	                 ,cash_amt
	                 ,cmt_amt
	                 ,sale_amt
	                 ,out_amt
	                 ,tot_amt
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5011_insert_buz_cyb_dd: mysql_query error...\n");
		ZzLOG(ERROR, "daem5011_insert_buz_cyb_dd: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (commit(con)!=0){
	    ZzLOG(ERROR, "daem5011_insert_buz_cyb_dd: commit error...\n");
		ZzLOG(ERROR, "daem5011_insert_buz_cyb_dd: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}

	return 0;
}

/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5011_get_sysdate()
{
	char szQuery[1000];		// query string
	char szQuery2[512];
	
	memset(szQuery, 0x00, sizeof(szQuery));
	memset(szQuery2, 0x00, sizeof(szQuery2));
	
	if (strcmp(gproc_date, "00000000") == 0)
	{
		strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d')");
		strcat(szQuery, "     , date_format(now(),'%H%i%s')");
		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -1 DAY),'%Y%m%d')");
	
	}
	else
	{
		sprintf( szQuery, "SELECT %s " , gproc_date );
		strcat(szQuery, "     , date_format(now(),'%H%i%s')");
		
		memset(szQuery2,0x00,sizeof(szQuery2));
		sprintf( szQuery2, "  , date_format(date_add(%s , INTERVAL -1 DAY), "  , gproc_date );
		strcat ( szQuery, szQuery2 );
		strcat ( szQuery, " '%Y%m%d') " );
		
		
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
	memset(greg_date , 0x00, sizeof(greg_date ));
	memset(greg_time , 0x00, sizeof(greg_time ));
	strcpy(greg_date , getstr(row, 0));
	strcpy(greg_time , getstr(row, 1));


	memset(gproc_date, 0x00, sizeof(gproc_date));
	strcpy(gproc_date, getstr(row, 2));

	
	mysql_free_result(res);

	return 0;
}


/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5011_init_process(int argc, char **argv)
{
	char stemp[128];
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("d_", "/logs/daemon"); 

    ZzLOG(ALWAY, "[daem5011]*****************프로그램 시작*****************\n");  

    // 파라미터 값 설정 및 초기화
    if (argc != 2) {
    	goto arg_error;
    }

	/* 처리일자 */
	memset(gproc_date, 0x00, sizeof(gproc_date));
	strcpy(gproc_date, argv[1]);

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(con=db_connect_nodb("")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1); 
	}
	
	//--------------------------------------------------------------------------
	// 처리일자 stting
	//--------------------------------------------------------------------------
	if (daem5011_get_sysdate() != 0){
		db_disconnect(con);
	   	return(-1); 
	}
	
    return (0);

arg_error:
    ZzLOG(ERROR, "usage : %s YYYYMMDD\n", argv[0]);
    ZzLOG(ERROR, "        YYYYMMDD : 처리일자(00000000:시스템일자)\n");

    ZzPRT(ERROR, "usage : %s YYYYMMDD\n", argv[0]);
    ZzPRT(ERROR, "        YYYYMMDD : 처리일자(00000000:시스템일자)\n");
    return -1;
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daem5011_term_process()
{
    // DB close
	db_disconnect(con);
    ZzLOG(ALWAY, "[daem5011]*****************프로그램 종료*****************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5011_signal(int nSignal)
{
    daem5011_term_process();
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
	signal(SIGTERM, daem5011_signal);
	signal(SIGINT,  daem5011_signal);
	signal(SIGQUIT, daem5011_signal);
	signal(SIGKILL, daem5011_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daem5011_init_process(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daem5011_main_process();
		/* 프로그램 종료루틴 */                    
		daem5011_term_process();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
