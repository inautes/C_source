/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daemadminout1.cc
 *         기능 : 분류별 무료다운로드 집계
 *         설명 : 1일 1회 작업한다.
 *                SYSTEM  (00000000) => sysdate - 1일 처리하며,
 *                직접입력(yyyymmdd) => yyyymmdd를  처리한다.
 *       작성자 : HCS
 *       작성일 : 2010/04/02
 *     수정이력 : 
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

int daemadminout_init_process(int argc, char **argv);
int daemadminout_main_process();
int daemadminout_term_process();
int daemadminout_delete_current();
int daemadminout_insert_data();
int daemadminout_insert_acct_data();
int daemadminout_down_acct_write_table(char* pUserId, char* pAcctStat);
int daemadminout_login_acct_write_table(char* pUserId, char* pAcctStat);
int daemadminout_insert_login_data();
int daemadminout_get_sysdate();
void daemadminout_signal(int nSignal);

MYSQL     *con;
MYSQL     *log_con;

char   gsys_date  [  8+1];	//처리일자(sysdate)
char   gproc_date [  8+1];	//처리일자(sysdate-1)
char   greg_date  [  8+1];	//등록일
char   greg_time  [  6+1];	//등록시간
//******************************************************************************
//* daemadminout main
//******************************************************************************
int daemadminout_main_process()
{
	//--------------------------------------------------------------------------
	// 트렌젝션시작
	//--------------------------------------------------------------------------

	// 처리일자의 자료삭제
	if (daemadminout_delete_current() != 0)
		return -1;

	// 로그인 정보 집계처리
	if (daemadminout_insert_login_data() != 0)
		return -1;

	// 일간 집계처리
	if (daemadminout_insert_data() != 0)
		return -1;

	// 월간, 연간 누적 집계처리
	if (daemadminout_insert_acct_data() != 0)
		return -1;


	return (0);

}


