/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5002
 *         기능 : 게시기간 만료 컨텐츠 처리
 *         설명 : 1. 프로세스 처리 예정시간 : 04:00 이후 daem5021 완료 이후에 수행.
 *                2. 자동연장기준 - 추천컨텐츠로 등록된자료의 만료기간은 항상 30일로 처리한다.
 *                                - vod 서비스의 말료기간은 항상 365일로 처리한다.
 *                                - 게시기간 만료일자가 3일전인 영화, 드라마, 동영상, 애니분류의 제휴 컨텐츠중 우수 컨텐츠 선별하여 게시기간 연장.
 *                                - 교육, 휴대기기분류의 제휴 컨텐츠만 게시기간 고정
 *                3. 삭제기준 - 기간연장 처리 후 만료일자가 처리일자 보다 작거나 같은 컨텐츠.
 *                            - 처리일자의 4일뒤에 실제 파일, 디비 정보가 Delete(5003, 5004) 되도록 처리.
 *     설치위치 : CMD01
 *
 *       작성자 : LEE
 *       작성일 : 2005/12/05
 *     수정이력 : HCS - 제휴컨텐츠의 게시기간을 고정한다.
 						 - 20081119 HCS : 쓰레기데이터 삭제위해 disp_end_date = '%s' 부분 변경
 				         - 20091117 HCS 
 				           제휴컨텐츠중 만료일자 -3인 영화, 드라마, 동영상, 애니 분류의 컨텐츠중
 				           우수 컨텐츠 선정하여 게시기간 연장
 				           교육, 휴대기기 분류는 무조건 기간 연장
						 - 20101015 HCS : 제휴 필드 값 추가. "H". 개발 게시판(no.741)			  
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

int daem5002_get_sysdate();
int daem5002_init_process(int argc, char **argv);
int daem5002_main_process();
int daem5002_term_process();
void daem5002_signal(int nSignal);
int daem5002_cpr_update_process();
int daem5002_inc_end_date();
int daem5002_del_end_date();
int daem5002_flog_data_del_end_date();

int daem5002_set_cpr_date();
//int daem5002_set_om_date(); //no.880

MYSQL	*con_bck;
MYSQL     *con;
MYSQL_RES *res;
MYSQL_ROW  row;

MYSQL     *con_cpr;
//MYSQL	  *con_om; //no.880

char   gproc_date [  8+1];	//처리일자
char   gdel_date  [  8+1];	//삭제예정일 (처리일자+3)
char   gdis_date  [  8+1];	//게시만료일
char   gsys_date  [  8+1];	//등록일
char   gsys_time  [  6+1];	//등록시간



