/******************************************************************************
 *   서브시스템 : 공통관리
 *   프로그램명 : commydb.cc
 *         기능 : 공통 MySQL모듈
 *         설명 : DB FUNCTION 호출하는 공통 함수
 *     수정이력 :
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/

#include "MysqlDB.h"
//#include "apstruct.h"
#include "apdefine.h"
#include "comcomm.h"
#include "comconf.h"

CMysqlDB::CMysqlDB()
{
	m_pDatabase = NULL;
	m_pRes = NULL;
	m_pRow = NULL;
	m_bAutoRollback = false;
	m_bDebug = true;
	m_bDoTran = false;
	m_bCanStoreResult = false;
	m_bIsConnect = false;
	memset(m_szErrorMsg,0x00,sizeof(m_szErrorMsg));
	memset(m_curQuery,0x00,sizeof(m_curQuery));
}

CMysqlDB::~CMysqlDB()
{
	free_database();
	disconnect_db();

}



void CMysqlDB::getLastError(const char* pQuery /*= NULL*/)
{

	memset(m_szErrorMsg,0x00,sizeof(m_szErrorMsg));
	if( m_pDatabase != NULL )
	{
		if( pQuery != NULL )
			sprintf(m_szErrorMsg,"\nquery [ %s ] | errno [ %d ] | messge  [ %s ]\n",pQuery,mysql_errno(m_pDatabase), mysql_error(m_pDatabase));
		else
			sprintf(m_szErrorMsg,"\nerrno = %d | messge = %s\n",mysql_errno(m_pDatabase), mysql_error(m_pDatabase));
	}

}
void CMysqlDB::getResultOk()
{

	m_szErrorMsg[0] = '\0';


	if( m_pDatabase != NULL )
	{
		sprintf(m_szErrorMsg, "작업을 완료 하였습니다.");
		if( m_bDebug )
		{
			//sprintf(m_szErrorMsg , "LINE [ %d ] FILE [ %s ] MSG[ %s ] \n",__LINE__ , __FILE__ , "작업을 완료 하였습니다.");
			//printf("%s",m_szErrorMsg);
		}
	}
}

void  CMysqlDB::setDebug(bool bDebugMode)
{
	m_bDebug = bDebugMode;
}
//AutoRollback설정
void  CMysqlDB::setAutoRollback(bool bAutoRollback)
{
	if( m_bDebug )
		infLOG(ALWAY,"setAutoRollback = %d \n",bAutoRollback);

	m_bAutoRollback = bAutoRollback;
}
void   CMysqlDB::setConnectTimeout(unsigned int unTimeOut)
{
	m_nConnectTimeoutSec = unTimeOut;
}
MYSQL* CMysqlDB::connect_db( const char* pDB_NAME, const char* pHOST_IP  , unsigned int nHOST_PORT , const char* pUSER_ID , const char* pUSER_PASSWD )
{
	if( m_bDebug )
		infLOG(ALWAY,"connect to db [%s] host [%s] port [%d] user [%s] passwd [%s]\n",pDB_NAME,pHOST_IP,nHOST_PORT,pUSER_ID,pUSER_PASSWD);

	free_database();

	m_pDatabase = mysql_init(NULL);
	if (!m_pDatabase)
	{
		getLastError();
		if( m_bDebug )
		{
			infLOG(ALWAY,"%s\n",m_szErrorMsg);

		}
		m_pDatabase = NULL;
		return NULL;
	}

	if( m_nConnectTimeoutSec > 0 )
	{
		mysql_options(m_pDatabase, MYSQL_OPT_CONNECT_TIMEOUT, (char*)&m_nConnectTimeoutSec);
	}

	if (!mysql_real_connect(m_pDatabase, pHOST_IP,pUSER_ID, pUSER_PASSWD, pDB_NAME, nHOST_PORT, NULL, 0))
	{

		getLastError();
		if( m_bDebug )
		{
			infLOG(ALWAY,"%s\n",m_szErrorMsg);

		}
		m_pDatabase = NULL;
		return NULL;
	}

	//getResultOk();
	m_bIsConnect = true;

	return m_pDatabase;
}

void CMysqlDB::setCon(MYSQL* pCon)
{
	m_pDatabase = pCon;
}

void CMysqlDB::disconnect_db()
{

	if( m_bIsConnect == true)
	{
		if( m_pDatabase != NULL )
		{
			mysql_close(m_pDatabase);
		}
		m_pDatabase = NULL;
	}

	m_bIsConnect = false;

}

