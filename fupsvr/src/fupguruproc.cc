#include "fupsock.h"

#include "fupdefine.h"
#include "fupguruproc.h"
#include "fupcomlib.h"

#include "apstruct.h" // HEADER
#include "comcomm.h"

#include "apdefine.h"
#include "comhead.h"

#include "com9104.h"

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

int RequestGuruFilUp(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	
	///// 파일 정보 구조체 생성 ////
	
	struct stat64 statbuf;
	LPHEADER pHeader = (LPHEADER)pRecvHead; //head
	
	ERR_HEADER errheader; //err head
	memset(&errheader,0x00,sizeof(ERR_HEADER));
	
	HEADER headers; //temp head
	memset(&headers,0x00,sizeof(HEADER));
	
	LPGURUFILEINFO pFileinfo = (LPGURUFILEINFO)pRecvData; //body
//					01234567890123456789	
	infLOG(ALWAY,  "RequestGuruFilUp	] RequestGuruFilUp (%s) \n",pHeader->szUserID);	

	#ifdef __DEBUG
//			01234567890123456789	
	printf("RequestGuruFilUp	] loading RequestGuruFilUp... (%s)\n",pHeader->szUserID);
	#endif
	
	

//아이디 저장	save User ID
	multimap<int,USERINFO>::iterator mi;
	
	mi = m_UserList.find(Socket);
	if(mi != m_UserList.end())
	{
		memcpy(mi->second.szUserID,pHeader->szUserID,min(12,(int)sizeof(pHeader->szUserID)));
	}

	
	#ifdef __DEBUG
//					01234567890123456789		
	printf("recv Body\n");
		
	printf("RequestGuruFilUp	] ID        ( %d      )\n" ,pFileinfo->fups4002.id         );
	printf("RequestGuruFilUp	] SIZE      ( %15.0f  )\n" ,pFileinfo->fups4002.file_size  );
	printf("RequestGuruFilUp	] FOLDER    ( %s      )\n" ,pFileinfo->fups4002.folder_yn  );
	printf("RequestGuruFilUp	] SID       ( %s      )\n" ,pFileinfo->fups4002.server_id  );
	printf("RequestGuruFilUp	] SPATH     ( %s      )\n"  ,pFileinfo->fups4002.sfile_path);
	printf("RequestGuruFilUp	] SNAME     ( %s      )\n" ,pFileinfo->fups4002.sfile_name );
	printf("RequestGuruFilUp	] LNAME     ( %s      )\n" ,pFileinfo->fups4002.lfile_name );
	printf("RequestGuruFilUp	] TYPE      ( %s      )\n" ,pFileinfo->fups4002.file_type  );
	printf("RequestGuruFilUp	] TEMP      ( %s      )\n" ,pFileinfo->fups4002.temp		 );
	printf("RequestGuruFilUp	] local 	( %s      )\n" ,pFileinfo->local_file_name	 ); 
	printf("RequestGuruFilUp	] local 	( %s      )\n" ,pFileinfo->local_file_path	 );
	printf("RequestGuruFilUp	] server 	( %s      )\n" ,pFileinfo->server_file_name	 ); 
	printf("RequestGuruFilUp	] server	( %s      )\n" ,pFileinfo->server_file_path	 );
	printf("RequestGuruFilUp	] sizeof    ( %d      )\n" ,sizeof(GURUFILEINFO));
	printf("	\n");
	#endif


	char strFullPath[255+1];
	memset(strFullPath,0x00,sizeof(strFullPath));
	char szFullName[255+1];
	memset(szFullName,0x00,sizeof(szFullName));
	char szSrcFilePath[255+1];
	memset(szSrcFilePath,0x00,sizeof(szSrcFilePath));
	
//////////////////////////////////////////////////////////////////////	
	
	CCOM9104_R pcom9104_r;

	double dTotalRecvLen = 0; //총 받은 길이
	double dTotalLen = 0; // down될 파일의 총 길이
	int nWriteLen=0;
	int nRecvLen=0;
	int count=0;
	bool bCreateFile=false;
	
//////////////////////////////////////////////////////////////////////	
		
	#ifdef __DEBUG
	printf("RequestGuruFilUp	] the file is GURU\n");
	#endif
	
	headers.nCmd  = RS_FILE_DATA_SIGN_CHECK; // 파일 전송 메세지    	
	srand((unsigned int)time(NULL))	; //random 이름을 위함 시드 지정
	bCreateFile = false;// 같은 이름이 생성되었는지 check
	int nCheckLoop = 0;
	while(bCreateFile != true) // 같은 이름이 있으면 roof안으로..
    {
    	nCheckLoop++;
		if( nCheckLoop > 50 ) //폴더를 생성하지 못하였을때.
		{
			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
			strcat(errheader.errmsg,"서버 에서 파일 만들기 실패 하였습니다.");
			
			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));
		
			return -RS_FILE_DATA_TRANSFER;			
		}
		
		///// 날짜 시간 생성 ////
		time_t			curtime;
		struct tm		*stm;
		time( &curtime );
		stm = (struct tm *) localtime(&curtime);
	
		localtime_r(&curtime, stm);
		
		
		//if(pFileinfo->nType == FT_FOLDER) //전송 받을 파일이 폴더 일경우
		if( strcmp(pFileinfo->fups4002.folder_yn,"Y") == 0 
		  ||strcmp(pFileinfo->fups4002.folder_yn,"y") == 0)
		{
			sprintf(szFullName,"%s/%s",pFileinfo->server_file_path,pFileinfo->server_file_name);
			strcpy(szSrcFilePath,pFileinfo->server_file_path);
			
			#ifdef __DEBUG
			printf("RequestGuruFilUp	] Folder path (%s)\n",szFullName);
			#endif
			
			
		}
		else //파일 일경우
		{
			memset(szSrcFilePath,0x00,sizeof(szSrcFilePath));
			//동적으로 /raid/fdata/freedata 얻기

			// 박병훈 20110616
			// 개발서버 폴더 제대로 바라보도록 수정

			#ifdef __DEBUG
			sprintf(szSrcFilePath,"/raid/fdata/freedata1/%04d/%02d/%02d/%02d"
													,  stm->tm_year+1900
													,  stm->tm_mon + 1
													,  stm->tm_mday
													,  stm->tm_hour);//./2004/02/18/16

			#else
                        sprintf(szSrcFilePath,"/raid/fdata/freedata/%04d/%02d/%02d/%02d"
                                                                                                        ,  stm->tm_year+1900
                                                                                                        ,  stm->tm_mon + 1
                                                                                                        ,  stm->tm_mday
                                                                                                        ,  stm->tm_hour);//./2004/02/18/16
			#endif
			
			#ifdef __DEBUG
			
			printf("RequestGuruFilUp	]  file path : %s",szSrcFilePath);										
			#endif
			memset(szFullName,0x00,sizeof(szFullName));					
	    	
	    	strcpy(szFullName,szSrcFilePath);					
			strcat(szFullName,"/"); //./2004/02/18/16/raid/   <-- '/' 추가
	    	//file name 얻기
	    	char szFilename[5];
	    	char szFileType[10];
	    	memset(szFilename,0x00,sizeof(szFilename));
	    	memset(szFileType,0x00,sizeof(szFileType));
	    	
	    	int nFileName = random()%20000; 	
			sprintf(szFilename,"%d",nFileName);		
	    	//local 파일이름으로 부터 확장자 얻기.
	    	//int nLen = GetReverseIndex(pFileinfo->szFileName , '.');
	    	int nLen = GetReverseIndex(pFileinfo->fups4002.lfile_name , '.');
			//nLen = nLen - 1; // ./raid/ -> ,./raid   , '/' delete
			if(nLen < 0)
			{
				#ifdef __DEBUG
				printf("RequestGuruFilUp	]  Error client file name  %s\n",pFileinfo->fups4002.lfile_name);		
				#endif
					//			01234567890123456789				
				infLOG(ERROR,  "RequestGuruFilUp Err] Make File Name Error (Not Have exp) (%s) (%d)\n",pFileinfo->fups4002.lfile_name,nLen);				
//infLOG(ERROR, "FileDataTransfer : File 이름 에러 (확장자 없음): < 통 과 >\n"); 
				
			}
			else
				GetRightString(pFileinfo->fups4002.lfile_name,strlen(pFileinfo->fups4002.lfile_name)-nLen,szFileType);
				//GetRightString(pFileinfo->fups4002.lfile_name,nLen,szFileType);
				
			
			
			
			
			#ifdef __DEBUG
			printf("RequestGuruFilUp	] client file name  %s\n",pFileinfo->fups4002.lfile_name);		
			#endif
			
			memset(pFileinfo->fups4002.sfile_name,0x00,sizeof(pFileinfo->fups4002.sfile_name));
			
			strcpy(pFileinfo->fups4002.sfile_path,szSrcFilePath);//,sizeof(pFileinfo->szDownPath));
			strcpy(pFileinfo->fups4002.sfile_name,szFilename);
			strcat(pFileinfo->fups4002.sfile_name,szFileType);
			
			
			#ifdef __DEBUG
			printf("RequestGuruFilUp	] server file name  %s\n",pFileinfo->fups4002.sfile_name);		
			#endif
			
			strcat(szFullName,szFilename);
			strcat(szFullName,szFileType);
							
		}	
		
			
		if( bCreateFile == false)
		{
	    		
			int stat = stat64(szFullName,&statbuf);
			if(stat != 0) //파일이 없으면 폴더 만들기.
			{
				//if(errno == ENOENT) //파일이나 패스가 없음
				{		
					/*
					if(MakeFolder(pFileinfo->szDownPath) == -1) // 폴더를 만들었으면 ..폴더 자동 생성.
					{
						// 폴더 만들기 실패
						pSendData = new char[sizeof(ERR_HEADER)];
						memset(pSendData,0x00,sizeof(ERR_HEADER));
						errheader.header.nCmd = RS_ERR;
						errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
						strcat(errheader.errmsg,"서버 에서 파일 만들기 실패 하였습니다.");
						
						memcpy(pSendData, &errheader, sizeof(ERR_HEADER));
			
					
						return -RS_FILE_DATA_TRANSFER;
						
					}*/
					
					MakeFolder(szSrcFilePath);					
				}	
				bCreateFile = true; //파일 이름이 결정 되었으면...true			
				
			}
			else // 같은 이름이 아닐때 까지 ...
			{
				bCreateFile = false;		
			}
		}
	}
	headers.nCmd = RS_FILE_DATA_SIGN_CHECK; //파일 전송



	
	////////////////////기본 정보 완료////////////////////////////////////////////////
	
	
	
	
	
	// 파일 제어 및 전송
	FILE* DownloadFile; //파일 포인터
	DownloadFile = NULL;
	//// 파일 open형식 결정////
	#ifdef __DEBUG
	printf("RequestGuruFilUp	] open file : %s\n",szFullName);
	#endif
	
		
	