bool IsConnectDB(MYSQL* pCon)
{
	if(!pCon)
	{
		return false;
	}
	char szQuery[10];
	memset(szQuery, 0x00, sizeof(szQuery));
	strcpy(szQuery, "select 1");
		
	if(mysql_query(pCon, szQuery))
	{
		return false;
	}	
	if(!(res = mysql_store_result(pCon)))
	{
		return false;	
	}
	mysql_free_result(res);
	
	return true;

}
//******************************************************************************
//* daem5002 main
//******************************************************************************
int daem5002_main_process()
{
    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "daem5002_main_process: gproc_date[%s]\n", gproc_date);  
    ZzLOG(ALWAY, "daem5002_main_process: gdis_date [%s]\n", gdis_date );  
    ZzLOG(ALWAY, "daem5002_main_process: gdel_date [%s]\n", gdel_date );  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");


	//--------------------------------------------------------------------------
	// 게시기간 만료일자가 3일전인 영화, 드라마, 동영상, 애니분류의 
	// 제휴 컨텐츠중 우수 컨텐츠 선별하여 게시기간 연장.
	//--------------------------------------------------------------------------

	if(daem5002_cpr_update_process() < 0)
	{
		return -1;
	}	
	
	//  2009/03/18 : 제휴 게시만료일 고정하던 작업 삭제.(만료일을 9990년 12월 31일로 넣으면서 변경) -- HCS
	//  2009/11/17 : 교육, 휴대기기분류의 제휴 컨텐츠만 게시기간 고정 -- HCS
	daem5002_set_cpr_date();
	//daem5002_set_om_date(); //no.880
	
	//--------------------------------------------------------------------------
	// 처리일자 -1의 거래건수 만클 게시기간 연장처리
	//--------------------------------------------------------------------------
	//if (daem5002_inc_end_date() != 0)
	//	return -1;
		
	//--------------------------------------------------------------------------
	// 처리일자 -1의 게시종료 자료를 삭제 처리한다.
	//--------------------------------------------------------------------------
	daem5002_del_end_date(); //test

	
	daem5002_flog_data_del_end_date(); //test
		
		
	
	return 0;
}
//******************************************************************************
//* daem5002_cpr_update_process() : 제휴 컨텐츠 기간연장 
//* - 처리일자 -3일의 제휴 컨텐츠중 우수 컨텐츠 선정하여 기간연장한다.
//* hcs. 2009-11-17
//******************************************************************************
int daem5002_cpr_update_process()
{
    ZzLOG(ALWAY, "daem5002_cpr_update_process start!!\n");
	
	MYSQL_RES *res;
	MYSQL_ROW  row;
	
	MYSQL_RES *cpr_res;
	MYSQL_ROW  cpr_row;
	
	char szQuery[10000];
	memset(szQuery, 0x00, sizeof(szQuery));
	
	unsigned long ulID = 0;
	char szSectCode[2+1];
	
	int nCnt = 0;

	/*
	cprDB 작업 테이블 삭제
	*/

	memset(szQuery, 0x00, sizeof(szQuery));
	strcpy(szQuery, "delete from zangsi_cpr.T_CPR_CONT_DEAL ");

	//ZzLOG(ALWAY, "\nquery=[%s]\n\n", szQuery); //test
	
	if(mysql_query(con_cpr, szQuery))
	{
		ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
		ZzLOG(ERROR, "daem5002_main_process: [%d](%s)\n",mysql_errno(con_cpr), mysql_error(con_cpr));
		return -1;
	}

	while(1)
	{
		/* 1.
		게시기간 만료일자가 -3일인 영화, 드라마, 동영상, 애니분류의 컨텐츠 선별하여
		cprDB 작업 테이블에 저장.
		CREATE TEMPORARY TABLE TEMP_CPR_DEAL_INFO
		*/
		
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " select a.id as id, a.sect_code as sect_code " 
						 " from zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_VIR_ID b "
						 " where a.id > %lu and a.sect_code in('01', '02', '03', '05') "
						 " and a.disp_end_date <= date_format(date_add(now(), INTERVAL 3 DAY),'%%Y%%m%%d') "
						 " and a.del_yn = 'N' "
						 " and a.id = b.id and b.copyright_yn in('C','H') order by a.id limit 100 "
						, ulID);
		//ZzLOG(ALWAY, "\nquery=[%s]\n\n", szQuery); //test

		if(IsConnectDB(con_bck) == false)
		{
			if (!(con_bck=db_connect_backup("zangsi")))
			{
				ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
			   	return(-1); 
			}
		}

		if(IsConnectDB(con) == false)
		{
			if (!(con=db_connect_local("zangsi")))
			{
				ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
				
			   	return(-1); 
			}
		}


		if (mysql_query(con_bck, szQuery))
		{
			ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
			ZzLOG(ERROR, "daem5002_cpr_update_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
		}
		if(!(res = mysql_store_result(con_bck)))
		{
			ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
			ZzLOG(ERROR, "daem5002_cpr_update_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
		}
		if(mysql_num_rows(res)==0)
		{
			ZzLOG(ALWAY, "daem5002_cpr_update_process: 종료. 처리건수 -- [%d]\n\n", nCnt);
			mysql_free_result(res);
			break;
		}
		while(row = mysql_fetch_row(res))
		{
			ulID = (unsigned long)getnum(row, 0);
			
			memset(szSectCode, 0x00, sizeof(szSectCode));
			strcpy(szSectCode, getstr(row,1));
			
			/* 2.
			cpr DB에서 45일간의 구매건수를 
			cprDB 작업 테이블에 저장
			*/

			if(IsConnectDB(con_cpr) == false)
			{
				if (!(con_cpr=db_connect_cprdb("zangsi_cpr")))
				{
					mysql_free_result(res);
					ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
				   	return(-1); 
				}
			}
	
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " select count(deal_no) "
							 " from zangsi_cpr.T_CPR_DEAL_INFO  "
							 " where id = %lu and proc_yn != 'R' and "
							 " deal_date >= date_format(date_add(now(), INTERVAL -45 DAY),'%%Y%%m%%d') "
							 , ulID);
							 
			//ZzLOG(ALWAY, "\nquery=[%s]\n\n", szQuery); //test
			
			if (mysql_query(con_cpr, szQuery))
			{
				ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
				ZzLOG(ERROR, "daem5002_cpr_update_process: [%d](%s)\n",mysql_errno(con_cpr), mysql_error(con_cpr));
				mysql_free_result(res);
				return -1;
			}
			if(!(cpr_res = mysql_store_result(con_cpr)))
			{
				ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
				ZzLOG(ERROR, "daem5002_cpr_update_process: [%d](%s)\n",mysql_errno(con_cpr), mysql_error(con_cpr));
				mysql_free_result(res);
				return -1;
			}

			cpr_row = mysql_fetch_row(cpr_res);
			int nDealCnt = (int)getint(cpr_row, 0);

			if(mysql_num_rows(cpr_res)==0)
			{
				nDealCnt = 0;
			}
			else
			{
				nDealCnt = (int)getint(cpr_row, 0);
			}
			
			mysql_free_result(cpr_res);	
			
			
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " select c.mgr_cd "
							 " from zangsi_cpr.T_CPR_CONT_MAP_SUB a, zangsi_cpr.T_CPR_HASH_INFO b, zangsi_cpr.T_CPR_CONT_LIST c "
							 " where a.id = %lu and a.cont_gu = 'WE' and a.chi_id = b.chi_id and b.list_id = c.list_id and b.comp_cd = c.comp_cd "
							 " group by c.mgr_cd "
							 , ulID);
			
			if (mysql_query(con_cpr, szQuery))
			{
				ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
				ZzLOG(ERROR, "daem5002_cpr_update_process: [%d](%s)\n",mysql_errno(con_cpr), mysql_error(con_cpr));
				mysql_free_result(res);
				return -1;
			}
			if(!(cpr_res = mysql_store_result(con_cpr)))
			{
				ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
				ZzLOG(ERROR, "daem5002_cpr_update_process: [%d](%s)\n",mysql_errno(con_cpr), mysql_error(con_cpr));
				mysql_free_result(res);
				return -1;
			}
			if(mysql_num_rows(cpr_res)==0)
			{
				
				ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
				ZzLOG(ERROR, "daem5002_cpr_update_process: 관리코드 없음. [%lu]\n",ulID);
				continue;
			}
			
			while(cpr_row = mysql_fetch_row(cpr_res))
			{
				MYSQL_RES *cpr_res1;
				MYSQL_ROW  cpr_row1;
				
				char szMgrCd[20+1];
				memset(szMgrCd, 0x00, sizeof(szMgrCd));
				
				strcpy(szMgrCd, getstr(cpr_row,0));
				
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " select id, sale_cnt from zangsi_cpr.T_CPR_CONT_DEAL "
								 " where mgr_cd = '%s' "
								 , szMgrCd);
								 
				if (mysql_query(con_cpr, szQuery))
				{
					ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
					ZzLOG(ERROR, "daem5002_cpr_update_process: [%d](%s)\n",mysql_errno(con_cpr), mysql_error(con_cpr));
					mysql_free_result(res);
					mysql_free_result(cpr_res);
					return -1;
				}
				if(!(cpr_res1 = mysql_store_result(con_cpr)))
				{
					ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
					ZzLOG(ERROR, "daem5002_cpr_update_process: [%d](%s)\n",mysql_errno(con_cpr), mysql_error(con_cpr));
					mysql_free_result(res);
					mysql_free_result(cpr_res);
					return -1;
				}
				if(mysql_num_rows(cpr_res1)==0)
				{
					mysql_free_result(cpr_res1);
					memset(szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, " insert into zangsi_cpr.T_CPR_CONT_DEAL "
									 " values "
									 " (%lu, '%s', '%s', %d) "
									 , ulID, szMgrCd, szSectCode, nDealCnt);
					
					//ZzLOG(ALWAY, "\nquery=[%s]\n\n", szQuery); //test
					
					if (mysql_query(con_cpr, szQuery))
					{
						ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
						ZzLOG(ERROR, "daem5002_cpr_update_process: [%d](%s)\n",mysql_errno(con_cpr), mysql_error(con_cpr));
						mysql_free_result(res);
						mysql_free_result(cpr_res);	
						return -1;
					}
				}
				else
				{
					cpr_row1 = mysql_fetch_row(cpr_res1);
					unsigned long ulNextID = (unsigned long)getnum(cpr_row1,0);
					int nNextDealCnt = (int)getint(cpr_row1,1);
					mysql_free_result(cpr_res1);	
					
					if(nDealCnt < nNextDealCnt)
					{
						memset(szQuery, 0x00, sizeof(szQuery));
						sprintf(szQuery, " update zangsi_cpr.T_CPR_CONT_DEAL "
										 " set id = %lu, sale_cnt = %d "
										 " where mgr_cd = '%s' "
										 , ulNextID
										 , nNextDealCnt
 										 , szMgrCd);
						
						//ZzLOG(ALWAY, "\nquery=[%s]\n\n", szQuery); //test
						
						if (mysql_query(con_cpr, szQuery))
						{
							ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
							ZzLOG(ERROR, "daem5002_cpr_update_process: [%d](%s)\n",mysql_errno(con_cpr), mysql_error(con_cpr));
							mysql_free_result(res);
							mysql_free_result(cpr_res);	
							return -1;
						}
					}
				}
			}
			mysql_free_result(cpr_res);	
			nCnt++;
		}
		mysql_free_result(res);
	}

	/* 3.
	분류별로 구매건수가 가장많은 한건의 컨텐츠를 읽어와 게시기간을 연장 처리
	등록자에게 우수 컨텐츠 선정과 게시기간 연장에 대한 쪽지를 발송.
	*/
	if(IsConnectDB(con) == false)
	{
		if (!(con=db_connect_local("zangsi")))
		{
			ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		   	return(-1); 
		}
	}

	for(int i=0;i<4;i++)
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		if(i == 0)	
		{
			strcpy(szQuery, " select id from zangsi_cpr.T_CPR_CONT_DEAL "
							" where sect_code = '01' "
							" group by id order by id ");
		}
		else if(i == 1)
		{
			strcpy(szQuery, " select id from zangsi_cpr.T_CPR_CONT_DEAL "
							" where sect_code = '02' "
							" group by id order by id ");
		}
		else if(i == 2)
		{
			strcpy(szQuery, " select id from zangsi_cpr.T_CPR_CONT_DEAL "
							" where sect_code = '03' "
							" group by id order by id ");
		}
		else if(i == 3)
		{
			strcpy(szQuery, " select id from zangsi_cpr.T_CPR_CONT_DEAL "
							" where sect_code = '05' "
							" group by id order by id ");
		}
		else
		{
			ZzLOG(ERROR, "daem5002_cpr_update_process: i error [%d] [%s]", i,szQuery);
		}
		
		//ZzLOG(ALWAY, "\nquery=[%s]\n\n", szQuery); //test
		
		if (mysql_query(con_cpr, szQuery))
		{
			ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
			ZzLOG(ERROR, "daem5002_cpr_update_process: [%d](%s)\n",mysql_errno(con_cpr), mysql_error(con_cpr));
			return -1;
		}
		if(!(cpr_res = mysql_store_result(con_cpr)))
		{
			ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
			ZzLOG(ERROR, "daem5002_cpr_update_process: [%d](%s)\n",mysql_errno(con_cpr), mysql_error(con_cpr));
			return -1;
		}
		if(mysql_num_rows(cpr_res)==0)
		{
			mysql_free_result(cpr_res);
			ZzLOG(ERROR, "mysql_num_rows error Query [ %s ]\n",szQuery);
			continue;
		}
		while(cpr_row = mysql_fetch_row(cpr_res))
		{		
			unsigned long ulUpdateID = (unsigned long)getnum(cpr_row,0);
			
			
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " select reg_user, title from zangsi.T_CONTENTS_INFO  "
							 " where id = %lu "
							 , ulUpdateID);
			//ZzLOG(ALWAY, "\nquery=[%s]\n\n", szQuery); //test
			if (mysql_query(con, szQuery))
			{
				mysql_free_result(cpr_res);
				ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
				ZzLOG(ERROR, "daem5002_cpr_update_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				return -1;
			}
			if(!(res = mysql_store_result(con)))
			{
				ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
				ZzLOG(ERROR, "daem5002_cpr_update_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				return -1;
			}
			if(mysql_num_rows(res)==0)
			{
				ZzLOG(ERROR, "mysql_num_rows error Query [ %s ]\n",szQuery);
				ZzLOG(ERROR, "daem5002_cpr_update_process: T_CONTENTS_INFO reg_user 없음.[%lu]\n", ulUpdateID);
				continue;
			}
			row = mysql_fetch_row(res);

			char szUserID[12+1];
			memset(szUserID, 0x00, sizeof(szUserID));
			
			char szTitle[2000];
			memset(szTitle, 0x00, sizeof(szTitle));			

			sprintf(szUserID, "%s", getstr(row,0));
			sprintf(szTitle, "%s", getstr(row,1));
			

			mysql_free_result(res);
			
			
			/*
			우수 컨텐츠 보유자 쪽지 발송
			*/
			
			char szMemoInfo[5000];
			memset(szMemoInfo,0x00,sizeof(szMemoInfo));
			ReplaceSingleToDouble(szTitle);

			sprintf(szMemoInfo, " 안녕하세요. 위디스크입니다.\r\n\r\n"
								"%s님께서 등록해주신 (%lu : %s)컨텐츠는 위디스크의 우수컨텐츠로 선정되었습니다.\r\n"
								"항상 위디스크를 위해 우수한 컨텐츠 등록으로 많은 회원님들에게 좋은 정보를 제공하여 주신 점 \r\n"
								"감사드립니다.\r\n\r\n"
								"우수컨텐츠로 선정된 %s님의 컨텐츠는 더 많은 회원님들에게 좋은 정보를 제공할수있도록 운영팀에서 자동으로 컨텐츠 게시기간이 45일 연장해드렸습니다.\r\n\r\n"
								"앞으로도 %s님의 보다 많은 애정과 관심을 부탁드립니다.\r\n\r\n"
								"감사합니다."
								, szUserID
								, ulUpdateID, szTitle
								, szUserID
								, szUserID);
/*
			sprintf(szQuery, " insert into zangsi.T_MEMO_INFO (  user_id, memo_cd, ref_id, descript  , send_user, recv_yn, recv_date, recv_time  ) " 
							" values (  '%s' , '01', 0, '%s ','운영팀' , 'N', '%s', '%s' ) "
							, szUserID,szMemoInfo, gsys_date, gsys_time);
			
			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
				ZzLOG(ERROR, "daem5002_cpr_update_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "daem5002_cpr_update_process: 쪽지 발송 실패 [%s][%lu]\n", szUserID, ulUpdateID);
			}
*/

			sprintf(szQuery, " insert into zangsi.T_SEND_MEMO (  memo_cd, descript, send_user, del_yn, send_date, send_time ) " 
								" values (  '04' "
								
								" ,'%s' "
								
								" , '운영팀' ,'N', '%s', '%s' ) "
								, szMemoInfo, gsys_date, gsys_time);
		
			ZzLOG(ALWAY, "[%s]\n\n", szQuery);
			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
				ZzLOG(ERROR, "daem5002_cpr_update_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "daem5002_cpr_update_process: 쪽지 발송 실패 [%s][%lu]\n", szUserID, ulUpdateID);
			}
		
			memset(szQuery , 0x00, sizeof(szQuery ));
			strcpy( szQuery, "SELECT last_insert_id() as send_seq_no" );
			
			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
				ZzLOG(ERROR, "daem5002_cpr_update_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "daem5002_cpr_update_process: 쪽지 발송 실패 [%s][%lu]\n", szUserID, ulUpdateID);
			}
			
			MYSQL_RES* myres = mysql_store_result(con);
			MYSQL_ROW myrow = mysql_fetch_row(myres);
		
			double send_seq_no  = getnum(myrow,0 );	
			
			mysql_free_result(myres);
				
			
			memset(szQuery , 0x00, sizeof(szQuery ));
			sprintf(szQuery," insert into zangsi.T_RECV_MEMO "
							"  (memo_seq_no, recv_user, recv_date, recv_time, read_yn, del_yn, save_yn)  "
							" values "
							"  (%.0f, '%s' , '%s', '%s' , 'N', 'N', 'N') "
							,send_seq_no,szUserID,gsys_date, gsys_time);
							
			ZzLOG(ALWAY, "[%s]\n\n", szQuery);
			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
				ZzLOG(ERROR, "daem5002_cpr_update_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "daem5002_cpr_update_process: 쪽지 발송 실패 [%s][%lu]\n", szUserID, ulUpdateID);
			}
						
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update zangsi.T_CONTENTS_INFO "
							 " set disp_end_date = date_format(date_add(now(), INTERVAL 45 DAY),'%%Y%%m%%d')  "
							 " where id = %lu "
							 , ulUpdateID);
			//ZzLOG(ALWAY, "\nquery=[%s]\n\n", szQuery); //test
			
			if (mysql_query(con, szQuery))
			{
				mysql_free_result(cpr_res);
				ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
				ZzLOG(ERROR, "daem5002_cpr_update_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				return -1;
			}
		}
		mysql_free_result(cpr_res);
	
	}

	return 0;	
}

