#define MAKE_MAPKEY(i) 	(-1000)*(i+1)
#define SLEEP_MAPKEY -1
#define ERASE_MAPKEY -2
#define READY_MAPKEY -3



#include "mysql_pool.h"
#include "stdlib.h"
#include "comconf.h"

CMysqlPool::CMysqlPool()
{
	m_nConnectTimeout = 0;

	m_ulAutoIncNum = 0;
	m_pMysqlCon = NULL;
	m_nPoolSize = 0;
	m_nMaxPoolSize = 0;
	m_nDefaultPoolSize = 0;
	m_nScheduleNum = 0;
	m_bMngThreadStop  = true;
	m_bMngThreadSleep = false;

/*
	m_DB_Pool_Mutex = PTHREAD_MUTEX_INITIALIZER;
	m_Schedule_Mutex = PTHREAD_MUTEX_INITIALIZER;
*/

	memset(	m_DB_Con , 0x00,sizeof(m_DB_Con));
	memset(	m_DB_Info , 0x00,sizeof(m_DB_Info));

	memset( m_szErrMsg , 0x00, sizeof(m_szErrMsg));

	memset( m_szDBName   , 0x00 , sizeof( m_szDBName));
	memset( m_szDBIP     , 0x00 , sizeof( m_szDBIP));
	memset( m_szUserID   , 0x00 , sizeof( m_szUserID));
	memset( m_szUserPass , 0x00 , sizeof( m_szUserPass));

	memset( m_szLogMark , 0x00 , sizeof( m_szLogMark));



	m_bUseDBMutex = false;
	m_bUseSCHMutex= false;
	m_bUseLogMark = false;
	m_bUseKeyMutex = false;
	//queue 셋팅

	//m_Log.setLog(" %s    |  CMysqlPool : Init CMysqlPool  \n" , m_szLogMark);

}

CMysqlPool::CMysqlPool(int nPoolSize , int nMaxPoolSize , bool bUseMutexSchedule = false )
{
	m_nConnectTimeout = 0;
	m_ulAutoIncNum = 0;

	m_nPoolSize = nPoolSize;
	m_pMysqlCon = NULL;
	m_nMaxPoolSize = nMaxPoolSize;
	m_nDefaultPoolSize = m_nPoolSize;

	m_nScheduleNum = 0;
	m_bMngThreadSleep = false;
	m_bMngThreadStop  = true;


	memset(	m_DB_Con , 0x00,sizeof(m_DB_Con));

	memset(	m_DB_Info , 0x00,sizeof(m_DB_Info));
	memset( m_szErrMsg , 0x00, sizeof(m_szErrMsg));

	memset( m_szDBName   , 0x00 , sizeof( m_szDBName));
	memset( m_szDBIP     , 0x00 , sizeof( m_szDBIP));
	memset( m_szUserID   , 0x00 , sizeof( m_szUserID));
	memset( m_szUserPass , 0x00 , sizeof( m_szUserPass));

	memset( m_szLogMark , 0x00 , sizeof( m_szLogMark));


/*
	m_DB_Pool_Mutex = PTHREAD_MUTEX_INITIALIZER;
	m_Schedule_Mutex = PTHREAD_MUTEX_INITIALIZER;
*/

	m_bUseMutex = false;
	m_bUseDBMutex = false;
	m_bUseSCHMutex = false;
	m_bUseLogMark = false;
	m_bUseKeyMutex =false;

	//m_Log.setLog(" %s    |  CMysqlPool : Init CMysqlPool  \n", m_szLogMark);

	//queue 셋팅
	m_Schedule.UseMutex(bUseMutexSchedule);

	CreaeMysqlPool( nPoolSize ,nMaxPoolSize );


	// nPoolSize 만큼 DB 연결


}
int CMysqlPool::SetLogInfo(char* pLogPath,char* pLogName)
{
	m_Log.initLog(pLogPath,pLogName);
	m_Schedule.m_Log.initLog(pLogPath,pLogName);
	m_DB_Pool.m_Log.initLog(pLogPath,pLogName);
}

/*****************************************************************************
* 시스템시간을  얻음 (HH:MI:SS:SEC)
*****************************************************************************/
unsigned long CMysqlPool::GetPrimaryKey()
{
	#ifdef __DEBUG
	printf("%d\n",m_ulAutoIncNum);
	#endif
	Key_Lock();
	m_ulAutoIncNum++;
	if( m_ulAutoIncNum > 200000000)
		m_ulAutoIncNum = 1;
	Key_UnLock();

	return m_ulAutoIncNum;

}
// Thread-Specific Data pthread_getspecific

void CMysqlPool::SetLogMark(char* pMark)
{
	if( pMark !=NULL)
	{
		if( strlen(pMark) < 512)
		{
			m_bUseLogMark = true;
			strcpyA(m_szLogMark,pMark, sizeof(m_szLogMark));
		}
	}
}
char* CMysqlPool::GetLogMark()
{
	return m_szLogMark;
}

void CMysqlPool::UseMutexSchedule(bool bUse)
{
	m_Schedule.UseMutex(bUse);
}


CMysqlPool::~CMysqlPool()
{

	//m_Log.setLog(" %s    |  CMysqlPool : Clear CMysqlPool  \n", m_szLogMark);
	CloseMysqlPool();
}
int CMysqlPool::CloseSchedule(long thread_id) // queue 에 해당 스레드 제거
{


	DB_SCHEDULE Schedule_info;
	/*
	for( int nIndex = 0 ; nIndex < m_nScheduleSize ; nIndex++ )
	{
		memset(&Schedule_info,0x00,sizeof(Schedule_info));
		Schedule_info = m_Schedule.front();
		m_Log.setLog(" %s    |  CMysqlPool : CloseSchedule Schedule_info.pid = thread_id ( %d ) == ( %d )  \n", m_szLogMark,Schedule_info.pid,thread_id);

		if( Schedule_info.pid == thread_id )
		{
			m_Schedule.pop();
			m_Log.setLog(" %s    |  CMysqlPool : CloseSchedule m_Schedule[%d].pop  \n", m_szLogMark,nIndex);

		}
	}
	*/

	if( m_Schedule.Find(thread_id,Schedule_info, true) )
	{
	//	m_Log.setLog(" %s    |  +++++++++++++++++++++++++         CMysqlPool : CloseSchedule : close schedule ( %ld ) \n", m_szLogMark,thread_id);
	}

	return -1;
}

int CMysqlPool::CloseDBPool(long thread_id)
{

	DB_INFO* pDB_INFO = m_DB_Pool.ReplaceKey(	thread_id ,READY_MAPKEY) ;
	if( pDB_INFO != NULL )
	{
//		m_Log.setLog(" %s    |  CMysqlPool : CloseDBPool (%ld) ( %x )  \n", m_szLogMark,thread_id,pDB_INFO->pMysqlCon);
		return 0;
	}

	return -1;
}

