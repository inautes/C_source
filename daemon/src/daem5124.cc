/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5124.cc
 *         기능 : 리콜대상자 보상처리 (zangsi.T_BUZ_GIVE_ROB_MNY)
 *         설명 : 
 *       작성자 : HCS
 *       작성일 : 2008/01/17
 *     수정이력 : no.753(2010.10.21)
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

int Daem5124InitPrecess(int argc, char **argv); 
int Daem5124BuyUserProcess(); 
int Daem5124SaleUserProcess(); 
int Daem5124TermProcess(); 
int Daem5124InsertCompensationList(unsigned long ulBuzID, unsigned long ulDealNo, char *pBuyUser, int nSaleAmt);
int Daem5124RepayProcess(unsigned long ulBuzID, unsigned long ulContentsID, int nSaleAmt, char *pBuyUser, char *pSaleUser, char *pTitle);
int Daem5124RepayToUpUser(unsigned long ulContentsID, int nSaleAmt, char *pSaleUser, char * pBuyUser, char *pTitle, char *pContGu);

char* AppendSpecialChar(char* strSrcString, char cReplace,char* pResult);

int Daem5124GetSystemDate(); 
void Daem5124Signal(int nSignal); 

MYSQL     *gcon; 
MYSQL     *gcon_bck;

char   greg_date  [  8+1];	//등록일
char   gdel_date  [  8+1];	//T_CONTENTS_AMDDEL에 넣을 날짜
char   greg_time  [  6+1];	//등록시간


