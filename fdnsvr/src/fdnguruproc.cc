#include "fdnsock.h"

#include "fdndefine.h"
#include "fdnguruproc.h"
#include "fdncomlib.h"

#include "apstruct.h" // HEADER
#include "comcomm.h"
#include "comconf.h"

#include "apdefine.h"
#include "comhead.h"


#include "fdns3004.h" // dcmdserver 의 dcmdfdns3004 와 데이터 동기화 유지 필요

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

#include "fdnuserqueue.h"
extern multimap<int,USERINFO>m_UserList;
extern char g_szOsp_id[10];
extern char g_szDcmdIP[16];
extern int  g_nDcmdPort; //  = 0;

extern char g_szSUB_DcmdIP[16];
extern int  g_nSUB_DcmdPort;
extern bool g_bConnect1 ;
extern bool g_bConnect2 ;


int GuruFileRequestList(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{

	infLOG(ALWAY, " ] 필수 자료실 파일 목록 조회 \n");

	LPHEADER pHeader = (LPHEADER)pRecvHead;
	ERR_HEADER errheader;
	memset(&errheader,0x00,sizeof(ERR_HEADER));

	HEADER headers;
	memset(&headers,0x00,sizeof(HEADER));

	LPCFDNLIST_R pRecv = (LPCFDNLIST_R)pRecvData;

	// 파일 목록 얻기 (폴더로 부터)

	CFDNS3004_R fdn3004;
	memset(&fdn3004,0x00,sizeof(CFDNS3004_R));

	/*
	char pErrMsg[256];
	memset(pErrMsg,0x00,256);
	*/
	if( fdns3004(pRecv->dwConID,&fdn3004,errheader.errmsg) < 0)
	{
		pSendData = new char[sizeof(ERR_HEADER)];
		memset(pSendData,0x00,sizeof(ERR_HEADER));
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_GURU_FILE_DN_LIST;
		//strcpy(errheader.errmsg,pErrMsg);

		memcpy(pSendData, &errheader, sizeof(ERR_HEADER));


		return -RS_GURU_FILE_DN_LIST;

	}

	if( strcmp(fdn3004.folder_yn , "N") == 0
	  ||strcmp(fdn3004.folder_yn , "n") == 0) //파일 이면
	{

		GURUFILEINFO Result;
		memset(&Result,0x00,sizeof(GURUFILEINFO));

		strcpyA(Result.local_file_name,fdn3004.lfile_name, sizeof(Result.local_file_name));
		strcpyA(Result.server_file_path,fdn3004.sfile_path, sizeof(Result.server_file_path));
		strcpyA(Result.server_file_name,fdn3004.sfile_name, sizeof(Result.server_file_name));
		memcpy(&Result.fdn3004 , &fdn3004,sizeof(CFDNS3004_R));

		memset(&headers,0x00,sizeof(HEADER));
		headers.nCmd = RS_GURU_FILE_DN_LIST;
		headers.nDataCnt = 1;
		headers.nDataSize = sizeof(GURUFILEINFO);
		headers.nErrorCode = 0;

		pSendData = new char[sizeof(HEADER) + headers.nDataCnt * headers.nDataSize];

		memset(pSendData,0x00,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize);
		memcpy(pSendData,&headers,sizeof(HEADER)); //head
		memcpy(pSendData+sizeof(HEADER),&Result,headers.nDataCnt * headers.nDataSize);




	}
	else
	{


		LPGURUFILEINFO pResult = new GURUFILEINFO[2000]; //2000개의 목록을 보낼수 있다.
		memset(pResult,0x00,sizeof(GURUFILEINFO)*2000);

		char szFullPath[612];
		memset(szFullPath,0x00,sizeof(szFullPath));


		// full path 만들기.
		strcpyA(szFullPath,fdn3004.sfile_path, sizeof(szFullPath));
		strcat(szFullPath,"/");
		strcat(szFullPath,fdn3004.sfile_name);



		if((headers.nDataCnt = GetGuruFileList(szFullPath,0,pResult,fdn3004.lfile_name , &fdn3004)) < 0) //파일 리스트 갯수를 얻는다.
		{

			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_GURU_FILE_DN_LIST;
			strcat(errheader.errmsg,"리스트를 작성에 실패 하였습니다.");

			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));
			delete[] pResult;

			infLOG(ALWAY, " ] 리스트를 작성에 실패 하였습니다. ( %s )",szFullPath);

			return -RS_GURU_FILE_DN_LIST;
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
			infLOG(ALWAY, " ] 파일 갯수 오류 : 0개의 파일이 있습니다. RS_EOL 전송. \n");

			return 1;
		}else if(headers.nDataCnt >= 2000)
		{
			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_FILE_REQUEST_LIST;
			strcat(errheader.errmsg,"한폴더 안에 2000개 이상의 목록을 작성 할수 없습니다.");

			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

			infLOG(ALWAY, " ] 한폴더 안에 2000개 이상의 목록을 작성 할수 없습니다.\n");

			delete[] pResult;


			return -RS_FILE_DATA_TRANSFER;

		}



		headers.nCmd = RS_GURU_FILE_DN_LIST;


		headers.nDataSize = sizeof(GURUFILEINFO);
		headers.nErrorCode = 0;

		pSendData = new char[sizeof(HEADER) + headers.nDataCnt * headers.nDataSize];

		memset(pSendData,0x00,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize);
		memcpy(pSendData,&headers,sizeof(HEADER)); //head
		memcpy(pSendData+sizeof(HEADER),pResult,headers.nDataCnt * headers.nDataSize);
		delete[] pResult;
	}


	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));
}



