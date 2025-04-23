#ifndef _FDN_FUP_PROC	
#define _FDN_FUP_PROC


#ifndef _FILE_OFFSET_BITS 
#define _FILE_OFFSET_BITS 64
#endif

int Processed(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
int WaitForRequest(int& Socket);
void HandleTCPClient(int clntSocket);   /* TCP client handling function */
void *ThreadMain(void *arg);  /* Main program of a thread */

void SendRealCount(int nAppand,unsigned long uFileid=0);

#endif
