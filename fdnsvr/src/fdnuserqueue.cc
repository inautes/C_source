
#include "fdnuserqueue.h"



CUserQueue::CUserQueue()
{
	// create HEAD and TAIL NODE
	m_pHead = NULL;
	m_pTail	 = NULL;

	InitQue();
}



CUserQueue::~CUserQueue()
{

	if(m_pHead != NULL)
		delete m_pHead;
	if(m_pTail != NULL)
		delete m_pTail;
}



USERINFO* CUserQueue::ReplaceKey(int uiSrcKey , int uiDestKey )
{
	//m_Log.setLog("CUserQueue : ReplaceKey ( %ld ) ( %ld ) \n",uiSrcKey,uiDestKey );
	int nCount =0;
	NODE* pTempNode = m_pHead->next;
	while(pTempNode != m_pTail )
	{
		nCount++;
		//m_Log.setLog("CUserQueue : find ReplaceKey ( %ld ) =  ( %ld ) \n",uiSrcKey,pTempNode->nKey );
			
		if(uiSrcKey == pTempNode->nKey)
		{	
			//m_Log.setLog("CUserQueue : find ReplaceKey ( %ld ) ( %ld ) search count ( %d ) \n",uiSrcKey,uiDestKey ,nCount);
			
			pTempNode->nKey = uiDestKey;
			return &pTempNode->data;
		}
		pTempNode = pTempNode->next;
	}

	return NULL;
}




USERINFO* CUserQueue::Find(int nKey )
{
	////m_Log.setLog("CUserQueue : Find ( %ld )  \n",nKey );
	int nCount =0 ;
	
	NODE* pTempNode = m_pHead->next;
	while(pTempNode != m_pTail )
	{
		nCount++;
		////m_Log.setLog("CUserQueue : Find !! ( %ld ) == ( %d)   \n",nKey ,pTempNode->nKey);
		if(nKey == pTempNode->nKey)
		{	
			//m_Log.setLog("CUserQueue : Find !! ( %ld )  search count( %d )\n",nKey ,nCount);
			return &pTempNode->data;
		}
		pTempNode = pTempNode->next;
	}
	return NULL;
}


/*

T CUserQueue::Find(int nKey )
{
	NODE* pTempNode = m_pHead->next;
	while(pTempNode != m_pTail )
	{
		if(nKey == pTempNode->nKey)
		{	
			return pTempNode->data;
		}
		pTemoNode = pTempNode->next;
	}
	return NULL;
}
*/

bool CUserQueue::Find(int nKey ,USERINFO & data , bool bDeleteNode =  true)
{
	NODE* pTempNode = m_pHead->next;
	while(pTempNode != m_pTail )
	{
		if(nKey == pTempNode->nKey)
		{	
			data = pTempNode->data;
			if( bDeleteNode )
			{
				Remove(pTempNode);
			}
			return true;
		}
		pTempNode = pTempNode->next;
	}
	return false;
	
}


void CUserQueue::InitQue()
{
	// create HEAD and TAIL NODE

	m_bThreadSafe = false;
	m_bUseMutex   = false;
	
	if(m_pHead != NULL || m_pTail != NULL)
	{
		RemoveAll();
		if(m_pHead != NULL)
			delete m_pHead;
		if(m_pTail != NULL)
			delete m_pTail;
	}


	
	m_lNodeCount = 0;

	m_pHead = new NODE;
	m_pTail = new NODE;

	// Set Head NODE
	m_pHead->prev = m_pHead;
	m_pHead->next = m_pTail;
	// Set Tail NODE
	m_pTail->prev = m_pHead;
	m_pTail->next = m_pTail;
}


void CUserQueue::Push(int nKey , USERINFO* pData)
{
	
	//m_lInsertNodeCount++;
	m_lNodeCount++;
	
	//m_Log.setLog("CUserQueue : Push ( %ld )  \n",nKey );

 	NODE* pInsertNode = new NODE;
	memset(pInsertNode,0x00,sizeof(NODE));
	
	pInsertNode->nKey = nKey;

	pInsertNode->data = *pData;
	m_pTail->prev->next = pInsertNode;
	pInsertNode->prev = m_pTail->prev;
	m_pTail->prev = pInsertNode;
	pInsertNode->next = m_pTail;
	
	//m_Log.setLog("CUserQueue : Push ( %ld ) end \n",nKey );
}



