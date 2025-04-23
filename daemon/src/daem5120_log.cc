/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5120_log.cc
 *         기능 : 재로그인 통계정보 생성 (zangsi_sum.T_STAT_TOT)
 *         설명 : 
 *       작성자 : HCS
 *       작성일 : 
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
#include <math.h>

#include "daemcom.h"
#include "commydb.h"

#define  MAX_ROWS	1
#define _DEBUG_

int daem5120_log_init_process(int argc, char **argv); 

int daem5120_log_main_process(); 
int daem5120_log_term_process(); 

int daem5120_log_get_login_info();

int daem5120_log_get_sysdate(); 
void daem5120_log_signal(int nSignal); 

MYSQL     *con;

char   gsys_date  [  8+1];	//처리일자(sysdate)
char   gproc_date [  8+1];	//처리일자(sysdate-1)
char   greg_date  [  8+1];	//등록일
char   greg_time  [  6+1];	//등록시간
//******************************************************************************
//* daem5120_log main
//******************************************************************************
int daem5120_log_main_process()
{
	daem5120_log_get_login_info();
	return 0;
}



//******************************************************************************
//* daem5120_log_get_login_info()
//* 재로그인 정보 수집
//******************************************************************************
int daem5120_log_get_login_info()
{
	MYSQL_RES *res; 
	MYSQL_ROW  row; 

	char szQuery[10000];		// query string
	int n15 = 0 ;
	int n16_30 = 0;
	int n31_45 = 0;
	int n46_60 = 0;
	int n61 = 0;
	
	//--------------------------------------------------------------------------
	// 하루동안 방문자 조회
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select user_id from zangsi.T_LOGIN_LOG where login_date = '%s' " 
					 " group by user_id "
					 , gproc_date);
					 
	ZzLOG(ALWAY, "daem5120_log_get_login_info : %s\n\n",szQuery);
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5120_log_get_login_info: mysql_query error. [%d](%s)\n",mysql_errno(con), mysql_error(con));
	   	return -1; 
	}
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daem5120_log_get_login_info: mysql_store_result error. [%d](%s)",mysql_errno(con), mysql_error(con));
	   	return -1; 
	}
	int nRowCnt = 0;
	nRowCnt = mysql_num_rows(res);
 	if (nRowCnt == 0)
 	{
	    ZzLOG(ALWAY, "daem5120_log_get_login_info: %s에 대한 로그인 정보 없음.\n", gproc_date);
		mysql_free_result(res);
	   	return -1; 
	}
		
	MYSQL_RES *log_res; 
	MYSQL_ROW  log_row; 
	
	char szUserId[16+1];

	int nProcCnt = 0;
	
	ZzLOG(ALWAY, "daem5120_log_get_login_info: 처리할 회원수 %d명\n", nRowCnt);
	
	int nPer = 0;
	int nPrePer = 0;
	
	while(row = mysql_fetch_row(res))
	{
		memset(szUserId, 0x00, sizeof(szUserId));			
		sprintf(szUserId, "%s", getstr(row,0));
		
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " select "
						 " to_days(date_format('%s', '%%Y%%m%%d')) - to_days(date_format(max(login_date) , '%%Y%%m%%d')) "
						 " from zangsi.T_LOGIN_LOG where user_id = '%s' and login_date < '%s' " 
						, gproc_date, szUserId, gproc_date);
				
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5120_log_get_login_info: mysql_query error. [%d](%s)\n",mysql_errno(con), mysql_error(con));
		   	return -1; 
		}
		if (!(log_res = mysql_store_result(con)))
		{
		    ZzLOG(ERROR, "daem5120_log_get_login_info: mysql_store_result error. [%d](%s)",mysql_errno(con), mysql_error(con));
		   	return -1; 
		}
	 	if (mysql_num_rows(log_res)==0)
	 	{
	 		ZzLOG(ERROR, "daem5120_log_get_login_info: 로그인 내역 없음.[%s]\n", szQuery);
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
			
		nProcCnt++;
		
		double dPer = ((double)nProcCnt / (double)nRowCnt) * 100;
		
		nPrePer = nPer;
		
		nPer = 0;
		nPer = (int)dPer;
		
		double dDivRes = (double)nPer / 10;
		
		double dModRes = 0, dModReal = 0;
		
		dModRes = modf(dDivRes, &dModReal);
		
		if(dModRes == 0 && nPer != nPrePer)
			ZzLOG(ALWAY, "daem5120_log_get_login_info: %d 퍼센트 완료.(%d건 처리 완료)\n", nPer, nProcCnt);
		
	}
	mysql_free_result(res);
	
	ZzLOG(ALWAY, "daem5120_log_get_login_info: 처리한 회원수 %d명\n", nProcCnt);
	
	sprintf(szQuery, " INSERT INTO zangsi.T_RE_LOGIN_STAT "
					 " (stat_date, re_login_term_15, re_login_term_30, re_login_term_45, re_login_term_60, re_login_term_end, reg_date, reg_time) "
					 " VALUES "
					 " ('%s', %d, %d, %d, %d, %d, '%s', '%s') "
					 ,  gproc_date, n15, n16_30, n31_45, n46_60, n61, greg_date, greg_time);

	ZzLOG(ALWAY, "daem5120_log_get_login_info : %s\n\n",szQuery);

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5120_log_get_login_info : mysql_query error. [%d](%s)\n",mysql_errno(con), mysql_error(con));
	   	return -1; 
	}	
	
	return 0;	
	
}


