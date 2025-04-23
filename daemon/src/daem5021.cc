/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5021
 *         기능 : 내디스크 용량 계산.
 *         설명 : 1. 프로세스 처리 예정시간 - 04:00에 수행.
 *             
 *     설치위치 : CMD01
 *
 *       작성자 : LEE
 *       작성일 : 2005/12/05
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

int daem5021_process();
int daem5021_process_db();
int daem5021_process_init(int argc, char **argv);
int daem5021_process_term();
void daem5021_signal(int nSignal);

MYSQL     *con, *con2;
MYSQL_RES *res;
MYSQL_ROW  row;

char   guser_id   [ 12+1];	// 사용자ID
char   gserver_id [  5+1];	// 서버ID
char   gproc_date [  8+1];	// 처리일자
char   gend_date  [  8+1];	// 삭제예정일
char   ged_date   [  8+1];	// 삭제예정일
double gdisk_size        ;	// 디스크크기
double gdisk_size_bef    ;	// 디스크크기(전일)
//******************************************************************************
//* daem5021 main
//******************************************************************************
int daem5021_process()
{
	char szQuery[1000];  // query string
	int i, j, count;
	int st_min, ed_min;
	int nRowcnt=0;

    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "gproc_date : [%s]\n", gproc_date);  
    ZzLOG(ALWAY, "gend_date  : [%s]\n", gend_date );  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");
	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/
	count = 0;
	while(1) {
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "SELECT user_id , disk_size"
                        "   FROM zangsi.T_MYDATA_INFO"
                        "  WHERE end_date   = '%s'   "
                        "  LIMIT 100 ; /* daem5021 */ "
                        , gend_date
                        );
		if (mysql_query(con, szQuery)){
		    ZzLOG(ERROR, "daem5021_process: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
	
		if (!(res = mysql_store_result(con))) {
		    ZzLOG(ERROR, "daem5021_process: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
	 	if (mysql_num_rows(res)==0)	{
			mysql_free_result(res);
		    ZzLOG(ALWAY, "daem5021_process: 처리할 자료가 없습니다.\n");
			break;
		}

	    ZzLOG(ALWAY, "daem5021_process: select_cnt(%d)\n", mysql_num_rows(res));
	
		nRowcnt = 0;
		while((row = mysql_fetch_row(res))) {
			memset(&guser_id      , 0x00, sizeof(guser_id      ));
			memset(&gdisk_size_bef, 0x00, sizeof(gdisk_size_bef));

			strcpy(guser_id,   getstr(row, 0));
			gdisk_size_bef   = getnum(row, 1);

			if (daem5021_process_db() != 0)
			{
				mysql_free_result(res);
				return -1;
			}
		}
		mysql_free_result(res);
	}
	return 0;
}

//******************************************************************************
//* daem5021 db 처리로직
//******************************************************************************
int daem5021_process_db()
{
	char szQuery[1000];		// query string
	char szSysQuery [612];	// system commend
	char szfullfile [612];	// file full name
	int  nRowcnt;			// select row count
	int  ret;
	MYSQL_RES *res2;
	MYSQL_ROW  row2;
	
	//--------------------------------------------------------------------------
	// Update Disk 사용량 Update
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT ifnull(sum(buy_size),0)+31457280 as disk_size"
	                 "     , min(ed_date) as ed_date "
	                 "  FROM zangsi.T_MYDISK_PAYMENT "
	                 " WHERE user_id  = '%s'"
	                 "   AND st_date <= '%s'"
	                 "   AND ed_date >= '%s' ; /* daem5021 */ "
	                 , guser_id
	                 , gproc_date
	                 , gproc_date
	                 );

	if (mysql_query(con2, szQuery)){
	    ZzLOG(ERROR, "sysdate: mysql_query error...\n");
	    goto daem5021_process_db_err;
	}
	
	if (!(res2 = mysql_store_result(con2)))
	{
	    ZzLOG(ERROR, "sysdate: mysql_store_result error...\n");
	    goto daem5021_process_db_err;
	}
 	if (mysql_num_rows(res2)==0)
 	{
	    ZzLOG(ERROR, "sysdate: mysql_num_rows error...\n");
	    goto daem5021_process_db_err;
	}
	row2 = mysql_fetch_row(res2);
	memset(&ged_date  , 0x00, sizeof(ged_date  ));
	memset(&gdisk_size, 0x00, sizeof(gdisk_size));

	gdisk_size = getnum(row2, 0);
	strcpy(ged_date, getstr(row2, 1));
	
	mysql_free_result(res2);
	//--------------------------------------------------------------------------
	// 트렌젝션시작
	//--------------------------------------------------------------------------
	if (tran_begin(con2)!=0) {
	    ZzLOG(ERROR, "tran_begin: 테이베이스 오류입니다.\n");  
	    return -1;
	}

	//--------------------------------------------------------------------------
	// 내디스크 사이즈조정
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "UPDATE zangsi.T_MYDATA_INFO"
	                 "   SET disk_size  = %15.0f"
	                 "     , end_date   = '%s' "
	                 " WHERE user_id    = '%s' ; /* daem5021 */ "
	                 ,gdisk_size
	                 ,ged_date
	                 ,guser_id
	                 );
	if (mysql_query(con2, szQuery)){
	    ZzLOG(ERROR, "process_db: UPDATE zangsi.T_MYDISK_INFO error...\n");
	    goto daem5021_process_db_err;
    }

	//--------------------------------------------------------------------------
	// 서버 디스크 사용량조정
	//--------------------------------------------------------------------------
	// 서버사용량은 컨텐츠 삭제시 UPDATE 되어짐
	//--------------------------------------------------------------------------
	/*
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "UPDATE zangsi.T_SERVER_INFO"
	                 "   SET disk_use  = disk_use - %15.0f"
	                 " WHERE server_id = '%s' "
	                 ,gdisk_size_bef - gdisk_size
	                 ,gserver_id
	                 );
	if (mysql_query(con2, szQuery)){
	    ZzLOG(ERROR, "process_db: UPDATE zangsi.T_SERVER_INFO error...\n");
	    goto daem5021_process_db_err;
    }
	*/


	if (tran_commit(con2)!=0)
	{
	    ZzLOG(ERROR, "process_db: tran_commit error...\n");
	    goto daem5021_process_db_err;
	}
	//--------------------------------------------------------------------------
	// 트렌젝션종료
	//--------------------------------------------------------------------------
	if (tran_end(con2)!=0)	{
	    ZzLOG(ERROR, "process_db: tran_end 테이베이스 오류입니다.\n");  
	    goto daem5021_process_db_err;
	}

	return 0;

daem5021_process_db_err:
	ZzLOG(ERROR, "process_db: [%d](%s)",mysql_errno(con2), mysql_error(con2));
	if (res2) mysql_free_result(res2);
	tran_rollback(con2);
	tran_end(con2);
	return -1;	
}

