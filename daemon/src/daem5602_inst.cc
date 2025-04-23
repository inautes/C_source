/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5602.cc
 *         기능 : 이주전 inout_amount 내역을 백업
 *         설명 : 1. 프로세스 처리 예정시간 : 05:00 
 *     설치위치 : 관리자DB에 위치
 *
 *       작성자 : HCS
 *       작성일 : 2008/07/28
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

#define  MAX_ROWS	1
#define	 NUMBER		10
#define	 DATE		11

int daem5602_process();
int daem5602_process_db();
int daem5602_process_init(int argc, char **argv);
int daem5602_process_term();
void daem5602_signal(int nSignal);


MYSQL     *con;
MYSQL_RES *res;
MYSQL_ROW  row;

MYSQL     *con_backup;


bool gbIsUserDate      ; //날짜입력
char gst_date[8+1];	// 처리일자
char gst_time[6+1];	// 처리시작시간
char ged_time[6+1];	// 처리마지막시간
char gproc_log[80]; // 로그메시지버퍼
int gnType = 0;

unsigned long gl_start_no = 0;
unsigned long gl_stop_no = 0;

//******************************************************************************
//* daem5602 main
//******************************************************************************
int daem5602_process()
{
	char szQuery[1000];  // query string
	memset(szQuery, 0x00, sizeof(szQuery));	

	// 처리일자가 "00000000"일때는 시스템일자 -14을 한다.
	if (strcmp(gst_date, "00000000") == 0)
	{
		gbIsUserDate = false;
		strcpy(szQuery, "SELECT date_format(date_add(now(), INTERVAL - 14 DAY),'%Y%m%d')");
		// 현재로부터 14일 이전 날짜를 얻는다.

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
		gbIsUserDate=true;

	}
	
	#ifdef __DEBUG
	printf("적용 날짜 : %s\n",gst_date);
	#endif 
	
	if( daem5602_process_db() !=0)
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
//* daem5602 db 처리로직
//******************************************************************************
int daem5602_process_db()
{
	char szQuery[1600];		// query string
	int  ret;
	
	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/

    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "gproc_date : [%s]\n", gst_date);  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/
	char szBuffer[600];
	unsigned long dwCount = 0;
	int nFailCnt = 0;
	
	unsigned long lFirstNo = 0;

	
	while(1) 
	{
		if( gbIsUserDate ) //하루 
		{
			sprintf(szQuery," select user_id, inout_date, seq_no, inout_time, inout_code, inout_gu, cmt_yn, r_in_amt, in_amt, out_amt, rem_amt, " 
							" inout_cmt, inout_rat, id, deal_no, proc_date, del_yn, cnl_yn " 
							" from zangsi.T_INOUT_AMOUNT "
	                        " where inout_date = '%s'           "
							" and proc_date is null					"
	                        " order by inout_date               " 
	                        "  LIMIT 100                        "
	                        , gst_date
	                        );			
		}
		else
		{
			sprintf(szQuery," select user_id, inout_date, seq_no, inout_time, inout_code, inout_gu, cmt_yn, r_in_amt, in_amt, out_amt, rem_amt, " 
							" inout_cmt, inout_rat, id, deal_no, proc_date, del_yn, cnl_yn " 
							" from zangsi.T_INOUT_AMOUNT "
	                        " where inout_date >= '20120619' and inout_date < '%s'           "
							" and proc_date is null					"
	                        " order by inout_date               " 
	                        "  LIMIT 100                        "
	                        , gst_date
	                        );			
		}
	
		ZzLOG(ALWAY,"daem5602_process [ %s ]\n",szQuery);
		
		#ifdef __DEBUG
		//printf("%s\n\n",szQuery);
		#endif
		
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5602_process: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "daem5602_process: ( %s )\n", szQuery);
			return -1;
		}
	
		if (!(res = mysql_store_result(con))) 
		{
		    ZzLOG(ERROR, "daem5602_process: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
		
	 	if (mysql_num_rows(res)==0)	
	 	{
			mysql_free_result(res);
		    ZzLOG(ALWAY, "daem5602_process: 처리할 자료가 없습니다.\n");
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
							
			if (tran_begin(con_backup)!=0)	
			{
			    ZzLOG(ERROR, "tran_begin2: 테이베이스 오류입니다. [ %d ] \n",mysql_errno(con_backup));  
			    
			    if( mysql_errno(con_backup) == 2006 )
		    	{
					if (!(con_backup=db_connect_sumdb("")))
					{
						ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
						return(-1); 
					}
					if (tran_begin(con_backup)!=0)	
					{
						ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
						ZzLOG(ERROR, "tran_begin2: 테이베이스 오류입니다.\n");  
						db_disconnect(con_backup);
						return -1;
					}
				
			    		
		    	}
		    	else
		    		return -1;
			}

			memset(szQuery, 0x00, sizeof(szQuery));
			#ifdef __DEBUG
			sprintf(szQuery, " INSERT INTO zangsi_sum.T_INOUT_AMOUNT " 
							 " (user_id, inout_date, seq_no, inout_time, inout_code, inout_gu, cmt_yn, r_in_amt, in_amt, out_amt, rem_amt, "
							 " inout_cmt, inout_rat, deal_no, id, proc_date, del_yn, cnl_yn )" 
							 " VALUES "
							 "( '%s', '%s', %ld, '%s', '%s',"
							 " '%s', '%s', %ld, %ld, %ld, "
							 "%ld, %ld, %ld, %ld, %ld,"
							 "'%s','%s','%s'); "
							 , getstr(row,0), getstr(row,1), (unsigned long)getnum(row,2), getstr(row,3), getstr(row,4)
							 , getstr(row,5), getstr(row,6), (unsigned long)getnum(row,7), (unsigned long)getnum(row,8), (unsigned long)getnum(row,9)
							 , (unsigned long)getnum(row,10), (unsigned long)getnum(row,11), (unsigned long)getnum(row,12), (unsigned long)getnum(row,13), (unsigned long)getnum(row,14)
							 , getstr(row,15), getstr(row,16), getstr(row,17));
			#else
			sprintf(szQuery, " INSERT INTO zangsi_sum.T_INOUT_AMOUNT " 
							 " (user_id, inout_date, seq_no, inout_time, inout_code, inout_gu, cmt_yn, r_in_amt, in_amt, out_amt, rem_amt, "
							 " inout_cmt, inout_rat, deal_no, id, proc_date, del_yn, cnl_yn )" 
							 " VALUES "
							 "('%s', '%s', %ld, '%s', '%s', "
							 " '%s', '%s', %ld, %ld, %ld,"
							 " %ld, %ld, %ld, %ld, %ld,"
							 " '%s','%s','%s'); "
							 , getstr(row,0), getstr(row,1), (unsigned long)getnum(row,2), getstr(row,3), getstr(row,4)
							 , getstr(row,5), getstr(row,6), (unsigned long)getnum(row,7), (unsigned long)getnum(row,8), (unsigned long)getnum(row,9)
							 , (unsigned long)getnum(row,10), (unsigned long)getnum(row,11), (unsigned long)getnum(row,12), (unsigned long)getnum(row,13), (unsigned long)getnum(row,14)
							 , getstr(row,15), getstr(row,16), getstr(row,17));
			#endif
	
			#ifdef _DEBUG
			printf("\n\n %s \n\n", szQuery);
			#endif
			
			int nMysqlErrno = 0;
			bool bIsSuceed = true;
			if (mysql_query(con_backup, szQuery))
			{
				bIsSuceed = false;
				nMysqlErrno = mysql_errno(con_backup);
			    ZzLOG(ERROR, "daem5602_delete_data: INSERT T_INOUT_AMOUNT error...\n");
				ZzLOG(ERROR, "daem5602_delete_data: [%d](%s)\n",nMysqlErrno, mysql_error(con_backup));
				ZzLOG(ERROR, "%s\n\n",szQuery);
				if( nMysqlErrno != 1062 )
					goto daem5602_process_db_err;
		    }
		    
		    if(bIsSuceed == true || (bIsSuceed == false && nMysqlErrno == 1062))
		    {
				dwCount	++;
				memset (szQuery, 0x00, sizeof(szQuery));
		
				sprintf(szQuery, "UPDATE zangsi.T_INOUT_AMOUNT SET proc_date='1' "
								 "where user_id = '%s' and inout_date = '%s' and seq_no = %ld"
								 ,getstr(row,0), getstr(row,1), (unsigned long)getnum(row, 2));
	
				if (mysql_query(con, szQuery))
				{
				    ZzLOG(ERROR, "daem5602_delete_data: UPDATE T_INOUT_AMOUNT error...\n");
					ZzLOG(ERROR, "daem5602_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
					ZzLOG(ERROR, "%s\n",szQuery);
					goto daem5602_process_db_err;
			    }
			}
			if (tran_commit(con_backup)!=0)
			{
			    ZzLOG(ERROR, "process_db2: tran_commit error...\n");
			    goto daem5602_process_db_err;
			}
			//--------------------------------------------------------------------------
			// 트렌젝션종료
			//--------------------------------------------------------------------------
			if (tran_end(con_backup)!=0)	{
			    ZzLOG(ERROR, "process_db2: tran_end 테이베이스 오류입니다.\n");  
			    goto daem5602_process_db_err;
			}
			
						

			if (tran_commit(con)!=0)
			{
			    ZzLOG(ERROR, "process_db1: tran_commit error...\n");
			    goto daem5602_process_db_err;
			}
			//--------------------------------------------------------------------------
			// 트렌젝션종료
			//--------------------------------------------------------------------------
			if (tran_end(con)!=0)	{
			    ZzLOG(ERROR, "process_db1: tran_end 테이베이스 오류입니다.\n");  
			    goto daem5602_process_db_err;
			}

		}
		mysql_free_result(res);
	}
	ZzLOG(ALWAY, "daem5602_process: 백업된 자료의 갯수는 %ld 입니다.\n",dwCount);	
	return 0;

daem5602_process_db_err:
	ZzLOG(ERROR, "process_db1: [%d](%s)\n",mysql_errno(con), mysql_error(con));

	ZzLOG(ERROR, "process_db2: [%d](%s)\n",mysql_errno(con_backup), mysql_error(con_backup));	

	tran_rollback(con_backup);
	tran_end(con_backup);
	
	tran_rollback(con);
	tran_end(con);

	mysql_free_result(res);
	ZzLOG(ALWAY, "daem5602_process: 백업된 자료의 갯수는 %ld 입니다.\n",dwCount);	
		
	return -1;	
}


/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5602_process_init(int argc, char **argv)
{
	char stemp[128];
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("d_", "/logs/daemon"); 

    ZzLOG(ALWAY, "[daem5602]*****************프로그램 시작*****************\n");  

    // 파라미터 값 설정 및 초기화
    if (argc != 2) {
    	goto arg_error;
    }

	memset(gst_date, 0x00, sizeof(gst_date));
	strcpy(gst_date, argv[1]);
	
	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(con=db_connect_local("zangsi")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1); 
	}

	if (!(con_backup=db_connect_sumdb("")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con_backup);
	   	return(-1); 
	}
		
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
int daem5602_process_term()
{
    // DB close
	db_disconnect(con);
	db_disconnect(con_backup);	
	
    ZzLOG(ALWAY, "[daem5602]*****************프로그램 종료*****************\n");  

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5602_signal(int nSignal)
{
    daem5602_process_term();
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
	signal(SIGTERM, daem5602_signal);
	signal(SIGINT,  daem5602_signal);
	signal(SIGQUIT, daem5602_signal);
	signal(SIGKILL, daem5602_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daem5602_process_init(argc, argv) == 0 ) 
	{
		/* 프로그램 메인루틴 */
		rc = daem5602_process();
		/* 프로그램 종료루틴 */                    
		daem5602_process_term();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
