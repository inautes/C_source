/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem8003.cc
 *         기능 : DB정보없는 파일찾기(파일기준)
 *         설명 :
 *     설치위치 : 각각 서버마다 위치한다.
 *       작성자 : 몬몬
 *       작성일 : 20160912
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
#include "Property.h"

#define  MAX_ROWS	1
#define _DEBUG_

#define MY_SERVER 1
#define WE_SERVER 2

#define FT_FOLDER 3
#define FT_FILE 4

static char szErrorMsg[256];

char gszServerID[16];
char gszRootPath[256];
int  gnLog = 0;
int  gnChekDepth = 0;
char szSystemQuery[1000];
int  gnTotalCnt;
int  gnSkipCnt;
int  gnDeleteCnt;
int  gnErrorCnt;

char szTempPath[256];

bool gbIsDel = true;

int  daem8003_init_process();
int  daem8003_main_process(int argc,char** argv);
int  daem8003_term_process();
void daem8003_signal(int nSignal);
int	 daem8003_db_check(char* pserver_id, char* pRoot_path , char* pName, char* forder_yn);

int db_check1(char* pserver_id, char* pRoot_path , char* pName, char* forder_yn);
int db_check2(char* pserver_id, char* pRoot_path , char* pName, char* forder_yn);
int db_check3(char* pserver_id, char* pRoot_path , char* pName, char* forder_yn);
int db_check4(char* pserver_id, char* pRoot_path , char* pName, char* forder_yn);
int db_check5(char* pserver_id, char* pRoot_path , char* pName, char* forder_yn);
int db_check6(char* pserver_id, char* pRoot_path , char* pName, char* forder_yn);
int db_check7(char* pserver_id, char* pRoot_path , char* pName, char* forder_yn);
int db_check8(char* pserver_id, char* pRoot_path , char* pName, char* forder_yn);
int db_check9(char* pserver_id, char* pRoot_path , char* pName, char* forder_yn);
void RmPrint(char *szPath, char* szName, char *szType);

MYSQL     *con;
MYSQL_RES *res;
MYSQL_ROW  row;

void GetErrMsg(int nErrno)
{
	int nTempErrNo = nErrno;
	memset(szErrorMsg,0x00,sizeof(szErrorMsg));
	strcpy(szErrorMsg,strerror(nTempErrNo));
}


