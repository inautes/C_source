/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5109.cc
 *         기능 : 업로드 포인트 통계
 *         설명 : 
 *     설치위치 : cmdsvr에 위치한다.
 *
 *       작성자 : LEE
 *       작성일 : 2007/05/08
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

int daem5109_process_req_rank();  //HCS
int daem5109_process_result_sum();
int daem5109_process();
int daem5109_process_init(int argc, char **argv);
int daem5109_process_term();
void daem5109_signal(int nSignal);


MYSQL     *con;
MYSQL_RES *res;
MYSQL_ROW  row;

MYSQL	  *con_main;



char szMode[256];
char greg_date     [8+1];	// 처리일자
char greg_ym       [6+1];	// 처리일자
char g_today_reg_date[8+1];
char g_today_ym[8+1];

char g_first_day[2+1];

char g_ago_reg_date[8+1]; //처리일자로부터 한달전

int daem5109_process_req_rank()
{
	/*
		zangsi_sum.T_PERM_UPLOAD_POINT_CNT 여기에 들어간 넘들
		포인트를 (해당월)
		zangsi.T_PERM_UPLOAD_AUTH에  upload_point_mon 에 update한다.
	*/
	char szQuery[2048];		// query string
	int ret;

	memset(szQuery, 0x00, sizeof(szQuery));
	
	int cnt = 0;
	char point_ym[6+1];
	char user_id[16];
	char date[2];
	memset(point_ym, 0x00, sizeof(point_ym));
	memset(user_id, 0x00, sizeof(user_id));
	memset(szQuery, 0x00, sizeof(szQuery));
	
	strcpy(date, &g_today_reg_date[6]);
	
	if(strcmp(date, "01") == 0)
	{
		memcpy(point_ym, greg_ym, 6);
	
	}
	else
	{
		memcpy(point_ym, g_today_ym, 6);
	}

    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "----------------------- daem5109_process_req_rank 시 작 ( %s ) ---------------------\n",point_ym);
 	ZzLOG(ALWAY, "daem5109_process:  g_first_day = %s\n",g_first_day);
	
	if(strcmp(g_first_day, "01") == 0)
	{
		strcpy(szQuery,"update zangsi.T_PERM_UPLOAD_AUTH set upload_point_mon = 0 ");
		if (mysql_query(con_main, szQuery))
		{
		    ZzLOG(ERROR, "daem5109_process_req_rank:  mysql_query error...\n");
		    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		    return -1;	
		}	    
		ZzLOG(ALWAY, "daem5109_process_req_rank:  %s\n",szQuery);		
	}


	sprintf(szQuery,"select cnt, user_id from zangsi_sum.T_PERM_UPLOAD_POINT_CNT where point_ym = '%s'", point_ym);	

	ZzLOG(ALWAY, "daem5109_process_req_rank:  %s\n",szQuery);
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5109_process_req_rank:  mysql_query error...\n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;	
	}
 
	if ( !(res = mysql_store_result(con)) )
	{
	    ZzLOG(ERROR, "daem5109_process_req_rank: mysql_store_result error...\n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;	
	}

 	if ( mysql_num_rows(res) ==0 )
 	{
	    ZzLOG(ERROR, "daem5109_process_req_rank: mysql_num_rows  == 0 \n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    mysql_free_result(res);
	    return -1;
	} 			
	
	while(row = mysql_fetch_row(res))
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		cnt = getint(row,0) ;
		strcpy( user_id , getstr(row,1));

		sprintf(szQuery,"update zangsi.T_PERM_UPLOAD_AUTH set upload_point_mon = %d where user_id = '%s'", cnt, user_id);
	
		ZzLOG(ALWAY, "daem5109_process_req_rank:   %s\n", szQuery);
		
		if (mysql_query(con_main, szQuery))
		{
		    ZzLOG(ERROR, "daem5109_process_req_rank:  mysql_query error...\n\n\n\n");
		    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		    mysql_free_result(res);
		    return -1;	
		}
	}
		
		
	mysql_free_result(res);
    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "----------------------- daem5109_process_req_rank 종 료 ( %s ) ---------------------\n",greg_ym);
	
}

