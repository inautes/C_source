/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : cprdaem1003.cc
 *         기능 : 하루 한번 엔트랜드 DB로 영화코드별 다운로드 건수와 금액 정보 집계하여 전송.
 *         설명 :
 *     설치위치 : 유료컨텐츠DB에 위치
 *
 *       작성자 : HCS
 *       작성일 : 2008/04/16
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

int cprdaem1003_process();
int cprdaem1003_send_deal_info();
int cprdaem1003_cine21_send_deal_info() ;
int cprdaem1003_check_reserve( unsigned long long gulStartDealSeq , unsigned long long gulEndDealSeq ,char* pCompCD);
int cprdaem1003_process_init(int argc, char **argv);
int cprdaem1003_process_term();
void cprdaem1003_signal(int nSignal);


MYSQL     *cpr_con;
MYSQL     *cpr_real_con;
MYSQL_RES *cpr_res;
MYSQL_ROW  cpr_row;

MYSQL     *contRoad_con;
MYSQL_RES *contRoad_res;
MYSQL_ROW  contRoad_row;

MYSQL_RES *cpr_res2;
MYSQL_ROW  cpr_row2;

unsigned long long gulStartDealSeq =0;
unsigned long long gulEndDealSeq =0;

bool gbIsUserDate      ; //날짜입력
char gst_date[8+1];	// 처리일자
char gst_time[6+1];	// 처리시작시간
char gst_old_date[8+1];	// 이전 처리일자
char gst_old_time[6+1];	// 이전 처리시작시간
char gproc_log[80]; // 로그메시지버퍼
char gService[15];
int gnType = 0;

//******************************************************************************
//* cprdaem1003 main
//******************************************************************************
int cprdaem1003_process()
{
	char szQuery[1000];  // query string
	memset(szQuery, 0x00, sizeof(szQuery));

	// 처리일자가 "00000000"일때는 시스템일자 -1을 한다.
	if (strcmp(gst_date, "00000000") == 0)
	{
		gbIsUserDate = false;
		strcpy(szQuery, "SELECT date_format(date_add(now(), INTERVAL - 1 DAY),'%Y%m%d') , date_format(now(),'%H%i%s') , date_format(now(),'%Y%m%d')");


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

		if( strcmp(gService,"cine21") == 0 )
			strcpy(gst_date,   getstr(cpr_row, 2));

		mysql_free_result(cpr_res);


		strcpy( gst_old_date ,gst_date);
		strcpy( gst_old_time ,gst_time);


	}
	else
	{
		gbIsUserDate=true;

	}

	#ifdef __DEBUG
	printf("적용 날짜 : %s\n",gst_date);
	#endif

	if( strcmp(gService,"croad") == 0 )
	{
		//유료컨텐츠DB에서 영화코드별 구매정보를 CONTENTS ROAD DB로 전송
		if( cprdaem1003_send_deal_info() !=0)
		{
			return -1;
		}
	}
	else if( strcmp(gService,"cine21") == 0 )
	{


		//유료컨텐츠DB에서 영화코드별 구매정보를 CONTENTS ROAD DB로 전송
		//while( cprdaem1003_cine21_send_deal_info() ==0)
		int ret = cprdaem1003_cine21_send_deal_info();
		if(  ret==0)
		{
			ZzLOG(ALWAY, "\n\n5분동안 대기 합니다.\n\n");
		//	sleep(300);
		}
		if( ret != -2 )
		{
			char szCompCD[7];
			memset(szCompCD,0x00,sizeof(szCompCD));
			strcpy(szCompCD,"010003");
			char szQuery[1600];		// query string

			memset(szQuery,0x00,sizeof(szQuery));
			sprintf(szQuery, " update zangsi_cpr.T_CPR_DEAL_SEQ_TEMP set using_yn ='N' where comp_cd = '%s'",szCompCD);

			ZzLOG(ALWAY, "\n%s\n", szQuery);

			if (mysql_query(cpr_real_con, szQuery))
			{
			    ZzLOG(ERROR, "cprdaem1003_send_deal_info: update T_CPR_DEAL_SEQ_TEMP error...\n");
				ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_real_con), mysql_error(cpr_real_con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				return -1;
			}
		}

		ZzLOG(ALWAY, "\n\n프로그램 종료.\n\n");

		return -1;
	}

	return 0;
}

