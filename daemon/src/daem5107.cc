/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5107.cc
 *         기능 : 업로드 권한을 위한 사용자별 컨텐츠 갯수 정보 파악
 *         설명 : 1. 프로세스 처리 예정시간 : 07:00 
 *     설치위치 : cmdsvr에 위치한다.
 *
 *       작성자 : LEE
 *       작성일 : 2005/03/08
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

#include "daemcom.h"
#include "commydb.h"



int daem5107_process();
int daem5107_process_db();
int daem5107_process_init(int argc, char **argv);
int daem5107_process_term();
void daem5107_signal(int nSignal);


MYSQL     *con;
MYSQL_RES *res;
MYSQL_ROW  row;




char gst_date[8+1];	// 처리일자
char gst_time[6+1];	// 처리시작시간
char ged_time[6+1];	// 처리마지막시간
char gproc_log[80]; // 로그메시지버퍼



//******************************************************************************
//* daem5107 main
//******************************************************************************
int daem5107_process()
{
	char szQuery[1000];  // query string
	memset(szQuery, 0x00, sizeof(szQuery));	
	
	
	


	// 처리일자가 "00000000"일때는 시스템일자 -3을 한다.
	if (strcmp(gst_date, "00000000") == 0)
	{
		
		strcpy(szQuery, "SELECT date_format(date_add( now(),INTERVAL - 1 DAY ),'%Y%m%d') ");
		// 현재로부터 30일 이전 날짜를 얻는다.

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
		strcpy(gst_date,   getstr(row, 0));
		mysql_free_result(res);
				
	}
	else
	{
		return -1;

	}



	
	#ifdef __DEBUG
	printf("적용 날짜 : %s\n",gst_date);
	#endif 


	
	if( daem5107_process_db() !=0)
	{
		return -1;
	}	
	

	
	return 0;
}











