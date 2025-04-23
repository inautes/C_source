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

#include "fdndefine.h"
#include "fdnsock.h" //sock send recv
#include "fdnproc.h"
#include "fdncomlib.h"
#include "fdndownproc.h"
#include "fdncomproc.h"
#include "apstruct.h"
#include "comhead.h"
#include "apdefine.h" //for log
#include "comcomm.h" //for log
#include "comconf.h"
#include "fdnsock.h"
#include "com9005.h"
#include "com9006.h"
#include "cmd5.h"



//#include <sys/mman.h>
extern multimap<int,USERINFO>m_UserList;
extern char g_szOsp_id[10];
extern char g_szDcmdIP[16];
extern int  g_nDcmdPort; //  = 0;

extern char g_szSUB_DcmdIP[16];
extern int  g_nSUB_DcmdPort;
extern bool g_bConnect1 ;
extern bool g_bConnect2 ;

long Send_com9005(char* pUserID , char* pData ,char* pErrMsg)
{

	long nRet=-1;

	if(g_bConnect1 == true)
	{
		nRet = com9005 ( pUserID,pData,pErrMsg,g_szDcmdIP,g_nDcmdPort);
	}
	else
	{	// 1번 안되면 2번 무조건 접속.
		nRet = com9005 ( pUserID,pData,pErrMsg,g_szSUB_DcmdIP,g_nSUB_DcmdPort);
	}
	return nRet;

}

long Send_com9002 (CCOM9002_R pcom9002_r,int nFlag)
{
	long nRet=-1;

	if(g_bConnect1 == true)
	{
		nRet = com9002 ( pcom9002_r,nFlag,g_szDcmdIP,g_nDcmdPort);
	}
	else
	{	// 1번 안되면 2번 무조건 접속.
		nRet = com9002 ( pcom9002_r,nFlag,g_szSUB_DcmdIP,g_nSUB_DcmdPort);
	}

	return nRet;
}



// RS_FILE_REQUEST_NEXT_FILE =================
int FileRequestNextFile(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{

	if( pRecvHead == NULL )
		infLOG(ERROR, "pRecvHead == NULL\n");


	HEADER headers;
	memset(&headers,0x00,HEADER_SIZE);

	pSendData = new char[sizeof(HEADER)];
	memset(pSendData,0x00,sizeof(HEADER));

	headers.nCmd = RS_FILE_REQUEST_NEXT_FILEINFO; //파일 정보 요청
	headers.nDataCnt = 0;
	headers.nDataSize = 0;

	memcpy(pSendData,&headers,sizeof(HEADER));

	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));//return 1;

}

// RS_GRID_DATA_TRANSFER_NEXT
int FileRequestGridTransferNext(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	if( pRecvHead == NULL )
		infLOG(ERROR, "pRecvHead == NULL\n");


	HEADER headers;
	memset(&headers,0x00,HEADER_SIZE);

	pSendData = new char[sizeof(HEADER)];
	memset(pSendData,0x00,sizeof(HEADER));

	headers.nCmd = RS_GRID_DATA_TRANSFER_NEXT; //파일 정보 요청
	headers.nDataCnt = 0;
	headers.nDataSize = 0;

	memcpy(pSendData,&headers,sizeof(HEADER));

	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));//return 1;

}


// RS_GRID_FAIL
int FileRequestGridFail(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	if( pRecvHead == NULL )
		infLOG(ERROR, "pRecvHead == NULL\n");

	HEADER headers;
	memset(&headers,0x00,HEADER_SIZE);

	pSendData = new char[sizeof(HEADER)];
	memset(pSendData,0x00,sizeof(HEADER));

	headers.nCmd = RS_GRID_FAIL; //파일 정보 요청
	headers.nDataCnt = 0;
	headers.nDataSize = 0;

	memcpy(pSendData,&headers,sizeof(HEADER));

	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));//return 1;
}


// RS_GRID_KEEPALIVE_CHECK
int FileRequestGridKeepAliveCheck(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{

	if( pRecvHead == NULL )
		infLOG(ERROR, "pRecvHead == NULL\n");

	HEADER headers;
	memset(&headers,0x00,HEADER_SIZE);

	pSendData = new char[sizeof(HEADER)];
	memset(pSendData,0x00,sizeof(HEADER));

	headers.nCmd = RS_GRID_KEEPALIVE_CHECK; //파일 정보 요청
	headers.nDataCnt = 0;
	headers.nDataSize = 0;

	memcpy(pSendData,&headers,sizeof(HEADER));


	multimap<int,USERINFO>::iterator mi; //IP 조회
	//mi = m_UserList.begin();
	mi = m_UserList.find(Socket);
	if(mi != m_UserList.end())
	{
		mi->second.nRealDown = 0;
	}

	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));//return 1;
}



int RequesetHoldTime(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{

	if( pRecvHead == NULL )
		infLOG(ERROR, "pRecvHead == NULL\n");
	if( pRecvData == NULL )
		infLOG(ERROR, "pRecvData == NULL\n");
	HEADER headers;
	memset(&headers,0x00,HEADER_SIZE);

	ERR_HEADER errheader;
	memset(&errheader,0x00,sizeof(ERR_HEADER));


	HOLDTIME hold_time;

	headers.nCmd = RS_FDN_REQUEST_HOLD_TIME; //파일 정보 요청
	headers.nDataCnt = 1;
	headers.nDataSize = sizeof(HOLDTIME);
	headers.nErrorCode = 0;

	//정액제 현재 시간 보내기


//	if( com9006(pRecvData ,(char*)&hold_time,errheader.errmsg) < 0 )


	LPFILEINFO pFileinfo = (LPFILEINFO)pRecvData;

	int nRet=-1;
	if(g_bConnect1 == true)
	{
		nRet = com9006(pRecvData ,(char*)&hold_time,errheader.errmsg,g_szDcmdIP,g_nDcmdPort);
	}
	else
	{	// 1번 안되면 2번 무조건 접속.
		nRet = com9006(pRecvData ,(char*)&hold_time,errheader.errmsg,g_szSUB_DcmdIP,g_nSUB_DcmdPort);
	}


	if( nRet < 0 )
	{
		pSendData = new char[sizeof(ERR_HEADER)];
		memset(pSendData,0x00,sizeof(ERR_HEADER));
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
		//strcat(errheader.errmsg,szErrMsg);

		memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

		return -RS_FDN_REQUEST_HOLD_TIME;

	}

	pSendData = new char[sizeof(HEADER) + headers.nDataCnt * headers.nDataSize];

	memset(pSendData,0x00,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize);
	memcpy(pSendData,&headers,sizeof(HEADER)); //head
	memcpy(pSendData+sizeof(HEADER),(char*)&hold_time,headers.nDataCnt * headers.nDataSize);

	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));//return 1;
}