int daem5109_process_result_sum()
{
	char szQuery[2048];		// query string
	int ret;

    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "----------------------- daem5109_process_result_sum 시 작 ( %s ) ---------------------\n",greg_ym);

	memset(szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery, " update zangsi_sum.T_EVENT_POINT_CNT set "
					 " yester_rank = today_rank " 
					 " where "
					 " point_ym = '%s' "
					 ,greg_ym  ) ;
			
	ZzLOG(ALWAY, "daem5109_process:    %s\n",  szQuery);
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5109_process:  mysql_query error...\n\n\n\n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;	
	}

		
		
	
 	ZzLOG(ALWAY, "daem5109_process:  g_first_day = %s\n",g_first_day);
	
 	if( strcmp(g_first_day,"01") == 0 )
 	{
 		
 		memset (szQuery, 0x00, sizeof(szQuery));
 		sprintf(szQuery,"update zangsi_sum.T_EVENT_POINT_CNT set yester_rank =0 , point = 0 where point_ym = '%s' " ,greg_ym  );
 		
		/*
		sprintf(szQuery,"replace into zangsi_sum.T_EVENT_POINT_CNT ( "
				"	point_ym, user_id, cnt ,yester_rank "
				" ) select '%s', a.contents_reg_user,  0  , 0"
				" from zangsi.T_PERM_UPLOAD_POINT_HIST a  "
				" where substring(a.reg_date, 1,6) = '%s'  "
				" group by a.contents_reg_user " ,greg_ym,greg_ym);
		*/
 	}
	else
	{
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update zangsi_sum.T_EVENT_POINT_CNT set "
				 " yester_rank = today_rank " 
				 " where "
				 " point_ym = '%s' "
				 ,greg_ym  ) ;
		
			
		/*
		sprintf(szQuery,"replace into zangsi_sum.T_PERM_UPLOAD_POINT_CNT ( "
				"	point_ym, user_id, cnt ,yester_rank "
				" ) select '%s', a.contents_reg_user,  count(*) as cnt  , b.yester_rank"
				" from zangsi.T_PERM_UPLOAD_POINT_HIST a left outer join  zangsi_sum.T_PERM_UPLOAD_POINT_CNT b "
				" on a.contents_reg_user = b.user_id and b.point_ym = '%s' "
				" where substring(a.reg_date, 1,6) = '%s'  "
				" group by a.contents_reg_user, substring(a.reg_date, 1,6) " ,greg_ym,greg_ym,greg_ym);
		*/
	}	
	
	ZzLOG(ALWAY, "daem5109_process:  %s\n",szQuery);
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5109_process:  mysql_query error...\n");
	    return -1;	
	}



	memset (szQuery, 0x00, sizeof(szQuery));
	
	strcpy(szQuery,"create temporary table zangsi_sum.TEMP_EVENT_POINT_CNT ( "
					 " user_id varchar(12) binary,  point	decimal (11,2) 	) ");
			
			
	ZzLOG(ALWAY, "daem5109_process:  %s\n",szQuery);	
	

	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5109_process:  mysql_query error...\n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;	
	}
	
		
 	memset (szQuery, 0x00, sizeof(szQuery));
	
	sprintf(szQuery," insert into zangsi_sum.TEMP_EVENT_POINT_CNT ( "
				   " user_id, point ) select user_id, cnt from zangsi_sum.T_PERM_UPLOAD_POINT_CNT "			   
				   " where point_ym = '%s' " , greg_ym  );

	ZzLOG(ALWAY, "daem5109_process:  %s\n",szQuery);	
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5109_process:  mysql_query error...\n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    
	 	memset (szQuery, 0x00, sizeof(szQuery));
		
		strcpy(szQuery," drop temporary table zangsi_sum.TEMP_EVENT_POINT_CNT " );

		ZzLOG(ALWAY, "daem5109_process:  %s\n",szQuery);	
		
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5109_process:  mysql_query error...\n");
		    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		}
			
	
	    
	    return -1;	
	}
		

 	memset (szQuery, 0x00, sizeof(szQuery));
	
	sprintf(szQuery," insert into zangsi_sum.TEMP_EVENT_POINT_CNT ( "
				   " user_id, point ) select user_id, (sum(nmnt_point) / 100) "
				   " from zangsi_sum.T_CONTENTS_NMNT_CNT where reg_date = '%s' group by user_id " ,  greg_ym  );


	ZzLOG(ALWAY, "daem5109_process:  %s\n",szQuery);	
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5109_process:  mysql_query error...\n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    
	 	memset (szQuery, 0x00, sizeof(szQuery));
		
		strcpy(szQuery," drop temporary table zangsi_sum.TEMP_EVENT_POINT_CNT " );
		ZzLOG(ALWAY, "daem5109_process:  %s\n",szQuery);	
		
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5109_process:  mysql_query error...\n");
		    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		}
			
	
	    
	    return -1;	
	}
		


 	memset (szQuery, 0x00, sizeof(szQuery));
	
	sprintf(szQuery," replace into zangsi_sum.T_EVENT_POINT_CNT ( "
	 			   " point_ym, user_id	, point	 ,yester_rank ) select '%s', a.user_id 	, sum(a.point)  "
	 			   " , max(b.yester_rank)"
	 			   " from  	zangsi_sum.TEMP_EVENT_POINT_CNT a left outer join zangsi_sum.T_EVENT_POINT_CNT b "
	 			   " on a.user_id = b.user_id and b.point_ym= '%s' group by user_id ",  greg_ym  ,  greg_ym  );

	ZzLOG(ALWAY, "daem5109_process:  %s\n",szQuery);	
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5109_process:  mysql_query error...\n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;	
	}
		
 	memset (szQuery, 0x00, sizeof(szQuery));
	
	strcpy(szQuery," drop temporary table zangsi_sum.TEMP_EVENT_POINT_CNT " );
	ZzLOG(ALWAY, "daem5109_process:  %s\n",szQuery);	
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5109_process:  mysql_query error...\n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	}
		
		
		
		
		

   //랭킹 구하기
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery," update zangsi_sum.T_EVENT_POINT_CNT set today_rank = 0  " 
					" where point_ym = '%s' " ,greg_ym);
			
			
	ZzLOG(ALWAY, "daem5109_process:  %s\n",szQuery);	
	

	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5109_process:  mysql_query error...\n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;	
	}
	
		
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery,"select  a.user_id , " 
					" a.point   "
					"	from zangsi_sum.T_EVENT_POINT_CNT  a , zangsi.T_USER_INFO b, zangsi.T_PERM_UPLOAD_AUTH c "
					" where  a.user_id = b.user_id and a.point_ym = '%s' and b.use_stat='0' and a.user_id = c.user_id and c.perm_gu in('2','6')"
					" order by 2 desc limit 100 "
					,greg_ym);
			
			
	ZzLOG(ALWAY, "daem5109_process:  %s\n",szQuery);	
	

	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5109_process:  mysql_query error...\n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;	
	}
	
	
 
	if ( !(res = mysql_store_result(con)) )
	{
	    ZzLOG(ERROR, "daem5109_process: mysql_store_result error...\n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;	
	}

 	if ( mysql_num_rows(res) ==0 )
 	{
	    ZzLOG(ERROR, "daem5109_process: mysql_num_rows  == 0 \n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    mysql_free_result(res);
	    return -1;
	} 			
 	
 			
	
 	long nCurRanking = 1; 

 	
 	char szUserId[256];
 	char szUseStat[12];
 	
	while( (row = mysql_fetch_row(res)  ) && nCurRanking < 31 )
	{


		memset(szQuery, 0x00, sizeof(szQuery));
 		memset(szUserId,0x00,sizeof(szUserId));

 	 	

		strcpy( szUserId , getstr(row,0) );
		
		
			sprintf(szQuery, " update zangsi_sum.T_EVENT_POINT_CNT set "
							 "  today_rank = %d " 
							 " where "
							 " point_ym = '%s' "
							 " and user_id = '%s'  "
							 ,nCurRanking ,greg_ym ,szUserId ) ;
					
		
		ZzLOG(ALWAY, "daem5109_process:   %d %s\n", nCurRanking, szQuery);
		
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5109_process:  mysql_query error...\n\n\n\n");
		    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		    mysql_free_result(res);
		    return -1;	
		}
		nCurRanking++;
	}
		
		
	mysql_free_result(res);
	

 	
	
 		
	
}