void CMysqlPool::CreateDBPoolMngThread()
{
	//DB Pool을 감시하여 빈것을 que에 넣어준다.

	//m_Log.setLog(" %s    |  \n ========================  DB Pool 에서 데이터를 읽어 들입니다. ======================== \n", m_szLogMark);



	m_DbPoolMngThreadID = getpid();
	m_bMngThreadStop = false;

	time_t ltime;
	time( &ltime );
	time_t cur_time ;
	int nCheckPoolTime = 0 ;

	int nPoolSize = 0;
	int nQueSize = 0 ;
	int nLogCnt = 0 ;




/* 시그널 처리로 바꿀 시
	sigset_t signalset;
	siginfo_t siginfo;
	int signum,  revents;

	sigemptyset(&signalset);
	sigaddset(&signalset, SIGRTMIN);

	struct timespec stTimeOut;

	struct timeval stNowTime;
    gettimeofday(&stNowTime, NULL);
    stTimeOut.tv_sec = stNowTime.tv_sec + 5;
*/



	while(!m_bMngThreadStop)
	{

		//time( &cur_time );
		//nCheckPoolTime++;


		//-m_Log.setLog(" %s    |  \n\n\n\n CreateDBPoolMngThread ==>   nPoolSize ( %d ) m_nPoolSize ( %d ) ======================== \n", m_szLogMark, nPoolSize , m_nPoolSize);

		nQueSize = GetToalScheduleSize();


		//nPoolSize = ( nQueSize/m_nPoolSize );
		nPoolSize = ((nQueSize - m_nPoolSize)/2) + m_nPoolSize;




		if( nLogCnt > 10000 )
		{

			//m_Log.setLog(" %s    |  CreateDBPoolMngThread :  DBPOOL검사 nPoolSize ( %d ) m_nPoolSize < m_nMaxPoolSize ( %d ) < ( %d ) nQueSize ( %d ) \n\n", m_szLogMark,nPoolSize , m_nPoolSize , m_nMaxPoolSize,nQueSize);
			//m_DB_Pool.ViewKeyList();
			nLogCnt = 0;
		}

		nLogCnt++;
		//여기 튜닝
		if(  nPoolSize > 1 && nPoolSize  > m_nPoolSize  && m_nPoolSize < m_nMaxPoolSize) //여기
		{


				//nQueSize = GetToalScheduleSize();
				//nPoolSize = m_nPoolSize + (nQueSize/5) ;
				//nPoolSize = ( nQueSize/m_nPoolSize );

			//m_Log.setLog(" %s    |  CreateDBPoolMngThread :   DB Pool 증가 AddPool nPoolSize ( %d ) m_nPoolSize ( %d ) nQueSize ( %d )======================== \n", m_szLogMark, nPoolSize , m_nPoolSize,nQueSize);
			#ifdef __DEBUG
			printf("\n\n\n\n CreateDBPoolMngThread :   DB Pool 증가 AddPool ( %d ) ( %d ) ======================== \n", nPoolSize , m_nPoolSize);
			#endif

			AddDBPool();

		}



		if( m_nPoolSize < 1 )
		{

			m_Log.setLog("\nCreateDBPoolMngThread :  치명적 오류 ]  DB Pool 의 사이즈가 음수 입니다. ======================== \n", m_szLogMark);
			m_nPoolSize = 0;
			AddDBPool();
		}




		DB_INFO * pDB_INFO  = m_DB_Pool.Find(READY_MAPKEY);




		if( pDB_INFO != NULL )
		{
			//m_Log.setLog(" %s    |  nPoolSize = (( nQueSize[ %d ] - m_nPoolSize[ %d ] )/2) + m_nPoolSize  \n", m_szLogMark, nQueSize, m_nPoolSize);

			m_DB_Pool.ReplaceKey(READY_MAPKEY,MAKE_MAPKEY(pDB_INFO->nIndex));
			m_DBPoolSchedule.push(MAKE_MAPKEY(pDB_INFO->nIndex));

			//m_Log.setLog(" %s    |  CreateDBPoolMngThread :  DB Pool 관리자에 ( %d ) ( %d ) 추가 ======================== \n", m_szLogMark, pDB_INFO->nIndex,MAKE_MAPKEY(pDB_INFO->nIndex));
		}
		else
		{
			usleep(10);
		}

	}

	//printf("\n\n-------------------- 종료 ------------------- \n");


}