int GetFileList(char* path,int nCount,FILEINFO* &pResult,char* pRecvData,char* pSrcPath)
{
	DIR *pdir = NULL;


	pdir = opendir(path);//strPath);

	if(pdir == NULL)
	{
		infLOG(ALWAY, " ] GetFileList Error ( pdir == NULL )\n");
		return -1;
	}



	struct dirent *pent;
	struct stat64 statbuf;


	char fullpath[1024];


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

				nCount = GetFileList( fullpath,nCount,pResult,pRecvData,pSrcPath);
				if(nCount == -1)
				{
                    infLOG(ALWAY, " ] GetFileList Error ( nCount == -1 )\n");
					closedir(pdir);
					return -1;
				}
			}
			else
			{
				if(S_ISREG (statbuf.st_mode))
				{


					memcpy(&pResult[nCount],pRecvData,sizeof(FILEINFO));

					long len = strlen(path);
					long src_len = strlen(pSrcPath);


					if(len > src_len)
					{
						char szAddPath[512];
						memset(szAddPath,0x00,sizeof(char)*512);

						if(	GetRightString(path,len - src_len,szAddPath))
						{

							strcat(pResult[nCount].szDownPath,"\\");

							char szResult[512];
							memset(&szResult,0x00,sizeof(szResult));

							SetWordReplace(szAddPath,pResult[nCount].szFileName,pResult[nCount].szDownFileName,szResult);

							strcat(pResult[nCount].szDownPath,szResult);
							StrReplace(pResult[nCount].szDownPath,"/","\\");

						}

					}


					pResult[nCount].nType = FT_FOLDER; //종류
					strcpyA(pResult[nCount].szFileName ,pent->d_name, sizeof(pResult[nCount].szFileName)); // 이름
					pResult[nCount].dFileSize = (double)statbuf.st_size; // 크기
				//	strcpyA(pResult[nCount].szSaveDate , ctime(&statbuf.st_ctime)); //수정날짜
					strcpyA(pResult[nCount].szSrcPath , path, sizeof(pResult[nCount].szSrcPath));


					//그리드 시작
					char szFullPath[1024];
					memset(szFullPath,0x00,sizeof(szFullPath));

					// full path 만들기.
					strcpyA(szFullPath,path, sizeof(szFullPath));
					strcat(szFullPath,"/");
					strcat(szFullPath,pent->d_name);

					CMD5 md5;
			        char* pResultHash = md5.GetHashFromFile(szFullPath);
			        if( pResultHash != NULL)
			        {
			        	strcat(pResult[nCount].szSrcPath,"|");
			        	strcat(pResult[nCount].szSrcPath,pResultHash);
			        	strcat(pResult[nCount].szSrcPath,"|");
			        }
			        	//strcat(pResult[nCount].szFileName,pResultHash);
			        //그리드 끝


					LPFILEINFO pFile = (LPFILEINFO)pRecvData;
					//memcpy(&pResult[nCount].GroupInfo,&pFile->GroupInfo,sizeof(FILEGROUPINFO));

					nCount++;
				}
				else
				{


					if(stat != 0 )
						nCount = -1;
					//return -1;
					closedir(pdir);
                    infLOG(ALWAY, " ] GetFileList Error ( S_ISREG (statbuf.st_mode) )\n");
					return -1;
				}

				//change
				if(nCount >= 2000)
				{
					nCount = 2000;
					break;
				}
			}
		}
	}


	closedir(pdir);

	return nCount;
}


int FileRequestList(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{

	LPHEADER pHeader = (LPHEADER)pRecvHead;
	ERR_HEADER errheader;
	memset(&errheader,0x00,sizeof(ERR_HEADER));

	HEADER headers;
	memset(&headers,0x00,sizeof(HEADER));

	if( Send_com9005(pHeader->szUserID,pRecvData, errheader.errmsg) < 0)
	{
		//예외처리

		pSendData = new char[sizeof(ERR_HEADER)];
		memset(pSendData,0x00,sizeof(ERR_HEADER));
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_FILE_REQUEST_LIST;
		//strcat(errheader.errmsg,szErrMsg);

		memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

		return -RS_FILE_DATA_TRANSFER;
	}

	LPFILEINFO pRecv = (LPFILEINFO)pRecvData;

	// 파일 목록 얻기 (폴더로 부터)
	#ifdef __DEBUG
	printf("nTypeDisk   	%d\n",pRecv->nTypeDisk	);
	printf("nType			%d\n",pRecv->nType		);
	printf("nDealType   	%d\n",pRecv->nDealType	);
	printf("DealID	    	%d\n",pRecv->dwDealID	);
	printf("szServerID   	%s\n",pRecv->szServerID);
	printf("szServerIP   	%s\n",pRecv->szServerIP);
	printf("dwServerPort   	%d\n",pRecv->dwServerPort);
	printf("nNumber			%ld\n",pRecv->nNumber);
	printf("szUserID      	%s\n", pRecv->szUserID     );
	printf("szFileOwnerID 	%s\n", pRecv->szFileOwnerID);
	printf("szDownFileName	%s\n", pRecv->szDownFileName);
	printf("szFileName    	%s\n", pRecv->szFileName   );
	printf("szSrcPath     	%s\n", pRecv->szSrcPath    );
	printf("szDownPath    	%s\n", pRecv->szDownPath   );
	printf("dFileSize     	%15.0f\n",pRecv->dFileSize);
	printf("szCerKey     	%s\n",pRecv->szCerKey);
	#endif
	//szSrcPath : 그리드를 위해 끝자리 32자리를 해쉬 값 전송으로 사용함.
	//md5 가져오기

	if( pRecv->nType == FT_FILE)
	{

		// 그리드를 위해 md5 생성
		//그리드 시작
		char szFullPath[1024];
		memset(szFullPath,0x00,sizeof(szFullPath));

		// full path 만들기.
		strcpyA(szFullPath,pRecv->szSrcPath, sizeof(szFullPath));
		strcat(szFullPath,"/");
		strcat(szFullPath,pRecv->szFileName);

		#ifdef __DEBUG
		printf("szFullPath [ %s ] \n",szFullPath);
		#endif

		CMD5 md5;
        char* pResult = md5.GetHashFromFile(szFullPath);

        if( pResult != NULL)
        {
        	#ifdef __DEBUG
        	printf("> FT_FILE hash [ %s ] \n",pResult);
        	#endif
        	strcat(pRecv->szSrcPath,"|");
        	strcat(pRecv->szSrcPath,pResult);
        	strcat(pRecv->szSrcPath,"|");
        }
        	//strcat(pRecv->szFileName,pResult);


		headers.nCmd = RS_FILE_REQUEST_LIST;

		headers.nDataCnt = 1;
		headers.nDataSize = sizeof(FILEINFO);
		headers.nErrorCode = 0;

		pSendData = new char[sizeof(HEADER) + headers.nDataCnt * headers.nDataSize];

		memset(pSendData,0x00,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize);
		memcpy(pSendData,&headers,sizeof(HEADER)); //head
		memcpy(pSendData+sizeof(HEADER),pRecv,headers.nDataCnt * headers.nDataSize);

	}
	else
	{
		LPFILEINFO pResult = new FILEINFO[2000]; //2000개의 목록을 보낼수 있다.
		memset(pResult,0x00,sizeof(FILEINFO)*2000);

		char szFullPath[1024];
		memset(szFullPath,0x00,sizeof(szFullPath));

		// full path 만들기.
		strcpyA(szFullPath,pRecv->szSrcPath, sizeof(szFullPath));
		strcat(szFullPath,"/");
		strcat(szFullPath,pRecv->szFileName);

		headers.nDataCnt = GetFileList(szFullPath,0,pResult,pRecvData,pRecv->szSrcPath);
		if( headers.nDataCnt< 0 ) //파일 리스트 갯수를 얻는다.
		{

			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_FILE_REQUEST_LIST;
			strcat(errheader.errmsg,"서버측 에서 파일이 없거나 열수 없습니다.");

			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

			infLOG(ALWAY, " ]FileRequestList: 서버 파일 없음 (%s)", szFullPath);


			delete[] pResult;



			return -RS_FILE_DATA_TRANSFER;
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
			return 1;
		}
		else if(headers.nDataCnt >= 2000)
		{
			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_FILE_REQUEST_LIST;
			strcat(errheader.errmsg,"한폴더 안에 2000개 이상의 목록을 작성 할수 없습니다..");

			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

			infLOG(ERROR, " ]FileRequestList: 목록 초과 작성 (%d)", headers.nDataCnt);



			delete[] pResult;



			return -RS_FILE_REQUEST_LIST;

		}



		headers.nCmd = RS_FILE_REQUEST_LIST;


		headers.nDataSize = sizeof(FILEINFO);
		headers.nErrorCode = 0;

		pSendData = new char[sizeof(HEADER) + headers.nDataCnt * headers.nDataSize];

		memset(pSendData,0x00,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize);
		memcpy(pSendData,&headers,sizeof(HEADER)); //head
		memcpy(pSendData+sizeof(HEADER),pResult,headers.nDataCnt * headers.nDataSize);
		delete[] pResult;
	}

	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));
}

