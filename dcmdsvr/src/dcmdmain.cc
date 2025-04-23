/******************************************************************************
 *   서브시스템 :
 *   프로그램명 :
 *         기능 :
 *         설명 :
 *       작성자 :
 *       작성일 :
 *     수정이력 :
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#include <map>
#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h> /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <errno.h>
#include <sys/stat.h>
#include <time.h> //randomize()
#include <pthread.h>        /* for POSIX threads */

#include <signal.h>

#include "dcmdsock.h"

#include "dcmddefine.h"
#include "dcmdproc.h"
#include "apstruct.h"
#include "apdefine.h"
#include "dcmdmain.h"
#include "comcomm.h"
#include "comconf.h"

#include "comsock.h"
#include "Property.h"
using namespace std;

//#include "dcmdfups4001.h"

//#define __DEBUG

/*===========================================================================*/
/* 전역변수 선언															 */
/*===========================================================================*/


CMysqlPool* m_g_clMysqlPool;
CMysqlPool* m_g_clMysqlPoolCopyRight;
CMysqlPool* m_g_clMysqlPoolDnLog;

SUserParm_T gstUserParm;
char	errMsg[256];




pthread_cond_t async_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t async_mutex = PTHREAD_MUTEX_INITIALIZER;



/*============================================================================*/
/* 함수 프로토타입                                                            */
/*============================================================================*/
int     infInitProcess(int, char**);
int     infMainRoutine(int, char**);
void    infTermProcess();

/*****************************************************************************
* CMD서버 Main
* (I) 1. int argc    : Argument Count 수
*     2. char **argv : Argunment Data Point
*****************************************************************************/


int main(int argc, char** argv)
{

	 int ret = 0;
	 ret = check_single_running(argv[0]);
	 if(ret !=0){
		 syslog(LOG_INFO,"duplicate Running!! %s \n",argv[0]);
		 exit(0);
	 }else{
		 syslog(LOG_INFO,"Single Running!! %s \n",argv[0]);
	 }

	// 다중 다운로드 관련 하여 수정 - 20081224
	infMainRoutine(argc, argv);
    infTermProcess();
}
/*****************************************************************************
* 프로그램 시작
* (I) 1. int argc    : Argument Count 수
*     2. char **argv : Argunment Data Point
* (R) 1. 정상 : '0'
*     2. 오류 : 음수
*****************************************************************************/
int     infInitProcess(int argc, char** argv)
{
    infLOG(ALWAY, "================= 프로그램 시작 ====================\n");
    return RETOK;
}

/*****************************************************************************
* 프로그램 종료
* (I) void
* (R) void
*****************************************************************************/
void    infTermProcess()
{
    infLOG(ALWAY, "================= 프로그램 종료 ====================\n");
    exit(0);
}


/*****************************************************************************
* SIGNAL HANDLING
* (I) int nSigNo : Signal Number
* (R) void
*****************************************************************************/
//void    infSigHandler(int nSigNo)
void infSigHandler(int nSigNo, siginfo_t *siginfo, void* p)
{

	//infLOG(ALWAY, "----------| infSigHandler`SigNo[%d]\n", nSigNo);

	sigset_t sigset, oldset;
    sigfillset(&sigset);
    // 새로들어오는 모든 시그널에 대해서 block 한다.
/*
    if (sigprocmask(SIG_BLOCK, &sigset, &oldset) < 0)
    {
		infLOG(ERROR, "----------|  infSigHandler`SigNo[%d]\n", nSigNo);
    }
	infLOG(ALWAY, "infSigHandler       ] infSigHandler`SigNo[%d]\n", nSigNo);
*/

}