/*	if(headers.nCmd == RS_FILE_REQUEST_CONTINUE) //append mode
	{
		#ifdef __DEBUG
		printf("  ./open append mode\n");
		#endif
		DownloadFile = fopen64(szFullName,"a+tb");	
	}
	else
	{
		*/
		#ifdef __DEBUG
		printf("RequestGuruFilUp	] open write mode\n");	
		#endif
		DownloadFile = fopen64(szFullName,"w+tb");
//	}
		infLOG(ERROR, "RequestGuruFilUp	] UploaddFile ( %s )\n",szFullName);	

	if(DownloadFile == NULL) //파일을 열수 없으면
	{
		#ifdef __DEBUG
		printf("RequestGuruFilUp ERR] file error : file is null\n");
		#endif
		infLOG(ERROR, "RequestGuruFilUp ERR] DownloadFile == NULL\n");		
		
		#ifdef __DEBUG
		printf("RequestGuruFilUp ERR] DownloadFile == %s : <메세지 전달>\n",szFullName);
		#endif
		pSendData = new char[sizeof(ERR_HEADER)];
		memset(pSendData,0x00,sizeof(ERR_HEADER));
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
		strcat(errheader.errmsg,"서버 에서 파일 만들기 실패 하였습니다.");
		
		memcpy(pSendData, &errheader, sizeof(ERR_HEADER));
		
		if( strcmp(pFileinfo->fups4002.folder_yn,"N") == 0
			  ||strcmp(pFileinfo->fups4002.folder_yn,"n") == 0)
		{
			
			if(DeleteFile(FT_FILE,szFullName) == -1)
			{
				
			}
			#ifdef __DEBUG
			printf("RequestGuruFilUp ERR] file delete ok..\n");
			#endif
		}
		else if( strcmp(pFileinfo->fups4002.folder_yn,"Y") == 0
			   ||strcmp(pFileinfo->fups4002.folder_yn,"y") == 0)
		{
			
			
			if(DeleteFile(FT_FOLDER,strFullPath) == -1)	
			{
				
			}
			#ifdef __DEBUG
			printf("RequestGuruFilUp ERR] folder delete ok..\n");
			#endif
		}	
	
		return -RS_FILE_DATA_TRANSFER;
	}
	


    #ifdef __DEBUG
    printf("RequestGuruFilUp	] make folder complete ...\n");
    #endif
    headers.nDataCnt = 0;
	headers.nDataSize = 0;
	headers.nErrorCode = 0;
   
	infLOG(ALWAY, "RequestGuruFilUp	] Send RS_FILE_DATA_SIGN_CHECK \n");  
    //// 전송하기전에 메세지를 알림...
    if(	SendData(Socket,(char*)&headers,sizeof(struct _HEADER))<0)  //struct _PACKET == PACKET
	{

		#ifdef __DEBUG
		printf("RequestGuruFilUp ERR] send 에러 1 : <client 죽음>\n");
		#endif
		
		infLOG(ERROR, "RequestGuruFilUp ERR] Send RS_FILE_DATA_SIGN_CHECK error\n");

		if( strcmp(pFileinfo->fups4002.folder_yn,"N") == 0
			  ||strcmp(pFileinfo->fups4002.folder_yn,"n") == 0)
		{
			
			if(DeleteFile(FT_FILE,szFullName) == -1)
			{
				
			}
		}
		else if( strcmp(pFileinfo->fups4002.folder_yn,"Y") == 0
			   ||strcmp(pFileinfo->fups4002.folder_yn,"y") == 0)
		{
			
			
			if(DeleteFile(FT_FOLDER,strFullPath) == -1)	
			{
				
			}
		}
		return 0;
	}

	
	
