/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5123.cc
 *         기능 : 리콜 대상자 집계(zangsi_bck.T_RECALL_LIST)
 *         설명 : 
 *       작성자 : HCS
 *       작성일 : 2008/01/16
 *     수정이력 :
 		reg_reapy_yn = 'N' : 판매자보상 필요 없음
		reg_reapy_yn = 'P' : 판매자보상 요청
		reg_reapy_yn = 'R' : 판매자보상 목록 집계완료
		reg_reapy_yn = 'Y' : 판매자보상 완료
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

#define _DEBUG_

int Daem5123InitProcess(int argc, char **argv); 

int Daem5123DnUserTotalProcess(); 

int Daem5123UpUserTotalProcess(); 

int Daem5123TermProcess(); 

int Daem5123InsertRecallListDnUser(unsigned long ulID, char* pContGu, char* pServerID, int nServerSeqNo, char* pErrDate, char* pErrTime, char* pStopDate, char* pStopTime, char* pTitle);

int Daem5123InsertRecallListUpUser(unsigned long ulID, char* pContGu, char* pSaleUser, int nSaleAmt, char* pTitle, char* pContRegDate, char* pContRegTime, char* pServerID, int nServerSeqNo, char* pErrDate, char* pErrTime, char* pStopDate, char* pStopTime);

char* AppendSpecialChar(char* strSrcString, char cReplace,char* pResult);

int Daem5123GetSystemDate(); 

void Daem5123Signal(int nSignal); 

MYSQL     *gcon; 

char   gszSysDate  [  8+1];	//처리일자(sysdate)
char   gszProcDate [  8+1];	//처리일자(sysdate-1)
char   gszRegDate  [  8+1];	//등록일
char   gszRegTime  [  6+1];	//등록시간

char   gszBckDate  [  8+1];	//bck로 옮겨진 날자

