#ifndef _DCMD_PROC
#define _DCMD_PROC


#ifndef _FILE_OFFSET_BITS 
#define _FILE_OFFSET_BITS 64
#endif

#include <stdio.h>      /* for printf() and fprintf() */ 
#include <sys/socket.h> /* for socket(), bind(), and connect() */ 
#include <string.h>     /* for memset() */ 
#include <errno.h> 
#include <pthread.h>        /* for POSIX threads */
#include <unistd.h>     /* for close() getpid()*/ 
#include <sys/socket.h> /* for socket(), bind(), and connect() */ 
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */ 

#include "dcmddefine.h"
#include "dcmdsock.h" //sock send recv 
#include "apstruct.h"
#include "comhead.h"
#include "apdefine.h" //for log
#include "comcomm.h" //for log
#include "comconf.h"

#include "dcmd9001.h"
#include "dcmd9002.h"
#include "dcmd9004.h"
#include "dcmd9005.h"
#include "dcmd9006.h"
//#include "dcmd9009.h"
#include "dcmd9101.h"
#include "dcmd9102.h"
#include "dcmd9103.h"
#include "dcmd9104.h"
#include "dcmd9105.h"
#include "dcmd9106.h" // AeCO ¡Æ¢®¢Òo ¨ú¡À¡¤I¥ìa
#include "dcmd9201.h"
#include "dcmd5160.h" // AeCO

#include "dcmdfdns3004.h"
#include "dcmdfups4001.h"
#include "dcmdfups4002.h"
#include "dcmdfups4003.h"
#include "dcmdfups4005.h"
#include "dcmdfups4006.h"
#include "dcmdfups40051.h"
#include "dcmdfups40061.h"

void *ThreadMain(void *threadArgs);
void HandleTCPClient(int clntSocket);
int WaitForRequest(int& Socket);
long processed(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
int RequestUserList(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
void GetErrMsg(int nErrno,char* szErrMsg);

/*
#include "dcmddsp1031.h"
#include "dcmddsp1021.h"
*/
#endif

