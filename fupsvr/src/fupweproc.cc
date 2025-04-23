#include "fupsock.h"
#include "fupdefine.h"
#include "comcomm.h"
#include "apdefine.h"
#include "fupweproc.h"
#include "fupcomlib.h"
#include "comhead.h"
#include "com9001.h" //사용자수 증가
#include "com9004.h" //성인물 컨텐츠 등록 제어
#include "com9101.h" //사용자수 감소
#include "com9104.h" //업로드 오류 처리
#include "com9103.h" //필로그 자료실 업로드 용량 확인
#include "com9105.h" //T_CONTENTS_TEMP 에 파일 저장
#include "com9106.h" // 중복파일 체크
#include "fups40010.h"
#include "fups4005.h"
#include "fups4006.h"
#include "cmd5.h"

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

// 다음 파일 요구
int FileRequestNextFile(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	infLOG(ALWAY,"CMD > FileRequestNextFile\n");

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

// 파일 정보 요구
int FileRequestFile(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	infLOG(ALWAY,"CMD > FileRequestFile\n");
	HEADER headers;
	memset(&headers,0x00,HEADER_SIZE);

	pSendData = new char[sizeof(HEADER)];
	memset(pSendData,0x00,sizeof(HEADER));

	headers.nCmd = RS_FILE_REQUEST_FILE_FILINFO; //파일 정보 요청
	headers.nDataCnt = 0;
	headers.nDataSize = 0;

	memcpy(pSendData,&headers,sizeof(HEADER));

	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));//return 1;
}

//파일 목록( MY_DISK 에서 폴더 일때 필요 -> 다른 사람에게 공유 할 디스크  )
//내자료실일때 추가

// 파일 리스트 요구
int FileRequestList(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	infLOG(ALWAY,"CMD > FileRequestList\n");

	LPHEADER pHeader = (LPHEADER)pRecvHead; //head

	ERR_HEADER errheader; //err head
	memset(&errheader,0x00,sizeof(ERR_HEADER));

	HEADER headers; //temp head
	memset(&headers,0x00,sizeof(HEADER));

	LPFILEINFO pFileinfo = (LPFILEINFO)pRecvData; //body

	char szFullName[768];
	memset(szFullName,0x00,sizeof(szFullName));

	char szCheckName[768];
	memset(szCheckName,0x00,sizeof(szCheckName));

	char szFolderName[50];
	memset(szFolderName,0x00,sizeof(szFolderName));

	struct stat64 statbuf;

	time_t	curtime;
	struct tm		*stm;
	time( &curtime );
	stm = (struct tm *) localtime(&curtime);

	localtime_r(&curtime, stm);

	//pFileinfo->szDownPath 는 root_path = /raid/fdata/wedisk
	sprintf(szFullName,"%s/%04d/%02d/%02d/%02d"
									, pFileinfo->szDownPath
									,  stm->tm_year+1900
									,  stm->tm_mon + 1
									,  stm->tm_mday
									,  stm->tm_hour);

	sprintf(szFolderName,"temp%lu", pFileinfo->cfups4001.id);
	sprintf(szCheckName,"%s/%s",szFullName,szFolderName);


	infLOG(ALWAY,"============ pFileinfo->cfups4001.copyright_yn [ %s ] \n",pFileinfo->cfups4001.copyright_yn);

	infLOG(ALWAY,"Server Folder Check [ %s ] \n",szCheckName);

	int stat = lstat64(szCheckName,&statbuf);

	if(stat != 0) //파일이 없으면 폴더 만들기.
	{
		infLOG(ALWAY,"Make Folder [ %s ]\n",szCheckName );

		if(MakeFolder(szCheckName)== -1)
		{
			infLOG(ALWAY,"Make Folder ERROR [ %s ] \n",szCheckName);
		}
	}


	headers.nCmd = RS_FILE_REQUEST_LIST;

	headers.nDataCnt =1;
	headers.nDataSize = sizeof(FILEINFO);
	headers.nErrorCode = 0;

	FILEINFO FolderInfo;
	memset(&FolderInfo,0x00,sizeof(FILEINFO));

	memcpy(&FolderInfo,pFileinfo,sizeof(FILEINFO));

	strcpy(FolderInfo.szDownPath,szFullName);

	memcpy(FolderInfo.cfups4001.file_path,szFullName,sizeof(szFullName)); //서버측 패스
	memcpy(FolderInfo.cfups4001.file_name2,pFileinfo->szFolderName,sizeof(pFileinfo->szFolderName)); //로컬파일 이름
	memcpy(FolderInfo.cfups4001.file_name1,szFolderName,sizeof(szFolderName));			 //서버파일

	pSendData = new char[sizeof(HEADER) + headers.nDataCnt * headers.nDataSize];
	memset(pSendData,0x00,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize);

	memcpy(pSendData,&headers,sizeof(HEADER)); //head
	memcpy(pSendData+sizeof(HEADER),&FolderInfo,  headers.nDataCnt * headers.nDataSize);

	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));
}

