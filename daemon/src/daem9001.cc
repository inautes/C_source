/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem90011.cc
 *         기능 : 추천인 이벤트 순위 집계
 *         설명 : 1일 1회 작업한다.
 *                SYSTEM  (00000000) => sysdate - 1일 처리하며,
 *                직접입력(yyyymmdd) => yyyymmdd를  처리한다.
 *       작성자 : HCS
 *       작성일 : 2010/05/01
 *     수정이력 : 
 *				  
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

int daem9001_init_process(int argc, char **argv);
int daem9001_main_process();
int daem9001_term_process();
int daem9001_insert_deal();
int daem9001_get_sysdate();
void daem9001_signal(int nSignal);

MYSQL     *bck_con;
MYSQL     *con;


char   gsys_date  [  8+1];	//처리일자(sysdate)
char   gproc_date [  8+1];	//처리일자(sysdate-1)
char   gproc_yymm [  6+1];	//처리년월
char   grank_yymm [  6+1];  //순위 매기는 달. 처리날짜 -1
char   greg_date  [  8+1];	//등록일
char   greg_time  [  6+1];	//등록시간
//******************************************************************************
//* daem9001 main
//******************************************************************************
int daem9001_main_process()
{


	//--------------------------------------------------------------------------
	// 트렌젝션시작
	//--------------------------------------------------------------------------
	if (tran_begin(con)!=0) {
	    ZzLOG(ERROR, "tran_begin: 테이베이스 오류입니다.\n");  
		ZzLOG(ERROR, "daem9001_main_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
	}

	// 판매 집계처리
	if (daem9001_insert_deal() != 0)
		goto daem9001_main_process_err;

	if (tran_commit(con)!=0){
	    ZzLOG(ERROR, "daem9001_main_process: tran_commit error...\n");
		ZzLOG(ERROR, "daem9001_main_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    goto daem9001_main_process_err;
	}
	
	tran_end(con);

	return (0);

daem9001_main_process_err:
	tran_rollback(con);
	tran_end(con);
    return -1;
}

//******************************************************************************
//* daem9001_insert_deal()
//* 일일 사용자별 거래집계처리
//******************************************************************************
int daem9001_insert_deal()
{
	#ifdef _DEBUG_
	printf("daem9001_insert_deal\n");
	#endif
	
	char szQuery[10000];		// query string
	int ret=0;
	int nRowcnt = 0;
	int nAddRowcnt = 0;
	
	MYSQL_RES* res;
	MYSQL_ROW  row;

	MYSQL_RES* res2;
	MYSQL_ROW  row2;

	MYSQL_RES* t_res;
	MYSQL_ROW  t_row;
	
	
	//--------------------------------------------------------------------------
	// 템프 테이블 생성
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	strcpy(szQuery, " CREATE TEMPORARY TABLE zangsi.T_USER_RCMD_RANK_TEMP "
					 " select user_id, rcmd_cnt, y_rank, rank_date "
					 " from zangsi.T_USER_RCMD_RANK ");

	ZzLOG(ALWAY, "szQuery=[%s]\n\n", szQuery);
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem9001_insert_deal: mysql_query error...\n");
		ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
	    return -1;
    }
	
	
	//--------------------------------------------------------------------------
	// 해당일자의 자료를 생성한다
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select rcmd_user, count(*) "
					 " from zangsi.T_USER_RCMD_CHARGE "
					 " where substring(reg_date, 1, 6) = '%s' and cnl_yn = 'N' "
					 " group by rcmd_user "
					 , gproc_yymm);
	ZzLOG(ALWAY, "szQuery=[%s]\n\n", szQuery);
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem9001_insert_deal: mysql_query error...\n");
		ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
	    return -1;
    }
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daem9001_insert_deal: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
	    return -1;
    }
	if (mysql_num_rows(res)==0)
	{
		mysql_free_result(res);
	    ZzLOG(ALWAY, "daem9001_insert_deal: 결과 없음.\n");
		ZzLOG(ALWAY, "daem9001_insert_deal: (%s)\n", szQuery);
    }
    else
	{    	
	    while(row = mysql_fetch_row(res))
	    {
		    char szUserId[12+1];
		    memset(szUserId, 0x00, sizeof(szUserId));
		    strcpy(szUserId, getstr(row,0));
		    
		    int nRcmdCnt = 0;
		    nRcmdCnt = (unsigned long)getnum(row,1);
		    
			
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " select user_id from zangsi.T_USER_RCMD_RANK_TEMP where user_id = '%s' and rank_date = '%s' ", szUserId, gproc_yymm);
			ZzLOG(ALWAY, "szQuery=[%s]\n\n", szQuery);
			if (mysql_query(con, szQuery))
			{
				mysql_free_result(res);
			    ZzLOG(ERROR, "daem9001_insert_deal: mysql_query error...\n");
				ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
			    return -1;
		    }
			if (!(res2 = mysql_store_result(con)))
			{
				mysql_free_result(res);
			    ZzLOG(ERROR, "daem9001_insert_deal: mysql_store_result error...\n");
				ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
			    return -1;
		    }
			if (mysql_num_rows(res2)==0)
			{
				mysql_free_result(res2);
				memset (szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " insert into zangsi.T_USER_RCMD_RANK_TEMP "
								 " values "
								 " ('%s', %d, 0, '%s') "
								 , szUserId
								 , nRcmdCnt
								 , gproc_yymm);
				ZzLOG(ALWAY, "szQuery=[%s]\n\n", szQuery);
				if (mysql_query(con, szQuery))
				{
					mysql_free_result(res);
				    ZzLOG(ERROR, "daem9001_insert_deal: mysql_query error...\n");
					ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
				    return -1;
			    }
		    }
		    else
		    {
		    	mysql_free_result(res2);
				
				memset (szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " update zangsi.T_USER_RCMD_RANK_TEMP "
								 " set rcmd_cnt = %d "
								 " where user_id = '%s' and rank_date = '%s' "
								 , nRcmdCnt
								 , szUserId
								 , gproc_yymm);
				ZzLOG(ALWAY, "szQuery=[%s]\n\n", szQuery);
				if (mysql_query(con, szQuery))
				{
					mysql_free_result(res);
				    ZzLOG(ERROR, "daem9001_insert_deal: mysql_query error...\n");
					ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
				    return -1;
			    }
		    }
		    nRowcnt++;
		}
		mysql_free_result(res);
	}
	ZzLOG(ALWAY, "daem9001_insert_deal: %d 건 집계 완료.\n", nRowcnt);
	
	
	//--------------------------------------------------------------------------
	//추가 데이터 집계
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select user_id, rcmd_cnt "
					 " from zangsi_bck.T_USER_RCMD_RANK_ADD "
					 " where rank_date = '%s' and rcmd_cnt > 0 "
					 , gproc_yymm);
	ZzLOG(ALWAY, "szQuery=[%s]\n\n", szQuery);
	if (mysql_query(bck_con, szQuery))
	{
	    ZzLOG(ERROR, "daem9001_insert_deal: mysql_query error...\n");
		ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (!(res = mysql_store_result(bck_con)))
	{
	    ZzLOG(ERROR, "daem9001_insert_deal: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(bck_con), mysql_error(bck_con), szQuery);
	    return -1;
    }
	if (mysql_num_rows(res)==0)
	{
	    ZzLOG(ALWAY, "daem9001_insert_deal: 추가할 데이터 없음.\n");
	    mysql_free_result(res);
    }
    else
    {	
	    while(row = mysql_fetch_row(res))
	    {
	    	char szAddUser[12+1];
	    	memset(szAddUser, 0x00, sizeof(szAddUser));
	    	strcpy(szAddUser, getstr(row,0));
			
			int nAddCnt = 0;
			nAddCnt = (int)getint(row,1);
			
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " select user_id from zangsi.T_USER_RCMD_RANK_TEMP where user_id = '%s' and rank_date = '%s' ", szAddUser, gproc_yymm);
			ZzLOG(ALWAY, "szQuery=[%s]\n\n", szQuery);
			if(mysql_query(con, szQuery))
			{
				mysql_free_result(res);
			    ZzLOG(ERROR, "daem9001_insert_deal: mysql_query error...\n");
				ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
			    return -1;
		    }
			if (!(t_res = mysql_store_result(con)))
			{
				mysql_free_result(res);
			    ZzLOG(ERROR, "daem9001_insert_deal: mysql_store_result error...\n");
				ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
			    return -1;
		    }
			if (mysql_num_rows(t_res)==0)
			{
				mysql_free_result(t_res);
				memset (szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " insert into zangsi.T_USER_RCMD_RANK_TEMP "
								 " values "
								 " ('%s', %d, 0, '%s') "
								 , szAddUser
								 , nAddCnt
								 , gproc_yymm);
				ZzLOG(ALWAY, "szQuery=[%s]\n\n", szQuery);
				if (mysql_query(con, szQuery))
				{
					mysql_free_result(res);
				    ZzLOG(ERROR, "daem9001_insert_deal: mysql_query error...\n");
					ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
				    return -1;
			    }
		    }
			else
			{
				mysql_free_result(t_res);
				memset (szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " update zangsi.T_USER_RCMD_RANK_TEMP "
								 " set rcmd_cnt = rcmd_cnt + %d "
								 " where user_id = '%s' and rank_date = '%s' "
								 , nAddCnt
								 , szAddUser
								 , gproc_yymm);
				ZzLOG(ALWAY, "szQuery=[%s]\n\n", szQuery);
				if(mysql_query(con, szQuery))
				{
					mysql_free_result(res);
				    ZzLOG(ERROR, "daem9001_insert_deal: mysql_query error...\n");
					ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
				    return -1;
			    }
			}
			nAddRowcnt++;
		}
		mysql_free_result(res);
	}
	
	ZzLOG(ALWAY, "daem9001_insert_deal: %d 건 추가 집계 완료.\n", nAddRowcnt);
	
	//-------------------------------------------------------
	//전일 순위 집계
	//-------------------------------------------------------
	int nYrank = 0;
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select user_id "
					 " from zangsi.T_USER_RCMD_RANK "
					 " where rank_date = '%s' "
					 " order by rcmd_cnt desc "
					 , grank_yymm);
	ZzLOG(ALWAY, "szQuery=[%s]\n\n", szQuery);
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem9001_insert_deal: mysql_query error...\n");
		ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
	    return -1;
    }
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daem9001_insert_deal: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
	    return -1;
    }
	if (mysql_num_rows(res) == 0)
	{
	    ZzLOG(ERROR, "daem9001_insert_deal: 결과 없음.\n");
		ZzLOG(ERROR, "daem9001_insert_deal: (%s)\n", szQuery);
		mysql_free_result(res);
    }
    else
	{    	
	    while(row = mysql_fetch_row(res))
		{
			char szUserId[12+1];
			memset(szUserId, 0x00, sizeof(szUserId));
			strcpy(szUserId, getstr(row, 0));
			nYrank++;
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update zangsi.T_USER_RCMD_RANK_TEMP "
							 " set y_rank = %d "
							 " where user_id = '%s' and rank_date = '%s' "
							 , nYrank
							 , szUserId
							 , grank_yymm);
			ZzLOG(ALWAY, "szQuery=[%s]\n\n", szQuery);
			if (mysql_query(con, szQuery))
			{
				mysql_free_result(res);
			    ZzLOG(ERROR, "daem9001_insert_deal: mysql_query error...\n");
				ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
			    return -1;
		    }
		}	
		mysql_free_result(res);
	}	
	
	
	//-------------------------------------------------------
	// 템프 정보 입력
	//-------------------------------------------------------
	
	int nRealCnt = 0;
	memset (szQuery, 0x00, sizeof(szQuery));
	strcpy(szQuery, " select user_id, rcmd_cnt, y_rank, rank_date "
					 " from zangsi.T_USER_RCMD_RANK_TEMP ");

	ZzLOG(ALWAY, "szQuery=[%s]\n\n", szQuery);
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem9001_insert_deal: mysql_query error...\n");
		ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
	    return -1;
    }
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "daem9001_insert_deal: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
	    return -1;
    }
	if (mysql_num_rows(res)==0)
	{
	    ZzLOG(ERROR, "daem9001_insert_deal: 결과 없음.\n");
		ZzLOG(ERROR, "daem9001_insert_deal: (%s)\n", szQuery);
	    return -1;
    }
    while(row = mysql_fetch_row(res))
    {
    	char szUserId[12+1];
    	memset(szUserId, 0x00, sizeof(szUserId));
		strcpy(szUserId, getstr(row, 0));
		
		int nRcmdCnt = 0;
		nRcmdCnt = (int)getint(row,1);
		
		int nYrank = 0;
		nYrank = (int)getint(row,2);
		
		char szRankDate[8+1];
    	memset(szRankDate, 0x00, sizeof(szRankDate));
		strcpy(szRankDate, getstr(row, 3));
		
		    	
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " select user_id from zangsi.T_USER_RCMD_RANK where user_id = '%s' and rank_date = '%s' ", szUserId, szRankDate);
		ZzLOG(ALWAY, "szQuery=[%s]\n\n", szQuery);
		if(mysql_query(con, szQuery))
		{
			mysql_free_result(res);
		    ZzLOG(ERROR, "daem9001_insert_deal: mysql_query error...\n");
			ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		    return -1;
	    }
		if (!(t_res = mysql_store_result(con)))
		{
			mysql_free_result(res);
		    ZzLOG(ERROR, "daem9001_insert_deal: mysql_store_result error...\n");
			ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		    return -1;
	    }
		if (mysql_num_rows(t_res)==0)
		{
			mysql_free_result(t_res);
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi.T_USER_RCMD_RANK "
							 " values "
							 " ('%s', %d, %d, '%s') "
							 , szUserId
							 , nRcmdCnt
							 , nYrank
							 , szRankDate);
			ZzLOG(ALWAY, "szQuery=[%s]\n\n", szQuery);
			if (mysql_query(con, szQuery))
			{
				mysql_free_result(res);
			    ZzLOG(ERROR, "daem9001_insert_deal: mysql_query error...\n");
				ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
			    return -1;
		    }
		    nRealCnt++;
	    }
		else
		{
			mysql_free_result(t_res);
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update zangsi.T_USER_RCMD_RANK "
							 " set rcmd_cnt = %d, y_rank = %d "
							 " where user_id = '%s' and rank_date = '%s' "
							 , nRcmdCnt
							 , nYrank
							 , szUserId
							 , szRankDate);
			ZzLOG(ALWAY, "szQuery=[%s]\n\n", szQuery);
			if(mysql_query(con, szQuery))
			{
				mysql_free_result(res);
			    ZzLOG(ERROR, "daem9001_insert_deal: mysql_query error...\n");
				ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
			    return -1;
		    }
		    nRealCnt++;
		}
    	
	}
	mysql_free_result(res);
	
	ZzLOG(ALWAY, "daem9001_insert_deal: 집계 완료.[%d]\n", nRealCnt);
	
	memset (szQuery, 0x00, sizeof(szQuery));
	strcpy(szQuery, " delete from zangsi.T_USER_RCMD_RANK_TEMP ");
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem9001_insert_deal: mysql_query error...\n");
		ZzLOG(ERROR, "daem9001_insert_deal: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
	    return -1;
    }
	
	return 0;
}



