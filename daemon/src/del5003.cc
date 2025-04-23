/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5003
 *         기능 : 삭제된 컨텐츠 파일 Delete 처리
 *         설명 : 1. 프로세스 처리 예정시간 - 07:00 ~ 09:00사이에 수행.
 *                2. 디비정보가 삭제된 컨텐츠에 대해서 실제 파일삭제 처리
 				  3. 네트워크 오류 유무 판단 ( 링크 스피드 확인 )
 *     설치위치 : 컨텐츠 서버
 *
 *       작성자 : LEE
 *       작성일 : 2004/02/16
 *     수정이력 : 2007/02/10
 *		 수정자 : HCS

 daem5003_process() : zangsi.T_CONTENTS_DEL 목록 삭제, file_del_yn = 'Y' 업데이트
 daem5003_file_del_process() : -30일 이전 업로드 실패 파일, T_CONTENTS_TEMP

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
#include <sys/stat.h>
#include <errno.h>
#include <sys/vfs.h>
#include <unistd.h>     /* for close() */
#include <math.h>

#include "daemcom.h"
#include "commydb.h"

#define  MAX_ROWS	1
#define _DEBUG_

int RunSystem(char* pSystemQuery);
int daem5003_file_del_process();
int daem5003_process_deletefile();
int daem5003_process_init(int argc, char **argv);
int daem5003_process_term();
void daem5003_signal(int nSignal);
void UpdateDiskSize();

MYSQL     *con;
MYSQL_RES *res;
MYSQL_ROW  row;

MYSQL_RES *res2;
MYSQL_ROW  row2;

char   gserver_id [  5+1];	// 서버ID
char   gcont_gu   [  2+1];	// 서버구분
char   gproc_date [  8+1];	// 처리일자
char   gdel_date  [  8+1];	// 삭제예정일
char   gfile_path [255+1];	// 파일결로
char   gfile_name1 [ 30+1];	// 파일이름
char   gfile_name2 [ 30+1];
char   gfile_name [ 30+1];
char   gfolder_yn [ 2   ];
unsigned long gid         ;	// 컨텐츠ID
unsigned int finalId	;
double gfile_size        ;	// 파일크기
char   gdel_date_ago_3day [  8+1];	// 삭제예정일자
char   gdel_date_ago_14day [  8+1];	// 삭제예정일자

//******************************************************************************
//* daem5003 main
//******************************************************************************
int RunSystem(char* pSystemQuery)
{
	int nError = 0;
	nError = system(pSystemQuery);

	if(nError == 127 || nError < 0)
	{
		
		printf("system()호출에 실패 했습니다. nError : [%d] [ %s ]", errno,strerror(errno));
		return -1;
		//return 0;
	}
	return 0;
}


void UpdateDiskSize()
{
	char szQuery[1000];		// query string
	int  ret;
	struct stat64 statbuf;

//	if (tran_begin(con)!=0) {
//	    ZzLOG(ERROR, "tran_begin: 테이베이스 오류입니다.\n");
//	    return -1;
//	}

	MYSQL_RES *tmp_res;
	MYSQL_ROW  tmp_row;

	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "select a.root_path from zangsi.T_SERVER_INFO  a where a.server_id = '%s' ; /* daem5003 */" ,gserver_id);

	if (mysql_query(con, szQuery)){
	    return;
    }

	if (!(tmp_res = mysql_store_result(con)))
	{
		printf("[%d](%s)",mysql_errno(con), mysql_error(con));
		return ;
	}
 	if (mysql_num_rows(tmp_res)==0)
 	{
		printf("[%d](%s)",mysql_errno(con), mysql_error(con));
		mysql_free_result(tmp_res);
		return ;
	}
	tmp_row = mysql_fetch_row(tmp_res);

	char disk_name[256]; memset(disk_name,0,256);
	strcpy(disk_name,   getstr(tmp_row, 0));
	mysql_free_result(tmp_res);

	struct statfs64 fs;
	ret = statfs64(disk_name, &fs) ;
	ZzLOG(ALWAY,"statfs ret [ %d ] error [ %d ] error msg [ %s ] \n",ret,errno,strerror(errno));
	double total		= 	(double)fs.f_bsize * (double)fs.f_blocks;
	double remainder	= (double)fs.f_bsize * (double)fs.f_bavail;

	/* 2016-10-12 디스크 사용량 체크후 사용량이 99% 이상이면 컨텐츠서버의 업로드를 차단한다. 이동민 */

	double nUsePercent = (total-remainder)*100/total;

	ZzLOG(ALWAY,"UseDiskPercent  [ %.f ] ] \n",nUsePercent);
	if ( nUsePercent > 99.3 )
	{
		sprintf(szQuery,	 "UPDATE zangsi.T_SERVER_INFO"
								 " SET disk_size = %15.0f , disk_use   = %15.0f , real_disk_use   = %15.0f, upload_yn = 'N' "
								 " WHERE server_id  = '%s' ; /* daem5003 */ "
								 ,total
								 ,total-remainder
								 ,remainder
								 ,gserver_id
								 );
		ZzLOG(ALWAY, "%s\n\n",szQuery);

	}
	else
	{
		sprintf(szQuery,	 "UPDATE zangsi.T_SERVER_INFO"
							 " SET disk_size = %15.0f , disk_use   = %15.0f , real_disk_use   = %15.0f , upload_yn = 'N'"
							 " WHERE server_id  = '%s' ; /* daem5003 */ "
							 ,total
							 ,total-remainder
							 ,remainder
							 ,gserver_id
							 );
		ZzLOG(ALWAY, "%s\n\n",szQuery);
	}

	ZzLOG(ALWAY, "%s\n\n",szQuery);

	if (mysql_query(con, szQuery)){
		ZzLOG(ERROR, "process_db: UPDATE zangsi.T_SERVER_INFO error...\n");
		return ;
	}

	if( ret != 0) //err
	{
		return ;
	}

	if(tran_commit(con)!=0)
	{
		ZzLOG(ERROR, "process_db: tran_commit error...\n");
		return ;
	}
	if(tran_end(con)!=0)
	{
		ZzLOG(ERROR, "process_db: tran_end 테이베이스 오류입니다.\n");
		return ;
	}

}

