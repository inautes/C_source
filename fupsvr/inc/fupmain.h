/******************************************************************************
 *   ����ý��� : file upload ����
 *   ���α׷��� : fupmain.h
 *         ��� : cmdmain�� ���� ���
 *         ���� :
 *       �ۼ��� : LEE
 *       �ۼ��� : 2004/02/16
 *     �����̷� :
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#ifndef	_FUPMAIN_H_
#define	_FUPMAIN_H_

#ifndef _FILE_OFFSET_BITS 
#define _FILE_OFFSET_BITS 64
#endif


//void *ThreadMain(void *arg);  /* Main program of a thread */
void* mon_thread(void *arg);  // moniter thread


extern SUserParm_T gstUserParm;
extern char	errMsg[256];

extern multimap<int,USERINFO>m_UserList;

extern pthread_cond_t async_cond ;
extern pthread_mutex_t mutex_lock ;
extern pthread_mutex_t async_mutex ;

extern char g_szDcmdIP[20];
extern int g_nDcmdPort;
extern char g_szSUB_DcmdIP[20];
extern int g_nSUB_DcmdPort;


#endif

/******************************************************************************
 * End of file...
 *****************************************************************************/

