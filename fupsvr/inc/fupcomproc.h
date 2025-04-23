#ifndef _FUP_COMMON_PROC
#define _FUP_COMMON_PROC


#ifndef _FILE_OFFSET_BITS 
#define _FILE_OFFSET_BITS 64
#endif

int RequestSpeedCheck(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
int RequestEol(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
int FileCheckID(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
int FileRequestIdDisconnect(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);
int RequestUserList(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData); //myd 사용자 목록 요청 10051
int AdminRequestIdDisconnect(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);

#endif