bool CUserQueue::Pop(USERINFO & pDestBuffer)
{

	
	if(m_pHead->next == m_pTail)
	{
		//printf("\nError :: Remove Function");
		return false;
	}

	
	
	NODE* pTemp;
	pTemp = m_pHead->next;
	pDestBuffer = pTemp->data;
	

	m_pHead->next = pTemp->next;
	pTemp->next->prev = m_pHead;
	
	delete  pTemp;
	
	m_lNodeCount--;
	//m_lDeleteNodeCount++;
	
	return true;
	

}



bool CUserQueue::RemoveAll()
{
	if(m_pHead->next == m_pTail)
	{
		if( GetTotalNodeCount() != 0)
		//printf("Error : Node TotalCount Error , Node Count is %ld\n",GetTotalNodeCount());
		//printf("Error :: RemoveAll");
		return false;
	}

   NODE* pCurrent;
   NODE* pOld;
   pCurrent = m_pHead->next;
   while(pCurrent != m_pTail)
   {
	    pOld = pCurrent;
		pCurrent = pCurrent->next;
		//m_lDeleteNodeCount++;
		delete pOld;
		m_lNodeCount--;
   }
   m_pHead->next = m_pTail;
   m_pTail->prev = m_pHead;

   return true;
}



bool CUserQueue::Remove(NODE* pNode)
{
   if(m_pHead->next == m_pTail)
   {
	   //printf("\nError :: Remove Function");
	   return false;
   }


   pNode->prev->next = pNode->next;
   pNode->next->prev = pNode->prev;

   delete  pNode;
	
	m_lNodeCount--;	
	
   //m_lDeleteNodeCount++;
   return true;
}



int CUserQueue::GetTotalNodeCount()
{
	
	return m_lNodeCount;
	/*
	NODE* pTemp;
	unsigned long ulNum=0;
	if(m_pHead->next == m_pTail)
		return ulNum;

	pTemp = m_pHead->next;
	while( pTemp != m_pTail)
	{
		ulNum++;
		pTemp = pTemp->next;
	}
	return ulNum;
	*/
	
}


int CUserQueue::MutexLock()
{
	if( m_bUseMutex )	
	{
		#ifdef __DEBUG
		//printf( "CUserQueue : MutexLock\n");
		#endif
		
		//m_Log.setLog( "CUserQueue : MutexLock\n");
		
		return pthread_mutex_lock(&m_pthread_mutex);
	}
	return -1;
}
int CUserQueue::MutexUnLock()
{
	if( m_bUseMutex )
	{
		#ifdef __DEBUG
		//printf( "CUserQueue : MutexUnLock\n");
		#endif
		//m_Log.setLog( "CUserQueue : MutexUnLock\n");
		return pthread_mutex_unlock(&m_pthread_mutex);
	}
	return -1;
	
}
bool CUserQueue::IsThreadSafe()
{	
	return m_bThreadSafe;
}

void CUserQueue::LockThreadSafe()
{
	m_bThreadSafe = false;
	MutexLock();
	
	
	
}
void CUserQueue::UnLockThreadSafe()
{
	MutexUnLock();
	m_bThreadSafe = true;	
	
	
}


bool CUserQueue::IsEmpty()
{
	if(m_pHead->next == m_pTail)
		return true;
	return false;
}


void CUserQueue::UseMutex(bool bUse)
{
	m_bUseMutex = bUse;
	
	if( m_bUseMutex )
	{
		#ifdef __DEBUG
		//printf( "CUserQueue : pthread_mutex_init\n");
		#endif
		pthread_mutex_init(&m_pthread_mutex,NULL); //	m_pthread_mutex  = PTHREAD_MUTEX_INITIALIZER;
	}
	

}


void CUserQueue::ViewKeyList()
{
	NODE* pTempNode = m_pHead->next;
	while(pTempNode != m_pTail )
	{
		//m_Log.setLog("CUserQueue : ViewKeyList !!  ( %d)   \n",pTempNode->nKey);
		pTempNode = pTempNode->next;
	}
}


