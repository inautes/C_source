/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5006
 *         기능 : vir_id2 정리
 *         설명 : 1. 프로세스 처리 예정시간 : 05:10에 수행.
 *                2. T_CONTENTS_VIR_ID, T_CONTENTS_VIR_ID2 데이터 맞춤. 
 *     설치위치 : CMD01
 *
 *       작성자 : HCS
 *       작성일 : 2008/11/20
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

int    daem5006_init_process(int argc, char **argv);
int    daem5006_main_process();
int    daem5006_delete_vir_id2();
int    daem5006_term_process();
void   daem5006_signal(int nSignal);

MYSQL     *con;
MYSQL_RES *res;
MYSQL_ROW  row;

char   gproc_date [  8+1];	// 처리일자

//******************************************************************************
//* daem5006 main
//******************************************************************************
int daem5006_main_process()
{
	char szQuery[1000];  // query string
	int i, j, count;
	int nRowcnt=0;
	int ret = 0;

    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "daem5006_main_process: gproc_date[%s]\n", gproc_date);  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*  - T_CONTENTS_ADMDEL 기준으로 처리                                     */
	/*------------------------------------------------------------------------*/

	//----------------------------------------------------------------------
	// 트렌젝션시작
	//----------------------------------------------------------------------
	if (tran_begin(con)!=0) {
	    ZzLOG(ERROR, "daem5006_main_process: tran_begin: 테이베이스 오류입니다.\n");  
		ZzLOG(ERROR, "daem5006_main_process: [%d](%s)\n", mysql_errno(con), mysql_error(con));
		return -1;
	}

	ret = daem5006_delete_vir_id2();

	if (ret < 0) 
	{
	    ZzLOG(ERROR, "[daem5006_main_process] daem5006_delete_vir_id2() error...\n");
		tran_rollback(con);
		tran_end(con);
		return -1;	
	}

	//----------------------------------------------------------------------
	// commit
	//----------------------------------------------------------------------
	if (commit(con)!=0){
	    ZzLOG(ERROR, "[daem5006_main_process] commit 1 error...\n");
		ZzLOG(ERROR, "[daem5006_main_process] [%d](%s)\n", mysql_errno(con), mysql_error(con));
		tran_rollback(con);
		tran_end(con);
		return -1;	
	}
	tran_end(con);

    ZzLOG(ALWAY, "daem5006_main_process: 정상적으로 처리 완료!!!\n");
	return  0;
}
//******************************************************************************
//* daem5006_delete_vir_id2
//*-----------------------------------------------------------------------------
//* 설명 : T_CONTENTS_VIR_ID2 의 vir_id와 T_CONTENTS_VIR_ID2의 vir_id를 비교하여
//*			데이터 싱크 맞춤
//******************************************************************************
int daem5006_delete_vir_id2()
{
	char szQuery[1000];
	memset (szQuery, 0x00, sizeof(szQuery));
	

	MYSQL_RES *s_res;
	MYSQL_ROW s_row;
	MYSQL_RES *res;
	MYSQL_ROW row;

	char szSectCode[2+1];
	memset(szSectCode, 0x00, sizeof(szSectCode));	
	
	memset (szQuery, 0x00, sizeof(szQuery));
	strcpy(szQuery, " select minor_code from zangsi.T_MINOR_CODE where major_code = '01'  ; /* daem5006 */   ");

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5006: [ %s ] \n" , szQuery);
		ZzLOG(ERROR, "daem5006: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
    }
	if (!(s_res = mysql_store_result(con))) 
	{
	    ZzLOG(ERROR, "daem5006: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem5006: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		mysql_free_result(s_res);
		return -1;
	}
 	if (mysql_num_rows(s_res)==0)	
 	{
	    ZzLOG(ERROR, "daem5006: T_CONTENTS_VIR_ID2 vir_id mysql_num_rows error...\n");
		mysql_free_result(s_res);
		return -1;
	}
	while(s_row = mysql_fetch_row(s_res))
	{
		memset(szSectCode, 0x00, sizeof(szSectCode));
		strcpy(szSectCode, getstr(s_row,0));
		
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " select count(*) from zangsi.T_CONTENTS_VIR_ID2 where sect_code = '%s' /* daem5006 */ ",szSectCode);

		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5006: [ %s ] \n" , szQuery);
			ZzLOG(ERROR, "daem5006: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			mysql_free_result(s_res);
			return -1;
	    }
		if (!(res = mysql_store_result(con))) 
		{
		    ZzLOG(ERROR, "daem5006: mysql_store_result error...\n");
			ZzLOG(ERROR, "daem5006: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			mysql_free_result(s_res);
			return -1;
		}
	 	if (mysql_num_rows(res)==0)	
	 	{
		    ZzLOG(ERROR, "daem5006: T_CONTENTS_VIR_ID2 vir_id mysql_num_rows error...\n");
			mysql_free_result(s_res);
			mysql_free_result(res);
			return -1;
		}
		row = mysql_fetch_row(res);
		unsigned long ulVirCount = (unsigned long)getnum(row,0);
		mysql_free_result(res);
	    
	    ZzLOG(ALWAY,"daem5006 : Vir_ID2 Count [ %ul ]\n" , ulVirCount );
	    if( ulVirCount  > 1000  )
    	{
    		memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " select vir_id from zangsi.T_CONTENTS_VIR_ID2 where sect_code = '%s' "
							 " order by vir_id desc  limit 1001 ,1  ; /* daem5006 */ "
							 , szSectCode);
			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daem5006: [ %s ] \n" , szQuery);
				ZzLOG(ERROR, "daem5006: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				mysql_free_result(s_res);
				return -1;
		    }
			if (!(res = mysql_store_result(con))) 
			{
			    ZzLOG(ERROR, "daem5006: mysql_store_result error...\n");
				ZzLOG(ERROR, "daem5006: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				mysql_free_result(s_res);
				return -1;
			}
			row = mysql_fetch_row(res);
			unsigned long ulSectVirId = (unsigned long)getnum(row,0);
			mysql_free_result(res);

			memset(szQuery, 0x00, sizeof(szQuery));		
			
			sprintf(szQuery, " delete from zangsi.T_CONTENTS_VIR_ID2 where vir_id < %ld and sect_code = '%s'  ; /* daem5006 */" 
							 , ulSectVirId, szSectCode);
			
			ZzLOG(ALWAY, "daem5006: Query = [%s]\n", szQuery);
	
			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daem5006: [ %s ] \n" , szQuery);
				ZzLOG(ERROR, "daem5006: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				mysql_free_result(s_res);
				return -1;
		    }	
    	}
    	
	}
	mysql_free_result(s_res);
}

/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5006_get_sysdate()
{
	char szQuery[1000];		// query string
	char sztemp [100];      // query temp

//	if (strcmp(gproc_date, "00000000") == 0)
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d')  ; /* daem5006 */ ");
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
	strcpy(gproc_date,   getstr(row, 0));

	mysql_free_result(res);

	return 0;
}


/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5006_init_process(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("d_", "/logs/daemon"); 

    ZzLOG(ALWAY, "[daem5006]*****************프로그램 시작*****************\n");  
    ZzLOG(ALWAY, "[daem5006] VIR_ID2 데이터 맞추기\n");

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

	/* 처리일자 */
	memset(gproc_date, 0x00, sizeof(gproc_date));
	strcpy(gproc_date, argv[1]);
	ret=daem5006_get_sysdate();
	if (ret < 0){
		db_disconnect(con);
		return -1;
	}

    return (0);


arg_error:
    ZzLOG(ERROR, "usage : %s YYYYMMDD XXXXX\n", argv[0]);
    ZzLOG(ERROR, "        YYYYMMDD(처리일자)-> '00000000'인경우 system일자로처리\n");

    ZzPRT(ERROR, "usage : %s YYYYMMDD XXXXX\n", argv[0]);
    ZzPRT(ERROR, "        YYYYMMDD(처리일자)-> '00000000'인경우 system일자로처리\n");
    return -1;
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daem5006_term_process()
{
    // DB close
	db_disconnect(con);	
    ZzLOG(ALWAY, "[daem5006]*****************프로그램 종료*****************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5006_signal(int nSignal)
{
    daem5006_term_process();
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
/*
	signal(SIGTERM, daem5006_signal);
	signal(SIGINT,  daem5006_signal);
	signal(SIGQUIT, daem5006_signal);
	signal(SIGKILL, daem5006_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
*/
	if ( daem5006_init_process(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daem5006_main_process();
		/* 프로그램 종료루틴 */
	}
	daem5006_term_process();
	return(0);
}
/*****************************************************************************
*  End of file...
*****************************************************************************/