int CheckFileList(char* path, int nDepth)
{
	if(gnChekDepth < nDepth)
	{
		printf("stop : %d nDepth\n",nDepth);
		return -1;
	}

	struct stat64 rootstatbuf;

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

					//
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
							gnTotalCnt++;

							memset(szfullpath,0x00,sizeof(szfullpath));
							strcpy(szfullpath, szrootfullpath);
							strcat(szfullpath,"/");
							strcat(szfullpath, Subdirentp[nj]->d_name);

							printf("[%s]\n ",szfullpath);

							struct stat64 subststatbuf;
							stat = stat64(szfullpath,&subststatbuf);


							if(stat != 0)
							{
								printf("continue.. \n");
								continue;
							}

							int nResult = 0;
							printf("[%d, %d] ",gnChekDepth,nDepth);

							if(S_ISDIR (subststatbuf.st_mode))
							{
								if(gnChekDepth > nDepth)
								{
									nResult = db_check2(gszServerID,szrootfullpath,Subdirentp[nj]->d_name,"Y");
									printf("db_check2 = %d\n",nResult);

									if(nResult <= 0)
									{
										printf("del DIR %s, %s, %s\n",gszServerID,szfullpath,Subdirentp[nj]->d_name);
										gnDeleteCnt++;
										RmPrint(szrootfullpath,Subdirentp[nj]->d_name,"D");

									}
									else
									{
										printf("skip DIR %s, %s, %s\n",gszServerID,szfullpath,Subdirentp[nj]->d_name);
										gnSkipCnt++;
									}


								}
								else if(gnChekDepth == nDepth)
								{
									nResult =  daem8003_db_check(gszServerID,szrootfullpath,Subdirentp[nj]->d_name,"Y");

									printf("daem8003_db_check = %d\n",nResult);
									if(nResult > 0)
									{
										printf("check DIR skip %s, %s, %s\n",gszServerID,szfullpath,Subdirentp[nj]->d_name);
										gnSkipCnt++;
									}
									else
									{
										printf("check DIR del %s, %s, %s\n",gszServerID,szfullpath,Subdirentp[nj]->d_name);
										gnDeleteCnt++;
										RmPrint(szrootfullpath,Subdirentp[nj]->d_name,"D");
									}

								}
								else
								{
									gnErrorCnt++;
									printf("error DIR ing \n");
								}
							}
							else // if (S_ISREG (subststatbuf.st_mode))
							{
								if(gnChekDepth > nDepth)
								{
									nResult = db_check2(gszServerID,szrootfullpath,Subdirentp[nj]->d_name,"N");
									printf("db_check2 = %d\n",nResult);
									if(nResult <= 0)
									{
										printf("del FILE %s, %s, %s\n",gszServerID,szfullpath,Subdirentp[nj]->d_name);
										gnDeleteCnt++;
										RmPrint(szrootfullpath,Subdirentp[nj]->d_name,"F");
									}
									else
									{
										printf("skip FILE %s, %s, %s\n",gszServerID,szfullpath,Subdirentp[nj]->d_name);
										gnSkipCnt++;
									}
								}
								else if(gnChekDepth == nDepth)
								{
									printf("check FILE %s, %s, %s\n",gszServerID,szfullpath,Subdirentp[nj]->d_name);
									nResult =  daem8003_db_check(gszServerID,szrootfullpath,Subdirentp[nj]->d_name,"N");
									printf("daem8003_db_check = %d\n",nResult);

									if(nResult > 0)
									{
										gnSkipCnt++;
									}
									else
									{
										gnDeleteCnt++;
										RmPrint(szrootfullpath,Subdirentp[nj]->d_name,"F");
									}

								}
								else
								{
									printf("error FILE ing \n");
								}
							}
//							else //그외 형태.
//							{
//								ZzPRT(ERROR, "-----> %s/%s unknown file type (see file owner and group)\n", szfullpath, Subdirentp[nj]->d_name);
//								ZzLOG(ERROR, "-----> %s/%s unknown file type (see file owner and group)\n", szfullpath, Subdirentp[nj]->d_name);
//								//return -1;
//							}
						}
						else //존재 하지 않는 폴더일꺼야.
						{
							GetErrMsg(errno);
						}
					}


				}
				else
				{
					if( CheckFileList(szrootfullpath,nDepth+1) == -1 )
					{
						return -1;
					}
				}
			}
			else
			{
				gnTotalCnt++;

				if(gnChekDepth > nDepth)
				{
					// 파일만 삭제
					// 폴더는 로그만 남김.

					int nResult = db_check2(gszServerID,szrootfullpath,direntp[ni]->d_name,"N");
					printf("db_check2 = %d\n",nResult);

					if(	nResult <= 0)
					{
						printf("=== error del file = %s , %s\n",szrootfullpath,direntp[ni]->d_name);
						gnDeleteCnt++;
						RmPrint(szrootfullpath,direntp[ni]->d_name,"F");
					}
					else
					{
						printf("=== db_check2 %d \n",nResult);
						gnSkipCnt++;
					}

				}
				else if(gnChekDepth == nDepth)
				{
					printf("=== error check file = %s , %s\n",szrootfullpath,direntp[ni]->d_name);
					// 들어오면 안되는곳
					//gnDeleteCnt++;
					//gnSkipCnt++;
					// 파일 체크
				}
				else
				{
					gnErrorCnt++;
					printf("error end \n");
				}

			}
		}


    }


	//RmPrint(szPath);

	/*// 빈폴더 삭제하기
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
	*/


	return 0;
}


void RmPrint(char *szPath, char* szName, char *szType)
{
	if(gbIsDel == true)
	{
		//posix_spawn(NULL, "/bin/rmdir", NULL, NULL, argvb, NULL);
		usleep(500000);
		// 로그 남김
		if(strcmp(szType,"D")==0)
		{
			printf("D %s/%s\n",szPath,szName);
			rmLOG(ALWAY, "/bin/rmdir %s/%s\n", szPath,szName );
		}
		else
		{
			printf("F %s/%s\n",szPath,szName);
			rmLOG(ALWAY, "/bin/rm -rf %s/%s\n", szPath, szName );
		}

	}
}

//******************************************************************************
//* daem8003 main
//******************************************************************************
int daem8003_main_process(int argc,char** argv)
{

	Property pP;
	pP.SetProcName(argv[0]);
//	pP.GetStrProperty("[INFO]", "SERVER_ID"	, gszServerID);
	pP.GetStrProperty("[INFO]", "ROOTPATH"	, gszRootPath);
	pP.GetIntProperty("[INFO]", "BOOL_LOG"	, gnLog);
	pP.GetIntProperty("[INFO]", "CHECK_DEPTH"	, gnChekDepth); // 5

	gnTotalCnt=0;
	gnDeleteCnt=0;
	gnSkipCnt=0;
	gnErrorCnt=0;

	if(strcmp(gszRootPath,"") == 0 || strcmp(gszRootPath,"") == 0 )
		return -1;

	char* pHost = Func_getHostname();
	//infLOG(ALWAY,"pHost = %s , sizeof(pHost) = %d \n",pHost,sizeof(pHost));
	strcpy(gszServerID,pHost);

	if( strlen ( gszServerID) <= 0 )
	{
		//infLOG(ALWAY,"HOSTNAME is null\n");
		return false;
	}

	printf ("gszServerID = %s\n",gszServerID);
	printf ("gszRootPath = %s\n",gszRootPath);
	printf ("gnLog = %d\n",gnLog);
	printf ("gnChekDepth = %d\n",gnChekDepth);

	CheckFileList(gszRootPath,0);

	ZzPRT(ALWAY, "총 스캔 갯수 ( %d )\n", gnTotalCnt);
	ZzPRT(ALWAY, "총 삭제 갯수 ( %d )\n", gnDeleteCnt);
	ZzPRT(ALWAY, "총 통과 갯수 ( %d )\n", gnSkipCnt);
	ZzPRT(ALWAY, "총 오류 갯수 ( %d )\n", gnErrorCnt);

}