//******************************************************************************
//* daem5002_inc_end_date() : 기간연장 main
//* - 처리일자 -1일의 판매건수 만큼 기간연장처리 한다.
//******************************************************************************
int daem5002_inc_end_date()
{
	char szQuery[10000];  // query string

	//--------------------------------------------------------------------------
	// 트렌젝션시작
	//--------------------------------------------------------------------------
	if (tran_begin(con)!=0) {
	    ZzLOG(ERROR, "tran_begin: 테이베이스 오류입니다.\n");  
		ZzLOG(ERROR, "daem5105_main_process: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;
	}

	//--------------------------------------------------------------------------
	// 처리일자 -1일의 판매건수를 TEMPORARY에 생성한다.
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "CREATE TEMPORARY TABLE TEMP_DEAL_INFO "
	                 "SELECT a.id, count(a.id) cnt "
	                 "  FROM T_DEAL_INFO     a     "
	                 " 	   , T_CONTENTS_INFO b     "
	                 " WHERE a.deal_date = '%s'    "
	                 "   AND a.cont_gu   = 'WE'    "
	                 "   AND a.id        =  b.id   "
	                 "   AND b.del_yn    = 'N'     "
	                 " GROUP BY a.id               "
	                 ,gproc_date
	                 );
	if (mysql_query(con, szQuery)){
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daem5002_inc_end_date: CREATE TEMP_DEAL_INFO error...\n");
		goto daem5002_inc_end_date_err;
    }

	//--------------------------------------------------------------------------
	// 해당자료의 게시종료일 UPDATE
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	strcat (szQuery, "UPDATE TEMP_DEAL_INFO a, T_CONTENTS_INFO b");
	strcat (szQuery, "   SET b.disp_end_date = date_format(date_add(b.disp_end_date, INTERVAL a.cnt DAY),'%Y%m%d')");
	strcat (szQuery, " WHERE a.id     = b.id");
	strcat (szQuery, "   AND b.del_yn = 'N' ");

	 #ifdef __DEBUG
	 printf("%s\n\n",szQuery);
	 #endif 	
	if (mysql_query(con, szQuery)){
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daem5002_inc_end_date: UPDATE T_CONTENTS_INFO error...\n");
		ZzLOG(ERROR, "daem5002_inc_end_date: [%d](%s)\n",mysql_errno(con), mysql_error(con));
    }

	//--------------------------------------------------------------------------
	// 해당자료의 검색엔진 적용
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	strcat (szQuery, "INSERT T_CONTENTS_CREATE  (id, cont_gu, udt_cd) ");
	strcat (szQuery, "SELECT id, '01', 'U' FROM TEMP_DEAL_INFO        ");
	if (mysql_query(con, szQuery)){
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daem5002_inc_end_date: UPDATE T_CONTENTS_INFO error...\n");
		ZzLOG(ERROR, "daem5002_inc_end_date: [%d](%s)\n",mysql_errno(con), mysql_error(con));
    }

	//--------------------------------------------------------------------------
	// TEMPORARY 테이블을 삭제 한다.
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	strcpy(szQuery, "DROP TABLE TEMP_DEAL_INFO ");
	if (mysql_query(con, szQuery)){
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daem5002_inc_end_date: DROP TEMP_DEAL_INFO error...\n");
		goto daem5002_inc_end_date_err;
    }
    
	if (tran_commit(con)!=0)
	{
	    ZzLOG(ERROR, "daem5002_inc_end_date: tran_commit error...\n");
	    goto daem5002_inc_end_date_err;
	}

	return 0;

daem5002_inc_end_date_err:
	ZzLOG(ERROR, "daem5002_inc_end_date: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	tran_rollback(con);
	tran_end(con);
	return -1;	
}


//******************************************************************************
//* daem5002_del_end_date()
//* - 처리일자 -1의 게시종료일자를 삭제처리한다.
//******************************************************************************
int daem5002_del_end_date()
{
	char szQuery[10000];		// query string
	char szAddQuery[10000];		// query string	
	int nCnt = 0;
	unsigned long id = 0;
	char szRegUser[24];
	memset(szRegUser,0x00,sizeof(szRegUser));
	
	//--------------------------------------------------------------------------
	// 트렌젝션시작
	//--------------------------------------------------------------------------
	if (tran_begin(con)!=0) 
	{
	    ZzLOG(ERROR, "tran_begin: 테이베이스 오류입니다.\n");
	    return -1;
	}
	//--------------------------------------------------------------------------
	// 추천컨텐츠로 등록된자료의 만료기간은 항상 30일로 처리한다.
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	strcpy (szQuery, "UPDATE T_CONTENTS_BEST a, T_CONTENTS_INFO b ");
	strcat (szQuery, "   SET b.disp_end_date = date_format(date_add(now(), INTERVAL 30 DAY),'%Y%m%d')");
	strcat (szQuery, " WHERE a.best_gu in( 'B' ,'C')");
	strcat (szQuery, "   AND a.id      = b.id ");

	 #ifdef __DEBUG
	 printf("%s\n\n",szQuery);
	 #endif 
	 	
	if (mysql_query(con, szQuery)){
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daem5002_del_end_date: UPDATE T_CONTENTS_INFO error...\n");
	    goto daem5002_del_end_date_err;
		//if (mysql_errno(con) != 1062)
    }

	//--------------------------------------------------------------------------
	// moremusic 예외처리
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	strcpy (szQuery, "UPDATE T_CONTENTS_INFO a ");
	strcat (szQuery, "   SET a.disp_end_date = date_format(date_add(now(), INTERVAL 30 DAY),'%Y%m%d')");
	strcat (szQuery, " WHERE a.reg_user='moremusic' and a.disp_end_date <= date_format(date_add(now(), INTERVAL 1 DAY),'%Y%m%d')");

	 #ifdef __DEBUG
	 printf("%s\n\n",szQuery);
	 #endif 

	ZzLOG(ALWAY,"%s\n",szQuery);
	 	
	if (mysql_query(con, szQuery)){
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daem5002_del_end_date: UPDATE T_CONTENTS_INFO error...\n");
	    goto daem5002_del_end_date_err;
		//if (mysql_errno(con) != 1062)
    }


/*
	//--------------------------------------------------------------------------
	// vod 서비스는 365일 제공.
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	strcpy (szQuery, "UPDATE T_CONTENTS_INFO_VOD a, T_CONTENTS_INFO b ");
	strcat (szQuery, "   SET b.disp_end_date = date_format(date_add(now(), INTERVAL 365 DAY),'%Y%m%d')");
	strcat (szQuery, " WHERE  a.id      = b.id ");

	 #ifdef __DEBUG
	 printf("%s\n\n",szQuery);
	 #endif 
	 	
	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daem5002_del_end_date: UPDATE T_CONTENTS_INFO_VOD error...\n");
	    goto daem5002_del_end_date_err;
		//if (mysql_errno(con) != 1062)
    }
*/
	//T_CONTENTS_BEST 에 등록되어 있고 삭제된 컨텐츠에 대한 처리
	{
		//성지순례 중인 것 중 삭제 되는컨텐츠
		
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery,
						"SELECT a.id ,a.reg_user ,b.best_gu "
				         "  FROM zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_BEST b  "
				         " WHERE a.disp_stat     = 'Y'          "
				         "   AND a.disp_end_date <= '%s'         "
				         "   AND a.del_yn        = 'N'          "
				         "   AND a.id = b.id and b.best_gu in('HK' ,'HS','HD') "
				         ,gdis_date
				         );
				         
		 #ifdef __DEBUG
		 printf("%s\n\n",szQuery);
		 #endif 
		ZzLOG(ALWAY,"Best에 등록된 컨텐츠 처리 [ %s ]\n",szQuery);
		if (mysql_query(con, szQuery)){
			ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
		    ZzLOG(ERROR, "daem5002_del_end_date: INSERT T_CONTENTS_ADMDEL error...\n");
		    goto daem5002_del_end_date_err;
			//if (mysql_errno(con) != 1062)
	    }
	    				         
		MYSQL_RES*	best_res = mysql_store_result(con);
		MYSQL_ROW	best_row = NULL;
		unsigned long cont_id=0;
		char szRegUser[24];
		memset(szRegUser,0x00,sizeof(szRegUser));
		char best_gu[10];
		memset(best_gu,0x00,sizeof(best_gu));
		
		while( (best_row=mysql_fetch_row(best_res)) != NULL )
		{
			cont_id = (unsigned long)getnum(best_row,0);
			strcpy(szRegUser ,getstr(best_row,1));
			strcpy(best_gu ,getstr(best_row,2));
			
			if ( cont_id <=  0 )
				break;
				
			memset (szQuery, 0x00, sizeof(szQuery));
			
			
			
			 
			//----------------------------------------------------------------------
			// 검색엔진의 색인정보 삭제
			//----------------------------------------------------------------------
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_CREATE "
			                "        (id, cont_gu, udt_cd) "
			                 "SELECT id     , '01' , 'D'         "
			                 "  FROM zangsi.T_CONTENTS_INFO   "
			                 " WHERE disp_stat     = 'Y'          "
			                 "   AND disp_end_date <= '%s'         "
			                 "   AND del_yn        = 'N'          " 
			                 "	AND id = %lu "
			                , gdis_date
			                , cont_id);
			if (mysql_query(con, szQuery)){
			    ZzLOG(ERROR, "daem5002_delete_data: INSERT T_CONTENTS_CREATE error...\n");
				ZzLOG(ERROR, "daem5002_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				goto daem5002_del_end_date_err;
		    }
		    
			if( strcmp(best_gu,"HK") == 0 )
			{
				sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_ADMDEL"
				                 "     ( cont_gu  , id                "
				                 "     , del_desc , reg_user          "
				                 "     , reg_date , reg_time		 "
				                 "     , proc_yn )         "//20130523 - 성지순례 정보만 남기기
				                 "SELECT '01'     , id                "
				                 "     , '게시기간 만료' , 'system'    "
				                 "     , '%s'     ,'%s'               "
				                 "     , 'T' " //20130523 - 성지순례 정보만 남기기
				                 "  FROM zangsi.T_CONTENTS_INFO "
				                 " WHERE disp_stat     = 'Y'          "
				                 "   AND disp_end_date <= '%s'         "
				                 "   AND del_yn        = 'N'          "
				                 "	AND id = %lu "
				                 ,gsys_date
				                 ,gsys_time
				                 ,gdis_date
				                 ,cont_id
				                 );
				                 
				                 
				 #ifdef __DEBUG
				 printf("%s\n\n",szQuery);
				 #endif 
			
				if (mysql_query(con, szQuery)){
					ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
				    ZzLOG(ERROR, "daem5002_del_end_date: INSERT T_CONTENTS_ADMDEL error...\n");
				    goto daem5002_del_end_date_err;
			    }	
				                
				                 
				memset (szQuery, 0x00, sizeof(szQuery));
				//20100824 - no.575
				sprintf(szQuery, " update zangsi.T_CONTENTS_VIR_ID2  "
				                 " set del_yn = 'Y' , copyright_yn= 'T' "
				                 " where id = %lu "
				                 ,cont_id  );
				 #ifdef __DEBUG
				 printf("%s\n\n",szQuery);
				 #endif 
				 	                 
				if (mysql_query(con, szQuery)){
					ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
				    ZzLOG(ERROR, "daem5002_del_end_date: UPDATE T_CONTENTS_INFO error...\n");
				    goto daem5002_del_end_date_err;
			    }
			
				memset (szQuery, 0x00, sizeof(szQuery));
				//20100824 - no.575
				sprintf(szQuery, " update zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_VIR_ID b "
				                 " set a.disp_stat = 'N', a.del_yn = 'Y', b.del_yn = 'Y' "
				                 " , b.copyright_yn= 'T'  "
				                 " where a.id = b.id and a.disp_stat = 'Y' and a.disp_end_date <= '%s' and a.del_yn = 'N' and a.id = %lu "
				                 ,gdis_date,cont_id
				                 );
				 #ifdef __DEBUG
				 printf("%s\n\n",szQuery);
				 #endif 
				 	                 
				if (mysql_query(con, szQuery)){
					ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
				    ZzLOG(ERROR, "daem5002_del_end_date: UPDATE T_CONTENTS_INFO error...\n");
				    goto daem5002_del_end_date_err;
			    }
			    
			}
			else
			{
				sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_ADMDEL"
				                 "     ( cont_gu  , id                "
				                 "     , del_desc , reg_user          "
				                 "     , reg_date , reg_time		 "
				                 "     , proc_yn )         "//20130523 - 성지순례 정보만 남기기
				                 "SELECT '01'     , id                "
				                 "     , '게시기간 만료' , 'system'    "
				                 "     , '%s'     ,'%s'               "
				                 "     , 'S' " //20130523 - 성지순례 정보만 남기기
				                 "  FROM zangsi.T_CONTENTS_INFO "
				                 " WHERE disp_stat     = 'Y'          "
				                 "   AND disp_end_date <= '%s'         "
				                 "   AND del_yn        = 'N'          "
				                 "	AND id = %lu "
				                 ,gsys_date
				                 ,gsys_time
				                 ,gdis_date
				                 ,cont_id
				                 );
				                 
				memset (szQuery, 0x00, sizeof(szQuery));
				//20100824 - no.575
				sprintf(szQuery, " update zangsi.T_CONTENTS_VIR_ID2  "
				                 " set del_yn = 'Y' , copyright_yn= 'S' "
				                 " where id = %lu "
				                 ,cont_id  );
				 #ifdef __DEBUG
				 printf("%s\n\n",szQuery);
				 #endif 
				 	                 
				if (mysql_query(con, szQuery)){
					ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
				    ZzLOG(ERROR, "daem5002_del_end_date: UPDATE T_CONTENTS_INFO error...\n");
				    goto daem5002_del_end_date_err;
			    }
			
				memset (szQuery, 0x00, sizeof(szQuery));
				//20100824 - no.575
				sprintf(szQuery, " update zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_VIR_ID b "
				                 " set a.disp_stat = 'N', a.del_yn = 'Y', b.del_yn = 'Y' "
				                 " , b.copyright_yn= 'S'  "
				                 " where a.id = b.id and a.disp_stat = 'Y' and a.disp_end_date <= '%s' and a.del_yn = 'N' and a.id = %lu "
				                 ,gdis_date,cont_id
				                 );
				 #ifdef __DEBUG
				 printf("%s\n\n",szQuery);
				 #endif 
				 	                 
				if (mysql_query(con, szQuery)){
					ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
				    ZzLOG(ERROR, "daem5002_del_end_date: UPDATE T_CONTENTS_INFO error...\n");
				    goto daem5002_del_end_date_err;
			    }
			    
			}
			
		    
		    //--------------------------------------------------------------------------
			// T_CONTENTS_DEL insert
			//--------------------------------------------------------------------------
			//유일해쉬 확인
			
			sprintf(szQuery," select default_hash from zangsi.T_CONTENTS_FILELIST_SUB where id = %lu and file_size > 10485760 ",cont_id);
			 
			
			if(mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "DeleteContents: INSERT T_CONTENTS_DEL error...\n");
			    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
				goto daem5002_del_end_date_err;

		    }
		    MYSQL_RES* hash_ori_res = NULL;
		    MYSQL_ROW hash_ori_row = NULL;
		    bool only_one_contents = false;
			if(!(hash_ori_res = mysql_store_result(con)))
			{
				ZzLOG(ERROR, "DeleteContents: mysql_store_result error [%s]\n", szQuery);
				ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
				goto daem5002_del_end_date_err;
			}
			
			
			                 
			while(( hash_ori_row = mysql_fetch_row(hash_ori_res)) != NULL )
			{
				sprintf(szQuery," select id from zangsi.T_CONTENTS_FILELIST_SUB where id != %lu and default_hash = '%s' limit 1 ",cont_id,getstr(hash_ori_row,0) );

				if(mysql_query(con, szQuery))
				{
					ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
				    ZzLOG(ERROR, "DeleteContents: INSERT T_CONTENTS_DEL error...\n");
				    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
					goto daem5002_del_end_date_err;
	
			    }
			    MYSQL_RES* hash_res = NULL;
			    char file_type[6];
			    memset(file_type,0x00,sizeof(file_type));
	
				if(!(hash_res = mysql_store_result(con)))
				{
					ZzLOG(ERROR, "DeleteContents: mysql_store_result error [%s]\n", szQuery);
					ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
					goto daem5002_del_end_date_err;
				}

				int num_row = mysql_num_rows(hash_res);
				if(num_row  == 0)
				{
					strcpy(file_type,"|U|");
					only_one_contents= true;
					memset (szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_DEL "
					                 "     ( cont_gu      , title       "
					                 "     , id           , folder_yn   "
					                 "     , server_id    , del_date    "
					                 "     , file_path    , file_name1  "
					                 "     , file_name2   , file_size   "
					                 "     , file_type    , reg_user    "
					                 "     , reg_date     , reg_time)   "
					                 "SELECT 'WE'         ,a.title      "
					                 "     , a.id         , b.folder_yn "
					                 "     , b.server_id  , '%s'        "
					                 "     , b.file_path  , b.file_name1"
					                 "     , b.file_name2 , b.file_size "
					                 "     , '%s'  , b.reg_user  "
					                 "     , b.reg_date   , b.reg_time  "
					                 "  FROM zangsi.T_CONTENTS_INFO a   "
					                 "     , zangsi.T_CONTENTS_FILE b   "
					                 " WHERE a.disp_stat     = 'Y'      "
					                 "   AND a.disp_end_date <= '%s'     "
					                 "   AND a.del_yn        = 'N'      "
									 "   AND b.file_resoX    = 0        "	                 
					                 "   AND a.id            = b.id     "	
					                 "	and a.id = %lu "
					                 ,gdel_date
					                 ,file_type
					                 ,gdis_date
					                 ,cont_id
					                 );
					              
				}
				
				mysql_free_result(hash_res);
				
				if( only_one_contents )
					break;
			
				
			}
