/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : cprdaem1004.cc
 *         기능 : 하루 한번 제휴 컨텐츠 구매취소건에 대해 관리코드별로 집계
 *         설명 : proc_yn = 'F' Fail - 가격정보 조회 실패.
                  proc_yn = 'G' Garbage - 해시에 엮인 컨텐츠 정보가 없음.
 
 *     설치위치 : 유료컨텐츠DB에 위치
 *
 *       작성자 : HCS
 *       작성일 : 2009/02/09
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
#include <unistd.h> //for sleep()

#include "daemcom.h"
#include "commydb.h"

#define  MAX_ROWS	1
#define	 NUMBER		10
#define	 NORMAL		11

int cprdaem1004_process();
int cprdaem1004_sum_cnl_dd();
int cprdaem1004_process_init(int argc, char **argv);
int cprdaem1004_process_term();
void cprdaem1004_signal(int nSignal);


MYSQL     *cpr_con;
MYSQL     *bck_con;

MYSQL_RES *cpr_res;
MYSQL_ROW  cpr_row;

bool gbIsUserDate      ; //날짜입력
char gst_date[8+1];	// 처리일자
char gst_time[6+1];	// 처리시작시간
char g_proc_type[5];
char gproc_log[80]; // 로그메시지버퍼
int gnType = 0;