int gnTotalCount = 0;
//******************************************************************************
//* daem5123 main
//******************************************************************************
int Daem5123DnUserTotalProcess()
{
	/*
	1.T_SERVER_ERR_HIST에서 처리일자의 정보를 읽어와 오류난서버의 컨텐츠 아이디를 검색.
	2.Daem5123InsertRecallListDnUser()에 컨텐츠 정보와 리콜관련 정보를 넘김.
	3.Daem5123InsertRecallListDnUser()은 건네 받은 컨텐츠 정보로 T_DEAL_INFO에서 리콜대상자를 검색하여 T_RECALL_LIST에 INSERT.
	4.T_SERVER_ERR_HIST에서 읽어온 한행이 처리가 되면 repay_yn을 R로 고치고, repay_date와 repay_time을 기재		
	*/
	MYSQL_RES *res; 
	MYSQL_RES *res2; 
	MYSQL_ROW  row; 
	MYSQL_ROW  row2; 

	char szQuery[10000];
	memset(szQuery, 0x00, sizeof(szQuery));
	
	char szBuffer[10000];
	memset(szBuffer, 0x00, sizeof(szBuffer));

	int ret=0;
	
	strcpy(szQuery, " select server_id, seq_no,err_date, err_time, stop_date, stop_time "
					" from zangsi_bck.T_SERVER_ERR_HIST "	
					" where repay_yn = 'P' "
					" order by server_id, err_date ");

	ZzLOG(ALWAY, "Daem5123DnUserTotalProcess : %s\n\n",szQuery);

	if(mysql_query(gcon, szQuery))
	{
		ZzLOG(ERROR, "Daem5123DnUserTotalProcess : mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
		return -1;
	}

	if(!(res = mysql_store_result(gcon)))
	{
		ZzLOG(ERROR, "Daem5123DnUserTotalProcess: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon), mysql_error(gcon));
		return  -1;
	}
 	if(mysql_num_rows(res)==0)
 	{
		ZzLOG(ERROR, "Daem5123DnUserTotalProcess: 처리할 목록이 없습니다...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon), mysql_error(gcon));
		mysql_free_result(res);
		
		return 1;
	}

	while(row = mysql_fetch_row(res))
	{
		gnTotalCount = 0;
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery," select a.id, a.title, 'WE' as cont_gu  "
						" from zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_FILE b "
						" where a.id = b.id and b.server_id = '%s' "
						" union all "
						" select a.id, a.title, 'FD' as cont_gu "
						" from zangsi.T_CONTFLOG_INFO a, zangsi.T_CONTFLOG_FILE b "
						" where a.id = b.id and b.server_id = '%s' "
						, getstr(row,0), getstr(row,0));
						
		ZzLOG(ALWAY, "Daem5123DnUserTotalProcess : %s\n\n",szQuery);				
		if(mysql_query(gcon, szQuery))
		{
			ZzLOG(ERROR, "Daem5123DnUserTotalProcess : mysql_query error...\n");
			ZzLOG(ERROR, "Daem5123DnUserTotalProcess : %s\n\n",szQuery);
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			mysql_free_result(res);
			return -1;
		}
	
		if(!(res2 = mysql_store_result(gcon)))
		{
			ZzLOG(ERROR, "Daem5123DnUserTotalProcess: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon), mysql_error(gcon));
			mysql_free_result(res);
			return  -1;
		}
	 	if(mysql_num_rows(res2)==0)
	 	{
			ZzLOG(ERROR, "Daem5123DnUserTotalProcess: 서버에 컨텐츠가 존재하지 않습니다...[server_id=(%s)]\n", getstr(row,0));
			mysql_free_result(res2);
			continue;
		}

		if(tran_begin(gcon)!=0) 
		{
		    ZzLOG(ERROR, "tran_begin: 테이베이스 오류입니다.\n");  
			ZzLOG(ERROR, "Daem5123DnUserTotalProcess: [%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			mysql_free_result(res);
		    return -1;
		}
		
		while(row2 = mysql_fetch_row(res2))
		{
			memset(szBuffer, 0x00, sizeof(szBuffer));
			AppendSpecialChar(getstr(row2,1) , '\\' , szBuffer);
			ret = Daem5123InsertRecallListDnUser((unsigned long) getnum(row2,0), getstr(row2,2), getstr(row,0), getint(row,1), getstr(row,2), getstr(row,3), getstr(row,4), getstr(row,5), szBuffer);
			if(ret < 0)
			{
				tran_rollback(gcon);
				mysql_free_result(res);
				mysql_free_result(res2);
				return -1;
			}
			else if(ret == -2)
			{	
				continue;
			}			
		}
		mysql_free_result(res2);		
		
		ZzLOG(ALWAY,"Daem5123DnUserTotalProcess: (%s) 총 리콜 대상건수 (%d)\n ",getstr(row,0), gnTotalCount);
		/*
		T_SERVER_ERR_HIST에 재기입.(repay_yn = 'R', repay_date = gszRegDate, repay_time = gszRegTime)
		*/
		memset(szQuery, 0x00, sizeof(szQuery));
		
		sprintf(szQuery, " update zangsi_bck.T_SERVER_ERR_HIST "
						" set repay_yn = 'R', proc_buy_cnt = '%d' "
						" where server_id = '%s' and seq_no = %d "
						, gnTotalCount
						, getstr(row,0), getint(row,1));

		ZzLOG(ALWAY, "Daem5123DnUserTotalProcess : %s\n\n",szQuery);	
		if(mysql_query(gcon, szQuery))
		{
			ZzLOG(ERROR, "Daem5123DnUserTotalProcess : mysql_query error...\n");
			ZzLOG(ERROR, "Daem5123DnUserTotalProcess : %s\n\n",szQuery);
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			tran_rollback(gcon);
			mysql_free_result(res);
			return -1;
		}

		if(tran_commit(gcon)!=0)
		{
		    ZzLOG(ERROR, "Daem5123DnUserTotalProcess: tran_commit error...\n");
			ZzLOG(ERROR, "Daem5123DnUserTotalProcess: [%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			tran_rollback(gcon);
			mysql_free_result(res);
		    return -1;
		}
	}

	mysql_free_result(res);

	return (0);
}
int Daem5123UpUserTotalProcess()
{
	/*
	1.T_SERVER_ERR_HIST에서 처리일자의 정보를 읽어와 복구불가인 서버의 컨텐츠 아이디를 검색(삭제, 기간만료 제외).
	2.컨텐츠 정보를 T_RECALL_LIST에 INSERT.
	3.T_SERVER_ERR_HIST에서 읽어온 한행이 처리가 되면 reg_repay_yn을 R로 고치고, reg_repay_date와 reg_repay_time을 기재		
	*/
	MYSQL_RES *res; 
	MYSQL_RES *res2; 
	MYSQL_ROW  row; 
	MYSQL_ROW  row2; 

	char szQuery[10000];
	memset(szQuery, 0x00, sizeof(szQuery));
	
	char szBuffer[10000];
	memset(szBuffer, 0x00, sizeof(szBuffer));
	

	int ret=0;
	int nTotalCount = 0;
	
	strcpy(szQuery, " select server_id, seq_no,err_date, err_time, stop_date, stop_time "
					" from zangsi_bck.T_SERVER_ERR_HIST "	
					" where reg_repay_yn = 'P' "
					" order by server_id, err_date ");

	ZzLOG(ALWAY, "Daem5123UpUserTotalProcess : %s\n\n",szQuery);

	if(mysql_query(gcon, szQuery))
	{
		ZzLOG(ERROR, "Daem5123UpUserTotalProcess : mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
		return -1;
	}

	if(!(res = mysql_store_result(gcon)))
	{
		ZzLOG(ERROR, "Daem5123UpUserTotalProcess: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon), mysql_error(gcon));
		return  -1;
	}
 	if(mysql_num_rows(res)==0)
 	{
		ZzLOG(ERROR, "Daem5123UpUserTotalProcess: 처리할 목록이 없습니다...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon), mysql_error(gcon));
		mysql_free_result(res);
		
		return 1;
	}

	while(row = mysql_fetch_row(res))
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery," select a.id, 'WE' as cont_gu, a.reg_user, a.price_amt, a.title, a.reg_date, a.reg_time "
						" from zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_FILE b "
						" where a.id = b.id and b.server_id = '%s' and a.del_yn = 'N' "
						" union all "
						" select a.id, 'FD' as cont_gu, a.reg_user, a.price_amt, a.title, a.reg_date, a.reg_time "
						" from zangsi.T_CONTFLOG_INFO a, zangsi.T_CONTFLOG_FILE b "
						" where a.id = b.id and b.server_id = '%s' and a.del_yn = 'N' " 
						" order by 3 "
						, getstr(row,0), getstr(row,0));
						
		ZzLOG(ALWAY, "Daem5123UpUserTotalProcess : %s\n\n",szQuery);				
		if(mysql_query(gcon, szQuery))
		{
			ZzLOG(ERROR, "Daem5123UpUserTotalProcess : mysql_query error...\n");
			ZzLOG(ERROR, "Daem5123UpUserTotalProcess : %s\n\n",szQuery);
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			mysql_free_result(res);
			return -1;
		}
	
		if(!(res2 = mysql_store_result(gcon)))
		{
			ZzLOG(ERROR, "Daem5123UpUserTotalProcess: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon), mysql_error(gcon));
			mysql_free_result(res);
			return  -1;
		}
	 	if(mysql_num_rows(res2)==0)
	 	{
			ZzLOG(ERROR, "Daem5123UpUserTotalProcess: 서버에 컨텐츠가 존재하지 않습니다...[server_id=(%s)]\n", getstr(row,0));
			mysql_free_result(res2);
			continue;
		}

		if(tran_begin(gcon)!=0) 
		{
		    ZzLOG(ERROR, "tran_begin: 테이베이스 오류입니다.\n");  
			ZzLOG(ERROR, "Daem5123UpUserTotalProcess: [%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			mysql_free_result(res);
		    return -1;
		}
		
		while(row2 = mysql_fetch_row(res2))
		{
			memset(szBuffer, 0x00, sizeof(szBuffer));
			AppendSpecialChar(getstr(row2, 4) , '\\' , szBuffer);
			ret = Daem5123InsertRecallListUpUser((unsigned long)getnum(row2,0), getstr(row2,1), getstr(row2,2), getint(row2,3), szBuffer, getstr(row2,5), getstr(row2,6), getstr(row,0), getint(row,1), getstr(row,2), getstr(row,3), getstr(row,4), getstr(row,5));
			if(ret < 0)
			{
				tran_rollback(gcon);
				mysql_free_result(res);
				mysql_free_result(res2);
				return -1;
			}
			else if(ret == -2)
			{	
				continue;
			}	
			nTotalCount++;		
		}
		mysql_free_result(res2);		
		
		ZzLOG(ALWAY,"Daem5123UpUserTotalProcess: (%s) 총 판매자 리콜 대상건수 (%d)\n ",getstr(row,0), nTotalCount);
		/*
		T_SERVER_ERR_HIST에 재기입.(repay_yn = 'R', repay_date = gszRegDate, repay_time = gszRegTime)
		*/
		memset(szQuery, 0x00, sizeof(szQuery));
		
		sprintf(szQuery, " update zangsi_bck.T_SERVER_ERR_HIST "
						" set reg_repay_yn = 'R', proc_sale_cnt = '%d' "
						" where server_id = '%s' and seq_no = %d "
						, nTotalCount
						, getstr(row,0), getint(row,1));

		ZzLOG(ALWAY, "Daem5123UpUserTotalProcess : %s\n\n",szQuery);	
		if(mysql_query(gcon, szQuery))
		{
			ZzLOG(ERROR, "Daem5123UpUserTotalProcess : mysql_query error...\n");
			ZzLOG(ERROR, "Daem5123UpUserTotalProcess : %s\n\n",szQuery);
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			tran_rollback(gcon);
			mysql_free_result(res);
			return -1;
		}

		if(tran_commit(gcon)!=0)
		{
		    ZzLOG(ERROR, "Daem5123UpUserTotalProcess: tran_commit error...\n");
			ZzLOG(ERROR, "Daem5123UpUserTotalProcess: [%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			tran_rollback(gcon);
			mysql_free_result(res);
		    return -1;
		}
	}

	mysql_free_result(res);

	return (0);
}

int Daem5123InsertRecallListDnUser(unsigned long ulID, char *pContGu, char *pServerID, int nServerSeqNo, char *pErrDate, char *pErrTime, char *pStopDate, char *pStopTime, char *pTitle)
{
	//T_DEAL_INFO에서 에러시간대에 구매한내역이 있는지 검색하여 있다면 T_RECALL_LIST에 추가
	MYSQL_RES *res_backup;
	MYSQL_ROW row_backup;
	
	char szQuery[10000];
	memset(szQuery, 0x00, sizeof(szQuery));
	
	char szDbName[10+1];
	memset(szDbName, 0x00, sizeof(szDbName));
	
	if(strcmp(pErrDate,gszBckDate) <= 0)
	{	
//		strcpy(szDbName, "zangsi_bck");
//		ZzLOG(ALWAY, "Daem5123InsertRecallListDnUser: szDbName(%s), err_date(%s), gszBckDate(%s)\n",szDbName, pErrDate, gszBckDate);
		ZzLOG(ALWAY, "Daem5123InsertRecallListDnUser: 다운내역을 알수 없음 err_date(%s), gszBckDate(%s)\n", pErrDate, gszBckDate);
		return 0;
	}
	else
	{
		strcpy(szDbName, "zangsi");
	}
	
	int nCount = 0;
	if(strcmp(pErrDate, pStopDate) == 0)
	{
		sprintf(szQuery, " select deal_no, cont_gu, deal_date, deal_time, fixamt_yn, "
						 " coupon_code, buy_user, sale_user, if(fixamt_yn = '0', sale_amt + comp_amt, price_amt) as sale_amt, id as cont_id "
						 " from %s.T_DEAL_INFO "
						 //" where id = %d and fixamt_yn not in('1', '2', '3', '4') and deal_date = '%s' and deal_time >= '%s' and deal_time <= '%s' "
						 " where id = %lu and cont_gu = '%s' and fixamt_yn in ('0', '5', '6', '7', '8', '9','P') and " 
						 " deal_date = '%s' and deal_time >= '%s' and deal_time <= '%s' "
						 " order by deal_time "
						 , szDbName, ulID, pContGu, pErrDate, pErrTime, pStopTime);
		ZzLOG(ALWAY, "Daem5123InsertRecallListDnUser : %s\n\n",szQuery);	

		if(mysql_query(gcon, szQuery))
		{
			ZzLOG(ERROR, "Daem5123InsertRecallListDnUser : mysql_query error...\n");
			ZzLOG(ERROR, "Daem5123InsertRecallListDnUser : %s\n\n",szQuery);
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));			
			return -1;
		}
		
		if(!(res_backup = mysql_store_result(gcon)))
		{
			ZzLOG(ERROR, "Daem5123InsertRecallListDnUser: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			return  -1;
		}
		
	 	if(mysql_num_rows(res_backup)==0)
	 	{
			//ZzLOG(ERROR, "Daem5123InsertRecallListDnUser: 오류시간대에 %d번(%s) 컨텐츠의 구매내역이 없습니다...(%s ~ %s)\n", ulID, pContGu, pErrTime, pStopTime);
			mysql_free_result(res_backup);
			return 1;
		}
		while(row_backup = mysql_fetch_row(res_backup))
		{
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi_bck.T_RECALL_LIST                         "
							 " (seq_no, inout_cd, deal_no, cont_gu,                                    "
							 " deal_date, deal_time, fixamt_yn, coupon_code,                "
							 " buy_user, sale_user, reg_date, reg_time, sale_amt,           "
							 " proc_flag, cont_id, title, server_id, server_seq_no, err_date, err_time, stop_date, stop_time)            "
							 " select                                                       "
							 " ifnull(max(seq_no)+1,0), 'B', %d, '%s',               "
							 " '%s', '%s', '%s', '%s',        "
							 " '%s', '%s', '%s', '%s', %d, "
							 " 'P', %lu, '%s', '%s', '%d', '%s', '%s', '%s', '%s'  "
							 " from zangsi_bck.T_RECALL_LIST                                "
							 , getint(row_backup,0), getstr(row_backup,1)
							 , getstr(row_backup,2), getstr(row_backup,3) , getstr(row_backup,4), getstr(row_backup,5)
							 , getstr(row_backup,6), getstr(row_backup,7), gszRegDate, gszRegTime, getint(row_backup,8)
							 , ulID, pTitle, pServerID, nServerSeqNo, pErrDate, pErrTime, pStopDate, pStopTime);
			
			ZzLOG(ALWAY, "Daem5123InsertRecallListDnUser:  %s\n",szQuery);
			if(mysql_query(gcon, szQuery))
			{
				ZzLOG(ERROR, "Daem5123InsertRecallListDnUser : mysql_query error...\n");
				ZzLOG(ERROR, "Daem5123InsertRecallListDnUser : %s\n\n",szQuery);
				ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
				mysql_free_result(res_backup);
				return -1;
			}
			nCount++;
		}
		mysql_free_result(res_backup);
		ZzLOG(ALWAY, "Daem5123InsertRecallListDnUser: 컨텐츠 %d의 오류시간대 구매건수 = (%lu)\n", ulID, nCount);
		gnTotalCount = gnTotalCount + nCount;
	}	
	else if(strcmp(pErrDate, pStopDate) < 0)
	{
		/*
		pStopDate가 pErrDate보다 크다면...(서버오류가 난후에 해당 서버 처리에 하루이상 걸린경우. 그럴일은 없겠지만...ㅡ,,ㅡ;;;)
		*/
		
		int nFirst = 0;		
		char szErrDate[8+1];
		memset(szErrDate, 0x00, sizeof(szErrDate));
		
		strcpy(szErrDate, pErrDate);
				
		while(strcmp(szErrDate, pStopDate) <= 0)
		{	
//			ZzLOG(ALWAY, "Daem5123InsertRecallListDnUser: szErrDate(%s), stop_date(%s), nFirst(%d)\n\n\n", szErrDate, pStopDate, nFirst);
			if(nFirst ==0)
			{	
				sprintf(szQuery, " select deal_no, cont_gu, deal_date, deal_time, fixamt_yn, "
								 " coupon_code, buy_user, sale_user, if(fixamt_yn = '0', comp_amt, price_amt) as sale_amt, id as cont_id "
								 " from %s.T_DEAL_INFO "
								 " where id = %lu and cont_gu = '%s' and fixamt_yn in ('0', '5', '6', '7', '8', '9','P') " 
								 " and deal_date = '%s' and deal_time >= '%s' "
								 " order by deal_time "
								 , szDbName, ulID, pContGu, szErrDate, pErrTime);
				nFirst = 1;
			}
			else
			{
				sprintf(szQuery, "select date_format(date_add('%s',INTERVAL +1 DAY),'%%Y%%m%%d')", szErrDate);
				
				if(mysql_query(gcon, szQuery))
				{
					ZzLOG(ERROR, "Daem5123InsertRecallListDnUser : mysql_query error...\n");
					ZzLOG(ERROR, "Daem5123InsertRecallListDnUser : %s\n\n",szQuery);
					ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
					return -1;
				}
				
				if(!(res_backup = mysql_store_result(gcon)))
				{
					ZzLOG(ERROR, "Daem5123InsertRecallListDnUser: mysql_store_result error...\n");
					ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
					return  -1;
				}
				
			 	if(mysql_num_rows(res_backup)==0)
			 	{
					ZzLOG(ERROR, "Daem5123InsertRecallListDnUser: mysql_num_rows error...\n");
					ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
					mysql_free_result(res_backup);
					return 1;
				}

				row_backup = mysql_fetch_row(res_backup);

				strcpy(szErrDate, getstr(row_backup,0));
				
//				ZzLOG(ALWAY,"Daem5123InsertRecallListDnUser: szErrDate(%s)\n\n",szErrDate);
				
				mysql_free_result(res_backup);
				
				memset(szQuery, 0x00, sizeof(szQuery));			
				
				sprintf(szQuery, " select deal_no, cont_gu, deal_date, deal_time, fixamt_yn, "
								 " coupon_code, buy_user, sale_user, if(fixamt_yn = '0', comp_amt, price_amt) as sale_amt, id as cont_id "
								 " from %s.T_DEAL_INFO "
								 " where id = %lu and cont_gu = '%s' and fixamt_yn in ('0', '5', '6', '7', '8', '9','P') " 
								 " and deal_date = '%s' "
								 " order by deal_time "
								 , szDbName, ulID, pContGu, szErrDate);
			}
			
			if(strcmp(szErrDate, pStopDate) == 0)
			{
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " select deal_no, cont_gu, deal_date, deal_time, fixamt_yn, "
								 " coupon_code, buy_user, sale_user, if(fixamt_yn = '0', comp_amt, price_amt) as sale_amt, id as cont_id "
								 " from %s.T_DEAL_INFO "
								 " where id = %lu and cont_gu = '%s' and fixamt_yn in ('0', '5', '6', '7', '8', '9','P') " 
								 " and deal_date = '%s' and deal_time <= '%s' "
								 " order by deal_time "
								 , szDbName, ulID, pContGu, pStopDate, pStopTime);
			}	
			
			ZzLOG(ALWAY, " Daem5123InsertRecallListDnUser: %s\n\n", szQuery);
			
			if(mysql_query(gcon, szQuery))
			{
				ZzLOG(ERROR, "Daem5123InsertRecallListDnUser : mysql_query error...\n");
				ZzLOG(ERROR, "Daem5123InsertRecallListDnUser : %s\n\n",szQuery);
				ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
				return -1;
			}
			
			if(!(res_backup = mysql_store_result(gcon)))
			{
				ZzLOG(ERROR, "Daem5123InsertRecallListDnUser: mysql_store_result error...\n");
				ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
				return  -1;
			}
			
		 	if(mysql_num_rows(res_backup)==0)
		 	{
				//ZzLOG(ERROR, "Daem5123InsertRecallListDnUser: 오류시간대에 %d번(%s) 컨텐츠의 구매내역이 없습니다...\n", id, pContGu);
				//ZzLOG(ERROR, "Daem5123InsertRecallListDnUser: szQuery\n( %s )\n",szQuery);
				mysql_free_result(res_backup);
				continue;
			}
			while(row_backup = mysql_fetch_row(res_backup))
			{
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " insert into zangsi_bck.T_RECALL_LIST                         "
								 " (seq_no, deal_no, cont_gu,                                    "
								 " deal_date, deal_time, fixamt_yn, coupon_code,                "
								 " buy_user, sale_user, reg_date, reg_time, sale_amt,           "
								 " proc_flag, cont_id, title, server_id, err_date, err_time, stop_date, stop_time)            "
								 " select                                                       "
								 " ifnull(max(seq_no)+1,0), %d, '%s',               "
								 " '%s', '%s', '%s', '%s',        "
								 " '%s', '%s', '%s', '%s', %d, "
								 " 'P', %lu, '%s', '%s', '%s', '%s', '%s', '%s'  "
								 " from zangsi_bck.T_RECALL_LIST                                "
								 , getint(row_backup,0), getstr(row_backup,1)
								 , getstr(row_backup,2), getstr(row_backup,3) , getstr(row_backup,4), getstr(row_backup,5)
								 , getstr(row_backup,6), getstr(row_backup,7), gszRegDate, gszRegTime, getint(row_backup,8)
								 , ulID, pServerID, pTitle, pErrDate, pErrTime, pStopDate, pStopTime);

				ZzLOG(ALWAY,"Daem5123InsertRecallListDnUser:  %s\n\n",szQuery);
				if(mysql_query(gcon, szQuery))
				{
					ZzLOG(ERROR, "Daem5123InsertRecallListDnUser : mysql_query error...\n");
					ZzLOG(ERROR, "Daem5123InsertRecallListDnUser : %s\n\n",szQuery);
					ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
					mysql_free_result(res_backup);
					return -1;
				}
				nCount++;
			}
			mysql_free_result(res_backup);
			ZzLOG(ALWAY, "Daem5123InsertRecallListDnUser: 컨텐츠 %d(%s)의 오류시간대 구매건수 = (%lu)\n", ulID, pContGu, nCount);
			gnTotalCount = gnTotalCount + nCount;
		}
	}
	else	//pErrDate 보다 pStopDate가 더 작다면 error
	{
		ZzLOG(ERROR, "Daem5123InsertRecallListDnUser: 처리일자 에러...[err_date=(%s)][stop_date=(%s)]\n" ,pErrDate, pStopDate);
		return -2;
	}
	return 0;
		
}
int Daem5123InsertRecallListUpUser(unsigned long ulID, char* pContGu, char* pSaleUser, int nSaleAmt, char* pTitle, char* pContRegDate, char* pContRegTime, char* pServerID, int nServerSeqNo, char* pErrDate, char* pErrTime, char* pStopDate, char* pStopTime)
{
	//복구 불가 인서버의 컨텐츠정보를 T_RECALL_LIST에 추가
	char szQuery[10000];
	memset(szQuery, 0x00, sizeof(szQuery));
	
	sprintf(szQuery, " insert into zangsi_bck.T_RECALL_LIST "
					 " (seq_no, inout_cd, deal_no, cont_gu, "
					 " deal_date, deal_time, fixamt_yn, coupon_code, "
					 " buy_user, sale_user, reg_date, reg_time, sale_amt, "
					 " proc_flag, cont_id, title, server_id, server_seq_no, err_date, err_time, stop_date, stop_time) "
					 " select "
					 " ifnull(max(seq_no)+1,0), 'S', 00, '%s', "
					 " '%s', '%s', '0', '0', "
					 " 'none', '%s', '%s', '%s', %d, "
					 " 'P', %lu, '%s', '%s', '%d', '%s', '%s', '%s', '%s' "
					 " from zangsi_bck.T_RECALL_LIST "
					 , pContGu
					 , pContRegDate, pContRegTime
					 , pSaleUser, gszRegDate, gszRegTime, nSaleAmt
					 , ulID, pTitle, pServerID, nServerSeqNo, pErrDate, pErrTime, pStopDate, pStopTime);
			
	ZzLOG(ALWAY, "Daem5123InsertRecallListDnUser:  %s\n",szQuery);
	if(mysql_query(gcon, szQuery))
	{
		ZzLOG(ERROR, "Daem5123InsertRecallListUpUser : mysql_query error...\n");
		ZzLOG(ERROR, "Daem5123InsertRecallListUpUser : %s\n\n",szQuery);
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
		return -1;
	}
	
	return 0;
		
}
char* AppendSpecialChar(char* strSrcString, char cReplace,char* pResult)
{
	char strResult[130000];
	memset(strResult,0x00,sizeof(strResult));
	
	int cTemp;
	int nSpecialCount=0;
	int nFileLen = strlen(strSrcString);
	int cOldTemp ;

	for(unsigned long i=0; i<nFileLen ; i++)
	{
		if( i > 0 )
		{
			cOldTemp = strSrcString[i-1];
		}
		cTemp = strSrcString[i];
		
		if( cTemp == '\'' && cOldTemp != '\\' ) 
		{
			strResult[nSpecialCount] = (char)cReplace;
			nSpecialCount++;
			strResult[nSpecialCount] = (char)cTemp;
		}
		else
		{
			if(cTemp<0)//한글 (is korean)
			{
				char* pHan = &strSrcString[i];
				memcpy(&strResult[nSpecialCount] ,pHan,2);
				nSpecialCount++;
				
				i=i+1;
			}
			else
			{
				strResult[nSpecialCount] = (char)cTemp;		
			}
		}
		nSpecialCount++;
	}
	strcpy(pResult,strResult);

	return pResult;
}

/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int Daem5123GetSystemDate()
{
	MYSQL_RES *res; 
	MYSQL_ROW  row; 

	char szQuery[1000];		// query string
	char sztemp [100];      // query temp

	memset(szQuery, 0x00, sizeof(szQuery));
	if(strcmp(gszSysDate, "00000000")==0)
	{
		strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d')");
		strcat(szQuery, "     , date_format(now(),'%H%i%s')");
		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -1 DAY),'%Y%m%d')");
		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -6 DAY),'%Y%m%d')");
	}
	else
	{
		strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d')");
		strcat(szQuery, "     , date_format(now(),'%H%i%s')");
		strcat(szQuery, "     , '");
		strcat(szQuery, gszSysDate);
		strcat(szQuery, "'");
		strcat(szQuery, "     , '");
		strncat(szQuery, gszSysDate, 6);
		strcat(szQuery, "'");
	}
	if(mysql_query(gcon, szQuery))
	{
	    ZzLOG(ERROR, "sysdate: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon), mysql_error(gcon));
		return -1;
	}
	if(!(res = mysql_store_result(gcon)))
	{
	    ZzLOG(ERROR, "sysdate: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon), mysql_error(gcon));
		return -1;
	}
 	if(mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "sysdate: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon), mysql_error(gcon));
		mysql_free_result(res);
		return -1;
	}
	
	row = mysql_fetch_row(res);
	memset(gszRegDate , 0x00, sizeof(gszRegDate ));
	memset(gszRegTime , 0x00, sizeof(gszRegTime ));
	memset(gszProcDate, 0x00, sizeof(gszProcDate));
	memset(gszBckDate, 0x00, sizeof(gszBckDate));
	
	strcpy(gszRegDate ,   getstr(row, 0));
	strcpy(gszRegTime ,   getstr(row, 1));
	strcpy(gszProcDate,   getstr(row, 2));
	strcpy(gszBckDate,   getstr(row, 3));
	
	mysql_free_result(res);

	return 0;
}

/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int Daem5123InitProcess(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
    #ifdef __DEBUG
    ZzInitGlobalVariable2("D_daem5123", "/logs/daemon"); 
    #else
    ZzInitGlobalVariable2("daem5123", "/logs/daemon"); 
    #endif

    ZzLOG(ALWAY, "[daem5123]*****************프로그램 시작*****************\n");  
    

    // 파라미터 값 설정 및 초기화
    if(argc != 2)
    {
	    ZzLOG(ERROR, "usage : %s 00000000\n", argv[0]);
	    return -1;
    }

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if(!(gcon=db_connect_nodb("")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		db_disconnect(gcon);
	   	return(-1); 
	}

	/* 처리일자 */
	memset(gszSysDate, 0x00, sizeof(gszSysDate));
	strcpy(gszSysDate, argv[1]);
	ret=Daem5123GetSystemDate();
	if(ret < 0)
	{
		db_disconnect(gcon);
		return -1;
	}
	
    return (0);
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int Daem5123TermProcess()
{
    // DB close
	if(tran_end(gcon))
	{
	    ZzLOG(ERROR, "sysdate: MasterDB tran_end() error ...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon), mysql_error(gcon));
	}	

	db_disconnect(gcon);
	
    ZzLOG(ALWAY, "[daem5123]*****************프로그램 종료*****************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  Daem5123Signal(int nSignal)
{
    Daem5123TermProcess();
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
	signal(SIGTERM, Daem5123Signal);
	signal(SIGINT,  Daem5123Signal);
	signal(SIGQUIT, Daem5123Signal);
	signal(SIGKILL, Daem5123Signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if( Daem5123InitProcess(argc, argv) == 0 ) 
	{
		/* 프로그램 메인루틴 */
		if(Daem5123DnUserTotalProcess() < 0)
		{
			if(tran_end(gcon))
			{
			    ZzLOG(ERROR, "sysdate: MasterDB tran_end() error ...\n");
				ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon), mysql_error(gcon));
			}	
		
			db_disconnect(gcon);			
		}

		if(Daem5123UpUserTotalProcess() < 0)
		{
			if(tran_end(gcon))
			{
			    ZzLOG(ERROR, "sysdate: MasterDB tran_end() error ...\n");
				ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon), mysql_error(gcon));
			}	
		
			db_disconnect(gcon);			
		}
			
		Daem5123TermProcess();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