// 2014.03.13  modify by leesh.. 밑에 꺼 땜시..
//			- 2014-03-13 오후 5:16
//			해시 값이 10개 이하인 경우 보존 자료로 처리
//			C 데몬, 삭제 예정일을 99991231 로
//			del_yn: C
			if( only_one_contents == false )
			{
				if(num_row > 10)
				{
					memset (szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_DEL "
									 "     ( cont_gu      , title       "
									 "     , id           , folder_yn   "
									 "     , server_id    , del_date    "
									 "     , file_path    , file_name1  "
									 "     , file_name2   , file_size   "
									 "     , file_type    , reg_user    "
									 "     , reg_date     , reg_time)   "
									 "SELECT 'WE'         ,a.title      "
									 "     , a.id         , b.folder_yn "
									 "     , b.server_id  , '%s'        "
									 "     , b.file_path  , b.file_name1"
									 "     , b.file_name2 , b.file_size "
									 "     , b.file_type  , b.reg_user  "
									 "     , b.reg_date   , b.reg_time  "
									 "  FROM zangsi.T_CONTENTS_INFO a   "
									 "     , zangsi.T_CONTENTS_FILE b   "
									 " WHERE a.disp_stat     = 'Y'      "
									 "   AND a.disp_end_date <= '%s'     "
									 "   AND a.del_yn        = 'N'      "
									 "   AND b.file_resoX    = 0        "
									 "   AND a.id            = b.id     "
									 "	and a.id = %lu "
									 ,gdel_date
									 ,gdis_date
									 ,cont_id
									 );
				}
				else
				{
					memset (szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_DEL "
									 "     ( cont_gu      , title       "
									 "     , id           , folder_yn   "
									 "     , server_id    , del_date    "
									 "     , file_path    , file_name1  "
									 "     , file_name2   , file_size   "
									 "     , file_type    , reg_user    "
									 "     , reg_date     , reg_time"
									 "     , del_yn                  )   "
									 "SELECT 'WE'         ,a.title      "
									 "     , a.id         , b.folder_yn "
									 "     , b.server_id  , '%s'        "
									 "     , b.file_path  , b.file_name1"
									 "     , b.file_name2 , b.file_size "
									 "     , b.file_type  , b.reg_user  "
									 "     , b.reg_date   , b.reg_time "
									 "     , 'C'                        "
									 "  FROM zangsi.T_CONTENTS_INFO a   "
									 "     , zangsi.T_CONTENTS_FILE b   "
									 " WHERE a.disp_stat     = 'Y'      "
									 "   AND a.disp_end_date <= '%s'     "
									 "   AND a.del_yn        = 'N'      "
									 "   AND b.file_resoX    = 0        "
									 "   AND a.id            = b.id     "
									 "	and a.id = %lu "
									 ,"99991231"
									 ,gdis_date
									 ,cont_id
									 );
				}
			}	                 
			#ifdef __DEBUG
			printf("%s\n\n",szQuery);
			#endif 
			 	                 
			if (mysql_query(con, szQuery)){
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "daem5002_del_end_date: INSERT T_CONTENTS_DEL error...\n");
			    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
		    	goto daem5002_del_end_date_err;
		    }
		    		                 
			mysql_free_result(hash_ori_res);    	
		    	
			cont_id = 0;
			memset(szRegUser,0x00,sizeof(szRegUser));	
			memset(best_gu,0x00,sizeof(best_gu));
		}
			
		mysql_free_result(best_res);	 
	} 
    
    
    //memo가 50개 이상일 경우 처리 
    {
    	memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery,
						"SELECT a.id ,a.reg_user "
				         "  FROM zangsi.T_CONTENTS_INFO a  "
				         " WHERE a.disp_stat     = 'Y'          "
				         "   AND a.disp_end_date <= '%s'         "
				         "   AND a.del_yn        = 'N'          "
				         " and a.memo_cnt >= 50 "
				         ,gdis_date
				         );
		                 				         
				         
		 #ifdef __DEBUG
		 printf("%s\n\n",szQuery);
		 #endif
		
		if (mysql_query(con, szQuery)){
			ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
		    ZzLOG(ERROR, "daem5002_del_end_date: INSERT T_CONTENTS_ADMDEL error...\n");
		    goto daem5002_del_end_date_err;
			//if (mysql_errno(con) != 1062)
	    }
	     
	     				         
		MYSQL_RES*	best_res = mysql_store_result(con);
		ZzLOG(ALWAY,"Memo가 50개 이상일 경우 처리 [ %s ]\n",szQuery);
		MYSQL_ROW	best_row = NULL;
		unsigned long cont_id=0;
		char szRegUser[24];
		memset(szRegUser,0x00,sizeof(szRegUser));
		
		
		while( (best_row=mysql_fetch_row(best_res)) != NULL )
		{
			cont_id = (unsigned long)getnum(best_row,0);
			strcpy(szRegUser ,getstr(best_row,1));
		
			ZzLOG(ALWAY,"cont_id [ %lu ] , user_id [ %s ]\n",cont_id,szRegUser);
			
			if ( cont_id <=  0 )
				break;
				
			memset (szQuery, 0x00, sizeof(szQuery));
			
			
			sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_BEST  ( best_gu, id ,reg_date,reg_time) values ( 'HK' ,%lu ,'%s','%s') "
			 				 ,cont_id
			 				 ,gsys_date
			                 ,gsys_time
			                 );
				                 
				                 
			 #ifdef __DEBUG
			 printf("%s\n\n",szQuery);
			 #endif 
		
			if (mysql_query(con, szQuery)){
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "daem5002_del_end_date: INSERT T_CONTENTS_ADMDEL error...\n");
			    goto daem5002_del_end_date_err;
		    }	
			 
			//----------------------------------------------------------------------
			// 검색엔진의 색인정보 삭제
			//----------------------------------------------------------------------
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_CREATE "
			                "        (id, cont_gu, udt_cd) "
			                 "SELECT id     , '01' , 'D'         "
			                 "  FROM zangsi.T_CONTENTS_INFO   "
			                 " WHERE disp_stat     = 'Y'          "
			                 "   AND disp_end_date <= '%s'         "
			                 "   AND del_yn        = 'N'          " 
			                 "	AND id = %lu "
			                , gdis_date
			                , cont_id);
			if (mysql_query(con, szQuery)){
			    ZzLOG(ERROR, "daem5002_delete_data: INSERT T_CONTENTS_CREATE error...\n");
				ZzLOG(ERROR, "daem5002_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				goto daem5002_del_end_date_err;
		    }
	    
			sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_ADMDEL"
			                 "     ( cont_gu  , id                "
			                 "     , del_desc , reg_user          "
			                 "     , reg_date , reg_time		 "
			                 "     , proc_yn )         "//20130523 - 성지순례 정보만 남기기
			                 "SELECT '01'     , id                "
			                 "     , '게시기간 만료' , 'system'    "
			                 "     , '%s'     ,'%s'               "
			                 "     , 'T' " //20130523 - 성지순례 정보만 남기기
			                 "  FROM zangsi.T_CONTENTS_INFO "
			                 " WHERE disp_stat     = 'Y'          "
			                 "   AND disp_end_date <= '%s'         "
			                 "   AND del_yn        = 'N'          "
			                 "	AND id = %lu "
			                 ,gsys_date
			                 ,gsys_time
			                 ,gdis_date
			                 ,cont_id
			                 );
				                 
				                 
			 #ifdef __DEBUG
			 printf("%s\n\n",szQuery);
			 #endif 
		
			if (mysql_query(con, szQuery)){
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "daem5002_del_end_date: INSERT T_CONTENTS_ADMDEL error...\n");
			    goto daem5002_del_end_date_err;
		    }	
				                
				                 
			memset (szQuery, 0x00, sizeof(szQuery));
			//20100824 - no.575
			sprintf(szQuery, " update zangsi.T_CONTENTS_VIR_ID2  "
			                 " set del_yn = 'Y' , copyright_yn= 'T' "
			                 " where id = %lu "
			                 ,cont_id  );
			 #ifdef __DEBUG
			 printf("%s\n\n",szQuery);
			 #endif 
			 	                 
			if (mysql_query(con, szQuery)){
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "daem5002_del_end_date: UPDATE T_CONTENTS_INFO error...\n");
			    goto daem5002_del_end_date_err;
		    }
		
			memset (szQuery, 0x00, sizeof(szQuery));
			//20100824 - no.575
			sprintf(szQuery, " update zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_VIR_ID b "
			                 " set a.disp_stat = 'N', a.del_yn = 'Y', b.del_yn = 'Y' "
			                 " , b.copyright_yn= 'T'  "
			                 " where a.id = b.id and a.disp_stat = 'Y' and a.disp_end_date <= '%s' and a.del_yn = 'N' and a.id = %lu "
			                 ,gdis_date,cont_id
			                 );
			 #ifdef __DEBUG
			 printf("%s\n\n",szQuery);
			 #endif 
			 	                 
			if (mysql_query(con, szQuery)){
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "daem5002_del_end_date: UPDATE T_CONTENTS_INFO error...\n");
			    goto daem5002_del_end_date_err;
		    }
		  	
		    
		    //--------------------------------------------------------------------------
			// T_CONTENTS_DEL insert
			//--------------------------------------------------------------------------
			//유일해쉬 확인
			sprintf(szQuery," select default_hash from zangsi.T_CONTENTS_FILELIST_SUB where id = %lu and file_size > 10485760 ",cont_id);
			 
			
			if(mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "DeleteContents: INSERT T_CONTENTS_DEL error...\n");
			    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
				goto daem5002_del_end_date_err;

		    }
		    MYSQL_RES* hash_ori_res = NULL;
		    MYSQL_ROW hash_ori_row = NULL;
			bool only_one_contents = false;	
			if(!(hash_ori_res = mysql_store_result(con)))
			{
				ZzLOG(ERROR, "DeleteContents: mysql_store_result error [%s]\n", szQuery);
				ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
				goto daem5002_del_end_date_err;
			}
			while(( hash_ori_row = mysql_fetch_row(hash_ori_res)) != NULL )
			{
				sprintf(szQuery," select id from zangsi.T_CONTENTS_FILELIST_SUB where id != %lu and default_hash = '%s' limit 1 ",cont_id,getstr(hash_ori_row,0) );

				if(mysql_query(con, szQuery))
				{
					ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
				    ZzLOG(ERROR, "DeleteContents: INSERT T_CONTENTS_DEL error...\n");
				    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
					goto daem5002_del_end_date_err;
	
			    }
			    MYSQL_RES* hash_res = NULL;
			    char file_type[6];
			    memset(file_type,0x00,sizeof(file_type));
	
				if(!(hash_res = mysql_store_result(con)))
				{
					ZzLOG(ERROR, "DeleteContents: mysql_store_result error [%s]\n", szQuery);
					ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
					goto daem5002_del_end_date_err;
				}

				int row_num = mysql_num_rows(hash_res);
				if(row_num == 0)
				{
					strcpy(file_type,"|U|");
					only_one_contents= true;
					memset (szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_DEL "
					                 "     ( cont_gu      , title       "
					                 "     , id           , folder_yn   "
					                 "     , server_id    , del_date    "
					                 "     , file_path    , file_name1  "
					                 "     , file_name2   , file_size   "
					                 "     , file_type    , reg_user    "
					                 "     , reg_date     , reg_time)   "
					                 "SELECT 'WE'         ,a.title      "
					                 "     , a.id         , b.folder_yn "
					                 "     , b.server_id  , '%s'        "
					                 "     , b.file_path  , b.file_name1"
					                 "     , b.file_name2 , b.file_size "
					                 "     , '%s'  , b.reg_user  "
					                 "     , b.reg_date   , b.reg_time  "
					                 "  FROM zangsi.T_CONTENTS_INFO a   "
					                 "     , zangsi.T_CONTENTS_FILE b   "
					                 " WHERE a.disp_stat     = 'Y'      "
					                 "   AND a.disp_end_date <= '%s'     "
					                 "   AND a.del_yn        = 'N'      "
									 "   AND b.file_resoX    = 0        "	                 
					                 "   AND a.id            = b.id     "	
					                 "	and a.id = %lu "
					                 ,gdel_date
					                 ,file_type
					                 ,gdis_date
					                 ,cont_id
					                 );
					              
				}
				
				mysql_free_result(hash_res);
				
				if( only_one_contents )
					break;
			
				
			}
			if( only_one_contents == false )
			{
				if(row_num >10)
				{
				memset (szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_DEL "
				                 "     ( cont_gu      , title       "
				                 "     , id           , folder_yn   "
				                 "     , server_id    , del_date    "
				                 "     , file_path    , file_name1  "
				                 "     , file_name2   , file_size   "
				                 "     , file_type    , reg_user    "
				                 "     , reg_date     , reg_time)   "
				                 "SELECT 'WE'         ,a.title      "
				                 "     , a.id         , b.folder_yn "
				                 "     , b.server_id  , '%s'        "
				                 "     , b.file_path  , b.file_name1"
				                 "     , b.file_name2 , b.file_size "
				                 "     , b.file_type  , b.reg_user  "
				                 "     , b.reg_date   , b.reg_time  "
				                 "  FROM zangsi.T_CONTENTS_INFO a   "
				                 "     , zangsi.T_CONTENTS_FILE b   "
				                 " WHERE a.disp_stat     = 'Y'      "
				                 "   AND a.disp_end_date <= '%s'     "
				                 "   AND a.del_yn        = 'N'      "
								 "   AND b.file_resoX    = 0        "	                 
				                 "   AND a.id            = b.id     "	
				                 "	and a.id = %lu "
				                 ,gdel_date
				                 ,gdis_date
				                 ,cont_id
				                 );
	                 }
	                 else
	                 {
	     				memset (szQuery, 0x00, sizeof(szQuery));
	     				sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_DEL "
	     				                 "     ( cont_gu      , title       "
	     				                 "     , id           , folder_yn   "
	     				                 "     , server_id    , del_date    "
	     				                 "     , file_path    , file_name1  "
	     				                 "     , file_name2   , file_size   "
	     				                 "     , file_type    , reg_user    "
	     				                 "     , reg_date     , reg_time    "
	     				                 "     , del_yn)                    "
	     				                 "SELECT 'WE'         ,a.title      "
	     				                 "     , a.id         , b.folder_yn "
	     				                 "     , b.server_id  , '%s'        "
	     				                 "     , b.file_path  , b.file_name1"
	     				                 "     , b.file_name2 , b.file_size "
	     				                 "     , b.file_type  , b.reg_user  "
	     				                 "     , b.reg_date   , b.reg_time  "
	     				                 "     , 'C'                        "
	     				                 "  FROM zangsi.T_CONTENTS_INFO a   "
	     				                 "     , zangsi.T_CONTENTS_FILE b   "
	     				                 " WHERE a.disp_stat     = 'Y'      "
	     				                 "   AND a.disp_end_date <= '%s'     "
	     				                 "   AND a.del_yn        = 'N'      "
	     								 "   AND b.file_resoX    = 0        "
	     				                 "   AND a.id            = b.id     "
	     				                 "	and a.id = %lu "
	     				                 ,gdel_date
	     				                 ,gdis_date
	     				                 ,cont_id
	     				                 );

	                 }
                 }
			}	                 
			#ifdef __DEBUG
			printf("%s\n\n",szQuery);
			#endif 
			ZzLOG(ALWAY, "daem5002 Query [ %s ]\n",szQuery);
			if (mysql_query(con, szQuery)){
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "daem5002_del_end_date: INSERT T_CONTENTS_DEL error...\n");
			    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
		    	goto daem5002_del_end_date_err;
		    }
		    		                 
			mysql_free_result(hash_ori_res);    
		    	
			cont_id = 0;
			memset(szRegUser,0x00,sizeof(szRegUser));	
		}
			
		mysql_free_result(best_res);
    }
    
    //일반 처리 로직  - 성지 순례처리에서 del_yn = Y 로 바꾸기 때문에 중첩 처리 되지 않음.
    //일반 로직 처리  - 위에서 처리하지 않는 모든것들을 처리 한다.
    {
    	
    	memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery,
						"SELECT a.id ,a.reg_user ,a.sect_code"
				         "  FROM zangsi.T_CONTENTS_INFO a "
				         " WHERE a.disp_stat     = 'Y'          "
				         "   AND a.disp_end_date <= '%s'         "
				         "   AND a.del_yn        = 'N'          "
				         ,gdis_date
				         );
				         
		 #ifdef __DEBUG
		 printf("%s\n\n",szQuery);
		 #endif 
		ZzLOG(ALWAY,"일반 컨텐츠 처리 [ %s ]\n",szQuery);
		if (mysql_query(con, szQuery)){
			ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
		    ZzLOG(ERROR, "daem5002_del_end_date: INSERT T_CONTENTS_ADMDEL error...\n");
		    goto daem5002_del_end_date_err;
			//if (mysql_errno(con) != 1062)
	    }
	    
	    MYSQL_RES*	best_res = mysql_store_result(con);
		MYSQL_ROW	best_row = NULL;
		unsigned long cont_id=0;
		char szRegUser[24];
		memset(szRegUser,0x00,sizeof(szRegUser));
		char sect_code[6];
		memset(sect_code,0x00,sizeof(sect_code));
		
		while( (best_row=mysql_fetch_row(best_res)) != NULL )
		{
			cont_id = (unsigned long)getnum(best_row,0);
			strcpy(szRegUser ,getstr(best_row,1));
			strcpy(sect_code ,getstr(best_row,2));
			
			if ( cont_id <=  0 )
				break;
				
			//--------------------------------------------------------------------------
			// T_CONTENTS_ADMDEL insert 
			// 												 
			//--------------------------------------------------------------------------
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_ADMDEL"
			                 "     ( cont_gu  , id                "
			                 "     , del_desc , reg_user          "
			                 "     , reg_date , reg_time		 "
			                 "     , proc_yn )         "
			                 "SELECT '01'     , id                "
			                 "     , '게시기간 만료' , 'system'    "
			                 "     , '%s'     ,'%s'               "
			                 "     ,'N' " 
			                 "  FROM zangsi.T_CONTENTS_INFO       "
			                 " WHERE disp_stat     = 'Y'          "
			                 "   AND disp_end_date <= '%s'         "
			                 "   AND del_yn        = 'N'          "
			               	 "	AND id = %lu "
			                 ,gsys_date
			                 ,gsys_time
			                 ,gdis_date
			                 ,cont_id
			                 );
					                 
			                 
			 #ifdef __DEBUG
			 printf("%s\n\n",szQuery);
			 #endif 
		
			if (mysql_query(con, szQuery)){
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "daem5002_del_end_date: INSERT T_CONTENTS_ADMDEL error...\n");
			    goto daem5002_del_end_date_err;
				//if (mysql_errno(con) != 1062)
		    }
			//----------------------------------------------------------------------
			// 검색엔진의 색인정보 삭제
			//----------------------------------------------------------------------
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_CREATE "
			                "        (id, cont_gu, udt_cd) "
			                 "SELECT id     , '01' , 'D'         "
			                 "  FROM zangsi.T_CONTENTS_INFO       "
			                 " WHERE disp_stat     = 'Y'          "
			                 "   AND disp_end_date <= '%s'         "
			                 "   AND del_yn        = 'N'          "
			                 " and id = %lu "
			                , gdis_date
			                ,cont_id );
			if (mysql_query(con, szQuery)){
			    ZzLOG(ERROR, "daem5002_delete_data: INSERT T_CONTENTS_CREATE error...\n");
				ZzLOG(ERROR, "daem5002_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				goto daem5002_del_end_date_err;
		    }
	
			//----------------------------------------------------------------------
			// 이미지 정보 추가
			//----------------------------------------------------------------------
			memset (szQuery, 0x00, sizeof(szQuery));
			memset (szAddQuery, 0x00, sizeof(szAddQuery));
			
			//20100806. hcs(성인분류 이미지 남김)     
			sprintf(szQuery,  "insert into zangsi.T_IMG_DEL  "
			                "        ( cont_gu, seq_no, filog_cn  "
			                " 		   , img01, img02, img03, img04, img05 "
			                "		   , file_path1, file_path2, file_name, reg_user, reg_date, reg_time  )"
			                 " SELECT 'WE'     , a.img_no , 0 ,'N','N','N','N','N'         "
			                 " ,'/zangsi/project/zangsi/files_zangsi/contents/img',a.img_spath,a.img_sname,a.reg_user,date_format(curdate(), '%%Y%%m%%d'), date_format(now(), '%%H%%i%%s') "
			                 "  FROM zangsi.T_CONTENTS_IMG  a , zangsi.T_CONTENTS_INFO b     "
			                 " WHERE a.cont_gu = 'WE' and a.reg_user = b.reg_user and a.id = b.id  and b.disp_stat = 'Y'  and b.del_yn = 'N' " 
			                 " and b.sect_code != '11' " 
			                 " and b.disp_end_date <= '%s' "
			                 " and b.id = %lu "
			                 , gdis_date
			                 , cont_id );
			//ZzLOG(ALWAY, " Query [ %s ] \n",szQuery);
			
			
			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daem5002_delete_data: INSERT T_IMG_DEL error...\n");
				ZzLOG(ERROR, "daem5002_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				goto daem5002_del_end_date_err;
		    }
		
		    
			memset (szQuery, 0x00, sizeof(szQuery));
			
			if( strcmp(sect_code,"11") != 0 )
			{
				sprintf(szQuery,"delete from zangsi.T_CONTENTS_IMG where cont_gu='WE' and reg_user = '%s' and id = %lu",szRegUser,cont_id );
			
				if (mysql_query(con, szQuery))
				{
				    ZzLOG(ERROR, "daem5002_delete_data: delete T_CONTENTS_IMG error...\n");
					ZzLOG(ERROR, "daem5002_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
					goto daem5002_del_end_date_err;
				}
			}
			
			//--------------------------------------------------------------------------
			// T_CONTENTS_DEL insert
			//--------------------------------------------------------------------------
			//유일해쉬 확인
			sprintf(szQuery," select default_hash from zangsi.T_CONTENTS_FILELIST_SUB where id = %lu and file_size > 10485760 ",cont_id);
			 
			
			if(mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "DeleteContents: INSERT T_CONTENTS_DEL error...\n");
			    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
				goto daem5002_del_end_date_err;

		    }
		    MYSQL_RES* hash_ori_res = NULL;
		    MYSQL_ROW hash_ori_row = NULL;
			bool only_one_contents = false;	
			if(!(hash_ori_res = mysql_store_result(con)))
			{
				ZzLOG(ERROR, "DeleteContents: mysql_store_result error [%s]\n", szQuery);
				ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
				goto daem5002_del_end_date_err;
			}
			while(( hash_ori_row = mysql_fetch_row(hash_ori_res)) != NULL )
			{
				sprintf(szQuery," select id from zangsi.T_CONTENTS_FILELIST_SUB where id != %lu and default_hash = '%s' limit 1 ",cont_id,getstr(hash_ori_row,0) );

				if(mysql_query(con, szQuery))
				{
					ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
				    ZzLOG(ERROR, "DeleteContents: INSERT T_CONTENTS_DEL error...\n");
				    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
					goto daem5002_del_end_date_err;
	
			    }
			    MYSQL_RES* hash_res = NULL;
			    char file_type[6];
			    memset(file_type,0x00,sizeof(file_type));
	
				if(!(hash_res = mysql_store_result(con)))
				{
					ZzLOG(ERROR, "DeleteContents: mysql_store_result error [%s]\n", szQuery);
					ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
					goto daem5002_del_end_date_err;
				}
				if(mysql_num_rows(hash_res) == 0)
				{
					strcpy(file_type,"|U|");
					only_one_contents= true;
					memset (szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_DEL "
					                 "     ( cont_gu      , title       "
					                 "     , id           , folder_yn   "
					                 "     , server_id    , del_date    "
					                 "     , file_path    , file_name1  "
					                 "     , file_name2   , file_size   "
					                 "     , file_type    , reg_user    "
					                 "     , reg_date     , reg_time)   "
					                 "SELECT 'WE'         ,a.title      "
					                 "     , a.id         , b.folder_yn "
					                 "     , b.server_id  , '%s'        "
					                 "     , b.file_path  , b.file_name1"
					                 "     , b.file_name2 , b.file_size "
					                 "     , '%s'  , b.reg_user  "
					                 "     , b.reg_date   , b.reg_time  "
					                 "  FROM zangsi.T_CONTENTS_INFO a   "
					                 "     , zangsi.T_CONTENTS_FILE b   "
					                 " WHERE a.disp_stat     = 'Y'      "
					                 "   AND a.disp_end_date <= '%s'     "
					                 "   AND a.del_yn        = 'N'      "
									 "   AND b.file_resoX    = 0        "	                 
					                 "   AND a.id            = b.id     "	
					                 "	and a.id = %lu "
					                 ,gdel_date
					                 ,file_type
					                 ,gdis_date
					                 ,cont_id
					                 );
					              
				}
				
				mysql_free_result(hash_res);
				
				if( only_one_contents )
					break;
			
				
			}
			if( only_one_contents == false )
			{
				memset (szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_DEL "
				                 "     ( cont_gu      , title       "
				                 "     , id           , folder_yn   "
				                 "     , server_id    , del_date    "
				                 "     , file_path    , file_name1  "
				                 "     , file_name2   , file_size   "
				                 "     , file_type    , reg_user    "
				                 "     , reg_date     , reg_time)   "
				                 "SELECT 'WE'         ,a.title      "
				                 "     , a.id         , b.folder_yn "
				                 "     , b.server_id  , '%s'        "
				                 "     , b.file_path  , b.file_name1"
				                 "     , b.file_name2 , b.file_size "
				                 "     , b.file_type  , b.reg_user  "
				                 "     , b.reg_date   , b.reg_time  "
				                 "  FROM zangsi.T_CONTENTS_INFO a   "
				                 "     , zangsi.T_CONTENTS_FILE b   "
				                 " WHERE a.disp_stat     = 'Y'      "
				                 "   AND a.disp_end_date <= '%s'     "
				                 "   AND a.del_yn        = 'N'      "
								 "   AND b.file_resoX    = 0        "	                 
				                 "   AND a.id            = b.id     "	
				                 "	and a.id = %lu "
				                 ,gdel_date
				                 ,gdis_date
				                 ,cont_id
				                 );
			}	                 
			#ifdef __DEBUG
			printf("%s\n\n",szQuery);
			#endif 
			ZzLOG(ALWAY, "daem5002 Query [ %s ]\n",szQuery); 	                 
			if (mysql_query(con, szQuery)){
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "daem5002_del_end_date: INSERT T_CONTENTS_DEL error...\n");
			    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
		    	goto daem5002_del_end_date_err;
		    }
		    		                 
			mysql_free_result(hash_ori_res);    
			//--------------------------------------------------------------------------
			// 게시기간 종료처리
			//--------------------------------------------------------------------------
			
			memset (szQuery, 0x00, sizeof(szQuery));
			//20100824 - no.575
			sprintf(szQuery, " update  zangsi.T_CONTENTS_VIR_ID2 "
			                 " set del_yn = 'Y' "
			                 " where id = %lu "
			                 ,cont_id
			                 );
			 #ifdef __DEBUG
			 printf("%s\n\n",szQuery);
			 #endif 
			 	                 
			if (mysql_query(con, szQuery)){
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "daem5002_del_end_date: UPDATE T_CONTENTS_INFO error...\n");
			    goto daem5002_del_end_date_err;
		    }
		
			memset (szQuery, 0x00, sizeof(szQuery));
			//20100824 - no.575
			sprintf(szQuery, " update zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_VIR_ID b "
							 " set a.disp_stat = 'N', a.del_yn = 'Y', b.del_yn = 'Y' "
			                 " where a.id = b.id and a.disp_stat = 'Y' and a.disp_end_date <= '%s' and a.del_yn = 'N' and a.id = %lu "
			                 ,gdis_date,cont_id
			                 );
			 #ifdef __DEBUG
			 printf("%s\n\n",szQuery);
			 #endif 
			 	                 
			if (mysql_query(con, szQuery)){
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "daem5002_del_end_date: UPDATE T_CONTENTS_INFO error...\n");
			    goto daem5002_del_end_date_err;
		    }
			cont_id = 0;
			memset(szRegUser,0x00,sizeof(szRegUser));	
			memset(sect_code,0x00,sizeof(sect_code));
		
		}
			
		mysql_free_result(best_res);
	}

	//--------------------------------------------------------------------------
	// 전체 컨텐츠 갯수 업데이트 -- 2009/10/21
	//--------------------------------------------------------------------------
	memset(szQuery, 0x00, sizeof(szQuery));
	strcpy(szQuery, " select count(a.id)  "
					" from zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_FILE b "
					" where a.id = b.id ");
					
	if(mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daem5002_del_end_date: select contents cnt error...\n");
	    goto daem5002_del_end_date_err;
	}	
	if(!(res = mysql_store_result(con)))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daem5002_del_end_date: select contents cnt result error...\n");
	    goto daem5002_del_end_date_err;
	}
	if(mysql_num_rows(res)==0)
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daem5002_del_end_date: select contents cnt num_rows error...\n");
	    mysql_free_result(res);	
	    goto daem5002_del_end_date_err;
	}
	row = mysql_fetch_row(res);
	
	nCnt = getint(row,0);
		
	mysql_free_result(res);	
	
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " update zangsi.T_MINOR_CODE set minor_name = '%d' where major_code = '999' and minor_code = '99' "
					 , nCnt);
				
	if(mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daem5002_del_end_date: update contents cnt error...\n");
	    goto daem5002_del_end_date_err;
	}	


	if (tran_commit(con)!=0)
	{
	    ZzLOG(ERROR, "daem5002_del_end_date: tran_commit error...\n");
	    goto daem5002_del_end_date_err;
	}

	//--------------------------------------------------------------------------
	// 트렌젝션종료
	//--------------------------------------------------------------------------
	if (tran_end(con)!=0)	{
	    ZzLOG(ERROR, "daem5002_del_end_date: tran_end 테이베이스 오류입니다.\n");  
	    goto daem5002_del_end_date_err;
	}

	return 0;

