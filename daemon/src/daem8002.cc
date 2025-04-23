/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem8002.cc
 *         기능 : DB정보없는 파일찾기(파일기준)
 *         설명 : 1. 서버의 파일을 기준으로 DB정보 찾아 없는경우 삭제처리한다.
 *                2. 파일삭제 처리하면서 빈폴더 삭제기능을 처리한다.
 *                3. T_CONTENTS_FILE_DEL_TEMP 생성일자 까지만 추적하여 삭제한다.
 *                4. 생성일자는 T_MINOR_CODE.major_code='100'
 *                              T_MINOR_CODE.minor_code='01'
 *                   에서 찾으며 데이타 형식은 YYYY/MM/DD
 *
 *     설치위치 : 각각 서버마다 위치한다.
 *
 *       작성자 : 
 *       작성일 : 
 *     수정이력 : 2015.02.24 파일 삭제하지 않고 로그 남기는걸로 변경.
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#include <mysql.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <spawn.h> //for posix_spawn()
#include <unistd.h>

#include "daemcom.h"
#include "commydb.h"

#define  MAX_ROWS	1
#define _DEBUG_

#define MY_SERVER 1
#define WE_SERVER 2

#define FT_FOLDER 3
#define FT_FILE 4

static long g_lTotalRecordCount;
static char szErrorMsg[256];

char szSystemQuery[1000];
int  gnTotalCnt;
int  gnSkipCnt;
int  gnDeleteCnt;
int  gnErrorCnt;

char szTempPath[256];

bool gbIsDel = true;

int  daem8002_init_process();
int  daem8002_main_process(int argc,char** argv);
int  daem8002_term_process();
void daem8002_signal(int nSignal);
int daem8002_db_select(char* pserver_id, char* pfile_path , char* pfile_name);
int daem8002_createdb_date(char *szStopDatePath);

int DeleteFileList_One(char* pServerID, char* path);
int DeleteFileList_Days(char* pServerID, char* path, char* pStopDatePath);

MYSQL     *con;
MYSQL_RES *res;
MYSQL_ROW  row;

	
void GetErrMsg(int nErrno)
{
	int nTempErrNo = nErrno;
	memset(szErrorMsg,0x00,sizeof(szErrorMsg));
	strcpy(szErrorMsg,strerror(nTempErrNo));	
}