//******************************************************************************
//* cprdaem1004 main
//******************************************************************************
int cprdaem1004_process()
{
	char szQuery[1000];  // query string
	memset(szQuery, 0x00, sizeof(szQuery));	

	// 처리일자가 "00000000"일때는 시스템일자 -1을 한다.
	if (strcmp(gst_date, "00000000") == 0)
	{
		gbIsUserDate = false;
		strcpy(szQuery, "SELECT date_format(date_add(now(), INTERVAL - 1 DAY),'%Y%m%d') , date_format(now(),'%H%i%s')");


		if (mysql_query(cpr_con, szQuery)){
		    ZzLOG(ERROR, "sysdate: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_con), mysql_error(cpr_con));
			return -1;
		}
		
		if (!(cpr_res = mysql_store_result(cpr_con)))
		{
		    ZzLOG(ERROR, "sysdate: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_con), mysql_error(cpr_con));
			return -1;
		}
	 	if (mysql_num_rows(cpr_res)==0)
	 	{
		    ZzLOG(ERROR, "sysdate: mysql_num_rows error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_con), mysql_error(cpr_con));
			return -1;
		}
		cpr_row = mysql_fetch_row(cpr_res);
		memset(gst_date, 0x00, sizeof(gst_date));
		strcpy(gst_date,   getstr(cpr_row, 0));
		strcpy(gst_time,   getstr(cpr_row, 1));
		mysql_free_result(cpr_res);
				
	}
	else
	{
		gbIsUserDate=true;

	}
	
	#ifdef __DEBUG
	printf("적용 날짜 : %s\n",gst_date);
	#endif 
	
	//mgr_cd별로 구매정보 집계처리
	if( cprdaem1004_sum_cnl_dd())
	{
		ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd() 실패!!!\n");
		return -1;
	}
		
	return 0;
}

//******************************************************************************
//* cprdaem1004 db 처리로직
//******************************************************************************

int cprdaem1004_sum_cnl_dd()
{
    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "cprdaem1004_sum_cnl_dd start - gproc_date : [%s]\n", gst_date);  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");
	/*
	zangsi_bck.T_CNL_CPR_HIST에서 셀렉트
	*/
	MYSQL_RES *bck_res;
	MYSQL_ROW  bck_row;

	MYSQL_RES *res1;
	MYSQL_ROW  row1;

	char szQuery[1600];		// query string
	int  ret;
	
	char szBuffer[600];
	unsigned long ulCount = 0;
	unsigned long ulSeqNo = 0;
	unsigned long ulId = 0;
	unsigned long ulDealNo = 0;
	unsigned long ulOpDealNo = 0;
	
	char szDealDate[8+1];
	memset(szDealDate, 0x00, sizeof(szDealDate));
	
	char szDealTime[6+1];
	memset(szDealTime, 0x00, sizeof(szDealTime));
	
	char szSaleCd[2+1];
	memset(szSaleCd, 0x00, sizeof(szSaleCd));

	char szMgrCd[20+1];
	memset(szMgrCd, 0x00, sizeof(szMgrCd));
	
	char szCompCd[6+1];
	memset(szCompCd, 0x00, sizeof(szCompCd));

	char szAcctCd[2+1];
	memset(szAcctCd, 0x00, sizeof(szAcctCd));
	
	while(1)
	{
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " select seq_no, deal_no, op_deal_no, id, acct_cd from zangsi_bck.T_CNL_CPR_HIST " 
						 " where reg_date <= '%s' and proc_yn = 'N' order by deal_no limit 100 "
						 , gst_date);	

		if (mysql_query(bck_con, szQuery))
		{
		    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: select T_CPR_DEAL_INFO error...\n");
			ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: [%d](%s)\n",mysql_errno(bck_con), mysql_error(bck_con));
			ZzLOG(ERROR, "%s\n\n",szQuery);
			return -1;
	    }
	   	if (!(bck_res = mysql_store_result(bck_con))) 
		{
		    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(bck_con), mysql_error(bck_con));
			return -1;
		}
		if (mysql_num_rows(bck_res) <= 0)	
	 	{
		    ZzLOG(ALWAY, "cprdaem1004_sum_cnl_dd: 처리할 자료 없음. 집계한 취소건수 - (%lu)\n", ulCount);
		    mysql_free_result(bck_res);
		    return 0;
		}
		
		while(bck_row = mysql_fetch_row(bck_res))
		{
			/*
			가져온 딜넘버로 t_cpr_deal_info에서 정보 가져와 집계처리.
			*/
			ulSeqNo = 0;
			ulId = 0;
			ulDealNo = 0;
			ulOpDealNo = 0;
			memset(szAcctCd, 0x00, sizeof(szAcctCd));

			ulSeqNo = (unsigned long)getnum(cpr_row, 0);
			ulDealNo = (unsigned long)getnum(cpr_row, 1);
			ulOpDealNo = (unsigned long)getnum(cpr_row, 2);
			ulId = (unsigned long)getnum(cpr_row, 3);
			strcpy(szAcctCd, getstr(cpr_row, 4));
			
			memset(szQuery, 0x00, sizeof(szQuery));

			if( ulOpDealNo == 0)
			{
				sprintf(szQuery, " select seq_no, deal_date, deal_time, sale_cd from zangsi_cpr.T_CPR_DEAL_INFO " 
							 " where deal_no = %lu and id = %lu and proc_yn = 'Y'"
		   					 , ulDealNo, ulId);
		   	}
		   	else if(ulDealNo == 0)
		   	{
				sprintf(szQuery, " select seq_no, deal_date, deal_time, sale_cd from zangsi_cpr.T_CPR_DEAL_INFO " 
							 " where op_deal_no = %lu and id = %lu and proc_yn = 'Y'"
		   					 , ulOpDealNo, ulId);
		   	}	
		   	else
		   	{
			    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: deal_no error...ulDealNo=[%lu], ulOpDealNo=[%lu]\n", ulDealNo,ulOpDealNo);
				mysql_free_result(bck_res);
				return -1;
		   	}	

			if (mysql_query(cpr_con, szQuery))
			{
			    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: select error...\n");
				ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
				ZzLOG(ERROR, "%s\n\n",szQuery);
				mysql_free_result(bck_res);
				return -1;
		    }
		   	if (!(cpr_res = mysql_store_result(cpr_con))) 
			{
			    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: mysql_store_result error...\n");
				ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
				mysql_free_result(bck_res);
				return -1;
			}
			if (mysql_num_rows(cpr_res) <= 0)	
		 	{
		 		mysql_free_result(cpr_res);
			    ZzLOG(ALWAY, "cprdaem1004_sum_cnl_dd: 구매정보 없음. - (%lu)\n", ulDealNo);
			    
			    memset(szQuery, 0x00, sizeof(szQuery));
			    sprintf(szQuery, " update zangsi_bck.T_CNL_CPR_HIST set proc_yn = 'F' "
			    				 " where seq_no = %lu "
			    				 , ulSeqNo);
			    
				if (mysql_query(bck_con, szQuery))
				{
					mysql_free_result(bck_res);
				    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: select T_CPR_DEAL_INFO error...\n");
					ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: [%d](%s)\n",mysql_errno(bck_con), mysql_error(bck_con));
					ZzLOG(ERROR, "%s\n\n",szQuery);
					return -1;
			    }
			    continue;
		 	}
			
			cpr_row = mysql_fetch_row(cpr_res);
			memset(szDealDate, 0x00, sizeof(szDealDate));
			memset(szDealTime, 0x00, sizeof(szDealTime));
			
			unsigned long ulCprSeqNo = 0;
			
			ulCprSeqNo = (unsigned long)getnum(cpr_row, 0);
			strcpy(szDealDate, getstr(cpr_row, 1));
			strcpy(szDealTime, getstr(cpr_row, 2));
			strcpy(szSaleCd, getstr(cpr_row, 3));

			mysql_free_result(cpr_res);	
			
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " select b.chi_id "
							 " from zangsi_cpr.T_CPR_CONT_MAP_SUB a, zangsi_cpr.T_CPR_HASH_INFO b "
							 " where a.chi_id = b.chi_id and a.id = %lu " , ulId);
			
			if (mysql_query(cpr_con, szQuery))
			{
			    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: select error...\n");
				ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
				ZzLOG(ERROR, "%s\n\n",szQuery);
				mysql_free_result(bck_res);	
				return -1;
		    }
		   	if (!(cpr_res = mysql_store_result(cpr_con))) 
			{
				mysql_free_result(bck_res);	
			    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: mysql_store_result error...\n");
				ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
				return -1;
			}
			if (mysql_num_rows(cpr_res) <= 0)	
		 	{
			    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: 데이터 에러!!(%lu)\n", ulId);
				ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
				ZzLOG(ERROR, "%s\n\n",szQuery);

			    mysql_free_result(cpr_res);
			    
			    memset(szQuery, 0x00, sizeof(szQuery));
			    sprintf(szQuery, " update zangsi_bck.T_CNL_CPR_HIST set proc_yn = 'G' "
			    				 " where seq_no = %lu "
			    				 , ulSeqNo);
			    
				if (mysql_query(bck_con, szQuery))
				{
					mysql_free_result(bck_res);
				    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: select T_CPR_DEAL_INFO error...\n");
					ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: [%d](%s)\n",mysql_errno(bck_con), mysql_error(bck_con));
					ZzLOG(ERROR, "%s\n\n",szQuery);
					return -1;
			    }
			}
			
			while(cpr_row = mysql_fetch_row(cpr_res))
			{
				unsigned long ulChiId = (unsigned long)getnum(cpr_row, 0);
				
				double dPriceAmt = 0;
				double dSaleAmt = 0;
				double dCompAmt = 0;
				double dCprAmt = 0;
				
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " select price_amt, mgr_cd, comp_cd, cpr_payment_rate, apply_date, apply_time "
								 " from zangsi_cpr.T_CPR_PRICE_HIST  "
								 " where chi_id = %lu " 
								 " and concat(apply_date, apply_time) <= '%s%s' "
								 " order by apply_date desc, apply_time desc limit 1 "
								 , ulChiId, szDealDate, szDealTime);
				
				if (mysql_query(cpr_con, szQuery))
				{
				    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: select T_CPR_PRICE_HIST error...\n");
					ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
					ZzLOG(ERROR, "%s\n\n",szQuery);
					mysql_free_result(cpr_res);
					mysql_free_result(bck_res);
					return -1;
			    }
			   	if (!(res1 = mysql_store_result(cpr_con))) 
				{
				    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: mysql_store_result error...\n");
					ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
					mysql_free_result(cpr_res);
					mysql_free_result(bck_res);
					return -1;
				}
				if (mysql_num_rows(res1) <= 0)	
			 	{
				    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: 금액정보 없음!!(%lu)\n", ulChiId);
				    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: query (%s)\n", szQuery);
					ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
					mysql_free_result(cpr_res);
					mysql_free_result(bck_res);
				    mysql_free_result(res1);
				    return -1;
				}
				row1 = mysql_fetch_row(res1);

				dPriceAmt = 0;
				dPriceAmt = getnum(row1, 0);
				memset(szMgrCd, 0x00, sizeof(szMgrCd));
				strcpy(szMgrCd, getstr(row1,1));

				memset(szCompCd, 0x00, sizeof(szCompCd));
				strcpy(szCompCd, getstr(row1,2));
				double dCprPaymentRate = atof(getstr(row1, 3));
			
				mysql_free_result(res1);

				if(dPriceAmt == 0)
				{
					ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: 가격정보를 가져오는데 실패했습니다\n.");
					ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: deal_no(%lu), op_deal_no(%lu), chi_id(%lu) szDealDate,Time-[%s],[%s]\n"
					           , ulDealNo, ulOpDealNo, ulChiId, szDealDate, szDealTime);

					memset(szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, " update zangsi_cpr.T_CPR_DEAL_INFO set proc_yn = 'F' "
									 " where seq_no = %lu ", ulCprSeqNo);
		
					if (mysql_query(cpr_con, szQuery))
					{
					    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: update T_CPR_DEAL_INFO error...\n");
						ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
						ZzLOG(ERROR, "%s\n\n",szQuery);
						mysql_free_result(cpr_res);
						mysql_free_result(bck_res);
						return -1;
				    }
					
					continue;
				}
					
				if(dCprPaymentRate > 0)
				{
					dCprPaymentRate = dCprPaymentRate * 0.01;
				}
				else if(dCprPaymentRate <= 0)
				{	
					MYSQL_RES *price_res;
					MYSQL_ROW price_row;

					memset(szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, " select minor_name from zangsi_cpr.T_MINOR_CODE where major_code = '82' and minor_code = '%s' ", szCompCd);	
					
					if (mysql_query(cpr_con, szQuery))
					{
					    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: select T_MINOR_CODE error...\n");
						ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
						ZzLOG(ERROR, "%s\n\n",szQuery);
						mysql_free_result(cpr_res);
						mysql_free_result(bck_res);
						return -1;
				    }
				   	if (!(price_res = mysql_store_result(cpr_con))) 
					{
					    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: mysql_store_result error...\n");
						ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
						mysql_free_result(cpr_res);
						mysql_free_result(bck_res);
						return -1;
					}
					if (mysql_num_rows(price_res) <= 0)	
				 	{
					    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: T_MINOR_CODE 금액정보 없음!!\n");
					    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: query (%s)\n", szQuery);
						ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
						mysql_free_result(price_res);
						mysql_free_result(cpr_res);
						mysql_free_result(bck_res);
					    return -1;
					}
					price_row = mysql_fetch_row(price_res);

					char szRate[2];
					memset(szRate, 0x00, sizeof(szRate));
					
					strcpy(szRate, getstr(price_row,0));
					dCprPaymentRate = atof(szRate);
					
					dCprPaymentRate = dCprPaymentRate * 0.01;
					
					mysql_free_result(price_res);
				}						
				
				char szTableName[126];
				memset(szTableName, 0x00, sizeof(szTableName));
				if(strcmp(szSaleCd, "00") ==0)
				{
					dCprAmt = dPriceAmt * dCprPaymentRate;
					dSaleAmt = dPriceAmt * 0.05;
					dCompAmt = (dPriceAmt - dCprAmt) - dSaleAmt; 
					strcpy(szTableName, "T_CNL_CPR_HIST");
				}
				else
				{
					dCprAmt = dPriceAmt * dCprPaymentRate;
					dCompAmt = dPriceAmt - dCprAmt; 
					strcpy(szTableName, "T_CNL_CPR_HIST");
				}
				
				char szContCd[2+1];
				memset(szContCd, 0x00, sizeof(szContCd));
				
				if(ulOpDealNo == 0)
					strcpy(szContCd, "00");
				else
					strcpy(szContCd, "01");
					
					
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, "select * from zangsi_cpr.%s "
								 " where cont_cd = '%s' and sum_date = '%s' and mgr_cd = '%s'"
								 , szTableName, szContCd, szDealDate, szMgrCd);

				if (mysql_query(cpr_con, szQuery))
				{
				    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: select %s error...\n", szTableName);
					ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
					ZzLOG(ERROR, "%s\n\n",szQuery);
					mysql_free_result(cpr_res);
					mysql_free_result(bck_res);
					return -1;
			    }
			   	if (!(res1 = mysql_store_result(cpr_con))) 
				{
				    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: mysql_store_result error...\n");
					ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
					mysql_free_result(cpr_res);
					mysql_free_result(bck_res);
					return -1;
				}
				if (mysql_num_rows(res1) <= 0)	
			 	{
			 		memset(szQuery, 0x00, sizeof(szQuery));
			 		sprintf(szQuery, " insert into zangsi_cpr.%s values ( "
			 						 " '%s', '%s', '%s', '%s', %d, 1, %5.1f, %5.1f, %d, 'N') "
			 						 , szTableName, szDealDate, szMgrCd, szCompCd, szContCd
			 						 , (int)dPriceAmt, dCprAmt, dCompAmt, dSaleAmt);
				}
				else
				{
			 		memset(szQuery, 0x00, sizeof(szQuery));
			 		sprintf(szQuery, " update zangsi_cpr.%s "
			 						 " set price_amt = price_amt + %d, sale_cnt = sale_cnt + 1,"
			 						 " cpr_amt = cpr_amt + %5.1f, comp_amt = comp_amt + %5.1f, sale_amt = sale_amt + %d "
			 						 " where cont_cd = '%s' and sum_date = '%s' and mgr_cd = '%s' and comp_cd = '%s' "
			 						 , szTableName, (int)dPriceAmt, dCprAmt, dCompAmt, dSaleAmt
			 						 , szContCd, szDealDate, szMgrCd, szCompCd);
				}
				mysql_free_result(res1);
				
