#include "CSchQueue.h"

CSchQueue::CSchQueue()
{
	// create HEAD and TAIL NODE
	m_pHead = NULL;
	m_pTail	 = NULL;

	InitQue();
}


CSchQueue::~CSchQueue()
{

	if(m_pHead != NULL)
		delete m_pHead;
	if(m_pTail != NULL)
		delete m_pTail;
}



DB_SCHEDULE* CSchQueue::ReplaceKey(int uiSrcKey , int uiDestKey )
{
	
	//m_Log.setLog("CQueue : ReplaceKey ( %ld ) ( %ld ) \n",uiSrcKey,uiDestKey );
	LockThreadSafe();
	NODE* pTempNode = m_pHead->next;
	while( pTempNode != NULL &&  pTempNode != m_pTail )
	{
		//m_Log.setLog("CQueue : ReplaceKey  \n" );
		
		if(uiSrcKey == pTempNode->nKey)
		{	
			//m_Log.setLog("CQueue : find ReplaceKey ( %ld ) ( %ld ) \n",uiSrcKey,uiDestKey );
			
			pTempNode->nKey = uiDestKey;
			UnLockThreadSafe();
			return &pTempNode->data;
		}
		UnLockThreadSafe();
		LockThreadSafe();		
		pTempNode = pTempNode->next;
		
	}
	
	UnLockThreadSafe();
	return NULL;
}



DB_SCHEDULE* CSchQueue::Find(int nKey )
{
	////m_Log.setLog("CSchQueue : Find ( %ld )  \n",nKey );
	
	LockThreadSafe();
	
	NODE* pTempNode = m_pHead->next;
	while( pTempNode != NULL && pTempNode != m_pTail )
	{
		////m_Log.setLog("CSchQueue : Find  ( %d ) == ( %d )  \n",nKey ,pTempNode->nKey );
		if(nKey == pTempNode->nKey)
		{	
		//	//m_Log.setLog("CQueue : Find !! ( %ld )  \n",nKey );
			UnLockThreadSafe();
			return &pTempNode->data;
		}
		UnLockThreadSafe();
		LockThreadSafe();		
		pTempNode = pTempNode->next;		
		
	}
	
	UnLockThreadSafe();
	return NULL;
}