// RS_FILE_REQUEST_FILE_WITH_HOLD_TIME_GRID, RS_FILE_REQUEST_FILE_WITH_HOLD_TIME //정액제 처음 타는 부분
// RS_FILE_REQUEST_FILE ==========
// RS_FILE_REQUEST_FILE_GRID
int FileRequestFile(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	printf("FileRequestFile\n");
	LPHEADER pHeader = (LPHEADER)pRecvHead;

	ERR_HEADER errheader;
	memset(&errheader,0x00,sizeof(ERR_HEADER));

	HEADER headers;
	memset(&headers,0x00,sizeof(HEADER));

	LPFILEINFO pFileinfo = (LPFILEINFO)pRecvData;

	multimap<int,USERINFO>::iterator mi; //IP 조회
	mi = m_UserList.find(Socket);
	if(mi != m_UserList.end())
	{

		if(mi->second.gCom9xxx.com9002_R.temp_id != pFileinfo->nNumber || mi->second.gCom9xxx.nStatus == 0 )
		{
			//DB 업데이트 , 새로운 컨텐츠의 시작

			strcpyA(mi->second.gCom9xxx.com9002_R.server_id , pFileinfo->szServerID);

			strcpyA(mi->second.gCom9xxx.com9102_R.server_id , pFileinfo->szServerID);
			mi->second.gCom9xxx.com9002_R.temp_id = pFileinfo->nNumber;

			mi->second.gCom9xxx.com9102_R.temp_id = pFileinfo->nNumber;

			if(pFileinfo->nTypeDisk == 0x003)//컨텐츠구분(WE:위디스크(0x004), MY:공개자료실(0x003), MD:내자료실(0x005))
			{
				strcpyA(mi->second.gCom9xxx.com9002_R.cont_gu,"FD");

				strcpyA(mi->second.gCom9xxx.com9102_R.cont_gu,"FD");
			}
			else if(pFileinfo->nTypeDisk == 0x005)
			{
				strcpyA(mi->second.gCom9xxx.com9002_R.cont_gu,"MD");

				strcpyA(mi->second.gCom9xxx.com9102_R.cont_gu,"MD");
			}
			else if(pFileinfo->nTypeDisk == 0x004)
			{
				strcpyA(mi->second.gCom9xxx.com9002_R.cont_gu,"WE");

				strcpyA(mi->second.gCom9xxx.com9102_R.cont_gu,"WE");
			}
			else //exception
			{
				strcpyA(mi->second.gCom9xxx.com9002_R.cont_gu,"ER");

				strcpyA(mi->second.gCom9xxx.com9102_R.cont_gu,"ER");
			}

			if(mi->second.gCom9xxx.nStatus == 0)
			{
				Send_com9002(mi->second.gCom9xxx.com9002_R,1); //서버 사용자수 증가
			}
			else
			{
				Send_com9002(mi->second.gCom9xxx.com9002_R,0); //사용자 수정
			}

			mi->second.gCom9xxx.nStatus = 1;

			/*
			if(mi->second.temp_id != pFileinfo->nNumber )
			{
				//DB 업데이트 , 새로운 컨텐츠의 시작
				strcpyA(mi->second.server_id , pFileinfo->szServerID);
				mi->second.temp_id = pFileinfo->nNumber;
				mi->second.nRealDown = 0; // 초기화
				SendRealCount(0,pFileinfo->nNumber);
			}
			*/
		}

	}

	long seq_no; // 예취 ..돈계산을 위한 시퀀스 번호
	int nError;// 에러 정보 저장
//	CFDNS3003_R cfdn; //사이버 머니 계산

	#ifdef __DEBUG
	printf(" ] 파일 정보 확인 \n"
		   "    file name 		: %s\n"
		   "	file size 		: %15.0f\n"
		   "	file path 		: %s\n"
		   "	file DealID     : %d\n"
		   "	file deal_type  : %d\n"
		   "	file owner id   : %s\n"
		   "	file szSrcPath 	: %s\n"
		   ,pFileinfo->szFileName
		   ,pFileinfo->dFileSize
		   ,pFileinfo->szDownPath
		   ,pFileinfo->dwDealID
		   ,pFileinfo->nDealType
		   ,pFileinfo->szFileOwnerID
		   ,pFileinfo->szSrcPath);
	#endif



	memset(&headers,0x00,HEADER_SIZE);
	headers.nCmd = RS_FILE_REQUEST_FILE; //파일 요청

	if( pHeader->nCmd == RS_FILE_REQUEST_FILE_GRID )
	{
		headers.nCmd = RS_FILE_REQUEST_FILE_GRID; //파일 정보 요청
		#ifdef __DEBUG
		printf("RS_FILE_REQUEST_FILE_GRID (%d)\n", headers.nCmd);
		#endif


	}
	else
	{
		headers.nCmd = RS_FILE_REQUEST_FILE; //파일 정보 요청
		#ifdef __DEBUG
		printf("RS_FILE_REQUEST_FILE (%d)\n", headers.nCmd);
		#endif

	}

	headers.nDataCnt = 0;
	headers.nDataSize = 0;

	if(SendData(Socket,(char*)&headers,HEADER_SIZE)<0)  //struct _PACKET == PACKET
	{
		infLOG(ALWAY, " ] SendData Error(RS_FILE_REQUEST_FILE)\n");
		#ifdef __DEBUG
		printf(" ] SendData Error(RS_FILE_REQUEST_FILE)\n");
		#endif
		//com9102(mi->second.gCom9xxx.com9102_R);
		return 0;
	}



	memset(&headers,0x00,sizeof(HEADER));


	if(RecvData(Socket,(char*)&headers,HEADER_SIZE ) < 0)
	{
		infLOG(ERROR, " ]FileRequestFile: RecvData head Error\n");
		return 0;
	}

	if(headers.nCmd == RS_EOL)
	{
		memset(&headers,0x00,HEADER_SIZE);

		pSendData = new char[sizeof(HEADER)];
		memset(pSendData,0x00,sizeof(HEADER));

		headers.nCmd = RS_EOL;
		headers.nDataCnt = 0;
		headers.nDataSize = 0;

		memcpy(pSendData,&headers,sizeof(HEADER));

		infLOG(ERROR, " ]FileRequestFile: 오류 headers.nCmd == RS_EOL (return END)\n");

		//com9102(mi->second.gCom9xxx.com9102_R);
		return END;
	}
	else if (headers.nCmd == RS_FILE_REQUEST_NEXT_FILE)
	{
		pHeader->nCmd = RS_FILE_REQUEST_NEXT_FILEINFO; // send 하게 변경

		infLOG(ALWAY, " ]FileRequestFile: 다음 파일 요청 \n");


		memset(&headers,0x00,HEADER_SIZE);

		pSendData = new char[sizeof(HEADER)];
		memset(pSendData,0x00,sizeof(HEADER));

		headers.nCmd = RS_FILE_REQUEST_NEXT_FILEINFO; //파일 정보 요청
		headers.nDataCnt = 0;
		headers.nDataSize = 0;

		memcpy(pSendData,&headers,sizeof(HEADER));

		return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));//return 1;

	}
	else if(headers.nCmd == RS_GRID_DATA_TRANSFER) //그리드 시작
	{

		infLOG(ALWAY, " ]FileRequestFile: 그리드 파일 처리 RS_GRID_DATA_TRANSFER \n");
		memset(&headers,0x00,HEADER_SIZE);

		pSendData = new char[sizeof(HEADER)];
		memset(pSendData,0x00,sizeof(HEADER));

		pHeader->nCmd = RS_GRID_DATA_TRANSFER; // send 하게 변경


		headers.nCmd = RS_GRID_DATA_TRANSFER; //파일 정보 요청
		headers.nDataCnt = 0;
		headers.nDataSize = 0;

		memcpy(pSendData,&headers,sizeof(HEADER));

		return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));//return 1;
	} //그리드 끝

	//예외 상황 강화 테스트 해야 함

	LPFILEHEAD pFileHead = new FILEHEAD;  //head 전송
	memset(pFileHead,0x00,sizeof(FILEHEAD));

	#ifdef __DEBUG
	printf("FILEHEAD\n");
	#endif


	infLOG(ALWAY, " ]FileRequestFile: 파일 전송 준비 - 해더 수신 \n");

	if(RecvData(Socket,(char*)pFileHead,sizeof(FILEHEAD))<0)
	{
		infLOG(ERROR, " ]FileRequestFile: 파일 해더 수신 오류\n");
		delete pFileHead;
		return 0;
	}

	#ifdef __DEBUG
	printf("	파일 해더 채크  \n"
		   "    file name : %s"
		   "    file size : %.0f"
		   "	sizeof	  : %d\n"
		   ,    pFileHead->szFullFileName
		   ,	pFileHead->dCurrentSize
		   ,	sizeof(FILEHEAD));
	#endif

	infLOG(ALWAY, "	]FileRequestFile: 파일 해더 채크  \n"
				  "    file name : %s\n"
				  "    current get file size : %.0f\n"
				  ,    pFileHead->szFullFileName
				  ,	   pFileHead->dCurrentSize );


	FILE* UploadFile;
	UploadFile = NULL;

	double dFileLen = 0;


	double nTotalnRecvLen = 0;

		//////////데이터 전송 루틴
	//그리드로 인한 수정
	char szFileFullName[1024];
	memset(szFileFullName,0x00,sizeof(szFileFullName));
	//memcpy(szFileFullName,pFileHead->szFullFileName , strlen(pFileHead->szFullFileName)-32);

	char* pFindStr = strchr(pFileinfo->szSrcPath,'|');

	if( pFindStr)
	{
		memcpy(szFileFullName,pFileinfo->szSrcPath ,strlen( pFileinfo->szSrcPath) - strlen(pFindStr) );
		strcat(szFileFullName,"/");
		strcat(szFileFullName,pFileinfo->szFileName);
	}
	else
		strcpy(szFileFullName,pFileHead->szFullFileName);

	// file open
	infLOG(ALWAY, " mi->second.nRealDown =	 %d , pFileinfo->nNumber = %ld,pFileHead->dCurrentSize : %.0f \n"
	,mi->second.nRealDown
	,pFileinfo->nNumber
	,pFileHead->dCurrentSize);

	if(pFileHead->dCurrentSize == 0)
	{
		// 필수
		infLOG(ALWAY, " X1\n");
		mi->second.nRealDown = 1;
		SendRealCount(1,pFileinfo->nNumber); // 카운트 올리기, 서버에서 처음부터 다운로드 시작.
	}
	else
	{
		if(mi->second.nRealDown == 0)
		{
			infLOG(ALWAY, " X2\n");
			mi->second.nRealDown = 1;
			SendRealCount(1,pFileinfo->nNumber);
		}
		else
		{
			infLOG(ALWAY, " X3\n");
			// 접속 카운트만 올리기
			SendRealCount(1,pFileinfo->nNumber);
		}
	}

	UploadFile = fopen64(szFileFullName,"rb");

	if(UploadFile == NULL)
	{
		infLOG(ERROR, " ]FileRequestFile: UploadFile == NULL 파일 없음. (%s)\n",pFileHead->szFullFileName);

		#ifdef __DEBUG
		printf(" ] FileRequestFile : UploadFile == NULL: <client 죽음>\n");
		printf(" ] file open error : %s \n",pFileHead->szFullFileName);
		#endif

		pSendData = new char[sizeof(ERR_HEADER)];
		memset(pSendData,0x00,sizeof(ERR_HEADER));
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
		strcat(errheader.errmsg,"서버측 에서 파일이 없거나 열수 없습니다.");

		memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

		delete pFileHead;

		pHeader->nCmd = RS_ERR;
		return -RS_FILE_DATA_TRANSFER;

	}
	#ifdef __DEBUG
	printf("Open File [ %s ]\n",szFileFullName);
	#endif

	//set file head , 전체 사이즈 , 파일 이름
	struct stat64 statbuf;

	//그리드로 인한 수정
	int stat = stat64(szFileFullName,&statbuf);
	//int stat = stat64(pFileHead->szFullFileName,&statbuf);


	if(stat != 0) //error
	{
		if(pFileHead != NULL)
			delete pFileHead;

		if(errno == ENOENT) //파일이나 패스가 없음
		{
			infLOG(ERROR, " ]FileRequestFile: 파일 오류  errno == ENOENT ( %s ) \n",pFileHead->szFullFileName);

			#ifdef __DEBUG
			printf("Open File ERROR [ %s ]\n",szFileFullName);
			#endif

			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
			strcat(errheader.errmsg,"서버측 에서 파일이 없거나 열수 없습니다.");

			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

			pHeader->nCmd = RS_ERR;
			return -RS_FILE_DATA_TRANSFER;
		}
		return 0;
	}

	// 현제 client에 있는 사이즈 만큼 이동 하기