void CMysqlPool::CreateQueMngThread()
{

	//queue 검사
	//return pthread_create(&m_QueMngThreadID, NULL, QueMngThread, NULL ) ;
	//pthread_create(&m_QueMngThreadID, NULL, QueMngThread, NULL);
	m_QueMngThreadID = getpid();
/* 시그널 처리로 바꿀 시
	sigset_t signalset;
	siginfo_t siginfo;
	int signum,  revents;

	sigemptyset(&signalset);
	sigaddset(&signalset, SIGRTMIN);

	struct timespec stTimeOut;

	struct timeval stNowTime;
    gettimeofday(&stNowTime, NULL);
    stTimeOut.tv_sec = stNowTime.tv_sec + 5;




int sigwaitinfo(const sigset_t *set, siginfo_t *info);
int sigtimedwait(const sigset_t *set, siginfo_t *info, const
      struct timespec *timeout);
int sigqueue(pid_tpid, int sig, const union sigval value);

*/




	//m_Log.setLog(" %s    |  \nCreateQueMngThread ========================  Queue 에서 데이터를 읽어 드립니다. ======================== \n", m_szLogMark);
	m_bMngThreadStop = false;
	DB_INFO * pDB_INFO = NULL;


	DB_SCHEDULE stSchedule_info;
	int nLogCnt = 0 ;
	long lEraseCnt = 0;
	int nQueSize = 0 ;
	int nReadyDBPoolQueSize = 0 ;


	int nPoolSize= 0;

	long nDBPoolMapKey = 0 ;
	while(!m_bMngThreadStop)
	{

		nQueSize = GetToalScheduleSize();
		//nPoolSize = ( nQueSize/m_nPoolSize );
		nPoolSize = ((nQueSize - m_nPoolSize)/2) + m_nPoolSize;

		lEraseCnt ++;
		nLogCnt++;
		if( nLogCnt > 1000 )
		{
//			m_Log.setLog(" %s    |   CreateQueMngThread : Que 검사 ( %d ) 준비된 DB POOL  ( %d ) nPoolSize ( %d )  m_nPoolSize ( %d )======================== \n", m_szLogMark,nQueSize ,nReadyDBPoolQueSize,nPoolSize,m_nPoolSize);
			//m_Schedule.ViewKeyList();
			nLogCnt = 0;
		}

		if( lEraseCnt > 72000 ) // 12초 = 6000
		{

			//m_Schedule.ViewKeyList();

//			m_Log.setLog(" %s    |   CreateQueMngThread : EraseDBPool 검사 nPoolSize ( %d )  m_nPoolSize ( %d )======================== \n", m_szLogMark,nPoolSize,m_nPoolSize);

			if( nPoolSize < m_nPoolSize	)
			{
				/*
				m_Log.setLog(" %s    |   CreateQueMngThread : EraseDBPool  검사 : lEraseCnt >= (%d)  && nPoolSize ( %d ) > m_nDefaultPoolSize ( %d ) && m_nPoolSize ( %d ) > m_nDefaultPoolSize ( %d ) && m_nPoolSize ( %d ) > 1 && nPoolSize ( %d ) <  m_nPoolSize/2 ( %d )  \n"
															, m_szLogMark			,lEraseCnt          , nPoolSize          , m_nDefaultPoolSize       , m_nPoolSize         ,m_nDefaultPoolSize          ,m_nPoolSize              ,nPoolSize          ,m_nPoolSize/2 );
				*/
				if( /* nPoolSize > m_nDefaultPoolSize &&*/ m_nPoolSize > m_nDefaultPoolSize && m_nPoolSize > 0 )//&& nPoolSize < m_nPoolSize/2  )
				{
					EraseDBPool();
				}
			}


			lEraseCnt = 0;

		}




		if(  nQueSize <= 0 )
		{

			usleep(10);

			/* 시그널 처리로 바꿀 시
			do
			{
				signum = sigtimedwait (&signalset, &siginfo,&stTimeOut);
			}while(signum != SIGRTMIN && !m_bMngThreadStop )
			*/

			//wiat signal

		}
		else
		{

			nReadyDBPoolQueSize = m_DBPoolSchedule.size();


			//memset(&Schedule_info,0x00,sizeof(Schedule_info));
			if(  !m_DBPoolSchedule.empty()  ) //( pSchedule_info = m_Schedule.Front())!= NULL
			{


				//m_Log.setLog(" %s    |   준비된  ======================== \n", m_szLogMark);



					if(  m_Schedule.Pop(stSchedule_info) )
					{

						//m_Log.setLog(" %s    |  CreateQueMngThread  : make process  처음 키 ( %d ) -> 변경 키 ( %d )  ======================== \n", m_szLogMark,nDBPoolMapKey,stSchedule_info.pid);
						nDBPoolMapKey = 0 ;
						nDBPoolMapKey = m_DBPoolSchedule.front();
						m_DBPoolSchedule.pop();
						//m_Log.setLog(" %s    |  CreateQueMngThread  : m_DBPoolSchedule.front()  \n", m_szLogMark);
						//m_Log.setLog(" %s    |  CreateQueMngThread  : make process  nDBPoolMapKey ( %d )  stSchedule_info.pid( %d )  ======================== \n", m_szLogMark,nDBPoolMapKey,stSchedule_info.pid);


						//m_Log.setLog(" %s    |   CreateQueMngThread : make process  Que 검사 ( %d ) 준비된 DB POOL ( %d ) nPoolSize ( %d ) m_nPoolSize ( %d )======================== \n", m_szLogMark,nQueSize ,nReadyDBPoolQueSize,nPoolSize,m_nPoolSize);

						DB_INFO * pDB_INFO = m_DB_Pool.ReplaceKey(nDBPoolMapKey,stSchedule_info.pid);

						//m_Log.setLog(" %s    |  CreateQueMngThread: stSchedule_info.pOwnerCheck ( %s ) pid( %ld ) \n", m_szLogMark,stSchedule_info.pOwnerCheck,stSchedule_info.pid);


						if( stSchedule_info.pOwnerCheck != NULL && strcmp(stSchedule_info.pOwnerCheck ,"li") == 0 )
						{
							//m_Log.setLog(" %s    |  CreateQueMngThread: WaitSchedule Send Signal ( %d ) ( %ld ) ( %x )  \n", m_szLogMark,nDBPoolMapKey,stSchedule_info.pid  ,pDB_INFO->pMysqlCon);
							#ifdef __DEBUG
							printf("CreateQueMngThread : WaitSchedule Send Signal ( %d ) ( %d ) ( %x )  \n",nDBPoolMapKey,stSchedule_info.pid  ,pDB_INFO->pMysqlCon);
							#endif

							if( pthread_cond_signal(stSchedule_info.pthread_cond) != 0 )
							{
								#ifdef __DEBUG
								printf(" %s    |  CreateQueMngThread: WaitSchedule Send Signal failed ( %d ) ( %ld ) ( %x ) ( %d ) ( %s )\n", m_szLogMark,nDBPoolMapKey,stSchedule_info.pid  ,pDB_INFO->pMysqlCon,errno,strerror(errno));
								#endif

								//m_Log.setLog(" %s    |  CreateQueMngThread: WaitSchedule Send Signal failed ( %d ) ( %ld ) ( %x ) ( %d ) ( %s )\n", m_szLogMark,nDBPoolMapKey,stSchedule_info.pid  ,pDB_INFO->pMysqlCon,errno,strerror(errno));
								CloseDBPool(stSchedule_info.pid);
							}
							#ifdef __DEBUG
							printf(" %s    |  CreateQueMngThread: WaitSchedule Send Signal Sucessed ( %d ) ( %ld ) ( %x )  \n", m_szLogMark,nDBPoolMapKey,stSchedule_info.pid  ,pDB_INFO->pMysqlCon);
							#endif

							//m_Log.setLog(" %s    |  CreateQueMngThread: WaitSchedule Send Signal Sucessed ( %d ) ( %ld ) ( %x )  \n", m_szLogMark,nDBPoolMapKey,stSchedule_info.pid  ,pDB_INFO->pMysqlCon);
						}
						else
						{
							//m_Log.setLog(" %s    |  CreateQueMngThread: WaitSchedule Send Signal Failed ( %d ) ( %ld ) ( %x )  \n", m_szLogMark,nDBPoolMapKey,stSchedule_info.pid  ,pDB_INFO->pMysqlCon);
							CloseDBPool(stSchedule_info.pid);
						}

						if( stSchedule_info.pOwnerCheck  )
						{
							//m_Log.setLog(" %s    |  CreateQueMngThread: delete[] stSchedule_info.pOwnerCheck start\n", m_szLogMark);
							delete[] stSchedule_info.pOwnerCheck;
							stSchedule_info.pOwnerCheck = NULL;
							//m_Log.setLog(" %s    |  CreateQueMngThread: delete[] stSchedule_info.pOwnerCheck complete\n", m_szLogMark);
						}
						usleep(1);
					}
					else
					{
						m_Log.setLog(" %s    |  CreateQueMngThread: DB Pool Pop 오류입니다.\n", m_szLogMark);
						usleep(5);
					}


			}
			else
			{
				m_Log.setLog(" %s    |  CreateQueMngThread: DB Pool 이 없습니다.\n", m_szLogMark);
				usleep(5);


			}


		}

	}

	//m_Log.setLog(" %s    |  \nCreateQueMngThread  ======================== end ======================== \n", m_szLogMark);


}