//******************************************************************************
//* daem5109 main
//******************************************************************************
int daem5109_process_upload()
{
	char szQuery[2048];		// query string
	int ret;

    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "----------------------- 시 작 ( %s ) ---------------------\n",greg_ym);







	memset(szQuery, 0x00, sizeof(szQuery));

 	
 	
 	

	if(strcmp(g_first_day, "01") == 0)
	{
		strcpy(szQuery,"update zangsi.T_PERM_UPLOAD_AUTH set  nmnt_first_point = 0, nmnt_second_point = 0, nmnt_third_point = 0   ");
		if (mysql_query(con_main, szQuery))
		{
		    ZzLOG(ERROR, "daem5109_process_upload:  mysql_query error...\n");
		    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		    return -1;	
		}	    
		ZzLOG(ALWAY, "daem5109_process_upload:  %s\n",szQuery);		
	}
 	

		
		
	
 	ZzLOG(ALWAY, "daem5109_process:  g_first_day = %s\n",g_first_day);
 	if( strcmp(g_first_day,"01") == 0 )
 	{
 
 		memset (szQuery, 0x00, sizeof(szQuery));
 		

	 	sprintf(szQuery," replace into zangsi_sum.T_CONTENTS_NMNT_CNT (      reg_date        , user_id, nmnt_code, nmnt_point, yester_rank ) "
						"  select '%s' , a.reg_user, a.nmnt_code, sum(a.nmnt_point)  as point  , 0  "
						" from zangsi.T_CONTENTS_NMNT a  "
						" left outer join zangsi_sum.T_CONTENTS_NMNT_CNT  c "
						" on a.reg_user = c.user_id and a.nmnt_code = c.nmnt_code and c.reg_date = '%s'  "
						" where substring(a.nmnt_date,1,6) = '%s' and a.del_yn = 'N' group by  a.reg_user , a.nmnt_code "
						,greg_ym,greg_ym,greg_ym );

		
 	}
	else
	{
 		memset (szQuery, 0x00, sizeof(szQuery));
 		

	 	sprintf(szQuery," replace into zangsi_sum.T_CONTENTS_NMNT_CNT (      reg_date        , user_id, nmnt_code, nmnt_point, yester_rank ) "
						"  select '%s' , a.reg_user, a.nmnt_code, sum(a.nmnt_point)  as point  , max(c.today_rank)  "
						" from zangsi.T_CONTENTS_NMNT a  "
						" left outer join zangsi_sum.T_CONTENTS_NMNT_CNT  c  "
						" on a.reg_user = c.user_id and a.nmnt_code = c.nmnt_code and c.reg_date = '%s'  "
						" where substring(a.nmnt_date,1,6) = '%s' and a.del_yn = 'N' group by  a.reg_user , a.nmnt_code "
						,greg_ym,greg_ym,greg_ym);

	}	
	
	ZzLOG(ALWAY, "daem5109_process_upload:  %s\n",szQuery);
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5109_process:  mysql_query error...\n");
	    return -1;	
	}



	sprintf(szQuery," select user_id , count(user_id) from  zangsi.T_FRND_BAD  "
					" group by user_id "  ) ;
					
	ZzLOG(ALWAY, "daem5109_process_upload:  %s\n",szQuery);

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5109_process_upload:  mysql_query error...\n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1; 
	}
	
 
	if ( !(res = mysql_store_result(con)) )
	{
	    ZzLOG(ERROR, "daem5109_process_upload: mysql_store_result error...\n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;	
	}

 	if ( mysql_num_rows(res) ==0 )
 	{
	    ZzLOG(ERROR, "daem5109_process_upload: mysql_num_rows  == 0 \n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	} 			
 				
 	char szUserId[256];
 	int nMinValue = 0; 
 	char szCode[12];
 	
 		
	while( row = mysql_fetch_row(res)    )
	{

 		memset(szUserId,0x00,sizeof(szUserId));
 		memset(szCode,0x00,sizeof(szCode));
 	 	
		strcpy( szUserId , getstr(row,0) );
		nMinValue = getint(row,1) ;
		
			
		sprintf(szQuery, " update zangsi_sum.T_CONTENTS_NMNT_CNT set "
				 "  nmnt_point = nmnt_point - %d " 
				 " where "
				 " reg_date = '%s' "
				 " and user_id = '%s'  "
				 ,nMinValue,greg_ym ,szUserId ) ;	
	
		ZzLOG(ALWAY, "daem5109_process_upload:  %s\n",  szQuery);
		
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5109_process_upload:  mysql_query error...\n\n\n\n");
		    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		    mysql_free_result(res);
		    return -1;	
		}
					
	}

	mysql_free_result(res);

	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery," CREATE TEMPORARY TABLE zangsi_sum.T_CONTENTS_NMNT_TEMP  "
					"  select b.user_id as user_id , sum(b.nmnt_point) as point , nmnt_code   from zangsi_sum.T_CONTENTS_NMNT_CNT b "
   					" where b.reg_date = '%s' group by b.user_id , b.nmnt_code "
					,greg_ym);
	ZzLOG(ALWAY, "daem5109_process_upload:  %s\n",szQuery);	
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5109_process:  mysql_query error...\n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;	
	}


