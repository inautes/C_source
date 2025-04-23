/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem8004.cc
 *         기능 : 업로드시 쓰레기 파일 삭제
 *         설명 : 1. T_CONTENTS_TEMP를 기준을 파일삭제
 *                2. 입력된 처리일자 이전까지 처리한다.
 *                   단 입력값이 00000000 이면 현재일자 - 15로 처리한다.
 *                3. T_CONTENTS_TEMP의 파일정보로 T_CONTENTS_FILE_DEL_TEMP 검색
 *                   - YES -> T_CONTENTS_TEMP 정보 삭제
 *                   - NO  -> T_CONTENTS_TEMP 정보 삭제, 파일삭제
 *                    
 *     설치위치 : svr에 각각 위치한다.
 *
 *       작성자 : LEE
 *       작성일 : 2004/11/25
 *     수정이력 :

daem8004_process();

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
#include <unistd.h>
#include <spawn.h> //for posix_spawn()


#include "daemcom.h"
#include "commydb.h"

#define  MAX_ROWS	1
#define _DEBUG_

int daem8004_process();
int daem8004_process_db();
int daem8004_process_init(int argc, char **argv);
int daem8004_process_term();
void daem8004_signal(int nSignal);

MYSQL     *con;
MYSQL_RES *res;
MYSQL_ROW  row;

MYSQL     *con_backup;      // 관리자DB


char   gserver_id [  5+1];	// 서버ID
char   gcont_gu   [  2+1];	// 서버구분
char   gproc_date [  8+1];	// 처리일자
char   gdel_date  [  8+1];	// 삭제예정일
char   gfile_path [255+1];	// 파일결로
char   gfile_name [ 30+1];	// 파일이름
unsigned int gid         ;	// 컨텐츠ID
double gfile_size        ;	// 파일크기

bool gbIsDel = true;