int daem5003_file_del_process()
{
	
	char szQuery[1000];
	memset(szQuery, 0x00, sizeof(szQuery));
	//con = NULL;
	res = NULL;
	row = NULL;
	gid = 0;

		
	char   szproc_date [  8+1];	// 처리일자
	memset(szproc_date, 0x00, sizeof(szproc_date));

	int	nRowcnt = 0;

	printf("5003 file del start \n");

	sprintf(szQuery, "SELECT date_format(date_add(now(), INTERVAL -30 DAY),'%Y%m%d') ; /* daem5003 */ ");
	printf("Getting date : %s \n",szQuery);

		
	printf("query: %s\n",szQuery);

	if (mysql_query(con, szQuery))
	{
		printf("[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con)))
	{
		printf("[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (mysql_num_rows(res)==0)
	{
		mysql_free_result(res);
	    printf("daem5003_file_del_process: 날짜 얻기 실패.\n");
		return 0;
	}

	row = mysql_fetch_row(res);
	strcpy(szproc_date, getstr(row, 0));

	memset(szQuery, 0x00, sizeof(szQuery));
	//con = NULL;
	res = NULL;
	row = NULL;

	sprintf(szQuery, "  select a.id, a.file_path,a.file_name1, a.file_name2, a.folder_yn"
					 "	from zangsi.T_CONTENTS_FILE_DEL a "
					 "	where a.server_id = '%s' "
                     "  and a.reg_date  >= '00000000' 								"
                     "  and a.reg_date  <= '%s' 									"
                     " and a.id > %ld and a.id < %ld order by a.id desc"
                     "  LIMIT 3000 	; /* daem5003 */ "
					 , gserver_id, gproc_date, gid, finalId);
		
	printf("select contents set query: %s \n",szQuery);
	
	if (mysql_query(con, szQuery))
	{
		printf("[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con)))
	{
		mysql_free_result(res);
	    //ZzLOG(ERROR, "daem5003_file_del_process: mysql_store_result error...\n");
		printf("[%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (mysql_num_rows(res)==0)
	{
		mysql_free_result(res);
	    printf("daem5003_file_del_process: 처리할 자료가 없습니다.\n");
		return 0;
	}

	printf("daem5003_file_del_process: select_cnt(%d)\n", mysql_num_rows(res));
	
	int nCount = (mysql_num_rows(res));
	
	while((row = mysql_fetch_row(res))!= NULL)
	{
		gid = 0;
		memset(gfile_path, 0x00, sizeof(gfile_path));
		memset(gfile_name1, 0x00, sizeof(gfile_name1));
		
		memset(gfile_name2, 0x00, sizeof(gfile_name2));
		memset(gfolder_yn, 0x00, sizeof(gfolder_yn));

		gid = getint(row, 0);
		strcpy(gfile_path, getstr(row, 1));
		strcpy(gfile_name1, getstr(row, 2));
		strcpy(gfile_name2, getstr(row, 3));
		strcpy(gfolder_yn, getstr(row, 4));
		
		finalId = getint(row, 0);


		if(daem5003_process_deletefile() < 0)
		{
			printf("%d번 %s/%s 처리 실패", gid, gfile_path, gfile_name);
		}
	}
	mysql_free_result(res);
	printf("Query Count Num : %d",nCount);
	return nCount;

}

int daem5003_process_deletefile()
{
	char szQuery[1000];
	memset(szQuery, 0x00, sizeof(szQuery));

	char szSysQueryZero[612];
	memset(szSysQueryZero, 0x00, sizeof(szSysQueryZero));
	
//	char szSysQuery[1024];
	char szSysQuery[612];
	memset(szSysQuery, 0x00, sizeof(szSysQuery));
	
//	char szfullfile[1024];
	char szfullfile[612];
	memset(szfullfile, 0x00, sizeof(szfullfile));

	int  ret = 0;
	struct stat64 statbuf;

	if((strcmp(gfile_path, "")!=0) && (strcmp(gfile_name1, "")!=0))
	{
/////////////////////////////////////////////
/*				db 처리				   */
/////////////////////////////////////////////
		memset(szQuery, 0x00, sizeof(szQuery));

/////////////////////////////////////////////
/*				파일 삭제				   */
/////////////////////////////////////////////
		
		if(!strcmp(gfolder_yn,"Y"))
		{
			strcpy(szfullfile, gfile_path);
			strcat(szfullfile, "/");
			strcat(szfullfile, gfile_name1);
			strcat(szfullfile, "/");
			strcat(szfullfile, gfile_name2);
		}else
		{

			strcpy(szfullfile, gfile_path);
			strcat(szfullfile, "/");
			strcat(szfullfile, gfile_name1);
		}
		
		
		strcpy(szSysQueryZero, "cat \"/dev/null\" > ");
		
		strcat(szSysQueryZero, "\"");
		strcat(szSysQueryZero, szfullfile);
		strcat(szSysQueryZero, "\"");
		
		strcpy(szSysQuery, "rm -rf ");
		strcat(szSysQuery, szfullfile);

		printf("\ndelete target file name : %s\n", szfullfile);

		if (strcmp(szfullfile, "/")==0 || strlen(szfullfile) <= strlen("/raid/fdata/wedisk/2005/00/00/00/") )
		{
			goto daem5003_process_deletefile_err;
		}
		//파일이 있는지 검사한다.
		ret = lstat64(szfullfile, &statbuf);

		printf("File exist check return : %d\n, %d", ret, errno);
		
		if( errno != 2 )
		{
			printf("zero space setting : %s\n",szSysQueryZero);
			if(RunSystem(szSysQueryZero) != 0)
			{
				
				printf("%s file change null fail \n",szfullfile);
								
				//goto daem5003_process_deletefile_err;
			}
			
			if(RunSystem(szSysQuery) != 0)
			{
				printf("%s file delete process _fail \n",szfullfile);
				goto daem5003_process_deletefile_err;
			}

			//UpdateDiskSize();

			ret = 0;
			ret = lstat64(szfullfile, &statbuf);


			if(ret == 0)
			{
				printf("%s delete fail", szfullfile);
				goto daem5003_process_deletefile_err;
			}
		}

/////////////////////////////////////////////
/*				db commit				   */
/////////////////////////////////////////////
		if(tran_commit(con)!=0)
		{
		    printf("process_db: tran_commit error...\n");
		    goto daem5003_process_deletefile_err;
		}
		if(tran_end(con)!=0)
		{
		    printf("process_db: tran_end 테이베이스 오류입니다.\n");
		    goto daem5003_process_deletefile_err;
		}
	}

	return 0;

daem5003_process_deletefile_err:
	printf( "daem5003_process_deletefile: [%d](%s)",mysql_errno(con), mysql_error(con));
	return -1;

}


/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5003_get_sysdate()
{
	char szQuery[1000];		// query string
	char sztemp [100];      // query temp

	//if (strcmp(gproc_date, "00000000") != 0)
//		return 0;

	memset(szQuery, 0x00, sizeof(szQuery));
	//strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d')");
	if (strcmp(gproc_date, "00000000") == 0)
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d')");
		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -4 DAY),'%Y%m%d')");
		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -14 DAY),'%Y%m%d')  ; /* daem5003 */  ");
	}
	else
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		strcpy(szQuery, "SELECT date_format('");
		strcat(szQuery, gproc_date);
		strcat(szQuery, "','%Y%m%d')");
		strcat(szQuery, "     , date_format(date_add('");
		strcat(szQuery, gproc_date);
		strcat(szQuery, "', INTERVAL -3 DAY),'%Y%m%d')");
		strcat(szQuery, "     , date_format(date_add('");
		strcat(szQuery, gproc_date);
		strcat(szQuery, "', INTERVAL -14 DAY),'%Y%m%d')  ; /* daem5003 */  ");
	}


	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "sysdate: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}

	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "sysdate: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "sysdate: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
	row = mysql_fetch_row(res);
	memset(gproc_date, 0x00, sizeof(gproc_date));
	memset(gdel_date_ago_3day, 0x00, sizeof(gdel_date_ago_3day));
	memset(gdel_date_ago_14day, 0x00, sizeof(gdel_date_ago_14day));

	strcpy(gproc_date,   getstr(row, 0));
	strcpy(gdel_date_ago_3day,   getstr(row, 1));
	strcpy(gdel_date_ago_14day,   getstr(row, 2));

	mysql_free_result(res);

	return 0;
}


/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5003_process_init(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("daem5003", "/logs/daemon");
	finalId = 40000000;
    //ZzLOG(ALWAY, "***************프로그램 시작***************\n");
	printf("init program start\n");
    // 파라미터 값 설정 및 초기화
    if (argc != 3){

		printf("init argument is not 3");

    	goto arg_error;
    }

    // 서버이름 얻기
	memset(gserver_id, 0x00, sizeof(gserver_id));
	strcpy(gserver_id, argv[2]);
	if (strcmp(gserver_id, "00000")==0)
	{
		memset (gserver_id, 0x00, sizeof(gserver_id));
		sprintf(gserver_id, "%s", getenv("SERVER_ID"));
		if (strcmp(gserver_id, "(null)")==0) {
			goto arg_error;
		}
	}
	if (strcmp(gserver_id, "")==0)
		goto arg_error;

	ZzPRT(ALWAY, "gserver_id : (%s)\n", gserver_id);

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(con=db_connect_backup("zangsi")))
	{
		printf("DB에 접속하지 못 하였습니다...\n");
	   	return(-1);
	}

	/* 처리일자 */
	memset(gproc_date, 0x00, sizeof(gproc_date));
	strcpy(gproc_date, argv[1]);
	ret=daem5003_get_sysdate();
	if (ret < 0){
		db_disconnect(con);
		return -1;
	}

	printf("init process complete");

    return (0);


arg_error:
    printf("usage : %s YYYYMMDD XXXXX\n", argv[0]);
    printf("        YYYYMMDD(처리일자)-> '00000000'인경우 system일자로처리\n");
    printf("        XXXXX   (서버ID  )-> '00000'   인경우 getenv(SERVER_ID)참조\n");

    printf("usage : %s YYYYMMDD XXXXX\n", argv[0]);
    printf("        YYYYMMDD(처리일자)-> '00000000'인경우 system일자로처리\n");
    printf("        XXXXX   (서버ID  )-> '00000'   인경우 getenv(SERVER_ID)참조\n");
    return -1;
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daem5003_process_term()
{
    // DB close
	db_disconnect(con);
    printf("***************프로그램 종료***************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5003_signal(int nSignal)
{
    daem5003_process_term();
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
	signal(SIGTERM, daem5003_signal);
	signal(SIGINT,  daem5003_signal);
	signal(SIGQUIT, daem5003_signal);
	signal(SIGKILL, daem5003_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
	printf("start program ......\n");
    if( argc >= 2 )
	{
		printf("argument check....\n");
		if(strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "-V") == 0)
		{
			printf("version : 2013,6,28  \n");
	    	exit(1);
	    }
	}

	if ( daem5003_process_init(argc, argv) == 0 )
	{
		long nProcess = 0;
		//UpdateDiskSize();

		/* 프로그램 메인루틴 */
		//rc = daem5003_process();
		do
		{
		nProcess = daem5003_file_del_process();
		printf("\n nProcess count : %ld\n", nProcess);
		}while((nProcess > 2999) || (nProcess == 0));
		
		/* 프로그램 종료루틴 */
		daem5003_process_term();
	}
	return(0);
}
/*****************************************************************************
*  End of file...
*****************************************************************************/
