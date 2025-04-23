/******************************************************************************
 *   프로그램명 : 다운로드 리스트 생성 (통합 db 참조)
 *       작성자 : 김일오
 *       작성일 : 
 *              : cmd bck 서버에서 임시로 돌리기로함.
				: SUB DB 별로 한개씩 총 10개
				: PORT  9501~9510
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/

#include "dcmdmain.h"
//#define __DEBUG

/*===========================================================================*/
/* 전역변수 선언															 */
/*===========================================================================*/
CMysqlPool* m_g_clMysqlPool;
CMysqlPool* m_g_clMysqlPoolHadoop;

SUserParm_T gstUserParm;
char		errMsg[256];

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

void infSigHandler(int nSigNo, siginfo_t *siginfo, void* p) 
{
	sigset_t sigset, oldset;
    sigfillset(&sigset);

    if (sigprocmask(SIG_BLOCK, &sigset, &oldset) < 0)
    {
		infLOG(ERROR, "infSigHandler    ERR] infSigHandler`SigNo[%d]\n", nSigNo);    	
    }
   
}


int infMainRoutine(int argc,char** argv)
{   
	infSetUserParm(&gstUserParm, argc, argv);

	int servSock;                    /* Socket descriptor for server */
    int clntSock;                    /* Socket descriptor for client */
    int ServPort=0;		 /* Server port */

    pthread_t threadID;              /* Thread ID from pthread_create() */
    pthread_t QueMngThreadID;              /* Thread ID from pthread_create() */
    pthread_t DBPoolQueMngThreadID;              /* Thread ID from pthread_create() */
    
    pthread_t CprQueMngThreadID;              /* Thread ID from pthread_create() */
    pthread_t CprDBPoolQueMngThreadID;              /* Thread ID from pthread_create() */
	pthread_t HadoopMngThreadID;		// 통합 db
	pthread_t HadoopPoolMngThreadID;
    
	pthread_t DnLogQueMngThreadID;              /* Thread ID from pthread_create() */
    pthread_t DnLogDBPoolQueMngThreadID;              /* Thread ID from pthread_create() */

    struct ThreadArgs *threadArgs;   /* Pointer to argument structure for thread */

	char	szHadoopIP[16]; memset(szHadoopIP,0x00,sizeof(szHadoopIP));
	char	szOspDBIP[16];  memset(szOspDBIP,0x00,sizeof(szOspDBIP));
	
	
	Property pP; 
	pP.SetProcName(argv[0]);
	pP.GetStrProperty("[INFO]", "TONG_IP"			, szHadoopIP);		
	pP.GetStrProperty("[INFO]", "OSP_IP"			, szOspDBIP);		
	pP.GetIntProperty("[INFO]", "DAEMON_PORT"		, ServPort  );		
	
	infLOG(ALWAY, "szOspDBIP = %s\n" ,szOspDBIP);	
	infLOG(ALWAY, "szHadoopIP = %s\n",szHadoopIP);	
	infLOG(ALWAY, "ServPort = %d\n",ServPort);	
	
	if(	strlen(szHadoopIP) == 0 ||
		strlen(szOspDBIP) == 0 ||
		ServPort == 0 )
	{
		infLOG(ERROR, "Error .cfg option \n");    
		return -1;
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
 

    servSock = CreateTCPServerSocket(ServPort);
    
    infLOG(ALWAY, "----------| 서버 초기화 ( %ld ) |----------\n\n\n" ,getpid()); 
	
	
	char szHostName[128];
	memset(szHostName,0x00,sizeof(szHostName));
	
	char szTemp[2048];
	memset(szTemp,0x00,sizeof(szTemp));
		
	char* pHost = getenv("HOME");
	
	if( pHost != NULL )
		strcpy(szHostName,pHost);
	else
	{
		infLOG(ALWAY, "----------| 호스트 이름을 가져 올 수 없습니다.  |----------\n"
					  "----------| 중개서버 로그가 작동 안 할 수도 있습니다.  |----------\n\n\n" ); 
	}

	// 로그 위치 변경 2015.02.24
	strcpy(szTemp,"/logs");
	
	m_g_clMysqlPool = new CMysqlPool();
	m_g_clMysqlPool->UseMutexSchedule(true);
	m_g_clMysqlPool->SetLogMark("Normal");
	m_g_clMysqlPool->SetLogInfo(szTemp,"MysqlPool");

	m_g_clMysqlPoolHadoop = new CMysqlPool();
	m_g_clMysqlPoolHadoop->UseMutexSchedule(true);
	m_g_clMysqlPoolHadoop->SetLogMark("MysqlHadoop");
	m_g_clMysqlPoolHadoop->SetLogInfo(szTemp,"MysqlHadoopPool");

	#ifdef __DEBUG
	m_g_clMysqlPool			->SetDB( OSP_DB_NAME , szOspDBIP  , OSP_DB_USER , OSP_DB_PASS );
	m_g_clMysqlPoolHadoop	->SetDB( TONG_DB_NAME, szHadoopIP , TONG_DB_USER, TONG_DB_PASS ); // 통합 db qc
	#else
	m_g_clMysqlPool			->SetDB( OSP_DB_NAME , szOspDBIP  , OSP_DB_USER , OSP_DB_PASS );		// we main bck "61.252.171.37"
	m_g_clMysqlPoolHadoop	->SetDB( TONG_DB_NAME, szHadoopIP , TONG_DB_USER, TONG_DB_PASS ); // 통합 db main "183.110.46.38"
	#endif

	m_g_clMysqlPool->CreaeMysqlPool( 10 ,  20);
	m_g_clMysqlPoolHadoop->CreaeMysqlPool(10, 20);
	
	
	infLOG(ALWAY, "DBPoolQueMngThreadID ==  \n");	
	if (pthread_create(&DBPoolQueMngThreadID, NULL, DBPoolQueManager, NULL ) != 0)
	{
		printf("DB Pool Queue 관리 시스템 실행 오류 입니다.\n");
		return -1;	
	}
	infLOG(ALWAY, "QueMngThreadID ==  \n");	
	if (pthread_create(&QueMngThreadID, NULL, QueManager, NULL ) != 0)
	{
		printf("Queue 관리 시스템  실행 오류 입니다.\n");
		return -1;	
	}
	infLOG(ALWAY, "HadoopPoolMngThreadID ==  \n");
	if (pthread_create(&HadoopPoolMngThreadID, NULL, HadoopPoolManager, NULL ) != 0)
	{
		printf("Hadoop Pool Queue 관리 시스템 실행 오류 입니다.\n");
		return -1;	
	}
	infLOG(ALWAY, "HadoopMngThreadID ==  \n");
	if (pthread_create(&HadoopMngThreadID, NULL, HadoopManager, NULL ) != 0)
	{
		printf("Hadoop 관리 시스템  실행 오류 입니다.\n");
		return -1;	
	}

    for (;;) /* run forever */
    {

	//	clntSock = AcceptTCPConnection(servSock);
//////////////////////////////// accept //////////////////////////////
	
	    struct sockaddr_in ClntAddr; /* Client address */
	    unsigned int clntLen;            /* Length of client address data structure */
	    
		memset(&ClntAddr,0x00,sizeof(ClntAddr));
	    
	    clntLen = sizeof(ClntAddr);
		//infLOG(ALWAY, "----------| Accept Socket |----------\n");	    
		
	    /* Wait for a client to connect */
	    if ((clntSock = accept(servSock, (struct sockaddr *) &ClntAddr, &clntLen)) < 0)
	    {
	    	char szErrMsg[255];
	    	memset(szErrMsg,0x00,255);
			
	    	
	    	int err_num =errno;
	    	GetErrMsg(err_num,szErrMsg);
	    	
	    	#ifdef __DEBUG
	    	printf("----------|  accept failed  %d %s\n",err_num,szErrMsg);
	    	#endif

			infLOG(ERROR, "----------| Critical Error accept failed  %d %s\n",err_num,szErrMsg);
			
			sleep(1);
	    	break;
	    }
	    	
		//infLOG(ALWAY,"----------|  Handling client ( %s ) port ( %d ) socket ( %d ) |----------\n", inet_ntoa(ClntAddr.sin_addr),ClntAddr.sin_port,clntSock);

		LPUSERINFO pUserInfo = new USERINFO;
		sprintf(pUserInfo->thread.userIP ,"%s", inet_ntoa(ClntAddr.sin_addr));
		pUserInfo->thread.clntSock = clntSock;
		
		

        /* Create client thread */
        if (pthread_create(&threadID, NULL, ThreadMain, (void *) pUserInfo) != 0)
        {
        	

    		char szErrMsg[255];
	    	memset(szErrMsg,0x00,255);
	    	
	    	int err_num =errno;
	    	GetErrMsg(err_num,szErrMsg);

			infLOG(ERROR, "----------| 쓰레드 생성 실패 : %d %s |----------\n",err_num,szErrMsg);
		 	#ifdef __DEBUG
		 	printf("쓰레드 생성 실패 (%ld) : %d %s\n",threadID,err_num,szErrMsg);
        	#endif			        	
    	}

		
	    
	    /* clntSock is connected to a client! */
	    
	    #ifdef __DEBUG
	    printf("----------|  Handling client %s port = %d socket num == %d	\n", inet_ntoa(ClntAddr.sin_addr),ClntAddr.sin_port,clntSock);
	    #endif
	    
		
    
 ///////////////////////////////accept end /////////////////////////////////
 
        /* Create separate memory for client argument */
      /*  if ((threadArgs = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs))) 
               == NULL)
            DieWithError("malloc() failed");
   */
       
        
		
    }
    delete m_g_clMysqlPool;
	delete m_g_clMysqlPoolHadoop;
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

void* HadoopPoolManager( void *threadArgs )
{
	infLOG(ALWAY, "HadoopPoolManager ==  \n");
	pthread_detach(pthread_self()); 
	m_g_clMysqlPoolHadoop->CreateDBPoolMngThread();

}

void* HadoopManager(void *threadArgs)
{
	infLOG(ALWAY, "HadoopManager ==  \n");
	pthread_detach(pthread_self()); 
	m_g_clMysqlPoolHadoop->CreateQueMngThread();

}