//******************************************************************************
//* daem8003_db_check
//*-----------------------------------------------------------------------------
//* 설  명 :  server_id, file_path, file_name으로 컨텐츠삭제정보을 찾는다
//* return : (-1) database 오류로 발생한 코드
//*               정상으로 자료 검색함
//******************************************************************************
int daem8003_db_check(char* pserver_id, char* pRoot_path , char* pName, char* forder_yn)
{
	int  nSearchCnt = -1;		// select count

	nSearchCnt = db_check1 (pserver_id,pRoot_path,pName,forder_yn);

	if(nSearchCnt > 0)
		return nSearchCnt;

	nSearchCnt = db_check2 (pserver_id,pRoot_path,pName,forder_yn);

	if(nSearchCnt > 0)
		return nSearchCnt;

	nSearchCnt = db_check3 (pserver_id,pRoot_path,pName,forder_yn);

	if(nSearchCnt > 0)
		return nSearchCnt;

	nSearchCnt = db_check4 (pserver_id,pRoot_path,pName,forder_yn);

	if(nSearchCnt > 0)
		return nSearchCnt;

	nSearchCnt = db_check5 (pserver_id,pRoot_path,pName,forder_yn);

	if(nSearchCnt > 0)
		return nSearchCnt;

	nSearchCnt = db_check6 (pserver_id,pRoot_path,pName,forder_yn);

	if(nSearchCnt > 0)
		return nSearchCnt;

	nSearchCnt = db_check7 (pserver_id,pRoot_path,pName,forder_yn);

	if(nSearchCnt > 0)
		return nSearchCnt;

	nSearchCnt = db_check8 (pserver_id,pRoot_path,pName,forder_yn);

	if(nSearchCnt > 0)
		return nSearchCnt;

	nSearchCnt = db_check9 (pserver_id,pRoot_path,pName,forder_yn);

	if(nSearchCnt > 0)
		return nSearchCnt;



	return nSearchCnt;
}

