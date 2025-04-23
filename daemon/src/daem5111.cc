/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5111.cc
 *         기능 : 준업로더 포인트 집계
 *         설명 : 등록한 컨텐츠 갯수, 해쉬정보가 없는 컨텐츠 등록 갯수, 요청자료 등록 갯수, 삭제 갯수
 *     설치위치 : SUM DB에 위치한다.
 *
 *       작성자 : HCS
 *       작성일 : 2008/10/07
 *     수정이력 : 2009/11/25 -- HCS
 							 : 담아가기 컨텐츠 판매건수 추가
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


int daem5111_process();
int daem5111_process_db();

int daem5111_select_reg_contents(char* pUserId);
int daem5111_select_new_contents(char* pUserId);
int daem5111_select_req_contents(char* pUserId);
int daem5111_select_admdel_contents(char* pUserId);
int daem5111_select_del_contents(char* pUserId);
int daem5111_select_req_del_contents(char* pUserId);
int daem5111_select_op_contents(char* pUserId);
int daem5111_insert_semi_uploder_point(char* pUserId, int nRegCnt, int nNewCnt, int nReqCnt, int nAdmDelCnt, int nDelCnt, int nReqDelCnt, int nOpDealCnt);

int daem5111_process_init(int argc, char **argv);
int daem5111_process_term();
void daem5111_signal(int nSignal);


MYSQL     *con;
MYSQL_RES *res;
MYSQL_ROW  row;

MYSQL     *con_sum;
MYSQL     *con_om;

bool gbIsUserDate      ; //날짜입력
char gst_date[8+1];	// 처리일자
char gst_time[6+1];	// 처리시작시간
char ged_time[6+1];	// 처리마지막시간
char gproc_log[80]; // 로그메시지버퍼



