/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5403.cc
 *         기능 : 관리자 상담글 일자별 집계 처리
 *         설명 : 
 *                
 *     설치위치 :
 *
 *       작성자 : LEE
 *       작성일 : 2005/01/24
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

int daem5403_process();
int daem5403_process_init(int argc, char **argv);
int daem5403_process_term();
void daem5403_signal(int nSignal);

MYSQL     *con;
MYSQL_RES *res;
MYSQL_ROW  row;

char   gproc_date [  8+1];	// 1일전 처리일자
char   gproc_date2 [  8+1];	// 2일전 처리일자
char   gproc_date3 [  8+1];	// 2일전 처리일자

//******************************************************************************
//* daem5403 main
//******************************************************************************
int daem5403_process()
{

	char szQuery[65535];  // query string

    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "[daem5403]관리자 상담글 집계 추가\n");  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");

	/*------------------------------------------------------------------------*/
	/* 최저 사용량   구하기                                                    */
	/*------------------------------------------------------------------------*/

	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery," replace into zangsi_bck.T_CONSULT_NUMBER                          "
					" select                                                           "
					" '%s' ,'D'                                                          "
					" ,sum( if( lcode ='001' and reply_yn = 'Y', 1 , 0 ) ) AS '001-1'  "
					" ,sum( if( lcode ='001' and quan_yn='Q' , 1 , 0 ) ) AS '001'                      "
					" ,sum( if( lcode ='002' and reply_yn = 'Y', 1 , 0 ) ) AS '002-1'  "
					" ,sum( if( lcode ='002' and quan_yn='Q' , 1 , 0 ) ) AS '002'                      "
					" ,sum( if( lcode ='003' and reply_yn = 'Y', 1 , 0 ) ) AS '003-1'  "
					" ,sum( if( lcode ='003' and quan_yn='Q' , 1 , 0 ) ) AS '003'                      "
					" ,sum( if( lcode ='004' and reply_yn = 'Y', 1 , 0 ) ) AS '004-1'  "
					" ,sum( if( lcode ='004' and quan_yn='Q' , 1 , 0 ) ) AS '004'                      "
					" ,sum( if( lcode ='005' and reply_yn = 'Y', 1 , 0 ) ) AS '005-1'  "
					" ,sum( if( lcode ='005' and quan_yn='Q' , 1 , 0 ) ) AS '005'                      "
					" ,sum( if( lcode ='006' and reply_yn = 'Y', 1 , 0 ) ) AS '006-1'  "
					" ,sum( if( lcode ='006' and quan_yn='Q' , 1 , 0 ) ) AS '006'                      "
					" ,sum( if( lcode ='007' and reply_yn = 'Y', 1 , 0 ) ) AS '007-1'  "
					" ,sum( if( lcode ='007' and quan_yn='Q' , 1 , 0 ) ) AS '007'                      "
					" ,sum( if( lcode ='008' and reply_yn = 'Y', 1 , 0 ) ) AS '008-1'  "
					" ,sum( if( lcode ='008' and quan_yn='Q' , 1 , 0 ) ) AS '008'                      "
					" ,sum( if( lcode ='009' and reply_yn = 'Y', 1 , 0 ) ) AS '009-1'  "
					" ,sum( if( lcode ='009' and quan_yn='Q' , 1 , 0 ) ) AS '009'                      "
					" ,sum( if( lcode ='010' and reply_yn = 'Y', 1 , 0 ) ) AS '010-1'  "
					" ,sum( if( lcode ='010' and quan_yn='Q' , 1 , 0 ) ) AS '010'                      "
					"  from zangsi.T_CONSULT_MAIN                                           "
					" where                                                            "
					" reg_date = '%s'                                                  "
					" and lcode in ('001', '002', '003', '004', '005', '006', '007', '008', '009', '010') "
					" group by reg_date ",gproc_date,gproc_date);


	ZzLOG(ALWAY, "[daem5403]%s\n",szQuery);
                    
    if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5403_process: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}


	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery," replace into zangsi_bck.T_CONSULT_NUMBER                          "
					" select                                                           "
					" '%s'  ,'W',                                                          "
					" 0,                               "            
					" ifnull(sum(case when lcode='001' and reply_yn ='N' then 1 end),'0') as R001, "
					" 0,                               "
					" ifnull(sum(case when lcode='002' and reply_yn ='N' then 1 end),'0') as R002, "
					" 0,                               "      
					" ifnull(sum(case when lcode='003' and reply_yn ='N' then 1 end),'0') as R003, "         
					" 0,                               " 					
					" ifnull(sum(case when lcode='004' and reply_yn ='N' then 1 end),'0') as R004, "   
					" 0,                               "            
					" ifnull(sum(case when lcode='005' and reply_yn ='N' then 1 end),'0') as R005, "   
					" 0,                               "            
					" ifnull(sum(case when lcode='006' and reply_yn ='N' then 1 end),'0') as R006, "   
					" 0,                               "            
					" ifnull(sum(case when lcode='007' and reply_yn ='N' then 1 end),'0') as R007, "   
					" 0,                               "            
					" ifnull(sum(case when lcode='008' and reply_yn ='N' then 1 end),'0') as R008, "   
					" 0,                               "            
					" ifnull(sum(case when lcode='009' and reply_yn ='N' then 1 end),'0') as R009, "   
					" 0,			 					   "           
					" ifnull(sum(case when lcode='010' and reply_yn ='N' then 1 end),'0') as R010 "   
					" from zangsi.T_CONSULT_MAIN                                                        "  
					" WHERE reg_date <= '%s'  and quan_yn='Q' and "
					, gproc_date,gproc_date );
	strcat(szQuery, " reg_date >= date_format(date_add( now(), interval -7 day ),'%Y%m%d') ");
					


	ZzLOG(ALWAY, "[daem5403]%s\n",szQuery);
                    
    if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5403_process: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}

	
	int value1 ,value2	,value3	,value4	,value5	,value6	,value7	,value8	,value9	,value10;


	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery," select                      "
					" ifnull(sum(case when a.lcode='001' then 1 end),'0') - b.lcode_001_y "            
					" ,ifnull(sum(case when a.lcode='002' then 1 end),'0') - b.lcode_002_y "
					" ,ifnull(sum(case when a.lcode='003' then 1 end),'0') - b.lcode_003_y "
					" ,ifnull(sum(case when a.lcode='004' then 1 end),'0') - b.lcode_004_y "
					" ,ifnull(sum(case when a.lcode='005' then 1 end),'0') - b.lcode_005_y "
					" ,ifnull(sum(case when a.lcode='006' then 1 end),'0') - b.lcode_006_y "
					" ,ifnull(sum(case when a.lcode='007' then 1 end),'0') - b.lcode_007_y "
					" ,ifnull(sum(case when a.lcode='008' then 1 end),'0') - b.lcode_008_y "
					" ,ifnull(sum(case when a.lcode='009' then 1 end),'0') - b.lcode_009_y "
					" ,ifnull(sum(case when a.lcode='010' then 1 end),'0') - b.lcode_010_y "
					" from zangsi.T_CONSULT_MAIN a, zangsi_bck.T_CONSULT_NUMBER b "
					" WHERE  a.quan_yn='A'  and b.con_flag = 'D' " );
					
	strcat( szQuery," and a.reg_date = date_format(date_add( now(), interval -1 day ),'%Y%m%d') "
					" and b.con_date = date_format(date_add( now(), interval -1 day ),'%Y%m%d') " );
					


	ZzLOG(ALWAY, "[daem5403]%s\n",szQuery);
                    
    if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5403_process: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}

	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "[daem5403]sysdate: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "[daem5403]sysdate: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
	row = mysql_fetch_row(res);
	
	value1 = getint(row,0);	
	value2 = getint(row,1);
	value3 = getint(row,2);
	value4 = getint(row,3);
	value5 = getint(row,4);
	value6 = getint(row,5);
	value7 = getint(row,6);
	value8 = getint(row,7);
	value9 = getint(row,8);
	value10 = getint(row,9);
	mysql_free_result(res);
	
	




	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery," update zangsi_bck.T_CONSULT_NUMBER    set                     "
					" lcode_001_y = %d "            
					" ,lcode_002_y = %d "
					" ,lcode_003_y = %d "
					" ,lcode_004_y = %d "
					" ,lcode_005_y = %d "
					" ,lcode_006_y = %d "
					" ,lcode_007_y = %d "
					" ,lcode_008_y = %d "
					" ,lcode_009_y = %d "
					" ,lcode_010_y = %d "
					" where  con_date = '%s' and con_flag = 'W' "
					,value1,value2,value3,value4,value5,value6,value7,value8,value9,value10,gproc_date2 );	
				


	ZzLOG(ALWAY, "[daem5403]%s\n",szQuery);
                    
    if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5403_process: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}


	return 0;
}

