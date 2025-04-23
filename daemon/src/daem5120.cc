/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5120.cc
 *         기능 : 통계정보 생성 (zangsi_sum.T_STAT_TOT)
 *         설명 : 
 *       작성자 : LEE
 *       작성일 : 2007/08/01
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

int daem5120_init_process(int argc, char **argv); 

int daem5120_main_process(); 
int daem5120_term_process(); 

int daem5120_cmt();
int daem5120_conn_user(); 
int daem5120_stat_user();
int daem5120_today_login_user();
int daem5120_relogin_user();
int daem5120_total_in_user();

int daem5120_total_out_amt();
int daem5120_total_uplaod_contents();
int daem5120_acct_period();
int daem5120_total_sale_cnt();

int daem5120_acct_period_loop();
int daem5120_acct_period_percent(); // acct_period 뒤에 항시 실행

int daem5120_get_login_info();



int daem5120_get_sysdate(); 
void daem5120_signal(int nSignal); 

MYSQL     *con;
MYSQL_RES *res; 
MYSQL_ROW  row; 

MYSQL_RES *res_backup;
MYSQL_ROW  row_backup;
MYSQL     *con_backup;

char   gsys_date  [  8+1];	//처리일자(sysdate)
char   gproc_date [  8+1];	//처리일자(sysdate-1)
char   gproc_yymm [  6+1];	//처리년월
char   greg_date  [  8+1];	//등록일
char   greg_time  [  6+1];	//등록시간
//******************************************************************************
//* daem5120 main
//******************************************************************************
int daem5120_main_process()
{

	//--------------------------------------------------------------------------
	// 트렌젝션시작
	//--------------------------------------------------------------------------
	if (tran_begin(con)!=0) 
	{
	    ZzLOG(ERROR, "tran_begin: 테이베이스 오류입니다.\n");  
		ZzLOG(ERROR, "daem5120_main_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
	}


	//DB 생성 및 결제액 집계
	if( daem5120_cmt() != 0 )
		goto daem5120_main_process_err;


	// 서버접속자수 0시기준 (웹, 다운, 업) 
	if ( daem5120_conn_user() != 0 )
		goto daem5120_main_process_err;
		
	// 회원현황(가입, 탈퇴) 
	if ( daem5120_stat_user() != 0 )
		goto daem5120_main_process_err;	
	
	// 방문자수 집계처리 
	if ( daem5120_today_login_user() != 0 )
		goto daem5120_main_process_err;				

				
	// 전체 회원수(탈퇴제외) 
	if ( daem5120_total_in_user() != 0 )
		goto daem5120_main_process_err;
	else
	{
		// 15일 이내 재로그인
		if ( daem5120_relogin_user() != 0 )
			goto daem5120_main_process_err;
	}
		
	// 출금액집계 
	if( daem5120_total_out_amt() != 0 )
		goto daem5120_main_process_err;
		
	// 컨텐츠 등록건수 집계 
	if( daem5120_total_uplaod_contents() != 0 )
		goto daem5120_main_process_err;

	// 결제주기 계산 
	if( daem5120_acct_period() != 0 )
		goto daem5120_main_process_err;
	else
	{
		// 결제율 계산 
		 if( daem5120_acct_period_percent() != 0 )
		 	goto daem5120_main_process_err;
	}

	if( daem5120_total_sale_cnt() != 0 )
		goto daem5120_main_process_err;
		
	
		
	


	if (tran_commit(con)!=0)
	{
	    ZzLOG(ERROR, "daem5120_main_process: tran_commit error...\n");
		ZzLOG(ERROR, "daem5120_main_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    goto daem5120_main_process_err;
	}

	return (0);

daem5120_main_process_err:
	tran_rollback(con);
    return -1;
}

//******************************************************************************
//* daem5120_cmt()
//* 결제액 및 DB 추가 
//* daem5120_cmt() 프로세스 이후 실행
//******************************************************************************
int daem5120_cmt()
{
	char szQuery[10000];		// query string
	long total_cnt = 0; 
	long today_cnt = 0; 
	

	int ret=0;

	//--------------------------------------------------------------------------
	// 모든 결제 처리
	//--------------------------------------------------------------------------


	memset(szQuery, 0x00, sizeof(szQuery));
	
	//2009-05-19 : HCS - 틴캐쉬, T_cash 추가.
	sprintf(szQuery, " insert into zangsi_sum.T_STAT_TOT ( "
					 " stat_date, charge ) "
					 " select  cmt_date          , sum(io_amt) as ss   from zangsi_sum.T_COMIS_DD "
					 "  where   cmt_code >= '31' and cmt_code <=  '3Z' "
					 "  and cmt_date = date_format(date_add('%s' " , greg_date);
	strcat( szQuery ," ,INTERVAL -1 DAY ),'%Y%m%d') group by 1  ");
					  

	ZzLOG(ALWAY, "daem5120_cmt : %s\n\n",szQuery);

	if (mysql_query(con, szQuery))
	{
	  ZzLOG(ERROR, "daem5120_cmt : mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}

	return 0;	
}

//******************************************************************************
//* daem5120_acct_period_percent()
//* 결제율 계산
//* daem5120_acct_period() 프로세스 이후 실행
//******************************************************************************
int daem5120_acct_period_percent()
{
		
	char szQuery[10000];		// query string
	long total_cnt = 0; 
	long today_cnt = 0; 
	

	int ret=0;

	//--------------------------------------------------------------------------
	// 계좌이체를 제외한 모든 결제 처리
	//--------------------------------------------------------------------------


	memset(szQuery, 0x00, sizeof(szQuery));
	
	strcpy(szQuery, "select count(*) from zangsi_sum.T_ACCT_PERIOD where acct_date_last is not null  ");

	ZzLOG(ALWAY, "daem5120_acct_period_percent : %s\n\n",szQuery);

	if (mysql_query(con, szQuery))
	{
	  ZzLOG(ERROR, "daem5120_acct_period_percent : mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	

	if (!(res = mysql_store_result(con)))
	{
	  ZzLOG(ERROR, "daem5120_acct_period_percent: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return  -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	  ZzLOG(ERROR, "daem5120_acct_period_percent: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
	row = mysql_fetch_row(res);
	total_cnt = getint(row,0); 
	mysql_free_result(res);
	
	memset(szQuery, 0x00, sizeof(szQuery));
	
	sprintf(szQuery, "select re_charge_term_15+ re_charge_term_30+ re_charge_term_45 + re_charge_term_60 + re_charge_term_end as today_cnt "
									" from zangsi_sum.T_STAT_TOT where stat_date = date_format(date_add('%s' " ,  greg_date );
	strcat( szQuery ," ,INTERVAL -1 DAY ),'%Y%m%d')  ");

	ZzLOG(ALWAY, "daem5120_acct_period_percent : %s\n\n",szQuery);

	if (mysql_query(con, szQuery))
	{
	  ZzLOG(ERROR, "daem5120_acct_period_percent : mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	

	if (!(res = mysql_store_result(con)))
	{
	  ZzLOG(ERROR, "daem5120_acct_period_percent: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return  -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	  ZzLOG(ERROR, "daem5120_acct_period_percent: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
	row = mysql_fetch_row(res);
	today_cnt = getint(row,0); 
	mysql_free_result(res);

	

	//    오늘결제내역/전체재결제내역 = 재결제율

	memset(szQuery, 0x00, sizeof(szQuery));
	
	double percent = 0;
	percent = ((double)today_cnt/(double)total_cnt)*(double)100;
	
	#ifdef _DEBUG_
	percent = 0;
	#endif
			
	sprintf(szQuery, "update  zangsi_sum.T_STAT_TOT  set re_charge_per = %3.2f where stat_date = date_format(date_add('%s' "
	 , percent ,  greg_date );
	strcat( szQuery ," ,INTERVAL -1 DAY ),'%Y%m%d')  ");
	
	ZzLOG(ALWAY, "daem5120_acct_period_percent : [ today - %d ] / [  total - %d ]  : %s\n\n",today_cnt , total_cnt , szQuery);

	if (mysql_query(con, szQuery))
	{
	  ZzLOG(ERROR, "daem5120_acct_period_percent : mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	
	return 0;
	

	
	
}

//******************************************************************************
//* daem5120_acct_period_percent()
//* 판매 집계 계산
//******************************************************************************
int daem5120_total_sale_cnt()
{
		
	char szQuery[10000];		// query string
	int total_cnt = 0; 
	int nBuyUserCnt = 0;

	int ret=0;

	//--------------------------------------------------------------------------
	// 판매 집계(T_SALE_DD의 fix_cnt + sale_cnt 값.)
	//--------------------------------------------------------------------------
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT count(deal_no) "
	                 "  FROM zangsi.T_DEAL_INFO             "
	                 " where deal_date = date_format(date_add('%s'  "
					 ,  greg_date );

	/*
	sprintf(szQuery, "select sum(fix_cnt + sale_cnt + coupon_cnt + point_cnt + cpr_cnt) "
					 " from zangsi_sum.T_SALE_DD where deal_date = date_format(date_add('%s' " 
					 ,  greg_date );
	*/

	strcat( szQuery ," ,INTERVAL -1 DAY ),'%Y%m%d')  ");

	ZzLOG(ALWAY, "daem5120_total_sale_cnt : %s\n\n",szQuery);

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "daem5120_total_sale_cnt : mysql_query error. [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con)))
	{
		ZzLOG(ERROR, "daem5120_total_sale_cnt: mysql_store_result error. [%d](%s)",mysql_errno(con), mysql_error(con));
		return  -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
		ZzLOG(ERROR, "daem5120_total_sale_cnt: mysql_num_rows error. [%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
	row = mysql_fetch_row(res);
	total_cnt = getint(row,0); 
	mysql_free_result(res);
	
	//--------------------------------------------------------------------------
	// 하루동안 구매 회원수 -- 20100716
	//--------------------------------------------------------------------------
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " SELECT count(distinct(user_id)) from zangsi_sum.T_USER_DEAL_DD where deal_date = '%s' ",  gproc_date );

	ZzLOG(ALWAY, "daem5120_total_sale_cnt : %s\n\n",szQuery);

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "daem5120_total_sale_cnt : mysql_query error. [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con)))
	{
		ZzLOG(ERROR, "daem5120_total_sale_cnt: mysql_store_result error. [%d](%s)",mysql_errno(con), mysql_error(con));
		return  -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
		ZzLOG(ERROR, "daem5120_total_sale_cnt: mysql_num_rows error. [%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
	row = mysql_fetch_row(res);
	nBuyUserCnt = getint(row,0); 
	mysql_free_result(res);


	memset(szQuery, 0x00, sizeof(szQuery));
	
	sprintf(szQuery, "update  zangsi_sum.T_STAT_TOT  set sale_cnt = %d, buy_user_cnt = %d where stat_date = date_format(date_add('%s' "
	 , total_cnt, nBuyUserCnt ,  greg_date );
	strcat( szQuery ," ,INTERVAL -1 DAY ),'%Y%m%d')  ");

	if (mysql_query(con, szQuery))
	{
	  ZzLOG(ERROR, "daem5120_total_sale_cnt : mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
int daem5120_acct_period_loop()
{
	char szQuery[1000];		// query string
	char sztemp [100];      // query temp

	char szTempdate[9];

	memset(szTempdate,0x00, sizeof(szTempdate));
	strcpy(szTempdate, "20040428");
//	strcpy(szTempdate, "20071203");

	while(1)
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "SELECT date_format(date_add('%s', ", szTempdate );
		strcat( szQuery, " INTERVAL 1 DAY ), '%Y%m%d') ");	
		
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

		strcpy(greg_date ,   getstr(row, 0));
		
		mysql_free_result(res);

		memset(szTempdate,0x00, sizeof(szTempdate));
		strcpy(szTempdate, greg_date);
		

		printf("======> greg_date(%s)\n", greg_date);

		/* 결제주기 계산 */
		if( daem5120_acct_period() != 0 )
			return -1;	

		if(strcmp(greg_date,"20071210") >= 0)
		{
			break;
		}		
		
	}


	return 0;
}



//******************************************************************************
//* daem5120_acct_period()
//* 결제주기 계산
//******************************************************************************
int daem5120_acct_period()
{
	char szQuery[10000];		// query string
	char szQuerytmp[10000];	// query string
	int acct_cnt15 = 0; 
	int acct_cnt30 = 0; 
	int acct_cnt45 = 0; 
	int acct_cnt60 = 0; 
	int acct_cnt61 = 0; 

	int ret=0;

	//--------------------------------------------------------------------------
	// 계좌이체를 제외한 모든 결제 처리
	//--------------------------------------------------------------------------
	memset(szQuerytmp, 0x00, sizeof(szQuerytmp));
	strcpy(szQuerytmp, "date_format(now(),'%Y%m%d') ");

	memset(szQuery, 0x00, sizeof(szQuery));

	if( strcmp(greg_date , "20070215" ) < 0 )
	{
		sprintf(szQuery, "REPLACE zangsi_sum.T_ACCT_PERIOD (user_id , acct_cnt, acct_date , acct_date_last, reg_date , total_charge ) "
						 " SELECT a.user_id "
						 "      , ifnull(b.acct_cnt,0) + 1 "
						 "      , a.reg_date  "
						 "      , b.acct_date "
						 "      , %s  "
						 "      , ifnull(a.amount,0)  + ifnull(b.total_charge,0) "
						 "   FROM zangsi_bck.T_ACCT_SOFT a left outer join zangsi_sum.T_ACCT_PERIOD b "
						 "     ON a.user_id  = b.user_id  "
						 "  WHERE a.proc_yn  = 'Y' "
						 "    AND a.reg_date = date_format(date_add('%s', ",szQuerytmp, greg_date ) ;
		strcat( szQuery, " INTERVAL - 1 DAY ), '%Y%m%d') ");	

	}
	else
	{
		sprintf(szQuery, "REPLACE zangsi_sum.T_ACCT_PERIOD (user_id , acct_cnt, acct_date , acct_date_last, reg_date, total_charge ) "
						 " SELECT a.user_id "
						 "      , ifnull(b.acct_cnt,0) + 1 "
						 "      , a.reg_date  "
						 "      , b.acct_date "
						 "      , %s  "
						 "      , ifnull(a.amount,0)  + ifnull(b.total_charge,0) "
						 "   FROM zangsi.T_ACCT_SOFT a left outer join zangsi_sum.T_ACCT_PERIOD b "
						 "     ON a.user_id  = b.user_id  "
						 "  WHERE a.proc_yn  = 'Y' "
						 "    AND a.reg_date = date_format(date_add('%s', ",szQuerytmp, greg_date ) ;
		strcat( szQuery, " INTERVAL - 1 DAY ), '%Y%m%d') ");	
	}

	ZzLOG(ALWAY, "daem5120_acct_period : %s\n\n",szQuery);

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5120_acct_period : mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		tran_rollback(con);
		return -1;
	}

	//--------------------------------------------------------------------------
	// 계좌이체 처리
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "REPLACE zangsi_sum.T_ACCT_PERIOD (user_id , acct_cnt, acct_date , acct_date_last, reg_date, total_charge ) "
					 " SELECT a.user_id "
					 "      , ifnull(b.acct_cnt,0) + 1 "
					 "      , a.reg_date  "
					 "      , b.acct_date "
					 "      , %s  "
					 "      , ifnull(a.amount,0)  + ifnull(b.total_charge,0) "
					 "   FROM zangsi.T_ACCT_BANK a left outer join zangsi_sum.T_ACCT_PERIOD b "
					 "     ON a.user_id  = b.user_id  "
					 "  WHERE a.proc_yn  = 'Y' "
					 "    AND a.reg_date = date_format(date_add('%s', ",szQuerytmp, greg_date ) ;
	strcat( szQuery, " INTERVAL - 1 DAY ), '%Y%m%d') ");	

	ZzLOG(ALWAY, "daem5120_acct_period : %s\n\n",szQuery);

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5120_acct_period : mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		tran_rollback(con);
		return -1;
	}

	
	//--------------------------------------------------------------------------
	// 당일 결제자중 재결제 주기 계산
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	strcpy (szQuery, "SELECT sum(if (to_days(date_format(a.acct_date, '%Y%m%d')) - to_days(date_format(a.acct_date_last, '%Y%m%d')) < 16,1,0)) as acct_cnt15 "
	                 "     , sum(if (to_days(date_format(a.acct_date, '%Y%m%d')) - to_days(date_format(a.acct_date_last, '%Y%m%d')) > 15 and "
	                 "               to_days(date_format(a.acct_date, '%Y%m%d')) - to_days(date_format(a.acct_date_last, '%Y%m%d')) < 31,1,0)) as acct_cnt30 "
	                 "     , sum(if (to_days(date_format(a.acct_date, '%Y%m%d')) - to_days(date_format(a.acct_date_last, '%Y%m%d')) > 30 and "
	                 "               to_days(date_format(a.acct_date, '%Y%m%d')) - to_days(date_format(a.acct_date_last, '%Y%m%d')) < 46,1,0)) as acct_cnt45 "
	                 "     , sum(if (to_days(date_format(a.acct_date, '%Y%m%d')) - to_days(date_format(a.acct_date_last, '%Y%m%d')) > 45 and "
	                 "               to_days(date_format(a.acct_date, '%Y%m%d')) - to_days(date_format(a.acct_date_last, '%Y%m%d')) < 61,1,0)) as acct_cnt60 "
	                 "     , sum(if (to_days(date_format(a.acct_date, '%Y%m%d')) - to_days(date_format(a.acct_date_last, '%Y%m%d')) > 60,1,0)) as acct_cnt61 "
	                 "  FROM zangsi_sum.T_ACCT_PERIOD a "
	                 " WHERE a.acct_date  = date_format(date_add(");

	memset (szQuerytmp, 0x00, sizeof(szQuerytmp));
	sprintf(szQuerytmp, "'%s', ", greg_date);
	strcat (szQuery, szQuerytmp);	
	strcat (szQuery, " INTERVAL - 1 DAY ), '%Y%m%d') ");	
	strcat (szQuery, "   AND acct_cnt > 1");
	strcat (szQuery, " GROUP BY a.acct_date");

	ZzLOG(ALWAY, "daem5120_acct_period : %s\n\n",szQuery);


	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5120_acct_period: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		tran_rollback(con);
		return -1;		
	}

	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daem5120_acct_period: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		tran_rollback(con);
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "daem5120_acct_period: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
	}
	else
	{
		row = mysql_fetch_row(res);
		acct_cnt15 = getint(row,0); 
		acct_cnt30 = getint(row,1); 
		acct_cnt45 = getint(row,2); 
		acct_cnt60 = getint(row,3); 
		acct_cnt61 = getint(row,4); 
	}
	mysql_free_result(res);

	
	//--------------------------------------------------------------------------
	// 결제주기 UPDATE
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "UPDATE zangsi_sum.T_STAT_TOT "
	                 "   SET re_charge_term_15  = '%ld' "
	                 "     , re_charge_term_30  = '%ld' "
	                 "     , re_charge_term_45  = '%ld' "
	                 "     , re_charge_term_60  = '%ld' "
	                 "     , re_charge_term_end = '%ld' "
	                 " WHERE stat_date =  date_format(date_add('%s', "
	                 ,acct_cnt15
	                 ,acct_cnt30
	                 ,acct_cnt45
	                 ,acct_cnt60
	                 ,acct_cnt61
	                 ,greg_date);
	strcat( szQuery, "  INTERVAL - 1 DAY ), '%Y%m%d') " );
	
	ZzLOG(ALWAY, "daem5120_acct_period : %s\n\n",szQuery);

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5120_acct_period: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		tran_rollback(con);
		return -1;
	}


	return 0;	
}




//******************************************************************************
//* daem5120_total_uplaod_contents()
//* 컨텐츠 등록건수 집계
//******************************************************************************
int daem5120_total_uplaod_contents()
{
	char szQuery[10000];		// query string
	int cont_cnt = 0 ;
	
	int ret=0;
	
	//--------------------------------------------------------------------------
	// 
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	
	sprintf(szQuery, "  select count(*)  from zangsi.T_CONTENTS_INFO  where  "
					 " reg_date =  date_format(date_add('%s' " , greg_date ) ;
	strcat( szQuery , " ,INTERVAL -1 DAY ),'%Y%m%d') ");
					 
	
					 
	ZzLOG(ALWAY, "daem5120_total_uplaod_contents : %s\n\n",szQuery);
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5120_total_uplaod_contents: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}


	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daem5120_total_uplaod_contents: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "daem5120_total_uplaod_contents: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
	
	row = mysql_fetch_row(res);

	cont_cnt = getint(row,0); 

	
	
	mysql_free_result(res);

	sprintf(szQuery, " update zangsi_sum.T_STAT_TOT a set a.cont_reg_cnt = %ld  "
					 "	where a.stat_date = date_format(date_add('%s' " , cont_cnt ,  greg_date );
	strcat( szQuery ," ,INTERVAL -1 DAY ),'%Y%m%d')  ");


	ZzLOG(ALWAY, "daem5120_total_uplaod_contents : %s\n\n",szQuery);

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5120_total_uplaod_contents : mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}	
	
	
	return 0;		
		
}
//******************************************************************************
//* daem5120_total_out_amt()
//* 출금액집계
//******************************************************************************
int daem5120_total_out_amt()
{
	char szQuery[10000];		// query string
	int out_amt_sum = 0 ;
	
	int ret=0;
	
	//--------------------------------------------------------------------------
	// 
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	
	sprintf(szQuery, "  select sum(out_cmt)  from zangsi.T_OUT_AMOUNT where proc_stat in ('3', '5') "
					 " and proc_date =  date_format(date_add('%s' " , greg_date ) ;
	strcat( szQuery , " ,INTERVAL -1 DAY ),'%Y%m%d') ");
					 
	
					 
	ZzLOG(ALWAY, "daem5120_total_out_amt : %s\n\n",szQuery);
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5120_total_out_amt: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}


	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daem5120_total_out_amt: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "daem5120_total_out_amt: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
	
	row = mysql_fetch_row(res);

	out_amt_sum = getint(row,0); 

	
	
	mysql_free_result(res);

	sprintf(szQuery, " update zangsi_sum.T_STAT_TOT a set a.out_amt = %ld  "
					 "	where a.stat_date = date_format(date_add('%s' " , out_amt_sum ,  greg_date );
	strcat( szQuery ," ,INTERVAL -1 DAY ),'%Y%m%d')  ");


	ZzLOG(ALWAY, "daem5120_total_out_amt : %s\n\n",szQuery);

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5120_total_out_amt : mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}	
	
	
	return 0;		
	
}



//******************************************************************************
//* daem5120_total_in_user()
//* 전체 회원수(탈퇴제외)
//******************************************************************************
int daem5120_total_in_user()
{
	char szQuery[10000];		// query string
	int total_in_cnt = 0 ;
	
	int ret=0;
	
	//--------------------------------------------------------------------------
	// 
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	
	sprintf(szQuery, " select count(user_id) from zangsi.T_USER_INFO where "
					 " reg_date < '%s' and use_stat = '0' " , greg_date ) ;
	
					 
	ZzLOG(ALWAY, "daem5120_total_in_user : %s\n\n",szQuery);
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5120_total_in_user: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}


	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daem5120_total_in_user: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "daem5120_total_in_user: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
	
	row = mysql_fetch_row(res);

	total_in_cnt = getint(row,0); 

	
	
	mysql_free_result(res);

	sprintf(szQuery, " update zangsi_sum.T_STAT_TOT a set a.user_cnt = %ld  "
					 "	where a.stat_date = date_format(date_add('%s' " , total_in_cnt ,  greg_date );
	strcat( szQuery ," ,INTERVAL -1 DAY ),'%Y%m%d')  ");


	ZzLOG(ALWAY, "daem5120_total_in_user : %s\n\n",szQuery);

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5120_total_in_user : mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}	
	
	
	return 0;		
}
//******************************************************************************
//* daem5120_today_login_user()
//* 방문자수 집계처리
//******************************************************************************
int daem5120_today_login_user()
{
	MYSQL     *log_con = NULL; 

	char szQuery[10000];		// query string
	int login_cnt = 0 ;
	double today_login_avg = 0;
	
	int ret=0;
	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(log_con=db_connect_logdb("zangsi")))
	{
		ZzLOG(ERROR, "logDB에 접속하지 못 하였습니다...\n");
		db_disconnect(log_con);
	   	return(0); 
	}
	
	//--------------------------------------------------------------------------
	// 하루 방문자수 조회
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	
	sprintf(szQuery, " select count(distinct(user_id)), count(user_id) / count(distinct(user_id))  from zangsi.T_LOGIN_LOG where "
					 " login_date = date_format(date_add('%s' " , greg_date ) ;
	strcat( szQuery , " ,INTERVAL -1 DAY ),'%Y%m%d') ");
					 
	ZzLOG(ALWAY, "daem5120_today_login_user : %s\n\n",szQuery);
	
	if (mysql_query(log_con, szQuery))
	{
	    ZzLOG(ERROR, "daem5120_today_login_user: mysql_query error. [%d](%s)\n",mysql_errno(log_con), mysql_error(log_con));
		db_disconnect(log_con);
	   	return(0); 
	}


	if (!(res = mysql_store_result(log_con)))
	{
	    ZzLOG(ERROR, "daem5120_today_login_user: mysql_store_result error. [%d](%s)",mysql_errno(log_con), mysql_error(log_con));
		db_disconnect(log_con);
	   	return(0); 
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "daem5120_today_login_user: mysql_num_rows error. [%d](%s)",mysql_errno(log_con), mysql_error(log_con));
		mysql_free_result(res);
		db_disconnect(log_con);
	   	return(0); 
	}
	
	row = mysql_fetch_row(res);

	login_cnt = getint(row,0); 
	today_login_avg = getnum(row,1);

	mysql_free_result(res);

	int n15 = 0 ;
	int n16_30 = 0;
	int n31_45 = 0;
	int n46_60 = 0;
	int n61 = 0;

	//--------------------------------------------------------------------------
	// 재로그인 정보 조회
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select "
					 " re_login_term_15, re_login_term_30, re_login_term_45, re_login_term_60, re_login_term_end "
					 " from zangsi.T_RE_LOGIN_STAT where stat_date = '%s' "
					 ,gproc_date ) ;
					 
	ZzLOG(ALWAY, "daem5120_today_login_user : %s\n\n",szQuery);
	
	if (mysql_query(log_con, szQuery))
	{
	    ZzLOG(ERROR, "daem5120_today_login_user: mysql_query error. [%d](%s)\n",mysql_errno(log_con), mysql_error(log_con));
		db_disconnect(log_con);
	   	return(0); 
	}


	if (!(res = mysql_store_result(log_con)))
	{
	    ZzLOG(ERROR, "daem5120_today_login_user: mysql_store_result error. [%d](%s)",mysql_errno(log_con), mysql_error(log_con));
		db_disconnect(log_con);
	   	return(0); 
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "daem5120_today_login_user: mysql_num_rows error. [%d](%s)",mysql_errno(log_con), mysql_error(log_con));
	}
	else
	{	
	
		row = mysql_fetch_row(res);
	
	    n15 = (int)getint(row,0);
	    n16_30 = (int)getint(row,1);
	    n31_45 = (int)getint(row,2);
	    n46_60 = (int)getint(row,3);
	    n61 = (int)getint(row,4);
		 

	}
	mysql_free_result(res);

	db_disconnect(log_con);

	sprintf(szQuery, " update zangsi_sum.T_STAT_TOT a " 
					 " set "
					 " a.today_login_cnt = %ld, "
					 " a.today_login_avg = %.2f, "
					 " a.re_login_term_15 = %d, "
					 " a.re_login_term_30 = %d, "
					 " a.re_login_term_45 = %d, "
					 " a.re_login_term_60 = %d, "
					 " a.re_login_term_end = %d "
					 "	where a.stat_date = date_format(date_add('%s' " , login_cnt, today_login_avg, n15, n16_30, n31_45, n46_60, n61, greg_date );
	strcat( szQuery ," ,INTERVAL -1 DAY ),'%Y%m%d')  ");


	ZzLOG(ALWAY, "daem5120_today_login_user : %s\n\n",szQuery);

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5120_today_login_user : mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	   	return(0); 
	}	
	
	
	return 0;	
	
}
//******************************************************************************
//* daem5120_relogin_user()
//* 재방문자수 집계 처리 
//* daem5120_total_in_user() 프로세스 이후 가동
//******************************************************************************
int daem5120_relogin_user()
{
	char szQuery[10000];		// query string
	int relogin_cnt = 0 ;
	int user_cnt = 0 ;
	
	int ret=0;
	
	//--------------------------------------------------------------------------
	// 15일 이내 재접속자
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	
	sprintf(szQuery, " select count(*) as relogin_cnt from zangsi.T_USER_STAT a , zangsi.T_USER_INFO b where  "
					 " to_days(date_format(a.crt_conn_date, '%Y%%m%%d'))  >=  to_days(date_format(date_add('%s' ,INTERVAL -16 DAY ),'%Y%%m%%d')) " 
					 " and to_days(date_format(a.crt_conn_date, '%Y%%m%%d'))  <=  to_days(date_format(date_add('%s' ,INTERVAL -1 DAY ),'%Y%%m%%d')) " 
					 " and to_days(date_format(a.crt_conn_date, '%Y%%m%%d')) - to_days(date_format(a.lst_conn_date, '%Y%%m%%d'))  < 16 "
					 " and a.user_id = b.user_id and b.use_stat ='0' " 
					 , greg_date , greg_date ) ;
					 
	strcat( szQuery , " ");
					 
	ZzLOG(ALWAY, "daem5120_relogin_user : %s\n\n",szQuery);
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5120_relogin_user: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}


	if (!(res = mysql_store_result(con)))
	{
	  ZzLOG(ERROR, "daem5120_relogin_user: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	  ZzLOG(ERROR, "daem5120_relogin_user: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return 0;
	}
	
	row = mysql_fetch_row(res);

	relogin_cnt = getint(row,0); 

	mysql_free_result(res);


	memset (szQuery, 0x00, sizeof(szQuery));
	
	sprintf(szQuery, " select user_cnt  from zangsi_sum.T_STAT_TOT where  "
					 " stat_date = date_format(date_add('%s' " ,  greg_date );
	strcat( szQuery ," ,INTERVAL -1 DAY ),'%Y%m%d')  ");
					 
	ZzLOG(ALWAY, "daem5120_relogin_user : %s\n\n",szQuery);
	
	if (mysql_query(con, szQuery))
	{
	  ZzLOG(ERROR, "daem5120_relogin_user: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}


	if (!(res = mysql_store_result(con)))
	{
	  ZzLOG(ERROR, "daem5120_relogin_user: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
		ZzLOG(ERROR, "daem5120_relogin_user: T_STAT_TOT 데이터 없음.\n");
		mysql_free_result(res);
		return -1;
	}
	
	row = mysql_fetch_row(res);

	user_cnt = getint(row,0); 

	mysql_free_result(res);


	double percent = ((double)relogin_cnt / (double)user_cnt)*(double)100;

	sprintf(szQuery, " update zangsi_sum.T_STAT_TOT a set a.re_login_per = %3.2f, relogin_cnt = %d  "
					 "	where a.stat_date = date_format(date_add('%s' " , percent, relogin_cnt ,  greg_date );
	strcat( szQuery ," ,INTERVAL -1 DAY ),'%Y%m%d')  ");


	ZzLOG(ALWAY, "daem5120_relogin_user : %s\n\n",szQuery);

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5120_relogin_user : mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}	
	
	
	return 0;	
	
}

//******************************************************************************
//* daem5120_stat_user()
//* 회원현황(가입, 탈퇴)
//******************************************************************************
int daem5120_stat_user()
{
	char szQuery[10000];		// query string
	int user_in = 0 , user_out = 0;
	
	int ret=0;
	
	//--------------------------------------------------------------------------
	// 
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	
	sprintf(szQuery, " select sum(if( a.use_stat = '0' , 1, 0)) as user_in , sum(if( a.use_stat != '0' , 1, 0)) as user_out "
					 "	from  zangsi.T_USER_INFO a "
					 "	where a.reg_date = date_format(date_add('%s' " , greg_date );
	strcat( szQuery ," ,INTERVAL -1 DAY ),'%Y%m%d') group by a.reg_date  ");


	ZzLOG(ALWAY, "daem5120_stat_user : %s\n\n",szQuery);
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5120_stat_user: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}


	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daem5120_stat_user: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "daem5120_stat_user: 가입 / 탈퇴자 없음.\n");
		mysql_free_result(res);
		return 0;
	}
	
	row = mysql_fetch_row(res);

	user_in = getint(row,0); 
	user_out = getint(row,1); 
	
	
	mysql_free_result(res);
	
	
	
	sprintf(szQuery, " update zangsi_sum.T_STAT_TOT a set a.member_reg = %ld , a.member_break = %ld  "
					 "	where a.stat_date = date_format(date_add('%s' " , user_in , user_out , greg_date );
	strcat( szQuery ," ,INTERVAL -1 DAY ),'%Y%m%d')  ");


	ZzLOG(ALWAY, "daem5120_stat_user : %s\n\n",szQuery);
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5120_stat_user: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}	
	
	
	return 0;
}

