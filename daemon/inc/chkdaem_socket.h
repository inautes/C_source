#ifndef _CHKS_SOCKET
#define _CHKS_SOCKET

/*
#ifndef _FILE_OFFSET_BITS 
#define _FILE_OFFSET_BITS 64
#endif
*/

int Connect(char* pServerIP, unsigned int nPort);
int SendData(int nSockID,char* SendBuffer,int nSendLen);
int RecvData(int nSockID,char* RecvBuffer,int nRecvLen);

#endif
