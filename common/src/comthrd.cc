/******************************************************************************
 *   서브시스템 : 공통모듈
 *   프로그램명 : apthread.c
 *         기능 : THREAD 관련함수
 *         설명 :
 *       작성자 : 임성은
 *       작성일 : 1999/04/10
 *     수정이력 :
 *
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

/*
** THREAD MUTEX
*/
pthread_mutex_t mutexGLB;
pthread_mutex_t mutexLOG;
pthread_mutex_t mutexDBR;
pthread_mutex_t mutexRTN;

#define      THREAD_LOCK_GLB()       pthread_mutex_lock(  &mutexGLB )
#define      THREAD_UNLK_GLB()       pthread_mutex_unlock(&mutexGLB )
#define      THREAD_LOCK_LOG()       pthread_mutex_lock(  &mutexLOG )
#define      THREAD_UNLK_LOG()       pthread_mutex_unlock(&mutexLOG )
#define      THREAD_LOCK_DBR()       pthread_mutex_lock(  &mutexDBR )
#define      THREAD_UNLK_DBR()       pthread_mutex_unlock(&mutexDBR )
#define      THREAD_LOCK_RTN()       pthread_mutex_lock(  &mutexRTN )
#define      THREAD_UNLK_RTN()       pthread_mutex_unlock(&mutexRTN )

/******************************************************************************
** MUTEX 설정
******************************************************************************/
int		infInitMutex(char *szErrMsg)
{
    if ( pthread_mutex_init(&mutexGLB, NULL) != 0 )
    {
		sprintf(szErrMsg, "Thread Mutex Init Fail(GLB)...errno[%s]\n", strerror(errno));
		return(-1);
	}
    if ( pthread_mutex_init(&mutexLOG, NULL) != 0 )
    {
		sprintf(szErrMsg, "Thread Mutex Init Fail(LOG)...errno[%s]\n", strerror(errno));
		return(-1);
	}
    if ( pthread_mutex_init(&mutexDBR, NULL) != 0 )
    {
		sprintf(szErrMsg, "Thread Mutex Init Fail(DBR)...errno[%s]\n", strerror(errno));
		return(-1);
	}
    if ( pthread_mutex_init(&mutexRTN, NULL) != 0 )
    {
		sprintf(szErrMsg, "Thread Mutex Init Fail(RTN)...errno[%s]\n", strerror(errno));
		return(-1);
	}

	return (0);
}

/******************************************************************************
** MUTEX 닫기
******************************************************************************/
void	infTermMutex()
{
	pthread_mutex_destroy(&mutexGLB);
	pthread_mutex_destroy(&mutexDBR);
	pthread_mutex_destroy(&mutexLOG);
	pthread_mutex_destroy(&mutexRTN);
}

/******************************************************************************
 * End of file...
 *****************************************************************************/