//******************************************************************************
//* daemadminout_delete_current()
//* 처리일자의 자료를 삭제처리 한다.
//******************************************************************************
int daemadminout_delete_current()
{
	char szQuery[1000];		// query string

	//--------------------------------------------------------------------------
	// 처리일자 해당되는 자료삭제(재작업을 위해)
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi_bck.T_ADMIN_INOUT_HIST"
	                 " WHERE reg_date = '%s' and inout_code = '11' and inout_code_sub = '01'"
	                 ,gproc_date
	                 );
	#ifdef __DEBUG
	printf("%s\n\n", szQuery);	
	#endif
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daemadminout_delete_current: DELETE T_ADMIN_INOUT_HIST error...\n");
		ZzLOG(ERROR, "daemadminout_delete_current: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
	    return -1;
    }


	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi_bck.T_ADMIN_INOUT_SUM_DD"
	                 " WHERE sum_date    = '%s' "
	                 ,gproc_date
	                 );

	#ifdef __DEBUG
	printf("%s\n\n", szQuery);	
	#endif
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daemadminout_delete_current: DELETE T_ADMIN_INOUT_HIST error...\n");
		ZzLOG(ERROR, "daemadminout_delete_current: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
	    return -1;
    }

    return 0;
}
int daemadminout_insert_login_data()
{
    ZzLOG(ALWAY, "[daemadminout]*******************************************\n");  
    ZzLOG(ALWAY, "[daemadminout]처리일자 = %s,   등록일자 = %s\n", gproc_date, greg_date);  
    ZzLOG(ALWAY, "[daemadminout]*******************************************\n");  

    ZzLOG(ALWAY, "daemadminout_insert_login_data 처리 시작\n");  

	MYSQL_RES *res;
	MYSQL_ROW  row;

	MYSQL_RES *log_res;
	MYSQL_ROW  log_row;

	char szQuery[10000];		// query string
	int ret=0;
	int nRowcnt = 0;
	
	//--------------------------------------------------------------------------
	// 해당일자의 자료를 생성한다.
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	strcpy(szQuery, " select a.user_id, a.auth_code from zangsi.T_USER_ADMIN a, zangsi.T_USER_INFO b  "
					" where a.user_id = b.user_id and b.auth_code = 'SA' ");

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daemadminout_insert_login_data: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daemadminout_insert_login_data: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ALWAY, "daemadminout_insert_login_data: T_USER_ADMIN 결과 없음. 처리일자=[%s]\n", gproc_date);
		mysql_free_result(res);
		return 0;
	}
	
	while(row = mysql_fetch_row(res))
	{
		char szUserId[16+1];
		memset(szUserId, 0x00, sizeof(szUserId));
		strcpy(szUserId, getstr(row,0));
		
		
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " select login_ip, login_time "
						 " from zangsi.T_LOGIN_LOG where login_date = '%s' and user_id = '%s' and exit_url is null "
						 , gproc_date, szUserId);
		if (mysql_query(log_con, szQuery))
		{
		    ZzLOG(ERROR, "daemadminout_insert_login_data: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(log_con), mysql_error(log_con), szQuery);
			return -1;
		}
		if (!(log_res = mysql_store_result(log_con)))
		{
		    ZzLOG(ERROR, "daemadminout_insert_login_data: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(log_con), mysql_error(log_con), szQuery);
			return -1;
		}
	 	if (mysql_num_rows(log_res)==0)
	 	{
		    ZzLOG(ALWAY, "daemadminout_insert_login_data: T_LOGIN_LOG 결과 없음. user_id=[%s]\n", szUserId);
			mysql_free_result(log_res);
			continue;
		}
		
		while(log_row = mysql_fetch_row(log_res))
		{
			char szLoginIp[15+1];
			memset(szLoginIp, 0x00, sizeof(szLoginIp));
			strcpy(szLoginIp, getstr(log_row,0));
	
			char szLoginTime[6+1];
			memset(szLoginTime, 0x00, sizeof(szLoginTime));
			strcpy(szLoginTime, getstr(log_row,1));
	
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi_bck.T_ADMIN_INOUT_HIST "
							 " (user_id, inout_code, inout_code_sub, login_ip, reg_date, reg_time) "
							 " values "
							 " ('%s', '11', '01', '%s', '%s', '%s') "
							 , szUserId, szLoginIp, gproc_date, szLoginTime);
			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daemadminout_insert_login_data: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
				mysql_free_result(log_res);
				return -1;
			}
		}
		mysql_free_result(log_res);
		nRowcnt++;
	}	
	mysql_free_result(res);
	
	ZzLOG(ALWAY, "daemadminout_insert_login_data: %d 건 처리 완료.\n\n", nRowcnt);

	return 0;
}