//	unsigned long dCurrentLen = pFileHead->dCurrentSize;
	double dCurrentLen = pFileHead->dCurrentSize;

	off64_t current_set = (off64_t)dCurrentLen;

	fseeko64(UploadFile,current_set,SEEK_SET);

	 // 해더 전송  실제 사이즈와 이름.

///////////////////////////////////////////////////////////////////////
		 	//예취 전환.  복
	char DnDate[9];
	memset(DnDate,0x00,9);

	memset(pFileHead,0x00,sizeof(FILEHEAD));

	dFileLen  = (double)statbuf.st_size;
	if(dFileLen < 0 )
		dFileLen = 0;


	pFileHead->dwID = seq_no;
/*
	if(pFileinfo->GroupInfo.nMode == 3 || pFileinfo->GroupInfo.nMode == 1)
	{
		pFileinfo->GroupInfo.dwID = seq_no;
		strcpyA(pFileinfo->GroupInfo.DnDate,DnDate);
	}
*/
	strcpyA(pFileHead->DnDate,DnDate);

	pFileHead->dCurrentSize = dFileLen;
	strcpyA(pFileHead->szFullFileName ,pFileinfo->szFileName);

	#ifdef __DEBUG
	printf("	파일 확인\n"
		   "    file seq = %lu : file name : %s  : dn date = %s :"
		   "    file size : %.0f\n"
		   ,	pFileHead->dwID
		   ,    pFileHead->szFullFileName
		   ,	DnDate
		   ,	pFileHead->dCurrentSize);
	#endif

	infLOG(ALWAY, " ]FileRequestFile: 파일 확인\n"
				  "   file seq = %lu : file name : %s  : dn date = %s \n"
				  "   file size : %.0f\n"
				,	pFileHead->dwID
				,    pFileHead->szFullFileName
				,	DnDate
				,	pFileHead->dCurrentSize);





		//데이터 전송

			/////////////////////////////

		memset(&headers,0x00,HEADER_SIZE);

		headers.nCmd = RS_FILE_DATA_TRANSFER;
		headers.nDataCnt = 1;
		headers.nDataSize = sizeof(FILEHEAD);


		char* pSendBuffer = new char[HEADER_SIZE + headers.nDataCnt*headers.nDataSize ];
		memset(pSendBuffer, 0x00, HEADER_SIZE + headers.nDataCnt*headers.nDataSize );

		memcpy(pSendBuffer,&headers,sizeof(HEADER)); //head
		memcpy(pSendBuffer + HEADER_SIZE,pFileHead,headers.nDataCnt * headers.nDataSize); //body



		infLOG(ALWAY, " ]FileRequestFile: 파일 해더 송신\n");
		if(	SendData(Socket,pSendBuffer,HEADER_SIZE + headers.nDataCnt*headers.nDataSize)<=0)  //struct _PACKET == PACKET
		{
			infLOG(ALWAY, " ]FileRequestFile: 파일 해더 송신 오류\n");

			#ifdef __DEBUG
			printf("SEND RS_FILE_DATA_TRANSFER ERROR\n");
			#endif


			delete[] pSendBuffer;
			delete pFileHead;

			//com9102(mi->second.gCom9xxx.com9102_R);
			return 0;

		}

		delete[] pSendBuffer;
		delete pFileHead;


		//////////데이터 전송 루틴
		double dTotalLen = dFileLen;
		double dTempTotalLen = dTotalLen;
		dTotalLen = dTotalLen - dCurrentLen; //현재 이동한 만큼 계산

		int nReadLen=0;
		int nSendLen=0;

		int nStop = 0;