// 

	
//nmnt_first_point	

	int nLimit_Cnt = 1;
	int nLimit = 100;
	int nLimitV = 0;
	int nExcStop = 0;
	
	for( int i = 1 ;i < 4 ; i++ )
	{
		

		nLimit_Cnt = 0;
		nLimit     = 100;
		nLimitV    = 0;
		nExcStop   = 0;
		memset(szCode,0x00,sizeof(szCode));	 		
		sprintf(szCode , "%02d",i);
				
		
		while( 1 && nExcStop < 60000 )
		{

				
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery," select user_id , point  from zangsi_sum.T_CONTENTS_NMNT_TEMP  "
							" where   nmnt_code = '%s' limit %ld ,100 " , szCode, nLimitV );

			ZzLOG(ALWAY, "daem5109_process_upload:  %s\n",szQuery);	
			
			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daem5109_process:  mysql_query error...\n");
			    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
			    return -1;	
			}


			if ( !(res = mysql_store_result(con)) )
			{
			    ZzLOG(ERROR, "daem5109_process_upload: mysql_store_result error...\n");
			    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
			    return -1;
			}
	
		 	if ( mysql_num_rows(res) ==0 )
		 	{
			    ZzLOG(ERROR, "daem5109_process_upload: mysql_num_rows  == 0 \n");
			    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
			    mysql_free_result(res);		
			    break;
			} 			
			else
			{
		 				
			 	char szUserId[256];
			 	memset( szUserId,0x00,sizeof(szUserId));
			 	
			 	long nPoint = 0; 
			 	
			 	char szQueryAdd[256];
			 	memset(szQueryAdd,0x00,sizeof(szQueryAdd));
		
			 	
				while( row = mysql_fetch_row(res)    )
				{
					memset( szUserId,0x00,sizeof(szUserId));
			 		nPoint = 0; 
			 		memset(szQueryAdd,0x00,sizeof(szQueryAdd));
					memset (szQuery, 0x00, sizeof(szQuery));
					
					
					strcpy( szUserId,getstr(row,0));
					nPoint = getint(row,1);
					
					if( strcmp(szCode,"01") == 0 )
					{
						strcpy( szQueryAdd, "nmnt_first_point");
					}
					else if( strcmp(szCode,"02") == 0 )
					{
						strcpy( szQueryAdd, "nmnt_second_point");
					}
					else if( strcmp(szCode,"03") == 0 )
					{
						strcpy( szQueryAdd, "nmnt_third_point");
					}

					sprintf(szQuery," update zangsi.T_PERM_UPLOAD_AUTH set  %s = %ld  where user_id = '%s' " , szQueryAdd , nPoint , szUserId );

					ZzLOG(ALWAY, "daem5109_process_upload:  %s\n",szQuery);	

					if (mysql_query(con_main, szQuery))
					{
					    ZzLOG(ERROR, "daem5109_process:  mysql_query error...\n");
					    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
					    mysql_free_result(res);		
					    return -1;	
					}
					
				}
			}
			
			nExcStop++;
			nLimit_Cnt++;
			nLimitV = nLimit * nLimit_Cnt ;
			
			mysql_free_result(res);		
			
		}
		
	}
	
	
	sprintf(szQuery, "DROP TEMPORARY TABLE zangsi_sum.T_CONTENTS_NMNT_TEMP  ");

	ZzLOG(ALWAY, "daem5109_process_upload:  %s\n",szQuery);	
		
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5109_process_upload:  mysql_query error...\n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;	
    }
    	
 
 
 
 
 	// 요청 자료와 추천 마일리지 랭킹은 이후 합산에 - 20070719일 수정
	 		
 	long nCurRanking1 = 1; 

	int i = 0;

	

	for (  i =1 ; i <4 ; i++ )
	{
		
		nCurRanking1 = 1;
		memset(szCode,0x00,sizeof(szCode));	 		
		sprintf(szCode , "%02d",i);
		
		
		
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery,"  select a.user_id , a.nmnt_code , a.nmnt_point from zangsi_sum.T_CONTENTS_NMNT_CNT a , zangsi.T_USER_INFO b, zangsi.T_PERM_UPLOAD_AUTH c "
						 " where a.reg_date= '%s' and a.user_id = b.user_id and b.use_stat = '0' and nmnt_code = '%s' and a.user_id = c.user_id and c.perm_gu in('2','6')  "
						 " order by  3 desc  limit 100  "
							,greg_ym,szCode);
	
		ZzLOG(ALWAY, "daem5109_process_upload:  %s\n",szQuery);	
		
	
		
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5109_process_upload:  mysql_query error...\n");
		    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		    return -1;	
		}
		
	 
		if ( !(res = mysql_store_result(con)) )
		{
		    ZzLOG(ERROR, "daem5109_process_upload: mysql_store_result error...\n");
		    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		    return -1;	
		}
	
	 	if ( mysql_num_rows(res) ==0 )
	 	{
		    ZzLOG(ERROR, "daem5109_process_upload: mysql_num_rows  == 0 \n");
		    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));

		} 			
		else
		{	 				
			
			while( (row = mysql_fetch_row(res)) && nCurRanking1 < 100 )
			{

				memset(szUserId,0x00,sizeof(szUserId));
				strcpy(szUserId,getstr(row,0));
								
				sprintf(szQuery, " update zangsi_sum.T_CONTENTS_NMNT_CNT set "
						 "  today_rank = %d " 
						 " where "
						 " reg_date = '%s' "
						 " and user_id = '%s'  and nmnt_code = '%s' "
						 ,nCurRanking1,greg_ym ,szUserId ,szCode) ;			
		

		
				if (mysql_query(con, szQuery))
				{
				    ZzLOG(ERROR, "daem5109_process_upload:  mysql_query error...\n");
				    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
				    return -1;	
				}
				ZzLOG(ALWAY, "daem5109_process_upload:  %s\n",szQuery);	
				nCurRanking1++;	 								
			}	
			
		}
			
		mysql_free_result(res);
					
	}
	
	
	
	

	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery," 	select a.user_id , SUM(CASE a.nmnt_code WHEN '01' THEN a.today_rank ELSE 0 END ) as rank1 "
					"	, SUM(CASE a.nmnt_code WHEN '02' THEN a.today_rank ELSE 0 END ) as rank2 "
					"	, SUM(CASE a.nmnt_code WHEN '03' THEN a.today_rank ELSE 0 END ) as rank3 " 
					"	 from zangsi_sum.T_CONTENTS_NMNT_CNT a , zangsi.T_USER_INFO b, zangsi.T_PERM_UPLOAD_AUTH c  "
					"		 where a.reg_date = '%s'  "
					"		 and a.user_id = b.user_id and b.use_stat = '0' and a.user_id = c.user_id and c.perm_gu in ('2', '6') "
					"		 group by a.user_id  having ( rank1 + rank2 + rank3  > 0 ) "
					,greg_ym);

			
			
	ZzLOG(ALWAY, "daem5109_process_upload:  %s\n",szQuery);	
	
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5109_process_upload:  mysql_query error...\n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;	
	}
	
 
	if ( !(res = mysql_store_result(con)) )
	{
	    ZzLOG(ERROR, "daem5109_process_upload: mysql_store_result error...\n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;	
	}

 	if ( mysql_num_rows(res) ==0 )
 	{
	    ZzLOG(ERROR, "daem5109_process_upload: mysql_num_rows  == 0 \n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));

	} 			
 		
 
 	int nRank1= 0;
 	int nRank2= 0;
 	int nRank3= 0;

 		
	while( row = mysql_fetch_row(res)    )
	{
		memset(szQuery, 0x00, sizeof(szQuery));
 		memset(szUserId,0x00,sizeof(szUserId));
 		nRank1 = 0 ;
 		nRank2 = 0 ;
 		nRank3 = 0 ;
 		
 		strcpy(szUserId,getstr(row,0));
 		nRank1 = getint(row,1);
 		nRank2 = getint(row,2);
 		nRank3 = getint(row,3);
 		
 		if( nRank1 == 0 )
 		{
 			nRank1 = 10000;
 		}
 		if( nRank2 == 0 )
 		{
 			nRank2 = 10000;
 		}
 		if( nRank3 == 0 )
 		{
 			nRank3 = 10000;
 		}	 		
 		
 		if( nRank1 > nRank2 )
 		{
 			if( nRank2 > nRank3 )
 			{
 				//nRank3 
 				nRank1 = 0;
 				nRank2 = 0;
 			}
 			else
 			{
 				//nRank2
 				nRank1 = 0;
 				nRank3 = 0;
 			}
 		}
 		else
 		{
 			if( nRank1 > nRank3 )
 			{
 				//nRank3
 				nRank1 = 0;
 				nRank2 = 0;
 			}
 			else
 			{
 				//nRamk1
 				nRank2 = 0;
 				nRank3 = 0;
 			}
 		}
 		
 		if( nRank1 == 0 )
 		{
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery," update zangsi_sum.T_CONTENTS_NMNT_CNT set today_rank = 0 where "
							"  user_id = '%s' and reg_date = '%s' and nmnt_code = '01' "
							,szUserId,greg_ym);
		
			ZzLOG(ALWAY, "daem5109_process_upload:  %s\n",szQuery);	
			
			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daem5109_process_upload:  mysql_query error...\n");
			    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
			    return -1;	
			}
				 			
 			
 		}
 		if( nRank2 == 0 )
 		{
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery," update zangsi_sum.T_CONTENTS_NMNT_CNT set today_rank = 0 where "
							"  user_id = '%s' and reg_date = '%s' and nmnt_code = '02' "
							,szUserId,greg_ym);
		
			ZzLOG(ALWAY, "daem5109_process_upload:  %s\n",szQuery);	
			
			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daem5109_process_upload:  mysql_query error...\n");
			    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
			    return -1;	
			}
 			
 		}
 		if( nRank3 == 0 )
 		{
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery," update zangsi_sum.T_CONTENTS_NMNT_CNT set today_rank = 0 where "
							"  user_id = '%s' and reg_date = '%s' and nmnt_code = '03' "
							,szUserId,greg_ym);
		
			ZzLOG(ALWAY, "daem5109_process_upload:  %s\n",szQuery);	
			
			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daem5109_process_upload:  mysql_query error...\n");
			    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
			    return -1;	
			}
 		}	 			 			 		
	}	
	mysql_free_result(res);




	for (  i =1 ; i <4 ; i++ )
	{
		
		nCurRanking1 = 1;
		memset(szCode,0x00,sizeof(szCode));	 		
		sprintf(szCode , "%02d",i);
		
		
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery,"  select a.user_id , a.nmnt_code , a.today_rank from zangsi_sum.T_CONTENTS_NMNT_CNT a , zangsi.T_USER_INFO b, zangsi.T_PERM_UPLOAD_AUTH c"
						 " where a.reg_date= '%s' and a.user_id = b.user_id and b.use_stat = '0' and a.nmnt_code = '%s' and a.today_rank > 0 and a.user_id = c.user_id and c.perm_gu in('2','6') "
						 " order by  3   limit 100  "
							,greg_ym,szCode);
	
		ZzLOG(ALWAY, "daem5109_process_upload:  %s\n",szQuery);	
		
	
		
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5109_process_upload:  mysql_query error...\n");
		    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		    return -1;	
		}
		
	 
		if ( !(res = mysql_store_result(con)) )
		{
		    ZzLOG(ERROR, "daem5109_process_upload: mysql_store_result error...\n");
		    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		    return -1;	
		}
	
	 	if ( mysql_num_rows(res) ==0 )
	 	{
		    ZzLOG(ERROR, "daem5109_process_upload: mysql_num_rows  == 0 \n");
		    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));

		} 			
		else
		{	 				
			
			while( ( row = mysql_fetch_row(res)) && nCurRanking1 < 100)
			{
				
				memset(szUserId,0x00,sizeof(szUserId));
				strcpy(szUserId,getstr(row,0));
				
				
				sprintf(szQuery, " update zangsi_sum.T_CONTENTS_NMNT_CNT set "
						 "  today_rank = %d " 
						 " where "
						 " reg_date = '%s' "
						 " and user_id = '%s'  and nmnt_code = '%s' "
						 ,nCurRanking1,greg_ym ,szUserId ,szCode) ;			
		

		
				if (mysql_query(con, szQuery))
				{
				    ZzLOG(ERROR, "daem5109_process_upload:  mysql_query error...\n");
				    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
				    return -1;	
				}
				ZzLOG(ALWAY, "daem5109_process_upload:  %s\n",szQuery);	
				nCurRanking1++;	 								
			}	
			
		}
			
		mysql_free_result(res);
					
	}
	
	

	
	return 0;	
}

