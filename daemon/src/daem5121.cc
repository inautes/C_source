/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5121.cc
 *         기능 : 출금 정보 집계
 *         설명 : 1.프로세스 처리 예정시간 - 09:00 
 *                2.총 출금 건수(tot_out_cnt), 총 출금액(tot_out_cmt), 한달간 총 출금액(mon_out_cmt) 생성(zangsi.T_PERM_UPLOAD_AUTH)
 *     설치위치 : CMD01
 *
 *       작성자 : HCS
 *       작성일 : 2007/08/30
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

int daem5121_process();
int daem5121_process_db_all();
int daem5121_process_db_day();

int daem5121_process_init(int argc, char **argv);
int daem5121_process_term();
void daem5121_signal(int nSignal);

MYSQL     *con;
MYSQL_RES *res;
MYSQL_RES *res2;
MYSQL_ROW  row;
MYSQL_ROW  row2;

char gst_date[8+1];	// 처리일자

//******************************************************************************
//* daem5121 main
//******************************************************************************

int daem5121_process()
{
	char szQuery[1000];  // query string
	memset(szQuery, 0x00, sizeof(szQuery));	

	// 처리일자가 "00000000"일때는 시스템일자 -3을 한다.
	if (strcmp(gst_date, "00000000") == 0)
	{
		
		strcpy(szQuery, "SELECT date_format(date_add( now(),INTERVAL - 1 DAY ),'%Y%m%d')  ");
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
				
		if( daem5121_process_db_day() !=0)
		{
			return -1;
		}	
	}
	else if(strcmp(gst_date, "all") == 0)

	{
		if( daem5121_process_db_all() !=0)
		{
			return -1;
		}	
		return 0;
	}
	else
	{
	    ZzPRT(ERROR, "usage : 00000000 or all\n");
		return -1;
	}
	
	ZzLOG(ALWAY,"적용 날짜 : %s\n",gst_date);
	
	

	
	return 0;
}

