/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daemipcmpinfo.cc
 *         기능 : 아파치 access_log 내 IP와 Log-in IP 비교
 *
 *       작성자 : 박병훈
 *       작성일 : 2010-11-26
 *			 수정일 : 2010-12-01
 *		 수정내역 : con_time 시간 변경 > 접속 시간부터 10초 이내
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
#include <regex.h>
#include <sys/stat.h>
#include <sys/types.h> //opendir
#include <dirent.h> //opendir

#include "daemcom.h"
#include "commydb.h"

#define  MAX_ROWS	1
#define _DEBUG_

int daemipcmpinfo_init_process(int argc, char **argv);
int daemipcmpinfo_process();
int daemipcmpinfo_term_process();
void daemipcmpinfo_signal(int nSignal);
int DirProc(char* pPath);
int FileProc(char* pPath);
int FileHandler(char* pPath);
int ApacheParser(char* szApacheData);
char* ConvertWeek(char* szWeek);
char* CopyApcheData(const char *buf, int start, int end);
int DBWriter(char* pConnIP, char* pConnDate, char* pConnTime);

MYSQL     *con;

char gszLogPath[1024];
char gszUdtDate[8+1];
char gszHostName[10+1];
//******************************************************************************
//* daemipcmpinfo main
//******************************************************************************

int DirProc(char* pPath)
{
	DIR *pdir = NULL;


	pdir = opendir(pPath);//strPath); 
	
	
	
	if(pdir == NULL)
	{
		ZzLOG(ERROR, "DirProc: Error ( pdir == NULL )\n");
		return -1;	
	}
	
	struct dirent *pent;
	struct stat64 statbuf;
	
	char fullpath[612];
	
	
	while( (pent = readdir(pdir)) != NULL )
	{
		memset(fullpath,0x00,sizeof(fullpath));
		strcpy(fullpath,pPath);//strPath);
		strcat(fullpath , "/");
		strcat(fullpath, pent->d_name);  
		
		
		int stat= stat64(fullpath,&statbuf); 
	 
	 	if((strcmp(".",pent->d_name) !=0 ) && (strcmp("..",pent->d_name) != 0) && (GetFistCharIndex(pent->d_name,'.') != 0))
		{
			if(S_ISDIR (statbuf.st_mode))
			{
				
				int nRes = DirProc( fullpath);
				if(nRes == -1)
				{
                    ZzLOG(ALWAY, "DirProc: Error ( nRes == -1 )\n");					
					closedir(pdir);
					return -1;
				}
			}
			else
			{
				if(S_ISREG (statbuf.st_mode))
				{
					memset(gszUdtDate, 0x00, sizeof(gszUdtDate));
					memcpy(gszUdtDate, pent->d_name + 14, 8);
					
					if(FileHandler(fullpath) < 0)
					{
						return -1;
					}
				}
				else
				{
					closedir(pdir);
          ZzLOG(ERROR, "DirProc: Error ( S_ISREG (statbuf.st_mode) )\n");										
					return -1;
				}
			}
		}
	}
	
	closedir(pdir);
	
	return 0;
}

int FileProc(char* pPath)
{
	char szFileName[64];
	memset(szFileName, 0x00, sizeof(szFileName));
	
	int nRootPathLen = GetReverseIndex(pPath, '/');
	
	
	nRootPathLen = nRootPathLen +1;
	int nTotLen = strlen(pPath);

	
	int nFileNameLen = nTotLen - nRootPathLen;

	
	memcpy(szFileName, pPath + nRootPathLen, nFileNameLen);
	
	memset(gszUdtDate, 0x00, sizeof(gszUdtDate));
	memcpy(gszUdtDate, szFileName + 14, 8);

	if(FileHandler(pPath) < 0)
	{
		return -1;
	}
	return 0;	
}

int FileHandler(char* pPath)
{
	ZzLOG(ALWAY,"FileHandler: [%s] 분석 시작.\n", pPath);

	FILE* pFLog = NULL;
	
	char szStringBuffer[1024*2];
	memset(szStringBuffer, 0x00, sizeof(szStringBuffer));
	
	pFLog = fopen(pPath,"rt");

	if(pFLog == NULL)
	{
		ZzLOG(ERROR,"FileHandler: 파일을 열수 없습니다.[%s]\n", pPath);
		return -1;
	}	
	
	while(fgets(szStringBuffer, sizeof(szStringBuffer), pFLog)!=NULL)
	{	
		ApacheParser(szStringBuffer);		
		memset(szStringBuffer, 0x00, sizeof(szStringBuffer));	
	}
	ZzLOG(ALWAY,"FileHandler: %s 분석 정상 종료.\n", pPath);
	
	return 0;
}

