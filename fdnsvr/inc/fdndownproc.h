#ifndef _FUDN_DOWN_PROC
#define _FUP_DOWN_PROC


#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

//grid
int FileRequestGridTransferNext(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
int FileRequestGridFail(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
int FileRequestGridKeepAliveCheck(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
//grid


int FileRequestNextFile(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
int GetFileList(char* path,int nCount,FILEINFO* &pResult,char* pRecvData,char* pSrcPath);
int FileRequestList(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
int FileRequestFileWithHoldTime(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
int FileRequestFile(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
int RequesetHoldTime(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);

#endif