int daem5109_process()
{
	char szQuery[2048];		// query string
	int ret;

    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "----------------------- 시 작 ( %s ) ---------------------\n",greg_ym);




	memset(szQuery, 0x00, sizeof(szQuery));

 	

	/*
	sprintf(szQuery, " update zangsi_sum.T_PERM_UPLOAD_POINT_CNT set "
					 " yester_rank = today_rank , today_rank = %d " 
					 " where "
					 " point_ym = '%s' "
					 " and user_id = '%s'  "
					 ,nCurRanking ,greg_ym ,szUserId ) ;
					 */
	sprintf(szQuery, " update zangsi_sum.T_PERM_UPLOAD_POINT_CNT set "
					 " yester_rank = today_rank " 
					 " where "
					 " point_ym = '%s' "
					 ,greg_ym  ) ;
			
	ZzLOG(ALWAY, "daem5109_process:    %s\n",  szQuery);
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5109_process:  mysql_query error...\n\n\n\n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;	
	}

		
		
	
 	
 	if( strcmp(g_first_day,"01") == 0 )
 	{
 		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery,"replace into zangsi_sum.T_PERM_UPLOAD_POINT_CNT ( "
				"	point_ym, user_id, cnt ,yester_rank "
				" ) select '%s', a.contents_reg_user,  0  , 0"
				" from zangsi.T_PERM_UPLOAD_POINT_HIST a  "
				" where substring(a.reg_date, 1,6) = '%s'  "
				" group by a.contents_reg_user " ,greg_ym,greg_ym);
		
 	}
	else
	{
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery,"replace into zangsi_sum.T_PERM_UPLOAD_POINT_CNT ( "
				"	point_ym, user_id, cnt ,yester_rank "
				" ) select '%s', a.contents_reg_user,  count(*) as cnt  , b.yester_rank"
				" from zangsi.T_PERM_UPLOAD_POINT_HIST a left outer join  zangsi_sum.T_PERM_UPLOAD_POINT_CNT b "
				" on a.contents_reg_user = b.user_id and b.point_ym = '%s' "
				" where substring(a.reg_date, 1,6) = '%s'  "
				" group by a.contents_reg_user, substring(a.reg_date, 1,6) " ,greg_ym,greg_ym,greg_ym);
		
	}	
	
	ZzLOG(ALWAY, "daem5109_process:  %s\n",szQuery);
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5109_process:  mysql_query error...\n");
	    return -1;	
	}