/*

T CSchQueue::Find(int nKey )
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

bool CSchQueue::Find(int nKey ,DB_SCHEDULE & data , bool bDeleteNode =  true)
{
	LockThreadSafe();

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
			
			UnLockThreadSafe();
			return true;
		}
		UnLockThreadSafe();
		LockThreadSafe();		
		pTempNode = pTempNode->next;
	}
	
	UnLockThreadSafe();
	return false;
	
}


void CSchQueue::InitQue()
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
	//m_lInsertNodeCount = 0 ;
	//m_lDeleteNodeCount = 0 ;

	m_pHead = new NODE;
	m_pTail = new NODE;

	// Set Head NODE
	m_pHead->prev = m_pHead;
	m_pHead->next = m_pTail;
	// Set Tail NODE
	m_pTail->prev = m_pHead;
	m_pTail->next = m_pTail;
	
	//pthread_mutex_init(&m_node_count_mutex,NULL);
	
	
	
}

void CSchQueue::UseMutex(bool bUse)
{
	m_bUseMutex = bUse;
	
	if( m_bUseMutex )
	{
		#ifdef __DEBUG
		//printf( "CSchQueue : pthread_mutex_init\n");
		#endif
		pthread_mutex_init(&m_pthread_mutex,NULL); //	m_pthread_mutex  = PTHREAD_MUTEX_INITIALIZER;
	}
	

}

DB_SCHEDULE* CSchQueue::Push(int nKey , DB_SCHEDULE* pData)
{
	//m_Log.setLog("CSchQueue : Push ( %ld )  \n",nKey );
	LockThreadSafe();
	
	//pthread_mutex_lock(&m_node_count_mutex);
	m_lNodeCount++;
	//pthread_mutex_unlock(&m_node_count_mutex);
	//m_lInsertNodeCount++;
	

 	NODE* pInsertNode = new NODE;
	memset(pInsertNode,0x00,sizeof(NODE));
	
	pInsertNode->nKey = nKey;

	pInsertNode->data = *pData;
	m_pTail->prev->next = pInsertNode;
	pInsertNode->prev = m_pTail->prev;
	m_pTail->prev = pInsertNode;
	pInsertNode->next = m_pTail;
	
	
	UnLockThreadSafe();	
	//m_Log.setLog("CSchQueue : Push ( %ld ) end \n",nKey );
	return &(pInsertNode->data);
	
}


bool CSchQueue::GetKey(int & nDestKey)
{
	
	if(m_pHead->next == m_pTail)
	{
		//printf("\nError :: Remove Function");
		return false;
	}
	NODE* pTemp;
	pTemp = m_pHead->next;
	nDestKey = pTemp->nKey;
	
	return true;
	
	
}
bool CSchQueue::Pop(DB_SCHEDULE & pDestBuffer)
{

	LockThreadSafe();
	
	if(m_pHead->next == m_pTail)
   {
	   //printf("\nError :: Remove Function");
	   UnLockThreadSafe();
	   
	   //m_Log.setLog("CSchQueue : Pop return false \n");
	   
	   return false;
   }

   NODE* pTemp;
   pTemp = m_pHead->next;
   pDestBuffer = pTemp->data;

   m_pHead->next = pTemp->next;
   pTemp->next->prev = m_pHead;
//   m_pTail->prev=m_pTail->prev->prev;

	//m_Log.setLog("CSchQueue : Pop  Key ( %d ) \n",pTemp->nKey);
	
   delete  pTemp;
   pTemp = NULL;
	
	//pthread_mutex_lock(&m_node_count_mutex);
   	m_lNodeCount--;
	//pthread_mutex_unlock(&m_node_count_mutex);	
	
   //m_lDeleteNodeCount++ ;
   

   UnLockThreadSafe();
   
   
   
   return true;
   
	
}


DB_SCHEDULE *  CSchQueue::Front()
{

	LockThreadSafe();
	
	if(m_pHead->next == m_pTail)
   {
	   //printf("\nError :: Remove Function");
	   UnLockThreadSafe();
	   return NULL;
   }

   NODE* pTemp;
   pTemp = m_pHead->next;
   UnLockThreadSafe();
   
   return &pTemp->data;
	
}


bool CSchQueue::RemoveAll()
{
	LockThreadSafe();
	
	if(m_pHead->next == m_pTail)
	{
		if( GetTotalNodeCount() != 0)
			printf("Error : Node TotalCount Error , Node Count is %ld\n",GetTotalNodeCount());
		//printf("Error :: RemoveAll");
		
		UnLockThreadSafe();
		return false;
	}

   NODE* pCurrent;
   NODE* pOld;
   pCurrent = m_pHead->next;
   while(pCurrent != m_pTail)
   {
	    pOld = pCurrent;
		pCurrent = pCurrent->next;
		
		//pthread_mutex_lock(&m_node_count_mutex);
		m_lNodeCount--;
		//pthread_mutex_unlock(&m_node_count_mutex);
		
		//m_lDeleteNodeCount++;

				
		delete pOld;
		pOld = NULL;
   }
   m_pHead->next = m_pTail;
   m_pTail->prev = m_pHead;

	UnLockThreadSafe();
	
   return true;
}



bool CSchQueue::Remove(NODE* pNode)
{

	
	LockThreadSafe();
	
	
   if(m_pHead->next == m_pTail)
   {
	   //printf("\nError :: Remove Function");
	   
	   UnLockThreadSafe();
	   return false;
   }


   pNode->prev->next = pNode->next;
   pNode->next->prev = pNode->prev;

	
   delete  pNode;
   pNode = NULL;
	
	//pthread_mutex_lock(&m_node_count_mutex);
  	m_lNodeCount--;
   	//pthread_mutex_unlock(&m_node_count_mutex);
   
   	//m_lDeleteNodeCount++;
   

	UnLockThreadSafe();
   
   return true;
}



int CSchQueue::GetTotalNodeCount()
{
	
	////m_Log.setLog("CSchQueue : GetTotalNodeCount ( %ld ) - ( %ld ) \n",//m_lInsertNodeCount,//m_lDeleteNodeCount );
	
	//return //m_lInsertNodeCount - //m_lDeleteNodeCount ;
	return m_lNodeCount;
	
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
	
	
}



int CSchQueue::MutexLock()
{
	if( m_bUseMutex )	
	{
		#ifdef __DEBUG
		//printf( "CSchQueue : MutexLock\n");
		#endif
		
		//m_Log.setLog( "CSchQueue : MutexLock\n");
		
		return pthread_mutex_lock(&m_pthread_mutex);
	}
	return -1;
}
int CSchQueue::MutexUnLock()
{
	if( m_bUseMutex )
	{
		#ifdef __DEBUG
		//printf( "CSchQueue : MutexUnLock\n");
		#endif
		//m_Log.setLog( "CSchQueue : MutexUnLock\n");
		return pthread_mutex_unlock(&m_pthread_mutex);
	}
	return -1;
	
}
bool CSchQueue::IsThreadSafe()
{	
	return m_bThreadSafe;
}

void CSchQueue::LockThreadSafe()
{
	m_bThreadSafe = false;
	MutexLock();
	
	
	
}
void CSchQueue::UnLockThreadSafe()
{
	MutexUnLock();
	m_bThreadSafe = true;	
	
	
}

bool CSchQueue::IsEmpty()
{
	if(m_pHead->next == m_pTail)
		return true;
	return false;
}


void CSchQueue::ViewKeyList()
{
	NODE* pTempNode = m_pHead->next;
	while(pTempNode != m_pTail )
	{
		//m_Log.setLog("CSchQueue : ViewKeyList !!  ( %d)   \n",pTempNode->nKey);
		pTempNode = pTempNode->next;
	}
}

