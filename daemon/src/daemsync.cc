/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daemsync.cc
 *         기능 : 차단 리스트의 분류별 동기화
 *         설명 : 
 *       작성자 : HCS
 *       작성일 : 2009/10/08
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
#include <sys/stat.h>
#include <unistd.h> //for usleep();

#include "daemcom.h"
#include "commydb.h"

#define  MAX_ROWS	1
#define _DEBUG_


int daemsync_init_process(int argc, char **argv);
int daemsync_main_process();
int daemsync_serch_hash(unsigned long ulID);
int daemsync_insert_list(unsigned long ulSearchID);
char* AppendSpecialChar(char* strSrcString, char cReplace,char* pResult);
int daemsync_get_sysdate();
int daemsync_term_process();
void daemsync_signal(int nSignal);

MYSQL     *con;
MYSQL     *con_search;

char   gsys_date  [  8+1];	//등록일
char   gsys_time  [  6+1];	//등록시간
char   gsect_code [  2+1];	//분류

//******************************************************************************
//* daemsync main
//******************************************************************************
int daemsync_main_process()
{
    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "daemsync_main_process: gsys_date[%s]\n", gsys_date);  
    ZzLOG(ALWAY, "daemsync_main_process: gsys_time [%s]\n", gsys_time );  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    
	MYSQL_RES *res;
	MYSQL_ROW  row;

    char szQuery[1000];
    memset(szQuery, 0x00, sizeof(szQuery));
    
    unsigned long ulID = 0;
    
    int nCount = 0;

	while(1)
	{    
    	sprintf(szQuery, " select id "
    					 " from nori.T_CONTENTS_COPYRIGHT_HASH_INFO  "
    					 " where id > %lu and sect_code = '%s' "
    					 " order by id limit 100 "
    					 , ulID
    					 , gsect_code);
	
#ifdef __DEBUG
ZzLOG(ALWAY,"Query = [%s]\n", szQuery);
#endif    					 
		if (mysql_query(con_search, szQuery))
		{
			ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
		    ZzLOG(ERROR, "daemsync_main_process: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con_search), mysql_error(con_search));
			return -1;
		}
		
		if (!(res = mysql_store_result(con_search)))
		{
			ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
		    ZzLOG(ERROR, "daemsync_main_process: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con_search), mysql_error(con_search));
			return -1;
		}
	 	if (mysql_num_rows(res)==0)
	 	{
		    ZzLOG(ALWAY, "daemsync_main_process: 처리할 자료가 없습니다.[%d]건 성공\n", nCount);
			mysql_free_result(res);
			return 0;
		}
		while(row = mysql_fetch_row(res))
		{
			ulID = (unsigned long)getint(row,0);			

			int nRes = daemsync_serch_hash(ulID);
			
			if(nRes == 1)
			{
				continue;
			}
			else if(nRes < 0)
			{
				mysql_free_result(res);
				return nRes;
			}
			else
			{
				if(daemsync_insert_list(ulID) < 0)
				{
					mysql_free_result(res);
					return -1;
				}
				usleep(500);
				nCount++;
			}		
		}
		mysql_free_result(res);
    }
	
	ZzLOG(ALWAY, "daemsync_main_process: [%d]건 성공\n", nCount);
	return 0;
}

int daemsync_serch_hash(unsigned long ulID)
{
	MYSQL_RES *res;
	MYSQL_ROW  row;
	
	char szQuery[1000];
	memset(szQuery, 0x00, sizeof(szQuery));

   	sprintf(szQuery, " select default_hash, file_size "
   					 " from nori.T_CONTENTS_COPYRIGHT_HASH_FILE  "
   					 " where info_id = %lu  "
   					 , ulID);

#ifdef __DEBUG
ZzLOG(ALWAY,"Query = [%s]\n", szQuery);
#endif    					 
   					 
   					 
	if (mysql_query(con_search, szQuery))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daemsync_serch_hash: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con_search), mysql_error(con_search));
		return -1;
	}
	
	if (!(res = mysql_store_result(con_search)))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daemsync_serch_hash: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con_search), mysql_error(con_search));
		mysql_free_result(res);
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "daemsync_serch_hash: 해시 정보가 없습니다.(%lu)\n", ulID);
		mysql_free_result(res);
		return 1;
	}
	
	while(row = mysql_fetch_row(res))
	{
		char szDefaultHash[32+1];
		memset(szDefaultHash, 0x00, sizeof(szDefaultHash));
		
		double dFileSize = 0;
		
		strcpy(szDefaultHash, getstr(row,0));
		dFileSize = getnum(row,1);
		
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " select seq_no from zangsi.T_CONTENTS_COPYRIGHT_HASH_FILE "
						 " where default_hash = '%s' and file_size = %15.0f "
						 " limit 1 "
						 , szDefaultHash, dFileSize);