//컨텐츠 파일 전송
int FileDataTransfer(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	infLOG(ALWAY, "FileDataTransfer\n");

	char szErrMsg[1024];
	memset(szErrMsg,0x00,sizeof(szErrMsg));

	struct stat64 statbuf;
	LPHEADER pHeader = (LPHEADER)pRecvHead; //head

	ERR_HEADER errheader; //err head
	memset(&errheader,0x00,sizeof(ERR_HEADER));

	HEADER headers; //temp head
	memset(&headers,0x00,sizeof(HEADER));

	LPFILEINFO pFileinfo = (LPFILEINFO)pRecvData; //body

	infLOG(ALWAY,"============ pFileinfo->cfups4001.copyright_yn [ %s ] \n",pFileinfo->cfups4001.copyright_yn);

	//성인물 등록 제한 9004
	COM9004D com9004Result;
	memset(&com9004Result,0x00,sizeof(COM9004D));

	com9004Result = com9004(pHeader->szUserID, pFileinfo->cfups4001.id , pFileinfo->cfups4001.file_size, pFileinfo->cfups4001.descript/*no.767*/);
	int nCType = com9004Result.temp_id;

	infLOG(ALWAY,"Check 9004 Packet : \n"
				" long long temp_id = %lld      \n" //long long type
				" double file_size	= %15.0f    \n"
				" char user_id[16]  = %s        \n"
				" char auth_num[3]  = %s        \n"
				,	com9004Result.temp_id ,com9004Result.file_size	,com9004Result.user_id , com9004Result.auth_num );


	infLOG(ALWAY,"Check nCType[등록타입] \ncom9004 result [ %d ] \n[ -90042 파일 정보 조회 오류 ]\n[ -4 :필로그 ] \n[ -3 : 파일변경 ] \n[ -2 : 하루에 한건 ] \n[ -1 : 등록한 파일정보를 찾을 수 없습니다.] \n[ -5 : 업로드 모듈 변경 ] \n[ 1 : 위디스크 ]  \n",nCType );

	bool bHaveCopyright = false;
	bool bHaveCompany = false;
	bool bGhostMode = false; //파일을 생성하지 않고 네트웍만 받는 모드 여부
	int nTotalRecvFileCnt = 0;

	char szSubFilePath[512];
	char szFolderPath[512];
	char szFolderFullPath[768];

	memset(szSubFilePath,0x00,sizeof(szSubFilePath));
	memset(szFolderPath,0x00,sizeof(szFolderPath));
	memset(szFolderFullPath,0x00,sizeof(szFolderFullPath));


	//change upload module - 현재 사용안하는 버전
	if( nCType == -5)  //no.767
	{
		pSendData = new char[sizeof(ERR_HEADER)];
		memset(pSendData,0x00,sizeof(ERR_HEADER));
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;

		strcat(errheader.errmsg,"위디스크 프로그램을 삭제 후 최신 버전으로 업데이트 해주세요.");

		memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

		pHeader->nCmd = RS_ERR;


		return -RS_FILE_DATA_TRANSFER;

	}
	else if( nCType == -4)  //필로그 자료실
	{
		infLOG(ALWAY, "Start Send Filog Data \n");

		//9001 호출 // 사용자수 증가
		//9101 호출 //사용자수 감소

		CCOM9001_R com9001_r ;
		memset(&com9001_r,0x00,sizeof(CCOM9001_R));

		multimap<int,USERINFO>::iterator mi; //IP 조회
		mi = m_UserList.find(Socket); 		//mi = m_UserList.begin();
		if(mi != m_UserList.end())
			strcpy(com9001_r.conn_ip ,mi->second.thread.userIP);

		strcpy(com9001_r.cont_gu ,"FD");
		strcpy(com9001_r.server_id , pFileinfo->szServerID);
		com9001_r.temp_id =  pFileinfo->cfups4001.id;
		memcpy(com9001_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
		com9001_r.upload_size = pFileinfo->cfups4001.file_size;
		com9001 ( com9001_r);

		CCOM9101_R com9101_r ;
		memset(&com9101_r,0x00,sizeof(CCOM9101_R));
		strcpy(com9101_r.conn_ip , com9001_r.conn_ip);
		strcpy(com9101_r.server_id , com9001_r.server_id);
		com9101_r.temp_id =  com9001_r.temp_id;
		strcpy(com9101_r.user_id ,com9001_r.user_id); // 사용자
		com9101_r.upload_size = com9001_r.upload_size;

		infLOG(ALWAY,"제목 검사 [ %s ] \n",pFileinfo->cfups4001.title);
		infLOG(ALWAY,"파일 검사 [ %s ]\n",pFileinfo->cfups4001.file_path);

		char szFullPath[768];
		memset(szFullPath,0x00,sizeof(szFullPath));

		char szFullName[768];
		memset(szFullName,0x00,sizeof(szFullName));

		int stat = -1;                 // 파일 상태 결정
		bool bFOpenAppendMode = false; // 파일 append 모드 결정

		CCOM9104_R pcom9104_r; // 받던 파일 취소시 DB 돌리기 ( T_CONTENTS_TEMP 삭제 )

		FILEINFO rFileInfo;

		double dTotalRecvLen = 0; //총 받은 길이
		double dTotalLen = 0; // down될 파일의 총 길이
		int nWriteLen=0;      // 파일에 write 한 크기
		int nRecvLen=0;       // 소켓으로 recv 한 크기
		int nCheckStop = 0; //while 루프 제어

		CCOM9103_R pcom9103_r; // 필로그 용량 제어
		memset(&pcom9103_r,0x00,sizeof(CCOM9103_R));

		pcom9103_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 4=filog disk
		pcom9103_r.file_size = pFileinfo->cfups4001.file_size;
		memcpy(pcom9103_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID));

		memset(szErrMsg,0x00,sizeof(szErrMsg));

		if(com9103(pcom9103_r,szErrMsg)< 0)
		{
			infLOG(ALWAY, "필로그 자료실의 용량을 업데이트 할 수 없습니다. [ com9013 - T_PERM_UPLOAD_AUTH 테이블을 확인하세요 ]\n");
			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
			strcat(errheader.errmsg,"필로그 자료실 용량 업데이트 오류 입니다.");
			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));
			pHeader->nCmd = RS_ERR;

			com9101 ( com9101_r);

			return -RS_FILE_DATA_TRANSFER;
		}

		CCOM9105_R com9105_r;		// temp 에 파일 정보 저장.
		memset(&com9105_r,0x00,sizeof(CCOM9105_R));

		if(pFileinfo->nType == FT_FOLDER)
		{
			infLOG(ALWAY, "폴더 업로드입니다.\n");

			//9105 폴더	목록 조회
			memset(&com9105_r,0x00,sizeof(CCOM9105_R));
			com9105_r.id = pFileinfo->cfups4001.id;
			memcpy(&com9105_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
			strcpy(com9105_r.server_id ,pFileinfo->szServerID);
			strcpy(com9105_r.sfile_path ,pFileinfo->cfups4001.file_path);
			strcpy(com9105_r.sfile_name ,pFileinfo->cfups4001.file_name1);
			com9105(com9105_r);
		}

		do
		{
			nCheckStop++; //예외처리
			if(nCheckStop >= 1100)
			{


				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
				pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
				pcom9104_r.id         = pFileinfo->cfups4001.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
				pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자

				//필로그 용량을 복구 시킨다.
				infLOG(ERROR, "필로그의 업로그 갯수를 초과 하였습니다.\ntemp_id [ %lu ]file count = %d , rollback size = %.0f [ com9104 ]\n",pFileinfo->cfups4001.id,nCheckStop , pcom9104_r.file_size);

				if(com9104(pcom9104_r) < 0)
				{
					infLOG(ERROR, "필로그 용량 복구 중 오류가 발생하였습니ㅏ다.[com9104]\n");
				}
				com9101 ( com9101_r);
				return 0;

			}
	    	infLOG(ALWAY,"이어 올리기 Flag[ %d ] >> [ 1 , 2 는 재시도 0 은 일반 ] \n",pFileinfo->nReUploadFlag);

		    headers.nCmd  = RS_FILE_DATA_SIGN_CHECK; // 파일 전송 메세지

			if(pFileinfo->nReUploadFlag == RECONNECT_UPLOAD || pFileinfo->nReUploadFlag == RE_UPLOAD)
			{// 이어 올리기
				if( pFileinfo->nType == FT_FOLDER)
				{
					strcpy(szFullPath, pFileinfo->cfups4001.file_path); //./2004/02/18/16/raid
					strcat(szFullPath,"/");

					memset(szFolderFullPath,0x00,sizeof(szFolderFullPath));
					strcpy(szFolderFullPath, szFullPath); //./2004/02/18/16/raid

					strcat(szFullPath,pFileinfo->cfups4001.file_name1);//<- 요거 값 잘못 들어옴

					strcpy(szFullName,pFileinfo->szDownPath); //szDownPath 은 ./raid
	    			strcat(szFullName,"/");
	    			strcat(szFullName,pFileinfo->szFileName); //szfilename 은 a.txt

					infLOG(ALWAY, "폴더 이어 올리기 - 위치 [ %s ] 최종 위치 [ %s ]\n",szFullPath,szFullName);
				}
				else
				{
			    	strcpy(szFullName,pFileinfo->szDownPath);
					strcat(szFullName,"/"); //./2004/02/18/16/raid/   <-- '/' 추가
					strcat(szFullName,pFileinfo->szFileName);

					strcpy(szFullPath, pFileinfo->cfups4001.file_path); //./2004/02/18/16/raid
					strcat(szFullPath,"/");
					strcat(szFullPath,pFileinfo->cfups4001.file_name1);//


					infLOG(ALWAY, "파일 이어 올리기 - 위치 [ %s ] 최종 위치 [ %s ]\n",szFullPath,szFullName);

					//9105 파일
					memset(&com9105_r,0x00,sizeof(CCOM9105_R));

					com9105_r.id = pFileinfo->cfups4001.id;
					memcpy(&com9105_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
					strcpy(com9105_r.server_id ,pFileinfo->szServerID);
					strcpy(com9105_r.sfile_path ,pFileinfo->szDownPath);
					strcpy(com9105_r.sfile_name ,pFileinfo->szFileName);

					com9105(com9105_r);

				}

				stat = stat64(szFullName,&statbuf);
				if(stat != 0) //파일이 없으면 폴더 만들기.
				{
					infLOG(ALWAY,"파일이 없음으로 폴더를 생성합니다. 파일 [ %s ] 폴더 [ %s ] \n",pFileinfo->szDownPath,szFullName);
					MakeFolder(pFileinfo->szDownPath) ;

				}
				else
				{
					infLOG(ALWAY,"파일이 이미 존재 합니다. Append 모드로 파일을 제공합니다. 파일 [ %s ] 폴더 [ %s ] \n",pFileinfo->szDownPath,szFullName);
					bFOpenAppendMode = true;

				}

			}
			else
			{ //이어 올리기 아님.
		    	infLOG(ALWAY,"일반 업로드 모드 입니다.\n" );

		    	srand((unsigned int)time(NULL))	; //random 이름을 위함 시드 지정

				///// 날짜 시간 생성 ////
				time_t			curtime;
				struct tm		*stm;
				time( &curtime );
				stm = (struct tm *) localtime(&curtime);

				localtime_r(&curtime, stm);
				bool bResult = false;

		  		if( pFileinfo->nType == FT_FILE)
		  		{
		  			infLOG(ALWAY,"파일 업로드 입니다.\n");

		  			infLOG(ALWAY,"파일 Root Path 는 [ %s ] 입니다.\n",pFileinfo->szDownPath);

					sprintf(pFileinfo->szDownPath,"%s/%04d/%02d/%02d/%02d",  pFileinfo->szDownPath
										,  stm->tm_year+1900
										,  stm->tm_mon + 1
										,  stm->tm_mday
										,  stm->tm_hour);//./2004/02/18/16

		  			infLOG(ALWAY,"파일 Root Path 를 설정합니다. [ %s ]\n",pFileinfo->szDownPath);

					memset(szFullName,0x00,sizeof(szFullName));

			    	strcpy(szFullName,pFileinfo->szDownPath);
					strcat(szFullName,"/"); //./2004/02/18/16/raid/   <-- '/' 추가

			    	//file name 얻기

			    	char szFilename[50];
			    	char szFileType[10];
			    	memset(szFilename,0x00,sizeof(szFilename));
			    	memset(szFileType,0x00,sizeof(szFileType));


					sprintf(szFilename,"temp%lu",pFileinfo->cfups4001.id);
			    	//local 파일이름으로 부터 확장자 얻기.
			    	int nLen = GetReverseIndex(pFileinfo->cfups4001.file_name2 , '.');
					//	nLen = nLen - 1; // a.txt -> for .txt 하기 위해 nLen -1 해줌
					//	nLen = nLen - 1; // ./raid/ -> ,./raid   , '/' delete
					infLOG(ALWAY, "파일 이름 검사 [ %s ] \n",pFileinfo->cfups4001.file_name2);

					if(nLen < 0)
						infLOG(ALWAY, "파일 이름의 확장자가 없습니다. [ 무시 ]\n");
					else
					{
					    GetRightString(pFileinfo->cfups4001.file_name2,strlen(pFileinfo->cfups4001.file_name2)-nLen,szFileType);
					    infLOG(ALWAY, "파일 확장자 검사 [ %s ] \n",szFileType);
					}

					strcpy(pFileinfo->cfups4001.file_name2,pFileinfo->szFileName);
					memset(pFileinfo->szFileName,0x00,sizeof(pFileinfo->szFileName));

					strcpy(pFileinfo->szFileName,szFilename);
					strcat(pFileinfo->szFileName,szFileType);
					strcat(szFullName,szFilename);
					strcat(szFullName,szFileType);

					//// 이름 저장 ////
					memcpy(pFileinfo->cfups4001.file_name1,pFileinfo->szFileName,sizeof(pFileinfo->szFileName));
					strcpy(pFileinfo->cfups4001.file_path,pFileinfo->szDownPath);//,sizeof(pFileinfo->szDownPath));

					stat = stat64(szFullName,&statbuf);

					if(stat != 0) //파일이 없으면 폴더 만들기.
					{
						MakeFolder(pFileinfo->szDownPath) ;
						infLOG(ALWAY,"폴더가 없음으로 폴더를 생성합니다. [ %s ] \n",pFileinfo->szDownPath);
					}
					else
					{
						infLOG(ALWAY,"폴더가 이미 존재 합니다. Append 모드로 파일을 제공합니다. [ %s ] \n",szFullName);
						bFOpenAppendMode = true;
					}

					//9105 파일
					memset(&com9105_r,0x00,sizeof(CCOM9105_R));

					com9105_r.id = pFileinfo->cfups4001.id;
					memcpy(&com9105_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
					strcpy(com9105_r.server_id ,pFileinfo->szServerID);
					strcpy(com9105_r.sfile_path ,pFileinfo->cfups4001.file_path);
					strcpy(com9105_r.sfile_name ,pFileinfo->cfups4001.file_name1);

					com9105(com9105_r);

				}
				else if(pFileinfo->nType == FT_FOLDER)//전송 받을 파일이 폴더 일경우
				{
					infLOG(ALWAY,"폴더 업로드 입니다.\n");

					strcpy(szFullPath, pFileinfo->cfups4001.file_path); //./2004/02/18/16/raid
					strcat(szFullPath,"/");

					memset(szFolderFullPath,0x00,sizeof(szFolderFullPath));
					strcpy(szFolderFullPath, szFullPath); //./2004/02/18/16/raid

					strcat(szFullPath,pFileinfo->cfups4001.file_name1);//

					//////////////////////////////////////////////////////////////////////////

					strcpy(szFullName,pFileinfo->szDownPath); //szDownPath 은 ./raid
	    			strcat(szFullName,"/");
	    			strcat(szFullName,pFileinfo->szFileName); //szfilename 은 a.txt

					stat = stat64(szFullName,&statbuf);


					if(stat != 0) //파일이 없으면 폴더 만들기.
					{
						MakeFolder(pFileinfo->szDownPath) ;
						infLOG(ALWAY,"폴더가 없음으로 폴더를 생성합니다. [ %s ] \n",pFileinfo->szDownPath);
					}
					else
					{
						infLOG(ALWAY,"폴더가 이미 존재 합니다. Append 모드로 파일을 제공합니다. [ %s ] \n",szFullName);
						bFOpenAppendMode = true;
					}
				}
			}



			headers.nCmd = RS_FILE_DATA_SIGN_CHECK; //파일 전송
			int nSRet = 0;

			if( pFileinfo->nType == FT_FILE )
			{
				infLOG(ALWAY,"Send RS_FILE_DATA_SIGN_CHECK - sizeof(FILEINFO) [%d]\n",sizeof(FILEINFO));

			    headers.nDataCnt = 1;
				headers.nDataSize = sizeof(FILEINFO);
				headers.nErrorCode = 0;

				char szSendData[HEADER_SIZE + sizeof(FILEINFO)];
				memset(szSendData,0x00,HEADER_SIZE + sizeof(FILEINFO));

				memcpy(szSendData,&headers,HEADER_SIZE);
				memcpy(szSendData + HEADER_SIZE , pFileinfo , sizeof(FILEINFO));

				nSRet = SendData(Socket,szSendData,HEADER_SIZE + headers.nDataCnt*headers.nDataSize);
			}
			else
			{
				infLOG(ALWAY,"Send RS_FILE_DATA_SIGN_CHECK\n");
			    headers.nDataCnt = 0;
				headers.nDataSize = 0;
				headers.nErrorCode = 0;
				nSRet = SendData(Socket,(char*)&headers,sizeof(struct _HEADER));
			}

		    //// 전송하기전에 메세지를 알림...
		    if(	nSRet <=0 )
			{
				infLOG(ERROR, "RS_FILE_DATA_SIGN_CHECK 전송 오류.\n");

				// 받던 파일 삭제
				///////////////////////////////////////////////
				// temp 삭제									 //
				///////////////////////////////////////////////

				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
				pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
				pcom9104_r.id         = pFileinfo->cfups4001.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
				pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자

				infLOG(ERROR, "필로그 용량을 복구 합니다. File Size [ %.0f ]\n",pcom9104_r.file_size);
				if(com9104(pcom9104_r) < 0)
				{
					infLOG(ERROR, "필로그 용량 복구 중 오류가 발생하였습니ㅏ다.[com9104]\n");
				}
				com9101 ( com9101_r);
				return 0;
			}

			memset(&headers,0x00,sizeof(HEADER));

			if(	RecvData(Socket,(char*)&headers,sizeof(struct _HEADER))<=0)  //struct _PACKET == PACKET
			{
				infLOG(ERROR, "RS_FILE_DATA_SIGN_CHECK 결과 받기 오류\n");
				// 받던 파일 삭제
				///////////////////////////////////////////////
				// temp 삭제									 //
				///////////////////////////////////////////////

				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
				pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
				pcom9104_r.id         = pFileinfo->cfups4001.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
				pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자

				infLOG(ERROR, "필로그 용량을 복구 합니다. File Size [ %.0f ]\n",pcom9104_r.file_size);
				if(com9104(pcom9104_r) < 0)
				{
					infLOG(ERROR, "필로그 용량 복구 중 오류가 발생하였습니ㅏ다.[com9104]\n");
				}
				com9101 ( com9101_r);
				return 0;
			}

			if(headers.nCmd == RS_EOL)
			{
				infLOG(ALWAY, "RS_FILE_DATA_SIGN_CHECK 결과 받기 - RS_EROL \n");

				pSendData = new char[sizeof(HEADER)];
				memset(pSendData,0x00,sizeof(HEADER));

				headers.nCmd = RS_EOL;
				headers.nDataCnt = 0;
				headers.nDataSize = 0;
				headers.nErrorCode = 0;

				memcpy(pSendData, &headers, sizeof(HEADER));

				// 받던 파일 삭제
				///////////////////////////////////////////////
				// temp 삭제									 //
				///////////////////////////////////////////////

				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
				pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
				pcom9104_r.id         = pFileinfo->cfups4001.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
				pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자

				infLOG(ERROR, "필로그 용량을 복구 합니다. File Size [ %.0f ]\n",pcom9104_r.file_size);
				if(com9104(pcom9104_r) < 0)
				{
					infLOG(ERROR, "필로그 용량 복구 중 오류가 발생하였습니ㅏ다.[com9104]\n");
				}
				com9101 ( com9101_r);
				return END;
			}
			else if(headers.nCmd == RS_OK)
			{
				infLOG(ALWAY, "RS_FILE_DATA_SIGN_CHECK 결과 받기 - RS_OK \n");

			}

			//2009/09/09(필로그 필터링 연동) 뮤레카 정보 받기.
			int nMurekaCnt = headers.nDataCnt;

			LPMUREKA_VINFO pMurekaVInfo = NULL;

			infLOG(ALWAY, "필터링 결과 파일 확인 - 갯수 [ %d ] \n",nMurekaCnt);

			if(nMurekaCnt > 0)
			{
				pMurekaVInfo = new MUREKA_VINFO[nMurekaCnt];
				if(	RecvData(Socket,(char*)pMurekaVInfo,sizeof(MUREKA_VINFO)*nMurekaCnt)<=0)  //struct _PACKET == PACKET
				{

					infLOG(ERROR,"필로그 뮤레카 결과 받기 오류 size : (%d) nMurekaCnt : (%d) \n", sizeof(MUREKA_VINFO)*nMurekaCnt, nMurekaCnt);

					// 받던 파일 삭제
					///////////////////////////////////////////////
					// temp 삭제									 //
					///////////////////////////////////////////////
					memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
					pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
					pcom9104_r.id         = pFileinfo->cfups4001.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
					pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
					memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자

					infLOG(ERROR, "필로그 용량을 복구 합니다. File Size [ %.0f ]\n",pcom9104_r.file_size);
					if(com9104(pcom9104_r) < 0)
					{
						infLOG(ERROR, "필로그 용량 복구 중 오류가 발생하였습니ㅏ다.[com9104]\n");
					}

					com9101 ( com9101_r);
					return 0;
				}

				#ifdef __DEBUG
				for(int i=0; i < nMurekaCnt; i++)
				{
					printf("필로그 뮤레카 정보 확인(%d).\n", i);
					printf("video_status : %d\n",pMurekaVInfo[i].nResultCode);
					printf("video_status : %s\n",pMurekaVInfo[i].filename);
					printf("video_status : %s\n",pMurekaVInfo[i].mureka_hash);
					printf("video_status : %s\n",pMurekaVInfo[i].video_status);
					printf("video_id : %s\n",pMurekaVInfo[i].video_id);
					printf("video_title : %s\n",pMurekaVInfo[i].video_title);
					printf("video_jejak_year : %s\n",pMurekaVInfo[i].video_jejak_year);
					printf("video_right_name : %s\n",pMurekaVInfo[i].video_right_name);
					printf("video_right_content_id : %s\n",pMurekaVInfo[i].video_right_content_id);
					printf("video_grade : %s\n",pMurekaVInfo[i].video_grade);
					printf("video_price : %s\n",pMurekaVInfo[i].video_price);
					printf("video_cha : %s\n",pMurekaVInfo[i].video_cha);
				}
				#endif

			}

			////////////////////기본 정보 완료////////////////////////////////////////////////

			//2009/09/09(필로그 필터링 연동) 일반 컨테츠 업로드 로직의 4005, 4006부분 추가해야함.

			if( strcmp(pFileinfo->szCopyright_yn ,"P") == 0 )
			{
				infLOG(ALWAY,"저작권 flag 재성정 : P -> N \n");
				strcpy(pFileinfo->szCopyright_yn ,"N") ;
			}

			CFUPS4005 Fups4005;
			CFUPS4006 Fups4006;

			memset( &Fups4005, 0x00, sizeof(CFUPS4005));
			memset( &Fups4006, 0x00, sizeof(CFUPS4006));

			memset(szSubFilePath,0x00,sizeof(szSubFilePath));
			memset(szFolderPath,0x00,sizeof(szFolderPath));
			int depth = 0;
			int seq_no = 0;

			strcpy(Fups4005.cont_gu , "FD");
			Fups4005.id = pFileinfo->cfups4001.id;
			Fups4005.file_size = pFileinfo->dFileSize;
			strcpy(Fups4005.sect_code , pFileinfo->cfups4001.sect_code );
			strcpy(Fups4005.sect_sub , pFileinfo->cfups4001.sect_sub );
			if( pFileinfo->cfups4001.reg_user == NULL || strlen( pFileinfo->cfups4001.reg_user) <= 0 )
			{
				strcpy(Fups4005.user_id, pHeader->szUserID);
			}
			else
			{
				strcpy(Fups4005.user_id, pFileinfo->cfups4001.reg_user);
			}
			strcpy(Fups4005.folder_yn , pFileinfo->cfups4001.folder_yn );
			strcpy(Fups4005.default_hash , pFileinfo->szDefault_hash );
			strcpy(Fups4005.audio_hash , pFileinfo->szAudio_hash );
			strcpy(Fups4005.video_hash , pFileinfo->szVideo_hash );
			strcpy(Fups4005.copyright_yn , pFileinfo->szCopyright_yn );
			strcpy(Fups4005.mureka_yn , pFileinfo->szMureka_yn );

			//2009/06/14 뮤레카 조회 갯수.
			Fups4005.mureka_cnt = nMurekaCnt;


			infLOG(ALWAY,"fups4005 ] : id       	  [ %d ]     \n",Fups4005.id       		);
			infLOG(ALWAY,"fups4005 ] : seq_no	      [ %d ]     \n",Fups4005.seq_no	       );
			infLOG(ALWAY,"fups4005 ] : depth	      [ %d ]     \n",Fups4005.depth	       );
			infLOG(ALWAY,"fups4005 ] : file_size      [ %13.0f ]     \n",Fups4005.file_size     );
			infLOG(ALWAY,"fups4005 ] : sect_code      [ %s ]     \n",Fups4005.sect_code     );
			infLOG(ALWAY,"fups4005 ] : sect_sub       [ %s ]     \n",Fups4005.sect_sub     );
			infLOG(ALWAY,"fups4005 ] : folder_yn      [ %s ]     \n",Fups4005.folder_yn     );
			infLOG(ALWAY,"fups4005 ] : user_id        [ %s ]     \n",Fups4005.user_id       );
			infLOG(ALWAY,"fups4005 ] : folder_name    [ %s ]     \n",Fups4005.folder_name   );
			infLOG(ALWAY,"fups4005 ] : file_name      [ %s ] 	\n",Fups4005.file_name     );
			infLOG(ALWAY,"fups4005 ] : default_hash   [ %s ] 	\n",Fups4005.default_hash  );
			infLOG(ALWAY,"fups4005 ] : audio_hash	  [ %s ] 	\n",Fups4005.audio_hash	   );
			infLOG(ALWAY,"fups4005 ] : video_hash	  [ %s ] 	\n",Fups4005.video_hash	   );
			infLOG(ALWAY,"fups4005 ] : copyright_yn   [ %s ] 	\n",Fups4005.copyright_yn  );
			infLOG(ALWAY,"fups4005 ] : mureka_yn	  [ %s ] 	\n",Fups4005.mureka_yn  );



			if(strcmp(Fups4005.folder_yn,"Y")==0)
			{
				int nMoveLen = strlen(szFolderFullPath);
				int nDestLen = strlen(pFileinfo->szDownPath);

				if( strstr( pFileinfo->szDownPath ,szFolderFullPath ) != NULL  && nDestLen - nMoveLen > 0 )
				{
					memcpy(szSubFilePath,pFileinfo->szDownPath + nMoveLen ,  nDestLen - nMoveLen );
				}

				char* pTemp = strtok(szSubFilePath,"/");

				while(pTemp!=NULL )
				{
					depth ++ ;
					pTemp = strtok(NULL,"/");

					if(  pTemp != NULL)
					{
						strcat(szFolderPath ,pTemp);
						strcat(szFolderPath ,"/");
					}
				}
				if( depth > 0 )
					depth--;

				Fups4005.depth = depth;

				strcpy(Fups4005.folder_name, szFolderPath);
				strcpy(Fups4005.file_name , pFileinfo->szFileName);
			}
			else if(strcmp(Fups4005.folder_yn, "N") == 0)
			{
				Fups4005.seq_no = seq_no;
				seq_no++;
				strcpy(Fups4005.file_name , pFileinfo->cfups4001.file_name2);
			}
			Fups4005.depth = depth;

			//저작권 정보 재수정
			infLOG(ALWAY,"필로그 저작권 정보 확인 1 : tpye [ %d ] == [ %d ] : sect_code [ %s ] : copyright [ %s ] \n", pFileinfo->nType , FT_FOLDER , pFileinfo->cfups4001.sect_code , pFileinfo->szCopyright_yn);

			#ifdef __DEBUG
			printf("fups4005 ] : id       		[ %d ]     \n",Fups4005.id       		);
			printf("fups4005 ] : seq_no	        [ %d ]     \n",Fups4005.seq_no	       );
			printf("fups4005 ] : depth	        [ %d ]     \n",Fups4005.depth	       );
			printf("fups4005 ] : file_size      [ %13.0f ]     \n",Fups4005.file_size     );
			printf("fups4005 ] : sect_code      [ %s ]     \n",Fups4005.sect_code     );
			printf("fups4005 ] : sect_sub       [ %s ]     \n",Fups4005.sect_sub     );
			printf("fups4005 ] : folder_yn      [ %s ]     \n",Fups4005.folder_yn     );
			printf("fups4005 ] : user_id        [ %s ]     \n",Fups4005.user_id       );
			printf("fups4005 ] : folder_name    [ %s ]     \n",Fups4005.folder_name   );
			printf("fups4005 ] : file_name      [ %s ] 	\n",Fups4005.file_name     );
			printf("fups4005 ] : default_hash   [ %s ] 	\n",Fups4005.default_hash  );
			printf("fups4005 ] : audio_hash	    [ %s ] 	\n",Fups4005.audio_hash	   );
			printf("fups4005 ] : video_hash	    [ %s ] 	\n",Fups4005.video_hash	   );
			printf("fups4005 ] : copyright_yn   [ %s ] 	\n",Fups4005.copyright_yn  );
			printf("fups4005 ] : mureka_yn      [ %s ] 	\n",Fups4005.mureka_yn  );
			printf("fups4005 ] : cont_gu      [ %s ] 	\n",Fups4005.cont_gu  );
			#endif

			infLOG(ALWAY,"fups4005 데이터를 업데이트 중입니다.\n"	);
			infLOG(ALWAY,"fups4005 ] : id       	  [ %d ]     \n",Fups4005.id       		);
			infLOG(ALWAY,"fups4005 ] : seq_no	      [ %d ]     \n",Fups4005.seq_no	       );
			infLOG(ALWAY,"fups4005 ] : depth	      [ %d ]     \n",Fups4005.depth	       );
			infLOG(ALWAY,"fups4005 ] : file_size      [ %13.0f ]     \n",Fups4005.file_size     );
			infLOG(ALWAY,"fups4005 ] : sect_code      [ %s ]     \n",Fups4005.sect_code     );
			infLOG(ALWAY,"fups4005 ] : sect_sub       [ %s ]     \n",Fups4005.sect_sub     );
			infLOG(ALWAY,"fups4005 ] : folder_yn      [ %s ]     \n",Fups4005.folder_yn     );
			infLOG(ALWAY,"fups4005 ] : user_id        [ %s ]     \n",Fups4005.user_id       );
			infLOG(ALWAY,"fups4005 ] : folder_name    [ %s ]     \n",Fups4005.folder_name   );
			infLOG(ALWAY,"fups4005 ] : file_name      [ %s ] 	\n",Fups4005.file_name     );
			infLOG(ALWAY,"fups4005 ] : default_hash   [ %s ] 	\n",Fups4005.default_hash  );
			infLOG(ALWAY,"fups4005 ] : audio_hash	  [ %s ] 	\n",Fups4005.audio_hash	   );
			infLOG(ALWAY,"fups4005 ] : video_hash	  [ %s ] 	\n",Fups4005.video_hash	   );
			infLOG(ALWAY,"fups4005 ] : copyright_yn   [ %s ] 	\n",Fups4005.copyright_yn  );
			infLOG(ALWAY,"fups4005 ] : mureka_yn	  [ %s ] 	\n",Fups4005.mureka_yn  );

			Fups4006.id       		  =  Fups4005.id;
			Fups4006.seq_no	          =  Fups4005.seq_no;
			Fups4006.depth	          =  Fups4005.depth;
			Fups4006.file_size        =  Fups4005.file_size;
			strcpy(Fups4006.sect_code,  Fups4005.sect_code);
			strcpy(Fups4006.sect_sub, Fups4005.sect_sub);
			strcpy(Fups4006.folder_yn    , Fups4005.folder_yn);
			strcpy(Fups4006.user_id      , Fups4005.user_id);
			strcpy(Fups4006.folder_name  , Fups4005.folder_name);
			strcpy(Fups4006.file_name    , Fups4005.file_name);
			strcpy(Fups4006.default_hash , Fups4005.default_hash);
			strcpy(Fups4006.audio_hash	 ,  Fups4005.audio_hash);
			strcpy(Fups4006.video_hash	 ,  Fups4005.video_hash);
			strcpy(Fups4006.copyright_yn , Fups4005.copyright_yn);
			strcpy(Fups4006.mureka_yn    , Fups4005.mureka_yn);
			strcpy(Fups4006.cont_gu    , Fups4005.cont_gu);
			strcpy(Fups4006.auth_num    , com9004Result.auth_num );

			//2009/06/14 뮤레카 조회 갯수.
			Fups4006.mureka_cnt = nMurekaCnt;


			#ifdef __DEBUG
			printf("fups4006 ] : id       		[ %d ]     \n",Fups4006.id       		);
			printf("fups4006 ] : seq_no	        [ %d ]     \n",Fups4006.seq_no	       );
			printf("fups4006 ] : depth	        [ %d ]     \n",Fups4006.depth	       );
			printf("fups4006 ] : file_size      [ %13.0f ]     \n",Fups4006.file_size     );
			printf("fups4006 ] : sect_code      [ %s ]     \n",Fups4006.sect_code     );
			printf("fups4006 ] : sect_sub       [ %s ]     \n",Fups4006.sect_sub     );
			printf("fups4006 ] : folder_yn      [ %s ]     \n",Fups4006.folder_yn     );
			printf("fups4006 ] : user_id        [ %s ]     \n",Fups4006.user_id       );
			printf("fups4006 ] : folder_name    [ %s ]     \n",Fups4006.folder_name   );
			printf("fups4006 ] : file_name      [ %s ] 	\n",Fups4006.file_name     );
			printf("fups4006 ] : default_hash   [ %s ] 	\n",Fups4006.default_hash  );
			printf("fups4006 ] : audio_hash	    [ %s ] 	\n",Fups4006.audio_hash	   );
			printf("fups4006 ] : video_hash	    [ %s ] 	\n",Fups4006.video_hash	   );
			printf("fups4006 ] : copyright_yn   [ %s ] 	\n",Fups4006.copyright_yn  );
			printf("fups4006 ] : mureka_yn      [ %s ] 	\n",Fups4006.mureka_yn  );
			printf("fups4006 ] : cont_gu   		[ %s ] 	\n",Fups4006.cont_gu  );
			printf("fups4006 ] : auth_num       [ %s ] 	\n",Fups4006.auth_num  );
			#endif

			infLOG(ALWAY,"\n\nfups4006 ] : id       	  [ %d ]     \n",Fups4006.id       		);
			infLOG(ALWAY,"fups4006 ] : seq_no	      [ %d ]     \n",Fups4006.seq_no	       );
			infLOG(ALWAY,"fups4006 ] : depth	      [ %d ]     \n",Fups4006.depth	       );
			infLOG(ALWAY,"fups4006 ] : file_size      [ %13.0f ]     \n",Fups4006.file_size     );
			infLOG(ALWAY,"fups4006 ] : sect_code      [ %s ]     \n",Fups4006.sect_code     );
			infLOG(ALWAY,"fups4006 ] : sect_sub       [ %s ]     \n",Fups4006.sect_sub     );
			infLOG(ALWAY,"fups4006 ] : folder_yn      [ %s ]     \n",Fups4006.folder_yn     );
			infLOG(ALWAY,"fups4006 ] : user_id        [ %s ]     \n",Fups4006.user_id       );
			infLOG(ALWAY,"fups4006 ] : folder_name    [ %s ]     \n",Fups4006.folder_name   );
			infLOG(ALWAY,"fups4006 ] : file_name      [ %s ] 	\n",Fups4006.file_name     );
			infLOG(ALWAY,"fups4006 ] : default_hash   [ %s ] 	\n",Fups4006.default_hash  );
			infLOG(ALWAY,"fups4006 ] : audio_hash	  [ %s ] 	\n",Fups4006.audio_hash	   );
			infLOG(ALWAY,"fups4006 ] : video_hash	  [ %s ] 	\n",Fups4006.video_hash	   );
			infLOG(ALWAY,"fups4006 ] : copyright_yn   [ %s ] 	\n",Fups4006.copyright_yn  );
			infLOG(ALWAY,"fups4006 ] : mureka_yn	  [ %s ] 	\n",Fups4006.mureka_yn  );
			infLOG(ALWAY,"fups4006 ] : cont_gu   	  [ %s ] 	\n",Fups4006.cont_gu  );
			infLOG(ALWAY,"fups4006 ] : auth_num       [ %s ] 	\n",Fups4006.auth_num  );

			infLOG(ALWAY,"============ pFileinfo->cfups4001.copyright_yn [ %s ] \n",pFileinfo->cfups4001.copyright_yn);
			int nCopyRight = 0;
			int nCompany  = 0;

			if( strcmp(com9004Result.auth_num ,"CPR") != 0)
			{
				nCopyRight = fupsflog4005(Fups4005, pMurekaVInfo);	//저작권 조회
			}
			infLOG(ALWAY,"저작권 조회 결과 [ %d ] \n\n\n",nCopyRight);
			if( nCopyRight <= 0 )
				nCompany = fupsflog4006(Fups4006, pMurekaVInfo);	//저작권에 걸리지않는 자료라면 유료컨텐츠여부 조회.
			infLOG(ALWAY,"제휴 조회 결과   [ %d ] \n\n\n",nCompany  );


			if(pMurekaVInfo)
			{
				delete[] pMurekaVInfo;
				pMurekaVInfo = NULL;
			}

			// 20140523 : 보류 처리하기
			//infLOG(ALWAY,"============ cfups4001.copyright_yn [ %s ] \n",cfups4001.copyright_yn);
			//if(strcmp (pFileinfo->cfups4001.copyright_yn ,"B") != 0)
			//{
				strcpy( pFileinfo->cfups4001.copyright_yn , "N");

				if( bHaveCopyright  )
				{
					strcpy( pFileinfo->cfups4001.copyright_yn , "Y");
				}
				else
				{
					if( bHaveCompany )
						strcpy( pFileinfo->cfups4001.copyright_yn , "C");

					if( nCopyRight > 0   )
					{
						bHaveCopyright = true;
						strcpy( pFileinfo->cfups4001.copyright_yn , "Y");
					}
					else
					{
						if( nCompany > 0 )
						{
							bHaveCompany = true;
							strcpy( pFileinfo->cfups4001.copyright_yn , "C");

						}
					}

				}
			//}

			infLOG(ALWAY,"필로그 자료실 확인 : sect_code [ %s ] : copyright_yn [ %s ] \n" , pFileinfo->cfups4001.sect_code , pFileinfo->cfups4001.copyright_yn);

			if( nCopyRight < 0 )
			{
				infLOG(ERROR, "저작권 조회 오류입니다. Error Num [ %d ]\n",nCopyRight);

				pSendData = new char[sizeof(ERR_HEADER)];
				memset(pSendData,0x00,sizeof(ERR_HEADER));
				errheader.header.nCmd = RS_ERR;
				errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
				strcat(errheader.errmsg,"파일 정보 등록 오류 입니다.");

				memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

				pHeader->nCmd = RS_ERR;

				// 받던 파일 삭제
				///////////////////////////////////////////////
				// temp 삭제									 //
				///////////////////////////////////////////////
				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
				pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
				pcom9104_r.id         = pFileinfo->cfups4001.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
				pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자

				infLOG(ERROR, "필로그 용량을 복구 합니다. File Size [ %.0f ]\n",pcom9104_r.file_size);
				if(com9104(pcom9104_r) < 0)
				{
					infLOG(ERROR, "필로그 용량 복구 중 오류가 발생하였습니ㅏ다.[com9104]\n");
				}

				com9101 ( com9101_r);
				return -RS_FILE_DATA_TRANSFER;
			}

			// 파일 제어 및 전송

			FILE* DownloadFile; //파일 포인터
			DownloadFile = NULL;
			//// 파일 open형식 결정////

			infLOG(ALWAY,"파일열기 : [ %s ]\n",szFullName);

			if( bFOpenAppendMode) //append mode
			{
				DownloadFile = fopen64(szFullName,"ar+tb");
				infLOG(ALWAY, "append mode ( %s )\n",szFullName);

			}
			else
			{
				DownloadFile = fopen64(szFullName,"wr+tb");
				infLOG(ALWAY, "write mode ( %s )\n",szFullName);
			}

			if(DownloadFile == NULL) //파일을 열수 없으면
			{
				memset(szErrMsg,0x00,sizeof(szErrMsg));
				GetErrMsg(errno,szErrMsg);

				infLOG(ERROR, "파일 열기 오류 입니다. [ %s ] error num [ %d ] msg [ %s ] \n",szFullName,errno, szErrMsg);

				pSendData = new char[sizeof(ERR_HEADER)];
				memset(pSendData,0x00,sizeof(ERR_HEADER));
				errheader.header.nCmd = RS_ERR;
				errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
				strcat(errheader.errmsg,"서버에서 파일 만들기 실패 하였습니다.");

				memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

				pHeader->nCmd = RS_ERR;

				// 받던 파일 삭제
				///////////////////////////////////////////////
				// temp 삭제									 //
				///////////////////////////////////////////////

				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));


				pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
				pcom9104_r.id         = pFileinfo->cfups4001.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
				pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자


				infLOG(ERROR, "필로그 용량을 복구 합니다. File Size [ %.0f ]\n",pcom9104_r.file_size);
				if(com9104(pcom9104_r) < 0)
				{
					infLOG(ERROR, "필로그 용량 복구 중 오류가 발생하였습니ㅏ다.[com9104]\n");
				}

				com9101 ( com9101_r);
				return -RS_FILE_DATA_TRANSFER;
			}

			//// 이어 받기를 위한 파일 해더 구조체 생성 ////
			infLOG(ALWAY,"파일의 Seek 를 마지막 위치로 이동 합니다.\n");
			if(fseeko64(DownloadFile,0l,SEEK_END) < 0)
			{
				infLOG(ALWAY,"파일의 Seek 를 마지막 위치로 이동 중 류가 발생하였습니다. errno [ %d ] error msg [ %s ]\n",errno,strerror(errno));

				pSendData = new char[sizeof(ERR_HEADER)];
				memset(pSendData,0x00,sizeof(ERR_HEADER));
				errheader.header.nCmd = RS_ERR;
				errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
				strcat(errheader.errmsg,"서버 파일의 섹터 이동 실패 하였습니다.");

				memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

				pHeader->nCmd = RS_ERR;
				// 받던 파일 삭제
				///////////////////////////////////////////////
				// temp 삭제									 //
				///////////////////////////////////////////////

				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));

				pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
				pcom9104_r.id         = pFileinfo->cfups4001.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
				pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자

				infLOG(ERROR, "필로그 용량을 복구 합니다. File Size [ %.0f ]\n",pcom9104_r.file_size);
				if(com9104(pcom9104_r) < 0)
				{
					infLOG(ERROR, "필로그 용량 복구 중 오류가 발생하였습니ㅏ다.[com9104]\n");
				}

				com9101 ( com9101_r);
				return -RS_FILE_DATA_TRANSFER;
			}

			LPFILEHEAD pFileHead = new FILEHEAD;
			memset(pFileHead,0x00,sizeof(FILEHEAD));

			double dCurrentLen	= 0;

			dCurrentLen = (double)ftello64 (DownloadFile); // 파일이 어디까지 있는지 결정

			infLOG(ALWAY, "최근 이동한 파일 사이즈 ( %.0f )\n",dCurrentLen);

			if(dCurrentLen < 0)
				dCurrentLen = 0;

			pFileHead->dCurrentSize = dCurrentLen; //해드에 현 파일 길이 저장

			// head 작성
			memset(&headers,0x00,sizeof(HEADER));

			infLOG(ALWAY,"Send RS_FILE_DATA_TRANSFER\n");

			headers.nCmd = RS_FILE_DATA_TRANSFER ; // 데이터 전송
			headers.nDataCnt = 1;
			headers.nDataSize = sizeof(FILEHEAD);
			headers.nErrorCode = 0;

			pSendData = new char[sizeof(HEADER) + headers.nDataCnt*headers.nDataSize];

			memcpy(pSendData,&headers,sizeof(HEADER));

			memcpy(pSendData + HEADER_SIZE,pFileHead, headers.nDataCnt*headers.nDataSize);

			//// body 작성////
			if(	SendData(Socket,pSendData,HEADER_SIZE + headers.nDataCnt*headers.nDataSize)<0)  //struct _PACKET == PACKET
			{
				infLOG(ERROR,"Send RS_FILE_DATA_TRANSFER ERROR\n");
				delete pFileHead;
				// 용량 복원 하기
				// 받던 파일 삭제
				///////////////////////////////////////////////
				// temp 삭제									 //
				///////////////////////////////////////////////

				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));

				pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
				pcom9104_r.id         = pFileinfo->cfups4001.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
				pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자

				infLOG(ERROR, "필로그 용량을 복구 합니다. File Size [ %.0f ]\n",pcom9104_r.file_size);
				if(com9104(pcom9104_r) < 0)
				{
					infLOG(ERROR, "필로그 용량 복구 중 오류가 발생하였습니ㅏ다.[com9104]\n");
				}


				com9101 ( com9101_r);
				return 0;
			}
			delete[] pSendData;
			pSendData = NULL;

			delete pFileHead;

			infLOG(ALWAY,"Send RS_FILE_DATA_TRANSFER OK \n");

		 ///////////////////////// 데이터 전송 //////////////////////////////////

			dTotalRecvLen = 0; //총 받은 길이
			dTotalLen = pFileinfo->dFileSize - dCurrentLen; // down될 파일의 총 길이
			nWriteLen=0;
			nRecvLen=0;

			char* szRecvBuffer = new char[RECVBUF]; //recv buffer

			infLOG(ALWAY,"파일 확인 [ %s ] : 받을 파일 전체 길이 [ %.0f ] = [ %.0f (전체) - %.0f(최근이동한) ] \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize ,dCurrentLen);

			int fno = fileno(DownloadFile);

			while(dTotalLen > 0  )
			{
				memset(szRecvBuffer,0x00,RECVBUF);
				///// 파일받기 /////

				nRecvLen =  RecvFileData(Socket, szRecvBuffer, RECVBUF, dTotalLen) ;

		        if(nRecvLen > 0)
		        {
		        	nWriteLen = write(fno ,szRecvBuffer,nRecvLen);
		      	   	//nWriteLen = fwrite(szRecvBuffer,nRecvLen,1,DownloadFile);
		        }
		        else
		        	nWriteLen = 0;

		        //	fwrite(szRecvBuffer,1,nRecvLen,DownloadFile); //받은 길이 만큼 file에 저장

		    	if(nWriteLen <= 0)
	        	{


	        		if(nWriteLen == 0)
	        		{
	        			#ifdef __DEBUG
	        			printf(" ] Write File End (%s) : 파일 전체 길이 (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
	        			#endif
	        			infLOG(ALWAY,"Write File End (%s) : 파일 전체 길이 (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
	        		}
	        		else
	        		{
	        			#ifdef __DEBUG
	        			printf(" ] Write File ERROR (%s) : 파일 전체 길이 (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
	        			#endif
	        			infLOG(ERROR," ] Write File ERROR (%s) : 파일 전체 길이 (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
	        			nRecvLen = -1;
	        		}
	        		infLOG(ERROR,"errno [ %d ] error msg [ %s ]\n",errno,strerror(errno));
	        	}

		        if(nRecvLen <= 0 && dTotalLen != 0)	//받다가 오류가 났을시...DB처리
		        {


					if(nRecvLen < 0)
		        	{
						memset(szErrMsg,0x00,sizeof(szErrMsg));
						GetErrMsg(-nRecvLen,szErrMsg);
						infLOG(ERROR, "데이터를 발을 수 없습니다. ( %d )( %s )\n",nRecvLen,szErrMsg);
		        	}
		        	else if(nRecvLen == 0)
		        	{
						memset(szErrMsg,0x00,sizeof(szErrMsg));
						GetErrMsg(-nRecvLen,szErrMsg);

		        		infLOG(ERROR, "접속이 끊어 졌습니다.[ 이경우는 보통 클라이언트에서 데이터를 제대로 보내지 못할때 발생합니다. ] \n" );

		        	}


					infLOG(ERROR,"필로그 취소 (%s) RecvLen (%d) (%15.0f) TotalLen(%15.0f)\n ",pFileinfo->cfups4001.file_name2,nRecvLen ,dTotalRecvLen,dTotalLen);
					infLOG(ERROR,"errno [ %d ] error msg [ %s ]\n",errno,strerror(errno));

					if(DownloadFile)
					{
						fclose(DownloadFile);
						DownloadFile == NULL ;
					}

					memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));

					pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
					pcom9104_r.id         = pFileinfo->cfups4001.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
					pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
					memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자

					infLOG(ERROR, "필로그 용량을 복구 합니다. File Size [ %.0f ]\n",pcom9104_r.file_size);
					if(com9104(pcom9104_r) < 0)
					{
						infLOG(ERROR, "필로그 용량 복구 중 오류가 발생하였습니ㅏ다.[com9104]\n");
					}

					////////////////////////////////////////////////

				   	if(szRecvBuffer)
						delete[] szRecvBuffer;

					com9101 ( com9101_r);
					return 0;
					//	return END;
	        	}
	       		dTotalLen = dTotalLen - (double)nRecvLen;  //총길이에서  받은 길이 제거
	        	dTotalRecvLen = dTotalRecvLen + (double)nRecvLen; //받은 길이 만큼 더함
			}

			if(DownloadFile)
			{
				fclose(DownloadFile);
				DownloadFile == NULL ;
			}

			if(	szRecvBuffer)
				delete[] szRecvBuffer;

			///////////////////////////////////////////////
			//파일 이름 바꾸기
			// DB 넣기..

			infLOG(ALWAY,"필로그 데이터 받기 완료 및 확인 - 파일이름 (%s) 임시번호 (%lu) 임시번호 (%lu)\n",pFileinfo->cfups4001.file_name2, pFileinfo->nNumber,pFileinfo->cfups4001.id);

			//여기서 부터 받기 시작

			infLOG(ALWAY,"전송 완료 후 해더 응답 대기.\n");
			memset(&headers,0x00,sizeof(HEADER));
			if(RecvData(Socket,(char*)&headers,HEADER_SIZE ) <= 0)
			{
				infLOG(ERROR,"전송 완료 후 해더 응답 대기 오류.\n");

				// 받던 파일 삭제
				///////////////////////////////////////////////
				// temp 삭제									 //
				//////////////////////////////////////////////
				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));

				pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
				pcom9104_r.id         = pFileinfo->cfups4001.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
				pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자

				infLOG(ERROR, "필로그 용량을 복구 합니다. File Size [ %.0f ]\n",pcom9104_r.file_size);
				if(com9104(pcom9104_r) < 0)
				{
					infLOG(ERROR, "필로그 용량 복구 중 오류가 발생하였습니ㅏ다.[com9104]\n");
				}

				com9101 ( com9101_r);

				return 0;
			}
			infLOG(ERROR,"전송 완료 후 해더 응답 대기 결과 [ %d ].\n",headers.nCmd);

			if(headers.nCmd == RS_FILE_REQUEST_NEXT_FILE )
			{

				infLOG(ALWAY, "RS_FILE_REQUEST_NEXT_FILE\n다음 파일을 받습니다.\n");

				memset(&headers,0x00,HEADER_SIZE);

				headers.nCmd = RS_FILE_REQUEST_NEXT_FILEINFO; //파일 정보 요청
				headers.nDataCnt = 0;
				headers.nDataSize = 0;

				if(	SendData(Socket,(char*)&headers,HEADER_SIZE )<0)  //struct _PACKET == PACKET
				{
					infLOG(ERROR, "RS_FILE_REQUEST_NEXT_FILE 전송 오류 \n");


					// 받던 파일 삭제
					///////////////////////////////////////////////
					// temp 삭제									 //
					///////////////////////////////////////////////

					memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));

					pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
					pcom9104_r.id         = pFileinfo->cfups4001.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
					pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
					memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자

					infLOG(ERROR, "필로그 용량을 복구 합니다. File Size [ %.0f ]\n",pcom9104_r.file_size);
					if(com9104(pcom9104_r) < 0)
					{
						infLOG(ERROR, "필로그 용량 복구 중 오류가 발생하였습니ㅏ다.[com9104]\n");
					}


					com9101 ( com9101_r);

					return 0;
				}
				infLOG(ERROR, "RS_FILE_REQUEST_NEXT_FILE 응답 대기 중 \n");

				//recv file_transfer
				memset(&headers,0x00,HEADER_SIZE);
				if(RecvData(Socket,(char*)&headers,HEADER_SIZE ) <= 0)
				{
					infLOG(ERROR, "RS_FILE_REQUEST_NEXT_FILE 응답 대기 중 오류 [ 이경우는 보통 클라이언트로부터 데이터를 받지 못할 때 발생합니다. \n");
					// 받던 파일 삭제
					///////////////////////////////////////////////
					// temp 삭제									 //
					///////////////////////////////////////////////

					memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));

					pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
					pcom9104_r.id         = pFileinfo->cfups4001.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
					pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
					memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자

					infLOG(ERROR, "필로그 용량을 복구 합니다. File Size [ %.0f ]\n",pcom9104_r.file_size);
					if(com9104(pcom9104_r) < 0)
					{
						infLOG(ERROR, "필로그 용량 복구 중 오류가 발생하였습니ㅏ다.[com9104]\n");
					}


					com9101 ( com9101_r);
					return 0;
				}

				memset(&rFileInfo,0x00,sizeof(FILEINFO));
				infLOG(ERROR, "다음 파일정보 받기 대기 중 \n");

				if( RecvData(Socket,(char*)&rFileInfo,sizeof(FILEINFO) ) <= 0)
				{
					infLOG(ERROR, "다음 파일정보 받기 대기 중 오류 \n");

					// 받던 파일 삭제
					///////////////////////////////////////////////
					// temp 삭제									 //
					///////////////////////////////////////////////

					memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));

					pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
					pcom9104_r.id         = pFileinfo->cfups4001.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
					pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
					memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자

					infLOG(ERROR, "필로그 용량을 복구 합니다. File Size [ %.0f ]\n",pcom9104_r.file_size);
					if(com9104(pcom9104_r) < 0)
					{
						infLOG(ERROR, "필로그 용량 복구 중 오류가 발생하였습니ㅏ다.[com9104]\n");
					}


					com9101 ( com9101_r);
					return 0;
				}
				pFileinfo = &rFileInfo;
			}
			else if(headers.nCmd == RS_EOL)
			{
				//등록

				infLOG(ALWAY,"RS_EOL\n필로그의 컨텐츠를 등록합니다. 임시 번호 검사(T_CONTENTS_TEMP) [ %lu ]\n",pFileinfo->nNumber );

				// 등록후 eol 보내기
				if(pFileinfo->nTypeDisk == FT_WEDISK && pFileinfo->nNumber > 0 ) //컨텐츠 등록
				{
					if( dTotalLen == 0)
					{
						infLOG(ALWAY,"필로그 파일 등록 - 임시번호 [ %lu ] 파일이름  [ %s ]  \n",pFileinfo->nNumber,pFileinfo->cfups4001.file_name2);

						int nResult = fups4001(pFileinfo->cfups4001);
						infLOG(ALWAY,"필로그 등록결과(fups4001) Result [ %d ] \n",nResult);
						if(  nResult < 0 )//pFileinfo->cfups4001) == -1)
						{
							// 받던 파일 삭제
							///////////////////////////////////////////////
							// temp 삭제									 //
							///////////////////////////////////////////////

							//필로그 등록중 오류 발생 ...꼭 삭제 해야 할 목록들

							infLOG(ERROR, "================== 필로그 등록 오류(FilogError) ===================\n"
										  "임시번호 ( %lu )서버 아이디( %s ) 파일경로 ( %s )                         \n"
										  "=========================================================\n" ,pFileinfo->nNumber,pFileinfo->cfups4001.server_id ,szFullPath);

							memset(&headers,0x00,sizeof(HEADER));

							headers.nCmd = RS_FILE_END_FAIL;
							headers.nDataCnt = 0;
							headers.nDataSize = 0;
							headers.nErrorCode = 4001;

							if( nResult == -2)
								headers.nErrorCode = 400199;

							memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));


							pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
							pcom9104_r.id         = pFileinfo->cfups4001.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
							pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
							memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자

							infLOG(ALWAY,"RS_FILE_END_FAIL 전송 \n");


							if(	SendData(Socket,(char*)&headers,HEADER_SIZE)<0)  //struct _PACKET == PACKET
							{
								infLOG(ERROR, "필로그 용량을 복구 합니다. File Size [ %.0f ]\n",pcom9104_r.file_size);
								if(com9104(pcom9104_r) < 0)
								{
									infLOG(ERROR, "필로그 용량 복구 중 오류가 발생하였습니ㅏ다.[com9104]\n");
								}

								com9101 ( com9101_r);
								return 0;
							}
							infLOG(ERROR, "필로그 용량을 복구 합니다. File Size [ %.0f ]\n",pcom9104_r.file_size);
							if(com9104(pcom9104_r) < 0)
							{
								infLOG(ERROR, "필로그 용량 복구 중 오류가 발생하였습니ㅏ다.[com9104]\n");
							}


							com9101 ( com9101_r);
							return 1;
						}
						infLOG(ALWAY,"============== 필로그 파일 등록 완료 ===============\n");
					}
					else
					{
						infLOG(ERROR, "============ 필로그 등록 오류 - 파일을 완전히 받지 못하였습니다. ========== \n");
						memset(&headers,0x00,sizeof(HEADER));

						// 받던 파일 삭제

						headers.nCmd = RS_FILE_END_FAIL;
						headers.nDataCnt = 0;
						headers.nDataSize = 0;
						headers.nErrorCode = 4001;

						if(	SendData(Socket,(char*)&headers,HEADER_SIZE)<=0)  //struct _PACKET == PACKET
						{
							infLOG(ERROR, "필로그 용량을 복구 합니다. File Size [ %.0f ]\n",pcom9104_r.file_size);
							if(com9104(pcom9104_r) < 0)
							{
								infLOG(ERROR, "필로그 용량 복구 중 오류가 발생하였습니ㅏ다.[com9104]\n");
							}

							com9101 ( com9101_r);
							return 0;
						}
						infLOG(ERROR, "필로그 용량을 복구 합니다. File Size [ %.0f ]\n",pcom9104_r.file_size);
						if(com9104(pcom9104_r) < 0)
						{
							infLOG(ERROR, "필로그 용량 복구 중 오류가 발생하였습니ㅏ다.[com9104]\n");
						}


						com9101 ( com9101_r);
						return 1;
					}
				}


				infLOG(ALWAY, "RS_EROL 전송\n");

				memset(&headers,0x00,HEADER_SIZE);

				headers.nCmd = RS_EOL; //파일 정보 요청
				headers.nDataCnt = 0;
				headers.nDataSize = 0;

				if(	SendData(Socket,(char*)&headers,HEADER_SIZE )<0)  //struct _PACKET == PACKET
				{
					infLOG(ERROR, "필로그 용량을 복구 합니다. File Size [ %.0f ]\n",pcom9104_r.file_size);
					if(com9104(pcom9104_r) < 0)
					{
						infLOG(ERROR, "필로그 용량 복구 중 오류가 발생하였습니ㅏ다.[com9104]\n");
					}

					com9101 ( com9101_r);
					return 0;
				}
				com9101 ( com9101_r);
				return 0;
			}
			else
			{
				infLOG(ERROR,"전송 완료 후 해더 응답 대기 오류 [ %d ]명령어가 없습니다.\n",headers.nCmd);

				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));

				pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
				pcom9104_r.id         = pFileinfo->cfups4001.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
				pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자

				infLOG(ERROR, "필로그 용량을 복구 합니다. File Size [ %.0f ]\n",pcom9104_r.file_size);
				if(com9104(pcom9104_r) < 0)
				{
					infLOG(ERROR, "필로그 용량 복구 중 오류가 발생하였습니ㅏ다.[com9104]\n");
				}


				com9101 ( com9101_r);
				return 0;
			}

		}while( 1 );

		com9101 ( com9101_r);
	}
	else if( nCType == -2)  //하루에 한건 - 현재 사용안함
	{
		infLOG(ERROR,"성인물 컨텐츠는 하루에 두건만 등록 가능 합니다.");
		pSendData = new char[sizeof(ERR_HEADER)];
		memset(pSendData,0x00,sizeof(ERR_HEADER));
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;

		strcat(errheader.errmsg,"성인물 컨텐츠는 하루에 두건만 등록 가능 합니다.");

		memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

		pHeader->nCmd = RS_ERR;

		return -RS_FILE_DATA_TRANSFER;
	}
	else if( nCType == -3)  //파일 사이즈 변경되었음.
	{
		infLOG(ERROR,"업로드 파일이 변경 되었습니다.");

		pSendData = new char[sizeof(ERR_HEADER)];
		memset(pSendData,0x00,sizeof(ERR_HEADER));
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;

		strcat(errheader.errmsg,"업로드 파일이 변경 되었습니다.");

		memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

		pHeader->nCmd = RS_ERR;

		return -RS_FILE_DATA_TRANSFER;
	}
	else if( nCType == -90042 ) //파일 정보 조회 오류
	{
		infLOG(ERROR,"파일 정보 조회 중 오류가 발생하였습니다.잠시 후 재시도 해주십시오.");
		pSendData = new char[sizeof(ERR_HEADER)];
		memset(pSendData,0x00,sizeof(ERR_HEADER));
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;

		strcat(errheader.errmsg,"파일 정보 조회 중 오류가 발생하였습니다.잠시 후 재시도 해주십시오.");

		memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

		pHeader->nCmd = RS_ERR;


		return -RS_FILE_DATA_TRANSFER;
	}
	else if( nCType == -1 )
	{
		infLOG(ERROR,"등록한 파일 정보를 찾을 수 없습니다.");

		pSendData = new char[sizeof(ERR_HEADER)];
		memset(pSendData,0x00,sizeof(ERR_HEADER));
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;

		strcat(errheader.errmsg,"등록한 파일 정보를 찾을 수 없습니다.");

		memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

		pHeader->nCmd = RS_ERR;


		return -RS_FILE_DATA_TRANSFER;
	}
	else if( nCType == 1 ) //위디스크
	{

		infLOG(ALWAY,"위디스크 등록 시작.");
		//9001 호출 // 사용자수 증가
		//9101 호출 //사용자수 감소

		CCOM9001_R com9001_r ;
		memset(&com9001_r,0x00,sizeof(CCOM9001_R));

		multimap<int,USERINFO>::iterator mi; //IP 조회
		//mi = m_UserList.begin();
		mi = m_UserList.find(Socket);
		if(mi != m_UserList.end())
		{
			strcpy(com9001_r.conn_ip ,mi->second.thread.userIP);
		}

		strcpy(com9001_r.cont_gu ,"WE");
		strcpy(com9001_r.server_id , pFileinfo->szServerID);
		com9001_r.temp_id =  pFileinfo->cfups4001.id;
		memcpy(com9001_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
		com9001_r.upload_size = pFileinfo->cfups4001.file_size;


		infLOG(ALWAY,"9001 Checking\n"
					"com9001_r.conn_ip [ %s ] \n"
					"com9001_r.cont_gu [ %s ] \n"
					"com9001_r.serv_id [ %s ] \n"
					"com9001_r.temp_id [ %d ] \n"
					"com9001_r.user_id [ %s ] \n"
					"com9001_r.up_size [ %15.0f ] \n"
					, com9001_r.conn_ip,com9001_r.cont_gu,com9001_r.server_id
					, com9001_r.temp_id , com9001_r.user_id , com9001_r.upload_size );

		com9001 ( com9001_r);

		infLOG(ALWAY, " CCOM9101_R Setting ...   ]\n");

		CCOM9101_R com9101_r ;
		memset(&com9101_r,0x00,sizeof(CCOM9101_R));
		strcpy(com9101_r.conn_ip , com9001_r.conn_ip);
		strcpy(com9101_r.server_id , com9001_r.server_id);
		com9101_r.temp_id =  com9001_r.temp_id;
		strcpy(com9101_r.user_id ,com9001_r.user_id); // 사용자
		com9101_r.upload_size = com9001_r.upload_size;



		char szFullPath[768];
		memset(szFullPath,0x00,sizeof(szFullPath));

		char szFullName[768];
		memset(szFullName,0x00,sizeof(szFullName));

		int stat = -1;                 // 파일 상태 결정
		bool bFOpenAppendMode = false; // 파일 append 모드 결정

		CCOM9104_R pcom9104_r; // 받던 파일 취소쉬 DB 돌리기 ( T_CONTENTS_TEMP 삭제 )

		FILEINFO rFileInfo;

		double dTotalRecvLen = 0; //총 받은 길이
		double dTotalLen = 0; // down될 파일의 총 길이
		int nWriteLen=0;      // 파일에 write 한 크기
		int nRecvLen=0;       // 소켓으로 recv 한 크기
		int nCheckStop = 0; //while 루프 제어

		CCOM9105_R com9105_r;		// temp 에 파일 정보 저장.
		memset(&com9105_r,0x00,sizeof(CCOM9105_R));

		if(pFileinfo->nType == FT_FOLDER)
		{
			infLOG(ALWAY,"폴더 업로드 입니다.\n");
			//9105 폴더
			memset(&com9105_r,0x00,sizeof(CCOM9105_R));

			com9105_r.id = pFileinfo->cfups4001.id;
			memcpy(&com9105_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
			strcpy(com9105_r.server_id ,pFileinfo->szServerID);
			strcpy(com9105_r.sfile_path ,pFileinfo->cfups4001.file_path);
			strcpy(com9105_r.sfile_name ,pFileinfo->cfups4001.file_name1);

			com9105(com9105_r);
		}

		do
		{
			bGhostMode = false;

			nCheckStop++;
			if(nCheckStop >= 1100)
			{
				infLOG(ERROR, "업로그 갯수를 초과 하였습니다.\ntemp_id [ %lu ]file count = %d \n",pFileinfo->cfups4001.id,nCheckStop );

				//받던 파일 삭제

				com9101 ( com9101_r);
				return 0;

			}

			infLOG(ALWAY,"이어 올리기 Flag[ %d ] >> [ 1 , 2 는 재시도 0 은 일반 ] \n",pFileinfo->nReUploadFlag);

		    headers.nCmd  = RS_FILE_DATA_SIGN_CHECK; // 파일 전송 메세지


			if(pFileinfo->nReUploadFlag == RECONNECT_UPLOAD || pFileinfo->nReUploadFlag == RE_UPLOAD)
			{

				if( pFileinfo->nType == FT_FOLDER)
				{
					strcpy(szFullPath, pFileinfo->cfups4001.file_path); //./2004/02/18/16/raid
					strcat(szFullPath,"/");

					memset(szFolderFullPath,0x00,sizeof(szFolderFullPath));
					strcpy(szFolderFullPath, szFullPath); //./2004/02/18/16/raid


					strcat(szFullPath,pFileinfo->cfups4001.file_name1);//<- 요거 값 잘못 들어옴

					//////////////////////////////////////////////////////////////////////////

					strcpy(szFullName,pFileinfo->szDownPath); //szDownPath 은 ./raid
	    			strcat(szFullName,"/");
	    			strcat(szFullName,pFileinfo->szFileName); //szfilename 은 a.txt

	    			infLOG(ALWAY, "폴더 이어 올리기 - 위치 [ %s ] 최종 위치 [ %s ]\n",szFullPath,szFullName);

				}
				else
				{
			    	strcpy(szFullName,pFileinfo->szDownPath);
					strcat(szFullName,"/"); //./2004/02/18/16/raid/   <-- '/' 추가
					strcat(szFullName,pFileinfo->szFileName);

					strcpy(szFullPath, pFileinfo->cfups4001.file_path); //./2004/02/18/16/raid
					strcat(szFullPath,"/");
					strcat(szFullPath,pFileinfo->cfups4001.file_name1);//

					infLOG(ALWAY, "파일 이어 올리기 - 위치 [ %s ] 최종 위치 [ %s ]\n",szFullPath,szFullName);


					//9105 파일
					memset(&com9105_r,0x00,sizeof(CCOM9105_R));

					com9105_r.id = pFileinfo->cfups4001.id;
					memcpy(&com9105_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
					strcpy(com9105_r.server_id ,pFileinfo->szServerID);
					strcpy(com9105_r.sfile_path ,pFileinfo->szDownPath);
					strcpy(com9105_r.sfile_name ,pFileinfo->szFileName);

					com9105(com9105_r);

				}



				stat = stat64(szFullName,&statbuf);
				if(stat != 0) //파일이 없으면 폴더 만들기.
				{
					MakeFolder(pFileinfo->szDownPath) ;
					infLOG(ALWAY,"폴더가 없음으로 폴더를 생성합니다. [ %s ] \n",pFileinfo->szDownPath);
				}
				else
				{
					infLOG(ALWAY,"폴더가 이미 존재 합니다. Append 모드로 파일을 제공합니다. [ %s ] \n",szFullName);
					bFOpenAppendMode = true;
				}
			}
			else
			{
				infLOG(ALWAY,"일반 업로드 모드 입니다.\n" );

		    	srand((unsigned int)time(NULL))	; //random 이름을 위함 시드 지정


				///// 날짜 시간 생성 ////
				time_t			curtime;
				struct tm		*stm;
				time( &curtime );
				stm = (struct tm *) localtime(&curtime);

				localtime_r(&curtime, stm);
				bool bResult = false;


		  		if( pFileinfo->nType == FT_FILE)
		  		{
		  			infLOG(ALWAY,"파일 업로드 입니다.\n");

		  			infLOG(ALWAY,"파일 Root Path 는 [ %s ] 입니다.\n",pFileinfo->szDownPath);

					sprintf(pFileinfo->szDownPath,"%s/%04d/%02d/%02d/%02d",  pFileinfo->szDownPath
										,  stm->tm_year+1900
										,  stm->tm_mon + 1
										,  stm->tm_mday
										,  stm->tm_hour);//./2004/02/18/16


					infLOG(ALWAY,"파일 Root Path 를 설정합니다. [ %s ]\n",pFileinfo->szDownPath);

					memset(szFullName,0x00,sizeof(szFullName));

			    	strcpy(szFullName,pFileinfo->szDownPath);
					strcat(szFullName,"/"); //./2004/02/18/16/raid/   <-- '/' 추가

			    	//file name 얻기

			    	char szFilename[50];
			    	char szFileType[10];
			    	memset(szFilename,0x00,sizeof(szFilename));
			    	memset(szFileType,0x00,sizeof(szFileType));


					sprintf(szFilename,"temp%lu",pFileinfo->cfups4001.id);
			    	//local 파일이름으로 부터 확장자 얻기.
			    	int nLen = GetReverseIndex(pFileinfo->cfups4001.file_name2 , '.');
					//	nLen = nLen - 1; // a.txt -> for .txt 하기 위해 nLen -1 해줌
					//	nLen = nLen - 1; // ./raid/ -> ,./raid   , '/' delete
					infLOG(ALWAY, "파일 이름 검사 [ %s ] \n",pFileinfo->cfups4001.file_name2);

					if(nLen < 0)
					{
						infLOG(ALWAY, "파일 이름의 확장자가 없습니다. [ 무시 ]\n");
					}
					else
					{
					    GetRightString(pFileinfo->cfups4001.file_name2,strlen(pFileinfo->cfups4001.file_name2)-nLen,szFileType);
					    infLOG(ALWAY, "파일 확장자 검사 [ %s ] \n",szFileType);
					}
						//GetRightString(pFileinfo->szFileName,strlen(pFileinfo->szFileName)-nLen,szFileType);


					strcpy(pFileinfo->cfups4001.file_name2,pFileinfo->szFileName);
					memset(pFileinfo->szFileName,0x00,sizeof(pFileinfo->szFileName));
					strcpy(pFileinfo->szFileName,szFilename);
					strcat(pFileinfo->szFileName,szFileType);
					strcat(szFullName,szFilename);
					strcat(szFullName,szFileType);

					//// 이름 저장 ////
					memcpy(pFileinfo->cfups4001.file_name1,pFileinfo->szFileName,sizeof(pFileinfo->szFileName));
					strcpy(pFileinfo->cfups4001.file_path,pFileinfo->szDownPath);//,sizeof(pFileinfo->szDownPath));




					stat = stat64(szFullName,&statbuf);


					if(stat != 0) //파일이 없으면 폴더 만들기.
					{

						MakeFolder(pFileinfo->szDownPath) ;
						infLOG(ALWAY,"폴더가 없음으로 폴더를 생성합니다. [ %s ] \n",pFileinfo->szDownPath);
					}
					else
					{
						infLOG(ALWAY,"폴더가 이미 존재 합니다. Append 모드로 파일을 제공합니다. [ %s ] \n",szFullName);
						bFOpenAppendMode = true;
					}




					//9105 파일
					memset(&com9105_r,0x00,sizeof(CCOM9105_R));

					com9105_r.id = pFileinfo->cfups4001.id;
					memcpy(&com9105_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
					strcpy(com9105_r.server_id ,pFileinfo->szServerID);
					strcpy(com9105_r.sfile_path ,pFileinfo->cfups4001.file_path);
					strcpy(com9105_r.sfile_name ,pFileinfo->cfups4001.file_name1);

					com9105(com9105_r);

				}
				else if(pFileinfo->nType == FT_FOLDER)//전송 받을 파일이 폴더 일경우
				{

					infLOG(ALWAY,"폴더 업로드 입니다.\n");

					strcpy(szFullPath, pFileinfo->cfups4001.file_path); //./2004/02/18/16/raid
					strcat(szFullPath,"/");

					memset(szFolderFullPath,0x00,sizeof(szFolderFullPath));
					strcpy(szFolderFullPath, szFullPath); //./2004/02/18/16/raid

					strcat(szFullPath,pFileinfo->cfups4001.file_name1);//

					//////////////////////////////////////////////////////////////////////////

					strcpy(szFullName,pFileinfo->szDownPath); //szDownPath 은 ./raid
	    			strcat(szFullName,"/");
	    			strcat(szFullName,pFileinfo->szFileName); //szfilename 은 a.txt



					stat = stat64(szFullName,&statbuf);

	    			#ifdef __DEBUG
					printf(" ] FOLDER full path ( %s ) full name ( %s ) (%d)\n",szFullPath,szFullName,stat);
					#endif

					if(stat != 0) //파일이 없으면 폴더 만들기.
					{
						MakeFolder(pFileinfo->szDownPath) ;
						infLOG(ALWAY,"폴더가 없음으로 폴더를 생성합니다. [ %s ] \n",pFileinfo->szDownPath);
					}
					else
					{
						infLOG(ALWAY,"폴더가 이미 존재 합니다. Append 모드로 파일을 제공합니다. [ %s ] \n",szFullName);
						bFOpenAppendMode = true;
					}


				}
			}
	//		}while(bCreateFile != true) // 같은 이름이 있으면 roof안으로..

			headers.nCmd = RS_FILE_DATA_SIGN_CHECK; //파일 전송
			int nSRet = 0;


			if( pFileinfo->nType == FT_FILE )
			{
				infLOG(ALWAY,"Send RS_FILE_DATA_SIGN_CHECK - sizeof(FILEINFO) [%d]\n",sizeof(FILEINFO));

			    headers.nDataCnt = 1;
				headers.nDataSize = sizeof(FILEINFO);
				headers.nErrorCode = 0;

				char szSendData[HEADER_SIZE + sizeof(FILEINFO)];
				memset(szSendData,0x00,HEADER_SIZE + sizeof(FILEINFO));

				/*
				pSendData = new char[HEADER_SIZE + headers.nDataCnt*headers.nDataSize];
				memset(pSendData,0x00,HEADER_SIZE + headers.nDataCnt*headers.nDataSize);
				memcpy(pSendData,&headers,HEADER_SIZE);
				memcpy(pSendData + HEADER_SIZE , pFileinfo , sizeof(FILEINFO));
				nSRet = SendData(Socket,pSendData,HEADER_SIZE + headers.nDataCnt*headers.nDataSize);
				*/

				memcpy(szSendData,&headers,HEADER_SIZE);
				memcpy(szSendData + HEADER_SIZE , pFileinfo , sizeof(FILEINFO));

				nSRet = SendData(Socket,szSendData,HEADER_SIZE + headers.nDataCnt*headers.nDataSize);

				//delete[] pSendData;
				//pSendData = NULL;

			}
			else
			{
				infLOG(ALWAY,"Send RS_FILE_DATA_SIGN_CHECK\n");

			    headers.nDataCnt = 0;
				headers.nDataSize = 0;
				headers.nErrorCode = 0;
				nSRet = SendData(Socket,(char*)&headers,sizeof(struct _HEADER));
			}


			//server file 내용 보내기



		    //// 전송하기전에 메세지를 알림...
		    if(	nSRet <=0 )  //struct _PACKET == PACKET
			{
				infLOG(ERROR, "RS_FILE_DATA_SIGN_CHECK 전송 오류.\n");

				// 받던 파일 삭제
				///////////////////////////////////////////////
				// temp 삭제									 //
				///////////////////////////////////////////////


				com9101 ( com9101_r);
				return 0;
			}


			//	HEADER recvHeader;

			// 이부분 확인 하기 .......................

			memset(&headers,0x00,sizeof(HEADER));

			if(	RecvData(Socket,(char*)&headers,sizeof(struct _HEADER))<=0)  //struct _PACKET == PACKET
			{
				infLOG(ERROR, "RS_FILE_DATA_SIGN_CHECK 결과 받기 오류\n");

				// 받던 파일 삭제
				///////////////////////////////////////////////
				// temp 삭제									 //
				///////////////////////////////////////////////


				com9101 ( com9101_r);
				return 0;
			}

			if(headers.nCmd == RS_EOL)
			{
				infLOG(ALWAY, "RS_FILE_DATA_SIGN_CHECK 결과 받기 - RS_EROL \n");


				pSendData = new char[sizeof(HEADER)];
				memset(pSendData,0x00,sizeof(HEADER));

				headers.nCmd = RS_EOL;
				headers.nDataCnt = 0;
				headers.nDataSize = 0;
				headers.nErrorCode = 0;

				memcpy(pSendData, &headers, sizeof(HEADER));

				// 받던 파일 삭제
				///////////////////////////////////////////////
				// temp 삭제									 //
				///////////////////////////////////////////////



				com9101 ( com9101_r);
				return END;
			}
			else if(headers.nCmd == RS_OK)
			{
				infLOG(ALWAY, "RS_FILE_DATA_SIGN_CHECK 결과 받기 - RS_OK \n");
			}

			//2009/06/13 뮤레카 정보 받기.
			int nMurekaCnt = headers.nDataCnt;
			infLOG(ALWAY, "필터링 결과 파일 확인 - 갯수 [ %d ] \n",nMurekaCnt);

			LPMUREKA_VINFO pMurekaVInfo = NULL;
			if(nMurekaCnt > 0)
			{
				pMurekaVInfo = new MUREKA_VINFO[nMurekaCnt];



				if(	RecvData(Socket,(char*)pMurekaVInfo,sizeof(MUREKA_VINFO)*nMurekaCnt)<=0)  //struct _PACKET == PACKET
				{
					infLOG(ERROR,"필로그 뮤레카 결과 받기 오류 size : (%d) nMurekaCnt : (%d) \n", sizeof(MUREKA_VINFO)*nMurekaCnt, nMurekaCnt);
					// 받던 파일 삭제
					///////////////////////////////////////////////
					// temp 삭제									 //
					///////////////////////////////////////////////


					com9101 ( com9101_r);
					return 0;
				}

				#ifdef __DEBUG
				for(int i=0; i < nMurekaCnt; i++)
				{
					printf("뮤레카 정보 확인(%d).\n", i);
					printf("video_status : %d\n",pMurekaVInfo[i].nResultCode);
					printf("video_status : %s\n",pMurekaVInfo[i].filename);
					printf("video_status : %s\n",pMurekaVInfo[i].mureka_hash);
					printf("video_status : %s\n",pMurekaVInfo[i].video_status);
					printf("video_id : %s\n",pMurekaVInfo[i].video_id);
					printf("video_title : %s\n",pMurekaVInfo[i].video_title);
					printf("video_jejak_year : %s\n",pMurekaVInfo[i].video_jejak_year);
					printf("video_right_name : %s\n",pMurekaVInfo[i].video_right_name);
					printf("video_right_content_id : %s\n",pMurekaVInfo[i].video_right_content_id);
					printf("video_grade : %s\n",pMurekaVInfo[i].video_grade);
					printf("video_price : %s\n",pMurekaVInfo[i].video_price);
					printf("video_cha : %s\n",pMurekaVInfo[i].video_cha);
				}
				#endif

			}

			////////////////////기본 정보 완료////////////////////////////////////////////////

			//CMD5 md5;

			//char* pResult = md5.GetHashFromFile(szFullName,pFileinfo->dFileSize);
			//strcpy(Fups4005.szHashCode,pResult);

			//4005에 해쉬값 넣구

			if( strcmp(pFileinfo->szCopyright_yn ,"P") == 0 )
			{
				infLOG(ALWAY,"저작권 flag 재성정 : P -> N \n");
				strcpy(pFileinfo->szCopyright_yn ,"N") ;
			}

			//20190124 1mb hash
			CFUPS4005_1M_HASH Fups1MHash;
			memset( &Fups1MHash, 0x00, sizeof(CFUPS4006_1M_HASH));
			
			CFUPS4006_1M_HASH Fups4006_1MHash;
			memset( &Fups4006_1MHash, 0x00, sizeof(CFUPS4006_1M_HASH));
			
			
			CFUPS4005 Fups4005;
			CFUPS4006 Fups4006;

			memset( &Fups4005, 0x00, sizeof(CFUPS4005));
			memset( &Fups4006, 0x00, sizeof(CFUPS4006));

			memset(szSubFilePath,0x00,sizeof(szSubFilePath));
			memset(szFolderPath,0x00,sizeof(szFolderPath));
			int depth = 0;
			int seq_no = 0;

			strcpy(Fups4005.cont_gu , "WE");
			Fups4005.id = pFileinfo->cfups4001.id;
			Fups4005.file_size = pFileinfo->dFileSize;
			strcpy(Fups4005.sect_code , pFileinfo->cfups4001.sect_code );
			strcpy(Fups4005.sect_sub , pFileinfo->cfups4001.sect_sub );
			if( pFileinfo->cfups4001.reg_user == NULL || strlen( pFileinfo->cfups4001.reg_user) <= 0 )
			{
				strcpy(Fups4005.user_id, pHeader->szUserID);
			}
			else
			{
				strcpy(Fups4005.user_id, pFileinfo->cfups4001.reg_user);
			}
			strcpy(Fups4005.folder_yn , pFileinfo->cfups4001.folder_yn );
			strcpy(Fups4005.default_hash , pFileinfo->szDefault_hash );
			strcpy(Fups4005.audio_hash , pFileinfo->szAudio_hash );
			strcpy(Fups4005.video_hash , pFileinfo->szVideo_hash );
			strcpy(Fups4005.copyright_yn , pFileinfo->szCopyright_yn );
			strcpy(Fups4005.mureka_yn , pFileinfo->szMureka_yn );
			infLOG(ALWAY,"== Fups4005.copyright_yn : [%s] \n",Fups4005.copyright_yn);
			
			
			//20190123 1mb hash insert from fups4001.descript
			
			if( strlen(pFileinfo->cfups4001.descript) >  0 )
			{
				char* pTemp = strtok(pFileinfo->cfups4001.descript,"|");

				if ( pTemp != NULL && strlen(pTemp) > 0 )
				{
					strcpy(Fups1MHash.hash_1m,pTemp);
					strcpy(Fups4006_1MHash.hash_1m,Fups1MHash.hash_1m);
					
					
				}
				pTemp = strtok(NULL,"|");

				if ( pTemp != NULL && strlen(pTemp) > 0 )
				{
					strcpy(Fups1MHash.hash_1m_mureka,pTemp);
					strcpy(Fups4006_1MHash.hash_1m_mureka,Fups1MHash.hash_1m_mureka);
					
				}
				
			}
			
			
			//2009/06/14 뮤레카 조회 갯수.
			Fups4005.mureka_cnt = nMurekaCnt;
			infLOG(ALWAY,"nMurekaCnt [ %d ] \n",nMurekaCnt      		);
			infLOG(ALWAY,"fups4005 ] : id       	  [ %d ]     \n",Fups4005.id       		);
			infLOG(ALWAY,"fups4005 ] : seq_no	      [ %d ]     \n",Fups4005.seq_no	       );
			infLOG(ALWAY,"fups4005 ] : depth	      [ %d ]     \n",Fups4005.depth	       );
			infLOG(ALWAY,"fups4005 ] : file_size      [ %13.0f ]     \n",Fups4005.file_size     );
			infLOG(ALWAY,"fups4005 ] : sect_code      [ %s ]     \n",Fups4005.sect_code     );
			infLOG(ALWAY,"fups4005 ] : sect_sub       [ %s ]     \n",Fups4005.sect_sub     );
			infLOG(ALWAY,"fups4005 ] : folder_yn      [ %s ]     \n",Fups4005.folder_yn     );
			infLOG(ALWAY,"fups4005 ] : user_id        [ %s ]     \n",Fups4005.user_id       );
			infLOG(ALWAY,"fups4005 ] : folder_name    [ %s ]     \n",Fups4005.folder_name   );
			infLOG(ALWAY,"fups4005 ] : file_name      [ %s ] 	\n",Fups4005.file_name     );
			infLOG(ALWAY,"fups4005 ] : default_hash   [ %s ] 	\n",Fups4005.default_hash  );
			infLOG(ALWAY,"fups4005 ] : audio_hash	  [ %s ] 	\n",Fups4005.audio_hash	   );
			infLOG(ALWAY,"fups4005 ] : video_hash	  [ %s ] 	\n",Fups4005.video_hash	   );
			infLOG(ALWAY,"fups4005 ] : copyright_yn   [ %s ] 	\n",Fups4005.copyright_yn  );
			infLOG(ALWAY,"fups4005 ] : mureka_yn	  [ %s ] 	\n",Fups4005.mureka_yn  );
			infLOG(ALWAY,"fups4005 ] : hash_1m	  [ %s ] 	\n",Fups1MHash.hash_1m  );
			infLOG(ALWAY,"fups4005 ] : hash_1m_mureka	  [ %s ] 	\n",Fups1MHash.hash_1m_mureka  );
			
			if(strcmp(Fups4005.folder_yn,"Y")==0)
			{
				int nMoveLen = strlen(szFolderFullPath);
				int nDestLen = strlen(pFileinfo->szDownPath);

				if( strstr( pFileinfo->szDownPath ,szFolderFullPath ) != NULL  && nDestLen - nMoveLen > 0 )
				{

					memcpy(szSubFilePath,pFileinfo->szDownPath + nMoveLen ,  nDestLen - nMoveLen );
				}

				char* pTemp = strtok(szSubFilePath,"/");

				while(pTemp!=NULL )
				{
					depth ++ ;
					pTemp = strtok(NULL,"/");

					if(  pTemp != NULL)
					{
						strcat(szFolderPath ,pTemp);
						strcat(szFolderPath ,"/");
					}
				}
				if( depth > 0 )
					depth--;

				Fups4005.depth = depth;



				strcpy(Fups4005.folder_name, szFolderPath);
				strcpy(Fups4005.file_name , pFileinfo->szFileName);
			}
			else if(strcmp(Fups4005.folder_yn, "N") == 0)
			{
				Fups4005.seq_no = seq_no;
				seq_no++;
				strcpy(Fups4005.file_name , pFileinfo->cfups4001.file_name2);
			}
			Fups4005.depth = depth;

			//저작권 정보 재수정
			infLOG(ALWAY,"정보 확인 1 : tpye [ %d ] == [ %d ] : sect_code [ %s ] : copyright [ %s ] \n", pFileinfo->nType , FT_FOLDER , pFileinfo->cfups4001.sect_code , pFileinfo->szCopyright_yn);


			#ifdef __DEBUG
			printf("fups4005 ] : id       		[ %d ]     \n",Fups4005.id       		);
			printf("fups4005 ] : seq_no	        [ %d ]     \n",Fups4005.seq_no	       );
			printf("fups4005 ] : depth	        [ %d ]     \n",Fups4005.depth	       );
			printf("fups4005 ] : file_size      [ %13.0f ]     \n",Fups4005.file_size     );
			printf("fups4005 ] : sect_code      [ %s ]     \n",Fups4005.sect_code     );
			printf("fups4005 ] : sect_sub       [ %s ]     \n",Fups4005.sect_sub     );
			printf("fups4005 ] : folder_yn      [ %s ]     \n",Fups4005.folder_yn     );
			printf("fups4005 ] : user_id        [ %s ]     \n",Fups4005.user_id       );
			printf("fups4005 ] : folder_name    [ %s ]     \n",Fups4005.folder_name   );
			printf("fups4005 ] : file_name      [ %s ] 	\n",Fups4005.file_name     );
			printf("fups4005 ] : default_hash   [ %s ] 	\n",Fups4005.default_hash  );
			printf("fups4005 ] : audio_hash	    [ %s ] 	\n",Fups4005.audio_hash	   );
			printf("fups4005 ] : video_hash	    [ %s ] 	\n",Fups4005.video_hash	   );
			printf("fups4005 ] : copyright_yn   [ %s ] 	\n",Fups4005.copyright_yn  );
			printf("fups4005 ] : mureka_yn      [ %s ] 	\n",Fups4005.mureka_yn  );
			printf("fups4005 ] : cont_gu      [ %s ] 	\n",Fups4005.cont_gu  );
			printf("fups4005 ] : hash_1m      [ %s ] 	\n",Fups1MHash.hash_1m  );
			printf("fups4005 ] : hash_1m_mureka      [ %s ] 	\n",Fups1MHash.hash_1m_mureka  );
			
			#endif

			infLOG(ALWAY,"fups4005 데이터를 업데이트 중입니다.\n"	);
			infLOG(ALWAY,"fups4005 ] : id       	  [ %d ]     \n",Fups4005.id       		);
			infLOG(ALWAY,"fups4005 ] : seq_no	      [ %d ]     \n",Fups4005.seq_no	       );
			infLOG(ALWAY,"fups4005 ] : depth	      [ %d ]     \n",Fups4005.depth	       );
			infLOG(ALWAY,"fups4005 ] : file_size      [ %13.0f ]     \n",Fups4005.file_size     );
			infLOG(ALWAY,"fups4005 ] : sect_code      [ %s ]     \n",Fups4005.sect_code     );
			infLOG(ALWAY,"fups4005 ] : sect_sub       [ %s ]     \n",Fups4005.sect_sub     );
			infLOG(ALWAY,"fups4005 ] : folder_yn      [ %s ]     \n",Fups4005.folder_yn     );
			infLOG(ALWAY,"fups4005 ] : user_id        [ %s ]     \n",Fups4005.user_id       );
			infLOG(ALWAY,"fups4005 ] : folder_name    [ %s ]     \n",Fups4005.folder_name   );
			infLOG(ALWAY,"fups4005 ] : file_name      [ %s ] 	\n",Fups4005.file_name     );
			infLOG(ALWAY,"fups4005 ] : default_hash   [ %s ] 	\n",Fups4005.default_hash  );
			infLOG(ALWAY,"fups4005 ] : audio_hash	  [ %s ] 	\n",Fups4005.audio_hash	   );
			infLOG(ALWAY,"fups4005 ] : video_hash	  [ %s ] 	\n",Fups4005.video_hash	   );
			infLOG(ALWAY,"fups4005 ] : copyright_yn   [ %s ] 	\n",Fups4005.copyright_yn  );
			infLOG(ALWAY,"fups4005 ] : mureka_yn	  [ %s ] 	\n",Fups4005.mureka_yn  );
			infLOG(ALWAY,"fups4005 ] : hash_1m	  [ %s ] 	\n",Fups1MHash.hash_1m  );
			infLOG(ALWAY,"fups4005 ] : hash_1m_mureka	  [ %s ] 	\n",Fups1MHash.hash_1m_mureka  );



			Fups4006.id       		  =  Fups4005.id;
			Fups4006.seq_no	          =  Fups4005.seq_no;
			Fups4006.depth	          =  Fups4005.depth;
			Fups4006.file_size        =  Fups4005.file_size;
			strcpy(Fups4006.sect_code,  Fups4005.sect_code);
			strcpy(Fups4006.sect_sub, Fups4005.sect_sub);
			strcpy(Fups4006.folder_yn    , Fups4005.folder_yn);
			strcpy(Fups4006.user_id      , Fups4005.user_id);
			strcpy(Fups4006.folder_name  , Fups4005.folder_name);
			strcpy(Fups4006.file_name    , Fups4005.file_name);
			strcpy(Fups4006.default_hash , Fups4005.default_hash);
			strcpy(Fups4006.audio_hash	 ,  Fups4005.audio_hash);
			strcpy(Fups4006.video_hash	 ,  Fups4005.video_hash);
			strcpy(Fups4006.copyright_yn , Fups4005.copyright_yn);
			strcpy(Fups4006.mureka_yn    , Fups4005.mureka_yn);
			strcpy(Fups4006.cont_gu    , Fups4005.cont_gu);
			strcpy(Fups4006.auth_num    , com9004Result.auth_num );

			//2009/06/14 뮤레카 조회 갯수.
			Fups4006.mureka_cnt = nMurekaCnt;


			#ifdef __DEBUG
			printf("fups4006 ] : id       		[ %d ]     \n",Fups4006.id       		);
			printf("fups4006 ] : seq_no	        [ %d ]     \n",Fups4006.seq_no	       );
			printf("fups4006 ] : depth	        [ %d ]     \n",Fups4006.depth	       );
			printf("fups4006 ] : file_size      [ %13.0f ]     \n",Fups4006.file_size     );
			printf("fups4006 ] : sect_code      [ %s ]     \n",Fups4006.sect_code     );
			printf("fups4006 ] : sect_sub       [ %s ]     \n",Fups4006.sect_sub     );
			printf("fups4006 ] : folder_yn      [ %s ]     \n",Fups4006.folder_yn     );
			printf("fups4006 ] : user_id        [ %s ]     \n",Fups4006.user_id       );
			printf("fups4006 ] : folder_name    [ %s ]     \n",Fups4006.folder_name   );
			printf("fups4006 ] : file_name      [ %s ] 	\n",Fups4006.file_name     );
			printf("fups4006 ] : default_hash   [ %s ] 	\n",Fups4006.default_hash  );
			printf("fups4006 ] : audio_hash	    [ %s ] 	\n",Fups4006.audio_hash	   );
			printf("fups4006 ] : video_hash	    [ %s ] 	\n",Fups4006.video_hash	   );
			printf("fups4006 ] : copyright_yn   [ %s ] 	\n",Fups4006.copyright_yn  );
			printf("fups4006 ] : mureka_yn      [ %s ] 	\n",Fups4006.mureka_yn  );
			printf("fups4006 ] : cont_gu   		[ %s ] 	\n",Fups4006.cont_gu  );
			printf("fups4006 ] : auth_num       [ %s ] 	\n",Fups4006.auth_num  );



			#endif

			infLOG(ALWAY,"fups4006 ] : id       	  [ %d ]     \n",Fups4006.id       		);
			infLOG(ALWAY,"fups4006 ] : seq_no	      [ %d ]     \n",Fups4006.seq_no	       );
			infLOG(ALWAY,"fups4006 ] : depth	      [ %d ]     \n",Fups4006.depth	       );
			infLOG(ALWAY,"fups4006 ] : file_size      [ %13.0f ]     \n",Fups4006.file_size     );
			infLOG(ALWAY,"fups4006 ] : sect_code      [ %s ]     \n",Fups4006.sect_code     );
			infLOG(ALWAY,"fups4006 ] : sect_sub       [ %s ]     \n",Fups4006.sect_sub     );
			infLOG(ALWAY,"fups4006 ] : folder_yn      [ %s ]     \n",Fups4006.folder_yn     );
			infLOG(ALWAY,"fups4006 ] : user_id        [ %s ]     \n",Fups4006.user_id       );
			infLOG(ALWAY,"fups4006 ] : folder_name    [ %s ]     \n",Fups4006.folder_name   );
			infLOG(ALWAY,"fups4006 ] : file_name      [ %s ] 	\n",Fups4006.file_name     );
			infLOG(ALWAY,"fups4006 ] : default_hash   [ %s ] 	\n",Fups4006.default_hash  );
			infLOG(ALWAY,"fups4006 ] : audio_hash	  [ %s ] 	\n",Fups4006.audio_hash	   );
			infLOG(ALWAY,"fups4006 ] : video_hash	  [ %s ] 	\n",Fups4006.video_hash	   );
			infLOG(ALWAY,"fups4006 ] : copyright_yn   [ %s ] 	\n",Fups4006.copyright_yn  );
			infLOG(ALWAY,"fups4006 ] : mureka_yn	  [ %s ] 	\n",Fups4006.mureka_yn  );
			infLOG(ALWAY,"fups4006 ] : cont_gu   	  [ %s ] 	\n",Fups4006.cont_gu  );
			infLOG(ALWAY,"fups4006 ] : auth_num       [ %s ] 	\n",Fups4006.auth_num  );
			infLOG(ALWAY,"fups4006 ] : hash_1m       [ %s ] 	\n",Fups1MHash.hash_1m  );
			infLOG(ALWAY,"fups4006 ] : hash_1m_mureka       [ %s ] 	\n",Fups1MHash.hash_1m_mureka  );			

			infLOG(ALWAY,"============ pFileinfo->cfups4001.copyright_yn [ %s ] \n",pFileinfo->cfups4001.copyright_yn);
			infLOG(ALWAY,"============ pFileinfo->cfups4001.descript [ %s ] \n",pFileinfo->cfups4001.descript);
			
			int nCopyRight = 0;
			int nCompany  = 0;

			if( strcmp(com9004Result.auth_num ,"CPR") != 0)
			{
				
				//20190124 1m hash
				if( strlen(Fups1MHash.hash_1m) > 0 || strlen(Fups1MHash.hash_1m_mureka) > 0 )
				{
					nCopyRight = fups4005hash(Fups4005, pMurekaVInfo,(CFUPS4005_1M_HASH)Fups1MHash);	//저작권 조회
				}
				else
					nCopyRight = fups4005(Fups4005, pMurekaVInfo);	//저작권 조회
			}
			infLOG(ALWAY,"저작권 조회 결과 [ %d ] \n\n\n",nCopyRight);
			if( nCopyRight <= 0 )
			{
				//20190124 1m hash
				if( strlen(Fups4006_1MHash.hash_1m) > 0 || strlen(Fups4006_1MHash.hash_1m_mureka) > 0 )
				{
					nCompany = fups4006hash(Fups4006, pMurekaVInfo,Fups4006_1MHash);	//저작권 조회
				}
				else
					nCompany = fups4006(Fups4006, pMurekaVInfo);	//저작권에 걸리지않는 자료라면 유료컨텐츠여부 조회.
			}
			infLOG(ALWAY,"제휴 조회 결과   [ %d ] \n\n\n",nCompany  );

			if(pMurekaVInfo)
			{
				delete[] pMurekaVInfo;
				pMurekaVInfo = NULL;
			}


			infLOG(ALWAY,"nCopyRight [ %d ] \n",nCopyRight);
			infLOG(ALWAY,"nCompany   [ %d ] \n",nCompany  );
			// 20140523 : 보류 처리하기
			//	infLOG(ALWAY,"============ cfups4001.copyright_yn [ %s ] \n",cfups4001.copyright_yn);
			//if(strcmp (pFileinfo->cfups4001.copyright_yn ,"B") != 0)
			//{
				strcpy( pFileinfo->cfups4001.copyright_yn , "N");

				if( bHaveCopyright  )
				{
					infLOG(ALWAY,"1 copyright_yn = y\n");
					strcpy( pFileinfo->cfups4001.copyright_yn , "Y");
				}
				else
				{
					if( bHaveCompany )
					{
						infLOG(ALWAY,"2 copyright_yn = C\n");
						strcpy( pFileinfo->cfups4001.copyright_yn , "C");
					}

					if( nCopyRight > 0   )
					{
						bHaveCopyright = true;
						infLOG(ALWAY,"3 copyright_yn = Y\n");
						strcpy( pFileinfo->cfups4001.copyright_yn , "Y");
					}
					else
					{
						if( nCompany > 0 )
						{
							bHaveCompany = true;
							infLOG(ALWAY,"4 copyright_yn = C\n");
							strcpy( pFileinfo->cfups4001.copyright_yn , "C");

						}
					}

				}
			//}

/*
			if( strcmp(pFileinfo->cfups4001.sect_code ,"07") == 0 ) //음악일경우 무조건 N로 설정
				strcpy( pFileinfo->cfups4001.copyright_yn , "N");
*/

			infLOG(ALWAY,"자료실 확인 : sect_code [ %s ] : copyright_yn [ %s ] \n" , pFileinfo->cfups4001.sect_code , pFileinfo->cfups4001.copyright_yn);
			if( nCopyRight < 0 )
			{
				infLOG(ERROR, "저작권 조회 오류입니다. Error Num [ %d ]\n",nCopyRight);

				pSendData = new char[sizeof(ERR_HEADER)];
				memset(pSendData,0x00,sizeof(ERR_HEADER));
				errheader.header.nCmd = RS_ERR;
				errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
				strcat(errheader.errmsg,"파일 정보 등록 오류 입니다.");

				memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

				pHeader->nCmd = RS_ERR;

				com9101 ( com9101_r);
				return -RS_FILE_DATA_TRANSFER;
			}



			// 파일 제어 및 전송
			FILE* DownloadFile; //파일 포인터
			DownloadFile = NULL;
			//// 파일 open형식 결정////
			if( bFOpenAppendMode) //append mode
			{
				
				DownloadFile = fopen64(szFullName,"ar+tb");
				infLOG(ALWAY, "파일 생성 : append mode ( %s )\n",szFullName);
				
			}
			else
			{
				DownloadFile = fopen64(szFullName,"wr+tb");
				infLOG(ALWAY, "파일 생성 : write mode ( %s )\n",szFullName);
			
			}


			if(  DownloadFile == NULL) //파일을 열수 없으면
			{
				infLOG(ERROR, "파일 열기 오류 입니다. [ %s ] error num [ %d ] msg [ %s ] \n",szFullName,errno, szErrMsg);


				pSendData = new char[sizeof(ERR_HEADER)];
				memset(pSendData,0x00,sizeof(ERR_HEADER));
				errheader.header.nCmd = RS_ERR;
				errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
				strcat(errheader.errmsg,"서버에서 파일 만들기 실패 하였습니다.");

				memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

				pHeader->nCmd = RS_ERR;

				// 받던 파일 삭제
				///////////////////////////////////////////////
				// temp 삭제									 //
				///////////////////////////////////////////////

				com9101 ( com9101_r);
				return -RS_FILE_DATA_TRANSFER;
			}

			//// 이어 받기를 위한 파일 해더 구조체 생성 ////

			if( !bGhostMode )
			{
				infLOG(ALWAY,"파일의 Seek 를 마지막 위치로 이동 합니다.\n");

				if(fseeko64(DownloadFile,0l,SEEK_END) < 0)
				{
					infLOG(ALWAY,"파일의 Seek 를 마지막 위치로 이동 중 류가 발생하였습니다. errno [ %d ] error msg [ %s ]\n",errno,strerror(errno));
					pSendData = new char[sizeof(ERR_HEADER)];
					memset(pSendData,0x00,sizeof(ERR_HEADER));
					errheader.header.nCmd = RS_ERR;
					errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
					strcat(errheader.errmsg,"서버 파일의 섹터 이동 실패 하였습니다.");

					memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

					pHeader->nCmd = RS_ERR;
					// 받던 파일 삭제
					///////////////////////////////////////////////
					// temp 삭제									 //
					///////////////////////////////////////////////
					com9101 ( com9101_r);
					return -RS_FILE_DATA_TRANSFER;
				}
			}

			LPFILEHEAD pFileHead = new FILEHEAD;
			memset(pFileHead,0x00,sizeof(FILEHEAD));

			//double dCurrentLen = (double)ftello64 (DownloadFile); // 파일이 어디까지 있는지 결정

			double dCurrentLen	= 0;

	//				dCurrentLen = (double)statbuf.st_size;
			if( !bGhostMode )
			{
				dCurrentLen = (double)ftello64 (DownloadFile); // 파일이 어디까지 있는지 결정
				infLOG(ALWAY, "최근 이동한 파일 사이즈 ( %.0f )\n",dCurrentLen);

			}

			if(dCurrentLen < 0)
				dCurrentLen = 0;

			pFileHead->dCurrentSize = dCurrentLen; //해드에 현 파일 길이 저장

			////////////////////////////////////////////////
			//처음 용량 업데이트 : 실패시 복원 해야 함.

			// head 작성
			memset(&headers,0x00,sizeof(HEADER));

			headers.nCmd = RS_FILE_DATA_TRANSFER ; // 데이터 전송
			headers.nDataCnt = 1;
			headers.nDataSize = sizeof(FILEHEAD);
			headers.nErrorCode = 0;

			pSendData = new char[sizeof(HEADER) + headers.nDataCnt*headers.nDataSize];

			memcpy(pSendData,&headers,sizeof(HEADER));

			memcpy(pSendData + HEADER_SIZE,pFileHead, headers.nDataCnt*headers.nDataSize);

			infLOG(ALWAY,"Send RS_FILE_DATA_TRANSFER\n");
			//// body 작성////
			if(	SendData(Socket,pSendData,HEADER_SIZE + headers.nDataCnt*headers.nDataSize)<0)  //struct _PACKET == PACKET
			{
				infLOG(ERROR,"Send RS_FILE_DATA_TRANSFER ERROR\n");
				delete pFileHead;
				// 용량 복원 하기
				// 받던 파일 삭제
				///////////////////////////////////////////////
				// temp 삭제									 //
				///////////////////////////////////////////////

				com9101 ( com9101_r);
				return 0;
			}
			delete[] pSendData;
			pSendData = NULL;

			delete pFileHead;

			infLOG(ALWAY,"Send RS_FILE_DATA_TRANSFER OK \n");

		 ///////////////////////// 데이터 전송 //////////////////////////////////

			dTotalRecvLen = 0; //총 받은 길이
			dTotalLen = pFileinfo->dFileSize - dCurrentLen; // down될 파일의 총 길이
			nWriteLen=0;
			nRecvLen=0;

			char* szRecvBuffer = new char[RECVBUF]; //recv buffer


			// 저작권 정보

			// 음악이면서 저작권 정보에 걸리면 데이터만 받고 실제로 서버에 저장하지 않는다.
			// 유동적으로 하기 위해서 네트워크에서 데이터까지 	받아준다. 이부분 바꿀려면 사용자 모듈에서 보내는것 처럼 보이게 하여야 한다.
			infLOG(ALWAY,"파일 확인 [ %s ] : 받을 파일 전체 길이 [ %.0f ] = [ %.0f (전체) - %.0f(최근이동한) ] \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize ,dCurrentLen);


			/*
			if( bGhostMode )
			{
				infLOG(ALWAY,"가상 업로드 모드 \n");
				//int fno = fileno(DownloadFile);

				while(dTotalLen > 0  )
				{
					memset(szRecvBuffer,0x00,RECVBUF);
					///// 파일받기 /////

					 nRecvLen =  RecvFileData(Socket, szRecvBuffer, RECVBUF, dTotalLen) ;

			        if(nRecvLen > 0)
			        {
			        	nWriteLen = 1;
			        }
			        else
			        	nWriteLen = 0;

			    	if(nWriteLen <= 0)
		        	{
		        		if(nWriteLen == 0)
		        		{
		        			#ifdef __DEBUG
		        			printf(" ] Write File End (%s) : 파일 전체 길이 (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
		        			#endif
		        			infLOG(ALWAY," ] Write File End (%s) : 파일 전체 길이 (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
		        		}
		        		else
		        		{
		        			#ifdef __DEBUG
		        			printf(" ] Write File ERROR (%s) : 파일 전체 길이 (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
		        			#endif
		        			infLOG(ERROR," ] Write File ERROR (%s) : 파일 전체 길이 (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
		        			nRecvLen = -1;
		        		}
		        	}

			        if(nRecvLen <= 0 && dTotalLen != 0)	//받다가 오류가 났을시...DB처리
			        {
						#ifdef __DEBUG
						printf(" ] file recv exception \n");
						#endif

						if(nRecvLen < 0)
			        	{
							memset(szErrMsg,0x00,sizeof(szErrMsg));
							GetErrMsg(-nRecvLen,szErrMsg);
							infLOG(ERROR, " ] RecvSocket Error ( %d )( %s )\n",nRecvLen,szErrMsg);

							#ifdef __DEBUG
							printf(" ] RecvSocket Error ( %d )( %s )\n",nRecvLen,szErrMsg);
							#endif
			        	}
			        	else if(nRecvLen == 0)
			        	{
							memset(szErrMsg,0x00,sizeof(szErrMsg));
							GetErrMsg(-nRecvLen,szErrMsg);

			        		infLOG(ERROR, " ] RecvSocket Error ( 접속을 끊었습니다. ) (%s)\n",szErrMsg);

							#ifdef __DEBUG
							printf(" ] RecvSocket Error ( 접속을 끊었습니다. ) (%s)\n",szErrMsg);
							#endif
			        	}

						infLOG(ERROR," ] WE 디스크 취소 (%s) RecvLen (%d) (%15.0f) TotalLen(%15.0f)\n ",pFileinfo->cfups4001.file_name2,nRecvLen ,dTotalRecvLen,dTotalLen);

						if(DownloadFile)
						{
							fclose(DownloadFile);
							DownloadFile == NULL ;
						}

					   	if(szRecvBuffer)
							delete[] szRecvBuffer;

						infLOG(ERROR," ] WE 디스크 취소 (%s) RecvLen (%d) (%15.0f) TotalLen(%15.0f)\n ", pFileinfo->cfups4001.file_name2,nRecvLen ,dTotalRecvLen,dTotalLen);

						com9101 ( com9101_r);
						return 0;
					//	return END;
		        	}

	        		dTotalLen = dTotalLen - (double)nRecvLen;  //총길이에서  받은 길이 제거
		        	dTotalRecvLen = dTotalRecvLen + (double)nRecvLen; //받은 길이 만큼 더함
				}
			}
			else
			*/
			{
				nTotalRecvFileCnt++;
				infLOG(ALWAY,"업로드 갯수 [ %d ] \n",nTotalRecvFileCnt);
				int fno = fileno(DownloadFile);

				while(dTotalLen > 0  )
				{
					memset(szRecvBuffer,0x00,RECVBUF);
					///// 파일받기 /////

					nRecvLen =  RecvFileData(Socket, szRecvBuffer, RECVBUF, dTotalLen) ;

				    if(nRecvLen > 0)
				    {
				    	nWriteLen = write(fno ,szRecvBuffer,nRecvLen);
				      	//nWriteLen = fwrite(szRecvBuffer,nRecvLen,1,DownloadFile);
				    }
				    else
				    	nWriteLen = 0;

				    //fwrite(szRecvBuffer,1,nRecvLen,DownloadFile); //받은 길이 만큼 file에 저장

				    if(nWriteLen <= 0)
			        {
			        	if(nWriteLen == 0)
			        	{
			        		#ifdef __DEBUG
			        		printf(" ] Write File End (%s) : 파일 전체 길이 (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
			        		#endif
			        		infLOG(ALWAY,"Write File End (%s) : 파일 전체 길이 (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
			        	}
			        	else
			        	{
			        		#ifdef __DEBUG
			        		printf(" ] Write File ERROR (%s) : 파일 전체 길이 (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
			        		#endif
			        		infLOG(ERROR," ] Write File ERROR (%s) : 파일 전체 길이 (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
			        		nRecvLen = -1;
			        	}
			        }

				    if(nRecvLen <= 0 && dTotalLen != 0)	//받다가 오류가 났을시...DB처리
				    {

						if(nRecvLen < 0)
				       	{
							memset(szErrMsg,0x00,sizeof(szErrMsg));
							GetErrMsg(-nRecvLen,szErrMsg);
							infLOG(ERROR, "데이터를 발을 수 없습니다. ( %d )( %s )\n",nRecvLen,szErrMsg);

				       	}
				       	else if(nRecvLen == 0)
				       	{
							memset(szErrMsg,0x00,sizeof(szErrMsg));
							GetErrMsg(-nRecvLen,szErrMsg);
							infLOG(ERROR, "접속이 끊어 졌습니다.[ 이경우는 보통 클라이언트에서 데이터를 제대로 보내지 못할때 발생합니다. ] \n" );
				       	}


						infLOG(ERROR,"위디스크 취소 (%s) RecvLen (%d) (%15.0f) TotalLen(%15.0f)\n ",pFileinfo->cfups4001.file_name2,nRecvLen ,dTotalRecvLen,dTotalLen);
						infLOG(ERROR,"errno [ %d ] error msg [ %s ]\n",errno,strerror(errno));


						if(DownloadFile)
						{
							fclose(DownloadFile);
							DownloadFile == NULL ;
						}

							// 받던 파일 삭제
					///////////////////////////////////////////////
					// temp 삭제								 //
					///////////////////////////////////////////////

					   	if(szRecvBuffer)
							delete[] szRecvBuffer;

						com9101 ( com9101_r);
						return 0;
					//	return END;
			        }
	        		dTotalLen = dTotalLen - (double)nRecvLen;  //총길이에서  받은 길이 제거
		        	dTotalRecvLen = dTotalRecvLen + (double)nRecvLen; //받은 길이 만큼 더함
				}
			}

	/*
			#ifdef __DEBUG
			printf("\r\ ] writeing to file %15.0f\n",dTotalRecvLen);
			#endif
	*/

			if(DownloadFile)
			{
				fclose(DownloadFile);
				DownloadFile == NULL ;
			}

			if(	szRecvBuffer)
				delete[] szRecvBuffer;

			///////////////////////////////////////////////
			//파일 이름 바꾸기
			// DB 넣기..
/******************************해쉬값 추출 시작***********************************************/

			infLOG(ALWAY,"위디스크 데이터 받기 완료 및 확인 - 파일이름 (%s) 임시번호 (%lu) 임시번호 (%lu) 실제 총 받은 갯수 ( %d )\n",pFileinfo->cfups4001.file_name2, pFileinfo->nNumber,pFileinfo->cfups4001.id,nTotalRecvFileCnt);
			pFileinfo->cfups4001.down_cnt = nTotalRecvFileCnt;

			//여기서 부터 받기 시작

			infLOG(ALWAY,"전송 완료 후 해더 응답 대기.\n");
			memset(&headers,0x00,sizeof(HEADER));
			if(RecvData(Socket,(char*)&headers,HEADER_SIZE ) <= 0)
			{
				infLOG(ERROR,"전송 완료 후 해더 응답 대기 오류.\n");
				// 받던 파일 삭제
				///////////////////////////////////////////////
				// temp 삭제									 //
				//////////////////////////////////////////////

				com9101 ( com9101_r);

				return 0;

			}
			infLOG(ERROR,"전송 완료 후 해더 응답 대기 결과 [ %d ].\n",headers.nCmd);
			if(headers.nCmd == RS_FILE_REQUEST_NEXT_FILE )
			{
				infLOG(ALWAY, "RS_FILE_REQUEST_NEXT_FILE\n다음 파일을 받습니다.\n");

				memset(&headers,0x00,HEADER_SIZE);

				headers.nCmd = RS_FILE_REQUEST_NEXT_FILEINFO; //파일 정보 요청
				headers.nDataCnt = 0;
				headers.nDataSize = 0;

				if(	SendData(Socket,(char*)&headers,HEADER_SIZE )<0)  //struct _PACKET == PACKET
				{
					infLOG(ERROR, "RS_FILE_REQUEST_NEXT_FILE 전송 오류 \n");


					// 받던 파일 삭제
					///////////////////////////////////////////////
					// temp 삭제									 //
					///////////////////////////////////////////////

					com9101 ( com9101_r);

					return 0;

				}
				infLOG(ERROR, "RS_FILE_REQUEST_NEXT_FILE 응답 대기 중 \n");

				//recv file_transfer
				memset(&headers,0x00,HEADER_SIZE);
				if(RecvData(Socket,(char*)&headers,HEADER_SIZE ) <= 0)
				{
					infLOG(ERROR, "RS_FILE_REQUEST_NEXT_FILE 응답 대기 중 오류 [ 이경우는 보통 클라이언트로부터 데이터를 받지 못할 때 발생합니다. \n");


					// 받던 파일 삭제
					///////////////////////////////////////////////
					// temp 삭제									 //
					///////////////////////////////////////////////

					com9101 ( com9101_r);
					return 0;

				}

				memset(&rFileInfo,0x00,sizeof(FILEINFO));
				infLOG(ERROR, "다음 파일정보 받기 대기 중 \n");

				if( RecvData(Socket,(char*)&rFileInfo,sizeof(FILEINFO) ) <= 0)
				{
					infLOG(ERROR, "다음 파일정보 받기 대기 중 오류 \n");


					// 받던 파일 삭제
					///////////////////////////////////////////////
					// temp 삭제									 //
					///////////////////////////////////////////////

					com9101 ( com9101_r);
					return 0;
				}
				pFileinfo = &rFileInfo;
			}
			else if(headers.nCmd == RS_EOL)
			{
				infLOG(ALWAY,"RS_EOL\n위디스크의 컨텐츠를 등록합니다. 임시 번호 검사(T_CONTENTS_TEMP) [ %lu ]\n",pFileinfo->nNumber );


			// 등록후 eol 보내기
				if(pFileinfo->nTypeDisk == FT_WEDISK && pFileinfo->nNumber > 0 ) //컨텐츠 등록
				{
					if( dTotalLen == 0)
					{
						infLOG(ALWAY,"위디스크 파일 등록 - 임시번호 [ %lu ] 파일이름  [ %s ]  \n",pFileinfo->nNumber,pFileinfo->cfups4001.file_name2);

						//여기로 일반 업로드 적용

						int nResult = fups4001(pFileinfo->cfups4001);
						infLOG(ALWAY,"위디스크 등록결과(fups4001) Result [ %d ] \n",nResult);

						if(  nResult < 0 )//pFileinfo->cfups4001) == -1)
						{
							// 받던 파일 삭제
							///////////////////////////////////////////////
							// temp 삭제									 //
							///////////////////////////////////////////////

							//컨텐츠 등록중 오류 발생 ...꼭 삭제 해야 할 목록들

							infLOG(ERROR, "================== 위디스크 등록 오류 ===================\n"
										  "임시번호 ( %lu )서버 아이디( %s ) 파일경로 ( %s )                         \n"
										  "=========================================================\n" ,pFileinfo->nNumber,pFileinfo->cfups4001.server_id ,szFullPath);

							memset(&headers,0x00,sizeof(HEADER));

			/*
							if(	RecvData(Socket,(char*)&headers,sizeof(struct _HEADER))<=0)  //struct _PACKET == PACKET
							{
								return 0;
							}

			*/
							//////////////////////////////////////////


							headers.nCmd = RS_FILE_END_FAIL;
							headers.nDataCnt = 0;
							headers.nDataSize = 0;
							headers.nErrorCode = 4001;

							if( nResult == -2)
								headers.nErrorCode = 400199;

							infLOG(ALWAY,"RS_FILE_END_FAIL 전송 \n");

							if(	SendData(Socket,(char*)&headers,HEADER_SIZE)<0)  //struct _PACKET == PACKET
							{
								com9101 ( com9101_r);
								return 0;
							}
							com9101 ( com9101_r);
							return 1;
						}

						char szRunSystem[1024] = {0,};
						//  2013.11.19.. add by lee
						sprintf(szRunSystem,"chmod -R 755 %s",szFullName);
						infLOG(ALWAY,"++===========> %s   :: szFullPath [%s]\n",szRunSystem,szFullPath);
						system(szRunSystem);

						memset(szRunSystem,0x00,sizeof(szRunSystem));
						sprintf(szRunSystem,"chown -R ezwon:ezwon /raid/fdata/");
						infLOG(ALWAY,"++===========> %s\n",szRunSystem);
						system(szRunSystem);
/*
						infLOG(ALWAY,"chmod -R 755 %s\n",szFullPath);
						//  2013.11.19.. add by lee
						char szRunSystem[1024] = {0,};
						sprintf("chmod -R 755 %s",szFullPath);
						system(szRunSystem);
						infLOG(ALWAY,"chown -R ezwon:ezwon %s\n",szFullPath);
						sprintf("chown -R ezwon:ezwon %s",szFullPath);
						system(szRunSystem);
						// 2013.11.19.. add by
*/
						infLOG(ALWAY,"============== 위디스크 파일 등록 완료 ===============\n");

					}
					else
					{
						infLOG(ERROR, "============ 필로그 등록 오류 - 파일을 완전히 받지 못하였습니다. ========== \n");
						memset(&headers,0x00,sizeof(HEADER));

			/*			if(	RecvData(Socket,(char*)&headers,sizeof(struct _HEADER))<=0)  //struct _PACKET == PACKET
						{
							return 0;
						}
			*/
						// 받던 파일 삭제

						#ifdef __DEBUG
						printf(" ] file recv cancel..2\n");
						#endif

						headers.nCmd = RS_FILE_END_FAIL;
						headers.nDataCnt = 0;
						headers.nDataSize = 0;
						headers.nErrorCode = 4001;
						infLOG(ALWAY,"RS_FILE_END_FAIL 전송 \n");

						if(	SendData(Socket,(char*)&headers,HEADER_SIZE)<=0)  //struct _PACKET == PACKET
						{
							com9101 ( com9101_r);
							return 0;
						}
						com9101 ( com9101_r);
						return 1;
					}
				}

				infLOG(ALWAY, "RS_EROL 전송\n");

				memset(&headers,0x00,HEADER_SIZE);

				headers.nCmd = RS_EOL; //파일 정보 요청
				headers.nDataCnt = 0;
				headers.nDataSize = 0;

				if(	SendData(Socket,(char*)&headers,HEADER_SIZE )<0)  //struct _PACKET == PACKET
				{
					com9101 ( com9101_r);
					return 0;
				}
				com9101 ( com9101_r);
				return 0;
			}
			else
			{
				infLOG(ERROR,"전송 완료 후 해더 응답 대기 오류 [ %d ]명령어가 없습니다.\n",headers.nCmd);

				// 받던 파일 삭제
				///////////////////////////////////////////////
				// temp 삭제									 //
				///////////////////////////////////////////////
				com9101 ( com9101_r);
				return 0;
			}
		}while( 1 );

		com9101 ( com9101_r);
	}
	else
	{
		infLOG(ERROR," com9004 오류 - nCType ( %d ) 이 없습니다. \n" ,nCType );
		pSendData = new char[sizeof(ERR_HEADER)];
		memset(pSendData,0x00,sizeof(ERR_HEADER));
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;

		strcat(errheader.errmsg,"업로드 오류입니다. 잘못된 서비스 선택입니다.");

		memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

		pHeader->nCmd = RS_ERR;

		return -RS_FILE_DATA_TRANSFER;
	}
	return 0;
}