int DeleteFileList(char* pServerID, char* path, char* pStopDatePath)
{
	ZzPRT(ERROR, ">> path=%s , pStopDatePath=%s \n",path,pStopDatePath);

	struct stat64 rootstatbuf;
	int nFileCount = 0;
	
	char szPath[256];
	memset(szPath,0x00,sizeof(szPath));
	strcpy(szPath, path);

	int stat = stat64(szPath,&rootstatbuf); 
	if(stat !=0 )
	{
		printf("Error : %s cant find folder\n",szPath);	
		return -1;	
	}
	
	struct dirent **direntp;
	int nCnt, ni;

    nCnt = scandir(szPath, &direntp, 0, alphasort);

	// 
	if(nCnt <= 0)
	{
		GetErrMsg(errno);
		printf("Error : cant root opendir folder %s , errMsg : %s\n",szPath,szErrorMsg);
		return -1;	
	}

	char szfullpath[612];
	char szrootfullpath[612];
	memset(szfullpath,0x00,sizeof(szfullpath));
	memset(szrootfullpath,0x00,sizeof(szrootfullpath));
	
	// 데이타 생성일자 전일까지만 추적하여 삭제한다.
	char szTemp[612];
	memset(szTemp, 0x00, sizeof(szTemp));
	ZzGetDelimitFullData(szPath,'/',szTemp,4);

	if (strcmp(szTemp, pStopDatePath) >= 0 )
	{
		ZzLOG(ALWAY, "-------------------->>>%s folder skip...\n", szTemp);
		return -2;		
	}
	

	for (ni = 0; ni < nCnt; ni++)
    {
        stat64(direntp[ni]->d_name,&rootstatbuf);

		memset(szrootfullpath,0x00,sizeof(szrootfullpath));
		strcpy(szrootfullpath,szPath);
		strcat(szrootfullpath,"/");		
		strcat(szrootfullpath, direntp[ni]->d_name);  

		stat = stat64(szrootfullpath,&rootstatbuf);
	

		if( (strcmp(".",direntp[ni]->d_name) !=0 ) && (strcmp("..",direntp[ni]->d_name) != 0) && (GetFistCharIndex(direntp[ni]->d_name,'.') != 0))
		{
			if(S_ISDIR (rootstatbuf.st_mode) ) //폴더 일때 DB 쿼리 해서 검사 해보기
			{

				if((strlen(szrootfullpath) >= 32) || (ZzCharCount(szrootfullpath, '/') == 7)) // 원하는 패스일경우
				{
					int nSubCnt, nj;
					struct dirent **Subdirentp;				
				    struct stat64 statbuf;
					nSubCnt = scandir(szrootfullpath, &Subdirentp, 0, alphasort);

					if(nSubCnt <= 0)
					{
						GetErrMsg(errno);
						printf("Error : cant root opendir folder %s , errMsg : %s\n",szPath,szErrorMsg);
						return -1;	
					}

					for (nj = 0; nj < nSubCnt; nj++)
				    {
						memset(szfullpath,0x00,sizeof(szfullpath));
						strcpy(szfullpath, szrootfullpath);
				
						stat = stat64(szfullpath,&statbuf); 
					 	if(stat == 0 && (strcmp(".",Subdirentp[nj]->d_name) !=0 ) && (strcmp("..",Subdirentp[nj]->d_name) != 0) && (GetFistCharIndex(Subdirentp[nj]->d_name,'.') != 0))
						{
							if((S_ISDIR (statbuf.st_mode)) || (S_ISREG (statbuf.st_mode)))
							{
								int nResult = 0;
								nResult = daem8002_db_select(pServerID,szfullpath,Subdirentp[nj]->d_name);
									
								if( nResult == 0)
								{
									memset(szSystemQuery, 0x00, sizeof(szSystemQuery));
									sprintf(szSystemQuery, "%s/%s", szfullpath, Subdirentp[nj]->d_name);
									char *argvv[] = {"/bin/rm", "-r", "-f", szSystemQuery};
									sprintf(szSystemQuery, "/bin/rm -rf %s/%s", szfullpath, Subdirentp[nj]->d_name);

									int ret = 0;
									
									// 파일 삭제
									if(gbIsDel == true)
									{
										usleep(500000); 
										// 로그 남김
										rmLOG(ALWAY, "%s\n", szSystemQuery );
									}

									if (ret == 0) {
										gnDeleteCnt++;
										gnTotalCnt++;
										ZzPRT(ALWAY, "-----> %s/%s DELETE OK\n", szfullpath, Subdirentp[nj]->d_name);
										ZzLOG(ALWAY, "-----> %s/%s DELETE OK\n", szfullpath, Subdirentp[nj]->d_name);
									}
									else
									{
										gnErrorCnt++;
										gnTotalCnt++;
										ZzPRT(ERROR, "-----> ret=%d %s/%s DELETE ERROR errno(%d)[%s]\n",ret, szfullpath, Subdirentp[nj]->d_name, errno, strerror(errno));
										ZzLOG(ERROR, "-----> %s/%s DELETE ERROR errno(%d)[%s]\n", szfullpath, Subdirentp[nj]->d_name, errno, strerror(errno));
									}
								}	
								else if( nResult > 0)
								{
									gnSkipCnt++;
									gnTotalCnt++;
									ZzPRT(ALWAY, "-----> %s/%s 사용중\n", szfullpath, Subdirentp[nj]->d_name);
									ZzLOG(ALWAY, "-----> %s/%s 사용중\n", szfullpath, Subdirentp[nj]->d_name);
								}
								else
								{
									gnDeleteCnt++;
									gnTotalCnt++;
									ZzPRT(ERROR, "-----> %s/%s DB Error\n", szfullpath, Subdirentp[nj]->d_name);
									ZzLOG(ERROR, "-----> %s/%s DB Error\n", szfullpath, Subdirentp[nj]->d_name);
								}	
							}
							else //그외 형태.
							{
								ZzPRT(ERROR, "-----> %s/%s unknown file type (see file owner and group)\n", szfullpath, Subdirentp[nj]->d_name);
								ZzLOG(ERROR, "-----> %s/%s unknown file type (see file owner and group)\n", szfullpath, Subdirentp[nj]->d_name);
								return -1;
							}
						}
						else //존재 하지 않는 폴더일꺼야.
						{
							GetErrMsg(errno);
						}
					}


					// 빈폴더 삭제하기
					char *argva[] = {"/bin/rmdir", szrootfullpath};
					
					if(gbIsDel == true)
					{
						//posix_spawn(NULL, "/bin/rmdir", NULL, NULL, argva, NULL);
						usleep(500000);
						
						// 로그 남김
						memset(szSystemQuery, 0x00, sizeof(szSystemQuery));
						sprintf(szSystemQuery, "/bin/rmdir %s", szrootfullpath);
						rmLOG(ALWAY, "/bin/rmdir %s\n", szrootfullpath );
					}
				}
				else
				{
					if( DeleteFileList(pServerID, szrootfullpath, pStopDatePath) == -2 )
					{
						return -1;
					}
				}
			}
			else
			{
				//printf("file\n");
			}
		}


    }

	// 빈폴더 삭제하기
	char *argvb[] = {"/bin/rmdir", szPath};
	
	if(gbIsDel == true)
	{
		//posix_spawn(NULL, "/bin/rmdir", NULL, NULL, argvb, NULL);
		usleep(500000);
		// 로그 남김
		memset(szSystemQuery, 0x00, sizeof(szSystemQuery));
		sprintf(szSystemQuery, "/bin/rmdir %s", szPath);
		rmLOG(ALWAY, "/bin/rmdir %s\n", szPath );	
	}
	
	return 0;
}





