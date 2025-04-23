//##########################################################//
//##    User     Queue                                    ##//
//##    LeeHyungChur                                      ##//
//##    2005. 3. 22                                        ##//
//##########################################################//



#ifndef _QUSER
#define _QUSER

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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <string.h>

#include "fdndefine.h"
//#include "apdefine.h" // for log
//#include "comcomm.h" // for log


class CUserQueue
{
	protected:
	   
		typedef struct _NODE
		{
			int nKey; //  Å°°ª
			USERINFO data;
			_NODE* next;
			_NODE* prev;
		}NODE,*LPNODE;
	
	public:
		CUserQueue();
		virtual ~CUserQueue();
	
		//operation
		void Push(int nKey , USERINFO* pData);       //Push Next NODE of Head
		bool Pop(USERINFO & pDestbuffer);       //auto delete
		
		bool RemoveAll();  //Delete All
		bool Remove(NODE* pNode);     //Delete Prev NODE of tail
		bool Find(int nKey ,USERINFO & data , bool bDeleteNode /*= true*/);
		//T Find(int nKey );
		USERINFO* Find(int nKey );
		USERINFO* ReplaceKey(int uiSrcKey , int uiDestKey );
	
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

