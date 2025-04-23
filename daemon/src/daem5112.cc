/*===============================================================================
 *   서브시스템 : contents daemon프로세스
 *   프로그램명 : daem5112.cc
 *         기능 : Contents 구매 수량 집계
 *         설명 : 메일링 회원의 구매한 Contents 구매 수량 현황을 분류별로 집계한다.
 *     설치위치 : SUM DB에 위치한다.
 *
 *       작성자 : HCS
 *       작성일 : 2009/06/18
 *     수정이력 : 
 *===============================================================================
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
===============================================================================*/

#include <mysql.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h> //for sleep()

//사용자 정의 헤더파일==============================================================
#include "daemcom.h"
#include "commydb.h"
//==================================================================================

#define	MAX_ROWS	1
#define	U	0
#define I	1
#define WE	10
#define FD	20



int daem5112_process();
int daem5112_process_db();


int daem5112_process_init(int argc, char **argv);
int daem5112_process_term();
void daem5112_signal(int nSignal);
int CountDealInfo(char* pUserId);
int CountFDealInfo(char* pUserId);
int InsertUserDealCnt(char* pUserId, char* pSectCode, int nBuyCnt, int nContGu, int nMode);


//Database Variable=============================================================
MYSQL     *con;
MYSQL_RES *res;
MYSQL_ROW  row;
MYSQL     *sum_con;
//==============================================================================

//Global Variable===============================================================
bool gbIsUserDate      ; //날짜입력
char gst_date[8+1];	// 처리일자
char gst_time[6+1];	// 처리시작시간
char ged_time[6+1];	// 처리마지막시간
char gproc_log[80]; // 로그메시지버퍼
//==============================================================================


