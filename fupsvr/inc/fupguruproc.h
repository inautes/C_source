#ifndef _FUP_GURU_PROC
#define _FUP_GURU_PROC


#ifndef _FILE_OFFSET_BITS 
#define _FILE_OFFSET_BITS 64
#endif

int RequestGuruFilUp(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData); //공유 광장 파일 업로드
int GuruFileRequestList(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData);

#endif