#ifdef __DEBUG
ZzLOG(ALWAY,"Query = [%s]\n", szQuery);
#endif    					 

		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
		    ZzLOG(ERROR, "daemsync_serch_hash: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
		
		if (!(res = mysql_store_result(con)))
		{
		    ZzLOG(ERROR, "daemsync_serch_hash: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			mysql_free_result(res);
			return -1;
		}
	 	if(mysql_num_rows(res)==0)
	 	{
	 		continue;
	 	}
	 	else
	 	{
			ZzLOG(ALWAY,"daemsync_serch_hash: 해시 파일 정보가 이미 존재합니다.(%s)(%15.0f)\n", szDefaultHash, dFileSize);
	 		mysql_free_result(res);
	 		return 1;
	 	}	
	}
	
	mysql_free_result(res);
	
	return 0;
}

int daemsync_insert_list(unsigned long ulSearchID)
{
	MYSQL_RES *res;
	MYSQL_ROW  row;

	char szQuery[1000];
	memset(szQuery, 0x00, sizeof(szQuery));

   	sprintf(szQuery, " select copyright_user, title, sect_code, sect_sub, proc_stat, '시스템'  "
  					 " from nori.T_CONTENTS_COPYRIGHT_HASH_INFO  "
   					 " where id = %lu "
  					 , ulSearchID);

#ifdef __DEBUG
ZzLOG(ALWAY,"Query = [%s]\n", szQuery);
#endif    					 
   					 
	if (mysql_query(con_search, szQuery))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daemsync_insert_list: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con_search), mysql_error(con_search));
		return -1;
	}
	
	if (!(res = mysql_store_result(con_search)))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daemsync_insert_list: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con_search), mysql_error(con_search));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "daemsync_insert_list: 해시 정보 조회 결과가 없습니다.(%lu)\n", ulSearchID);
		mysql_free_result(res);
		return 0;
	}
	row = mysql_fetch_row(res);

	char szCprUser[16+1];
	memset(szCprUser, 0x00, sizeof(szCprUser));
	
	char szTitle[255+1];
	memset(szTitle, 0x00, sizeof(szTitle));
	
	char szSectCode[2+1];
	memset(szSectCode, 0x00, sizeof(szSectCode));

	char szSectSub[2+1];
	memset(szSectSub, 0x00, sizeof(szSectSub));

	char szProcStat[1+1];
	memset(szProcStat, 0x00, sizeof(szProcStat));
	
	char szRegUser[16+1];
	memset(szRegUser, 0x00, sizeof(szRegUser));
	
	strcpy(szCprUser, getstr(row,0));
	AppendSpecialChar( getstr(row, 1) , '\\' , szTitle );
	strcpy(szSectCode, getstr(row,2));
	strcpy(szSectSub, getstr(row,3));
	strcpy(szProcStat, getstr(row,4));
	strcpy(szRegUser, getstr(row,5));
	
	mysql_free_result(res);
	
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " insert into zangsi.T_CONTENTS_COPYRIGHT_HASH_INFO "
					 " (copyright_user, title, sect_code, sect_sub, proc_stat, reg_user, reg_date, reg_time) "
					 " values "
					 " ('%s'          , '%s' , '%s'     , '%s'    , '%s'     , '%s'    , '%s'    , '%s') "
					 , szCprUser
					 , szTitle
					 , szSectCode
					 , szSectSub
					 , szProcStat
					 , szRegUser
					 , gsys_date
					 , gsys_time);

#ifdef __DEBUG
ZzLOG(ALWAY,"Query = [%s]\n", szQuery);
#endif    					 
					 
	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daemsync_insert_list: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con_search), mysql_error(con_search));
		return -1;
	}
	
	unsigned long ulID = 0;
	ulID = mysql_insert_id(con);
	
	memset(szQuery, 0x00, sizeof(szQuery));
   	sprintf(szQuery, " select filter_yn, mureka_yn, default_hash, audio_hash, video_hash, file_name, file_size, '시스템' "
  					 " from nori.T_CONTENTS_COPYRIGHT_HASH_FILE  "
   					 " where info_id = %lu "
  					 , ulSearchID);

