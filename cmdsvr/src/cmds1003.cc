/******************************************************************************
 *   서브시스템 : CMD서버
 *   프로그램명 : cmds1003.cc (WEDISK 친구목록조회)
 *         기능 : 사용자ID로 친구목록을 조회한다.
 *         설명 :
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

#include "apdefine.h"
#include "comcomm.h"
#include "commydb.h"
#include "cmds1003.h"

#define  MAX_ROWS	500
//#define  _DEBUG_
/******************************************************************************
** nServerFlag -> 1 : We디스크
**                2 : 내디스크
*******************************************************************************/
int cmds1003(char *pRecvHead, char *pRecvData, char* &pSendData)
{
	MYSQL     *con;
	MYSQL_RES *res;
	MYSQL_ROW  row;

	LPHEADER     p_headerr;

	char szQuery[1000];  // query string
	char ErrMsg  [256];  // error message
	int  ErrNum;         // error no
	int  nRowcnt;        // select row count

	// input variables
	char    user_id         [12+1];  // 사용자ID
	/*--------------------------------------------------------------------*/
	/* start!!!                                                           */
	/*--------------------------------------------------------------------*/
	p_headerr   = (LPHEADER) pRecvHead;
//	p_cmds1003r = (LPCCMDS1003_R) pRecvData;

	// local 변수 Clear
	memset(&ErrMsg   , 0x00, sizeof(ErrMsg   )); /* 오류메시지     */
	memset(&user_id  , 0x00, sizeof(user_id  )); /* 사용자ID       */

	memcpy(user_id, p_headerr->szUserID, sizeof(user_id)-1);
	#ifdef _DEBUG_
	printf("cmds1003->in user_id  =(%s)\n", user_id  );
	#endif

	// DB 연결

	if (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )))
	{
		ErrNum = -100301;
		sprintf(ErrMsg, "DB에 접속하지 못 하였습니다...\n");
		printf("cmds1003->ERRMSG: [%d](%s)",ErrNum, ErrMsg);
		printf("cmds1003->DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));

		db_disconnect(con);
		E_dump(ErrNum, ErrMsg, pSendData);
	   	return(ErrNum);
	}

	//--------------------------------------------------------------------------
	// 자료가 없는경우 오류발생 DB에 문제되므로 건수 있는지 확인후 다음로직진행
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery ));
	sprintf(szQuery, "select count(a.user_id)    "
	                 "  from T_FRND_INFO a"
	                 " where a.user_id = '%s'    "
	                 ,user_id
	                 );
	if (mysql_query(con, szQuery)){
		ErrNum = -100302;
		sprintf(ErrMsg, "검색시 오류가 발생하였습니다.\n");
		#ifdef _DEBUG_
		printf("cmds1003->ERRMSG: [%d](%s)",ErrNum, ErrMsg);
		printf("cmds1003->DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
		#endif

		db_disconnect(con);
		E_dump(ErrNum, ErrMsg, pSendData);
	   	return(ErrNum);
	}
	//질의 결과를 얻는다
	if (!(res = mysql_store_result(con)))
	{
		ErrNum = -100303;
		sprintf(ErrMsg, "검색시 오류가 발생하였습니다.\n");
		#ifdef _DEBUG_
		printf("cmds1003->ERRMSG: [%d](%s)",ErrNum, ErrMsg);
		printf("cmds1003->DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
		#endif

		db_disconnect(con);
		E_dump(ErrNum, ErrMsg, pSendData);
	   	return(ErrNum);
	}
 	if (mysql_num_rows(res) == 0)
	{
		ErrNum = -999999;
		sprintf(ErrMsg, "자료가 없습니다.\n");
		#ifdef _DEBUG_
		printf("cmds1003->ERRMSG: [%d](%s)",ErrNum, ErrMsg);
		printf("cmds1003->DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
		#endif

		db_disconnect(con);
		E_dump(ErrNum, ErrMsg, pSendData);
	   	return(ErrNum);
	}

	nRowcnt = 0;
	if (row = mysql_fetch_row(res))	{
		nRowcnt = getint(row, 0);
	}
	mysql_free_result(res);

	if (nRowcnt == 0)
	{
		ErrNum = -999999;
		sprintf(ErrMsg, "자료가 없습니다.\n");
		#ifdef _DEBUG_
		printf("cmds1003->ERRMSG: [%d](%s)",ErrNum, ErrMsg);
		printf("cmds1003->DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
		#endif

		db_disconnect(con);
		E_dump(ErrNum, ErrMsg, pSendData);
	   	return(ErrNum);
	}
	//--------------------------------------------------------------------------
	// 친구가 등록되어 있는경우 아래 로직으로...
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery ));
	sprintf(szQuery, "select a.frnd_id     , a.frnd_desc  "
	                 "     , if(a.share_yn='Y',if(ifnull(b.server_id, '0')='0',0,1),0) as share_yn"
	                 "     , b.server_id   , c.server_ip  "
	                 "     , c.server_port , b.root_path  , b.won_mega"
	                 "  from                              "
	                 "     ( T_FRND_INFO   a  left outer join "
	                 "       T_MYDISK_INFO b  "
	                 "    on a.frnd_id   = b.user_id) left outer join "
	                 "       T_SERVER_INFO c     "
	                 "    on b.server_id = c.server_id  "
	                 " where a.user_id = '%s'  "
	                 ,user_id
	                 );
	if (mysql_query(con, szQuery)){
		ErrNum = -100304;
		sprintf(ErrMsg, "검색시 오류가 발생하였습니다.\n");
		#ifdef _DEBUG_
		printf("cmds1003->ERRMSG: [%d](%s)",ErrNum, ErrMsg);
		printf("cmds1003->DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
		#endif

		db_disconnect(con);
		E_dump(ErrNum, ErrMsg, pSendData);
	   	return(ErrNum);
	}

	//질의 결과를 얻는다
	if (!(res = mysql_store_result(con)))
	{
		ErrNum = -100305;
		sprintf(ErrMsg, "검색시 오류가 발생하였습니다.\n");
		#ifdef _DEBUG_
		printf("cmds1003->ERRMSG: [%d](%s)",ErrNum, ErrMsg);
		printf("cmds1003->DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
		#endif

		db_disconnect(con);
		E_dump(ErrNum, ErrMsg, pSendData);
	   	return(ErrNum);
	}

	//임시버퍼생성
	LPCCMDS1003_S t_cmds1003s = new CCMDS1003_S[MAX_ROWS];//struct _FILEINFO;
	memset(t_cmds1003s, 0x00, (sizeof(CCMDS1003_S)*MAX_ROWS));

	nRowcnt = 0;
 	if (mysql_num_rows(res) > 0)
 	{
		while((row = mysql_fetch_row(res))) {
			#ifdef _DEBUG_
			printf("cmds1003s[%d].frnd_id    (%s)\n", nRowcnt, getstr(row, 0));
			printf("cmds1003s[%d].frnd_desc  (%s)\n", nRowcnt, getstr(row, 1));
			printf("cmds1003s[%d].server_id  (%s)\n", nRowcnt, getstr(row, 3));
			printf("cmds1003s[%d].server_ip  (%s)\n", nRowcnt, getstr(row, 4));
			printf("cmds1003s[%d].share_yn   (%d)\n", nRowcnt, getint(row, 2));
			printf("cmds1003s[%d].server_port(%d)\n", nRowcnt, getint(row, 5));
			printf("cmds1003s[%d].root_path  (%s)\n", nRowcnt, getstr(row, 6));
			#endif

			strcpy(t_cmds1003s[nRowcnt].frnd_id     , getstr(row, 0));
			strcpy(t_cmds1003s[nRowcnt].frnd_desc   , getstr(row, 1));
			strcpy(t_cmds1003s[nRowcnt].server_id   , getstr(row, 3));
			strcpy(t_cmds1003s[nRowcnt].server_ip   , getstr(row, 4));
			       t_cmds1003s[nRowcnt].share_yn    = getint(row, 2);
			       t_cmds1003s[nRowcnt].server_port = getint(row, 5);
			strcpy(t_cmds1003s[nRowcnt].root_path   , getstr(row, 6));
			       t_cmds1003s[nRowcnt].won_mega    = getint(row, 7);

			nRowcnt++;

			// buffer크기 보다 큰경우
			if (nRowcnt >= MAX_ROWS)
				break;
		}
	}
	mysql_free_result(res);
	db_disconnect(con);
	if (nRowcnt < 1)
	{
		delete[] t_cmds1003s;

		ErrNum = -999999;
		sprintf(ErrMsg, "자료가 없습니다.\n");
		#ifdef _DEBUG_
		printf("cmds1003->ERRMSG: [%d](%s)",ErrNum, ErrMsg);
		printf("cmds1003->DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
		#endif

		db_disconnect(con);
		E_dump(ErrNum, ErrMsg, pSendData);
	   	return(ErrNum);
	}

	//전송할 버퍼생성
	int nDataSize = HEADER_SIZE + sizeof(CCMDS1003_S) * nRowcnt;

	HEADER headers;
	memcpy(&headers, p_headerr, HEADER_SIZE); //header
	headers.nDataCnt  = nRowcnt;
	headers.nDataSize = sizeof(CCMDS1003_S);

	pSendData = new char[nDataSize];
	memset(pSendData, 0x00, nDataSize);

	memcpy(pSendData, &headers, sizeof(HEADER)); //header
	memcpy(pSendData+HEADER_SIZE, t_cmds1003s, nDataSize - HEADER_SIZE); //body
	delete[] t_cmds1003s;
	#ifdef _DEBUG_
	printf("cmds1003-> end sucess sendsize(%d)\n", nDataSize);
	#endif

	return (1);

}

/*------------------------------------------------------------------------------
select a.frnd_id     , a.frnd_desc
     , if(a.share_yn='Y',if(ifnull(b.server_id, '0')='0',0,1),0) as share_yn
     , b.server_id   , c.server_ip
     , c.server_port
  from
     ( T_FRND_INFO   a  left outer join
       T_MYDISK_INFO b
    on a.frnd_id   = b.user_id) left outer join
       T_SERVER_INFO c
    on b.server_id = c.server_id
 where a.user_id = 'jbk'
------------------------------------------------------------------------------*/