//******************************************************************************
//* daem5120_conn_user()
//* 서버접속자수 0시기준 (웹, 다운, 업)
//******************************************************************************
int daem5120_conn_user()
{
	char szQuery[10000];		// query string
	int web_cnt;
	int up_cnt;
	int dn_cnt;
	int ret=0;
	
	//--------------------------------------------------------------------------
	// 
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	
	sprintf(szQuery, " update zangsi_sum.T_STAT_TOT a , zangsi_bck.T_SUM_CONCNT b "
					 " set a.web_conn = b.web_cnt , a.server_up_conn = b.up_cnt , a.server_dn_conn = b.dn_cnt "
	                 "  WHERE  b.reg_date = '%s' AND b.reg_hour = '00' "
	                 , greg_date );
	strcat(szQuery  , " and a.stat_date = date_format(date_add(b.reg_date,INTERVAL -1 DAY ),'%Y%m%d') ");
/*	                 
	sprintf(szQuery, " SELECT web_cnt, up_cnt, dn_cnt "
	                 "   FROM zangsi_sum.T_SUM_CONCNT "
	                 "  WHERE reg_date = '%s' "
	                 "    AND reg_hour = '00' "
	                 , greg_date );
*/
	
	ZzLOG(ALWAY, "daem5120_conn_user : %s\n\n",szQuery);
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5120_conn_user: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	

/*	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daem5120_conn_user: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "daem5120_conn_user: mysql_num_rows error...\n");
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
*/
	return 0;
}


