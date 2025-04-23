/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem90021.cc
 *         기능 : 추천인 이벤트 통계
 *         설명 : 1일 1회 작업한다.
 *                SYSTEM  (00000000) => sysdate - 1일 처리하며,
 *                직접입력(yyyymmdd) => yyyymmdd를  처리한다.
 *       작성자 : HCS
 *       작성일 : 2010/05/01
 *     수정이력 : 
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
//#define _DEBUG_

int daem9002_init_process(int argc, char **argv);
int daem9002_main_process();
int daem9002_term_process();
int daem9002_rcmd_stat(); //신규 회원 대비 추천인 이벤트 통계
int daem9002_rcmd_sect_deal(); //분류별 거래 통계
int daem9002_rcmd_free_stat(); //무료 쿠폰 통계
int daem9002_delete_current(); //처리 일자 데이터 삭제
int daem9002_get_sysdate();
void daem9002_signal(int nSignal);



MYSQL     *bck_con;

char   gsys_date  [  8+1];	//처리일자(sysdate)
char   gproc_date [  8+1];	//처리일자(sysdate-1)
char   gproc_yymm [  6+1];	//처리년월
char   grank_yymm [  6+1];  //순위 매기는 달. 처리날짜 -1
char   greg_date  [  8+1];	//등록일
char   greg_time  [  6+1];	//등록시간
//******************************************************************************
//* daem9002 main
//******************************************************************************
int daem9002_main_process()
{
	ZzLOG(ALWAY, "daem9002_main_process: proc_date = %s,     reg_date = %s\n\n", gproc_date, greg_date);
	//--------------------------------------------------------------------------
	// 트렌젝션 시작
	//--------------------------------------------------------------------------
	if (tran_begin(bck_con)!=0) 
	{
	    ZzLOG(ERROR, "tran_begin: 테이베이스 오류입니다.\n");  
		ZzLOG(ERROR, "daem9002_main_process: [%d](%s)\n",mysql_errno(bck_con), mysql_error(bck_con));
	    return -1;
	}
	//--------------------------------------------------------------------------
	
	//--------------------------------------------------------------------------
	// 처리 일자 데이터 삭제
	//--------------------------------------------------------------------------
	if (daem9002_delete_current() != 0)
		goto daem9002_main_process_err;
	//--------------------------------------------------------------------------
	
	
	//--------------------------------------------------------------------------
	// 추천인 이벤트 통계
	//--------------------------------------------------------------------------
	if (daem9002_rcmd_stat() != 0)
		goto daem9002_main_process_err;
	//--------------------------------------------------------------------------


	//--------------------------------------------------------------------------
	// 분류별 거래 통계
	//--------------------------------------------------------------------------
	if (daem9002_rcmd_sect_deal() != 0)
		goto daem9002_main_process_err;
	//--------------------------------------------------------------------------

	//--------------------------------------------------------------------------
	// 트렌젝션 적용
	//--------------------------------------------------------------------------
	if (tran_commit(bck_con)!=0)
	{
	    ZzLOG(ERROR, "daem9002_main_process: tran_commit error...\n");
		ZzLOG(ERROR, "daem9002_main_process: [%d](%s)\n",mysql_errno(bck_con), mysql_error(bck_con));
	    goto daem9002_main_process_err;
	}
	
	tran_end(bck_con);
	//--------------------------------------------------------------------------

	return (0);


daem9002_main_process_err:
	tran_rollback(bck_con);
	tran_end(bck_con);
    return -1;
}
//******************************************************************************
//* 처리 일자 데이터 삭제
//******************************************************************************
int daem9002_delete_current()
{
	char szQuery[10000];		// query string
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " delete from zangsi_sum.T_RCMD_STAT where stat_date = '%s' ", gproc_date);
	if (mysql_query(bck_con, szQuery))
	{
	    ZzLOG(ERROR, "daem9002_delete_current: mysql_query error...\n");
		ZzLOG(ERROR, "daem9002_delete_current: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }

	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " delete from zangsi_sum.T_RCMD_SALE_DD where deal_date = '%s' ", gproc_date);
	if (mysql_query(bck_con, szQuery))
	{
	    ZzLOG(ERROR, "daem9002_delete_current: mysql_query error...\n");
		ZzLOG(ERROR, "daem9002_delete_current: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }

    return 0;
}


//******************************************************************************
//* 신규 회원 대비 추천인 이벤트 통계
//******************************************************************************
int daem9002_rcmd_stat()
{
	char szQuery[10000];		// query string
	
	MYSQL_RES* res;
	MYSQL_ROW  row;
	
	int nTotInCnt = 0;
	int nTotOutCnt = 0;
	int nRcmdInCnt = 0;
	int nRcmdInChargeCnt = 0;
	int nTotChargeAmt = 0;
	int nTotChargeCnt = 0;
	int nRcmdChargeAmt = 0;
	int nRcmdDealAmt = 0;
	int nRcmdMultiIpPwCnt = 0;
	int nRcmdMultiIpCnt = 0;
	int nIssueFreeCnt = 0;
	int nUseFreeCnt = 0;
	
	//--------------------------------------------------------------------------
	// 전체 신규가입 회원 수(탈퇴자 제외) nTotInCnt
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select count(user_id) from zangsi.T_USER_INFO where reg_date = '%s' and use_stat = '0' ", gproc_date);

	if (mysql_query(bck_con, szQuery))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_query error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (!(res = mysql_store_result(bck_con)))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (mysql_num_rows(res)==0)
	{
		mysql_free_result(res);
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_num_rows error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
		return -1;
    }
    row = mysql_fetch_row(res);
    nTotInCnt = (int)getint(row, 0);
	mysql_free_result(res);
	ZzLOG(ALWAY, "daem9002_rcmd_stat: 전체 신규가입 회원 수(탈퇴자 제외)=[%d]\n", nTotInCnt);
	//--------------------------------------------------------------------------
	
	
	//--------------------------------------------------------------------------
	// 전체 신규 가입 회원 중 당일 탈퇴자 수 nTotOutCnt
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select count(user_id) from zangsi.T_USER_INFO where reg_date = '%s' and use_stat != '0' ", gproc_date);

	if (mysql_query(bck_con, szQuery))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_query error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (!(res = mysql_store_result(bck_con)))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (mysql_num_rows(res)==0)
	{
		mysql_free_result(res);
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_num_rows error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
		return -1;
    }
    row = mysql_fetch_row(res);
    nTotOutCnt = (int)getint(row, 0);
	mysql_free_result(res);
	ZzLOG(ALWAY, "daem9002_rcmd_stat: 전체 신규 가입 회원 중 당일 탈퇴자 수=[%d]\n", nTotOutCnt);
	//--------------------------------------------------------------------------


	//--------------------------------------------------------------------------
	// 전체 신규 가입 회원 결제 금액 nTotChargeAmt
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select new_amt from zangsi_sum.T_ANALY_ACCT where acct_date = '%s' ", gproc_date);

	if (mysql_query(bck_con, szQuery))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_query error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (!(res = mysql_store_result(bck_con)))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (mysql_num_rows(res)==0)
	{
		mysql_free_result(res);
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_num_rows error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
		return -1;
    }
    row = mysql_fetch_row(res);
    nTotChargeAmt = (int)getint(row, 0);
	mysql_free_result(res);
	ZzLOG(ALWAY, "daem9002_rcmd_stat: 전체 신규 가입 회원 결제 금액=[%d]\n", nTotChargeAmt);
	//--------------------------------------------------------------------------

	//--------------------------------------------------------------------------
	// 전체 신규 가입 회원 결제 건수 nTotChargeCnt
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select new_cnt from zangsi_sum.T_ANALY_ACCT where acct_date = '%s' ", gproc_date);

	if (mysql_query(bck_con, szQuery))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_query error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (!(res = mysql_store_result(bck_con)))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (mysql_num_rows(res)==0)
	{
		mysql_free_result(res);
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_num_rows error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
		return -1;
    }
    row = mysql_fetch_row(res);
    nTotChargeCnt = (int)getint(row, 0);
	mysql_free_result(res);
	ZzLOG(ALWAY, "daem9002_rcmd_stat: 전체 신규 가입 회원 결제 건수=[%d]\n", nTotChargeCnt);
	//--------------------------------------------------------------------------



	//--------------------------------------------------------------------------
	// 추천을 하고 가입한 회원 수 nRcmdInCnt
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select count(user_id) from zangsi.T_USER_RCMD where reg_date = '%s' ", gproc_date);

	if (mysql_query(bck_con, szQuery))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_query error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (!(res = mysql_store_result(bck_con)))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (mysql_num_rows(res)==0)
	{
		mysql_free_result(res);
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_num_rows error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
		return -1;
    }
    row = mysql_fetch_row(res);
    nRcmdInCnt = (int)getint(row, 0);
	mysql_free_result(res);
	ZzLOG(ALWAY, "daem9002_rcmd_stat: 추천을 하고 가입한 회원 수=[%d]\n", nRcmdInCnt);
	//--------------------------------------------------------------------------


	//--------------------------------------------------------------------------
	// 추천을 하고 가입한 회원 중 결제자 수 nRcmdInChargeCnt
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select count(user_id) from zangsi.T_USER_RCMD_CHARGE where reg_date = '%s' and cnl_yn = 'N' ", gproc_date);

	if (mysql_query(bck_con, szQuery))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_query error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (!(res = mysql_store_result(bck_con)))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (mysql_num_rows(res)==0)
	{
		mysql_free_result(res);
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_num_rows error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
		return -1;
    }
    row = mysql_fetch_row(res);
    nRcmdInChargeCnt = (int)getint(row, 0);
	mysql_free_result(res);
	ZzLOG(ALWAY, "daem9002_rcmd_stat: 추천을 하고 가입한 회원 중 결제자 수=[%d]\n", nRcmdInChargeCnt);
	//--------------------------------------------------------------------------


	//--------------------------------------------------------------------------
	// 추천을 하고 가입한 회원의 결제 금액 nRcmdChargeAmt
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select sum(price_amt) from zangsi.T_USER_RCMD_CHARGE where reg_date = '%s' and cnl_yn = 'N' ", gproc_date);

	if (mysql_query(bck_con, szQuery))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_query error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (!(res = mysql_store_result(bck_con)))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (mysql_num_rows(res)==0)
	{
		mysql_free_result(res);
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_num_rows error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
		return -1;
    }
    row = mysql_fetch_row(res);
    nRcmdChargeAmt = (int)getint(row, 0);
	mysql_free_result(res);
	ZzLOG(ALWAY, "daem9002_rcmd_stat: 추천을 하고 가입한 회원의 결제 금액=[%d]\n", nRcmdChargeAmt);
	//--------------------------------------------------------------------------



	//--------------------------------------------------------------------------
	// 이벤트 가입 회원이 결제 후 사용한 금액 nRcmdDealAmt
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select sum(b.price_amt) from zangsi.T_USER_RCMD_CHARGE a, zangsi.T_DEAL_INFO b  "
					 " where a.user_id = b.buy_user and b.deal_date = '%s' and a.reg_date <= '%s' and b.fixamt_yn = '0' "
					 , gproc_date, gproc_date);

	if (mysql_query(bck_con, szQuery))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_query error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (!(res = mysql_store_result(bck_con)))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (mysql_num_rows(res)==0)
	{
		mysql_free_result(res);
	    ZzLOG(ALWAY, "daem9002_rcmd_stat: 결과 없음.\n");
		ZzLOG(ALWAY, "daem9002_rcmd_stat: (%s)\n", szQuery);
    }
    else
    {	
	    row = mysql_fetch_row(res);
	    nRcmdDealAmt = (int)getint(row, 0);
		mysql_free_result(res);
	}
	ZzLOG(ALWAY, "daem9002_rcmd_stat: 이벤트 가입 회원이 결제 후 사용한 금액=[%d]\n", nRcmdDealAmt);
	//--------------------------------------------------------------------------

	
	//--------------------------------------------------------------------------
	// 이벤트 가입 후 결제한 인원 중 다중아이디 수(IP & PW, IP) nRcmdMultiIpPwCnt & nRcmdMultiIpCnt
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select rcmd_user, user_id, ip_addr from zangsi.T_USER_RCMD_CHARGE where reg_date = '%s' ", gproc_date);
	if (mysql_query(bck_con, szQuery))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_query error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (!(res = mysql_store_result(bck_con)))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (mysql_num_rows(res)==0)
	{
		mysql_free_result(res);
	    ZzLOG(ALWAY, "daem9002_rcmd_stat: 결과 없음.\n");
		ZzLOG(ALWAY, "daem9002_rcmd_stat: (%s)\n", szQuery);
    }
    else
    {
	    while(row = mysql_fetch_row(res))
	    {
			char szRcmdUser[12+1];
			memset(szRcmdUser, 0x00, sizeof(szRcmdUser));
			strcpy(szRcmdUser, getstr(row,0));
			
			char szUserId[12+1];
			memset(szUserId, 0x00, sizeof(szUserId));
			strcpy(szUserId, getstr(row,1));
			
			MYSQL_RES* res2;
			MYSQL_ROW  row2;
			
			int nMultiCnt = 0;
			
			//--------------------------------------------------------------------------
			// IP & PW
			//--------------------------------------------------------------------------
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " select count(*) "
							 " from zangsi.T_USER_INFO b, zangsi.T_USER_STAT c, zangsi.T_USER_INFO d, zangsi.T_USER_STAT e "
							 " where "
							 " b.user_id = c.user_id and d.user_id = e.user_id and "
							 " b.pass_wd = d.pass_wd and c.conn_ip = e.conn_ip and b.user_id != d.user_id and "
							 " b.user_id = '%s' and d.user_id = '%s' "
							 , szRcmdUser, szUserId);
			if (mysql_query(bck_con, szQuery))
			{
			    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_store_result error...\n");
				ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
			    return -1;
			}	
			if (!(res2 = mysql_store_result(bck_con)))
			{
			    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_store_result error...\n");
				ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
			    return -1;
			}	
			if (mysql_num_rows(res2)==0)
			{
			    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_num_rows error...\n");
				ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
			    return -1;
			}
			row2 = mysql_fetch_row(res2);
			nMultiCnt = (int)getint(row2, 0);
	    	mysql_free_result(res2);
	    	
	    	if(nMultiCnt > 0)
	    		nRcmdMultiIpPwCnt++;
			//--------------------------------------------------------------------------
			
			
			//--------------------------------------------------------------------------
			//IP
			//--------------------------------------------------------------------------
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " select count(*) "
							 " from zangsi.T_USER_INFO b, zangsi.T_USER_STAT c, zangsi.T_USER_INFO d, zangsi.T_USER_STAT e "
							 " where "
							 " b.user_id = c.user_id and d.user_id = e.user_id and "
							 " c.conn_ip = e.conn_ip and b.user_id != d.user_id and "
							 " b.user_id = '%s' and d.user_id = '%s' "
							 , szRcmdUser, szUserId);
			if (mysql_query(bck_con, szQuery))
			{
			    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_store_result error...\n");
				ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
			    return -1;
			}	
			if (!(res2 = mysql_store_result(bck_con)))
			{
			    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_store_result error...\n");
				ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
			    return -1;
			}	
			if (mysql_num_rows(res2)==0)
			{
			    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_num_rows error...\n");
				ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
			    return -1;
			}
			row2 = mysql_fetch_row(res2);
			nMultiCnt = 0;
			nMultiCnt = (int)getint(row2, 0);
	    	mysql_free_result(res2);
	    	
	    	if(nMultiCnt > 0)
	    		nRcmdMultiIpCnt++;
	    	//--------------------------------------------------------------------------
		}
		mysql_free_result(res);
	}
	ZzLOG(ALWAY, "daem9002_rcmd_stat: 이벤트 가입 후 결제한 인원 중 다중아이디 수(IP & PW)=[%d]\n", nRcmdMultiIpPwCnt);
	ZzLOG(ALWAY, "daem9002_rcmd_stat: 이벤트 가입 후 결제한 인원 중 다중아이디 수(IP)=[%d]\n", nRcmdMultiIpCnt);
	//--------------------------------------------------------------------------
	
	
	
	//--------------------------------------------------------------------------
	// 발행된 쿠폰 수  nIssueFreeCnt
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select count(distinct(rcmd_user)) from zangsi.T_USER_RCMD_CHARGE where reg_date = '%s' and cnl_yn = 'N' ", gproc_date);

	if (mysql_query(bck_con, szQuery))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_query error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (!(res = mysql_store_result(bck_con)))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (mysql_num_rows(res)==0)
	{
		mysql_free_result(res);
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_num_rows error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
		return -1;
    }
    row = mysql_fetch_row(res);
    nIssueFreeCnt = (int)getint(row, 0);
	mysql_free_result(res);
	ZzLOG(ALWAY, "daem9002_rcmd_stat: 발행된 쿠폰 수=[%d]\n", nIssueFreeCnt);
	//--------------------------------------------------------------------------


	
	//--------------------------------------------------------------------------
	// 사용된 쿠폰 수  nUseFreeCnt
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select count(distinct(a.deal_no)) from zangsi.T_EVENT_DEAL a, zangsi.T_EVENT_FREE b "
					 " where a.reg_date = '%s' and a.buy_user = b.user_id "
					 , gproc_date);

	if (mysql_query(bck_con, szQuery))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_query error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (!(res = mysql_store_result(bck_con)))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (mysql_num_rows(res)==0)
	{
		mysql_free_result(res);
	    ZzLOG(ALWAY, "daem9002_rcmd_stat: 결과 없음.\n");
		ZzLOG(ALWAY, "daem9002_rcmd_stat: (%s)\n", szQuery);
    }
    else
	{    	
	    row = mysql_fetch_row(res);
	    nUseFreeCnt = (int)getint(row, 0);
		mysql_free_result(res);
	}
	ZzLOG(ALWAY, "daem9002_rcmd_stat: 사용된 쿠폰 수=[%d]\n", nUseFreeCnt);
	//--------------------------------------------------------------------------

	
	//--------------------------------------------------------------------------
	//집계 데이터 등록
	//--------------------------------------------------------------------------
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " insert into zangsi_sum.T_RCMD_STAT "
					 " (stat_date "
					 " , tot_in_cnt, tot_out_cnt, rcmd_in_cnt, rcmd_in_charge_cnt, tot_charge_cnt, tot_charge_amt, rcmd_charge_amt, rcmd_deal_amt, rcmd_multi_ip_pw_cnt, rcmd_multi_pw_cnt, issue_free_cnt, use_free_cnt "
					 " , reg_date, reg_time) "
					 " values "
					 " ('%s', %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, '%s', '%s')  "
					 , gproc_date
					 , nTotInCnt, nTotOutCnt, nRcmdInCnt, nRcmdInChargeCnt, nTotChargeCnt, nTotChargeAmt, nRcmdChargeAmt, nRcmdDealAmt, nRcmdMultiIpPwCnt, nRcmdMultiIpCnt, nIssueFreeCnt, nUseFreeCnt
					 , greg_date, greg_time);
	
	if (mysql_query(bck_con, szQuery))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_stat: mysql_query error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
    
    ZzLOG(ALWAY, "daem9002_rcmd_stat: 추천인 이벤트 통계 집계 완료\n");

	return 0;
}

