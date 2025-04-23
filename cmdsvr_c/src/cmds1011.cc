/******************************************************************************
 *   서브시스템 : CMD서버
 *   프로그램명 : cmds1011.cc (등록자료삭제)
 *         기능 : We디스크의 등록자료를 삭제 한다.
 *         설명 : 다운로드 건수가 있는경우는 삭제할 수 없다.
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

#include "apdefine.h"
#include "comcomm.h"
#include "commydb.h"
#include "cmds1011.h"

#define  MAX_ROWS	1
//#define  _DEBUG_
/******************************************************************************
**  cmds1011 main
*******************************************************************************/
int cmds1011(char *pRecvHead, char *pRecvData, char* &pSendData)
{
	MYSQL     *con;
	MYSQL_RES *res;
	MYSQL_ROW  row;

	LPHEADER     p_headerr;
	CCMDS1011_R	*p_cmds1011r;
	int nDataSize;

	char szQuery[1000];  // query string
	char szQuery1[1000];  // query string
	char ErrMsg  [256];  // error message
	int  ErrNum;         // error no
	int  nRowcnt;        // select row count

	// input variables
	char     user_id           [12+1];  // 사용자ID
	unsigned long id                 ;  // 등록id
	char     reg_user          [12+1];  // 등록자ID
	int      down_cnt             = 0;
	char     del_date          [ 8+1];


	int conn_cnt;
	/*--------------------------------------------------------------------*/
	/* start!!!                                                           */
	/*--------------------------------------------------------------------*/
	#ifdef _DEBUG_
	printf("cmds1011-> start...\n");
	#endif

	p_headerr   = (LPHEADER) pRecvHead;
	p_cmds1011r = (LPCCMDS1011_R) pRecvData;

	// local 변수 Clear
	memset(&ErrMsg   , 0x00, sizeof(ErrMsg   )); // 오류메시지
	memset(&id       , 0x00, sizeof(id       )); // 등록자료ID
	memset(&user_id  , 0x00, sizeof(user_id  )); // 등록자ID
	memset(&reg_user , 0x00, sizeof(reg_user )); // 등록자ID
	memset(&down_cnt , 0x00, sizeof(down_cnt ));
	memset(&del_date , 0x00, sizeof(del_date ));

	//strcpy(user_id, p_cmds1011r->user_id);
	memcpy( user_id , p_cmds1011r->user_id,sizeof(p_cmds1011r->user_id));
	id = p_cmds1011r->id;

	#ifdef _DEBUG_
	printf("cmds1011->in id       =(%d)\n", id  );
	printf("cmds1011->in user_id  =(%s)\n", user_id  );
	#endif

	//컨텐츠 등록자료 조회
	memset (szQuery, 0x00, sizeof(szQuery ));
	memset (szQuery1, 0x00, sizeof(szQuery1));
	strcpy (szQuery, "select a.down_cnt ,a.reg_user  ");
	strcat (szQuery, "     , date_format(date_add(now(), INTERVAL 3 DAY),'%Y%m%d')");
	strcat (szQuery, "  from T_CONTENTS_FILE a");
	sprintf(szQuery1," where a.id = %d", id);
	strcat (szQuery, szQuery1);

	// DB 연결

	if (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )))
	{
		ErrNum = -101191;
		sprintf(ErrMsg, "DB에 접속하지 못 하였습니다.\n");
		goto cmds1011_err;
	}


	if (mysql_query(con, szQuery)){
		ErrNum = -101102;
		sprintf(ErrMsg, "검색시 오류가 발생하였습니다.\n");
		goto cmds1011_err;
	}

	if (!(res = mysql_store_result(con)))
	{
		ErrNum = -101103;
		sprintf(ErrMsg, "검색시 오류가 발생하였습니다.\n");
		goto cmds1011_err;
	}
 	if (mysql_num_rows(res)==0)
 	{
		ErrNum = -101104;
		sprintf(ErrMsg, "검색시 오류가 발생하였습니다.\n");
		goto cmds1011_err;
	}

	if (row = mysql_fetch_row(res))
	{
		down_cnt = getint(row, 0);
		strcpy(reg_user, getstr(row, 1));
		strcpy(del_date, getstr(row, 2));
	}

	mysql_free_result(res);

	// 삭제조건에 맞는지 확인한다.
	if (strcmp(reg_user, user_id)!=0)
	{
		ErrNum = -101105;
		sprintf(ErrMsg, "업로드한 사용자가 아닙니다.\n");
		goto cmds1011_err;
	}

	// 다운로드 건수가 있는경우는 파일삭제를 3일후에 처리한다.
	if (down_cnt == 0)
	{
		memset(&del_date , 0x00, sizeof(del_date ));
		strcpy( del_date , "00000000");
	}

	//--------------------------------------------------------------------------
	// 트렌젝션시작
	//--------------------------------------------------------------------------
	if (tran_begin(con)!=0)
	{
		ErrNum = -300392;
		sprintf(ErrMsg, "테이베이스 오류입니다.\n");
		goto cmds1011_tran_err;
	}

	//--------------------------------------------------------------------------
	// 1. T_CONTENTS_DEL(생성)
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery ));
	sprintf(szQuery, "INSERT INTO T_CONTENTS_DEL"
	                 "     ( id         , folder_yn  "
	                 "     , server_id  , del_date   , file_path  "
	                 "     , file_name1 , file_name2 "
	                 "     , file_size  , file_type  "
	                 "     , reg_user   , reg_date   "
	                 "     , reg_time)               "
	                 "SELECT id         , folder_yn  "
	                 "     , server_id  , '%s'       , file_path  "
	                 "     , file_name1 , file_name2 "
	                 "     , file_size  , file_type  "
	                 "     , reg_user   , reg_date   "
	                 "     , reg_time                "
	                 "  FROM T_CONTENTS_FILE  "
	                 " WHERE id  = %d                "
	                 ,del_date
	                 ,id
	                 );
	if (mysql_query(con, szQuery)){
		if(mysql_errno(con) != 1062)
		{
			ErrNum = -101105;
			sprintf(ErrMsg, "자료를 삭제하지 못했습니다.\n");
			goto cmds1011_tran_err;
		}
    }

	//--------------------------------------------------------------------------
	// 2. T_CONTENTS_CREATE
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery ));
	sprintf(szQuery, "INSERT INTO T_CONTENTS_CREATE"
	                 "     ( id         , cont_gu    "
	                 "     , udt_cd     ) VALUES     "
	                 "     ( %d         , '01'       "
	                 "     , 'D'        )            "
	                 ,id
	                 );
	if (mysql_query(con, szQuery)){
		ErrNum = -101106;
		sprintf(ErrMsg, "자료를 삭제하지 못했습니다.\n");
		goto cmds1011_tran_err;
    }

	//--------------------------------------------------------------------------
	// 3. T_CONTENTS_INFO(삭제)
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery ));
	sprintf(szQuery, "DELETE FROM T_CONTENTS_INFO"
	                 " WHERE id    = %d            "
	                 ,id
	                 );
	if (mysql_query(con, szQuery)){
		ErrNum = -101107;
		sprintf(ErrMsg, "자료를 삭제하지 못했습니다.\n");
		goto cmds1011_tran_err;
    }

	//--------------------------------------------------------------------------
	// 4. T_CONTENTS_FILE(삭제)
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery ));
	sprintf(szQuery, "DELETE FROM T_CONTENTS_FILE"
	                 " WHERE id    = %d            "
	                 ,id
	                 );
	if (mysql_query(con, szQuery)){
		ErrNum = -101108;
		sprintf(ErrMsg, "자료를 삭제하지 못했습니다.\n");
		goto cmds1011_tran_err;
    }

	if (tran_commit(con)!=0)
	{
		ErrNum = -101193;
		sprintf(ErrMsg, "테이베이스 오류입니다.\n");
		goto cmds1011_tran_err;
	}
	db_disconnect(con);

	//전송할 버퍼생성
	nDataSize = HEADER_SIZE;

	HEADER headers;
	memcpy(&headers, p_headerr, HEADER_SIZE); //header
	headers.nDataCnt  = 0;
	headers.nDataSize = 0;

	pSendData = new char[nDataSize];
	memset(pSendData, 0x00, nDataSize);

	memcpy(pSendData, &headers, sizeof(HEADER)); //header

	#ifdef _DEBUG_
	printf("cmds1011-> end sucess sendsize(%d)\n", nDataSize);
	#endif

	return (1);

//------------------------------------------------------------------------------
cmds1011_err:
	#ifdef _DEBUG_
	printf("cmds1011->ERRMSG: [%d](%s)",ErrNum, ErrMsg);
	printf("cmds1011->DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
	#endif

	db_disconnect(con);
	E_dump(ErrNum, ErrMsg, pSendData);
   	return(ErrNum);

cmds1011_tran_err:
	#ifdef _DEBUG_
	printf("cmds1011->ERRMSG: [%d](%s)",ErrNum, ErrMsg);
	printf("cmds1011->DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
	#endif

	tran_rollback(con);
	db_disconnect(con);
	E_dump(ErrNum, ErrMsg, pSendData);
   	return(ErrNum);

}