//		char* szSendBuffer = new char[SENDBUF];
		char szSendBuffer[SENDBUF];



		////[][]


		#ifdef __DEBUG
		printf("Checking File (%s) : file size (%.0f ) = ( %.0f - %.0f )\n",pFileinfo->szFileName , dTotalLen ,pFileinfo->dFileSize ,dCurrentLen);
		#endif

		infLOG(ALWAY," ]FileRequestFile: (%lu)"
					 "Checking File (%s) : file size (%.0f ) = ( %.0f - %.0f )\n"
					 , pFileinfo->dwDealID
					 ,pFileinfo->szFileName , dTotalLen ,pFileinfo->dFileSize ,dCurrentLen);

		int fno = fileno(UploadFile);


		//off_t offset = 0;


		while(dTotalLen > 0 && !nStop )
    	{

			memset(szSendBuffer,0x00,SENDBUF);
			nReadLen = read(fno,szSendBuffer,SENDBUF);

	    	if(nReadLen > 0 )
	    		nSendLen = SendData(Socket,szSendBuffer,nReadLen);
	    	else
	    		nSendLen = 0;

	        if(nSendLen <= 0)
	        {
	        	#ifdef __DEBUG
	        	printf("-=----------- 예외 상황 파일 받기 ------11\n");
	        	#endif

	        	if(nSendLen < 0)
	        	{
	        		char szErrMsg[1024];
					memset(szErrMsg,0x00,sizeof(szErrMsg));
					GetErrMsg(-nSendLen,szErrMsg);
					infLOG(ERROR, " ]FileRequestFile: SendSocket Error ( %d ) ( %d )( %s )\n",nReadLen ,nSendLen,szErrMsg);

					#ifdef __DEBUG
					printf(" ] SendSocket Error ( %d ) ( %d )( %s )\n",nReadLen ,nSendLen,szErrMsg);
					#endif


	        	}
	        	else if(nSendLen == 0)
	        	{
	        		infLOG(ERROR, " ]FileRequestFile: SendSocket Error ( %d ) ( 접속을 끊었습니다. )\n",nReadLen );

					#ifdef __DEBUG
					printf(" ] SendSocket Error ( %d ) ( 접속을 끊었습니다. )\n",nReadLen );
					#endif
	        	}

	        	infLOG(ERROR, " ]FileRequestFile: 파일 전송 취소 \n");

				//보내다가 오류가 났을시...DB처리

				if(UploadFile)
					fclose(UploadFile);
				return 0;

	        }


	        dTotalLen = dTotalLen - (double)nSendLen;
	    	nTotalnRecvLen = nTotalnRecvLen + (double)nSendLen;


	}

		#ifdef __DEBUG
	    printf("\r  ./dTotalLen = %.0f totalsend len = %.0f   send len = %d\n",dTotalLen, nTotalnRecvLen,nSendLen);
	    #endif

		//if(nReadLen != -1 && UploadFile)
		if(UploadFile)
			fclose(UploadFile);

		/*
		if(szSendBuffer)
			delete[] szSendBuffer;
		*/


	mi->second.nRealDown = 0; // 정상 다운 완료.

	//infLOG(ERROR," ] 다운로드 완료 : user %s :  File (%s) \n",mi->second.gCom9xxx.com9102_R.user_id, pFileinfo->szFileName);

	unsigned long ulDealID = pFileinfo->dwDealID;

	char szUserID[20];
	memset(szUserID, 0x00, sizeof(szUserID));

	sprintf(szUserID, "%s", mi->second.szUserID);
	sprintf(szUserID, "%s", mi->second.gCom9xxx.com9102_R.user_id);

	char szFileNM[1024];
	memset(szFileNM, 0x00, sizeof(szFileNM));

	sprintf(szFileNM, "%s", pFileinfo->szFileName);

	infLOG(ALWAY," > 다운로드 완료 : DealId (%lu) : user %s :  File (%s) \n",ulDealID , szUserID, szFileNM);

	return 1;
}


