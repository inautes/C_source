#include "fdnsock.h"
#include "fdndefine.h"
#include "apdefine.h"
#include "comcomm.h"
#include "comhead.h"
#include "fdncomproc.h"

#include <stdio.h>
#include <string.h>     /* for memset() */
#include <sys/types.h>
#include <fcntl.h>
#include <time.h> //randomize()
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>     /* for close() */
#include <stdlib.h>     /* for atoi() and exit() */

extern multimap<int,USERINFO>m_UserList;
extern char g_szOsp_id[10];
extern char g_szDcmdIP[16];
extern int  g_nDcmdPort; //  = 0;

extern char g_szSUB_DcmdIP[16];
extern int  g_nSUB_DcmdPort;
extern bool g_bConnect1 ;
extern bool g_bConnect2 ;

int RequestEol(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{


	HEADER headers;
	memset(&headers,0x00,HEADER_SIZE);

	pSendData = new char[sizeof(HEADER)];
	memset(pSendData,0x00,sizeof(HEADER));

	headers.nCmd = RS_EOL;
	headers.nDataCnt = 0;
	headers.nDataSize = 0;

	memcpy(pSendData,&headers,sizeof(HEADER));


	return END;//(HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));//return 1;
}



// RS_FILE_REQUEST_ID_DISCONNECT ====================
int FileRequestIdDisconnect(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	LPHEADER pHeader = (LPHEADER)pRecvHead;
	ERR_HEADER errheader;
	memset(&errheader,0x00,sizeof(ERR_HEADER));
	HEADER headers;
	memset(&headers,0x00,sizeof(HEADER));
	CSERVERINFO serverinfo;
	memset(&serverinfo,0x00,sizeof(CSERVERINFO));

	bool bFind = false;
	multimap<int,USERINFO>::iterator mi;

	mi = m_UserList.begin();
	while(mi != m_UserList.end())

	{

		if(strcmp(mi->second.szUserID,pHeader->szUserID) == 0)

		{

			 close(mi->first);

			 m_UserList.erase(mi);

			 bFind = true;



			 break;

		}

		mi++;



	}

	mi = m_UserList.find(Socket);

	if(mi != m_UserList.end())

	{

		memcpy(mi->second.szUserID, pHeader->szUserID , sizeof(pHeader->szUserID));

	}


	headers.nCmd = RS_FILE_REQUEST_ID_DISCONNECT_OK;

	headers.nDataCnt =0;
	headers.nDataSize = 0;
	headers.nErrorCode = 0;

	pSendData = new char[sizeof(HEADER) + headers.nDataCnt * headers.nDataSize];

	memset(pSendData,0x00,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize);
	memcpy(pSendData,&headers,sizeof(HEADER)); //head


	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));


}


int FileCheckID(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	/////// id check ////////////
	LPHEADER pHeader = (LPHEADER)pRecvHead;
	ERR_HEADER errheader;
	memset(&errheader,0x00,sizeof(ERR_HEADER));

	HEADER headers;
	memset(&headers,0x00,sizeof(HEADER));

	CSERVERINFO serverinfo;
	memset(&serverinfo,0x00,sizeof(CSERVERINFO));


	bool bFind = false;

	multimap<int,USERINFO>::iterator mi;
	//mi = m_UserList.begin(); change

	mi = m_UserList.find(Socket);
	if(mi != m_UserList.end())
	{
		memcpy(mi->second.szUserID, pHeader->szUserID , sizeof(pHeader->szUserID));
	}

	strcpy(mi->second.conn_ip ,mi->second.thread.userIP);
	strcpy(mi->second.gCom9xxx.com9002_R.conn_ip ,mi->second.thread.userIP);
	strcpy(mi->second.gCom9xxx.com9102_R.conn_ip ,mi->second.thread.userIP);
	memcpy(mi->second.gCom9xxx.com9002_R.user_id,pHeader->szUserID,sizeof(pHeader->szUserID));
	memcpy(mi->second.gCom9xxx.com9102_R.user_id,pHeader->szUserID,sizeof(pHeader->szUserID));

	CCOM9201_R com9201_r;
	memset(&com9201_r,0x00,sizeof(CCOM9201_R));

	strcpy(com9201_r.conn_ip ,mi->second.conn_ip);
	memcpy(com9201_r.user_id,pHeader->szUserID,sizeof(pHeader->szUserID));


	if(pRecvData)
	{
		LPFILEINFO pFileinfo = (LPFILEINFO)pRecvData;
		strcpy(com9201_r.cer_key ,pFileinfo->szCerKey); // , sizeof(com9201_r.cer_key));
	}


	int nRet=-1;
	if(g_bConnect1 == true)
	{
		nRet = com9201( com9201_r,g_szDcmdIP,g_nDcmdPort);
	}
	else
	{	// 1번 안되면 2번 무조건 접속.
		nRet = com9201( com9201_r,g_szSUB_DcmdIP,g_nSUB_DcmdPort);
	}

	headers.nCmd = RS_FILE_CHECK_ID_OK;

	headers.nDataCnt =0;
	headers.nDataSize = 0;
	headers.nErrorCode = 0;

	pSendData = new char[sizeof(HEADER) + headers.nDataCnt * headers.nDataSize];

	memset(pSendData,0x00,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize);
	memcpy(pSendData,&headers,sizeof(HEADER)); //head
	// 아이디 중복 채크 제거 ///



	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));
}


