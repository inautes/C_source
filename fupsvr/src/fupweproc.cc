#include "fupsock.h"
#include "fupdefine.h"
#include "comcomm.h"
#include "apdefine.h"
#include "fupweproc.h"
#include "fupcomlib.h"
#include "comhead.h"
#include "com9001.h" //����ڼ� ����
#include "com9004.h" //���ι� ������ ��� ����
#include "com9101.h" //����ڼ� ����
#include "com9104.h" //���ε� ���� ó��
#include "com9103.h" //�ʷα� �ڷ�� ���ε� �뷮 Ȯ��
#include "com9105.h" //T_CONTENTS_TEMP �� ���� ����
#include "com9106.h" // �ߺ����� üũ
#include "fups40010.h"
#include "fups4005.h"
#include "fups4006.h"
#include "cmd5.h"
#include "fupmain.h"

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

// ���� ���� �䱸
int FileRequestNextFile(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	infLOG(ALWAY,"CMD > FileRequestNextFile\n");

	HEADER headers;
	memset(&headers,0x00,HEADER_SIZE);

	pSendData = new char[sizeof(HEADER)];
	memset(pSendData,0x00,sizeof(HEADER));

	headers.nCmd = RS_FILE_REQUEST_NEXT_FILEINFO; //���� ���� ��û
	headers.nDataCnt = 0;
	headers.nDataSize = 0;

	memcpy(pSendData,&headers,sizeof(HEADER));

	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));//return 1;
}

// ���� ���� �䱸
int FileRequestFile(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData)
{
	infLOG(ALWAY,"CMD > FileRequestFile\n");
	HEADER headers;
	memset(&headers,0x00,HEADER_SIZE);

	pSendData = new char[sizeof(HEADER)];
	memset(pSendData,0x00,sizeof(HEADER));

	headers.nCmd = RS_FILE_REQUEST_FILE_FILINFO; //���� ���� ��û
	headers.nDataCnt = 0;
	headers.nDataSize = 0;

	memcpy(pSendData,&headers,sizeof(HEADER));

	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));//return 1;
}

//���� ���( MY_DISK ���� ���� �϶� �ʿ� -> �ٸ� ������� ���� �� ��ũ  )
//���ڷ���϶� �߰�

// ���� ����Ʈ �䱸
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

	//pFileinfo->szDownPath �� root_path = /raid/fdata/wedisk
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

	if(stat != 0) //������ ������ ���� �����.
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

	memcpy(FolderInfo.cfups4001.file_path,szFullName,sizeof(szFullName)); //������ �н�
	memcpy(FolderInfo.cfups4001.file_name2,pFileinfo->szFolderName,sizeof(pFileinfo->szFolderName)); //�������� �̸�
	memcpy(FolderInfo.cfups4001.file_name1,szFolderName,sizeof(szFolderName));			 //��������

	pSendData = new char[sizeof(HEADER) + headers.nDataCnt * headers.nDataSize];
	memset(pSendData,0x00,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize);

	memcpy(pSendData,&headers,sizeof(HEADER)); //head
	memcpy(pSendData+sizeof(HEADER),&FolderInfo,  headers.nDataCnt * headers.nDataSize);

	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));
}

