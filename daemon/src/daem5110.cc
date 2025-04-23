/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5110.cc
 *         기능 : 탈퇴회원 정보 삭제
 *         설명 : 
 *     설치위치 : cmdsvr에 위치한다.
 *
 *       작성자 : HCS
 *       작성일 : 2008/07/30
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


int daem5110_process();
int daem5110_process_db();
int daem5110_process_init(int argc, char **argv);
int daem5110_process_term();
void daem5110_signal(int nSignal);
int daem5110_process_DBQuery(char* in_szQuery);
int daem5110_process_userchk();
int daem5110_process_DBDelete(char* in_szQuery);

MYSQL     *con;
MYSQL_RES *res;
MYSQL_ROW  row;

MYSQL     *con_backup;


bool gbIsUserDate ; //날짜입력
char gst_date[8+1];	// 처리일자
char gst_time[6+1];	// 처리시작시간
char ged_time[6+1];	// 처리마지막시간
char gproc_log[80]; // 로그메시지버퍼
char gszUserId[18];



//******************************************************************************
//* daem5110 main
//******************************************************************************
int daem5110_process()
{
	char szQuery[1000];  // query string
	memset(szQuery, 0x00, sizeof(szQuery));	
	
	
	// 처리일자가 "00000000"일때는 시스템일자 -1을 한다.
	if (strcmp(gst_date, "00000000") == 0)
	{
		gbIsUserDate = false;
		strcpy(szQuery, "SELECT date_format(date_add(now(), INTERVAL -1 DAY),'%Y%m%d')");
		// 현재로부터 1일 이전 날짜를 얻는다.

		if (mysql_query(con, szQuery)){
		    ZzLOG(ERROR, "[daem5110]sysdate: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
		
		if (!(res = mysql_store_result(con)))
		{
		    ZzLOG(ERROR, "[daem5110]sysdate: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
	 	if (mysql_num_rows(res)==0)
	 	{
		    ZzLOG(ERROR, "[daem5110]sysdate: mysql_num_rows error...\n");
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


	
	if( daem5110_process_db() !=0)
	{
		return -1;
	}	
//	if( daem5110

	
	return 0;
}

//******************************************************************************
//* daem5110 db 처리로직
//*
//* 탈퇴사용자를 찾아 데이타 정리 작업을 함.
//******************************************************************************
int daem5110_process_db()
{
	char szQuery[1600];		// query string
	int  ret;
	char szBuffer[600];
	unsigned long dwCount = 0;

    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "[daem5110]gproc_date : [%s]\n", gst_date);  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/
	while(1) 
	{
		memset (szQuery, 0x00, sizeof(szQuery));
		if( gbIsUserDate ) //하루 
		{
			sprintf(szQuery, " SELECT user_id FROM zangsi.T_USER_INFO " 
							 "  WHERE udt_date  = '%s' "
							 "    AND use_stat != 0 "
							 "   ORDER BY udt_date, user_id limit 3 "
	                        , gst_date , dwCount);			
		}
		else
		{
			sprintf(szQuery, " SELECT user_id FROM zangsi.T_USER_INFO " 
							 "  WHERE udt_date <= '%s' "
							 "    AND use_stat != 0 "
							 "  ORDER BY udt_date, user_id limit 3 "
	                        , gst_date );			
		}

		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5110_process: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
		}

		if (!(res = mysql_store_result(con))) {
		    ZzLOG(ERROR, "daem5110_process: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
		}
	 	if (mysql_num_rows(res)==0)	{
			mysql_free_result(res);
		    ZzLOG(ALWAY, "daem5110_process: 처리할 자료가 없습니다.\n");
			break;
		}
				
		while((row = mysql_fetch_row(res))) 
		{
			memset(gszUserId, 0x00, sizeof(gszUserId));
			strcpy(gszUserId, getstr(row,0));
			dwCount	++;

			if ( daem5110_process_userchk() != 0 )
				break;
		}
		break;
		
		
		mysql_free_result(res);
	}
	
	ZzLOG(ALWAY, "daem5110_process: 삭제된 탈퇴회원 정보의 갯수는 %ld 입니다.\n",dwCount);
	return 0;

daem5110_process_db_err:
	ZzLOG(ERROR, "[daem5110]process_db1: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	ZzLOG(ERROR, "[daem5110]process_db2: [%d](%s)\n",mysql_errno(con_backup), mysql_error(con_backup));	
/*	
	tran_rollback(con_backup);
	tran_end(con_backup);
	
	tran_rollback(con);
	tran_end(con);
*/
	mysql_free_result(res);

	ZzLOG(ALWAY, "daem5110_process: 백업된 자료의 갯수는 %ld 입니다.\n",dwCount);	
		
	return -1;	
}


//******************************************************************************
//* daem5110_process_userdel
//*
//* 특정 사용자에 한하여 모든 데이타 삭제 처리 한다.
//******************************************************************************
int daem5110_process_userdel()
{
	char szQuery[1600];		// query string

	//필로그 추천인
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi.T_BLOG_NMNT    where user_id = '%s' ", gszUserId);
	if (daem5110_process_DBDelete(szQuery) != 0) return -1;
	

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi.T_BLOG_BODM_00 where user_id = '%s' ", gszUserId);
	if (daem5110_process_DBDelete(szQuery) != 0) return -1;


	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi.T_BLOG_BODS_00 where user_id = '%s' ", gszUserId);
	if (daem5110_process_DBDelete(szQuery) != 0) return -1;
	
	return 0;

}


//******************************************************************************
//* daem5110 db 처리로직
//******************************************************************************
int daem5110_process_DBDelete(char* in_szQuery)
{
	if (mysql_query(con, in_szQuery)){
	    ZzLOG(ERROR, "%s ERROR!!!\n", in_szQuery);
		ZzLOG(ERROR, "errno[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
    }

	return 0;
}



//******************************************************************************
//* daem5110_process_userchk
//* 삭제하기 전 테스트 목적으로 확인하는 루틴
//* 
//* 평상시는 처리 하지 않으나, 개발자가 테스트 하기위해 사용한다..
//******************************************************************************
int daem5110_process_userchk()
{
	MYSQL_RES *loc_res;
	MYSQL_ROW  lov_row;
	char szQuery[1600];		// query string
	int select_cnt;
	
	printf("----------------------------->(%s)\n", gszUserId);

	select_cnt = 0;
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " SELECT count(user_id) FROM zangsi.T_BLOG_MAST where user_id = '%s' " , gszUserId);
	select_cnt = daem5110_process_DBQuery(szQuery);
	if( select_cnt < 0 ) return -1;
	printf("T_BLOG_MAST select count(%d)\n", select_cnt);


	select_cnt = 0;
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " SELECT count(user_id) FROM zangsi.T_USER_STAT where user_id = '%s' " , gszUserId);
	select_cnt = daem5110_process_DBQuery(szQuery);
	if( select_cnt < 0 ) return -1;
	printf("T_USER_STAT select count(%d)\n", select_cnt);
	
	
	return 0;
}


//******************************************************************************
//* daem5110 db 처리로직
//******************************************************************************
int daem5110_process_DBQuery(char* in_szQuery)
{
MYSQL_RES *loc_res;
MYSQL_ROW  loc_row;
int select_cnt;

	select_cnt = 0;
	if (mysql_query(con, in_szQuery))
	{
	    ZzLOG(ERROR, "daem5110_process: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "%s\n\n",in_szQuery);
		return -1;
	}
	if (!(loc_res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem5110_process: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "%s\n\n",in_szQuery);
		return -1;
	}
 	if (mysql_num_rows(loc_res)==0)	{
		mysql_free_result(loc_res);
	    ZzLOG(ALWAY, "daem5110_process: 처리할 자료가 없습니다.\n");
		return -100;
	}

	while((loc_row = mysql_fetch_row(loc_res))) 
	{
		select_cnt = getint(loc_row, 0);
	}

	mysql_free_result(loc_res);
	return select_cnt;
}




/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5110_process_init(int argc, char **argv)
{
	char stemp[128];
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("d_", "/logs/daemon"); 

    ZzLOG(ALWAY, "[daem5110]*****************프로그램 시작*****************\n");  

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
	if (!(con=db_connect_local("zangsi")))
	{
		ZzLOG(ERROR, "[daem5110]DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1); 
	}

	if (!(con_backup=db_connect_nodb("zangsi")))
	{
		ZzLOG(ERROR, "[daem5110]DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con_backup);
	   	return(-1); 
	}
		
    return (0);

arg_error:
    ZzLOG(ERROR, "usage : %s YYYYMMDD NN\n", argv[0]);
    ZzLOG(ERROR, "        YYYYMMDD : 처리일자(00000000:시스템일자)\n");

    ZzPRT(ERROR, "usage : %s YYYYMMDD NN\n", argv[0]);
    ZzPRT(ERROR, "        보낸 쪽지 삭제 (30일 이전)\n");
    ZzPRT(ERROR, "        YYYYMMDD : 처리일자(00000000:시스템일자)\n");
    return -1;
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daem5110_process_term()
{
    // DB close
	db_disconnect(con);
	db_disconnect(con_backup);	
	
    ZzLOG(ALWAY, "[daem5110]*****************프로그램 종료*****************\n");  

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5110_signal(int nSignal)
{
    daem5110_process_term();
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
	signal(SIGTERM, daem5110_signal);
	signal(SIGINT,  daem5110_signal);
	signal(SIGQUIT, daem5110_signal);
	signal(SIGKILL, daem5110_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daem5110_process_init(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daem5110_process();
		/* 프로그램 종료루틴 */                    
		daem5110_process_term();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
