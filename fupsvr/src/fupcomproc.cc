#include "fupsock.h"

#include "fupdefine.h"

#include "apdefine.h"
#include "comcomm.h"

#include "comhead.h"

#include "fupcomproc.h"
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

int RequestEol(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	#ifdef __DEBUG
	printf("RequestEol          ] RequestEol\n");
	#endif	

	infLOG(ALWAY, "RequestEol          ] start\n");	
	
	HEADER headers;
	memset(&headers,0x00,HEADER_SIZE);
	
	pSendData = new char[sizeof(HEADER)];
	memset(pSendData,0x00,sizeof(HEADER));
	
	headers.nCmd = RS_EOL;
	headers.nDataCnt = 0;
	headers.nDataSize = 0;
	
	memcpy(pSendData,&headers,sizeof(HEADER));
	infLOG(ALWAY, "RequestEol          ] end\n");		
	
	
	return END;//(HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));//return 1;
}


//2번의 속도 테스트 32K 4K

int RequestSpeedCheck(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	#ifdef __DEBUG	
	printf("RequestSpeedCheck   ] \n");
	#endif
	
	
	infLOG(ALWAY, "RequestSpeedCheck   ] start\n");	

	HEADER headers;
	memset(&headers,0x00,HEADER_SIZE);
	
	pSendData = new char[sizeof(HEADER)];
	memset(pSendData,0x00,sizeof(HEADER));
	
	headers.nCmd = RS_SPEED_CHECK_OK;//RS_MYDISK_FILE_REQUEST_FILE_FILINFO; //파일 정보 요청
	headers.nDataCnt = 0;
	headers.nDataSize = 0;
	
	memcpy(pSendData,&headers,sizeof(HEADER));
	
	#ifdef __DEBUG
	printf("RequestSpeedCheck   ] end\n");
	#endif
	
	infLOG(ALWAY, "RequestSpeedCheck   ] end\n");
	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));//return 1;	
}



