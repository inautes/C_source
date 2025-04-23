#ifndef _FUP_FUP_PROC	
#define _FUP_FUP_PROC


#ifndef _FILE_OFFSET_BITS 
#define _FILE_OFFSET_BITS 64
#endif

long processed(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
int WaitForRequest(int& Socket);
void HandleTCPClient(int clntSocket);
void *ThreadMain(void *threadArgs);

#endif
