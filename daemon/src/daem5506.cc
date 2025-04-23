/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5506.cc
 *         기능 : 현재 날짜로부터 90일전의 login_info 정보삭제
 *         설명 : 
 *     설치위치 : DNDB
 *
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
#include <unistd.h> // for usleep()

#include "daemcom.h"
#include "commydb.h"

#define  MAX_ROWS	1


int daem5506_process();
int daem5506_process_db();
int daem5506_process_init(int argc, char **argv);
int daem5506_process_term();
void daem5506_signal(int nSignal);


MYSQL     *log_con;

bool gbIsUserDate      ; //날짜입력
char gproc_date[8+1];	// 처리일자
char greg_date[8+1];	// 등록일자
char greg_time[6+1];	// 등록일자
bool bIsSystemDay;



//******************************************************************************
//* daem5506 main
//******************************************************************************
int daem5506_process()
{
	char szQuery[1000];  // query string
	memset(szQuery, 0x00, sizeof(szQuery));	

	MYSQL_RES *res;
	MYSQL_ROW  row;

	// 처리일자가 "00000000"일때는 시스템일자 -1을 한다.
	if (strcmp(gproc_date, "00000000") == 0)
	{
		bIsSystemDay = true;
		strcpy(szQuery, "SELECT date_format(date_add(now(), INTERVAL -90 DAY),'%Y%m%d'), date_format(now(),'%Y%m%d'), date_format(now(),'%H%i%s')");
	}
	else
	{
		bIsSystemDay = false;
		sprintf(szQuery, " select '%s', date_format(now(),'%%Y%%m%%d'), date_format(now(),'%%H%%i%%s')", gproc_date);
	}

	ZzLOG(ALWAY, "%s\n", szQuery);

	if (mysql_query(log_con, szQuery)){
	    ZzLOG(ERROR, "[daem5506]sysdate: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(log_con), mysql_error(log_con));
		return -1;
	}
	
	if (!(res = mysql_store_result(log_con)))
	{
	    ZzLOG(ERROR, "[daem5506]sysdate: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(log_con), mysql_error(log_con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "[daem5506]sysdate: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(log_con), mysql_error(log_con));
		return -1;
	}
	row = mysql_fetch_row(res);
	memset(gproc_date, 0x00, sizeof(gproc_date));
	memset(greg_date, 0x00, sizeof(greg_date));
	memset(greg_time, 0x00, sizeof(greg_time));
	
	strcpy(gproc_date,   getstr(row, 0));
	strcpy(greg_date,   getstr(row, 1));
	strcpy(greg_time,   getstr(row, 2));
	mysql_free_result(res);

	if( daem5506_process_db() !=0)
	{
		return -1;
	}

	return 0;
}

//******************************************************************************
//* daem5506 db 처리로직
//******************************************************************************
int daem5506_process_db()
{
	char szQuery[1600];		// query string
	int  ret;
	
	MYSQL_RES *res;
	MYSQL_ROW  row;

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/
	

    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "[daem5506]gproc_date : [%s]\n", gproc_date);  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/
	
	int nTotCount = 0;
	int nDayCount = 0;
	int nSleepCnt = 0;

	char szLoginDate[8+1];
	memset(szLoginDate, 0x00, sizeof(szLoginDate));

	char szLoginDate_T[8+1];
	memset(szLoginDate_T, 0x00, sizeof(szLoginDate_T));
	
	while(1)
	{
		if(nSleepCnt >= 100)
		{
			ZzLOG(ALWAY, "daem5506_process: (일자:%s, 총 삭제건:%d). 0.05초간 쉽니다.\n",szLoginDate, nTotCount);
			nSleepCnt = 0;
			usleep(50000);//0.05
		}
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery," select user_id, login_date, login_time "
						" from zangsi.T_LOGIN_LOG "
						" where login_date <= '%s' "
						" limit 1000 "
						, gproc_date);
		if (mysql_query(log_con, szQuery))
		{
		    ZzLOG(ERROR, "daem5506_process: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)[%s]",mysql_errno(log_con), mysql_error(log_con),szQuery);
			return -1;
		}
	
		if (!(res = mysql_store_result(log_con))) 
		{
		    ZzLOG(ERROR, "daem5506_process: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)[%s]",mysql_errno(log_con), mysql_error(log_con),szQuery);
			return -1;
		}
	 	if (mysql_num_rows(res)==0)	
	 	{
		    ZzLOG(ALWAY, "daem5506_process: T_LOGIN_LOG 처리된 총 자료의 갯수는 %d 입니다.\n",nTotCount);
			return -1;
		}
	
		while((row = mysql_fetch_row(res))) 
		{
			char szUserId[16+1];
			memset(szUserId, 0x00, sizeof(szUserId));
	
			memset(szLoginDate, 0x00, sizeof(szLoginDate));

			char szLoginTime[6+1];
			memset(szLoginTime, 0x00, sizeof(szLoginTime));
			
			strcpy(szUserId, getstr(row,0));
			strcpy(szLoginDate, getstr(row,1));
			strcpy(szLoginTime, getstr(row,2));
			
			if(nDayCount == 0)
				strcpy(szLoginDate_T, szLoginDate);
			
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " delete from zangsi.T_LOGIN_LOG where user_id = '%s' and login_date = '%s' and login_time = '%s' ", szUserId, szLoginDate, szLoginTime);
	
			if (mysql_query(log_con, szQuery))
			{
				ZzLOG(ERROR, "daem5506_delete_data: [%d](%s)\n",mysql_errno(log_con), mysql_error(log_con));
				ZzLOG(ERROR, "%s\n",szQuery);
				mysql_free_result(res);
				return -1;
		    }

			nTotCount++;
			
			if(strcmp(szLoginDate, szLoginDate_T) == 0)
				nDayCount++;
			else
			{
				ZzLOG(ALWAY, "daem5506_process: %s일 %d 건 처리완료.\n",nDayCount);
				nDayCount = 0;
			}
		}
		mysql_free_result(res);

		nSleepCnt++;
	}
		
	return 0;
}	