MYSQL* CMysqlPool::GetMysqlCon(long thread_id)
{

	//m_Log.setLog(" %s    |  CMysqlPool : GetMysqlCon \n" , m_szLogMark);
	DB_INFO *pDB_INFO = NULL;

	pDB_INFO = m_DB_Pool.Find(thread_id);

	//m_Log.setLog(" %s    |  CMysqlPool : GetMysqlCon Find !! \n" , m_szLogMark);

	if( pDB_INFO != NULL )
	{

		//m_Log.setLog(" %s    |  CMysqlPool : GetMysqlCon -------> ( %d ) ( %d ) ( %x ) \n", m_szLogMark,thread_id,pDB_INFO->nIndex,pDB_INFO->pMysqlCon  );

		if( !IsConnectDB( pDB_INFO->pMysqlCon) )
		{
			ConnectDB(pDB_INFO->pMysqlCon);
		}

		#ifdef __DEBUG
		printf("CMysqlPool : GetMysqlCon -------> ( %d ) ( %x ) \n",thread_id,pDB_INFO->pMysqlCon  );
		#endif
		//m_Log.setLog(" %s    |  CMysqlPool : WaitSchedule Find Pool  \n", m_szLogMark);

		return pDB_INFO->pMysqlCon;


	}
	else
	{
		m_Log.setLog(" %s    |  CMysqlPool : GetMysqlCon -------> ( %d ) ( NULL ) \n", m_szLogMark,thread_id  );
		#ifdef __DEBUG
		printf("CMysqlPool : GetMysqlCon -------> ( %d ) ( NULL ) \n",thread_id );
		#endif

		return NULL;
	}


}

void * CMysqlPool::QueMngThread(void* pThreadArgs)
{
	pthread_detach(pthread_self());



}

void CMysqlPool::CreateSchedule(long thread_id , pthread_mutex_t* pthread_mutex , pthread_cond_t* pthread_cond ,char* pCheck)
// queue 에 해당 스레드 넣기
{

	//m_Log.setLog(" %s    |  CMysqlPool : CreateSchedule ( %ld ) \n", m_szLogMark,thread_id);
	time_t ltime;
	time( &ltime );
	/*
	DB_SCHEDULE data;
	data.pid = thread_id;
	data.st_time = ltime;
	data.pthread_mutex = pthread_mutex;
	data.pthread_cond = pthread_cond;
	*/


	//최소 사용 풀 선택

	int nScheduleNum = 0;


	DB_SCHEDULE stData;
	memset(&stData,0x00,sizeof(DB_SCHEDULE));
	//m_Log.setLog(" %s    |  CMysqlPool : CreateSchedule Make Data Mem ( %ld ) \n", m_szLogMark,thread_id);
	stData.pid = thread_id;
	stData.st_time = ltime;
	stData.pthread_mutex = pthread_mutex;
	stData.pthread_cond = pthread_cond;

	//여기 수정
	stData.pOwnerCheck  = pCheck ;

	//m_Log.setLog(" %s    |  CMysqlPool : CreateSchedule Push  ( %ld ) check String ( %s ) ( %s )\n", m_szLogMark,thread_id,stData.pOwnerCheck,pCheck);


	DB_SCHEDULE* pData = m_Schedule.Push(thread_id, &stData );

	//pData->pOwnerCheck  = pCheck ;

	//m_Log.setLog(" %s    |  CMysqlPool : CreateSchedule end ( %ld ) \n", m_szLogMark,thread_id);

}


bool CMysqlPool::IsConnectDB(MYSQL* pMysqlCon)
{

	//m_Log.setLog(" %s    |  CMysqlPool : IsConnectDB \n", m_szLogMark);
	//20120508 수정
	//if( pMysqlCon )
	m_Log.setLog(" %s    |  CMysqlPool : IsConnectDB host [ %s ] \n", pMysqlCon->host);
	if( pMysqlCon->host != NULL )
	{

		MYSQL_RES *res;

		//m_Log.setLog(" %s    |  CMysqlPool : SELECT 1 \n", m_szLogMark);

		if (mysql_query(pMysqlCon, "SELECT 1"))
		{
			//m_Log.setLog(" %s    |  CMysqlPool : return IsConnect false \n", m_szLogMark);
			return false;
		}
		//m_Log.setLog(" %s    |  CMysqlPool : mysql_store_result \n", m_szLogMark);
		if ((res = mysql_store_result(pMysqlCon)))
		{
			//m_Log.setLog(" %s    |  CMysqlPool : mysql_free_result \n", m_szLogMark);
			 mysql_free_result(res);

			//m_Log.setLog(" %s    |  CMysqlPool : Already Connected \n", m_szLogMark);
			return true;
		}

		//sprintf(m_szErrMsg,"Errno : %d | ErrMsg : %s ",mysql_errno(pMysqlCon) , mysql_error(pMysqlCon) );
		//m_Log.setLog(" %s    |  CMysqlPool : IsConnectDB ( %s )) \n", m_szLogMark,m_szErrMsg);
	}
	else
	{
		 m_Log.setLog(" %s    |  CMysqlPool : IsConnectDB FAIL \n", m_szLogMark);
	}


	return false;




}

void   CMysqlPool::SetConnectTimeout(unsigned int nSecond)
{
	m_nConnectTimeout = nSecond;
}