//	HEADER headers;
	
	memset(&headers,0x00,sizeof(HEADER));

	infLOG(ALWAY, "RequestGuruFilUp    ] Recv Header \n");
	
	if(	RecvData(Socket,(char*)&headers,sizeof(struct _HEADER))<0)  //struct _PACKET == PACKET
	{
		infLOG(ERROR, "RequestGuruFilUp ERR] Recv Header Error\n");
	
		#ifdef __DEBUG
		printf("RequestGuruFilUp ERR] recv 에러 1 : <client 죽음>\n");
		#endif
		
		
		if( strcmp(pFileinfo->fups4002.folder_yn,"N") == 0
			  ||strcmp(pFileinfo->fups4002.folder_yn,"n") == 0)
		{
			
			if(DeleteFile(FT_FILE,szFullName) == -1)
			{
				
			}

		}
		else if( strcmp(pFileinfo->fups4002.folder_yn,"Y") == 0
			   ||strcmp(pFileinfo->fups4002.folder_yn,"y") == 0)
		{
			
			
			if(DeleteFile(FT_FOLDER,strFullPath) == -1)	
			{
				
			}

		}
			
		
		return 0;
	}

	infLOG(ALWAY, "RequestGuruFilUp    ] Check Header\n");	
	if(headers.nCmd == RS_EOL)
	{
		infLOG(ALWAY, "RequestGuruFilUp    ] RS_EOL\n");		
		#ifdef __DEBUG
		printf("RequestGuruFilUp    ] file recv cancel..\n");
		#endif
		
		pSendData = new char[sizeof(HEADER)];
		memset(pSendData,0x00,sizeof(HEADER));
		
		headers.nCmd = RS_EOL;	
		headers.nDataCnt = 0;
		headers.nDataSize = 0;
		headers.nErrorCode = 0;
		
		memcpy(pSendData, &headers, sizeof(HEADER));		
		
		if( strcmp(pFileinfo->fups4002.folder_yn,"Y") == 0
		  ||strcmp(pFileinfo->fups4002.folder_yn,"y") == 0)
		{
			
			
			if(DeleteFile(FT_FOLDER,strFullPath) == -1)	
			{
				
			}
		}
		infLOG(ALWAY, "RequestGuruFilUp    ] return END\n");		
		#ifdef __DEBUG
		printf("RequestGuruFilUp    ] return END\n");
		#endif
		return END;
	}
	if(headers.nCmd == RS_OK)
	{
		infLOG(ALWAY, "RequestGuruFilUp    ] RS_OK\n");

		#ifdef __DEBUG
		printf("RequestGuruFilUp    ] RS_OK\n");
		#endif
	}
	
 ///////////////////////// 데이터 전송 //////////////////////////////////
	dTotalRecvLen = 0; //총 받은 길이
	//double dTotalLen = pFileinfo->dFileSize; // down될 파일의 총 길이
	dTotalLen = pFileinfo->fups4002.file_size; // down될 파일의 총 길이
	nWriteLen=0;
	nRecvLen=0;
	count=0;
	char* szRecvBuffer = new char[RECVBUF]; //recv buffer




	#ifdef __DEBUG
	printf("RequestGuruFilUp    ] Checking File (%s) : 파일 전체 길이 (%15.0f ) =  %15.0f \n",pFileinfo->local_file_name , dTotalLen ,pFileinfo->fups4002.file_size );
	#endif
			
	infLOG(ALWAY,"RequestGuruFilUp    ] Checking File (%s) : 파일 전체 길이 (%15.0f ) = %15.0f \n",pFileinfo->local_file_name , dTotalLen ,pFileinfo->fups4002.file_size );
	
	int fno = fileno(DownloadFile);	
	
	while(dTotalLen > 0  ) 
	{   	
		memset(szRecvBuffer,0x00,RECVBUF);	

		///// 파일받기 ///// 통합 - 무료 자료실은 통합이 아니다.
        nRecvLen =  RecvFileData(Socket, szRecvBuffer, RECVBUF, dTotalLen);
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
    			printf("RequestGuruFilUp    ] file의 끝\n");
    			#endif
    			infLOG(ALWAY,"RequestGuruFilUp    ] Write File End (%s) : 파일 전체 길이 (%15.0f ) = %15.0f \n",pFileinfo->local_file_name , dTotalLen ,pFileinfo->fups4002.file_size );
    			
    		}
    		else	
    		{
    			infLOG(ERROR,"RequestGuruFilUp ERR] Write File ERROR (%s) : 파일 전체 길이 (%15.0f ) = %15.0f \n",pFileinfo->local_file_name , dTotalLen ,pFileinfo->fups4002.file_size );
    			nRecvLen = -1;
    		}
    	}
    	
        if(nRecvLen <= 0 && dTotalLen != 0)	//받다가 오류가 났을시...DB처리
        {
			if(nRecvLen < 0)
        	{
        		char szErrMsg[1024];
				memset(szErrMsg,0x00,sizeof(szErrMsg));
				GetErrMsg(-nRecvLen,szErrMsg);
				infLOG(ERROR, "RequestGuruFilUp ERR] RecvSocket Error ( %d )( %s )\n",nRecvLen,szErrMsg); 
				
				#ifdef __DEBUG
				printf("RequestGuruFilUp ERR] RecvSocket Error ( %d )( %s )\n",nRecvLen,szErrMsg); 
				#endif
				

        	}
        	else if(nRecvLen == 0)
        	{
        		char szErrMsg[1024];
				memset(szErrMsg,0x00,sizeof(szErrMsg));
				GetErrMsg(-nRecvLen,szErrMsg);
					        		
        		infLOG(ERROR, "RequestGuruFilUp ERR] RecvSocket Error ( 접속을 끊었습니다. ) (%s)\n",szErrMsg); 
				#ifdef __DEBUG
				printf("RequestGuruFilUp ERR] RecvSocket Error ( 접속을 끊었습니다. ) (%s)\n",szErrMsg); 
				#endif
        	}       	
			infLOG(ERROR, "RequestGuruFilUp ERR] Recv Body Error\n");        	
			#ifdef __DEBUG
			printf("RequestGuruFilUp ERR] file recv exception \n");
			#endif
						
			infLOG(ERROR,"RequestGuruFilUp ERR] 공유 광장 취소 (%s)\n",szFullName); 

			if(DownloadFile)
				fclose(DownloadFile);

			//// 받던 파일 삭제///
			//if(pFileinfo->nType == FT_FILE)
			if( strcmp(pFileinfo->fups4002.folder_yn,"N") == 0
			  ||strcmp(pFileinfo->fups4002.folder_yn,"n") == 0)
			{
				
				
				if(DeleteFile(FT_FILE,szFullName) == -1)
				{
					
				}
			
			}
			else if( strcmp(pFileinfo->fups4002.folder_yn,"Y") == 0
			       ||strcmp(pFileinfo->fups4002.folder_yn,"y") == 0)
			{
				
				
				if(DeleteFile(FT_FOLDER,strFullPath) == -1)	
				{
					
				}

			}
			if(szRecvBuffer)
				delete[] szRecvBuffer;
			
			return 0;
		}
        	      
        dTotalLen = dTotalLen - (double)nRecvLen;  //총길이에서  받은 길이 제거
    	dTotalRecvLen = dTotalRecvLen + (double)nRecvLen; //받은 길이 만큼 더함
 
    	
    	count++;
        
    }
    
	infLOG(ALWAY, "RequestGuruFilUp    ] Recv Body\n");

    
    if(DownloadFile)
    	fclose(DownloadFile);
    if(szRecvBuffer)
		delete[] szRecvBuffer;
	

	
			
			
	#ifdef __DEBUG
	printf("RequestGuruFilUp    ] file close success\n");
	#endif
		
		
	///////////////////////////////////////////////
	//파일 이름 바꾸기
	// DB 넣기..

	long nResult;
	char pErrMsg[256];
	memset(pErrMsg,0x00,sizeof(pErrMsg));
	
	if( pFileinfo->nFlag == F_END)
	{

		#ifdef __DEBUG
		printf("RequestGuruFilUp    ] insert fups4002\n");
		#endif
		
		infLOG(ALWAY, "RequestGuruFilUp    ] fups4002\n");				
		nResult = fups4002(pFileinfo->fups4002,pErrMsg);
		infLOG(ALWAY, "RequestGuruFilUp    ] fups4002 (%d)\n",nResult);		
		if(nResult < 0)
		{
			infLOG(ERROR, "RequestGuruFilUp ERR]fups4002 Error (%d)\n",nResult);			
			#ifdef __DEBUG
			printf(" RequestGuruFilUp ERR] fups4002 error \n");
			#endif
			
			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = nResult;
			
			#ifdef __DEBUG
			printf("RequestGuruFilUp ERR] pErrMsg (%s)\n",pErrMsg);
			#endif
			strcpy(errheader.errmsg,pErrMsg);
			
			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));
		
			//// 받던 파일 삭제///
			//if(pFileinfo->nType == FT_FILE)
			
			infLOG(ERROR,"RequestGuruFilUp ERR] 공유 광장 취소 (%s)\n",szFullName); 
			
			
			if( strcmp(pFileinfo->fups4002.folder_yn,"N") == 0
			  ||strcmp(pFileinfo->fups4002.folder_yn,"n") == 0)
			{
				
				if(DeleteFile(FT_FILE,szFullName) == -1)
				{
					
				}

			}
			else if( strcmp(pFileinfo->fups4002.folder_yn,"Y") == 0
				   ||strcmp(pFileinfo->fups4002.folder_yn,"y") == 0)
			{
				
				
				if(DeleteFile(FT_FOLDER,strFullPath) == -1)	
				{
					
				}

			}

			//return END;
			return -RS_GURU_FILE_UP;
		}
		
		headers.nCmd = RS_EOL;
		headers.nDataCnt = 0;
		headers.nDataSize = 0;
		headers.nErrorCode = 0;
	    
	    
		infLOG(ERROR, "RequestGuruFilUp    ] Send RS_EOL\n");	    
	    //// 전송하기전에 메세지를 알림...
	    if(	SendData(Socket,(char*)&headers,sizeof(struct _HEADER))<0)  //struct _PACKET == PACKET
		{
			infLOG(ERROR, "RequestGuruFilUp ERR] Send RS_EOL Error\n");			

			#ifdef __DEBUG
			printf("RequestGuruFilUp ERR] send 에러 1 : <client 죽음>\n");
			#endif
			return 0;
		}
			
		return 0;
		//return END;
	}
		

	
	#ifdef __DEBUG
	printf("RequestGuruFilUp    ] recv sucessed...  (%d)(%s) \n",sizeof(GURUFILEINFO),pHeader->szUserID);
	#endif		

	infLOG(ALWAY, "RequestGuruFilUp    ] RequestGuruFilUp (%s)\n",pHeader->szUserID);		
	
	return 1;
	
}