int FileCheckID(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	/////// id check ////////////
	
	infLOG(ALWAY, "FileCheckID         ] FileCheckID\n");
	#ifdef __DEBUG
	printf("FileCheckID         ] loading file check id\n");
	#endif	
	
	LPHEADER pHeader = (LPHEADER)pRecvHead;
	ERR_HEADER errheader;
	memset(&errheader,0x00,sizeof(ERR_HEADER));
	HEADER headers;
	memset(&headers,0x00,sizeof(HEADER));
	CSERVERINFO serverinfo;
	memset(&serverinfo,0x00,sizeof(CSERVERINFO));

	bool bFind = false;
	
	//아이디 중복 제거//
	multimap<int,USERINFO>::iterator mi;
	
	
	//아이디 중복 제거//
	
	mi = m_UserList.begin();
	
	while(mi != m_UserList.end())
	{
		
		#ifdef __DEBUG
		printf("this ->  ID         ] Socket %d Find ID  ID : %s == %s\n",mi->first, mi->second.szUserID,pHeader->szUserID);
		#endif
			
		if(strcmp(mi->second.szUserID,pHeader->szUserID) == 0 && mi->first != Socket )
		{
			#ifdef __DEBUG
			printf("FileCheckID         ] Socket %d Find ID  ID : %s == %s\n\n",mi->first, mi->second.szUserID,pHeader->szUserID);
			#endif
			
			//close(mi->first);
			//m_UserList.erase(mi);
			bFind=true;
			break;
		}
		mi++;
	}
	
	
	/*
	// 아이디 삽입	
	//mi = m_UserList.begin(); change
	mi = m_UserList.find(Socket);
	if(mi != m_UserList.end())
	{
		memcpy(mi->second.szUserID,pHeader->szUserID,min(12,(int)sizeof(pHeader->szUserID)));
	}
	
	
	/*
	while(mi != m_UserList.end())
	{

		if(mi->first == Socket)
		{
			 //strcpy(mi->second.szUserID,pHeader->szUserID);
			 memcpy(mi->second.szUserID,pHeader->szUserID,min(12,(int)sizeof(pHeader->szUserID)));
			 break;
		}
		mi++;

	}
	*/


/*
	
	headers.nCmd = RS_FILE_CHECK_ID_OK;

	headers.nDataCnt =0;
	headers.nDataSize = 0;
	headers.nErrorCode = 0;
	
	pSendData = new char[sizeof(HEADER) + headers.nDataCnt * headers.nDataSize];

	memset(pSendData,0x00,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize);
	memcpy(pSendData,&headers,sizeof(HEADER)); //head
/*	
	//아이디 중복 제거//
	
	mi = m_UserList.begin();
	
	while(mi != m_UserList.end())
	{
		
		if(strcmp(mi->second.szUserID,pHeader->szUserID) == 0)
		{
			#ifdef __DEBUG
			printf("FileCheckID         ] Find ID  ID : %s == %s\n",mi->second.szUserID,pHeader->szUserID);
			#endif
			
			//bFind=true;
			break;
		}
		mi++;
	}
*/	
	

	if(bFind) //찾았으면
	{
		headers.nCmd = RS_FILE_CHECK_ID_FAIL; // 같은것이 있다.
		strcpy(serverinfo.szIP , mi->second.thread.userIP);
		
		headers.nDataCnt =1;
		headers.nDataSize =sizeof(CSERVERINFO);
		headers.nErrorCode = 0;
		
		#ifdef __DEBUG	
		printf("FileCheckID         ] %s ID send RS_FILE_CHECK_ID_FAIL\n",mi->second.szUserID);
		#endif
		
		infLOG(ERROR,"FileCheckID         ] %s ID send RS_FILE_CHECK_ID_FAIL\n",mi->second.szUserID);
		
		pSendData = new char[sizeof(HEADER) + headers.nDataCnt * headers.nDataSize];
	
		memset(pSendData,0x00,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize);
		
		memcpy(pSendData,&headers,sizeof(HEADER)); //head
		memcpy(pSendData + HEADER_SIZE,&serverinfo,headers.nDataCnt * headers.nDataSize); //body
	
	//	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));	
		
	}
	else //같은것이 없다.
	{
		#ifdef __DEBUG
		printf("FileCheckID         ] not have same id\n");
		#endif
		// socket 비교해서 id 넣기
		

    	mi = m_UserList.find(Socket);
    	if(mi != m_UserList.end())
    	{
    		memcpy(mi->second.szUserID,pHeader->szUserID,min(12,(int)sizeof(pHeader->szUserID)));
    	}
    			
		/*
		mi = m_UserList.begin();
		while(mi != m_UserList.end())
		{
	
			if(mi->first == Socket)
			{
				memcpy(mi->second.szUserID,pHeader->szUserID,min(12,(int)sizeof(pHeader->szUserID)));
				 //strcpy(mi->second.szUserID,pHeader->szUserID);
				 break;
			}
			mi++;
	
		}
	    */
		#ifdef __DEBUG
		printf("FileCheckID         ] ID %s send RS_FILE_CHECK_ID_OK\n",pHeader->szUserID);
		#endif
		
		headers.nCmd = RS_FILE_CHECK_ID_OK;

		headers.nDataCnt =0;
		headers.nDataSize = 0;
		headers.nErrorCode = 0;
		
		pSendData = new char[sizeof(HEADER) + headers.nDataCnt * headers.nDataSize];
	
		memset(pSendData,0x00,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize);
		memcpy(pSendData,&headers,sizeof(HEADER)); //head
		
	}
	infLOG(ALWAY, "FileCheckID         ] Successed FileCheckID\n");	
	
	
		
	
	#ifdef __DEBUG
	printf("FileCheckID         ] FileCheckID check succeed\n");
	#endif
	
	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));
}