int GuruFileRequestFile(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{

	/////////////// 아이디 인증 작업 하기 //////////////////////
	// fileinfo를 얻기 위해 ...
	//////////////////////////////////////////////////////////

	LPHEADER pHeader = (LPHEADER)pRecvHead;

	ERR_HEADER errheader;
	memset(&errheader,0x00,sizeof(ERR_HEADER));

	HEADER headers;
	memset(&headers,0x00,sizeof(HEADER));

	LPGURUFILEHEAD pFileHead = (LPGURUFILEHEAD)pRecvData;



	#ifdef __DEBUG
	printf(" ] 필수 자료실 파일 전송 (%s)\n\n",pHeader->szUserID);
	#endif

	infLOG(ALWAY, " ] 필수 자료실 파일 전송 (%s)\n\n",pHeader->szUserID);



	//아이디 저장	save User ID
	multimap<int,USERINFO>::iterator mi;

	//mi = m_UserList.begin();
	mi = m_UserList.find(Socket);

	if(mi != m_UserList.end())
	{
		memcpy(mi->second.szUserID,pHeader->szUserID,min(12,(int)sizeof(pHeader->szUserID)));
	}



/*
	LPUSERINFO pUserinfo;
	pUserinfo = m_UserList.Find(Socket);
	if(pUserinfo != NULL)
	{
		memcpy(pUserinfo->szUserID,pHeader->szUserID,min(12,(int)sizeof(pHeader->szUserID)));
	}
*/

	#ifdef __DEBUG

	printf("  ./필수 자료실 파일 해더 확인  \n"
		   "    file name : %s\n"
		   "    file size : %15.0f\n"
		   "	sizeof	  : %d\n"
		   ,    pFileHead->szFullFileName
		   ,	pFileHead->dCurrentSize
		   ,	sizeof(GURUFILEHEAD));
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
			infLOG(ALWAY, " ] 파일 생성 실패 : %s \n",pFileHead->szFullFileName);

			#ifdef __DEBUG
			printf(" ] 파일 생성 실패 : %s \n",pFileHead->szFullFileName);
			#endif

			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_GURU_FILE_DN;
			strcat(errheader.errmsg,"서버측 에서 파일이 없거나 열수 없습니다.");

			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));


			pHeader->nCmd = RS_ERR;
			infLOG(ALWAY, " > ERROR Exception ) Make File Error ( UploadFile == NULL )  \n");
			return -RS_GURU_FILE_DN;

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
				infLOG(ALWAY, " ] 파일 생성 실패[ENOENT] : %s \n",pFileHead->szFullFileName);
				#ifdef __DEBUG
				printf(" ] 파일 생성 실패[ENOENT] : %s \n",pFileHead->szFullFileName);
				#endif



				pSendData = new char[sizeof(ERR_HEADER)];
				memset(pSendData,0x00,sizeof(ERR_HEADER));
				errheader.header.nCmd = RS_ERR;
				errheader.header.nErrorCode = -RS_GURU_FILE_DN;
				strcat(errheader.errmsg,"서버측 에서 파일이 없거나 열수 없습니다.");

				memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

				pHeader->nCmd = RS_ERR;


				return -RS_GURU_FILE_DN;


			}

			return 0;

		}




		// 현제 client에 있는 사이즈 만큼 이동 하기
		double dCurrentLen = pFileHead->dCurrentSize;

		off64_t current_set = (off64_t)dCurrentLen;

		fseeko64(UploadFile,current_set,SEEK_SET);
		 // 해더 전송  실제 사이즈와 이름.



		//데이터 전송

			/////////////////////////////


		memset(&headers,0x00,HEADER_SIZE);

		headers.nCmd = RS_GURU_FILE_DN;
		headers.nDataCnt = 1;
		headers.nDataSize = sizeof(GURUFILEHEAD);

		GURUFILEHEAD fhead;
		memset(&fhead,0x00,sizeof(GURUFILEHEAD));
		dFileLen = (double)statbuf.st_size;
		fhead.dCurrentSize = dFileLen;


		char* pSBuffer = new char[HEADER_SIZE + headers.nDataCnt*headers.nDataSize ];
		memset(pSBuffer , 0x00 , HEADER_SIZE + headers.nDataCnt*headers.nDataSize );

		memcpy( pSBuffer, &headers, HEADER_SIZE);
		memcpy( pSBuffer + HEADER_SIZE , &fhead , headers.nDataCnt*headers.nDataSize );

		if(	SendData(Socket,pSBuffer,HEADER_SIZE + headers.nDataCnt*headers.nDataSize )<=0)  //struct _PACKET == PACKET
		{

			infLOG(ALWAY, " ] ERROR FileRequestFile : send 에러 예취금 복원 1: <client 죽음>\n");
			#ifdef __DEBUG
			printf(" ] FileRequestFile : send 에러 예취금 복원 1: <client 죽음>");
			#endif

			delete[] pSBuffer;

			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_GURU_FILE_DN;
			strcat(errheader.errmsg,"서버측 에서 파일이 없거나 열수 없습니다.");

			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

			pHeader->nCmd = RS_ERR;
			return -RS_GURU_FILE_DN;

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



		////[][]
		#ifdef __DEBUG
		printf(" ] Checking File (%s) : 파일 전체 길이 (%15.0f ) = ( %15.0f - %15.0f )",pFileHead->szFullFileName , nTotalLen ,dFileLen ,dCurrentLen);
		#endif

		infLOG(ALWAY," ] Checking File (%s) : 파일 전체 길이 (%15.0f ) = ( %15.0f - %15.0f )",pFileHead->szFullFileName , nTotalLen ,dFileLen ,dCurrentLen);


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
					infLOG(ALWAY, " ] RecvSocket Error ( %d )( %s )\n",nSendLen,szErrMsg);

					#ifdef __DEBUG
					printf(" ] RecvSocket Error ( %d )( %s )\n",nSendLen,szErrMsg);
					#endif


	        	}
	        	else if(nSendLen == 0)
	        	{
	        		infLOG(ALWAY, " ] RecvSocket Error ( 접속을 끊었습니다. )\n");

					#ifdef __DEBUG
					printf(" ] RecvSocket Error ( 접속을 끊었습니다. )\n");
					#endif
	        	}


				//보내다가 오류가 났을시...DB처리

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

	return 1;


}



