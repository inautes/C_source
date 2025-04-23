#ifndef _DCMD_PROC
#define _DCMD_PROC


#ifndef _FILE_OFFSET_BITS 
#define _FILE_OFFSET_BITS 64
#endif

#include <stdio.h>      
#include <sys/socket.h> 
#include <string.h>     
#include <errno.h> 
#include <pthread.h>    
#include <unistd.h>     
#include <sys/socket.h> 
#include <arpa/inet.h>  

#include "dcmddefine.h"
#include "dcmdsock.h" 
#include "dcmd5160.h" 

#include "apstruct.h"
#include "comhead.h"
#include "apdefine.h" 
#include "comcomm.h" 
#include "comconf.h"

void *ThreadMain(void *threadArgs);
void HandleTCPClient(int clntSocket);
int WaitForRequest(int& Socket);
long processed(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
int RequestUserList(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
void GetErrMsg(int nErrno,char* szErrMsg);

#endif