void CMysqlDB::setUseDisconnect(bool bUse)
{
	if( m_pDatabase != NULL )
	{
		m_bIsConnect = bUse;
	}

}
void CMysqlDB::free_database()
{

	if( m_bDoTran == true)
	{
		if( m_bAutoRollback == true)
			tran_rollback();
		else
			tran_commit();
		tran_end();
	}
	free_result();

	m_bAutoRollback = false;

	memset(m_szErrorMsg,0x00,sizeof(m_szErrorMsg));

	//getResultOk();
}
//m_pRes메모리 해제
void CMysqlDB::free_result()
{
	if( m_pRes != NULL )
	{
		mysql_free_result(m_pRes);
		//delete m_pRes;
	}
	m_pRes = NULL;
	m_pRow = NULL;
	//getResultOk();
}
//트랜잭션 시작
int CMysqlDB::tran_begin()
{
	if( m_bDebug )
		infLOG(ALWAY,"%s\n","tran_begin()");



	memset(m_curQuery, 0x00, sizeof(m_curQuery));
	strcpy(m_curQuery, "set autocommit=0");//inno트랜잭션을 사용하기 위해 처음에 설정
	if (mysql_query(m_pDatabase, m_curQuery) != 0)//sql명령 실행. 성공시 0을 리턴
	{
		getLastError();
		if( m_bDebug )
		{
			infLOG(ALWAY,"%s\n",m_szErrorMsg);

		}
		return (-1);
	}

	m_bDoTran = true;
	//getResultOk();
	return (0);
}
//트랜잭션 끝
int CMysqlDB::tran_end()
{
	if( m_bDebug )
		infLOG(ALWAY,"%s\n","tran_end()");


	memset(m_curQuery, 0x00, sizeof(m_curQuery));
	strcpy(m_curQuery, "set autocommit=1");
	if (mysql_query(m_pDatabase, m_curQuery) != 0)
	{
		getLastError();
		if( m_bDebug )
		{
			infLOG(ALWAY,"%s\n",m_szErrorMsg);

		}
		return (-1);
	}
	m_bDoTran = false;
	//getResultOk();
	return (0);
}

int CMysqlDB::tran_commit()
{
	if( m_bDebug )
		infLOG(ALWAY,"%s\n","tran_commit()");


	memset(m_curQuery, 0x00, sizeof(m_curQuery));
	strcpy(m_curQuery, "commit");
	if (mysql_query(m_pDatabase, m_curQuery) != 0)
	{
		if( m_bAutoRollback )
			tran_rollback();
		getLastError();
		if( m_bDebug )
		{
			infLOG(ALWAY,"%s\n",m_szErrorMsg);

		}
		return (-1);
	}
	//getResultOk();
	return tran_end();

}

int CMysqlDB::tran_rollback()
{
	if( m_bDebug )
		infLOG(ALWAY,"%s\n","tran_rollback()");


	memset(m_curQuery, 0x00, sizeof(m_curQuery));
	strcpy(m_curQuery, "rollback");
	if (mysql_query(m_pDatabase, m_curQuery) != 0)
	{
		getLastError();
		if( m_bDebug )
		{
			infLOG(ALWAY,"%s\n",m_szErrorMsg);

		}
		return (-1);
	}
	//getResultOk();
	return 0;//tran_end();
}


//-----------------------------------------------------------------------------
// Get an integer value from the result.
// @param x 0-based field index
// @return The integer value
//-----------------------------------------------------------------------------
long CMysqlDB::getint(int colum_idx)
{
	//if( m_bDebug )
		//printf("getint(%d)",colum_idx);

	if( m_pRes != NULL && m_pRow != NULL && m_pRow[colum_idx]  )
		return atol(m_pRow[colum_idx])  ;
		//return m_pRow && m_pRow[colum_idx] ? atol(m_pRow[colum_idx]) : 0;
	return 0;
}
//-----------------------------------------------------------------------------
// Get a floating point number from the result.
// @param x 0-based field index
// @return The floating point value
//-----------------------------------------------------------------------------
//컬럼 정보 리턴
double CMysqlDB::getnum(int colum_idx)
{
	//if( m_bDebug )
		//printf("getnum(%d)",colum_idx);

	if( m_pRes != NULL && m_pRow != NULL && m_pRow[colum_idx]  )
		return atof(m_pRow[colum_idx]);
		//return m_pRow && m_pRow[colum_idx] ? atof(m_pRow[colum_idx]) : 0;
	return 0;
}
//-----------------------------------------------------------------------------
// Get a string from the result.
// @param x 0-based field index
// @return String ptr
//-----------------------------------------------------------------------------
char* CMysqlDB::getstr(int colum_idx)
{
	//if( m_bDebug )
		//printf("getstr(%d)",colum_idx);

	if (m_pRes != NULL && m_pRow != NULL &&  m_pRow[colum_idx] )
		return m_pRow[colum_idx];
		//return m_pRow && m_pRow[colum_idx] ? m_pRow[colum_idx] : (char *)"";
	return (char *)"";
}

