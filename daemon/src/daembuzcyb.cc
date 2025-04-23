/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daembuzcyb.cc
 *         기능 : T_BUZ_CYB_DD 다시 계산
 *         설명 : 
 *       작성자 : HCS
 *       작성일 : 2009/10/12
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
#include <unistd.h> //for usleep();

#include "daemcom.h"
#include "commydb.h"

#define  MAX_ROWS	1
#define _DEBUG_


int daembuzcyb_init_process(int argc, char **argv);
int daembuzcyb_main_process();
int daembuzcyb_insert(char* pIoDate);
int daembuzcyb_term_process();
void daembuzcyb_signal(int nSignal);

MYSQL     *con;

//******************************************************************************
//* daembuzcyb main
//******************************************************************************
int daembuzcyb_main_process()
{
    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "daembuzcyb_main_process START !! \n");  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    
	MYSQL_RES *res;
	MYSQL_ROW  row;


    char szQuery[1000];
    memset(szQuery, 0x00, sizeof(szQuery));

    int nCount = 0;

	strcpy(szQuery, " select cmt_date "
					" from zangsi_sum.T_COMIS_DD "
					" group by cmt_date order by cmt_date ");

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daembuzcyb_main_process: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
	
	if (!(res = mysql_store_result(con)))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daembuzcyb_main_process: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ALWAY, "daembuzcyb_main_process: 처리할 자료가 없습니다.\n");
		mysql_free_result(res);
		return 0;
	}
	
	while(row = mysql_fetch_row(res))
	{
		if(daembuzcyb_insert(getstr(row,0)) < 0)
		{
			break;
		}
		nCount++;
		ZzLOG(ALWAY,"nCount = [%d],  szIoDate=[%s]\n", nCount, getstr(row,0));
	}
	mysql_free_result(res);

	
	ZzLOG(ALWAY, "daembuzcyb_main_process: [%d]건 성공\n", nCount);
	return 0;
}