/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5506_process_init(int argc, char **argv)
{
	char stemp[128];
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("daem5506_", "/logs/daemon"); 

    ZzLOG(ALWAY, "[daem5506]*****************프로그램 시작*****************\n");  

    // 파라미터 값 설정 및 초기화
    if (argc != 2) {
    	goto arg_error;
    }

	/* 처리일자 */
	memset(gproc_date, 0x00, sizeof(gproc_date));
	strcpy(gproc_date, argv[1]);
	

	
	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(log_con=db_connect_logdb("zangsi")))
	{
		ZzLOG(ERROR, "[daem5506]log DB에 접속하지 못 하였습니다...\n");
		db_disconnect(log_con);
	   	return(-1); 
	}

		
    return (0);

arg_error:
    ZzLOG(ERROR, "usage : %s YYYYMMDD NN\n", argv[0]);
    ZzLOG(ERROR, "        YYYYMMDD : 처리일자(00000000:시스템일자)\n");

    ZzPRT(ERROR, "usage : %s YYYYMMDD NN\n", argv[0]);
    ZzPRT(ERROR, "        deal info 삭제 (30일 이전)\n");
    ZzPRT(ERROR, "        YYYYMMDD : 처리일자(00000000:시스템일자)\n");
    return -1;
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daem5506_process_term()
{
    // DB close
	db_disconnect(log_con);
	
    ZzLOG(ALWAY, "[daem5506]*****************프로그램 종료*****************\n");  

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5506_signal(int nSignal)
{
    daem5506_process_term();
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
	signal(SIGTERM, daem5506_signal);
	signal(SIGINT,  daem5506_signal);
	signal(SIGQUIT, daem5506_signal);
	signal(SIGKILL, daem5506_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daem5506_process_init(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daem5506_process();
		/* 프로그램 종료루틴 */                    
		daem5506_process_term();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