//쿼리문 입력
int CMysqlDB::query(const char* pQuery, ... )
{
	memset(m_curQuery, 0x00, sizeof(m_curQuery));

	va_list args;// 가변변수 리스트 선언
	va_start(args, pQuery);//사용시작
	vsprintf(m_curQuery, pQuery, args);//m_curQuery에 pQuery를 설정
	va_end(args);

	if( m_bDebug )
		infLOG(ALWAY,"query [ %s ]  \n",m_curQuery);
	//query 시작
	if (mysql_query(m_pDatabase, m_curQuery) != 0)
	{
		getLastError(m_curQuery);
		if( m_bDebug )
		{

			infLOG(ALWAY,"%s\n",m_szErrorMsg);

		}
		if( m_bAutoRollback )
			tran_rollback();

		m_bCanStoreResult = false;
		return -1;
	}
	m_bCanStoreResult = true;
	return 0;
}

//auto_increment가 걸려있는 타입의 필드값을 반환
double CMysqlDB::insert_id(void)
{
	static double dInsertIdRet = 0;
	if( m_bDebug )
		infLOG(ALWAY,"%s\n","insert_id");


	if (m_pDatabase )
	{
		if( store_result("SELECT LAST_INSERT_ID()" ) == NULL )//SELECT LAST_INSERT_ID() 쿼리의 결과를 모두 리턴
		{
			return -1;
		}
		if( fetch_row() == NULL )//쿼리결과에서 레코드를 가져온다
		{
			free_result();
			return -1;
		}
		dInsertIdRet = getnum(0);//m_pRow[0]값을 반환
		free_result();//메모리 해제
		return dInsertIdRet;
		//return mysql_insert_id(m_pDatabase);//select LAST_INSERT_ID()
	}
	return -1;
}

long CMysqlDB::num_rows(void)
{
	if( m_bDebug )
		infLOG(ALWAY,"%s\n","num_rows");

	if (m_pDatabase && m_pRes )
		return mysql_num_rows(m_pRes);
	return -1;
}

long CMysqlDB::num_rows(const char* pQuery,...)
{
	if( m_bDebug )
		infLOG(ALWAY,"%s\n","num_rows ...");

	free_result();//메모리 해제

	if( pQuery != NULL )
	{


		memset(m_curQuery, 0x00, sizeof(m_curQuery));


		va_list args;
		va_start(args, pQuery);
		vsprintf(m_curQuery, pQuery, args);
		va_end(args);

		if( m_bDebug )
			infLOG(ALWAY,"num_rows query [ %s ]  \n",m_curQuery);
		//query 시작
		if (mysql_query(m_pDatabase, m_curQuery) != 0)
		{
			getLastError(m_curQuery);
			if( m_bDebug )
			{
				infLOG(ALWAY,"%s\n",m_szErrorMsg);
			}
			if( m_bAutoRollback )
				tran_rollback();
			m_bCanStoreResult = false;

		}

		m_bCanStoreResult = true;

	}

	if (m_pDatabase && m_bCanStoreResult )
	{
		if( m_bDebug )
			infLOG(ALWAY,"num_rows store_result\n");


		m_pRes = mysql_store_result(m_pDatabase);//쿼리의 결과를 모두 리턴
		//getResultOk();

		if( m_pRes == NULL )
		{
			getLastError();
			if( m_bDebug )
			{
				infLOG(ALWAY,"num_rows %s\n",m_szErrorMsg);
			}
			if( m_bAutoRollback )
				tran_rollback();
			m_bCanStoreResult = false;
		}
	}

	if (m_pDatabase && m_pRes && m_bCanStoreResult )
			return mysql_num_rows(m_pRes);
	return -1;
}