//******************************************************************************
//* daem5121 db 처리로직
//******************************************************************************
int daem5121_process_db_all()
{
	
	/*
	데몬은 하루에 한번!!
	
	전체 데이터로 돌아가는것 하나..
	어제 데이터읽어와서 돌아가는것 하나..
	
	T_OUT_AMOUNT에서 어제 날짜의 user_id, count(*) sum(out_cmt)를 읽어와 
	T_PERM_UPLOAD_AUTH의 tot_out_cnt, tot_out_cmt의 값에 합산하여 업데이트

	T_OUT_AMOUNT에서 한달동안의 출금액을 읽어와 
	T_PERM_UPLOAD_AUTH의 mon_out_cmt의 값에 합산하여 업데이트
	*/
    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "---------------daem5121_process_db_all-------------------\n");  
    ZzLOG(ALWAY, "gproc_date : [%s]\n", gst_date);  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");

	char szQuery[1600];		// query string
	int  ret;

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/
	
	unsigned long dwCount = 0;
	
	memset (szQuery, 0x00, sizeof(szQuery));
	
	strcpy(szQuery,"select user_id, count(*), sum(out_cmt)	"
				   "from zangsi.T_OUT_AMOUNT				"
				   "where proc_stat = 3						"			 
				   "group by user_id 						" );

	ZzLOG(ALWAY, "%s\n\n",szQuery);
	
	
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5121_process_db_all: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}	
	
	if (!(res = mysql_store_result(con))) 
	{
	    ZzLOG(ERROR, "daem5121_process_db_all: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (mysql_num_rows(res)==0)	
	{
	    ZzLOG(ALWAY, "daem5121_process_db_all: 처리할 자료가 없습니다.\n");
		goto daem5121_process_db_err;
	}

	ZzLOG(ALWAY, "daem5121_process_db_all: tot_out_cnt, tot_out_cmt select_cnt(%d)\n", mysql_num_rows(res));

	//--------------------------------------------------------------------------
	// 트렌젝션시작
	//--------------------------------------------------------------------------
			
	while((row = mysql_fetch_row(res))) 
	{
		dwCount	++;
		// 총 출금 건수(tot_out_cnt), 총 출금액(tot_out_cmt) update

		memset (szQuery, 0x00, sizeof(szQuery));
		
		sprintf(szQuery, "update zangsi.T_PERM_UPLOAD_AUTH set tot_out_cnt = %ld, tot_out_cmt = %ld where user_id = '%s' "
						 ,(unsigned long)getnum(row, 1), (unsigned long)getnum(row, 2), getstr(row, 0) );
		
		

		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5121_process_db_all: update T_PERM_UPLOAD_AUTH  error...\n");
			ZzLOG(ERROR, "daem5121_process_db_all: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "%s\n",szQuery);
			goto daem5121_process_db_err;
	    }
	}
	
	mysql_free_result(res);
	
	ZzLOG(ALWAY, "daem5121_process_db_all: 업데이트된 자료의 갯수는 %ld 입니다.\n",dwCount);		
	
	memset (szQuery, 0x00, sizeof(szQuery));
	
	strcpy(szQuery,"select user_id, sum(out_cmt)														"
				   "from zangsi.T_OUT_AMOUNT															"
				   "where proc_stat = '3' and substring(proc_date, 1,6) = date_format(now(), '%Y%m')	"
				   "group by user_id																	");

	ZzLOG(ALWAY, "%s\n\n",szQuery);
	
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5121_process_db_all: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}	

	if (!(res = mysql_store_result(con))) 
	{
	    ZzLOG(ERROR, "daem5121_process_db_all: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
	
	if (mysql_num_rows(res)==0)	
	{
		
	    ZzLOG(ALWAY, "daem5121_process_db_all: 처리할 자료가 없습니다.\n");
		goto daem5121_process_db_err;
	}

	ZzLOG(ALWAY, "daem5121_process_db_all: mon_out_cmt select_cnt(%d)\n", mysql_num_rows(res));

	//--------------------------------------------------------------------------
	// 트렌젝션시작
	//--------------------------------------------------------------------------

	dwCount	 = 0;
			
	while(row = mysql_fetch_row(res))
	{
		dwCount ++;
		memset (szQuery, 0x00, sizeof(szQuery));
		
		sprintf(szQuery, "update zangsi.T_PERM_UPLOAD_AUTH set mon_out_cmt = %ld  where user_id = '%s' "
						 , (unsigned long)getnum(row, 1)  , getstr(row, 0) );
	
		
	
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5121_process_db_all: update zangsi.T_PERM_UPLOAD_AUTH  error...\n");
			ZzLOG(ERROR, "daem5121_process_db_all: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "%s\n",szQuery);
			goto daem5121_process_db_err;
	    }
 	}	
 	ZzLOG(ALWAY, "daem5121_process_db_all: 업데이트된 자료의 갯수는 %ld 입니다.\n",dwCount);		
	
	mysql_free_result(res);

			
	ZzLOG(ALWAY,"완료\n");

	return 0;

daem5121_process_db_err:
	ZzLOG(ERROR, "process_db1: [%d](%s)\n",mysql_errno(con), mysql_error(con));

	mysql_free_result(res);

	ZzLOG(ALWAY, "daem5121_process_db_all: 업데이트된 자료의 갯수는 %ld 입니다.\n",dwCount);	
		
	return -1;	
}

int daem5121_process_db_day()
{
	
	/*
	데몬은 하루에 한번!!
	
	전체 데이터로 돌아가는것 하나..
	어제 데이터읽어와서 돌아가는것 하나..
	
	T_OUT_AMOUNT에서 어제 날짜의 user_id, count(*) sum(out_cmt)를 읽어와 
	T_PERM_UPLOAD_AUTH의 tot_out_cnt, tot_out_cmt의 값에 합산하여 업데이트

	T_OUT_AMOUNT에서 한달동안의 출금액을 읽어와 
	T_PERM_UPLOAD_AUTH의 mon_out_cmt의 값에 합산하여 업데이트
	*/
    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "---------------daem5121_process_db_day-------------------\n");  
    ZzLOG(ALWAY, "gproc_date : [%s]\n", gst_date);  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");

	char szQuery[1600];		// query string
	int  ret;
	unsigned long tot_out_cnt = 0;
	unsigned long tot_out_cmt = 0;
	unsigned long mon_out_cmt = 0;

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/
	
	unsigned long dwCount = 0;
	
	memset (szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery,"select user_id, count(*), sum(out_cmt)		"
				   "from zangsi.T_OUT_AMOUNT					"
				   "where proc_stat = 3	and proc_date = '%s'	"			 
				   "group by user_id 							"
				   ,gst_date );

	ZzLOG(ALWAY, "%s\n\n",szQuery);



	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5121_process_db_day: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}	
	
	if (!(res = mysql_store_result(con))) 
	{
	    ZzLOG(ERROR, "daem5121_process_db_day: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (mysql_num_rows(res)==0)	
	{
	    ZzLOG(ALWAY, "daem5121_process_db_day: 처리할 자료가 없습니다.\n");
		goto daem5121_process_db_err;
	}

	ZzLOG(ALWAY, "daem5121_process_db_day: tot_out_cnt, tot_out_cmt select_cnt(%d)\n", mysql_num_rows(res));

	//--------------------------------------------------------------------------
	// 트렌젝션시작
	//--------------------------------------------------------------------------
			
	while((row = mysql_fetch_row(res))) 
	{
		dwCount	++;
		
		// 총 출금 건수(tot_out_cnt), 총 출금액(tot_out_cmt) update
		memset (szQuery, 0x00, sizeof(szQuery));
	
		sprintf(szQuery,"select tot_out_cnt, tot_out_cmt			"
					   "from zangsi.T_PERM_UPLOAD_AUTH			"
					   "where user_id = '%s'					"			 
					   "group by user_id 						"
					   ,getstr(row, 0) );
		
		
		
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5121_process_db_day: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}	
		
		if (!(res2 = mysql_store_result(con))) 
		{
		    ZzLOG(ERROR, "daem5121_process_db_day: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
		if (mysql_num_rows(res2)==0)	
		{
		    ZzLOG(ALWAY, "daem5121_process_db_day: 처리할 자료가 없습니다.\n");
			goto daem5121_process_db_err;
		}
		row2 = mysql_fetch_row(res2);
		tot_out_cnt = (unsigned long)getnum(row2, 0);
		tot_out_cmt = (unsigned long)getnum(row2, 1);
		mysql_free_result(res2);

		memset (szQuery, 0x00, sizeof(szQuery));
		
		tot_out_cnt = tot_out_cnt + (unsigned long)getnum(row, 1);
		tot_out_cmt = tot_out_cmt + (unsigned long)getnum(row, 2);

		sprintf(szQuery, "update zangsi.T_PERM_UPLOAD_AUTH set tot_out_cnt = %ld, tot_out_cmt = %ld where user_id = '%s' "
						 ,tot_out_cnt, tot_out_cmt, getstr(row, 0) );
		
		

		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5121_process_db_day: update zangsi.T_PERM_UPLOAD_AUTH  error...\n");
			ZzLOG(ERROR, "daem5121_process_db_day: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "%s\n",szQuery);
			goto daem5121_process_db_err;
	    }
	}
	
	mysql_free_result(res);
	
	ZzLOG(ALWAY, "daem5121_process_db_day: 업데이트된 자료의 갯수는 %ld 입니다.\n",dwCount);		


	memset (szQuery, 0x00, sizeof(szQuery));
	
	strcpy(szQuery,"update zangsi.T_PERM_UPLOAD_AUTH set mon_out_cmt = 0 "
				   "where mon_out_cmt > 0 	");

	ZzLOG(ALWAY, "%s\n\n",szQuery);
	

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5121_process_db_day: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}	


	
	memset (szQuery, 0x00, sizeof(szQuery));
	
	strcpy(szQuery,"select user_id, sum(out_cmt)														"
				   "from zangsi.T_OUT_AMOUNT															"
				   "where proc_stat = '3' and substring(proc_date, 1,6) = date_format(now(), '%Y%m')	"
				   "group by user_id																	");

	ZzLOG(ALWAY, "%s\n\n",szQuery);
	

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5121_process_db_day: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}	

	if (!(res = mysql_store_result(con))) 
	{
	    ZzLOG(ERROR, "daem5121_process_db_day: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
	
	if (mysql_num_rows(res)==0)	
	{
		
	    ZzLOG(ALWAY, "daem5121_process_db_day: 처리할 자료가 없습니다.\n");
		goto daem5121_process_db_err;
	}

	ZzLOG(ALWAY, "daem5121_process_db_day: select_cnt(%d)\n", mysql_num_rows(res));

	//--------------------------------------------------------------------------
	// 트렌젝션시작
	//--------------------------------------------------------------------------

	dwCount	 = 0;
			
	while(row = mysql_fetch_row(res))
	{
		dwCount ++;

		memset (szQuery, 0x00, sizeof(szQuery));
		
		sprintf(szQuery, "update zangsi.T_PERM_UPLOAD_AUTH set mon_out_cmt = %ld  where user_id = '%s' "
						 , (unsigned long)getnum(row, 1), getstr(row, 0) );
	
		ZzLOG(ALWAY, "%s\n\n",szQuery);
		
		
		
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5121_process_db_day: g zangsi.T_PERM_UPLOAD_AUTH  error...\n");
			ZzLOG(ERROR, "daem5121_process_db_day: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "%s\n",szQuery);
			goto daem5121_process_db_err;
		}
	}	
	ZzLOG(ALWAY, "daem5121_process_db_day: 업데이트된 자료의 갯수는 %ld 입니다.\n",dwCount);		
	
	mysql_free_result(res);
			
	ZzLOG(ALWAY,"완료\n");

	return 0;

daem5121_process_db_err:
	ZzLOG(ERROR, "process_db1: [%d](%s)\n",mysql_errno(con), mysql_error(con));

	mysql_free_result(res);

	ZzLOG(ALWAY, "daem5121_process_db_day: 업데이트된 자료의 갯수는 %ld 입니다.\n",dwCount);	
		
	return -1;	
}


/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5121_process_init(int argc, char **argv)
{
	char stemp[128];
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("daem5121", "/logs/daemon"); 

    ZzLOG(ALWAY, "[daem5121]*****************프로그램 시작*****************\n");  

    // 파라미터 값 설정 및 초기화
    if (argc != 2) {
    	goto arg_error;
    }

	/* 처리일자 */
	memset(gst_date, 0x00, sizeof(gst_date));
	strcpy(gst_date, argv[1]);

	
	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(con=db_connect("zangsi")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1); 
	}
		
    return (0);

arg_error:
    ZzLOG(ERROR, "usage : 00000000 or all\n");

    ZzPRT(ERROR, "usage : 00000000 or all\n");
    return -1;
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daem5121_process_term()
{
    // DB close
	db_disconnect(con);
	
    ZzLOG(ALWAY, "[daem5121]*****************프로그램 종료*****************\n");  

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5121_signal(int nSignal)
{
    daem5121_process_term();
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
	signal(SIGTERM, daem5121_signal);
	signal(SIGINT,  daem5121_signal);
	signal(SIGQUIT, daem5121_signal);
	signal(SIGKILL, daem5121_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daem5121_process_init(argc, argv) == 0 ) 
	{
		/* 프로그램 메인루틴 */
		rc = daem5121_process();
		/* 프로그램 종료루틴 */                    
		daem5121_process_term();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