//******************************************************************************
//* daem5120_insert_login()
//* 일일 사용자별 거래집계처리
//******************************************************************************
int daem5120_insert_data()
{
	char szQuery[10000];		// query string
	int ret=0;
	
	//--------------------------------------------------------------------------
	// 
	//--------------------------------------------------------------------------
/*	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "update zangsi_sum.T_STAT_SUM   "
	                 "     ( server_up_conn       , server_dn_conn ) 
	                 " SELECT '000000'      , user_id       "
	                 "  FROM zangsi_sum.T_SUM_CONCNT   "
	                 " WHERE reg_date = '%s'  "
	                 ,greg_date );
                 
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5120_insert_current: INSERT T_ANALY_DEAL_YM error...\n");
		ZzLOG(ERROR, "daem5120_insert_current: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
    }
*/	
	return 0;
}
//******************************************************************************
//* daem5120_get_login_info()
//* 재로그인 정보 수집
//******************************************************************************
int daem5120_get_login_info()
{
	MYSQL     *log_con = NULL; 

	char szQuery[10000];		// query string
	int n15 = 0 ;
	int n16_30 = 0;
	int n31_45 = 0;
	int n46_60 = 0;
	int n61 = 0;
	
	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(log_con=db_connect_logdb("zangsi")))
	{
		ZzLOG(ERROR, "logDB에 접속하지 못 하였습니다...\n");
		db_disconnect(log_con);
	   	return(0); 
	}

	
	//--------------------------------------------------------------------------
	// 
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select user_id from zangsi.T_LOGIN_LOG where login_date = '%s' " 
					 " group by user_id "
					 , gproc_date);
					 
	ZzLOG(ALWAY, "daem5120_get_login_info : %s\n\n",szQuery);
	
	if (mysql_query(log_con, szQuery))
	{
	    ZzLOG(ERROR, "daem5120_get_login_info: mysql_query error. [%d](%s)\n",mysql_errno(log_con), mysql_error(log_con));
		db_disconnect(log_con);
	   	return(0); 
	}
	if (!(res = mysql_store_result(log_con)))
	{
	    ZzLOG(ERROR, "daem5120_get_login_info: mysql_store_result error. [%d](%s)",mysql_errno(log_con), mysql_error(log_con));
		db_disconnect(log_con);
	   	return(0); 
	}
	int nRowCnt = 0;
	nRowCnt = mysql_num_rows(res);
 	if (nRowCnt == 0)
 	{
	    ZzLOG(ALWAY, "daem5120_get_login_info: %s에 대한 로그인 정보 없음.\n", gproc_date);
		mysql_free_result(res);
		db_disconnect(log_con);
	   	return(0); 
	}
		
	MYSQL_RES *log_res; 
	MYSQL_ROW  log_row; 
	
	char szUserId[16+1];

	int nCnt = 0;
	
	ZzLOG(ALWAY, "daem5120_get_login_info: 처리할 회원수 %d명\n", nRowCnt);
	
	while(row = mysql_fetch_row(res))
	{
		memset(szUserId, 0x00, sizeof(szUserId));			
		sprintf(szUserId, "%s", getstr(row,0));
		
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " select "
						 " to_days(date_format('%s', '%%Y%%m%%d')) - to_days(date_format(max(login_date) , '%%Y%%m%%d')) "
						 " from zangsi.T_LOGIN_LOG where user_id = '%s' and login_date < '%s' " 
						, gproc_date, szUserId, gproc_date);
				
		if (mysql_query(log_con, szQuery))
		{
		    ZzLOG(ERROR, "daem5120_get_login_info: mysql_query error. [%d](%s)\n",mysql_errno(log_con), mysql_error(log_con));
			db_disconnect(log_con);
		   	return(0); 
		}
		if (!(log_res = mysql_store_result(log_con)))
		{
		    ZzLOG(ERROR, "daem5120_get_login_info: mysql_store_result error. [%d](%s)",mysql_errno(log_con), mysql_error(log_con));
			db_disconnect(log_con);
		   	return(0); 
		}
	 	if (mysql_num_rows(log_res)==0)
	 	{
	 		ZzLOG(ERROR, "daem5120_get_login_info: 로그인 내역 없음.[%s]\n", szQuery);
			mysql_free_result(log_res);
			continue;
		}
		
		log_row = mysql_fetch_row(log_res);
		int nLoginDays = 0;
		nLoginDays = (int)getint(log_row,0);
		mysql_free_result(log_res);
		
		if(nLoginDays <= 15)
			n15++;
		else if(nLoginDays >= 16 && nLoginDays <= 30)
			n16_30++;
		else if(nLoginDays >= 31 && nLoginDays <= 45)
			n31_45++;
		else if(nLoginDays >= 46 && nLoginDays <= 60)
			n46_60++;
		else
			n61++;
			
		nCnt++;
	}
	mysql_free_result(res);
	
	ZzLOG(ALWAY, "daem5120_get_login_info: 처리한 회원수 %d명\n", nCnt);
	

	db_disconnect(log_con);
	

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(con=db_connect_nodb("")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1); 
	}


	sprintf(szQuery, " update zangsi_sum.T_STAT_TOT "
					 " set "
					 " re_login_term_15 = %d, "
					 " re_login_term_30 = %d, "
					 " re_login_term_45 = %d, "
					 " re_login_term_60 = %d, "
					 " re_login_term_end = %d "
					 " where stat_date = '%s' " 
					 , n15, n16_30, n31_45, n46_60, n61
					 ,  gproc_date );
	


	ZzLOG(ALWAY, "daem5120_get_login_info : %s\n\n",szQuery);

	if (mysql_query(con, szQuery))
	{
		
	    ZzLOG(ERROR, "daem5120_get_login_info : mysql_query error. [%d](%s)\n",mysql_errno(con), mysql_error(con));
	   	return(0); 
	}	
	
	return 0;	
	
}


/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5120_get_sysdate()
{
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

	
	if (strcmp(gsys_date, "00000000")==0)
		strcpy(greg_date ,   getstr(row, 0));
	else
		strcpy(greg_date ,  gsys_date);
		
	
	
	
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
int daem5120_init_process(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
    
    #ifdef __DEBUG
    ZzInitGlobalVariable2("Dd_", "/logs/daemon"); 
    #else
    ZzInitGlobalVariable2("d_", "/logs/daemon"); 
    #endif
    
    ZzLOG(ALWAY, "[daem5120]*****************프로그램 시작*****************\n");  
    

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
	ret=daem5120_get_sysdate();
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
int daem5120_term_process()
{
    // DB close
	db_disconnect(con);
    ZzLOG(ALWAY, "[daem5120]*****************프로그램 종료*****************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5120_signal(int nSignal)
{
    daem5120_term_process();
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
	signal(SIGTERM, daem5120_signal);
	signal(SIGINT,  daem5120_signal);
	signal(SIGQUIT, daem5120_signal);
	signal(SIGKILL, daem5120_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daem5120_init_process(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		
		rc = daem5120_main_process();
//		daem5120_acct_period_loop();	
		/* 프로그램 종료루틴 */                    
		daem5120_term_process();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
