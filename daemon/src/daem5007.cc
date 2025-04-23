/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5007
 *         기능 : T_CONTENTS_FILE_USER_CNT 초기화
 *         설명 : 1. 프로세스 처리 예정시간 : 05:10이후 daem5006 완료 이후에 수행.
 *                
 *     설치위치 : CMD01
 *
 *       작성자 : 염인탁
 *       작성일 : 2008/12.04
 *     수정이력 :
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

#include "daemcom.h"
#include "commydb.h"

#define	MAX_ROWS
#define _DEBUG_

int daem5007_process();
int daem5007_process_init(int argc, char **argv);
int daem5007_process_term();

MYSQL   *con;
MYSQL_RES   *res;
MYSQL_ROW   row;


int daem5007_process_init(int argc, char **argv)
{
    ZzInitGlobalVariable2("d_", "/logs/daemon");

    ZzLOG(ALWAY, "[daem5007]***************프로그램 시작***************\n");

    if (!(con=db_connect("zangsi")))
    {
        ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
        db_disconnect(con);
        return(-1);
    }
}

int daem5007_process_term()
{
    // DB close
	db_disconnect(con);

    ZzLOG(ALWAY, "[daem5007]***************프로그램 종료***************\n\n");

    return (0);
}

int daem5007_process()
{
	char szQuery[1000];
	int last_id;
	memset (szQuery, 0x00, sizeof(szQuery));
	
	last_id = 0;	

	while(1)
	{
    		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery,	"SELECT * FROM zangsi.T_CONTENTS_FILE_USER_CNT"
							" WHERE cur_user_cnt > 0"
							" and id > %d "
							" LIMIT 1 "
							, last_id);
		if(mysql_query(con,szQuery))
		{
			ZzLOG(ERROR, "process: SELECT zangsi.T_CONTENTS_FILE_USER_CNT error...\n");
			return 0;
		}
		if(!(res = mysql_store_result(con)))
		{
			ZzLOG(ERROR, "process: mysql_store_result\n");
			mysql_free_result(res);
			return 0;
		}
		if (mysql_num_rows(res)==0)
		{
			mysql_free_result(res);
			break;
		}
		if(row = mysql_fetch_row(res))
		{
			last_id = getint(row, 0);
		}
    		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery,	"UPDATE zangsi.T_CONTENTS_FILE_USER_CNT "
							" SET cur_user_cnt = 0 "
							" WHERE id=%d "
							, last_id);

/*		sprintf(szQuery,	"UPDATE zangsi.T_CONTENTS_FILE_USER_CNT "
							" SET cur_user_cnt = 0 "
							" WHERE cur_user_cnt > 0 "
							" LIMIT 100 "					
							);
*/
		if(mysql_query(con,szQuery))
		{
			ZzLOG(ERROR, "process: UPDATE zangsi.T_CONTENTS_FILE_USER_CNT error...\n");
			return 0;
		}
		if( mysql_affected_rows(con) < 1)
		{
			break;
		}
	}
}

int main(int argc, char **argv)
{                
	char    szTemp[1024];
	int     rc;
                 
	if ( daem5007_process_init(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = daem5007_process();
		/* 프로그램 종료루틴 */                    
		daem5007_process_term();
	}
	return(0);
}

