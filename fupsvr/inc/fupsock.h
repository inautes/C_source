#ifndef _FUP_SOCKET
#define _FUP_SOCKET


#ifndef _FILE_OFFSET_BITS 
#define _FILE_OFFSET_BITS 64
#endif

int AcceptTCPConnection(int servSock);
int CreateTCPServerSocket(unsigned short port);
int RecvFileData(int nSockID,char* RecvBuffer,long nRecvLen,double dTotalLen);
int RecvData(int nSockID,char* RecvBuffer,int nRecvLen);
int SendData(int nSockID,char* SendBuffer,int nSendLen);

#endif
