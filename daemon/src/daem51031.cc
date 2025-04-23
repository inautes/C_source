/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem510311.cc
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

int daem51031_init_process(int argc, char **argv);
int daem51031_main_process();
int daem51031_term_process();
int daem51031_delete_current();
int daem51031_insert_deal();
int daem51031_get_sysdate();
void daem51031_signal(int nSignal);

MYSQL     *con;
MYSQL_RES *res;
MYSQL_ROW  row;

char   gsys_date  [  8+1];	//처리일자(sysdate)
char   gproc_date [  8+1];	//처리일자(sysdate-1)
char   gproc_yymm [  6+1];	//처리년월
char   greg_date  [  8+1];	//등록일
char   greg_time  [  6+1];	//등록시간
//******************************************************************************
//* daem51031 main
//******************************************************************************
int daem51031_main_process()
{
	//--------------------------------------------------------------------------
	// 트렌젝션시작
	//--------------------------------------------------------------------------
	if (tran_begin(con)!=0) {
	    ZzLOG(ERROR, "tran_begin: 테이베이스 오류입니다.\n");  
		ZzLOG(ERROR, "daem51031_main_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
	}

	// 처리일자의 자료삭제
	if (daem51031_delete_current() != 0)
		goto daem51031_main_process_err;

	// 판매 집계처리
	if (daem51031_insert_deal() != 0)
		goto daem51031_main_process_err;

	if (tran_commit(con)!=0){
	    ZzLOG(ERROR, "daem51031_main_process: tran_commit error...\n");
		ZzLOG(ERROR, "daem51031_main_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    goto daem51031_main_process_err;
	}

	return (0);

daem51031_main_process_err:
	tran_rollback(con);
    return -1;
}


//******************************************************************************
//* daem51031_delete_current()
//* 처리일자의 자료를 삭제처리 한다.
//******************************************************************************
int daem51031_delete_current()
{
	char szQuery[1000];		// query string

	//--------------------------------------------------------------------------
	// 처리일자 해당되는 자료삭제(재작업을 위해)
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi_sum.T_FREE_DD"
	                 " WHERE deal_date    = '%s' "
	                 ,gproc_date
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem51031_delete_current: DELETE zangsi.T_AUD_ENTER error...\n");
		ZzLOG(ERROR, "daem51031_delete_current: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
    }

    return 0;
}


//******************************************************************************
//* daem51031_insert_deal()
//* 일일 사용자별 거래집계처리
//******************************************************************************
int daem51031_insert_deal()
{
	char szQuery[10000];		// query string
	int ret=0;
	int nRowcnt = 0;
	
	//--------------------------------------------------------------------------
	// 해당일자의 자료를 생성한다.
	//--------------------------------------------------------------------------
	ret = 0;
	
	int nNewFreeCnt = 0;
	int nDealUserCnt = 0;
	int nTotFreeCnt = 0;
	double dAveDownCnt = 0;
	int n2MonReloginCnt = 0;
	
	//일자별 데이터 생성
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " INSERT INTO zangsi_sum.T_FREE_DD (deal_date, free_cnt, free_amt, reg_date, reg_time) "
					 " select '%s', count(deal_no), sum(price_amt), '%s', '%s'  "
					 " from zangsi.T_EVENT_DEAL where reg_date = '%s' "
					 , gproc_date, greg_date, greg_time, gproc_date);
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem51031_insert_deal: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}

	//신규이용건수 생성
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select max(reg_date), count(deal_no) "
					 " from zangsi.T_EVENT_DEAL where reg_date <= '%s' "
					 " group by buy_user "
					 " having count(deal_no) = 1 and max(reg_date) = '%s' "
					 , gproc_date, gproc_date);

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem51031_insert_deal: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daem51031_insert_deal: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
 	nNewFreeCnt = mysql_num_rows(res);
	
	mysql_free_result(res);
	
	
	//중복 데이터 제거 위한 템프 테이블 생성
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " CREATE TEMPORARY TABLE zangsi_sum.TEMP_FREE_DD  "
					 " select reg_date, buy_user  " 
					 " from zangsi.T_EVENT_DEAL  "
					 " where reg_date = '%s' "
					 " group by buy_user "
					 , gproc_date);


	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem51031_insert_deal: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	
	//참여인원 캐시 사용 건수 생성
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select count(distinct(b.buy_user)) "
					 " from zangsi_sum.TEMP_FREE_DD a, zangsi.T_DEAL_INFO b "
					 " where a.buy_user = b.buy_user and a.reg_date = b.deal_date and a.reg_date = '%s' and b.fixamt_yn in('0', 'C') "
					 , gproc_date);


	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem51031_insert_deal: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daem51031_insert_deal: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "daem51031_insert_deal: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		mysql_free_result(res);
		return -1;
	}
	
	row = mysql_fetch_row(res);
	nDealUserCnt = (int)getint(row,0);
	mysql_free_result(res);

	//템프 테이블 삭제
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DROP TABLE zangsi_sum.TEMP_FREE_DD ");
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5103_insert_deal: DROP zangsi_sum.TEMP_FREE_DD error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
    }

	//평균 일일 다운건수 생성
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select count(deal_no) / count(distinct(buy_user)) "
					 " from zangsi.T_DEAL_INFO "
					 " where deal_date = '%s' and fixamt_yn in('0', 'C') "
					 , gproc_date);


	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem51031_insert_deal: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daem51031_insert_deal: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "daem51031_insert_deal: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		mysql_free_result(res);
		return -1;
	}
	
	row = mysql_fetch_row(res);
	dAveDownCnt = getnum(row,0);
	mysql_free_result(res);
	
	//누적 참여 인원 생성
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select count(distinct(buy_user)) "
					 " from zangsi.T_EVENT_DEAL where reg_date <= '%s' "
					 , gproc_date);


	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem51031_insert_deal: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daem51031_insert_deal: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "daem51031_insert_deal: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		mysql_free_result(res);
		return -1;
	}
	
	row = mysql_fetch_row(res);
	nTotFreeCnt = (int)getint(row,0);
	mysql_free_result(res);

	//두달만의 방문자수 생성
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select count(*) as relogin_cnt "
					 " from zangsi_sum.T_LOGIN_INFO "
					 " where "
					 " to_days(date_format(login_date, '%Y%%m%%d'))  >=  to_days(date_format(date_add('%s' ,INTERVAL -60 DAY ),'%Y%%m%%d')) "
					 " and to_days(date_format(login_date, '%Y%%m%%d'))  <=  to_days(date_format(date_add('%s' ,INTERVAL -1 DAY ),'%Y%%m%%d')) "
					 " and to_days(date_format(login_date, '%Y%%m%%d')) - to_days(date_format(last_date, '%Y%%m%%d')) = 60 "
					 , gproc_date, gproc_date);


	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem51031_insert_deal: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daem51031_insert_deal: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "daem51031_insert_deal: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		mysql_free_result(res);
		return -1;
	}
	
	row = mysql_fetch_row(res);
	n2MonReloginCnt = (int)getint(row,0);
	mysql_free_result(res);
	


	//중복 아이피 템프 테이블 생성
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " CREATE TEMPORARY TABLE zangsi_sum.TEMP_DEAL "
					 " select deal_ip, count(*) as cnt " 
					 " from zangsi.T_EVENT_DEAL "
					 " where (reg_date  = '%s' and reg_time >= '070000') "
					 " or  (reg_date  = date_format(date_add('%s' ,INTERVAL 1 DAY ),'%Y%%m%%d')   and reg_time <  '070000') "
					 " group by deal_ip "
					 " having count(*) > 1 "
					 , gproc_date, gproc_date);


	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem51031_insert_deal: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	
	//중복 사용자 ip건수, 중복 사용자 다운건수 추가
	memset (szQuery, 0x00, sizeof(szQuery));
	strcpy(szQuery, " select count(deal_ip), sum(cnt) "
					 "  from zangsi_sum.TEMP_DEAL ");


	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem51031_insert_deal: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daem51031_insert_deal: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "daem51031_insert_deal: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		mysql_free_result(res);
		return -1;
	}
	
	row = mysql_fetch_row(res);
	int nDupIpCnt = (int)getint(row,0);
	int nDupDealCnt = (int)getint(row,1);
	mysql_free_result(res);

	//템프 테이블 삭제
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DROP TABLE zangsi_sum.TEMP_DEAL ");
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5103_insert_deal: DROP zangsi_sum.TEMP_FREE_DD error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
    }




	//나머지 데이터 업데이트
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " update zangsi_sum.T_FREE_DD "
					 " set new_free_cnt = %d , deal_user_cnt = %d, tot_free_cnt = %d, ave_down_cnt = %.2f, 2mon_relogin_cnt = %d, dup_ip_cnt = %d, dup_deal_cnt = %d "
					 " where deal_date = '%s' "
					 , nNewFreeCnt, nDealUserCnt, nTotFreeCnt, dAveDownCnt, n2MonReloginCnt, nDupIpCnt, nDupDealCnt
					 , gproc_date);


	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem51031_insert_deal: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}

	
	
	return 0;
}



/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem51031_get_sysdate()
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
int daem51031_init_process(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("d_", "/logs/daemon"); 

    ZzLOG(ALWAY, "[daem51031]***************프로그램 시작***************\n");  

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
	ret=daem51031_get_sysdate();
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
int daem51031_term_process()
{
    // DB close
	db_disconnect(con);
    ZzLOG(ALWAY, "[daem51031]***************프로그램 종료***************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem51031_signal(int nSignal)
{
    daem51031_term_process();
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
	signal(SIGTERM, daem51031_signal);
	signal(SIGINT,  daem51031_signal);
	signal(SIGQUIT, daem51031_signal);
	signal(SIGKILL, daem51031_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);

	if ( daem51031_init_process(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daem51031_main_process();
	
		/* 프로그램 종료루틴 */                    
		daem51031_term_process();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
