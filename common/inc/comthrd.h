/******************************************************************************
 *   서브시스템 : 공통모듈
 *   프로그램명 : apthread.h
 *         기능 : THREAD 관련 헤더
 *         설명 :
 *       작성자 : 임성은
 *       작성일 : 1999/04/10
 *     수정이력 :
 *
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/

#ifndef	_APTHREAD_H_
#define	_APTHREAD_H_

/*
** 전역변수
*/
extern	pthread_mutex_t mutexGLB;
extern	pthread_mutex_t mutexLOG;
extern	pthread_mutex_t mutexDBR;
extern	pthread_mutex_t mutexRTN;

/*
** 매크로정의
*/
#define      THREAD_LOCK_GLB()       pthread_mutex_lock(  &mutexGLB )
#define      THREAD_UNLK_GLB()       pthread_mutex_unlock(&mutexGLB )
#define      THREAD_LOCK_LOG()       pthread_mutex_lock(  &mutexLOG )
#define      THREAD_UNLK_LOG()       pthread_mutex_unlock(&mutexLOG )
#define      THREAD_LOCK_DBR()       pthread_mutex_lock(  &mutexDBR )
#define      THREAD_UNLK_DBR()       pthread_mutex_unlock(&mutexDBR )
#define      THREAD_LOCK_RTN()       pthread_mutex_lock(  &mutexRTN )
#define      THREAD_UNLK_RTN()       pthread_mutex_unlock(&mutexRTN )

/******************************************************************************
** 함수 프로토타입 정의
******************************************************************************/
extern	int		infInitMutex(char *szErrMsg);
extern	void	infTermMutex();

#endif

/******************************************************************************
 * End of file...
 *****************************************************************************/
