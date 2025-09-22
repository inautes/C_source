/*----------------------------------------------------------------------------
   �޴� ���� ���� 
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
#include "fupmain.h"

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
	
	headers.nCmd = RS_MYDISK_FILE_REQUEST_NEXT_FILEINFO;//RS_MYDISK_FILE_REQUEST_FILE_FILINFO; //���� ���� ��û
	headers.nDataCnt = 0;
	headers.nDataSize = 0;
	
	memcpy(pSendData,&headers,sizeof(HEADER));
	
	
	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));//return 1;
}

//11�� 15�� ���� ���� 

int MyDiskFileRequestFile(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{

	HEADER headers;
	memset(&headers,0x00,HEADER_SIZE);
	
	pSendData = new char[sizeof(HEADER)];
	memset(pSendData,0x00,sizeof(HEADER));
	
	headers.nCmd = RS_MYDISK_FILE_REQUEST_FILE_FILINFO;//RS_MYDISK_FILE_REQUEST_FILE_FILINFO; //���� ���� ��û
	headers.nDataCnt = 0;
	headers.nDataSize = 0;
	
	memcpy(pSendData,&headers,sizeof(HEADER));

	
	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));//return 1;
}



int MyDiskFileRequestList(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	

	///// ���� ���� ����ü ���� ////
	
	
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
		printf("FileRequestList      ] ���� �˻�	 %s\n",szCheckName);
		#endif		
		
			
		int stat = lstat64(szCheckName,&statbuf);
		if(stat != 0) //������ ������ ���� �����.
		{
			#ifdef __DEBUG
//					01234567890123456789 ]
			printf("FileRequestList      ] ���� ����   %s\n",szCheckName);
			#endif		
			
			if(MakeFolder(szCheckName)== -1)
			{
				#ifdef __DEBUG
//						01234567890123456789 ]
				printf("FileRequestList      ] ���� ���� ���� %s\n",szCheckName);
				#endif		
			}
		}
		else
		{
		    infLOG(ERROR, "FileDataTransfer  ERR]  === ���� ���� ���� === \n"); 	
		}
		
/*	
	while(bResult == false)
	{
		nCheckLoop++;
		if( nCheckLoop > 50 ) //������ �������� ���Ͽ�����.
		{
			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
			strcat(errheader.errmsg,"���� ���� ���� ����⸦ ���� �Ͽ����ϴ�.");
			
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
	
		
				
	
		//folder �̸� ���
		
		
		
		srand((unsigned int)time(NULL))	; //random �̸��� ���� �õ� ����
		
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
		if(stat != 0) //������ ������ ���� �����.
		{
			if(MakeFolder(szCheckName)== -1)
			{

			}
			bResult = true; //���� �̸��� ���� �Ǿ�����...true							
		}
		else // ���� �̸��� �ƴҶ� ���� ...
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
	
	memcpy(FolderInfo.cfups4003.file_path,szFullName,sizeof(szFullName)); //������ �н�
	#ifdef __DEBUG
	printf("MyDiskFileRequestList] ������ �н� : %s    /  %s\n",pFileinfo->szDownPath,szFullName);
	#endif

	memcpy(FolderInfo.cfups4003.file_name2,pFileinfo->szFolderName,sizeof(pFileinfo->szFolderName)); //�������� �̸�
	
	#ifdef __DEBUG
	printf("MyDiskFileRequestList] ������ ���� �̸� : %s\n",pFileinfo->szFolderName);
	#endif

	
	memcpy(FolderInfo.cfups4003.file_name1,szFolderName,sizeof(szFolderName));			 //��������

	#ifdef __DEBUG
	printf("MyDiskFileRequestList] ������ ���� �̸� : %s\n",szFolderName);
	#endif

			
	
	
	pSendData = new char[sizeof(HEADER) + headers.nDataCnt * headers.nDataSize];
	memset(pSendData,0x00,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize);
	
	memcpy(pSendData,&headers,sizeof(HEADER)); //head
	memcpy(pSendData+sizeof(HEADER),&FolderInfo,  headers.nDataCnt * headers.nDataSize);
	
	
	
	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));

	
}


//�ʷα� & ���ڷ��
int MyDiskFileDataTransfer(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	
	infLOG(ALWAY, "MyDiskFileDataTran   ] FileDataTransfer\n");
	#ifdef __DEBUG
	printf("MyDiskFileDataTran   ] loading file data transfer...\n");
	#endif
	///// ���� ���� ����ü ���� ////
	

	bool bFOpenAppendMode = false;	        // ���� ���� ��� 
	int stat = -1;                          // ���� ����
	struct stat64 statbuf;                  
	LPHEADER pHeader = (LPHEADER)pRecvHead; //head
	
	ERR_HEADER errheader; //err head
	memset(&errheader,0x00,sizeof(ERR_HEADER));
	
	HEADER headers; //temp head
	memset(&headers,0x00,sizeof(HEADER));
	
	LPMFILEINFO pFileinfo = (LPMFILEINFO)pRecvData; //body
	
	//9001 ȣ�� // ����ڼ� ����
	//9101 ȣ�� //����ڼ� ����			
				
	CCOM9001_R com9001_r ;  
	memset(&com9001_r,0x00,sizeof(CCOM9001_R));

	multimap<int,USERINFO>::iterator mi; //IP ��ȸ
	
	mi = m_UserList.find(Socket);
	if(mi != m_UserList.end())
	{
		strcpy(com9001_r.conn_ip ,mi->second.thread.userIP);
	}
	
	strcpy(com9001_r.server_id , pFileinfo->szServerID);
	com9001_r.temp_id =  pFileinfo->cfups4003.id;
	memcpy(com9001_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
	com9001_r.upload_size = pFileinfo->cfups4003.file_size;
	

	com9001 ( com9001_r, g_szDcmdIP, g_nDcmdPort );

	CCOM9101_R com9101_r ;
	
	memset(&com9101_r,0x00,sizeof(CCOM9101_R));
	strcpy(com9101_r.conn_ip , com9001_r.conn_ip);
	strcpy(com9101_r.server_id , com9001_r.server_id);
	com9101_r.temp_id =  com9001_r.temp_id;
	strcpy(com9101_r.user_id ,com9001_r.user_id); // �����
	com9101_r.upload_size = com9001_r.upload_size;
	
	
	
	
	#ifdef __DEBUG
	printf("MyDiskFileDataTran   ] Recv Fileinfo title : %s\n",pFileinfo->cfups4003.title);
	printf("MyDiskFileDataTran   ] list path ( %s )\n",pFileinfo->cfups4003.file_path);
	#endif
	

	char strFullPath[768];
	memset(strFullPath,0x00,sizeof(strFullPath));
	char szFullName[768];
	memset(szFullName,0x00,sizeof(szFullName));


	
	CCOM9104_R pcom9104_r; // �޴� ���� ��ҽ� DB ������ ( T_CONTENTS_TEMP ���� )

	MFILEINFO rMFileinfo;
	
	double dTotalRecvLen = 0; //�� ���� ����
	double dTotalLen = 0; // down�� ������ �� ����
	int nWriteLen=0;      // ���Ͽ� �� ����
	int nRecvLen=0;       // �������� ���� ���� ����
	bool bCreateFile=false;
		
	int nCheckStop = 0;  // while �� ����
	
	/*
	char szErrMsg[256];
	memset(szErrMsg,0x00,sizeof(szErrMsg));
	*/
	CCOM9105_R com9105_r; // temp �� ���� ���� ����.	
	memset(&com9105_r,0x00,sizeof(CCOM9105_R));
	
	CCOM9103_R pcom9103_r; // �ʷα� �뷮 ����
	memset(&pcom9103_r,0x00,sizeof(CCOM9103_R));
	

	if(	pFileinfo->nType == FT_FILE)
	{
		pcom9103_r.proc_flag  =  2;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
		pcom9103_r.file_size = pFileinfo->dFileSize ;
		memcpy(pcom9103_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID));
		
		if(com9103(pcom9103_r, errheader.errmsg, g_szDcmdIP, g_nDcmdPort)< 0)
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
	
			pHeader->nCmd = RS_ERR; // ���� ó�� 
			com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
			
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
		
		if(com9103(pcom9103_r, errheader.errmsg, g_szDcmdIP, g_nDcmdPort)< 0)
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
			
			com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
			return -RS_MYDISK_FILE_DATA_TRANSFER;
			
		}
		
		//9105 ����	
		com9105_r.id = pFileinfo->cfups4003.id;  
		memcpy(&com9105_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
		strcpy(com9105_r.server_id ,pFileinfo->szServerID);
		strcpy(com9105_r.sfile_path ,pFileinfo->cfups4003.file_path);
		strcpy(com9105_r.sfile_name ,pFileinfo->cfups4003.file_name1);
		
		com9105(com9105_r, g_szDcmdIP, g_nDcmdPort);

	
	}

		
	do
	{
		nCheckStop++;

		if(nCheckStop >= 1100)
		{
		
			///////////////////////////////////////////////
			// temp ����									 //
			// ���� �뷮 (upload_size) ������Ʈ rollback  //
			// ���� ���� 								 //
			///////////////////////////////////////////////
			
			memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
			
			
			
			pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
			pcom9104_r.id         = pFileinfo->cfups4003.id;        // ������ID(T_CONTENTS_TEMP.id)
			memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
			pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
			if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
			{
				infLOG(ERROR, "Exception )  ���ڷ�� rollback ���� (com9104)\n"); 	
			}

	
			com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);

			return 0;			
						
		}			

	    	
		headers.nCmd  = RS_FILE_DATA_SIGN_CHECK; // ���� ���� �޼���    	
		
		if(pFileinfo->nReUploadFlag == RECONNECT_UPLOAD || pFileinfo->nReUploadFlag == RE_UPLOAD)
		{
	    	#ifdef __DEBUG
	    	printf("MyDiskFileDataTran   ] ( %d )\n" ,pFileinfo->nReUploadFlag);
	    	#endif
	    	
	    				
			if( pFileinfo->nType == FT_FOLDER)
			{
				strcpy(strFullPath, pFileinfo->cfups4003.file_path); //./2004/02/18/16/raid
				strcat(strFullPath,"/");
				strcat(strFullPath,pFileinfo->cfups4003.file_name1);//<- ��� �� �߸� ����
				
				//////////////////////////////////////////////////////////////////////////
				
				strcpy(szFullName,pFileinfo->szDownPath); //szDownPath �� ./raid
    			strcat(szFullName,"/");
    			strcat(szFullName,pFileinfo->szFileName); //szfilename �� a.txt
    			
    			#ifdef __DEBUG
				printf("MyDiskFileDataTran   ] FOLDER full path ( %s ) full name ( %s )\n",strFullPath,szFullName);
				#endif	
				infLOG(ERROR, "FileDataTransfer   RE] FOLDER full path ( %s ) full name ( %s )\n",strFullPath,szFullName);
								
			}
			else
			{
				
		    	strcpy(szFullName,pFileinfo->szDownPath);					
				strcat(szFullName,"/"); //./2004/02/18/16/raid/   <-- '/' �߰�
				strcat(szFullName,pFileinfo->szFileName);
				
				strcpy(strFullPath, pFileinfo->cfups4003.file_path); //./2004/02/18/16/raid
				strcat(strFullPath,"/");
				strcat(strFullPath,pFileinfo->cfups4003.file_name1);//
				
				infLOG(ERROR, "FileDataTransfer   RE] FOLDER full path ( %s ) full name ( %s )\n",strFullPath,szFullName);
				
				//9105 ����
				
				memset(&com9105_r,0x00,sizeof(CCOM9105_R));
			
				com9105_r.id = pFileinfo->cfups4003.id;  
				memcpy(&com9105_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
				strcpy(com9105_r.server_id ,pFileinfo->szServerID);
				strcpy(com9105_r.sfile_path ,pFileinfo->szDownPath);
				strcpy(com9105_r.sfile_name ,pFileinfo->szFileName);
				
				com9105(com9105_r, g_szDcmdIP, g_nDcmdPort);
						
				#ifdef __DEBUG
				printf("MyDiskFileDataTran   ] FOLDER full path ( %s ) full name ( %s )\n",strFullPath,szFullName);
				#endif							
					
			
					
									
			}

						
							 
			int stat = stat64(szFullName,&statbuf); 
			if(stat != 0) //������ ������ ���� �����.
			{
				#ifdef __DEBUG
				printf("MyDiskFileDataTran   ] the file is not visible..\n");
				#endif
				//if(errno == ENOENT) //�����̳� �н��� ����
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
				printf("MyDiskFileDataTran   ] ���� �� ���� 		] %s\n",pFileinfo->szDownPath);
				
				#endif
			}
						
		}
		else
		{
			
			#ifdef __DEBUG
	    	printf("MyDiskFileDataTran   ] NORMAL_UPLOAD\n" );
	    	#endif					
			
			bCreateFile = false;// ���� �̸��� �����Ǿ����� check
			
			///// ��¥ �ð� ���� ////
			time_t			curtime;
			struct tm		*stm;
			time( &curtime );
			stm = (struct tm *) localtime(&curtime);
		
			localtime_r(&curtime, stm);
			bool bResult = false;
			int nCheckLoop = 0;	
			
			srand((unsigned int)time(NULL))	; //random �̸��� ���� �õ� ����
						
			if(pFileinfo->nType == FT_FILE)
			{
				sprintf(pFileinfo->szDownPath,"%s/%04d/%02d/%02d/%02d",  pFileinfo->szDownPath
							      ,  stm->tm_year+1900
							      ,  stm->tm_mon + 1
							      ,  stm->tm_mday
							      ,  stm->tm_hour);//./2004/02/18/16
							      
				
	
				memset(szFullName,0x00,sizeof(szFullName));					
		    	
		    	strcpy(szFullName,pFileinfo->szDownPath);					
				strcat(szFullName,"/"); //./2004/02/18/16/raid/   <-- '/' �߰�
		    	//file name ���
		    	char szFilename[50];
		    	char szFileType[10];
		    	memset(szFilename,0x00,sizeof(szFilename));
		    	memset(szFileType,0x00,sizeof(szFileType));
		    	
		    	
				sprintf(szFilename,"%lu",pFileinfo->cfups4003.id);		
		    	
		    	//local �����̸����� ���� Ȯ���� ���.
		    	int nLen = GetReverseIndex(pFileinfo->cfups4003.file_name2 , '.');
				
				
				infLOG(ALWAY, " MyDiskFileDataTran   ] File �̸� ( %s ) \n",pFileinfo->cfups4003.file_name2); 
				if(nLen < 0)
				{
					#ifdef __DEBUG
					printf("MyDiskFileDataTran   ] Ȯ���ڸ� ������ ���� �ʽ��ϴ�.  %s\n",pFileinfo->cfups4003.file_name2);		
					#endif
					
					infLOG(ERROR, "MyDiskFileDataTran ER] Ȯ���ڸ� ������ ���� �ʽ��ϴ�.\n"); 
					
				}
				else		
					GetRightString(pFileinfo->cfups4003.file_name2 ,strlen(pFileinfo->cfups4003.file_name2 )-nLen,szFileType);
				
					
				
				strcpy(pFileinfo->cfups4003.file_name2,pFileinfo->szFileName);
				memset(pFileinfo->szFileName,0x00,sizeof(pFileinfo->szFileName));
				strcpy(pFileinfo->szFileName,szFilename);
				strcat(pFileinfo->szFileName,szFileType);
				strcat(szFullName,szFilename);
				strcat(szFullName,szFileType);
				
				//// �̸� ���� ////
				memcpy(pFileinfo->cfups4003.file_name1,pFileinfo->szFileName,sizeof(pFileinfo->szFileName));	
				strcpy(pFileinfo->cfups4003.file_path,pFileinfo->szDownPath);//,sizeof(pFileinfo->szDownPath));
										
			
				

				stat = stat64(szFullName,&statbuf); 
				
				infLOG(ALWAY, "MyDiskFileDataTran   ] File Ȯ��  ( %s ) ( %s ) (%d)\n",pFileinfo->szDownPath,szFullName,stat); 
				
				if(stat != 0) //������ ������ ���� �����.
				{

					//if(errno == ENOENT) //�����̳� �н��� ����
					{
						MakeFolder(pFileinfo->szDownPath) ;
						#ifdef __DEBUG
					//			01234567890123456789]
						printf("MyDiskFileDataTran   ] ���� ����  %s\n",pFileinfo->szDownPath);
						#endif
					}		
					bResult = true;	
				}
				else
				{
					#ifdef __DEBUG
				//			01234567890123456789]
					printf("MyDiskFileDataTran   ] ���� �� ����  %s\n",szFullName);
					#endif
					
					bResult = false;
				}
					

				//9105 ����					
				memset(&com9105_r,0x00,sizeof(CCOM9105_R));
			
				com9105_r.id = pFileinfo->cfups4003.id;  
				memcpy(&com9105_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
				strcpy(com9105_r.server_id ,pFileinfo->szServerID);
				strcpy(com9105_r.sfile_path ,pFileinfo->cfups4003.file_path);
				strcpy(com9105_r.sfile_name ,pFileinfo->cfups4003.file_name1);
				
				com9105(com9105_r, g_szDcmdIP, g_nDcmdPort);
										
			}
			else if(pFileinfo->nType == FT_FOLDER) //���� ���� ������ ���� �ϰ�� 
			{	


				strcpy(strFullPath, pFileinfo->cfups4003.file_path); //./2004/02/18/16/raid
				strcat(strFullPath,"/");
				strcat(strFullPath,pFileinfo->cfups4003.file_name1); 
		
				//////////////////////////////////////////////////////////////////////////
					
				strcpy(szFullName,pFileinfo->szDownPath); //szDownPath �� ./raid
				strcat(szFullName,"/");
				strcat(szFullName,pFileinfo->szFileName); //szfilename �� a.txt
		    	///// ���� ������ ��� ���� ����ü  ////
		    	
   				#ifdef __DEBUG
				printf("MyDiskFileDataTran   ] full path ( %s ) full name ( %s )\n",strFullPath,szFullName);
				#endif	

				int stat = stat64(szFullName,&statbuf); 
				if(stat != 0) //������ ������ ���� �����.
				{

					//if(errno == ENOENT) //�����̳� �н��� ����
					{
						MakeFolder(pFileinfo->szDownPath) ;
						#ifdef __DEBUG
					//			01234567890123456789]
						printf("MyDiskFileDataTran   ] ���� ����  %s\n",pFileinfo->szDownPath);
						#endif
					}		
				}
				else
				{
					#ifdef __DEBUG
					//			01234567890123456789]
					printf("MyDiskFileDataTran   ] ���� �� ����  %s\n",szFullName);
					#endif
					
					bFOpenAppendMode = true;	 
	
				}	
					
			}		
		}	
					
	//		}
	
		headers.nCmd = RS_FILE_DATA_SIGN_CHECK; //���� ����
	
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
			  
	    
	    
	
	    //// �����ϱ����� �޼����� �˸�...
//	    if(SendData(Socket,(char*)&headers,sizeof(struct _HEADER))<0)  //struct _PACKET == PACKET
		if( nSRet <= 0)
		{
			infLOG(ERROR, "MyDiskFileDataTran ER] send ���� 1: <client ����>\n"); 
			#ifdef __DEBUG
			printf("MyDiskFileDataTran ER] send ���� 1 : <client ����>\n");
			#endif
			if( pFileinfo->nType == FT_FILE )			
			{
				delete[] pSendData;
				pSendData = NULL;
			}
				
			///////////////////////////////////////////////
			// temp ����									 //
			// ���� �뷮 (upload_size) ������Ʈ rollback  //
			// ���� ���� 								 //
			///////////////////////////////////////////////
			
			memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
			
			
			
			pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
			pcom9104_r.id         = pFileinfo->cfups4003.id;        // ������ID(T_CONTENTS_TEMP.id)
			memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
			pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
			if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
			{
				infLOG(ERROR, "MyDiskFileDataTran ER] ���ڷ�� rollback ���� (com9104)\n"); 	
			}						
		
			
			com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
			// �޴� ���� ����
			
			return 0;
		}
	
	
		memset(&headers,0x00,sizeof(HEADER));
		
		if(	RecvData(Socket,(char*)&headers,sizeof(struct _HEADER))<0)  //struct _PACKET == PACKET
		{
			infLOG(ERROR, "MyDiskFileDataTran ER] recv ���� 1: <client ����>\n"); 
			#ifdef __DEBUG
			printf("MyDiskFileDataTran ER] recv ���� 1 : <client ����>\n");
			#endif
			
			
			///////////////////////////////////////////////
			// temp ����									 //
			// ���� �뷮 (upload_size) ������Ʈ rollback  //
			// ���� ���� 								 //
			///////////////////////////////////////////////
			
			memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
			
			
			
			pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
			pcom9104_r.id         = pFileinfo->cfups4003.id;        // ������ID(T_CONTENTS_TEMP.id)
			memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
			pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
			if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
			{
				infLOG(ERROR, "MyDiskFileDataTran ER]  ���ڷ�� rollback ���� (com9104)\n"); 	
			}						
		
		
			
			com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);	
			// �޴� ���� ����
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
			// temp ����									 //
			// ���� �뷮 (upload_size) ������Ʈ rollback  //
			// ���� ���� 								 //
			///////////////////////////////////////////////
			
			memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
			
			
			
			pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
			pcom9104_r.id         = pFileinfo->cfups4003.id;        // ������ID(T_CONTENTS_TEMP.id)
			memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
			pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
			if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
			{
				infLOG(ERROR, "MyDiskFileDataTran ER]  ���ڷ�� rollback ���� (com9104)\n"); 	
			}						
		
		
			
			com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);	

			// �޴� ���� ����
				
			return END;
		}
		if(headers.nCmd == RS_OK)
		{
			#ifdef __DEBUG
			printf("MyDiskFileDataTran   ] file recv ok..");
			#endif
		}
		
		////////////////////�⺻ ���� �Ϸ�////////////////////////////////////////////////
		
	
		
		
		
		// ���� ���� �� ����
		FILE* DownloadFile; //���� ������
		DownloadFile = NULL;
		//// ���� open���� ����////
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
				
		if(DownloadFile == NULL) //������ ���� ������
		{
			#ifdef __DEBUG
			char szErrMsg[1024];
			memset(szErrMsg,0x00,sizeof(szErrMsg));
			GetErrMsg(errno,szErrMsg);
			
			printf("MyDiskFileDataTran ER] file error : file is null\n"
			       "MyDiskFileDataTran ER]  file error : %s \n ",szErrMsg);
			       
			#endif
			infLOG(ERROR, "MyDiskFileDataTran ER] DownloadFile == NULL : <�޼��� ����>\n"); 
			#ifdef __DEBUG
			printf("MyDiskFileDataTran ER] DownloadFile == %s : <�޼��� ����>\n",szFullName);
			#endif
			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_MYDISK_FILE_DATA_TRANSFER;
			strcat(errheader.errmsg,"���� ���� ���� ����� ���� �Ͽ����ϴ�.");
			
			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));
		
			pHeader->nCmd = RS_ERR;
			
			///////////////////////////////////////////////
			// temp ����									 //
			// ���� �뷮 (upload_size) ������Ʈ rollback  //
			// ���� ���� 								 //
			///////////////////////////////////////////////
			
			memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
			
			
			
			pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
			pcom9104_r.id         = pFileinfo->cfups4003.id;        // ������ID(T_CONTENTS_TEMP.id)
			memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
			pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
			if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
			{
				infLOG(ERROR, "MyDiskFileDataTran ER]  ���ڷ�� rollback ���� (com9104)\n"); 	
			}						
		
		
			
			com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);	

			// �޴� ���� ����			
			return -RS_MYDISK_FILE_DATA_TRANSFER;
		}
		
		
		//// �̾� �ø��⸦ ���� ���� �ش� ����ü ���� ////
	
		//// ���� ���� �����͸� ������ �̵�////
	
	//	if(fseek(DownloadFile,0l,SEEK_END)!=0) //err
		if(fseeko64(DownloadFile,0l,SEEK_END))
		{
			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_MYDISK_FILE_DATA_TRANSFER;
			strcat(errheader.errmsg,"���� ���� ���� �̵� ���� �Ͽ����ϴ�.");
			
			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));
		
			pHeader->nCmd = RS_ERR;
			
			///////////////////////////////////////////////
			// temp ����									 //
			// ���� �뷮 (upload_size) ������Ʈ rollback  //
			// ���� ���� 								 //
			///////////////////////////////////////////////
			
			memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
			
			
			
			pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
			pcom9104_r.id         = pFileinfo->cfups4003.id;        // ������ID(T_CONTENTS_TEMP.id)
			memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
			pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
			if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
			{
				infLOG(ERROR, "MyDiskFileDataTran ER] ���ڷ�� rollback ���� (com9104)\n"); 	
			}						
		
		
			
			com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);	

			// �޴� ���� ����
			return -RS_MYDISK_FILE_DATA_TRANSFER;
		}

		LPFILEHEAD pFileHead = new FILEHEAD;
		memset(pFileHead,0x00,sizeof(FILEHEAD));
		
		
		//double dCurrentLen = (double)ftello64 (DownloadFile); // ������ ������ �ִ��� ����
		
		double dCurrentLen = 0;
	//			dCurrentLen = (double)statbuf.st_size;
		dCurrentLen = (double)ftello64 (DownloadFile); // ������ ������ �ִ��� ����

		
		
		infLOG(ALWAY, " MyDiskFileDataTran   ] Current File Size ( %15.0f )\n",dCurrentLen); 
	
		if(dCurrentLen < 0)
			dCurrentLen = 0;
			
		pFileHead->dCurrentSize = dCurrentLen; //�ص忡 �� ���� ���� ����		
		#ifdef __DEBUG
		printf("MyDiskFileDataTran   ] dCurrentLen : %15.0f\n",dCurrentLen);
		#endif
	
		
		// head �ۼ�
		memset(&headers,0x00,sizeof(HEADER));
	
		//key
		headers.nCmd = RS_MYDISK_FILE_DATA_TRANSFER ; // ������ ����		
		headers.nDataCnt = 1;
		headers.nDataSize = sizeof(FILEHEAD);
		headers.nErrorCode = 0;
	
		pSendData = new char[sizeof(HEADER) + headers.nDataCnt*headers.nDataSize];
	
		memcpy(pSendData,&headers,sizeof(HEADER));
		
		memcpy(pSendData + HEADER_SIZE,pFileHead, headers.nDataCnt*headers.nDataSize);
		
		//// body �ۼ�////
		if(SendData(Socket,pSendData,HEADER_SIZE + headers.nDataCnt*headers.nDataSize)<0)  //struct _PACKET == PACKET
		{
			delete pFileHead;
			///////////////////////////////////////////////
			// temp ����									 //
			// ���� �뷮 (upload_size) ������Ʈ rollback  //
			// ���� ���� 								 //
			///////////////////////////////////////////////
			
			memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
			
			
			
			pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
			pcom9104_r.id         = pFileinfo->cfups4003.id;        // ������ID(T_CONTENTS_TEMP.id)
			memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
			pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
			if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
			{
				infLOG(ERROR, "MyDiskFileDataTran ER]  ���ڷ�� rollback ���� (com9104)\n"); 	
			}						
		
		
			
			com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);	

			// �޴� ���� ����

			
			return 0;
		}
		delete[] pSendData;
		pSendData = NULL;
				
		delete pFileHead;
		
		#ifdef __DEBUG
		printf("MyDiskFileDataTran   ] file Head  send ok..%d\n",headers.nCmd);
		#endif
		
		dTotalRecvLen = 0; //�� ���� ����
		dTotalLen = pFileinfo->dFileSize - dCurrentLen; // down�� ������ �� ����
		#ifdef __DEBUG
		printf("MyDiskFileDataTran   ] dCurrentLen : %15.0f = %15.0f - %15.0f\n",dTotalLen,pFileinfo->dFileSize,dCurrentLen);
		#endif
				
		nWriteLen=0;
		nRecvLen=0;
		
		char* szRecvBuffer = new char[RECVBUF]; //recv buffer
	

		infLOG(ALWAY, "MyDiskFileDataTran   ] File Send Body \n\n"); 		
		
		
		#ifdef __DEBUG
		printf("MyDiskFileDataTran   ] Checking File (%s) : ���� ��ü ���� (%15.0f ) = %15.0f\n ",pFileinfo->cfups4003.file_name2 , dTotalLen ,pFileinfo->dFileSize );
		#endif
				
		infLOG(ALWAY,"MyDiskFileDataTran   ] Checking File (%s) : ���� ��ü ���� (%15.0f ) =  %15.0f \n",pFileinfo->cfups4003.file_name2 , dTotalLen ,pFileinfo->dFileSize );
		
			
		int fno = fileno(DownloadFile);	
		
		while(dTotalLen > 0  ) 
		{   	
			memset(szRecvBuffer,0x00,RECVBUF);	
			
			///// ���Ϲޱ� ///// ///// �� �ڷ�� ���� ó�� ����.
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
        			printf("MyDiskFileDataTran   ] file�� ��\n");
        			#endif
        			infLOG(ALWAY,"MyDiskFileDataTran   ] Write File End (%s) : ���� ��ü ���� (%15.0f ) =  %15.0f \n",pFileinfo->cfups4003.file_name2 , dTotalLen ,pFileinfo->dFileSize );
        			
        		}
        		else	
        		{
        			infLOG(ERROR,"MyDiskFileDataTran   ] Write File ERROR (%s) : ���� ��ü ���� (%15.0f ) =  %15.0f \n",pFileinfo->cfups4003.file_name2 , dTotalLen ,pFileinfo->dFileSize );
        			nRecvLen = -1;
        		}
        	}


	        if(nRecvLen <= 0 && dTotalLen != 0)	//�޴ٰ� ������ ������...DBó��
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
						        		
	        		infLOG(ERROR, " MyDiskFileDataTran ER] RecvSocket Error ( ������ �������ϴ�. ) (%s)\n",szErrMsg); 
					
					#ifdef __DEBUG
					printf("MyDiskFileDataTran ER] RecvSocket Error ( ������ �������ϴ�. ) (%s)\n",szErrMsg); 
					#endif
	        	}       	
				#ifdef __DEBUG
				printf("MyDiskFileDataTran   ] mydisk file recv exception \n");
				#endif														
		
				if(DownloadFile)
					fclose(DownloadFile);
		

	
			// temp �� �����ϱ�

							
				
				

				///////////////////////////////////////////////
					
				infLOG(ERROR,"MyDiskFileDataTran ER] �� �ڷ�� ���� �ޱ� ��� (%s) RecvLen (%d) dTotalRecvLen(%15.0f) TotalLen(%15.0f)\n"
				, pFileinfo->cfups4003.file_name2,nRecvLen  ,dTotalRecvLen,dTotalLen);  		
				

				//////////////////////////////////////////
				///////////////////////////////////////////////
				// temp ����									 //
				// ���� �뷮 (upload_size) ������Ʈ rollback  //
				// ���� ���� 								 //
				///////////////////////////////////////////////
				
				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
				
				
				
				pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
				pcom9104_r.id         = pFileinfo->cfups4003.id;        // ������ID(T_CONTENTS_TEMP.id)
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
				pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
				if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
				{
					infLOG(ERROR, "MyDiskFileDataTran ER]  ���ڷ�� rollback ���� (com9104)\n"); 	
				}						
			
			
				
				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);	
	
				// �޴� ���� ����
				
				//return END;
				return 0;
        	
        	}        
	        dTotalLen = dTotalLen - (double)nRecvLen;  //�ѱ��̿���  ���� ���� ����
	    	dTotalRecvLen = dTotalRecvLen + (double)nRecvLen; //���� ���� ��ŭ ����

        
	    	
	    	
	        
	
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
		//���� �̸� �ٲٱ�
		// DB �ֱ�..
	
	
	
		#ifdef __DEBUG
		printf("MyDiskFileDataTran   ] contents number : (%s) ( %lu ) (%lu)\n",pFileinfo->cfups4003.file_name2, pFileinfo->nNumber,pFileinfo->cfups4003.id);
		#endif
			
		infLOG(ALWAY,"MyDiskFileDataTran   ] My ��ũ Temp ��ȣ (%s) (%lu) (%lu)\n",pFileinfo->cfups4003.file_name2, pFileinfo->nNumber,pFileinfo->cfups4003.id); 
	
	
	//���⼭ ���� �ޱ� ����
			
		memset(&headers,0x00,sizeof(HEADER));
		if(RecvData(Socket,(char*)&headers,HEADER_SIZE ) <= 0)
		{
	
			infLOG(ERROR, "MyDiskFileDataTran ER] FileTransfer Recv Head Error\n");			
			
			#ifdef __DEBUG
			printf("MyDiskFileDataTran ER] WaitForRequst : ���� ����� ���� 1: <client ����>\n");
			#endif
			
			///////////////////////////////////////////////
			// temp ����									 //
			// ���� �뷮 (upload_size) ������Ʈ rollback  //
			// ���� ���� 								 //
			///////////////////////////////////////////////
			
			memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
			
			
			
			pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
			pcom9104_r.id         = pFileinfo->cfups4003.id;        // ������ID(T_CONTENTS_TEMP.id)
			memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
			pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
			if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
			{
				infLOG(ERROR, "MyDiskFileDataTran ER]  ���ڷ�� rollback ���� (com9104)\n"); 	
			}						
		
		
			
			com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);	

			// �޴� ���� ����
			return 0;
		}
	
		if(headers.nCmd == RS_MYDISK_FILE_REQUEST_NEXT_FILE )
		{
			
			#ifdef __DEBUG
			printf("\nMyDiskFileDataTran   ] RS_MYDISK_FILE_REQUEST_NEXT_FILE\n");
			#endif
			memset(&headers,0x00,HEADER_SIZE);
			
			headers.nCmd = RS_MYDISK_FILE_REQUEST_NEXT_FILEINFO; //���� ���� ��û
			headers.nDataCnt = 0;
			headers.nDataSize = 0;
			
			if(	SendData(Socket,(char*)&headers,HEADER_SIZE )<0)  //struct _PACKET == PACKET
			{
				infLOG(ERROR, "MyDiskFileDataTran ER]  SendData \n"); 
				#ifdef __DEBUG
				printf("MyDiskFileDataTran ER]  SendData \n");
				#endif
		
				///////////////////////////////////////////////
				// temp ����									 //
				// ���� �뷮 (upload_size) ������Ʈ rollback  //
				// ���� ���� 								 //
				///////////////////////////////////////////////
				
				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
				
				
				
				pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
				pcom9104_r.id         = pFileinfo->cfups4003.id;        // ������ID(T_CONTENTS_TEMP.id)
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
				pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
				if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
				{
					infLOG(ERROR, "MyDiskFileDataTran ER] ���ڷ�� rollback ���� (com9104)\n"); 	
				}						
			
			
				
				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);	
	
				// �޴� ���� ����
				
				return 0;
				
			}
			
			//recv file_transfer
			memset(&headers,0x00,HEADER_SIZE);
			if(RecvData(Socket,(char*)&headers,HEADER_SIZE ) <= 0)
			{
		
				infLOG(ERROR, "MyDiskFileDataTran ER] second FileTransfer Recv Head Error\n");			
				
				#ifdef __DEBUG
				printf("MyDiskFileDataTran ER]  WaitForRequst : ���� ����� ���� 1: <client ����>\n");
				#endif
			
				///////////////////////////////////////////////
				// temp ����									 //
				// ���� �뷮 (upload_size) ������Ʈ rollback  //
				// ���� ���� 								 //
				///////////////////////////////////////////////
				
				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
				
				
				
				pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
				pcom9104_r.id         = pFileinfo->cfups4003.id;        // ������ID(T_CONTENTS_TEMP.id)
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
				pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
				if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
				{
					infLOG(ERROR, "MyDiskFileDataTran ER] ���ڷ�� rollback ���� (com9104)\n"); 	
				}						
			
			
				
				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);	
	
				// �޴� ���� ����
				
				return 0;					
				
			}
		/*	 �ڵ� �߰� 
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
				// temp ����									 //
				// ���� �뷮 (upload_size) ������Ʈ rollback  //
				// ���� ���� 								 //
				///////////////////////////////////////////////
				
				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
				
				
				
				pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
				pcom9104_r.id         = pFileinfo->cfups4003.id;        // ������ID(T_CONTENTS_TEMP.id)
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
				pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
				if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
				{
					infLOG(ERROR, "MyDiskFileDataTran ER] ���ڷ�� rollback ���� (com9104)\n"); 	
				}						
			
			
				
				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);	
	
				// �޴� ���� ����
				
				return 0;		
			}
			
			pFileinfo = &rMFileinfo;
		
		}
		else if(headers.nCmd == RS_EOL)
		{
			
			if(pFileinfo->nTypeDisk == FT_MYDISK && pFileinfo->nNumber > 0 ) //������ ���
			{
				#ifdef __DEBUG
				printf("MyDiskFileDataTran ER] ������ ���\n");
				#endif
				if( dTotalLen == 0)
				{
					#ifdef __DEBUG
					printf("MyDiskFileDataTran ER] My ��ũ ���� ��� (%s)\n",pFileinfo->cfups4003.file_name2);
					#endif			
					infLOG(ALWAY,"MyDiskFileDataTran ER] My ��ũ ���� ��� (%s)\n",pFileinfo->cfups4003.file_name2); 
				
							
						
					if( fups4003(pFileinfo->cfups4003) == -1)//pFileinfo->cfups4001) == -1)
					{
						#ifdef __DEBUG
						printf("MyDiskFileDataTran ER] ���ڷ�� ��� ����\n");
						#endif
						infLOG(ERROR,"MyDiskFileDataTran ER]  ���ڷ�� ��� ���� \n"); 	
										
						memset(&headers,0x00,sizeof(HEADER));
						
								
						///////////////////////////////////////////////
						// temp ����									 //
						// ���� �뷮 (upload_size) ������Ʈ rollback  //
						// ���� ���� 								 //
						///////////////////////////////////////////////
						
						memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
						
						
						
						pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
						pcom9104_r.id         = pFileinfo->cfups4003.id;        // ������ID(T_CONTENTS_TEMP.id)
						memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
						pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
						if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
						{
							infLOG(ERROR, "MyDiskFileDataTran ER] ���ڷ�� rollback ���� (com9104)\n"); 	
						}						
					
					
						
						com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);	
						// �޴� ���� ����
						
										
						infLOG(ERROR, "MyDiskFileDataTran ER] ================== �� ��ũ ��� ���� ===================\n"
									  "MyDiskFileDataTran ER] ���� �Ƶ���( %s ) ���ϰ�� ( %s )                         \n"
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

							com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
							return 0;
						}
						com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
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
					// temp ����									 //
					// ���� �뷮 (upload_size) ������Ʈ rollback  //
					// ���� ���� 								 //
					///////////////////////////////////////////////
					
					memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
					
					
					
					pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
					pcom9104_r.id         = pFileinfo->cfups4003.id;        // ������ID(T_CONTENTS_TEMP.id)
					memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
					pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
					if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
					{
						infLOG(ERROR, "MyDiskFileDataTran ER]  ���ڷ�� rollback ���� (com9104)\n"); 	
					}						
				
				
					
					com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);	
		
					// �޴� ���� ����
					
					
					if(	SendData(Socket,(char*)&headers,HEADER_SIZE)<0)  //struct _PACKET == PACKET
					{
						infLOG(ERROR, "MyDiskFileDataTran ER]  send \n"); 
						#ifdef __DEBUG
						printf("MyDiskFileDataTran ER] send \n");
						#endif
						com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
						return 0;
					}
					com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
					return 1;
				}
				
			}
		
			#ifdef __DEBUG
			printf("MyDiskFileDataTran   ] recv sucessed...  \n");
			#endif
		
			infLOG(ALWAY, "MyDiskFileDataTran   ] Ouput ) FileDataTransfer\n");
				
				
			memset(&headers,0x00,HEADER_SIZE);
			
			
			headers.nCmd = RS_EOL; //���� ���� ��û
			headers.nDataCnt = 0;
			headers.nDataSize = 0;
			
			if(	SendData(Socket,(char*)&headers,HEADER_SIZE )<0)  //struct _PACKET == PACKET
			{
				infLOG(ERROR, "MyDiskFileDataTran ER]  SendData \n"); 
				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
				return 0;
			}
			com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
			return 0;
			
		}
		else
		{
			infLOG(ERROR, "MyDiskFileDataTran ER]  FileTransfer Recv  \n"); 
			#ifdef __DEBUG
			printf("MyDiskFileDataTran ER]  FileTransfer Recv  \n");
			#endif
			
			

			///////////////////////////////////////////////
			// temp ����									 //
			// ���� �뷮 (upload_size) ������Ʈ rollback  //
			// ���� ���� 								 //
			///////////////////////////////////////////////
			
			memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
			
			
			
			pcom9104_r.proc_flag  =  2;   // 1=wedisk 2=mydata
			pcom9104_r.id         = pFileinfo->cfups4003.id;        // ������ID(T_CONTENTS_TEMP.id)
			memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
			pcom9104_r.file_size = pFileinfo->cfups4003.file_size;
			if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
			{
				infLOG(ERROR, "MyDiskFileDataTran ER]  ���ڷ�� rollback ���� (com9104)\n"); 	
			}						
		
		
			
			com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);	

			// �޴� ���� ����
			return 0;
		}
		
				
	}while( 1 );
	
		com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
	return 0;	
	
}