//******************************************************************************
//* daem8004 main
//******************************************************************************
int daem8004_process()
{
	char szQuery[1000];  // query string
	int nRowcnt=0;


    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "gproc_date : [%s]\n", gproc_date);  
    ZzLOG(ALWAY, "gserver_id : [%s]\n", gserver_id);  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*------------------------------------------------------------------------*/
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery,"SELECT id, sfile_path, sfile_name"
                    "   FROM zangsi.T_CONTENTS_TEMP"
                    "  WHERE server_id = '%s' "
                    "    AND reg_date  < '%s' ; /* daem8004 */ "
                    , gserver_id
                    , gproc_date
                    );

	#ifdef _DEBUG_
	printf("%s\n",szQuery );
    #endif
        
	if (mysql_query(con_backup, szQuery)){
	    ZzLOG(ERROR, "daem8004_process: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con_backup), mysql_error(con_backup));
		return -1;
	}
	
	if (!(res = mysql_store_result(con_backup))) {
	    ZzLOG(ERROR, "daem8004_process: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con_backup), mysql_error(con_backup));
		return -1;
	}
 	if (mysql_num_rows(res)==0)	
 	{
		mysql_free_result(res);
	    ZzLOG(ALWAY, "daem8004_process: 처리할 자료가 없습니다.\n");
	    return -1;
	}

    ZzLOG(ALWAY, "daem8004_process: select_cnt(%d)\n", mysql_num_rows(res));
	
	nRowcnt = 0;
	while((row = mysql_fetch_row(res))) {
		gid        = 0;
		memset(&gfile_path, 0x00, sizeof(gfile_path));
		memset(&gfile_name, 0x00, sizeof(gfile_name));

		gid        = getint(row,0);
		strcpy(gfile_path, getstr(row, 1));
		strcpy(gfile_name, getstr(row, 2));

		if (daem8004_process_db() != 0)
		{
			mysql_free_result(res);
			return -1;
		}
	}
	mysql_free_result(res);

	return 0;
}


//******************************************************************************
//* daem8004 db 처리로직
//******************************************************************************
int daem8004_process_db()
{
	char szQuery[1000];		// query string
	char szSysQuery [612];	// system commend
	char szfullfile [612];	// file full name
	char strSystemQuery[612];
	int  nRowcnt;			// select row count
	int  ret;
	int  nSearchCnt;		// select count
	struct stat64 statbuf;

	MYSQL_RES *lres;
	MYSQL_ROW  lrow;

	if ((strcmp(gfile_path, "")!=0) && (strcmp(gfile_name, "")!=0))
	{
		//--------------------------------------------------------------------------
		//  사용되고 있는 컨텐츠 인지 검사
		//--------------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery,"SELECT count(*) "
	                    "   FROM zangsi_sum.T_CONTENTS_FILE_DEL_TEMP " // 현재 서비스중인 목록.
	                    "  WHERE file_path = '%s' "
	                    "    AND file_name = '%s' ; /* daem8004 */ "
	                    , gfile_path
	                    , gfile_name
	                    );

		if (mysql_query(con_backup, szQuery)){
		    ZzLOG(ERROR, "daem8004_process_db: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con_backup), mysql_error(con_backup));
			return -1;
		}
	
		if (!(lres = mysql_store_result(con_backup))) {
		    ZzLOG(ERROR, "daem8004_process_db: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)",mysql_errno(con_backup), mysql_error(con_backup));
			return -1;
		}
	 	if (mysql_num_rows(lres)==0)	
	 	{
			mysql_free_result(lres);
		    ZzLOG(ALWAY, "daem8004_process_db: 처리할 자료가 없습니다.\n");
		    return -1;
			//break;
		}
	
		lrow = mysql_fetch_row(lres);
		nSearchCnt = 0;
		nSearchCnt = getint(lrow,0);
		mysql_free_result(lres);
		
		
		// 사용 안하고 있음
		if (nSearchCnt == 0)
		{
			memset(szSysQuery, 0x00, sizeof(szSysQuery));
			memset(szfullfile, 0x00, sizeof(szfullfile));

			strcpy(szfullfile, gfile_path);
			strcat(szfullfile, "/");
			strcat(szfullfile, gfile_name);

			if (strcmp(szfullfile, "/")==0)
				return 0;
	
			//파일이 있는지 검사한다.
			ret = lstat64(szfullfile, &statbuf); 
			if( ret == 0 ) {
				memset(strSystemQuery,0x00,sizeof(strSystemQuery));
				strcpy(strSystemQuery,"rm -r -f ");		
				strcat(strSystemQuery,szfullfile);

				char *argvv[] = {"/bin/rm", "-r", "-f", szfullfile};
				
				
				if(gbIsDel == true)
					ret = posix_spawn(NULL, "/bin/rm", NULL, NULL, argvv, NULL);
				
				//rmLOG(ALWAY, "%s\n", strSystemQuery );
				
				ZzPRT(ALWAY, " id[%ld]: %s delete ret(%d) errno(%d)[%s]!!!\n", gid, szfullfile, ret, errno, strerror(errno));
				ZzLOG(ALWAY, " id[%ld]: %s delete ret(%d) errno(%d)[%s]!!!\n", gid, szfullfile, ret, errno, strerror(errno));
			}
			else {
				ZzPRT(ALWAY, " id[%ld]: %s not found skip!!!\n", gid, szfullfile);
			}
		}
		else {
			ZzPRT(ALWAY, " id[%ld]: %s 사용중 skip!!!\n", gid, szfullfile);
		}
	}

	//--------------------------------------------------------------------------
	//  사용되고 있는 컨텐츠 인지 검사
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi.T_CONTENTS_TEMP "
	                 " WHERE id = %ld ; /* daem8004 */ "
	               , gid);

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem8004_process_db: DELETE T_CONTENTS_TEMP error...\n");
		ZzLOG(ERROR, "daem8004_process_db: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "%s\n",szQuery);
		goto daem8004_process_db_err;
    }
	ZzPRT(ALWAY, " id[%ld]: DELETE T_CONTENTS_TEMP OK!!!\n", gid);
	ZzLOG(ALWAY, " id[%ld]: DELETE T_CONTENTS_TEMP OK!!!\n", gid);

	return 0;

daem8004_process_db_err:
	ZzLOG(ERROR, "daem8004_process_db: [%d](%s)",mysql_errno(con), mysql_error(con));
	return -1;	
}

