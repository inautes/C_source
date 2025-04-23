/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daemudtinfo.cc
 *         기능 : 
 *
 *       작성자 : HCS
 *       작성일 : 2010-01-18
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
#include <sys/types.h> //opendir
#include <dirent.h> //opendir

#include "daemcom.h"
#include "commydb.h"

#define  MAX_ROWS	1
#define _DEBUG_

int daemudtinfo_init_process(int argc, char **argv);
int daemudtinfo_process();
int daemudtinfo_term_process();
void daemudtinfo_signal(int nSignal);
int daemudtinfo_cpr_process(char* pMgrCd, char* pCompNm, char* pSectCode, char* pSectSub, char* pRate, int nPrice);
int DirProc(char* pPath);
int FileProc(char* pPath);
int FileHandler(char* pPath);
int DBWriter(char* pUdtCd, char* pUserId);

MYSQL     *con;

char gszLogPath[1024];
char gszUdtDate[8+1];
char gszHostName[10+1];
//******************************************************************************
//* daemudtinfo main
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
	ZzLOG(ALWAY,"FileHandler: %s 분석 시작.\n", pPath);

	FILE* pFLog = NULL;
	char szStringBuffer[1024*2];
	memset(szStringBuffer, 0x00, sizeof(szStringBuffer));
	
	char szUdtCd[2+1];
	memset(szUdtCd, 0x00, sizeof(szUdtCd));
	
	pFLog = fopen(pPath,"rt");
	if(pFLog == NULL)
	{
		ZzLOG(ERROR,"FileHandler: 파일을 열수 없습니다.[%s]\n", pPath);
		return -1;
	}	
	
	while(fgets(szStringBuffer,sizeof(szStringBuffer),pFLog)!=NULL)
	{
		memset(szUdtCd, 0x00, sizeof(szUdtCd));
		if(strstr(szStringBuffer, "WeDisk.ocx") != NULL)
		{
			strcpy(szUdtCd, "DN");
		}	
		else if(strstr(szStringBuffer, "WediskUpManager.ocx") != NULL)
		{
			strcpy(szUdtCd, "UP");
		}

		if(strlen(szUdtCd) > 0 )
		{		
			int nFirstLen = GetFistCharIndex(szStringBuffer, '[');
			nFirstLen = nFirstLen + 1;
			
			int nLastLen = GetReverseIndex(szStringBuffer + nFirstLen, ']');
			nLastLen = nLastLen - 1;
			
			char szUserId[16+1];
			memset(szUserId, 0x00, sizeof(szUserId));
			
			memcpy(szUserId, szStringBuffer + nFirstLen, nLastLen+1);
			
			if(strlen(szUserId) > 16 || strlen(szUserId) <= 0)
			{
				ZzLOG(ALWAY, "DBWriter: 양식에 맞지 않습니다.user_id=[%s]\n", szUserId);
				continue;
			}	
			if(DBWriter(szUdtCd, szUserId) < 0)
				return -1;
		}		
		memset(szStringBuffer, 0x00, sizeof(szStringBuffer));
	}
	ZzLOG(ALWAY,"FileHandler: %s 분석 정상 종료.\n", pPath);
	return 0;
}