char* AppendSpecialChar(char* strSrcString, char cReplace,char* pResult)
{

	char strResult[600];
	char szHangul[2];
	memset(strResult,0x00,sizeof(strResult));
	
	int cTemp;
	int nSpecialCount=0;
	int nFileLen = strlen(strSrcString);
	
	
	
	for(unsigned long i=0; i<nFileLen ; i++)
	{
		cTemp = strSrcString[i];
		
	
		
		if( cTemp == '\'' ) 
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
//* daem5107 db 처리로직
//******************************************************************************
int daem5107_process_db()
{
    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "gproc_date : [%s]\n", gst_date);  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");


	char szQuery[1600];		// query string
	char szQuery2[1600];		// query string
	char szQuery3[1600];		// query string
	char szQuery4[1600];		// query string
	memset(szQuery, 0x00, sizeof(szQuery));	
	memset(szQuery2, 0x00, sizeof(szQuery2));	
	memset(szQuery3, 0x00, sizeof(szQuery3));	
	memset(szQuery4, 0x00, sizeof(szQuery4));	
	
	int  ret;
/*
	//DB 정보 삭제
	sprintf(szQuery,"delete from zangsi_sum.T_CONTENTS_CNT_INFO " );	
	ZzLOG(ERROR, "%s\n\n",szQuery);				
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5107_process: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
*/	
	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/
	
	unsigned long dwCount = 0;
	

		
		memset (szQuery, 0x00, sizeof(szQuery));

		/*
		strcpy(szQuery,"insert into zangsi_sum.T_CONTENTS_CNT_INFO select reg_user , count(reg_user) ,0 ,0, 0 , 0 ,0 ,0"
					   " from zangsi.T_CONTENTS_INFO where disp_end_date >= date_format(now(),'%Y%m%d' ) and del_yn = 'N' group by reg_user ");



		ZzLOG(ERROR, "%s\n\n",szQuery);
		
		#ifdef __DEBUG
		printf("%s\n\n",szQuery);
		#endif
		
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5107_process: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
		*/
		memset (szQuery, 0x00, sizeof(szQuery));	
		
/*		strcpy(szQuery,"select deal_no from zangsi_bck.T_DEAL_INFO where deal_date >= date_format(date_add(now(),INTERVAL - 30 DAY),'%Y%m%d')  limit 1 ");
					   
		ZzLOG(ALWAY, "%s\n\n",szQuery);
		
		#ifdef __DEBUG
		printf("%s\n\n",szQuery);
		#endif
		
		unsigned long deal_no = 0;
		
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5107_process: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		}	
		else
		{
			if (!(res = mysql_store_result(con))) {
			    ZzLOG(ERROR, "daem5107_process: mysql_store_result error...\n");
				ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			}
			{
			 	if (mysql_num_rows(res) > 0)	
			 	{
					row = mysql_fetch_row(res);
					deal_no = (unsigned long)getnum(row, 0);
				}
			}
		}
		



		sprintf(szQuery2,"  a.deal_no > %ld ",deal_no );
		
*/				
		int nStartRow = 0;
		int nTotalRowCnt = 0;
		char szBuyUser[16];
		
		while( nStartRow >= 0 && nStartRow < 1000000)
		{
			
			memset (szQuery4, 0x00, sizeof(szQuery4));	
			sprintf(szQuery4,"select user_id from zangsi_sum.T_CONTENTS_CNT_INFO limit %d , 1000 ",nStartRow);
			ZzLOG(ALWAY, "%s\n\n",szQuery4);
		
			if (mysql_query(con, szQuery4))
			{
			    ZzLOG(ERROR, "daem5107_process: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
				return -1;
								
			}	
			else
			{
				if (!(res = mysql_store_result(con))) 
				{
				    ZzLOG(ERROR, "daem5107_process: mysql_store_result error...\n");
					ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
					return -1;
				}
				{
				 	if (mysql_num_rows(res) > 0)	
				 	{
						while( (row = mysql_fetch_row(res) ) )
						{
							nTotalRowCnt++;
							ZzLOG(ALWAY, "%d 번째 처리 \n\n",nTotalRowCnt);
			
							memset (szBuyUser,0x00,sizeof(szBuyUser));
							memset (szQuery3,0x00,sizeof(szQuery3));
						
							
							strcpy(szBuyUser , getstr(row, 0) );
							
							memset (szQuery, 0x00, sizeof(szQuery));


							strcpy(szQuery,"replace into zangsi_sum.T_CONTENTS_CNT_INFO select reg_user , count(reg_user) ,0 ,0, 0 , 0 ,0 ,0"
										   " from zangsi.T_CONTENTS_INFO where disp_end_date >= date_format(now(),'%Y%m%d' ) and del_yn = 'N' ");
							
							memset (szQuery3,0x00,sizeof(szQuery3));
							
							sprintf( szQuery3 , " and reg_user = '%s' group by reg_user " , szBuyUser);
							strcat(szQuery,szQuery3);
							


							ZzLOG(ERROR, "%s\n\n",szQuery);
							
							#ifdef __DEBUG
							printf("%s\n\n",szQuery);
							#endif
							
							if (mysql_query(con, szQuery))
							{
							    ZzLOG(ERROR, "daem5107_process: mysql_query error...\n");
								ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
								return -1;
							}
					
					
					
							memset (szQuery, 0x00, sizeof(szQuery));
					
							strcpy(szQuery,	"replace into zangsi_sum.T_CONTENTS_CNT_INFO ( user_id , contents_cnt , other_contents_cnt , other_contents_pct) "
								"select /*STRAIGHT_JOIN*/ "
								"	d.user_id "
								", d.contents_cnt  "
								", d.other_contents_cnt + count(distinct(a.id) )  , round(  ( ( d.other_contents_cnt + count( distinct( a.id ) ) ) /d.contents_cnt ) * 100)  "
								"from  "
								"zangsi_sum.T_CONTENTS_CNT_INFO d "
								",zangsi.T_DEAL_INFO a "
								",zangsi.T_CONTENTS_FILELIST_SUB b  "
								",zangsi.T_CONTENTS_FILELIST_SUB c "
								"where ");
					
							

							sprintf(szQuery3," d.user_id = '%s' ",szBuyUser);
							
							strcat(szQuery , szQuery3);
							strcat(szQuery ,
								"  and a.buy_user = d.user_id and a.id = b.id "
								"  and c.reg_user = d.user_id and b.default_hash = c.default_hash  and b.file_size = c.file_size  "
								"  and a.deal_date >= date_format(date_add(now(),INTERVAL - 30 DAY),'%Y%m%d')  "
								"   and b.reg_user != c.reg_user "
								" group by d.user_id  "  );
					
					
							ZzLOG(ALWAY, "%s\n\n",szQuery);
							
							#ifdef __DEBUG
							printf("%s\n\n",szQuery);
							#endif
							
							
							if (mysql_query(con, szQuery))
							{
							    ZzLOG(ERROR, "daem5107_process: mysql_query error...\n");
								ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
								goto daem5107_process_db_err;
							}

/*
 							memset (szQuery3,0x00,sizeof(szQuery3));

							memset (szQuery, 0x00, sizeof(szQuery));
					
							strcpy(szQuery,	"replace into zangsi_sum.T_CONTENTS_CNT_INFO ( user_id , contents_cnt , other_contents_cnt , other_contents_pct) "
//								"select STRAIGHT_JOIN "
								"select  "
								"	d.user_id "
								", d.contents_cnt  "
								", d.other_contents_cnt + count(distinct(a.id) )  , round(  ( ( d.other_contents_cnt + count( distinct( a.id ) ) ) /d.contents_cnt ) * 100)  "
								"from  "
								"zangsi_sum.T_CONTENTS_CNT_INFO d "
								",zangsi_bck.T_DEAL_INFO a "
								",zangsi.T_CONTENTS_FILELIST_SUB b  "
								",zangsi.T_CONTENTS_FILELIST_SUB c "
								"where ");
					
											
							sprintf(szQuery3," and d.user_id = '%s' ",szBuyUser);
							
							strcat( szQuery , szQuery2);
							strcat( szQuery , szQuery3);
							strcat(szQuery,
								"  and a.buy_user = d.user_id and a.id = b.id "
								"  and c.reg_user = d.user_id and b.default_hash = c.default_hash  and b.file_size = c.file_size  "
								"  and a.deal_date >= date_format(date_add(now(),INTERVAL - 30 DAY),'%Y%m%d')  "
								"   and b.reg_user != c.reg_user "
								" group by d.user_id  "  );							
							ZzLOG(ALWAY, "%s\n===============================\n\n\n\n",szQuery);							
							#ifdef __DEBUG
							printf("%s\n\n",szQuery);
							#endif
							
							if (mysql_query(con, szQuery))
							{
							    ZzLOG(ERROR, "daem5107_process: mysql_query error...\n");
								ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
								goto daem5107_process_db_err;
							}
*/	
							
						}
						mysql_free_result(res);
					}
					else
					{
						nStartRow = -1;
						mysql_free_result(res);
						break;
					}
				}
			}
			
			nStartRow = nStartRow +1000;
							
			
		}
		
/*		memset (szQuery, 0x00, sizeof(szQuery));
 		memset (szQuery, 0x00, sizeof(szQuery));

		strcpy(szQuery,	"update zangsi_sum.T_CONTENTS_CNT_INFO set other_contents_pct = round(  (other_contents_cnt /contents_cnt) * 100)    " );
			


		ZzLOG(ERROR, "%s\n\n",szQuery);
		
		#ifdef __DEBUG
		printf("%s\n\n",szQuery);
		#endif
		
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5107_process: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}			
*/	
		strcpy(szQuery,"select reg_user , count(reg_user) from zangsi.T_CONTENTS_INFO "
					   " where disp_end_date >= date_format(now(),'%Y%m%d') and del_yn = 'N' and sect_code in ('13','09','14') group by reg_user " );
		

            		
		

		ZzLOG(ERROR, "%s\n\n",szQuery);
		
		#ifdef __DEBUG
		printf("%s\n\n",szQuery);
		#endif
		
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5107_process: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}	
	
		if (!(res = mysql_store_result(con))) {
		    ZzLOG(ERROR, "daem5107_process: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
	 	if (mysql_num_rows(res)==0)	
	 	{
		    ZzLOG(ALWAY, "daem5107_process: 처리할 자료가 없습니다.\n");
			goto daem5107_process_db_err;
		}

	    ZzLOG(ALWAY, "daem5107_process: select_cnt(%d)\n", mysql_num_rows(res));
	

		//--------------------------------------------------------------------------
		// 트렌젝션시작
		//--------------------------------------------------------------------------



				
		while((row = mysql_fetch_row(res))) 
		{

			dwCount	++;
			
				

			memset (szQuery, 0x00, sizeof(szQuery));



			
			
			sprintf(szQuery, "update zangsi_sum.T_CONTENTS_CNT_INFO set kns_contents_cnt = %ld  where user_id = '%s' "
							 , (unsigned long)getnum(row, 1)  , getstr(row, 0) );
							 
			
			
			#ifdef __DEBUG
			//printf("%s\n",szQuery);			 
			#endif		


			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daem5107_delete_data: update T_CONTENTS_CNT_INFO  error...\n");
				ZzLOG(ERROR, "daem5107_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "%s\n",szQuery);
				goto daem5107_process_db_err;
		    }
		    
		}
		
		mysql_free_result(res);
		
		
		
		
		ZzLOG(ALWAY, "daem5107_process: 백업된 자료의 갯수는 %ld 입니다.\n",dwCount);		
			
			
			
			
	
		memset (szQuery, 0x00, sizeof(szQuery));

		
		strcpy(szQuery,"select reg_user , count(reg_user) from zangsi.T_CONTENTS_INFO "
					   " where disp_end_date >= date_format(now(),'%Y%m%d') and del_yn = 'N' and req_id > 0 group by reg_user ");

		


		

		ZzLOG(ERROR, "%s\n\n",szQuery);
		
		#ifdef __DEBUG
		printf("%s\n\n",szQuery);
		#endif
		
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5107_process: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}	
	
	
	
	
	
	
	
		if (!(res = mysql_store_result(con))) {
		    ZzLOG(ERROR, "daem5107_process: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
	 	if (mysql_num_rows(res)==0)	
	 	{
			
		    ZzLOG(ALWAY, "daem5107_process: 처리할 자료가 없습니다.\n");
			goto daem5107_process_db_err;
		}

	    ZzLOG(ALWAY, "daem5107_process: select_cnt(%d)\n", mysql_num_rows(res));
	

		//--------------------------------------------------------------------------
		// 트렌젝션시작
		//--------------------------------------------------------------------------

		

		dwCount	 = 0;
				
		while((row = mysql_fetch_row(res))) 
		{

			dwCount	++;
			
			
	


			memset (szQuery, 0x00, sizeof(szQuery));



			
			
			sprintf(szQuery, "update zangsi_sum.T_CONTENTS_CNT_INFO set req_contents_cnt = %ld  where user_id = '%s' "
							 , (unsigned long)getnum(row, 1)  , getstr(row, 0) );
							 
			
			
			#ifdef __DEBUG
			//printf("%s\n",szQuery);			 
			#endif		


			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daem5107_delete_data: update T_CONTENTS_CNT_INFO  error...\n");
				ZzLOG(ERROR, "daem5107_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "%s\n",szQuery);
				goto daem5107_process_db_err;
		    }
		    
		}
		
		mysql_free_result(res);
				






		memset (szQuery, 0x00, sizeof(szQuery));

		
		strcpy(szQuery,"select reg_user , count(reg_user) from zangsi.T_CONTENTS_LIMIT "
					   " where reg_date >= date_format(date_add(now(),INTERVAL - 30 DAY ),'%Y%m%d') and sect_code in( '10' , "
					   " '03' )  group by reg_user ");
					   

		

            		
		

		ZzLOG(ERROR, "%s\n\n",szQuery);
		
		#ifdef __DEBUG
		printf("%s\n\n",szQuery);
		#endif
		
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5107_process: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}	
	
	
	
	
	
	
	
		if (!(res = mysql_store_result(con))) {
		    ZzLOG(ERROR, "daem5107_process: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
	 	if (mysql_num_rows(res)==0)	
	 	{
		
		    ZzLOG(ALWAY, "daem5107_process: 처리할 자료가 없습니다.\n");
			goto daem5107_process_db_err;
		}

	    ZzLOG(ALWAY, "daem5107_process: select_cnt(%d)\n", mysql_num_rows(res));
	

		//--------------------------------------------------------------------------
		// 트렌젝션시작
		//--------------------------------------------------------------------------

		

		dwCount	 = 0;
				
		while((row = mysql_fetch_row(res))) 
		{

			dwCount	++;
			
			
	
			// 보낸 편지 삭제 30일 이전.
			

			memset (szQuery, 0x00, sizeof(szQuery));



			
			
			sprintf(szQuery, "update zangsi_sum.T_CONTENTS_CNT_INFO set adult_contents_cnt = %ld  , adult_contents_pct "
							 " =  round((adult_contents_cnt/contents_cnt) * 100)  where user_id = '%s' "
							 , (unsigned long)getnum(row, 1)  , getstr(row, 0) );
							 
			
			
			#ifdef __DEBUG
			//printf("%s\n",szQuery);			 
			#endif		


			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daem5107_delete_data: update T_CONTENTS_CNT_INFO \n");
				ZzLOG(ERROR, "daem5107_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "%s\n",szQuery);
				goto daem5107_process_db_err;
		    }
		    
		}
		
		mysql_free_result(res);
		
		
		
		
		
		
		
		
		
		
				
		ZzLOG(ALWAY,"완료\n");



	
	return 0;

daem5107_process_db_err:
	ZzLOG(ERROR, "process_db1: [%d](%s)\n",mysql_errno(con), mysql_error(con));

	

	mysql_free_result(res);

	ZzLOG(ALWAY, "daem5107_process: 백업된 자료의 갯수는 %ld 입니다.\n",dwCount);	
		
	return -1;	
}


/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5107_process_init(int argc, char **argv)
{
	char stemp[128];
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("daem5107", "/logs/daemon"); 

    ZzLOG(ALWAY, "[daem5107]*****************프로그램 시작*****************\n");  

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
		if (!(con=db_connect_nodb("")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
	   	return(-1); 
	}


/*	if (!(con=db_connect("zangsi")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1); 
	}
*/		
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
int daem5107_process_term()
{
    // DB close
	db_disconnect(con);
	
    ZzLOG(ALWAY, "[daem5107]*****************프로그램 종료*****************\n");  

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5107_signal(int nSignal)
{
    daem5107_process_term();
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
	signal(SIGTERM, daem5107_signal);
	signal(SIGINT,  daem5107_signal);
	signal(SIGQUIT, daem5107_signal);
	signal(SIGKILL, daem5107_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daem5107_process_init(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daem5107_process();
		/* 프로그램 종료루틴 */                    
		daem5107_process_term();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