int DeleteFileList_Days(char* pServerID, char* path, char* pStopDatePath)
{
	ZzPRT(ERROR, ">> path=%s , pStopDatePath=%s \n",path,pStopDatePath);

//	struct stat64 statbuf;
	struct stat64 rootstatbuf;
	int nFileCount = 0;
	
	char szPath[256];
	memset(szPath,0x00,sizeof(szPath));
	strcpy(szPath, path);

	int stat = stat64(szPath,&rootstatbuf); 
	if(stat !=0 )
	{
		printf("Error : %s cant find folder\n",szPath);	
		return -1;	
	}
	
	struct dirent **direntp;
	int nCnt, ni;
//    struct stat64 statbuf;

    nCnt = scandir(szPath, &direntp, 0, alphasort);

	// 
	if(nCnt <= 0)
	{
		GetErrMsg(errno);
		printf("Error : cant root opendir folder %s , errMsg : %s\n",szPath,szErrorMsg);
		return -1;	
	}

	char szfullpath[612];
	char szrootfullpath[612];
	memset(szfullpath,0x00,sizeof(szfullpath));
	memset(szrootfullpath,0x00,sizeof(szrootfullpath));
	
	// 데이타 생성일자 전일까지만 추적하여 삭제한다.
	char szTemp[612];
	memset(szTemp, 0x00, sizeof(szTemp));
	ZzGetDelimitFullData(szPath,'/',szTemp,4);

	if (strcmp(szTemp, pStopDatePath) >= 0 )
	{
		ZzLOG(ALWAY, "-------------------->>>%s folder skip...\n", szTemp);
		ZzPRT(ALWAY, "Temp Path ( %s )\n", szTempPath);
		return -2;		
	}
	else
	{
		sprintf(szTempPath,"%s",path);		
	}
	

	for (ni = 0; ni < nCnt; ni++)
    {
        stat64(direntp[ni]->d_name,&rootstatbuf);

		memset(szrootfullpath,0x00,sizeof(szrootfullpath));
		strcpy(szrootfullpath,szPath);
		strcat(szrootfullpath,"/");		
		strcat(szrootfullpath, direntp[ni]->d_name);  

		stat = stat64(szrootfullpath,&rootstatbuf);
	

		if( (strcmp(".",direntp[ni]->d_name) !=0 ) && (strcmp("..",direntp[ni]->d_name) != 0) && (GetFistCharIndex(direntp[ni]->d_name,'.') != 0))
		{
			if(S_ISDIR (rootstatbuf.st_mode) ) //폴더 일때 DB 쿼리 해서 검사 해보기
			{

				if((strlen(szrootfullpath) >= 32) || (ZzCharCount(szrootfullpath, '/') == 7)) // 원하는 패스일경우
				{
								
				}
				else
				{
					if( DeleteFileList_Days(pServerID, szrootfullpath, pStopDatePath) == -2 )
					{
						return -1;
					}
				}
			}
			else
			{
				//printf("file\n");
			}
		}


    }


	return 0;
}


	
int DeleteFileList_One(char* pServerID, char* path)
{
	ZzPRT(ERROR, ">> path=%s \n",path);

	struct stat6464 statbuf;
	struct stat6464 rootstatbuf;
	DIR *root_pdir = NULL;
	DIR *pdir = NULL;
	int nFileCount = 0;
	
	char szPath[256];
	memset(szPath,0x00,sizeof(szPath));
	strcpy(szPath, path);

	int stat = stat64(szPath,&rootstatbuf); 
	if(stat !=0 )
	{
		printf("Error : %s cant find folder\n",szPath);	
		return -1;	
	}
	
	root_pdir = opendir(szPath);//strPath); 
	if(root_pdir == NULL)
	{
		GetErrMsg(errno);
		printf("Error : cant root opendir folder %s , errMsg : %s\n",szPath,szErrorMsg);
		return -1;	
	}

	struct dirent *root_pent;
	struct dirent *pent;
	
	char szfullpath[612];
	char szrootfullpath[612];
	memset(szfullpath,0x00,sizeof(szfullpath));
	memset(szrootfullpath,0x00,sizeof(szrootfullpath));
	
	// 데이타 생성일자 전일까지만 추적하여 삭제한다.
	char szTemp[612];
	memset(szTemp, 0x00, sizeof(szTemp));
	ZzGetDelimitFullData(szPath,'/',szTemp,4);

	while( (root_pent = readdir(root_pdir)) != NULL )
	{
		memset(szrootfullpath,0x00,sizeof(szrootfullpath));
		strcpy(szrootfullpath,szPath);
		strcat(szrootfullpath,"/");		
		strcat(szrootfullpath, root_pent->d_name);  

		stat = stat64(szrootfullpath,&rootstatbuf);
	


		if( (strcmp(".",root_pent->d_name) !=0 ) && (strcmp("..",root_pent->d_name) != 0) && (GetFistCharIndex(root_pent->d_name,'.') != 0))
		{
			if(S_ISDIR (rootstatbuf.st_mode) ) //폴더 일때 DB 쿼리 해서 검사 해보기
			{

				if((strlen(szrootfullpath) >= 32) || (ZzCharCount(szrootfullpath, '/') == 7)) // 원하는 패스일경우
				{
					pdir = opendir(szrootfullpath);//strPath); 
					if(pdir == NULL)
					{
						GetErrMsg(errno);
						printf("Error : cant sub opendir folder %s , errMsg : %s\n",path,szErrorMsg);
						closedir(root_pdir);
						return -1;	
					}
					
					//여기부터 이름 가져 오는 반복문
					while( (pent = readdir(pdir)) != NULL ) // /raid/fdata/2004/10/10/10 이후 부터 얻어낸다.
					{
						memset(szfullpath,0x00,sizeof(szfullpath));
						strcpy(szfullpath, szrootfullpath);
				
						stat = stat64(szfullpath,&statbuf); 
					 	if(stat == 0 && (strcmp(".",pent->d_name) !=0 ) && (strcmp("..",pent->d_name) != 0) && (GetFistCharIndex(pent->d_name,'.') != 0))
						{
							if((S_ISDIR (statbuf.st_mode)) || (S_ISREG (statbuf.st_mode)))
							{
								int nResult = 0;
								nResult = daem8002_db_select(pServerID,szfullpath,pent->d_name);
									
								if( nResult == 0)
								{
									memset(szSystemQuery, 0x00, sizeof(szSystemQuery));
									sprintf(szSystemQuery, "%s/%s", szfullpath, pent->d_name);
									char *argvv[] = {"/bin/rm", "-r", "-f", szSystemQuery};
									
									//sprintf(szSystemQuery, "/bin/rm -rf %s/%s", szfullpath, pent->d_name);

									int ret = 0;
									
									// 파일 삭제
									if(gbIsDel == true)
									{
										ret = posix_spawn(NULL, "/bin/rm", NULL, NULL, argvv, NULL);
										//ret = system(szSystemQuery);
										usleep(500000);
										// 로그 남김
										rmLOG(ALWAY, "%s\n", szSystemQuery );
									}

									if (ret == 0) {
										gnDeleteCnt++;
										gnTotalCnt++;
										ZzPRT(ALWAY, "-----> %s/%s DELETE OK\n", szfullpath, pent->d_name);
										ZzLOG(ALWAY, "-----> %s/%s DELETE OK\n", szfullpath, pent->d_name);
									}
									else
									{
										gnErrorCnt++;
										gnTotalCnt++;
										ZzPRT(ERROR, "-----> %s/%s DELETE ERROR errno(%d)[%s]\n", szfullpath, pent->d_name, errno, strerror(errno));
										ZzLOG(ERROR, "-----> %s/%s DELETE ERROR errno(%d)[%s]\n", szfullpath, pent->d_name, errno, strerror(errno));
									}
								}	
								else if( nResult > 0)
								{
									gnSkipCnt++;
									gnTotalCnt++;
									ZzPRT(ALWAY, "-----> %s/%s 사용중\n", szfullpath, pent->d_name);
									ZzLOG(ALWAY, "-----> %s/%s 사용중\n", szfullpath, pent->d_name);
								}
								else
								{
									gnDeleteCnt++;
									gnTotalCnt++;
									ZzPRT(ERROR, "-----> %s/%s DB Error\n", szfullpath, pent->d_name);
									ZzLOG(ERROR, "-----> %s/%s DB Error\n", szfullpath, pent->d_name);
								}	
							}
							else //그외 형태.
							{
								ZzPRT(ERROR, "-----> %s/%s unknown file type (see file owner and group)\n", szfullpath, pent->d_name);
								ZzLOG(ERROR, "-----> %s/%s unknown file type (see file owner and group)\n", szfullpath, pent->d_name);
								closedir(pdir);
								closedir(root_pdir);
								return -1;
							}
						}
						else //존재 하지 않는 폴더일꺼야.
						{
							GetErrMsg(errno);
							//printf("Error : (%d) This Folder is . or .. %s , errMsg : %s\n",stat,szfullpath,szErrorMsg);
							
						}
					}
					closedir(pdir);

					// 빈폴더 삭제하기
					char *argva[] = {"/bin/rmdir", szrootfullpath};
					
					if(gbIsDel == true)
					{
						posix_spawn(NULL, "/bin/rmdir", NULL, NULL, argva, NULL);
						usleep(500000);
						// 로그 남김
						//memset(szSystemQuery, 0x00, sizeof(szSystemQuery));
						//sprintf(szSystemQuery, "/bin/rmdir %s", szrootfullpath);
						//system(szSystemQuery);

						rmLOG(ALWAY, "/bin/rmdir %s\n", szrootfullpath );
					}
						

					
				}
				else
				{
					//다음 폴더 안으로 
//					printf("\n\nFolder %sin\n",szrootfullpath);
					
					if( DeleteFileList_One(pServerID, szrootfullpath) == -2 )
					{
						closedir(pdir);
						closedir(root_pdir);
						return -1;
					}
				}
			}
			else
			{
				//printf("file\n");
			}
		}
		
	}
	closedir(root_pdir);

	// 빈폴더 삭제하기
	char *argvb[] = {"/bin/rmdir", szPath};
	
	if(gbIsDel == true)
	{
		posix_spawn(NULL, "/bin/rmdir", NULL, NULL, argvb, NULL);
		// 로그 남김
		usleep(500000);
		memset(szSystemQuery, 0x00, sizeof(szSystemQuery));
		//sprintf(szSystemQuery, "/bin/rmdir %s", szrootfullpath);
		//system(szSystemQuery);
		rmLOG(ALWAY, "/bin/rmdir %s\n", szPath );	
	}
		
	return 0;
}

