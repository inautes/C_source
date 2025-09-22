/******************************************************************************
 *   ����ý��� : �������
 *   ���α׷��� :
 *         ��� : ���� MySQL���
 *         ���� : DB CLASS
 *       �ۼ��� :
 *       �ۼ��� : 2004/02/16
 *     �����̷� :
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/


#ifndef _MYSQLDBCLASS_H
#define _MYSQLDBCLASS_H

#define _MYSQLDBCLASS_V1_0

#include <mysql.h>
#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> //memset
#include <stdarg.h> //va_list

#define DEFAULT_DB_IP ""
#define DEFAULT_SUB_DB_IP ""
#define DEFAULT_DB_USER ""
#define DEFAULT_DB_PASSWD ""
#define DEFAULT_DB_NAME ""


class CMysqlDB
{


	private:
		char* m_pUtf8String;


		MYSQL *m_pDatabase;
		MYSQL_RES *m_pRes;
		MYSQL_ROW m_pRow;
		bool m_bDebug;
		bool m_bAutoRollback;
		bool m_bDoTran;
		char m_szErrorMsg[8096];
		bool m_bCanStoreResult;
		bool m_bIsConnect;
		unsigned int m_nConnectTimeoutSec;
		char m_curQuery[8096];

	private:
		void getLastError(const char* pQuery = NULL);
		void getResultOk();

	public:
		CMysqlDB();
		virtual ~CMysqlDB();

	public:
		void  setDebug(bool bDebugMode = false);
		void  setAutoRollback(bool bAutoRollback = true);
		void  setConnectTimeout(unsigned int unTimeOut = 5 );
		void  setUseDisconnect(bool bUse = true);

		MYSQL* connect_db( const char* pDB_NAME = DEFAULT_DB_NAME, const char* pHOST_IP = DEFAULT_DB_IP , unsigned int nHOST_PORT = 3306 , const char* pUSER_ID =DEFAULT_DB_USER , const char* pUSER_PASSWD = DEFAULT_DB_PASSWD );
		void free_database();
		void free_result();
		bool isConnect();
		void disconnect_db();

	public:
		int tran_begin();
		int tran_end();
		int tran_commit();
		int tran_rollback();
		long getint(int colum_idx ) ;
		double getnum(int colum_idx) ;
		char *getstr(int colum_idx) ;
		//int query( char* pQuery);
		int query( const char* pQuery, ... );
		MYSQL_ROW fetch_row(void) ;
		double insert_id(void) ;
		long num_rows(void) ;
		long num_rows(const char* pQuery,...) ;
		//MYSQL_RES *store_result(char *pQuery = NULL) ;
		MYSQL_RES *store_result(const char* pQuery = NULL ,...) ;

		char* getQueryString();
		char* getErrorMessage();
		int	getErrorCode();

		void setCon(MYSQL* pCon);
		MYSQL* getCon();
};

#endif
