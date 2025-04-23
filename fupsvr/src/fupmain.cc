/******************************************************************************
 *   서브시스템 : File Upload 서버
 *   프로그램명 : fupmain.cc
 *         기능 : FUP서버의 Main
 *         설명 :
 *       작성자 : LEE
 *       작성일 : 2004/02/16
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

#include "fupdefine.h"
#include "fupsock.h"
#include "fupproc.h"
#include "apstruct.h"
#include "apdefine.h"
#include "fupmain.h"
#include "comcomm.h"
#include "comconf.h"
#include "fupcomlib.h"


using namespace std;



//#define __DEBUG

/*===========================================================================*/
/* 전역변수 선언															 */
/*===========================================================================*/


SUserParm_T gstUserParm;
char	errMsg[256];

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
//int 	CheckUser();
/*****************************************************************************
* CMD서버 Main
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

/*******************************************************************************
* SOCKET CLOSE
* (I) SUserParm_T *pUser : 사용자 정의 변수 구조체
* (R) int iSockId : Socket Connect ID
*******************************************************************************/
int     infReleaseSocket()
{

    return 0;
}

/*****************************************************************************
* SIGNAL HANDLING
* (I) int nSigNo : Signal Number
* (R) void
*****************************************************************************/
//void    infSigHandler(int nSigNo)
void infSigHandler(int nSigNo, siginfo_t *siginfo, void* p) 
{

	infLOG(ALWAY, "infMainRoutine       ] infSigHandler`SigNo[%d]\n", nSigNo);

	sigset_t sigset, oldset;
    sigfillset(&sigset);
    // 새로들어오는 모든 시그널에 대해서 block 한다. 
    if (sigprocmask(SIG_BLOCK, &sigset, &oldset) < 0)
    {
		infLOG(ERROR, "infMainRoutine    ERR] infSigHandler`SigNo[%d]\n", nSigNo);    	
    	#ifdef __DEBUG
        printf("infMainRoutine       ] sigprocmask %d error \n", nSigNo);
        #endif
    }
    
	multimap<int,USERINFO>::iterator mi;
	
	mi = m_UserList.find(siginfo->si_pid);
	if( mi != m_UserList.end())
	{
		m_UserList.erase(mi);
		
		#ifdef __DEBUG
		printf("infMainRoutine       ] close socket %d\n",mi->first);
		#endif
		infLOG(ALWAY, "infMainRoutine       ] Close Socket ( %d ) \n",mi->first);
		
		close(mi->first);
	
	}
/*    
    mi = m_UserList.begin();
	while(mi != m_UserList.end())
	{
		#ifdef __DEBUG
		printf("pipe error search %d  == %d\n",mi->first,siginfo->si_pid);
		#endif
		if(mi->first == siginfo->si_pid)
		{
		
			m_UserList.erase(mi);
	
			#ifdef __DEBUG
			printf(" -- >close socket %d\n",mi->first);
			#endif
infLOG(ALWAY, "Close Socket\n");

			close(mi->first);
							
			break;
		}
		mi++;
	}

*/
infLOG(ALWAY, "infMainRoutine       ] infSigHandler`SigNo[%d]\n", nSigNo);
	
/*    int i;

    infLOG(ALWAY, "infSigHandler`SigNo[%d]\n", nSigNo);

  
    infReleaseSocket();

    infTermProcess();*/
}