//******************************************************************************
//* 분류별 거래 통계
//******************************************************************************
int daem9002_rcmd_sect_deal()
{
	char szQuery[10000];		// query string
	
	//--------------------------------------------------------------------------
	// 분류별 다운로드 수, 금액 
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " insert into zangsi_sum.T_RCMD_SALE_DD "
					 " select b.deal_date, sect_code, count(deal_no), sum(b.price_amt), '%s', '%s' "
					 " from zangsi.T_USER_RCMD_CHARGE a, zangsi.T_DEAL_INFO b "
					 " where a.user_id = b.buy_user and b.deal_date = '%s' and a.reg_date <= '%s' and b.fixamt_yn = '0' "
					 " and cont_gu = 'WE' "
					 " group by sect_code "
					 , greg_date, greg_time, gproc_date, gproc_date);

	if (mysql_query(bck_con, szQuery))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_sect_deal: mysql_query error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_sect_deal: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }

	//--------------------------------------------------------------------------
	// 분류별 다운로드 수, 금액 
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " insert into zangsi_sum.T_RCMD_SALE_DD "
					 " select b.deal_date, 'FD', count(deal_no), sum(b.price_amt), '%s', '%s' "
					 " from zangsi.T_USER_RCMD_CHARGE a, zangsi.T_DEAL_INFO b "
					 " where a.user_id = b.buy_user and b.deal_date = '%s' and a.reg_date <= '%s' and b.fixamt_yn = '0' "
					 " and cont_gu = 'FD' "
					 " group by sect_code "
					 , greg_date, greg_time, gproc_date, gproc_date);

	if (mysql_query(bck_con, szQuery))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_sect_deal: mysql_query error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_sect_deal: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }

    ZzLOG(ALWAY, "daem9002_rcmd_stat: 추천인 이벤트 참가자 분류별 거래 통계 집계 완료\n");

	return 0;
}

