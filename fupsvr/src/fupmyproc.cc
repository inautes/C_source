/*----------------------------------------------------------------------------
   받던 파일 삭제 
if(pFileinfo->nTypeDisk == FT_MYDISK && pFileinfo->nType == FT_FILE)
{

	if(DeleteFile(pFileinfo->nType,szFullName) == -1)
	{
		
	}
}
else if(pFileinfo->nTypeDisk == FT_MYDISK && pFileinfo->nType == FT_FOLDER)
{
	
	if(DeleteFile(pFileinfo->nType,strFullPath) == -1)	
	{
		
	}
}
----------------------------------------------------------------------------*/

			
#include "fupsock.h"

#include "fupdefine.h"
#include "comcomm.h"
#include "apdefine.h"
#include "fupmyproc.h"
#include "fupcomlib.h"

#include "comhead.h"

#include "com9003.h"
#include "com9103.h"
#include "com9001.h"
#include "com9101.h"
#include "com9104.h"
#include "com9105.h"

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

int MyDiskFileRequestNextFile(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	
	HEADER headers;
	memset(&headers,0x00,HEADER_SIZE);
	
	pSendData = new char[sizeof(HEADER)];
	memset(pSendData,0x00,sizeof(HEADER));
	
	headers.nCmd = RS_MYDISK_FILE_REQUEST_NEXT_FILEINFO;//RS_MYDISK_FILE_REQUEST_FILE_FILINFO; //파일 정보 요청
	headers.nDataCnt = 0;
	headers.nDataSize = 0;
	
	memcpy(pSendData,&headers,sizeof(HEADER));
	
	
	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));//return 1;
}

//11월 15일 여기 까지 

int MyDiskFileRequestFile(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{

	HEADER headers;
	memset(&headers,0x00,HEADER_SIZE);
	
	pSendData = new char[sizeof(HEADER)];
	memset(pSendData,0x00,sizeof(HEADER));
	
	headers.nCmd = RS_MYDISK_FILE_REQUEST_FILE_FILINFO;//RS_MYDISK_FILE_REQUEST_FILE_FILINFO; //파일 정보 요청
	headers.nDataCnt = 0;
	headers.nDataSize = 0;
	
	memcpy(pSendData,&headers,sizeof(HEADER));

	
	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));//return 1;
}



int MyDiskFileRequestList(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	

	///// 파일 정보 구조체 생성 ////
	
	
	LPHEADER pHeader = (LPHEADER)pRecvHead; //head
	
	ERR_HEADER errheader; //err head
	memset(&errheader,0x00,sizeof(ERR_HEADER));
	
	HEADER headers; //temp head
	memset(&headers,0x00,sizeof(HEADER));
	
	LPMFILEINFO pFileinfo = (LPMFILEINFO)pRecvData; //body
	

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
	
	sprintf(szFullName,"%s/%04d/%02d/%02d/%02d"
									, pFileinfo->szDownPath
									,  stm->tm_year+1900
									,  stm->tm_mon + 1
									,  stm->tm_mday
									,  stm->tm_hour);
								//	,pFileinfo->szFolderName); //./2004/02/18/16/raid


        sprintf(szFolderName,"%lu", pFileinfo->cfups4003.id);
		sprintf(szCheckName,"%s/%s",szFullName,szFolderName);

		
		#ifdef __DEBUG
//				01234567890123456789 ]
		printf("FileRequestList      ] 폴더 검색	 %s\n",szCheckName);
		#endif		
		
			
		int stat = lstat64(szCheckName,&statbuf);
		if(stat != 0) //파일이 없으면 폴더 만들기.
		{
			#ifdef __DEBUG
//					01234567890123456789 ]
			printf("FileRequestList      ] 폴더 생성   %s\n",szCheckName);
			#endif		
			
			if(MakeFolder(szCheckName)== -1)
			{
				#ifdef __DEBUG
//						01234567890123456789 ]
				printf("FileRequestList      ] 폴더 생성 실패 %s\n",szCheckName);
				#endif		
			}
		}
		else
		{
		    infLOG(ERROR, "FileDataTransfer  ERR]  === 폴더 생성 오류 === \n"); 	
		}
		
/*	
	while(bResult == false)
	{
		nCheckLoop++;
		if( nCheckLoop > 50 ) //폴더를 생성하지 못하였을때.
		{
			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
			strcat(errheader.errmsg,"서버 에서 폴더 만들기를 실패 하였습니다.");
			
			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));
		
			return -RS_MYDISK_FILE_REQUEST_LIST;			
		}
		
				
		time_t	curtime;
		struct tm		*stm;
		time( &curtime );
		stm = (struct tm *) localtime(&curtime);
	
		localtime_r(&curtime, stm);
		
		sprintf(szFullName,"%s/%04d/%02d/%02d/%02d"
										, pFileinfo->szDownPath
										,  stm->tm_year+1900
										,  stm->tm_mon + 1
										,  stm->tm_mday
										,  stm->tm_hour);
									//	,pFileinfo->szFolderName); //./2004/02/18/16/raid
	
		
				
	
		//folder 이름 얻기
		
		
		
		srand((unsigned int)time(NULL))	; //random 이름을 위함 시드 지정
		
		nFolderName = random()%100000000; 	
		
		strcpy(szCheckName,szFullName);
		strcat(szCheckName,"/");
		sprintf(szFolderName,"%ld",nFolderName);
		strcat(szCheckName,szFolderName);
		
		
		#ifdef __DEBUG
		printf("MyDiskFileRequestList] fullname %s \n" 
		       "MyDiskFileRequestList] checkName %s\n",szFullName,szCheckName);
		#endif
			
		int stat = lstat64(szCheckName,&statbuf);
		if(stat != 0) //파일이 없으면 폴더 만들기.
		{
			if(MakeFolder(szCheckName)== -1)
			{

			}
			bResult = true; //파일 이름이 결정 되었으면...true							
		}
		else // 같은 이름이 아닐때 까지 ...
			bResult = false;		
	
	}
*/	
	
	headers.nCmd = RS_MYDISK_FILE_REQUEST_LIST;

	headers.nDataCnt =1;
	headers.nDataSize = sizeof(MFILEINFO);
	headers.nErrorCode = 0;
	
	MFILEINFO FolderInfo;
	memset(&FolderInfo,0x00,sizeof(MFILEINFO));
	
	memcpy(&FolderInfo,pFileinfo,sizeof(MFILEINFO));
	
	strcpy(FolderInfo.szDownPath,szFullName);
	#ifdef __DEBUG
	printf("MyDiskFileRequestList] %s\n",FolderInfo.szDownPath);
	#endif
	
	memcpy(FolderInfo.cfups4003.file_path,szFullName,sizeof(szFullName)); //서버측 패스
	#ifdef __DEBUG
	printf("MyDiskFileRequestList] 서버측 패스 : %s    /  %s\n",pFileinfo->szDownPath,szFullName);
	#endif

	memcpy(FolderInfo.cfups4003.file_name2,pFileinfo->szFolderName,sizeof(pFileinfo->szFolderName)); //로컬파일 이름
	
	#ifdef __DEBUG
	printf("MyDiskFileRequestList] 로컬측 파일 이름 : %s\n",pFileinfo->szFolderName);
	#endif

	
	memcpy(FolderInfo.cfups4003.file_name1,szFolderName,sizeof(szFolderName));			 //서버파일

	#ifdef __DEBUG
	printf("MyDiskFileRequestList] 서버측 파일 이름 : %s\n",szFolderName);
	#endif

			
	
	
	pSendData = new char[sizeof(HEADER) + headers.nDataCnt * headers.nDataSize];
	memset(pSendData,0x00,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize);
	
	memcpy(pSendData,&headers,sizeof(HEADER)); //head
	memcpy(pSendData+sizeof(HEADER),&FolderInfo,  headers.nDataCnt * headers.nDataSize);
	
	
	
	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));

	
}