//				ZzLOG(ALWAY,"\n(%s)\n", szQuery);
				if (mysql_query(cpr_con, szQuery))
				{
				    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: INSERT %s error...\n", szTableName);
					ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
					ZzLOG(ERROR, "%s\n\n",szQuery);
					mysql_free_result(cpr_res);
					mysql_free_result(bck_res);
					mysql_free_result(res1);
					return -1;
			    }

			}
			
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update zangsi_cpr.T_CPR_DEAL_INFO set aproval_yn = 'N' "
							 " where seq_no = %lu ", ulCprSeqNo);

			if (mysql_query(cpr_con, szQuery))
			{
			    ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: update T_CPR_DEAL_INFO error...\n");
				ZzLOG(ERROR, "cprdaem1004_sum_cnl_dd: [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
				ZzLOG(ERROR, "%s\n\n",szQuery);
				mysql_free_result(cpr_res);
				mysql_free_result(bck_res);
				return -1;
		    }
			
			mysql_free_result(bck_res);
			ulCount++;
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
int cprdaem1004_process_init(int argc, char **argv)
{
	char stemp[128];
    /*
    ** 전역변수 초기화
    */
    memset(g_proc_type, 0x00, sizeof(g_proc_type));

	strcpy(g_proc_type, argv[2]);
	
    ZzInitGlobalVariable2("cprdaem1004", "/logs/daemon"); 

    ZzLOG(ALWAY, "[cprdaem1004]*****************프로그램 시작*****************\n");  

    // 파라미터 값 설정 및 초기화
    if (argc != 3){
    	goto arg_error;
    }

	
	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(cpr_con=db_connect_cprdb("zangsi_cpr")))
	{
		ZzLOG(ERROR, "cprDB에 접속하지 못 하였습니다...\n");
		db_disconnect(cpr_con);
	   	return(-1); 
	}

	if (!(bck_con=db_connect_cprdb("zangsi_bck")))
	{
		ZzLOG(ERROR, "bckDB에 접속하지 못 하였습니다...\n");
		db_disconnect(cpr_con);
		db_disconnect(bck_con);
	   	return(-1); 
	}

	memset(gst_date, 0x00, sizeof(gst_date));
	strcpy(gst_date, argv[1]);

    return (0);

arg_error:
    ZzLOG(ERROR, "usage : %s YYYYMMDD proctype\n", argv[0]);
    ZzLOG(ERROR, "        YYYYMMDD(처리일자): 00000000 = 시스템일자 , proctype(처리타입) : all = 제휴아이디 포함 전체 데이터 집계. mgr: 제휴아이디 제외. user: 제휴아이디만 집계\n", argv[0]);
    ZzPRT(ERROR, "usage : %s YYYYMMDD proctype\n", argv[0]);
    ZzPRT(ERROR, "        YYYYMMDD(처리일자): 00000000 = 시스템일자 \nproctype(처리타입) : all = 제휴아이디 포함 전체 데이터 집계. mgr: 제휴아이디 제외. user: 제휴아이디만 집계\n", argv[0]);
    return -1;
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int cprdaem1004_process_term()
{
    // DB close

	db_disconnect(cpr_con);	
	db_disconnect(bck_con);	
	
	
    ZzLOG(ALWAY, "[cprdaem1004]*****************프로그램 종료*****************\n");  

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  cprdaem1004_signal(int nSignal)
{
    cprdaem1004_process_term();
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
	signal(SIGTERM, cprdaem1004_signal);
	signal(SIGINT,  cprdaem1004_signal);
	signal(SIGQUIT, cprdaem1004_signal);
	signal(SIGKILL, cprdaem1004_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( cprdaem1004_process_init(argc, argv) == 0 ) 
	{
		/* 프로그램 메인루틴 */
		rc = cprdaem1004_process();
		/* 프로그램 종료루틴 */                    
		cprdaem1004_process_term();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
