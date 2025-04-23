/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5122.cc
 *         기능 : 관리자 상담글 통계
 *         설명 : 1. 프로세스 처리 예정시간 : 07:00 
 *     설치위치 : cmdsvr에 위치한다.
 *
 *       작성자 : HCS
 *       작성일 : 2007/11/19
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

#include "daemcom.h"
#include "commydb.h"

int daem5122_process();
int daem5122_process_db_term();
int daem5122_process_db_score();

int daem5122_process_init(int argc, char **argv);
int daem5122_process_end();
void daem5122_signal(int nSignal);

MYSQL     *con;
MYSQL_RES *res;
MYSQL_RES *res2;
MYSQL_ROW  row;
MYSQL_ROW  row2;

char gst_date[8+1];	// 처리일자

//******************************************************************************
//* daem5122 main
//******************************************************************************

int daem5122_process()
{
	char szQuery[1000];  // query string
	memset(szQuery, 0x00, sizeof(szQuery));	

	// 처리일자가 "00000000"일때는 시스템일자 -1을 한다.
	if (strcmp(gst_date, "00000000") == 0)
	{
		
		strcpy(szQuery, "SELECT date_format(date_add( now(),INTERVAL - 1 DAY ),'%Y%m%d') ");
		// 현재로부터 1일 이전 날짜를 얻는다.

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
			return -1;
		}
		row = mysql_fetch_row(res);
		memset(gst_date, 0x00, sizeof(gst_date));
		strcpy(gst_date,   getstr(row, 0));
		mysql_free_result(res);

		if( daem5122_process_db_score() !=0)
		{
			return -1;
		}	
		if( daem5122_process_db_term() !=0 )
		{
			return -1;
		}
		
	}			
	else
	{
		if( daem5122_process_db_score() !=0)
		{
			return -1;
		}	

		if( daem5122_process_db_term() !=0)
		{
			return -1;
		}	
	}

	#ifdef __DEBUG
	printf("적용 날짜 : %s\n",gst_date);
	#endif 
	
	

	
	return 0;
}

//******************************************************************************
//* daem5122 db 처리로직
//******************************************************************************
int daem5122_process_db_term()
{
    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "---------------daem5122_process_db_term-------------------\n");  
    ZzLOG(ALWAY, "gproc_date : [%s]\n", gst_date);  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");

	char szQuery[3000];		// query string

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/
	
	memset (szQuery, 0x00, sizeof(szQuery));
	
	sprintf(szQuery," insert into zangsi_sum.T_CONSULT_TERM_STAT ( "
					" reg_date, "
					" lcode_all_time, "
					" lcode_001_time, "
					" lcode_002_time, "
					" lcode_003_time, "
					" lcode_004_time, "
					" lcode_005_time, "
					" lcode_006_time, "
					" lcode_007_time, "
					" lcode_008_time, "
					" lcode_009_time, "
					" lcode_010_time) "
				    " select "
				    " '%s' , "
				    " avg(unix_timestamp(concat(d.reg_date, d.reg_time)) - unix_timestamp(concat(a.reg_date, a.reg_time))) as lcode_all_time, "
				    " ifnull(	avg( case when a.lcode='001' then unix_timestamp(concat(d.reg_date, d.reg_time)) - unix_timestamp(concat(a.reg_date, a.reg_time)) end ), 0 )as lcode_001_time, "
				    " ifnull(	avg( case when a.lcode='002' then unix_timestamp(concat(d.reg_date, d.reg_time)) - unix_timestamp(concat(a.reg_date, a.reg_time)) end ), 0 )as lcode_002_time, "
				    " ifnull(	avg( case when a.lcode='003' then unix_timestamp(concat(d.reg_date, d.reg_time)) - unix_timestamp(concat(a.reg_date, a.reg_time)) end ), 0 )as lcode_003_time, "
				    " ifnull(	avg( case when a.lcode='004' then unix_timestamp(concat(d.reg_date, d.reg_time)) - unix_timestamp(concat(a.reg_date, a.reg_time)) end ), 0 )as lcode_004_time, "
				    " ifnull(	avg( case when a.lcode='005' then unix_timestamp(concat(d.reg_date, d.reg_time)) - unix_timestamp(concat(a.reg_date, a.reg_time)) end ), 0 )as lcode_005_time, "
				    " ifnull(	avg( case when a.lcode='006' then unix_timestamp(concat(d.reg_date, d.reg_time)) - unix_timestamp(concat(a.reg_date, a.reg_time)) end ), 0 )as lcode_006_time, "
				    " ifnull(	avg( case when a.lcode='007' then unix_timestamp(concat(d.reg_date, d.reg_time)) - unix_timestamp(concat(a.reg_date, a.reg_time)) end ), 0 )as lcode_007_time, "
				    " ifnull(	avg( case when a.lcode='008' then unix_timestamp(concat(d.reg_date, d.reg_time)) - unix_timestamp(concat(a.reg_date, a.reg_time)) end ), 0 )as lcode_008_time, "
				    " ifnull(	avg( case when a.lcode='009' then unix_timestamp(concat(d.reg_date, d.reg_time)) - unix_timestamp(concat(a.reg_date, a.reg_time)) end ), 0 )as lcode_009_time, "
				    " ifnull(	avg( case when a.lcode='010' then unix_timestamp(concat(d.reg_date, d.reg_time)) - unix_timestamp(concat(a.reg_date, a.reg_time)) end ), 0 )as lcode_010_time "
				    " from zangsi.T_CONSULT_MAIN a, zangsi.T_CONSULT_MAIN d "			 
				    " where	a.id = d.quest_id and d.quan_yn = 'A' and d.reg_date = '%s' group by d.reg_date	"
				   ,gst_date ,gst_date );

	ZzLOG(ALWAY, "%s\n\n",szQuery);
	
	#ifdef __DEBUG
	printf("%s\n\n",szQuery);
	#endif
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5122_process_db_score: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}	

	memset (szQuery, 0x00, sizeof(szQuery));
	
	sprintf(szQuery," update "
				    " zangsi_sum.T_CONSULT_TERM_STAT "			 
				    " set lcode_all_time = (lcode_001_time+lcode_002_time+lcode_003_time " 
                    " +lcode_004_time+lcode_005_time+lcode_006_time+lcode_007_time "																																																			
                    " +lcode_008_time+lcode_009_time+lcode_010_time ) /10 "																																																			
				    " where reg_date = '%s' "
				   ,gst_date ,gst_date );

	ZzLOG(ALWAY, "%s\n\n",szQuery);
	
	#ifdef __DEBUG
	printf("%s\n\n",szQuery);
	#endif
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5122_process_db_score: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}	
		
		
    ZzLOG(ALWAY, "---------------daem5122_process_db_term end-------------------\n");  

	return 0;
}

