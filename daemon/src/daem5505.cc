/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5505.cc
 *         기능 : 월간 평균 접속자수 집계
 *         설명 : 
 *     설치위치 : 관리자 디비
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

#include "daemcom.h"
#include "commydb.h"

#define  MAX_ROWS	1


int daem5505_process();
int daem5505_process_db();
int daem5505_stat_data();
int daem5505_process_init(int argc, char **argv);
int daem5505_process_term();
void daem5505_signal(int nSignal);


MYSQL     *log_con;
MYSQL_RES *res;
MYSQL_ROW  row;

MYSQL     *bck_con;


bool gbIsUserDate      ; //날짜입력
char gproc_date[8+1];	// 처리일자
char greg_date[8+1];	// 등록일자
char greg_time[6+1];	// 등록일자
bool bIsSystemDay;



//******************************************************************************
//* daem5505 main
//******************************************************************************
int daem5505_process()
{
	char szQuery[1000];  // query string
	memset(szQuery, 0x00, sizeof(szQuery));	

	// 처리일자가 "00000000"일때는 시스템일자 -1을 한다.
	if (strcmp(gproc_date, "00000000") == 0)
	{
		bIsSystemDay = true;
		strcpy(szQuery, "SELECT date_format(date_add(now(), INTERVAL -1 DAY),'%Y%m%d'), date_format(now(),'%Y%m%d'), date_format(now(),'%H%i%s') ; /* daem5505 */");
	}
	else
	{
		bIsSystemDay = false;
		sprintf(szQuery, " select '%s', date_format(now(),'%%Y%%m%%d'), date_format(now(),'%%H%%i%%s') ; /* daem5505 */ ", gproc_date);
	}

	ZzLOG(ALWAY, "%s\n", szQuery);

	if (mysql_query(log_con, szQuery)){
	    ZzLOG(ERROR, "[daem5505]sysdate: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(log_con), mysql_error(log_con));
		return -1;
	}
	
	if (!(res = mysql_store_result(log_con)))
	{
	    ZzLOG(ERROR, "[daem5505]sysdate: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(log_con), mysql_error(log_con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "[daem5505]sysdate: mysql_num_rows error...\n");
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
			

	
	#ifdef __DEBUG
	printf("적용 날짜 : %s\n",gproc_date);
	#endif 


	if( daem5505_process_db() !=0)
	{
		return -1;
	}

	if( daem5505_stat_data() !=0)
	{
		return -1;
	}

	
	return 0;
}

//******************************************************************************
//* daem5505 db 처리로직
//******************************************************************************
int daem5505_process_db()
{
	char szQuery[1600];		// query string
	int  ret;
	
	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/
	

    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "[daem5505]gproc_date : [%s]\n", gproc_date);  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/
	
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " delete from zangsi.T_LOGIN_DAY where login_date = '%s' ; /* daem5505 */ ", gproc_date);

	if (mysql_query(log_con, szQuery))
	{
		ZzLOG(ERROR, "daem5505_delete_data: [%d](%s)\n",mysql_errno(log_con), mysql_error(log_con));
		ZzLOG(ERROR, "%s\n",szQuery);
		mysql_free_result(res);
		return -1;
    }
					
	int nCount = 0;
	

	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery," select user_id, login_date "
					" from zangsi.T_LOGIN_LOG "
					" where login_date = '%s' "
					" group by user_id ; /* daem5505 */ "
					, gproc_date);

	
	#ifdef __DEBUG
	//printf("%s\n\n",szQuery);
	#endif
	
	if (mysql_query(log_con, szQuery))
	{
	    ZzLOG(ERROR, "daem5505_process: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)[%s]",mysql_errno(log_con), mysql_error(log_con),szQuery);
		return -1;
	}

	if (!(res = mysql_store_result(log_con))) 
	{
	    ZzLOG(ERROR, "daem5505_process: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)[%s]",mysql_errno(log_con), mysql_error(log_con),szQuery);
		return -1;
	}
 	if (mysql_num_rows(res)==0)	
 	{
	    ZzLOG(ALWAY, "daem5505_process: 처리할 자료가 없습니다.\n");
		return -1;
	}

	if (!(bck_con=db_connect_nodb("zangsi_sum")))
	{
		ZzLOG(ERROR, "[daem5505]bck DB에 접속하지 못 하였습니다...\n");
		db_disconnect(bck_con);
	   	return(-1); 
	}

	while((row = mysql_fetch_row(res))) 
	{
		nCount++;
		char szUserId[16+1];
		memset(szUserId, 0x00, sizeof(szUserId));

		char szLoginDate[8+1];
		memset(szLoginDate, 0x00, sizeof(szLoginDate));
		
		strcpy(szUserId, getstr(row,0));
		strcpy(szLoginDate, getstr(row,1));
		
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi.T_LOGIN_DAY values ('%s', '%s') ; /* daem5505 */ ", szUserId, szLoginDate);

		if (mysql_query(log_con, szQuery))
		{
			ZzLOG(ERROR, "daem5505_delete_data: [%d](%s)\n",mysql_errno(log_con), mysql_error(log_con));
			ZzLOG(ERROR, "%s\n",szQuery);
			mysql_free_result(res);
			return -1;
	    }
	    if(bIsSystemDay)
	    {	
		    MYSQL_RES *bck_res; 
			MYSQL_ROW  bck_row; 
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " select user_id from zangsi_sum.T_LOGIN_INFO where user_id = '%s' ; /* daem5505 */ ", szUserId);
			
			if (mysql_query(bck_con, szQuery))
			{
				ZzLOG(ERROR, "daem5505_delete_data: [%d](%s)\n",mysql_errno(bck_con), mysql_error(bck_con));
				ZzLOG(ERROR, "%s\n",szQuery);
				mysql_free_result(res);
				return -1;
			}
			
			if (!(bck_res = mysql_store_result(bck_con)))
			{
				ZzLOG(ERROR, "daem5505_delete_data: [%d](%s)\n",mysql_errno(bck_con), mysql_error(bck_con));
				ZzLOG(ERROR, "%s\n",szQuery);
				mysql_free_result(res);
				return -1;
			}
		 	if (mysql_num_rows(bck_res)==0)
		 	{
				mysql_free_result(bck_res);
				
				memset (szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " insert into zangsi_sum.T_LOGIN_INFO values ('%s', '%s', '') ; /* daem5505 */ ", szUserId, szLoginDate);
				
				if (mysql_query(bck_con, szQuery))
				{
					ZzLOG(ERROR, "daem5505_delete_data: [%d](%s)\n",mysql_errno(bck_con), mysql_error(bck_con));
					ZzLOG(ERROR, "%s\n",szQuery);
					mysql_free_result(res);
					return -1;
				}
			}
			else
			{
				mysql_free_result(bck_res);
	
				memset (szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " update zangsi_sum.T_LOGIN_INFO set last_date = login_date, login_date = '%s'  where user_id = '%s' ; /* daem5505 */ ", szLoginDate, szUserId);
				
				if (mysql_query(bck_con, szQuery))
				{
					ZzLOG(ERROR, "daem5505_delete_data: [%d](%s)\n",mysql_errno(bck_con), mysql_error(bck_con));
					ZzLOG(ERROR, "%s\n",szQuery);
					mysql_free_result(res);
					return -1;
				}
			}
		}
	}
	mysql_free_result(res);

	ZzLOG(ALWAY, "daem5505_process: T_LOGIN_LOG 처리된 자료의 갯수는 %d 입니다.\n",nCount);	
		
	return 0;
}	

