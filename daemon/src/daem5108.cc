/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5108.cc
 *         기능 : 일자별 접속자수 종합 생성(시간별)
 *         설명 : 1. 프로세스 처리 예정시간 - 매시 3분 
 *                2. 매시마다 웝접속자, 채팅접속자, 업로드, 다운로드, 시간당결재금액  
 *     설치위치 : CMD01
 *
 *       작성자 : JDP
 *       작성일 : 2006/08/25
 *     수정이력 : 2009/09/14 - 라인별 업, 다운로드, 구매건수 T_SUM_LINE_UPDN 기록
 				  2009/11/11 - T_SUM_LINE_UPDN에 서버댓수도 포함.
 					
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

int daem5108_process();
int daem5108_process_init(int argc, char **argv);
int daem5108_process_term();
void daem5108_signal(int nSignal);

MYSQL     *con;
MYSQL_RES *res;
MYSQL_ROW  row;

MYSQL	  *con_real;
MYSQL *dn_con;


char greg_date     [8+1];	// 처리일자
char greg_hour     [4+1];	// 처리시간
char greg_time     [6+1];	// 처리시간
char ginout_date   [8+1];	// 결제일자
char ginout_time_fr[6+1];	// 결제시간from
char ginout_time_to[6+1];	// 결제시간to
char ginout_hour   [4+1];	// 결제시간

