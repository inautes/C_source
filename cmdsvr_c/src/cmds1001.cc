/******************************************************************************
 *   서브시스템 : CMD서버
 *   프로그램명 : cmds1001.cc
 *         기능 : 업로드 각서버군에서 적정서버를 찾아 IP와 Port를 전송한다.
 *         설명 :
 *       작성자 : LEE
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
#include "cmds1001.h"

#define  MAX_ROWS	1

//#define  _DEBUG_

//******************************************************************************
//* cmds1001 main
//******************************************************************************
int cmds1001(char *pRecvHead, char *pRecvData, char* &pSendData)
{
	MYSQL     *con;
	MYSQL_RES *res;
	MYSQL_ROW  row;

	CCMDS1001_S t_cmds1001s;//struct _FILEINFO;
	memset(&t_cmds1001s, 0x00, sizeof(CCMDS1001_S));

	LPHEADER     p_headerr;
	CCMDS1001_R	*p_cmds1001r;
	CCMDS1001_R cmd1001r;
	memset(&cmd1001r,0x00,sizeof(CCMDS1001_R));

	char szServerGu[5];
	memset(szServerGu,0x00,sizeof(szServerGu));

	char szSelectLine[50];
	memset(szSelectLine,0x00,sizeof(szSelectLine));
	char szQuery[2048];  // query string
	char ErrMsg[256];    // error message
	int  ErrNum;         // error no
	int  nRowcnt;        // select row count

	// input variables
	int  find_flag  = 0       ;  // 검색구분 [1]WE디스크  [2]MY디스크

	/*--------------------------------------------------------------------*/
	/* start!!!                                                           */
	/*--------------------------------------------------------------------*/
	#ifdef _DEBUG_
	printf("cmds1001-> start...\n");
	#endif


	//20120222
	int nUpVersion = ((LPHEADER ) pRecvHead)->nErrorCode ;
	((LPHEADER     ) pRecvHead)->nErrorCode =0;

	p_headerr   = (LPHEADER     ) pRecvHead;
	p_cmds1001r = (LPCCMDS1001_R) pRecvData;


	// local 변수 Clear
	memset(&ErrMsg   , 0x00, sizeof(ErrMsg   )); /* 오류메시지     */

	find_flag   = p_cmds1001r->find_flag;

	if(find_flag == 2) // 20100209 -- HCS : 내디스크도 일반 컨텐츠서버로...
		strcpy(szServerGu, "01");
	else
		sprintf(szServerGu , "%0.2d" ,find_flag);

	#ifdef _DEBUG_
	printf("cmds1001->szServerGu: [%s]\n", szServerGu);
	#endif

    infLOG(TRACE,"cmds1001->find_flag=(%d)\ncmds1001->temp_id=(%ld)\ncmds1001->sect_code=(%s)\ncmds1001->sect_sub=(%s)\n"
    			"cmds1001->adult_yn=(%s)\ncmds1001->file_size=(%15.0f)\n"
    			, find_flag , p_cmds1001r->temp_id , p_cmds1001r->sect_code , p_cmds1001r->sect_sub
    			,p_cmds1001r->adult_yn ,  p_cmds1001r->file_size );


	if (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )))
	{
		ErrNum = -10011;
		sprintf(ErrMsg, "DB에 접속하지 못 하였습니다...\n");
		infLOG(ERROR, "cmds1001->ERRMSG: [%d]. DB 접속 실패.\n",ErrNum);
		E_dump(ErrNum, ErrMsg, pSendData);
	   	return(ErrNum);
	}

	//20120222
	//char szUpToolName[400];
	//memset(szUpToolName,0x00,sizeof(szUpToolName));
	if( find_flag == 1 )
	{
		//버전검사
		infLOG(ALWAY,"버전검사 [ %d ] == [ %d ] \n",nUpVersion,(12605+12603));//12426
		if( nUpVersion < (12605+12603) )
		{
			ErrNum = -10000;
			//sprintf(ErrMsg, "업로드 모듈이 손상되었습니다.삭제 후 다시 설치해 주십시오\n");
			///sprintf(ErrMsg, "");
			sprintf(ErrMsg, "최신 모듈이 아닙니다.삭제 후 다시 설치해 주십시오.\n");
			infLOG(ERROR, "cmds1001->ERRMSG: [%d] [%s]\n",ErrNum,ErrMsg);
			E_dump(ErrNum, ErrMsg, pSendData);
			db_disconnect(con);
		   	return(ErrNum);
		}
		/*
		//디비로부터 유해 프로세스 목록 가져오기
		memset(szQuery, 0x00, sizeof(szQuery));
		strcat(szQuery, " select * from zangsi.T_UPLOAD_TOOL_NAME  ");

		if( mysql_query(con, szQuery) )
		{
			infLOG(ERROR, "cmds1001->ERRMSG: 버전검사 실패 \n");
	     	//pass
		}
		else
		{
			if( res = mysql_store_result(con) )
			{
			 	if(mysql_num_rows(res) > 0)
			 	{
			 		while( (row = mysql_fetch_row(res)))
			 		{
			 			strcat(szUpToolName,getstr(row,0));
			 			strcat(szUpToolName,"|");
			 		}
			 	}
			 	mysql_free_result(res);
			}
		}
		*/

	}

	if (find_flag == 1 || find_flag == 3 || find_flag == 2 )
	{
		nRowcnt = 0;
		memset(&t_cmds1001s, 0x00, sizeof(CCMDS1001_S));

		//운선순위 300 서버 질의
		nRowcnt = 0;
		nRowcnt = GetPriServerInfo(con, 300, szServerGu, &t_cmds1001s);
		if(nRowcnt < 0)
		{
			ErrNum = -10013;
			sprintf(ErrMsg, "검색시 오류가 발생하였습니다.\n");
			infLOG(ERROR, "cmds1001->ERRMSG: [%d]",ErrNum);
			infLOG(ERROR, "cmds1001->DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
			db_disconnect(con);
			E_dump(ErrNum, ErrMsg, pSendData);
			return(ErrNum);
		}

		if(nRowcnt == 1)
		{//질의 결과가 있다면
			goto Job_Success;
		}

 		 if( strcmp(p_cmds1001r->sect_code,"03") == 0 && strcmp(p_cmds1001r->sect_sub,"07") == 0  )
 		 	strcpy(p_cmds1001r->sect_code,"11");

 		//먼저 추가 해논 라인을 선택하여 서버 선택
 		// 성인11   동영상성인 03/07
 		if(  strcmp(p_cmds1001r->sect_code,"02") == 0  || strcmp(p_cmds1001r->sect_code,"11") == 0  )
 		{
 			nRowcnt = 0;
 			nRowcnt = GetSectServerInfo(con, szServerGu, p_cmds1001r->sect_code, &t_cmds1001s);
			if(nRowcnt < 0)
			{
				ErrNum = -10014;
				sprintf(ErrMsg, "검색시 오류가 발생하였습니다.\n");
				infLOG(ERROR, "cmds1001->ERRMSG: [%d]",ErrNum);
				infLOG(ERROR, "cmds1001->DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));

				db_disconnect(con);
				E_dump(ErrNum, ErrMsg, pSendData);
				return(ErrNum);
			}

			if(nRowcnt == 1)
			{//질의 결과가 있다면
				goto Job_Success;
			}
		}

		//우선권을 가진 서버가 없고, 지정된 렉이 없다면 업로드 최적라인 선택하여 업로드 서버 질의 한다.
		nRowcnt = 0;
		nRowcnt = GetServerInfo(con, szServerGu, &t_cmds1001s);
		if(nRowcnt < 0)
		{
			ErrNum = -10015;
			sprintf(ErrMsg, "검색시 오류가 발생하였습니다.\n");
			infLOG(ERROR, "cmds1001->ERRMSG: [%d]",ErrNum);
			infLOG(ERROR, "cmds1001->DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));

			db_disconnect(con);
			E_dump(ErrNum, ErrMsg, pSendData);
			return(ErrNum);
		}

		if(nRowcnt == 1)
		{//질의 결과가 있다면
			goto Job_Success;
		}

		//선택가능한 업로드 서버가 없음. 에러 처리.
		ErrNum = -10016;
		sprintf(ErrMsg, "사용자가 많아서 서버에 접속 할수 없습니다.\n잠시후 다시 시도해 주십시오\n");
		infLOG(ERROR, "cmds1001->ERRMSG: [%d]. 업로드 가능 서버없음.",ErrNum);
		db_disconnect(con);
		E_dump(ErrNum, ErrMsg, pSendData);
		return(ErrNum);

	}
	else
	{
		ErrNum = -10017;
		sprintf(ErrMsg, "명시 되지 않은 서비스 입니다. 고객센터에 문의해 주세요\n");
		infLOG(ERROR, "cmds1001->ERRMSG: [%d]. find_flag 에러[%d]",ErrNum, find_flag);
		db_disconnect(con);
		E_dump(ErrNum, ErrMsg, pSendData);
		return(ErrNum);
	}



Job_Success:
	db_disconnect(con);

	//전송할 버퍼생성
	int nDataSize = HEADER_SIZE + sizeof(CCMDS1001_S) * 1;

	HEADER headers;
	memcpy(&headers, p_headerr, HEADER_SIZE); //header
	headers.nDataCnt  = 1;
	headers.nDataSize = sizeof(CCMDS1001_S);

	pSendData = new char[nDataSize];
	memset(pSendData, 0x00, nDataSize);

	memcpy(pSendData, &headers, sizeof(HEADER)); //header
	//20120222
	/*
	if( strlen(szUpToolName) > 0 )
	{
		strcat(t_cmds1001s.root_path,"[");
		strcat(t_cmds1001s.root_path,szUpToolName);
	}
	*/

	memcpy(pSendData+HEADER_SIZE, &t_cmds1001s, nDataSize - HEADER_SIZE); //body
//	delete[] t_cmds1001s;

	infLOG(ALWAY, "업로드 서버 선택 완료.\n");
	#ifdef _DEBUG_
	printf("cmds1001-> end sucess sendsize(%d)\n", nDataSize);
	#endif

	return (1);
}


int GetPriServerInfo(MYSQL *con, int nLimit, char* pServerGu, LPCCMDS1001_S pCmd1001_S)
{
	char szQuery[2048];
	int nResult = 1;

	MYSQL_RES *res;
	MYSQL_ROW  row;

	memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "select a.server_id,   a.server_ip,  "
		                 "       a.upsvr_port,  a.dnsvr_port, "
		                 "       a.root_path                  "
		                 "  from zangsi.T_SERVER_INFO a              "
		                 " where a.server_gu = '%s' and a.user_cnt = 300 and a.admin_open_yn = 'Y' and a.upload_yn = 'Y' "
		                 " and a.dn_user < 1500  "
		                 " order by  a.dn_user   limit 1 "
		        ,pServerGu);


	if(mysql_query(con, szQuery) || !(res = mysql_store_result(con)))
	{
     	return -1;
	}
 	if(mysql_num_rows(res) > 0)
 	{
 		row = mysql_fetch_row(res);
		#ifdef _DEBUG_
		printf("pCmd1001_S->server_id(%s)\n", getstr(row, 0));
		printf("pCmd1001_S->server_ip(%s)\n", getstr(row, 1));
		printf("pCmd1001_S->up_Port  (%d)\n", getint(row, 2));
		printf("pCmd1001_S->dn_port  (%d)\n", getint(row, 3));
		printf("pCmd1001_S->root_path(%s)\n", getstr(row, 4));
		#endif

		strcpy(pCmd1001_S->server_id, getstr(row, 0));
		strcpy(pCmd1001_S->server_ip, getstr(row, 1));
		       pCmd1001_S->up_Port  = getint(row, 2);
		       pCmd1001_S->dn_port  = getint(row, 3);
		strcpy(pCmd1001_S->root_path, getstr(row, 4));

		infLOG(ALWAY, "GetPriServerInfo: user_cnt = %d : %s : %s : %ld : %ld : %s \n"
					, nLimit, pCmd1001_S->server_id, pCmd1001_S->server_ip, pCmd1001_S->up_Port
					, pCmd1001_S->dn_port, pCmd1001_S->root_path);

 	}
 	else
 		nResult = 0;

	mysql_free_result(res);

	return nResult;
}

int GetSectServerInfo(MYSQL *con, char* pServerGu, char* pSectCode, LPCCMDS1001_S pCmd1001_S)
{
	char szQuery[2048];
	int nResult = 1;

	MYSQL_RES *res;
	MYSQL_ROW  row;

	//지정된 렉이 있는지 질의
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "select minor_name from zangsi.T_MINOR_CODE  "
					 "where major_code = '800' and minor_code = '%s' "
					 ,pServerGu);

	if(mysql_query(con, szQuery) || !(res = mysql_store_result(con)))
	{
     	return -1;
	}

	if(mysql_num_rows(res) == 0)
	{
		mysql_free_result(res);
		return 0;
	}

	row = mysql_fetch_row(res);
	char szLineCode[80];
	memset(szLineCode, 0x00, sizeof(szLineCode));
	strcpy(szLineCode, getstr(row,0));
	mysql_free_result(res);

	if(strcmp(szLineCode, "") ==0)
	{
		return 0;
	}

	//질의 결과가 있다면 지정된 렉에서 업로드 서버 선택
	memset (szQuery, 0x00, sizeof(szQuery ));
	sprintf(szQuery, "select a.server_id,   a.server_ip,                "
	                 "       a.upsvr_port,  a.dnsvr_port,               "
	                 "       a.root_path                                "
	                 "  from T_SERVER_INFO a                      		"
	                 " where a.server_gu = '%s' and a.upload_yn = 'Y'     "
	                 "    and a.dn_user < 35 and a.up_user < 1 and user_cnt not in(0,20,50,300)  and a.server_port in ( %s ) "
	                 " order by  a.user_cnt , a.up_user , a.dn_user limit 1 	"
	        		 ,pServerGu, szLineCode);

	if(mysql_query(con, szQuery) || !(res = mysql_store_result(con)))
	{
     	return -1;
	}
 	if(mysql_num_rows(res) > 0)
 	{
 		row = mysql_fetch_row(res);
		#ifdef _DEBUG_
		printf("pCmd1001_S->server_id(%s)\n", getstr(row, 0));
		printf("pCmd1001_S->server_ip(%s)\n", getstr(row, 1));
		printf("pCmd1001_S->up_Port  (%d)\n", getint(row, 2));
		printf("pCmd1001_S->dn_port  (%d)\n", getint(row, 3));
		printf("pCmd1001_S->root_path(%s)\n", getstr(row, 4));
		#endif

		strcpy(pCmd1001_S->server_id, getstr(row, 0));
		strcpy(pCmd1001_S->server_ip, getstr(row, 1));
		       pCmd1001_S->up_Port  = getint(row, 2);
		       pCmd1001_S->dn_port  = getint(row, 3);
		strcpy(pCmd1001_S->root_path, getstr(row, 4));

		infLOG(ALWAY, "GetSectServerInfo: %s : %s : %s : %ld : %ld : %s \n"
					, szLineCode, pCmd1001_S->server_id, pCmd1001_S->server_ip, pCmd1001_S->up_Port
					, pCmd1001_S->dn_port, pCmd1001_S->root_path);

 	}
 	else
 		nResult = 0;

	mysql_free_result(res);

	return nResult;
}