/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem8004_get_sysdate()
{
	char szQuery[1000];		// query string
	char sztemp [100];      // query temp

	if (strcmp(gproc_date, "00000000") != 0)
		return 0;

	memset(szQuery, 0x00, sizeof(szQuery));
	strcpy(szQuery, "SELECT date_format(date_add(now(), INTERVAL -15 DAY),'%Y%m%d') ; /* daem8004 */ ");

	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "sysdate: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		ZzPRT(ERROR, "sysdate: mysql_query error...\n");
		
		return -1;
	}
	
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "sysdate: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		ZzPRT(ERROR, "sysdate: mysql_store_result error...\n");
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
	memset(gproc_date, 0x00, sizeof(gproc_date));

	strcpy(gproc_date,   getstr(row, 0));
	
	mysql_free_result(res);

	return 0;
}


/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem8004_process_init(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("daem8004", "/logs/daemon"); 

    ZzLOG(ALWAY, "***************프로그램 시작***************\n");  

    // 파라미터 값 설정 및 초기화
    if (argc != 3){
    	
    	goto arg_error;
    }

    // 서버이름 얻기
	memset(gserver_id, 0x00, sizeof(gserver_id));
	strcpy(gserver_id, argv[2]);

	if (strcmp(gserver_id, "")==0)
	{
		strcpy(gserver_id, getenv("HOSTNAME"));
		goto arg_error;
	}
	
	if (strcmp(argv[1], "test") == 0)
	{
		gbIsDel = false;
	}
	
	

	ZzPRT(ALWAY, "gserver_id : (%s) , DB 연결 시도\n", gserver_id);

	//--------------------------------------------------------------------------
	// DB 연결 main
	//--------------------------------------------------------------------------
	if (!(con=db_connect("zangsi")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		ZzPRT(ALWAY, "DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1); 
	}
	ZzPRT(ALWAY, "main DB 접속완료\n");

	//--------------------------------------------------------------------------
	// DB 연결 backup
	//--------------------------------------------------------------------------
	if (!(con_backup=db_connect_backup("zangsi")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		ZzPRT(ALWAY, "DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con_backup);
	   	return(-1); 
	}
	ZzPRT(ALWAY, "main backup 접속완료\n");


	ZzPRT(ALWAY, "처리 일자 얻기 시도\n");
	/* 처리일자 */
	memset(gproc_date, 0x00, sizeof(gproc_date));
	
	if(gbIsDel == true)
		strcpy(gproc_date, argv[1]);
	else
		strcpy(gproc_date, "00000000");	
	
	ret=daem8004_get_sysdate();
	if (ret < 0)
	{
		ZzPRT(ALWAY, "시스템 날짜를 얻지 못하였습니다....\n");
		db_disconnect(con);
		db_disconnect(con_backup);
		return -1;
	}

    return (0);


arg_error:
    ZzLOG(ERROR, "usage : %s YYYYMMDD ServerID[ WES01 ]\n", argv[0]);
    ZzLOG(ERROR, "        YYYYMMDD(처리일자)-> '00000000'인경우 system일자로처리\n");
    ZzLOG(ERROR, "        XXXXX   (서버ID  )-> '00000'   인경우 getenv(SERVER_ID)참조\n");

    ZzPRT(ERROR, "usage : %s YYYYMMDD XXXXX\n", argv[0]);
    ZzPRT(ERROR, "        YYYYMMDD(처리일자)-> '00000000'인경우 system일자로처리\n");
    ZzPRT(ERROR, "        XXXXX   (서버ID  )-> '00000'   인경우 getenv(SERVER_ID)참조\n");
    return -1;
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daem8004_process_term()
{
    // DB close
	db_disconnect(con);
	db_disconnect(con_backup);
    ZzLOG(ALWAY, "***************프로그램 종료***************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem8004_signal(int nSignal)
{
    daem8004_process_term();
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
	signal(SIGTERM, daem8004_signal);
	signal(SIGINT,  daem8004_signal);
	signal(SIGQUIT, daem8004_signal);
	signal(SIGKILL, daem8004_signal);
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
	if ( daem8004_process_init(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daem8004_process();
		/* 프로그램 종료루틴 */                    
		daem8004_process_term();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
