/******************************************************************************
 *   서브시스템 : 공통관리
 *   프로그램명 : commydb.cc
 *         기능 : 공통 MySQL모듈
 *         설명 : DB FUNCTION 호출하는 공통 함수
 *       작성자 : JDP
 *       작성일 : 2004/02/16
 *     수정이력 : 2008/10/21 - SUMDB 연결 추가
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#include <mysql.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "daemcom.h"
#include "commydb.h"



//------------------------------------------------------------------------------
// DB 연결
// @param dbname
// @return MYSQL *
//------------------------------------------------------------------------------
MYSQL *db_connect_to_main(char *dbname)
{
	MYSQL *db = mysql_init(NULL);
	if (!db)
	{
		ZzLOG(ERROR, "db_connect:mysql_init failed(no memory)");
		return NULL;
	}
	
	if (!mysql_real_connect(db, "192.168.0.38", "dmonc", "akqthtk!#%", dbname, 0, NULL, 0))
	{
		ZzLOG(ERROR, "db_connect:mysql_real_connect failed((%s))", mysql_error(db));
		return NULL;
	}
	
	return db;
}



//------------------------------------------------------------------------------
// DB 연결
// @param dbname
// @return MYSQL *
//------------------------------------------------------------------------------
MYSQL *db_connect_nodb(char *dbname)
{
	//printf("db_connect: start! \n");

	MYSQL *db = mysql_init(NULL);
	if (!db)
	{
		printf("db_connect: mysql_init failed(no memory)\n");
		return NULL;
	}
	//printf("db_connect: mysql_init ok! \n");

	if (!mysql_real_connect(db, "61.252.0.230", "dmonc", "akqthtk!#%", dbname, 0, NULL, 0))
	{
		printf("db_connect: mysql_real_connect failed(%s)\n", mysql_error(db));
		return NULL;
	}
	//printf("db_connect: mysql_real_connect ok! \n");
	
	return db;
}


//------------------------------------------------------------------------------
// CONTENTSROAD DB 연결
// @param dbname
// @return MYSQL *
//------------------------------------------------------------------------------
MYSQL *db_connect_controaddb(char *dbname) // 사용 안함.
{
	//printf("db_connect: start! \n");

	MYSQL *db = mysql_init(NULL);
	if (!db)
	{
		printf("db_connect: mysql_init failed(no memory)\n");
		return NULL;
	}
	//printf("db_connect: mysql_init ok! \n");

	if (!mysql_real_connect(db, "211.55.33.109", "contentsroad", "5621612", dbname, 0, NULL, 0))
	{
		printf("db_connect: mysql_real_connect failed(%s)\n", mysql_error(db));
		return NULL;
	}
	//printf("db_connect: mysql_real_connect ok! \n");
	
	return db;
}

//------------------------------------------------------------------------------
// 유료컨텐츠 DB 연결
// @param dbname
// @return MYSQL *
//------------------------------------------------------------------------------
MYSQL *db_connect_cprdb(char *dbname)
{
	//printf("db_connect: start! \n");

	MYSQL *db = mysql_init(NULL);
	if (!db)
	{
		printf("db_connect: mysql_init failed(no memory)\n");
		return NULL;
	}
	//printf("db_connect: mysql_init ok! \n");
	ZzLOG(ALWAY, "db_connect_cprdb 129 \n");

	if (!mysql_real_connect(db, "192.168.0.129", "dmonc", "akqthtk!#%", dbname, 0, NULL, 0))
	{
		ZzLOG(ALWAY,"db_connect: mysql_real_connect failed(%s)\n", mysql_error(db));
		return NULL;
	}
	//printf("db_connect: mysql_real_connect ok! \n");
	
	return db;
}

//------------------------------------------------------------------------------
// 유료컨텐츠 DB bck연결
// @param dbname
// @return MYSQL *
//------------------------------------------------------------------------------
MYSQL *db_connect_cprbckdb(char *dbname)
{
	//printf("db_connect: start! \n");

	MYSQL *db = mysql_init(NULL);
	if (!db)
	{
		printf("db_connect: mysql_init failed(no memory)\n");
		return NULL;
	}

	if (!mysql_real_connect(db, "192.168.0.129", "dmonc", "akqthtk!#%", dbname, 0, NULL, 0))
	{
		ZzLOG(ALWAY, "db_connect: mysql_real_connect failed(%s)\n", mysql_error(db));
		return NULL;
	}
	//printf("db_connect: mysql_real_connect ok! \n");
	
	return db;
}

//------------------------------------------------------------------------------
// DB 연결종료
// @param dbname
// @return MYSQL *
//------------------------------------------------------------------------------
void db_disconnect(MYSQL *db)
{
	if (db)
	{
		mysql_close(db);
	}
}

//------------------------------------------------------------------------------
// DB autocommit = 0
// @param MYSQL *db
// @return successful(0)
//------------------------------------------------------------------------------
int tran_begin(MYSQL *db)
{
	char tmpQuery[100];
	
//	ZzPRT(ALWAY, "tran_begin: start\n");
	
	memset(tmpQuery, 0x00, sizeof(tmpQuery));
	strcpy(tmpQuery, "set autocommit=0");

//	ZzPRT(ALWAY, "tran_begin: start1\n");
	if (mysql_query(db, tmpQuery) != 0)
	{
		ZzLOG(ERROR, "tran_begin: %s [%d]\n", mysql_error(db), mysql_errno(db));
		return (-1);
	}
//	ZzPRT(ALWAY, "tran_begin: ok\n");

	return (0);
}

//------------------------------------------------------------------------------
// DB autocommit = 1
// @param dbname
// @return MYSQL *
//------------------------------------------------------------------------------
int tran_end(MYSQL *db)
{
	char tmpQuery[100];
	
	memset(tmpQuery, 0x00, sizeof(tmpQuery));
	strcpy(tmpQuery, "set autocommit=1");
	if (mysql_query(db, tmpQuery) != 0)
	{
		ZzLOG(ERROR, "tran_end: %s [%d]\n", mysql_error(db), mysql_errno(db));
		return(-1);
	}
	return (0);
}

//------------------------------------------------------------------------------
// DB transation start를 setting한다.
// @param MYSQL *db
// @return successful(0)
//------------------------------------------------------------------------------
int tran_commit(MYSQL *db)
{
	char tmpQuery[100];
	
	memset(tmpQuery, 0x00, sizeof(tmpQuery));
	strcpy(tmpQuery, "commit");
	if (mysql_query(db, tmpQuery) != 0)
	{
		ZzLOG(ERROR, "tran_commit: %s [%d]\n", mysql_error(db), mysql_errno(db));
		return (-1);
	}
	tran_end(db);
	return (0);
}

//------------------------------------------------------------------------------
// DB transation start를 setting한다.
// @param MYSQL *db
// @return successful(0)
//------------------------------------------------------------------------------
int commit(MYSQL *db)
{
	char tmpQuery[100];
	
	memset(tmpQuery, 0x00, sizeof(tmpQuery));
	strcpy(tmpQuery, "commit");
	if (mysql_query(db, tmpQuery) != 0)
	{
		printf("tran_commit: %s [%d]\n", mysql_error(db), mysql_errno(db));
		return (-1);
	}
	return (0);
}

//------------------------------------------------------------------------------
// DB transation start를 setting한다.
// @param MYSQL *db
// @return successful(0)
//------------------------------------------------------------------------------
int tran_rollback(MYSQL *db)
{
	char tmpQuery[100];
	
	memset(tmpQuery, 0x00, sizeof(tmpQuery));
	strcpy(tmpQuery, "rollback");
	if (mysql_query(db, tmpQuery) != 0)
	{
		ZzLOG(ERROR, "tran_rollback: %s [%d]\n", mysql_error(db), mysql_errno(db));
		return (-1);
	}
	tran_end(db);
	return (0);
}

//-----------------------------------------------------------------------------
// Get an integer value from the result.
// @param x 0-based field index
// @return The integer value
//-----------------------------------------------------------------------------
long getint(MYSQL_ROW row, int x) {
	return row && row[x] ? atol(row[x]) : 0;
}

//-----------------------------------------------------------------------------
// Get a floating point number from the result.
// @param x 0-based field index
// @return The floating point value
//-----------------------------------------------------------------------------
double getnum(MYSQL_ROW row, int x) {
	return row && row[x] ? atof(row[x]) : 0;
}

//-----------------------------------------------------------------------------
// Get a string from the result.
// @param x 0-based field index
// @return String ptr
//-----------------------------------------------------------------------------
char *getstr(MYSQL_ROW row, int x) {
	if (row)
		return row[x] ? row[x] : (char *)"";
	else
		return NULL;
}


