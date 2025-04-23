#include "installsock.h"

#include "installdefine.h"
#include "installsendproc.h"
#include "installcomlib.h"

#include "comcomm.h"
#include "apdefine.h"
#include "comhead.h" //for log

#include <stdio.h>
#include <string.h>     /* for memset() */
#include <fcntl.h>
#include <time.h> //randomize()
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>     /* for close() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <sys/types.h> //opendir
#include <dirent.h> //opendir

extern char	g_szOsp_id[10];
extern char	g_szFilepath[256];
extern int	g_nPort;

int FileRequestList(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	LPHEADER pHeader = (LPHEADER)pRecvHead;
	ERR_HEADER errheader;
	memset(&errheader,0x00,sizeof(ERR_HEADER));

	HEADER headers;
	memset(&headers,0x00,sizeof(HEADER));

	LPCFDNLIST_R pRecv = (LPCFDNLIST_R)pRecvData;

	int nFileCount = pHeader->nDataCnt;

	// 파일 목록 얻기 (폴더로 부터)

	LPINSTALLFILEINFO pResult = new INSTALLFILEINFO[nFileCount]; //보낼수 있는 파일 갯수는최대 100개
	memset(pResult,0x00,sizeof(INSTALLFILEINFO)*nFileCount);

	char szFullPath[612];
	memset(szFullPath,0x00,sizeof(szFullPath));

	char szLocalName[612];
	memset(szLocalName,0x00,sizeof(szLocalName));

	// full path 만들기.

	strcpy(szFullPath, g_szFilepath	);

	if((headers.nDataCnt = GetFileList(szFullPath,nFileCount,pResult, pRecv)) < 0) //파일 리스트 갯수를 얻는다.
	{
		pSendData = new char[sizeof(ERR_HEADER)];
		memset(pSendData,0x00,sizeof(ERR_HEADER));
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_INSTALL_FILE_DN_LIST;
		strcat(errheader.errmsg,"리스트를 작성에 실패 하였습니다.");

		memcpy(pSendData, &errheader, sizeof(ERR_HEADER));
		delete[] pResult;

		infLOG(ERROR, " ] 리스트를 작성에 실패 하였습니다. ( %s )",szFullPath);

		#ifdef __DEBUG
		printf(" ] 리스트를 작성에 실패 하였습니다. ( %s )",szFullPath);
		#endif

		return -RS_INSTALL_FILE_DN_LIST;
	}
	else if(headers.nDataCnt == 0) //파일 리스트 갯수를 얻는다.
	{
		headers.nCmd = RS_EOL;
		headers.nDataCnt =0;
		headers.nDataSize = 0;
		headers.nErrorCode = 0;

		pSendData = new char[sizeof(HEADER) + headers.nDataCnt * headers.nDataSize];

		memset(pSendData,0x00,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize);
		memcpy(pSendData,&headers,sizeof(HEADER)); //head
		delete[] pResult;
		infLOG(ERROR, " ] 업데이트할 목록이 없습니다. RS_EOL 전송. \n");

		#ifdef __DEBUG
		printf(" ] 업데이트할 목록이 없습니다. RS_EOL 전송. \n");
		#endif

		return 1;
	}
	headers.nCmd = RS_INSTALL_FILE_DN_LIST;
	headers.nDataSize = sizeof(INSTALLFILEINFO);
	headers.nErrorCode = 0;

	pSendData = new char[sizeof(HEADER) + headers.nDataCnt * headers.nDataSize];

	memset(pSendData,0x00,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize);
	memcpy(pSendData,&headers,sizeof(HEADER)); //head
	memcpy(pSendData+sizeof(HEADER),pResult,headers.nDataCnt * headers.nDataSize);
	delete[] pResult;

	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));
}



int FileRequestFile(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	/////////////// 아이디 인증 작업 하기 //////////////////////
	// fileinfo를 얻기 위해 ...
	//////////////////////////////////////////////////////////

	LPHEADER pHeader = (LPHEADER)pRecvHead;

	ERR_HEADER errheader;
	memset(&errheader,0x00,sizeof(ERR_HEADER));

	HEADER headers;
	memset(&headers,0x00,sizeof(HEADER));

	LPINSTALLFILEHEAD pFileHead = (LPINSTALLFILEHEAD)pRecvData;

	#ifdef __DEBUG
	printf(" ] 자동 업데이트 파일 전송 (%s)\n\n",pHeader->szUserID);
	printf("  ./자동 업데이트 파일 해더 확인  \n"
		   "    file name : %s\n"
		   "    file size : %15.0f\n"
		   "	sizeof	  : %d\n"
		   ,    pFileHead->szFullFileName
		   ,	pFileHead->dCurrentSize
		   ,	sizeof(INSTALLFILEHEAD));
	#endif

	FILE* UploadFile;
	UploadFile = NULL;

	double dFileLen = 0;
	double nTotalnRecvLen = 0;

	//////////데이터 전송 루틴
	// file open
	UploadFile = fopen64(pFileHead->szFullFileName,"rtb");

	if(UploadFile == NULL)
	{
		infLOG(ERROR, " ] 파일 생성 실패 : %s \n",pFileHead->szFullFileName);

		#ifdef __DEBUG
		printf(" ] 파일 생성 실패 : %s \n",pFileHead->szFullFileName);
		#endif

		pSendData = new char[sizeof(ERR_HEADER)];
		memset(pSendData,0x00,sizeof(ERR_HEADER));
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_INSTALL_FILE_DN;
		strcat(errheader.errmsg,"서버측 에서 파일이 없거나 열수 없습니다.");

		memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

		pHeader->nCmd = RS_ERR;
		infLOG(ERROR, " > ERROR Exception ) Make File Error ( UploadFile == NULL )  \n");
		return -RS_INSTALL_FILE_DN;
	}
	#ifdef __DEBUG
	printf(" ] 파일 생성 : %s \n",pFileHead->szFullFileName);
	#endif

	//set file head , 전체 사이즈 , 파일 이름

	struct stat64 statbuf;

	int stat = stat64(pFileHead->szFullFileName,&statbuf);

	if(stat != 0) //error
	{
		if(errno == ENOENT) //파일이나 패스가 없음
		{
			infLOG(ERROR, " ] 파일 생성 실패[ENOENT] : %s \n",pFileHead->szFullFileName);
			#ifdef __DEBUG
			printf(" ] 파일 생성 실패[ENOENT] : %s \n",pFileHead->szFullFileName);
			#endif

			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_INSTALL_FILE_DN;
			strcat(errheader.errmsg,"서버측 에서 파일이 없거나 열수 없습니다.");

			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));
			pHeader->nCmd = RS_ERR;
			return -RS_INSTALL_FILE_DN;
		}
		return 0;
	}

	// 현제 client에 있는 사이즈 만큼 이동 하기
	double dCurrentLen = pFileHead->dCurrentSize;

	off64_t current_set = (off64_t)dCurrentLen;

	fseeko64(UploadFile,current_set,SEEK_SET);
	 // 해더 전송  실제 사이즈와 이름.

	//데이터 전송
	memset(&headers,0x00,HEADER_SIZE);

	headers.nCmd = RS_INSTALL_FILE_DN;
	headers.nDataCnt = 1;
	headers.nDataSize = sizeof(INSTALLFILEHEAD);

	INSTALLFILEHEAD fhead;
	memset(&fhead,0x00,sizeof(INSTALLFILEHEAD));
	dFileLen = (double)statbuf.st_size;
	fhead.dCurrentSize = dFileLen;

	char* pSBuffer = new char[HEADER_SIZE + headers.nDataCnt*headers.nDataSize ];
	memset(pSBuffer , 0x00 , HEADER_SIZE + headers.nDataCnt*headers.nDataSize );

	memcpy( pSBuffer, &headers, HEADER_SIZE);
	memcpy( pSBuffer + HEADER_SIZE , &fhead , headers.nDataCnt*headers.nDataSize );

	if(	SendData(Socket,pSBuffer,HEADER_SIZE + headers.nDataCnt*headers.nDataSize )<=0)  //struct _PACKET == PACKET
	{
		infLOG(ERROR, " ] ERROR FileRequestFile : send 에러 1: <client 죽음>\n");
		#ifdef __DEBUG
		printf(" ] FileRequestFile : send 에러 1: <client 죽음>");
		#endif

		delete[] pSBuffer;

		pSendData = new char[sizeof(ERR_HEADER)];
		memset(pSendData,0x00,sizeof(ERR_HEADER));
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_INSTALL_FILE_DN;
		strcat(errheader.errmsg,"서버측 에서 파일이 없거나 열수 없습니다.");

		memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

		pHeader->nCmd = RS_ERR;
		return -RS_INSTALL_FILE_DN;
	}
	if(pSBuffer)
		delete[] pSBuffer;

	//////////데이터 전송 루틴
	double nTotalLen = dFileLen;
	double nTempTotalLen = nTotalLen;
	nTotalLen = nTotalLen - dCurrentLen; //현재 이동한 만큼 계산

	int nReadLen=0;
	int nSendLen=0;
	int count=0;
	int nStop = 0;

	char* szSendBuffer = new char[SENDBUF];

	#ifdef __DEBUG
	printf(" ] Checking File (%s) : 파일 전체 길이 (%15.0f ) = ( %15.0f - %15.0f )",pFileHead->szFullFileName , nTotalLen ,dFileLen ,dCurrentLen);
	#endif

	int fno = fileno(UploadFile);

	while(nTotalLen > 0 && !nStop )
    {
		memset(szSendBuffer,0x00,SENDBUF);
    	nReadLen = read(fno,szSendBuffer,SENDBUF);

    	if(nReadLen > 0 )
        	nSendLen = SendData(Socket,szSendBuffer,nReadLen);
        else
    		nSendLen = 0;
        if(nSendLen <= 0)
        {
   		    if(nSendLen < 0)
        	{
        		char szErrMsg[1024];
				memset(szErrMsg,0x00,sizeof(szErrMsg));
				GetErrMsg(-nSendLen,szErrMsg);
				infLOG(ERROR, " ] send Error ( %d )( %s )\n",nSendLen,szErrMsg);

				#ifdef __DEBUG
				printf(" ] send Error ( %d )( %s )\n",nSendLen,szErrMsg);
				#endif
        	}
        	else if(nSendLen == 0)
        	{
        		infLOG(ERROR, " ] send Error ( 접속을 끊었습니다. )\n");

				#ifdef __DEBUG
				printf(" ] send Error ( 접속을 끊었습니다. )\n");
				#endif
        	}
			if(UploadFile)
				fclose(UploadFile);

			if(szSendBuffer)
				delete[] szSendBuffer;

			return 0;
        }
        nTotalLen = nTotalLen - (double)nSendLen;
    	nTotalnRecvLen = nTotalnRecvLen + (double)nSendLen;
    	count++;
    }

	if(UploadFile)
		fclose(UploadFile);
	if(szSendBuffer)
		delete[] szSendBuffer;
	char szTemp[768];
	memset(szTemp, 0x00, sizeof(szTemp));
	strcpy(szTemp, pFileHead->szFullFileName);

	infLOG(ALWAY, "] [%s] %s 업데이트 완료.\n", pHeader->szUserID, szTemp);

	return 1;
}

int GetFileList(char* path,int nCount,INSTALLFILEINFO* &pResult , LPCFDNLIST_R pUpdateList)
{

	for(int i=0; i<nCount; i++)
	{
		if(CheckFileName(pUpdateList[i].szFileName) == false)
		{
			return -1;
		}

		strcpy(pResult[i].server_file_path , path);
		strcpy(pResult[i].server_file_name , pUpdateList[i].szFileName);

		#ifdef __DEBUG
		printf("pUpdateList[%d].szFileName=[%s]\n", i, pUpdateList[i].szFileName);
		printf("pResult[%d].server_file_name=[%s]\n", i, pResult[i].server_file_name);
		#endif
	}

	return nCount;
}

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

bool CheckFileName(char *pFileName)
{
	int nFileLen = strlen(pFileName);
	for(int i=0; i< nFileLen; i++)
	{
		if(pFileName[i] == '.')
		{
			if(pFileName[i+1] == '.')
				return false;
		}
	}
	return true;
}