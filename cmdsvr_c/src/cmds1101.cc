/******************************************************************************
 *   서브시스템 : CMD서버
 *   프로그램명 : cmds1101.cc
 *         기능 : 업로드 내디스크에서 적정서버를 찾아 IP와 Port를 전송한다.
 *         설명 :
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
#include "cmds1101.h"

#define  MAX_ROWS	1


//#define  _DEBUG_

//******************************************************************************
//* cmds1101 main
//******************************************************************************
int cmds1101(char *pRecvHead, char *pRecvData, char* &pSendData)
{
	MYSQL     *con;
	MYSQL_RES *res;
	MYSQL_ROW  row;

	CCMDS1101_S t_cmds1101s[MAX_ROWS];//struct _FILEINFO;
	memset(t_cmds1101s, 0x00, (sizeof(CCMDS1101_S)*MAX_ROWS));

	LPHEADER     p_headerr;
	CCMDS1101_R	*p_cmds1101r;
	CCMDS1101_R cmd1101r;
	memset(&cmd1101r,0x00,sizeof(CCMDS1101_R));

	char szServerGu[5];
	memset(szServerGu,0x00,sizeof(szServerGu));

	char szSelectLine[50];
	memset(szSelectLine,0x00,sizeof(szSelectLine));

	char szRootPath[255];
	memset(szRootPath,0x00,sizeof(szRootPath));

	char szQuery[1000];  // query string
	char ErrMsg[256];    // error message
	int  ErrNum;         // error no
	int  nRowcnt;        // select row count
	bool bLineSelect = true;

	int  find_flag         ;  // 검색구분 [1]WE디스크  [2]MY디스크

	/*--------------------------------------------------------------------*/
	/* start!!!                                                           */
	/*--------------------------------------------------------------------*/
	#ifdef _DEBUG_
	printf("cmds1101-> start...\n");
	#endif
	p_headerr   = (LPHEADER     ) pRecvHead;

	p_cmds1101r = (LPCCMDS1101_R) pRecvData;
    infLOG(TRACE,"cmds1101->find_flag=(%d)\n"
    			"cmds1101->file_size=(%15.0f)\n"
    			, find_flag,  p_cmds1101r->total_file_size );
	#ifdef _DEBUG_
    printf("cmds1101->find_flag=(%d)\n"
    			"cmds1101->file_size=(%15.0f)\n"
			, find_flag,  p_cmds1101r->total_file_size );
	#endif


	// local 변수 Clear
	memset(&ErrMsg   , 0x00, sizeof(ErrMsg   )); /* 오류메시지     */
	memset(&find_flag, 0x00, sizeof(find_flag)); /* 검색구분       */

	find_flag   = p_cmds1101r->find_flag     ;
	strcpy(szServerGu, "01");


	if (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )))
	{
		ErrNum = -110191;
		sprintf(ErrMsg, "DB에 접속하지 못 하였습니다...\n");
		#ifdef _DEBUG_
		printf("cmds1101->ERRMSG: [%d](%s)\n",ErrNum, ErrMsg);
		#endif
		E_dump(ErrNum, ErrMsg, pSendData);
	   	return(ErrNum);
	}

	memset (szQuery, 0x00, sizeof(szQuery ));
	strcpy(szQuery, "select minor_name "
	                 "  from zangsi.T_MINOR_CODE "
	                 " where major_code = '36' and minor_code = 'MY' ");

	#ifdef _DEBUG_
	printf(" szQuery : %s \n", szQuery);
	#endif
	if (mysql_query(con, szQuery))
	{
		ErrNum = -110102;
		sprintf(ErrMsg, "mysql_query. 루트 패스 조회 실패\n");
		#ifdef _DEBUG_
		printf("cmds1101->ERRMSG: [%d](%s)\n",ErrNum, ErrMsg);
		printf("cmds1101->DB ERR: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		#endif

		db_disconnect(con);
		E_dump(ErrNum, ErrMsg, pSendData);
     	return(ErrNum);
	}

	//질의 결과를 얻는다
	if (!(res = mysql_store_result(con)))
	{
		ErrNum = -110103;
		sprintf(ErrMsg, "mysql_store_result. 루트 패스 조회 실패\n");
		#ifdef _DEBUG_
		printf("cmds1101->ERRMSG: [%d](%s)\n",ErrNum, ErrMsg);
		printf("cmds1101->DB ERR: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		#endif

		db_disconnect(con);
		E_dump(ErrNum, ErrMsg, pSendData);
      	return(ErrNum);
	}

 	if (mysql_num_rows(res) == 0)
 	{
 		mysql_free_result(res);
		ErrNum = -110103;
		sprintf(ErrMsg, "mysql_num_rows. 루트 패스 조회 실패\n");
		#ifdef _DEBUG_
		printf("cmds1101->ERRMSG: [%d](%s)\n",ErrNum, ErrMsg);
		#endif

		db_disconnect(con);
		E_dump(ErrNum, ErrMsg, pSendData);
      	return(ErrNum);
	}
	row = mysql_fetch_row(res);
	strcpy(szRootPath, getstr(row,0));
	mysql_free_result(res);

	#ifdef _DEBUG_
	printf("> szRootPath = [%s]\n", szRootPath);
	#endif

	int nLineSymbol = 0;
	memset (szQuery, 0x00, sizeof(szQuery ));
	sprintf(szQuery, " select "
	                 " round(server_port / 100) ,sum(dn_user+up_user )/count(server_id) as total_user "
	                 " from zangsi.T_SERVER_INFO "
	                 " where server_gu ='%s' and admin_open_yn = 'Y' and upload_yn = 'Y' and user_cnt != 0  "
	                 " group by round(server_port / 100) "
	                 " order by total_user limit 1 "
	                 ,szServerGu);

	#ifdef _DEBUG_
	printf("\n\n------->(%s)\n\n", szQuery);
	#endif

	if (mysql_query(con, szQuery))
	{
		ErrNum = -110122;
		sprintf(ErrMsg, "mysql_query.검색시 오류가 발생하였습니다(라인선택).\n");
		#ifdef _DEBUG_
		printf("cmds1101->ERRMSG: [%d](%s)\n",ErrNum, ErrMsg);
		printf("cmds1101->DB ERR: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		#endif

		db_disconnect(con);
		E_dump(ErrNum, ErrMsg, pSendData);
   	  	return(ErrNum);
	}

	//질의 결과를 얻는다
	if (!(res = mysql_store_result(con)))
	{
		ErrNum = -110123;
		sprintf(ErrMsg, "mysql_store_result.검색시 오류가 발생하였습니다(라인선택).\n");
		#ifdef _DEBUG_
		printf("cmds1101->ERRMSG: [%d](%s)\n",ErrNum, ErrMsg);
		printf("cmds1101->DB ERR: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		#endif

		db_disconnect(con);
		E_dump(ErrNum, ErrMsg, pSendData);
      	return(ErrNum);
	}

 	if (mysql_num_rows(res)!=0)
 	{
 		if((row = mysql_fetch_row(res)))
		{
			nLineSymbol = (int)getnum(row,0);
		}
	}

	// 수치가 가장낮은 라인에서 가장낮을 수치의 렉 선택
	mysql_free_result(res);

	memset (szQuery, 0x00, sizeof(szQuery ));
	sprintf(szQuery, "select                                                                      			"
	                 " server_port,sum(dn_user+up_user )/count(server_id) as total_user	      	  			"
	                 " from zangsi.T_SERVER_INFO                                                  			"
	                 " where server_gu ='%s' and admin_open_yn = 'Y' and upload_yn = 'Y' and user_cnt != 0  "
	                 " and round(server_port / 100) = %d	                                			"
	                 " group by server_port                                                       			"
	                 " order by total_user limit 1                                                		    "
	                 ,szServerGu, nLineSymbol);

	#ifdef _DEBUG_
	printf("\n\n------->(%s)\n\n", szQuery);
	#endif
	if (mysql_query(con, szQuery))
	{
		ErrNum = -110122;
		sprintf(ErrMsg, "mysql_query.검색시 오류가 발생하였습니다(포트선택).\n");
		#ifdef _DEBUG_
		printf("cmds1101->ERRMSG: [%d](%s)\n",ErrNum, ErrMsg);
		printf("cmds1101->DB ERR: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		#endif

		db_disconnect(con);
		E_dump(ErrNum, ErrMsg, pSendData);
     	  	return(ErrNum);
	}

	//질의 결과를 얻는다
	if (!(res = mysql_store_result(con)))
	{
		ErrNum = -110123;
		sprintf(ErrMsg, "mysql_store_result.검색시 오류가 발생하였습니다(포트선택).\n");
		#ifdef _DEBUG_
		printf("cmds1101->ERRMSG: [%d](%s)\n",ErrNum, ErrMsg);
		printf("cmds1101->DB ERR: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		#endif

		db_disconnect(con);
		E_dump(ErrNum, ErrMsg, pSendData);
      	return(ErrNum);
	}

 	if (mysql_num_rows(res)!=0)
 	{
 		if((row = mysql_fetch_row(res)))
		{
			sprintf(szSelectLine," and server_port = %d ",(int)getnum(row,0));
		}
	}

	mysql_free_result(res);

	memset (szQuery, 0x00, sizeof(szQuery ));
	sprintf(szQuery, "select a.server_id,   a.server_ip, "
	                 "       a.upsvr_port,  a.dnsvr_port, "
	                 "       '%s' "
	                 "  from T_SERVER_INFO a "
	                 " where a.server_gu = '%s' and upload_yn = 'Y' "
	                 "    and dn_user < 25 and up_user < 1  %s "
	                 " order by  user_cnt , dn_user , up_user  limit 1 "
	        , szRootPath, szServerGu, szSelectLine);

	#ifdef _DEBUG_
	printf(" szQuery : %s \n",szQuery);
	#endif

	if (mysql_query(con, szQuery))
	{
		ErrNum = -110102;
		sprintf(ErrMsg, "검색시 오류가 발생하였습니다.\n");
		#ifdef _DEBUG_
		printf("cmds1101->ERRMSG: [%d](%s)\n",ErrNum, ErrMsg);
		printf("cmds1101->DB ERR: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		#endif

		db_disconnect(con);
		E_dump(ErrNum, ErrMsg, pSendData);
     	return(ErrNum);
	}

	//질의 결과를 얻는다
	if (!(res = mysql_store_result(con)))
	{
		ErrNum = -110103;
		sprintf(ErrMsg, "검색시 오류가 발생하였습니다.\n");
		#ifdef _DEBUG_
		printf("cmds1101->ERRMSG: [%d](%s)\n",ErrNum, ErrMsg);
		printf("cmds1101->DB ERR: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		#endif

		db_disconnect(con);
		E_dump(ErrNum, ErrMsg, pSendData);
      	return(ErrNum);
	}

	memset(t_cmds1101s, 0x00, (sizeof(CCMDS1101_S)*MAX_ROWS));

	nRowcnt = 0;
 	if (mysql_num_rows(res)!=0)
 	{
		while((row = mysql_fetch_row(res)))
		{
			#ifdef _DEBUG_
			printf("t_cmds1101s[nRowcnt].server_id(%s)\n", getstr(row, 0));
			printf("t_cmds1101s[nRowcnt].server_ip(%s)\n", getstr(row, 1));
			printf("t_cmds1101s[nRowcnt].up_Port  (%d)\n", getint(row, 2));
			printf("t_cmds1101s[nRowcnt].dn_port  (%d)\n", getint(row, 3));
			printf("t_cmds1101s[nRowcnt].root_path(%s)\n", getstr(row, 4));
			#endif

			strcpy(t_cmds1101s[nRowcnt].server_id, getstr(row, 0));
			strcpy(t_cmds1101s[nRowcnt].server_ip, getstr(row, 1));
			       t_cmds1101s[nRowcnt].up_Port  = getint(row, 2);
			       t_cmds1101s[nRowcnt].dn_port  = getint(row, 3);
			strcpy(t_cmds1101s[nRowcnt].root_path, getstr(row, 4));

			infLOG(ALWAY, " %s : %s : %ld : %ld : %s \n"
						, t_cmds1101s[nRowcnt].	server_id,t_cmds1101s[nRowcnt].server_ip,t_cmds1101s[nRowcnt].up_Port
						, t_cmds1101s[nRowcnt].dn_port,t_cmds1101s[nRowcnt].root_path );


			nRowcnt++;

			// buffer크기 보다 큰경우
			if (nRowcnt >= MAX_ROWS)
				break;
		}
		mysql_free_result(res);
	}
	else
	{
		mysql_free_result(res);

		memset (szQuery, 0x00, sizeof(szQuery ));
		sprintf(szQuery, "select a.server_id,   a.server_ip, "
		                 "       a.upsvr_port,  a.dnsvr_port, "
		                 "       a.root_path , "
						 "	sum(dn_user+up_user )/count(server_id) as total_user "
		                 "  from zangsi.T_SERVER_INFO a "
		                 " where a.server_gu = '%s' and upload_yn = 'Y' "
                            "  group by round(server_port / 100) "
		                 " order by  user_cnt, total_user , dn_user , up_user  limit 1 "
		                 ,szServerGu);

		#ifdef _DEBUG_
		printf("\n\n------->(%s)\n\n", szQuery);
		#endif

		if (mysql_query(con, szQuery))
		{
			ErrNum = -110102;
			sprintf(ErrMsg, "검색시 오류가 발생하였습니다.\n");
			#ifdef _DEBUG_
			printf("cmds1101->ERRMSG: [%d](%s)\n",ErrNum, ErrMsg);
			printf("cmds1101->DB ERR: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			#endif

			db_disconnect(con);
			E_dump(ErrNum, ErrMsg, pSendData);
	       	return(ErrNum);
		}

		//질의 결과를 얻는다
		if (!(res = mysql_store_result(con)))
		{
			ErrNum = -110103;
			sprintf(ErrMsg, "검색시 오류가 발생하였습니다.\n");
			#ifdef _DEBUG_
			printf("cmds1101->ERRMSG: [%d](%s)\n",ErrNum, ErrMsg);
			printf("cmds1101->DB ERR: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			#endif

			db_disconnect(con);
			E_dump(ErrNum, ErrMsg, pSendData);
	       	return(ErrNum);
		}

		//임시버퍼생성
		memset(t_cmds1101s, 0x00, (sizeof(CCMDS1101_S)*MAX_ROWS));

		nRowcnt = 0;
	 	if (mysql_num_rows(res)!=0)
	 	{
			while((row = mysql_fetch_row(res)))
			{
				#ifdef _DEBUG_
				printf("t_cmds1101s[nRowcnt].server_id(%s)\n", getstr(row, 0));
				printf("t_cmds1101s[nRowcnt].server_ip(%s)\n", getstr(row, 1));
				printf("t_cmds1101s[nRowcnt].up_Port  (%d)\n", getint(row, 2));
				printf("t_cmds1101s[nRowcnt].dn_port  (%d)\n", getint(row, 3));
				printf("t_cmds1101s[nRowcnt].root_path(%s)\n", getstr(row, 4));
				#endif

				strcpy(t_cmds1101s[nRowcnt].server_id, getstr(row, 0));
				strcpy(t_cmds1101s[nRowcnt].server_ip, getstr(row, 1));
				       t_cmds1101s[nRowcnt].up_Port  = getint(row, 2);
				       t_cmds1101s[nRowcnt].dn_port  = getint(row, 3);
				strcpy(t_cmds1101s[nRowcnt].root_path, getstr(row, 4));

				infLOG(ALWAY, " %s : %s : %ld : %ld : %s \n"
					, t_cmds1101s[nRowcnt].	server_id,t_cmds1101s[nRowcnt].server_ip,t_cmds1101s[nRowcnt].up_Port
					, t_cmds1101s[nRowcnt].dn_port,t_cmds1101s[nRowcnt].root_path );

				nRowcnt++;

				// buffer크기 보다 큰경우
				if (nRowcnt >= MAX_ROWS)
					break;
			}
			mysql_free_result(res);
		}
		else
		{
			ErrNum = -110104;
			sprintf(ErrMsg, "사용자가 많아서 서버에 접속 할수 없습니다.\n잠시후 다시 시도해 주십시오\n");
			#ifdef _DEBUG_
			printf("cmds1101->ERRMSG: [%d](%s)\n",ErrNum, ErrMsg);
			printf("cmds1101->DB ERR: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			#endif
			mysql_free_result(res);
			db_disconnect(con);
			E_dump(ErrNum, ErrMsg, pSendData);
			return(ErrNum);
		}
	}

	db_disconnect(con);

	//전송할 버퍼생성
	int nDataSize = HEADER_SIZE + sizeof(CCMDS1101_S) * 1;

	HEADER headers;
	memcpy(&headers, p_headerr, HEADER_SIZE); //header
	headers.nDataCnt  = 1;
	headers.nDataSize = sizeof(CCMDS1101_S);

	pSendData = new char[nDataSize];
	memset(pSendData, 0x00, nDataSize);

	memcpy(pSendData, &headers, sizeof(HEADER)); //header
	memcpy(pSendData+HEADER_SIZE, t_cmds1101s, nDataSize - HEADER_SIZE); //body
//	delete[] t_cmds1101s;

	#ifdef _DEBUG_
	printf("cmds1101-> end sucess sendsize(%d)\n", nDataSize);
	#endif

	return (1);
}
