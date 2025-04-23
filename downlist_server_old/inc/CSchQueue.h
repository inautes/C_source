//##########################################################//
//##    Template Queue                                    ##//
//##    LeeHyungChur                                      ##//
//##    2005. 3. 22                                        ##//
//##########################################################//



#ifndef _QSCHQUEUE
#define _QSCHQUEUE

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

typedef struct _db_schedule_
{
	pthread_t pid;
	pthread_cond_t  *pthread_cond;
	pthread_mutex_t *pthread_mutex;
	char* pOwnerCheck;
	time_t st_time;
}DB_SCHEDULE , *LPDB_SCHEDULE;

class CSchQueue
{
	protected:
	   
		typedef struct _NODE
		{
			int nKey; //  Å°°ª
			DB_SCHEDULE data;
			_NODE* next;
			_NODE* prev;
		}NODE,*LPNODE;
	
	public:
		CSchQueue();
		virtual ~CSchQueue();
	
		//operation
		DB_SCHEDULE* Push(int nKey , DB_SCHEDULE* pData);
		bool Pop(DB_SCHEDULE & pDestbuffer);       //auto delete
		DB_SCHEDULE *  Front();
		bool GetKey(int & nDestKey);
		bool RemoveAll();  //Delete All
		bool Remove(NODE* pNode);     //Delete Prev NODE of tail
		bool Find(int nKey ,DB_SCHEDULE & data , bool bDeleteNode /*= true*/);
		//T Find(int nKey );
		DB_SCHEDULE* Find(int nKey );
		DB_SCHEDULE* ReplaceKey(int nSrcKey , int nDestKey );
	
		int GetTotalNodeCount();
		void InitQue();
		void RemoveInit();
		
		void UnLockThreadSafe();
		void LockThreadSafe();
		bool IsThreadSafe();
		void UseMutex(bool bUse);
		
		int MutexUnLock();
		int MutexLock();
		bool IsEmpty();
		
		void ViewKeyList();
		
		CLog64 m_Log;
						
	private:
	

	
		long m_lNodeCount;
		long m_lInsertNodeCount;
		long m_lDeleteNodeCount;
		bool m_bThreadSafe ;
		bool m_bUseMutex;
		pthread_mutex_t m_pthread_mutex;
		pthread_mutex_t m_node_count_mutex;
		
		NODE* m_pHead;
		NODE* m_pTail;
		

				
};
 
 
#endif //class Queue