#ifdef __DEBUG
ZzLOG(ALWAY,"Query = [%s]\n", szQuery);
#endif    					 
  					 
	if (mysql_query(con_search, szQuery))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daemsync_insert_list: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con_search), mysql_error(con_search));
		return -1;
	}
	
	if (!(res = mysql_store_result(con_search)))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daemsync_insert_list: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con_search), mysql_error(con_search));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "daemsync_insert_list: 해시 파일 조회 결과가 없습니다.(%lu)\n", ulSearchID);
		mysql_free_result(res);
		return 0;
	}
	while(row = mysql_fetch_row(res))
	{
		char szFileName[255+1];
		memset(szFileName, 0x00, sizeof(szFileName));
		
		AppendSpecialChar( getstr(row, 5) , '\\' , szFileName );
		
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi.T_CONTENTS_COPYRIGHT_HASH_FILE "
						 " (info_id, filter_yn, mureka_yn, default_hash, audio_hash, video_hash, file_name, file_size, reg_user, reg_date, reg_time) "
						 " values "
						 " (%lu    , '%s'     , '%s'     , '%s'        , '%s'      , '%s'      , '%s'     , %15.0f   , '%s'    , '%s'    , '%s'    ) "
						 , ulID
						 , getstr(row,0)
						 , getstr(row,1)
						 , getstr(row,2)
						 , getstr(row,3)
						 , getstr(row,4)
						 , szFileName
						 , getnum(row,6)
						 , getstr(row,7)
						 , gsys_date
						 , gsys_time);

#ifdef __DEBUG
ZzLOG(ALWAY,"Query = [%s]\n", szQuery);
#endif    					 
		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
		    ZzLOG(ERROR, "daemsync_insert_list: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			mysql_free_result(res);
			
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "delete from zangsi.T_CONTENTS_COPYRIGHT_HASH_INFO where id = %lu", ulID);

			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "daemsync_insert_list: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			}
			return -1;
		}
	}
	
	ZzLOG(ALWAY, "daemsync_main_process: %lu 입력 성공\n", ulID);
	mysql_free_result(res);	
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
int daemsync_get_sysdate()
{
	MYSQL_RES *res;
	MYSQL_ROW  row;

	char szQuery[1000];		// query string
	char sztemp [100];      // query nori

	memset(szQuery, 0x00, sizeof(szQuery));
	strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d'), date_format(now(),'%H%i%s')");

	if (mysql_query(con_search, szQuery))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "sysdate: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con_search), mysql_error(con_search));
		return -1;
	}
	
	if (!(res = mysql_store_result(con_search)))
	{
	    ZzLOG(ERROR, "sysdate: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con_search), mysql_error(con_search));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "sysdate: mysql_nurows error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con_search), mysql_error(con_search));
		mysql_free_result(res);
		return -1;
	}
	row = mysql_fetch_row(res);
	memset(gsys_date , 0x00, sizeof(gsys_date ));
	memset(gsys_time , 0x00, sizeof(gsys_time ));

	strcpy(gsys_date ,   getstr(row, 0));
	strcpy(gsys_time ,   getstr(row, 1));
	
	mysql_free_result(res);

	return 0;
}

/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daemsync_init_process(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
#ifdef __DEBUG
    ZzInitGlobalVariable2("D_daemsync", "/logs/daemon"); 
#else
    ZzInitGlobalVariable2("daemsync", "/logs/daemon"); 
#endif    

    ZzLOG(ALWAY, "[daemsync]*****************프로그램 시작*****************\n");  

    // 파라미터 값 설정 및 초기화
    if (argc != 2){
    	goto arg_error;
    }

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(con=db_connect("zangsi")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1); 
	}
	

	if (!(con_search=db_connect_search_db("nori")))
	{
		ZzLOG(ERROR, "search bck DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
		db_disconnect(con_search);
	   	return(-1); 
	}

	/* 처리일자 */
	memset(gsect_code, 0x00, sizeof(gsect_code));
	strcpy(gsect_code, argv[1]);
	ret=daemsync_get_sysdate();
	if (ret < 0)
	{
		db_disconnect(con);
		db_disconnect(con_search);
		return -1;
	}
	
    return (0);


arg_error:
    ZzLOG(ERROR, "usage : %s XX\n", argv[0]);
    ZzLOG(ERROR, "        XX(분류코드)\n", argv[0]);
    ZzPRT(ERROR, "usage : %s XX\n", argv[0]);
    ZzPRT(ERROR, "        XX(분류코드)\n", argv[0]);
    return -1;
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daemsync_term_process()
{
    // DB close
	db_disconnect(con);
	db_disconnect(con_search);
	
    ZzLOG(ALWAY, "[daemsync]*****************프로그램 종료*****************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daemsync_signal(int nSignal)
{
    daemsync_term_process();
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
	signal(SIGTERM, daemsync_signal);
	signal(SIGINT,  daemsync_signal);
	signal(SIGQUIT, daemsync_signal);
	signal(SIGKILL, daemsync_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daemsync_init_process(argc, argv) == 0 ) 
	{
		/* 프로그램 메인루틴 */
		rc = daemsync_main_process();
		/* 프로그램 종료루틴 */                    
		daemsync_term_process();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
