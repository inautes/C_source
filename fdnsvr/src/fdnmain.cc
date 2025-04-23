#include <map>
#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h> /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <errno.h>
#include <sys/stat.h>
#include <time.h>		//randomize()
#include <pthread.h>        /* for POSIX threads */
#include <sys/ioctl.h>
#include <signal.h>

#include "Property.h"
#include "fdndefine.h"
#include "fdnsock.h"
#include "fdnproc.h"
#include "fdncomlib.h"

#include "apstruct.h"
#include "apdefine.h"
#include "fdnmain.h"
#include "comcomm.h"
#include "comconf.h"
using namespace std;
//#define __DEBUG

#define STACK_SIZE ( 4 * 1024 )
static int is_altstack_defined =0 ;
static char tmp_stack[STACK_SIZE];

SUserParm_T gstUserParm;
char	errMsg[256];

char g_szOsp_id				[10] = {0,};
int  g_nPort = 0;
char g_szDcmdIP				[16] = {0,};
int  g_nDcmdPort = 0;
char g_szSUB_DcmdIP			[16] = {0,};
int  g_nSUB_DcmdPort = 0;
bool g_bConnect1 = true;
bool g_bConnect2 = true;

multimap<int,USERINFO>m_UserList;

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
* SIGNAL HANDLING
* (I) int nSigNo : Signal Number
* (R) void
*****************************************************************************/
//void    infSigHandler(int nSigNo)
void infSigHandler(int nSigNo, siginfo_t *siginfo,void* p)
{

	infLOG(ERROR,"infSigHandler [%d]\n", nSigNo);
	sigset_t sigset, oldset;
	sigfillset(&sigset);

	exit(-1);

}


static void register_sigaltstack()
{
    stack_t newSS, oldSS;

    if(is_altstack_defined)
    {
        return;
    }

    newSS.ss_sp = tmp_stack;

    newSS.ss_size = STACK_SIZE;
    newSS.ss_flags = 0;

    if(sigaltstack(&newSS, &oldSS) < 0)
    {
        printf ("error altstack");
    }

    is_altstack_defined = 1;
 }


void* AliveChkManager(void *threadArgs)
{

	while(1)
	{

		struct sockaddr_in dcmdSerAddr;
		int dcmd_socket;

		struct timeval tv;
		tv.tv_sec = 1; //1초

		int st = setsockopt(dcmd_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));

		struct timeval tv2;
		tv2.tv_sec = 1; //1초

		st = setsockopt(dcmd_socket, SOL_SOCKET, SO_SNDTIMEO, &tv2, sizeof(struct timeval));



		if ( ( dcmd_socket = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0 )
		{
			infLOG(ERROR, "] AliveChkManager : sock create error.\n");
			exit(0);
		}

		memset(&dcmdSerAddr,0x00,sizeof(dcmdSerAddr));
		dcmdSerAddr.sin_family      = AF_INET;
		dcmdSerAddr.sin_addr.s_addr = inet_addr(g_szDcmdIP);
		dcmdSerAddr.sin_port        = htons(g_nDcmdPort);

		if( connect(dcmd_socket,(struct sockaddr * )&dcmdSerAddr,sizeof(dcmdSerAddr)) < 0 )
		{
			g_bConnect1 = false;
			infLOG(ERROR, " ] AliveChkManager connect error  : [ %s ] [ %d ]  \n"	,g_szDcmdIP,g_nDcmdPort);
			infLOG(ERROR, " ] Err msg  : [ %d ] [ %s ]  \n"	,errno,strerror(errno));

			dcmdSerAddr.sin_addr.s_addr = inet_addr(g_szSUB_DcmdIP);
			dcmdSerAddr.sin_port        = htons(g_nSUB_DcmdPort);

			if( connect(dcmd_socket,(struct sockaddr * )&dcmdSerAddr,sizeof(dcmdSerAddr)) < 0 )
			{
				g_bConnect2 = false;
				infLOG(ERROR, " ] AliveChkManager2 connect error : [ %s ] [ %d ] \n" ,g_szSUB_DcmdIP,g_nSUB_DcmdPort);
				infLOG(ERROR, " ] Err msg  : [ %d ] [ %s ]  \n"	,errno,strerror(errno));
			}
			else
			{
				g_bConnect2 = true;
			}
		}
		else
		{
			g_bConnect1 = true;
		}

		close(dcmd_socket);
		sleep(1);
	}

}

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


    infMainRoutine(argc, argv);
    infTermProcess();
}

