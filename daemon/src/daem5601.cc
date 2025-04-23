/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5601.cc
 *         기능 : 하루치 다운로드 내역을 백업
 *         설명 : 1. 프로세스 처리 예정시간 : 05:00
 *     설치위치 : 관리자DB에 위치
 *
 *       작성자 : HCS
 *       작성일 : 2008/04/05
 *     수정이력 :

그외 7일
모바일 일반 8일

 down_st_date = '2' 모바일 '1'PC
 SECT_CODE 11
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
#define	 NUMBER		10
#define	 DATE		11

int daem5601_process();
int daem5601_process_db();
int daem5601_process_db_mobile();
int daem5601_process_init(int argc, char **argv);
int daem5601_process_term();
void daem5601_signal(int nSignal);


MYSQL     *con;
MYSQL_RES *res;
MYSQL_ROW  row;

MYSQL     *con_backup;


bool gbIsUserDate      ; //날짜입력
char gst_date[8+1];	// 처리일자
char gst_date_8[8+1];	// 처리일자
char gst_time[6+1];	// 처리시작시간
char ged_time[6+1];	// 처리마지막시간
char gproc_log[80]; // 로그메시지버퍼
int gnType = 0;


unsigned long gl_start_no = 0;
unsigned long gl_stop_no = 0;

void ReplaceSingleToDouble2(char* pString)
{
        int cTemp;
        int nFileLen = strlen(pString);

          int cReplace = '"';
        int cSingle = '\'';
        int cFind = '\\';
        int cBlank = ' ';


        for(int i=0; i<nFileLen ; i++)
        {
                cTemp = pString[i];
                if( cTemp == cSingle ) // 96 is '
                {
                    pString[i] = (char)cReplace;
                }
                else if( cTemp == cFind )
            	{
            		pString[i] = (char)cBlank;
            	}
        }
}

