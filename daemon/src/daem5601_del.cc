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


char gst_date[8+1];	// 처리일자
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
		printf("날자를 입력하세요.\n");
		return -1;				
	}
	
	#ifdef __DEBUG
	printf("적용 날짜 : %s\n",gst_date);
	#endif 
	
	if( daem5601_process_db() !=0)
	{
		return -1;
	}
	/*
	if( daem5601_process_db_mobile() !=0)
	{
		return -1;
	}
	*/
	
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
int daem5601_process_db()
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
	unsigned long lFirstNo = 0;

	unsigned long lZangsiCount= 0;
	unsigned long lZangsiSumCount = 0;
	
	// lZangsiCount
	sprintf(szQuery," select count(*) from zangsi.T_DEAL_INFO where deal_date = '%s'  " , gst_date );			
	
	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "daem5601_process: ( %s )\n", szQuery);
		return -1;
	}

	if (!(res = mysql_store_result(con))) 
	{
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
	
	if (mysql_num_rows(res)==0)	
	{
		mysql_free_result(res);
		ZzLOG(ALWAY, "daem5601_process: 처리할 자료가 없습니다.\n");
		return -1;
	}
	
	if(row = mysql_fetch_row(res))
	{
		lZangsiCount = (unsigned long)getnum(row, 0);
	}
	mysql_free_result(res);

	///////
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery," select count(*) from zangsi_sum.T_DEAL_INFO where deal_date = '%s'  " , gst_date );			
	
	if (mysql_query(con_backup, szQuery))
	{
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con_backup), mysql_error(con_backup));
		ZzLOG(ERROR, "daem5601_process: ( %s )\n", szQuery);
		return -1;
	}

	if (!(res = mysql_store_result(con_backup))) 
	{
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con_backup), mysql_error(con_backup));
		return -1;
	}
	
	if (mysql_num_rows(res)==0)	
	{
		mysql_free_result(res);
		ZzLOG(ALWAY, "daem5601_process: 처리할 자료가 없습니다.\n");
		return -1;
	}
	
	if(row = mysql_fetch_row(res))
	{
		lZangsiSumCount = (unsigned long)getnum(row, 0);
	}
	mysql_free_result(res);

	printf("Date = %s [%ld ][%ld] \n",gst_date,lZangsiCount,lZangsiSumCount);
	ZzLOG(ALWAY, "Date = %s [%ld ][%ld] \n",gst_date,lZangsiCount,lZangsiSumCount);

	if(lZangsiCount != lZangsiSumCount)
	{
		printf("count check!!!!\n");
		return -1;
	}

	//20120516
	while(1) 
	{
			if (tran_begin(con)!=0)	{
			    ZzLOG(ERROR, "tran_begin1: 테이베이스 오류입니다.\n");  
				return -1;
			}

			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "DELETE FROM zangsi.T_DEAL_INFO "
							 "where deal_date = '%s' limit 100"
							 ,gst_date);

			printf("%s\n",szQuery);


			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daem5601_delete_data: DELETE T_DEAL_INFO error...\n");
				ZzLOG(ERROR, "daem5601_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "%s\n",szQuery);
				goto daem5601_process_db_err;
		    }

			//--------------------------------------------------------------------------
			// 트렌젝션종료
			//--------------------------------------------------------------------------
			
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

			dwCount++;

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