// 요청 자료와 추천 마일리지 랭킹은 이후 합산에 - 20070719일 수정	 		
	
	memset (szQuery, 0x00, sizeof(szQuery));
/*	sprintf(szQuery,"select  a.contents_reg_user , "
					" count(*)     , c.use_stat "
					"	from zangsi.T_PERM_UPLOAD_POINT_HIST a , zangsi_sum.T_PERM_UPLOAD_POINT_CNT b , zangsi.T_USER_INFO c  "
					" where a.reg_date >= '20070509' and substring(a.reg_date, 1, 6) = '%s'  "
					" and a.contents_reg_user = b.user_id and b.user_id = c.user_id  "
					"  group by substring(a.reg_date, 1, 6), a.contents_reg_user "
					" order by 2 desc limit 100"
					,greg_ym);
*/			
	sprintf(szQuery," select  a.user_id ,  cnt    "
					" from zangsi_sum.T_PERM_UPLOAD_POINT_CNT a , zangsi.T_USER_INFO b, zangsi.T_PERM_UPLOAD_AUTH c  "
					" where a.point_ym = '%s' and a.user_id = b.user_id and b.use_stat = '0' and a.user_id = c.user_id and c.perm_gu in('2','6') "
					" order by 2 desc limit 100 " , greg_ym);
					
	ZzLOG(ALWAY, "daem5109_process:  %s\n",szQuery);	
	

	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5109_process:  mysql_query error...\n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;	
	}
	
	
 
	if ( !(res = mysql_store_result(con)) )
	{
	    ZzLOG(ERROR, "daem5109_process: mysql_store_result error...\n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    return -1;	
	}

 	if ( mysql_num_rows(res) ==0 )
 	{
	    ZzLOG(ERROR, "daem5109_process: mysql_num_rows  == 0 \n");
	    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
	    mysql_free_result(res);
	    return -1;
	} 			
 	
 	
	
 	long nCurRanking = 1; 

 	
 	char szUserId[256];


	
	while( (row = mysql_fetch_row(res)  ) && nCurRanking < 31 )
	{


		memset(szQuery, 0x00, sizeof(szQuery));
 		memset(szUserId,0x00,sizeof(szUserId));

		strcpy( szUserId , getstr(row,0) );
	

			sprintf(szQuery, " update zangsi_sum.T_PERM_UPLOAD_POINT_CNT set "
							 "  today_rank = %d " 
							 " where "
							 " point_ym = '%s' "
							 " and user_id = '%s'  "
							 ,nCurRanking ,greg_ym ,szUserId ) ;
					
	
		ZzLOG(ALWAY, "daem5109_process:   %d %s\n", nCurRanking, szQuery);
		
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5109_process:  mysql_query error...\n\n\n\n");
		    ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		    mysql_free_result(res);
		    return -1;	
		}
		nCurRanking++;
	}
		
		
	mysql_free_result(res);
	

	daem5109_process_req_rank();
	
	return 0;


	
}


