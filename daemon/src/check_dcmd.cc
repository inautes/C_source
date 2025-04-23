/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : check_dcmd.cc
 *         기능 : 디비중계서버의 체크및 재실행
 *         설명 : 디비중계서버가 실행되는지 체크하고 죽어있다면 다시 실행 시킴
 *       작성자 : HCS
 *       작성일 : 2007/04/19
 *     수정이력 :
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/time.h>
//#include <time.h>

#include "check_dcmd.h"
#include "daemcom.h"
//#include "apdefine.h"
//#include "comcomm.h"



//#define __DEBUG

/*===========================================================================*/
/* 전역변수 선언															 */
/*===========================================================================*/
//static bool bCloseMain;
int RunSystem(char* pSystemQuery)
{
	int nError = 0;
	nError = system(pSystemQuery);

	return 0;
}
void RegularCheck()
{
	struct stat statbuf;
    int nServerExit = 0;
	int nServerPort = 4999;
	
	FILE *pFile;
	
	char szHome[256];
	memset(szHome, 0x00, sizeof(szHome));
	strcpy(szHome , getenv("HOME"));

	char szPath[512];
	memset(szPath, 0x00, sizeof(szPath));

	char szMsgBuf[256];
	memset(szMsgBuf, 0x00, sizeof(szMsgBuf));

	char szServerPath[512];
	memset(szServerPath, 0x00, sizeof(szServerPath));
	strcpy(szServerPath, "/root/dcmdserver/bin");
	
	char szServerName[512];
	memset(szServerName, 0x00, sizeof(szServerName));
	strcpy(szServerName, "dcmdserver");
	
	char szSystemQuery[1024];
	memset(szSystemQuery, 0x00, sizeof(szSystemQuery));
		
	sprintf(szSystemQuery , " ps -ef | grep %s | grep %d | head -1 | awk '{print $9}' > %s/%s.txt ", szServerName, nServerPort, szHome, szServerName);
	RunSystem(szSystemQuery);
	

	sprintf(szSystemQuery , "%s/%s.txt",szHome,szServerName);   
	int nStat = stat(szSystemQuery,&statbuf);
	
	nServerExit = (int)statbuf.st_size;

	sprintf(szPath,"%s/%s.txt",szHome, szServerName);
	pFile = fopen(szPath,"r");
	if(pFile != NULL)
	{
		fgets(szMsgBuf, 5, pFile);		
	}
	else
	{
	    fclose(pFile);
		pFile = NULL;
		return ;	
	}
	
//	if( !nServerExit) 
	if(strcmp(szMsgBuf, "4999") == 0)
	{
	}
	else
	{
		sprintf(szSystemQuery,"nohup %s/%s %d &",szServerPath, szServerName , nServerPort );
		RunSystem(szSystemQuery);
	}
    fclose(pFile);
    pFile = NULL;
	
}

void ReserveCheck()
{
	struct stat statbuf;
    int nServerExit = 0;
	int nServerPort = 4998;
	
	FILE *pFile;
	
	char szHome[256];
	memset(szHome, 0x00, sizeof(szHome));
	strcpy(szHome , getenv("HOME"));

	char szPath[512];
	memset(szPath, 0x00, sizeof(szPath));

	char szMsgBuf[256];
	memset(szMsgBuf, 0x00, sizeof(szMsgBuf));

	char szServerPath[512];
	memset(szServerPath, 0x00, sizeof(szServerPath));
	strcpy(szServerPath, "/root/dcmdserver/bin");
	
	char szServerName[512];
	memset(szServerName, 0x00, sizeof(szServerName));
	strcpy(szServerName, "dcmdserver");
	
	char szSystemQuery[1024];
	memset(szSystemQuery, 0x00, sizeof(szSystemQuery));
		
	sprintf(szSystemQuery , " ps -ef | grep %s | grep %d | head -1 | awk '{print $9}' > %s/%s.txt ", szServerName, nServerPort, szHome, szServerName);
	RunSystem(szSystemQuery);
	

	sprintf(szSystemQuery , "%s/%s.txt",szHome,szServerName);   
	int nStat = stat(szSystemQuery,&statbuf);
	
	nServerExit = (int)statbuf.st_size;

	sprintf(szPath,"%s/%s.txt",szHome, szServerName);
	pFile = fopen(szPath,"r");
	if(pFile != NULL)
	{
		fgets(szMsgBuf, 5, pFile);		
	}
	else
	{
        fclose(pFile);
        pFile = NULL;
	
		return ;	
	}


//	if( !nServerExit) 
	if(strcmp(szMsgBuf, "4998") == 0)
	{
	}
	else
	{
		sprintf(szSystemQuery,"nohup %s/%s %d &",szServerPath, szServerName , nServerPort );
		RunSystem(szSystemQuery);
	}
    fclose(pFile);
    pFile = NULL;
	
}

void process()
{
	RegularCheck();
	ReserveCheck();
}



/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  signal_exception(int nSignal)
{
    #ifdef __DEBUG
    printf("signal exception %d",nSignal);
    #endif
}



//******************************************************************************
//* chkdaemon main
//******************************************************************************
int main(int argc,char** argv)
{
	
	
	signal(SIGTERM, signal_exception);
	signal(SIGINT,  signal_exception);
	signal(SIGQUIT, signal_exception);
	signal(SIGKILL, signal_exception);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);

	process();
	
    return 0;
}
