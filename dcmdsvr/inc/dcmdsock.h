#ifndef _DCMD_SOCKET
#define _DCMD_SOCKET

int SendData(int nSockID,char* SendBuffer,int nSendLen); 
int RecvData(int nSockID,char* RecvBuffer,int nRecvLen); 
int AcceptTCPConnection(int servSock);
int CreateTCPServerSocket(unsigned short port);
int Connect(char* pServerIP, int nPort);
int SetSockTimeOut(int nSock, unsigned int nSec,int nMode  );
#endif