int     infInitProcess(int argc, char** argv)
{
    infLOG(ALWAY, "================= infInitProcess Start v.20170425 ====================\n");
    return RETOK;
}


void    infTermProcess()
{
    infLOG(ALWAY, "================= infTermProcess End ====================\n");
    exit(0);
}

/*******************************************************************************
* SOCKET CLOSE
* (I) SUserParm_T *pUser :
* (R) int iSockId : Socket Connect ID
*******************************************************************************/
int     infReleaseSocket()
{
    return 0;
}


int infMainRoutine(int argc,char** argv)
{

	Property pP;
	pP.SetProcName(argv[0]);
	pP.GetStrProperty("[INFO]", "OSP_ID"		, g_szOsp_id);
	pP.GetIntProperty("[INFO]", "PORT"			, g_nPort);
	pP.GetStrProperty("[INFO]", "DCMD_IP"		, g_szDcmdIP);
	pP.GetIntProperty("[INFO]", "DCMD_PORT"		, g_nDcmdPort);
	pP.GetStrProperty("[INFO]", "SUB_DCMD_IP"	, g_szSUB_DcmdIP);
	pP.GetIntProperty("[INFO]", "SUB_DCMD_PORT"	, g_nSUB_DcmdPort);


	infSetUserParm(&gstUserParm, argc, argv);

	if(	strlen(g_szOsp_id) == 0 ||
		strlen(g_szDcmdIP) == 0 ||
		g_nPort < 1000 ||
		g_nDcmdPort < 1000 )
	{
		infLOG(ERROR, "Error .cfg option \n");
		return -1;
	}


    int servSock;
    int clntSock;
	pthread_t threadID;
    struct ThreadArgs *threadArgs;

	signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGINT,  SIG_IGN);
    signal(SIGKILL, SIG_IGN);
    signal(SIGHUP,  SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

	infLOG(ALWAY, "================= infMainRoutine ====================\n");
	

    pthread_t AliveThreadID;
	
	
	if (pthread_create(&AliveThreadID, NULL, AliveChkManager, NULL ) != 0)
	{
		infLOG(ERROR, "Hadoop 관리 시스템  실행 오류 입니다.\n");
		return -1;
	}

    servSock = CreateTCPServerSocket(g_nPort);
    if(servSock < 0)
    	return -1;


    for (;;) /* run forever */
    {
		//////////////////////////////// accept //////////////////////////////
		struct sockaddr_in ClntAddr; /* Client address */
		unsigned int clntLen;            /* Length of client address data structure */

		/* Set the size of the in-out parameter */
		clntLen = sizeof(ClntAddr);

		/* Wait for a client to connect */
		if ((clntSock = accept(servSock, (struct sockaddr *) &ClntAddr, &clntLen)) < 0)
		{
			DieWithError("accept() failed");
		}

		LPUSERINFO pUserInfo = new USERINFO;
		sprintf(pUserInfo->thread.userIP ,"%s", inet_ntoa(ClntAddr.sin_addr));
		pUserInfo->thread.clntSock = clntSock;
		time(&pUserInfo->curtime);

		infLOG(ALWAY, "pUserInfo->curtime = %d\n",pUserInfo->curtime);

	    if (pthread_create(&threadID, NULL, ThreadMain, (void *) pUserInfo) != 0)
	    {
	  		infLOG(ERROR, " ] pthread_create() failed %s socket num == %d	\n", inet_ntoa(ClntAddr.sin_addr),clntSock);
	    }
			else
			{
				#ifdef __DEBUG
				printf("Create with thread %ld\n", (long int) threadID);
				#endif

			}
    }
    return 1;

}