//******************************************************************************
//* 무료 쿠폰 통계
//******************************************************************************
int daem9002_rcmd_free_stat()
{
	char szQuery[10000];		// query string
	
	MYSQL_RES* res;
	MYSQL_ROW  row;

	
	//--------------------------------------------------------------------------
	// 
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));

	if (mysql_query(bck_con, szQuery))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_free_stat: mysql_query error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_free_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (!(res = mysql_store_result(bck_con)))
	{
	    ZzLOG(ERROR, "daem9002_rcmd_free_stat: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem9002_rcmd_free_stat: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (mysql_num_rows(res)==0)
	{
		mysql_free_result(res);
	    ZzLOG(ALWAY, "daem9002_rcmd_free_stat: 결과 없음.\n");
		ZzLOG(ALWAY, "daem9002_rcmd_free_stat: (%s)\n", szQuery);
    }
    while(row = mysql_fetch_row(res))
    {
	}
	mysql_free_result(res);

	return 0;
}


/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem9002_get_sysdate()
{
	MYSQL_RES* res;
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
		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -2 DAY),'%Y%m')");
	}
	else
	{
		sprintf(szQuery, "SELECT date_format(now(),'%%Y%%m%%d')"
						 "     , date_format(now(),'%%H%%i%%s')"
						 "     , '%s' "
						 "     , substring('%s', 1, 6) "
						 "     , date_format(date_add('%s', INTERVAL -1 DAY),'%%Y%%m') "
						 , gsys_date
						 , gsys_date
						 , gsys_date);
	}
	if (mysql_query(bck_con, szQuery))
	{
	    ZzLOG(ERROR, "sysdate: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(bck_con), mysql_error(bck_con));
		return -1;
	}
	if (!(res = mysql_store_result(bck_con)))
	{
	    ZzLOG(ERROR, "sysdate: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(bck_con), mysql_error(bck_con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "sysdate: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(bck_con), mysql_error(bck_con));
		mysql_free_result(res);
		return -1;
	}
	
	row = mysql_fetch_row(res);
	memset(greg_date , 0x00, sizeof(greg_date ));
	memset(greg_time , 0x00, sizeof(greg_time ));
	memset(gproc_date, 0x00, sizeof(gproc_date));
	memset(gproc_yymm, 0x00, sizeof(gproc_yymm));
	memset(grank_yymm, 0x00, sizeof(grank_yymm));

	strcpy(greg_date ,   getstr(row, 0));
	strcpy(greg_time ,   getstr(row, 1));
	strcpy(gproc_date,   getstr(row, 2));
	strcpy(gproc_yymm,   getstr(row, 3));
	strcpy(grank_yymm,   getstr(row, 4));
	
	
	mysql_free_result(res);


	return 0;
}

