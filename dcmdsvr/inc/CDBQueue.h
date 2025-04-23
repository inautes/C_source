//##########################################################//
//##    Template Queue                                    ##//
//##    LeeHyungChur                                      ##//
//##    2005. 3. 22                                        ##//
//##########################################################//



#ifndef _QDBUEUE
#define _QDBUEUE

#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE    1
#endif 

/*
#ifndef NULL
#define NULL    0
#endif
*/
#include <mysql.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <string.h>

#include "apdefine.h" // for log
#include "comcomm.h" // for log

#include "CLog64.h"

typedef struct _db_info_
{
	unsigned long pid;
	int sock_fd;
	int nIndex;
	MYSQL* pMysqlCon;
	
}DB_INFO,*LPDB_INFO;


class CDBQueue
{
	protected:
	   
		typedef struct _NODE
		{
			int nKey; //  Å°°ª
			DB_INFO data;
			_NODE* next;
			_NODE* prev;
		}NODE,*LPNODE;
	
	public:
		CDBQueue();
		virtual ~CDBQueue();
	
		//operation
		void Push(int nKey , DB_INFO* pData);       //Push Next NODE of Head
		bool Pop(DB_INFO & pDestbuffer);       //auto delete
		
		bool RemoveAll();  //Delete All
		bool Remove(NODE* pNode);     //Delete Prev NODE of tail
		bool Find(int nKey ,DB_INFO & data , bool bDeleteNode /*= true*/);
		//T Find(int nKey );
		DB_INFO* Find(int nKey );
		DB_INFO* ReplaceKey(int uiSrcKey , int uiDestKey );
	
		int GetTotalNodeCount();
		void InitQue();
		void RemoveInit();
		bool IsEmpty();

		void UnLockThreadSafe();
		void LockThreadSafe();
		bool IsThreadSafe();
		void UseMutex(bool bUse);
		
		int MutexUnLock();
		int MutexLock();
				
		void ViewKeyList();
		
		CLog64 m_Log;
		
	private:
		bool m_bThreadSafe ;
		bool m_bUseMutex;
		pthread_mutex_t m_pthread_mutex;		
		long m_lNodeCount;
		long m_lInsertNodeCount;
		long m_lDeleteNodeCount;		
		NODE* m_pHead;
		NODE* m_pTail;
		

};

#endif