daem5002_del_end_date_err:
	ZzLOG(ERROR, "daem5002_del_end_date: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	tran_rollback(con);
	tran_end(con);
	return -1;	
}



//******************************************************************************
//* daem5002_flog_data_del_end_date()
//* - 처리일자 -1의 게시종료일자를 삭제처리한다.
//******************************************************************************

int daem5002_flog_data_del_end_date()
{
	char szQuery[100000];		// query string

	//--------------------------------------------------------------------------
	// 트렌젝션시작
	//--------------------------------------------------------------------------

	//--------------------------------------------------------------------------
	// 각 해당 필로그 자료실의 사용자 별 용량을 업데이트 ( T_PERM_UPLOAD_AUTH )
	//--------------------------------------------------------------------------
	
	char szRegUser[24];
	memset(szRegUser,0x00,sizeof(szRegUser));
	
	double dFileSize = 0;
	long nUserCnt = 0;
	
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT a.reg_user,sum(b.file_size)  "
					 "  FROM zangsi.T_CONTFLOG_INFO a     "
					 "     , zangsi.T_CONTFLOG_FILE b     "
					 " WHERE a.disp_stat     = 'Y'        "
					 "    AND a.disp_end_date <= '%s'      "
					 "    AND a.del_yn        = 'N'        "
					 "    AND a.id            = b.id       "
					 " group by a.reg_user                 "
					 ,gdis_date );
	
	 #ifdef __DEBUG   
	 printf("필로그 자료실 확인 :  [ %s ]\n\n",szQuery);
	 #endif
	
	ZzLOG(ALWAY, "%s\n\n",szQuery);
	if (mysql_query(con, szQuery) )
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daem5002_del_end_date: UPDATE T_CONTENTS_INFO error...\n");
	    ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
		//if (mysql_errno(con) != 1062)
    }


	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "sysdate: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
	nUserCnt = mysql_num_rows(res);
 	if (nUserCnt==0)
 	{
	    ZzLOG(ERROR, "sysdate: 업데이트 할 사용자가 없습니다.\n");
		goto daem5002_del_end_date_err;
		return -1;
	}
	ZzLOG(ALWAY, "sysdate: 업데이트 할 사용자가 %d 명 있습니다.\n",nUserCnt);
	
	while((row = mysql_fetch_row(res))) 
	{
		ZzLOG(ALWAY, "sysdate: 업데이트 할 사용자 %d 명 남았습니다.\n",--nUserCnt);
		if (tran_begin(con)!=0) 
		{
		    ZzLOG(ERROR, "tran_begin: 테이베이스 오류입니다.\n");
		    goto daem5002_del_end_date_err;
		}
		
		memset(szRegUser,0x00,sizeof(szRegUser));
		dFileSize = 0;
		
		strcpy(szRegUser,   getstr(row, 0));
		dFileSize = (double)getnum(row, 1);
		
		//--------------------------------------------------------------------------
		//사용자 용량 업데이트
		//--------------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "UPDATE zangsi.T_PERM_UPLOAD_AUTH			"
		                 "   SET disk_use    = disk_use    - %15.0f "
	                 	 " WHERE user_id   = '%s'                   " 
	                 	 ,dFileSize
		                 ,szRegUser
		                 );
		                 
		 #ifdef __DEBUG
		 printf("%s\n\n",szQuery);
		 #endif 
		 
		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
		    ZzLOG(ERROR, "daem5002_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		    goto daem5002_del_end_date_tran_err;
			//if (mysql_errno(con) != 1062)
	    }
	   	 ZzLOG(ALWAY, "%s\n\n",szQuery);         

		//--------------------------------------------------------------------------
		// T_CONTENTS_ADMDEL insert
		//--------------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_ADMDEL"
		                 "     ( cont_gu  , id                "
		                 "     , del_desc , reg_user          "
		                 "     , reg_date , reg_time)         "
		                 "SELECT 'FD'     , id                "
		                 "     , '게시기간 만료' , 'system'    "
		                 "     , '%s'     ,'%s'               "
		                 "  FROM zangsi.T_CONTFLOG_INFO       "
		                 " WHERE disp_stat     = 'Y'          "
		                 "   AND disp_end_date <= '%s'         "
		                 "   AND del_yn        = 'N'          " 
		                 "   AND reg_user      = '%s'         " 
		                 " group by id " 
		                 ,gsys_date
		                 ,gsys_time
		                 ,gdis_date
		                 ,szRegUser
		                 );

		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
		    ZzLOG(ERROR, "daem5002_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		    //goto daem5002_del_end_date_tran_err;
			//if (mysql_errno(con) != 1062)
	    }

		//----------------------------------------------------------------------
		// 이미지 정보 추가
		//----------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery));

		char szAddQuery[10000];
		memset (szAddQuery, 0x00, sizeof(szAddQuery));
		strcpy( szAddQuery, "insert into zangsi.T_IMG_DEL  "
		                "        ( cont_gu, seq_no, filog_cn  "
		                " 		   , img01, img02, img03, img04, img05 "
		                "		   , file_path1, file_path2, file_name, reg_user, reg_date, reg_time  )"
		                 " SELECT 'FD'     , a.img_no , 0 ,'N','N','N','N','N'         "
		                 " ,'/zangsi/project/zangsi/files_zangsi/contents/img',a.img_spath,a.img_sname,a.reg_user,date_format(curdate(), '%Y%m%d'), date_format(now(), '%H%i%s') "
		                 "  FROM zangsi.T_CONTFLOG_IMG  a , zangsi.T_CONTFLOG_INFO b     "
		                 " WHERE a.cont_gu = 'FD' and a.reg_user = b.reg_user and a.id = b.id  and b.disp_stat = 'Y'  and b.del_yn = 'N' and a.img_no > 0  " 
		                 );
		sprintf(szQuery, "%s and b.disp_end_date <= '%s' ",szAddQuery, gdis_date);
		ZzLOG(ERROR, " Query [ %s ] \n",szQuery);
		
		
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5002_delete_data: INSERT T_IMG_DEL error...\n");
			ZzLOG(ERROR, "daem5002_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			//goto daem5002_del_end_date_err;
	    }

		MYSQL_RES *res_local;
		memset (szQuery, 0x00, sizeof(szQuery));
		memset (szAddQuery, 0x00, sizeof(szAddQuery));

		strcpy(szAddQuery, "select id ,reg_user from zangsi.T_CONTFLOG_INFO where disp_stat='Y' and del_yn='N'  ");
		sprintf(szQuery, "%s and disp_end_date <= '%s' ", szAddQuery , gdis_date );
		ZzLOG(ALWAY, " Query [ %s ] \n",szQuery);
		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "daem5002_delete_data: delete T_CONTFLOG_INFO error...\n");
			ZzLOG(ERROR, "daem5002_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			goto daem5002_del_end_date_err;
	  }	
	
		if(!(res_local = mysql_store_result(con)))
		{
			ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
		  ZzLOG(ERROR, "daem5002_del_end_date: select contents cnt result error...\n");
		  goto daem5002_del_end_date_err;
		}
		if(mysql_num_rows(res_local)==0)
		{
			ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
		    ZzLOG(ERROR, "daem5002_del_end_date: select contents cnt num_rows error...\n");
		    mysql_free_result(res_local);	
		    goto daem5002_del_end_date_err;
		}
		unsigned long id = 0;
		char szRegUser[24];
		memset(szRegUser,0x00,sizeof(szRegUser));
		
		while( row = mysql_fetch_row(res_local))
		{
			id= 0;
			memset(szRegUser,0x00,sizeof(szRegUser));
			
			id = (unsigned long)getnum(row, 0);
			strcpy(szRegUser,getstr(row,1));
			
			if( id > 0 && strlen(szRegUser) > 0 )
			{
				memset(szQuery,0x00,sizeof(szQuery));
				sprintf(szQuery,"delete from zangsi.T_CONTFLOG_IMG where cont_gu='FD' and reg_user = '%s' and id = %lu",szRegUser,id );
	
				if (mysql_query(con, szQuery))
				{
				    ZzLOG(ERROR, "daem5002_delete_data: delete T_CONTFLOG_IMG error...\n");
					ZzLOG(ERROR, "daem5002_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
					mysql_free_result(res_local);	
					goto daem5002_del_end_date_err;
			  }
			    			
			}
			
		}
		
		mysql_free_result(res_local);	
/*		    
		memset (szQuery, 0x00, sizeof(szQuery));
		memset (szAddQuery, 0x00, sizeof(szAddQuery));

		
		strcpy(szAddQuery, " delete zangsi.T_CONTFLOG_IMG from "
						 " zangsi.T_CONTFLOG_IMG a ,zangsi.T_CONTFLOG_INFO b where "
						 " a.cont_gu = 'FD' and a.reg_user = b.reg_user and a.id = b.id  and b.disp_stat = 'Y'  and b.del_yn = 'N' "
						 );
		sprintf(szQuery, "%s and b.disp_end_date <= '%s' ", szAddQuery , gdis_date );
		
		ZzLOG(ALWAY, " Query [ %s ] \n",szQuery);
		
		
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5002_delete_data: delete T_CONTENTS_IMG error...\n");
			ZzLOG(ERROR, "daem5002_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			goto daem5002_del_end_date_err;
	    }
*/
		//--------------------------------------------------------------------------
		// T_CONTENTS_DEL insert
		//--------------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_DEL "
		                 "     ( cont_gu                    "
		                 "     , id           , folder_yn   "
		                 "     , server_id    , del_date    "
		                 "     , file_path    , file_name1  "
		                 "     , file_name2   , file_size   "
		                 "     , file_type    , reg_user    "
		                 "     , reg_date     , reg_time)   "
		                 "SELECT 'FD'                       "
		                 "     , a.id         , b.folder_yn "
		                 "     , b.server_id  , '%s'        "
		                 "     , b.file_path  , b.file_name1"
		                 "     , b.file_name2 , b.file_size "
		                 "     , b.file_type  , b.reg_user  "
		                 "     , b.reg_date   , b.reg_time  "
		                 "  FROM zangsi.T_CONTFLOG_INFO a   "
		                 "     , zangsi.T_CONTFLOG_FILE b   "
		                 " WHERE a.disp_stat     = 'Y'      "
		                 "   AND a.disp_end_date <= '%s'     "
		                 "   AND a.del_yn        = 'N'      "
		                 "   AND a.id            = b.id     "
 						 "  AND a.reg_user        = '%s'    "
		                 ,gdel_date
		                 ,gdis_date
		                 ,szRegUser
		                 );
		 #ifdef __DEBUG
		 printf("%s\n\n",szQuery);
		 #endif 
		 ZzLOG(ALWAY, " Query [ %s ] \n",szQuery);	                 
		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
		    ZzLOG(ERROR, "daem5002_del_end_date: INSERT T_CONTENTS_DEL error...\n");
		    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
	    	goto daem5002_del_end_date_tran_err;
	    }
	
		//--------------------------------------------------------------------------
		// 게시기간 종료처리
		//--------------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "UPDATE zangsi.T_CONTFLOG_INFO"
		                 "   SET disp_stat     = 'N'   "
		                 "     , del_yn        = 'Y'   "
		                 " WHERE disp_stat     = 'Y'   "
		                 "   AND disp_end_date <= '%s'  "
		                 "   AND del_yn        = 'N'   "
		                 "   AND reg_user        = '%s'    "
		                 ,gdis_date
		                 ,szRegUser
		                 );
		 #ifdef __DEBUG
		 printf("%s\n\n",szQuery);
		 #endif 
		 ZzLOG(ALWAY, " Query [ %s ] \n",szQuery);	 	                 
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5002_del_end_date: UPDATE T_CONTENTS_INFO error...\n");
		    goto daem5002_del_end_date_tran_err;
	    }
		
		ZzLOG(ALWAY, " Query [ %s ] \n","tran_commit");	 	                 
		if (tran_commit(con)!=0)
		{
		    ZzLOG(ERROR, "daem5002_del_end_date: tran_commit error...\n");
		    goto daem5002_del_end_date_tran_err;
		}
	
		//--------------------------------------------------------------------------
		// 트렌젝션종료
		//--------------------------------------------------------------------------
		ZzLOG(ALWAY, " Query [ %s ] \n","tran_end");	 	                 
		if (tran_end(con)!=0)	
		{
		    ZzLOG(ERROR, "daem5002_del_end_date: tran_end 테이베이스 오류입니다.\n");  
		    goto daem5002_del_end_date_tran_err;
		}		
	}

	mysql_free_result(res);


	return 0;