//******************************************************************************
//* daem8002 main
//******************************************************************************
int daem8002_main_process(int argc,char** argv)
{
	if(argc == 2 || argc ==3)
	{
		
	}
	else
	{
		printf("Usage : command <Server ID[hostname]> current_date\n");
		return -1;
	}

	
	int nServerType = 0;
	unsigned long dwTotalCount = 0;
	char szRootPath[256];
	char szStopDatePath[80];
	
	gnTotalCnt=0;
	gnDeleteCnt=0;
	gnSkipCnt=0;
	gnErrorCnt=0;


	memset(szRootPath, 0x00, sizeof(szRootPath));
	strcpy(szRootPath, "/raid/fdata");

	// 데이타 정보생성일자 검색
	memset(szStopDatePath,0x00,sizeof(szStopDatePath));
	if (daem8002_createdb_date(szStopDatePath) != 0)
	{
		return -1;
	}
	if (strlen(szStopDatePath) != 10)
	{
		// 자료생성일자 형식 (yyyy/mm/dd)
		ZzLOG(ERROR, "자료생성일자 형식이 잘 못 되어있습니다. ( %s )\n", szStopDatePath);
		return -1;
	}
	
	printf("start daem8002\n");
	printf("start ... Search Root Path : %s | Stop Path : %s\n\n", szRootPath, szStopDatePath);


	if(argc == 3 && strcmp(argv[2],"test") == 0)
	{
		gbIsDel = false;
	}	 	

//  하루 전날
//	DeleteFileList_Days(argv[1], szRootPath, szStopDatePath);
//	DeleteFileList(argv[1], szTempPath, szStopDatePath);
//  전체 날자
	DeleteFileList(argv[1], szRootPath, szStopDatePath);
	
	//sprintf(szTempPath,"%s",path);

	ZzPRT(ALWAY, "총 스캔 갯수 ( %d )\n", gnTotalCnt);
	ZzPRT(ALWAY, "총 삭제 갯수 ( %d )\n", gnDeleteCnt);
	ZzPRT(ALWAY, "총 통과 갯수 ( %d )\n", gnSkipCnt);
	ZzPRT(ALWAY, "총 오류 갯수 ( %d )\n", gnErrorCnt);
	
}

