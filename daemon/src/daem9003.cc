/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem9003.cc
 *         기능 : Top100 집게
 *         설명 : 5분마다 집계
 *                SYSTEM  (00000000) => sysdate - 1일 처리하며,
 *                직접입력(yyyymmdd) => yyyymmdd를  처리한다.
 *       작성자 : LEE
 *       작성일 : 2013/01/16
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

int daem9003_init_process(int argc, char **argv);
int daem9003_main_process();
int daem9003_term_process();
int daem9003_get_sysdate();
void daem9003_signal(int nSignal);

MYSQL     *con;
MYSQL     *con_log;
MYSQL     *con_bck;



char   gsys_date  [  8+1];	//처리일자(sysdate)
char   gproc_date [  8+1];	//처리일자(sysdate-1)
char   greg_date  [  8+1];	//등록일
char   greg_time  [  6+1];	//등록시간

char   gproc_query[200000];

int   delete_data()
{
	memset(gproc_query,0x00,sizeof(gproc_query));
//	sprintf(gproc_query,"delete from zangsi.T_SEARCH_TOP100 where reg_date < date_add(now(), interval -7 day) ",gproc_date);
	sprintf(gproc_query,"delete from zangsi.T_SEARCH_TOP100 where reg_date < date_add(now(), interval -2 day) ",gproc_date);
	
	if (mysql_query(con, gproc_query))
	{
	    ZzLOG(ERROR, "delete_data: mysql_query error...\n");
		ZzLOG(ERROR, "delete_data: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), gproc_query);
	    return -1;
    }
    
    
	/*
	char szQuery[1024];
	
	memset(szQuery,0x00,sizeof(szQuery));
	sprintf(szQuery,"delete from zangsi_bck.T_EVENT_POINT_STAT_1TH where reg_date='%s'\n",gproc_date);
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem9003_delete_stat_data: mysql_query error...\n");
		ZzLOG(ERROR, "daem9003_delete_stat_data: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
	    return -1;
    }
    
    memset(szQuery,0x00,sizeof(szQuery));
	sprintf(szQuery,"delete from zangsi_bck.T_EVENT_POINT_STAT where stat_date='%s'\n",gproc_date);
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem9003_delete_stat_data: mysql_query error...\n");
		ZzLOG(ERROR, "daem9003_delete_stat_data: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), szQuery);
	    return -1;
    }
    */
    return 0;
	
	
}
int make_stat()
{
	ZzLOG(ALWAY,"start make stat\n");
	MYSQL_RES* res;
	MYSQL_ROW  row;
	
	memset(gproc_query,0x00,sizeof(gproc_query));
	sprintf(gproc_query,"select " 
						" sum(case when top_type = '배너' and id = 0 then 1 end) as banner_click_cnt     "
						" ,sum(case when top_type = '전체' and id = 0 then 1 end) as type1_click_cnt     " 
						" ,sum(case when top_type = '전체' and id > 0 then 1 end) as type1_cont_cnt          " 
						" ,sum(case when top_type = '전체구매' and id > 0 then 1 end) as type1_deal_cnt      " 
						" ,sum(case when top_type = '영화' and id = 0 then 1 end) as type2_click_cnt     " 
						" ,sum(case when top_type = '영화' and id > 0 then 1 end) as type2_cont_cnt          " 
						" ,sum(case when top_type = '영화구매' and id > 0 then 1 end) as type2_deal_cnt      " 
						" ,sum(case when top_type = '드라마' and id = 0 then 1 end) as type3_click_cnt   " 
						" ,sum(case when top_type = '드라마' and id > 0 then 1 end) as type3_cont_cnt        " 
						" ,sum(case when top_type = '드라마구매' and id > 0 then 1 end) as type3_deal_cnt    " 
						" ,sum(case when top_type = '동영상' and id = 0 then 1 end) as type4_click_cnt   " 
						" ,sum(case when top_type = '동영상' and id > 0 then 1 end) as type4_cont_cnt        " 
						" ,sum(case when top_type = '동영상구매' and id > 0 then 1 end) as type4_deal_cnt    " 
						" ,sum(case when top_type = '게임' and id = 0 then 1 end) as type5_click_cnt     " 
						" ,sum(case when top_type = '게임' and id > 0 then 1 end) as type5_cont_cnt          " 
						" ,sum(case when top_type = '게임구매' and id > 0 then 1 end) as type5_deal_cnt      " 
						" ,sum(case when top_type = '애니' and id = 0 then 1 end) as type6_click_cnt     " 
						" ,sum(case when top_type = '애니' and id > 0 then 1 end) as type6_cont_cnt          " 
						" ,sum(case when top_type = '애니구매' and id > 0 then 1 end) as type6_deal_cnt      " 
						" ,sum(case when top_type = '댓글' and id = 0 then 1 end) as type7_click_cnt     " 
						" ,sum(case when top_type = '댓글' and id > 0 then 1 end) as type7_cont_cnt      " 
	                    " ,sum(case when top_type = '댓글구매' and id > 0 then 1 end) as type7_deal_cnt  "  
	                    "  ,count(distinct user_id) as user_cnt                                              "  
	                    "  from zangsi_log.T_SEARCH_TOP100_LOG                                                 "   
                        "  where reg_date = '%s'       "   ,gproc_date);
	ZzLOG(ALWAY,"[%s]\n",gproc_query);
    if (mysql_query(con_log, gproc_query))
	{
	    ZzLOG(ERROR, "daem9003_make_list: mysql_query error...\n");
		ZzLOG(ERROR, "daem9003_make_list: [%d](%s)(%s)\n",mysql_errno(con_log), mysql_error(con_log), gproc_query);
	    return -1;
    }
    
    if (!(res = mysql_store_result(con_log)))     
    {
	    ZzLOG(ERROR, "make_stat: mysql_store_result error...\n");
		ZzLOG(ERROR, "make_stat: [%d](%s)\n",mysql_errno(con_log), mysql_error(con_log));
		mysql_free_result(res);
		return -1;
	}
 	if (mysql_num_rows(res)==0)	
 	{
 		ZzLOG(ERROR, "make_stat:numrows = 0 \n");
		mysql_free_result(res);
		return -1;
	}

	long banner_click_cnt= 0;    
	long type1_click_cnt = 0;   
	long type1_cont_cnt  = 0;       
	long type1_deal_cnt  = 0;   
	long type2_click_cnt = 0;   
	long type2_cont_cnt  = 0;       
	long type2_deal_cnt  = 0;   
	long type3_click_cnt = 0; 
	long type3_cont_cnt  = 0;     
	long type3_deal_cnt  = 0; 
	long type4_click_cnt = 0; 
	long type4_cont_cnt  = 0;     
	long type4_deal_cnt  = 0; 
	long type5_click_cnt = 0;   
	long type5_cont_cnt  = 0;       
	long type5_deal_cnt  = 0;   
	long type6_click_cnt = 0;   
	long type6_cont_cnt  = 0;       
	long type6_deal_cnt  = 0;   
	long type7_click_cnt = 0;   
	long type7_cont_cnt  = 0;   
	long type7_deal_cnt  = 0;
	long user_cnt = 0;

	if (row = mysql_fetch_row(res))
	{
		banner_click_cnt = (long)getnum(row,0 );
		type1_click_cnt  = (long)getnum(row,1 );
		type1_cont_cnt   = (long)getnum(row,2 );   
		type1_deal_cnt   = (long)getnum(row,3 );
		type2_click_cnt  = (long)getnum(row,4 );
		type2_cont_cnt   = (long)getnum(row,5 );   
		type2_deal_cnt   = (long)getnum(row,6 );
		type3_click_cnt  = (long)getnum(row,7 );
		type3_cont_cnt   = (long)getnum(row,8 ); 
		type3_deal_cnt   = (long)getnum(row,9 );
		type4_click_cnt  = (long)getnum(row,10);
		type4_cont_cnt   = (long)getnum(row,11); 
		type4_deal_cnt   = (long)getnum(row,12);
		type5_click_cnt  = (long)getnum(row,13);
		type5_cont_cnt   = (long)getnum(row,14);   
		type5_deal_cnt   = (long)getnum(row,15);
		type6_click_cnt  = (long)getnum(row,16);
		type6_cont_cnt   = (long)getnum(row,17);   
		type6_deal_cnt   = (long)getnum(row,18);
		type7_click_cnt  = (long)getnum(row,19);
		type7_cont_cnt   = (long)getnum(row,20);
		type7_deal_cnt   = (long)getnum(row,21);      
		user_cnt		 = (long)getnum(row,22);      
	}
	mysql_free_result(res);
	
	memset(gproc_query,0x00,sizeof(gproc_query));
	sprintf(gproc_query,"replace into zangsi_sum.T_SEARCH_TOP100_STAT_DD  ("
			 " banner_click_cnt , type1_click_cnt  , type1_cont_cnt   , type1_deal_cnt   , type2_click_cnt  , type2_cont_cnt   , type2_deal_cnt   "
			 ", type3_click_cnt , type3_cont_cnt   , type3_deal_cnt   , type4_click_cnt  , type4_cont_cnt   , type4_deal_cnt   , type5_click_cnt  "
			 ", type5_cont_cnt  , type5_deal_cnt   , type6_click_cnt  , type6_cont_cnt   ,   type6_deal_cnt   , type7_click_cnt  , type7_cont_cnt  "
			 ", type7_deal_cnt  , user_cnt ,stat_date	) "
			 " values ( "
			 " %ld , %ld  , %ld   , %ld   , %ld  , %ld   , %ld   "
			 ", %ld , %ld   , %ld   , %ld  , %ld   , %ld   , %ld  "
			 ", %ld  , %ld   , %ld  , %ld   ,   %ld   , %ld  , %ld  "
			 ", %ld  , %ld	, '%s' ) "
			 , banner_click_cnt , type1_click_cnt  , type1_cont_cnt   , type1_deal_cnt   , type2_click_cnt  , type2_cont_cnt   , type2_deal_cnt   
			 , type3_click_cnt , type3_cont_cnt   , type3_deal_cnt   , type4_click_cnt  , type4_cont_cnt   , type4_deal_cnt   , type5_click_cnt  
			 , type5_cont_cnt  , type5_deal_cnt   , type6_click_cnt  , type6_cont_cnt   ,   type6_deal_cnt   , type7_click_cnt  , type7_cont_cnt  
			 , type7_deal_cnt  , user_cnt	, gproc_date);
	ZzLOG(ALWAY,"[%s]\n",gproc_query);

    if (mysql_query(con_bck, gproc_query))
	{
	    ZzLOG(ERROR, "daem9003_make_list: mysql_query error...\n");
		ZzLOG(ERROR, "daem9003_make_list: [%d](%s)(%s)\n",mysql_errno(con_bck), mysql_error(con_bck), gproc_query);
	    return -1;
    }	
	
	memset(gproc_query,0x00,sizeof(gproc_query));
	sprintf(gproc_query,"select " 	
						"	id "
					    "   , max(title) as title "
					    "   , sum(case when top_type in ('전체구매','영화구매','드라마구매','동영상구매','게임구매','애니구매','댓글구매') then 1 end) as top100_deal_cnt  "
					    "   , sum( case when top_type in ('전체','영화','드라마','동영상','게임','애니','댓글') and id > 0 then 1 end ) as top100_click_cnt "
						"	from zangsi_log.T_SEARCH_TOP100_LOG " 
						"	where reg_date = '%s' "
						"   and id is not null "
						"	 group by id "
						,gproc_date);
	ZzLOG(ALWAY,"[%s]\n",gproc_query);
	if (mysql_query(con_log, gproc_query))
	{
		ZzLOG(ERROR, "daem9003_make_list: mysql_query error...\n");
		ZzLOG(ERROR, "daem9003_make_list: [%d](%s)(%s)\n",mysql_errno(con_log), mysql_error(con_log), gproc_query);
		return -1;
	}
	
	if (!(res = mysql_store_result(con_log)))     
    {
	    ZzLOG(ERROR, "make_stat: mysql_store_result error...\n");
		ZzLOG(ERROR, "make_stat: [%d](%s)\n",mysql_errno(con_log), mysql_error(con_log));
		mysql_free_result(res);
		return -1;
	}
 	if (mysql_num_rows(res)==0)	
 	{
 		ZzLOG(ERROR, "make_stat:numrows = 0 \n");
		mysql_free_result(res);
		return -1;
	}

	MYSQL_RES* res2;
	MYSQL_ROW  row2;
	
	unsigned long id = 0;
	char title[8096];
	memset(title,0x00,sizeof(title));
	long top100_deal_cnt=0;
	long top100_click_cnt=0;
	long total_deal_cnt=0;
	
	while( row = mysql_fetch_row(res) )
	{
		id = (unsigned long)getnum(row,0);
		strcpy(title,getstr(row,1));
		ReplaceSingleToDouble(title);
		top100_deal_cnt = (unsigned long)getnum(row,2);
		top100_click_cnt = (unsigned long)getnum(row,3);
		
		
		memset(gproc_query,0x00,sizeof(gproc_query));
		sprintf(gproc_query,"select " 	
							"	count(id) as total_deal_cnt"
							"	from zangsi.T_DEAL_INFO " 
							"	where id = %ld and deal_date='%s'"
							,id , gproc_date);		
		ZzLOG(ALWAY,"[%s]\n",gproc_query);
		if (mysql_query(con, gproc_query))
		{
			ZzLOG(ERROR, "daem9003_make_list: mysql_query error...\n");
			ZzLOG(ERROR, "daem9003_make_list: [%d](%s)(%s)\n",mysql_errno(con_log), mysql_error(con_log), gproc_query);
			return -1;
		}
		if (!(res2 = mysql_store_result(con)))     
	    {
		    ZzLOG(ERROR, "make_stat: mysql_store_result error...\n");
			ZzLOG(ERROR, "make_stat: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			mysql_free_result(res2);
			return -1;
		}
		
		row2 = mysql_fetch_row(res2);
		total_deal_cnt = (unsigned long)getnum(row2,0);
	 	mysql_free_result(res2);
		
		
		memset(gproc_query,0x00,sizeof(gproc_query));
		sprintf(gproc_query,"replace into zangsi_sum.T_SEARCH_TOP100_CONT_STAT_DD  ("
				 " id , title, top100_deal_cnt,top100_click_cnt,total_deal_cnt ,stat_date	) "
				 " values ( " 
				 " %ld , '%s' , %ld , %ld ,%ld ,'%s' ) "
				 , id , title, top100_deal_cnt,top100_click_cnt,total_deal_cnt ,gproc_date );	
				 
		ZzLOG(ALWAY,"[%s]\n",gproc_query);
	    if (mysql_query(con_bck, gproc_query))
		{
		    ZzLOG(ERROR, "daem9003_make_list: mysql_query error...\n");
			ZzLOG(ERROR, "daem9003_make_list: [%d](%s)(%s)\n",mysql_errno(con_bck), mysql_error(con_bck), gproc_query);
		    return -1;
	    }	
				
		
	}

	mysql_free_result(res);

	ZzLOG(ALWAY,"통계가 완료 되었습니다.\n");
	return 0;

}

int   make_list()
{
	ZzLOG(ALWAY,"start make list\n");
	//delete from zangsi.T_SEARCH_TOP100 where reg_date < date_add(now(), interval -1 day);        
	
	memset(gproc_query,0x00,sizeof(gproc_query));
	sprintf(gproc_query," insert into zangsi.T_SEARCH_TOP100(top_type,id,title,file_size,cnt,reg_date)"
						" select '전체' as top_type"
						"        , a.id "
						"        , c.title"
						"        , if(b.file_size/1024/1024<999, concat(round(b.file_size/1024/1024,1),' M'), concat(round(b.file_size/1024/1024/1024,1),' G')) as file_size"
						"        , a.cur_user_cnt as dn_cnt "
						"        ,now() "
						" from zangsi.T_CONTENTS_FILE_USER_CNT a , zangsi.T_CONTENTS_FILE b , zangsi.T_PERM_UPLOAD_AUTH e ,zangsi.T_CONTENTS_VIR_ID f "
						" , zangsi.T_CONTENTS_INFO c use index (PRIMARY) left outer join zangsi.T_SEARCH_TOP100_DEL d on c.id = d.id AND d.top_type = '전체'"
						" where a.id = c.id"
						" and c.id = f.id "
						" and f.copyright_yn in('N','H','C') "
						" and c.id = b.id"
						" and c.reg_user = e.user_id"
						" and a.cont_gu = 'WE'"
						" and c.sect_code in ('01','02','03','04','05')"
						" and a.cur_user_cnt >= 0 "
						" and d.id is null"
						" and e.auth_num <> 'CPR'"
						" order by dn_cnt desc, a.id desc  "
						" limit 200  "
						);
	ZzLOG(ALWAY," [ %s ] \n",gproc_query);
	if (mysql_query(con, gproc_query))
	{
	    ZzLOG(ERROR, "daem9003_make_list: mysql_query error...\n");
		ZzLOG(ERROR, "daem9003_make_list: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), gproc_query);
	    return -1;
    }
    
    memset(gproc_query,0x00,sizeof(gproc_query));
	sprintf(gproc_query," insert into zangsi.T_SEARCH_TOP100(top_type,id,title,file_size,cnt,reg_date)                                                                        "
						" select '영화' as top_type                                                                                                                         "
						"        , a.id                                                                                                                                     "
						"        , c.title                                                                                                                                  "
						"        , if(b.file_size/1024/1024<999, concat(round(b.file_size/1024/1024,1),' M'), concat(round(b.file_size/1024/1024/1024,1),' G')) as file_size"
						"        , a.cur_user_cnt as dn_cnt                                                                                                                 "
						"        ,now()                                                                                                                     "
						" from zangsi.T_CONTENTS_FILE_USER_CNT a , zangsi.T_CONTENTS_FILE b , zangsi.T_PERM_UPLOAD_AUTH e ,zangsi.T_CONTENTS_VIR_ID f                                                         "
						" , zangsi.T_CONTENTS_INFO c use index (PRIMARY) left outer join zangsi.T_SEARCH_TOP100_DEL d on c.id = d.id AND d.top_type = '영화'                    "
						" where a.id = c.id"
						" and c.id = f.id "
						" and f.copyright_yn in('N','H','C') "
						" and c.id = b.id                                                                                                                                   "
						" and c.reg_user = e.user_id                                                                                                                        "
						" and a.cont_gu = 'WE'                                                                                                                              "
						" and c.sect_code in ('01')                                                                                                                         "
						" and a.cur_user_cnt >= 0                                                                                                                            "
						" and d.id is null                                                                                                                                  "
						" and e.auth_num <> 'CPR'                                                                                                                           "
						" order by dn_cnt desc, a.id desc                                                                                                                               "
						" limit 200 ;                                                                                                                                       "
						);
	
	ZzLOG(ALWAY," [ %s ] \n",gproc_query);
	if (mysql_query(con, gproc_query))
	{
	    ZzLOG(ERROR, "daem9003_make_list: mysql_query error...\n");
		ZzLOG(ERROR, "daem9003_make_list: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), gproc_query);
	    return -1;
    }
    
    memset(gproc_query,0x00,sizeof(gproc_query));
	sprintf(gproc_query," insert into zangsi.T_SEARCH_TOP100(top_type,id,title,file_size,cnt,reg_date)                                                                        "
						" select '드라마' as top_type                                                                                                                         "
						"        , a.id                                                                                                                                     "
						"        , c.title                                                                                                                                  "
						"        , if(b.file_size/1024/1024<999, concat(round(b.file_size/1024/1024,1),' M'), concat(round(b.file_size/1024/1024/1024,1),' G')) as file_size"
						"        , a.cur_user_cnt as dn_cnt                                                                                                                 "
						"        ,now()                                                                                                                     "
						" from zangsi.T_CONTENTS_FILE_USER_CNT a , zangsi.T_CONTENTS_FILE b , zangsi.T_PERM_UPLOAD_AUTH e ,zangsi.T_CONTENTS_VIR_ID f                               "
						" , zangsi.T_CONTENTS_INFO c use index (PRIMARY) left outer join zangsi.T_SEARCH_TOP100_DEL d on c.id = d.id AND d.top_type = '드라마'                    "
						" where a.id = c.id"
						" and c.id = f.id "
						" and f.copyright_yn in('N','H','C') "
						" and c.id = b.id                                                                                                                                   "
						" and c.reg_user = e.user_id                                                                                                                        "
						" and a.cont_gu = 'WE'                                                                                                                              "
						" and c.sect_code in ('02')                                                                                                                         "
						" and a.cur_user_cnt >= 0                                                                                                                            "
						" and d.id is null                                                                                                                                  "
						" and e.auth_num <> 'CPR'                                                                                                                           "
						" order by dn_cnt desc, a.id desc                                                                                                                   "
						" limit 200 ;                                                                                                                                       "
						);
	
	ZzLOG(ALWAY," [ %s ] \n",gproc_query);
	if (mysql_query(con, gproc_query))
	{
	    ZzLOG(ERROR, "daem9003_make_list: mysql_query error...\n");
		ZzLOG(ERROR, "daem9003_make_list: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), gproc_query);
	    return -1;
    }
    
     memset(gproc_query,0x00,sizeof(gproc_query));
	sprintf(gproc_query," insert into zangsi.T_SEARCH_TOP100(top_type,id,title,file_size,cnt,reg_date)                                                                        "
						" select '동영상' as top_type                                                                                                                         "
						"        , a.id                                                                                                                                     "
						"        , c.title                                                                                                                                  "
						"        , if(b.file_size/1024/1024<999, concat(round(b.file_size/1024/1024,1),' M'), concat(round(b.file_size/1024/1024/1024,1),' G')) as file_size"
						"        , a.cur_user_cnt as dn_cnt                                                                                                                 "
						"        ,now()                                                                                                                     "
						" from zangsi.T_CONTENTS_FILE_USER_CNT a , zangsi.T_CONTENTS_FILE b , zangsi.T_PERM_UPLOAD_AUTH e ,zangsi.T_CONTENTS_VIR_ID f                               "
						" , zangsi.T_CONTENTS_INFO c use index (PRIMARY) left outer join zangsi.T_SEARCH_TOP100_DEL d on c.id = d.id AND d.top_type = '동영상'                   "
						" where a.id = c.id"
						" and c.id = f.id "
						" and f.copyright_yn in('N','H','C') "
						" and c.id = b.id                                                                                                                                   "
						" and c.reg_user = e.user_id                                                                                                                        "
						" and a.cont_gu = 'WE'                                                                                                                              "
						" and c.sect_code in ('03')                                                                                                                         "
						" and a.cur_user_cnt >= 0                                                                                                                            "
						" and d.id is null                                                                                                                                  "
						" and e.auth_num <> 'CPR'                                                                                                                           "
						" order by dn_cnt desc, a.id desc                                                                                                                   "
						" limit 200 ;                                                                                                                                       "
						);
	
	ZzLOG(ALWAY," [ %s ] \n",gproc_query);
	if (mysql_query(con, gproc_query))
	{
	    ZzLOG(ERROR, "daem9003_make_list: mysql_query error...\n");
		ZzLOG(ERROR, "daem9003_make_list: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), gproc_query);
	    return -1;
    }
    
     memset(gproc_query,0x00,sizeof(gproc_query));
	sprintf(gproc_query," insert into zangsi.T_SEARCH_TOP100(top_type,id,title,file_size,cnt,reg_date)                                                                        "
						" select '게임' as top_type                                                                                                                         "
						"        , a.id                                                                                                                                     "
						"        , c.title                                                                                                                                  "
						"        , if(b.file_size/1024/1024<999, concat(round(b.file_size/1024/1024,1),' M'), concat(round(b.file_size/1024/1024/1024,1),' G')) as file_size"
						"        , a.cur_user_cnt as dn_cnt                                                                                                                 "
						"        ,now()                                                                                                                     "
						" from zangsi.T_CONTENTS_FILE_USER_CNT a , zangsi.T_CONTENTS_FILE b , zangsi.T_PERM_UPLOAD_AUTH e ,zangsi.T_CONTENTS_VIR_ID f                               "
						" , zangsi.T_CONTENTS_INFO c use index (PRIMARY) left outer join zangsi.T_SEARCH_TOP100_DEL d on c.id = d.id AND d.top_type = '게임'                    "
						" where a.id = c.id"
						" and c.id = f.id "
						" and f.copyright_yn in('N','H','C') "
						" and c.id = b.id                                                                                                                                   "
						" and c.reg_user = e.user_id                                                                                                                        "
						" and a.cont_gu = 'WE'                                                                                                                              "
						" and c.sect_code in ('04')                                                                                                                         "
						" and a.cur_user_cnt >= 0                                                                                                                            "
						" and d.id is null                                                                                                                                  "
						" and e.auth_num <> 'CPR'                                                                                                                           "
						" order by dn_cnt desc, a.id desc                                                                                                                   "
						" limit 200 ;                                                                                                                                       "
						);
	
	ZzLOG(ALWAY," [ %s ] \n",gproc_query);
	if (mysql_query(con, gproc_query))
	{
	    ZzLOG(ERROR, "daem9003_make_list: mysql_query error...\n");
		ZzLOG(ERROR, "daem9003_make_list: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), gproc_query);
	    return -1;
    }
    memset(gproc_query,0x00,sizeof(gproc_query));
	sprintf(gproc_query," insert into zangsi.T_SEARCH_TOP100(top_type,id,title,file_size,cnt,reg_date)                                                                        "
						" select '애니' as top_type                                                                                                                         "
						"        , a.id                                                                                                                                     "
						"        , c.title                                                                                                                                  "
						"        , if(b.file_size/1024/1024<999, concat(round(b.file_size/1024/1024,1),' M'), concat(round(b.file_size/1024/1024/1024,1),' G')) as file_size"
						"        , a.cur_user_cnt as dn_cnt                                                                                                                 "
						"        ,now()                                                                                                                     "
						" from zangsi.T_CONTENTS_FILE_USER_CNT a , zangsi.T_CONTENTS_FILE b , zangsi.T_PERM_UPLOAD_AUTH e ,zangsi.T_CONTENTS_VIR_ID f                               "
						" , zangsi.T_CONTENTS_INFO c use index (PRIMARY) left outer join zangsi.T_SEARCH_TOP100_DEL d on c.id = d.id AND d.top_type = '애니'                    "
						" where a.id = c.id"
						" and c.id = f.id "
						" and f.copyright_yn in('N','H','C') "
						" and c.id = b.id                                                                                                                                   "
						" and c.reg_user = e.user_id                                                                                                                        "
						" and a.cont_gu = 'WE'                                                                                                                              "
						" and c.sect_code in ('05')                                                                                                                         "
						" and a.cur_user_cnt >= 0                                                                                                                           "
						" and d.id is null                                                                                                                                  "
						" and e.auth_num <> 'CPR'                                                                                                                           "
						" order by dn_cnt desc, a.id desc                                                                                                                   "
						" limit 200 ;                                                                                                                                       "
						);
	
	ZzLOG(ALWAY," [ %s ] \n",gproc_query);
	if (mysql_query(con, gproc_query))
	{
	    ZzLOG(ERROR, "daem9003_make_list: mysql_query error...\n");
		ZzLOG(ERROR, "daem9003_make_list: [%d](%s)(%s)\n",mysql_errno(con), mysql_error(con), gproc_query);
	    return -1;
    }

	return 0;
	
    
}
//******************************************************************************
//* daem9003 main
//******************************************************************************
int daem9003_main_process()
{
	// 리스트 생성
	

}



/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem9003_get_sysdate()
{
	MYSQL_RES* res;
	MYSQL_ROW  row;
	
	char szQuery[1000];		// query string
	char sztemp [100];      // query temp

	memset(szQuery, 0x00, sizeof(szQuery));
	
	if( strlen(gsys_date) > 0 )
	{
		if (strcmp(gsys_date, "00000000")==0)
		{
			strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d')");
			strcat(szQuery, "     , date_format(now(),'%H%i%s')");
			strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -1 DAY),'%Y%m%d')");
		}
		else
		{
			sprintf(szQuery, "SELECT date_format(now(),'%%Y%%m%%d')"
						 "     , date_format(now(),'%%H%%i%%s')"
						 "     , '%s' "
						 , gsys_date);
						 
			
		}
	}
	else
	{
		sprintf(szQuery, "SELECT date_format(now(),'%%Y%%m%%d')"
						 "     , date_format(now(),'%%H%%i%%s')"
						 "     , date_format(now(),'%%Y%%m%%d')"
						 );
	}
	ZzLOG(ALWAY,"%s\n",szQuery);
	if (mysql_query(con, szQuery))
	{
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
	memset(greg_date , 0x00, sizeof(greg_date ));
	memset(greg_time , 0x00, sizeof(greg_time ));
	memset(gproc_date, 0x00, sizeof(gproc_date));

	strcpy(greg_date ,   getstr(row, 0));
	strcpy(greg_time ,   getstr(row, 1));
	strcpy(gproc_date,   getstr(row, 2));
	
	mysql_free_result(res);

	ZzLOG(ALWAY,"데이터를 생성할 날짜 [ %s ]\n",gproc_date);
	
	return 0;
}

/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem9003_init_process(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
    
    ZzInitGlobalVariable2("daem9003", "/logs/daemon"); 
    
    
    ZzLOG(ALWAY, "[daem9003]***************프로그램 시작***************\n");  

  
	

	if( argc > 1  )
	{
		if( argc != 3 )
		{
			ZzLOG(ALWAY,"잘못된 사용 방식입니다.\n");
			return -1;
		}
	}
		
	/* 처리일자 */
	memset(gsys_date, 0x00, sizeof(gsys_date));
	if( argc  == 3 && strcmp(argv[1] ,"stat")==0 )
	{
		//connect to sum
		//if (!(con_bck=db_connect_sumdb("zangsi_sum")))
		if (!(con_bck=db_connect_backup("zangsi_sum")))
		{
			ZzLOG(ERROR, "SUM DB에 접속하지 못 하였습니다...\n");
		   	return(-1); 
		}
		 
		
		//connect to log
		if (!(con_log=db_connect_logdb("zangsi_log")))
		{
			ZzLOG(ERROR, "LOG DB에 접속하지 못 하였습니다...\n");
		   	return(-1); 
		}
		strcpy(gsys_date, argv[2]);
	}
	//connect to main
	if (!(con=db_connect("zangsi")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
	   	return(-1); 
	}
	ret=daem9003_get_sysdate();
	if (ret < 0){
		db_disconnect(con);
		db_disconnect(con_bck);
		db_disconnect(con_log);
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
int daem9003_term_process()
{
    // DB close
	db_disconnect(con);
	db_disconnect(con_bck);
	db_disconnect(con_log);
	ZzLOG(ALWAY, "[daem9003]***************프로그램 종료***************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem9003_signal(int nSignal)
{
    daem9003_term_process();
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
	signal(SIGTERM, daem9003_signal);
	signal(SIGINT,  daem9003_signal);
	signal(SIGQUIT, daem9003_signal);
	signal(SIGKILL, daem9003_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);

	
	if ( daem9003_init_process(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		ZzLOG(ALWAY,"argc[%d]\n",argc);
		if( argc  == 3 && strcmp(argv[1] ,"stat")==0)
		{
			delete_data();
			rc = make_stat();
		}
		else if( argc == 1 )
		{
			rc = make_list();
		}
		/* 프로그램 종료루틴 */                    
		daem9003_term_process();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
