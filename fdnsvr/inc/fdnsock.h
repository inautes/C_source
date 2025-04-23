#ifndef _FDN_SOCKET
#define _FDN_SOCKET


#ifndef _FILE_OFFSET_BITS 
#define _FILE_OFFSET_BITS 64
#endif

long SendData(int nSockID,char* SendBuffer,long nSendLen);
long RecvData(int nSockID,char* RecvBuffer,long nRecvLen);
int AcceptTCPConnection(int servSock);
int CreateTCPServerSocket(unsigned short port);

#endif