//******************************************************************************
//* daem5601 main
//******************************************************************************
int daem5601_process()
{
	char szQuery[1000];  // query string
	memset(szQuery, 0x00, sizeof(szQuery));

	// 처리일자가 "00000000"일때는 시스템일자 -1을 한다.
	if (strcmp(gst_date, "00000000") == 0)
	{
		gbIsUserDate = false;
		strcpy(szQuery, "SELECT date_format(date_add(now(), INTERVAL - 7 DAY),'%Y%m%d'),date_format(date_add(now(), INTERVAL - 7 DAY),'%Y%m%d')");
		
		//strcpy(szQuery, "SELECT date_format(date_add(now(), INTERVAL - 8 DAY),'%Y%m%d') as 7day ,date_format(date_add(now(), INTERVAL - 15 DAY),'%Y%m%d') as 14day; /*daem5601*/ ");
		// 현재로부터 4일 이전 날짜를 얻는다.

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
			return -1;
		}
		row = mysql_fetch_row(res);
		memset(gst_date, 0x00, sizeof(gst_date));
		memset(gst_date_8, 0x00, sizeof(gst_date_8));

		strcpy(gst_date,   getstr(row, 0));
		strcpy(gst_date_8,   getstr(row, 1));

		mysql_free_result(res);

	}
	else
	{
		gbIsUserDate=true;

	}

	#ifdef __DEBUG
	printf("적용 날짜 : %s\n",gst_date);
	#endif

	if( daem5601_process_db() !=0)
	{
		return -1;
	}

	if( daem5601_process_db_mobile() !=0)
	{
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

//******************************************************************************
//* daem5601 db 처리로직
//******************************************************************************
int daem5601_process_db_mobile()
{
	char szQuery[1600];		// query string
	char szdelQuery[1600];
	int  ret;

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/

    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "gproc_date : [%s]\n", gst_date);
    ZzLOG(ALWAY, "gproc_date_8 : [%s]\n", gst_date_8);
    ZzLOG(ALWAY, "---------------------------------------------------------\n");

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/
	char szBuffer[600];
	unsigned long dwCount = 0;
	unsigned long lFirstNo = 0;

	//20120516
	while(1)
	{
		if( gbIsUserDate ) //하루
		{
			sprintf(szQuery," SELECT a.deal_no,a.cont_gu,a.id,a.deal_date,a.deal_time,a.sale_user,a.buy_user,a.link_user  "
							" ,a.title,a.sect_code,a.share_meth,a.price_amt,a.won_mega,a.sale_amt,a.comp_amt,a.file_size  "
							" ,a.down_st_date,a.down_st_time,a.down_yn,a.del_yn,a.adult_yn,a.fixamt_yn,a.coupon_code ,IFNULL(b.value1, '00000000000000') "
							" FROM zangsi.T_DEAL_INFO a, zangsi.T_DEAL_IP b "
							" WHERE a.deal_date = '%s' AND a.sect_code != '11' AND a.down_st_date = '2' AND a.deal_no = b.deal_no AND IFNULL(b.value1, '00000000000000') <= DATE_FORMAT(NOW(), '%%Y%%m%%d%%H%%i%%s') "
							" ORDER BY a.deal_date LIMIT 100 ; /*daem5601*/  "
	                        , gst_date_8
	                        );
		}
		else
		{
			sprintf(szQuery," SELECT a.deal_no,a.cont_gu,a.id,a.deal_date,a.deal_time,a.sale_user,a.buy_user,a.link_user  "
							" ,a.title,a.sect_code,a.share_meth,a.price_amt,a.won_mega,a.sale_amt,a.comp_amt,a.file_size  "
							" ,a.down_st_date,a.down_st_time,a.down_yn,a.del_yn,a.adult_yn,a.fixamt_yn,a.coupon_code ,IFNULL(b.value1, '00000000000000') "
							" FROM zangsi.T_DEAL_INFO a, zangsi.T_DEAL_IP b "
							" WHERE a.deal_date <= '%s' AND a.sect_code != '11' AND a.down_st_date = '2' AND a.deal_no = b.deal_no AND IFNULL(b.value1, '00000000000000') <= DATE_FORMAT(NOW(), '%%Y%%m%%d%%H%%i%%s') "
							" ORDER BY a.deal_date LIMIT 100 ; /*daem5601*/  "
	                        , gst_date_8
	                        );

		}

		#ifdef __DEBUG
		//printf("%s\n\n",szQuery);
		#endif

	    //ZzLOG(ALWAY, "szQuery = %s\n",szQuery);

		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5601_process: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "daem5601_process: ( %s )\n", szQuery);
			return -1;
		}

		if (!(res = mysql_store_result(con)))
		{
		    ZzLOG(ERROR, "daem5601_process: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}

	 	if (mysql_num_rows(res)==0)
	 	{
			mysql_free_result(res);
		    ZzLOG(ALWAY, "daem5601_process: 처리할 자료가 없습니다.\n");
			break;
		}

		//--------------------------------------------------------------------------
		// 트렌젝션시작
		//--------------------------------------------------------------------------

		while((row = mysql_fetch_row(res)))
		{

			if (tran_begin(con)!=0)	{
			    ZzLOG(ERROR, "tran_begin1: 테이베이스 오류입니다.\n");
				return -1;
			}
//			ZzLOG(ALWAY, "#.");
			if (tran_begin(con_backup)!=0)	{
			    ZzLOG(ERROR, "tran_begin2: 테이베이스 오류입니다.\n");
				return -1;
			}

			char szTitle[4096];
			memset(szTitle,0x00,sizeof(szTitle));
			strcpy(szTitle,getstr(row,8));
			ReplaceSingleToDouble2(szTitle);

			if( (strcmp(getstr(row,21),"C" )== 0) || (strcmp(getstr(row,21),"0" )== 0))
			{
				sprintf(szQuery, " INSERT INTO zangsi_sum.T_DEAL_INFO "
								" ( deal_no,cont_gu,id,deal_date,deal_time,sale_user,buy_user,link_user "
								" ,title,sect_code,share_meth,price_amt,won_mega,sale_amt,comp_amt,file_size "
								" ,down_st_date,down_st_time,down_yn,del_yn,adult_yn,fixamt_yn,coupon_code ) "
								 " VALUES ( "
								 " %ld ,'%s' ,%ld ,'%s' ,'%s' ,'%s' ,'%s','%s' "
								 " ,'%s','%s','%s',%d,%d,%d,%d,%.0f "
								 " ,'%s','%s','%s','%s','%s','%s','%s' ) ; /*daem5601*/ "
								  , (unsigned long)getnum(row, 0)  ,getstr(row,1) ,(unsigned long)getnum(row, 2)  ,getstr(row,3) ,getstr(row,4) ,getstr(row,5) ,getstr(row,6),getstr(row,7)
								  ,szTitle,getstr(row,9),getstr(row,10),(int)getnum(row, 11),(int)getnum(row, 12),(int)getnum(row, 13),(int)getnum(row, 14),(double)getnum(row, 15)
								  ,getstr(row,16),getstr(row,17),getstr(row,18),getstr(row,19),getstr(row,20),getstr(row,21),getstr(row,22)
								  );
				#ifdef _DEBUG
//				printf("\n\n %s \n\n", szQuery);
				#endif

				//ZzLOG(ALWAY, "szQuery = %s\n",szQuery);

				if (mysql_query(con_backup, szQuery))
				{
				    ZzLOG(ERROR, "daem5601_delete_data: INSERT T_DEAL_INFO error...\n");
					ZzLOG(ERROR, "daem5601_delete_data: [%d](%s)\n",mysql_errno(con_backup), mysql_error(con_backup));
					ZzLOG(ERROR, "%s\n\n",szQuery);
					goto daem5601_process_db_err;
			    }
			}


			if( (strcmp(getstr(row,21),"C" )== 0) || (strcmp(getstr(row,21),"0" )== 0))
			{
				sprintf(szdelQuery, " INSERT INTO zangsi_sum.T_DEAL_IP (deal_no,buy_user,ip_addr,deal_date,deal_time,service_gu,cnt,sect_sub,mobile_chk,option1,value1) "
							" SELECT deal_no,buy_user,ip_addr,deal_date,deal_time,service_gu,cnt,sect_sub,mobile_chk,option1,value1 FROM zangsi.T_DEAL_IP where deal_no = %ld"
							,(unsigned long)getnum(row, 0));

				if (mysql_query(con_backup, szdelQuery))
				{
				    ZzLOG(ERROR, "daem5601_delete_data: INSERT T_DEAL_IP error...\n");
					ZzLOG(ERROR, "daem5601_delete_data: [%d](%s)\n",mysql_errno(con_backup), mysql_error(con_backup));
					ZzLOG(ERROR, "%s\n\n",szdelQuery);
					goto daem5601_process_db_err;
				}
			}
			/*
		    sprintf(szQuery, " INSERT INTO zangsi_sum.T_DOWN_INFO "
							" ( deal_no,cont_gu,deal_date,deal_time,sale_user,buy_user "
							" , sect_code,share_meth,price_amt,won_mega,sale_amt,comp_amt "
							" , down_yn,del_yn,adult_yn,fixamt_yn,coupon_code ) "
							 " VALUES ( "
							 " %ld ,'%s' ,'%s' ,'%s' ,'%s' ,'%s' "
							 " ,'%s','%s',%d,%d,%d,%d"
							 " ,'%s','%s','%s','%s','%s' ) "
							  , (unsigned long)getnum(row, 0)  ,getstr(row,1) ,getstr(row,3) ,getstr(row,4) ,getstr(row,5) ,getstr(row,6)
							  ,getstr(row,9),getstr(row,10),(int)getnum(row, 11),(int)getnum(row, 12),(int)getnum(row, 13),(int)getnum(row, 14)
							  ,getstr(row,18),getstr(row,19),getstr(row,20),getstr(row,21),getstr(row,22)
							  );
			#ifdef _DEBUG
			printf("\n\n %s \n\n", szQuery);
			#endif

			if (mysql_query(con_backup, szQuery))
			{
			    ZzLOG(ERROR, "daem5601_delete_data: INSERT T_DEAL_INFO error...\n");
				ZzLOG(ERROR, "daem5601_delete_data: [%d](%s)\n",mysql_errno(con_backup), mysql_error(con_backup));
				ZzLOG(ERROR, "%s\n\n",szQuery);
				goto daem5601_process_db_err;
		    }
		    */

			dwCount	++;


			memset (szQuery, 0x00, sizeof(szQuery));
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "DELETE FROM zangsi.T_DEAL_INFO "
							 "where deal_no = %ld ; /*daem5601*/ "
							 ,(unsigned long)getnum(row, 0));

			
			sprintf(szdelQuery, "DELETE FROM zangsi.T_DEAL_IP "
							 "where deal_no = %ld ; /*daem5601*/  "
							 ,(unsigned long)getnum(row, 0));

			//ZzLOG(ALWAY, "szQuery = %s\n",szQuery);
			//ZzLOG(ALWAY, "szQuery = %s\n",szQuery);

			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daem5601_delete_data: DELETE T_DEAL_INFO error...\n");
				ZzLOG(ERROR, "daem5601_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "%s\n",szQuery);
				goto daem5601_process_db_err;
		    }
			
			if (mysql_query(con,szdelQuery))
			{
				
			    ZzLOG(ERROR, "daem5601_delete_data: DELETE T_DEAL_IP error...\n");
				ZzLOG(ERROR, "daem5601_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "%s\n",szdelQuery);
				goto daem5601_process_db_err;
			}
			
			if (tran_commit(con_backup)!=0)
			{
			    ZzLOG(ERROR, "process_db2: tran_commit error...\n");
			    goto daem5601_process_db_err;
			}
			//--------------------------------------------------------------------------
			// 트렌젝션종료
			//--------------------------------------------------------------------------
			if (tran_end(con_backup)!=0)	{
			    ZzLOG(ERROR, "process_db2: tran_end 테이베이스 오류입니다.\n");
			    goto daem5601_process_db_err;
			}



			if (tran_commit(con)!=0)
			{
			    ZzLOG(ERROR, "process_db1: tran_commit error...\n");
			    goto daem5601_process_db_err;
			}
			//--------------------------------------------------------------------------
			// 트렌젝션종료
			//--------------------------------------------------------------------------
			if (tran_end(con)!=0)	{
			    ZzLOG(ERROR, "process_db1: tran_end 테이베이스 오류입니다.\n");
			    goto daem5601_process_db_err;
			}

		}
		mysql_free_result(res);
	}
	
	ZzLOG(ALWAY, "daem5601_process: 백업된 자료의 갯수는 %ld 입니다.\n",dwCount);
	return 0;

daem5601_process_db_err:
	ZzLOG(ERROR, "process_db1: [%d](%s)\n",mysql_errno(con), mysql_error(con));

	ZzLOG(ERROR, "process_db2: [%d](%s)\n",mysql_errno(con_backup), mysql_error(con_backup));

	tran_rollback(con_backup);
	tran_end(con_backup);

	tran_rollback(con);
	tran_end(con);

	mysql_free_result(res);
	ZzLOG(ALWAY, "daem5601_process: 백업된 자료의 갯수는 %ld 입니다.\n",dwCount);

	return -1;
}