//파일 목록( 무료 자료실 에서 폴더 일때 필요 )
int GuruFileRequestList(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	infLOG(ALWAY, "RequestGuruFilUp    ] GuruFileRequestList\n");
	#ifdef __DEBUG
	printf("RequestGuruFilUp    ] loading GURU UP file request List ...\n");
	#endif
	///// 파일 정보 구조체 생성 ////
	
	
	LPHEADER pHeader = (LPHEADER)pRecvHead; //head
	
	ERR_HEADER errheader; //err head
	memset(&errheader,0x00,sizeof(ERR_HEADER));
	
	HEADER headers; //temp head
	memset(&headers,0x00,sizeof(HEADER));

	char szFullName[768];	
	memset(szFullName,0x00,sizeof(szFullName));

	char szCheckName[768];	
	memset(szCheckName,0x00,sizeof(szCheckName));
	
	char szFolderName[10];
	memset(szFolderName,0x00,sizeof(szFolderName));
	
	long nFolderName = 0;
	
	bool bResult = false;
	struct stat64 statbuf;
	
	while(bResult == false)
	{
		time_t	curtime;
		struct tm		*stm;
		time( &curtime );
		stm = (struct tm *) localtime(&curtime);
	
		localtime_r(&curtime, stm);
		
		//서버 패스 가져오는 부분 동적으로 변경;
                
		// 박병훈 20110616
		// 개발서버 폴더 제대로 바라보도록 수정	
	
		#ifdef __DEBUG
		sprintf(szFullName,"/raid/fdata/freedata1/%04d/%02d/%02d/%02d"
													,  stm->tm_year+1900
													,  stm->tm_mon + 1
													,  stm->tm_mday
													,  stm->tm_hour);//./2004/02/18/16
		
		#else
                sprintf(szFullName,"/raid/fdata/freedata/%04d/%02d/%02d/%02d"
                                                                                                        ,  stm->tm_year+1900
                                                                                                        ,  stm->tm_mon + 1
                                                                                                        ,  stm->tm_mday
                                                                                                        ,  stm->tm_hour);//./2004/02/18/16		

		#endif
	
		//folder 이름 얻기
		
		#ifdef __DEBUG
		printf("RequestGuruFilUp    ] fullname : %s\n",szFullName);
		#endif
		
		nFolderName = random()%1000000; 	
		
		strcpy(szCheckName,szFullName);
		strcat(szCheckName,"/");
		sprintf(szFolderName,"%ld",nFolderName);
		strcat(szCheckName,szFolderName);

		
		
			
		int stat = lstat64(szCheckName,&statbuf);
		if(stat != 0) //폴더가 없으면 폴더 만들기.
		{
			
			
			if(MakeFolder(szCheckName)== -1)
			{
			
			}
			bResult = true; //파일 이름이 결정 되었으면...true							
		}
		else // 같은 이름이 아닐때 까지 ...
			bResult = false;		
	
	}
	

		
			
	headers.nCmd = RS_GURUFILE_REQUEST_LIST;

	headers.nDataCnt =1;
	headers.nDataSize = sizeof(GURUFOLDERPATH);
	headers.nErrorCode = 0;
	
	GURUFOLDERPATH pGuruPath;
	memset(&pGuruPath,0x00,sizeof(GURUFOLDERPATH));

	#ifdef __DEBUG
	printf("RequestGuruFilUp    ] send RS_GURUFILE_REQUEST_LIST_OK \n");
	printf("RequestGuruFilUp    ] szFolderName = (%s)\n",szFolderName);
	#endif
	
	memcpy(pGuruPath.szFolderName,szFolderName,sizeof(szFolderName));
	memcpy(pGuruPath.szFolderPath,szFullName,sizeof(szFullName)); //서버측 패스
	
	
	#ifdef __DEBUG
	printf("RequestGuruFilUp    ] 서버측 패스 : %s    /  %s\n",pGuruPath.szFolderPath,szFullName);
	printf("RequestGuruFilUp    ] 서버측 이름 : %s    /  %s\n",pGuruPath.szFolderName,szFolderName);
	#endif
	
	
	
	
	pSendData = new char[sizeof(HEADER) + headers.nDataCnt * headers.nDataSize];
	memset(pSendData,0x00,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize);
	
	memcpy(pSendData,&headers,sizeof(HEADER)); //head
	memcpy(pSendData+sizeof(HEADER),&pGuruPath,  headers.nDataCnt * headers.nDataSize);
	
	
	#ifdef __DEBUG
	printf("RequestGuruFilUp    ]/ free reqeust list succeed \n");
	#endif

	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));	
}