//������ ���� ����
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

	//���ι� ��� ���� 9004
	COM9004D com9004Result;
	memset(&com9004Result,0x00,sizeof(COM9004D));

	com9004Result = com9004(pHeader->szUserID, pFileinfo->cfups4001.id , pFileinfo->cfups4001.file_size, pFileinfo->cfups4001.descript/*no.767*/, g_szDcmdIP, g_nDcmdPort);
	int nCType = com9004Result.temp_id;

	infLOG(ALWAY,"Check 9004 Packet : \n"
				" long long temp_id = %lld      \n" //long long type
				" double file_size	= %15.0f    \n"
				" char user_id[16]  = %s        \n"
				" char auth_num[3]  = %s        \n"
				,	com9004Result.temp_id ,com9004Result.file_size	,com9004Result.user_id , com9004Result.auth_num );


	infLOG(ALWAY,"Check nCType[���Ÿ��] \ncom9004 result [ %d ] \n[ -90042 ���� ���� ��ȸ ���� ]\n[ -4 :�ʷα� ] \n[ -3 : ���Ϻ��� ] \n[ -2 : �Ϸ翡 �Ѱ� ] \n[ -1 : ����� ���������� ã�� �� �����ϴ�.] \n[ -5 : ���ε� ��� ���� ] \n[ 1 : ����ũ ]  \n",nCType );

	bool bHaveCopyright = false;
	bool bHaveCompany = false;
	bool bGhostMode = false; //������ �������� �ʰ� ��Ʈ���� �޴� ��� ����
	int nTotalRecvFileCnt = 0;

	char szSubFilePath[512];
	char szFolderPath[512];
	char szFolderFullPath[768];

	memset(szSubFilePath,0x00,sizeof(szSubFilePath));
	memset(szFolderPath,0x00,sizeof(szFolderPath));
	memset(szFolderFullPath,0x00,sizeof(szFolderFullPath));


	//change upload module - ���� �����ϴ� ����
	if( nCType == -5)  //no.767
	{
		pSendData = new char[sizeof(ERR_HEADER)];
		memset(pSendData,0x00,sizeof(ERR_HEADER));
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;

		strcat(errheader.errmsg,"����ũ ���α׷��� ���� �� �ֽ� �������� ������Ʈ ���ּ���.");

		memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

		pHeader->nCmd = RS_ERR;


		return -RS_FILE_DATA_TRANSFER;

	}
	else if( nCType == -4)  //�ʷα� �ڷ��
	{
		infLOG(ALWAY, "Start Send Filog Data \n");

		//9001 ȣ�� // ����ڼ� ����
		//9101 ȣ�� //����ڼ� ����

		CCOM9001_R com9001_r ;
		memset(&com9001_r,0x00,sizeof(CCOM9001_R));

		multimap<int,USERINFO>::iterator mi; //IP ��ȸ
		mi = m_UserList.find(Socket); 		//mi = m_UserList.begin();
		if(mi != m_UserList.end())
			strcpy(com9001_r.conn_ip ,mi->second.thread.userIP);

		strcpy(com9001_r.cont_gu ,"FD");
		strcpy(com9001_r.server_id , pFileinfo->szServerID);
		com9001_r.temp_id =  pFileinfo->cfups4001.id;
		memcpy(com9001_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
		com9001_r.upload_size = pFileinfo->cfups4001.file_size;
		com9001 ( com9001_r, g_szDcmdIP, g_nDcmdPort);

		CCOM9101_R com9101_r ;
		memset(&com9101_r,0x00,sizeof(CCOM9101_R));
		strcpy(com9101_r.conn_ip , com9001_r.conn_ip);
		strcpy(com9101_r.server_id , com9001_r.server_id);
		com9101_r.temp_id =  com9001_r.temp_id;
		strcpy(com9101_r.user_id ,com9001_r.user_id); // �����
		com9101_r.upload_size = com9001_r.upload_size;

		infLOG(ALWAY,"���� �˻� [ %s ] \n",pFileinfo->cfups4001.title);
		infLOG(ALWAY,"���� �˻� [ %s ]\n",pFileinfo->cfups4001.file_path);

		char szFullPath[768];
		memset(szFullPath,0x00,sizeof(szFullPath));

		char szFullName[768];
		memset(szFullName,0x00,sizeof(szFullName));

		int stat = -1;                 // ���� ���� ����
		bool bFOpenAppendMode = false; // ���� append ��� ����

		CCOM9104_R pcom9104_r; // �޴� ���� ��ҽ� DB ������ ( T_CONTENTS_TEMP ���� )

		FILEINFO rFileInfo;

		double dTotalRecvLen = 0; //�� ���� ����
		double dTotalLen = 0; // down�� ������ �� ����
		int nWriteLen=0;      // ���Ͽ� write �� ũ��
		int nRecvLen=0;       // �������� recv �� ũ��
		int nCheckStop = 0; //while ���� ����

		CCOM9103_R pcom9103_r; // �ʷα� �뷮 ����
		memset(&pcom9103_r,0x00,sizeof(CCOM9103_R));

		pcom9103_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 4=filog disk
		pcom9103_r.file_size = pFileinfo->cfups4001.file_size;
		memcpy(pcom9103_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID));

		memset(szErrMsg,0x00,sizeof(szErrMsg));

		if(com9103(pcom9103_r, szErrMsg, g_szDcmdIP, g_nDcmdPort)< 0)
		{
			infLOG(ALWAY, "�ʷα� �ڷ���� �뷮�� ������Ʈ �� �� �����ϴ�. [ com9013 - T_PERM_UPLOAD_AUTH ���̺��� Ȯ���ϼ��� ]\n");
			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
			strcat(errheader.errmsg,"�ʷα� �ڷ�� �뷮 ������Ʈ ���� �Դϴ�.");
			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));
			pHeader->nCmd = RS_ERR;

			com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);

			return -RS_FILE_DATA_TRANSFER;
		}

		CCOM9105_R com9105_r;		// temp �� ���� ���� ����.
		memset(&com9105_r,0x00,sizeof(CCOM9105_R));

		if(pFileinfo->nType == FT_FOLDER)
		{
			infLOG(ALWAY, "���� ���ε��Դϴ�.\n");

			//9105 ����	��� ��ȸ
			memset(&com9105_r,0x00,sizeof(CCOM9105_R));
			com9105_r.id = pFileinfo->cfups4001.id;
			memcpy(&com9105_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
			strcpy(com9105_r.server_id ,pFileinfo->szServerID);
			strcpy(com9105_r.sfile_path ,pFileinfo->cfups4001.file_path);
			strcpy(com9105_r.sfile_name ,pFileinfo->cfups4001.file_name1);
			com9105(com9105_r, g_szDcmdIP, g_nDcmdPort);
		}

		do
		{
			nCheckStop++; //����ó��
			if(nCheckStop >= 1100)
			{


				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
				pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
				pcom9104_r.id         = pFileinfo->cfups4001.id;        // ������ID(T_CONTENTS_TEMP.id)
				pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����

				//�ʷα� �뷮�� ���� ��Ų��.
				infLOG(ERROR, "�ʷα��� ���α� ������ �ʰ� �Ͽ����ϴ�.\ntemp_id [ %lu ]file count = %d , rollback size = %.0f [ com9104 ]\n",pFileinfo->cfups4001.id,nCheckStop , pcom9104_r.file_size);

				if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
				{
					infLOG(ERROR, "�ʷα� �뷮 ���� �� ������ �߻��Ͽ����Ϥ���.[com9104]\n");
				}
				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
				return 0;

			}
	    	infLOG(ALWAY,"�̾� �ø��� Flag[ %d ] >> [ 1 , 2 �� ��õ� 0 �� �Ϲ� ] \n",pFileinfo->nReUploadFlag);

		    headers.nCmd  = RS_FILE_DATA_SIGN_CHECK; // ���� ���� �޼���

			if(pFileinfo->nReUploadFlag == RECONNECT_UPLOAD || pFileinfo->nReUploadFlag == RE_UPLOAD)
			{// �̾� �ø���
				if( pFileinfo->nType == FT_FOLDER)
				{
					strcpy(szFullPath, pFileinfo->cfups4001.file_path); //./2004/02/18/16/raid
					strcat(szFullPath,"/");

					memset(szFolderFullPath,0x00,sizeof(szFolderFullPath));
					strcpy(szFolderFullPath, szFullPath); //./2004/02/18/16/raid

					strcat(szFullPath,pFileinfo->cfups4001.file_name1);//<- ��� �� �߸� ����

					strcpy(szFullName,pFileinfo->szDownPath); //szDownPath �� ./raid
	    			strcat(szFullName,"/");
	    			strcat(szFullName,pFileinfo->szFileName); //szfilename �� a.txt

					infLOG(ALWAY, "���� �̾� �ø��� - ��ġ [ %s ] ���� ��ġ [ %s ]\n",szFullPath,szFullName);
				}
				else
				{
			    	strcpy(szFullName,pFileinfo->szDownPath);
					strcat(szFullName,"/"); //./2004/02/18/16/raid/   <-- '/' �߰�
					strcat(szFullName,pFileinfo->szFileName);

					strcpy(szFullPath, pFileinfo->cfups4001.file_path); //./2004/02/18/16/raid
					strcat(szFullPath,"/");
					strcat(szFullPath,pFileinfo->cfups4001.file_name1);//


					infLOG(ALWAY, "���� �̾� �ø��� - ��ġ [ %s ] ���� ��ġ [ %s ]\n",szFullPath,szFullName);

					//9105 ����
					memset(&com9105_r,0x00,sizeof(CCOM9105_R));

					com9105_r.id = pFileinfo->cfups4001.id;
					memcpy(&com9105_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
					strcpy(com9105_r.server_id ,pFileinfo->szServerID);
					strcpy(com9105_r.sfile_path ,pFileinfo->szDownPath);
					strcpy(com9105_r.sfile_name ,pFileinfo->szFileName);

					com9105(com9105_r, g_szDcmdIP, g_nDcmdPort);

				}

				stat = stat64(szFullName,&statbuf);
				if(stat != 0) //������ ������ ���� �����.
				{
					infLOG(ALWAY,"������ �������� ������ �����մϴ�. ���� [ %s ] ���� [ %s ] \n",pFileinfo->szDownPath,szFullName);
					MakeFolder(pFileinfo->szDownPath) ;

				}
				else
				{
					infLOG(ALWAY,"������ �̹� ���� �մϴ�. Append ���� ������ �����մϴ�. ���� [ %s ] ���� [ %s ] \n",pFileinfo->szDownPath,szFullName);
					bFOpenAppendMode = true;

				}

			}
			else
			{ //�̾� �ø��� �ƴ�.
		    	infLOG(ALWAY,"�Ϲ� ���ε� ��� �Դϴ�.\n" );

		    	srand((unsigned int)time(NULL))	; //random �̸��� ���� �õ� ����

				///// ��¥ �ð� ���� ////
				time_t			curtime;
				struct tm		*stm;
				time( &curtime );
				stm = (struct tm *) localtime(&curtime);

				localtime_r(&curtime, stm);
				bool bResult = false;

		  		if( pFileinfo->nType == FT_FILE)
		  		{
		  			infLOG(ALWAY,"���� ���ε� �Դϴ�.\n");

		  			infLOG(ALWAY,"���� Root Path �� [ %s ] �Դϴ�.\n",pFileinfo->szDownPath);

					sprintf(pFileinfo->szDownPath,"%s/%04d/%02d/%02d/%02d",  pFileinfo->szDownPath
										,  stm->tm_year+1900
										,  stm->tm_mon + 1
										,  stm->tm_mday
										,  stm->tm_hour);//./2004/02/18/16

		  			infLOG(ALWAY,"���� Root Path �� �����մϴ�. [ %s ]\n",pFileinfo->szDownPath);

					memset(szFullName,0x00,sizeof(szFullName));

			    	strcpy(szFullName,pFileinfo->szDownPath);
					strcat(szFullName,"/"); //./2004/02/18/16/raid/   <-- '/' �߰�

			    	//file name ���

			    	char szFilename[50];
			    	char szFileType[10];
			    	memset(szFilename,0x00,sizeof(szFilename));
			    	memset(szFileType,0x00,sizeof(szFileType));


					sprintf(szFilename,"temp%lu",pFileinfo->cfups4001.id);
			    	//local �����̸����� ���� Ȯ���� ���.
			    	int nLen = GetReverseIndex(pFileinfo->cfups4001.file_name2 , '.');
					//	nLen = nLen - 1; // a.txt -> for .txt �ϱ� ���� nLen -1 ����
					//	nLen = nLen - 1; // ./raid/ -> ,./raid   , '/' delete
					infLOG(ALWAY, "���� �̸� �˻� [ %s ] \n",pFileinfo->cfups4001.file_name2);

					if(nLen < 0)
						infLOG(ALWAY, "���� �̸��� Ȯ���ڰ� �����ϴ�. [ ���� ]\n");
					else
					{
					    GetRightString(pFileinfo->cfups4001.file_name2,strlen(pFileinfo->cfups4001.file_name2)-nLen,szFileType);
					    infLOG(ALWAY, "���� Ȯ���� �˻� [ %s ] \n",szFileType);
					}

					strcpy(pFileinfo->cfups4001.file_name2,pFileinfo->szFileName);
					memset(pFileinfo->szFileName,0x00,sizeof(pFileinfo->szFileName));

					strcpy(pFileinfo->szFileName,szFilename);
					strcat(pFileinfo->szFileName,szFileType);
					strcat(szFullName,szFilename);
					strcat(szFullName,szFileType);

					//// �̸� ���� ////
					memcpy(pFileinfo->cfups4001.file_name1,pFileinfo->szFileName,sizeof(pFileinfo->szFileName));
					strcpy(pFileinfo->cfups4001.file_path,pFileinfo->szDownPath);//,sizeof(pFileinfo->szDownPath));

					stat = stat64(szFullName,&statbuf);

					if(stat != 0) //������ ������ ���� �����.
					{
						MakeFolder(pFileinfo->szDownPath) ;
						infLOG(ALWAY,"������ �������� ������ �����մϴ�. [ %s ] \n",pFileinfo->szDownPath);
					}
					else
					{
						infLOG(ALWAY,"������ �̹� ���� �մϴ�. Append ���� ������ �����մϴ�. [ %s ] \n",szFullName);
						bFOpenAppendMode = true;
					}

					//9105 ����
					memset(&com9105_r,0x00,sizeof(CCOM9105_R));

					com9105_r.id = pFileinfo->cfups4001.id;
					memcpy(&com9105_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
					strcpy(com9105_r.server_id ,pFileinfo->szServerID);
					strcpy(com9105_r.sfile_path ,pFileinfo->cfups4001.file_path);
					strcpy(com9105_r.sfile_name ,pFileinfo->cfups4001.file_name1);

					com9105(com9105_r, g_szDcmdIP, g_nDcmdPort);

				}
				else if(pFileinfo->nType == FT_FOLDER)//���� ���� ������ ���� �ϰ��
				{
					infLOG(ALWAY,"���� ���ε� �Դϴ�.\n");

					strcpy(szFullPath, pFileinfo->cfups4001.file_path); //./2004/02/18/16/raid
					strcat(szFullPath,"/");

					memset(szFolderFullPath,0x00,sizeof(szFolderFullPath));
					strcpy(szFolderFullPath, szFullPath); //./2004/02/18/16/raid

					strcat(szFullPath,pFileinfo->cfups4001.file_name1);//

					//////////////////////////////////////////////////////////////////////////

					strcpy(szFullName,pFileinfo->szDownPath); //szDownPath �� ./raid
	    			strcat(szFullName,"/");
	    			strcat(szFullName,pFileinfo->szFileName); //szfilename �� a.txt

					stat = stat64(szFullName,&statbuf);


					if(stat != 0) //������ ������ ���� �����.
					{
						MakeFolder(pFileinfo->szDownPath) ;
						infLOG(ALWAY,"������ �������� ������ �����մϴ�. [ %s ] \n",pFileinfo->szDownPath);
					}
					else
					{
						infLOG(ALWAY,"������ �̹� ���� �մϴ�. Append ���� ������ �����մϴ�. [ %s ] \n",szFullName);
						bFOpenAppendMode = true;
					}
				}
			}



			headers.nCmd = RS_FILE_DATA_SIGN_CHECK; //���� ����
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

		    //// �����ϱ����� �޼����� �˸�...
		    if(	nSRet <=0 )
			{
				infLOG(ERROR, "RS_FILE_DATA_SIGN_CHECK ���� ����.\n");

				// �޴� ���� ����
				///////////////////////////////////////////////
				// temp ����									 //
				///////////////////////////////////////////////

				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
				pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
				pcom9104_r.id         = pFileinfo->cfups4001.id;        // ������ID(T_CONTENTS_TEMP.id)
				pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����

				infLOG(ERROR, "�ʷα� �뷮�� ���� �մϴ�. File Size [ %.0f ]\n",pcom9104_r.file_size);
				if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
				{
					infLOG(ERROR, "�ʷα� �뷮 ���� �� ������ �߻��Ͽ����Ϥ���.[com9104]\n");
				}
				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
				return 0;
			}

			memset(&headers,0x00,sizeof(HEADER));

			if(	RecvData(Socket,(char*)&headers,sizeof(struct _HEADER))<=0)  //struct _PACKET == PACKET
			{
				infLOG(ERROR, "RS_FILE_DATA_SIGN_CHECK ��� �ޱ� ����\n");
				// �޴� ���� ����
				///////////////////////////////////////////////
				// temp ����									 //
				///////////////////////////////////////////////

				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
				pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
				pcom9104_r.id         = pFileinfo->cfups4001.id;        // ������ID(T_CONTENTS_TEMP.id)
				pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����

				infLOG(ERROR, "�ʷα� �뷮�� ���� �մϴ�. File Size [ %.0f ]\n",pcom9104_r.file_size);
				if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
				{
					infLOG(ERROR, "�ʷα� �뷮 ���� �� ������ �߻��Ͽ����Ϥ���.[com9104]\n");
				}
				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
				return 0;
			}

			if(headers.nCmd == RS_EOL)
			{
				infLOG(ALWAY, "RS_FILE_DATA_SIGN_CHECK ��� �ޱ� - RS_EROL \n");

				pSendData = new char[sizeof(HEADER)];
				memset(pSendData,0x00,sizeof(HEADER));

				headers.nCmd = RS_EOL;
				headers.nDataCnt = 0;
				headers.nDataSize = 0;
				headers.nErrorCode = 0;

				memcpy(pSendData, &headers, sizeof(HEADER));

				// �޴� ���� ����
				///////////////////////////////////////////////
				// temp ����									 //
				///////////////////////////////////////////////

				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
				pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
				pcom9104_r.id         = pFileinfo->cfups4001.id;        // ������ID(T_CONTENTS_TEMP.id)
				pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����

				infLOG(ERROR, "�ʷα� �뷮�� ���� �մϴ�. File Size [ %.0f ]\n",pcom9104_r.file_size);
				if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
				{
					infLOG(ERROR, "�ʷα� �뷮 ���� �� ������ �߻��Ͽ����Ϥ���.[com9104]\n");
				}
				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
				return END;
			}
			else if(headers.nCmd == RS_OK)
			{
				infLOG(ALWAY, "RS_FILE_DATA_SIGN_CHECK ��� �ޱ� - RS_OK \n");

			}

			//2009/09/09(�ʷα� ���͸� ����) �·�ī ���� �ޱ�.
			int nMurekaCnt = headers.nDataCnt;

			LPMUREKA_VINFO pMurekaVInfo = NULL;

			infLOG(ALWAY, "���͸� ��� ���� Ȯ�� - ���� [ %d ] \n",nMurekaCnt);

			if(nMurekaCnt > 0)
			{
				pMurekaVInfo = new MUREKA_VINFO[nMurekaCnt];
				if(	RecvData(Socket,(char*)pMurekaVInfo,sizeof(MUREKA_VINFO)*nMurekaCnt)<=0)  //struct _PACKET == PACKET
				{

					infLOG(ERROR,"�ʷα� �·�ī ��� �ޱ� ���� size : (%d) nMurekaCnt : (%d) \n", sizeof(MUREKA_VINFO)*nMurekaCnt, nMurekaCnt);

					// �޴� ���� ����
					///////////////////////////////////////////////
					// temp ����									 //
					///////////////////////////////////////////////
					memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
					pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
					pcom9104_r.id         = pFileinfo->cfups4001.id;        // ������ID(T_CONTENTS_TEMP.id)
					pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
					memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����

					infLOG(ERROR, "�ʷα� �뷮�� ���� �մϴ�. File Size [ %.0f ]\n",pcom9104_r.file_size);
					if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
					{
						infLOG(ERROR, "�ʷα� �뷮 ���� �� ������ �߻��Ͽ����Ϥ���.[com9104]\n");
					}

					com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
					return 0;
				}

				#ifdef __DEBUG
				for(int i=0; i < nMurekaCnt; i++)
				{
					printf("�ʷα� �·�ī ���� Ȯ��(%d).\n", i);
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

			////////////////////�⺻ ���� �Ϸ�////////////////////////////////////////////////

			//2009/09/09(�ʷα� ���͸� ����) �Ϲ� ������ ���ε� ������ 4005, 4006�κ� �߰��ؾ���.

			if( strcmp(pFileinfo->szCopyright_yn ,"P") == 0 )
			{
				infLOG(ALWAY,"���۱� flag �缺�� : P -> N \n");
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

			//2009/06/14 �·�ī ��ȸ ����.
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

			//���۱� ���� �����
			infLOG(ALWAY,"�ʷα� ���۱� ���� Ȯ�� 1 : tpye [ %d ] == [ %d ] : sect_code [ %s ] : copyright [ %s ] \n", pFileinfo->nType , FT_FOLDER , pFileinfo->cfups4001.sect_code , pFileinfo->szCopyright_yn);

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

			infLOG(ALWAY,"fups4005 �����͸� ������Ʈ ���Դϴ�.\n"	);
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

			//2009/06/14 �·�ī ��ȸ ����.
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
				nCopyRight = fupsflog4005(Fups4005, pMurekaVInfo);	//���۱� ��ȸ
			}
			infLOG(ALWAY,"���۱� ��ȸ ��� [ %d ] \n\n\n",nCopyRight);
			if( nCopyRight <= 0 )
				nCompany = fupsflog4006(Fups4006, pMurekaVInfo);	//���۱ǿ� �ɸ����ʴ� �ڷ��� �������������� ��ȸ.
			infLOG(ALWAY,"���� ��ȸ ���   [ %d ] \n\n\n",nCompany  );


			if(pMurekaVInfo)
			{
				delete[] pMurekaVInfo;
				pMurekaVInfo = NULL;
			}

			// 20140523 : ���� ó���ϱ�
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

			infLOG(ALWAY,"�ʷα� �ڷ�� Ȯ�� : sect_code [ %s ] : copyright_yn [ %s ] \n" , pFileinfo->cfups4001.sect_code , pFileinfo->cfups4001.copyright_yn);

			if( nCopyRight < 0 )
			{
				infLOG(ERROR, "���۱� ��ȸ �����Դϴ�. Error Num [ %d ]\n",nCopyRight);

				pSendData = new char[sizeof(ERR_HEADER)];
				memset(pSendData,0x00,sizeof(ERR_HEADER));
				errheader.header.nCmd = RS_ERR;
				errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
				strcat(errheader.errmsg,"���� ���� ��� ���� �Դϴ�.");

				memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

				pHeader->nCmd = RS_ERR;

				// �޴� ���� ����
				///////////////////////////////////////////////
				// temp ����									 //
				///////////////////////////////////////////////
				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));
				pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
				pcom9104_r.id         = pFileinfo->cfups4001.id;        // ������ID(T_CONTENTS_TEMP.id)
				pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����

				infLOG(ERROR, "�ʷα� �뷮�� ���� �մϴ�. File Size [ %.0f ]\n",pcom9104_r.file_size);
				if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
				{
					infLOG(ERROR, "�ʷα� �뷮 ���� �� ������ �߻��Ͽ����Ϥ���.[com9104]\n");
				}

				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
				return -RS_FILE_DATA_TRANSFER;
			}

			// ���� ���� �� ����

			FILE* DownloadFile; //���� ������
			DownloadFile = NULL;
			//// ���� open���� ����////

			infLOG(ALWAY,"���Ͽ��� : [ %s ]\n",szFullName);

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

			if(DownloadFile == NULL) //������ ���� ������
			{
				memset(szErrMsg,0x00,sizeof(szErrMsg));
				GetErrMsg(errno,szErrMsg);

				infLOG(ERROR, "���� ���� ���� �Դϴ�. [ %s ] error num [ %d ] msg [ %s ] \n",szFullName,errno, szErrMsg);

				pSendData = new char[sizeof(ERR_HEADER)];
				memset(pSendData,0x00,sizeof(ERR_HEADER));
				errheader.header.nCmd = RS_ERR;
				errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
				strcat(errheader.errmsg,"�������� ���� ����� ���� �Ͽ����ϴ�.");

				memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

				pHeader->nCmd = RS_ERR;

				// �޴� ���� ����
				///////////////////////////////////////////////
				// temp ����									 //
				///////////////////////////////////////////////

				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));


				pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
				pcom9104_r.id         = pFileinfo->cfups4001.id;        // ������ID(T_CONTENTS_TEMP.id)
				pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����


				infLOG(ERROR, "�ʷα� �뷮�� ���� �մϴ�. File Size [ %.0f ]\n",pcom9104_r.file_size);
				if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
				{
					infLOG(ERROR, "�ʷα� �뷮 ���� �� ������ �߻��Ͽ����Ϥ���.[com9104]\n");
				}

				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
				return -RS_FILE_DATA_TRANSFER;
			}

			//// �̾� �ޱ⸦ ���� ���� �ش� ����ü ���� ////
			infLOG(ALWAY,"������ Seek �� ������ ��ġ�� �̵� �մϴ�.\n");
			if(fseeko64(DownloadFile,0l,SEEK_END) < 0)
			{
				infLOG(ALWAY,"������ Seek �� ������ ��ġ�� �̵� �� ���� �߻��Ͽ����ϴ�. errno [ %d ] error msg [ %s ]\n",errno,strerror(errno));

				pSendData = new char[sizeof(ERR_HEADER)];
				memset(pSendData,0x00,sizeof(ERR_HEADER));
				errheader.header.nCmd = RS_ERR;
				errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
				strcat(errheader.errmsg,"���� ������ ���� �̵� ���� �Ͽ����ϴ�.");

				memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

				pHeader->nCmd = RS_ERR;
				// �޴� ���� ����
				///////////////////////////////////////////////
				// temp ����									 //
				///////////////////////////////////////////////

				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));

				pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
				pcom9104_r.id         = pFileinfo->cfups4001.id;        // ������ID(T_CONTENTS_TEMP.id)
				pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����

				infLOG(ERROR, "�ʷα� �뷮�� ���� �մϴ�. File Size [ %.0f ]\n",pcom9104_r.file_size);
				if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
				{
					infLOG(ERROR, "�ʷα� �뷮 ���� �� ������ �߻��Ͽ����Ϥ���.[com9104]\n");
				}

				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
				return -RS_FILE_DATA_TRANSFER;
			}

			LPFILEHEAD pFileHead = new FILEHEAD;
			memset(pFileHead,0x00,sizeof(FILEHEAD));

			double dCurrentLen	= 0;

			dCurrentLen = (double)ftello64 (DownloadFile); // ������ ������ �ִ��� ����

			infLOG(ALWAY, "�ֱ� �̵��� ���� ������ ( %.0f )\n",dCurrentLen);

			if(dCurrentLen < 0)
				dCurrentLen = 0;

			pFileHead->dCurrentSize = dCurrentLen; //�ص忡 �� ���� ���� ����

			// head �ۼ�
			memset(&headers,0x00,sizeof(HEADER));

			infLOG(ALWAY,"Send RS_FILE_DATA_TRANSFER\n");

			headers.nCmd = RS_FILE_DATA_TRANSFER ; // ������ ����
			headers.nDataCnt = 1;
			headers.nDataSize = sizeof(FILEHEAD);
			headers.nErrorCode = 0;

			pSendData = new char[sizeof(HEADER) + headers.nDataCnt*headers.nDataSize];

			memcpy(pSendData,&headers,sizeof(HEADER));

			memcpy(pSendData + HEADER_SIZE,pFileHead, headers.nDataCnt*headers.nDataSize);

			//// body �ۼ�////
			if(	SendData(Socket,pSendData,HEADER_SIZE + headers.nDataCnt*headers.nDataSize)<0)  //struct _PACKET == PACKET
			{
				infLOG(ERROR,"Send RS_FILE_DATA_TRANSFER ERROR\n");
				delete pFileHead;
				// �뷮 ���� �ϱ�
				// �޴� ���� ����
				///////////////////////////////////////////////
				// temp ����									 //
				///////////////////////////////////////////////

				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));

				pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
				pcom9104_r.id         = pFileinfo->cfups4001.id;        // ������ID(T_CONTENTS_TEMP.id)
				pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����

				infLOG(ERROR, "�ʷα� �뷮�� ���� �մϴ�. File Size [ %.0f ]\n",pcom9104_r.file_size);
				if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
				{
					infLOG(ERROR, "�ʷα� �뷮 ���� �� ������ �߻��Ͽ����Ϥ���.[com9104]\n");
				}


				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
				return 0;
			}
			delete[] pSendData;
			pSendData = NULL;

			delete pFileHead;

			infLOG(ALWAY,"Send RS_FILE_DATA_TRANSFER OK \n");

		 ///////////////////////// ������ ���� //////////////////////////////////

			dTotalRecvLen = 0; //�� ���� ����
			dTotalLen = pFileinfo->dFileSize - dCurrentLen; // down�� ������ �� ����
			nWriteLen=0;
			nRecvLen=0;

			char* szRecvBuffer = new char[RECVBUF]; //recv buffer

			infLOG(ALWAY,"���� Ȯ�� [ %s ] : ���� ���� ��ü ���� [ %.0f ] = [ %.0f (��ü) - %.0f(�ֱ��̵���) ] \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize ,dCurrentLen);

			int fno = fileno(DownloadFile);

			while(dTotalLen > 0  )
			{
				memset(szRecvBuffer,0x00,RECVBUF);
				///// ���Ϲޱ� /////

				nRecvLen =  RecvFileData(Socket, szRecvBuffer, RECVBUF, dTotalLen) ;

		        if(nRecvLen > 0)
		        {
		        	nWriteLen = write(fno ,szRecvBuffer,nRecvLen);
		      	   	//nWriteLen = fwrite(szRecvBuffer,nRecvLen,1,DownloadFile);
		        }
		        else
		        	nWriteLen = 0;

		        //	fwrite(szRecvBuffer,1,nRecvLen,DownloadFile); //���� ���� ��ŭ file�� ����

		    	if(nWriteLen <= 0)
	        	{


	        		if(nWriteLen == 0)
	        		{
	        			#ifdef __DEBUG
	        			printf(" ] Write File End (%s) : ���� ��ü ���� (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
	        			#endif
	        			infLOG(ALWAY,"Write File End (%s) : ���� ��ü ���� (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
	        		}
	        		else
	        		{
	        			#ifdef __DEBUG
	        			printf(" ] Write File ERROR (%s) : ���� ��ü ���� (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
	        			#endif
	        			infLOG(ERROR," ] Write File ERROR (%s) : ���� ��ü ���� (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
	        			nRecvLen = -1;
	        		}
	        		infLOG(ERROR,"errno [ %d ] error msg [ %s ]\n",errno,strerror(errno));
	        	}

		        if(nRecvLen <= 0 && dTotalLen != 0)	//�޴ٰ� ������ ������...DBó��
		        {


					if(nRecvLen < 0)
		        	{
						memset(szErrMsg,0x00,sizeof(szErrMsg));
						GetErrMsg(-nRecvLen,szErrMsg);
						infLOG(ERROR, "�����͸� ���� �� �����ϴ�. ( %d )( %s )\n",nRecvLen,szErrMsg);
		        	}
		        	else if(nRecvLen == 0)
		        	{
						memset(szErrMsg,0x00,sizeof(szErrMsg));
						GetErrMsg(-nRecvLen,szErrMsg);

		        		infLOG(ERROR, "������ ���� �����ϴ�.[ �̰��� ���� Ŭ���̾�Ʈ���� �����͸� ����� ������ ���Ҷ� �߻��մϴ�. ] \n" );

		        	}


					infLOG(ERROR,"�ʷα� ��� (%s) RecvLen (%d) (%15.0f) TotalLen(%15.0f)\n ",pFileinfo->cfups4001.file_name2,nRecvLen ,dTotalRecvLen,dTotalLen);
					infLOG(ERROR,"errno [ %d ] error msg [ %s ]\n",errno,strerror(errno));

					if(DownloadFile)
					{
						fclose(DownloadFile);
						DownloadFile == NULL ;
					}

					memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));

					pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
					pcom9104_r.id         = pFileinfo->cfups4001.id;        // ������ID(T_CONTENTS_TEMP.id)
					pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
					memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����

					infLOG(ERROR, "�ʷα� �뷮�� ���� �մϴ�. File Size [ %.0f ]\n",pcom9104_r.file_size);
					if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
					{
						infLOG(ERROR, "�ʷα� �뷮 ���� �� ������ �߻��Ͽ����Ϥ���.[com9104]\n");
					}

					////////////////////////////////////////////////

				   	if(szRecvBuffer)
						delete[] szRecvBuffer;

					com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
					return 0;
					//	return END;
	        	}
	       		dTotalLen = dTotalLen - (double)nRecvLen;  //�ѱ��̿���  ���� ���� ����
	        	dTotalRecvLen = dTotalRecvLen + (double)nRecvLen; //���� ���� ��ŭ ����
			}

			if(DownloadFile)
			{
				fclose(DownloadFile);
				DownloadFile == NULL ;
			}

			if(	szRecvBuffer)
				delete[] szRecvBuffer;

			///////////////////////////////////////////////
			//���� �̸� �ٲٱ�
			// DB �ֱ�..

			infLOG(ALWAY,"�ʷα� ������ �ޱ� �Ϸ� �� Ȯ�� - �����̸� (%s) �ӽù�ȣ (%lu) �ӽù�ȣ (%lu)\n",pFileinfo->cfups4001.file_name2, pFileinfo->nNumber,pFileinfo->cfups4001.id);

			//���⼭ ���� �ޱ� ����

			infLOG(ALWAY,"���� �Ϸ� �� �ش� ���� ���.\n");
			memset(&headers,0x00,sizeof(HEADER));
			if(RecvData(Socket,(char*)&headers,HEADER_SIZE ) <= 0)
			{
				infLOG(ERROR,"���� �Ϸ� �� �ش� ���� ��� ����.\n");

				// �޴� ���� ����
				///////////////////////////////////////////////
				// temp ����									 //
				//////////////////////////////////////////////
				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));

				pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
				pcom9104_r.id         = pFileinfo->cfups4001.id;        // ������ID(T_CONTENTS_TEMP.id)
				pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����

				infLOG(ERROR, "�ʷα� �뷮�� ���� �մϴ�. File Size [ %.0f ]\n",pcom9104_r.file_size);
				if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
				{
					infLOG(ERROR, "�ʷα� �뷮 ���� �� ������ �߻��Ͽ����Ϥ���.[com9104]\n");
				}

				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);

				return 0;
			}
			infLOG(ERROR,"���� �Ϸ� �� �ش� ���� ��� ��� [ %d ].\n",headers.nCmd);

			if(headers.nCmd == RS_FILE_REQUEST_NEXT_FILE )
			{

				infLOG(ALWAY, "RS_FILE_REQUEST_NEXT_FILE\n���� ������ �޽��ϴ�.\n");

				memset(&headers,0x00,HEADER_SIZE);

				headers.nCmd = RS_FILE_REQUEST_NEXT_FILEINFO; //���� ���� ��û
				headers.nDataCnt = 0;
				headers.nDataSize = 0;

				if(	SendData(Socket,(char*)&headers,HEADER_SIZE )<0)  //struct _PACKET == PACKET
				{
					infLOG(ERROR, "RS_FILE_REQUEST_NEXT_FILE ���� ���� \n");


					// �޴� ���� ����
					///////////////////////////////////////////////
					// temp ����									 //
					///////////////////////////////////////////////

					memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));

					pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
					pcom9104_r.id         = pFileinfo->cfups4001.id;        // ������ID(T_CONTENTS_TEMP.id)
					pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
					memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����

					infLOG(ERROR, "�ʷα� �뷮�� ���� �մϴ�. File Size [ %.0f ]\n",pcom9104_r.file_size);
					if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
					{
						infLOG(ERROR, "�ʷα� �뷮 ���� �� ������ �߻��Ͽ����Ϥ���.[com9104]\n");
					}


					com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);

					return 0;
				}
				infLOG(ERROR, "RS_FILE_REQUEST_NEXT_FILE ���� ��� �� \n");

				//recv file_transfer
				memset(&headers,0x00,HEADER_SIZE);
				if(RecvData(Socket,(char*)&headers,HEADER_SIZE ) <= 0)
				{
					infLOG(ERROR, "RS_FILE_REQUEST_NEXT_FILE ���� ��� �� ���� [ �̰��� ���� Ŭ���̾�Ʈ�κ��� �����͸� ���� ���� �� �߻��մϴ�. \n");
					// �޴� ���� ����
					///////////////////////////////////////////////
					// temp ����									 //
					///////////////////////////////////////////////

					memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));

					pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
					pcom9104_r.id         = pFileinfo->cfups4001.id;        // ������ID(T_CONTENTS_TEMP.id)
					pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
					memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����

					infLOG(ERROR, "�ʷα� �뷮�� ���� �մϴ�. File Size [ %.0f ]\n",pcom9104_r.file_size);
					if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
					{
						infLOG(ERROR, "�ʷα� �뷮 ���� �� ������ �߻��Ͽ����Ϥ���.[com9104]\n");
					}


					com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
					return 0;
				}

				memset(&rFileInfo,0x00,sizeof(FILEINFO));
				infLOG(ERROR, "���� �������� �ޱ� ��� �� \n");

				if( RecvData(Socket,(char*)&rFileInfo,sizeof(FILEINFO) ) <= 0)
				{
					infLOG(ERROR, "���� �������� �ޱ� ��� �� ���� \n");

					// �޴� ���� ����
					///////////////////////////////////////////////
					// temp ����									 //
					///////////////////////////////////////////////

					memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));

					pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
					pcom9104_r.id         = pFileinfo->cfups4001.id;        // ������ID(T_CONTENTS_TEMP.id)
					pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
					memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����

					infLOG(ERROR, "�ʷα� �뷮�� ���� �մϴ�. File Size [ %.0f ]\n",pcom9104_r.file_size);
					if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
					{
						infLOG(ERROR, "�ʷα� �뷮 ���� �� ������ �߻��Ͽ����Ϥ���.[com9104]\n");
					}


					com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
					return 0;
				}
				pFileinfo = &rFileInfo;
			}
			else if(headers.nCmd == RS_EOL)
			{
				//���

				infLOG(ALWAY,"RS_EOL\n�ʷα��� �������� ����մϴ�. �ӽ� ��ȣ �˻�(T_CONTENTS_TEMP) [ %lu ]\n",pFileinfo->nNumber );

				// ����� eol ������
				if(pFileinfo->nTypeDisk == FT_WEDISK && pFileinfo->nNumber > 0 ) //������ ���
				{
					if( dTotalLen == 0)
					{
						infLOG(ALWAY,"�ʷα� ���� ��� - �ӽù�ȣ [ %lu ] �����̸�  [ %s ]  \n",pFileinfo->nNumber,pFileinfo->cfups4001.file_name2);

						int nResult = fups4001(pFileinfo->cfups4001);
						infLOG(ALWAY,"�ʷα� ��ϰ��(fups4001) Result [ %d ] \n",nResult);
						if(  nResult < 0 )//pFileinfo->cfups4001) == -1)
						{
							// �޴� ���� ����
							///////////////////////////////////////////////
							// temp ����									 //
							///////////////////////////////////////////////

							//�ʷα� ����� ���� �߻� ...�� ���� �ؾ� �� ��ϵ�

							infLOG(ERROR, "================== �ʷα� ��� ����(FilogError) ===================\n"
										  "�ӽù�ȣ ( %lu )���� ���̵�( %s ) ���ϰ�� ( %s )                         \n"
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
							pcom9104_r.id         = pFileinfo->cfups4001.id;        // ������ID(T_CONTENTS_TEMP.id)
							pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
							memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����

							infLOG(ALWAY,"RS_FILE_END_FAIL ���� \n");


							if(	SendData(Socket,(char*)&headers,HEADER_SIZE)<0)  //struct _PACKET == PACKET
							{
								infLOG(ERROR, "�ʷα� �뷮�� ���� �մϴ�. File Size [ %.0f ]\n",pcom9104_r.file_size);
								if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
								{
									infLOG(ERROR, "�ʷα� �뷮 ���� �� ������ �߻��Ͽ����Ϥ���.[com9104]\n");
								}

								com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
								return 0;
							}
							infLOG(ERROR, "�ʷα� �뷮�� ���� �մϴ�. File Size [ %.0f ]\n",pcom9104_r.file_size);
							if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
							{
								infLOG(ERROR, "�ʷα� �뷮 ���� �� ������ �߻��Ͽ����Ϥ���.[com9104]\n");
							}


							com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
							return 1;
						}
						infLOG(ALWAY,"============== �ʷα� ���� ��� �Ϸ� ===============\n");
					}
					else
					{
						infLOG(ERROR, "============ �ʷα� ��� ���� - ������ ������ ���� ���Ͽ����ϴ�. ========== \n");
						memset(&headers,0x00,sizeof(HEADER));

						// �޴� ���� ����

						headers.nCmd = RS_FILE_END_FAIL;
						headers.nDataCnt = 0;
						headers.nDataSize = 0;
						headers.nErrorCode = 4001;

						if(	SendData(Socket,(char*)&headers,HEADER_SIZE)<=0)  //struct _PACKET == PACKET
						{
							infLOG(ERROR, "�ʷα� �뷮�� ���� �մϴ�. File Size [ %.0f ]\n",pcom9104_r.file_size);
							if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
							{
								infLOG(ERROR, "�ʷα� �뷮 ���� �� ������ �߻��Ͽ����Ϥ���.[com9104]\n");
							}

							com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
							return 0;
						}
						infLOG(ERROR, "�ʷα� �뷮�� ���� �մϴ�. File Size [ %.0f ]\n",pcom9104_r.file_size);
						if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
						{
							infLOG(ERROR, "�ʷα� �뷮 ���� �� ������ �߻��Ͽ����Ϥ���.[com9104]\n");
						}


						com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
						return 1;
					}
				}


				infLOG(ALWAY, "RS_EROL ����\n");

				memset(&headers,0x00,HEADER_SIZE);

				headers.nCmd = RS_EOL; //���� ���� ��û
				headers.nDataCnt = 0;
				headers.nDataSize = 0;

				if(	SendData(Socket,(char*)&headers,HEADER_SIZE )<0)  //struct _PACKET == PACKET
				{
					infLOG(ERROR, "�ʷα� �뷮�� ���� �մϴ�. File Size [ %.0f ]\n",pcom9104_r.file_size);
					if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
					{
						infLOG(ERROR, "�ʷα� �뷮 ���� �� ������ �߻��Ͽ����Ϥ���.[com9104]\n");
					}

					com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
					return 0;
				}
				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
				return 0;
			}
			else
			{
				infLOG(ERROR,"���� �Ϸ� �� �ش� ���� ��� ���� [ %d ]���ɾ �����ϴ�.\n",headers.nCmd);

				memset(&pcom9104_r,0x00,sizeof(CCOM9104_R));

				pcom9104_r.proc_flag  =  4;   //1=wedisk, 2=mydisk, 3=mydata 	4=filog disk
				pcom9104_r.id         = pFileinfo->cfups4001.id;        // ������ID(T_CONTENTS_TEMP.id)
				pcom9104_r.file_size = pFileinfo->cfups4001.file_size;
				memcpy(pcom9104_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����

				infLOG(ERROR, "�ʷα� �뷮�� ���� �մϴ�. File Size [ %.0f ]\n",pcom9104_r.file_size);
				if(com9104(pcom9104_r, g_szDcmdIP, g_nDcmdPort) < 0)
				{
					infLOG(ERROR, "�ʷα� �뷮 ���� �� ������ �߻��Ͽ����Ϥ���.[com9104]\n");
				}


				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
				return 0;
			}

		}while( 1 );

		com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
	}
	else if( nCType == -2)  //�Ϸ翡 �Ѱ� - ���� ������
	{
		infLOG(ERROR,"���ι� �������� �Ϸ翡 �ΰǸ� ��� ���� �մϴ�.");
		pSendData = new char[sizeof(ERR_HEADER)];
		memset(pSendData,0x00,sizeof(ERR_HEADER));
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;

		strcat(errheader.errmsg,"���ι� �������� �Ϸ翡 �ΰǸ� ��� ���� �մϴ�.");

		memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

		pHeader->nCmd = RS_ERR;

		return -RS_FILE_DATA_TRANSFER;
	}
	else if( nCType == -3)  //���� ������ ����Ǿ���.
	{
		infLOG(ERROR,"���ε� ������ ���� �Ǿ����ϴ�.");

		pSendData = new char[sizeof(ERR_HEADER)];
		memset(pSendData,0x00,sizeof(ERR_HEADER));
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;

		strcat(errheader.errmsg,"���ε� ������ ���� �Ǿ����ϴ�.");

		memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

		pHeader->nCmd = RS_ERR;

		return -RS_FILE_DATA_TRANSFER;
	}
	else if( nCType == -90042 ) //���� ���� ��ȸ ����
	{
		infLOG(ERROR,"���� ���� ��ȸ �� ������ �߻��Ͽ����ϴ�.��� �� ��õ� ���ֽʽÿ�.");
		pSendData = new char[sizeof(ERR_HEADER)];
		memset(pSendData,0x00,sizeof(ERR_HEADER));
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;

		strcat(errheader.errmsg,"���� ���� ��ȸ �� ������ �߻��Ͽ����ϴ�.��� �� ��õ� ���ֽʽÿ�.");

		memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

		pHeader->nCmd = RS_ERR;


		return -RS_FILE_DATA_TRANSFER;
	}
	else if( nCType == -1 )
	{
		infLOG(ERROR,"����� ���� ������ ã�� �� �����ϴ�.");

		pSendData = new char[sizeof(ERR_HEADER)];
		memset(pSendData,0x00,sizeof(ERR_HEADER));
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;

		strcat(errheader.errmsg,"����� ���� ������ ã�� �� �����ϴ�.");

		memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

		pHeader->nCmd = RS_ERR;


		return -RS_FILE_DATA_TRANSFER;
	}
	else if( nCType == 1 ) //����ũ
	{

		infLOG(ALWAY,"����ũ ��� ����.");
		//9001 ȣ�� // ����ڼ� ����
		//9101 ȣ�� //����ڼ� ����

		CCOM9001_R com9001_r ;
		memset(&com9001_r,0x00,sizeof(CCOM9001_R));

		multimap<int,USERINFO>::iterator mi; //IP ��ȸ
		//mi = m_UserList.begin();
		mi = m_UserList.find(Socket);
		if(mi != m_UserList.end())
		{
			strcpy(com9001_r.conn_ip ,mi->second.thread.userIP);
		}

		strcpy(com9001_r.cont_gu ,"WE");
		strcpy(com9001_r.server_id , pFileinfo->szServerID);
		com9001_r.temp_id =  pFileinfo->cfups4001.id;
		memcpy(com9001_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
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

		com9001 ( com9001_r, g_szDcmdIP, g_nDcmdPort);

		infLOG(ALWAY, " CCOM9101_R Setting ...   ]\n");

		CCOM9101_R com9101_r ;
		memset(&com9101_r,0x00,sizeof(CCOM9101_R));
		strcpy(com9101_r.conn_ip , com9001_r.conn_ip);
		strcpy(com9101_r.server_id , com9001_r.server_id);
		com9101_r.temp_id =  com9001_r.temp_id;
		strcpy(com9101_r.user_id ,com9001_r.user_id); // �����
		com9101_r.upload_size = com9001_r.upload_size;



		char szFullPath[768];
		memset(szFullPath,0x00,sizeof(szFullPath));

		char szFullName[768];
		memset(szFullName,0x00,sizeof(szFullName));

		int stat = -1;                 // ���� ���� ����
		bool bFOpenAppendMode = false; // ���� append ��� ����

		CCOM9104_R pcom9104_r; // �޴� ���� ��ҽ� DB ������ ( T_CONTENTS_TEMP ���� )

		FILEINFO rFileInfo;

		double dTotalRecvLen = 0; //�� ���� ����
		double dTotalLen = 0; // down�� ������ �� ����
		int nWriteLen=0;      // ���Ͽ� write �� ũ��
		int nRecvLen=0;       // �������� recv �� ũ��
		int nCheckStop = 0; //while ���� ����

		CCOM9105_R com9105_r;		// temp �� ���� ���� ����.
		memset(&com9105_r,0x00,sizeof(CCOM9105_R));

		if(pFileinfo->nType == FT_FOLDER)
		{
			infLOG(ALWAY,"���� ���ε� �Դϴ�.\n");
			//9105 ����
			memset(&com9105_r,0x00,sizeof(CCOM9105_R));

			com9105_r.id = pFileinfo->cfups4001.id;
			memcpy(&com9105_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
			strcpy(com9105_r.server_id ,pFileinfo->szServerID);
			strcpy(com9105_r.sfile_path ,pFileinfo->cfups4001.file_path);
			strcpy(com9105_r.sfile_name ,pFileinfo->cfups4001.file_name1);

			com9105(com9105_r, g_szDcmdIP, g_nDcmdPort);
		}

		do
		{
			bGhostMode = false;

			nCheckStop++;
			if(nCheckStop >= 1100)
			{
				infLOG(ERROR, "���α� ������ �ʰ� �Ͽ����ϴ�.\ntemp_id [ %lu ]file count = %d \n",pFileinfo->cfups4001.id,nCheckStop );

				//�޴� ���� ����

				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
				return 0;

			}

			infLOG(ALWAY,"�̾� �ø��� Flag[ %d ] >> [ 1 , 2 �� ��õ� 0 �� �Ϲ� ] \n",pFileinfo->nReUploadFlag);

		    headers.nCmd  = RS_FILE_DATA_SIGN_CHECK; // ���� ���� �޼���


			if(pFileinfo->nReUploadFlag == RECONNECT_UPLOAD || pFileinfo->nReUploadFlag == RE_UPLOAD)
			{

				if( pFileinfo->nType == FT_FOLDER)
				{
					strcpy(szFullPath, pFileinfo->cfups4001.file_path); //./2004/02/18/16/raid
					strcat(szFullPath,"/");

					memset(szFolderFullPath,0x00,sizeof(szFolderFullPath));
					strcpy(szFolderFullPath, szFullPath); //./2004/02/18/16/raid


					strcat(szFullPath,pFileinfo->cfups4001.file_name1);//<- ��� �� �߸� ����

					//////////////////////////////////////////////////////////////////////////

					strcpy(szFullName,pFileinfo->szDownPath); //szDownPath �� ./raid
	    			strcat(szFullName,"/");
	    			strcat(szFullName,pFileinfo->szFileName); //szfilename �� a.txt

	    			infLOG(ALWAY, "���� �̾� �ø��� - ��ġ [ %s ] ���� ��ġ [ %s ]\n",szFullPath,szFullName);

				}
				else
				{
			    	strcpy(szFullName,pFileinfo->szDownPath);
					strcat(szFullName,"/"); //./2004/02/18/16/raid/   <-- '/' �߰�
					strcat(szFullName,pFileinfo->szFileName);

					strcpy(szFullPath, pFileinfo->cfups4001.file_path); //./2004/02/18/16/raid
					strcat(szFullPath,"/");
					strcat(szFullPath,pFileinfo->cfups4001.file_name1);//

					infLOG(ALWAY, "���� �̾� �ø��� - ��ġ [ %s ] ���� ��ġ [ %s ]\n",szFullPath,szFullName);


					//9105 ����
					memset(&com9105_r,0x00,sizeof(CCOM9105_R));

					com9105_r.id = pFileinfo->cfups4001.id;
					memcpy(&com9105_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
					strcpy(com9105_r.server_id ,pFileinfo->szServerID);
					strcpy(com9105_r.sfile_path ,pFileinfo->szDownPath);
					strcpy(com9105_r.sfile_name ,pFileinfo->szFileName);

					com9105(com9105_r, g_szDcmdIP, g_nDcmdPort);

				}



				stat = stat64(szFullName,&statbuf);
				if(stat != 0) //������ ������ ���� �����.
				{
					MakeFolder(pFileinfo->szDownPath) ;
					infLOG(ALWAY,"������ �������� ������ �����մϴ�. [ %s ] \n",pFileinfo->szDownPath);
				}
				else
				{
					infLOG(ALWAY,"������ �̹� ���� �մϴ�. Append ���� ������ �����մϴ�. [ %s ] \n",szFullName);
					bFOpenAppendMode = true;
				}
			}
			else
			{
				infLOG(ALWAY,"�Ϲ� ���ε� ��� �Դϴ�.\n" );

		    	srand((unsigned int)time(NULL))	; //random �̸��� ���� �õ� ����


				///// ��¥ �ð� ���� ////
				time_t			curtime;
				struct tm		*stm;
				time( &curtime );
				stm = (struct tm *) localtime(&curtime);

				localtime_r(&curtime, stm);
				bool bResult = false;


		  		if( pFileinfo->nType == FT_FILE)
		  		{
		  			infLOG(ALWAY,"���� ���ε� �Դϴ�.\n");

		  			infLOG(ALWAY,"���� Root Path �� [ %s ] �Դϴ�.\n",pFileinfo->szDownPath);

					sprintf(pFileinfo->szDownPath,"%s/%04d/%02d/%02d/%02d",  pFileinfo->szDownPath
										,  stm->tm_year+1900
										,  stm->tm_mon + 1
										,  stm->tm_mday
										,  stm->tm_hour);//./2004/02/18/16


					infLOG(ALWAY,"���� Root Path �� �����մϴ�. [ %s ]\n",pFileinfo->szDownPath);

					memset(szFullName,0x00,sizeof(szFullName));

			    	strcpy(szFullName,pFileinfo->szDownPath);
					strcat(szFullName,"/"); //./2004/02/18/16/raid/   <-- '/' �߰�

			    	//file name ���

			    	char szFilename[50];
			    	char szFileType[10];
			    	memset(szFilename,0x00,sizeof(szFilename));
			    	memset(szFileType,0x00,sizeof(szFileType));


					sprintf(szFilename,"temp%lu",pFileinfo->cfups4001.id);
			    	//local �����̸����� ���� Ȯ���� ���.
			    	int nLen = GetReverseIndex(pFileinfo->cfups4001.file_name2 , '.');
					//	nLen = nLen - 1; // a.txt -> for .txt �ϱ� ���� nLen -1 ����
					//	nLen = nLen - 1; // ./raid/ -> ,./raid   , '/' delete
					infLOG(ALWAY, "���� �̸� �˻� [ %s ] \n",pFileinfo->cfups4001.file_name2);

					if(nLen < 0)
					{
						infLOG(ALWAY, "���� �̸��� Ȯ���ڰ� �����ϴ�. [ ���� ]\n");
					}
					else
					{
					    GetRightString(pFileinfo->cfups4001.file_name2,strlen(pFileinfo->cfups4001.file_name2)-nLen,szFileType);
					    infLOG(ALWAY, "���� Ȯ���� �˻� [ %s ] \n",szFileType);
					}
						//GetRightString(pFileinfo->szFileName,strlen(pFileinfo->szFileName)-nLen,szFileType);


					strcpy(pFileinfo->cfups4001.file_name2,pFileinfo->szFileName);
					memset(pFileinfo->szFileName,0x00,sizeof(pFileinfo->szFileName));
					strcpy(pFileinfo->szFileName,szFilename);
					strcat(pFileinfo->szFileName,szFileType);
					strcat(szFullName,szFilename);
					strcat(szFullName,szFileType);

					//// �̸� ���� ////
					memcpy(pFileinfo->cfups4001.file_name1,pFileinfo->szFileName,sizeof(pFileinfo->szFileName));
					strcpy(pFileinfo->cfups4001.file_path,pFileinfo->szDownPath);//,sizeof(pFileinfo->szDownPath));




					stat = stat64(szFullName,&statbuf);


					if(stat != 0) //������ ������ ���� �����.
					{

						MakeFolder(pFileinfo->szDownPath) ;
						infLOG(ALWAY,"������ �������� ������ �����մϴ�. [ %s ] \n",pFileinfo->szDownPath);
					}
					else
					{
						infLOG(ALWAY,"������ �̹� ���� �մϴ�. Append ���� ������ �����մϴ�. [ %s ] \n",szFullName);
						bFOpenAppendMode = true;
					}




					//9105 ����
					memset(&com9105_r,0x00,sizeof(CCOM9105_R));

					com9105_r.id = pFileinfo->cfups4001.id;
					memcpy(&com9105_r.user_id ,pHeader->szUserID,sizeof(pHeader->szUserID)); // �����
					strcpy(com9105_r.server_id ,pFileinfo->szServerID);
					strcpy(com9105_r.sfile_path ,pFileinfo->cfups4001.file_path);
					strcpy(com9105_r.sfile_name ,pFileinfo->cfups4001.file_name1);

					com9105(com9105_r, g_szDcmdIP, g_nDcmdPort);

				}
				else if(pFileinfo->nType == FT_FOLDER)//���� ���� ������ ���� �ϰ��
				{

					infLOG(ALWAY,"���� ���ε� �Դϴ�.\n");

					strcpy(szFullPath, pFileinfo->cfups4001.file_path); //./2004/02/18/16/raid
					strcat(szFullPath,"/");

					memset(szFolderFullPath,0x00,sizeof(szFolderFullPath));
					strcpy(szFolderFullPath, szFullPath); //./2004/02/18/16/raid

					strcat(szFullPath,pFileinfo->cfups4001.file_name1);//

					//////////////////////////////////////////////////////////////////////////

					strcpy(szFullName,pFileinfo->szDownPath); //szDownPath �� ./raid
	    			strcat(szFullName,"/");
	    			strcat(szFullName,pFileinfo->szFileName); //szfilename �� a.txt



					stat = stat64(szFullName,&statbuf);

	    			#ifdef __DEBUG
					printf(" ] FOLDER full path ( %s ) full name ( %s ) (%d)\n",szFullPath,szFullName,stat);
					#endif

					if(stat != 0) //������ ������ ���� �����.
					{
						MakeFolder(pFileinfo->szDownPath) ;
						infLOG(ALWAY,"������ �������� ������ �����մϴ�. [ %s ] \n",pFileinfo->szDownPath);
					}
					else
					{
						infLOG(ALWAY,"������ �̹� ���� �մϴ�. Append ���� ������ �����մϴ�. [ %s ] \n",szFullName);
						bFOpenAppendMode = true;
					}


				}
			}
	//		}while(bCreateFile != true) // ���� �̸��� ������ roof������..

			headers.nCmd = RS_FILE_DATA_SIGN_CHECK; //���� ����
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


			//server file ���� ������



		    //// �����ϱ����� �޼����� �˸�...
		    if(	nSRet <=0 )  //struct _PACKET == PACKET
			{
				infLOG(ERROR, "RS_FILE_DATA_SIGN_CHECK ���� ����.\n");

				// �޴� ���� ����
				///////////////////////////////////////////////
				// temp ����									 //
				///////////////////////////////////////////////


				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
				return 0;
			}


			//	HEADER recvHeader;

			// �̺κ� Ȯ�� �ϱ� .......................

			memset(&headers,0x00,sizeof(HEADER));

			if(	RecvData(Socket,(char*)&headers,sizeof(struct _HEADER))<=0)  //struct _PACKET == PACKET
			{
				infLOG(ERROR, "RS_FILE_DATA_SIGN_CHECK ��� �ޱ� ����\n");

				// �޴� ���� ����
				///////////////////////////////////////////////
				// temp ����									 //
				///////////////////////////////////////////////


				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
				return 0;
			}

			if(headers.nCmd == RS_EOL)
			{
				infLOG(ALWAY, "RS_FILE_DATA_SIGN_CHECK ��� �ޱ� - RS_EROL \n");


				pSendData = new char[sizeof(HEADER)];
				memset(pSendData,0x00,sizeof(HEADER));

				headers.nCmd = RS_EOL;
				headers.nDataCnt = 0;
				headers.nDataSize = 0;
				headers.nErrorCode = 0;

				memcpy(pSendData, &headers, sizeof(HEADER));

				// �޴� ���� ����
				///////////////////////////////////////////////
				// temp ����									 //
				///////////////////////////////////////////////



				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
				return END;
			}
			else if(headers.nCmd == RS_OK)
			{
				infLOG(ALWAY, "RS_FILE_DATA_SIGN_CHECK ��� �ޱ� - RS_OK \n");
			}

			//2009/06/13 �·�ī ���� �ޱ�.
			int nMurekaCnt = headers.nDataCnt;
			infLOG(ALWAY, "���͸� ��� ���� Ȯ�� - ���� [ %d ] \n",nMurekaCnt);

			LPMUREKA_VINFO pMurekaVInfo = NULL;
			if(nMurekaCnt > 0)
			{
				pMurekaVInfo = new MUREKA_VINFO[nMurekaCnt];



				if(	RecvData(Socket,(char*)pMurekaVInfo,sizeof(MUREKA_VINFO)*nMurekaCnt)<=0)  //struct _PACKET == PACKET
				{
					infLOG(ERROR,"�ʷα� �·�ī ��� �ޱ� ���� size : (%d) nMurekaCnt : (%d) \n", sizeof(MUREKA_VINFO)*nMurekaCnt, nMurekaCnt);
					// �޴� ���� ����
					///////////////////////////////////////////////
					// temp ����									 //
					///////////////////////////////////////////////


					com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
					return 0;
				}

				#ifdef __DEBUG
				for(int i=0; i < nMurekaCnt; i++)
				{
					printf("�·�ī ���� Ȯ��(%d).\n", i);
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

			////////////////////�⺻ ���� �Ϸ�////////////////////////////////////////////////

			//CMD5 md5;

			//char* pResult = md5.GetHashFromFile(szFullName,pFileinfo->dFileSize);
			//strcpy(Fups4005.szHashCode,pResult);

			//4005�� �ؽ��� �ֱ�

			if( strcmp(pFileinfo->szCopyright_yn ,"P") == 0 )
			{
				infLOG(ALWAY,"���۱� flag �缺�� : P -> N \n");
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
			
			
			//2009/06/14 �·�ī ��ȸ ����.
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

			//���۱� ���� �����
			infLOG(ALWAY,"���� Ȯ�� 1 : tpye [ %d ] == [ %d ] : sect_code [ %s ] : copyright [ %s ] \n", pFileinfo->nType , FT_FOLDER , pFileinfo->cfups4001.sect_code , pFileinfo->szCopyright_yn);


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

			infLOG(ALWAY,"fups4005 �����͸� ������Ʈ ���Դϴ�.\n"	);
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

			//2009/06/14 �·�ī ��ȸ ����.
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
					nCopyRight = fups4005hash(Fups4005, pMurekaVInfo,(CFUPS4005_1M_HASH)Fups1MHash);	//���۱� ��ȸ
				}
				else
					nCopyRight = fups4005(Fups4005, pMurekaVInfo);	//���۱� ��ȸ
			}
			infLOG(ALWAY,"���۱� ��ȸ ��� [ %d ] \n\n\n",nCopyRight);
			if( nCopyRight <= 0 )
			{
				//20190124 1m hash
				if( strlen(Fups4006_1MHash.hash_1m) > 0 || strlen(Fups4006_1MHash.hash_1m_mureka) > 0 )
				{
					nCompany = fups4006hash(Fups4006, pMurekaVInfo,Fups4006_1MHash);	//���۱� ��ȸ
				}
				else
					nCompany = fups4006(Fups4006, pMurekaVInfo);	//���۱ǿ� �ɸ����ʴ� �ڷ��� �������������� ��ȸ.
			}
			infLOG(ALWAY,"���� ��ȸ ���   [ %d ] \n\n\n",nCompany  );

			if(pMurekaVInfo)
			{
				delete[] pMurekaVInfo;
				pMurekaVInfo = NULL;
			}


			infLOG(ALWAY,"nCopyRight [ %d ] \n",nCopyRight);
			infLOG(ALWAY,"nCompany   [ %d ] \n",nCompany  );
			// 20140523 : ���� ó���ϱ�
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
			if( strcmp(pFileinfo->cfups4001.sect_code ,"07") == 0 ) //�����ϰ�� ������ N�� ����
				strcpy( pFileinfo->cfups4001.copyright_yn , "N");
*/

			infLOG(ALWAY,"�ڷ�� Ȯ�� : sect_code [ %s ] : copyright_yn [ %s ] \n" , pFileinfo->cfups4001.sect_code , pFileinfo->cfups4001.copyright_yn);
			if( nCopyRight < 0 )
			{
				infLOG(ERROR, "���۱� ��ȸ �����Դϴ�. Error Num [ %d ]\n",nCopyRight);

				pSendData = new char[sizeof(ERR_HEADER)];
				memset(pSendData,0x00,sizeof(ERR_HEADER));
				errheader.header.nCmd = RS_ERR;
				errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
				strcat(errheader.errmsg,"���� ���� ��� ���� �Դϴ�.");

				memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

				pHeader->nCmd = RS_ERR;

				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
				return -RS_FILE_DATA_TRANSFER;
			}



			// ���� ���� �� ����
			FILE* DownloadFile; //���� ������
			DownloadFile = NULL;
			//// ���� open���� ����////
			if( bFOpenAppendMode) //append mode
			{
				
				DownloadFile = fopen64(szFullName,"ar+tb");
				infLOG(ALWAY, "���� ���� : append mode ( %s )\n",szFullName);
				
			}
			else
			{
				DownloadFile = fopen64(szFullName,"wr+tb");
				infLOG(ALWAY, "���� ���� : write mode ( %s )\n",szFullName);
			
			}


			if(  DownloadFile == NULL) //������ ���� ������
			{
				infLOG(ERROR, "���� ���� ���� �Դϴ�. [ %s ] error num [ %d ] msg [ %s ] \n",szFullName,errno, szErrMsg);


				pSendData = new char[sizeof(ERR_HEADER)];
				memset(pSendData,0x00,sizeof(ERR_HEADER));
				errheader.header.nCmd = RS_ERR;
				errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
				strcat(errheader.errmsg,"�������� ���� ����� ���� �Ͽ����ϴ�.");

				memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

				pHeader->nCmd = RS_ERR;

				// �޴� ���� ����
				///////////////////////////////////////////////
				// temp ����									 //
				///////////////////////////////////////////////

				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
				return -RS_FILE_DATA_TRANSFER;
			}

			//// �̾� �ޱ⸦ ���� ���� �ش� ����ü ���� ////

			if( !bGhostMode )
			{
				infLOG(ALWAY,"������ Seek �� ������ ��ġ�� �̵� �մϴ�.\n");

				if(fseeko64(DownloadFile,0l,SEEK_END) < 0)
				{
					infLOG(ALWAY,"������ Seek �� ������ ��ġ�� �̵� �� ���� �߻��Ͽ����ϴ�. errno [ %d ] error msg [ %s ]\n",errno,strerror(errno));
					pSendData = new char[sizeof(ERR_HEADER)];
					memset(pSendData,0x00,sizeof(ERR_HEADER));
					errheader.header.nCmd = RS_ERR;
					errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;
					strcat(errheader.errmsg,"���� ������ ���� �̵� ���� �Ͽ����ϴ�.");

					memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

					pHeader->nCmd = RS_ERR;
					// �޴� ���� ����
					///////////////////////////////////////////////
					// temp ����									 //
					///////////////////////////////////////////////
					com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
					return -RS_FILE_DATA_TRANSFER;
				}
			}

			LPFILEHEAD pFileHead = new FILEHEAD;
			memset(pFileHead,0x00,sizeof(FILEHEAD));

			//double dCurrentLen = (double)ftello64 (DownloadFile); // ������ ������ �ִ��� ����

			double dCurrentLen	= 0;

	//				dCurrentLen = (double)statbuf.st_size;
			if( !bGhostMode )
			{
				dCurrentLen = (double)ftello64 (DownloadFile); // ������ ������ �ִ��� ����
				infLOG(ALWAY, "�ֱ� �̵��� ���� ������ ( %.0f )\n",dCurrentLen);

			}

			if(dCurrentLen < 0)
				dCurrentLen = 0;

			pFileHead->dCurrentSize = dCurrentLen; //�ص忡 �� ���� ���� ����

			////////////////////////////////////////////////
			//ó�� �뷮 ������Ʈ : ���н� ���� �ؾ� ��.

			// head �ۼ�
			memset(&headers,0x00,sizeof(HEADER));

			headers.nCmd = RS_FILE_DATA_TRANSFER ; // ������ ����
			headers.nDataCnt = 1;
			headers.nDataSize = sizeof(FILEHEAD);
			headers.nErrorCode = 0;

			pSendData = new char[sizeof(HEADER) + headers.nDataCnt*headers.nDataSize];

			memcpy(pSendData,&headers,sizeof(HEADER));

			memcpy(pSendData + HEADER_SIZE,pFileHead, headers.nDataCnt*headers.nDataSize);

			infLOG(ALWAY,"Send RS_FILE_DATA_TRANSFER\n");
			//// body �ۼ�////
			if(	SendData(Socket,pSendData,HEADER_SIZE + headers.nDataCnt*headers.nDataSize)<0)  //struct _PACKET == PACKET
			{
				infLOG(ERROR,"Send RS_FILE_DATA_TRANSFER ERROR\n");
				delete pFileHead;
				// �뷮 ���� �ϱ�
				// �޴� ���� ����
				///////////////////////////////////////////////
				// temp ����									 //
				///////////////////////////////////////////////

				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
				return 0;
			}
			delete[] pSendData;
			pSendData = NULL;

			delete pFileHead;

			infLOG(ALWAY,"Send RS_FILE_DATA_TRANSFER OK \n");

		 ///////////////////////// ������ ���� //////////////////////////////////

			dTotalRecvLen = 0; //�� ���� ����
			dTotalLen = pFileinfo->dFileSize - dCurrentLen; // down�� ������ �� ����
			nWriteLen=0;
			nRecvLen=0;

			char* szRecvBuffer = new char[RECVBUF]; //recv buffer


			// ���۱� ����

			// �����̸鼭 ���۱� ������ �ɸ��� �����͸� �ް� ������ ������ �������� �ʴ´�.
			// ���������� �ϱ� ���ؼ� ��Ʈ��ũ���� �����ͱ��� 	�޾��ش�. �̺κ� �ٲܷ��� ����� ��⿡�� �����°� ó�� ���̰� �Ͽ��� �Ѵ�.
			infLOG(ALWAY,"���� Ȯ�� [ %s ] : ���� ���� ��ü ���� [ %.0f ] = [ %.0f (��ü) - %.0f(�ֱ��̵���) ] \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize ,dCurrentLen);


			/*
			if( bGhostMode )
			{
				infLOG(ALWAY,"���� ���ε� ��� \n");
				//int fno = fileno(DownloadFile);

				while(dTotalLen > 0  )
				{
					memset(szRecvBuffer,0x00,RECVBUF);
					///// ���Ϲޱ� /////

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
		        			printf(" ] Write File End (%s) : ���� ��ü ���� (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
		        			#endif
		        			infLOG(ALWAY," ] Write File End (%s) : ���� ��ü ���� (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
		        		}
		        		else
		        		{
		        			#ifdef __DEBUG
		        			printf(" ] Write File ERROR (%s) : ���� ��ü ���� (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
		        			#endif
		        			infLOG(ERROR," ] Write File ERROR (%s) : ���� ��ü ���� (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
		        			nRecvLen = -1;
		        		}
		        	}

			        if(nRecvLen <= 0 && dTotalLen != 0)	//�޴ٰ� ������ ������...DBó��
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

			        		infLOG(ERROR, " ] RecvSocket Error ( ������ �������ϴ�. ) (%s)\n",szErrMsg);

							#ifdef __DEBUG
							printf(" ] RecvSocket Error ( ������ �������ϴ�. ) (%s)\n",szErrMsg);
							#endif
			        	}

						infLOG(ERROR," ] WE ��ũ ��� (%s) RecvLen (%d) (%15.0f) TotalLen(%15.0f)\n ",pFileinfo->cfups4001.file_name2,nRecvLen ,dTotalRecvLen,dTotalLen);

						if(DownloadFile)
						{
							fclose(DownloadFile);
							DownloadFile == NULL ;
						}

					   	if(szRecvBuffer)
							delete[] szRecvBuffer;

						infLOG(ERROR," ] WE ��ũ ��� (%s) RecvLen (%d) (%15.0f) TotalLen(%15.0f)\n ", pFileinfo->cfups4001.file_name2,nRecvLen ,dTotalRecvLen,dTotalLen);

						com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
						return 0;
					//	return END;
		        	}

	        		dTotalLen = dTotalLen - (double)nRecvLen;  //�ѱ��̿���  ���� ���� ����
		        	dTotalRecvLen = dTotalRecvLen + (double)nRecvLen; //���� ���� ��ŭ ����
				}
			}
			else
			*/
			{
				nTotalRecvFileCnt++;
				infLOG(ALWAY,"���ε� ���� [ %d ] \n",nTotalRecvFileCnt);
				int fno = fileno(DownloadFile);

				while(dTotalLen > 0  )
				{
					memset(szRecvBuffer,0x00,RECVBUF);
					///// ���Ϲޱ� /////

					nRecvLen =  RecvFileData(Socket, szRecvBuffer, RECVBUF, dTotalLen) ;

				    if(nRecvLen > 0)
				    {
				    	nWriteLen = write(fno ,szRecvBuffer,nRecvLen);
				      	//nWriteLen = fwrite(szRecvBuffer,nRecvLen,1,DownloadFile);
				    }
				    else
				    	nWriteLen = 0;

				    //fwrite(szRecvBuffer,1,nRecvLen,DownloadFile); //���� ���� ��ŭ file�� ����

				    if(nWriteLen <= 0)
			        {
			        	if(nWriteLen == 0)
			        	{
			        		#ifdef __DEBUG
			        		printf(" ] Write File End (%s) : ���� ��ü ���� (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
			        		#endif
			        		infLOG(ALWAY,"Write File End (%s) : ���� ��ü ���� (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
			        	}
			        	else
			        	{
			        		#ifdef __DEBUG
			        		printf(" ] Write File ERROR (%s) : ���� ��ü ���� (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
			        		#endif
			        		infLOG(ERROR," ] Write File ERROR (%s) : ���� ��ü ���� (%15.0f ) =  %15.0f \n",pFileinfo->cfups4001.file_name2 , dTotalLen ,pFileinfo->dFileSize );
			        		nRecvLen = -1;
			        	}
			        }

				    if(nRecvLen <= 0 && dTotalLen != 0)	//�޴ٰ� ������ ������...DBó��
				    {

						if(nRecvLen < 0)
				       	{
							memset(szErrMsg,0x00,sizeof(szErrMsg));
							GetErrMsg(-nRecvLen,szErrMsg);
							infLOG(ERROR, "�����͸� ���� �� �����ϴ�. ( %d )( %s )\n",nRecvLen,szErrMsg);

				       	}
				       	else if(nRecvLen == 0)
				       	{
							memset(szErrMsg,0x00,sizeof(szErrMsg));
							GetErrMsg(-nRecvLen,szErrMsg);
							infLOG(ERROR, "������ ���� �����ϴ�.[ �̰��� ���� Ŭ���̾�Ʈ���� �����͸� ����� ������ ���Ҷ� �߻��մϴ�. ] \n" );
				       	}


						infLOG(ERROR,"����ũ ��� (%s) RecvLen (%d) (%15.0f) TotalLen(%15.0f)\n ",pFileinfo->cfups4001.file_name2,nRecvLen ,dTotalRecvLen,dTotalLen);
						infLOG(ERROR,"errno [ %d ] error msg [ %s ]\n",errno,strerror(errno));


						if(DownloadFile)
						{
							fclose(DownloadFile);
							DownloadFile == NULL ;
						}

							// �޴� ���� ����
					///////////////////////////////////////////////
					// temp ����								 //
					///////////////////////////////////////////////

					   	if(szRecvBuffer)
							delete[] szRecvBuffer;

						com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
						return 0;
					//	return END;
			        }
	        		dTotalLen = dTotalLen - (double)nRecvLen;  //�ѱ��̿���  ���� ���� ����
		        	dTotalRecvLen = dTotalRecvLen + (double)nRecvLen; //���� ���� ��ŭ ����
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
			//���� �̸� �ٲٱ�
			// DB �ֱ�..
/******************************�ؽ��� ���� ����***********************************************/

			infLOG(ALWAY,"����ũ ������ �ޱ� �Ϸ� �� Ȯ�� - �����̸� (%s) �ӽù�ȣ (%lu) �ӽù�ȣ (%lu) ���� �� ���� ���� ( %d )\n",pFileinfo->cfups4001.file_name2, pFileinfo->nNumber,pFileinfo->cfups4001.id,nTotalRecvFileCnt);
			pFileinfo->cfups4001.down_cnt = nTotalRecvFileCnt;

			//���⼭ ���� �ޱ� ����

			infLOG(ALWAY,"���� �Ϸ� �� �ش� ���� ���.\n");
			memset(&headers,0x00,sizeof(HEADER));
			if(RecvData(Socket,(char*)&headers,HEADER_SIZE ) <= 0)
			{
				infLOG(ERROR,"���� �Ϸ� �� �ش� ���� ��� ����.\n");
				// �޴� ���� ����
				///////////////////////////////////////////////
				// temp ����									 //
				//////////////////////////////////////////////

				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);

				return 0;

			}
			infLOG(ERROR,"���� �Ϸ� �� �ش� ���� ��� ��� [ %d ].\n",headers.nCmd);
			if(headers.nCmd == RS_FILE_REQUEST_NEXT_FILE )
			{
				infLOG(ALWAY, "RS_FILE_REQUEST_NEXT_FILE\n���� ������ �޽��ϴ�.\n");

				memset(&headers,0x00,HEADER_SIZE);

				headers.nCmd = RS_FILE_REQUEST_NEXT_FILEINFO; //���� ���� ��û
				headers.nDataCnt = 0;
				headers.nDataSize = 0;

				if(	SendData(Socket,(char*)&headers,HEADER_SIZE )<0)  //struct _PACKET == PACKET
				{
					infLOG(ERROR, "RS_FILE_REQUEST_NEXT_FILE ���� ���� \n");


					// �޴� ���� ����
					///////////////////////////////////////////////
					// temp ����									 //
					///////////////////////////////////////////////

					com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);

					return 0;

				}
				infLOG(ERROR, "RS_FILE_REQUEST_NEXT_FILE ���� ��� �� \n");

				//recv file_transfer
				memset(&headers,0x00,HEADER_SIZE);
				if(RecvData(Socket,(char*)&headers,HEADER_SIZE ) <= 0)
				{
					infLOG(ERROR, "RS_FILE_REQUEST_NEXT_FILE ���� ��� �� ���� [ �̰��� ���� Ŭ���̾�Ʈ�κ��� �����͸� ���� ���� �� �߻��մϴ�. \n");


					// �޴� ���� ����
					///////////////////////////////////////////////
					// temp ����									 //
					///////////////////////////////////////////////

					com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
					return 0;

				}

				memset(&rFileInfo,0x00,sizeof(FILEINFO));
				infLOG(ERROR, "���� �������� �ޱ� ��� �� \n");

				if( RecvData(Socket,(char*)&rFileInfo,sizeof(FILEINFO) ) <= 0)
				{
					infLOG(ERROR, "���� �������� �ޱ� ��� �� ���� \n");


					// �޴� ���� ����
					///////////////////////////////////////////////
					// temp ����									 //
					///////////////////////////////////////////////

					com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
					return 0;
				}
				pFileinfo = &rFileInfo;
			}
			else if(headers.nCmd == RS_EOL)
			{
				infLOG(ALWAY,"RS_EOL\n����ũ�� �������� ����մϴ�. �ӽ� ��ȣ �˻�(T_CONTENTS_TEMP) [ %lu ]\n",pFileinfo->nNumber );


			// ����� eol ������
				if(pFileinfo->nTypeDisk == FT_WEDISK && pFileinfo->nNumber > 0 ) //������ ���
				{
					if( dTotalLen == 0)
					{
						infLOG(ALWAY,"����ũ ���� ��� - �ӽù�ȣ [ %lu ] �����̸�  [ %s ]  \n",pFileinfo->nNumber,pFileinfo->cfups4001.file_name2);

						//����� �Ϲ� ���ε� ����

						int nResult = fups4001(pFileinfo->cfups4001);
						infLOG(ALWAY,"����ũ ��ϰ��(fups4001) Result [ %d ] \n",nResult);

						if(  nResult < 0 )//pFileinfo->cfups4001) == -1)
						{
							// �޴� ���� ����
							///////////////////////////////////////////////
							// temp ����									 //
							///////////////////////////////////////////////

							//������ ����� ���� �߻� ...�� ���� �ؾ� �� ��ϵ�

							infLOG(ERROR, "================== ����ũ ��� ���� ===================\n"
										  "�ӽù�ȣ ( %lu )���� ���̵�( %s ) ���ϰ�� ( %s )                         \n"
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

							infLOG(ALWAY,"RS_FILE_END_FAIL ���� \n");

							if(	SendData(Socket,(char*)&headers,HEADER_SIZE)<0)  //struct _PACKET == PACKET
							{
								com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
								return 0;
							}
							com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
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
						infLOG(ALWAY,"============== ����ũ ���� ��� �Ϸ� ===============\n");

					}
					else
					{
						infLOG(ERROR, "============ �ʷα� ��� ���� - ������ ������ ���� ���Ͽ����ϴ�. ========== \n");
						memset(&headers,0x00,sizeof(HEADER));

			/*			if(	RecvData(Socket,(char*)&headers,sizeof(struct _HEADER))<=0)  //struct _PACKET == PACKET
						{
							return 0;
						}
			*/
						// �޴� ���� ����

						#ifdef __DEBUG
						printf(" ] file recv cancel..2\n");
						#endif

						headers.nCmd = RS_FILE_END_FAIL;
						headers.nDataCnt = 0;
						headers.nDataSize = 0;
						headers.nErrorCode = 4001;
						infLOG(ALWAY,"RS_FILE_END_FAIL ���� \n");

						if(	SendData(Socket,(char*)&headers,HEADER_SIZE)<=0)  //struct _PACKET == PACKET
						{
							com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
							return 0;
						}
						com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
						return 1;
					}
				}

				infLOG(ALWAY, "RS_EROL ����\n");

				memset(&headers,0x00,HEADER_SIZE);

				headers.nCmd = RS_EOL; //���� ���� ��û
				headers.nDataCnt = 0;
				headers.nDataSize = 0;

				if(	SendData(Socket,(char*)&headers,HEADER_SIZE )<0)  //struct _PACKET == PACKET
				{
					com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
					return 0;
				}
				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
				return 0;
			}
			else
			{
				infLOG(ERROR,"���� �Ϸ� �� �ش� ���� ��� ���� [ %d ]���ɾ �����ϴ�.\n",headers.nCmd);

				// �޴� ���� ����
				///////////////////////////////////////////////
				// temp ����									 //
				///////////////////////////////////////////////
				com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
				return 0;
			}
		}while( 1 );

		com9101 ( com9101_r, g_szDcmdIP, g_nDcmdPort);
	}
	else
	{
		infLOG(ERROR," com9004 ���� - nCType ( %d ) �� �����ϴ�. \n" ,nCType );
		pSendData = new char[sizeof(ERR_HEADER)];
		memset(pSendData,0x00,sizeof(ERR_HEADER));
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_FILE_DATA_TRANSFER;

		strcat(errheader.errmsg,"���ε� �����Դϴ�. �߸��� ���� �����Դϴ�.");

		memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

		pHeader->nCmd = RS_ERR;

		return -RS_FILE_DATA_TRANSFER;
	}
	return 0;
}





