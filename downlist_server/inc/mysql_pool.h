
#include "CDBQueue.h"	
#include "CSchQueue.h"	
#include "CLog64.h"

#include <queue>
#include <map>
using namespace std;

#include <mysql.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <stdarg.h>

#include "apdefine.h" // for log
#include "comcomm.h" // for log


class CMysqlPool
{

	public:
		CMysqlPool();
		CMysqlPool(int nPoolSize , int nMaxPoolSize , bool bUseMutexSchedule);
		virtual  ~CMysqlPool();
		
		unsigned long GetPrimaryKey();
		
		CLog64 m_Log;
		
		int SetLogInfo(char* pLogPath,char* pLogName);
		
	protected:
		unsigned long m_ulAutoIncNum;
		pthread_mutex_t m_DB_Pool_Mutex;
		pthread_mutex_t m_Schedule_Mutex;
		pthread_mutex_t m_Mutex;
		
		pthread_mutex_t m_KeyMutex;
		
		
		long m_QueMngThreadID;
		long m_DbPoolMngThreadID;
		
		bool m_bMngThreadStop;
		bool m_bMngThreadSleep;
		
		bool m_bUseMutex;
		bool m_bUseDBMutex;
		bool m_bUseSCHMutex;
		bool m_bUseLogMark;
		bool m_bUseKeyMutex;
		
		int m_nOldPoolSize;
		int m_nPoolSize;
		int m_nDefaultPoolSize;
		int m_nMaxPoolSize;
		int m_nScheduleSize;
		char m_szErrMsg[1024];
		
		char m_szDBName[512];
		char m_szDBIP[512];
		char m_szUserID[512];
		char m_szUserPass[512];
		char m_szLogMark[512];
		
		MYSQL m_DB_Con[100];
		MYSQL* m_pMysqlCon;
		DB_INFO m_DB_Info[100];
		DB_SCHEDULE m_Schedule_Info[100];

		CSchQueue m_Schedule;
		queue<long> m_DBPoolSchedule;

		CDBQueue m_DB_Pool;

		int m_nScheduleNum;
		unsigned int m_nConnectTimeout;

	protected:
	
		
		void * QueMngThread(void* pThreadArgs);
		bool IsConnectDB(MYSQL* pMysqlCon);
		MYSQL* ConnectDB(MYSQL* pMysqlCon);
		
		void IncreasePool();
		void CheckPool();
	
		void Lock();  
		void UnLock();
	
		void DB_Lock();
		void DB_UnLock();
		void SCH_Lock();
		void SCH_UnLock();
		
		void Key_Lock();
		void Key_UnLock();

	public:
		void SetLogMark(char* pMark);
		char* GetLogMark();
		bool IsSleepMngThread();
		void AddDBPool();
		void EraseDBPool();
		MYSQL* GetMysqlCon(long thread_id);
		void CreateDBPoolMngThread();
		void CreateQueMngThread();
		int GetToalScheduleSize();
		void SetDB(char* pDBName, char* pDBIP , char* pUserID, char* pUserPass );
		int  CreaeMysqlPool(int nPoolSize,int nMaxPoolSize );
		void CloseMysqlPool();
		
		void CloseDB(MYSQL* pMysqlCon);
		
		void UseMutexSchedule(bool bUse);
		void CreateSchedule(long thread_id , pthread_mutex_t* pthread_mutex , pthread_cond_t* pthread_cond , char* pCheck );// queue 에 해당 스레드 넣기
		int CloseSchedule(long thread_id);
		int CloseDBPool(long thread_id);
		
		void   SetConnectTimeout(unsigned int nSecond);
				
};


class CMysqlCon
{
	public:

		CMysqlCon();	
		CMysqlCon( CMysqlPool * pParent , unsigned long pid );	
		virtual ~CMysqlCon();

	protected:

		char* m_pCheck;
		pthread_mutex_t m_pthread_mutex ; /* = PTHREAD_MUTEX_INITIALIZER;*/
		pthread_cond_t  m_pthread_cond;
			
		CMysqlPool* m_pParent;
		long m_pid;
		bool m_bSearch;
		
		bool m_bFreeResource;
	public:
		void FreeResource();
		int	ConnectPool( CMysqlPool * pParent , unsigned long pid );
		MYSQL* GetMysqlCon();
		int MysqlQuery(MYSQL* pMysqlCon , char* pErrorMsg , const char* pQuery);
		int MysqlLargeQuery(MYSQL* pMysqlCon , char* pErrorMsg , char* pQuery , ... );


};

class CMysqlConnector
{
	public:

		CMysqlConnector();	
		CMysqlConnector( char* pDBName, char* pDBIp, char* pDBUser, char* pDBPass );	
		virtual ~CMysqlConnector();

	protected:
		MYSQL* m_pDCon;
	public:
		MYSQL* GetMysqlCon();	
		MYSQL* ConnectDB(char* pDBName, char* pDBIp, char* pDBUser, char* pDBPass );
		void   CloseDB();
	
};