int daembuzcyb_insert(char* pIoDate)
{
	MYSQL_RES *res;
	MYSQL_ROW  row;

    char szQuery[10000];
    memset(szQuery, 0x00, sizeof(szQuery));

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
	tot_amt  = 0;
	//--------------------------------------------------------------------------
	// 전일잔액을 구한다.
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT tot_amt             "
	                 "  FROM zangsi_sum.T_BUZ_CYB_DD "
                     " WHERE io_date = date_format(date_add('%s',"
	                 ,pIoDate
	                 );
	strcat (szQuery, "INTERVAL -1 DAY),'%Y%m%d')");

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daembuzcyb_insert: 전일잔액 SELECT mysql_query error...\n");
		ZzLOG(ERROR, "daembuzcyb_insert: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) 
	{
	    ZzLOG(ERROR, "daembuzcyb_insert: 전일잔액 SELECT mysql_store_result error...\n");
		ZzLOG(ERROR, "daembuzcyb_insert: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)!=0)	
 	{
		if (row = mysql_fetch_row(res))	
		{
			bef_amt = getnum(row, 0);
		}
	}
	mysql_free_result(res);
	
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
					 "           WHEN '3Z' THEN io_amt" 
	                 "           ELSE 0"
	                 "           END),0) as '충전' " // +
	                 "     , IFNULL( "
	                 "       SUM(CASE cmt_code"
	                 "           WHEN '11' THEN io_amt" // 충전
	                 "           WHEN '12' THEN io_amt" // 판매(위디스크)
	                 "           WHEN '13' THEN io_amt" // 판매(다운로드)
	                 "           WHEN '16' THEN io_amt" // 판매(자료실)
	                 "           WHEN '19' THEN io_amt" // 캐시지급(불량컨텐츠보상)
	                 "           WHEN '30' THEN io_amt" // 정액제 취소
	                 "           WHEN '46' THEN io_amt" // 디스크 구매취소
	                 "           WHEN '47' THEN io_amt" // 디스크 기간연장취소
	                 "           WHEN '63' THEN io_amt" // 당첨금(보물찾기)
	                 "           WHEN '1A' THEN io_amt" // 판매(필로그자료실)
	                 "           WHEN '1C' THEN io_amt" // 이벤트캐시
	                 "           WHEN '1Z' THEN io_amt" // 아이템환불
	                 "           WHEN '14' THEN io_amt" // 캐시받기(용도파악안됨. 제외)
	                 "           WHEN '15' THEN io_amt" // 판매(정액제)
	                 "           ELSE 0"
	                 "           END),0) as '캐시입금' " // +
	                 "     , IFNULL( "
	                 "       SUM(CASE cmt_code"
	                 "           WHEN '21' THEN io_amt" // 출금
	                 "           WHEN '29' THEN io_amt" // 자료실 판매금(예전코드)
	                 "           WHEN '41' THEN io_amt" // 캐시환수(불량컨텐츠수익금회수)
	                 "           WHEN '42' THEN io_amt" // 아이템 구매
	                 "           WHEN '43' THEN io_amt" // 당첨금 지급(보물찾기)
	                 "           WHEN '99' THEN io_amt" // 출금(띠앗)
	                 "           WHEN '1B' THEN io_amt"  // 기간연장(필로그자료실)
	                 "           WHEN '2A' THEN io_amt" // 구매(필로그자료실)
	                 "           WHEN '2B' THEN io_amt" // 구매(VOD)
	                 "           WHEN '2C' THEN io_amt" // 구매(제휴)
	                 "           WHEN '24' THEN io_amt" // 자율캐시(용도파악안됨)
//	                 "           WHEN '18' THEN io_amt" // 이벤트(용도파악안됨)
	                 "           WHEN '51' THEN io_amt" // 결제취소
	                 "           WHEN '52' THEN io_amt" // 결제취소
	                 "           WHEN '54' THEN io_amt" // 결제취소
	                 "           WHEN '55' THEN io_amt" // 결제취소
	                 "           WHEN '57' THEN io_amt" // 결제취소
	                 "           WHEN '58' THEN io_amt" // 결제취소
	                 "           ELSE 0"
	                 "           END),0) as '수수료' " // -
	                 "     , IFNULL("
	                 "       SUM(CASE cmt_code"
	                 "           WHEN '25' THEN io_amt" // 문자메시지
	                 "           WHEN '26' THEN io_amt" // 자료실용량구매
	                 "           WHEN '27' THEN io_amt" // 자료실기간연장
	                 "           WHEN '28' THEN io_amt" // 정액제신청
	                 "           ELSE 0"
	                 "           END),0) as '매출' " // -
	                 "  FROM zangsi_sum.T_COMIS_DD     "
	                 " WHERE cmt_date = '%s'       "
	                 ,pIoDate
	                 );
	                 

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daembuzcyb_insert: 수수료 SELECT mysql_query error...\n");
		ZzLOG(ERROR, "daembuzcyb_insert: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) 
	{
	    ZzLOG(ERROR, "daembuzcyb_insert: 수수료 SELECT mysql_store_result error...\n");
		ZzLOG(ERROR, "daembuzcyb_insert: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)!=0)	
 	{
		if (row = mysql_fetch_row(res))	
		{
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
	tot_amt = bef_amt + inc_amt + cash_amt - (cmt_amt + sale_amt);

/*
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
	                 ,pIoDate
	                 ,bef_amt
	                 ,inc_amt
	                 ,cash_amt
	                 ,cmt_amt
	                 ,sale_amt
	                 ,out_amt
	                 ,tot_amt
	                 );
*/

	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " update zangsi_sum.T_BUZ_CYB_DD "
	                 " set bef_amt = %12.0f "
	                 ", inc_amt = %12.0f, cash_amt = %12.0f "
	                 ", cmt_amt = %12.0f, sale_amt = %12.0f "
	                 ", out_amt = %12.0f, tot_amt = %12.0f "
	                 " where io_date = '%s'"
	                 ,bef_amt
	                 ,inc_amt
	                 ,cash_amt
	                 ,cmt_amt
	                 ,sale_amt
	                 ,out_amt
	                 ,tot_amt
	                 ,pIoDate
	                 );

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daembuzcyb_insert: mysql_query error...\n");
		ZzLOG(ERROR, "daembuzcyb_insert: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}

	return 0;	
}





/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daembuzcyb_init_process(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
#ifdef __DEBUG
    ZzInitGlobalVariable2("daembuzcyb", "/home/ezwon/zangsi_with_dcmd/daemon/log"); 

#else
    ZzInitGlobalVariable2("daembuzcyb", "/logs/daemon"); 
#endif    

    ZzLOG(ALWAY, "[daembuzcyb]*****************프로그램 시작*****************\n");  

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(con=db_connect_nodb("zangsi_sum")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1); 
	}
	
    return (0);
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daembuzcyb_term_process()
{
    // DB close
	db_disconnect(con);
	
    ZzLOG(ALWAY, "[daembuzcyb]*****************프로그램 종료*****************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daembuzcyb_signal(int nSignal)
{
    daembuzcyb_term_process();
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
	signal(SIGTERM, daembuzcyb_signal);
	signal(SIGINT,  daembuzcyb_signal);
	signal(SIGQUIT, daembuzcyb_signal);
	signal(SIGKILL, daembuzcyb_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daembuzcyb_init_process(argc, argv) == 0 ) 
	{
		/* 프로그램 메인루틴 */
		rc = daembuzcyb_main_process();
		/* 프로그램 종료루틴 */                    
		daembuzcyb_term_process();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