/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem9001_get_sysdate()
{
	MYSQL_RES* res;
	MYSQL_ROW  row;
	
	char szQuery[1000];		// query string
	char sztemp [100];      // query temp

	memset(szQuery, 0x00, sizeof(szQuery));
	if (strcmp(gsys_date, "00000000")==0)
	{
		strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d')");
		strcat(szQuery, "     , date_format(now(),'%H%i%s')");
		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -1 DAY),'%Y%m%d')");
		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -1 DAY),'%Y%m')");
		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -2 DAY),'%Y%m')");
	}
	else
	{
		sprintf(szQuery, "SELECT date_format(now(),'%%Y%%m%%d')"
						 "     , date_format(now(),'%%H%%i%%s')"
						 "     , '%s' "
						 "     , substring('%s', 1, 6) "
						 "     , date_format(date_add('%s', INTERVAL -1 DAY),'%%Y%%m') "
						 , gsys_date
						 , gsys_date
						 , gsys_date);
	}
	if (mysql_query(bck_con, szQuery))
	{
	    ZzLOG(ERROR, "sysdate: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(bck_con), mysql_error(bck_con));
		return -1;
	}
	if (!(res = mysql_store_result(bck_con)))
	{
	    ZzLOG(ERROR, "sysdate: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(bck_con), mysql_error(bck_con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "sysdate: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(bck_con), mysql_error(bck_con));
		mysql_free_result(res);
		return -1;
	}
	
	row = mysql_fetch_row(res);
	memset(greg_date , 0x00, sizeof(greg_date ));
	memset(greg_time , 0x00, sizeof(greg_time ));
	memset(gproc_date, 0x00, sizeof(gproc_date));
	memset(gproc_yymm, 0x00, sizeof(gproc_yymm));
	memset(grank_yymm, 0x00, sizeof(grank_yymm));

	strcpy(greg_date ,   getstr(row, 0));
	strcpy(greg_time ,   getstr(row, 1));
	strcpy(gproc_date,   getstr(row, 2));
	strcpy(gproc_yymm,   getstr(row, 3));
	strcpy(grank_yymm,   getstr(row, 4));
	
	
	mysql_free_result(res);


	return 0;
}