//******************************************************************************
//* daem8002_db_select
//*-----------------------------------------------------------------------------
//* 설  명 :  server_id, file_path, file_name으로 컨텐츠삭제정보을 찾는다
//* return : (-1) database 오류로 발생한 코드
//*               정상으로 자료 검색함
//******************************************************************************
int daem8002_db_select(char* pserver_id, char* pfile_path , char* pfile_name)
{
	char szQuery[1000];			// query string
	int  nSearchCnt;		// select count


	//ZzPRT(ALWAY, "-----> DB check %s/%s\n", pfile_path, pfile_name);
		
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT count(*) FROM zangsi_sum.T_CONTENTS_FILE_DEL_TEMP WHERE file_path  = '%s' AND file_name  = '%s' ; /* daem8002 */ "
	                 , pfile_path
	                 , pfile_name
	                 );

	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, " mysql_query error...\n");
		ZzLOG(ERROR, " [%d](%s)\n",mysql_errno(con), mysql_error(con));
		ZzPRT(ERROR, " [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, " mysql_store_result error...\n");
		ZzLOG(ERROR, " [%d](%s)\n",mysql_errno(con), mysql_error(con));
		ZzPRT(ERROR, " [%d](%s)\n",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
	row = mysql_fetch_row(res);
	nSearchCnt = 0;
	nSearchCnt = getint(row,0);
		
	mysql_free_result(res);
	return nSearchCnt;
}

//******************************************************************************
//* daem8002_createdb_date
//*-----------------------------------------------------------------------------
//* 설  명 : T_CONTENTS_FILE_DEL_TEMP 생성일자를 얻어 온다
//*
//* return : (-1) database 오류로 발생한 코드
//******************************************************************************
int daem8002_createdb_date(char *szStopDatePath)
{
	char szQuery[1000];			// query string
	int  nSearchCnt;		// select count

	int nCnt_Minor_code = 0;
	int nCnt_Contents_file_del_temp = 0;

	//1. T_MINOR_CODE 카운트 조회
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery ,"select minor_name from zangsi.T_MINOR_CODE where major_code = '100' and minor_code = '02' ; /* daem8002 */ ");
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem8002_createdb_date: zangsi.T_MINOR_CODE error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
	
	if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem8002_createdb_date: zangsi.T_MINOR_CODE mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)	
 	{
		mysql_free_result(res);
	    ZzLOG(ALWAY, "daem8002_createdb_date: zangsi.T_MINOR_CODE 처리할 자료가 없습니다.\n");
	    return -1;
	}
	row = mysql_fetch_row(res);
	nCnt_Minor_code = getint(row,0);

	mysql_free_result(res);

	//2. T_CONTENTS_FILE_DEL_TEMP 카운트 조회
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery ,"select count(*) as cnt from zangsi_sum.T_CONTENTS_FILE_DEL_TEMP ; /* daem8002 */");

	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem8002_createdb_date: zangsi_sum.T_CONTENTS_FILE_DEL_TEMP error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
	
	if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem8002_createdb_date: zangsi_sum.T_CONTENTS_FILE_DEL_TEMP mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)	
 	{
		mysql_free_result(res);
	    ZzLOG(ALWAY, "daem8002_createdb_date: zangsi_sum.T_CONTENTS_FILE_DEL_TEMP 처리할 자료가 없습니다.\n");
	    return -1;
	}
	row = mysql_fetch_row(res);
	nCnt_Contents_file_del_temp = getint(row,0);
	mysql_free_result(res);


	if(nCnt_Contents_file_del_temp != nCnt_Minor_code)
	{
		ZzLOG(ERROR, "nCnt_Contents_file_del_temp=%d \n",nCnt_Contents_file_del_temp);
		ZzLOG(ERROR, "nCnt_Minor_code=%d \n",nCnt_Minor_code);		
		return -1;
	}

	if(nCnt_Contents_file_del_temp == 0 && nCnt_Minor_code == 0 )
	{
		ZzLOG(ERROR, "nCnt_Minor_code=%d \n",nCnt_Minor_code);		
		return -1;
	}