//******************************************************************************
//* daemadminout_insert_data()
//* 일일 사용자별 거래집계처리
//******************************************************************************
int daemadminout_insert_data()
{
    ZzLOG(ALWAY, "daemadminout_insert_data 처리 시작\n");  

	MYSQL_RES *res;
	MYSQL_ROW  row;

	MYSQL_RES *log_res;
	MYSQL_ROW  log_row;

	char szQuery[10000];		// query string
	int ret=0;
	
	//--------------------------------------------------------------------------
	// 해당일자의 자료를 생성한다.
	//--------------------------------------------------------------------------
	
	//일자별 데이터 생성
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select user_id, inout_code, inout_code_sub, sum(inout_amt), count(user_id) "
					 " from zangsi_bck.T_ADMIN_INOUT_HIST where reg_date = '%s' and inout_code != '11' "
					 " group by user_id, inout_code, inout_code_sub "
					 , gproc_date);
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daemadminout_insert_data: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daemadminout_insert_data: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ALWAY, "daemadminout_insert_data: T_ADMIN_INOUT_HIST 결과 없음. 처리일자=[%s]\n", gproc_date);
		mysql_free_result(res);
	}
	else
	{		
		while(row = mysql_fetch_row(res))
		{
			char szUserId[16+1];
			memset(szUserId, 0x00, sizeof(szUserId));
			strcpy(szUserId, getstr(row,0));
	
			char szInoutCd[2+1];
			memset(szInoutCd, 0x00, sizeof(szInoutCd));
			strcpy(szInoutCd, getstr(row,1));
	
			char szInoutCdSub[2+1];
			memset(szInoutCdSub, 0x00, sizeof(szInoutCdSub));
			strcpy(szInoutCdSub, getstr(row,2));
			
			int nInoutAmt = 0;
			nInoutAmt = (int)getint(row,3);
			
			int nInoutCnt = 0;
			nInoutCnt = (int)getint(row,4);
			
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi_bck.T_ADMIN_INOUT_SUM_DD "
							 " (sum_date, user_id, inout_code, inout_code_sub, inout_amt, inout_cnt, reg_date, reg_time) "
							 " values "
							 " ('%s', '%s', '%s', '%s', %d, %d, '%s', '%s') "
							 , gproc_date, szUserId, szInoutCd, szInoutCdSub, nInoutAmt, nInoutCnt, greg_date, greg_time);
			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daemadminout_insert_data: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
				mysql_free_result(res);
				return -1;
			}
		}
		mysql_free_result(res);
	}
	
	//다운로드 내역 조회
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select user_id, count(seq_no), count(distinct(down_ip)) from zangsi.T_ADMIN_DOWN_HIST "
					 " where reg_date = '%s' "
					 " group by user_id "
					 , gproc_date);

	if (mysql_query(log_con, szQuery))
	{
	    ZzLOG(ERROR, "daemadminout_insert_data: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(log_con), mysql_error(log_con), szQuery);
		return -1;
	}
	if (!(log_res = mysql_store_result(log_con)))
	{
	    ZzLOG(ERROR, "daemadminout_insert_data: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(log_con), mysql_error(log_con), szQuery);
		return -1;
	}
 	if (mysql_num_rows(log_res)==0)
 	{
	    ZzLOG(ALWAY, "daemadminout_insert_data: T_ADMIN_DOWN_HIST 결과 없음.\n");
	}
	else
	{		
		while(log_row = mysql_fetch_row(log_res))
		{
			char szUserId[16+1];
			memset(szUserId, 0x00, sizeof(szUserId));
			strcpy(szUserId, getstr(log_row,0));
			
			int nDownCnt = 0;
			nDownCnt = (int)getint(log_row,1);
			
			int nDownIpCnt = 0;
			nDownIpCnt = (int)getint(log_row,2);
		
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi_bck.T_ADMIN_INOUT_SUM_DD "
							 " (sum_date, user_id, inout_code, inout_code_sub, inout_cnt, reg_date, reg_time) "
							 " values "
							 " ('%s', '%s', '10', '01', %d, '%s', '%s') "
							 , gproc_date, szUserId, nDownCnt, greg_date, greg_time);
		
			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daemadminout_insert_data: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
				mysql_free_result(log_res);
				return -1;
			}

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi_bck.T_ADMIN_INOUT_SUM_DD "
							 " (sum_date, user_id, inout_code, inout_code_sub, inout_cnt, reg_date, reg_time) "
							 " values "
							 " ('%s', '%s', '10', '02', %d, '%s', '%s') "
							 , gproc_date, szUserId, nDownIpCnt, greg_date, greg_time);
		
			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daemadminout_insert_data: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
				mysql_free_result(log_res);
				return -1;
			}
		}
	}
	mysql_free_result(log_res);

	//login_ip_cnt 업데이트

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select user_id from zangsi_bck.T_ADMIN_INOUT_HIST  "
					 " where reg_date = '%s' and inout_code = '11' and inout_code_sub = '01' "
					 " group by user_id"
					 , gproc_date);

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daemadminout_insert_data: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daemadminout_insert_data: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ALWAY, "daemadminout_insert_data: T_ADMIN_INOUT_HIST 로그인 결과 없음. 처리일자=[%s]\n", gproc_date);
	}
	else
	{		
		while(row = mysql_fetch_row(res))
		{
			MYSQL_RES *res2;
			MYSQL_ROW  row2;
			
			char szUserId[16+1];
			memset(szUserId, 0x00, sizeof(szUserId));
			strcpy(szUserId, getstr(row,0));
			
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " select count(login_ip), count(distinct(login_ip)) " 
							 " from zangsi_bck.T_ADMIN_INOUT_HIST "
							 " where reg_date = '%s' and user_id = '%s' and inout_code = '11' and inout_code_sub = '01' "
							 , gproc_date, szUserId);
			
			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daemadminout_insert_data: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
				mysql_free_result(res);
				return -1;
			}
			if (!(res2 = mysql_store_result(con)))
			{
			    ZzLOG(ERROR, "daemadminout_insert_data: mysql_store_result error...\n");
				ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
				mysql_free_result(res);
				return -1;
			}

			int nLoginCnt = 0;
			int nLoginIpCnt = 0;

		 	if(mysql_num_rows(res2) == 0)
		 	{
			    ZzLOG(ALWAY, "daemadminout_insert_data: 로그인 아이피 조회 결과 없음.\n");
				ZzLOG(ALWAY, "(%s)\n", szQuery);
			}
			else
			{
			 	row2 = mysql_fetch_row(res2);
				nLoginCnt = (int)getint(row2,0);
				nLoginIpCnt = (int)getint(row2,1);
			}	
					 	
			mysql_free_result(res2);

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi_bck.T_ADMIN_INOUT_SUM_DD "
							 " (sum_date, user_id, inout_code, inout_code_sub, inout_cnt, reg_date, reg_time) "
							 " values "
							 " ('%s', '%s', '11', '01', %d, '%s', '%s') "
							 , gproc_date, szUserId, nLoginCnt, greg_date, greg_time);
			
			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daemadminout_insert_data: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
				mysql_free_result(res);
				return -1;
			}
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi_bck.T_ADMIN_INOUT_SUM_DD "
							 " (sum_date, user_id, inout_code, inout_code_sub, inout_cnt, reg_date, reg_time) "
							 " values "
							 " ('%s', '%s', '11', '02', %d, '%s', '%s') "
							 , gproc_date, szUserId, nLoginIpCnt, greg_date, greg_time);
			
			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daemadminout_insert_data: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
				mysql_free_result(res);
				return -1;
			}
		}	
	}
	mysql_free_result(res);

	ZzLOG(ALWAY, "daemadminout_insert_data: 처리 완료.\n");

	return 0;
}
//******************************************************************************
//* daemadminout_insert_data()
//* 월간, 연간 사용자별 거래집계처리
//******************************************************************************
int daemadminout_insert_acct_data()
{
    ZzLOG(ALWAY, "daemadminout_insert_acct_data 처리 시작\n");  

	MYSQL_RES *res;
	MYSQL_ROW  row;

	MYSQL_RES *log_res;
	MYSQL_ROW  log_row;

	char szQuery[10000];		// query string
	int ret=0;
	
	//--------------------------------------------------------------------------
	// 해당일자의 자료를 생성한다.
	//--------------------------------------------------------------------------
	
	//월간 데이터 다운로드 데이터 생성
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select user_id "
					 " from zangsi_bck.T_ADMIN_INOUT_SUM_DD where sum_date = '%s' "
					 " group by user_id "
					 , gproc_date);
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daemadminout_insert_acct_data: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daemadminout_insert_acct_data: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ALWAY, "daemadminout_insert_acct_data: T_ADMIN_INOUT_SUM_DD 결과 없음. 처리일자=[%s]\n", gproc_date);
		mysql_free_result(res);
		return 0;
	}
	
	while(row = mysql_fetch_row(res))
	{
		MYSQL_RES *res2;
		MYSQL_ROW  row2;

		char szUserId[16+1];
		memset(szUserId, 0x00, sizeof(szUserId));
		strcpy(szUserId, getstr(row,0));
		
		if(daemadminout_down_acct_write_table(szUserId, "M") < 0)
		{	
			mysql_free_result(res);
			return -1;
		}
		if(daemadminout_down_acct_write_table(szUserId, "Y") < 0)
		{	
			mysql_free_result(res);
			return -1;
		}

		if(daemadminout_login_acct_write_table(szUserId, "M") < 0)
		{	
			mysql_free_result(res);
			return -1;
		}
		if(daemadminout_login_acct_write_table(szUserId, "Y") < 0)
		{	
			mysql_free_result(res);
			return -1;
		}
	}
	mysql_free_result(res);
	
	//다운로드 내역 조회


	ZzLOG(ALWAY, "daemadminout_insert_acct_data: 처리 완료.\n");

	return 0;
}