/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5021_get_sysdate()
{
	char szQuery[1000];		// query string
	char sztemp [100];      // query temp

	if (strcmp(gproc_date, "00000000") == 0)
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d')");
		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -1 DAY),'%Y%m%d') ; /* daem5021 */ ");
	}
	else
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		strcpy(szQuery, "SELECT date_format('");
		strcat(szQuery, gproc_date);
		strcat(szQuery, "','%Y%m%d')");
		strcat(szQuery, "     , date_format(date_add('");
		strcat(szQuery, gproc_date);
		strcat(szQuery, "', INTERVAL -1 DAY),'%Y%m%d') ; /* daem5021 */ ");
	}

	if (mysql_query(con, szQuery)){
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
	memset(gproc_date, 0x00, sizeof(gproc_date));
	memset(gend_date , 0x00, sizeof(gend_date ));

	strcpy(gproc_date,   getstr(row, 0));
	strcpy(gend_date ,   getstr(row, 1));
	
	mysql_free_result(res);

	return 0;
}


/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5021_process_init(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("d_", "/logs/daemon"); 

    ZzLOG(ALWAY, "[daem5021]***************프로그램 시작***************\n");  

    // 파라미터 값 설정 및 초기화
    if (argc != 2){
    	goto arg_error;
    }
	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(con=db_connect("zangsi")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1); 
	}
	if (!(con2=db_connect("zangsi")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con2);
	   	return(-1); 
	}


	/* 처리일자 */
	memset(gproc_date, 0x00, sizeof(gproc_date));
	strcpy(gproc_date, argv[1]);
	ret=daem5021_get_sysdate();
	if (ret < 0){
		db_disconnect(con);
		db_disconnect(con2);
		return -1;
	}

    return (0);


arg_error:
    ZzLOG(ERROR, "usage : %s YYYYMMDD\n", argv[0]);
    ZzLOG(ERROR, "        YYYYMMDD(처리일자)-> '00000000'인경우 system일자로처리\n");

    ZzPRT(ERROR, "usage : %s YYYYMMDD \n", argv[0]);
    ZzPRT(ERROR, "        YYYYMMDD(처리일자)-> '00000000'인경우 system일자로처리\n");
    return -1;
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daem5021_process_term()
{
    // DB close
	db_disconnect(con);
	db_disconnect(con2);
    ZzLOG(ALWAY, "[daem5021]***************프로그램 종료***************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5021_signal(int nSignal)
{
    daem5021_process_term();
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
	signal(SIGTERM, daem5021_signal);
	signal(SIGINT,  daem5021_signal);
	signal(SIGQUIT, daem5021_signal);
	signal(SIGKILL, daem5021_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daem5021_process_init(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daem5021_process();
		/* 프로그램 종료루틴 */                    
		daem5021_process_term();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
