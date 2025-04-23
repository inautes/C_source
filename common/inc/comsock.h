#ifndef _COM_SOCKET
#define _COM_SOCKET


#ifndef _FILE_OFFSET_BITS 
#define _FILE_OFFSET_BITS 64
#endif

long ComSendData(int nSockID,char* SendBuffer,long nSendLen);
long ComRecvData(int nSockID,char* RecvBuffer,long nRecvLen);

#endif