daem5002_del_end_date_err:
	mysql_free_result(res);
	ZzLOG(ERROR, "daem5002_del_end_date: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	return -1;	

daem5002_del_end_date_tran_err:
	mysql_free_result(res);
	ZzLOG(ERROR, "daem5002_del_end_date: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	tran_rollback(con);
	tran_end(con);
	return -1;
}
//******************************************************************************
//* daem5002_set_cpr_date()
//* 제휴컨텐츠의 기간을 고정한다.
//******************************************************************************
int daem5002_set_cpr_date()
{
    ZzLOG(ALWAY, "daem5002_set_cpr_date start!!\n");
	
	char szQuery[10000];		// query string
	char szAddQuery[10000];		// query string	
	
	unsigned long ulVirID = 0;
	unsigned long ulID = 0;

	char szSectCode[2+1];
	memset(szSectCode, 0x00, sizeof(szSectCode));

	char szSectSub[2+1];
	memset(szSectSub, 0x00, sizeof(szSectSub));
	
	char szAdultYn[1+1];
	memset(szAdultYn, 0x00, sizeof(szAdultYn));

	char szCopyrightYn[1+1];
	memset(szCopyrightYn, 0x00, sizeof(szCopyrightYn));

	char szUserID[12+1];
	memset(szUserID, 0x00, sizeof(szUserID));

	char szTitle[2000];
	memset(szTitle, 0x00, sizeof(szTitle));

	MYSQL_RES *res2;
	MYSQL_ROW  row2;

	while(1)
	{
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " select a.vir_id, b.id, b.sect_code, b.sect_sub, b.adult_yn, a.copyright_yn, b.reg_user, b.title "
						 " from zangsi.T_CONTENTS_VIR_ID a, zangsi.T_CONTENTS_INFO b "
						 " where a.id = b.id and a.copyright_yn in('C', 'H') and vir_id > %lu "
						 " and b.sect_code in('12','15') and b.del_yn = 'N' "
						 " and b.disp_end_date <= date_format(now(),'%%Y%%m%%d') "
						 " order by a.id "
						 " limit 100 ", ulVirID);
		
		ZzLOG(ALWAY, "daem5002_set_cpr_date : [ szQuery : %s ]\n",szQuery);
		
		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
		    ZzLOG(ERROR, "daem5002_set_cpr_date: UPDATE T_CONTENTS_INFO error...\n");
		    goto daem5002_set_cpr_date_err;
	    }

		if (!(res = mysql_store_result(con))) 
		{
			ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
		    ZzLOG(ERROR, "daem5002_set_cpr_date: UPDATE T_CONTENTS_INFO error...\n");
		    goto daem5002_set_cpr_date_err;
		}

	 	if (mysql_num_rows(res)==0)	
	 	{
			mysql_free_result(res);
		    ZzLOG(ALWAY, "daem5002_set_cpr_date: 처리할 자료가 없습니다.\n");
			return 0;
		}
		
		while((row = mysql_fetch_row(res))) 
		{
			ulVirID = (unsigned long)getnum(row, 0);
			ulID = (unsigned long)getnum(row, 1);

			memset(szSectCode, 0x00, sizeof(szSectCode));
			strcpy(szSectCode, getstr(row, 2));

			memset(szSectSub, 0x00, sizeof(szSectSub));
			strcpy(szSectSub, getstr(row, 3));

			memset(szAdultYn, 0x00, sizeof(szAdultYn));
			strcpy(szAdultYn, getstr(row, 4));

			memset(szCopyrightYn, 0x00, sizeof(szCopyrightYn));
			strcpy(szCopyrightYn, getstr(row, 5));

			memset(szUserID, 0x00, sizeof(szUserID));
			strcpy(szUserID, getstr(row, 6));

			memset(szTitle, 0x00, sizeof(szTitle));
			strcpy(szTitle, getstr(row, 7));
			
			
/*
			//영화, 드라마, 동영상 카테고리의 제휴컨텐츠는 기간을 90일로 고정.
			if(strcmp(szSectCode, "01") ==0 || strcmp(szSectCode, "02") ==0 || strcmp(szSectCode, "03") ==0)
			{	
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " update zangsi.T_CONTENTS_INFO  "
								 " set disp_end_date = date_format(date_add(now(), INTERVAL 90 DAY),'%Y%%m%%d')  "
								 " where id = %d and del_yn = 'N'  "
								 , ulID, szSectCode, szSectSub, szAdultYn);
			} 
			//성인, 동영상 성인 칸테고리의 제휴컨텐츠는 기간을 30일로 고정.
			else if(strcmp(szSectCode, "11") ==0 || (strcmp(szSectCode, "03") ==0 && strcmp(szSectSub, "07") ==0))
			{
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " update zangsi.T_CONTENTS_INFO  "
								 " set disp_end_date = date_format(date_add(now(), INTERVAL 30 DAY),'%Y%%m%%d')  "
								 " where id = %d and del_yn = 'N' "
								 , ulID, szSectCode, szSectSub, szAdultYn);
			}
			//영화, 드라마, 동영상, 동영상 성인, 성인을 제외한 제휴컨텐츠는 가간을 360일로 고정.
			else*/
			{
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " update zangsi.T_CONTENTS_INFO  "
								 " set disp_end_date = date_format(date_add(now(), INTERVAL 45 DAY),'%Y%%m%%d')  "
								 " where id = %lu and del_yn = 'N' "
								 , ulID);
			}
			
			//ZzLOG(ALWAY, "Query [ %s ]\n",szQuery); //test
			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "daem5002_set_cpr_date: UPDATE T_CONTENTS_INFO error...\n");
			    goto daem5002_set_cpr_date_err;
		    }
		    
		    //쪽지 발송
		    
			char szMemoInfo[5000];
			memset(szMemoInfo,0x00,sizeof(szMemoInfo));
			ReplaceSingleToDouble(szTitle);

			sprintf(szMemoInfo, " 안녕하세요. 위디스크입니다.\r\n\r\n"
								"%s님께서 등록해주신 (%lu : %s)컨텐츠는 위디스크의 우수컨텐츠로 선정되었습니다.\r\n"
								"항상 위디스크를 위해 우수한 컨텐츠 등록으로 많은 회원님들에게 좋은 정보를 제공하여 주신 점 \r\n"
								"감사드립니다.\r\n\r\n"
								"우수컨텐츠로 선정된 %s님의 컨텐츠는 더 많은 회원님들에게 좋은 정보를 제공할수있도록 운영팀에서 자동으로 컨텐츠 게시기간이 45일 연장해드렸습니다.\r\n\r\n"
								"앞으로도 %s님의 보다 많은 애정과 관심을 부탁드립니다.\r\n\r\n"
								"감사합니다."
								, szUserID
								, ulID, szTitle
								, szUserID
								, szUserID);