MYSQL* CMysqlPool::ConnectDB(MYSQL* pMysqlCon)
{
	//m_Log.setLog(" %s    |  CMysqlPool : CloseDB DB \n", m_szLogMark);

	CloseDB(pMysqlCon);

	//m_Log.setLog(" %s    |  CMysqlPool : mysql_init DB \n", m_szLogMark);

	if (!mysql_init(pMysqlCon) )
	{

		sprintf(m_szErrMsg,"Errno : %d | ErrMsg : %s ",mysql_errno(pMysqlCon) , mysql_error(pMysqlCon) );
		m_Log.setLog(" %s    |  CMysqlPool : ConnectDB ( %s )) \n", m_szLogMark,m_szErrMsg);
		return NULL;
	}

	#ifdef __DEBUG
	printf("\n\n\n\nConnectDB time out 2\n\n\n\n\n\n\n");
	#endif

	if( m_nConnectTimeout > 0 )
	{
		mysql_options(pMysqlCon, MYSQL_OPT_CONNECT_TIMEOUT, (char*)&m_nConnectTimeout);

	}
	//m_Log.setLog(" %s    |  CMysqlPool : mysql_real_connect DB \n", m_szLogMark);
	if ( !mysql_real_connect( pMysqlCon , m_szDBIP, m_szUserID, m_szUserPass, m_szDBName, 0, NULL, 0))
	{
		sprintf(m_szErrMsg,"Errno : %d | ErrMsg : %s ",mysql_errno(pMysqlCon) , mysql_error(pMysqlCon) );
		m_Log.setLog(" %s    |  CMysqlPool : ConnectDB ( %s )) \n", m_szLogMark,m_szErrMsg);
		return NULL;
	}
	//m_Log.setLog(" %s    |  CMysqlPool : ConnectDB Sucess \n", m_szLogMark);
	return pMysqlCon;

}
void CMysqlPool::CloseDB(MYSQL* pMysqlCon)
{
	//m_Log.setLog(" %s    |  CMysqlPool : CloseDB  MysqlCon ( %x )\n", m_szLogMark , pMysqlCon);
	mysql_close( pMysqlCon );
}


int CMysqlPool::CreaeMysqlPool(int nPoolSize,int nMaxPoolSize )
{
	//m_Log.setLog(" %s    |  CMysqlPool : CreaeMysqlPool PoolSize ( %d ) MaxPoolSize ( %d ) \n", m_szLogMark,nPoolSize,nMaxPoolSize);

	m_nDefaultPoolSize = nPoolSize;
	m_nMaxPoolSize = nMaxPoolSize;

	if( m_bUseDBMutex == false)
	{
		m_bUseDBMutex = true;
		if( pthread_mutex_init(&m_DB_Pool_Mutex ,NULL) != 0)
		{
			sprintf(m_szErrMsg,"m_DB_Pool_Mutex mutext init error : %d | ErrMsg : %s ",errno , strerror(errno) );
			m_Log.setLog(" %s    |  CMysqlPool : CreateMysqlPool ( %s )  \n", m_szLogMark,m_szErrMsg);
			m_bUseDBMutex = false;
		}
		//m_Log.setLog(" %s    |  CMysqlPool : CreateMysqlPool mutext init   \n", m_szLogMark);
	}

	if( m_bUseMutex == false)
	{
		m_bUseMutex = true;
		if( pthread_mutex_init(&m_Mutex ,NULL) != 0)
		{
			sprintf(m_szErrMsg,"m_Mutex mutext init error : %d | ErrMsg : %s ",errno , strerror(errno) );
			m_Log.setLog(" %s    |  CMysqlPool : CreateMysqlPool ( %s )  \n", m_szLogMark,m_szErrMsg);
			m_bUseDBMutex = false;
		}
		//m_Log.setLog(" %s    |  CMysqlPool : CreateMysqlPool mutext init   \n", m_szLogMark);
	}

	if( m_bUseSCHMutex == false)
	{
		m_bUseSCHMutex =true;
		if( pthread_mutex_init(&m_Schedule_Mutex,NULL) != 0)
		{
			sprintf(m_szErrMsg,"m_Schedule_Mutex mutext init error : %d | ErrMsg : %s ",errno , strerror(errno) );
			m_bUseSCHMutex = false;
		}
	}


	if( pthread_mutex_init(&m_KeyMutex,NULL) != 0)
	{
		sprintf(m_szErrMsg,"m_KeyMutex mutext init error : %d | ErrMsg : %s ",errno , strerror(errno) );
		m_bUseKeyMutex = false;
	}
	else
	{
		m_bUseKeyMutex = true;
	}






//	m_nPoolSize = m_nDefaultPoolSize;

	if ( m_nMaxPoolSize  > 20 )
	{
		m_nMaxPoolSize = 20;
	}


	//m_nScheduleSize = m_nPoolSize;
	m_nScheduleSize = 1;

	int i = 0 ;



	for( i = 0 ; i < m_nDefaultPoolSize ; i++)
	{
		AddDBPool();
	}

	//SCH_UnLock();

	//m_Log.setLog(" %s    |  CMysqlPool : CreateMysqlPool Sucess ( %d )  \n", m_szLogMark,m_nMaxPoolSize);

	return 0;

}

void CMysqlPool::AddDBPool()
{

	//m_Log.setLog(" %s    |  AddDBPool : increase pool [ %d ] [ %d  ] \n", m_szLogMark,m_nPoolSize , m_nMaxPoolSize);

	if( m_nPoolSize < m_nMaxPoolSize )
	{
		m_nPoolSize++;

		DB_INFO *pDB_INFO = NULL;
		pDB_INFO = m_DB_Pool.ReplaceKey(SLEEP_MAPKEY , READY_MAPKEY);

		//m_Log.setLog(" %s    |  AddDBPool : increase pool [ %d ] \n", m_szLogMark,m_nPoolSize);

		if( pDB_INFO != NULL)
		{

			//m_Log.setLog(" %s    |  AddDBPool : increase pool [ change key ] \n", m_szLogMark);
			//m_Log.setLog(" %s    |  AddDBPool : increase pool [ make memory ] index[ %d ] pMysqlCon [ %x ] \n", m_szLogMark,pDB_INFO->nIndex,pDB_INFO->pMysqlCon);

		}
		else
		{

			DB_INFO stDB_INFO ;
			memset(&stDB_INFO,0x00,sizeof(DB_INFO));

			stDB_INFO.nIndex = m_nPoolSize;
			stDB_INFO.pMysqlCon = &m_DB_Con[m_nPoolSize];
			m_DB_Pool.Push(READY_MAPKEY, &stDB_INFO);
			m_Log.setLog(" %s    |  AddDBPool : increase pool [ make memory ] index[ %d ] pMysqlCon [ %x ] \n", m_szLogMark,stDB_INFO.nIndex,stDB_INFO.pMysqlCon);
		}


	}
}
void CMysqlPool::EraseDBPool()
{
	//m_Log.setLog(" %s    |  EraseDBPool : decrease pool ( %d ) ( %d )\n", m_szLogMark,m_nPoolSize , m_DBPoolSchedule.size());

	if( m_nPoolSize > m_nDefaultPoolSize && m_DBPoolSchedule.size() > m_nDefaultPoolSize  )
	{


		int nDBPoolMapKey = 0 ;
		nDBPoolMapKey = m_DBPoolSchedule.front();
		m_DBPoolSchedule.pop();


		//m_Log.setLog(" %s    |  EraseDBPool : ======================== 제거할 DB ( %d )  ======================== \n", m_szLogMark,nDBPoolMapKey);
		DB_INFO * pDB_INFO = m_DB_Pool.ReplaceKey(nDBPoolMapKey,SLEEP_MAPKEY);
		if( pDB_INFO != NULL  )
		{
			mysql_close(pDB_INFO->pMysqlCon);
			//m_Log.setLog(" %s    |  EraseDBPool : decrease pool [ change key ] \n", m_szLogMark);
			m_nPoolSize--;



		}
		else
		{
			m_Log.setLog("\nEraseDBPool : ======================== 찾고자 하는 DB Pool 이 없습니다. ( %d )  ======================== \n", m_szLogMark,nDBPoolMapKey);
			#ifdef __DEBUG
			printf("\nEraseDBPool : ======================== 찾고자 하는 DB Pool 이 없습니다. ( %d )  ======================== \n",nDBPoolMapKey);
			#endif
		}


	}

}