//******************************************************************************
//* daem5111 main
//******************************************************************************
int daem5111_process()
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
		    ZzLOG(ERROR, "[daem5111]sysdate: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
		
		if (!(res = mysql_store_result(con)))
		{
		    ZzLOG(ERROR, "[daem5111]sysdate: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
	 	if (mysql_num_rows(res)==0)
	 	{
		    ZzLOG(ERROR, "[daem5111]sysdate: mysql_num_rows error...\n");
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
	
	int nResult = daem5111_process_db();
	
	if(nResult == -1)
	{
		tran_rollback(con_sum);
		tran_end(con_sum);
		return -1;
	}
	else if(nResult == -2)
	{
		return -1;
	}
	
	return 0;
}

//******************************************************************************
//* daem5111 db 처리로직
//******************************************************************************
int daem5111_process_db()
{

	char szQuery[1600];		// query string
	int  ret;

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/
	

    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "[daem5111]gproc_date : [%s]\n", gst_date);  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/
	
	char szBuffer[600];
	unsigned long dwCount = 0;
	int nCnt = 0;
	int nCount = 0;		
		
	while(1) 
	{
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " select b.user_id from zangsi.T_USER_INFO a, zangsi.T_PERM_UPLOAD_AUTH b "
						 " where a.user_id = b.user_id and b.perm_gu = '2' and a.use_stat = '0' and b.auth_num = '' "
						 " or a.user_id = b.user_id and b.perm_gu = '2' and a.use_stat = '0' and b.auth_num is null "
						 " limit %d, 100", nCnt);
		
		nCnt = nCnt + 100;		
		
		#ifdef __DEBUG
		printf("%s\n\n",szQuery);
		#endif
		
		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "daem5111_process_db: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
			return -2;
		}

		if (!(res = mysql_store_result(con))) 
		{
			ZzLOG(ERROR, "daem5111_process_db: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
			return -2;
		}
	 	if (mysql_num_rows(res)==0)	
	 	{
			mysql_free_result(res);
		    ZzLOG(ALWAY, "daem5111_process: 처리할 자료가 없습니다.[%s]\n", szQuery);
			break;
		}
		while((row = mysql_fetch_row(res))) 
		{
			char szUserId[18];
			memset(szUserId, 0x00, sizeof(szUserId));
			
			strcpy(szUserId, getstr(row,0));

			if (tran_begin(con_sum)!=0)	
			{
			    ZzLOG(ERROR, "tran_begin: 테이베이스 오류입니다.\n");  
				return -2;
			}
			
			/*
			준업로더 포인트 집계
			*/
			
			int nRegCnt = daem5111_select_reg_contents(szUserId);   // 업로드 아이디별 등록 콘텐츠 갯수(해당 날짜)
			int nNewCnt = daem5111_select_new_contents(szUserId);   //
			int nReqCnt = daem5111_select_req_contents(szUserId);
			int nAdmDelCnt = daem5111_select_admdel_contents(szUserId);
			int nDelCnt = daem5111_select_del_contents(szUserId);
			int nReqDelCnt = daem5111_select_req_del_contents(szUserId);
			int nOpDealCnt = daem5111_select_op_contents(szUserId);
			if(nRegCnt < 0 || nNewCnt < 0 || nReqCnt < 0 || nDelCnt < 0 || nReqDelCnt < 0 || nOpDealCnt < 0)
				return -1;

			if(daem5111_insert_semi_uploder_point(szUserId, nRegCnt, nNewCnt, nReqCnt, nAdmDelCnt, nDelCnt, nReqDelCnt, nOpDealCnt) < 0)
			{
				return -1;
			}
						
			if (tran_commit(con_sum)!=0)
			{
				ZzLOG(ERROR, "daem5111_process_db: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
				return -1;
			}
			//--------------------------------------------------------------------------
			// 트렌젝션종료
			//--------------------------------------------------------------------------
			if (tran_end(con_sum)!=0)	
			{
			    ZzLOG(ERROR, "daem5111_process_db: tran_end 테이베이스 오류입니다.\n");  
				return -2;
			}
			nCount++;
		}
		mysql_free_result(res);
	}
	ZzLOG(ALWAY,"daem5111_process_db: %d건이 처리되었습니다.\n", nCount);  
	return 0;
}
int daem5111_select_reg_contents(char* pUserId)
{
	//등록한 자료 갯수

	MYSQL_RES *select_res;
	MYSQL_ROW  select_row;
	int nResult = 0;

	char szQuery[1000];
	memset (szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery, " select count(a.id) " 
					 " from zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_FILE b " 
					 " where a.id = b.id and a.reg_user = '%s' and a.reg_date = '%s' "
					 , pUserId, gst_date);

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "daem5111_select_reg_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (!(select_res = mysql_store_result(con))) 
	{
		ZzLOG(ERROR, "daem5111_select_reg_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(select_res)==0)	
 	{
		ZzLOG(ERROR, "daem5111_select_reg_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		mysql_free_result(select_res);
		return -1;
	}

	select_row = mysql_fetch_row(select_res);
	
	nResult = getint(select_row, 0);
				
	mysql_free_result(select_res);

	return nResult;
}

int daem5111_select_new_contents(char* pUserId)
{
	//신규 등록 갯수(삭제된 자료도 포함하는지??)
	MYSQL_RES *select_res;
	MYSQL_ROW  select_row;
	int nResult = 0;

	char szQuery[1000];
	memset (szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery, " select count(id) from zangsi.T_CONTENTS_NEWHASH_ID where reg_user = '%s' ", pUserId);
	
	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "daem5111_select_new_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
    }
	if (!(select_res = mysql_store_result(con))) 
	{
		ZzLOG(ERROR, "daem5111_select_new_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
 	if (mysql_num_rows(select_res)==0)	
 	{
		ZzLOG(ERROR, "daem5111_select_new_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	select_row = mysql_fetch_row(select_res);
	
	nResult = getint(select_row, 0);
				
	mysql_free_result(select_res);
	
	return nResult;	
}

int daem5111_select_req_contents(char* pUserId)
{
	//요청자료 등록 갯수.
	MYSQL_RES *select_res;
	MYSQL_ROW  select_row;
	int nResult = 0;

	char szQuery[1000];
	memset (szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery, " select count(a.id) " 
					 " from zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_FILE b " 
					 " where a.id = b.id and a.reg_user = '%s' and a.reg_date = '%s' and a.req_id > 0 "
					 , pUserId, gst_date);
	
	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "daem5111_select_req_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	   }
	if (!(select_res = mysql_store_result(con))) 
	{
		ZzLOG(ERROR, "daem5111_select_req_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (mysql_num_rows(select_res)==0)	
	{
		ZzLOG(ERROR, "daem5111_select_req_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	select_row = mysql_fetch_row(select_res);
	
	nResult = getint(select_row, 0);
				
	mysql_free_result(select_res);
	
	return nResult;
}

int daem5111_select_req_del_contents(char* pUserId)
{
	//요청자료 등록 갯수.
	MYSQL_RES *select_res;
	MYSQL_ROW  select_row;
	
	int nResult = 0;
	int nReqAdmDelCnt = 0;
	int nReqSysDelCnt = 0;
	int nReqCnt = 0;
	int nCurReqCnt = 0;
	int nTodayReqCnt = 0;
	int nReqUserDel = 0;
	
	char szQuery[1000];
	memset (szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery, " select b.cont_gu, count(a.id) " 
					 " from zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_ADMDEL b " 
					 " where a.id = b.id and a.reg_user = '%s' and a.reg_date <= '%s' and a.del_yn = 'Y' and a.req_id > 0 "
					 " and b.cont_gu = '03' and b.reg_date = '%s'  "
					 " group by b.cont_gu "
					 , pUserId, gst_date, gst_date);
			
	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "daem5111_select_req_del_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (!(select_res = mysql_store_result(con))) 
	{
		ZzLOG(ERROR, "daem5111_select_req_del_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (mysql_num_rows(select_res) > 0)	
	{
		while(select_row = mysql_fetch_row(select_res))
		{
			if(strcmp(getstr(select_row,0), "01") == 0)
			{
				nReqSysDelCnt = getint(select_row, 1);				
			}
			else
			{
				nReqAdmDelCnt = getint(select_row, 1);
			}
		}
	
	}
	mysql_free_result(select_res);
	
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select count(a.id) " 
					 " from zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_FILE b " 
					 " where a.id = b.id and a.reg_user = '%s' and a.reg_date <= '%s' and a.del_yn = 'N' and a.req_id > 0 "
					 , pUserId, gst_date);
			
	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "daem5111_select_req_del_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (!(select_res = mysql_store_result(con))) 
	{
		ZzLOG(ERROR, "daem5111_select_req_del_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (mysql_num_rows(select_res)==0)	
	{
//		ZzLOG(ERROR, "daem5111_select_req_del_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		mysql_free_result(select_res);
		ZzLOG(ERROR, "daem5111_select_req_del_contents: nReqAdmDelCnt: [%d]\n", nReqAdmDelCnt);
		return nResult;
	}
	select_row = mysql_fetch_row(select_res);

	nCurReqCnt = getint(select_row, 0);

	mysql_free_result(select_res);

	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select count(a.id) " 
					 " from zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_FILE b " 
					 " where a.id = b.id and a.reg_user = '%s' and a.reg_date = '%s' and a.req_id > 0 "
					 , pUserId, gst_date);
			
	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "daem5111_select_req_del_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (!(select_res = mysql_store_result(con))) 
	{
		ZzLOG(ERROR, "daem5111_select_req_del_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (mysql_num_rows(select_res) > 0)	
	{
		select_row = mysql_fetch_row(select_res);
	
		nTodayReqCnt = getint(select_row, 0);
	}

	mysql_free_result(select_res);

	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select req_contents_cnt " 
					 " from zangsi_sum.T_SEMI_UPLOADER_POINT " 
					 " where user_id = '%s'"
					 , pUserId);
			
	if (mysql_query(con_sum, szQuery))
	{
		ZzLOG(ERROR, "daem5111_select_req_del_contents: [%d](%s)\n(%s)\n",mysql_errno(con_sum), mysql_error(con_sum), szQuery);
		return -1;
	}
	if (!(select_res = mysql_store_result(con_sum))) 
	{
		ZzLOG(ERROR, "daem5111_select_req_del_contents: [%d](%s)\n(%s)\n",mysql_errno(con_sum), mysql_error(con_sum), szQuery);
		return -1;
	}
	if (mysql_num_rows(select_res)==0)	
	{
		mysql_free_result(select_res);
//		ZzLOG(ERROR, "daem5111_select_req_del_contents: nReqAdmDelCnt: [%d] nCurReqCnt: [%d]\n", nReqAdmDelCnt, nCurReqCnt);
		return nResult;
	}
	select_row = mysql_fetch_row(select_res);

	nReqCnt = getint(select_row, 0);

	mysql_free_result(select_res);
	
	
	nReqUserDel/*자진 삭제 건수*/ = nReqCnt + nTodayReqCnt/*모든 요청자료 등록 건수*/ - nCurReqCnt/*삭제되지 않은 현재의 요청자료 건수*/ - nReqAdmDelCnt/*관리자가 삭제한 요청자료 건수*/;
	nReqUserDel = nReqUserDel - nReqSysDelCnt;

	if(nReqCnt <= 0 || nReqUserDel <= 0)
	{
//		ZzLOG(ERROR, "daem5111_select_req_del_contents: %d = %d - (%d + %d)\n", nReqUserDel, nReqCnt, nCurReqCnt, nReqAdmDelCnt);
		return nReqAdmDelCnt;
	}
	
	nResult = nReqAdmDelCnt + nReqUserDel/*관리자 삭제건수 + 사용자가 직접 삭제한 건수*/;
	
//	ZzLOG(ERROR, "daem5111_select_req_del_contents: %d = %d + %d - %d - %d  nResult : [%d]\n", nReqUserDel, nReqCnt, nTodayReqCnt, nCurReqCnt, nReqAdmDelCnt, nResult);
	
	return nResult;
}

int daem5111_select_admdel_contents(char* pUserId)
{
	//삭제된자료 갯수.
	MYSQL_RES *select_res;
	MYSQL_ROW  select_row;
	int nResult = 0;

	char szQuery[1000];
	memset (szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery, " select count(b.id) " 
					 " from zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_ADMDEL b " 
					 " where a.id = b.id and a.reg_user = '%s' "
					 " and b.cont_gu = '03' and b.reg_date = '%s' "
					 , pUserId, gst_date);
	
	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "daem5111_select_admdel_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
    }
	if (!(select_res = mysql_store_result(con))) 
	{
		ZzLOG(ERROR, "daem5111_select_admdel_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
 	if (mysql_num_rows(select_res)==0)	
 	{
		ZzLOG(ERROR, "daem5111_select_admdel_contents: [%s]\n", pUserId);
		mysql_free_result(select_res);
		return nResult;
	}
	select_row = mysql_fetch_row(select_res);
	
	nResult = getint(select_row, 0);
				
	mysql_free_result(select_res);

	return nResult;
}	

int daem5111_select_del_contents(char* pUserId)
{
	//삭제된자료 갯수.
	MYSQL_RES *select_res;
	MYSQL_ROW  select_row;
	int nResult = 0;
	int nCnt = 0;
	int nCurCnt = 0;
	int nTodayCnt = 0;
	int nAdmCnt = 0;
	int nSysCnt = 0;
	int nUserCnt = 0;

	char szQuery[1000];
	memset (szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery, " select b.cont_gu, count(b.id) " 
					 " from zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_ADMDEL b " 
					 " where a.id = b.id and a.reg_user = '%s' "
					 " and b.cont_gu = '03' and b.reg_date = '%s' "
					 " group by b.cont_gu "
					 , pUserId, gst_date);
	
	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "daem5111_select_del_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
    }
	if (!(select_res = mysql_store_result(con))) 
	{
		ZzLOG(ERROR, "daem5111_select_del_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
 	if (mysql_num_rows(select_res) > 0)	
 	{
		while(select_row = mysql_fetch_row(select_res))
		{
			if(strcmp(getstr(select_row, 0), "01") == 0)
			{
				nSysCnt = getint(select_row, 1);
			}
			else
			{
				nAdmCnt = getint(select_row, 1);
			}
			
		}
					
		mysql_free_result(select_res);
	}
	
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select count(a.id) " 
					 " from zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_FILE b " 
					 " where a.id = b.id and a.reg_user = '%s' and a.reg_date <= '%s' "
					 , pUserId, gst_date);
			
	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "daem5111_select_del_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (!(select_res = mysql_store_result(con))) 
	{
		ZzLOG(ERROR, "daem5111_select_del_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (mysql_num_rows(select_res) > 0)	
	{
		select_row = mysql_fetch_row(select_res);
	
		nCurCnt = getint(select_row, 0);
	
	}
	mysql_free_result(select_res);

	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select count(a.id) " 
					 " from zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_FILE b " 
					 " where a.id = b.id and a.reg_user = '%s' and a.reg_date = '%s' "
					 , pUserId, gst_date);
			
	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "daem5111_select_del_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (!(select_res = mysql_store_result(con))) 
	{
		ZzLOG(ERROR, "daem5111_select_del_contents: [%d](%s)\n(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
		return -1;
	}
	if (mysql_num_rows(select_res) > 0)	
	{
		select_row = mysql_fetch_row(select_res);
	
		nTodayCnt = getint(select_row, 0);
	
	}
	mysql_free_result(select_res);

	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select reg_contents_cnt " 
					 " from zangsi_sum.T_SEMI_UPLOADER_POINT " 
					 " where user_id = '%s'"
					 , pUserId);
			
	if (mysql_query(con_sum, szQuery))
	{
		ZzLOG(ERROR, "daem5111_select_del_contents: [%d](%s)\n(%s)\n",mysql_errno(con_sum), mysql_error(con_sum), szQuery);
		return -1;
	}
	if (!(select_res = mysql_store_result(con_sum))) 
	{
		ZzLOG(ERROR, "daem5111_select_del_contents: [%d](%s)\n(%s)\n",mysql_errno(con_sum), mysql_error(con_sum), szQuery);
		return -1;
	}
	if (mysql_num_rows(select_res)==0)	
	{
//		ZzLOG(ERROR, "daem5111_select_del_contents: nCurCnt: [%d] nAdmCnt: [%d]\n", nCurCnt, nAdmCnt);
		mysql_free_result(select_res);
		return nAdmCnt;
	}
	select_row = mysql_fetch_row(select_res);

	nCnt = getint(select_row, 0);

	mysql_free_result(select_res);
	
	nUserCnt = nCnt + nTodayCnt - nCurCnt - nAdmCnt;

	nUserCnt = nUserCnt - nSysCnt;

	if(nCnt <= 0 || nUserCnt <= 0)
	{
//		ZzLOG(ERROR, "daem5111_select_del_contents: [%d]\n", nAdmCnt);
		return nAdmCnt;
	}

	nResult = nAdmCnt + nUserCnt;
	
//	ZzLOG(ERROR, "daem5111_select_del_contents: %d = %d + %d - %d - %d  nResult : [%d]\n", nUserCnt, nTodayCnt, nCnt, nCurCnt, nAdmCnt, nResult);
	
	return nResult;
}	
int daem5111_select_op_contents(char* pUserId)
{
	//신규 등록 갯수(삭제된 자료도 포함하는지??)
	MYSQL_RES *select_res;
	MYSQL_ROW  select_row;
	int nResult = 0;

	char szQuery[1000];
	memset (szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery, " select COUNT(deal_no) from zangsi_op.T_OP_DEAL_INFO where sale_user = '%s' ", pUserId);
	
	if (mysql_query(con_om, szQuery))
	{
		ZzLOG(ERROR, "daem5111_select_op_contents: [%d](%s)\n(%s)\n",mysql_errno(con_om), mysql_error(con_om), szQuery);
		return -1;
    }
	if (!(select_res = mysql_store_result(con_om))) 
	{
		ZzLOG(ERROR, "daem5111_select_op_contents: [%d](%s)\n(%s)\n",mysql_errno(con_om), mysql_error(con_om), szQuery);
		return -1;
	}
 	if (mysql_num_rows(select_res)==0)	
 	{
		ZzLOG(ERROR, "daem5111_select_op_contents: [%d](%s)\n(%s)\n",mysql_errno(con_om), mysql_error(con_om), szQuery);
		return -1;
	}
	select_row = mysql_fetch_row(select_res);
	
	nResult = getint(select_row, 0);
				
	mysql_free_result(select_res);
	
	return nResult;	
}

int daem5111_insert_semi_uploder_point(char* pUserId, int nRegCnt, int nNewCnt, int nReqCnt, int nAdmDelCnt, int nDelCnt, int nReqDelCnt, int nOpDealCnt)
{
	//T_SEMI_UPLOAD_POINT 데이터 집계.
	MYSQL_RES *select_res;
	MYSQL_ROW  select_row;
	int nResult = 0;

	char szQuery[1000];
	memset (szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery, " select count(*) from zangsi_sum.T_SEMI_UPLOADER_POINT where user_id = '%s' ", pUserId);

	if (mysql_query(con_sum, szQuery))
	{
		ZzLOG(ERROR, "daem5111_insert_semi_uploder_point: [%d](%s)\n(%s)\n",mysql_errno(con_sum), mysql_error(con_sum), szQuery);
		return -1;
    }
	
	if (!(select_res = mysql_store_result(con_sum))) 
	{
		ZzLOG(ERROR, "daem5111_insert_semi_uploder_point: [%d](%s)\n(%s)\n",mysql_errno(con_sum), mysql_error(con_sum), szQuery);
		return -1;
	}
 	if (mysql_num_rows(select_res)==0)	
 	{
		ZzLOG(ERROR, "daem5111_insert_semi_uploder_point: [%d](%s)\n(%s)\n",mysql_errno(con_sum), mysql_error(con_sum), szQuery);
		return -1;
	}
	select_row = mysql_fetch_row(select_res);

	int nRowCnt = getint(select_row, 0);
				
	mysql_free_result(select_res);

	if(nRowCnt > 0)
	{
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update zangsi_sum.T_SEMI_UPLOADER_POINT "
						 " set reg_contents_cnt = reg_contents_cnt + %d, "
						 " new_contents_cnt = %d, "
						 " req_contents_cnt = req_contents_cnt + %d - %d, "
						 " admdel_contents_cnt = admdel_contents_cnt + %d, "
						 " del_contents_cnt = del_contents_cnt + %d, "
						 " op_sale_cnt = %d "
						 " where user_id = '%s' "
						 , nRegCnt
						 , nNewCnt
						 , nReqCnt
						 , nReqDelCnt
						 , nAdmDelCnt
						 , nDelCnt
						 , nOpDealCnt
						 , pUserId);

		if (mysql_query(con_sum, szQuery))
		{
			ZzLOG(ERROR, "daem5111_insert_semi_uploder_point: [%d](%s)\n(%s)\n",mysql_errno(con_sum), mysql_error(con_sum), szQuery);
			return -1;
	    }
	}
	else if(nRowCnt == 0)
	{
		int nReqProc = nReqCnt - nReqDelCnt;
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi_sum.T_SEMI_UPLOADER_POINT values ('%s', %d, %d, %d, %d, %d, %d) "
						 , pUserId
						 , nRegCnt
						 , nNewCnt
						 , nReqProc
						 , nAdmDelCnt
						 , nDelCnt
						 , nOpDealCnt);

		if (mysql_query(con_sum, szQuery))
		{
			ZzLOG(ERROR, "daem5111_insert_semi_uploder_point: [%d](%s)\n(%s)\n",mysql_errno(con_sum), mysql_error(con_sum), szQuery);
			return -1;
	    }
	}
	else
	{
		ZzLOG(ERROR, "daem5111_insert_semi_uploder_point: nRowCnt Error [%d]\n", nRowCnt);
	}
//	ZzLOG(ALWAY, "daem5111_insert_semi_uploder_point : %s\n\n ---[%d]------[%d]\n\n", szQuery, nReqCnt, nReqDelCnt);
	return 0;
}
/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5111_process_init(int argc, char **argv)
{
	char stemp[128];
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("d_", "/logs/daemon"); 

    ZzLOG(ALWAY, "[daem5111]*****************프로그램 시작*****************\n");  

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
		ZzLOG(ERROR, "[daem5111]master DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1); 
	}

	if (!(con_sum=db_connect_sumdb("zangsi_sum")))
	{
		ZzLOG(ERROR, "[daem5111]sum DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
		db_disconnect(con_sum);
	   	return(-1); 
	}

	if (!(con_om=db_connect_omdb("zangsi_op")))
	{
		ZzLOG(ERROR, "[daem5111]om DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
		db_disconnect(con_sum);
		db_disconnect(con_om);
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
int daem5111_process_term()
{
    // DB close
	db_disconnect(con);
	db_disconnect(con_sum);
	db_disconnect(con_om);
	
    ZzLOG(ALWAY, "[daem5111]*****************프로그램 종료*****************\n");  

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5111_signal(int nSignal)
{
    daem5111_process_term();
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
	signal(SIGTERM, daem5111_signal);
	signal(SIGINT,  daem5111_signal);
	signal(SIGQUIT, daem5111_signal);
	signal(SIGKILL, daem5111_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daem5111_process_init(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daem5111_process();
		/* 프로그램 종료루틴 */                    
		daem5111_process_term();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