/*
			sprintf(szQuery, " insert into zangsi.T_MEMO_INFO (  user_id, memo_cd, ref_id, descript  , send_user, recv_yn, recv_date, recv_time  ) " 
							" values (  '%s' , '01', 0, '%s ','운영팀' , 'N', '%s', '%s' ) "
							, szUserID,szMemoInfo, gsys_date, gsys_time);
			
			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
				ZzLOG(ERROR, "daem5002_set_cpr_date: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "daem5002_set_cpr_date: 쪽지 발송 실패 [%s][%lu]\n", szUserID, ulID);
			}
*/

			sprintf(szQuery, " insert into zangsi.T_SEND_MEMO (  memo_cd,  descript,send_user, del_yn, send_date, send_time ) " 
								" values (  '04' "
								
								" ,'%s' "
								
								" , '운영팀' ,'N', '%s', '%s' ) "
								, szMemoInfo, gsys_date, gsys_time);
		
			ZzLOG(ALWAY, "[%s]\n\n", szQuery);
			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
				ZzLOG(ERROR, "daem5002_set_cpr_date: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "daem5002_set_cpr_date: 쪽지 발송 실패 [%s][%lu]\n", szUserID, ulID);
			}
		
			memset(szQuery , 0x00, sizeof(szQuery ));
			strcpy( szQuery, "SELECT last_insert_id() as send_seq_no" );
			
			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
				ZzLOG(ERROR, "daem5002_set_cpr_date: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "daem5002_set_cpr_date: 쪽지 발송 실패 [%s][%lu]\n", szUserID, ulID);
			}
			
			MYSQL_RES* myres = mysql_store_result(con);
			MYSQL_ROW myrow = mysql_fetch_row(myres);
		
			double send_seq_no  = getnum(myrow,0 );	
			
			mysql_free_result(myres);
				
			
			memset(szQuery , 0x00, sizeof(szQuery ));
			sprintf(szQuery," insert into zangsi.T_RECV_MEMO "
							"  (memo_seq_no, recv_user, recv_date, recv_time, read_yn, del_yn, save_yn)  "
							" values "
							"  (%.0f, '%s' , '%s', '%s' , 'N', 'N', 'N') "
							,send_seq_no,szUserID,gsys_date, gsys_time);
							
			ZzLOG(ALWAY, "[%s]\n\n", szQuery);
			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "error Query [ %s ]\n",szQuery);
				ZzLOG(ERROR, "daem5002_set_cpr_date: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "daem5002_set_cpr_date: 쪽지 발송 실패 [%s][%lu]\n", szUserID, ulID);
			}
			
		    
		    
			//성인 제휴컨텐츠는 가간을 30일로 고정.
			/*
			if(strcmp(szAdultYn, "Y") ==0)
			{
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " update zangsi.T_CONTENTS_INFO  "
								 " set disp_end_date = date_format(date_add(now(), INTERVAL 30 DAY),'%Y%%m%%d')  "
								 " where id = %d and del_yn = 'N' "
								 , ulID, szSectCode, szSectSub, szAdultYn);
				if (mysql_query(con, szQuery))
				{
					ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
				    ZzLOG(ERROR, "daem5002_set_cpr_date: UPDATE T_CONTENTS_INFO error...\n");
				    goto daem5002_set_cpr_date_err;
			    }
			}
			*/	
		}
		mysql_free_result(res);
	}    

	return 0;