//******************************************************************************
//* daem5124 main
//******************************************************************************
int Daem5124BuyUserProcess()
{
	MYSQL_RES *res_bck;
	MYSQL_ROW  grow_bck;
	
	char szQuery[10000];		// query string
	memset(szQuery, 0x00, sizeof(szQuery));

	int nSeqNO = 0;
	unsigned long ulDealNo = 0; 
	int nSaleAmt = 0;
	unsigned long ulContentsID = 0;
	unsigned long ulBuzID = 0;

	char szBuyUser[12+1];
	memset(szBuyUser, 0x00, sizeof(szBuyUser));

	char szSaleUser[12+1];
	memset(szSaleUser, 0x00, sizeof(szSaleUser));

	char szTitle[10000];
	memset(szTitle, 0x00, sizeof(szTitle));

	int ret=0;
	
	/*
	다운로드 보상 시작.
	*/
	strcpy(szQuery, " select seq_no, deal_no, buy_user, sale_user, sale_amt, cont_id, title from zangsi_bck.T_RECALL_LIST "
					" where proc_flag = 'P' and inout_cd = 'B' ");

	ZzLOG(ALWAY, "Daem5124BuyUserProcess : %s\n\n",szQuery);

	if(mysql_query(gcon_bck, szQuery))
	{
		ZzLOG(ERROR, "Daem5124BuyUserProcess : mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon_bck), mysql_error(gcon_bck));
		return -1;
	}

	if(!(res_bck = mysql_store_result(gcon_bck)))
	{
		ZzLOG(ERROR, "Daem5124BuyUserProcess: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon_bck), mysql_error(gcon_bck));
		return  -1;
	}
	
 	if(mysql_num_rows(res_bck)==0)
 	{
		ZzLOG(ERROR, "Daem5124BuyUserProcess: 처리할 목록이 없습니다...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon_bck), mysql_error(gcon_bck));
		mysql_free_result(res_bck);
		return 1;
	}
	
	while(grow_bck = mysql_fetch_row(res_bck))
//	grow_bck = mysql_fetch_row(res_bck);
	{
		if(tran_begin(gcon)!=0) 
		{
		    ZzLOG(ERROR, "tran_begin: Mster테이베이스 오류입니다.\n");  
			ZzLOG(ERROR, "Daem5124BuyUserProcess: [%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			mysql_free_result(res_bck);
		    return -1;
		}
		
		if(tran_begin(gcon_bck)!=0) 
		{
		    ZzLOG(ERROR, "tran_begin: Backup테이베이스 오류입니다.\n");  
			ZzLOG(ERROR, "Daem5124BuyUserProcess: [%d](%s)\n",mysql_errno(gcon_bck), mysql_error(gcon_bck));
			mysql_free_result(res_bck);
			tran_rollback(gcon);
		    return -1;
		}

		nSeqNO = getint(grow_bck,0);
		ulDealNo = (unsigned long)getnum(grow_bck,1);
		strcpy(szBuyUser, getstr(grow_bck,2));
		strcpy(szSaleUser, getstr(grow_bck,3));
		nSaleAmt = getint(grow_bck,4);
		ulContentsID = (unsigned long)getnum(grow_bck,5);
		
		AppendSpecialChar(getstr(grow_bck, 6) , '\\' , szTitle);
		
		/*
		T_BUZ_GIVE_ROB_MNY에 INSERT
		*/
		
		if(Daem5124InsertCompensationList(ulBuzID, ulDealNo, szBuyUser, nSaleAmt) < 0)
		{
			mysql_free_result(res_bck);
			tran_rollback(gcon);
			tran_rollback(gcon_bck);
			
			return -1;		
		}
		/*
		T_POINT_IN에 UPDATE or INSERT 
		*/

		if(Daem5124RepayProcess(ulBuzID, ulContentsID, nSaleAmt, szBuyUser, szSaleUser, szTitle) < 0)
		{
			mysql_free_result(res_bck);
			tran_rollback(gcon);
			tran_rollback(gcon_bck);
			return -1;
		}
		
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update zangsi_bck.T_RECALL_LIST "
						" set proc_flag = 'Y', recall_date = '%s', recall_time = '%s' "
						" where seq_no = %d and deal_no = %lu "
						,greg_date, greg_time, nSeqNO, ulDealNo);
		
		ZzLOG(ALWAY, "Daem5124BuyUserProcess : %s\n\n",szQuery);
		
		if(mysql_query(gcon_bck, szQuery))
		{
			ZzLOG(ERROR, "Daem5124BuyUserProcess : mysql_query error...\n");
			ZzLOG(ERROR, "Daem5124BuyUserProcess : %s\n\n",szQuery);
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon_bck), mysql_error(gcon_bck));
			mysql_free_result(res_bck);
			tran_rollback(gcon);
			tran_rollback(gcon_bck);
			
			return -1;
		}
		
		if(tran_commit(gcon)!=0)
		{
		    ZzLOG(ERROR, "Daem5124BuyUserProcess: Master tran_commit error...\n");
			ZzLOG(ERROR, "Daem5124BuyUserProcess: [%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			mysql_free_result(res_bck);
			tran_rollback(gcon);
			tran_rollback(gcon_bck);
			
		    return -1;
		}
		
		if(tran_commit(gcon_bck)!=0)
		{
		    ZzLOG(ERROR, "Daem5124BuyUserProcess: Backup tran_commit error...\n");
			ZzLOG(ERROR, "Daem5124BuyUserProcess: [%d](%s)\n",mysql_errno(gcon_bck), mysql_error(gcon_bck));
			mysql_free_result(res_bck);
			tran_rollback(gcon);
			tran_rollback(gcon_bck);

		    return -1;
		}
	}

	mysql_free_result(res_bck);

	return (0);
}

int Daem5124SaleUserProcess()
{
	/*
	업로드 보상 시작.
	*/
	MYSQL_RES *res_bck;
	MYSQL_ROW  grow_bck;
	
	char szQuery[10000];		// query string
	memset(szQuery, 0x00, sizeof(szQuery));

	int nSeqNO = 0;
	unsigned long ulDealNo = 0; 
	int nSaleAmt = 0;
	unsigned long ulContentsID = 0;
	unsigned long ulBuzID = 0;

	char szBuyUser[12+1];
	memset(szBuyUser, 0x00, sizeof(szBuyUser));

	char szSaleUser[12+1];
	memset(szSaleUser, 0x00, sizeof(szSaleUser));

	char szTitle[10000];
	memset(szTitle, 0x00, sizeof(szTitle));
	
	char szContGu[2+1];
	memset(szContGu, 0x00, sizeof(szContGu));

	int ret=0;
	
	memset(szQuery, 0x00, sizeof(szQuery));
	strcpy(szQuery, " select seq_no, deal_no, buy_user, sale_user, sale_amt, cont_id, title, cont_gu from zangsi_bck.T_RECALL_LIST "
					" where proc_flag = 'P' and inout_cd = 'S' ");

	ZzLOG(ALWAY, "Daem5124BuyUserProcess : %s\n\n",szQuery);

	if(mysql_query(gcon_bck, szQuery))
	{
		ZzLOG(ERROR, "Daem5124BuyUserProcess : mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon_bck), mysql_error(gcon_bck));
		return -1;
	}

	if(!(res_bck = mysql_store_result(gcon_bck)))
	{
		ZzLOG(ERROR, "Daem5124BuyUserProcess: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon_bck), mysql_error(gcon_bck));
		return  -1;
	}
	
 	if(mysql_num_rows(res_bck)==0)
 	{
		ZzLOG(ERROR, "Daem5124BuyUserProcess: 처리할 목록이 없습니다...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon_bck), mysql_error(gcon_bck));
		mysql_free_result(res_bck);
		return 1;
	}
	
	while(grow_bck = mysql_fetch_row(res_bck))
	{
		if(tran_begin(gcon)!=0) 
		{
		    ZzLOG(ERROR, "tran_begin: Mster테이베이스 오류입니다.\n");  
			ZzLOG(ERROR, "Daem5124BuyUserProcess: [%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			mysql_free_result(res_bck);
		    return -1;
		}
		
		if(tran_begin(gcon_bck)!=0) 
		{
		    ZzLOG(ERROR, "tran_begin: Backup테이베이스 오류입니다.\n");  
			ZzLOG(ERROR, "Daem5124BuyUserProcess: [%d](%s)\n",mysql_errno(gcon_bck), mysql_error(gcon_bck));
			mysql_free_result(res_bck);
			tran_rollback(gcon);
		    return -1;
		}

		nSeqNO = getint(grow_bck,0);
		ulDealNo = (unsigned long)getnum(grow_bck,1);
		strcpy(szBuyUser, getstr(grow_bck,2));
		strcpy(szSaleUser, getstr(grow_bck,3));
		nSaleAmt = getint(grow_bck,4);
		ulContentsID = (unsigned long)getnum(grow_bck,5);

		AppendSpecialChar(getstr(grow_bck, 6) , '\\' , szTitle);

		strcpy(szContGu, getstr(grow_bck,7));
		
		if(Daem5124RepayToUpUser(ulContentsID, nSaleAmt, szSaleUser, szBuyUser, szTitle, szContGu) < 0)
		{
			mysql_free_result(res_bck);
			tran_rollback(gcon);
			tran_rollback(gcon_bck);			
			return -1;		
		}
		
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update zangsi_bck.T_RECALL_LIST "
						" set proc_flag = 'Y', recall_date = '%s', recall_time = '%s' "
						" where seq_no = %d and deal_no = %lu "
						,greg_date, greg_time, nSeqNO, ulDealNo);
		
		ZzLOG(ALWAY, "Daem5124BuyUserProcess : %s\n\n",szQuery);
		
		if(mysql_query(gcon_bck, szQuery))
		{
			ZzLOG(ERROR, "Daem5124BuyUserProcess : mysql_query error...\n");
			ZzLOG(ERROR, "Daem5124BuyUserProcess : %s\n\n",szQuery);
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon_bck), mysql_error(gcon_bck));
			mysql_free_result(res_bck);
			tran_rollback(gcon);
			tran_rollback(gcon_bck);
			
			return -1;
		}
		
		if(tran_commit(gcon)!=0)
		{
		    ZzLOG(ERROR, "Daem5124BuyUserProcess: Master tran_commit error...\n");
			ZzLOG(ERROR, "Daem5124BuyUserProcess: [%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			mysql_free_result(res_bck);
			tran_rollback(gcon);
			tran_rollback(gcon_bck);
			
		    return -1;
		}
		
		if(tran_commit(gcon_bck)!=0)
		{
		    ZzLOG(ERROR, "Daem5124BuyUserProcess: Backup tran_commit error...\n");
			ZzLOG(ERROR, "Daem5124BuyUserProcess: [%d](%s)\n",mysql_errno(gcon_bck), mysql_error(gcon_bck));
			mysql_free_result(res_bck);
			tran_rollback(gcon);
			tran_rollback(gcon_bck);

		    return -1;
		}
	}

	mysql_free_result(res_bck);	
	
	return 0;	
}
int Daem5124InsertCompensationList(unsigned long ulBuzID, unsigned long ulDealNo, char *pBuyUser, int nSaleAmt)
{
	//집계된 리콜대상자들을 T_BUZ_GIVE_ROB_MNY에 INSERT
	MYSQL_RES *res; 
	MYSQL_ROW  row; 

	char szQuery[10000];
	memset(szQuery, 0x00, sizeof(szQuery));

	strcpy(szQuery, " select ifnull(max(id)+1, 0) from zangsi.T_BUZ_GIVE_ROB_MNY ");

	ZzLOG(ALWAY, "Daem5124InsertCompensationList : %s\n\n",szQuery);
	if(mysql_query(gcon, szQuery))
	{
		ZzLOG(ERROR, "Daem5124InsertCompensationList : mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
		return -1;
	}

	if(!(res = mysql_store_result(gcon)))
	{
		ZzLOG(ERROR, "Daem5124InsertCompensationList: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon), mysql_error(gcon));
		return  -1;
	}

 	if(mysql_num_rows(res)==0)
 	{
		ZzLOG(ERROR, "Daem5124InsertCompensationList: T_BUZ_GIVE_ROB_MNY 발번 실패...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon), mysql_error(gcon));
		mysql_free_result(res);
		return -1;
	}

	row = mysql_fetch_row(res);
	ulBuzID = (unsigned long)getnum(row,0);
	
	mysql_free_result(res);

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " insert into zangsi.T_BUZ_GIVE_ROB_MNY "
					 " (gubun, id, deal_no, "
					 " pay_gubun, real_date, user_id, amt, "
					 " gm_code, descript, reg_user, reg_date, reg_time) "
					 " values "
					 " ('G', %lu, %lu, "
					 " 'point', '%s', '%s', %d, "
					 " '04', '컨텐츠 구매 보상', 'system', '%s', '%s') "
					 , ulBuzID, ulDealNo
					 , greg_date, pBuyUser, nSaleAmt
					 , greg_date, greg_time);

	ZzLOG(ALWAY, "Daem5124InsertCompensationList : %s\n\n",szQuery);
	if(mysql_query(gcon, szQuery))
	{
		ZzLOG(ERROR, "Daem5124InsertCompensationList : mysql_query error...\n");
		ZzLOG(ERROR, "Daem5124InsertCompensationList : %s\n\n",szQuery);
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
		return -1;
	}

	return 0;
}

int Daem5124RepayProcess(unsigned long ulBuzID, unsigned long ulContentsID, int nSaleAmt, char *pBuyUser, char *pSaleUser, char *pTitle)
{
	/*
	T_POINT_IN에 INSERT (포인트 적립내역)
	T_POINT_INFO에 UPDATE (포인트로 보상)
	*/
	//포인트 적립내역

	MYSQL_RES *res; 
	MYSQL_ROW  row; 

	char szQuery[10000];
	memset(szQuery, 0x00, sizeof(szQuery));

	char szDisc[1000];
	memset(szDisc, 0x00, sizeof(szDisc));
	
	sprintf(szDisc, "컨텐츠(%lu) 포인트로 환불 : %s님에게 %dp 적립", ulContentsID, pBuyUser, nSaleAmt);
	
	sprintf(szQuery, " insert into zangsi.T_POINT_IN "
					 " ( seq_no, point_cd, user_id "
					 " , in_date, in_time "
					 " , inout_seq_no, in_point "
					 " , descript, reg_date, reg_time, cnl_yn) "
					 " select "
					 " ifnull(max(seq_no)+1,0), '99', '%s' "
					 " , '%s', '%s' "
					 " , %lu, %d "
					 " , '%s', '%s', '%s', 'N' "
					 " from zangsi.T_POINT_IN "
					 , pBuyUser
					 , greg_date, greg_time
					 , ulBuzID, nSaleAmt
					 , szDisc, greg_date, greg_time);
	
	ZzLOG(ALWAY, "Daem5124RepayProcess : %s\n\n",szQuery);
	if(mysql_query(gcon, szQuery))
	{
		ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
		ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
		return -1;
	}
	
	//포인트로 보상
	
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select * from zangsi.T_POINT_INFO where user_id = '%s' ", pBuyUser);
	
	ZzLOG(ALWAY, "Daem5124RepayProcess : %s\n\n",szQuery);
	if(mysql_query(gcon, szQuery))
	{
		ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
		ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
		return -1;
	}

	if(!(res = mysql_store_result(gcon)))
	{
		ZzLOG(ERROR, "Daem5124InsertCompensationList: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon), mysql_error(gcon));
		return  -1;
	}

 	if(mysql_num_rows(res)==0)
 	{
		memset(szQuery, 0x00, sizeof(szQuery));
			
		sprintf(szQuery, " insert into zangsi.T_POINT_INFO values ('%s', %d, 0, %d) ", pBuyUser, nSaleAmt, nSaleAmt);
		
		ZzLOG(ALWAY, "Daem5124RepayProcess : %s\n\n",szQuery);
		if(mysql_query(gcon, szQuery))
		{
			ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
			ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			mysql_free_result(res);
			return -1;
		}		
	}
	else
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update zangsi.T_POINT_INFO set get_amt = get_amt + %d, cur_amt = cur_amt + %d where user_id = '%s' "
					 	 , nSaleAmt, nSaleAmt, pBuyUser);
		
		ZzLOG(ALWAY, "Daem5124RepayProcess : %s\n\n",szQuery);
		if(mysql_query(gcon, szQuery))
		{
			ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
			ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			mysql_free_result(res);
			return -1;
		}
	}	
	mysql_free_result(res);
	
	/*
	메모 보내기
	*/
	
	char szMemo[1000];
	memset(szMemo, 0x00, sizeof(szMemo));
	
	sprintf(szMemo, "안녕하세요 위디스크 운영팀입니다.\r\n\r\n"
					"회원님께서 내려받기 하신 컨텐츠 (%lu번 %s)는 서버오류가 발생하여\r\n\r\n"
					"내려받기가 정상적으로 되지 않아 다른 컨텐츠를 내려받기 하실 수 있도록 포인트를 보내드립니다.\r\n\r\n"
					"보내드린 포인트를 사용하여 다른 회원분의 동일한 컨텐츠를 다시 내려받기하여 보시기 바랍니다.\r\n\r\n"
					"서비스 이용에 불편을 드린점 다시 한번 사과드리며, 더욱 쾌적하고 믿을수 있는 위디스크가 되도록 노력하겠습니다.\r\n\r\n"
					, ulContentsID, pTitle);
	
	memset(szQuery, 0x00, sizeof(szQuery));
/*	
	sprintf(szQuery, " insert into zangsi.T_MEMO_INFO "
					 " ( user_id, seq_no "
					 " , memo_cd, ref_id "
					 " , descript "
					 " , send_user, recv_yn "
					 " , recv_date, recv_time) "
					 " select '%s', ifnull(max(seq_no)+1,0) "
					 " ,'01', 0 " 
					 " , '%s' " 
					 " , '운영팀', 'N' "
					 " , '%s', '%s'  "
					 " from zangsi.T_MEMO_INFO "
					 , pBuyUser, szMemo, greg_date, greg_time);
	
	ZzLOG(ALWAY, "Daem5124RepayProcess : %s\n\n",szQuery);
	if(mysql_query(gcon, szQuery))
	{
		ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
		ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
		return -1;
	}
*/

	sprintf(szQuery, " insert into zangsi.T_SEND_MEMO (  memo_cd, descript,send_user,  del_yn, send_date, send_time ) " 
						" values (  '05' "
						
						" ,'%s' "
						
						" , '운영팀' ,'N', '%s', '%s' ) "
						, szMemo, greg_date, greg_time);

	ZzLOG(ALWAY, "[%s]\n\n", szQuery);
	if(mysql_query(gcon, szQuery))
	{
		ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
		ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
		return -1;
	}

	memset(szQuery , 0x00, sizeof(szQuery ));
	strcpy( szQuery, "SELECT last_insert_id() as send_seq_no" );
	
	if(mysql_query(gcon, szQuery))
	{
		ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
		ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
		return -1;
	}
	
	MYSQL_RES* myres = mysql_store_result(gcon);
	MYSQL_ROW myrow = mysql_fetch_row(myres);

	double send_seq_no  = getnum(myrow,0 );	
	
	mysql_free_result(myres);
		
	
	memset(szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery," insert into zangsi.T_RECV_MEMO "
					"  (memo_seq_no, recv_user, recv_date, recv_time, read_yn, del_yn, save_yn)  "
					" values "
					"  (%.0f, '%s' , '%s', '%s' , 'N', 'N', 'N') "
					,send_seq_no,pBuyUser,greg_date, greg_time);
					
	ZzLOG(ALWAY, "[%s]\n\n", szQuery);
	if(mysql_query(gcon, szQuery))
	{
		ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
		ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
		return -1;
	}
		
}
int Daem5124RepayToUpUser(unsigned long ulContentsID, int nSaleAmt, char *pSaleUser, char * pBuyUser, char *pTitle, char *pContGu)
{
	/*
	보상 아이템 : 끌어올리기(11) 5개, 두껍게(02) 5개, 컬러 효과(01) 5개.
	
	1.보낼 아이템 내역을 기록.(zangsi.T_ITEM_SEND)
	2.리콜 대상자의 구매내역 히스토리에 기록.(20101023 : zangsi.T_ITEM_PAYMENT에서 zangsi.T_ITEMP_IN으로 변경[no.753])
	3.리콜 대상자의 아이템 갯수 증가 기록.(zangsi.T_ITEM_INFO)
	4.리콜 대상자에게 쪽지 전송.(zangsi.T_MEMO_INFO)
	*/
	MYSQL_RES *res; 
	MYSQL_ROW  row; 

	int i = 0;
	int nRepayItemCnt = 5;
	
	char szQuery[10000];
	memset(szQuery, 0x00, sizeof(szQuery));

	char szDisc[1000];
	memset(szDisc, 0x00, sizeof(szDisc));
	
	char szItemCode[2+1];
	memset(szItemCode, 0x00, sizeof(szItemCode));

	strcpy(szDisc, "운영팀으로부터 아이템 선물이 도착하였습니다.\r\n");
	strcat(szDisc, "아이템은 아이템 구매/보관에서 확인 하실 수 있습니다.\r\n\r\n");

	for(i=0; i<3; i++)
	{
		//1.보낼 아이템 내역을 기록.(zangsi.T_ITEM_SEND) 
		memset(szItemCode, 0x00, sizeof(szItemCode));
		if(i==0)
			strcpy(szItemCode, "01");
		if(i==1)
			strcpy(szItemCode, "02");
		if(i==2)
			strcpy(szItemCode, "11");
		
		sprintf(szQuery, " insert into zangsi.T_ITEM_SEND "
						 " ( send_user, recv_user, send_date, send_time "
						 " , send_mesg, item_code, recv_date, recv_time, item_count) "
						 " values "
						 " ('운영팀', '%s', '%s', '%s' "
						 " , '%s', '%s', '%s', '%s', %d) "
						 , pSaleUser, greg_date, greg_time
						 , szDisc, szItemCode, greg_date, greg_time, nRepayItemCnt);
		
		ZzLOG(ALWAY, "Daem5124RepayProcess : %s\n\n",szQuery);
		if(mysql_query(gcon, szQuery))
		{
			ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
			ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			return -1;
		}

		//2.리콜 대상자의 구매내역 히스토리에 기록.(zangsi.T_ITEM_PAYMENT)	
		sprintf(szQuery, " insert into zangsi.T_ITEM_IN "
						 " ( user_id, item_code, get_meth "
						 " , in_cnt, price_amt, descript "
						 " , reg_date, reg_time) "
						 " values "
						 " ('%s', '%s', '1' "
						 " , %d, 0, '%lu번 컨텐츠 보상' "
						 " , '%s', '%s') "
						 , pSaleUser, szItemCode
						 , nRepayItemCnt, ulContentsID
						 , greg_date, greg_time);
		
		ZzLOG(ALWAY, "Daem5124RepayProcess : %s\n\n",szQuery);
		if(mysql_query(gcon, szQuery))
		{
			ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
			ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			return -1;
		}

		//3.리콜 대상자의 아이템 갯수 증가 기록.(zangsi.T_ITEM_INFO)	
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " select * from zangsi.T_ITEM_INFO where user_id = '%s' and item_code = '%s' "
					 	 , pSaleUser, szItemCode);
		
		ZzLOG(ALWAY, "Daem5124RepayProcess : %s\n\n",szQuery);
		if(mysql_query(gcon, szQuery))
		{
			ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
			ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			return -1;
		}

		if(!(res = mysql_store_result(gcon)))
		{
			ZzLOG(ERROR, "Daem5124InsertCompensationList: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon), mysql_error(gcon));
			return  -1;
		}

	 	if(mysql_num_rows(res)==0)
	 	{
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi.T_ITEM_INFO "
							 " (user_id, item_code, buy_item, cur_item) " 
							 " values ('%s', '%s', %d, %d) "
							 , pSaleUser, szItemCode, nRepayItemCnt, nRepayItemCnt);
			
			ZzLOG(ALWAY, "Daem5124RepayProcess : %s\n\n",szQuery);
			if(mysql_query(gcon, szQuery))
			{
				ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
				ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
				ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
				mysql_free_result(res);
				return -1;
			}		
		}
		else
		{
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update zangsi.T_ITEM_INFO set buy_item = buy_item + %d, cur_item = cur_item + %d " 
							 " where user_id = '%s' and item_code = '%s' "
						 	 , nRepayItemCnt, nRepayItemCnt, pSaleUser, szItemCode);
			
			ZzLOG(ALWAY, "Daem5124RepayProcess : %s\n\n",szQuery);
			if(mysql_query(gcon, szQuery))
			{
				ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
				ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
				ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
				mysql_free_result(res);
				return -1;
			}
		}	
		mysql_free_result(res);
	}
	
	/*
	메모 보내기
	"안녕하세요 위디스크 운영팀입니다\r\n\r\n";                                                             
	"szSaleUser님께서 올리신\r\n szTitle 컨텐츠에 대한\r\n포인트환불이 이루어졌습니다\r\n\r\n"
	"감사합니다"	
	*/
	char szMemo[1000];
	memset(szMemo, 0x00, sizeof(szMemo));
	
	sprintf(szMemo, "안녕하세요 위디스크 운영팀입니다\r\n\r\n"
					"회원님께서 등록하신 컨텐츠(%lu번 %s)는 저장된 서버에 오류가 발생하여\r\n\r\n"
					"기술지원팀에서 많은 시간과 노력을 들여 서버를 복구하였으나 등록하신 해당컨텐츠는 복구가\r\n\r\n"
					"안되는 것으로 확인되어 운영팀에서 임의로 삭제하였습니다.\r\n\r\n"
					"회원님께서 많은 시간을 들여 힘들게 등록을 하셨는데 서버오류 문제로 삭제가 되어 회원님께 머리숙여 사과드립니다.\r\n\r\n"
					"그리고 힘들게 노력하여 등록하신 컨텐츠에 비할바는 아니지만 소정의 아이템을 보상으로 보내드리겠습니다.\r\n\r\n"
					"서비스 이용에 불편을 드린점 다시 한번 사과드리며, 더욱 쾌적하고 믿을수 있는\r\n\r\n"
					"위디스크가 되도록 노력하겠습니다.\r\n\r\n"
					"감사합니다."
					, ulContentsID, pTitle);
	
	memset(szQuery, 0x00, sizeof(szQuery));
/*	
	sprintf(szQuery, " insert into zangsi.T_MEMO_INFO "
					 " ( user_id, seq_no "
					 " , memo_cd, ref_id "
					 " , descript "
					 " , send_user, recv_yn "
					 " , recv_date, recv_time) "
					 " select '%s', ifnull(max(seq_no)+1,0) "
					 " ,'01', 0 " 
					 " , '%s' " 
					 " , '운영팀', 'N' "
					 " , '%s', '%s'  "
					 " from zangsi.T_MEMO_INFO "
					 , pSaleUser, szMemo, greg_date, greg_time);
	
	ZzLOG(ALWAY, "Daem5124RepayProcess : %s\n\n",szQuery);
	if(mysql_query(gcon, szQuery))
	{
		ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
		ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
		return -1;
	}	
*/

	sprintf(szQuery, " insert into zangsi.T_SEND_MEMO (  memo_cd,  descript,send_user, del_yn, send_date, send_time ) " 
						" values (  '04' "
						
						" ,'%s' "
						
						" , '운영팀' ,'N', '%s', '%s' ) "
						, szMemo, greg_date, greg_time);

	ZzLOG(ALWAY, "[%s]\n\n", szQuery);
	if(mysql_query(gcon, szQuery))
	{
		ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
		ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
		return -1;
	}

	memset(szQuery , 0x00, sizeof(szQuery ));
	strcpy( szQuery, "SELECT last_insert_id() as send_seq_no" );
	
	if(mysql_query(gcon, szQuery))
	{
		ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
		ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
		return -1;
	}
	
	MYSQL_RES* myres = mysql_store_result(gcon);
	MYSQL_ROW myrow = mysql_fetch_row(myres);

	double send_seq_no  = getnum(myrow,0 );	
	
	mysql_free_result(myres);
		
	
	memset(szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery," insert into zangsi.T_RECV_MEMO "
					"  (memo_seq_no, recv_user, recv_date, recv_time, read_yn, del_yn, save_yn)  "
					" values "
					"  (%.0f, '%s' , '%s', '%s' , 'N', 'N', 'N') "
					,send_seq_no,pSaleUser,greg_date, greg_time);
					
	ZzLOG(ALWAY, "[%s]\n\n", szQuery);
	if(mysql_query(gcon, szQuery))
	{
		ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
		ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
		return -1;
	}	
	
	/*
	보상후 컨텐츠 삭제 처리(T_CONTENTS_ADMDEL에 인서트. daem5124처리후 두시간후 daem5004--컨텐츠 삭제 데몬이 삭제 처리
						    T_CONTENTS_INFO의 del_yn는 Y로 바꾸지 않음. daem5004가 돌기전에 리스트에서 삭제된걸로 보이지 않게 하기 위해)
	
	*/

	char szDesc[1000];
	memset(szDesc, 0x00, sizeof(szDesc));
	
	strcpy(szDesc, "리콜완료후 삭제(복구불가)");
	
	memset(szQuery, 0x00, sizeof(szQuery));
	
	if(strcmp(pContGu, "WE") == 0)
	{
		memset(pContGu, 0x00, sizeof(pContGu));
		strcpy(pContGu, "01");	
	}
		
	sprintf(szQuery, " replace into zangsi.T_CONTENTS_ADMDEL "
					 " (cont_gu, id, del_desc, proc_yn, reg_user, reg_date, reg_time) "
					 " values "
					 " ('%s', %lu, '%s', 'N', 'system', '%s', '%s')"
					 , pContGu, ulContentsID, szDesc, gdel_date, greg_time);
	
	ZzLOG(ALWAY, "Daem5124RepayProcess : %s\n\n",szQuery);
	if(mysql_query(gcon, szQuery))
	{
		ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
		ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
		return -1;
	}
	
	char szTableNm[32];
	memset(szTableNm, 0x00, sizeof(szTableNm));
	
	if(strcmp(pContGu, "01") == 0)
	{
		memset(pContGu, 0x00, sizeof(pContGu));
		strcpy(pContGu, "WE");
		
		strcpy(szTableNm, "T_CONTENTS_IMG");

		//----------------------------------------------------------------------
		// 검색엔진의 색인정보 삭제
		//----------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " INSERT INTO zangsi.T_CONTENTS_CREATE "
		                 " (id , cont_gu, udt_cd) "
		                 " VALUES "
		                 " (%lu, '01'   , 'D'   )"
		                 , ulContentsID);
	
		if(mysql_query(gcon, szQuery))
		{
			ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
			ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			return -1;
		}
		
		//no.753
		memset (szQuery, 0x00, sizeof(szQuery));
		//20100824 - no.575
		sprintf(szQuery, " update zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_VIR_ID2 b "
		                 " set b.del_yn = 'Y' "
		                 " where a.id = b.id and a.id = %lu and a.del_yn = 'N'"
		                 , ulContentsID);
		 #ifdef __DEBUG
		 printf("%s\n\n",szQuery);
		 #endif 
		 	                 
		if(mysql_query(gcon, szQuery))
		{
			ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
			ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			return -1;
		}
	
		memset (szQuery, 0x00, sizeof(szQuery));
		//20100824 - no.575
		sprintf(szQuery, " update zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_VIR_ID b "
		                 " set a.del_yn = 'Y', b.del_yn = 'Y' "
		                 " where a.id = b.id and a.id = %lu and a.del_yn = 'N'"
		                 ,ulContentsID);
		 #ifdef __DEBUG
		 printf("%s\n\n",szQuery);
		 #endif 
		 	                 
		if(mysql_query(gcon, szQuery))
		{
			ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
			ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			return -1;
		}
	}
	else
	{	
		//no.753 필로그 컨텐츠 삭제 처리
		
		strcpy(szTableNm, "T_CONTFLOG_IMG");
		
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " select b.file_size from zangsi.T_CONTFLOG_INFO a, zangsi.T_CONTFLOG_FILE b "
						 " where a.id = b.id and a.id = %lu ", ulContentsID);
		if(mysql_query(gcon, szQuery))
		{
			ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
			ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			return -1;
		}	
		if(!(res = mysql_store_result(gcon)))
		{
			ZzLOG(ERROR, "Daem5124_repay_process : mysql_store_result error...\n");
			ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			return -1;
		}	
		if(mysql_num_rows(res) <= 0)
		{
			ZzLOG(ERROR, "Daem5124_repay_process : mysql_num_rows error...\n");
			ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
			return -1;
		}
		row = mysql_fetch_row(res);
		double dFileSize = 0;
		dFileSize = getnum(row,0);
		mysql_free_result(res);
		
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " UPDATE zangsi.T_PERM_UPLOAD_AUTH "
						 " SET disk_use    = disk_use    - %15.0f "
						 " WHERE user_id   = '%s'  " 
						 , dFileSize
						 , pSaleUser);
		if(mysql_query(gcon, szQuery))
		{
			ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
			ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			return -1;
		}
		
		memset (szQuery, 0x00, sizeof(szQuery));
		//20100824 - no.575
		sprintf(szQuery, " update zangsi.T_CONTFLOG_INFO "
		                 " set del_yn = 'Y' "
		                 " where id = %lu and del_yn = 'N' "
		                 ,ulContentsID);
		 #ifdef __DEBUG
		 printf("%s\n\n",szQuery);
		 #endif 
		 	                 
		if(mysql_query(gcon, szQuery))
		{
			ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
			ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
			return -1;
		}
	}
	
	 
	//----------------------------------------------------------------------
	// 이미지 삭제 정보 추가
	//----------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " INSERT INTO zangsi.T_IMG_DEL "
	                 " (cont_gu, seq_no, filog_cn, file_path1, file_path2, file_name, reg_user, reg_date, reg_time)"
	                 " SELECT "
	                 "  cont_gu, img_no, 0, '/zangsi/project/zangsi/files_zangsi/contents/img',"
	                 "  img_spath, img_sname, reg_user, '%s', '%s'"
	                 " FROM zangsi.%s "
	                 " WHERE cont_gu = '%s' and id = %lu "
	                 , gdel_date
	                 , greg_time
	                 , szTableNm
	                 , pContGu
	                 , ulContentsID);
	                 

	if(mysql_query(gcon, szQuery))
	{
		ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
		ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
		return -1;
	}
	 
	//----------------------------------------------------------------------
	// 이미지 정보 삭제
	//----------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " DELETE "
					 " FROM zangsi.%s "
					 " WHERE cont_gu = '%s' and id = %lu "
					 , szTableNm
					 , pContGu
					 , ulContentsID);
	
	if(mysql_query(gcon, szQuery))
	{
		ZzLOG(ERROR, "Daem5124_repay_process : mysql_query error...\n");
		ZzLOG(ERROR, "Daem5124_repay_process : %s\n\n",szQuery);
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(gcon), mysql_error(gcon));
		return -1;
	}

	
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
int Daem5124GetSystemDate()
{
	MYSQL_RES *res_bck;
	MYSQL_ROW  grow_bck;

	char szQuery[1000];		// query string
	char sztemp [100];      // query temp

	memset(szQuery, 0x00, sizeof(szQuery));

	strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d')");
	strcat(szQuery, "     , date_format(now(),'%H%i%s')");
	strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -2 DAY),'%Y%m%d')");

	if(mysql_query(gcon_bck, szQuery))
	{
	    ZzLOG(ERROR, "sysdate: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon_bck), mysql_error(gcon_bck));
		return -1;
	}
	if(!(res_bck = mysql_store_result(gcon_bck)))
	{
	    ZzLOG(ERROR, "sysdate: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon_bck), mysql_error(gcon_bck));
		return -1;
	}
 	if(mysql_num_rows(res_bck)==0)
 	{
	    ZzLOG(ERROR, "sysdate: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon_bck), mysql_error(gcon_bck));
		mysql_free_result(res_bck);
		return -1;
	}
	
	grow_bck = mysql_fetch_row(res_bck);
	memset(greg_date , 0x00, sizeof(greg_date ));
	memset(gdel_date , 0x00, sizeof(gdel_date ));
	memset(greg_time , 0x00, sizeof(greg_time ));

	strcpy(greg_date ,   getstr(grow_bck, 0));
	
	strcpy(greg_time ,   getstr(grow_bck, 1));

	strcpy(gdel_date ,   getstr(grow_bck, 2));
	
	mysql_free_result(res_bck);

	return 0;
}

/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int Daem5124InitPrecess(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
    #ifdef __DEBUG
    ZzInitGlobalVariable2("D_daem5124", "/logs/daemon"); 
    #else
    ZzInitGlobalVariable2("daem5124", "/logs/daemon"); 
    #endif

    ZzLOG(ALWAY, "[daem5124]*****************프로그램 시작*****************\n");  
    

    // 파라미터 값 설정 및 초기화
    if(argc != 2)
    {
	    ZzLOG(ERROR, "usage : %s 00000000\n", argv[0]);
	    return -1;
    }

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if(!(gcon =	db_connect_local("zangsi")))
	{
		ZzLOG(ERROR, "Master DB에 접속하지 못 하였습니다...\n");
		db_disconnect(gcon);
	   	return(-1); 
	}

	if(!(gcon_bck=db_connect_nodb("")))
	{
		ZzLOG(ERROR, "Backup DB에 접속하지 못 하였습니다...\n");
		db_disconnect(gcon_bck);
	   	return(-1); 
	}
	
	
	/* 처리일자 */
	ret=Daem5124GetSystemDate();
	if(ret < 0)
	{
		db_disconnect(gcon);
		db_disconnect(gcon_bck);
		
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
int Daem5124TermProcess()
{
    // DB close
   	if(tran_end(gcon))
	{
	    ZzLOG(ERROR, "sysdate: MasterDB tran_end() error ...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon), mysql_error(gcon));
	}	
	if(tran_end(gcon_bck))
	{
	    ZzLOG(ERROR, "sysdate: BackUp tran_end() error ...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon), mysql_error(gcon));
	}	

	db_disconnect(gcon);
	db_disconnect(gcon_bck);
    ZzLOG(ALWAY, "[daem5124]*****************프로그램 종료*****************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  Daem5124Signal(int nSignal)
{
    Daem5124TermProcess();
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
	signal(SIGTERM, Daem5124Signal);
	signal(SIGINT,  Daem5124Signal);
	signal(SIGQUIT, Daem5124Signal);
	signal(SIGKILL, Daem5124Signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if(Daem5124InitPrecess(argc, argv) == 0) 
	{
		/* 프로그램 메인루틴 */
		if(Daem5124BuyUserProcess() < 0)
		{
		   	if(tran_end(gcon))
			{
			    ZzLOG(ERROR, "sysdate: MasterDB tran_end() error ...\n");
				ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon), mysql_error(gcon));
			}	
			if(tran_end(gcon_bck))
			{
			    ZzLOG(ERROR, "sysdate: BackUp tran_end() error ...\n");
				ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon), mysql_error(gcon));
			}	
			db_disconnect(gcon);
			db_disconnect(gcon_bck);
		}

		if(Daem5124SaleUserProcess() < 0)
		{
		   	if(tran_end(gcon))
			{
			    ZzLOG(ERROR, "sysdate: MasterDB tran_end() error ...\n");
				ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon), mysql_error(gcon));
			}	
			if(tran_end(gcon_bck))
			{
			    ZzLOG(ERROR, "sysdate: BackUp tran_end() error ...\n");
				ZzLOG(ERROR, "[%d](%s)",mysql_errno(gcon), mysql_error(gcon));
			}	
			db_disconnect(gcon);
			db_disconnect(gcon_bck);
		}
			
		Daem5124TermProcess();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
