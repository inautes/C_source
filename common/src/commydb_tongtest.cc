/******************************************************************************
 *   서브시스템 : 공통관리
 *   프로그램명 : commydb.cc
 *         기능 : 공통 MySQL모듈
 *         설명 : DB FUNCTION 호출하는 공통 함수
 *       작성자 : JDP
 *       작성일 : 2004/02/16
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
#include "commydb.h"

MYSQL *db_connect1(const char *dbname);
void db_do_query(MYSQL *db, const char *query);

const char *server_groups[] = {
	"test_libmysqld_SERVER", "embedded", "server", NULL
};

int test_main()
{
	MYSQL *one, *two;

	one = db_connect1("zangsi");

	two = db_connect1(NULL);

	db_do_query(one, "SHOW TABLE STATUS");
	db_do_query(two, "SHOW DATABASES");

	mysql_close(two);
	mysql_close(one);

	/* This must be called after all other mysql functions */
	mysql_server_end();

	return(EXIT_SUCCESS);
}

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

MYSQL *db_connect(char *dbname,char *ip,char *user,char *passwd)
{

	MYSQL *db = mysql_init(NULL);
	if (!db)
	{
		return NULL;
	}

	if (!mysql_real_connect(db, ip, user, passwd , dbname, 0, NULL, 0))
	{
		return NULL;
	}

	return db;
}

MYSQL *db_connect1(const char *dbname)
{
	MYSQL *db = mysql_init(NULL);
	if (!db)
		die(db, "mysql_init failed: no memory");
	/*
	 * Notice that the client and server use separate group names.
	 * This is critical, because the server will not accept the
	 * client's options, and vice versa.
	 */
	//mysql_options(db, MYSQL_SET_CHARSET_NAME, "euckr"); // utf8"); // euckr
	//mysql_options(db, MYSQL_INIT_COMMAND, "SET NAMES euckr");

	if (!mysql_real_connect(db, "183.110.46.120", "fanoman", "akqthtk)(^", dbname, 0, NULL, 0))
		die(db, "mysql_real_connect failed: %s", mysql_error(db));

	return db;
}

//------------------------------------------------------------------------------
// DB 연결
// @param dbname
// @return MYSQL *
//------------------------------------------------------------------------------
MYSQL *db_connect(char *dbname)
{
	printf("db_connect: start! \n");

	MYSQL *db = mysql_init(NULL);
	if (!db)
	{
		printf("db_connect: mysql_init failed(no memory)\n");
		return NULL;
	}
	//printf("db_connect: mysql_init ok! \n");
	//mysql_options(db, MYSQL_SET_CHARSET_NAME, "euckr"); // euckr
	//mysql_options(db, MYSQL_INIT_COMMAND, "SET NAMES euckr");

	if (!mysql_real_connect(db, "183.110.46.120", "fanoman", "akqthtk)(^", dbname, 0, NULL, 0))
	{
		printf("db_connect: mysql_real_connect failed(%s)\n", mysql_error(db));
		return NULL;
	}
	//printf("db_connect: mysql_real_connect ok! \n");

	return db;
}

//------------------------------------------------------------------------------
// 유료 컨텐츠 DB 연결
// @param dbname
// @return MYSQL *
//------------------------------------------------------------------------------
MYSQL *Cpr_db_connect(char *dbname)
{
	//printf("db_connect: start! \n");

	MYSQL *db = mysql_init(NULL);
	if (!db)
	{
		printf("db_connect: mysql_init failed(no memory)\n");
		return NULL;
	}
	//printf("db_connect: mysql_init ok! \n");
	//mysql_options(db, MYSQL_SET_CHARSET_NAME, "euckr"); // euckr
	//mysql_options(db, MYSQL_INIT_COMMAND, "SET NAMES euckr");

	if (!mysql_real_connect(db, "183.110.46.120", "fanoman", "akqthtk)(^", dbname, 0, NULL, 0))
	{
		printf("db_connect: mysql_real_connect failed(%s)\n", mysql_error(db));
		return NULL;
	}
	//printf("db_connect: mysql_real_connect ok! \n");

	return db;
}
//------------------------------------------------------------------------------
// Op DB 연결
// @param dbname
// @return MYSQL *
//------------------------------------------------------------------------------
MYSQL *Op_db_connect(char *dbname)
{
	//printf("db_connect: start! \n");

	MYSQL *db = mysql_init(NULL);
	if (!db)
	{
		printf("db_connect: mysql_init failed(no memory)\n");
		return NULL;
	}
	//printf("db_connect: mysql_init ok! \n");
	//mysql_options(db, MYSQL_SET_CHARSET_NAME, "euckr"); // euckr
	//mysql_options(db, MYSQL_INIT_COMMAND, "SET NAMES euckr");

	if (!mysql_real_connect(db, "183.110.46.120", "fanoman", "akqthtk)(^", dbname, 0, NULL, 0))
	{
		printf("db_connect: mysql_real_connect failed(%s)\n", mysql_error(db));
		return NULL;
	}
	//printf("db_connect: mysql_real_connect ok! \n");

	return db;
}



//------------------------------------------------------------------------------
// 하둡 통합 DB
// @param dbname
// @return MYSQL *
//------------------------------------------------------------------------------
MYSQL *Hadoop_db_connect(char *dbname)
{
	//printf("db_connect: start! \n");

	MYSQL *db = mysql_init(NULL);
	if (!db)
	{
		printf("db_connect: mysql_init failed(no memory)\n");
		return NULL;
	}
	//mysql_options(db, MYSQL_SET_CHARSET_NAME, "euckr"); // euckr
	//mysql_options(db, MYSQL_INIT_COMMAND, "SET NAMES euckr");

	if (!mysql_real_connect(db, "183.110.46.38", "fanoman", "akqthtk)(^", dbname, 0, NULL, 0))
	{
		printf("db_connect: mysql_real_connect failed(%s)\n", mysql_error(db));
		return NULL;
	}

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

	memset(tmpQuery, 0x00, sizeof(tmpQuery));
	strcpy(tmpQuery, "set autocommit=0");
	if (mysql_query(db, tmpQuery) != 0)
	{
		printf("tran_begin: %s [%d]\n", mysql_error(db), mysql_errno(db));
		return (-1);
	}
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
		printf("end_tran: %s [%d]\n", mysql_error(db), mysql_errno(db));
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
		printf("tran_commit: %s [%d]\n", mysql_error(db), mysql_errno(db));
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
		printf("tran_rollback: %s [%d]\n", mysql_error(db), mysql_errno(db));
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