//==============================================================================
//* daem5112 핵심 수행 함수
//==============================================================================
int daem5112_process()
{
	char szQuery[1000];  // query string
	memset(szQuery, 0x00, sizeof(szQuery));	

	// 처리일자가 "00000000"일때는 시스템일자 -1을 한다.
	if (strcmp(gst_date, "00000000") == 0)
	{
		gbIsUserDate = false;
		
		// 현재로부터 1일 이전 날짜를 얻는다.
		strcpy(szQuery, "SELECT date_format(date_add(now(), INTERVAL -1 DAY),'%Y%m%d') ; /* daem5112 */");		

		if (mysql_query(con, szQuery)){
		  ZzLOG(ERROR, "[daem5112]sysdate: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
				
		if (!(res = mysql_store_result(con)))
		{
		  ZzLOG(ERROR, "[daem5112]sysdate: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
		
	 	if (mysql_num_rows(res)==0)
	 	{
		  ZzLOG(ERROR, "[daem5112]sysdate: mysql_num_rows error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
		
		row = mysql_fetch_row(res);
		memset(gst_date, 0x00, sizeof(gst_date));
		strcpy(gst_date,   getstr(row, 0));
		mysql_free_result(res);				
	}
	else
	{
		gbIsUserDate=true;
	}
		
	#ifdef __DEBUG
	printf("적용 날짜 : %s\n",gst_date);
	#endif 
	
	return daem5112_process_db();
	
}

//******************************************************************************
//* daem5112 db 처리로직
//******************************************************************************
int daem5112_process_db()
{
	char szQuery[1600];		// query string
	int  ret;
	
	int nSleepCnt = 0;
	int nLimitCnt = 0;
	int nCount = 0;

	ZzLOG(ALWAY, "---------------------------------------------------------\n");
	ZzLOG(ALWAY, "[daem5112]gproc_date : [%s]\n", gst_date);  
	ZzLOG(ALWAY, "---------------------------------------------------------\n");
	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/
	
	while(1) 
	{
		memset (szQuery, 0x00, sizeof(szQuery));
		
		sprintf(szQuery, " select user_id from zangsi.T_EVENT_EMAIL_LIST "
						 " where login_date = '%s' and send_yn = 'Y' limit %d, 100 ; /* daem5112 */ "
						 , gst_date, nLimitCnt);

		nLimitCnt = nLimitCnt + 100;
		
		#ifdef __DEBUG
		printf("%s\n\n",szQuery);
		#endif
		
		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "daem5112_process_db: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
			return -2;
		}
		
		if (!(res = mysql_store_result(con))) 
		{
			ZzLOG(ERROR, "daem5112_process_db: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
			return -2;
		}
		
	 	if (mysql_num_rows(res)==0)	
	 	{
			mysql_free_result(res);
			#ifdef __DEBUG
			printf("daem5112_process: 처리할 자료가 없습니다.\n");
			#endif
			break;
		}
		
		while((row = mysql_fetch_row(res))) 
		{
			char szUserId[18];
			memset(szUserId, 0x00, sizeof(szUserId));			
			strcpy(szUserId, getstr(row,0));
			
			if(CountDealInfo(szUserId) < 0)
			{
				#ifdef __DEBUG
				printf("daem5112_process: CountDealInfo 에러\n");
				#endif
				return -1;
			}

			if(CountFDealInfo(szUserId) < 0)
			{
				#ifdef __DEBUG
				printf("daem5112_process: CountFDealInfo 에러\n");
				#endif
				return -1;
			}
			
			nCount++;
			// 구매자 누접집계를 한다
		}
		mysql_free_result(res);
		
		usleep(500);
	}
	ZzLOG(ALWAY,"daem5112_process_db: %d명이 처리되었습니다.\n", nCount);  
	return 0;
}
int CountDealInfo(char* pUserId)
{
	MYSQL_RES *res;
	MYSQL_ROW  row;

	MYSQL_RES *sum_res;
	MYSQL_ROW  sum_row;
	
	char szQuery[10000];
	memset(szQuery, 0x00, sizeof(szQuery));
	

	
	sprintf(szQuery, " select sect_code, count(deal_no) "
					 " from zangsi.T_DEAL_INFO  "
					 " where buy_user = '%s' and deal_date = '%s' and cont_gu = 'WE' "
					 " group by sect_code ; /* daem5112 */ "
					 , pUserId, gst_date);
					 
	#ifdef __DEBUG
	printf("%s\n\n",szQuery);
	#endif

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "CountDealInfo: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	
	if (!(res = mysql_store_result(con))) 
	{
		ZzLOG(ERROR, "CountDealInfo: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	
 	if (mysql_num_rows(res)==0)	
 	{
		mysql_free_result(res);
		return 1;
	}
	
	while(row = mysql_fetch_row(res))
	{
		char szSectCode[6+1];
		memset(szSectCode, 0x00, sizeof(szSectCode));
		
		strcpy(szSectCode, getstr(row,0));
		int nBuyCnt = (int)getint(row,1);
		
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "select user_id from zangsi_sum.T_USER_DEAL_CNT where user_id = '%s' ; /* daem5112 */ ", pUserId);

		if (mysql_query(sum_con, szQuery))
		{
			ZzLOG(ERROR, "CountDealInfo: [%d](%s)\n(%s)\n",mysql_errno(sum_con), mysql_error(sum_con), szQuery);
			return -1;
		}
		
		if (!(sum_res = mysql_store_result(sum_con))) 
		{
			ZzLOG(ERROR, "CountDealInfo: [%d](%s)\n(%s)\n",mysql_errno(sum_con), mysql_error(sum_con), szQuery);
			return -1;
		}
		
	 	if (mysql_num_rows(sum_res)==0)	
	 	{
			mysql_free_result(sum_res);			
			if(InsertUserDealCnt(pUserId, szSectCode, nBuyCnt, WE, I) < 0)
			{
				ZzLOG(ERROR, "CountDealInfo: InsertUserDealCnt() 실패.[%s][%s][%d]", pUserId, szSectCode, nBuyCnt);
				return -1;
			}
			
		}
		else
		{
			mysql_free_result(sum_res);
			if(InsertUserDealCnt(pUserId, szSectCode, nBuyCnt, WE, U) < 0)
			{
				ZzLOG(ERROR, "CountDealInfo: InsertUserDealCnt() 실패.[%s][%s][%d]", pUserId, szSectCode, nBuyCnt);
				return -1;
			}
		}
	}
	
	mysql_free_result(res);					 
	
	return 0;
	
}
int CountFDealInfo(char* pUserId)
{
	MYSQL_RES *res;
	MYSQL_ROW  row;

	MYSQL_RES *sum_res;
	MYSQL_ROW  sum_row;
	
	char szQuery[10000];
	memset(szQuery, 0x00, sizeof(szQuery));
	
	sprintf(szQuery, " select count(deal_no) "
					 " from zangsi.T_DEAL_INFO  "
					 " where buy_user = '%s' and deal_date = '%s' and cont_gu = 'FD' "
					 " group by cont_gu ; /* daem5112 */ "
					 , pUserId, gst_date);
					 
	#ifdef __DEBUG
	printf("%s\n\n",szQuery);
	#endif
	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "CountFDealInfo: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	
	if (!(res = mysql_store_result(con))) 
	{
		ZzLOG(ERROR, "CountFDealInfo: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	
 	if (mysql_num_rows(res)==0)	
 	{
		mysql_free_result(res);
		return 1;
	}
	
	while(row = mysql_fetch_row(res))
	{
		char szSectCode[6+1];
		memset(szSectCode, 0x00, sizeof(szSectCode));
		
		int nBuyCnt = (int)getint(row,0);
		
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "select user_id from zangsi_sum.T_USER_DEAL_CNT where user_id = '%s' ; /* daem5112 */ ", pUserId);

		if (mysql_query(sum_con, szQuery))
		{
			ZzLOG(ERROR, "CountFDealInfo: [%d](%s)\n(%s)\n",mysql_errno(sum_con), mysql_error(sum_con), szQuery);
			return -1;
		}
		
		if (!(sum_res = mysql_store_result(sum_con))) 
		{
			ZzLOG(ERROR, "CountFDealInfo: [%d](%s)\n(%s)\n",mysql_errno(sum_con), mysql_error(sum_con), szQuery);
			return -1;
		}
		
	 	if (mysql_num_rows(sum_res)==0)	
	 	{
			mysql_free_result(sum_res);			
			if(InsertUserDealCnt(pUserId, "", nBuyCnt, FD, I) < 0)
			{
				ZzLOG(ERROR, "CountFDealInfo: InsertUserDealCnt() 실패.[%s][%s][%d]", pUserId, szSectCode, nBuyCnt);
				return -1;
			}
			
		}
		else
		{
			mysql_free_result(sum_res);
			if(InsertUserDealCnt(pUserId, "", nBuyCnt, FD, U) < 0)
			{
				ZzLOG(ERROR, "CountFDealInfo: InsertUserDealCnt() 실패.[%s][%s][%d]", pUserId, szSectCode, nBuyCnt);
				return -1;
			}
		}
	}
	
	mysql_free_result(res);					 
	
	return 0;
	
}

int InsertUserDealCnt(char* pUserId, char* pSectCode, int nBuyCnt, int nContGu, int nMode)
{
	MYSQL_RES *sum_res;
	MYSQL_ROW  sum_row;
	
	char szQuery[10000];
	memset(szQuery, 0x00, sizeof(szQuery));
	
	char szFieldNm[6+1];
	memset(szFieldNm, 0x00, sizeof(szFieldNm));
	
	
	if(nContGu == WE)
	{
		sprintf(szFieldNm, "%s_cnt", pSectCode);

		if(nMode == U)
		{
			sprintf(szQuery, " update zangsi_sum.T_USER_DEAL_CNT set %s = %s + %d where user_id = '%s' ; /* daem5112 */ "
							 , szFieldNm
							 , szFieldNm
							 , nBuyCnt
							 , pUserId);
		}
		else
		{
			sprintf(szQuery, "insert into zangsi_sum.T_USER_DEAL_CNT (user_id, %s) values ('%s', %d) ; /* daem5112 */ "
							 , szFieldNm, pUserId, nBuyCnt);
		}
	}
	else
	{
		if(nMode == U)
		{
			sprintf(szQuery, " update zangsi_sum.T_USER_DEAL_CNT set fd_cnt = fd_cnt + %d where user_id = '%s' ; /* daem5112 */ "
							 , nBuyCnt
							 , pUserId);
		}
		else
		{
			sprintf(szQuery, "insert into zangsi_sum.T_USER_DEAL_CNT (user_id, fd_cnt) values ('%s', %d) ; /* daem5112 */ "
							 , pUserId, nBuyCnt);
		}
		
	}

	#ifdef __DEBUG
	printf("%s\n\n",szQuery);
	#endif
	if (mysql_query(sum_con, szQuery))
	{
		ZzLOG(ERROR, "CountDealInfo: [%d](%s)\n(%s)\n",mysql_errno(sum_con), mysql_error(sum_con), szQuery);
		return -1;
	}
}

/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5112_process_init(int argc, char **argv)
{
	char stemp[128];
	
  //전역변수 초기화
  ZzInitGlobalVariable2("d_", "/logs/daemon"); 
  //프로그램 시작 로그
  ZzLOG(ALWAY, "[daem5112]*****************프로그램 시작*****************\n");  

  //파라미터 값 설정 및 초기화
  if (argc != 2) {goto arg_error;}

	//처리일자
	memset(gst_date, 0x00, sizeof(gst_date));
	strcpy(gst_date, argv[1]);
	
	//데이타베이스 연결-----------------------------------------------------
//	if (!(con=db_connect_local("zangsi")))
	if (!(con=db_connect_nodb("zangsi")))
	{
		ZzLOG(ERROR, "[daem5112]master DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1); 
	}

	if (!(sum_con=db_connect_nodb("zangsi_sum")))
	{
		ZzLOG(ERROR, "[daem5112]sum DB에 접속하지 못 하였습니다...\n");
		db_disconnect(sum_con);
	   	return(-1); 
	}		
    return (0);
  //--------------------------------------------------------------------------

arg_error:
    ZzLOG(ERROR, "usage : %s YYYYMMDD \n", argv[0]);
    ZzLOG(ERROR, "        YYYYMMDD : 처리일자(00000000:시스템일자)\n");
    return -1;
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daem5112_process_term()
{
  // DB close
	db_disconnect(con);
	db_disconnect(sum_con);	
	
  ZzLOG(ALWAY, "[daem5112]*****************프로그램 종료*****************\n");  

  return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5112_signal(int nSignal)
{
    daem5112_process_term();
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
	signal(SIGTERM, daem5112_signal);
	signal(SIGINT,  daem5112_signal);
	signal(SIGQUIT, daem5112_signal);
	signal(SIGKILL, daem5112_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daem5112_process_init(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daem5112_process();
		/* 프로그램 종료루틴 */                    
		daem5112_process_term();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