int infMainRoutine(int argc,char** argv)
{   
	
	

	
	int servSock;                    /* Socket descriptor for server */
    int clntSock;                    /* Socket descriptor for client */
    unsigned short ServPort;     /* Server port */
    pthread_t threadID;              /* Thread ID from pthread_create() */
    
    struct ThreadArgs *threadArgs;   /* Pointer to argument structure for thread */

	
	if(argc == 2 && strcmp(argv[1], "-v") == 0)
	{
		printf("fupserver version 9,1,2,4,1\n");
		exit(1);
	}

    if (argc != 2)     /* Test for correct number of arguments */
    {
        printf("Usage:  %s <SERVER PORT>\n", argv[0]);
        exit(1);
    }
    struct sigaction intsig; 
	
	intsig.sa_sigaction = infSigHandler; 
	    	

	sigemptyset(&intsig.sa_mask); 
	intsig.sa_flags = SA_SIGINFO; 
	
	if (sigaction(SIGPIPE, &intsig, 0) == -1) 
	{
		#ifdef __DEBUG
		printf("infMainRoutine       ] sigaction error ( SIGPIPE )\n");
		#endif
		infLOG(ERROR, "infMainRoutine       ] sigaction error ( SIGPIPE ) \n"); 
	}
//    signal(SIGPIPE, infSigHandler); // 닫힌 소켓에 쓰려는 시도..
/*
    signal(SIGTERM, infSigHandler);
    signal(SIGINT,  infSigHandler);
    signal(SIGQUIT, infSigHandler);
    signal(SIGKILL, infSigHandler);
    signal(SIGHUP,  SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
  */  
    infSetUserParm(&gstUserParm, argc, argv);

	/*
	실제 소켓 연결전에 T_SERVER_INFO 의 up_user와 up_size를 0으로 초기화
	*/


/*	CCOM9202_R com9202_r;
	
	memset(&com9202_r, 0x00, sizeof(CCOM9202_R));
	
	sprintf(com9202_r.server_id, "%s", getenv("HOSTNAME"));

	infLOG(ALWAY,"getenv() = %s     server_id = %s\n", getenv("HOSTNAME"), com9202_r.server_id);

	if(com9202(com9202_r) < 0)
		infLOG(ERROR,"초기화 실패");	
*/



    ServPort = atoi(argv[1]);  /* First arg:  local port */
    #ifdef __DEBUG
	//pthread_create(&threadID,NULL,mon_thread,(void**)NULL);
	#endif
    servSock = CreateTCPServerSocket(ServPort);
    
    
	
	
    for (;;) /* run forever */
    {

	//	clntSock = AcceptTCPConnection(servSock);

//////////////////////////////// accept //////////////////////////////
		/*
		1시간마다 한번씩 접속자수 확인
		*/
		//CheckUser();
	
	    struct sockaddr_in ClntAddr; /* Client address */
	    unsigned int clntLen;            /* Length of client address data structure */
	
	    /* Set the size of the in-out parameter */
	    clntLen = sizeof(ClntAddr);
	    
	    /* Wait for a client to connect */
	    if ((clntSock = accept(servSock, (struct sockaddr *) &ClntAddr, &clntLen)) < 0)
	    {
	    	char szErrMsg[255];
	    	memset(szErrMsg,0x00,255);
	    	
	    	int err_num =errno;
	    	GetErrMsg(err_num,szErrMsg);
	    	
	    	#ifdef __DEBUG
	    	printf("infMainRoutine       ] accept failed  %d %s\n",err_num,szErrMsg);
	    	#endif

			infLOG(ERROR, "infMainRoutine       ] accept failed  %d %s\n",err_num,szErrMsg);
			
			sleep(1);    	
	    	break;
	    }
	    
	    /* clntSock is connected to a client! */
	    
	    #ifdef __DEBUG
	    printf("infMainRoutine       ] Handling client %s port = %d socket num == %d	\n", inet_ntoa(ClntAddr.sin_addr),ClntAddr.sin_port,clntSock);
	    #endif
	    
		
    
 ///////////////////////////////accept end /////////////////////////////////
 
        /* Create separate memory for client argument */
      /*  if ((threadArgs = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs))) 
               == NULL)
            DieWithError("malloc() failed");
   */
        
		LPUSERINFO pUserInfo = new USERINFO;
		sprintf(pUserInfo->thread.userIP ,"%s", inet_ntoa(ClntAddr.sin_addr));
		pUserInfo->thread.clntSock = clntSock;
		
		

        /* Create client thread */
        if (pthread_create(&threadID, NULL, ThreadMain, (void *) pUserInfo) != 0)
        {
        	printf("infMainRoutine       ] main thread create error\n");
    	}
		else
		{
			#ifdef __DEBUG
			printf("infMainRoutine       ] Create with thread %ld\n", (long int) threadID);
			#endif
			
			//mutex
		}
    }
    return 1;
    /* NOT REACHED */
}

