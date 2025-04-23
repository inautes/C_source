/******************************************************************************
 *   서브시스템 : daemon프로세스 
 *   프로그램명 : daem5025.cc 
 *         기능 : 요청자료 게시판 정리
 *         설명 : 1. 프로세스 처리 예정시간 - 04:00이후 daem5002 완료 이후에 수행.
 *                2.한달이 지난 요청자료 게시판을 조회하여 등록된 컨텐츠가 없는 게시물을 삭제한다
 *     설치위치 : CMD01
 *
 *       작성자 : 염인탁
 *       작성일 : 2009/03/27
 *     수정이력 : 
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#include <mysql.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

#include "daemcom.h"
#include "commydb.h"

#define	MAX_ROWS	1
#define	_DEBUG_
#define	STDIN	0
#define STDOUT	1
#define STDOUT2	2
#define	STDERR	3

MYSQL     *con;
MYSQL_RES *res;
MYSQL_ROW  row;

int get_reqboard_id(int nId);
int del_from_table();
int daem5025_get_max_id();

char gst_date[8+1];
bool gbIsUserDate      ;
int gnProcCnt;

int gSleepTime;

int daem5025_process_init(int argc, char **argv)
{
	ZzInitGlobalVariable2("daem5025", "/logs/daemon"); 
	ZzLOG(ALWAY, "[daem5025]*****************프로그램 시작*****************\n");  

	if (argc != 2) {
		goto arg_error;
	}

	memset(gst_date, 0x00, sizeof(gst_date));
	strcpy(gst_date, argv[1]);

	if (!(con=db_connect("zangsi")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1); 
	}
	return (0);
arg_error:
	ZzLOG(ERROR, "usage : %s YYYYMMDD Time\n", argv[0]);
	ZzLOG(ERROR, "        YYYYMMDD : 처리일자(00000000:시스템일자) Time : 슬립타임(밀리섹크)\n");
	return (-1);
}

int main(int argc, char **argv)
{
	char reg_date;
	int nId=0;

	if ( daem5025_process_init(argc, argv) != 0 ) {
		return (-1);
	}

	nId=daem5025_get_max_id();

	get_reqboard_id(nId);

	db_disconnect(con);
	ZzLOG(ALWAY, "[daem5025]*****************프로그램 종료(%d건 삭제)*****************\n",gnProcCnt);  
	return(0);
}

int daem5025_get_max_id()
{
	int nId;
	char szQuery[1000];
	if (strcmp(gst_date, "00000000") == 0)
	{	
		strcpy(szQuery, "SELECT max(id) FROM zangsi.T_CONTENTS_REQ "
				"where reg_date < date_format(date_add(now(), INTERVAL -30 DAY), '%Y%m%d') "
		      );
	}
	else
	{
		sprintf(szQuery, "SELECT max(id) FROM zangsi.T_CONTENTS_REQ "
				"where reg_date < '%s')", gst_date);
	}
	ZzLOG(ALWAY, "Select Max ID Query [ %s ]\n",szQuery);

	if(mysql_query(con, szQuery)){
		perror( mysql_error(con) );
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
		ZzLOG(ERROR, "%s, %d: SELECT max(id) error [%s]\n", __FILE__ , __LINE__, mysql_error(con));
		return (-1);
	}
	if(!(res =mysql_store_result(con))){
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
		ZzLOG(ERROR, "%s, %d: mysql	store result [%s]\n", __FILE__ , __LINE__, mysql_error(con));
		return (-1);
	}
	row = mysql_fetch_row(res);
	nId=getint(row,0);

	mysql_free_result(res);
	return nId;
}

int get_reqboard_id(int nId)
{
	char szQuery[1000];
	int n;
	char upload_yn[1+1];
	MYSQL_RES *res1;
	

	if(nId==0){
		nId=daem5025_get_max_id();
	}
	
	ZzLOG(ALWAY, " nId = %d \n", nId);

	while(1)
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "SELECT id , upload_yn FROM zangsi.T_CONTENTS_REQ "
				"where id < %d order by id desc "
				"limit 100 \n"
				, nId );
	
		if(mysql_query(con, szQuery)){
			ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			ZzLOG(ERROR, "%s, %d: [%s]\n", __FILE__ , __LINE__, mysql_error(con));
			return (-5);
		}
		if(!(res =mysql_store_result(con))){
			ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			ZzLOG(ERROR, "%s, %d: mysql store result [%s]\n", __FILE__ , __LINE__, mysql_error(con));
			return (-3);
		}
	
		if(mysql_num_rows(res)<1)
			return (-1);
	
		while( row = mysql_fetch_row(res) )
		{
			int nTId = 0;
			memset(szQuery, 0x00, sizeof(szQuery));
			strcpy(upload_yn, getstr(row,1));//upload_yn = row[1]
			nTId = getint(row,0);
			nId = nTId;
	
			if(strcmp(upload_yn, "N")==0)
			{
				sprintf(szQuery, "DELETE FROM zangsi.T_CONTENTS_REQ where id = '%d' ", nTId);
			}
			else
			{
				sprintf(szQuery, "SELECT id FROM zangsi.T_CONTENTS_INFO WHERE req_id=%d ",nTId);
	
				if(mysql_query(con, szQuery)){
					mysql_free_result(res);
					ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
					ZzLOG(ERROR, "%s, %d: SELECT id FROM CONTENTS_INFO [%s]\n", __FILE__ , __LINE__, mysql_error(con));
					return (-1);
				}
				res1=mysql_store_result(con);
				if(mysql_num_rows(res1)<1)
				{
					memset(szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, "DELETE FROM zangsi.T_CONTENTS_REQ where id = %d ", nTId);
				}
				else
				{
					memset(szQuery, 0x00, sizeof(szQuery));				
				}
				mysql_free_result(res1);
			}
	
			if(strlen(szQuery) < 1)
				continue; 
	
			ZzLOG(ALWAY, "%d,\n",nTId);
	
			if(mysql_query(con, szQuery)){
				mysql_free_result(res);
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
				ZzLOG(ERROR, "%s, %d: SELECT max(id) error [%s]\n", __FILE__ , __LINE__, mysql_error(con));
				return (-2);
			}
			gnProcCnt++;
			ZzLOG(ALWAY, " %d개 삭제\n", gnProcCnt);
		}
		ZzLOG(ALWAY, "sleep 1 second,\n");
		sleep(1);
	}
	mysql_free_result(res);
	return nId;
}