int infMainRoutine(int argc,char** argv)
{
	
		
	if(argc == 2 && strcmp(argv[1], "-v") == 0)
	{
		printf("dcmdserver version 9,1,2,4,1\n");
		exit(1);
	}
	
	int servSock;                    /* Socket descriptor for server */
    int clntSock;                    /* Socket descriptor for client */
    int ServPort=0;					 /* Server port */
    pthread_t threadID;              /* Thread ID from pthread_create() */
    pthread_t QueMngThreadID;              /* Thread ID from pthread_create() */
    pthread_t DBPoolQueMngThreadID;              /* Thread ID from pthread_create() */

    pthread_t CprQueMngThreadID;              /* Thread ID from pthread_create() */
    pthread_t CprDBPoolQueMngThreadID;              /* Thread ID from pthread_create() */

    pthread_t DnLogQueMngThreadID;              /* Thread ID from pthread_create() */
    pthread_t DnLogDBPoolQueMngThreadID;              /* Thread ID from pthread_create() */


    struct ThreadArgs *threadArgs;   /* Pointer to argument structure for thread */

    struct sigaction intsig;

	intsig.sa_sigaction = infSigHandler;

	sigemptyset(&intsig.sa_mask);
	intsig.sa_flags = SA_SIGINFO;

	if (sigaction(SIGPIPE, &intsig, 0) == -1)
	{
		infLOG(ERROR, "----------| sigaction error ( SIGPIPE ) \n");
	}

	infSetUserParm(&gstUserParm, argc, argv);

	char szTemp[512]; memset(szTemp,0x00, sizeof(szTemp));

	Property pP;
	pP.SetProcName(argv[0]);
	pP.GetStrProperty("[INFO]", "LOG_PATH"			, szTemp);
	pP.GetIntProperty("[INFO]", "SERVER_PORT"		, ServPort);

	if(strcmp(szTemp,"") == 0)
		strcpy(szTemp,"/logs");

	if(ServPort == 0)
		ServPort = atoi(argv[1]);  /* First arg:  local port */

	infLOG(ALWAY, " --- server port = %d \n" ,ServPort);
	infLOG(ALWAY, " --- log path = %s \n"	 ,szTemp);

	if(ServPort == 0)
	{
		return -1;
	}

    servSock = CreateTCPServerSocket(ServPort);

    infLOG(ALWAY, "----------| dcmd server start  %ld ) |----------\n\n\n" ,getpid()); 


	//main db pool
	m_g_clMysqlPool = new CMysqlPool();
	m_g_clMysqlPool->UseMutexSchedule(true);
	m_g_clMysqlPool->SetLogMark("Normal");
	m_g_clMysqlPool->SetLogInfo(szTemp,"MysqlPool");
	//copyright db pool
	m_g_clMysqlPoolCopyRight = new CMysqlPool();
	m_g_clMysqlPoolCopyRight->UseMutexSchedule(true);
	m_g_clMysqlPoolCopyRight->SetLogMark("CopyRight");
	m_g_clMysqlPoolCopyRight->SetLogInfo(szTemp,"MysqlPoolCopyRight");

	//dn db pool ( logdb )
	m_g_clMysqlPoolDnLog = new CMysqlPool();
//	m_g_clMysqlPoolDnLog->SetConnectTimeout(5);
	m_g_clMysqlPoolDnLog->UseMutexSchedule(true);
	m_g_clMysqlPoolDnLog->SetLogMark("DnLog");
	m_g_clMysqlPoolDnLog->SetLogInfo(szTemp,"MysqlPoolDnLog");

	m_g_clMysqlPool->SetDB				( OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER		,OSP_DB_DCMD_PASS );
	m_g_clMysqlPoolCopyRight->SetDB		( OSP_CPR_DB_NAME	,OSP_CPR_DB_IP_PUB	,OSP_DB_DCMD_USER		,OSP_DB_DCMD_PASS );
	m_g_clMysqlPoolDnLog->SetDB			( OSP_LOG_DB_NAME	,OSP_LOG_DB_IP_PUB	,OSP_DB_DCMD_USER		,OSP_DB_DCMD_PASS );


	infLOG(ALWAY, " --- OSP_DB_NAME		= %s,%s \n"	 ,OSP_DB_NAME		,OSP_DB_IP_PUB);
	infLOG(ALWAY, " --- OSP_CPR_DB_NAME = %s,%s \n"	 ,OSP_CPR_DB_NAME	,OSP_CPR_DB_IP_PUB);
	infLOG(ALWAY, " --- OSP_LOG_DB_NAME = %s,%s \n"	 ,OSP_LOG_DB_NAME	,OSP_LOG_DB_IP_PUB);
/* myslq pool	 */
	if( argc == 4 )
	{
		m_g_clMysqlPool->CreaeMysqlPool(atoi(argv[2]),atoi(argv[3]));
		m_g_clMysqlPoolCopyRight->CreaeMysqlPool(atoi(argv[2]),atoi(argv[3]));
		m_g_clMysqlPoolDnLog->CreaeMysqlPool(atoi(argv[2]),atoi(argv[3]));
	}
	else
	{
		/*
		2008/09/25 HCS - 풀사이즈 5, 20에서 8, 20으로 변경.
		*/
		m_g_clMysqlPool->CreaeMysqlPool( 10 ,  20);
		m_g_clMysqlPoolCopyRight->CreaeMysqlPool(10, 20);
		m_g_clMysqlPoolDnLog->CreaeMysqlPool(10, 20);
	}



	if (pthread_create(&DBPoolQueMngThreadID, NULL, DBPoolQueManager, NULL ) != 0)
	{
		printf("DB Pool Queue 관리 시스템 실행 오류 입니다.\n");
		return -1;
	}

	if (pthread_create(&QueMngThreadID, NULL, QueManager, NULL ) != 0)
	{
		printf("Queue 관리 시스템  실행 오류 입니다.\n");
		return -1;
	}

	if (pthread_create(&CprDBPoolQueMngThreadID, NULL, CprDBPoolQueManager, NULL ) != 0)
	{
		printf("DB Pool Queue 관리 시스템 실행 오류 입니다.\n");
		return -1;
	}

	if (pthread_create(&CprQueMngThreadID, NULL, CprQueManager, NULL ) != 0)
	{
		printf("Queue 관리 시스템  실행 오류 입니다.\n");
		return -1;
	}

	if (pthread_create(&DnLogDBPoolQueMngThreadID, NULL, DnLogDBPoolQueManager, NULL ) != 0)
	{
		printf("DB Pool Queue 관리 시스템 실행 오류 입니다.\n");
		return -1;
	}

	if (pthread_create(&DnLogQueMngThreadID, NULL, DnLogQueManager, NULL ) != 0)
	{
		printf("Queue 관리 시스템  실행 오류 입니다.\n");
		return -1;
	}

    for (;;) /* run forever */
    {
		struct sockaddr_in ClntAddr;
		unsigned int clntLen;

		memset(&ClntAddr,0x00,sizeof(ClntAddr));
		clntLen = sizeof(ClntAddr);

		if ((clntSock = accept(servSock, (struct sockaddr *) &ClntAddr, &clntLen)) < 0)
		{
			char szErrMsg[255];
			memset(szErrMsg,0x00,sizeof(szErrMsg));

			int err_num =errno;
			GetErrMsg(err_num,szErrMsg);
			infLOG(ERROR, "----------| Critical Error accept failed  %d %s\n",err_num,szErrMsg);
			sleep(1);
			break;
		}

		//infLOG(ALWAY,"----------|  Handling client ( %s ) port ( %d ) socket ( %d ) |----------\n", inet_ntoa(ClntAddr.sin_addr),ClntAddr.sin_port,clntSock);

		LPUSERINFO pUserInfo = new USERINFO;
		sprintf(pUserInfo->thread.userIP ,"%s", inet_ntoa(ClntAddr.sin_addr));
		pUserInfo->thread.clntSock = clntSock;

		if (pthread_create(&threadID, NULL, ThreadMain, (void *) pUserInfo) != 0)
		{
			char szErrMsg[255];
			memset(szErrMsg,0x00,sizeof(szErrMsg));
			int err_num =errno;
			GetErrMsg(err_num,szErrMsg);

			infLOG(ERROR, "----------| 쓰레드 생성 실패 : %ld %d %s |----------\n",threadID,err_num,szErrMsg);

		}

    }

    delete m_g_clMysqlPool;
    delete m_g_clMysqlPoolCopyRight;
    delete m_g_clMysqlPoolDnLog;

    infLOG(ERROR, "----------| 프로그램 종료 |----------\n\n\n\n\n\n\n\n\n");

    return 1;
    /* NOT REACHED */
}