//필로그 & 내자료실
int MyDiskFileDataTransfer(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	
	infLOG(ALWAY, "MyDiskFileDataTran   ] FileDataTransfer\n");
	#ifdef __DEBUG
	printf("MyDiskFileDataTran   ] loading file data transfer...\n");
	#endif
	///// 파일 정보 구조체 생성 ////
	

	bool bFOpenAppendMode = false;	        // 파일 열기 모드 
	int stat = -1;                          // 파일 상태
	struct stat64 statbuf;                  
	LPHEADER pHeader = (LPHEADER)pRecvHead; //head
	
	ERR_HEADER errheader; //err head
	memset(&errheader,0x00,sizeof(ERR_HEADER));
	
	HEADER headers; //temp head
	memset(&headers,0x00,sizeof(HEADER));
	
	LPMFILEINFO pFileinfo = (LPMFILEINFO)pRecvData; //body
	
	//9001 호출 // 사용자수 증가
	//9101 호출 //사용자수 감소			
				
	CCOM9001_R com9001_r ;  
	memset(&com9001_r,0x00,sizeof(CCOM9001_R));

	multimap<int,USERINFO>::iterator mi; //IP 조회
	
	mi = m_UserList.find(Socket);
	if(mi != m_UserList.end())
	{
		strcpy(com9001_r.conn_ip ,mi->second.thread.userIP);
	}
	
	strcpy(com9001_r.server_id , pFileinfo->szServerID);
	com9001_r.temp_id =  pFileinfo->cfups4003.id;
	memcpy(com9001_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
	com9001_r.upload_size = pFileinfo->cfups4003.file_size;
	

	com9001 ( com9001_r );

	CCOM9101_R com9101_r ;
	
	memset(&com9101_r,0x00,sizeof(CCOM9101_R));
	strcpy(com9101_r.conn_ip , com9001_r.conn_ip);
	strcpy(com9101_r.server_id , com9001_r.server_id);
	com9101_r.temp_id =  com9001_r.temp_id;
	strcpy(com9101_r.user_id ,com9001_r.user_id); // 사용자
	com9101_r.upload_size = com9001_r.upload_size;
	
	
	
	
	#ifdef __DEBUG
	printf("MyDiskFileDataTran   ] Recv Fileinfo title : %s\n",pFileinfo->cfups4003.title);
	printf("MyDiskFileDataTran   ] list path ( %s )\n",pFileinfo->cfups4003.file_path);
	#endif
	

	char strFullPath[768];
	memset(strFullPath,0x00,sizeof(strFullPath));
	char szFullName[768];
	memset(szFullName,0x00,sizeof(szFullName));


	
	CCOM9104_R pcom9104_r; // 받던 파일 취소시 DB 돌리기 ( T_CONTENTS_TEMP 삭제 )

	MFILEINFO rMFileinfo;
	
	double dTotalRecvLen = 0; //총 받은 길이
	double dTotalLen = 0; // down될 파일의 총 길이
	int nWriteLen=0;      // 파일에 쓴 길이
	int nRecvLen=0;       // 소켓으로 부터 받은 길이
	bool bCreateFile=false;
		
	int nCheckStop = 0;  // while 문 제어
	
	/*
	char szErrMsg[256];
	memset(szErrMsg,0x00,sizeof(szErrMsg));
	*/
	CCOM9105_R com9105_r; // temp 에 파일 정보 저장.	
	memset(&com9105_r,0x00,sizeof(CCOM9105_R));
	
	CCOM9103_R pcom9103_r; // 필로그 용량 제어
	memset(&pcom9103_r,0x00,sizeof(CCOM9103_R));
	

	if(	pFileinfo->nType == FT_FILE)
	{
		pcom9103_r.proc_flag  =  2;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
		pcom9103_r.file_size = pFileinfo->dFileSize ;
		memcpy(pcom9103_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID));
		
		if(com9103(pcom9103_r,errheader.errmsg)< 0)
		{
			infLOG(ALWAY, "MyDiskFileDataTran ER] Update mydisk place (com9103)\n"); 	
			#ifdef __DEBUG
			printf("MyDiskFileDataTran ER] Update mydata place (com9103)\n"); 	
			#endif
			
			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_MYDISK_FILE_DATA_TRANSFER;
			//strcpy(errheader.errmsg,szErrMsg);
			
			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));
	
			pHeader->nCmd = RS_ERR; // 에러 처리 
			com9101 ( com9101_r);
			
			return -RS_MYDISK_FILE_DATA_TRANSFER;
			
		}
	}
	else 
	{
		
		
		#ifdef __DEBUG
		printf("MyDiskFileDataTran   ] pFileinfo->nfupFlag = ( %d ) pFileinfo->nNumber = ( %d )\n",pFileinfo->nfupsFlag,pFileinfo->nNumber);
		#endif 
		
		pcom9103_r.proc_flag  =  2;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
		pcom9103_r.file_size = pFileinfo->cfups4003.file_size;
		memcpy(pcom9103_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID));
		
		if(com9103(pcom9103_r,errheader.errmsg)< 0)
		{
			infLOG(ALWAY, "MyDiskFileDataTran ER] Update mydata place (com9103)\n"); 	
			#ifdef __DEBUG
			printf("MyDiskFileDataTran ER] Update mydata place (com9103)\n"); 	
			#endif
			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_MYDISK_FILE_DATA_TRANSFER;
			//strcpy(errheader.errmsg,szErrMsg);
			
			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));
	
			pHeader->nCmd = RS_ERR;
			
			com9101 ( com9101_r);
			return -RS_MYDISK_FILE_DATA_TRANSFER;
			
		}
		
		//9105 폴더	
		com9105_r.id = pFileinfo->cfups4003.id;  
		memcpy(&com9105_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
		strcpy(com9105_r.server_id ,pFileinfo->szServerID);
		strcpy(com9105_r.sfile_path ,pFileinfo->cfups4003.file_path);
		strcpy(com9105_r.sfile_name ,pFileinfo->cfups4003.file_name1);
		
		com9105(com9105_r);

	
	}

		
	do
	{
		nCheckStop++;

		if(nCheckStop >= 1100)
		{
		
			///////////////////////////////////////////////
			// temp 삭제									 //
			// 서버 용량 (upload_size) 업데이트 rollback  //
			// 파일 삭제 								 //
			///////////////////////////////////////////////
			
			memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
			
			
			
			pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
			pcom9104_r.id         = pFileinfo->cfups4003.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
			memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
			pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
			if(com9104(pcom9104_r) < 0)
			{
				infLOG(ERROR, "Exception )  내자료실 rollback 오류 (com9104)\n"); 	
			}

	
			com9101 ( com9101_r);

			return 0;			
						
		}			

	    	
		headers.nCmd  = RS_FILE_DATA_SIGN_CHECK; // 파일 전송 메세지    	
		
		if(pFileinfo->nReUploadFlag == RECONNECT_UPLOAD || pFileinfo->nReUploadFlag == RE_UPLOAD)
		{
	    	#ifdef __DEBUG
	    	printf("MyDiskFileDataTran   ] ( %d )\n" ,pFileinfo->nReUploadFlag);
	    	#endif
	    	
	    				
			if( pFileinfo->nType == FT_FOLDER)
			{
				strcpy(strFullPath, pFileinfo->cfups4003.file_path); //./2004/02/18/16/raid
				strcat(strFullPath,"/");
				strcat(strFullPath,pFileinfo->cfups4003.file_name1);//<- 요거 값 잘못 들어옴
				
				//////////////////////////////////////////////////////////////////////////
				
				strcpy(szFullName,pFileinfo->szDownPath); //szDownPath 은 ./raid
    			strcat(szFullName,"/");
    			strcat(szFullName,pFileinfo->szFileName); //szfilename 은 a.txt
    			
    			#ifdef __DEBUG
				printf("MyDiskFileDataTran   ] FOLDER full path ( %s ) full name ( %s )\n",strFullPath,szFullName);
				#endif	
				infLOG(ERROR, "FileDataTransfer   RE] FOLDER full path ( %s ) full name ( %s )\n",strFullPath,szFullName);
								
			}
			else
			{
				
		    	strcpy(szFullName,pFileinfo->szDownPath);					
				strcat(szFullName,"/"); //./2004/02/18/16/raid/   <-- '/' 추가
				strcat(szFullName,pFileinfo->szFileName);
				
				strcpy(strFullPath, pFileinfo->cfups4003.file_path); //./2004/02/18/16/raid
				strcat(strFullPath,"/");
				strcat(strFullPath,pFileinfo->cfups4003.file_name1);//
				
				infLOG(ERROR, "FileDataTransfer   RE] FOLDER full path ( %s ) full name ( %s )\n",strFullPath,szFullName);
				
				//9105 파일
				
				memset(&com9105_r,0x00,sizeof(CCOM9105_R));
			
				com9105_r.id = pFileinfo->cfups4003.id;  
				memcpy(&com9105_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
				strcpy(com9105_r.server_id ,pFileinfo->szServerID);
				strcpy(com9105_r.sfile_path ,pFileinfo->szDownPath);
				strcpy(com9105_r.sfile_name ,pFileinfo->szFileName);
				
				com9105(com9105_r);
						
				#ifdef __DEBUG
				printf("MyDiskFileDataTran   ] FOLDER full path ( %s ) full name ( %s )\n",strFullPath,szFullName);
				#endif							
					
			
					
									
			}

						
							 
			int stat = stat64(szFullName,&statbuf); 
			if(stat != 0) //파일이 없으면 폴더 만들기.
			{
				#ifdef __DEBUG
				printf("MyDiskFileDataTran   ] the file is not visible..\n");
				#endif
				//if(errno == ENOENT) //파일이나 패스가 없음
				{
					MakeFolder(pFileinfo->szDownPath) ;
					#ifdef __DEBUG
					printf("MyDiskFileDataTran   ] create folder : %s\n",pFileinfo->szDownPath);	
					#endif
				}	
			}
			else
			{
				bFOpenAppendMode = true;
				#ifdef __DEBUG

			//			01234567890123456789]
				printf("MyDiskFileDataTran   ] 폴더 가 있음 		] %s\n",pFileinfo->szDownPath);
				
				#endif
			}
						
		}
		else
		{
			
			#ifdef __DEBUG
	    	printf("MyDiskFileDataTran   ] NORMAL_UPLOAD\n" );
	    	#endif					
			
			bCreateFile = false;// 같은 이름이 생성되었는지 check
			
			///// 날짜 시간 생성 ////
			time_t			curtime;
			struct tm		*stm;
			time( &curtime );
			stm = (struct tm *) localtime(&curtime);
		
			localtime_r(&curtime, stm);
			bool bResult = false;
			int nCheckLoop = 0;	
			
			srand((unsigned int)time(NULL))	; //random 이름을 위함 시드 지정
						
			if(pFileinfo->nType == FT_FILE)
			{
				sprintf(pFileinfo->szDownPath,"%s/%04d/%02d/%02d/%02d",  pFileinfo->szDownPath
							      ,  stm->tm_year+1900
							      ,  stm->tm_mon + 1
							      ,  stm->tm_mday
							      ,  stm->tm_hour);//./2004/02/18/16
							      
				
	
				memset(szFullName,0x00,sizeof(szFullName));					
		    	
		    	strcpy(szFullName,pFileinfo->szDownPath);					
				strcat(szFullName,"/"); //./2004/02/18/16/raid/   <-- '/' 추가
		    	//file name 얻기
		    	char szFilename[50];
		    	char szFileType[10];
		    	memset(szFilename,0x00,sizeof(szFilename));
		    	memset(szFileType,0x00,sizeof(szFileType));
		    	
		    	
				sprintf(szFilename,"%lu",pFileinfo->cfups4003.id);		
		    	
		    	//local 파일이름으로 부터 확장자 얻기.
		    	int nLen = GetReverseIndex(pFileinfo->cfups4003.file_name2 , '.');
				
				
				infLOG(ALWAY, " MyDiskFileDataTran   ] File 이름 ( %s ) \n",pFileinfo->cfups4003.file_name2); 
				if(nLen < 0)
				{
					#ifdef __DEBUG
					printf("MyDiskFileDataTran   ] 확장자를 가지고 있지 않습니다.  %s\n",pFileinfo->cfups4003.file_name2);		
					#endif
					
					infLOG(ERROR, "MyDiskFileDataTran ER] 확장자를 가지고 있지 않습니다.\n"); 
					
				}
				else		
					GetRightString(pFileinfo->cfups4003.file_name2 ,strlen(pFileinfo->cfups4003.file_name2 )-nLen,szFileType);
				
					
				
				strcpy(pFileinfo->cfups4003.file_name2,pFileinfo->szFileName);
				memset(pFileinfo->szFileName,0x00,sizeof(pFileinfo->szFileName));
				strcpy(pFileinfo->szFileName,szFilename);
				strcat(pFileinfo->szFileName,szFileType);
				strcat(szFullName,szFilename);
				strcat(szFullName,szFileType);
				
				//// 이름 저장 ////
				memcpy(pFileinfo->cfups4003.file_name1,pFileinfo->szFileName,sizeof(pFileinfo->szFileName));	
				strcpy(pFileinfo->cfups4003.file_path,pFileinfo->szDownPath);//,sizeof(pFileinfo->szDownPath));
										
			
				

				stat = stat64(szFullName,&statbuf); 
				
				infLOG(ALWAY, "MyDiskFileDataTran   ] File 확인  ( %s ) ( %s ) (%d)\n",pFileinfo->szDownPath,szFullName,stat); 
				
				if(stat != 0) //파일이 없으면 폴더 만들기.
				{

					//if(errno == ENOENT) //파일이나 패스가 없음
					{
						MakeFolder(pFileinfo->szDownPath) ;
						#ifdef __DEBUG
					//			01234567890123456789]
						printf("MyDiskFileDataTran   ] 폴더 생성  %s\n",pFileinfo->szDownPath);
						#endif
					}		
					bResult = true;	
				}
				else
				{
					#ifdef __DEBUG
				//			01234567890123456789]
					printf("MyDiskFileDataTran   ] 폴더 가 있음  %s\n",szFullName);
					#endif
					
					bResult = false;
				}
					

				//9105 파일					
				memset(&com9105_r,0x00,sizeof(CCOM9105_R));
			
				com9105_r.id = pFileinfo->cfups4003.id;  
				memcpy(&com9105_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
				strcpy(com9105_r.server_id ,pFileinfo->szServerID);
				strcpy(com9105_r.sfile_path ,pFileinfo->cfups4003.file_path);
				strcpy(com9105_r.sfile_name ,pFileinfo->cfups4003.file_name1);
				
				com9105(com9105_r);
										
			}
			else if(pFileinfo->nType == FT_FOLDER) //전송 받을 파일이 폴더 일경우 
			{	


				strcpy(strFullPath, pFileinfo->cfups4003.file_path); //./2004/02/18/16/raid
				strcat(strFullPath,"/");
				strcat(strFullPath,pFileinfo->cfups4003.file_name1); 
		
				//////////////////////////////////////////////////////////////////////////
					
				strcpy(szFullName,pFileinfo->szDownPath); //szDownPath 은 ./raid
				strcat(szFullName,"/");
				strcat(szFullName,pFileinfo->szFileName); //szfilename 은 a.txt
		    	///// 파일 정보를 얻기 위한 구조체  ////
		    	
   				#ifdef __DEBUG
				printf("MyDiskFileDataTran   ] full path ( %s ) full name ( %s )\n",strFullPath,szFullName);
				#endif	

				int stat = stat64(szFullName,&statbuf); 
				if(stat != 0) //파일이 없으면 폴더 만들기.
				{

					//if(errno == ENOENT) //파일이나 패스가 없음
					{
						MakeFolder(pFileinfo->szDownPath) ;
						#ifdef __DEBUG
					//			01234567890123456789]
						printf("MyDiskFileDataTran   ] 폴더 생성  %s\n",pFileinfo->szDownPath);
						#endif
					}		
				}
				else
				{
					#ifdef __DEBUG
					//			01234567890123456789]
					printf("MyDiskFileDataTran   ] 폴더 가 있음  %s\n",szFullName);
					#endif
					
					bFOpenAppendMode = true;	 
	
				}	
					
			}		
		}	
					
	//		}
	
		headers.nCmd = RS_FILE_DATA_SIGN_CHECK; //파일 전송
	
		int nSRet = -1;
		if( pFileinfo->nType == FT_FILE )
		{
			
		    headers.nDataCnt = 1;
			headers.nDataSize = sizeof(MFILEINFO);
			headers.nErrorCode = 0;
			
			/*
			pSendData = new char[HEADER_SIZE + headers.nDataCnt*headers.nDataSize];
			memset(pSendData,0x00,HEADER_SIZE + headers.nDataCnt*headers.nDataSize);
			
			memcpy(pSendData,&headers,HEADER_SIZE);
			memcpy(pSendData + HEADER_SIZE , pFileinfo , sizeof(MFILEINFO));
			
			nSRet = SendData(Socket,pSendData,HEADER_SIZE + headers.nDataCnt*headers.nDataSize);
			*/
			
			char szSendData[HEADER_SIZE + sizeof(MFILEINFO)];
			memset(szSendData,0x00,HEADER_SIZE + sizeof(MFILEINFO));
		
			
			memcpy(szSendData,&headers,HEADER_SIZE);
			memcpy(szSendData + HEADER_SIZE  , pFileinfo , sizeof(MFILEINFO));
			
			nSRet = SendData(Socket,szSendData,HEADER_SIZE + headers.nDataCnt*headers.nDataSize);
			
			//delete[] pSendData;
			//pSendData = NULL;
			
			
		}
		else
		{
			
		    headers.nDataCnt = 0;
			headers.nDataSize = 0;
			headers.nErrorCode = 0;
			nSRet = SendData(Socket,(char*)&headers,sizeof(struct _HEADER));			
		}
			  
	    
	    
	
	    //// 전송하기전에 메세지를 알림...
//	    if(SendData(Socket,(char*)&headers,sizeof(struct _HEADER))<0)  //struct _PACKET == PACKET
		if( nSRet <= 0)
		{
			infLOG(ERROR, "MyDiskFileDataTran ER] send 에러 1: <client 죽음>\n"); 
			#ifdef __DEBUG
			printf("MyDiskFileDataTran ER] send 에러 1 : <client 죽음>\n");
			#endif
			if( pFileinfo->nType == FT_FILE )			
			{
				delete[] pSendData;
				pSendData = NULL;
			}
				
			///////////////////////////////////////////////
			// temp 삭제									 //
			// 서버 용량 (upload_size) 업데이트 rollback  //
			// 파일 삭제 								 //
			///////////////////////////////////////////////
			
			memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
			
			
			
			pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
			pcom9104_r.id         = pFileinfo->cfups4003.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
			memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
			pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
			if(com9104(pcom9104_r) < 0)
			{
				infLOG(ERROR, "MyDiskFileDataTran ER] 내자료실 rollback 오류 (com9104)\n"); 	
			}						
		
			
			com9101 ( com9101_r);
			// 받던 파일 삭제
			
			return 0;
		}
	
	
		memset(&headers,0x00,sizeof(HEADER));
		
		if(	RecvData(Socket,(char*)&headers,sizeof(struct _HEADER))<0)  //struct _PACKET == PACKET
		{
			infLOG(ERROR, "MyDiskFileDataTran ER] recv 에러 1: <client 죽음>\n"); 
			#ifdef __DEBUG
			printf("MyDiskFileDataTran ER] recv 에러 1 : <client 죽음>\n");
			#endif
			
			
			///////////////////////////////////////////////
			// temp 삭제									 //
			// 서버 용량 (upload_size) 업데이트 rollback  //
			// 파일 삭제 								 //
			///////////////////////////////////////////////
			
			memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
			
			
			
			pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
			pcom9104_r.id         = pFileinfo->cfups4003.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
			memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
			pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
			if(com9104(pcom9104_r) < 0)
			{
				infLOG(ERROR, "MyDiskFileDataTran ER]  내자료실 rollback 오류 (com9104)\n"); 	
			}						
		
		
			
			com9101 ( com9101_r);	
			// 받던 파일 삭제
			return 0;
		}
		
		if(headers.nCmd == RS_EOL)
		{
			#ifdef __DEBUG
			printf("MyDiskFileDataTran   ] file recv cancel..\n");
			#endif
			
			pSendData = new char[sizeof(HEADER)];
			memset(pSendData,0x00,sizeof(HEADER));
			
			headers.nCmd = RS_EOL;	
			headers.nDataCnt = 0;
			headers.nDataSize = 0;
			headers.nErrorCode = 0;
			
			memcpy(pSendData, &headers, sizeof(HEADER));		
			
			
			
			///////////////////////////////////////////////
			// temp 삭제									 //
			// 서버 용량 (upload_size) 업데이트 rollback  //
			// 파일 삭제 								 //
			///////////////////////////////////////////////
			
			memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
			
			
			
			pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
			pcom9104_r.id         = pFileinfo->cfups4003.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
			memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
			pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
			if(com9104(pcom9104_r) < 0)
			{
				infLOG(ERROR, "MyDiskFileDataTran ER]  내자료실 rollback 오류 (com9104)\n"); 	
			}						
		
		
			
			com9101 ( com9101_r);	

			// 받던 파일 삭제
				
			return END;
		}
		if(headers.nCmd == RS_OK)
		{
			#ifdef __DEBUG
			printf("MyDiskFileDataTran   ] file recv ok..");
			#endif
		}
		
		////////////////////기본 정보 완료////////////////////////////////////////////////
		
	
		
		
		
		// 파일 제어 및 전송
		FILE* DownloadFile; //파일 포인터
		DownloadFile = NULL;
		//// 파일 open형식 결정////
		#ifdef __DEBUG
		printf("MyDiskFileDataTran   ] open file : %s\n",szFullName);
		#endif
		
			
		if( bFOpenAppendMode) //append mode
		{
			#ifdef __DEBUG
			printf("MyDiskFileDataTran   ] open append mode\n");
			#endif
			DownloadFile = fopen64(szFullName,"a+tb");	
		}
		else
		{
			#ifdef __DEBUG
			printf("MyDiskFileDataTran   ] open write mode\n");	
			#endif
			DownloadFile = fopen64(szFullName,"w+tb");
		}		

		infLOG(ALWAY, "MyDiskFileDataTran   ] UploaddFile ( %s )\n",szFullName);	
				
		if(DownloadFile == NULL) //파일을 열수 없으면
		{
			#ifdef __DEBUG
			char szErrMsg[1024];
			memset(szErrMsg,0x00,sizeof(szErrMsg));
			GetErrMsg(errno,szErrMsg);
			
			printf("MyDiskFileDataTran ER] file error : file is null\n"
			       "MyDiskFileDataTran ER]  file error : %s \n ",szErrMsg);
			       
			#endif
			infLOG(ERROR, "MyDiskFileDataTran ER] DownloadFile == NULL : <메세지 전달>\n"); 
			#ifdef __DEBUG
			printf("MyDiskFileDataTran ER] DownloadFile == %s : <메세지 전달>\n",szFullName);
			#endif
			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_MYDISK_FILE_DATA_TRANSFER;
			strcat(errheader.errmsg,"서버 에서 파일 만들기 실패 하였습니다.");
			
			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));
		
			pHeader->nCmd = RS_ERR;
			
			///////////////////////////////////////////////
			// temp 삭제									 //
			// 서버 용량 (upload_size) 업데이트 rollback  //
			// 파일 삭제 								 //
			///////////////////////////////////////////////
			
			memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
			
			
			
			pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
			pcom9104_r.id         = pFileinfo->cfups4003.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
			memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
			pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
			if(com9104(pcom9104_r) < 0)
			{
				infLOG(ERROR, "MyDiskFileDataTran ER]  내자료실 rollback 오류 (com9104)\n"); 	
			}						
		
		
			
			com9101 ( com9101_r);	

			// 받던 파일 삭제			
			return -RS_MYDISK_FILE_DATA_TRANSFER;
		}
		
		
		//// 이어 올리기를 위한 파일 해더 구조체 생성 ////
	
		//// 파일 내부 포인터를 끝으로 이동////
	
	//	if(fseek(DownloadFile,0l,SEEK_END)!=0) //err
		if(fseeko64(DownloadFile,0l,SEEK_END))
		{
			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_MYDISK_FILE_DATA_TRANSFER;
			strcat(errheader.errmsg,"서버 파일 섹터 이동 실패 하였습니다.");
			
			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));
		
			pHeader->nCmd = RS_ERR;
			
			///////////////////////////////////////////////
			// temp 삭제									 //
			// 서버 용량 (upload_size) 업데이트 rollback  //
			// 파일 삭제 								 //
			///////////////////////////////////////////////
			
			memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
			
			
			
			pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
			pcom9104_r.id         = pFileinfo->cfups4003.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
			memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
			pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
			if(com9104(pcom9104_r) < 0)
			{
				infLOG(ERROR, "MyDiskFileDataTran ER] 내자료실 rollback 오류 (com9104)\n"); 	
			}						
		
		
			
			com9101 ( com9101_r);	

			// 받던 파일 삭제
			return -RS_MYDISK_FILE_DATA_TRANSFER;
		}

		LPFILEHEAD pFileHead = new FILEHEAD;
		memset(pFileHead,0x00,sizeof(FILEHEAD));
		
		
		//double dCurrentLen = (double)ftello64 (DownloadFile); // 파일이 어디까지 있는지 결정
		
		double dCurrentLen = 0;
	//			dCurrentLen = (double)statbuf.st_size;
		dCurrentLen = (double)ftello64 (DownloadFile); // 파일이 어디까지 있는지 결정

		
		
		infLOG(ALWAY, " MyDiskFileDataTran   ] Current File Size ( %15.0f )\n",dCurrentLen); 
	
		if(dCurrentLen < 0)
			dCurrentLen = 0;
			
		pFileHead->dCurrentSize = dCurrentLen; //해드에 현 파일 길이 저장		
		#ifdef __DEBUG
		printf("MyDiskFileDataTran   ] dCurrentLen : %15.0f\n",dCurrentLen);
		#endif
	
		
		// head 작성
		memset(&headers,0x00,sizeof(HEADER));
	
		//key
		headers.nCmd = RS_MYDISK_FILE_DATA_TRANSFER ; // 데이터 전송		
		headers.nDataCnt = 1;
		headers.nDataSize = sizeof(FILEHEAD);
		headers.nErrorCode = 0;
	
		pSendData = new char[sizeof(HEADER) + headers.nDataCnt*headers.nDataSize];
	
		memcpy(pSendData,&headers,sizeof(HEADER));
		
		memcpy(pSendData + HEADER_SIZE,pFileHead, headers.nDataCnt*headers.nDataSize);
		
		//// body 작성////
		if(SendData(Socket,pSendData,HEADER_SIZE + headers.nDataCnt*headers.nDataSize)<0)  //struct _PACKET == PACKET
		{
			delete pFileHead;
			///////////////////////////////////////////////
			// temp 삭제									 //
			// 서버 용량 (upload_size) 업데이트 rollback  //
			// 파일 삭제 								 //
			///////////////////////////////////////////////
			
			memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
			
			
			
			pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
			pcom9104_r.id         = pFileinfo->cfups4003.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
			memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
			pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
			if(com9104(pcom9104_r) < 0)
			{
				infLOG(ERROR, "MyDiskFileDataTran ER]  내자료실 rollback 오류 (com9104)\n"); 	
			}						
		
		
			
			com9101 ( com9101_r);	

			// 받던 파일 삭제

			
			return 0;
		}
		delete[] pSendData;
		pSendData = NULL;
				
		delete pFileHead;
		
		#ifdef __DEBUG
		printf("MyDiskFileDataTran   ] file Head  send ok..%d\n",headers.nCmd);
		#endif
		
		dTotalRecvLen = 0; //총 받은 길이
		dTotalLen = pFileinfo->dFileSize - dCurrentLen; // down될 파일의 총 길이
		#ifdef __DEBUG
		printf("MyDiskFileDataTran   ] dCurrentLen : %15.0f = %15.0f - %15.0f\n",dTotalLen,pFileinfo->dFileSize,dCurrentLen);
		#endif
				
		nWriteLen=0;
		nRecvLen=0;
		
		char* szRecvBuffer = new char[RECVBUF]; //recv buffer
	

		infLOG(ALWAY, "MyDiskFileDataTran   ] File Send Body \n\n"); 		
		
		
		#ifdef __DEBUG
		printf("MyDiskFileDataTran   ] Checking File (%s) : 파일 전체 길이 (%15.0f ) = %15.0f\n ",pFileinfo->cfups4003.file_name2 , dTotalLen ,pFileinfo->dFileSize );
		#endif
				
		infLOG(ALWAY,"MyDiskFileDataTran   ] Checking File (%s) : 파일 전체 길이 (%15.0f ) =  %15.0f \n",pFileinfo->cfups4003.file_name2 , dTotalLen ,pFileinfo->dFileSize );
		
			
		int fno = fileno(DownloadFile);	
		
		while(dTotalLen > 0  ) 
		{   	
			memset(szRecvBuffer,0x00,RECVBUF);	
			
			///// 파일받기 ///// ///// 내 자료는 통합 처리 안함.
			nRecvLen =  RecvFileData(Socket, szRecvBuffer, RECVBUF, dTotalLen);
	        
	        
	        if(nRecvLen > 0)
	        {
				nWriteLen = write(fno ,szRecvBuffer,nRecvLen);
	      	//	nWriteLen = fwrite(szRecvBuffer,nRecvLen,1,DownloadFile);
	        }
	        else 
	        	nWriteLen = 0;
		    
	    	if(nWriteLen <= 0)
        	{
        		if(nWriteLen == 0)
        		{
        			#ifdef __DEBUG
        			printf("MyDiskFileDataTran   ] file의 끝\n");
        			#endif
        			infLOG(ALWAY,"MyDiskFileDataTran   ] Write File End (%s) : 파일 전체 길이 (%15.0f ) =  %15.0f \n",pFileinfo->cfups4003.file_name2 , dTotalLen ,pFileinfo->dFileSize );
        			
        		}
        		else	
        		{
        			infLOG(ERROR,"MyDiskFileDataTran   ] Write File ERROR (%s) : 파일 전체 길이 (%15.0f ) =  %15.0f \n",pFileinfo->cfups4003.file_name2 , dTotalLen ,pFileinfo->dFileSize );
        			nRecvLen = -1;
        		}
        	}


	        if(nRecvLen <= 0 && dTotalLen != 0)	//받다가 오류가 났을시...DB처리
	        {
			   	if(szRecvBuffer)
					delete[] szRecvBuffer;	        	
	        	if(nRecvLen < 0)
	        	{
	        		char szErrMsg[1024];
					memset(szErrMsg,0x00,sizeof(szErrMsg));
					GetErrMsg(-nRecvLen,szErrMsg);
					infLOG(ERROR, " MyDiskFileDataTran ER] RecvSocket Error ( %d )( %s )\n",nRecvLen,szErrMsg); 
					
					#ifdef __DEBUG
					printf("MyDiskFileDataTran ER]RecvSocket Error ( %d )( %s )\n",nRecvLen,szErrMsg); 
					#endif
					

	        	}
	        	else if(nRecvLen == 0)
	        	{
	        		char szErrMsg[1024];
					memset(szErrMsg,0x00,sizeof(szErrMsg));
					GetErrMsg(-nRecvLen,szErrMsg);
						        		
	        		infLOG(ERROR, " MyDiskFileDataTran ER] RecvSocket Error ( 접속을 끊었습니다. ) (%s)\n",szErrMsg); 
					
					#ifdef __DEBUG
					printf("MyDiskFileDataTran ER] RecvSocket Error ( 접속을 끊었습니다. ) (%s)\n",szErrMsg); 
					#endif
	        	}       	
				#ifdef __DEBUG
				printf("MyDiskFileDataTran   ] mydisk file recv exception \n");
				#endif														
		
				if(DownloadFile)
					fclose(DownloadFile);
		

	
			// temp 에 저장하기

							
				
				

				///////////////////////////////////////////////
					
				infLOG(ERROR,"MyDiskFileDataTran ER] 내 자료실 파일 받기 취소 (%s) RecvLen (%d) dTotalRecvLen(%15.0f) TotalLen(%15.0f)\n"
				, pFileinfo->cfups4003.file_name2,nRecvLen  ,dTotalRecvLen,dTotalLen);  		
				

				//////////////////////////////////////////
				///////////////////////////////////////////////
				// temp 삭제									 //
				// 서버 용량 (upload_size) 업데이트 rollback  //
				// 파일 삭제 								 //
				///////////////////////////////////////////////
				
				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
				
				
				
				pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
				pcom9104_r.id         = pFileinfo->cfups4003.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
				pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
				if(com9104(pcom9104_r) < 0)
				{
					infLOG(ERROR, "MyDiskFileDataTran ER]  내자료실 rollback 오류 (com9104)\n"); 	
				}						
			
			
				
				com9101 ( com9101_r);	
	
				// 받던 파일 삭제
				
				//return END;
				return 0;
        	
        	}        
	        dTotalLen = dTotalLen - (double)nRecvLen;  //총길이에서  받은 길이 제거
	    	dTotalRecvLen = dTotalRecvLen + (double)nRecvLen; //받은 길이 만큼 더함

        
	    	
	    	
	        
	
	  	}
		infLOG(ALWAY, "MyDiskFileDataTran   ] File Send Body \n"); 		
			  	  

		
	   	if(DownloadFile)
	   		fclose(DownloadFile);		
	   	
	   	if(szRecvBuffer)
			delete[] szRecvBuffer;

	   	
		#ifdef __DEBUG
		printf("MyDiskFileDataTran   ] file close success\n");
		#endif
		///////////////////////////////////////////////
		//파일 이름 바꾸기
		// DB 넣기..
	
	
	
		#ifdef __DEBUG
		printf("MyDiskFileDataTran   ] contents number : (%s) ( %lu ) (%lu)\n",pFileinfo->cfups4003.file_name2, pFileinfo->nNumber,pFileinfo->cfups4003.id);
		#endif
			
		infLOG(ALWAY,"MyDiskFileDataTran   ] My 디스크 Temp 번호 (%s) (%lu) (%lu)\n",pFileinfo->cfups4003.file_name2, pFileinfo->nNumber,pFileinfo->cfups4003.id); 
	
	
	//여기서 부터 받기 시작
			
		memset(&headers,0x00,sizeof(HEADER));
		if(RecvData(Socket,(char*)&headers,HEADER_SIZE ) <= 0)
		{
	
			infLOG(ERROR, "MyDiskFileDataTran ER] FileTransfer Recv Head Error\n");			
			
			#ifdef __DEBUG
			printf("MyDiskFileDataTran ER] WaitForRequst : 응답 대기중 에러 1: <client 죽음>\n");
			#endif
			
			///////////////////////////////////////////////
			// temp 삭제									 //
			// 서버 용량 (upload_size) 업데이트 rollback  //
			// 파일 삭제 								 //
			///////////////////////////////////////////////
			
			memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
			
			
			
			pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
			pcom9104_r.id         = pFileinfo->cfups4003.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
			memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
			pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
			if(com9104(pcom9104_r) < 0)
			{
				infLOG(ERROR, "MyDiskFileDataTran ER]  내자료실 rollback 오류 (com9104)\n"); 	
			}						
		
		
			
			com9101 ( com9101_r);	

			// 받던 파일 삭제
			return 0;
		}
	
		if(headers.nCmd == RS_MYDISK_FILE_REQUEST_NEXT_FILE )
		{
			
			#ifdef __DEBUG
			printf("\nMyDiskFileDataTran   ] RS_MYDISK_FILE_REQUEST_NEXT_FILE\n");
			#endif
			memset(&headers,0x00,HEADER_SIZE);
			
			headers.nCmd = RS_MYDISK_FILE_REQUEST_NEXT_FILEINFO; //파일 정보 요청
			headers.nDataCnt = 0;
			headers.nDataSize = 0;
			
			if(	SendData(Socket,(char*)&headers,HEADER_SIZE )<0)  //struct _PACKET == PACKET
			{
				infLOG(ERROR, "MyDiskFileDataTran ER]  SendData \n"); 
				#ifdef __DEBUG
				printf("MyDiskFileDataTran ER]  SendData \n");
				#endif
		
				///////////////////////////////////////////////
				// temp 삭제									 //
				// 서버 용량 (upload_size) 업데이트 rollback  //
				// 파일 삭제 								 //
				///////////////////////////////////////////////
				
				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
				
				
				
				pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
				pcom9104_r.id         = pFileinfo->cfups4003.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
				pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
				if(com9104(pcom9104_r) < 0)
				{
					infLOG(ERROR, "MyDiskFileDataTran ER] 내자료실 rollback 오류 (com9104)\n"); 	
				}						
			
			
				
				com9101 ( com9101_r);	
	
				// 받던 파일 삭제
				
				return 0;
				
			}
			
			//recv file_transfer
			memset(&headers,0x00,HEADER_SIZE);
			if(RecvData(Socket,(char*)&headers,HEADER_SIZE ) <= 0)
			{
		
				infLOG(ERROR, "MyDiskFileDataTran ER] second FileTransfer Recv Head Error\n");			
				
				#ifdef __DEBUG
				printf("MyDiskFileDataTran ER]  WaitForRequst : 응답 대기중 에러 1: <client 죽음>\n");
				#endif
			
				///////////////////////////////////////////////
				// temp 삭제									 //
				// 서버 용량 (upload_size) 업데이트 rollback  //
				// 파일 삭제 								 //
				///////////////////////////////////////////////
				
				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
				
				
				
				pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
				pcom9104_r.id         = pFileinfo->cfups4003.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
				pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
				if(com9104(pcom9104_r) < 0)
				{
					infLOG(ERROR, "MyDiskFileDataTran ER] 내자료실 rollback 오류 (com9104)\n"); 	
				}						
			
			
				
				com9101 ( com9101_r);	
	
				// 받던 파일 삭제
				
				return 0;					
				
			}
		/*	 코드 추가 
			if( headers.nCmd == RS_EOL)
			{
					HEADER headers;
				memset(&headers,0x00,HEADER_SIZE);
				
				pSendData = new char[sizeof(HEADER)];
				memset(pSendData,0x00,sizeof(HEADER));
				
				headers.nCmd = RS_EOL;
				headers.nDataCnt = 0;
				headers.nDataSize = 0;
				
				memcpy(pSendData,&headers,sizeof(HEADER));
				
				return END;
			}
			*/
		
			memset(&rMFileinfo,0x00,sizeof(MFILEINFO));
			
			if( RecvData(Socket,(char*)&rMFileinfo,sizeof(MFILEINFO) ) <= 0)
			{
				infLOG(ERROR, "MyDiskFileDataTran ER]  RecvData \n"); 
				#ifdef __DEBUG
				printf("MyDiskFileDataTran ER] RecvData \n");
				#endif

	
				///////////////////////////////////////////////
				// temp 삭제									 //
				// 서버 용량 (upload_size) 업데이트 rollback  //
				// 파일 삭제 								 //
				///////////////////////////////////////////////
				
				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
				
				
				
				pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
				pcom9104_r.id         = pFileinfo->cfups4003.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
				pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
				if(com9104(pcom9104_r) < 0)
				{
					infLOG(ERROR, "MyDiskFileDataTran ER] 내자료실 rollback 오류 (com9104)\n"); 	
				}						
			
			
				
				com9101 ( com9101_r);	
	
				// 받던 파일 삭제
				
				return 0;		
			}
			
			pFileinfo = &rMFileinfo;
		
		}
		else if(headers.nCmd == RS_EOL)
		{
			
			if(pFileinfo->nTypeDisk == FT_MYDISK && pFileinfo->nNumber > 0 ) //컨텐츠 등록
			{
				#ifdef __DEBUG
				printf("MyDiskFileDataTran ER] 컨텐츠 등록\n");
				#endif
				if( dTotalLen == 0)
				{
					#ifdef __DEBUG
					printf("MyDiskFileDataTran ER] My 디스크 파일 등록 (%s)\n",pFileinfo->cfups4003.file_name2);
					#endif			
					infLOG(ALWAY,"MyDiskFileDataTran ER] My 디스크 파일 등록 (%s)\n",pFileinfo->cfups4003.file_name2); 
				
							
						
					if( fups4003(pFileinfo->cfups4003) == -1)//pFileinfo->cfups4001) == -1)
					{
						#ifdef __DEBUG
						printf("MyDiskFileDataTran ER] 내자료실 등록 오류\n");
						#endif
						infLOG(ERROR,"MyDiskFileDataTran ER]  내자료실 등록 오류 \n"); 	
										
						memset(&headers,0x00,sizeof(HEADER));
						
								
						///////////////////////////////////////////////
						// temp 삭제									 //
						// 서버 용량 (upload_size) 업데이트 rollback  //
						// 파일 삭제 								 //
						///////////////////////////////////////////////
						
						memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
						
						
						
						pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
						pcom9104_r.id         = pFileinfo->cfups4003.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
						memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
						pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
						if(com9104(pcom9104_r) < 0)
						{
							infLOG(ERROR, "MyDiskFileDataTran ER] 내자료실 rollback 오류 (com9104)\n"); 	
						}						
					
					
						
						com9101 ( com9101_r);	
						// 받던 파일 삭제
						
										
						infLOG(ERROR, "MyDiskFileDataTran ER] ================== 내 디스크 등록 오류 ===================\n"
									  "MyDiskFileDataTran ER] 서버 아디이( %s ) 파일경로 ( %s )                         \n"
									  "MyDiskFileDataTran ER] =========================================================\n" ,pFileinfo->cfups4003.server_id ,strFullPath); 
			
		//////////////////////////////////////////
						#ifdef __DEBUG
						printf("MyDiskFileDataTran ER] file recv cancel..\n");
						#endif
		
						headers.nCmd = RS_MYDISK_FILE_END_FAIL;	
						headers.nDataCnt = 0;
						headers.nDataSize = 0;
						headers.nErrorCode = 2003;
			
			
						if(	SendData(Socket,(char*)&headers,HEADER_SIZE)<0)  //struct _PACKET == PACKET
						{
							infLOG(ERROR, "MyDiskFileDataTran ER]  SendData (RS_MYDISK_FILE_END_FAIL)\n"); 
							#ifdef __DEBUG
							printf("MyDiskFileDataTran ER] SendData (RS_MYDISK_FILE_END_FAIL)\n");
							#endif

							com9101 ( com9101_r);
							return 0;
						}
						com9101 ( com9101_r);
						return 1;
						 
						
					}
				
					
				}
				else
				{
					
					memset(&headers,0x00,sizeof(HEADER));
		
					#ifdef __DEBUG
					printf("MyDiskFileDataTran ER] file recv cancel..2\n");
					#endif
					infLOG(ERROR, "MyDiskFileDataTran ER] file recv cancel (send [RS_FILE_END_FAIL])\n"); 
					
					headers.nCmd = RS_FILE_END_FAIL;	
					headers.nDataCnt = 0;
					headers.nDataSize = 0;
					headers.nErrorCode = 2003;
		
		
					///////////////////////////////////////////////
					// temp 삭제									 //
					// 서버 용량 (upload_size) 업데이트 rollback  //
					// 파일 삭제 								 //
					///////////////////////////////////////////////
					
					memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
					
					
					
					pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
					pcom9104_r.id         = pFileinfo->cfups4003.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
					memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
					pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
					if(com9104(pcom9104_r) < 0)
					{
						infLOG(ERROR, "MyDiskFileDataTran ER]  내자료실 rollback 오류 (com9104)\n"); 	
					}						
				
				
					
					com9101 ( com9101_r);	
		
					// 받던 파일 삭제
					
					
					if(	SendData(Socket,(char*)&headers,HEADER_SIZE)<0)  //struct _PACKET == PACKET
					{
						infLOG(ERROR, "MyDiskFileDataTran ER]  send \n"); 
						#ifdef __DEBUG
						printf("MyDiskFileDataTran ER] send \n");
						#endif
						com9101 ( com9101_r);
						return 0;
					}
					com9101 ( com9101_r);
					return 1;
				}
				
			}
		
			#ifdef __DEBUG
			printf("MyDiskFileDataTran   ] recv sucessed...  \n");
			#endif
		
			infLOG(ALWAY, "MyDiskFileDataTran   ] Ouput ) FileDataTransfer\n");
				
				
			memset(&headers,0x00,HEADER_SIZE);
			
			
			headers.nCmd = RS_EOL; //파일 정보 요청
			headers.nDataCnt = 0;
			headers.nDataSize = 0;
			
			if(	SendData(Socket,(char*)&headers,HEADER_SIZE )<0)  //struct _PACKET == PACKET
			{
				infLOG(ERROR, "MyDiskFileDataTran ER]  SendData \n"); 
				com9101 ( com9101_r);
				return 0;
			}
			com9101 ( com9101_r);
			return 0;
			
		}
		else
		{
			infLOG(ERROR, "MyDiskFileDataTran ER]  FileTransfer Recv  \n"); 
			#ifdef __DEBUG
			printf("MyDiskFileDataTran ER]  FileTransfer Recv  \n");
			#endif
			
			

			///////////////////////////////////////////////
			// temp 삭제									 //
			// 서버 용량 (upload_size) 업데이트 rollback  //
			// 파일 삭제 								 //
			///////////////////////////////////////////////
			
			memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
			
			
			
			pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
			pcom9104_r.id         = pFileinfo->cfups4003.id;        // 컨테츠ID(T_CONTENTS_TEMP.id)
			memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // 사용자
			pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
			if(com9104(pcom9104_r) < 0)
			{
				infLOG(ERROR, "MyDiskFileDataTran ER]  내자료실 rollback 오류 (com9104)\n"); 	
			}						
		
		
			
			com9101 ( com9101_r);	

			// 받던 파일 삭제
			return 0;
		}
		
				
	}while( 1 );
	
	com9101 ( com9101_r);
	return 0;	
	
}

