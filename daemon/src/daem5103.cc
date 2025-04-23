/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5103.cc
 *         기능 : 분류별 판매집계
 *         설명 : 1일 1회 작업한다.
 *                SYSTEM  (00000000) => sysdate - 1일 처리하며,
 *                직접입력(yyyymmdd) => yyyymmdd를  처리한다.
 *       작성자 : JDP / LEE
 *       작성일 : 2004/02/16
 *     수정이력 : 2007/11/30 
 *			      HCS
 *			      -포인트 추가
 *				  
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

int daem5103_init_process(int argc, char **argv);
int daem5103_main_process();
int daem5103_term_process();
int daem5103_delete_current();
int daem5103_insert_deal();
int daem5103_insert_fix_deal();
int daem5103_get_sysdate();
void daem5103_signal(int nSignal);

MYSQL     *con;

char   gsys_date  [  8+1];	//처리일자(sysdate)
char   gproc_date [  8+1];	//처리일자(sysdate-1)
char   gproc_yymm [  6+1];	//처리년월
char   greg_date  [  8+1];	//등록일
char   greg_time  [  6+1];	//등록시간
//******************************************************************************
//* daem5103 main
//******************************************************************************
int daem5103_main_process()
{
	//--------------------------------------------------------------------------
	// 트렌젝션시작
	//--------------------------------------------------------------------------
	if (tran_begin(con)!=0) {
	    ZzLOG(ERROR, "tran_begin: 테이베이스 오류입니다.\n");  
		ZzLOG(ERROR, "daem5103_main_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
	}

	// 처리일자의 자료삭제
	if (daem5103_delete_current() != 0)
		goto daem5103_main_process_err;

	// 판매 집계처리
	if (daem5103_insert_deal() != 0)
		goto daem5103_main_process_err;

	// 정액제 판매 집계 처리
	if (daem5103_insert_fix_deal() != 0)
		goto daem5103_main_process_err;

	if (tran_commit(con)!=0){
	    ZzLOG(ERROR, "daem5103_main_process: tran_commit error...\n");
		ZzLOG(ERROR, "daem5103_main_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    goto daem5103_main_process_err;
	}

	return (0);

daem5103_main_process_err:
	tran_rollback(con);
    return -1;
}


//******************************************************************************
//* daem5103_delete_current()
//* 처리일자의 자료를 삭제처리 한다.
//******************************************************************************
int daem5103_delete_current()
{
	char szQuery[1000];		// query string

	//--------------------------------------------------------------------------
	// 처리일자 해당되는 자료삭제(재작업을 위해)
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi_sum.T_SALE_DD"
	                 " WHERE deal_date    = '%s' "
	                 ,gproc_date
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5103_delete_current: DELETE zangsi.T_SALE_DD error...\n");
		ZzLOG(ERROR, "daem5103_delete_current: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
    }

	//--------------------------------------------------------------------------
	// 사용자별 거래건수 삭제
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi_sum.T_SALE_MM"
	                 " WHERE deal_yymm  = '%s'   "
	                 ,gproc_yymm
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5103_delete_current: DELETE zangsi.T_SALE_MM error...\n");
		ZzLOG(ERROR, "daem5103_delete_current: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
    }

	//--------------------------------------------------------------------------
	// 정액제 거래집계 삭제
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi_sum.T_FIX_SALE_DD"
	                 " WHERE deal_date  = '%s'   "
	                 ,gproc_date
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5103_delete_current: DELETE zangsi.T_FIX_SALE_DD error...\n");
		ZzLOG(ERROR, "daem5103_delete_current: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
    }

    return 0;
}


//******************************************************************************
//* daem5103_insert_deal()
//* 일일 사용자별 거래집계처리
//******************************************************************************
int daem5103_insert_deal()
{
	char szQuery[10000];		// query string
	int ret=0;
	int nRowcnt = 0;
	
	//--------------------------------------------------------------------------
	// 계산된 판매집계를 TEMPORARY에 생성한다.
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	
	/*
	TEMP_SALE_DD에 포인트 필드 추가.
	T_SALE_DD에 포인트 필드 추가.
	fixamt_yn이 'P'일 경우 포인트 필드는 1, 'P'가 아닐경우 0
	major_code = 24
	*/
	
	sprintf(szQuery, "CREATE TEMPORARY TABLE zangsi_sum.TEMP_SALE_DD   "
	                 "SELECT deal_date "
	                 "     , if( cont_gu ='FD' , cont_gu, sect_code )  sect_code "
	                 "     , 0                            as reg_cnt         "
	                 "     , sum(if(fixamt_yn in('1','2','3','4','5','6','7'), 1 ,0)) as fix_cnt " 
	                 "     , sum(if(fixamt_yn in('0', '8'), 1 ,0)) as sale_cnt "
                     "     , sum(if(fixamt_yn in('0', '8'), sale_amt, 0)) as sale_amt " //no.777
	                 "     , sum(comp_amt)                as comp_amt"
	                 "     , count(distinct(buy_user))    as buy_user"
	                 "     , count(distinct(sale_user))   as sale_user"
                     "    , sum(if(fixamt_yn = '9', 1, 0)) as coupon_cnt "
                     "    , sum(if(fixamt_yn = '9', price_amt, 0)) as coupon_amt "
                     "    , sum(if(fixamt_yn = 'P', 1, 0)) as point_cnt "
                     "    , sum(if(fixamt_yn = 'P', price_amt, 0)) as point_amt "
                     "    , sum(if(fixamt_yn = 'C', 1, 0)) as cpr_cnt "
                     "    , sum(if(fixamt_yn = 'C', price_amt, 0)) as cpr_amt "
                     "     , sum(if(fixamt_yn in('0', '8'), price_amt, 0)) as price_amt " //no.777
	                 "  FROM zangsi.T_DEAL_INFO             "
	                 " WHERE deal_date = '%s'               "
	                 " GROUP BY deal_date, if( cont_gu ='FD' , cont_gu, sect_code )"
	                 "UNION ALL                             "
	                 "SELECT reg_date         as deal_date  "
	                 "     , sect_code        as sect_code  "
	                 "     , count(reg_user)  as reg_cnt    "
	                 "     , 0                as fix_cnt    "
	                 "     , 0                as sale_cnt   "
	                 "     , 0                as sale_amt   "
	                 "     , 0                as comp_amt   "
	                 "     , 0                as buy_user   "
	                 "     , 0                as sale_user  "
	                 "     , 0                as coupon_cnt "	
	                 "     , 0                as coupon_amt "	
	                 "     , 0                as point_cnt "	
	                 "     , 0                as point_amt "	
	                 "     , 0                as cpr_cnt "	
	                 "     , 0                as cpr_amt "	
	                 "     , 0                as price_amt "	
	                 "  FROM zangsi.T_CONTENTS_INFO         "
	                 " WHERE reg_date = '%s'                "
	                 " GROUP BY reg_date, sect_code         "
	                 "UNION ALL                             "
	                 "SELECT reg_date         as deal_date  "
	                 "     , 'FD'             as sect_code  "
	                 "     , count(id)        as reg_cnt    "
	                 "     , 0                as fix_cnt    "
	                 "     , 0                as sale_cnt   "
	                 "     , 0                as sale_amt   "
	                 "     , 0                as comp_amt   "
	                 "     , 0                as buy_user   "
	                 "     , 0                as sale_user  "
	                 "     , 0                as coupon_cnt "	
	                 "     , 0                as coupon_amt "	
	                 "     , 0                as point_cnt "	
	                 "     , 0                as point_amt "	
	                 "     , 0                as cpr_cnt "	
	                 "     , 0                as cpr_amt "	
	                 "     , 0                as price_amt "	
	                 "  FROM zangsi.T_CONTFLOG_INFO         "
	                 " WHERE reg_date = '%s'                "
	                 " GROUP BY reg_date, sect_code         "
	                 ,gproc_date
	                 ,gproc_date
	                 ,gproc_date
	                 );
	                 
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5103_insert_deal: CREATE zangsi_sum.TEMP_DEAL reg_cnt error...\n");
		ZzLOG(ERROR, "daem5103_insert_deal: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
    }
	//--------------------------------------------------------------------------
	// 해당일자의 자료를 생성한다.
	//--------------------------------------------------------------------------
	ret = 0;
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "INSERT INTO zangsi_sum.T_SALE_DD   		 "
	                 "     ( deal_date  , sect_code  		 "
	                 "     , reg_cnt    , fix_cnt    		 "
	                 "     , sale_cnt   , sale_amt   		 "
	                 "     , comp_amt   , buy_user   		 "
	                 "     , sale_user  , coupon_cnt	, coupon_amt 		 "
	                 "     , point_cnt	, point_amt		, cpr_cnt	, cpr_amt , price_amt	 "
					 "     , reg_date   , reg_time   		 "
	                 "     )           				 		 "
	                 "SELECT deal_date     , sect_code    	 "
	                 "     , sum(reg_cnt)  , sum(fix_cnt) 	 "
	                 "     , sum(sale_cnt) , sum(sale_amt)	 "
	                 "     , sum(comp_amt) , sum(buy_user)	 "
	                 "     , sum(sale_user), sum(coupon_cnt) , sum(coupon_amt) "
	                 "     , sum(point_cnt), sum(point_amt)	 , sum(cpr_cnt)   , sum(cpr_amt), sum(price_amt)"
					 "     , '%s', '%s'   					 "
	                 "  FROM zangsi_sum.TEMP_SALE_DD                 	 "
	                 " GROUP BY deal_date, sect_code      	 "
	                 ,greg_date
	                 ,greg_time);
	                 
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5103_insert_deal: INSERT T_SALE_DD error...\n");
		ZzLOG(ERROR, "daem5103_insert_deal: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    ret = -1;
    }
    
	//--------------------------------------------------------------------------
	// TEMPORARY 테이블을 삭제 한다.
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DROP TABLE zangsi_sum.TEMP_SALE_DD ");
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5103_insert_deal: DROP zangsi_sum.TEMP_SALE_DD error...\n");
		ZzLOG(ERROR, "daem5103_insert_deal: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    ret = -1;
    }
	if (ret != 0) return ret;

	//--------------------------------------------------------------------------
	// 계산된 판매집계를 TEMPORARY에 생성한다. 20100217 -- HCS : 시간대별 판매 집계
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	
	/*
	TEMP_SALE_DD에 포인트 필드 추가.
	T_SALE_DD에 포인트 필드 추가.
	fixamt_yn이 'P'일 경우 포인트 필드는 1, 'P'가 아닐경우 0
	*/
	
	sprintf(szQuery, "CREATE TEMPORARY TABLE zangsi_sum.TEMP_SALE_HOUR_SUM   "
	                 "SELECT deal_date, substring(deal_time, 1, 2) as deal_time "
	                 "     , if(share_meth ='00', '00',  if( cont_gu ='FD' , cont_gu, sect_code ) ) sect_code "
	                 "     , 0                            as reg_cnt         "
	                 "     , sum(if(share_meth ='00', 0, if(sale_amt=0, 1 ,0))) as fix_cnt " 
	                 "     , sum(if(share_meth ='00', 1, if(sale_amt>0, 1 ,0))) as sale_cnt "
                     "     , sum(if(fixamt_yn in('0', '8'), sale_amt, 0)) as sale_amt " //no.777
	                 "     , sum(comp_amt)                as comp_amt"
	                 "     , count(distinct(buy_user))    as buy_user"
	                 "     , count(distinct(sale_user))   as sale_user"
                     "    , sum(if(isnull(coupon_code), 0, if(coupon_code = '99', 0, 1))) as coupon_cnt "
                     "    , sum(if(isnull(coupon_code), 0, if(coupon_code = '99', 0, price_amt))) as coupon_amt "
                     "    , sum(if(fixamt_yn = 'P', 1, 0)) as point_cnt "
                     "    , sum(if(fixamt_yn = 'P', price_amt, 0)) as point_amt "
                     "    , sum(if(fixamt_yn = 'C', 1, 0)) as cpr_cnt "
                     "    , sum(if(fixamt_yn = 'C', price_amt, 0)) as cpr_amt "
                     "     , sum(if(fixamt_yn in('0', '8'), price_amt, 0)) as price_amt " //no.777
	                 "  FROM zangsi.T_DEAL_INFO             "
	                 " WHERE deal_date = '%s'               "
	                 " GROUP BY deal_date, substring(deal_time, 1, 2), if(share_meth ='00', '00', if( cont_gu ='FD' , cont_gu, sect_code ))"
	                 "UNION ALL                             "
	                 "SELECT reg_date         as deal_date  "
	                 "     , substring(reg_time, 1, 2)        as deal_time  "
	                 "     , sect_code        as sect_code  "
	                 "     , count(reg_user)  as reg_cnt    "
	                 "     , 0                as fix_cnt    "
	                 "     , 0                as sale_cnt   "
	                 "     , 0                as sale_amt   "
	                 "     , 0                as comp_amt   "
	                 "     , 0                as buy_user   "
	                 "     , 0                as sale_user  "
	                 "     , 0                as coupon_cnt "	
	                 "     , 0                as coupon_amt "	
	                 "     , 0                as point_cnt "	
	                 "     , 0                as point_amt "	
	                 "     , 0                as cpr_cnt "	
	                 "     , 0                as cpr_amt "	
	                 "     , 0                as price_amt "	
	                 "  FROM zangsi.T_CONTENTS_INFO         "
	                 " WHERE reg_date = '%s'                "
	                 " GROUP BY reg_date, substring(reg_time, 1, 2), sect_code         "
	                 "UNION ALL                             "
	                 "SELECT reg_date         as deal_date  "
	                 "     , substring(reg_time, 1, 2)        as deal_time  "
	                 "     , 'FD'             as sect_code  "
	                 "     , count(id)        as reg_cnt    "
	                 "     , 0                as fix_cnt    "
	                 "     , 0                as sale_cnt   "
	                 "     , 0                as sale_amt   "
	                 "     , 0                as comp_amt   "
	                 "     , 0                as buy_user   "
	                 "     , 0                as sale_user  "
	                 "     , 0                as coupon_cnt "	
	                 "     , 0                as coupon_amt "	
	                 "     , 0                as point_cnt "	
	                 "     , 0                as point_amt "	
	                 "     , 0                as cpr_cnt "	
	                 "     , 0                as cpr_amt "	
	                 "     , 0                as price_amt "	
	                 "  FROM zangsi.T_CONTFLOG_INFO         "
	                 " WHERE reg_date = '%s'                "
	                 " GROUP BY reg_date, substring(reg_time, 1, 2), sect_code         "
	                 ,gproc_date
	                 ,gproc_date
	                 ,gproc_date
	                 );
	                 
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5103_insert_deal: CREATE zangsi_sum.TEMP_DEAL reg_cnt error...\n");
		ZzLOG(ERROR, "daem5103_insert_deal: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
    }
	//--------------------------------------------------------------------------
	// 해당일자의 자료를 생성한다. 20100217 -- HCS : 시간대별 판매 집계
	//--------------------------------------------------------------------------
	ret = 0;
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "INSERT INTO zangsi_sum.T_SALE_HOUR_SUM   		 "
	                 "     ( deal_date  , deal_time, sect_code  		 "
	                 "     , reg_cnt    , fix_cnt    		 "
	                 "     , sale_cnt   , sale_amt   		 "
	                 "     , comp_amt   , buy_user   		 "
	                 "     , sale_user  , coupon_cnt	, coupon_amt 		 "
	                 "     , point_cnt	, point_amt		, cpr_cnt	, cpr_amt , price_amt	 "
					 "     , reg_date   , reg_time   		 "
	                 "     )           				 		 "
	                 "SELECT deal_date  , deal_time   , sect_code    	 "
	                 "     , sum(reg_cnt)  , sum(fix_cnt) 	 "
	                 "     , sum(sale_cnt) , sum(sale_amt)	 "
	                 "     , sum(comp_amt) , sum(buy_user)	 "
	                 "     , sum(sale_user), sum(coupon_cnt) , sum(coupon_amt) "
	                 "     , sum(point_cnt), sum(point_amt)	 , sum(cpr_cnt)   , sum(cpr_amt), sum(price_amt)"
					 "     , '%s', '%s'   					 "
	                 "  FROM zangsi_sum.TEMP_SALE_HOUR_SUM                 	 "
	                 " GROUP BY deal_date, deal_time, sect_code      	 "
	                 ,greg_date
	                 ,greg_time);
	                 
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5103_insert_deal: INSERT T_SALE_DD error...\n");
		ZzLOG(ERROR, "daem5103_insert_deal: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    ret = -1;
    }
    
	//--------------------------------------------------------------------------
	// TEMPORARY 테이블을 삭제 한다. 20100217 -- HCS : 시간대별 판매 집계
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DROP TABLE zangsi_sum.TEMP_SALE_HOUR_SUM ");
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5103_insert_deal: DROP zangsi_sum.TEMP_SALE_DD error...\n");
		ZzLOG(ERROR, "daem5103_insert_deal: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    ret = -1;
    }
	if (ret != 0) return ret;


	//--------------------------------------------------------------------------
	// 해당일자의 자료를 생성한다.
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "INSERT INTO zangsi_sum.T_SALE_MM  				"
	                 "     ( deal_yymm  , sect_code 				"
	                 "     , reg_cnt    , fix_cnt   				"
	                 "     , sale_cnt   , sale_amt  				"
	                 "     , comp_amt   , buy_user  				"
	                 "     , sale_user  , coupon_cnt	, coupon_amt 		 "
	                 "     , point_cnt	, point_amt		, cpr_cnt	, cpr_amt, price_amt	 "
			 		 "     , reg_date   , reg_time 					"
	                 "     )           								"
	                 "SELECT SUBSTRING(deal_date,1,6)     			"
	                 "     , sect_code                    			"
	                 "     , sum(reg_cnt)  , sum(fix_cnt) 			"
	                 "     , sum(sale_cnt) , sum(sale_amt)			"
	                 "     , sum(comp_amt) , sum(buy_user)			"
	                 "     , sum(sale_user), sum(coupon_cnt) , sum(coupon_amt) "
	                 "     , sum(point_cnt), sum(point_amt)	 , sum(cpr_cnt)   , sum(cpr_amt), sum(price_amt)"
			 		 "     , '%s', '%s'   							"
	                 "  FROM zangsi_sum.T_SALE_DD             			"
	                 " WHERE deal_date >= concat('%s','01')			"
	                 "   AND deal_date <= concat('%s','99')			"
	                 " GROUP BY SUBSTRING(deal_date,1,6), sect_code "
	                 ,greg_date
	                 ,greg_time
	                 ,gproc_yymm
	                 ,gproc_yymm);
                 
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5103_insert_deal: INSERT T_SALE_MM error...\n");
		ZzLOG(ERROR, "daem5103_insert_deal: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
    }
	
	return 0;
}