daem5002_set_cpr_date_err:
	ZzLOG(ERROR, "daem5002_set_cpr_date: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	mysql_free_result(res);
	return -1;	
}

/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5002_get_sysdate()
{
	char szQuery[1000];		// query string
	char sztemp [100];      // query temp

	memset(szQuery, 0x00, sizeof(szQuery));
	if (strcmp(gproc_date, "00000000")==0) {
		strcpy(szQuery, "SELECT date_format(date_add(now(), INTERVAL -1 DAY),'%Y%m%d')");
//		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL  4 DAY),'%Y%m%d')");   // 2014.03.13    modify y leesh..
//		- 2014-03-12 오전 9:46
//		목록 에서 '제휴 컨텐츠 표시 안됨' '유일'
//		삭제 예정일 최초 +7일로 수정 C데몬
		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL  7 DAY),'%Y%m%d')");
		strcat(szQuery, "     , date_format(now(),'%Y%m%d')");
		strcat(szQuery, "     , date_format(now(),'%Y%m%d')");
		strcat(szQuery, "     , date_format(now(),'%H%i%s')");
	}
	else
	{
		sprintf(szQuery, "SELECT date_format(date_add('");
		strcat (szQuery, gproc_date);
		strcat (szQuery, "', INTERVAL -1 DAY),'%Y%m%d')");
		strcat (szQuery, " , date_format(date_add('");
		strcat (szQuery, gproc_date);
		strcat (szQuery, "', INTERVAL 4 DAY),'%Y%m%d')");
		strcat (szQuery, " , '");
		strcat (szQuery, gproc_date);
		strcat (szQuery, "', date_format(now(),'%Y%m%d')");
		strcat (szQuery, " , date_format(now(),'%H%i%s')");
	}

	 #ifdef __DEBUG
	 printf("%s\n\n",szQuery);
	 #endif 	

	if (mysql_query(con, szQuery)){
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
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
	memset(gdel_date , 0x00, sizeof(gdel_date ));
	memset(gdis_date , 0x00, sizeof(gdis_date ));
	memset(gsys_date , 0x00, sizeof(gsys_date ));
	memset(gsys_time , 0x00, sizeof(gsys_time ));

	strcpy(gproc_date,   getstr(row, 0));
	strcpy(gdel_date ,   getstr(row, 1));
	strcpy(gdis_date ,   getstr(row, 2));
	strcpy(gsys_date ,   getstr(row, 3));
	strcpy(gsys_time ,   getstr(row, 4));
	
	mysql_free_result(res);

	return 0;
}

/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5002_init_process(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("d_", "/logs/daemon"); 

    ZzLOG(ALWAY, "[daem5002]*****************프로그램 시작*****************\n");  

    // 파라미터 값 설정 및 초기화
    if (argc != 2){
    	goto arg_error;
    }

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	
	//if (!(con=db_connect_backup("zangsi")))
	
	if (!(con_bck=db_connect_backup("zangsi")))
	{
		ZzLOG(ERROR, "백업 DB에 접속하지 못 하였습니다...\n");
	   	return(-1); 		
	}


	if (!(con=db_connect("zangsi")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con_bck);
	   	return(-1); 
	}
	
	//hcs : 2009-11-17. 제휴 우수 컨텐츠 선정및 기간연장을 위한 제휴 디비 연결
	if (!(con_cpr=db_connect_cprdb("zangsi_cpr")))
	{
		ZzLOG(ERROR, "cpr DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con_bck);
		db_disconnect(con);
		db_disconnect(con_cpr);
	   	return(-1); 
	}
	
	//no.880
	/*
	if (!(con_om=db_connect_omdb("zangsi_op")))
	{
		ZzLOG(ERROR, "om DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con_bck);
		db_disconnect(con);
		db_disconnect(con_cpr);
		db_disconnect(con_om);
	   	return(-1); 
	}
	
	*/
	/* 처리일자 */
	memset(gproc_date, 0x00, sizeof(gproc_date));
	strcpy(gproc_date, argv[1]);
	ret=daem5002_get_sysdate();
	if (ret < 0){
		db_disconnect(con_bck);
		db_disconnect(con);
		db_disconnect(con_cpr);
		//db_disconnect(con_om);
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
int daem5002_term_process()
{
    // DB close
	db_disconnect(con_bck);
	db_disconnect(con);
	db_disconnect(con_cpr);
	//db_disconnect(con_om);
    ZzLOG(ALWAY, "[daem5002]*****************프로그램 종료*****************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5002_signal(int nSignal)
{
    daem5002_term_process();
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
	signal(SIGTERM, daem5002_signal);
	signal(SIGINT,  daem5002_signal);
	signal(SIGQUIT, daem5002_signal);
	signal(SIGKILL, daem5002_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daem5002_init_process(argc, argv) == 0 ) 
	{
		/* 프로그램 메인루틴 */
		rc = daem5002_main_process();
		/* 프로그램 종료루틴 */                    
		daem5002_term_process();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
