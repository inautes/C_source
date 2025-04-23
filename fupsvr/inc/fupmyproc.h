#ifndef _FUP_MYDISK_PROC
#define _FUP_MYDISK_PROC


#ifndef _FILE_OFFSET_BITS 
#define _FILE_OFFSET_BITS 64
#endif

int MyDiskFileRequestFile(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
int MyDiskFileRequestNextFile(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
int MyDiskFileDataTransfer(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
int MyDiskFileRequestList(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);

#endif
