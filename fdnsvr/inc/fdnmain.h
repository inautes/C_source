
#ifndef	_FDNMAIN_H_
#define	_FDNMAIN_H_

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif




extern multimap<int,USERINFO>m_UserList;

extern pthread_cond_t async_cond ;
extern pthread_mutex_t mutex_lock ;
extern pthread_mutex_t async_mutex ;



////////////////////////////////////////////////////////////////////////////////////////////////

void* mon_thread(void *arg);  // moniter thread


#endif /* end of __HEADER__ */