int daem5122_process_db_score()
{
    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "---------------daem5122_process_db_score-------------------\n");  
    ZzLOG(ALWAY, "gproc_date : [%s]\n", gst_date);  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");

	char szQuery[1600];		// query string

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/
	
	memset (szQuery, 0x00, sizeof(szQuery));
	
	sprintf(szQuery," insert into zangsi_sum.T_CONSULT_SCORE_STAT (reg_date, user_id, score)		"
				   " select '%s' , if(select_admin = '' or select_admin is NULL, '운영팀', select_admin), sum(score) as score				"
				   " from zangsi.T_CONSULT_MAIN "			 
				   " where quan_yn = 'A' and score_reg_date = '%s' and score is not null and score <> '' group by 2	 "
				   ,gst_date ,gst_date );

	ZzLOG(ALWAY, "%s\n\n",szQuery);
	
	#ifdef __DEBUG
	printf("%s\n\n",szQuery);
	#endif
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5122_process_db_score: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}	
    ZzLOG(ALWAY, "---------------daem5122_process_db_score end-------------------\n");  



	return 0;
}


/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5122_process_init(int argc, char **argv)
{
	char stemp[128];
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("d_", "/logs/daemon"); 

    ZzLOG(ALWAY, "[daem5122]*****************프로그램 시작*****************\n");  

    // 파라미터 값 설정 및 초기화
    if (argc != 2) 
    {
	    ZzLOG(ERROR, "usage : 00000000 or date\n");
	    return -1;
    }

	/* 처리일자 */
	memset(gst_date, 0x00, sizeof(gst_date));
	strcpy(gst_date, argv[1]);

	
	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(con=db_connect_nodb("")))
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
int daem5122_process_end()
{
    // DB close
    
	db_disconnect(con);
	
    ZzLOG(ALWAY, "[daem5122]*****************프로그램 종료*****************\n");  

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5122_signal(int nSignal)
{
    daem5122_process_end();
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
	signal(SIGTERM, daem5122_signal);
	signal(SIGINT,  daem5122_signal);
	signal(SIGQUIT, daem5122_signal);
	signal(SIGKILL, daem5122_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daem5122_process_init(argc, argv) == 0 ) 
	{
		/* 프로그램 메인루틴 */
		rc = daem5122_process();
		
		/* 프로그램 종료루틴 */                    
		daem5122_process_end();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