/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5109_get_sysdate(char* pNowDate)
{
	char szQuery[1000];		// query string
	char sztemp [100];      // query temp

	memset(szQuery, 0x00, sizeof(szQuery));
	if( pNowDate == NULL)
	{
		strcpy(szQuery, "SELECT date_format( date_add(now(), INTERVAL -1 DAY ),'%Y%m%d') ,date_format(now(),'%Y%m%d'), date_format(date_add(now(), INTERVAL - 31 DAY), '%Y%m%d')");
	}
	else
	{
		//strcpy(szQuery, "SELECT date_format(date_add('%s', INTERVAL -1 DAY ),'%Y%m%d') , date_format(now(),'%Y%m%d')",pNowDate);		
		sprintf(szQuery, "SELECT date_format(date_add('%s', ",pNowDate);
		strcat(szQuery,"INTERVAL -1 DAY ),'%Y%m%d') , date_format(now(),'%Y%m%d'), date_format(date_add(now(), INTERVAL - 31 DAY), '%Y%m%d')");
		
	}
	/*
	** 결제처리시간은 현재시간 -1시간
	** 현제시간 01:00:00 이라면 from 00:00:00 ~ 00:99:99
	*/
	
	

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "sysdate: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "sysdate: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "sysdate: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
	
	row = mysql_fetch_row(res);

	memset(greg_date     , 0x00, sizeof(greg_date     ));
	strcpy(greg_date ,   getstr(row, 0));

	memset(g_today_reg_date     , 0x00, sizeof(greg_date     ));
	strcpy(g_today_reg_date ,   getstr(row, 1));

	memset(g_ago_reg_date     , 0x00, sizeof(g_ago_reg_date     ));
	strcpy(g_ago_reg_date ,   getstr(row, 2));
	mysql_free_result(res);


	return 0;
}