int ApacheParser(char* pApacheData)
{
	int i;
	int result;
	int start=0, end=0;
	
	regex_t *compiled;
	regmatch_t *matchptr;
	size_t nmatch;	
		
	if( (compiled = (regex_t*)malloc(sizeof(regex_t))) == NULL ) 
	{
		ZzLOG(ERROR, "XmlParApacheParserser: regex_t malloc error\n" );
		return -1;
	}		
		
	char szRex[128];
	memset(szRex, 0x00, sizeof(szRex));

//------------------------------------------------------------------------------
//                   IP         IP      -      -    [27/Oct/2000:09:27:09 -0400]
//------------------------------------------------------------------------------

	strcpy(szRex, "^([0-9.]+) ([0-9.]+) (\\S+) (\\S+) \\[(.*)\\]");	
	
	ZzLOG(ALWAY, "ApacheParser: Parsing 시작.\n");
	ZzLOG(ALWAY, "ApacheParser: Parsing data > %s\n", pApacheData);
	
	if( regcomp( compiled, szRex, REG_EXTENDED | REG_ICASE | REG_NEWLINE ) != 0 ) 
	{
		ZzLOG(ERROR, "ApacheParser: regcomp error\n" );
		return -1;
	}
	
	nmatch = compiled->re_nsub+1;
	
	if( (matchptr = (regmatch_t*)malloc(sizeof(regmatch_t)*nmatch)) == NULL ) 
	{
		ZzLOG(ERROR, "ApacheParser: regmatch_t malloc error\n" );
		return -1;
	}		
		
	result = regexec( compiled, pApacheData+start, nmatch, matchptr, 0);

	if(result != 0)
	{
		ZzLOG(ALWAY, "ApacheParser: regexec result ERROR : [%d]\n", result);
		return -1;
	}

// matchptr[] buffer
// 1,2	: ip
// 3,4	: -
// 5		: DATE 및 기타 
	
	char* pConnIP = CopyApcheData( pApacheData, start+matchptr[2].rm_so, start+matchptr[2].rm_eo );
	
	ZzLOG(ALWAY, "ApacheParser: pConnIP > %s\n", pConnIP);
	
// IP
	char szConnIP[15+1];
	memset(szConnIP, 0x00, sizeof(szConnIP));
	memcpy(szConnIP, pConnIP, strlen(pConnIP));
	
	if(pConnIP)
		delete[] pConnIP;
	
	if(strlen(szConnIP) > 16 || strlen(szConnIP) <= 0)
	{
		ZzLOG(ALWAY, "ApacheParser: IP 양식에 맞지 않습니다.\n");
		return -1;
	}	
	
// 27/Oct/2000:09:27:09 -0400] "GET /java/javaResources.html HTTP/1.0" 200 10450...
// DATE : 27/Oct/2000:09:27:09 -0400
// conn_date : 20001027	
// conn_time : 092709
		
	char* pDate = CopyApcheData( pApacheData, start+matchptr[5].rm_so, start+matchptr[5].rm_eo );
	
	ZzLOG(ALWAY, "ApacheParser: pDate > %s\n", pDate);
	
	int nCount = 0;
	char seps[] = "/: ";
	char* token = NULL; 
				
	char szConnDay[2+1];	
	char szConnMon[2+1];
	char szConnYear[4+1];	
	char szHour[2+1];
	char szMin[2+1];				
	char szSec[2+1];
	
	token = strtok(pDate , seps);
				
	while( token != NULL )
	{		
		if(nCount == 0)				// DAY
		{
			memset(szConnDay, 0x00, sizeof(szConnDay));
			memcpy(szConnDay, token, strlen(token));
		}
		else if(nCount == 1)	// MONTH
		{
			memset(szConnMon, 0x00, sizeof(szConnMon));
			memcpy(szConnMon, ConvertWeek(token), strlen(token));					
		}
		else if(nCount == 2)	// YEAR
		{
			memset(szConnYear, 0x00, sizeof(szConnYear));
			memcpy(szConnYear, token, strlen(token));		
		}
		else if(nCount == 3)	// HOUR
		{
			memset(szHour, 0x00, sizeof(szHour));
			memcpy(szHour, token, strlen(token));			
		}
		else if(nCount == 4)	// MINUTE
		{
			memset(szMin, 0x00, sizeof(szMin));
			memcpy(szMin, token, strlen(token));	
		}												
		else if(nCount == 5) // SEC
		{
			memset(szSec, 0x00, sizeof(szSec));
			memcpy(szSec, token, strlen(token));
		}										
	
		token = strtok(NULL, seps);
		nCount++;
	}
	
	//start += matchptr.rm_eo;
	
	if(token)
		delete[] token;
	
	if(pDate)
		delete[] pDate;
		
	char szConnDate[8+1];
	char szConnTime[6+1];
		
	memset(szConnDate, 0x00, sizeof(szConnDate));
	memset(szConnTime, 0x00, sizeof(szConnTime));
		
	sprintf(szConnDate, "%s%s%s", szConnYear, szConnMon, szConnDay);
	sprintf(szConnTime, "%s%s%s", szHour, szMin, szSec);
		
	ZzLOG(ALWAY, "Parser Result: ConnIP=[%s]\n", szConnIP);
	ZzLOG(ALWAY, "Parser Result: ConnDate=[%s]\n", szConnDate);
	ZzLOG(ALWAY, "Parser Result: szConnTime=[%s]\n\n", szConnTime);
		
	DBWriter(szConnIP, szConnDate, szConnTime);	

	regfree( compiled );
	
	return 0;
}