int cprdaem1003_check_reserve(unsigned long long gulStartDealSeq , unsigned long long gulEndDealSeq ,char* pCompCD )
{


    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "gproc_date : [%s]\n", gst_date);
    ZzLOG(ALWAY, "---------------------------------------------------------\n");

	char szQuery[1600];		// query string
	unsigned long ulCount = 0;
	int nCnt = 0;
	char szDealDate[8+1];
	memset(szDealDate, 0x00, sizeof(szDealDate));
	char szDealTime[8+1];
	memset(szDealTime, 0x00, sizeof(szDealTime));

	while(1)
	{
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " select id, price_amt, seq_no, deal_date, deal_time  from zangsi_cpr.T_CPR_DEAL_INFO "
						 " where seq_no > %llu and seq_no <= %llu  and proc_yn = 'N' and comp_cd='%s' order by seq_no limit %d ,100 "
	   					 , gulStartDealSeq , gulEndDealSeq ,pCompCD
	   					 , nCnt);


		ZzLOG(ALWAY, "\n%s\n", szQuery);
		nCnt = nCnt + 100;
		if (mysql_query(cpr_con, szQuery))
		{
		    ZzLOG(ERROR, "cprdaem1002_check_reserve: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			return -1;
		}

		if (!(cpr_res = mysql_store_result(cpr_con)))
		{
		    ZzLOG(ERROR, "cprdaem1002_check_reserve: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_con), mysql_error(cpr_con));
			return -1;
		}

		if (mysql_num_rows(cpr_res) <= 0)
	 	{
			mysql_free_result(cpr_res);
		    ZzLOG(ALWAY, "cprdaem1002_check_reserve: 처리할 자료가 없습니다.\n");
		    return 0;
		}

		while((cpr_row = mysql_fetch_row(cpr_res)))
		{
			unsigned long ulId = 0;
			unsigned long ulSeqNo = 0;
			double dDealPriceAmt = 0;
			double dCprPriceAmt = 0;

			ulId = (unsigned long)getnum(cpr_row, 0);
			dDealPriceAmt = (double)getnum(cpr_row, 1);
			ulSeqNo = (unsigned long)getnum(cpr_row, 2);
			strcpy(szDealDate, getstr(cpr_row, 3));
			strcpy(szDealTime, getstr(cpr_row, 4));

			/*
			수정일 : 20080920
			수정자 : HCS
			수정내용 : T_CPR_CONT_MAP_SUB에 chi_id가 중복해서 들어간 경우 가격산출이 중복되지 않게 가격산출 쿼리에서 chi_id로 그룹바이후
					   가격합산을 기존 쿼리로 하는 것에서 코드로 변경
			*/
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " select a.id, b.price_amt, b.chi_id from zangsi_cpr.T_CPR_CONT_MAP_SUB a, zangsi_cpr.T_CPR_HASH_INFO b "
							 " where a.id = %ld and a.chi_id = b.chi_id "
							 " group by a.id, a.chi_id "
							 ,ulId);

			if (mysql_query(cpr_con, szQuery))
			{
			    ZzLOG(ERROR, "cprdaem1002_check_reserve: INSERT T_DEAL_INFO error...\n");
					ZzLOG(ERROR, "cprdaem1002_check_reserve: [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
					ZzLOG(ERROR, "%s\n\n",szQuery);
					mysql_free_result(cpr_res);
					return -1;
		    }
	    	if (!(cpr_res2 = mysql_store_result(cpr_con)))
			{
			    ZzLOG(ERROR, "cprdaem1002_check_reserve: mysql_store_result error...\n");
				ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_con), mysql_error(cpr_con));
				mysql_free_result(cpr_res);
				return -1;
			}
			if (mysql_num_rows(cpr_res2) <= 0)
		 	{
			    ZzLOG(ALWAY, "cprdaem1002_check_reserve: %ld는 T_CPR_CONT_MAP_SUB or T_CPR_HASH_INFO에 정보가 없음.\n", ulId);

				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " update zangsi_cpr.T_CPR_DEAL_INFO set proc_yn = 'R' where deal_date = '%s' and id = %ld "
								 ,gst_date
								 ,ulId);
				ZzLOG(ALWAY, "cprdaem1002_check_reserve : %s\n", szQuery);
				if (mysql_query(cpr_real_con, szQuery))
				{
				    ZzLOG(ERROR, "cprdaem1002_check_reserve: update T_CPR_DEAL_INFO error...\n");
						ZzLOG(ERROR, "cprdaem1002_check_reserve: [%d](%s)\n",mysql_errno(cpr_real_con), mysql_error(cpr_real_con));
						ZzLOG(ERROR, "%s\n\n",szQuery);
						mysql_free_result(cpr_res);
						mysql_free_result(cpr_res2);
						return -1;
			    }

			    mysql_free_result(cpr_res2);
			    ulCount	++;

			    continue;
			}
			while(cpr_row2 = mysql_fetch_row(cpr_res2))
			{
				MYSQL_RES *res1;
				MYSQL_ROW  row1;
				memset(szQuery, 0x00, sizeof(szQuery));
				//T_CPR_PRICE_HIST에서 가격변경된 내역 전부 가져옴.
				sprintf(szQuery, " select price_amt, apply_date, apply_time from zangsi_cpr.T_CPR_PRICE_HIST  "
								 " where chi_id = %ld "
								 " order by apply_date desc, apply_time desc "
								 , (unsigned long)getnum(cpr_row2,2));

				if (mysql_query(cpr_con, szQuery))
				{
				    ZzLOG(ERROR, "cprdaem1002_sum_mgr_dd: INSERT T_DEAL_INFO error...\n");
					ZzLOG(ERROR, "cprdaem1002_sum_mgr_dd: [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
					ZzLOG(ERROR, "%s\n\n",szQuery);
					mysql_free_result(cpr_res);
					mysql_free_result(cpr_res2);
					return -1;
			    }
			   	if (!(res1 = mysql_store_result(cpr_con)))
				{
				    ZzLOG(ERROR, "cprdaem1002_sum_mgr_dd: mysql_store_result error...\n");
					ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_con), mysql_error(cpr_con));
					mysql_free_result(cpr_res);
					mysql_free_result(cpr_res2);
					return -1;
				}
				if (mysql_num_rows(res1) <= 0)
			 	{
				    ZzLOG(ERROR, "cprdaem1002_sum_mgr_dd: 금액정보 없음!!(%ld)\n", (unsigned long)getnum(cpr_row2,2));
				    ZzLOG(ERROR, "cprdaem1002_sum_mgr_dd: query (%s)\n", szQuery);
						ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_con), mysql_error(cpr_con));
						mysql_free_result(cpr_res);
						mysql_free_result(cpr_res2);
				    mysql_free_result(res1);
				    return -1;
				}
				double dPriceAmt = 0;
				while(row1 = mysql_fetch_row(res1))
				{
					if(strcmp(szDealDate, getstr(row1,1)) == 0 )
					{//거래일자와 적용날짜가 같으면
						if(strcmp(szDealTime , getstr(row1,2)) >= 0)
						{//거래시간이 적용시간보다 크거나 같으면 가격정보 저장
							dPriceAmt = getnum(row1, 0);
							break;
						}
					}
					else if(strcmp(szDealDate, getstr(row1,1)) > 0 )
					{//거래일자가 적용날짜보다 크면 가격정보 저장
						dPriceAmt = getnum(row1, 0);
						break;
					}
					dPriceAmt = getnum(row1, 0);
				}
				mysql_free_result(res1);

				dCprPriceAmt = dCprPriceAmt + dPriceAmt;
			}
			mysql_free_result(cpr_res2);

			if(dDealPriceAmt != dCprPriceAmt)
			{
				ZzLOG(ALWAY,"\n dDealPriceAmt:%15.0f -- dCprPriceAmt:%15.0f \n", dDealPriceAmt, dCprPriceAmt);
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " update zangsi_cpr.T_CPR_DEAL_INFO set proc_yn = 'R' where seq_no = %ld and deal_date = '%s' and id = %ld "
								 ,ulSeqNo
								 ,gst_date
								 ,ulId);
				ZzLOG(ALWAY, "cprdaem1002_check_reserve : %s\n", szQuery);
				if (mysql_query(cpr_real_con, szQuery))
				{
				    ZzLOG(ERROR, "cprdaem1002_check_reserve: update T_CPR_DEAL_INFO error...\n");
					ZzLOG(ERROR, "cprdaem1002_check_reserve: [%d](%s)\n",mysql_errno(cpr_real_con), mysql_error(cpr_real_con));
					ZzLOG(ERROR, "%s\n\n",szQuery);
					mysql_free_result(cpr_res);
					return -1;
			    }
				ulCount	++;
			}
		}
		mysql_free_result(cpr_res);

	}
	ZzLOG(ALWAY, "cprdaem1003_check_reserve: 보류처리된 자료의 갯수는 %ld 입니다.\n",ulCount);
	return 0;


}

