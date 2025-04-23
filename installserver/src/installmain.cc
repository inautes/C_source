/******************************************************************************
 *   서브시스템 : Install파일 전송 서버
 *   프로그램명 : instmain.cc
 *         기능 : installserver의 Main
 *         설명 :
 *       작성자 : HCS
 *       작성일 : 2009/04/23
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
#include <sys/ioctl.h>
#include <signal.h>

#include "Property.h"
#include "installdefine.h"
#include "installmain.h"
#include "installsock.h"
#include "installproc.h"
#include "installcomlib.h"

#include "apdefine.h"
#include "comcomm.h"
#include "comconf.h"

using namespace std;

/*===========================================================================*/
/* 전역변수 선언															 */
/*===========================================================================*/

SUserParm_T gstUserParm;
char	errMsg[256];
int 	gnMode;

char	g_szOsp_id[10] = {0,};
char	g_szFilepath[256]= {0,};
int		g_nPort = 0;


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
*Main
* (I) 1. int argc    : Argument Count 수
*     2. char **argv : Argunment Data Point
*****************************************************************************/

int main(int argc, char** argv)
{
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
int infInitProcess(int argc, char** argv)
{
    infLOG(ALWAY, "================= 프로그램 시작 ====================\n");
    return RETOK;
}

/*****************************************************************************
* 프로그램 종료
* (I) void
* (R) void
*****************************************************************************/
void infTermProcess()
{
    infLOG(ALWAY, "================= 프로그램 종료 ====================\n");
    exit(0);
}

/*****************************************************************************
* SIGNAL HANDLING
* (I) int nSigNo : Signal Number
* (R) void
*****************************************************************************/
void infSigHandler(int nSigNo, siginfo_t *siginfo,void* p)
{
	infLOG(ALWAY, " ] 예외 신호 처리 : 신호 번호 [%d]\n", nSigNo);
	sigset_t sigset, oldset;
    sigfillset(&sigset);

    // 새로들어오는 모든 시그널에 대해서 block 한다.
    if (sigprocmask(SIG_BLOCK, &sigset, &oldset) < 0)
    {
    	infLOG(ERROR, " ] 신호 mask 처리 오류 : 신호 번호 [%d]\n", nSigNo);
    }

	multimap<int,USERINFO>::iterator mi;

	mi = m_UserList.find(siginfo->si_pid);
	if( mi != m_UserList.end())
	{
		m_UserList.erase(mi);

		infLOG(ALWAY, " ] Close Socket\n (%d) (%s)\n",mi->first,mi->second.szUserID);

		close(mi->first);
	}
	infLOG(ALWAY, " ] 예외 신호 처리 종료 : 신호 번호 [%d]\n", nSigNo);
}

int infMainRoutine(int argc,char** argv)
{


	Property pP;
	pP.SetProcName(argv[0]);
	pP.GetStrProperty("[INFO]", "OSP_ID"			, g_szOsp_id);
	pP.GetIntProperty("[INFO]", "PORT"				, g_nPort);
	pP.GetStrProperty("[INFO]", "INSTALL_FILEPATH"	, g_szFilepath);

	infSetUserParm(&gstUserParm, argc, argv);

	if(	g_nPort == 0 ||
		strlen(g_szFilepath) == 0
	  )
	{
		infLOG(ERROR, "Error .cfg option \n");
		return -1;
	}

	infLOG(ALWAY, " OSP_ID [%s]\n", g_szOsp_id);
	infLOG(ALWAY, " PORT [%d]\n", g_nPort);
	infLOG(ALWAY, " INSTALL_FILEPATH [%s]\n", g_szFilepath);

    int servSock;                    /* Socket descriptor for server */
    int clntSock;                    /* Socket descriptor for client */
    unsigned short ServPort;     /* Server port */
    pthread_t threadID;              /* Thread ID from pthread_create() */

    struct ThreadArgs *threadArgs;   /* Pointer to argument structure for thread */

	signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGINT,  SIG_IGN);
    signal(SIGKILL, SIG_IGN);
    signal(SIGHUP,  SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

	ServPort = g_nPort;

    servSock = CreateTCPServerSocket(ServPort);
    if(servSock < 0)
    	return -1;

    for (;;)
    {
		//////////////////////////////// accept //////////////////////////////

	    struct sockaddr_in ClntAddr; // Client address
	    unsigned int clntLen;            //Length of client address data structure

	    // Set the size of the in-out parameter
	    clntLen = sizeof(ClntAddr);

	     //Wait for a client to connect
	    if ((clntSock = accept(servSock, (struct sockaddr *) &ClntAddr, &clntLen)) < 0)
	    {
	    	DieWithError("accept() failed");
	    }

	    // clntSock is connected to a client!
	    #ifdef __DEBUG
	    printf("Handling client %s  socket num == %d	\n", inet_ntoa(ClntAddr.sin_addr),clntSock);
	    #endif

 		///////////////////////////////accept end /////////////////////////////////

		LPUSERINFO pUserInfo = new USERINFO;
		sprintf(pUserInfo->thread.userIP ,"%s", inet_ntoa(ClntAddr.sin_addr));
		pUserInfo->thread.clntSock = clntSock;
		//hcs RS_SET_INSTALLSERVER_IP

        if (pthread_create(&threadID, NULL, ThreadMain, (void *) pUserInfo) != 0)
        {
        	#ifdef __DEBUG
      		printf(" ] pthread_create() failed %s socket num == %d	\n", inet_ntoa(ClntAddr.sin_addr),clntSock);
      		#endif
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
