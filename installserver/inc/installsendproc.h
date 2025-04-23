#ifndef _INST_GURU_PROC
#define _INST_GURU_PROC


#ifndef _FILE_OFFSET_BITS 
#define _FILE_OFFSET_BITS 64
#endif

int FileRequestList(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
int FileRequestFile(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
int GetFileList(char* path,int nCount,INSTALLFILEINFO* &pResult , LPCFDNLIST_R pUpdateList);
int RequestEol(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
bool CheckFileName(char *pFileName);
#endif