char* ConvertWeek(char* szWeek)
{		
	if(strcmp(szWeek, "Jan") == 0)
		return "01";
	else if(strcmp(szWeek, "Feb") == 0)
		return "02";
	else if(strcmp(szWeek, "Mar") == 0)
		return "03";
	else if(strcmp(szWeek, "Apr") == 0)
		return "04";
	else if(strcmp(szWeek, "May") == 0)
		return "05";
	else if(strcmp(szWeek, "Jun") == 0)
		return "06";
	else if(strcmp(szWeek, "Jul") == 0)
		return "07";
	else if(strcmp(szWeek, "Aug") == 0)
		return "08";
	else if(strcmp(szWeek, "Sep") == 0)
		return "09";		
	else if(strcmp(szWeek, "Oct") == 0)
		return "10";
	else if(strcmp(szWeek, "Nov") == 0)
		return "11";
	else if(strcmp(szWeek, "Dec") == 0)
		return "12";
	else
		return "00";
}

char* CopyApcheData(const char *buf, int start, int end)
{
	int nDataSize = 0;
	nDataSize = end - start;
	
	if(nDataSize <= 0)
	{
		return NULL;
	}
	
	char* pRetData = new char[nDataSize+1];
	memset(pRetData, 0x00, nDataSize+1);
	memcpy(pRetData, buf+start, nDataSize);
	
	return pRetData;
}

