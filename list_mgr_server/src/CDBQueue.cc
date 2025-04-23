
#include "CDBQueue.h"



CDBQueue::CDBQueue()
{
	// create HEAD and TAIL NODE
	m_pHead = NULL;
	m_pTail	 = NULL;

	InitQue();
}


CDBQueue::~CDBQueue()
{

	if(m_pHead != NULL)
		delete m_pHead;
	if(m_pTail != NULL)
		delete m_pTail;
}



DB_INFO* CDBQueue::ReplaceKey(int uiSrcKey , int uiDestKey )
{
	//m_Log.setLog("CDBQueue : ReplaceKey ( %ld ) ( %ld ) \n",uiSrcKey,uiDestKey );
	NODE* pTempNode = m_pHead->next;
	while(pTempNode != m_pTail )
	{
		//m_Log.setLog("CDBQueue : find ReplaceKey ( %ld ) =  ( %ld ) \n",uiSrcKey,pTempNode->nKey );
			
		if(uiSrcKey == pTempNode->nKey)
		{	
			//m_Log.setLog("CDBQueue : find ReplaceKey ( %ld ) ( %ld ) \n",uiSrcKey,uiDestKey );
			
			pTempNode->nKey = uiDestKey;
			return &pTempNode->data;
		}
		pTempNode = pTempNode->next;
	}

	return NULL;
}




DB_INFO* CDBQueue::Find(int nKey )
{
	////m_Log.setLog("CDBQueue : Find ( %ld )  \n",nKey );
	NODE* pTempNode = m_pHead->next;
	while(pTempNode != m_pTail )
	{
		////m_Log.setLog("CDBQueue : Find !! ( %ld ) == ( %d)   \n",nKey ,pTempNode->nKey);
		if(nKey == pTempNode->nKey)
		{	
			////m_Log.setLog("CDBQueue : Find !! ( %ld )  \n",nKey );
			return &pTempNode->data;
		}
		pTempNode = pTempNode->next;
	}
	return NULL;
}

/*

T CDBQueue::Find(int nKey )
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

bool CDBQueue::Find(int nKey ,DB_INFO & data , bool bDeleteNode =  true)
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


void CDBQueue::InitQue()
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


void CDBQueue::Push(int nKey , DB_INFO* pData)
{
	
	//m_lInsertNodeCount++;
	m_lNodeCount++;
	
	//m_Log.setLog("CDBQueue : Push ( %ld )  \n",nKey );

 	NODE* pInsertNode = new NODE;
	memset(pInsertNode,0x00,sizeof(NODE));
	
	pInsertNode->nKey = nKey;

	pInsertNode->data = *pData;
	m_pTail->prev->next = pInsertNode;
	pInsertNode->prev = m_pTail->prev;
	m_pTail->prev = pInsertNode;
	pInsertNode->next = m_pTail;
	
	//m_Log.setLog("CDBQueue : Push ( %ld ) end \n",nKey );
}



bool CDBQueue::Pop(DB_INFO & pDestBuffer)
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



bool CDBQueue::RemoveAll()
{
	if(m_pHead->next == m_pTail)
	{
		if( GetTotalNodeCount() != 0)
		printf("Error : Node TotalCount Error , Node Count is %ld\n",GetTotalNodeCount());
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



bool CDBQueue::Remove(NODE* pNode)
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



int CDBQueue::GetTotalNodeCount()
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


int CDBQueue::MutexLock()
{
	if( m_bUseMutex )	
	{
		#ifdef __DEBUG
		//printf( "CDBQueue : MutexLock\n");
		#endif
		
		//m_Log.setLog( "CDBQueue : MutexLock\n");
		
		return pthread_mutex_lock(&m_pthread_mutex);
	}
	return -1;
}
int CDBQueue::MutexUnLock()
{
	if( m_bUseMutex )
	{
		#ifdef __DEBUG
		//printf( "CDBQueue : MutexUnLock\n");
		#endif
		//m_Log.setLog( "CDBQueue : MutexUnLock\n");
		return pthread_mutex_unlock(&m_pthread_mutex);
	}
	return -1;
	
}
bool CDBQueue::IsThreadSafe()
{	
	return m_bThreadSafe;
}

void CDBQueue::LockThreadSafe()
{
	m_bThreadSafe = false;
	MutexLock();
	
	
	
}
void CDBQueue::UnLockThreadSafe()
{
	MutexUnLock();
	m_bThreadSafe = true;	
	
	
}


bool CDBQueue::IsEmpty()
{
	if(m_pHead->next == m_pTail)
		return true;
	return false;
}


void CDBQueue::UseMutex(bool bUse)
{
	m_bUseMutex = bUse;
	
	if( m_bUseMutex )
	{
		#ifdef __DEBUG
		//printf( "CDBQueue : pthread_mutex_init\n");
		#endif
		pthread_mutex_init(&m_pthread_mutex,NULL); //	m_pthread_mutex  = PTHREAD_MUTEX_INITIALIZER;
	}
	

}


void CDBQueue::ViewKeyList()
{
	NODE* pTempNode = m_pHead->next;
	while(pTempNode != m_pTail )
	{
		//m_Log.setLog("CDBQueue : ViewKeyList !!  ( %d)   \n",pTempNode->nKey);
		pTempNode = pTempNode->next;
	}
}