int AdminRequestIdDisconnect(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	//접속 끝고 #define RS_FILE_REQUEST_ID_DISCONNECT_OK 514
		
	#ifdef __DEBUG
	printf("AdminRequestIdDis   ] loading disconnect id\n");
	#endif
	
	
	LPHEADER pHeader = (LPHEADER)pRecvHead;
	ERR_HEADER errheader;
	memset(&errheader,0x00,sizeof(ERR_HEADER));
	HEADER headers;
	memset(&headers,0x00,sizeof(HEADER));
	
	LPDISUSER pUserID = (LPDISUSER)pRecvData;
	
	multimap<int,USERINFO>::iterator mi;
	mi = m_UserList.begin();
	while(mi != m_UserList.end())
	{

		if(strcmp(mi->second.szUserID,pUserID->szUserID) == 0)
		{
			#ifdef __DEBUG
			printf("FileRequestIdDis    ] close socket : %d user (%d)",mi->first,pUserID->szUserID);
			#endif
			
			 close(mi->first);
			 m_UserList.erase(mi);
			 break;
		}
		mi++;

	}
	
	headers.nCmd = RS_ADMIN_REQUEST_ID_DISCONNECT_OK;

	headers.nDataCnt =0;
	headers.nDataSize = 0;
	headers.nErrorCode = 0;
	
	pSendData = new char[sizeof(HEADER) + headers.nDataCnt * headers.nDataSize];

	memset(pSendData,0x00,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize);
	memcpy(pSendData,&headers,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize); //head

	#ifdef __DEBUG
	printf(" AdminRequestIdDis   ] disconnet successed\n");
	#endif
	
	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));	
}
int FileRequestIdDisconnect(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	//접속 끝고 #define RS_FILE_REQUEST_ID_DISCONNECT_OK 514
		
	#ifdef __DEBUG
	printf("FileRequestIdDis    ] loading disconnect id\n");
	#endif
	
	
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
			#ifdef __DEBUG
			printf("FileRequestIdDis    ] close socket : %d",mi->first);
			#endif
			
			 close(mi->first);
			 m_UserList.erase(mi);
			 bFind = true;
			 break;
		}
		mi++;

	}
	
	
	
		// ID 저장하기
	//mi = m_UserList.begin(); change
	mi = m_UserList.find(Socket);
	if(mi != m_UserList.end())
	{
		memcpy(mi->second.szUserID,pHeader->szUserID,min(12,(int)sizeof(pHeader->szUserID)));
	}
	/*
	while(mi != m_UserList.end())
	{

		if(mi->first == Socket )
		{
			 
			 //strcpy(mi->second.szUserID,pHeader->szUserID);
			 memcpy(mi->second.szUserID ,pHeader->szUserID,sizeof(pHeader->szUserID));
			 #ifdef __DEBUG
			 printf("  ./copy id : %s",mi->second.szUserID);
			 #endif
			 break;
		}
		mi++;

	}
	*/
	
	headers.nCmd = RS_FILE_REQUEST_ID_DISCONNECT_OK;

	headers.nDataCnt =0;
	headers.nDataSize = 0;
	headers.nErrorCode = 0;
	
	pSendData = new char[sizeof(HEADER) + headers.nDataCnt * headers.nDataSize];

	memset(pSendData,0x00,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize);
	memcpy(pSendData,&headers,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize); //head

	#ifdef __DEBUG
	printf(" FileRequestIdDis    ] disconnet successed\n");
	#endif
	
	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));
}






int RequestUserList(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	#ifdef __DEBUG
	printf("RequestUserList     ] RequestUserList \n");
	#endif
	
	LPUSERLIST pData = new USERLIST[2000];
	memset(pData,0x00,sizeof(USERLIST)*2000);


	multimap<int,USERINFO>::iterator mi;
	mi = m_UserList.begin();
	
	int nCount =0;
	
	while(mi != m_UserList.end())
	{
	
		if(mi->first == Socket)
			strcpy(pData[nCount].szUserID,"jadmin");
		else
			strcpy(pData[nCount].szUserID,mi->second.szUserID);
		strcpy(pData[nCount].szUserIP,mi->second.thread.userIP);
		pData[nCount].nNumber = nCount ;
		strcpy(pData[nCount].szStartTime,mi->second.thread.startTime);
			
		#ifdef __DEBUG
		printf("%4d ] [소켓번호 : %d] [주소 : %s] [아이디 : %s] [접속시간 %s]\n"
					,nCount+1,mi->first,mi->second.thread.userIP,mi->second.szUserID,mi->second.thread.startTime);
		#endif
		
		mi++;
		nCount++;
		if(nCount >= 1999)
			break;
		
	}
	
	HEADER header;
	memset(&header,0x00,sizeof(header));
	
	header.nCmd = RS_CMD_REQUEST_USER_LIST;
	header.nDataCnt = nCount;
	header.nDataSize = sizeof(USERLIST);
	
	pSendData  = new char[HEADER_SIZE + sizeof(USERLIST)*nCount];
	memset(pSendData ,0x00,HEADER_SIZE + sizeof(USERLIST)*nCount);
	
	memcpy( pSendData , &header,HEADER_SIZE);
	
	if(sizeof(USERLIST)*nCount > 0)
		memcpy( pSendData + HEADER_SIZE, pData ,sizeof(USERLIST)*nCount);
	
	#ifdef __DEBUG
	printf("RequestUserList     ] end RequestUserList \n");
	#endif

	delete[] pData;
	return (HEADER_SIZE + (header.nDataCnt * header.nDataSize));	
}