int daem5505_stat_data()
{
	char szQuery[10000];
	memset(szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery, " delete from zangsi_sum.T_LOGIN_AVG_CNT where login_date = '%s' ; /* daem5505 */ " , gproc_date);
					 
	ZzLOG(ALWAY, "%s\n", szQuery);					 
						 
	if (mysql_query(bck_con, szQuery))
	{
		ZzLOG(ERROR, "daem5505_delete_data: [%d](%s)\n",mysql_errno(bck_con), mysql_error(bck_con));
		ZzLOG(ERROR, "%s\n",szQuery);
		
		return -1;
    }

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select substring('%s', 1,6) " , gproc_date);

	if (mysql_query(log_con, szQuery))
	{
		ZzLOG(ERROR, "daem5505_delete_data: [%d](%s)\n",mysql_errno(log_con), mysql_error(log_con));
		ZzLOG(ERROR, "%s\n",szQuery);
		return -1;
    }
    
	if (!(res = mysql_store_result(log_con)))
	{
		ZzLOG(ERROR, "daem5505_delete_data: [%d](%s)\n",mysql_errno(log_con), mysql_error(log_con));
		ZzLOG(ERROR, "%s\n",szQuery);
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
		ZzLOG(ERROR, "daem5505_delete_data: [%d](%s)\n",mysql_errno(log_con), mysql_error(log_con));
		ZzLOG(ERROR, "%s\n",szQuery);
		mysql_free_result(res);
		return -1;
	}
	row = mysql_fetch_row(res);

	char szRegDate[6+1];
	memset(szRegDate, 0x00, sizeof(szRegDate));
	strcpy(szRegDate, getstr(row,0));
	
	mysql_free_result(res);

	
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select count(user_id) , count(distinct(user_id)), count(user_id) / count(distinct(user_id)) from zangsi.T_LOGIN_DAY "
					 " where login_date >= '%s01'  and login_date <= '%s' ; /* daem5505 */ ", szRegDate, gproc_date);

	ZzLOG(ALWAY, "%s\n", szQuery);					 
	if (mysql_query(log_con, szQuery))
	{
		ZzLOG(ERROR, "daem5505_delete_data: [%d](%s)\n",mysql_errno(log_con), mysql_error(log_con));
		ZzLOG(ERROR, "%s\n",szQuery);
		return -1;
    }
    
	if (!(res = mysql_store_result(log_con)))
	{
		ZzLOG(ERROR, "daem5505_delete_data: [%d](%s)\n",mysql_errno(log_con), mysql_error(log_con));
		ZzLOG(ERROR, "%s\n",szQuery);
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
		ZzLOG(ERROR, "daem5505_delete_data: [%d](%s)\n",mysql_errno(log_con), mysql_error(log_con));
		ZzLOG(ERROR, "%s\n",szQuery);
		mysql_free_result(res);
		return -1;
	}
	row = mysql_fetch_row(res);
	int nTotalCnt = (int)getint(row,0);
	int nDistCnt = (int)getint(row,1);
	double dAvgCnt = getnum(row,2);
	mysql_free_result(res);
	
	
	sprintf(szQuery, " insert into zangsi_sum.T_LOGIN_AVG_CNT values ('%s', '%s', %.2f, %d, %d) ; /* daem5505 */ "
					 , szRegDate, gproc_date, dAvgCnt, nTotalCnt, nDistCnt);
					 
	ZzLOG(ALWAY, "%s\n", szQuery);					 
						 
	if (mysql_query(bck_con, szQuery))
	{
		int nErrno = mysql_errno(bck_con);

		if( nErrno == 2013 )
		{
			if (!(bck_con=db_connect_nodb("zangsi_sum")))
			{
				ZzLOG(ERROR, "[daem5505]bck DB에 접속하지 못 하였습니다...\n");
				db_disconnect(bck_con);
			   	return(-1); 
			}
	
			sprintf(szQuery, " insert into zangsi_sum.T_LOGIN_AVG_CNT values ('%s', '%s', %.2f, %d, %d) ; /* daem5505 */ "
					 , szRegDate, gproc_date, dAvgCnt, nTotalCnt, nDistCnt);
					 
			mysql_query(bck_con, szQuery);
			
	
		}
		else
		{
			ZzLOG(ERROR, "daem5505_delete_data: [%d](%s)\n",mysql_errno(bck_con), mysql_error(bck_con));
			ZzLOG(ERROR, "%s\n",szQuery);
					
			return -1;
		}
    }
    return 0;
}

/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5505_process_init(int argc, char **argv)
{
	char stemp[128];
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("d_", "/logs/daemon"); 

    ZzLOG(ALWAY, "[daem5505]*****************프로그램 시작*****************\n");  

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
		ZzLOG(ERROR, "[daem5505]log DB에 접속하지 못 하였습니다...\n");
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
int daem5505_process_term()
{
    // DB close
	db_disconnect(log_con);
	db_disconnect(bck_con);	
	
    ZzLOG(ALWAY, "[daem5505]*****************프로그램 종료*****************\n");  

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5505_signal(int nSignal)
{
    daem5505_process_term();
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
	signal(SIGTERM, daem5505_signal);
	signal(SIGINT,  daem5505_signal);
	signal(SIGQUIT, daem5505_signal);
	signal(SIGKILL, daem5505_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daem5505_process_init(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daem5505_process();
		/* 프로그램 종료루틴 */                    
		daem5505_process_term();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
