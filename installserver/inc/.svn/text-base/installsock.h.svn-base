#ifndef _INST_SOCKET
#define _INST_SOCKET


#ifndef _FILE_OFFSET_BITS 
#define _FILE_OFFSET_BITS 64
#endif

int SendData(int nSockID,char* SendBuffer,int nSendLen);
int RecvData(int nSockID,char* RecvBuffer,int nRecvLen);
int AcceptTCPConnection(int servSock);
int CreateTCPServerSocket(unsigned short port);
char* my_addrs();

#endif
