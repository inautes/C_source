/******************************************************************************
 *   서브시스템 : 공통관리
 *   프로그램명 : commydb.cc
 *         기능 : 공통 MySQL모듈
 *         설명 : DB FUNCTION 호출하는 공통 함수
 *       작성자 : 
 *       작성일 : 
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
#include "daemcom.h"
#include "commydb.h"

void db_do_query(MYSQL *db, const char *query);

const char *server_groups[] = {
	"test_libmysqld_SERVER", "embedded", "server", NULL
};

static void die(MYSQL *db, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)putc('\n', stderr);
	if (db)
		db_disconnect(db);
	exit(EXIT_FAILURE);
}


//------------------------------------------------------------------------------
// DB 연결
// @param dbname
// @return MYSQL *
//------------------------------------------------------------------------------
MYSQL *db_connect(char *dbname)
{
	MYSQL *db = mysql_init(NULL);
	if (!db)
	{
		ZzLOG(ERROR, "db_connect:mysql_init failed(no memory)");
		return NULL;
	}
	
	if (!mysql_real_connect(db, "192.168.171.144", "root", "qwepoi123", dbname, 0, NULL, 0))
		
	{
		ZzLOG(ERROR, "db_connect:mysql_real_connect failed((%s))", mysql_error(db));
		return NULL;
	}
	
	return db;
}

MYSQL *db_connect_local(char *dbname)
{
	MYSQL *db = mysql_init(NULL);
	if (!db)
	{
		ZzLOG(ERROR, "db_connect:mysql_init failed(no memory)");
		return NULL;
	}
	
	if (!mysql_real_connect(db, "192.168.171.144", "root", "qwepoi123", dbname, 0, NULL, 0))
	{
		ZzLOG(ERROR, "db_connect:mysql_real_connect failed((%s))", mysql_error(db));
		return NULL;
	}
	
	return db;
}


//------------------------------------------------------------------------------
//bck DB 연결
// @param dbname
// @return MYSQL *
//------------------------------------------------------------------------------
MYSQL *db_connect_backup(char *dbname)
{
	//printf("db_connect: start! \n");

	MYSQL *db = mysql_init(NULL);
	if (!db)
	{
		printf("db_connect: mysql_init failed(no memory)\n");
		return NULL;
	}
	//printf("db_connect: mysql_init ok! \n");
	
	if (!mysql_real_connect(db, "192.168.171.144", "root", "qwepoi123", dbname, 0, NULL, 0))
	{
		printf("db_connect: mysql_real_connect failed(%s)\n", mysql_error(db));
		return NULL;
	}
	//printf("db_connect: mysql_real_connect ok! \n");
	
	return db; 
}

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
	
	if (!mysql_real_connect(db, "192.168.171.144", "root", "qwepoi123", dbname, 0, NULL, 0))
	{
		printf("db_connect: mysql_real_connect failed(%s)\n", mysql_error(db));
		return NULL;
	}
	//printf("db_connect: mysql_real_connect ok! \n");
	
	return db;
}

//------------------------------------------------------------------------------
// 거래정보 백업 DB 연결
// @param dbname
// @return MYSQL *
//------------------------------------------------------------------------------
MYSQL *db_connect_sumdb(char *dbname)
{
	//printf("db_connect: start! \n");

	MYSQL *db = mysql_init(NULL);
	if (!db)
	{
		printf("db_connect: mysql_init failed(no memory)\n");
		return NULL;
	}
	//printf("db_connect: mysql_init ok! \n");

	if (!mysql_real_connect(db, "192.168.171.144", "root", "qwepoi123", dbname, 0, NULL, 0))
	{
		printf("db_connect: mysql_real_connect failed(%s)\n", mysql_error(db));
		return NULL;
	}
	//printf("db_connect: mysql_real_connect ok! \n");
	
	return db;
}

//------------------------------------------------------------------------------
// LOG DB 연결
// @param dbname
// @return MYSQL *
//------------------------------------------------------------------------------
MYSQL *db_connect_logdb(char *dbname)
{
	//printf("db_connect: start! \n");

	MYSQL *db = mysql_init(NULL);
	if (!db)
	{
		printf("db_connect: mysql_init failed(no memory)\n");
		return NULL;
	}
	//printf("db_connect: mysql_init ok! \n");

	
	if (!mysql_real_connect(db, "192.168.171.144", "root", "qwepoi123", dbname, 0, NULL, 0))
	{
		printf("db_connect: mysql_real_connect failed(%s)\n", mysql_error(db));
		return NULL;
	}
	//printf("db_connect: mysql_real_connect ok! \n");
	
	return db;
}
//------------------------------------------------------------------------------
// OM DB 연결(담아가기)
// @param dbname
// @return MYSQL *
//------------------------------------------------------------------------------
MYSQL *db_connect_omdb(char *dbname)
{
	//printf("db_connect: start! \n");

	MYSQL *db = mysql_init(NULL);
	if (!db)
	{
		printf("db_connect: mysql_init failed(no memory)\n");
		return NULL;
	}
	//printf("db_connect: mysql_init ok! \n");

	
	if (!mysql_real_connect(db, "192.168.171.144", "root", "dlwjarhd1029", dbname, 0, NULL, 0))
	{
		printf("db_connect: mysql_real_connect failed(%s)\n", mysql_error(db));
		return NULL;
	}
	//printf("db_connect: mysql_real_connect ok! \n");
	
	return db;
}
//------------------------------------------------------------------------------
// S2 관리자 DB 연결
// @param dbname
// @return MYSQL *
//------------------------------------------------------------------------------
MYSQL *db_connect_search_db(char *dbname)
{
	//printf("db_connect: start! \n");

	MYSQL *db = mysql_init(NULL);
	if (!db)
	{
		printf("db_connect: mysql_init failed(no memory)\n");
		return NULL;
	}
	//printf("db_connect: mysql_init ok! \n");
	
	if (!mysql_real_connect(db, "192.168.171.144", "root", "qndnwk123", dbname, 0, NULL, 0))
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

	if (!mysql_real_connect(db, "192.168.171.222", "root", "qwepoi123", dbname, 0, NULL, 0))
	{
		printf("db_connect: mysql_real_connect failed(%s)\n", mysql_error(db));
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

void db_do_query(MYSQL *db, const char *query)
{
	if (mysql_query(db, query) != 0)
		goto err;

	if (mysql_field_count(db) > 0)
	{
		MYSQL_RES   *res;
		MYSQL_ROW    row, end_row;
		int num_fields;

		if (!(res = mysql_store_result(db)))
			goto err;
		num_fields = mysql_num_fields(res);
		while ((row = mysql_fetch_row(res)))
		{
			(void)fputs(">> ", stdout);
			for (end_row = row + num_fields; row < end_row; ++row)
				(void)printf("%s\t", row ? (char*)*row : "NULL");
			(void)fputc('\n', stdout);
		}
		(void)fputc('\n', stdout);
		mysql_free_result(res);
	}
	else
		(void)printf("Affected rows: %lld\n", mysql_affected_rows(db));

	return;
err:
	die(db, "db_do_query failed: %s [%s]", mysql_error(db), query);
}