int FileRequestFileWithHoldTime(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{

	LPHEADER pHeader = (LPHEADER)pRecvHead;

	ERR_HEADER errheader;
	memset(&errheader,0x00,sizeof(ERR_HEADER));

	HEADER headers;
	memset(&headers,0x00,sizeof(HEADER));

	LPFILEINFO pFileinfo = (LPFILEINFO)pRecvData;


	//9002 다운로드 시작
	//9102 다운로드 끝

	multimap<int,USERINFO>::iterator mi; //IP 조회
	//mi = m_UserList.begin();
	mi = m_UserList.find(Socket);
	if(mi != m_UserList.end())
	{

		if(mi->second.gCom9xxx.com9002_R.temp_id != pFileinfo->nNumber || mi->second.gCom9xxx.nStatus == 0 )
		{
			//DB 업데이트 , 새로운 컨텐츠의 시작

			strcpyA(mi->second.gCom9xxx.com9002_R.server_id , pFileinfo->szServerID);
			strcpyA(mi->second.gCom9xxx.com9102_R.server_id , pFileinfo->szServerID);
			mi->second.gCom9xxx.com9002_R.temp_id = pFileinfo->nNumber;
			mi->second.gCom9xxx.com9102_R.temp_id = pFileinfo->nNumber;

			if(pFileinfo->nTypeDisk == 0x003)//컨텐츠구분(WE:위디스크(0x004), MY:공개자료실(0x003), MD:내자료실(0x005))
			{
				strcpyA(mi->second.gCom9xxx.com9002_R.cont_gu,"FD");
				strcpyA(mi->second.gCom9xxx.com9102_R.cont_gu,"FD");
			}
			else if(pFileinfo->nTypeDisk == 0x005)
			{
				strcpyA(mi->second.gCom9xxx.com9002_R.cont_gu,"MD");
				strcpyA(mi->second.gCom9xxx.com9102_R.cont_gu,"MD");
			}
			else if(pFileinfo->nTypeDisk == 0x004)
			{
				strcpyA(mi->second.gCom9xxx.com9002_R.cont_gu,"WE");
				strcpyA(mi->second.gCom9xxx.com9102_R.cont_gu,"WE");
			}
			else //exception
			{
				strcpyA(mi->second.gCom9xxx.com9002_R.cont_gu,"ER");
				strcpyA(mi->second.gCom9xxx.com9102_R.cont_gu,"ER");
			}



			if(mi->second.gCom9xxx.nStatus == 0)
			{
				Send_com9002(mi->second.gCom9xxx.com9002_R,1); //서버 사용자수 증가 , 사용자 수정
			}
			else
			{
				Send_com9002(mi->second.gCom9xxx.com9002_R,0); //사용자 수정
			}

			mi->second.gCom9xxx.nStatus = 1;


		}

		if(mi->second.temp_id != pFileinfo->nNumber )
		{
			strcpyA(mi->second.server_id , pFileinfo->szServerID);
			mi->second.temp_id = pFileinfo->nNumber;
		}

	}


	long seq_no; // 예취 ..돈계산을 위한 시퀀스 번호
	int nError;// 에러 정보 저장

	if(pFileinfo->nDealType)
	{

		char szSendData[HEADER_SIZE + sizeof(HOLDTIME)];
		memset(szSendData,0x00,sizeof(HEADER) + sizeof(HOLDTIME));

		pSendData = new char[sizeof(HEADER)];
		memset(pSendData,0x00,sizeof(HEADER));

		HOLDTIME hold_time;

		memset(&headers,0x00,sizeof(HEADER));
		if( pHeader->nCmd == RS_FILE_REQUEST_FILE_WITH_HOLD_TIME_GRID )
		{
			headers.nCmd = RS_FILE_REQUEST_FILE_WITH_HOLD_TIME_GRID; //파일 정보 요청
		}
		else
		{
			headers.nCmd = RS_FILE_REQUEST_FILE_WITH_HOLD_TIME; //파일 정보 요청
		}
		headers.nDataCnt = 1;
		headers.nDataSize = sizeof(HOLDTIME);
		headers.nErrorCode = 0;
		/*
		char szErrMsg[255];
		memset(szErrMsg,0x00,sizeof(szErrMsg));
		*/
		//정액제 현재 시간 보내기
		#ifdef __DEBUG
		printf("pFileinfo->nDealType [ %d ]\n",pFileinfo->nDealType);
		#endif

		int nRet=-1;
		if(g_bConnect1 == true)
		{
			nRet = com9006(pRecvData ,(char*)&hold_time,errheader.errmsg,g_szDcmdIP,g_nDcmdPort) ; // pFileinfo->dwDealID
		}
		else
		{	// 1번 안되면 2번 무조건 접속.
			nRet = com9006(pRecvData ,(char*)&hold_time,errheader.errmsg,g_szSUB_DcmdIP,g_nSUB_DcmdPort);
		}


		if( nRet < 0 )
		{
			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_FILE_REQUEST_FILE_WITH_HOLD_TIME;

			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

			pHeader->nCmd = RS_ERR;
			return -RS_FILE_REQUEST_FILE_WITH_HOLD_TIME;

		}

		#ifdef __DEBUG
		printf("hold_time.nHoldTimeMode [ %d ] != 0 \n",hold_time.nHoldTimeMode);
		#endif

		memcpy(szSendData,&headers,sizeof(HEADER)); //head
		memcpy(szSendData+sizeof(HEADER),(char*)&hold_time,headers.nDataCnt * headers.nDataSize);

		if(SendData(Socket,szSendData,HEADER_SIZE + headers.nDataCnt * headers.nDataSize )<0)  //struct _PACKET == PACKET
		{
			infLOG(ERROR, " ]FileRequestFileWithHoldTime:  ERROR Exception ) SendData (RS_FILE_REQUEST_FILE)\n");
			#ifdef __DEBUG
			printf("RS_FILE_REQUEST_FILE_WITH_HOLD_TIME : send 에러 1 : <client 죽음>\n");
			#endif
			return 0;
		}
	}
	else
	{

		#ifdef __DEBUG
		printf("//-----------------------------------------------\n"
			   "//정액제 범위 벗어남 							 \n"
			   "//-----------------------------------------------\n");
		#endif

		memset(&headers,0x00,HEADER_SIZE);
		headers.nCmd = RS_FILE_REQUEST_FILE; //파일 요청
		headers.nDataCnt = 0;
		headers.nDataSize = 0;

		if(SendData(Socket,(char*)&headers,HEADER_SIZE)<0)  //struct _PACKET == PACKET
		{
			infLOG(ALWAY, " ]FileRequestFileWithHoldTime:  ERROR Exception ) SendData (RS_FILE_REQUEST_FILE)\n");
			return 0;
		}
	}

	infLOG(ALWAY, " ]FileRequestFileWithHoldTime: SendData (RS_FILE_REQUEST_FILE)\n");
	// client에서 header을 수신

	memset(&headers,0x00,sizeof(HEADER));

	infLOG(ALWAY, " ]FileRequestFileWithHoldTime: Recv Header  \n");

	if(RecvData(Socket,(char*)&headers,HEADER_SIZE ) < 0)
	{
		infLOG(ERROR, " ]FileRequestFileWithHoldTime: ERROR Recv Header < 0 \n");

		return 0;
	}

	infLOG(ALWAY, " ]FileRequestFileWithHoldTime: headers.nCmd (%d) \n",headers.nCmd);

	if(headers.nCmd == RS_EOL)
	{

		memset(&headers,0x00,HEADER_SIZE);

		pSendData = new char[sizeof(HEADER)];
		memset(pSendData,0x00,sizeof(HEADER));

		headers.nCmd = RS_EOL;
		headers.nDataCnt = 0;
		headers.nDataSize = 0;

		memcpy(pSendData,&headers,sizeof(HEADER));

		infLOG(ALWAY, " ]FileRequestFileWithHoldTime: headers.nCmd == RS_EOL (return END)\n");

		return END;

	}
	else if (headers.nCmd == RS_FILE_REQUEST_NEXT_FILE)
	{
		pHeader->nCmd = RS_FILE_REQUEST_NEXT_FILEINFO; // send 하게 변경

		infLOG(ALWAY, " ]FileRequestFileWithHoldTime: RS_FILE_REQUEST_NEXT_FILEINFO \n");


		memset(&headers,0x00,HEADER_SIZE);

		pSendData = new char[sizeof(HEADER)];
		memset(pSendData,0x00,sizeof(HEADER));

		headers.nCmd = RS_FILE_REQUEST_NEXT_FILEINFO; //파일 정보 요청
		headers.nDataCnt = 0;
		headers.nDataSize = 0;

		memcpy(pSendData,&headers,sizeof(HEADER));

		#ifdef __DEBUG
		printf("  ./sucessed FileRequestNextFile...\n");
		#endif
		return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));//return 1;

	}else if(headers.nCmd == RS_GRID_DATA_TRANSFER) //그리드 시작
	{
		#ifdef __DEBUG
		printf("RS_GRID_DATA_TRANSFER\n");
		#endif

		infLOG(ALWAY, " ]FileRequestFileWithHoldTime: 그리드 파일 처리 RS_GRID_DATA_TRANSFER \n");
		memset(&headers,0x00,HEADER_SIZE);

		pSendData = new char[sizeof(HEADER)];
		memset(pSendData,0x00,sizeof(HEADER));

		pHeader->nCmd = RS_GRID_DATA_TRANSFER; // send 하게 변경


		headers.nCmd = RS_GRID_DATA_TRANSFER; //파일 정보 요청
		headers.nDataCnt = 0;
		headers.nDataSize = 0;

		memcpy(pSendData,&headers,sizeof(HEADER));

		return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));//return 1;
	}


	//예외 상황 강화 테스트 해야 함

	LPFILEHEAD pFileHead = new FILEHEAD;  //head 전송
	memset(pFileHead,0x00,sizeof(FILEHEAD));


	infLOG(ALWAY, " ]FileRequestFileWithHoldTime: Recv FILEHEAD \n");

	if(RecvData(Socket,(char*)pFileHead,sizeof(FILEHEAD))<0)
	{
		infLOG(ERROR, " ]FileRequestFileWithHoldTime: ERROR Exception ) Recv FILEHEAD \n");

		#ifdef __DEBUG
		printf("FileRequestFile : recv filehead 에러 : <client 죽음>\n");
		#endif

		delete pFileHead;
		return 0;
	}

	#ifdef __DEBUG
	printf("	파일 해더 채크 HOLD  \n"
		   "    file name : %s"
		   "    file size : %.0f"
		   "	sizeof	  : %d\n"
		   ,    pFileHead->szFullFileName
		   ,	pFileHead->dCurrentSize
		   ,	sizeof(FILEHEAD));
	#endif

	infLOG(ALWAY, " ]FileRequestFileWithHoldTime: 파일 해더 채크\n"
				  "    file name : %s\n"
				  "    current get file size : %.0f\n"
				  ,    pFileHead->szFullFileName
				  ,	   pFileHead->dCurrentSize );


	FILE* UploadFile;
	UploadFile = NULL;

	double dFileLen = 0;


	double nTotalnRecvLen = 0;

		//////////데이터 전송 루틴

	//그리드로 인한 수정
	char szFileFullName[1024];
	memset(szFileFullName,0x00,sizeof(szFileFullName));
	//memcpy(szFileFullName,pFileHead->szFullFileName , strlen(pFileHead->szFullFileName)-32);
	char* pFindStr = strchr(pFileinfo->szSrcPath,'|');
	if( pFindStr)
	{
		memcpy(szFileFullName,pFileinfo->szSrcPath ,strlen( pFileinfo->szSrcPath) - strlen(pFindStr) );
		strcat(szFileFullName,"/");
		strcat(szFileFullName,pFileinfo->szFileName);
	}
	else
		strcpy(szFileFullName,pFileHead->szFullFileName);


	infLOG(ALWAY, " mi->second.nRealDown =	 %d , pFileinfo->nNumber = %ld,pFileHead->dCurrentSize : %.0f \n"
	,mi->second.nRealDown
	,pFileinfo->nNumber
	,pFileHead->dCurrentSize);

	if(pFileHead->dCurrentSize == 0)
	{
		infLOG(ALWAY, " Z1\n");
		mi->second.nRealDown = 1;
		SendRealCount(1,pFileinfo->nNumber);
	}
	else
	{
		if(mi->second.nRealDown == 0)
		{
			infLOG(ALWAY, " Z2\n");
			mi->second.nRealDown = 1;
			SendRealCount(1,pFileinfo->nNumber);
		}
		else
		{
			infLOG(ALWAY, " Z3\n");
			SendRealCount(1,pFileinfo->nNumber);
		}
	}

	UploadFile = fopen64(szFileFullName,"rb");
	//UploadFile = fopen64(pFileHead->szFullFileName,"rb");
	//그리드로 인한 수정

	if(UploadFile == NULL)
	{
		infLOG(ERROR, " ]FileRequestFileWithHoldTime: ERROR Exception ) UploadFile == NULL (%s)\n",pFileHead->szFullFileName);

		#ifdef __DEBUG
		printf("FileRequestFile : UploadFile == NULL: <client 죽음>\n");
		printf("  ./file open error : %s \n",pFileHead->szFullFileName);
		#endif

		ERR_HEADER errheader;
		memset(&errheader,0x00,sizeof(ERR_HEADER));
		pSendData = new char[sizeof(ERR_HEADER)];
		memset(pSendData,0x00,sizeof(ERR_HEADER));
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
		strcat(errheader.errmsg,"서버측 에서 파일이 없거나 열수 없습니다.");

		memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

		delete pFileHead;

		pHeader->nCmd = RS_ERR;
		return -RS_FILE_DATA_TRANSFER;

	}

	#ifdef __DEBUG
	printf("Open File [ %s ]\n",szFileFullName);
	#endif


	//set file head , 전체 사이즈 , 파일 이름


	struct stat64 statbuf;

	//grid
	//int stat = stat64(pFileHead->szFullFileName,&statbuf);
	int stat = stat64(szFileFullName,&statbuf);


	if(stat != 0) //error
	{
		infLOG(ERROR, " ]FileRequestFileWithHoldTime: Exception ) stat != 0\n");

		if(pFileHead != NULL)
			delete pFileHead;

		if(errno == ENOENT) //파일이나 패스가 없음
		{
			infLOG(ALWAY, " ]FileRequestFileWithHoldTime: ERROR Exception ) errno == ENOENT \n");

			#ifdef __DEBUG
			printf("  ./file create error : %s \n",pFileHead->szFullFileName);
			#endif



			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
			strcat(errheader.errmsg,"서버측 에서 파일이 없거나 열수 없습니다.");

			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

			pHeader->nCmd = RS_ERR;
			return -RS_FILE_DATA_TRANSFER;


		}

		return 0;

	}




	// 현제 client에 있는 사이즈 만큼 이동 하기
	//	unsigned long dCurrentLen = pFileHead->dCurrentSize;
	double dCurrentLen = pFileHead->dCurrentSize;

	off64_t current_set = (off64_t)dCurrentLen;