//******************************************************************************
//* daem5108 main
//******************************************************************************
int daem5108_process()
{
	char szQuery[1024];		// query string
	int dn_cnt = 0;
	int up_cnt = 0;
	int web_cnt = 0;
	int chat_cnt = 0;
	int cash_amt = 0;
	int deal_cnt = 0;
	int nRowcnt = 0;
	int ret;

    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "greg_date     : [%s]\n", greg_date);  
    ZzLOG(ALWAY, "greg_time     : [%s]\n", greg_time);  
    ZzLOG(ALWAY, "ginout_date   : [%s]\n", ginout_date);  
    ZzLOG(ALWAY, "ginout_time_fr: [%s]\n", ginout_time_fr);  
    ZzLOG(ALWAY, "ginout_time_to: [%s]\n", ginout_time_to);  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");

	/*
	** 다운로드, 업로드 사용자 구하기
	*/
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery,"SELECT sum(dn_user) "
	                "     , sum(up_user) "
	                "  FROM zangsi.T_SERVER_INFO "
	                " WHERE server_gu in ('01','02') ");

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5108_process: 다운/업로드 사용자구하기 mysql_query error...\n");
	    goto daem5108_process_err;
	}
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daem5108_process: 다운/업로드 사용자구하기 mysql_store_result error...\n");
	    goto daem5108_process_err;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "daem5108_process: 다운/업로드 사용자구하기 mysql_num_rows error...\n");
	    goto daem5108_process_err;
	}
	
	row = mysql_fetch_row(res);
	
	dn_cnt = (int)getnum(row, 0);
	up_cnt = (int)getnum(row, 1);
	
	mysql_free_result(res);

	
	/*
	** 웹접속자 사용자 구하기
	*/
	
	/* -- L7 로그인 설정 기존 1시간에서 4시간으로 변경하면서 웹접속자수 로그인수로 대체
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery,"SELECT sum(if(web_conn_yn = 'Y',1,0)) "
	                "     , sum(if(conn_yn = 'Y',1,0)) "
	                "  FROM zangsi.T_USER_STAT ");
	*/
	
	
	
	
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery," select count(distinct(user_id)) from zangsi.T_LOGIN_LOG  "
	                " where login_date = '%s' and "
	                " login_time >= '%s' and login_time <= '%s' "
	                ,ginout_date
	                ,ginout_time_fr
	                ,ginout_time_to);
	
	

	if (mysql_query(dn_con, szQuery))
	{
	    ZzLOG(ERROR, "daem5108_process: 웹접속자 사용자 구하기 mysql_query error...\n");
	    goto daem5108_process_err;
	}
	if (!(res = mysql_store_result(dn_con)))
	{
	    ZzLOG(ERROR, "daem5108_process: 웹접속자 사용자 구하기 mysql_store_result error...\n");
	    goto daem5108_process_err;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "daem5108_process: 웹접속자 사용자 구하기 mysql_num_rows error...\n");
	    goto daem5108_process_err;
	}
	
	row = mysql_fetch_row(res);
	
	web_cnt  = (int)getnum(row, 0);
	
	mysql_free_result(res);
	


	/*
	** 시간당 결제금액 구하기
	*/
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery,"SELECT sum(in_amt) "
	                "  FROM zangsi.T_INOUT_AMOUNT "
	                " WHERE inout_date  = '%s' " 
	                "   AND inout_code >= '31' "
	                "   AND inout_code <= '3Z' "
	                "   AND inout_time >= '%s' "
	                "   AND inout_time <= '%s' "
	                "   AND cnl_yn      = 'N'  "
	                ,ginout_date
	                ,ginout_time_fr
	                ,ginout_time_to);

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5108_process: 시간당 결제금액 구하기 mysql_query error...\n");
	    goto daem5108_process_err;
	}
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daem5108_process: 시간당 결제금액 구하기 mysql_store_result error...\n");
	    goto daem5108_process_err;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "daem5108_process: 시간당 결제금액 구하기 mysql_num_rows error...\n");
	    goto daem5108_process_err;
		return -1;
	}
	
	row = mysql_fetch_row(res);
	
	cash_amt = (int)getnum(row, 0);

	mysql_free_result(res);


	/*
	** 시간당 거래건수 구하기
	*/
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery,"	SELECT count(deal_no)	"
					"	from zangsi.T_DEAL_INFO "
					"	WHERE deal_date = '%s'  "
					"	and deal_time >= '%s' 	"
					"	and deal_time <= '%s' 	"
	                ,ginout_date
	                ,ginout_time_fr
	                ,ginout_time_to);

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5108_process: 시간당 거래건수 구하기 mysql_query error...\n");
	    goto daem5108_process_err;
	}
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daem5108_process: 시간당 거래건수 구하기 mysql_store_result error...\n");
	    goto daem5108_process_err;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "daem5108_process: 시간당 거래건수 구하기 mysql_num_rows error...\n");
	    goto daem5108_process_err;
		return -1;
	}
	
	row = mysql_fetch_row(res);
	
	deal_cnt = (int)getnum(row, 0);

	mysql_free_result(res);

	/*
	** 구한정보 DB INSERT
	*/
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "INSERT INTO zangsi_bck.T_SUM_CONCNT (reg_date, reg_hour, reg_time, web_cnt, chat_cnt, dn_cnt, up_cnt, cash_amt, deal_cnt)"
                     " VALUES ('%s','%s','%s', 0, %d, %d, %d, 0, 0) "
                     ,greg_date
                     ,greg_hour
                     ,greg_time
                     //,web_cnt
                     ,chat_cnt
                     ,dn_cnt
                     ,up_cnt);

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5108_process: 구한정보 DB INSERT mysql_query error...\n");
	    goto daem5108_process_err;
	}
	

	/*
	** 결제정보 DB Update
	*/
	nRowcnt   = 0;
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "UPDATE zangsi_bck.T_SUM_CONCNT "
	                 "   SET web_cnt = %d, cash_amt  =  %d , deal_cnt = %d  "
	                 " WHERE reg_date  = '%s'  "
	                 "   AND reg_hour  = '%s'  "
	                 ,web_cnt
	                 ,cash_amt
	                 ,deal_cnt
	                 ,ginout_date
	                 ,ginout_hour
	                 );

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5108_process: 구한정보 DB UPDATE error...\n");
	    goto daem5108_process_err;
    }
	
	nRowcnt = mysql_affected_rows(con);
	if (nRowcnt == 0)
	{
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "INSERT INTO zangsi_bck.T_SUM_CONCNT (reg_date, reg_hour, reg_time, web_cnt, chat_cnt, dn_cnt, up_cnt, cash_amt, deal_cnt)"
	                     " VALUES ('%s','%s','%s', %d, 0, 0, 0, %d, %d) "
	                     ,ginout_date
	                     ,ginout_hour
	                     ,greg_time
	                     ,web_cnt
	                     ,cash_amt
	                     ,deal_cnt);
	
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5108_process: 구한정보 DB INSERT mysql_query error...\n");
		    goto daem5108_process_err;
		}
	}
	
	
	/*
	2009/09/14 
	라인별 업, 다운로드, 구매건수 기록
	*/
	
	//라인별 다운로드수 기록
	memset (szQuery, 0x00, sizeof(szQuery));
	strcpy(szQuery, " select b.line_name, sum(a.dn_user), count(a.server_id) "
					" from zangsi.T_SERVER_INFO a, zangsi_bck.T_SERVER_CODE b " 
					" where a.server_port = b.line_sub_code "
					" and a.server_gu = '01' and a.admin_open_yn = 'Y' "
					" group by b.line_sub_code ");
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5108_process: 라인별 다운로드수 구하기 mysql_query error...\n");
	    goto daem5108_process_err;
	}
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daem5108_process: 라인별 다운로드수 구하기 mysql_store_result error...\n");
	    goto daem5108_process_err;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "daem5108_process: 라인별 다운로드수 구하기 mysql_num_rows error...\n");
	    goto daem5108_process_err;
		return -1;
	}
	
	while(row = mysql_fetch_row(res))
	{
		char szLineName[255];
		memset(szLineName, 0x00, sizeof(szLineName));
		
		int nDnCnt = 0;
		int nSvrCnt = 0;
		
		strcpy(szLineName, getstr(row,0));
		nDnCnt = (int)getnum(row,1);
		nSvrCnt = (int)getnum(row,2);
		
		
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi_bck.T_SUM_LINE_UPDN "
						 " (reg_date, reg_hour, line_name, dn_cnt, server_cnt, reg_time) "
						 " values "
						 " ('%s', '%s', '%s', %d, %d, '%s') "
						 , greg_date, greg_hour, szLineName, nDnCnt, nSvrCnt, greg_time);

		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5108_process: 라인별 다운로드수 구하기 mysql_query error...\n");
		    goto daem5108_process_err;
		}
	}
	mysql_free_result(res);
	
	//라인별 업로드수 기록
	memset (szQuery, 0x00, sizeof(szQuery));
	strcpy(szQuery, " select b.line_name, sum(a.up_user) "
					" from zangsi.T_SERVER_INFO a, zangsi_bck.T_SERVER_CODE b " 
					" where a.server_port = b.line_sub_code "
					" and a.server_gu = '01' and a.upload_yn = 'Y' "
					" group by b.line_sub_code ");
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5108_process: 라인별 업로드수 구하기 mysql_query error...\n");
	    goto daem5108_process_err;
	}
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daem5108_process: 라인별 업로드수 구하기 mysql_store_result error...\n");
	    goto daem5108_process_err;
	}
 	if (mysql_num_rows(res) > 0)
 	{
		while(row = mysql_fetch_row(res))
		{
			char szLineName[255];
			memset(szLineName, 0x00, sizeof(szLineName));
			
			int nUpCnt = 0;
			
			strcpy(szLineName, getstr(row,0));
			nUpCnt = (int)getnum(row,1);
			
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update zangsi_bck.T_SUM_LINE_UPDN "
							 " set up_cnt = %d "
							 " where reg_date = '%s' and reg_hour = '%s' and line_name = '%s' "
							 , nUpCnt, greg_date, greg_hour, szLineName);
	
			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daem5108_process: 라인별 업로드수 구하기 mysql_query error...\n");
			    goto daem5108_process_err;
			}
		}
	}
	mysql_free_result(res);
	
	//라인별 구매건수 기록
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select d.line_name, count(distinct(a.deal_no)) "
					" from zangsi.T_DEAL_INFO a, zangsi.T_CONTENTS_FILE b " 
					" , zangsi.T_SERVER_INFO c, zangsi_bck.T_SERVER_CODE d "
					" where a.id = b.id and a.deal_date = '%s' "
					" and a.deal_time >= '%s' "
					" and a.deal_time <= '%s' "
					" and b.server_id = c.server_id and c.server_port = d.line_sub_code "
					" group by d.line_sub_code "
					, ginout_date
	                , ginout_time_fr
	                , ginout_time_to);
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5108_process: 라인별 구매건수 구하기 mysql_query error...\n");
	    goto daem5108_process_err;
	}
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daem5108_process: 라인별 구매건수 구하기 mysql_store_result error...\n");
	    goto daem5108_process_err;
	}
 	if (mysql_num_rows(res) > 0)
 	{
		while(row = mysql_fetch_row(res))
		{
			char szLineName[255];
			memset(szLineName, 0x00, sizeof(szLineName));
			
			int nDealCnt = 0;
			
			strcpy(szLineName, getstr(row,0));
			nDealCnt = (int)getnum(row,1);
			
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update zangsi_bck.T_SUM_LINE_UPDN "
							 " set deal_cnt = %d "
							 " where reg_date = '%s' and reg_hour = '%s' and line_name = '%s' "
							 , nDealCnt, ginout_date, ginout_hour, szLineName);
	
			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daem5108_process: 라인별 구매건수 구하기 mysql_query error...\n");
			    goto daem5108_process_err;
			}
			int nRowcnt = mysql_affected_rows(con);
			if(nRowcnt == 0)
			{
				memset (szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " insert into zangsi_bck.T_SUM_LINE_UPDN "
								 " (reg_date, reg_hour, line_name, deal_cnt, reg_time) "
								 " values "
								 " ('%s', '%s', '%s', %d, '%s') "
								 , ginout_date, ginout_hour, szLineName, nDealCnt, greg_time);
			
				if (mysql_query(con, szQuery))
				{
				    ZzLOG(ERROR, "daem5108_process: 구한정보 DB INSERT mysql_query error...\n");
				    goto daem5108_process_err;
				}
			}
		}
	}
	mysql_free_result(res);	
	
	return 0;