//******************************************************************************
//* cprdaem1003 db 처리로직
//******************************************************************************
int cprdaem1003_cine21_send_deal_info()
{
	ZzLOG(ALWAY, "\n시네21 정산 프로그램 가동\n");


	char szQuery[1600];		// query string
	int  ret;

	char szCompCD[7];
	memset(szCompCD,0x00,sizeof(szCompCD));
	strcpy(szCompCD,"010003");

	unsigned long ulDataCnt =0;

	char using_yn[2];
	memset(using_yn,0x00,sizeof(using_yn));

	char szFileName1[512];
	char szFileName2[512];
	char szFileName3[512];
	char szFileName4[512];


	char szFilePath1[1024];
	char szFilePath2[1024];
	char szFilePath3[1024];
	char szFilePath4[1024];



	char szData[1024] ;
	memset(szData,0x00,sizeof(szData));

	FILE *pFtpScriptFile = NULL;


	char szTemp[20];
	memset(szTemp,0x00,sizeof(szTemp));



	sprintf(szQuery," select seq_no,using_yn from zangsi_cpr.T_CPR_DEAL_SEQ_TEMP where comp_cd ='%s'",szCompCD);  //cine21 comp_cd
	ZzLOG(ALWAY, "\n%s\n", szQuery);

	if (mysql_query(cpr_con, szQuery))
	{
	    ZzLOG(ERROR, "cprdaem1003_send_deal_info: select T_CPR_DEAL_SEQ_TEMP error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return -1;
	}


	if (!(cpr_res = mysql_store_result(cpr_con)))
	{
	    ZzLOG(ERROR, "cprdaem1001_process: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_con), mysql_error(cpr_con));
		return -1;
	}


	if((cpr_row = mysql_fetch_row(cpr_res)))
	{
		gulStartDealSeq = (unsigned long long)getnum(cpr_row, 0);
		strcpy(using_yn,getstr(cpr_row,1));
		ZzLOG(ALWAY, "gulStartDealSeq 값 추출 [ %llu ]\n",gulStartDealSeq);

	}else
	{
		ZzLOG(ALWAY, "\n데이터가 없습니다.T_CPR_DEAL_SEQ_TEMP table 에 seq_no 를 입력해주세요.\n");
		mysql_free_result(cpr_res);
		return -1;
	}
	mysql_free_result(cpr_res);

	if( gulStartDealSeq == 0 )
	{

		ZzLOG(ALWAY, "\n데이터가 없습니다.T_CPR_DEAL_SEQ_TEMP table 에 seq_no 를 입력해주세요.\n");
		return -1;
	}

	if( strcmp(using_yn,"Y") == 0 || strcmp(using_yn,"y") == 0 )
	{
		ZzLOG(ALWAY, "\n프로세스가 실행중입니다. 만약 프로세스가 실행중이지 않다면 T_CPR_DEAL_SEQ_TEMP table 의 using_yn 를 N로 변경 해주세요.\n");
		return -2;
	}

	memset(szQuery,0x00,sizeof(szQuery));
	sprintf(szQuery, " update zangsi_cpr.T_CPR_DEAL_SEQ_TEMP set using_yn ='Y' where comp_cd = '%s'",szCompCD);

	ZzLOG(ALWAY, "\n%s\n", szQuery);

	if (mysql_query(cpr_real_con, szQuery))
	{
	    ZzLOG(ERROR, "cprdaem1003_send_deal_info: update T_CPR_DEAL_SEQ_TEMP error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_real_con), mysql_error(cpr_real_con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return -1;
	}



	strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d') , date_format(now(),'%H%i%s') ");


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

	memcpy(&gst_time[4],"00",2);


	mysql_free_result(cpr_res);


	memset(szFileName1,0x00,sizeof(szFileName1));
	memset(szFileName2,0x00,sizeof(szFileName2));
	memset(szFileName3,0x00,sizeof(szFileName3));
	memset(szFileName4,0x00,sizeof(szFileName4));

	//account

	sprintf(szFileName1,"RAT.REAL.WED01.%s%s",gst_old_date,gst_old_time);
	sprintf(szFileName2,"FLG.RAT.REAL.WED01.%s%s",gst_old_date,gst_old_time);
	//statics
	sprintf(szFileName3,"SVC.REAL.WED01.%s%s",gst_old_date,gst_old_time);
	sprintf(szFileName4,"FLG.SVC.REAL.WED01.%s%s",gst_old_date,gst_old_time);


	memset(szFilePath1,0x00,sizeof(szFilePath1));
	memset(szFilePath2,0x00,sizeof(szFilePath2));
	memset(szFilePath3,0x00,sizeof(szFilePath3));
	memset(szFilePath4,0x00,sizeof(szFilePath4));


	sprintf(szFilePath1,"/usr/local/mysql/data/cine21/%s/%s",gst_date,szFileName1);
	sprintf(szFilePath2,"/usr/local/mysql/data/cine21/%s/%s",gst_date,szFileName2);
	//statics
	sprintf(szFilePath3,"/usr/local/mysql/data/cine21/%s/%s",gst_date,szFileName3);
	sprintf(szFilePath4,"/usr/local/mysql/data/cine21/%s/%s",gst_date,szFileName4);


	memset(szQuery,0x00,sizeof(szQuery));
	sprintf(szQuery,"mkdir -m777 /usr/local/mysql/data/cine21/%s",gst_date);
	system(szQuery);


	//a.sale_cd = '00' and
	sprintf(szQuery, " select count(*) , max(a.seq_no) , '1'  "
						 " from zangsi_cpr.T_CPR_DEAL_INFO a, zangsi_cpr.T_CPR_CONT_MAP_SUB b,  "
						 " zangsi_cpr.T_CPR_HASH_INFO c  , zangsi_cpr.T_CPR_CONT_LIST d   "
						 " where a.seq_no > %llu and d.comp_cd = '%s' and  a.proc_yn in( 'Y' ,'N')  "
						 " and a.id = b.id and b.chi_id = c.chi_id and c.list_id = d.list_id  group by 3 ",gulStartDealSeq,szCompCD);

	ZzLOG(ALWAY, "\n%s\n", szQuery);

	if (mysql_query(cpr_con, szQuery))
	{
	    ZzLOG(ERROR, "cprdaem1003_send_deal_info: select T_CPR_DEAL_INFO error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return -1;
	}



	if (!(cpr_res = mysql_store_result(cpr_con)))
	{
	    ZzLOG(ERROR, "cprdaem1001_process: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_con), mysql_error(cpr_con));
		return -1;
	}


	if((cpr_row = mysql_fetch_row(cpr_res)))
	{
		ulDataCnt = (unsigned long ) getnum(cpr_row, 0);
		gulEndDealSeq = (unsigned long long)getnum(cpr_row, 1);

		ZzLOG( ALWAY, "First ulDataCnt [ %ld ] gulEndDealSeq [ %llu ] \n",ulDataCnt , gulEndDealSeq );

	}
	else
	{
		gulEndDealSeq = gulStartDealSeq;
		ZzLOG( ALWAY, "Second ulDataCnt [ %ld ] gulEndDealSeq [ %llu ] \n",ulDataCnt , gulEndDealSeq );
	}


	mysql_free_result(cpr_res);



	if( cprdaem1003_check_reserve(gulStartDealSeq,gulEndDealSeq,szCompCD) != 0)
	{
		ZzLOG(ALWAY, "\ncprdaem1003_check_reserve 오류입니다.\n");
		return -1;
	}



	//sprintf(szQuery," select seq_no , deal_no from zangsi_cpr.T_CPR_DEAL_INFO where deal_date ='%s' limit 1 ", gst_date);

	sprintf(szQuery, " select concat(a.deal_date, a.deal_time), 'SP00074', 'WED01', d.mgr_cd, '107' "
					 " , '202', '302', concat('' , repeat('0',6 - length(a.price_amt)),a.price_amt) , '0' ,'' INTO OUTFILE '%s' "
					 " FIELDS TERMINATED BY ';' LINES TERMINATED BY '\n' "
					 " from zangsi_cpr.T_CPR_DEAL_INFO a, zangsi_cpr.T_CPR_CONT_MAP_SUB b,  "
					 " zangsi_cpr.T_CPR_HASH_INFO c  , zangsi_cpr.T_CPR_CONT_LIST d   "
					 " where a.seq_no > %llu and a.seq_no <= %llu and d.comp_cd = '%s' and a.proc_yn in( 'Y','N')  "
					 " and a.id = b.id and b.chi_id = c.chi_id and c.list_id = d.list_id  "
					 " group by  a.seq_no "
					 " order by  a.deal_date, a.deal_time "
					, szFilePath1,gulStartDealSeq , gulEndDealSeq , szCompCD);


	ZzLOG(ALWAY, "\n%s\n", szQuery);

	if (mysql_query(cpr_con, szQuery))
	{
	    ZzLOG(ERROR, "cprdaem1003_send_deal_info: select T_SUM_MGR_DD error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return -1;
	}


	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select "
					 " %s%s ,'SP00074' ,'WED01' ,concat('' , repeat('0',6 - length('%ld')),'%ld')  ,'%s' ,'' "
					 "	 INTO OUTFILE '%s' "
					 " FIELDS TERMINATED BY ';' LINES TERMINATED BY '\n' "
					,gst_date,gst_time,ulDataCnt,ulDataCnt,szFileName1, szFilePath2);


	ZzLOG(ALWAY, "\n%s\n", szQuery);

	if (mysql_query(cpr_con, szQuery))
	{
	    ZzLOG(ERROR, "cprdaem1003_send_deal_info: select T_SUM_MGR_DD error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return -1;
	}





	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select "
					 " %s%s ,'SP00074' ,'WED01' ,concat('' , repeat('0',6 - length('%ld')),'%ld')   ,'%s' ,'' "
					 "	 INTO OUTFILE '%s' "
					 " FIELDS TERMINATED BY ';' LINES TERMINATED BY '\n' "
					,gst_date,gst_time,ulDataCnt,ulDataCnt,szFileName3, szFilePath4);


	ZzLOG(ALWAY, "\n%s\n", szQuery);

	if (mysql_query(cpr_con, szQuery))
	{
	    ZzLOG(ERROR, "cprdaem1003_send_deal_info: select T_SUM_MGR_DD error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return -1;
	}

	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery,"cp %s %s",szFilePath1 , szFilePath3 );
	system(szQuery);
/*
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery,"cp %s %s",szFilePath2 , szFilePath4 );
	system(szQuery);
*/


	pFtpScriptFile = fopen( "/usr/local/mysql/data/cine21/script1.sh" ,"wt");

	memset(szData,0x00,sizeof(szData));

	sprintf( szData , "#/bin/sh\n"
					  "USERNAME=wedisk\n"
					  "PASSWORD=rmfrdjfk\n"
					  "HOST=dcms.cine21i.com\n"
					  //"PUT_FILES=RAT.REAL.WED01.%s%s\n"
					  "PUT_FILES=%s\n"
					  "{\n"
					  "	echo user $USERNAME $PASSWORD\n"
					  "	echo bi\n"
					  "	echo prompt\n"
					  "	echo cd statistics\n"
					  "	echo lcd /usr/local/mysql/data/cine21/%s/\n"
					  "	echo put $PUT_FILES\n"
					  "	echo bye\n"
					  "} | ftp -n -v $HOST \n"
					  ,szFileName3,gst_date);
					  //"} | ftp -n -v $HOST > /usr/local/mysql/data/cine21/log/RAT.REAL.WED01.%s%s.log\n"
					  //,gst_date,gst_time,gst_date,gst_time);
	fwrite(szData , strlen(szData) , 1 , pFtpScriptFile );
	fclose( pFtpScriptFile );

	pFtpScriptFile = NULL;

	pFtpScriptFile = fopen( "/usr/local/mysql/data/cine21/script2.sh" ,"wt");


	memset(szData,0x00,sizeof(szData));
	sprintf( szData , "#/bin/sh\n"
					  "USERNAME=wedisk\n"
					  "PASSWORD=rmfrdjfk\n"
					  "HOST=dcms.cine21i.com\n"
					  //"PUT_FILES=FLG.RAT.REAL.WED01.%s%s\n"
					  "PUT_FILES=%s\n"
					  "{\n"
					  "	echo user $USERNAME $PASSWORD\n"
					  "	echo bi\n"
					  "	echo prompt\n"
					  "	echo cd statistics\n"
					  "	echo lcd /usr/local/mysql/data/cine21/%s/\n"
					  "	echo put $PUT_FILES\n"
					  "	echo bye\n"
					  "} | ftp -n -v $HOST "//> /usr/local/mysql/data/cine21/log/FLG.RAT.REAL.WED01.%s%s.log\n"
					  ,szFileName4,gst_date);

					  //,gst_date,gst_time,gst_date,gst_time);
	fwrite(szData , strlen(szData) , 1 , pFtpScriptFile );
	fclose( pFtpScriptFile );



	pFtpScriptFile = NULL;

	pFtpScriptFile = fopen( "/usr/local/mysql/data/cine21/script3.sh" ,"wt");


	memset(szData,0x00,sizeof(szData));
	sprintf( szData , "#/bin/sh\n"
					  "USERNAME=wedisk\n"
					  "PASSWORD=rmfrdjfk\n"
					  "HOST=dcms.cine21i.com\n"
					  //"PUT_FILES=FLG.RAT.REAL.WED01.%s%s\n"
					  "PUT_FILES=%s\n"
					  "{\n"
					  "	echo user $USERNAME $PASSWORD\n"
					  "	echo bi\n"
					  "	echo prompt\n"
					  "	echo cd account\n"
					  "	echo lcd /usr/local/mysql/data/cine21/%s/\n"
					  "	echo put $PUT_FILES\n"
					  "	echo bye\n"
					  "} | ftp -n -v $HOST " //> /usr/local/mysql/data/cine21/log/FLG.RAT.REAL.WED01.%s%s.log\n"
					  ,szFileName1,gst_date);
					  //,gst_date,gst_time,gst_date,gst_time);
	fwrite(szData , strlen(szData) , 1 , pFtpScriptFile );
	fclose( pFtpScriptFile );


	pFtpScriptFile = NULL;

	pFtpScriptFile = fopen( "/usr/local/mysql/data/cine21/script4.sh" ,"wt");


	memset(szData,0x00,sizeof(szData));
	sprintf( szData , "#/bin/sh\n"
					  "USERNAME=wedisk\n"
					  "PASSWORD=rmfrdjfk\n"
					  "HOST=dcms.cine21i.com\n"
					  //"PUT_FILES=RAT.REAL.WED01.%s%s\n"
					  "PUT_FILES=%s\n"
					  "{\n"
					  "	echo user $USERNAME $PASSWORD\n"
					  "	echo bi\n"
					  "	echo prompt\n"
					  "	echo cd account\n"
					  "	echo lcd /usr/local/mysql/data/cine21/%s/\n"
					  "	echo put $PUT_FILES\n"
					  "	echo bye\n"
					  "} | ftp -n -v $HOST " //>  /usr/local/mysql/data/cine21/log/RAT.REAL.WED01.%s%s.log\n"
					  ,szFileName2 ,gst_date);
					  //,gst_date,gst_time,gst_date,gst_time);
	fwrite(szData , strlen(szData) , 1 , pFtpScriptFile );
	fclose( pFtpScriptFile );

	sleep(5);
	system("chmod 777 /usr/local/mysql/data/cine21/script1.sh;/usr/local/mysql/data/cine21/script1.sh");
	system("chmod 777 /usr/local/mysql/data/cine21/script2.sh;/usr/local/mysql/data/cine21/script2.sh");
	system("chmod 777 /usr/local/mysql/data/cine21/script3.sh;/usr/local/mysql/data/cine21/script3.sh");
	system("chmod 777 /usr/local/mysql/data/cine21/script4.sh;/usr/local/mysql/data/cine21/script4.sh");


	sprintf(szQuery, " update zangsi_cpr.T_CPR_DEAL_SEQ_TEMP set seq_no = %llu , reg_date ='%s', reg_time='%s' where comp_cd = '%s'"
					, gulEndDealSeq , gst_date,gst_time,szCompCD);

	ZzLOG(ALWAY, "\n%s\n", szQuery);

	if (mysql_query(cpr_real_con, szQuery))
	{
	    ZzLOG(ERROR, "cprdaem1003_send_deal_info: update T_CPR_DEAL_SEQ_TEMP error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_real_con), mysql_error(cpr_real_con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return -1;
	}
/*
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery,"rm -rf  %s %s %s %s",szFilePath1 , szFilePath2 , szFilePath3 , szFilePath4 );


	ZzLOG(ERROR, "(%s)\n", szQuery);
	system(szQuery);
*/

	ZzLOG(ALWAY, "cprdaem1003_send_deal_info : 전송 완료 입니다.\n");

	return 0;
}

int cprdaem1003_send_deal_info()
{

	char szQuery[1600];		// query string
	int  ret;

	char szBuffer[600];
	int nCount = 0;

	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " delete "
					 " from zangsi_cpr.T_SUM_MGR_DD_T "
					 " where sum_date = '%s' and comp_cd = '010001' "
					, gst_date);

	ZzLOG(ALWAY, "\n%s\n", szQuery);

	if (mysql_query(cpr_con, szQuery))
	{
	    ZzLOG(ERROR, "cprdaem1003_send_deal_info: select T_SUM_MGR_DD error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return -1;
	}

	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " insert into zangsi_cpr.T_SUM_MGR_DD_T "
					 " (sum_date, mgr_cd, comp_cd, price_amt, sale_cnt, cpr_amt) "
					 " select sum_date, mgr_cd, comp_cd, sum(price_amt), sum(sale_cnt), sum(cpr_amt) "
					 " from zangsi_cpr.T_SUM_MGR_DD "
					 " where sum_date = '%s' and comp_cd = '010001' and send_yn = 'N' "
					 " group by mgr_cd "
					, gst_date);

	ZzLOG(ALWAY, "\n%s\n", szQuery);

	if (mysql_query(cpr_con, szQuery))
	{
	    ZzLOG(ERROR, "cprdaem1003_send_deal_info: select T_SUM_MGR_DD error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return -1;
	}

	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " insert into zangsi_cpr.T_SUM_MGR_DD_T "
					 " (sum_date, mgr_cd, comp_cd, price_amt, sale_cnt, cpr_amt) "
					 " select sum_date, mgr_cd, comp_cd, sum(price_amt), sum(sale_cnt), sum(cpr_amt) "
					 " from zangsi_cpr.T_SUM_MGR_USER_DD "
					 " where sum_date = '%s' and comp_cd = '010001' and send_yn = 'N' "
					 " group by mgr_cd "
					, gst_date);

	ZzLOG(ALWAY, "\n%s\n", szQuery);

	if (mysql_query(cpr_con, szQuery))
	{
	    ZzLOG(ERROR, "cprdaem1003_send_deal_info: select T_SUM_MGR_DD error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return -1;
	}


	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select sum_date, mgr_cd, comp_cd, sum(price_amt), sum(sale_cnt), sum(cpr_amt) "
					 " from zangsi_cpr.T_SUM_MGR_DD_T "
					 " where sum_date = '%s' and comp_cd = '010001' "
					 " group by mgr_cd "
					, gst_date);

	ZzLOG(ALWAY, "\n%s\n", szQuery);

	if (mysql_query(cpr_con, szQuery))
	{
	    ZzLOG(ERROR, "cprdaem1003_send_deal_info: select T_SUM_MGR_DD error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return -1;
	}


	if (!(cpr_res = mysql_store_result(cpr_con)))
	{
	    ZzLOG(ERROR, "cprdaem1001_process: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(cpr_con), mysql_error(cpr_con));
		return -1;
	}

	while((cpr_row = mysql_fetch_row(cpr_res)))
	{
		char szSumDate[8+1];
		char szMgrCd[20+1];
		char szCompCd[6+1];

		memset(szSumDate, 0x00, sizeof(szSumDate));
		memset(szMgrCd, 0x00, sizeof(szMgrCd));
		memset(szCompCd, 0x00, sizeof(szCompCd));

		double dPriceAmt = 0;
		double dSaleCnt = 0;
		double dCprAmt = 0;

		strcpy(szSumDate, getstr(cpr_row, 0));
		strcpy(szMgrCd, getstr(cpr_row, 1));
		strcpy(szCompCd, getstr(cpr_row, 2));

		dPriceAmt = (double)getnum(cpr_row, 3);
		dSaleCnt = (double)getnum(cpr_row, 4);
		dCprAmt = (double)getnum(cpr_row, 5);

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into contentsroad_db.mgr_sale_info "
						 " values "
						 " ('%s', '%s', '%s', %d, %d, %d)  "
						 , szSumDate
						 , szMgrCd
						 , szCompCd
						 , (int)dPriceAmt
						 , (int)dSaleCnt
						 , (int)dCprAmt);

		if (mysql_query(contRoad_con, szQuery))
		{
		    ZzLOG(ERROR, "cprdaem1003_send_deal_info: insert mgr_sale_info error...\n");
			ZzLOG(ERROR, "cprdaem1003_send_deal_info: [%d](%s)\n",mysql_errno(contRoad_con), mysql_error(contRoad_con));
			ZzLOG(ERROR, "%s\n\n",szQuery);
			return -1;
	    }

	    memset(szQuery, 0x00, sizeof(szQuery));
	    sprintf(szQuery, " update zangsi_cpr.T_SUM_MGR_DD set send_yn = 'Y' where sum_date = '%s' and mgr_cd = '%s' and comp_cd = '%s' "
	    				 , szSumDate
	    				 , szMgrCd
	    				 , szCompCd);

		if (mysql_query(cpr_real_con, szQuery))
		{
		    ZzLOG(ERROR, "cprdaem1003_send_deal_info: UPDATE T_SUM_MGR_DD error...\n");
			ZzLOG(ERROR, "cprdaem1003_send_deal_info: [%d](%s)\n",mysql_errno(cpr_real_con), mysql_error(cpr_real_con));
			ZzLOG(ERROR, "%s\n\n",szQuery);
	    }

	    memset(szQuery, 0x00, sizeof(szQuery));
	    sprintf(szQuery, " update zangsi_cpr.T_SUM_MGR_USER_DD set send_yn = 'Y' where sum_date = '%s' and mgr_cd = '%s' and comp_cd = '%s' "
	    				 , szSumDate
	    				 , szMgrCd
	    				 , szCompCd);

		if (mysql_query(cpr_real_con, szQuery))
		{
		    ZzLOG(ERROR, "cprdaem1003_send_deal_info: UPDATE T_SUM_MGR_DD error...\n");
			ZzLOG(ERROR, "cprdaem1003_send_deal_info: [%d](%s)\n",mysql_errno(cpr_real_con), mysql_error(cpr_real_con));
			ZzLOG(ERROR, "%s\n\n",szQuery);
	    }

	 	nCount++;
	}
	mysql_free_result(cpr_res);

	ZzLOG(ALWAY, "cprdaem1003_send_deal_info : 전송한 데이터는 총 (%d)개입니다.\n", nCount);
	return 0;
}


/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int cprdaem1003_process_init(int argc, char **argv)
{
	char stemp[128];
    /*
    ** 전역변수 초기화
    */

    // 파라미터 값 설정 및 초기화
    if (argc != 3)
    {
    	printf("input date[ if cine21 is date+time ] service_name [cine21] [croad] ...\n");
    	goto arg_error;
    }

	memset(gService , 0x00 ,sizeof(gService));

	strcpy(gService ,argv[2]);
	if( strcmp(gService,"cine21") == 0 )
	{
		if( strlen( argv[1]) < 8)
		{
			printf("input date[ if cine21 is date+time ] service_name [cine21] [croad] ...\n");
			printf("ex > cprdaem1003 20080808 cine21 \n");
			goto arg_error;
		}
		ZzInitGlobalVariable2("cprdaem1003_cine21_", "/logs/daemon");
	}
	else if( strcmp(gService,"croad") == 0 )
	{
		ZzInitGlobalVariable2("cprdaem1003_croad_", "/logs/daemon");
	}
	//ZzInitGlobalVariable2("cprdaem1003", "/logs/daemon");

    ZzLOG(ALWAY, "[cprdaem1003]*****************프로그램 시작*****************\n");

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(cpr_con=db_connect_cprbckdb("zangsi_cpr")))
	{
		ZzLOG(ERROR, "zangsi_cpr DB에 접속하지 못 하였습니다...\n");
	   	return(-1);
	}

	if (!(cpr_real_con=db_connect_cprdb("zangsi_cpr")))
	{
		ZzLOG(ERROR, "zangsi_cpr DB에 접속하지 못 하였습니다...\n");
		db_disconnect(cpr_con);
	   	return(-1);
	}

	ZzLOG(ALWAY, "zangsi_cpr DB에 접속완료\n");

	if( strcmp(gService,"croad") == 0 )
	{
		if (!(contRoad_con=db_connect_controaddb("contentsroad_db")))
		{
			ZzLOG(ERROR, "contentsroad_db DB에 접속하지 못 하였습니다...\n");
			db_disconnect(contRoad_con);
			db_disconnect(cpr_con);
			db_disconnect(cpr_real_con);
		   	return(-1);
		}
	}

	memset(gst_date, 0x00, sizeof(gst_date));
	strcpy(gst_date, argv[1]);

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
int cprdaem1003_process_term()
{
    // DB close

	db_disconnect(cpr_con);
	db_disconnect(cpr_real_con);

	if( strcmp(gService,"croad") == 0 )
	{
		if( contRoad_con != NULL )
			db_disconnect(contRoad_con);
	}

    ZzLOG(ALWAY, "[cprdaem1003]*****************프로그램 종료*****************\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  cprdaem1003_signal(int nSignal)
{
    cprdaem1003_process_term();
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
	signal(SIGTERM, cprdaem1003_signal);
	signal(SIGINT,  cprdaem1003_signal);
	signal(SIGQUIT, cprdaem1003_signal);
	signal(SIGKILL, cprdaem1003_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);

	if ( cprdaem1003_process_init(argc, argv) == 0 )
	{
		/* 프로그램 메인루틴 */
		rc = cprdaem1003_process();
		/* 프로그램 종료루틴 */
		cprdaem1003_process_term();
	}
	return(0);
}
/*****************************************************************************
*  End of file...
*****************************************************************************/