/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5109_process_init(int argc, char **argv)
{
	memset(szMode,0x00,sizeof(szMode));
	
	int ret=0;

    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("d_", "/logs/daemon"); 
    ZzLOG(ALWAY, "[daem5109]*****************프로그램 시작*****************\n");  

    // 파라미터 값 설정 및 초기화
/*    if (argc != 2) {
    	goto arg_error;
    }
*/
	/*
	** DB 연결
	*/
	if (!(con=db_connect_nodb("")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
	   	return(-1); 
	}


	if (!(con_main=db_connect_local("zangsi")))
	{
		ZzLOG(ERROR, "상용 DB에 접속하지 못 하였습니다...\n");
	   	return(-1); 
	}


	/*
	** 처리일자
	*/
	
	if( argc >= 3 ) 
	{
		strcpy(	szMode , argv[2]);
		ZzLOG(ALWAY, "%s 모드 가동\n",szMode);
	}
	
	if(  strcmp(argv[1],"00000000") == 0 )
		ret=daem5109_get_sysdate(NULL);
	else
	{
		strcpy(greg_date, argv[1]);
		ret=daem5109_get_sysdate(greg_date);
		
	}
	memcpy(greg_ym,greg_date,6);
	memcpy(g_today_ym,g_today_reg_date,6);
	

	memcpy(g_first_day,greg_date+6,2);
	
	
	if (ret < 0)
	{
		db_disconnect(con);
		db_disconnect(con_main);
		return -1;
	}
		
    return (0);

arg_error:
    ZzLOG(ERROR, "usage : %s YYYYMMDD\n", argv[0]);
    ZzLOG(ERROR, "        YYYYMMDD : 처리일자 (00000000:시스템일자)\n");

    ZzPRT(ERROR, "usage : %s YYYYMMDD\n", argv[0]);
    ZzPRT(ERROR, "        YYYYMMDD : 처리일자 (00000000:시스템일자)\n");
    return -1;
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daem5109_process_term()
{
    // DB close
	db_disconnect(con);
	
    ZzLOG(ALWAY, "[daem5109]*****************프로그램 종료*****************\n");  

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5109_signal(int nSignal)
{
    daem5109_process_term();
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
	signal(SIGTERM, daem5109_signal);
	signal(SIGINT,  daem5109_signal);
	signal(SIGQUIT, daem5109_signal);
	signal(SIGKILL, daem5109_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daem5109_process_init(argc, argv) == 0 )
	{
		/* 프로그램 메인루틴 */
		
		if( strcmp(szMode,"req") == 0 )
			rc = daem5109_process();
		else if( strcmp(szMode,"upload") == 0 )
			rc = daem5109_process_upload();
		else if ( strcmp(szMode, "mon") == 0)
		{
			daem5109_process_req_rank(); 
		}
		else if( strcmp(szMode,"all") == 0 )
		{
			rc = daem5109_process();
//			rc = daem5109_process_upload();
			rc = daem5109_process_req_rank(); 
		}


		//if( rc == 0 )
		{
//			daem5109_process_result_sum();
			
		}

		
		/* 프로그램 종료루틴 */ 
                   
		daem5109_process_term();
		
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