/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5403_get_sysdate()
{	char szQuery[1000];		// query string
	char sztemp [100];      // query temp


	memset(szQuery, 0x00, sizeof(szQuery));
	strcpy(szQuery, "SELECT date_format(date_add(now(),INTERVAL - 1 DAY),'%Y%m%d') "
					" ,date_format(date_add(now(),INTERVAL - 2 DAY),'%Y%m%d')"
					" ,date_format(date_add(now(),INTERVAL - 3 DAY),'%Y%m%d')");

	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "[daem5403]sysdate: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "[daem5403]sysdate: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "[daem5403]sysdate: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
	row = mysql_fetch_row(res);
	memset(gproc_date, 0x00, sizeof(gproc_date));
	memset(gproc_date2, 0x00, sizeof(gproc_date2));
	memset(gproc_date3, 0x00, sizeof(gproc_date3));

	strcpy(gproc_date,   getstr(row, 0));
	strcpy(gproc_date2,   getstr(row, 1));
	strcpy(gproc_date3,   getstr(row, 2));
	
	mysql_free_result(res);
	
	ZzLOG(ERROR, "[daem5403] 관리자 상듬글 집계 처리 [ %s ] \n",gproc_date);

	

	return 0;
}


/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5403_process_init(int argc, char **argv)
{
	int ret=0;
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("daem5403", "/logs/daemon"); 

    ZzLOG(ALWAY, "[daem5403]***************프로그램 시작***************\n");  

    if (argc != 2) 
    {
    	ZzLOG(ALWAY, "Usage : daem5403 처리날짜 또는 00000000 \n");  
    	return (-1);
    }

	/* 처리일자 */
	memset(gproc_date, 0x00, sizeof(gproc_date));
	strcpy(gproc_date, argv[1]);
	


	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(con=db_connect_nodb("")))
	{
		ZzLOG(ERROR, "[daem5403]DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1); 
	}
	/* 처리일자 */

	if( strcmp(gproc_date,"00000000") == 0 )
	{
		ret=daem5403_get_sysdate();
		if (ret < 0)
		{
		    ZzLOG(ERROR, "daem5403_get_sysdate 오류...\n");
			db_disconnect(con);
			return -1;
		}

	}

    return (0);

}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daem5403_process_term()
{
    // DB close
	db_disconnect(con);
    ZzLOG(ALWAY, "[daem5403]***************프로그램 종료***************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5403_signal(int nSignal)
{
    daem5403_process_term();
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
	signal(SIGTERM, daem5403_signal);
	signal(SIGINT,  daem5403_signal);
	signal(SIGQUIT, daem5403_signal);
	signal(SIGKILL, daem5403_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daem5403_process_init(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daem5403_process();
		/* 프로그램 종료루틴 */                    
		daem5403_process_term();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