void* DBPoolQueManager( void *threadArgs )
{
	pthread_detach(pthread_self());
	m_g_clMysqlPool->CreateDBPoolMngThread();
}

void* QueManager(void *threadArgs)
{
	pthread_detach(pthread_self());
	m_g_clMysqlPool->CreateQueMngThread();

}

void* CprDBPoolQueManager( void *threadArgs )
{
	pthread_detach(pthread_self());
	m_g_clMysqlPoolCopyRight->CreateDBPoolMngThread();
}

void* CprQueManager(void *threadArgs)
{
	pthread_detach(pthread_self());
	m_g_clMysqlPoolCopyRight->CreateQueMngThread();
}

/*
void* OpDBPoolQueManager( void *threadArgs )
{
	pthread_detach(pthread_self());
	m_g_clMysqlPoolOp->CreateDBPoolMngThread();
}

void* OpQueManager(void *threadArgs)
{
	pthread_detach(pthread_self());
	m_g_clMysqlPoolOp->CreateQueMngThread();
}
*/
void* DnLogDBPoolQueManager( void *threadArgs )
{
	pthread_detach(pthread_self());
	m_g_clMysqlPoolDnLog->CreateDBPoolMngThread();
}

void* DnLogQueManager(void *threadArgs)
{
	pthread_detach(pthread_self());
	m_g_clMysqlPoolDnLog->CreateQueMngThread();
}