/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem9002_init_process(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */

    #ifdef __DEBUG
    ZzInitGlobalVariable2("D_daem9002", "/logs/daemon"); 
    #else
    ZzInitGlobalVariable2("daem9002", "/logs/daemon"); 
    #endif

    ZzLOG(ALWAY, "[daem9002]***************프로그램 시작***************\n");  

    // 파라미터 값 설정 및 초기화
    if (argc != 2){
    	goto arg_error;
    }

	//--------------------------------------------------------------------------
	// bck DB 연결
	//--------------------------------------------------------------------------
	if (!(bck_con=db_connect_nodb("")))
	{
		ZzLOG(ERROR, "bck DB에 접속하지 못 하였습니다...\n");
	   	return(-1); 
	}
	#ifdef __DEBUG
	printf("bck 디비연결\n");
	#endif

	/* 처리일자 */
	memset(gsys_date, 0x00, sizeof(gsys_date));
	strcpy(gsys_date, argv[1]);
	ret=daem9002_get_sysdate();
	if (ret < 0){
		db_disconnect(bck_con);
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
int daem9002_term_process()
{
    // DB close
	db_disconnect(bck_con);
    ZzLOG(ALWAY, "[daem9002]***************프로그램 종료***************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem9002_signal(int nSignal)
{
    daem9002_term_process();
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
	signal(SIGTERM, daem9002_signal);
	signal(SIGINT,  daem9002_signal);
	signal(SIGQUIT, daem9002_signal);
	signal(SIGKILL, daem9002_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);

	if ( daem9002_init_process(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daem9002_main_process();
	
		/* 프로그램 종료루틴 */                    
		daem9002_term_process();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
