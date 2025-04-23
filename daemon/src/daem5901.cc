/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5901.cc
 *         기능 : 비정상 결제자 추출
 *         설명 :  출금 집계한다
 *       작성자 : LEE
 *       작성일 : 2007/07/19
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

int daem5901_init_process(int argc, char **argv);
int daem5901_main_process();
int daem5901_term_process();
int daem5901_insert_cracker();
int daem5901_get_sysdate();
void daem5901_signal(int nSignal);

MYSQL     *con;
MYSQL_RES *res;
MYSQL_ROW  row;


char   gsys_date  [  8+1];	//처리일자(sysdate)
char   gproc_date [  8+1];	//처리일자(sysdate-1)
char   gproc_yymm [  6+1];	//처리년월
char   greg_date  [  8+1];	//등록일
char   greg_time  [  6+1];	//등록시간
//******************************************************************************
//* daem5901 main
//******************************************************************************
int daem5901_main_process()
{
	//--------------------------------------------------------------------------
	// 트렌젝션시작
	//--------------------------------------------------------------------------
/*	if (tran_begin(con)!=0) {
	    ZzLOG(ERROR, "tran_begin: 테이베이스 오류입니다.\n");  
		ZzLOG(ERROR, "daem5901_main_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
	}
*/
	// 판매 집계처리
	if (daem5901_insert_cracker() != 0)
		return 1; //goto daem5901_main_process_err;

/*	if (tran_commit(con)!=0){
	    ZzLOG(ERROR, "daem5901_main_process: tran_commit error...\n");
		ZzLOG(ERROR, "daem5901_main_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    goto daem5901_main_process_err;
	}
*/
	return (0);
/*
daem5901_main_process_err:
	tran_rollback(con);
    return -1;
    
*/  
}



//******************************************************************************
//* daem5901_insert_login()
//* 일일 사용자별 거래집계처리
//******************************************************************************
int daem5901_insert_cracker()
{
	char szQuery[10000];		// query string
	int ret=0;
	
	

    
	//--------------------------------------------------------------------------
	// 해당월 월집계
	//--------------------------------------------------------------------------
	
	
	ret = 0;
	memset (szQuery, 0x00, sizeof(szQuery));

	//충전(띠앗)추가 3I.20080828 - HCS
	//충전(T_cash, 틴캐쉬) 추가 3G, 3J. 20090512 - HCS
	sprintf(szQuery, " REPLACE INTO zangsi_sum.T_CRACKER_INFO " // user_id , acct_cd , reg_date 
	                 " SELECT  b.user_id  ,  a.minor_code ,'%s' ,0       "
	                 " from zangsi.T_MINOR_CODE a , zangsi.T_INOUT_AMOUNT b "
	                 " where a.major_code = '03' "
	                 " and a.minor_code > '30' and a.minor_code <='3Z' "
					 " and a.minor_code = b.inout_code "
					 " and b.inout_date = '%s' "
					 " and  b.r_in_amt !=  round(b.in_amt*10/11,-1)    "
					 " group by b.user_id ,b.inout_code ; /* daem5901 */"
	                 , gproc_date 
	                 , gproc_date);
	   
	                 	             
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5901_insert_current: [ %s ]  error...\n",szQuery);
		ZzLOG(ERROR, "daem5901_insert_current: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    ret = -1;
    }
    
    ZzLOG(ALWAY, "Query :  %s   \n\n",szQuery);
    sprintf(szQuery,
	" REPLACE INTO zangsi_sum.T_CRACKER_INFO "
	" SELECT  b.user_id  ,  a.minor_code ,'%s' ,id       "
     " from zangsi.T_MINOR_CODE a , zangsi.T_ACCT_SOFT b "
     " where a.major_code = '03' "
     " and a.minor_code > '30' and a.minor_code <='3Z' "
	 " and a.minor_code = b.acct_cd "
	 " and b.reg_date = '%s' "
	 " and b.charge not in (24200, 72600, 145200, 2200, 3300, 5500, 11000, 22000, 33000, 55000 ) "
	 " and b.acct_cd not in ('37', '38', '3E') and  ( b.real_date >= '20070721' and b.reg_time >='153000' ) ; /* daem5901 */ "
     , gproc_date 
     , gproc_date);					 

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "daem5901_insert_current: [ %s ]  error...\n",szQuery);
		ZzLOG(ERROR, "daem5901_insert_current: [%d](%s)\n\n\n\n",mysql_errno(con), mysql_error(con));
		ret = -1;
	}
    
    ZzLOG(ALWAY, "Query :  %s   \n\n",szQuery);

    sprintf(szQuery,
	" REPLACE INTO zangsi_sum.T_CRACKER_INFO "
	" SELECT  b.user_id  ,  a.minor_code ,'%s'   ,id     "
     " from zangsi.T_MINOR_CODE a , zangsi.T_ACCT_SOFT b "
     " where a.major_code = '03' "
     " and a.minor_code > '30' and a.minor_code <='3Z' "
	 " and a.minor_code = b.acct_cd "
	 " and b.reg_date = '%s' "
	 " and b.charge not in (24200, 72600, 145200, 2000, 3000, 5000, 10000, 20000, 30000, 50000)"
	 " and b.acct_cd  in ('37', '38', '3E') ; /* daem5901 */ "
     , gproc_date 
     , gproc_date);					 

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5901_insert_current: [ %s ]  error...\n",szQuery);
		ZzLOG(ERROR, "daem5901_insert_current: [%d](%s)\n\n\n\n",mysql_errno(con), mysql_error(con));
	    ret = -1;
    }
    
    ZzLOG(ALWAY, "Query :  %s   \n\n",szQuery);
		
	
		
	return 0;
}


/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5901_get_sysdate()
{
	char szQuery[1000];		// query string
	char sztemp [100];      // query temp

	memset(szQuery, 0x00, sizeof(szQuery));
	if (strcmp(gsys_date, "00000000")==0)
	{
		strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d')");
		strcat(szQuery, "     , date_format(now(),'%H%i%s')");
		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -1 DAY),'%Y%m%d')");
		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -1 DAY),'%Y%m') ; /* daem5901 */ ");
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
		strcat(szQuery, "' ; /* daem5901 */ ");
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

	ZzLOG(ERROR, "검색일자 : ( %s ) \n",gproc_date);
	    


	return 0;
}

/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5901_init_process(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("d_", "/logs/daemon"); 

    ZzLOG(ALWAY, "[daem5901]*****************프로그램 시작*****************\n");  
    ZzLOG(ALWAY, "[daem5901] 악성 사용자 추출\n");  

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
	ret=daem5901_get_sysdate();
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
int daem5901_term_process()
{
    // DB close
	db_disconnect(con);
    ZzLOG(ALWAY, "[daem5901]*****************프로그램 종료*****************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5901_signal(int nSignal)
{
    daem5901_term_process();
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
	signal(SIGTERM, daem5901_signal);
	signal(SIGINT,  daem5901_signal);
	signal(SIGQUIT, daem5901_signal);
	signal(SIGKILL, daem5901_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daem5901_init_process(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daem5901_main_process();
	
		/* 프로그램 종료루틴 */                    
		daem5901_term_process();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