int db_check1(char* pserver_id, char* pRoot_path , char* pName, char* forder_yn)
{
	printf("db_check1\n");
	char szQuery[1000];			// query string
	int  nSearchCnt;		// select count

	memset (szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery, "SELECT COUNT(*) "
					" FROM zangsi.T_CONTDATA_FILE a "
					" WHERE a.server_id = '%s' AND a.file_path = '%s' AND a.file_name1 = '%s' AND a.folder_yn = '%s';  /* daem8003*/ "
	                 , pserver_id
	                 , pRoot_path
 	                 , pName
  	                 , forder_yn
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

int db_check2(char* pserver_id, char* pRoot_path , char* pName, char* forder_yn)
{
	printf("db_check2\n");
	char szQuery[1000];			// query string
	int  nSearchCnt;		// select count

	memset (szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery, "SELECT COUNT(*) "
					" FROM zangsi.T_CONTDATA_MYFILE a "
					" WHERE a.server_id = '%s' AND a.file_path = '%s' AND a.file_name1 = '%s' AND a.folder_yn = '%s';  /* daem8003*/ "
	                 , pserver_id
	                 , pRoot_path
 	                 , pName
  	                 , forder_yn
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

int db_check3(char* pserver_id, char* pRoot_path , char* pName, char* forder_yn)
{
	printf("db_check3\n");
	char szQuery[1000];			// query string
	int  nSearchCnt;		// select count

	memset (szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery, "SELECT COUNT(*) "
					" FROM zangsi.T_CONTENTS_DEL a "
					" WHERE a.server_id = '%s' AND a.file_path = '%s' AND a.file_name1 = '%s' AND a.folder_yn = '%s';  /* daem8003*/ "
	                 , pserver_id
	                 , pRoot_path
 	                 , pName
  	                 , forder_yn
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

int db_check4(char* pserver_id, char* pRoot_path , char* pName, char* forder_yn)
{
	printf("db_check4\n");
	char szQuery[1000];			// query string
	int  nSearchCnt;		// select count

	memset (szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery, "SELECT COUNT(*) "
					" FROM zangsi.T_CONTENTS_FILE a "
					" WHERE a.server_id = '%s' AND a.file_path = '%s' AND a.file_name1 = '%s' AND a.folder_yn = '%s';  /* daem8003*/ "
	                 , pserver_id
	                 , pRoot_path
 	                 , pName
  	                 , forder_yn
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

int db_check5(char* pserver_id, char* pRoot_path , char* pName, char* forder_yn)
{
	printf("db_check5\n");
	char szQuery[1000];			// query string
	int  nSearchCnt;		// select count

	memset (szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery, "SELECT COUNT(*) "
					" FROM zangsi.T_CONTENTS_TEMP a "
					" WHERE a.server_id = '%s' AND a.sfile_path = '%s' AND a.sfile_name = '%s' ;  /* daem8003*/ "
	                 , pserver_id
	                 , pRoot_path
 	                 , pName
  	                 , forder_yn
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

int db_check6(char* pserver_id, char* pRoot_path , char* pName, char* forder_yn)
{
	printf("db_check6\n");
	char szQuery[1000];			// query string
	int  nSearchCnt;		// select count

	memset (szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery, "SELECT COUNT(*) "
					" FROM zangsi.T_BOARD_DATA_F a "
					" WHERE a.server_id = '%s' AND a.file_spath = '%s' AND a.file_sname = '%s' AND a.folder_yn = '%s';  /* daem8003*/ "
	                 , pserver_id
	                 , pRoot_path
 	                 , pName
  	                 , forder_yn
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

int db_check7(char* pserver_id, char* pRoot_path , char* pName, char* forder_yn)
{
	printf("db_check7\n");
	char szQuery[1000];			// query string
	int  nSearchCnt;		// select count

	memset (szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery, "SELECT COUNT(*) "
					" FROM zangsi.T_CONTFLOG_FILE a "
					" WHERE a.server_id = '%s' AND a.file_path = '%s' AND a.file_name1 = '%s' AND a.folder_yn = '%s';  /* daem8003*/ "
	                 , pserver_id
	                 , pRoot_path
 	                 , pName
  	                 , forder_yn
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

int db_check8(char* pserver_id, char* pRoot_path , char* pName, char* forder_yn)
{
	printf("db_check8\n");
	char szQuery[1000];			// query string
	int  nSearchCnt;		// select count

	memset (szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery, "SELECT COUNT(*) "
					" FROM zangsi.T_CONTENTS_FILE_DEL a "
					" WHERE a.server_id = '%s' AND a.file_path = '%s' AND a.file_name1 = '%s' AND a.folder_yn = '%s';  /* daem8003*/ "
	                 , pserver_id
	                 , pRoot_path
 	                 , pName
  	                 , forder_yn
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

int db_check9(char* pserver_id, char* pRoot_path , char* pName, char* forder_yn)
{
	printf("db_check9\n");
	char szQuery[1000];			// query string
	int  nSearchCnt;		// select count

	memset (szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery, "SELECT COUNT(*) "
					" FROM zangsi.T_CONTFLOG_FILE_DEL a "
					" WHERE a.server_id = '%s' AND a.file_path = '%s' AND a.file_name1 = '%s' AND a.folder_yn = '%s';  /* daem8003*/ "
	                 , pserver_id
	                 , pRoot_path
 	                 , pName
  	                 , forder_yn
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

/*******************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*******************************************************************************/
int daem8003_init_process()
{
	char stemp[128];
	int ret=0;

	/*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("daem8003", "/logs");

    ZzLOG(ALWAY, "[daem8003]*****************프로그램 시작*****************\n");
    ZzLOG(ALWAY, "[daem8003] 파일검색  삭제처리\n");

	printf("daem8003_init_process...\n");
	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(con=db_connect_sub1("zangsi")))
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
int daem8003_term_process()
{
    // DB close
	db_disconnect(con);
    ZzLOG(ALWAY, "[daem8003]*****************프로그램 종료*****************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem8003_signal(int nSignal)
{
    daem8003_term_process();
}

/*****************************************************************************
*  프로그램 메인
*****************************************************************************/
int main(int argc, char **argv)
{

	signal(SIGTERM, daem8003_signal);
	signal(SIGINT,  daem8003_signal);
	signal(SIGQUIT, daem8003_signal);
	signal(SIGKILL, daem8003_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);


	if ( daem8003_init_process() > 0 )  //DB 연결
	{
		daem8003_main_process(argc,argv); // 작업
	}

	daem8003_term_process(); // DB 연결 해제


	return(0);
}
/*****************************************************************************
*  End of file...
*****************************************************************************/