int daemadminout_down_acct_write_table(char* pUserId, char* pAcctStat)
{
	MYSQL_RES *res;
	MYSQL_ROW  row;

	char szQuery[10000];		// query string
	
	int nSubCnt = 0;
	if(strcmp(pAcctStat, "M") == 0)
		nSubCnt = 6;
	else if(strcmp(pAcctStat, "Y") == 0)
		nSubCnt = 4;
			
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select count(distinct(down_ip)) "
					 " from zangsi.T_ADMIN_DOWN_HIST "
					 " where user_id = '%s' and substring(reg_date, 1, %d) = substring('%s', 1, %d) and reg_date <= '%s' "
					 , pUserId, nSubCnt, gproc_date, nSubCnt, gproc_date);
	
	#ifdef __DEBUG
	printf("%s\n\n", szQuery);	
	#endif
	if (mysql_query(log_con, szQuery))
	{
	    ZzLOG(ERROR, "daemadminout_down_acct_write_table: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(log_con), mysql_error(log_con), szQuery);
		mysql_free_result(res);
		return -1;
	}
	if (!(res = mysql_store_result(log_con)))
	{
	    ZzLOG(ERROR, "daemadminout_down_acct_write_table: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(log_con), mysql_error(log_con), szQuery);
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ALWAY, "daemadminout_down_acct_write_table: 결과 없음. 처리일자=[%s]\n", gproc_date);
	    ZzLOG(ALWAY, "daemadminout_down_acct_write_table: [%s]\n", szQuery);
		mysql_free_result(res);
		return 0;
	}
	
	row = mysql_fetch_row(res);
	int nCnt = 0;
	nCnt = (int)getint(row,0);
	mysql_free_result(res);
	
	if(nCnt > 0)
	{	
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi_bck.T_ADMIN_INOUT_SUM_DD "
						 " (sum_date, acct_stat, user_id, inout_code, inout_code_sub, inout_cnt, reg_date, reg_time) "
						 " values "
						 " ('%s', '%s', '%s', '10', '02', %d, '%s', '%s') "
						 , gproc_date, pAcctStat, pUserId, nCnt, greg_date, greg_time);
		
		#ifdef __DEBUG
		printf("%s\n\n", szQuery);	
		#endif
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daemadminout_down_acct_write_table: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
			mysql_free_result(res);
			return -1;
		}
	}
	return 0;
}