void CMysqlPool::CloseMysqlPool()
{
	//m_Log.setLog(" %s    |  CMysqlPool : CloseMysqlPool start ( %d ) \n", m_szLogMark,m_nMaxPoolSize);
	for(int i = 0; i < m_nMaxPoolSize; i++)
	{
		DB_INFO Temp_DB_INFO;
		memset(&Temp_DB_INFO,0x00,sizeof(Temp_DB_INFO));
		if( m_DB_Pool.Pop(Temp_DB_INFO) )
			mysql_close(Temp_DB_INFO.pMysqlCon);
		//mysql_close( m_DB_Info[i].pMysqlCon );
	}
	//m_Log.setLog(" %s    |  CMysqlPool : CloseMysqlPool end ( %d )\n", m_szLogMark,m_nPoolSize);

}



void CMysqlPool::DB_Lock()
{

	if(m_bUseDBMutex)
	{
		//m_Log.setLog(" %s    |  CMysqlPool : DB_Lock \n", m_szLogMark);
		pthread_mutex_lock(&m_DB_Pool_Mutex);
	}
}

void CMysqlPool::DB_UnLock()
{


	if(m_bUseDBMutex)
	{
		//m_Log.setLog(" %s    |  CMysqlPool : DB_UnLock \n", m_szLogMark);
		pthread_mutex_unlock(&m_DB_Pool_Mutex);
	}
}

void CMysqlPool::Lock()
{
	if(m_bUseMutex)
	{
		pthread_mutex_lock(&m_Mutex);
	}
}
void CMysqlPool::UnLock()
{
	if(m_bUseMutex)
	{
		pthread_mutex_unlock(&m_Mutex);
	}
}

void CMysqlPool::SCH_Lock()
{
	if(m_bUseSCHMutex)
	{
		pthread_mutex_lock(&m_Schedule_Mutex);
	}
}

void CMysqlPool::SCH_UnLock()
{
	if(m_bUseSCHMutex)
	{
		pthread_mutex_unlock(&m_Schedule_Mutex);
	}
}

void CMysqlPool::Key_Lock()
{
	if(m_bUseKeyMutex)
	{
		pthread_mutex_lock(&m_KeyMutex);
	}
}
void CMysqlPool::Key_UnLock()
{
	if(m_bUseKeyMutex)
	{
		pthread_mutex_unlock(&m_KeyMutex);
	}
}

void CMysqlPool::SetDB(char* pDBName, char* pDBIP , char* pUserID, char* pUserPass )
{

	strcpyA( m_szDBName	    ,pDBName, sizeof(m_szDBName));
	strcpyA( m_szDBIP	    ,pDBIP, sizeof(m_szDBIP));
	strcpyA( m_szUserID	    ,pUserID, sizeof(m_szUserID));
	strcpyA( m_szUserPass 	,pUserPass, sizeof(m_szUserPass));

	m_Log.setLog(" %s    |  CMysqlPool : SetDB  ( %s ) ( %s ) ( %s ) ( %s ) \n", m_szLogMark,m_szDBName,m_szDBIP ,m_szUserID , m_szUserPass);
}

int CMysqlPool::GetToalScheduleSize()
{

	return m_Schedule.GetTotalNodeCount();
	//return m_Schedule.size();

	//m_Log.setLog(" %s    |  CMysqlPool : GetToalScheduleSize ( %d )\n", m_szLogMark, nSum);

}
bool CMysqlPool::IsSleepMngThread()
{
	return m_bMngThreadSleep;
}


CMysqlCon::CMysqlCon()
{
	m_bSearch = false;

	pthread_cond_init(&m_pthread_cond,NULL);
	pthread_mutex_init(&m_pthread_mutex,NULL); //	m_pthread_mutex  = PTHREAD_MUTEX_INITIALIZER;

	m_pCheck = new char[4];
	memset(m_pCheck,0x00,sizeof(char)*4);

	strcpy(m_pCheck,"li");

	m_bFreeResource = false;


}

CMysqlCon::CMysqlCon(CMysqlPool* pParent , unsigned long pid)
{

	m_pParent = pParent ;
	m_pid = pid;
	m_bSearch = false;

	pthread_cond_init(&m_pthread_cond,NULL);
	pthread_mutex_init(&m_pthread_mutex,NULL); //	m_pthread_mutex  = PTHREAD_MUTEX_INITIALIZER;

	m_pCheck = new char[4];
	memset(m_pCheck,0x00,sizeof(char)*4);
	strcpy(m_pCheck,"li");

	m_bFreeResource = false;

}

CMysqlCon::~CMysqlCon()
{
	if( !m_bFreeResource )
	{
		m_bFreeResource = true;
		//m_Log.setLog(" %s    |  CMysqlCon  : -----------------------end CMysqlCon ( %ld )--------------------\n",m_pParent->GetLogMark(),m_pid);
		m_pParent->CloseDBPool(m_pid);
		//m_Log.setLog(" %s    |  CMysqlCon  : -----------------------end CMysqlCon ( %ld )--------------------\n\n",m_pParent->GetLogMark(),m_pid);

		//20120308
		pthread_mutex_destroy(&m_pthread_mutex);



		int result = pthread_cond_destroy(&m_pthread_cond);
		//m_Log.setLog(" %s    |  CMysqlCon  : pthread_cond_destory  ( %ld ) RESULT ( %d )  \n",m_pParent->GetLogMark(),m_pid,result);

		/*
		if (result == EBUSY)
		{
			m_pParent->m_Log.setLog(" %s    |  CMysqlCon  : pthread_cond_destory  ( %ld ) Failed(EBUSY)  ERRNO( %d ) ERR_MSG ( %s ) \n",m_pParent->GetLogMark(),m_pid,errno,strerror(errno));
		}
		else
		{
			if( result != 0 )
				m_Log.setLog(" %s    |  CMysqlCon  : pthread_cond_destory  ( %ld ) Excetpion ERRNO( %d ) ERR_MSG ( %s ) \n",m_pParent->GetLogMark(),m_pid,errno,strerror(errno));
			else
				m_Log.setLog(" %s    |  CMysqlCon  : pthread_cond_destory  ( %ld ) Sucessed  \n",m_pParent->GetLogMark(),m_pid);

		}
		*/
	}


}