//////////////////////////////////////////////////////////////////////////////////////

	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT minor_name FROM zangsi.T_MINOR_CODE WHERE major_code  = '100' AND minor_code  = '01' ; /* daem8002 */ " );

	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, " mysql_query error...\n");
		ZzLOG(ERROR, " [%d](%s)\n",mysql_errno(con), mysql_error(con));
		ZzPRT(ERROR, " [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, " mysql_store_result error...\n");
		ZzLOG(ERROR, " [%d](%s)\n",mysql_errno(con), mysql_error(con));
		ZzPRT(ERROR, " [%d](%s)\n",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "sysdate: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		ZzPRT(ERROR, "sysdate: mysql_num_rows error...\n");
		mysql_free_result(res);
		return -1;
	}
	
	row = mysql_fetch_row(res);
	strcpy(szStopDatePath, getstr(row, 0));
	
	mysql_free_result(res);

	return 0;
}


/*******************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*******************************************************************************/
int daem8002_init_process()
{
	char stemp[128];
	int ret=0;
	g_lTotalRecordCount = 0;

	/*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("daem8002", "/logs"); 

    ZzLOG(ALWAY, "[daem8002]*****************프로그램 시작*****************\n");  
    ZzLOG(ALWAY, "[daem8002] 파일검색  삭제처리\n");  
	
	printf("daem8002_init_process...\n");
	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(con=db_connect_backup("zangsi")))
	{
		
		printf("DB에 접속하지 못 하였습니다...\n");
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1); 
	}

    return (1);

}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daem8002_term_process()
{
    // DB close
	db_disconnect(con);
    ZzLOG(ALWAY, "[daem8002]*****************프로그램 종료*****************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem8002_signal(int nSignal)
{
    daem8002_term_process();
}

/*****************************************************************************
*  프로그램 메인 
*****************************************************************************/
int main(int argc, char **argv)
{                
	char    szTemp[1024];
	int     rc;
                 
	/*       
	** SIGNAL 정의
	*/       

	signal(SIGTERM, daem8002_signal);
	signal(SIGINT,  daem8002_signal);
	signal(SIGQUIT, daem8002_signal);
	signal(SIGKILL, daem8002_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
	if( argc >= 2 )
	{
		if(strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "-V") == 0)
		{	
			printf("version : 2012,5,30  \n");
	    	exit(1);
	    }
	}
	if ( daem8002_init_process() > 0 ) 
	{
		/* 프로그램 메인루틴 */ 
		
		rc = daem8002_main_process(argc,argv);
		/* 프로그램 종료루틴 */
	}
	daem8002_term_process();

	
	return(0);
}
/*****************************************************************************
*  End of file...
*****************************************************************************/