int DBWriter(char* pUdtCd, char* pUserId)
{
	MYSQL_RES* res;
	MYSQL_ROW  row;
	
	char szQuery[1024];
	memset(szQuery, 0x00, sizeof(szQuery));
	
	int nUdtCnt = 0;
	
	sprintf(szQuery, " select udt_cnt "
					 " from zangsi_bck.T_CLIENT_UPDATE_SERVER_LOG "
					 " where udt_date = '%s' and server_id = '%s' and user_id = '%s' and udt_cd = '%s' "
					 , gszUdtDate, gszHostName, pUserId, pUdtCd);
	
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
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " INSERT INTO zangsi_bck.T_CLIENT_UPDATE_SERVER_LOG "
						 " (udt_date, server_id, user_id, udt_cd, udt_cnt) "
						 " VALUES "
						 " ('%s', '%s', '%s', '%s', 1) "
						 , gszUdtDate, gszHostName, pUserId, pUdtCd);
	}
	else
	{	
		row = mysql_fetch_row(res);
		nUdtCnt = (int)getint(row, 0);
		
		nUdtCnt = nUdtCnt + 1;

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update zangsi_bck.T_CLIENT_UPDATE_SERVER_LOG " 
						 " set udt_cnt = %d "
						 " where udt_date = '%s' and server_id = '%s' and user_id = '%s' and udt_cd = '%s' "
						 , nUdtCnt, gszUdtDate, gszHostName, pUserId, pUdtCd);
		
	}
	mysql_free_result(res);
	
	if(mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "DBWriter: mysql_query error\n");
		ZzLOG(ERROR, "DBWriter: szQuery=[%s]\n", szQuery);
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;			
	}
	
	nUdtCnt = 0;
	
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "select udt_cnt from zangsi_bck.T_CLIENT_UPDATE_USER_LOG where user_id = '%s' and udt_cd = '%s'", pUserId, pUdtCd);
	
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
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " INSERT INTO zangsi_bck.T_CLIENT_UPDATE_USER_LOG "
						 " (user_id, udt_cd, last_udt_date, udt_cnt) "
						 " VALUES "
						 " ('%s', '%s', '%s', 1) "
						 , pUserId, pUdtCd, gszUdtDate);
	}
	else
	{	
		row = mysql_fetch_row(res);
		nUdtCnt = (int)getint(row, 0);
		nUdtCnt = nUdtCnt + 1;
		
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update zangsi_bck.T_CLIENT_UPDATE_USER_LOG "
						 " set udt_cnt = %d,  last_udt_date = '%s'"
						 " where user_id = '%s' and udt_cd = '%s' "
						 , nUdtCnt, gszUdtDate, pUserId, pUdtCd);
	}
	mysql_free_result(res);
	
	if(mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "DBWriter: mysql_query error\n");
		ZzLOG(ERROR, "DBWriter: szQuery=[%s]\n", szQuery);
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;			
	}
	
	

	return 0;	
}

	

int daemudtinfo_process()
{
	DIR *pdir = NULL;
	pdir = opendir(gszLogPath);//strPath); 
	
	if(pdir == NULL)
	{
		ZzLOG(ALWAY, "daemudtinfo_process: [%s] 파일 처리.\n", gszLogPath);
		return FileProc(gszLogPath);
	}
	else
	{
		ZzLOG(ALWAY, "daemudtinfo_process: [%s] 폴더 처리.\n", gszLogPath);
		return DirProc(gszLogPath);
	}	
}


/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daemudtinfo_init_process(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
    
    ZzInitGlobalVariable2("daemudtinfo_", "/logs/daemon"); 

    ZzLOG(ALWAY, "[daemudtinfo]*****************프로그램 시작*****************\n");  
    ZzLOG(ALWAY, "[daemudtinfo] 업데이트 모듈 사용자 파악.\n");  
	
  	// 파라미터 값 설정 및 초기화
  	memset(gszLogPath, 0x00, sizeof(gszLogPath));

    if (argc != 2)
    {
		goto arg_error;
    }

	strcpy(gszLogPath, argv[1]);
	strcpy(gszHostName , getenv("HOSTNAME") );
	
	ZzLOG(ALWAY, "[daemudtinfo] [%s] [%s] 를 처리합니다.\n", gszHostName, gszLogPath);  
	
	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(con=db_connect_backup("zangsi_bck")))
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
int daemudtinfo_term_process()
{
    // DB close
	db_disconnect(con);	
    ZzLOG(ALWAY, "[daemudtinfo]*****************프로그램 종료*****************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daemudtinfo_signal(int nSignal)
{
    daemudtinfo_term_process();
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
	signal(SIGTERM, daemudtinfo_signal);
	signal(SIGINT,  daemudtinfo_signal);
	signal(SIGQUIT, daemudtinfo_signal);
	signal(SIGKILL, daemudtinfo_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
*/
	if ( daemudtinfo_init_process(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daemudtinfo_process();
		/* 프로그램 종료루틴 */
	}
	daemudtinfo_term_process();
	return(0);
}
/*****************************************************************************
*  End of file...
*****************************************************************************/