/*======================================================================
 int GetfirstCharIndex(char* srsString, char nChar)
 return   : false (-1) : true ( 0이상의 숫자)
 // srcString 에서 처음 nChar이 나타나는 곳의 자릿수를 return

 구조체 정의

typedef struct _FILEINFO
{
	int nTypeDisk; // 디스크 타입 (FG_MYDISK , FG_WEDISK)
	int nType; //파일타입 (FT_FOLDER or FT_FILE)
	int nNumber;//컨텐츠 번호
	int nMoneyType;
	long dwMoney; //돈 액수 // 메가 당, 건당
	long dFileSize; // 파일크기
	char szServerID[10];
	char szServerIP[16];
	long dwServerPort;
	char szUserID[12+1];
	char szFileOwnerID[12+1];
	char szSaveDate[100]; // 파일 날짜 .
	char szFileName[100]; // 파일이름
	char szSrcPath[512]; // 서버측 파일이 존재하는 폴더이름
	char szDownPath[512]; // 파일이 존재하는 폴더이름
}FILEINFO, *LPFILEINFO;

    ======================================================================*/

int GetGuruFileList(char* path,int nCount,GURUFILEINFO* &pResult,char* localname , CFDNS3004_R* pfdn3004)
{

 	DIR *pdir = NULL;

	pdir = opendir(path);//strPath);


	if(pdir == NULL)
	{
		infLOG(ALWAY, " ] GetGuruFileList Error ( pdir == NULL )\n");
		return -1;
	}



	struct dirent *pent;
	struct stat64 statbuf;


	char fullpath[612];


	while( (pent = readdir(pdir)) != NULL )
	{
		memset(fullpath,0x00,sizeof(fullpath));
		strcpyA(fullpath,path, sizeof(fullpath));//strPath);
		strcat(fullpath , "/");
		strcat(fullpath, pent->d_name);


		int stat= stat64(fullpath,&statbuf);

	 	if((strcmp(".",pent->d_name) !=0 ) && (strcmp("..",pent->d_name) != 0) && (GetFistCharIndex(pent->d_name,'.') != 0))
		{

			if(S_ISDIR (statbuf.st_mode))
			{

				nCount = GetGuruFileList( fullpath,nCount,pResult,localname,pfdn3004);
				if(nCount == -1)
				{
					infLOG(ALWAY, " ] GetGuruFileList Error ( nCount == -1 )\n");
					closedir(pdir);
					return -1;
				}
			}
			else
			{
				if(S_ISREG (statbuf.st_mode))
				{

					strcpyA(pResult[nCount].server_file_path , path, sizeof(pResult[nCount].server_file_path));
					strcpyA(pResult[nCount].server_file_name , pent->d_name, sizeof(pResult[nCount].server_file_name));
					strcpyA(pResult[nCount].local_file_name  , localname, sizeof(pResult[nCount].local_file_name));
					memcpy(&pResult[nCount].fdn3004 , pfdn3004 ,sizeof(CFDNS3004_R));

					#ifdef __DEBUG
					printf("\n ] sPath (%s)\n",pResult[nCount].server_file_path);
					printf(" ] sname (%s)\n",pResult[nCount].server_file_name);
					printf(" ] lname (%s)\n",pResult[nCount].local_file_name);
					#endif

					nCount++;
				}
				else
				{
					infLOG(ALWAY, " ] GetGuruFileList Error ( S_ISREG (statbuf.st_mode))\n");


					if(stat != 0 )
						nCount = -1;
					//return -1;
					closedir(pdir);

					return -1;
				}
			}
		}
	}


	closedir(pdir);

	return nCount;
}