//******************************************************************************
//* daem5601 db 처리로직
//******************************************************************************
int daem5601_process_db()
{
	char szQuery[1600];		// query string
	char szdelQuery[1600];
	int  ret;

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/

    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "gproc_date : [%s]\n", gst_date);
    ZzLOG(ALWAY, "gproc_date_8 : [%s]\n", gst_date_8);
    ZzLOG(ALWAY, "---------------------------------------------------------\n");

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/
	char szBuffer[600];
	unsigned long dwCount = 0;
	unsigned long lFirstNo = 0;

	//20120516
	while(1)
	{
		if( gbIsUserDate ) //하루
		{


			sprintf(szQuery," SELECT a.deal_no,a.cont_gu,a.id,a.deal_date,a.deal_time,a.sale_user,a.buy_user,a.link_user  "
							" ,a.title,a.sect_code,a.share_meth,a.price_amt,a.won_mega,a.sale_amt,a.comp_amt,a.file_size  "
							" ,a.down_st_date,a.down_st_time,a.down_yn,a.del_yn,a.adult_yn,a.fixamt_yn,a.coupon_code ,IFNULL(b.value1, '00000000000000') "
							" FROM zangsi.T_DEAL_INFO a, zangsi.T_DEAL_IP b "
							" WHERE a.deal_date = '%s' AND ( ( sect_code = '11' and down_st_date = '2' ) or down_st_date = '1' ) AND a.deal_no = b.deal_no AND IFNULL(b.value1, '00000000000000') <= DATE_FORMAT(NOW(), '%%Y%%m%%d%%H%%i%%s') "
							" ORDER BY a.deal_date LIMIT 100 ; /*daem5601*/  "
	                        , gst_date
	                        );
		}
		else
		{
			sprintf(szQuery," SELECT a.deal_no,a.cont_gu,a.id,a.deal_date,a.deal_time,a.sale_user,a.buy_user,a.link_user  "
							" ,a.title,a.sect_code,a.share_meth,a.price_amt,a.won_mega,a.sale_amt,a.comp_amt,a.file_size  "
							" ,a.down_st_date,a.down_st_time,a.down_yn,a.del_yn,a.adult_yn,a.fixamt_yn,a.coupon_code ,IFNULL(b.value1, '00000000000000') "
							" FROM zangsi.T_DEAL_INFO a, zangsi.T_DEAL_IP b "
							" WHERE a.deal_date <= '%s' AND ( ( sect_code = '11' and down_st_date = '2' ) or down_st_date = '1' ) AND a.deal_no = b.deal_no AND IFNULL(b.value1, '00000000000000') <= DATE_FORMAT(NOW(), '%%Y%%m%%d%%H%%i%%s') "
							" ORDER BY a.deal_date LIMIT 100 ; /*daem5601*/  "
	                        , gst_date
	                        );
		}

		#ifdef __DEBUG
		//printf("%s\n\n",szQuery);
		#endif

 	    //ZzLOG(ALWAY, "szQuery = %s\n",szQuery);

		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5601_process: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "daem5601_process: ( %s )\n", szQuery);
			return -1;
		}

		if (!(res = mysql_store_result(con)))
		{
		    ZzLOG(ERROR, "daem5601_process: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}

	 	if (mysql_num_rows(res)==0)
	 	{
			mysql_free_result(res);
		    ZzLOG(ALWAY, "daem5601_process: 처리할 자료가 없습니다.\n");
			break;
		}

		//--------------------------------------------------------------------------
		// 트렌젝션시작
		//--------------------------------------------------------------------------

//while
		while((row = mysql_fetch_row(res)))
		{

			if (tran_begin(con)!=0)	{
			    ZzLOG(ERROR, "tran_begin1: 테이베이스 오류입니다.\n");
				return -1;
			}
//			ZzLOG(ALWAY, "#.");

			if (tran_begin(con_backup)!=0)	{
			    ZzLOG(ERROR, "tran_begin2: 테이베이스 오류입니다.\n");
				return -1;
			}

			char szTitle[4096];
			memset(szTitle,0x00,sizeof(szTitle));
			strcpy(szTitle,getstr(row,8));
			ReplaceSingleToDouble2(szTitle);

			if( (strcmp(getstr(row,21),"C" )== 0) || (strcmp(getstr(row,21),"0" )== 0))
			//if( strcmp(getstr(row,21),"C" )== 0 )
			{
				sprintf(szQuery, " INSERT INTO zangsi_sum.T_DEAL_INFO "
								" ( deal_no,cont_gu,id,deal_date,deal_time,sale_user,buy_user,link_user "
								" ,title,sect_code,share_meth,price_amt,won_mega,sale_amt,comp_amt,file_size "
								" ,down_st_date,down_st_time,down_yn,del_yn,adult_yn,fixamt_yn,coupon_code ) "
								 " VALUES ( "
								 " %ld ,'%s' ,%ld ,'%s' ,'%s' ,'%s' ,'%s','%s' "
								 " ,'%s','%s','%s',%d,%d,%d,%d,%.0f "
								 " ,'%s','%s','%s','%s','%s','%s','%s' ) ; /*daem5601*/  "
								  , (unsigned long)getnum(row, 0)  ,getstr(row,1) ,(unsigned long)getnum(row, 2)  ,getstr(row,3) ,getstr(row,4) ,getstr(row,5) ,getstr(row,6),getstr(row,7)
								  ,szTitle,getstr(row,9),getstr(row,10),(int)getnum(row, 11),(int)getnum(row, 12),(int)getnum(row, 13),(int)getnum(row, 14),(double)getnum(row, 15)
								  ,getstr(row,16),getstr(row,17),getstr(row,18),getstr(row,19),getstr(row,20),getstr(row,21),getstr(row,22)
								  );
				#ifdef _DEBUG
				printf("\n\n %s \n\n", szQuery);
				#endif

 			    //ZzLOG(ALWAY, "szQuery = %s\n",szQuery);
				if (mysql_query(con_backup, szQuery))
				{
				    ZzLOG(ERROR, "daem5601_delete_data: INSERT T_DEAL_INFO error...\n");
					ZzLOG(ERROR, "daem5601_delete_data: [%d](%s)\n",mysql_errno(con_backup), mysql_error(con_backup));
					ZzLOG(ERROR, "%s\n\n",szQuery);
					goto daem5601_process_db_err;
			    }
			}

			if( (strcmp(getstr(row,21),"C" )== 0) || (strcmp(getstr(row,21),"0" )== 0))
			{
				sprintf(szdelQuery, " INSERT INTO zangsi_sum.T_DEAL_IP (deal_no,buy_user,ip_addr,deal_date,deal_time,service_gu,cnt,sect_sub,mobile_chk,option1,value1) "
							" SELECT deal_no,buy_user,ip_addr,deal_date,deal_time,service_gu,cnt,sect_sub,mobile_chk,option1,value1 FROM zangsi.T_DEAL_IP where deal_no = %ld"
							,(unsigned long)getnum(row, 0));

				if (mysql_query(con_backup, szdelQuery))
				{
				    ZzLOG(ERROR, "daem5601_delete_data: INSERT T_DEAL_IP error...\n");
					ZzLOG(ERROR, "daem5601_delete_data: [%d](%s)\n",mysql_errno(con_backup), mysql_error(con_backup));
					ZzLOG(ERROR, "%s\n\n",szdelQuery);
					goto daem5601_process_db_err;
				}
			}
			/*
		    sprintf(szQuery, " INSERT INTO zangsi_sum.T_DOWN_INFO "
							" ( deal_no,cont_gu,deal_date,deal_time,sale_user,buy_user "
							" , sect_code,share_meth,price_amt,won_mega,sale_amt,comp_amt "
							" , down_yn,del_yn,adult_yn,fixamt_yn,coupon_code ) "
							 " VALUES ( "
							 " %ld ,'%s' ,'%s' ,'%s' ,'%s' ,'%s' "
							 " ,'%s','%s',%d,%d,%d,%d"
							 " ,'%s','%s','%s','%s','%s' ) "
							  , (unsigned long)getnum(row, 0)  ,getstr(row,1) ,getstr(row,3) ,getstr(row,4) ,getstr(row,5) ,getstr(row,6)
							  ,getstr(row,9),getstr(row,10),(int)getnum(row, 11),(int)getnum(row, 12),(int)getnum(row, 13),(int)getnum(row, 14)
							  ,getstr(row,18),getstr(row,19),getstr(row,20),getstr(row,21),getstr(row,22)
							  );
			#ifdef _DEBUG
			printf("\n\n %s \n\n", szQuery);
			#endif

			if (mysql_query(con_backup, szQuery))
			{
			    ZzLOG(ERROR, "daem5601_igned long)getnum(row, 0));elete_data: INSERT T_DEAL_INFO error...\n");
				ZzLOG(ERROR, "daem5601_delete_data: [%d](%s)\n",mysql_errno(con_backup), mysql_error(con_backup));
				ZzLOG(ERROR, "%s\n\n",szQuery);
				goto daem5601_process_db_err;
		    }
		    */
			dwCount	++;


			memset (szQuery, 0x00, sizeof(szQuery));
			memset (szdelQuery, 0x00, sizeof(szdelQuery));
			
			sprintf(szQuery, "DELETE FROM zangsi.T_DEAL_INFO "
							 "where deal_no = %ld ; /*daem5601*/  "
							 ,(unsigned long)getnum(row, 0));

			
			sprintf(szdelQuery, "DELETE FROM zangsi.T_DEAL_IP "
							 "where deal_no = %ld ; /*daem5601*/  "
							 ,(unsigned long)getnum(row, 0));

			//ZzLOG(ALWAY, "szQuery = %s\n",szQuery);

			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daem5601_delete_data: DELETE T_DEAL_INFO error...\n");
				ZzLOG(ERROR, "daem5601_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "%s\n",szQuery);
				goto daem5601_process_db_err;
		    }
			if (mysql_query(con,szdelQuery))
			{
				
			    ZzLOG(ERROR, "daem5601_delete_data: DELETE T_DEAL_IP error...\n");
				ZzLOG(ERROR, "daem5601_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "%s\n",szdelQuery);
				goto daem5601_process_db_err;
			}
			
			if (tran_commit(con_backup)!=0)
			{
			    ZzLOG(ERROR, "process_db2: tran_commit error...\n");
			    goto daem5601_process_db_err;
			}
			//--------------------------------------------------------------------------
			// 트렌젝션종료
			//--------------------------------------------------------------------------
			if (tran_end(con_backup)!=0)	{
			    ZzLOG(ERROR, "process_db2: tran_end 테이베이스 오류입니다.\n");
			    goto daem5601_process_db_err;
			}

			if (tran_commit(con)!=0)
			{
			    ZzLOG(ERROR, "process_db1: tran_commit error...\n");
			    goto daem5601_process_db_err;
			}
			//--------------------------------------------------------------------------
			// 트렌젝션종료
			//--------------------------------------------------------------------------
			if (tran_end(con)!=0)	{
			    ZzLOG(ERROR, "process_db1: tran_end 테이베이스 오류입니다.\n");
			    goto daem5601_process_db_err;
			}
			

		}
		mysql_free_result(res);
	}


	ZzLOG(ALWAY, "daem5601_process: 백업된 자료의 갯수는 %ld 입니다.\n",dwCount);
	ZzLOG(ALWAY, "daem5601_process: 백업된 자료의 갯수는 %ld 입니다.\n",dwCount);
	return 0;

daem5601_process_db_err:
	ZzLOG(ERROR, "process_db1: [%d](%s)\n",mysql_errno(con), mysql_error(con));

	ZzLOG(ERROR, "process_db2: [%d](%s)\n",mysql_errno(con_backup), mysql_error(con_backup));

	tran_rollback(con_backup);
	tran_end(con_backup);

	tran_rollback(con);
	tran_end(con);

	mysql_free_result(res);
	ZzLOG(ALWAY, "daem5601_process: 백업된 자료의 갯수는 %ld 입니다.\n",dwCount);

	return -1;
}


/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5601_process_init(int argc, char **argv)
{
	char stemp[128];
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("daem5601_", "/logs/daemon");

    ZzLOG(ALWAY, "[daem5601]*****************프로그램 시작*****************\n");

    // 파라미터 값 설정 및 초기화
    if (argc != 2) {
    	goto arg_error;
    }

	memset(gst_date, 0x00, sizeof(gst_date));
	memset(gst_date_8, 0x00, sizeof(gst_date_8));

	strcpy(gst_date, argv[1]);
	strcpy(gst_date_8,gst_date);

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(con=db_connect_local("zangsi")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1);
	}


    ZzLOG(ALWAY, "[daem5601]================> db_connect_sumdb()\n");
	if (!(con_backup=db_connect_sumdb("zangsi_sum")))
	{
		ZzLOG(ERROR, "--> DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con_backup);
	   	return(-1);
	}
    ZzLOG(ALWAY, "[daem5601]================> db_connect_sumdb() success.... \n");

    return (0);

arg_error:
    ZzLOG(ERROR, "usage : %s YYYYMMDD\n", argv[0]);
    ZzLOG(ERROR, "        YYYYMMDD : 처리일자(00000000:시스템일자)\n");
    ZzPRT(ERROR, "usage : %s YYYYMMDD\n", argv[0]);
    ZzPRT(ERROR, "        YYYYMMDD : 처리일자(00000000:시스템일자)\n");
    return -1;
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daem5601_process_term()
{
    // DB close
	db_disconnect(con);
	db_disconnect(con_backup);

    ZzLOG(ALWAY, "[daem5601]*****************프로그램 종료*****************\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5601_signal(int nSignal)
{
    daem5601_process_term();
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
	signal(SIGTERM, daem5601_signal);
	signal(SIGINT,  daem5601_signal);
	signal(SIGQUIT, daem5601_signal);
	signal(SIGKILL, daem5601_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);

	if ( daem5601_process_init(argc, argv) == 0 )
	{
		/* 프로그램 메인루틴 */
		rc = daem5601_process();
		/* 프로그램 종료루틴 */
		daem5601_process_term();
	}
	return(0);
}
/*****************************************************************************
*  End of file...
*****************************************************************************/