/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem9001_init_process(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */

    #ifdef _DEBUG_
	ZzInitGlobalVariable2("D_daem9001", "/logs/daemon"); 
    #else
	ZzInitGlobalVariable2("daem9001", "/logs/daemon"); 
    #endif
	
    ZzLOG(ALWAY, "[daem9001]***************프로그램 시작***************\n");  

    // 파라미터 값 설정 및 초기화
    if (argc != 2){
    	goto arg_error;
    }

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(con=db_connect_local("zangsi")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
	   	return(-1); 
	}
	
	#ifdef _DEBUG_
	printf("zangsi 디비연결\n");
	#endif
	//--------------------------------------------------------------------------
	// bck DB 연결
	//--------------------------------------------------------------------------
	if (!(bck_con=db_connect_nodb("")))
	{
		ZzLOG(ERROR, "bck DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1); 
	}


	#ifdef _DEBUG_
	printf("bck 디비연결\n");
	#endif

	/* 처리일자 */
	memset(gsys_date, 0x00, sizeof(gsys_date));
	strcpy(gsys_date, argv[1]);
	ret=daem9001_get_sysdate();
	if (ret < 0){
		db_disconnect(con);
		db_disconnect(bck_con);
		return -1;
	}
	
    return (0);


arg_error:
    ZzLOG(ERROR, "usage : %s YYYYMMDD\n", argv[0]);
    ZzLOG(ERROR, "        YYYYMMDD(처리일자): 00000000 = 시스템일자\n", argv[0]);
    ZzPRT(ERROR, "usage : %s YYYYMMDD\n", argv[0]);
    ZzPRT(ERROR, "        YYYYMMDD(처리일자): 00000000 = 시스템일자\n", argv[0]);
    return -1;
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daem9001_term_process()
{
    // DB close
	db_disconnect(con);
	db_disconnect(bck_con);
    ZzLOG(ALWAY, "[daem9001]***************프로그램 종료***************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem9001_signal(int nSignal)
{
    daem9001_term_process();
}

/*****************************************************************************
*  프로그램 메인 
*****************************************************************************/
#include <stdio.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <sys/stat.h>

int main(int argc, char **argv)
{     
	char    szTemp[1024];
	int     rc;
                 
	/*       
	** SIGNAL 정의
	*/       
	signal(SIGTERM, daem9001_signal);
	signal(SIGINT,  daem9001_signal);
	signal(SIGQUIT, daem9001_signal);
	signal(SIGKILL, daem9001_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);

	if ( daem9001_init_process(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daem9001_main_process();
	
		/* 프로그램 종료루틴 */                    
		daem9001_term_process();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