daem5108_process_err:
//	ZzLOG(ERROR, "daem5108_process: [%d](%s)\n",mysql_errno(con_real), mysql_error(con_real));
	ZzLOG(ERROR, "daem5108_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	ZzLOG(ERROR, "%s\n\n",szQuery);
	mysql_free_result(res);
	return -1;	
}


/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5108_get_sysdate()
{
	char szQuery[1000];		// query string
	char sztemp [100];      // query temp


	/*
	** 결제처리시간은 현재시간 -1시간
	** 현제시간 01:00:00 이라면 from 00:00:00 ~ 00:99:99
	*/
	memset(szQuery, 0x00, sizeof(szQuery));
	strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d')");
	strcat(szQuery, "     , date_format(now(),'%H%i%s')");
	strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -1 HOUR),'%Y%m%d')");
	strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -1 HOUR),'%H')");

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
	memset(greg_date     , 0x00, sizeof(greg_date     ));
	memset(greg_hour     , 0x00, sizeof(greg_hour     ));
	memset(greg_time     , 0x00, sizeof(greg_time     ));
	memset(ginout_date   , 0x00, sizeof(ginout_date   ));
	memset(ginout_time_fr, 0x00, sizeof(ginout_time_fr));
	memset(ginout_time_to, 0x00, sizeof(ginout_time_to));
	memset(ginout_hour   , 0x00, sizeof(ginout_hour   ));

	strcpy(greg_date ,   getstr(row, 0));
	strcpy(greg_time ,   getstr(row, 1));
	strcpy(ginout_date,  getstr(row, 2));
	strcpy(ginout_hour,  getstr(row, 3));
	sprintf(ginout_time_fr, "%s0000", ginout_hour);
	sprintf(ginout_time_to, "%s5959", ginout_hour);
	strncpy(greg_hour, greg_time, 2);
	
	mysql_free_result(res);


	return 0;
}