/*
	#ifdef __DEBUG
	printf("  ./ check size 64 bit : %.0f / %.0f\n",(double)current_set,dCurrentLen);
	#endif
*/
	fseeko64(UploadFile,current_set,SEEK_SET);

	 // 해더 전송  실제 사이즈와 이름.



///////////////////////////////////////////////////////////////////////
		 	//예취 전환.  복
	char DnDate[9];
	memset(DnDate,0x00,9);

	memset(pFileHead,0x00,sizeof(FILEHEAD));

	dFileLen  = (double)statbuf.st_size;;
	if(dFileLen < 0 )
		dFileLen = 0;

	pFileHead->dwID = seq_no;
/*
	if(pFileinfo->GroupInfo.nMode == 3 || pFileinfo->GroupInfo.nMode == 1)
	{
		pFileinfo->GroupInfo.dwID = seq_no;
		strcpyA(pFileinfo->GroupInfo.DnDate,DnDate);
	}
*/
	strcpyA(pFileHead->DnDate,DnDate);

	pFileHead->dCurrentSize = dFileLen;
	strcpyA(pFileHead->szFullFileName ,pFileinfo->szFileName);


	#ifdef __DEBUG
	printf("  ./check in file head \n"
		   "    file seq = %ld : file name : %s  : dn date = %s :"
		   "    file size : %15.0f\n"
		   ,	pFileHead->dwID
		   ,    pFileHead->szFullFileName
		   ,	DnDate
		   ,	pFileHead->dCurrentSize);
	#endif

	infLOG(ALWAY, " ] FileRequestFileWithHoldTime >\n"
		"	 check ) file head \n"
		"    file seq = %ld : file name : %s  : dn date = %s \n"
		"    file size : %15.0f\n"
				,	pFileHead->dwID
				,    pFileHead->szFullFileName
				,	DnDate
				,	pFileHead->dCurrentSize);





	//데이터 전송

		/////////////////////////////

	#ifdef __DEBUG
	printf("  ./make file head\n");
	#endif

	memset(&headers,0x00,HEADER_SIZE);

	headers.nCmd = RS_FILE_DATA_TRANSFER;
	headers.nDataCnt = 1;
	headers.nDataSize = sizeof(FILEHEAD);


	char* pSendBuffer = new char[HEADER_SIZE + headers.nDataCnt*headers.nDataSize ];
	memset(pSendBuffer, 0x00, HEADER_SIZE + headers.nDataCnt*headers.nDataSize );

	memcpy(pSendBuffer,&headers,sizeof(HEADER)); //head
	memcpy(pSendBuffer + HEADER_SIZE,pFileHead,headers.nDataCnt * headers.nDataSize); //body

	#ifdef __DEBUG
	printf("  ./send head size : %d \n",HEADER_SIZE + headers.nDataCnt*headers.nDataSize);
	#endif


	infLOG(ALWAY, " ]FileRequestFileWithHoldTime: Send File Head\n");

	if(	SendData(Socket,pSendBuffer,HEADER_SIZE + headers.nDataCnt*headers.nDataSize)<=0)  //struct _PACKET == PACKET
	{
		infLOG(ERROR, " ]FileRequestFileWithHoldTime: ERROR Exception )Send File Head");

		#ifdef __DEBUG
		printf("FileRequestFile : send 에러 예취금 복원 1: <client 죽음>");
		#endif

		delete[] pSendBuffer;
		delete pFileHead;

		return 0;

	}

	delete[] pSendBuffer;
	delete pFileHead;


	//////////데이터 전송 루틴
	double dTotalLen = dFileLen;
	double dTempTotalLen = dTotalLen;
	dTotalLen = dTotalLen - dCurrentLen; //현재 이동한 만큼 계산

	int nReadLen=0;
	int nSendLen=0;

	int nStop = 0;



	//char* szSendBuffer = new char[SENDBUF];
	char szSendBuffer[SENDBUF];


	////[][]

	infLOG(ALWAY, " ] Send File Data\n");
	#ifdef __DEBUG
	printf("Checking File (%s) : file check (%.0f ) = ( %.0f - %.0f )\n",pFileinfo->szFileName , dTotalLen ,pFileinfo->dFileSize ,dCurrentLen);
	#endif


	infLOG(ALWAY," ]FileRequestFileWithHoldTime: (%lu)\n"
	             "   Checking File (%s) : file check  (%.0f ) = ( %.0f - %.0f )\n"
	             , pFileinfo->dwDealID
	             ,pFileinfo->szFileName , dTotalLen ,pFileinfo->dFileSize ,dCurrentLen);


	int fno = fileno(UploadFile);


	//off_t offset = 0;


	while(dTotalLen > 0 && !nStop )
	{

		memset(szSendBuffer,0x00,SENDBUF);
		nReadLen = read(fno,szSendBuffer,SENDBUF);

    	if(nReadLen > 0 )
    		nSendLen = SendData(Socket,szSendBuffer,nReadLen);
    	else
    		nSendLen = 0;

        if(nSendLen <= 0)
        {
        	#ifdef __DEBUG
        	printf("-=----------- 예외 상황 파일 받기 ------11\n");
        	#endif
        	if(nSendLen < 0)
        	{
        		char szErrMsg[1024];
				memset(szErrMsg,0x00,sizeof(szErrMsg));
				GetErrMsg(-nSendLen,szErrMsg);
				infLOG(ERROR, " ]FileRequestFileWithHoldTime: Check ) RecvSocket Error ( %d ) ( %d )( %s )\n",nReadLen ,nSendLen,szErrMsg);

				#ifdef __DEBUG
				printf("RecvSocket Error ( %d ) ( %d )( %s )\n",nReadLen ,nSendLen,szErrMsg);
				#endif


        	}
        	else if(nSendLen == 0)
        	{
        		infLOG(ERROR, " ]FileRequestFileWithHoldTime: Check ) RecvSocket Error ( %d ) ( 접속을 끊었습니다. )\n",nReadLen );

				#ifdef __DEBUG
				printf("RecvSocket Error ( %d ) ( 접속을 끊었습니다. )\n",nReadLen );
				#endif
        	}

			infLOG(ERROR, " ]FileRequestFileWithHoldTime: 파일 전송 취소\n");


			if(UploadFile)
				fclose(UploadFile);

			return 0;

        }


        dTotalLen = dTotalLen - (double)nSendLen;
    	nTotalnRecvLen = nTotalnRecvLen + (double)nSendLen;
	}

	#ifdef __DEBUG
    printf("\r  ./dTotalLen = %.0f totalsend len = %.0f   send len = %d\n",dTotalLen, nTotalnRecvLen,nSendLen);
    #endif

	if(UploadFile)
		fclose(UploadFile);


	infLOG(ALWAY, " Z4\n");
	mi->second.nRealDown = 0;

	unsigned long ulDealID = pFileinfo->dwDealID;

	char szUserID[20];
	memset(szUserID, 0x00, sizeof(szUserID));

	sprintf(szUserID, "%s", mi->second.szUserID);
	sprintf(szUserID, "%s", mi->second.gCom9xxx.com9102_R.user_id);

	char szFileNM[1024];
	memset(szFileNM, 0x00, sizeof(szFileNM));

	sprintf(szFileNM, "%s", pFileinfo->szFileName);

	infLOG(ALWAY," FileRequestFileWithHoldTime >\n > 다운로드 완료 : DealId (%lu)  user %s :  File (%s) \n",pFileinfo->dwDealID,mi->second.szUserID, pFileinfo->szFileName);

	return 1;


}