void CMysqlCon::FreeResource()
{
	if( !m_bFreeResource )
	{
		m_bFreeResource = true;
		//m_Log.setLog(" %s    |  CMysqlCon  : -----------------------end CMysqlCon ( %ld )--------------------\n",m_pParent->GetLogMark(),m_pid);
		m_pParent->CloseDBPool(m_pid);
		//m_Log.setLog(" %s    |  CMysqlCon  : -----------------------end CMysqlCon ( %ld )--------------------\n\n",m_pParent->GetLogMark(),m_pid);
		//m_pParent->CloseDBPool(m_pid);

		//20120308
		pthread_mutex_destroy(&m_pthread_mutex);



		int result = pthread_cond_destroy(&m_pthread_cond);
		/*
		m_Log.setLog(" %s    |  CMysqlCon  : pthread_cond_destory  ( %ld ) RESULT ( %d )  \n",m_pParent->GetLogMark(),m_pid,result);


		if (result == EBUSY)
		{
			m_pParent->m_Log.setLog(" %s    |  CMysqlCon  : pthread_cond_destory  ( %ld ) Failed(EBUSY)  ERRNO( %d ) ERR_MSG ( %s ) \n",m_pParent->GetLogMark(),m_pid,errno,strerror(errno));
		}
		else
		{
			if( result != 0 )
				m_Log.setLog(" %s    |  CMysqlCon  : pthread_cond_destory  ( %ld ) Excetpion ERRNO( %d ) ERR_MSG ( %s ) \n",m_pParent->GetLogMark(),m_pid,errno,strerror(errno));
			else
				m_Log.setLog(" %s    |  CMysqlCon  : pthread_cond_destory  ( %ld ) Sucessed  \n",m_pParent->GetLogMark(),m_pid);

		}
		*/
	}
}

int	CMysqlCon::ConnectPool( CMysqlPool * pParent , unsigned long pid )
{
	m_pParent = pParent ;
	m_pid = pid;
	m_bSearch = false;

}

MYSQL* CMysqlCon::GetMysqlCon()
{

	MYSQL* pMysqlCon = NULL;

	struct timespec stTimeOut;

	struct timeval stNowTime;
    gettimeofday(&stNowTime, NULL); //그리니치 표준시 현재시간 반환
    stTimeOut.tv_sec = stNowTime.tv_sec + 5;
    //stTimeOut.tv_nsec = stNowTime.tv_usec * 1000 ;

	pthread_mutex_lock(&m_pthread_mutex);

	m_pParent->CreateSchedule(m_pid , &m_pthread_mutex,&m_pthread_cond,m_pCheck);

	//m_pParent->m_Log.setLog(" %s    |  CMysqlCon  : wait signal  ( %ld ) check string ( %s ) \n",m_pParent->GetLogMark(),m_pid,m_pCheck);

	int result = pthread_cond_timedwait(&m_pthread_cond,&m_pthread_mutex,&stTimeOut);
	if( result  == 0 )
	{
		//m_pParent->m_Log.setLog(" %s    |  CMysqlCon  : recv signal ( %ld ) \n",m_pParent->GetLogMark(),m_pid);
		pMysqlCon = m_pParent->GetMysqlCon(m_pid);
		//m_Log.setLog(" %s    |  CMysqlCon  : recv signal 2(  ) \n",m_pParent->GetLogMark());
	}
	else //ETIMEDOUT 110
	{ //hcs - EINTR or ETIMEDOUT ?? 인터럽트 or 타임아웃 ??


		pMysqlCon = NULL;

		//m_pParent->m_Log.setLog(" %s    |  CMysqlCon  : SIGINAL timeout ( %ld ) ( %s ) pMysqlCon = NULL RESULT ( %d ) ERRNO ( %d ) ERR_MSG( %s ) \n",m_pParent->GetLogMark(),m_pid,m_pCheck,result,errno,strerror(errno));

		if(result == ETIMEDOUT)
			m_pParent->m_Log.setLog(" %s    |  CMysqlCon  : pthread_cond_timedwait result=[ETIMEDOUT]\n",m_pParent->GetLogMark());
		else if(result == EINTR)
			m_pParent->m_Log.setLog(" %s    |  CMysqlCon  : pthread_cond_timedwait result=[EINTR]\n",m_pParent->GetLogMark());
		else
			m_pParent->m_Log.setLog(" %s    |  CMysqlCon  : pthread_cond_timedwait result=[%d]\n",m_pParent->GetLogMark(), result);

		if( m_pCheck != NULL && strcmp(m_pCheck,"li") == 0 )
		{
			strcpy(m_pCheck,"di");

			m_pParent->m_Log.setLog(" %s    |  CMysqlCon  : recv signal timeout ( %ld ) ( %s )\n",m_pParent->GetLogMark(),m_pid,m_pCheck);

		}
		else
		{
			if( m_pCheck == NULL)
			{
				m_pParent->m_Log.setLog(" %s    |  CMysqlCon  : recv signal timeout ( %ld ) ( NULL )\n",m_pParent->GetLogMark(),m_pid);
			}
			else
			{
				m_pParent->m_Log.setLog(" %s    |  CMysqlCon  : recv signal timeout ( %ld ) \n",m_pParent->GetLogMark(),m_pid);
			}


		}

	}



//	pthread_cond_wait(&m_pthread_cond,&m_pthread_mutex) ;pMysqlCon = m_pParent->GetMysqlCon(m_pid);


	pthread_mutex_unlock(&m_pthread_mutex);

	//20120308 pthread_cond_destroy(&m_pthread_cond);
	/*

	if( pMysqlCon != NULL )
		m_Log.setLog(" %s    |  CMysqlCon  : \n-----------------------get mysql connection ( %ld ) ( %x )--------------------\n",m_pParent->GetLogMark(),m_pid,pMysqlCon);
	else
		m_Log.setLog(" %s    |  CMysqlCon  : \n-----------------------get mysql connection ( %ld ) ( NULL )--------------------\n",m_pParent->GetLogMark(),m_pid);
	*/
	return pMysqlCon ;
}



//차후 풀에서 분리