// conn_ip, user_id, conn_date, conn_time
//int DBWriter(char* pUdtCd, char* pUserId)
int DBWriter(char* pConnIP, char* pConnDate, char* pConnTime)
{
	MYSQL_RES* res;
	MYSQL_ROW  row;

	char szQuery[1024];
	memset(szQuery, 0x00, sizeof(szQuery));

	int nUdtCnt = 0;

//--------------------------------------------------------------
// LOGIN_LOG 비교
// login_time의 오차는 10초 간격.
// con_time-5 <= con_time <= con_time+5
// 수정 > con_time <= con_time <= con_time+10
//--------------------------------------------------------------
	int nConn_time = 0;
	nConn_time = atoi(pConnTime);

	sprintf(szQuery, " select user_id "
						" from zangsi.T_LOGIN_LOG "
						" where login_date = '%s' and login_ip = '%s' "
						" and login_time >= '%d' and login_time <= '%d' "
						, pConnDate, pConnIP, nConn_time, nConn_time+10);
	
	if(mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "DBWriter: mysql_query error\n");
		ZzLOG(ERROR, "DBWriter: szQuery=[%s]\n", szQuery);
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;	
	}
	
	if(!(res = mysql_store_result(con)))
	{
		ZzLOG(ERROR, "DBWriter: mysql_store_result error\n");
		ZzLOG(ERROR, "DBWriter: szQuery=[%s]\n", szQuery);
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;			
	}

	if(mysql_num_rows(res)==0)
	{
		ZzLOG(ERROR, "DBWriter: invaild user_id. con_ip > [%s]\n", pConnIP);
		
		row = mysql_fetch_row(res);
		
		char* pUserId = getstr(row,0);

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " INSERT INTO zangsi.T_LOGIN_CMP_REALCON "
								 " (con_ip, server_id, con_date, con_time) "
								 " VALUES "
								 " ('%s', '%s', '%s', '%s') "
								 , pConnIP, gszHostName, pConnDate, pConnTime);		
								 
		mysql_free_result(res);
	}
	else	// user_id column 입력
	{
		row = mysql_fetch_row(res);
		
		char* pUserId = getstr(row,0);

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " INSERT INTO zangsi.T_LOGIN_CMP_REALCON "
								 " (con_ip, server_id, user_id, con_date, con_time) "
								 " VALUES "
								 " ('%s', '%s', '%s', '%s', '%s') "
								 , pConnIP, gszHostName, pUserId, pConnDate, pConnTime);
								 
		mysql_free_result(res);
	}
	
	if(mysql_query(con, szQuery))
	{
			ZzLOG(ERROR, "DBWriter: mysql_query error\n");
			ZzLOG(ERROR, "DBWriter: szQuery=[%s]\n", szQuery);
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;			
	}
	
	return 0;	
}

int daemipcmpinfo_process()
{
	DIR *pdir = NULL;
	pdir = opendir(gszLogPath);//strPath); 
	
	if(pdir == NULL)
	{
		ZzLOG(ALWAY, "daemipcmpinfo_process: [%s] 파일 처리.\n", gszLogPath);
		return FileProc(gszLogPath);
	}
	else
	{
		ZzLOG(ALWAY, "daemipcmpinfo_process: [%s] 폴더 처리.\n", gszLogPath);
		return DirProc(gszLogPath);
	}	
}

/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daemipcmpinfo_init_process(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
    
    ZzInitGlobalVariable2("daemipcmpinfo_", "/logs/daemon"); 

    ZzLOG(ALWAY, "[daemipcmpinfo]*****************프로그램 시작*****************\n");  
    ZzLOG(ALWAY, "[daemipcmpinfo] 실제 로그인 사용자 파악.\n");  
	
  	// 파라미터 값 설정 및 초기화
  	memset(gszLogPath, 0x00, sizeof(gszLogPath));

    if (argc != 2)
    {
		goto arg_error;
    }

	strcpy(gszLogPath, argv[1]);
	strcpy(gszHostName , getenv("HOSTNAME") );
	
	ZzLOG(ALWAY, "[daemipcmpinfo] [%s] [%s] 를 처리합니다.\n", gszHostName, gszLogPath);  
	
	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(con=db_connect_logdb("zangsi")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
	   	return(-1); 
	}

    return (0);
arg_error:
    ZzLOG(ERROR, "usage : %s LOGPATH\n", argv[0]);
    ZzLOG(ERROR, "           LOGPATH : 처리할 로그 저장 위치(폴더단위)\n");
    return -1;
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daemipcmpinfo_term_process()
{
    // DB close
	db_disconnect(con);	
    ZzLOG(ALWAY, "[daemipcmpinfo]*****************프로그램 종료*****************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daemipcmpinfo_signal(int nSignal)
{
    daemipcmpinfo_term_process();
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
/*
	signal(SIGTERM, daemipcmpinfo_signal);
	signal(SIGINT,  daemipcmpinfo_signal);
	signal(SIGQUIT, daemipcmpinfo_signal);
	signal(SIGKILL, daemipcmpinfo_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
*/
	if ( daemipcmpinfo_init_process(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daemipcmpinfo_process();
		/* 프로그램 종료루틴 */
	}
	daemipcmpinfo_term_process();
	return(0);
}
/*****************************************************************************
*  End of file...
*****************************************************************************/