int daemadminout_login_acct_write_table(char* pUserId, char* pAcctStat)
{
	MYSQL_RES *res;
	MYSQL_ROW  row;

	char szQuery[10000];		// query string
	
	int nSubCnt = 0;
	if(strcmp(pAcctStat, "M") == 0)
		nSubCnt = 6;
	else if(strcmp(pAcctStat, "Y") == 0)
		nSubCnt = 4;
			
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select count(distinct(login_ip)) "
					 " from zangsi_bck.T_ADMIN_INOUT_HIST "
					 " where user_id = '%s' and substring(reg_date, 1, %d) = substring('%s', 1, %d) and reg_date <= '%s' "
					 " and inout_code = '11' and inout_code_sub = '01' "
					 , pUserId, nSubCnt, gproc_date, nSubCnt, gproc_date);
	
	#ifdef __DEBUG
	printf("%s\n\n", szQuery);	
	#endif
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daemadminout_login_acct_write_table: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		mysql_free_result(res);
		return -1;
	}
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daemadminout_login_acct_write_table: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ALWAY, "daemadminout_login_acct_write_table: 결과 없음. 처리일자=[%s]\n", gproc_date);
	    ZzLOG(ALWAY, "daemadminout_login_acct_write_table: [%s]\n", szQuery);
		mysql_free_result(res);
		return 0;
	}
	
	row = mysql_fetch_row(res);
	int nCnt = 0;
	nCnt = (int)getint(row,0);
	mysql_free_result(res);
	
	if(nCnt > 0)
	{	
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi_bck.T_ADMIN_INOUT_SUM_DD "
						 " (sum_date, acct_stat, user_id, inout_code, inout_code_sub, inout_cnt, reg_date, reg_time) "
						 " values "
						 " ('%s', '%s', '%s', '11', '02', %d, '%s', '%s') "
						 , gproc_date, pAcctStat, pUserId, nCnt, greg_date, greg_time);
		
		#ifdef __DEBUG
		printf("%s\n\n", szQuery);	
		#endif
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daemadminout_login_acct_write_table: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
			mysql_free_result(res);
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
int daemadminout_get_sysdate()
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
	}
	else
	{
		strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d')");
		strcat(szQuery, "     , date_format(now(),'%H%i%s')");
		strcat(szQuery, "     , '");
		strcat(szQuery, gsys_date);
		strcat(szQuery, "'");

	}
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "sysdate: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "sysdate: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "sysdate: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		mysql_free_result(res);
		return -1;
	}
	
	row = mysql_fetch_row(res);
	memset(greg_date , 0x00, sizeof(greg_date ));
	memset(greg_time , 0x00, sizeof(greg_time ));
	memset(gproc_date, 0x00, sizeof(gproc_date));

	strcpy(greg_date ,   getstr(row, 0));
	strcpy(greg_time ,   getstr(row, 1));
	strcpy(gproc_date,   getstr(row, 2));
	
	mysql_free_result(res);


	return 0;
}