//쿼리의 결과를 모두 리턴
MYSQL_RES* CMysqlDB::store_result(const char* pQuery,...)
{	// query, result


	free_result();//메모리 해제

	if( pQuery != NULL )
	{


		memset(m_curQuery, 0x00, sizeof(m_curQuery));


		va_list args;
		va_start(args, pQuery);
		vsprintf(m_curQuery, pQuery, args);
		va_end(args);

		if( m_bDebug )
			infLOG(ALWAY,"store_result \nquery [ %s ]  \n",m_curQuery);

		//query 시작

		if (mysql_query(m_pDatabase, m_curQuery) != 0)
		{
			getLastError(m_curQuery);
			if( m_bDebug )
			{
				infLOG(ALWAY,"%s\n",m_szErrorMsg);
			}
			if( m_bAutoRollback )
				tran_rollback();
			m_bCanStoreResult = false;

		}

		m_bCanStoreResult = true;
	}

	if (m_pDatabase && m_bCanStoreResult )
	{
		if( m_bDebug )
			infLOG(ALWAY,"store_result\n");


		m_pRes = mysql_store_result(m_pDatabase);//쿼리의 결과를 모두 리턴
		//getResultOk();

		if( m_pRes == NULL )
		{
			getLastError();
			if( m_bDebug )
			{
				infLOG(ALWAY,"%s\n",m_szErrorMsg);
			}
			if( m_bAutoRollback )
				tran_rollback();
			m_bCanStoreResult = false;
		}
	}
	return m_pRes;
}
/*
MYSQL_RES* CMysqlDB::store_result(char *pQuery)
{	// query, result

	free_result();


	if( pQuery != NULL )
	{
		query("%s",pQuery);
	}

	if (m_pDatabase && m_bCanStoreResult )
	{
		if( m_bDebug )
			infLOG(ALWAY,"store_result [ %s ]",pQuery);


		m_pRes = mysql_store_result(m_pDatabase);
		getResultOk();

		if( m_pRes == NULL )
		{
			getLastError();
			if( m_bDebug )
			{

				infLOG(ALWAY,"%s",m_szErrorMsg);

			}
			if( m_bAutoRollback )
				tran_rollback();
			m_bCanStoreResult = false;
		}
	}
	return m_pRes;
}
*/
//쿼리결과에서 레코드를 가져온다.
MYSQL_ROW CMysqlDB::fetch_row(void)
{

	if( m_pRes && m_bCanStoreResult )
	{
		if( m_bDebug )
			infLOG(ALWAY,"%s\n","fetch_row");


		m_pRow = mysql_fetch_row(m_pRes);//쿼리결과에서 레코드를 가져온다.
		//getResultOk();
	}
	if( m_pRow == NULL )
	{
		getLastError();
		if( m_bDebug )
		{
			infLOG(ALWAY,"%s\n",m_szErrorMsg);
		}
		if( m_bAutoRollback )
			tran_rollback();
	}

	return m_pRow;
}

char* CMysqlDB::getErrorMessage()
{
	//change utf8 to ansi
	/*
	m_pUtf8String=NULL;
	utf8_decode((char*)m_szErrorMsg,&m_pUtf8String);
	infLOG(ERROR,"szRegUser [%s]\n", m_pUtf8String);

	return m_pUtf8String;*/

	return m_szErrorMsg;
}

int	CMysqlDB::getErrorCode()
{
	if( m_pDatabase != NULL )
	{
		return mysql_errno(m_pDatabase);
	}
	return 0;
}
MYSQL* CMysqlDB::getCon()
{
	return m_pDatabase;
}
bool CMysqlDB::isConnect()
{
	if( m_pDatabase != NULL )
	{
		if( mysql_ping(m_pDatabase) == 0 )
			return true;
	}
	return false;
}
char* CMysqlDB::getQueryString()
{
	return m_curQuery;
}
/*
MYSQL_ROW row;
unsigned int num_fields;
unsigned int i;

num_fields = mysql_num_fields(result);
while ((row = mysql_fetch_row(result)))
{
   unsigned long *lengths;
   lengths = mysql_fetch_lengths(result);
   for(i = 0; i < num_fields; i++)
   {
       printf("[%.*s] ", (int) lengths[i],
              row[i] ? row[i] : "NULL");
   }
   printf("\n");
}

*/
