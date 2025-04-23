
#ifndef	_DCMDMAIN_H_
#define	_DCMDMAIN_H_

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#include <map>
#include <stdio.h>      
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <stdlib.h>    
#include <string.h>    
#include <unistd.h>    
#include <errno.h> 
#include <sys/stat.h> 
#include <time.h> 
#include <pthread.h>
#include <signal.h>

#include "dcmdsock.h"
#include "dcmddefine.h"
#include "dcmdproc.h"
#include "apstruct.h"
#include "apdefine.h"
#include "comcomm.h"
#include "comconf.h"
#include "comsock.h"
#include "dcmddefine.h"
#include "mysql_pool.h"

using namespace std;

extern multimap<int,USERINFO>m_UserList;

extern pthread_cond_t async_cond ;
extern pthread_mutex_t mutex_lock ;
extern pthread_mutex_t async_mutex ;

////////////////////////////////////////////////////////////////////////////////////////////////

void* QueManager(void *threadArgs);
void* DBPoolQueManager(void *threadArgs);
void* HadoopManager(void *threadArgs);
void* HadoopPoolManager(void *threadArgs);

#endif /* end of __HEADER__ */
