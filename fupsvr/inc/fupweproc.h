#ifndef _FUP_WEDISK_PROC
#define _FUP_WEDISK_PROC


#ifndef _FILE_OFFSET_BITS 
#define _FILE_OFFSET_BITS 64
#endif

int FileRequestFile(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
int FileRequestNextFile(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
int FileDataTransfer(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
int FileRequestList(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);

#endif
