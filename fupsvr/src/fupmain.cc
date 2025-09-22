/******************************************************************************
 *   ����ý��� : File Upload ����
 *   ���α׷��� : fupmain.cc
 *         ��� : FUP������ Main
 *         ���� :
 *       �ۼ��� : LEE
 *       �ۼ��� : 2004/02/16
 *     �����̷� :
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
#include "Property.h"


using namespace std;



//#define __DEBUG

/*===========================================================================*/
/* �������� ����															 */
/*===========================================================================*/


SUserParm_T gstUserParm;
char	errMsg[256];

multimap<int,USERINFO>m_UserList;

char g_szDcmdIP[20] = {0,};
int g_nDcmdPort = 0;
char g_szSUB_DcmdIP[20] = {0,};
int g_nSUB_DcmdPort = 0;

pthread_cond_t async_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t async_mutex = PTHREAD_MUTEX_INITIALIZER;



/*============================================================================*/
/* �Լ� ������Ÿ��                                                            */
/*============================================================================*/
int     infInitProcess(int, char**);
int     infMainRoutine(int, char**);
void    infTermProcess();
//int 	CheckUser();
/*****************************************************************************
* CMD���� Main
* (I) 1. int argc    : Argument Count ��
*     2. char **argv : Argunment Data Point
*****************************************************************************/

int main(int argc, char** argv)
{

	infMainRoutine(argc, argv);
	infTermProcess();
}
/*****************************************************************************
* ���α׷� ����
* (I) 1. int argc    : Argument Count ��
*     2. char **argv : Argunment Data Point
* (R) 1. ���� : '0'
*     2. ���� : ���� 
*****************************************************************************/
int     infInitProcess(int argc, char** argv) 
{
    infLOG(ALWAY, "================= ���α׷� ���� ====================\n"); 
    return RETOK;
}

/*****************************************************************************
* ���α׷� ����
* (I) void
* (R) void
*****************************************************************************/
void    infTermProcess()
{
    infLOG(ALWAY, "================= ���α׷� ���� ====================\n"); 
    exit(0);
}

/*******************************************************************************
* SOCKET CLOSE
* (I) SUserParm_T *pUser : ����� ���� ���� ����ü
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
    // ���ε����� ��� �ñ׳ο� ���ؼ� block �Ѵ�. 
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
//    signal(SIGPIPE, infSigHandler); // ���� ���Ͽ� ������ �õ�..
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
    // Property 설정 읽기 (dcmdserver 방식 참고)
    Property pP;
    pP.SetProcName(argv[0]);
    pP.GetStrProperty("[INFO]", "DCMD_IP", g_szDcmdIP);
    pP.GetIntProperty("[INFO]", "DCMD_PORT", g_nDcmdPort);
    pP.GetStrProperty("[INFO]", "SUB_DCMD_IP", g_szSUB_DcmdIP);
    pP.GetIntProperty("[INFO]", "SUB_DCMD_PORT", g_nSUB_DcmdPort);
    
    if(strlen(g_szDcmdIP) == 0 || g_nDcmdPort < 1000) {
        infLOG(ERROR, "Error .cfg DCMD option - DCMD_IP: %s, DCMD_PORT: %d\n", g_szDcmdIP, g_nDcmdPort);
        return -1;
    }
    
    infLOG(ALWAY, "DCMD Server Config - IP: %s, PORT: %d\n", g_szDcmdIP, g_nDcmdPort);
    infLOG(ALWAY, "SUB DCMD Server Config - IP: %s, PORT: %d\n", g_szSUB_DcmdIP, g_nSUB_DcmdPort);


	/*
	���� ���� �������� T_SERVER_INFO �� up_user�� up_size�� 0���� �ʱ�ȭ
	*/


/*	CCOM9202_R com9202_r;
	
	memset(&com9202_r, 0x00, sizeof(CCOM9202_R));
	
	sprintf(com9202_r.server_id, "%s", getenv("HOSTNAME"));

	infLOG(ALWAY,"getenv() = %s     server_id = %s\n", getenv("HOSTNAME"), com9202_r.server_id);

	if(com9202(com9202_r) < 0)
		infLOG(ERROR,"�ʱ�ȭ ����");	
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
		1�ð����� �ѹ��� �����ڼ� Ȯ��
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