/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5120_log_get_sysdate()
{
	MYSQL_RES *res; 
	MYSQL_ROW  row; 

	char szQuery[1000];		// query string

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
	    ZzLOG(ERROR, "sysdate: mysql_query error. [%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "sysdate: mysql_store_result error. %d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "sysdate: mysql_num_rows error. [%d](%s)",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
	
	row = mysql_fetch_row(res);
	memset(greg_date , 0x00, sizeof(greg_date ));
	memset(greg_time , 0x00, sizeof(greg_time ));
	memset(gproc_date, 0x00, sizeof(gproc_date));
	
	if (strcmp(gsys_date, "00000000")==0)
		strcpy(greg_date ,   getstr(row, 0));
	else
		strcpy(greg_date ,  gsys_date);
	
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
int daem5120_log_init_process(int argc, char **argv)
{
    /*
    ** 전역변수 초기화
    */
    
    #ifdef __DEBUG
    ZzInitGlobalVariable2("daem5120_log", "/home/ezwon/zangsi_with_dcmd/daemon/log"); 
    #else
    ZzInitGlobalVariable2("d_", "/logs/daemon"); 
    #endif
    
    ZzLOG(ALWAY, "[daem5120_log]*****************프로그램 시작*****************\n");  
    

    // 파라미터 값 설정 및 초기화
    if (argc != 2)
    	goto arg_error;

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(con=db_connect_logdb("zangsi")))
	{
		ZzLOG(ERROR, "logDB에 접속하지 못 하였습니다...\n");
	   	return -1; 
	}

	/* 처리일자 */
	memset(gsys_date, 0x00, sizeof(gsys_date));
	strcpy(gsys_date, argv[1]);
	
	if(daem5120_log_get_sysdate() < 0)
		return -1;

	
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
int daem5120_log_term_process()
{
    // DB close
	db_disconnect(con);
    ZzLOG(ALWAY, "[daem5120_log]*****************프로그램 종료*****************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5120_log_signal(int nSignal)
{
    daem5120_log_term_process();
}

/*****************************************************************************
*  프로그램 메인 
*****************************************************************************/
int main(int argc, char **argv)
{                
	/*       
	** SIGNAL 정의
	*/       
	signal(SIGTERM, daem5120_log_signal);
	signal(SIGINT,  daem5120_log_signal);
	signal(SIGQUIT, daem5120_log_signal);
	signal(SIGKILL, daem5120_log_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if( daem5120_log_init_process(argc, argv) == 0 )
	{
		/* 프로그램 메인루틴 */
		
		daem5120_log_main_process();

		/* 프로그램 종료루틴 */                    
		daem5120_log_term_process();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