int CMysqlCon::MysqlQuery(MYSQL* pMysqlCon , char* pErrorMsg , const char* pQuery)
{
	/*
	2009/04/06 - HCS
	가변함수에서 일반 함수로 수정.
	특정 플레이어의 이미지 이름 생성 방식때문에 변경.
	ex)%2554sdfv%sdf35%s.jsp
	*/
	if( pMysqlCon == NULL )
	{
		strcpy(pErrorMsg," CMysqlPool : ERROR!! : MESSAGE : Connection 이 NULL 입니다.  ");
		m_pParent->m_Log.setLog( "%s\n",pErrorMsg );
		return 10000;
	}

	if( pQuery == NULL )
	{
		strcpy(pErrorMsg," CMysqlPool : ERROR!! : MESSAGE : 적용할 쿼리가 없거나 NULL 입니다. ");
		m_pParent->m_Log.setLog( "%s --- Query [ %s ] \n",pErrorMsg ,pQuery );
		return 10001;
	}


	char    szBuffer[20000];
	memset(szBuffer, 0x00, sizeof(szBuffer));

	strcpy(szBuffer, pQuery);


	//ReplaceSingleQuotation(pQuery, '\'',szBuffer2);

	if( pErrorMsg == NULL )
	{
		static char szTempBuffer[256];
		memset(szTempBuffer,0x00,sizeof(szTempBuffer));
		pErrorMsg = szTempBuffer;
	}

	if (mysql_query(pMysqlCon, szBuffer))
	{
		int nErrno = mysql_errno(pMysqlCon);

		m_pParent->m_Log.setLog( " CMysqlPool : ERROR!! Query   : [ %s ] \n\n",szBuffer);
		sprintf(pErrorMsg," [ %d ] [ %s ]  ",nErrno, mysql_error(pMysqlCon) );
		m_pParent->m_Log.setLog( " CMysqlPool : ERROR!! MESSAGE : %s  \n\n" , pErrorMsg );
		return nErrno;
	}

  strcpy(pErrorMsg,"정상적으로 처리 되었습니다.");
  //m_pParent->m_Log.setLog(" %s    | CMysqlPool : Query : [ %s ] \n\n",m_pParent->GetLogMark(),szBuffer);

  return 0;

}


int CMysqlCon::MysqlLargeQuery(MYSQL* pMysqlCon , char* pErrorMsg , char* pQuery , ... )
{

	if( pMysqlCon == NULL )
	{
		strcpy(pErrorMsg," CMysqlPool : ERROR!! : MESSAGE : Connection 이 NULL 입니다.  ");
		m_pParent->m_Log.setLog( "%s\n",pErrorMsg );
		return 10000;
	}

	if( pQuery == NULL )
	{
		strcpy(pErrorMsg," CMysqlPool : ERROR!! : MESSAGE : 적용할 쿼리가 없거나 NULL 입니다. ");
		m_pParent->m_Log.setLog( "%s --- Query [ %s ] \n",pErrorMsg ,pQuery );
		return 10001;
	}

	char    szBuffer[1024*1024];

	va_list args;
	  memset(szBuffer, 0x00, sizeof(szBuffer));

	  va_start(args, pQuery);
	  vsprintf(szBuffer, pQuery, args);
	  va_end(args);

	if( pErrorMsg == NULL )
	{
		static char szTempBuffer[256];
		memset(szTempBuffer,0x00,sizeof(szTempBuffer));
		pErrorMsg = szTempBuffer;
	}





	if (mysql_query(pMysqlCon, szBuffer))
	{
		int nErrno = mysql_errno(pMysqlCon);

		m_pParent->m_Log.setLog( " CMysqlPool : ERROR!! Query   : [ %s ] \n\n",szBuffer);
		sprintf(pErrorMsg," [ %d ] [ %s ]  ",nErrno, mysql_error(pMysqlCon) );
		m_pParent->m_Log.setLog( " CMysqlPool : ERROR!! MESSAGE : %s  \n\n" , pErrorMsg );
		return nErrno;
	}

  strcpy(pErrorMsg,"정상적으로 처리 되었습니다.");
  //m_pParent->m_Log.setLog(" %s    | CMysqlPool : 정상처리 \n Query : [ %s ] \n\n",m_pParent->GetLogMark(),szBuffer);

  return 0;

}



CMysqlConnector::CMysqlConnector()
{
	m_pDCon = NULL;
}
CMysqlConnector::CMysqlConnector( char* pDBName, char* pDBIp, char* pDBUser, char* pDBPass )
{
	m_pDCon = NULL;
	ConnectDB( pDBName,  pDBIp,  pDBUser,  pDBPass);
}
CMysqlConnector::~CMysqlConnector()
{
	CloseDB();
}
MYSQL* CMysqlConnector::GetMysqlCon()
{
	return m_pDCon;
}

MYSQL* CMysqlConnector::ConnectDB(char* pDBName, char* pDBIp, char* pDBUser, char* pDBPass )
{
	if( pDBName == NULL )
	{
		infLOG(ALWAY,"CMysqlConnector | 디비 이름을 입력하세요\n");
		return NULL;
	}
	else if( pDBIp == NULL )
	{
		infLOG(ALWAY,"CMysqlConnector | 디비 아이피를 입력하세요\n");
		return NULL;
	}
	else if( pDBUser == NULL )
	{
		infLOG(ALWAY,"CMysqlConnector | 디비 사용자를 입력하세요\n");
		return NULL;
	}
	else if( pDBPass == NULL )
	{
		infLOG(ALWAY,"CMysqlConnector | 디비 암호를 입력하세요\n");
		return NULL;
	}

	CloseDB();

	m_pDCon = mysql_init(NULL);

	if (!m_pDCon)
	{
		infLOG(ALWAY,"CMysqlConnector | db_connect: mysql_init failed(no memory)\n");
		return NULL;
	}

	unsigned int unTimeOut = 5;
	mysql_options(m_pDCon, MYSQL_OPT_CONNECT_TIMEOUT, (char*)&unTimeOut);

	if (!mysql_real_connect(m_pDCon, pDBIp, pDBUser, pDBPass, pDBName, 0, NULL, 0))
	{
		infLOG(ALWAY,"CMysqlConnector | db_connect[%s][%s][%s][%s]: mysql_real_connect failed(%s) (%d) \n",pDBIp,pDBUser,pDBPass,pDBName, mysql_error(m_pDCon),mysql_errno(m_pDCon));
		return NULL;
	}
	//infLOG(ALWAY,"CMysqlConnector | Connect To DB [ %s ]\n",pDBIp);
	return m_pDCon;
}

void CMysqlConnector::CloseDB()
{
	//infLOG(ALWAY,"CMysqlConnector | CloseDB Check\n");
	if( m_pDCon )
	{
		//infLOG(ALWAY,"CMysqlConnector | CloseDB()\n");
		mysql_close(m_pDCon);
		m_pDCon = NULL;
	}
}
