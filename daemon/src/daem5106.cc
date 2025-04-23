/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5106.cc
 *         기능 : 사용자 실적분석
 *         설명 :  출금 집계한다
 *       작성자 : LEE
 *       작성일 : 2005/12/06
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

int daem5106_init_process(int argc, char **argv);
int daem5106_main_process();
int daem5106_term_process();
int daem5106_delete_current();
int daem5106_insert_current();
int daem5106_get_sysdate();
void daem5106_signal(int nSignal);

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
//* daem5106 main
//******************************************************************************
int daem5106_main_process()
{
	//--------------------------------------------------------------------------
	// 트렌젝션시작
	//--------------------------------------------------------------------------
	if (tran_begin(con)!=0) {
	    ZzLOG(ERROR, "tran_begin: 테이베이스 오류입니다.\n");  
		ZzLOG(ERROR, "daem5106_main_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
	}

	// 처리일자의 자료삭제
	if (daem5106_delete_current() != 0)
		goto daem5106_main_process_err;

	// 판매 집계처리
	if (daem5106_insert_current() != 0)
		goto daem5106_main_process_err;

	if (tran_commit(con)!=0){
	    ZzLOG(ERROR, "daem5106_main_process: tran_commit error...\n");
		ZzLOG(ERROR, "daem5106_main_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    goto daem5106_main_process_err;
	}

	return (0);

daem5106_main_process_err:
	tran_rollback(con);
    return -1;
}


//******************************************************************************
//* daem5106_delete_current()
//* 처리일자의 자료를 삭제처리 한다.
//******************************************************************************
int daem5106_delete_current()
{
	char szQuery[1000];		// query string

	//--------------------------------------------------------------------------
	// 처리일자 해당되는 자료삭제(재작업을 위해)
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi_sum.T_ANALY_DEAL_YM WHERE deal_ym in ('000000', '%s')", gproc_yymm);
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5106_delete_current: DELETE zangsi_sum.T_ANALY_DEAL_YM error...\n");
		ZzLOG(ERROR, "daem5106_delete_current: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
    }

    return 0;
}


//******************************************************************************
//* daem5106_insert_login()
//* 일일 사용자별 거래집계처리
//******************************************************************************
int daem5106_insert_current()
{
	char szQuery[10000];		// query string
	int ret=0;
	
	//--------------------------------------------------------------------------
	// 계산된 판매집계를 TEMPORARY에 생성한다.
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "CREATE TEMPORARY TABLE zangsi_sum.TEMP_ANALY_DEAL_YM     "
	                 "SELECT substring(proc_date,1,6) as deal_ym    "
	                 "     , user_id       as user_id               "
	                 "     , 0             as send_cnt              "
	                 "     , 0             as send_amt              "
	                 "     , 0             as recv_cnt              "
	                 "     , 0             as recv_amt              "
	                 "     , count(*)      as out_cnt               "
	                 "     , sum(amount)   as out_amt               "
	                 "     , 0             as nmnt_cnt              "
	                 "  FROM zangsi.T_OUT_AMOUNT                    "
	                 " WHERE proc_stat  = '3'                       "
	                 "   AND proc_date >= concat('%s','01')         "
	                 "   AND proc_date <= concat('%s','31')         "
	                 " GROUP BY substring(proc_date,1,6), user_id   "
	                 /*
	                 "SELECT substring(send_date,1,6) as deal_ym    "
	                 "     , send_user     as user_id               "
	                 "     , count(*)      as send_cnt              "
	                 "     , sum(send_amt) as send_amt              "
	                 "     , 0             as recv_cnt              "
	                 "     , 0             as recv_amt              "
	                 "     , 0             as out_cnt               "
	                 "     , 0             as out_amt               "
	                 "     , 0             as nmnt_cnt              "
	                 "  FROM T_SEND_MONEY                           "
	                 " WHERE send_date >= concat('%s','01')         "
	                 "   AND send_date <= concat('%s','31')         "
	                 " GROUP BY substring(send_date,1,6), send_user "
	                 "UNION ALL                                     "
	                 "SELECT substring(send_date,1,6) as deal_ym    "
	                 "     , recv_user     as user_id               "
	                 "     , 0             as send_cnt              "
	                 "     , 0             as send_amt              "
	                 "     , count(*)      as recv_cnt              "
	                 "     , sum(recv_amt) as recv_amt              "
	                 "     , 0             as out_cnt               "
	                 "     , 0             as out_amt               "
	                 "     , 0             as nmnt_cnt              "
	                 "  FROM T_SEND_MONEY                           "
	                 " WHERE send_date >= concat('%s','01')         "
	                 "   AND send_date <= concat('%s','31')         "
	                 " GROUP BY substring(send_date,1,6), recv_user "
	                 "UNION ALL                                     "
	                 */	                 
	                 /*
	                 "UNION ALL                                     "
	                 "SELECT substring(reg_date,1,6) as deal_ym     "
	                 "     , parent_user   as user_id               "
	                 "     , 0             as send_cnt              "
	                 "     , 0             as send_amt              "
	                 "     , 0             as recv_cnt              "
	                 "     , 0             as recv_amt              "
	                 "     , 0             as out_cnt               "
	                 "     , 0             as out_amt               "
	                 "     , count(*)      as nmnt_cnt              "
	                 "  FROM T_EVENT_USER                           "
	                 " WHERE event_id  = 5                          "
	                 "   AND reg_date >= concat('%s','01')          "
	                 "   AND reg_date <= concat('%s','31')          "
	                 " GROUP BY substring(reg_date,1,6), parent_user"
	                 ,gproc_yymm
	                 ,gproc_yymm
	                 */
	                 
	                 ,gproc_yymm
	                 ,gproc_yymm
	                 ,gproc_yymm
	                 ,gproc_yymm
	                 ,gproc_yymm
	                 ,gproc_yymm
	                 );
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5106_insert_current: CREATE zangsi_sum.TEMP_ANALY_DEAL_YM reg_cnt error...\n");
		ZzLOG(ERROR, "daem5106_insert_current: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
    }
    
	//--------------------------------------------------------------------------
	// 해당월 월집계
	//--------------------------------------------------------------------------
	ret = 0;
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "INSERT INTO zangsi_sum.T_ANALY_DEAL_YM    "
	                 "     ( deal_ym       , user_id        "
	                 "     , send_cnt      , send_amt       "
	                 "     , recv_cnt      , recv_amt       "
	                 "     , out_cnt       , out_amt        "
	                 "     , nmnt_cnt      , reg_date       "
	                 "     , reg_time      )                "
	                 "SELECT deal_ym       , user_id        "
	                 "     , sum(send_cnt) , sum(send_amt)  "
	                 "     , sum(recv_cnt) , sum(recv_amt)  "
	                 "     , sum(out_cnt)  , sum(out_amt)   "
	                 "     , sum(nmnt_cnt) , '%s', '%s'     "
	                 "  FROM zangsi_sum.TEMP_ANALY_DEAL_YM             "
	                 " GROUP BY deal_ym, user_id            "
	                 ,greg_date
	                 ,greg_time);
	                 
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5106_insert_current: INSERT T_ANALY_DEAL_YM error...\n");
		ZzLOG(ERROR, "daem5106_insert_current: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    ret = -1;
    }
    
	//--------------------------------------------------------------------------
	// TEMPORARY 테이블을 삭제 한다.
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DROP TABLE zangsi_sum.TEMP_ANALY_DEAL_YM ");
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5106_insert_current: DROP zangsi_sum.TEMP_ANALY_DEAL_YM error...\n");
		ZzLOG(ERROR, "daem5106_insert_current: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    ret = -1;
    }
	if (ret != 0) return ret;

	//--------------------------------------------------------------------------
	// 사용자별 전체 집계
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "INSERT INTO zangsi_sum.T_ANALY_DEAL_YM   "
	                 "     ( deal_ym       , user_id       "
	                 "     , send_cnt      , send_amt      "
	                 "     , recv_cnt      , recv_amt      "
	                 "     , out_cnt       , out_amt       "
	                 "     , nmnt_cnt      , reg_date      "
	                 "     , reg_time      )               "
	                 "SELECT '000000'      , user_id       "
	                 "     , sum(send_cnt) , sum(send_amt) "
	                 "     , sum(recv_cnt) , sum(recv_amt) "
	                 "     , sum(out_cnt)  , sum(out_amt)  "
	                 "     , sum(nmnt_cnt) , '%s', '%s'    "
	                 "  FROM zangsi_sum.T_ANALY_DEAL_YM        "
	                 " WHERE deal_ym >= '200401'           "
	                 "   AND deal_ym <= '999999'           "
	                 " GROUP BY user_id                    "
	                 ,greg_date
	                 ,greg_time);
                 
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5106_insert_current: INSERT T_ANALY_DEAL_YM error...\n");
		ZzLOG(ERROR, "daem5106_insert_current: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
    }
	
	return 0;
}


/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5106_get_sysdate()
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
int daem5106_init_process(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("d_", "/logs/daemon"); 

    ZzLOG(ALWAY, "[daem5106]*****************프로그램 시작*****************\n");  
    ZzLOG(ALWAY, "[daem5106] 캐시보내기 집계처리\n");  

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
	ret=daem5106_get_sysdate();
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
int daem5106_term_process()
{
    // DB close
	db_disconnect(con);
    ZzLOG(ALWAY, "[daem5106]*****************프로그램 종료*****************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5106_signal(int nSignal)
{
    daem5106_term_process();
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
	signal(SIGTERM, daem5106_signal);
	signal(SIGINT,  daem5106_signal);
	signal(SIGQUIT, daem5106_signal);
	signal(SIGKILL, daem5106_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daem5106_init_process(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daem5106_main_process();
	
		/* 프로그램 종료루틴 */                    
		daem5106_term_process();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