/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5108_process_init(int argc, char **argv)
{
	int ret=0;

    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("daem5108", "/logs/daemon"); 
    ZzLOG(ALWAY, "[daem5108]*****************프로그램 시작*****************\n");  

    // 파라미터 값 설정 및 초기화
    if (argc != 2) {
    	goto arg_error;
    }

	/*
	** DB 연결
	*/
	if (!(con=db_connect_nodb("")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
	   	return(-1); 
	}
	
	if (!(dn_con=db_connect_logdb("zangsi")))
	{
		ZzLOG(ERROR, "zangsi DN DB에 접속하지 못 하였습니다...\n");
	   	return(-1); 
	}
		

/*	if (!(con_real=db_connect("zangsi")))
	{
		ZzLOG(ERROR, "zangsi_bck DB에 접속하지 못 하였습니다...\n");
	   	return(-1); 
	}
*/		

	/*
	** 처리일자
	*/
	ret=daem5108_get_sysdate();
	if (ret < 0){
		db_disconnect(con);
//		db_disconnect(con_real);
		
		return -1;
	}
		
    return (0);

arg_error:
    ZzLOG(ERROR, "usage : %s YYYYMMDD\n", argv[0]);
    ZzLOG(ERROR, "        YYYYMMDD : 처리일자 (00000000:시스템일자)\n");

    ZzPRT(ERROR, "usage : %s YYYYMMDD\n", argv[0]);
    ZzPRT(ERROR, "        YYYYMMDD : 처리일자 (00000000:시스템일자)\n");
    return -1;
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daem5108_process_term()
{
    // DB close
	db_disconnect(con);
	db_disconnect(dn_con);	
	
	
    ZzLOG(ALWAY, "[daem5108]*****************프로그램 종료*****************\n");  

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5108_signal(int nSignal)
{
    daem5108_process_term();
}

/*****************************************************************************
*  프로그램 메인 
*****************************************************************************/
int main(int argc, char **argv)
{                
	int     rc;
                 
	/*       
	** SIGNAL 정의
	*/       
	signal(SIGTERM, daem5108_signal);
	signal(SIGINT,  daem5108_signal);
	signal(SIGQUIT, daem5108_signal);
	signal(SIGKILL, daem5108_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daem5108_process_init(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daem5108_process();
		/* 프로그램 종료루틴 */                    
		daem5108_process_term();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