int GetServerInfo(MYSQL *con, char* pServerGu, LPCCMDS1001_S pCmd1001_S)
{
	char szQuery[2048];
	char szSelectLine[256];
	memset(szSelectLine, 0x00, sizeof(szSelectLine));
	int nResult = 1;

	MYSQL_RES *res;
	MYSQL_ROW  row;

	int nLineSymbol = 0;
	memset (szQuery, 0x00, sizeof(szQuery ));
	sprintf(szQuery, " select "
	                 " round(server_port / 100) ,sum(dn_user+up_user )/count(server_id) as total_user "
	                 " from zangsi.T_SERVER_INFO "
	                 " where server_gu ='%s' and admin_open_yn = 'Y' and upload_yn = 'Y' and user_cnt != 0 "
	                 " group by round(server_port / 100) HAVING sum(if(user_cnt = 100,1,0)) > 0 "
	                 " order by total_user limit 1 "
	                 , pServerGu);


	if(mysql_query(con, szQuery) || !(res = mysql_store_result(con)))
	{
     	return -1;
	}
 	if(mysql_num_rows(res) == 0)
	{
		mysql_free_result(res);
		return 0;
	}

	row = mysql_fetch_row(res);
	nLineSymbol = (int)getnum(row,0);
	mysql_free_result(res);

	// 수치가 가장낮은 라인에서 가장낮을 수치의 렉 선택

	memset (szQuery, 0x00, sizeof(szQuery ));
	sprintf(szQuery, "select "
	                 " server_port,sum(dn_user+up_user )/count(server_id) as total_user "
	                 " from zangsi.T_SERVER_INFO "
	                 " where server_gu ='%s' and admin_open_yn = 'Y' and upload_yn = 'Y' and user_cnt not in(0,300) "
	                 " and round(server_port / 100) = %d "
	                 " group by server_port "
	                 " order by total_user limit 1 "
	                 ,pServerGu, nLineSymbol);

	if(mysql_query(con, szQuery) || !(res = mysql_store_result(con)))
	{
     	return -1;
	}
 	if(mysql_num_rows(res) == 0)
	{
		mysql_free_result(res);
		return 0;
	}

	row = mysql_fetch_row(res);
	sprintf(szSelectLine," and server_port = %d ",(int)getnum(row,0));
	mysql_free_result(res);


	//선택된 렉에 포함된 서버중 다운로드수가 25보다 작고 업로드 수가 1보다 작은 서버 선택
	memset (szQuery, 0x00, sizeof(szQuery ));
	sprintf(szQuery, "select a.server_id,   a.server_ip, "
	                 "       a.upsvr_port,  a.dnsvr_port, "
	                 "       a.root_path "
	                 "  from T_SERVER_INFO a "
	                 " where a.server_gu = '%s' and admin_open_yn = 'Y'  and upload_yn = 'Y'  and user_cnt not in(0,20,50,300) "
	                 "    and dn_user < 25 and up_user < 1  %s "
	                 " order by  dn_user , up_user  limit 1 "
	        ,pServerGu,szSelectLine);


	if(mysql_query(con, szQuery) || !(res = mysql_store_result(con)))
	{
     	return -1;
	}
 	if(mysql_num_rows(res) > 0)
 	{
 		row = mysql_fetch_row(res);
		#ifdef _DEBUG_
		printf("pCmd1001_S->server_id(%s)\n", getstr(row, 0));
		printf("pCmd1001_S->server_ip(%s)\n", getstr(row, 1));
		printf("pCmd1001_S->up_Port  (%d)\n", getint(row, 2));
		printf("pCmd1001_S->dn_port  (%d)\n", getint(row, 3));
		printf("pCmd1001_S->root_path(%s)\n", getstr(row, 4));
		#endif

		strcpy(pCmd1001_S->server_id, getstr(row, 0));
		strcpy(pCmd1001_S->server_ip, getstr(row, 1));
		       pCmd1001_S->up_Port  = getint(row, 2);
		       pCmd1001_S->dn_port  = getint(row, 3);
		strcpy(pCmd1001_S->root_path, getstr(row, 4));

		infLOG(ALWAY, "GetServerInfo : %s : %s : %ld : %ld : %s \n"
					, pCmd1001_S->server_id, pCmd1001_S->server_ip, pCmd1001_S->up_Port
					, pCmd1001_S->dn_port, pCmd1001_S->root_path);

 	}
 	else
 	{
 		mysql_free_result(res);
		//선택된 렉에 포함된 서버중 다운로드수와 업로드 수가 제일 작은 서버 선택
		memset (szQuery, 0x00, sizeof(szQuery ));
		sprintf(szQuery, "select a.server_id,   a.server_ip, "
		                 "       a.upsvr_port,  a.dnsvr_port, "
		                 "       a.root_path "
		                 "  from T_SERVER_INFO a "
		                 " where a.server_gu = '%s' and a.admin_open_yn = 'Y'  and a.upload_yn = 'Y'  and a.user_cnt not in(0,20,50,300) "
		                 " %s "
		                 " order by  a.dn_user , a.up_user  limit 1 "
		        ,pServerGu,szSelectLine);


		if(mysql_query(con, szQuery) || !(res = mysql_store_result(con)))
		{
	     	return -1;
		}
	 	if(mysql_num_rows(res) > 0)
	 	{
	 		row = mysql_fetch_row(res);
			#ifdef _DEBUG_
			printf("pCmd1001_S->server_id(%s)\n", getstr(row, 0));
			printf("pCmd1001_S->server_ip(%s)\n", getstr(row, 1));
			printf("pCmd1001_S->up_Port  (%d)\n", getint(row, 2));
			printf("pCmd1001_S->dn_port  (%d)\n", getint(row, 3));
			printf("pCmd1001_S->root_path(%s)\n", getstr(row, 4));
			#endif

			strcpy(pCmd1001_S->server_id, getstr(row, 0));
			strcpy(pCmd1001_S->server_ip, getstr(row, 1));
			       pCmd1001_S->up_Port  = getint(row, 2);
			       pCmd1001_S->dn_port  = getint(row, 3);
			strcpy(pCmd1001_S->root_path, getstr(row, 4));

			infLOG(ALWAY, "GetServerInfo2 : %s : %s : %ld : %ld : %s \n"
						, pCmd1001_S->server_id, pCmd1001_S->server_ip, pCmd1001_S->up_Port
						, pCmd1001_S->dn_port, pCmd1001_S->root_path);

	 	}
	 	else
	 		nResult = 0;
 	}

	mysql_free_result(res);

	return nResult;
}