/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daemadminout_init_process(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
    #ifdef __DEBUG
    	ZzInitGlobalVariable2("d_", "/home/ezwon/zangsi_with_dcmd/daemon/log"); 
    #else
    	ZzInitGlobalVariable2("d_", "/logs/daemon");
    #endif

    ZzLOG(ALWAY, "[daemadminout]***************프로그램 시작***************\n");  

    // 파라미터 값 설정 및 초기화
    if (argc != 2)
    {
    	goto arg_error;
    }

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(con=db_connect_nodb("")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
	   	return(-1); 
	}

	if (!(log_con=db_connect_logdb("")))
	{
		ZzLOG(ERROR, "login log DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1); 
	}

	/* 처리일자 */
	memset(gsys_date, 0x00, sizeof(gsys_date));
	strcpy(gsys_date, argv[1]);
	ret=daemadminout_get_sysdate();
	if (ret < 0){
		db_disconnect(con);
		db_disconnect(log_con);
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
int daemadminout_term_process()
{
    // DB close
	db_disconnect(con);
	db_disconnect(log_con);
	
    ZzLOG(ALWAY, "[daemadminout]***************프로그램 종료***************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daemadminout_signal(int nSignal)
{
    daemadminout_term_process();
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
	signal(SIGTERM, daemadminout_signal);
	signal(SIGINT,  daemadminout_signal);
	signal(SIGQUIT, daemadminout_signal);
	signal(SIGKILL, daemadminout_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);

	if ( daemadminout_init_process(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daemadminout_main_process();
	
		/* 프로그램 종료루틴 */                    
		daemadminout_term_process();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
