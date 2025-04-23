/******************************************************************************
 *   서브시스템 : 공통모듈
 *   프로그램명 : commydb.h
 *         기능 : MYSQL 관련 사항
 *         설명 :
 *       작성자 : LEE
 *       작성일 : 2004/02/16
 *     수정이력 :
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#ifndef	_COMMYDB_H_
#define	_COMMYDB_H_
#include "mysql.h"

MYSQL *db_connect_local(char *dbname);
MYSQL *db_connect(char *dbname);
MYSQL *db_connect2(char *dbname);
MYSQL *db_connect_sub1(char *dbname);
MYSQL *db_connect_nodb(char *dbname);
MYSQL *db_connect_backup(char *dbname);
MYSQL *db_connect_cprdb(char *dbname);
MYSQL *db_connect_sumdb(char *dbname);
MYSQL *db_connect_logdb(char *dbname);
MYSQL *db_connect_omdb(char *dbname);
MYSQL *db_connect_search_db(char *dbname);
MYSQL *db_connect_subdb(char *dbname);

void db_disconnect(MYSQL *db);
int tran_begin(MYSQL *db);
int tran_end(MYSQL *db);
int tran_commit(MYSQL *db);
int commit(MYSQL *db);
int tran_rollback(MYSQL *db);
long getint(MYSQL_ROW row, int x);
double getnum(MYSQL_ROW row, int x);
char *getstr(MYSQL_ROW row, int x);

#endif
/*******************************************************************************
 * End of file...
 ******************************************************************************/