//******************************************************************************
//* daem5103_insert_fix_deal()
//* 일일 사용자별 정액제 거래집계처리
//******************************************************************************
int daem5103_insert_fix_deal()
{
	/*
	하루 정액 구매건이 100건 이상 되는 회원의 E-mail, 최종 접속 IP, 총 구매 건수를 집계한다.
	*/

	MYSQL_RES *fix_res;
	MYSQL_ROW  fix_row;
	
	char szQuery[10000];		// query string
	int ret=0;
	int nRowcnt = 0;
	
	//--------------------------------------------------------------------------
	// 하루 정액 구매건이 100건 이상 되는 회원 조회
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select buy_user from zangsi.T_DEAL_INFO "
					 " where deal_date = '%s' and fixamt_yn in ('1','2','3','4','5','6','7') "
					 " group by buy_user having count(deal_no) >= 100 "
					 , gproc_date);
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5103_insert_fix_deal: mysql_query error...\n");
		ZzLOG(ERROR, "daem5103_insert_fix_deal: [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
	    return -1;
    }
    if (!(fix_res = mysql_store_result(con)))
    {
	    ZzLOG(ERROR, "daem5103_insert_fix_deal: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem5103_insert_fix_deal: [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
	    return -1;
    }
    if (mysql_num_rows(fix_res)==0)
    {
    	mysql_free_result(fix_res);
	    ZzLOG(ALWAY, "daem5103_insert_fix_deal: 100건 이상 구매자 없음...\n");
		ZzLOG(ALWAY, "daem5103_insert_fix_deal: [%s]\n", szQuery);
	    return 0;
    }	
	//--------------------------------------------------------------------------
	// 해당일자의 자료를 생성한다.
	//--------------------------------------------------------------------------
	while(fix_row = mysql_fetch_row(fix_res))
	{
		char szBuyUser[12+1];
		memset(szBuyUser, 0x00, sizeof(szBuyUser));
		sprintf(szBuyUser, "%s", getstr(fix_row,0));
		
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi_sum.T_FIX_SALE_DD "
						 " (deal_date, buy_user) "
						 " values "
						 " ('%s', '%s') "
						 , gproc_date
						 , szBuyUser);
		if (mysql_query(con, szQuery))
		{
			mysql_free_result(fix_res);
		    ZzLOG(ERROR, "daem5103_insert_fix_deal: mysql_query error...\n");
			ZzLOG(ERROR, "daem5103_insert_fix_deal: [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
		    return -1;
	    }
	}
	mysql_free_result(fix_res);
	
	//--------------------------------------------------------------------------
	// 100건이상 구매자의 E-mail, 최종 접속 IP, 총 구매 건수 입력
	//--------------------------------------------------------------------------
	MYSQL_RES *res;
	MYSQL_ROW  row;
	
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select buy_user from zangsi_sum.T_FIX_SALE_DD where deal_date = '%s' ", gproc_date);

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5103_insert_fix_deal: mysql_query error...\n");
		ZzLOG(ERROR, "daem5103_insert_fix_deal: [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
	    return -1;
    }
    if (!(fix_res = mysql_store_result(con)))
    {
	    ZzLOG(ERROR, "daem5103_insert_fix_deal: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem5103_insert_fix_deal: [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
	    return -1;
    }
    if (mysql_num_rows(fix_res)==0)
    {
    	mysql_free_result(fix_res);
	    ZzLOG(ERROR, "daem5103_insert_fix_deal: mysql_num_rows error...\n");
		ZzLOG(ERROR, "daem5103_insert_fix_deal: [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
	    return -1;
    }
	while(fix_row = mysql_fetch_row(fix_res))
	{
		char szUserId[12+1];
		memset(szUserId, 0x00, sizeof(szUserId));
		sprintf(szUserId, "%s", getstr(fix_row,0));
		
		//--------------------------------------------------------------------------
		// 총 구매 건수 조회
		//--------------------------------------------------------------------------
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " select count(deal_no) from zangsi.T_DEAL_INFO where deal_date = '%s' and buy_user = '%s' "
						 , gproc_date, szUserId);
		if (mysql_query(con, szQuery))
		{
	    	mysql_free_result(fix_res);
		    ZzLOG(ERROR, "daem5103_insert_fix_deal: mysql_query error...\n");
			ZzLOG(ERROR, "daem5103_insert_fix_deal: [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
		    return -1;
	    }
	    if (!(res = mysql_store_result(con)))
	    {
	    	mysql_free_result(fix_res);
		    ZzLOG(ERROR, "daem5103_insert_fix_deal: mysql_store_result error...\n");
			ZzLOG(ERROR, "daem5103_insert_fix_deal: [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
		    return -1;
	    }
	    if (mysql_num_rows(res)==0)
	    {
	    	mysql_free_result(fix_res);
	    	mysql_free_result(res);
		    ZzLOG(ERROR, "daem5103_insert_fix_deal: mysql_num_rows error...\n");
			ZzLOG(ERROR, "daem5103_insert_fix_deal: [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
		    return -1;
	    }
		row = mysql_fetch_row(res);
		int nDownCnt = 0;
		nDownCnt = (int)getint(row,0);
		mysql_free_result(res);
		
		//--------------------------------------------------------------------------
		// E-mail, 최종 접속 IP 조회
		//--------------------------------------------------------------------------
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " select a.email, b.conn_ip from zangsi.T_USER_INFO a, zangsi.T_USER_STAT b "
						 " where a.user_id = b.user_id and a.user_id = '%s' "
						 , szUserId);
		if (mysql_query(con, szQuery))
		{
	    	mysql_free_result(fix_res);
		    ZzLOG(ERROR, "daem5103_insert_fix_deal: mysql_query error...\n");
			ZzLOG(ERROR, "daem5103_insert_fix_deal: [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
		    return -1;
	    }
	    if (!(res = mysql_store_result(con)))
	    {
	    	mysql_free_result(fix_res);
		    ZzLOG(ERROR, "daem5103_insert_fix_deal: mysql_store_result error...\n");
			ZzLOG(ERROR, "daem5103_insert_fix_deal: [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
		    return -1;
	    }
	    if (mysql_num_rows(res)==0)
	    {
	    	mysql_free_result(res);
		    ZzLOG(ERROR, "daem5103_insert_fix_deal: mysql_num_rows error...\n");
			ZzLOG(ERROR, "daem5103_insert_fix_deal: [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
		    continue;
	    }
		row = mysql_fetch_row(res);
		char szEmail[100+1];
		memset(szEmail, 0x00, sizeof(szEmail));
		sprintf(szEmail, "%s", getstr(row,0));
		
		char szConnIp[16+1];
		memset(szConnIp, 0x00, sizeof(szConnIp));
		sprintf(szConnIp, "%s", getstr(row,1));
		
		mysql_free_result(res);
		
		char szAddQuery[1500];
		memset(szAddQuery, 0x00, sizeof(szAddQuery));
		if(strlen(szEmail) > 0)
			sprintf(szAddQuery, " set email = '%s', conn_ip = '%s', down_cnt = %d", szEmail, szConnIp, nDownCnt);
		else
			sprintf(szAddQuery, " set conn_ip = '%s', down_cnt = %d", szConnIp, nDownCnt);
		
		
		//--------------------------------------------------------------------------
		// 구한 값 업데이트
		//--------------------------------------------------------------------------
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update zangsi_sum.T_FIX_SALE_DD "
						 " %s "
						 " where deal_date = '%s' and buy_user = '%s' "
						 , szAddQuery, gproc_date, szUserId);
		if (mysql_query(con, szQuery))
		{
	    	mysql_free_result(fix_res);
		    ZzLOG(ERROR, "daem5103_insert_fix_deal: mysql_query error...\n");
			ZzLOG(ERROR, "daem5103_insert_fix_deal: [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
		    return -1;
	    }
	}    
	mysql_free_result(fix_res);
	
	return 0;
}


/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5103_get_sysdate()
{
	MYSQL_RES *res;
	MYSQL_ROW  row;
	
	char szQuery[1000];		// query string
	char sztemp [100];      // query temp

	memset(szQuery, 0x00, sizeof(szQuery));
	if (strcmp(gsys_date, "00000000")==0)
	{
		strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d')");
		strcat(szQuery, "     , date_format(now(),'%H%i%s')");
		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -1 DAY),'%Y%m%d')");
		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -1 DAY),'%Y%m')");
	}
	else
	{
		/*
		sprintf( szQuery, "SELECT %s " , gproc_date );
		strcat(szQuery, "     , date_format(now(),'%H%i%s')");
		
		memset(szQuery2,0x00,sizeof(szQuery2));
		sprintf( szQuery2, "  , date_format(date_add(%s , INTERVAL -1 DAY), "  , gproc_date );
		strcat ( szQuery, szQuery2 );
		strcat ( szQuery, " '%Y%m%d') " );
		
		memset(szQuery2,0x00,sizeof(szQuery2));
		sprintf( szQuery2, "  , date_format(date_add(%s , INTERVAL -1 DAY), "  , gproc_date );
		strcat ( szQuery, szQuery2 );
		strcat ( szQuery, " '%Y%m') " );		
		*/		
		
		strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d')");
		strcat(szQuery, "     , date_format(now(),'%H%i%s')");
		strcat(szQuery, "     , '");
		strcat(szQuery, gsys_date);
		strcat(szQuery, "'");
		strcat(szQuery, "     , '");
		strncat(szQuery, gsys_date, 6);
		strcat(szQuery, "'");

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
	memset(gproc_date, 0x00, sizeof(gproc_date));
	memset(gproc_yymm, 0x00, sizeof(gproc_yymm));

	strcpy(greg_date ,   getstr(row, 0));
	strcpy(greg_time ,   getstr(row, 1));
	strcpy(gproc_date,   getstr(row, 2));
	strcpy(gproc_yymm,   getstr(row, 3));
	
	mysql_free_result(res);


	return 0;
}

/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5103_init_process(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("d_", "/logs/daemon"); 

    ZzLOG(ALWAY, "[daem5103]***************프로그램 시작***************\n");  

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
	ret=daem5103_get_sysdate();
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
int daem5103_term_process()
{
    // DB close
	db_disconnect(con);
    ZzLOG(ALWAY, "[daem5103]***************프로그램 종료***************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5103_signal(int nSignal)
{
    daem5103_term_process();
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
	signal(SIGTERM, daem5103_signal);
	signal(SIGINT,  daem5103_signal);
	signal(SIGQUIT, daem5103_signal);
	signal(SIGKILL, daem5103_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);

	if ( daem5103_init_process(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daem5103_main_process();
	
		/* 프로그램 종료루틴 */                    
		daem5103_term_process();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
