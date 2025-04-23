/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5501.cc
 *         기능 : 30일이전의 받은쪽지 삭제 및 메일서버로 백업
 *         설명 : 1. 프로세스 처리 예정시간 : 05:00 
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

#define  MAX_ROWS	1


int daem5501_process();
int daem5501_process_db();
int daem5501_process_db_recv();
int daem5501_process_init(int argc, char **argv);
int daem5501_process_term();
void daem5501_signal(int nSignal);


MYSQL     *con;
MYSQL_RES *res;
MYSQL_ROW  row;

MYSQL     *con_backup;


bool gbIsUserDate      ; //날짜입력
char gst_date[8+1];	// 처리일자
char gst_recv_date[8+1];	// 처리일자

char gst_time[6+1];	// 처리시작시간
char ged_time[6+1];	// 처리마지막시간
char gproc_log[80]; // 로그메시지버퍼



//******************************************************************************
//* daem5501 main
//******************************************************************************
int daem5501_process()
{
	char szQuery[1000];  // query string
	memset(szQuery, 0x00, sizeof(szQuery));	

	// 처리일자가 "00000000"일때는 시스템일자 -3을 한다.
	if (strcmp(gst_date, "00000000") == 0)
	{
		gbIsUserDate = false;
		strcpy(szQuery, "SELECT date_format(date_add(now(), INTERVAL -30 DAY),'%Y%m%d'), date_format(date_add(now(), INTERVAL -14 DAY),'%Y%m%d')");
		// 현재로부터 30일 이전 날짜를 얻는다.

		if (mysql_query(con, szQuery)){
		    ZzLOG(ERROR, "[daem5501]sysdate: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
		
		if (!(res = mysql_store_result(con)))
		{
		    ZzLOG(ERROR, "[daem5501]sysdate: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
	 	if (mysql_num_rows(res)==0)
	 	{
		    ZzLOG(ERROR, "[daem5501]sysdate: mysql_num_rows error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
		row = mysql_fetch_row(res);
		memset(gst_date, 0x00, sizeof(gst_date));
		strcpy(gst_date,   getstr(row, 0));

		memset(gst_recv_date, 0x00, sizeof(gst_recv_date));
		strcpy(gst_recv_date,   getstr(row, 1));
		mysql_free_result(res);
				
	}
	else
	{
		gbIsUserDate=true;

	}



	
	#ifdef __DEBUG
	printf("적용 날짜 : %s\n",gst_date);
	#endif 



	
	if( daem5501_process_db() !=0)
	{
		return -1;
	}	

	//읽은 편지 처리
	if( daem5501_process_db_recv() !=0)
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
//* daem5501 db 처리로직
//******************************************************************************
int daem5501_process_db()
{
	char szQuery[131000];		// query string
	int  ret;
	
	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/

    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "[daem5501]gproc_date : [%s]\n", gst_date);  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/
	
	char szBuffer[130000];
	unsigned long dwCount = 0;	
	
	int nLimitCount = 0;
	
	while(1) 
	{
		
		memset (szQuery, 0x00, sizeof(szQuery));

		if( gbIsUserDate ) //하루 
		{
			sprintf(szQuery,"select * from zangsi.T_MEMO_INFO"
	                        " where recv_date = '%s'          "
	                        " order by recv_date             " 
	                        "  LIMIT 100                     "
	                        , gst_date
	                        );			
		}
		else
		{
			sprintf(szQuery,"select * from zangsi.T_MEMO_INFO"
	                        " where recv_date <= '%s'     "
	                        " order by recv_date             " 
	                        "  LIMIT 100                     "
	                        , gst_date                       
	                        );
                        			
		}


//		ZzLOG(ERROR, "%s\n\n",szQuery);
		
		#ifdef __DEBUG
		printf("%s\n\n",szQuery);
		#endif
		
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5501_process: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
	
		if (!(res = mysql_store_result(con))) {
		    ZzLOG(ERROR, "daem5501_process: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
	 	if (mysql_num_rows(res)==0)	{
			mysql_free_result(res);
		    ZzLOG(ALWAY, "daem5501_process: 처리할 자료가 없습니다.\n[%s]", szQuery);
		    #ifdef __DEBUG
		    printf("daem5501_process: 처리할 자료가 없습니다.\n[%s]", szQuery);
		    #endif
			break;
		}

//	    ZzLOG(ALWAY, "daem5501_process: select_cnt(%d)\n", mysql_num_rows(res));
	

		//--------------------------------------------------------------------------
		// 트렌젝션시작
		//--------------------------------------------------------------------------

		while((row = mysql_fetch_row(res))) 
		{
			if(strcmp(getstr(row, 7),gst_date) >= 0)
			{
				mysql_free_result(res);
			    ZzLOG(ALWAY, "daem5501_process: 처리할 자료가 없습니다.\n[%s]\n", getstr(row, 7));
			    #ifdef __DEBUG
			    printf("daem5501_process: 처리할 자료가 없습니다.\n[%s]\n", getstr(row, 7));
				printf("daem5503_process: 백업된 자료의 갯수는 %ld 입니다.\n",dwCount);		
			    #endif
				ZzLOG(ALWAY, "daem5503_process: 백업된 자료의 갯수는 %ld 입니다.\n",dwCount);		
				return 0;
			}
			if(strcmp(getstr(row, 6), "N") == 0 && strcmp(getstr(row, 7),gst_recv_date) >= 0)
			{
				#ifdef __DEBUG
				printf("\nrecv_yn = [%s], recv_date = [%s], datedate = [%s]\n", getstr(row, 6), getstr(row, 7), gst_recv_date);
				#endif
				continue;
			}
			
			
			dwCount ++ ;
			
			if (tran_begin(con)!=0)	{
			    ZzLOG(ERROR, "[daem5501]tran_begin1: 테이베이스 오류입니다.\n");  
				return -1;
			}
							
			if (tran_begin(con_backup)!=0)	{
			    ZzLOG(ERROR, "[daem5501]tran_begin2: 테이베이스 오류입니다.\n");  
				return -1;
			}
	
			// 받은 편지 삭제 30일 이전.
			

			memset (szQuery, 0x00, sizeof(szQuery));
			memset(szBuffer,0x00,sizeof(szBuffer));
			AppendSpecialChar( getstr(row, 4) , '\\' , szBuffer );
			

			sprintf(szQuery, "INSERT INTO zangsi_bck.T_MEMO_INFO (user_id, seq_no, memo_cd, ref_id, "
							 "descript, send_user, recv_yn, recv_date, recv_time) VALUES ( "
							 "'%s',%ld,'%s',%ld,'%s ','%s','%s','%s','%s')"
							 , getstr(row, 0),(unsigned long)getnum(row, 1),getstr(row, 2),(unsigned long)getnum(row, 3)
							 , szBuffer, getstr(row, 5), getstr(row, 6), getstr(row, 7), getstr(row, 8));			
			
			#ifdef __DEBUG
			//printf("%s\n",szQuery);
			#endif		

		
							 
			if (mysql_query(con_backup, szQuery))
			{
			    ZzLOG(ERROR, "daem5501_delete_data: INSERT T_MEMO_INFO error...\n");
				ZzLOG(ERROR, "daem5501_delete_data: [%d](%s)\n",mysql_errno(con_backup), mysql_error(con_backup));
				ZzLOG(ERROR, "%s\n",szQuery);
				goto daem5501_process_db_err;
		    }
		    


						
			memset (szQuery, 0x00, sizeof(szQuery));
	
			sprintf(szQuery, "DELETE FROM zangsi.T_MEMO_INFO "
							 "where user_id = '%s' and seq_no = %ld"
							 ,getstr(row, 0),(unsigned long)getnum(row, 1));

			
			
			#ifdef __DEBUG
			//printf("%s\n",szQuery);			 
			#endif
										 			


			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daem5501_delete_data: DELETE T_MEMO_INFO error...\n");
				ZzLOG(ERROR, "daem5501_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "%s\n",szQuery);
				goto daem5501_process_db_err;
		    }
		    							 			

			if (tran_commit(con_backup)!=0)
			{
			    ZzLOG(ERROR, "[daem5501]process_db2: tran_commit error...\n");
			    goto daem5501_process_db_err;
			}
			//--------------------------------------------------------------------------
			// 트렌젝션종료
			//--------------------------------------------------------------------------
			if (tran_end(con_backup)!=0)	{
			    ZzLOG(ERROR, "[daem5501]process_db2: tran_end 테이베이스 오류입니다.\n");  
			    goto daem5501_process_db_err;
			}
			
						

			if (tran_commit(con)!=0)
			{
			    ZzLOG(ERROR, "[daem5501]process_db1: tran_commit error...\n");
			    goto daem5501_process_db_err;
			}
			//--------------------------------------------------------------------------
			// 트렌젝션종료
			//--------------------------------------------------------------------------
			if (tran_end(con)!=0)	{
			    ZzLOG(ERROR, "[daem5501]process_db1: tran_end 테이베이스 오류입니다.\n");  
			    goto daem5501_process_db_err;
			}
		}
		
		mysql_free_result(res);
		nLimitCount += 100;
	}
	ZzLOG(ALWAY, "daem5503_process: 백업된 자료의 갯수는 %ld 입니다.\n",dwCount);	
	#ifdef __DEBUG
	printf("daem5503_process: 백업된 자료의 갯수는 %ld 입니다.\n",dwCount);		
	#endif	
	return 0;

daem5501_process_db_err:
	ZzLOG(ERROR, "[daem5501]process_db1: [%d](%s)\n",mysql_errno(con), mysql_error(con));

	ZzLOG(ERROR, "[daem5501]process_db2: [%d](%s)\n",mysql_errno(con_backup), mysql_error(con_backup));	
	
	tran_rollback(con_backup);
	tran_end(con_backup);
	
	tran_rollback(con);
	tran_end(con);

	ZzLOG(ALWAY, "daem5503_process: 백업된 자료의 갯수는 %ld 입니다.\n",dwCount);	
		
	return -1;	
}

//******************************************************************************
//* daem5501 db 처리로직(읽은 편지)
//******************************************************************************
int daem5501_process_db_recv()
{
	char szQuery[131000];		// query string
	int  ret;


	
	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/
	

    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "[daem5501_process_db_recv]gproc_date : [%s]\n", gst_recv_date);  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/
	
	char szBuffer[130000];
	unsigned long dwCount = 0;	
	
	int nLimitCount = 0;
	
	while(1) 
	{
		
		memset (szQuery, 0x00, sizeof(szQuery));

		if( gbIsUserDate ) //하루 
		{
			sprintf(szQuery,"select * from zangsi.T_MEMO_INFO"
	                        " where recv_date = '%s' and recv_yn = 'Y'         "
	                        " order by recv_date             " 
	                        "  LIMIT 100                     "
	                        , gst_recv_date
	                        );			
		}
		else
		{
			sprintf(szQuery,"select * from zangsi.T_MEMO_INFO"
	                        " where recv_date <= '%s' and recv_yn = 'Y'       "
	                        " order by recv_date             " 
	                        "  LIMIT 100                     "
	                        , gst_recv_date                       
	                        );
                        			
		}


//		ZzLOG(ERROR, "%s\n\n",szQuery);
		
		#ifdef __DEBUG
		printf("%s\n\n",szQuery);
		#endif
		
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5501_process_db_recv: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
	
		if (!(res = mysql_store_result(con))) {
		    ZzLOG(ERROR, "daem5501_process_db_recv: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
			return -1;
		}
	 	if (mysql_num_rows(res)==0)	{
			mysql_free_result(res);
		    ZzLOG(ALWAY, "daem5501_process_db_recv: 처리할 자료가 없습니다.\n[%s]", szQuery);
		    #ifdef __DEBUG
		    printf("daem5501_process_db_recv: 처리할 자료가 없습니다.\n[%s]", szQuery);
		    #endif
			break;
		}

//	    ZzLOG(ALWAY, "daem5501_process: select_cnt(%d)\n", mysql_num_rows(res));
	

		//--------------------------------------------------------------------------
		// 트렌젝션시작
		//--------------------------------------------------------------------------

		while((row = mysql_fetch_row(res))) 
		{
			
			dwCount ++ ;
			
			if (tran_begin(con)!=0)	{
			    ZzLOG(ERROR, "[daem5501_process_db_recv]tran_begin1: 테이베이스 오류입니다.\n");  
				return -1;
			}
							
			if (tran_begin(con_backup)!=0)	{
			    ZzLOG(ERROR, "[daem5501_process_db_recv]tran_begin2: 테이베이스 오류입니다.\n");  
				return -1;
			}
	
			//읽은 편지 삭제 14일 이전.
			

			memset (szQuery, 0x00, sizeof(szQuery));
			memset(szBuffer,0x00,sizeof(szBuffer));
			AppendSpecialChar( getstr(row, 4) , '\\' , szBuffer );
			

			sprintf(szQuery, "INSERT INTO zangsi_bck.T_MEMO_INFO (user_id, seq_no, memo_cd, ref_id, "
							 "descript, send_user, recv_yn, recv_date, recv_time) VALUES ( "
							 "'%s',%ld,'%s',%ld,'%s ','%s','%s','%s','%s')"
							 , getstr(row, 0),(unsigned long)getnum(row, 1),getstr(row, 2),(unsigned long)getnum(row, 3)
							 , szBuffer, getstr(row, 5), getstr(row, 6), getstr(row, 7), getstr(row, 8));			
			
			#ifdef __DEBUG
			//printf("%s\n",szQuery);
			#endif		

		
							 
			if (mysql_query(con_backup, szQuery))
			{
			    ZzLOG(ERROR, "daem5501_process_db_recv: INSERT T_MEMO_INFO error...\n");
				ZzLOG(ERROR, "daem5501_process_db_recv: [%d](%s)\n",mysql_errno(con_backup), mysql_error(con_backup));
				ZzLOG(ERROR, "%s\n",szQuery);
				goto daem5501_process_db_err;
		    }
		    


						
			memset (szQuery, 0x00, sizeof(szQuery));
	
			sprintf(szQuery, "DELETE FROM zangsi.T_MEMO_INFO "
							 "where user_id = '%s' and seq_no = %ld"
							 ,getstr(row, 0),(unsigned long)getnum(row, 1));

			
			
			#ifdef __DEBUG
			//printf("%s\n",szQuery);			 
			#endif
										 			


			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daem5501_process_db_recv: DELETE T_MEMO_INFO error...\n");
				ZzLOG(ERROR, "daem5501_process_db_recv: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "%s\n",szQuery);
				goto daem5501_process_db_err;
		    }
		    							 			

			if (tran_commit(con_backup)!=0)
			{
			    ZzLOG(ERROR, "[daem5501_process_db_recv]process_db2: tran_commit error...\n");
			    goto daem5501_process_db_err;
			}
			//--------------------------------------------------------------------------
			// 트렌젝션종료
			//--------------------------------------------------------------------------
			if (tran_end(con_backup)!=0)	{
			    ZzLOG(ERROR, "[daem5501_process_db_recv]process_db2: tran_end 테이베이스 오류입니다.\n");  
			    goto daem5501_process_db_err;
			}
			
						

			if (tran_commit(con)!=0)
			{
			    ZzLOG(ERROR, "[daem5501_process_db_recv]process_db1: tran_commit error...\n");
			    goto daem5501_process_db_err;
			}
			//--------------------------------------------------------------------------
			// 트렌젝션종료
			//--------------------------------------------------------------------------
			if (tran_end(con)!=0)	{
			    ZzLOG(ERROR, "[daem5501_process_db_recv]process_db1: tran_end 테이베이스 오류입니다.\n");  
			    goto daem5501_process_db_err;
			}
			

									
			
			
		}
		
		mysql_free_result(res);
		nLimitCount += 100;
	}
	ZzLOG(ALWAY, "daem5501_process_db_recv: 백업된 자료의 갯수는 %ld 입니다.\n",dwCount);	
	#ifdef __DEBUG
	printf("daem5501_process_db_recv: 백업된 자료의 갯수는 %ld 입니다.\n",dwCount);		
	#endif	
	return 0;

daem5501_process_db_err:
	ZzLOG(ERROR, "[daem5501_process_db_recv]process_db1: [%d](%s)\n",mysql_errno(con), mysql_error(con));

	ZzLOG(ERROR, "[daem5501_process_db_recv]process_db2: [%d](%s)\n",mysql_errno(con_backup), mysql_error(con_backup));	
	
	tran_rollback(con_backup);
	tran_end(con_backup);
	
	tran_rollback(con);
	tran_end(con);

	ZzLOG(ALWAY, "daem5501_process_db_recv: 백업된 자료의 갯수는 %ld 입니다.\n",dwCount);	
		
	return -1;	
}

/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5501_process_init(int argc, char **argv)
{
	char stemp[128];
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("d_", "/logs/daemon"); 

    ZzLOG(ALWAY, "[daem5501]*****************프로그램 시작*****************\n");  

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
	if (!(con=db_connect("zangsi")))
	{
		ZzLOG(ERROR, "[daem5501]DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1); 
	}

	if (!(con_backup=db_connect_backup("zangsi_bck")))
	{
		ZzLOG(ERROR, "[daem5501]DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con_backup);
	   	return(-1); 
	}
		
    return (0);

arg_error:
    ZzLOG(ERROR, "usage : %s YYYYMMDD NN\n", argv[0]);
    ZzLOG(ERROR, "        YYYYMMDD : 처리일자(00000000:시스템일자)\n");

    ZzPRT(ERROR, "usage : %s YYYYMMDD NN\n", argv[0]);
    ZzPRT(ERROR, "        받은 쪽지 삭제 (30일 이전)\n");
    ZzPRT(ERROR, "        YYYYMMDD : 처리일자(00000000:시스템일자)\n");
    return -1;
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daem5501_process_term()
{
    // DB close
	db_disconnect(con);
	db_disconnect(con_backup);	
	
    ZzLOG(ALWAY, "[daem5501]*****************프로그램 종료*****************\n");  

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5501_signal(int nSignal)
{
    daem5501_process_term();
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
	signal(SIGTERM, daem5501_signal);
	signal(SIGINT,  daem5501_signal);
	signal(SIGQUIT, daem5501_signal);
	signal(SIGKILL, daem5501_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daem5501_process_init(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daem5501_process();
		/* 프로그램 종료루틴 */                    
		daem5501_process_term();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
