/******************************************************************************
 *   서브시스템 : FDN서버
 *   프로그램명 : dcom9005.cc
 *         기능 : 다운로드 리스트 조회 시
 *         설명 : 다운로드 리스트 정보 조회
 *     수정이력 :
 *     수정내용 : 2.0 다운로드 일경우 OPDB에 접속하여 구매내역 검증.
 *                deal_type이 3이면 모든 검증 프로세스 그냥 통과.
*******************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#include <mysql.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../fdnsvr/inc/fdndefine.h"
#include "apdefine.h"
#include "comcomm.h"
#include "commydb.h"
#include "dcmd9005.h"
//#define  _DEBUG_
// db 제어 서버
#include <unistd.h>     /* for close() getpid()*/
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include "dcmdsock.h" //sock send recv
// db 제어 서버
#include "mysql_pool.h"

extern CMysqlPool * m_g_clMysqlPool;
//extern CMysqlPool * m_g_clMysqlPoolOp;
long dcmd9005(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData)//char* pUserID , char* pData ,char* pErrMsg)
{
	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);

	ERR_HEADER err_req_header;
	memset(&err_req_header , 0x00,ERR_HEADER_SIZE);
	err_req_header.header.nDataCnt = 1;
	err_req_header.header.nDataSize = ERR_HEADER_SIZE - HEADER_SIZE;

	char szQuery[10000];
	MYSQL       *con=NULL;
	MYSQL       *conOp=NULL;
	MYSQL_RES *res;
	MYSQL_ROW  row;

	LPFILEINFO pFileInfo = (LPFILEINFO)pRecvData;
	char user_id[13];
	memset(user_id,0x00,sizeof(user_id));

	unsigned long cont_id = 0;
	int disk_type = 0; //(0x004 위디스크 , 0x003 공개 , 0x005 내자료실 )
	int deal_type = 0; // 0 ID 1 Deal_ID

	if( deal_type ) // 구매 했는지 검사
		cont_id = pFileInfo->dwDealID;
	else
		cont_id = pFileInfo->nNumber;

	unsigned long ulDspID = 0;
	if(strlen(pFileInfo->szFileName) > 0)
		ulDspID = (unsigned long)atof(pFileInfo->szFileName);


	int  reg_count = 0;

	#ifdef _DEBUG_
	printf("dcom9005-> start-------------------------------\n");
	printf("dcom9005-> ulDspID = [%lu]\n", ulDspID);
	printf("dcom9005-> pFileInfo->nTypeDisk = [%d]\n", pFileInfo->nTypeDisk);
	printf("dcom9005-> pFileInfo->nType = [%d]\n", pFileInfo->nType);
	printf("dcom9005-> pFileInfo->nDealType = [%d]\n", pFileInfo->nDealType);
	printf("dcom9005-> pFileInfo->dwDealID = [%lu]\n", pFileInfo->dwDealID);
	printf("dcom9005-> pFileInfo->szServerID = [%s]\n", pFileInfo->szServerID);
	printf("dcom9005-> pFileInfo->szServerIP = [%s]\n", pFileInfo->szServerIP);
	printf("dcom9005-> pFileInfo->dwServerPort = [%d]\n", pFileInfo->dwServerPort);
	printf("dcom9005-> pFileInfo->nNumber = [%lu]\n", pFileInfo->nNumber);
	printf("dcom9005-> pFileInfo->szUserID = [%s]\n", pFileInfo->szUserID);
	printf("dcom9005-> pFileInfo->szFileOwnerID = [%s]\n", pFileInfo->szFileOwnerID);
	printf("dcom9005-> pFileInfo->szDownFileName = [%s]\n", pFileInfo->szDownFileName);
	printf("dcom9005-> pFileInfo->szFileName = [%s]\n", pFileInfo->szFileName);
	printf("dcom9005-> pFileInfo->szSrcPath = [%s]\n", pFileInfo->szSrcPath);
	printf("dcom9005-> pFileInfo->szDownPath = [%s]\n", pFileInfo->szDownPath);
	printf("dcom9005-> pFileInfo->szCerKey = [%s]\n", pFileInfo->szCerKey);
	#endif

	infLOG(ALWAY,"dcom9005-> nDealType = [%d] %lu\n", pFileInfo->nDealType,pFileInfo->dwDealID);

	//infLOG(ALWAY, "\ndcmd9005 \n");
	//cont_id = pFileInfo->nNumber;

	//strcpyA(user_id,pHeader->szUserID);
	memcpy(user_id,pHeader->szUserID,12);


	disk_type = pFileInfo->nTypeDisk;
	deal_type = pFileInfo->nDealType;

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	bool bCloseDB = false;
	//bool bCloseOpDB = false;

	CMysqlCon MysqlCon(m_g_clMysqlPool,m_g_clMysqlPool->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );

	con = MysqlCon.GetMysqlCon();


	if (con == NULL )
	{
		infLOG(ERROR, "DCMD9005 | GetMysqlCon is null\n");

		strcpyA(err_req_header.errmsg,"9005 에러 : DB에 접속하지 못 하였습니다.\n");

		int nRetry = 0;
		while (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			infLOG(ERROR, "DCMD9005 | Cannot DB Connect - GetMysqlCon is null\n");
			//infLOG(ERROR, "DCMD9005 | Mysql Error | Errno : %d | ErrMsg : %s \n\n", mysql_errno(con),mysql_error(con));
		}
		if( nRetry >= 5)
		{
			//req_header.nCmd = -1 ;
			err_req_header.header.nCmd = -1;
			strcpyA(err_req_header.errmsg,"9005 에러 : 서버접속 오류입니다.\n");

			pSendData = new char [ERR_HEADER_SIZE];
			memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);
			//memcpy(pSendData,&req_header,HEADER_SIZE);
			infLOG(ERROR, "DCMD9005 | Cannot DB Connect \n");
			//return HEADER_SIZE;
			return ERR_HEADER_SIZE;
	    }

	    bCloseDB = true;
	    infLOG(ERROR,"DCMD9005 | Connect DB direct\n");
	}


	#ifdef __DEBUG
	printf("Deal Type ] %d\n",deal_type);
	printf("Disk Type ] %d\n",disk_type);
	#endif


	//no.1127
	if(strstr(user_id, "'") != NULL || strstr(user_id, "\"") != NULL || strstr(user_id, " ") != NULL)
	{
		infLOG(ERROR, "DCMD9005 | 잘못된 user_id입니다. [%s]", user_id);

		ReplaceSingleQuotation(user_id, '\'',user_id);

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " INSERT INTO zangsi.T_HACKING_USER (server_gu, deal_no, user_id, reg_date, reg_time) "
						 " VALUES "
						 "('DCMD', %lu, '%s', date_format(now(),'%Y%m%d'), date_format(now(),'%H%i%s')) "
						 , cont_id
						 , user_id);

		if(mysql_query(con, szQuery))
		{
			infLOG(ERROR, "dcom9005[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			infLOG(ERROR, "dcom9005[SQL]: %s\n", szQuery);
		}
		err_req_header.header.nCmd = -1;
		strcpyA(err_req_header.errmsg,"9005 에러 : 잘못된 user_id입니다.\n");

		pSendData = new char [ERR_HEADER_SIZE];
		memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);
		if( bCloseDB )
			db_disconnect(con);
		return ERR_HEADER_SIZE;
	}
	
	if( deal_type ) // 구매 했는지 검사 1 이면 deal num
	{
		//----------------------------------------------------------------------
		// 구매정보 확인
		//----------------------------------------------------------------------

		memset (szQuery,  0x00, sizeof(szQuery));
		sprintf(szQuery, "SELECT a.id         			"
		                 "     , b.file_size  			"
		                 "     , '%s'  					"
		                 "     , b.file_path  			"
		                 "     , b.file_name1 			"
		                 "     , b.file_name2 			"
		                 "  FROM zangsi.T_DEAL_INFO a  		"
		                 "     , zangsi.T_DEAL_FILE b  		"
		                 " where a.deal_no  =  %lu		"
		                 "   and a.cont_gu  = b.cont_gu "
		                 "   and a.id       = b.id		"
		                 "   and a.buy_user = '%s'		"
		                 , pFileInfo->szServerID
		                 , cont_id
		                 , user_id);
	}
	else if(disk_type == 0x003 )//strcmp(cont_gu) == "FD(MY)")	// 공개자료실 내가 받을때
	{
		memset (szQuery,  0x00, sizeof(szQuery));
		sprintf(szQuery, "SELECT a.id         "
		                 "     , a.file_size  "
		                 "     , '%s'		  "
		                 "     , a.file_path  "
		                 "     , a.file_name1 "
		                 "     , a.file_name2 "
		                 "  FROM zangsi.T_CONTFLOG_FILE a "
		                 " where a.id       =  %lu"
		                 "   and a.reg_user = '%s'"
		                 ,pFileInfo->szServerID
		                 , cont_id
		                 , user_id);


		#ifdef _DEBUG_
		printf( "SELECT a.id         "
	                 "     , a.file_size  "
	                 "     , a.server_id  "
	                 "     , a.file_path  "
	                 "     , a.file_name1 "
	                 "     , a.file_name2 "
	                 "  FROM zangsi.T_CONTFLOG_FILE a "
	                 " where a.id       =  %lu"
	                 "   and a.reg_user = '%s'"
	                 , cont_id
	                 , user_id);

		#endif

	}
	else if(disk_type == 0x005 )//strcmp(cont_gu) == "MD")	// 파일금고 내가 받을때
	{
		memset (szQuery,  0x00, sizeof(szQuery));
		sprintf(szQuery, "SELECT a.id         "
		                 "     , a.file_size  "
		                 "     , a.server_id  "
		                 "     , a.file_path  "
		                 "     , a.file_name1 "
		                 "     , a.file_name2 "
		                 "  FROM zangsi.T_CONTDATA_MYFILE a "
		                 " where a.id       =  %lu"
		                 "   and a.reg_user = '%s'"
		                 , cont_id
		                 , user_id);
		#ifdef _DEBUG_
		printf( "SELECT a.id         "
		                 "     , a.file_size  "
		                 "     , a.server_id  "
		                 "     , a.file_path  "
		                 "     , a.file_name1 "
		                 "     , a.file_name2 "
		                 "  FROM zangsi.T_CONTDATA_MYFILE a "
		                 " where a.id       =  %lu"
		                 "   and a.reg_user = '%s'"
		                 , cont_id
		                 , user_id);
		#endif

	}
	else
	{
		if( disk_type ==  0x004 && !deal_type || deal_type == 3 ) //관리자.
		{//deal_type이 3이면 무조건 통과.

			infLOG(ALWAY,"관리자 다운 [ %lu ] [ %s ]\n",cont_id,user_id);
			//----------------------------------------------------------------------
			// 구매정보 확인
			//----------------------------------------------------------------------
			memset (szQuery,  0x00, sizeof(szQuery));
			sprintf(szQuery, "SELECT id         "
			                 "     , file_size  "
			                 "     , server_id  "
			                 "     , file_path  "
			                 "     , file_name1 "
			                 "     , file_name2 "
			                 "  FROM zangsi.T_CONTENTS_FILE  "
			                 "     where id = %lu			 "
			                 ,cont_id);

		#ifdef _DEBUG_
		printf( "SELECT id         "
			   "     , file_size  "
			   "     , server_id  "
			   "     , file_path  "
			   "     , file_name1 "
			   "     , file_name2 "
			   "  FROM zangsi.T_CONTENTS_FILE  "
			   "     where id = %lu			 "
			   ,cont_id);
		#endif


		}
		else
		{

			//오류
			infLOG(ERROR, "dcom9005[ERR]: 없는 서비스 입니다.\n");

			strcpyA(err_req_header.errmsg,"9005 에러 : 없는 서비스 입니다.\n");

			if( bCloseDB )
				db_disconnect(con);
/*
			if( bCloseOpDB )
				db_disconnect(conOp);
*/
			infLOG(ERROR, "dcom9005[SQL]: %s\n", szQuery);
			err_req_header.header.nCmd = -1 ;
			pSendData = new char[ERR_HEADER_SIZE];
			memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

	       	return ERR_HEADER_SIZE;
		}
	}

		if (mysql_query(con, szQuery))
		{
			infLOG(ERROR, "dcom9005[ERR]: SELECT T_CONTENTS_TEMP error\n");
			infLOG(ERROR, "dcom9005[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			strcpyA(err_req_header.errmsg,"90051 에러 : DB 에러 입니다.\n");
			if( bCloseDB )
				db_disconnect(con);
				/*
			if( bCloseOpDB )
				db_disconnect(conOp);
	*/
			infLOG(ERROR, "dcom9005[SQL]: %s\n", szQuery);
			err_req_header.header.nCmd = -2 ;
			pSendData = new char[ERR_HEADER_SIZE];
			memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

	       	return ERR_HEADER_SIZE;

		}
		if (!(res = mysql_store_result(con)))
		{
			infLOG(ERROR, "dcom9005[ERR]: mysql_store_result error\n");
			infLOG(ERROR, "dcom9005[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			strcpyA(err_req_header.errmsg,"90052 에러 : DB 에러 입니다.\n");
			if( bCloseDB )
				db_disconnect(con);
			/*
			if( bCloseOpDB )
				db_disconnect(conOp);
				*/
			infLOG(ERROR, "dcom9005[SQL]: %s\n", szQuery);
			req_header.nCmd = -2 ;
			pSendData = new char [ERR_HEADER_SIZE];
			memcpy(pSendData,&req_header,ERR_HEADER_SIZE);

	       	return ERR_HEADER_SIZE;
		}
	//}


 	if (mysql_num_rows(res)==0)
 	{
 		mysql_free_result(res);

 		if( (disk_type ==  0x004 && !deal_type) || deal_type == 3 ) //관리자.
		{//deal_type이 3이면 무조건 통과.

			infLOG(ALWAY,"관리자 다운2 [ %lu ] [ %s ]\n",cont_id,user_id);
			//----------------------------------------------------------------------
			// 구매정보 확인
			//----------------------------------------------------------------------
			memset (szQuery,  0x00, sizeof(szQuery));
			sprintf(szQuery, "SELECT id         "
			                 "     , file_size  "
			                 "     , server_id  "
			                 "     , file_path  "
			                 "     , file_name1 "
			                 "     , file_name2 "
			                 "  FROM zangsi.T_CONTENTS_FILE_DEL  "
			                 "     where id = %lu			 "
			                 ,cont_id);

			if (mysql_query(con, szQuery))
			{
				infLOG(ERROR, "dcom9005[ERR]: SELECT T_CONTENTS_TEMP error\n");
				infLOG(ERROR, "dcom9005[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				strcpyA(err_req_header.errmsg,"90051 에러 : DB 에러 입니다.\n");
				if( bCloseDB )
					db_disconnect(con);

				infLOG(ERROR, "dcom9005[SQL]: %s\n", szQuery);
				err_req_header.header.nCmd = -2 ;
				pSendData = new char[ERR_HEADER_SIZE];
				memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

		       	return ERR_HEADER_SIZE;

			}
			if (!(res = mysql_store_result(con)))
			{
				infLOG(ERROR, "dcom9005[ERR]: mysql_store_result error\n");
				infLOG(ERROR, "dcom9005[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				strcpyA(err_req_header.errmsg,"90052 에러 : DB 에러 입니다.\n");
				if( bCloseDB )
					db_disconnect(con);
				/*
				if( bCloseOpDB )
					db_disconnect(conOp);
					*/
				infLOG(ERROR, "dcom9005[SQL]: %s\n", szQuery);
				req_header.nCmd = -2 ;
				pSendData = new char [ERR_HEADER_SIZE];
				memcpy(pSendData,&req_header,ERR_HEADER_SIZE);

		       	return ERR_HEADER_SIZE;
			}
			if (mysql_num_rows(res)==0)
			{
				mysql_free_result(res);

				//구매 내역이 없음 , 예외처리
				if( deal_type ) // 구매 했는지 검사
				{
					strcpyA(err_req_header.errmsg,"90052 에러 : 구매 내역이 없습니다.\n");
				}
				else //파일정보가 없음
				{
					strcpyA(err_req_header.errmsg,"9005 에러 : 파일 정보가 없습니다.\n");
				}

				infLOG(ERROR, "dcom9005[ERR]: mysql_num_rows error\n");
				infLOG(ERROR, "dcom9005[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));

				if( bCloseDB )
					db_disconnect(con);
				/*
				if( bCloseOpDB )
					db_disconnect(conOp);
				*/

				infLOG(ERROR, "dcom9005[SQL]: %s\n", szQuery);
				err_req_header.header.nCmd = -2 ;
				pSendData = new char[ERR_HEADER_SIZE];
				memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

		       	return ERR_HEADER_SIZE;
			}

		}
		else
		{

	 		//구매 내역이 없음 , 예외처리
			if( deal_type ) // 구매 했는지 검사
			{
				strcpyA(err_req_header.errmsg,"90052 에러 : 구매 내역이 없습니다.\n");
			}
			else //파일정보가 없음
			{
				strcpyA(err_req_header.errmsg,"9005 에러 : 파일 정보가 없습니다.\n");
			}

			infLOG(ERROR, "dcom9005[ERR]: mysql_num_rows error\n");
			infLOG(ERROR, "dcom9005[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));

			if( bCloseDB )
				db_disconnect(con);
			/*
			if( bCloseOpDB )
				db_disconnect(conOp);
			*/

			infLOG(ERROR, "dcom9005[SQL]: %s\n", szQuery);
			err_req_header.header.nCmd = -2 ;
			pSendData = new char[ERR_HEADER_SIZE];
			memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

	       	return ERR_HEADER_SIZE;
	    }
	}

	row = mysql_fetch_row(res);



	pFileInfo->nNumber = (unsigned long)(getnum(row, 0));
	pFileInfo->dFileSize = getnum(row, 1);
	strcpyA(pFileInfo->szServerID ,  getstr(row, 2));
	strcpyA(pFileInfo->szSrcPath  , getstr(row, 3) );
	strcpyA(pFileInfo->szFileName ,  getstr(row, 4));
	strcpyA(pFileInfo->szDownFileName , getstr(row, 5) );


	mysql_free_result(res);

	if(ulDspID > 0)
	{
		infLOG(ALWAY, "dcmd9005-> 분산 처리 요청. 원본 ID = [%lu], 분산 ID = [%lu]\n", cont_id, ulDspID);
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "SELECT id         "
		                 "     , file_size  "
		                 "     , server_id  "
		                 "     , file_path  "
		                 "     , file_name1 "
		                 "     , file_name2 "
		                 "  FROM zangsi.T_CONTENTS_FILE  "
		                 "     where id = %lu			 "
		                 , ulDspID);


		if (mysql_query(con, szQuery))
		{
			infLOG(ERROR, "dcom9005[ERR]: 분산 처리 실패.\n");
			infLOG(ERROR, "dcom9005[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			infLOG(ERROR, "dcom9005[SQL]: %s\n", szQuery);
		}
		else
		{
			if (!(res = mysql_store_result(con)))
			{
				infLOG(ERROR, "dcom9005[ERR]: mysql_store_result error\n");
				infLOG(ERROR, "dcom9005[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
				infLOG(ERROR, "dcom9005[SQL]: %s\n", szQuery);
			}
			else
			{
				if (mysql_num_rows(res)==0)
				{
					infLOG(ERROR, "dcom9005[ERR]: 분산 처리 실패. 컨텐츠 없음.\n");
					infLOG(ERROR, "dcom9005[SQL]: %s\n", szQuery);
				}
				else
				{
					row = mysql_fetch_row(res);
					pFileInfo->nNumber = (unsigned long)(getnum(row, 0));
					pFileInfo->dFileSize = getnum(row, 1);
					strcpyA(pFileInfo->szServerID ,  getstr(row, 2));
					strcpyA(pFileInfo->szSrcPath  , getstr(row, 3) );
					strcpyA(pFileInfo->szFileName ,  getstr(row, 4));
					//strcpyA(pFileInfo->szDownFileName , getstr(row, 5) ); 파일명은 결제한 컨텐츠와 동일하게...

					#ifdef _DEBUG_
					printf("\n\n>분산처리 성공!!!\n");
					printf("dcom9005-> ulDspID = [%lu]\n", ulDspID);
					printf("dcom9005-> pFileInfo->nNumber = [%lu]\n", pFileInfo->nNumber);
					printf("dcom9005-> pFileInfo->dFileSize = [%.0f]\n", pFileInfo->dFileSize);
					printf("dcom9005-> pFileInfo->szServerID = [%s]\n", pFileInfo->szServerID);
					printf("dcom9005-> pFileInfo->szSrcPath = [%s]\n", pFileInfo->szSrcPath);
					printf("dcom9005-> pFileInfo->szFileName = [%s]\n", pFileInfo->szFileName);
					printf("dcom9005-> pFileInfo->szDownFileName = [%s]\n", pFileInfo->szDownFileName);
					printf("dcom9005-> pFileInfo->szDownPath = [%s]\n", pFileInfo->szDownPath);
					#endif
					infLOG(ALWAY, "\n\n>분산처리 성공!!!(%lu)\n", cont_id);
					infLOG(ALWAY, "dcmd9005-> ulDspID = [%lu]\n", ulDspID);
					infLOG(ALWAY, "dcmd9005-> pFileInfo->nNumber = [%lu]\n", pFileInfo->nNumber);
					infLOG(ALWAY, "dcmd9005-> pFileInfo->dFileSize = [%.0f]\n", pFileInfo->dFileSize);
					infLOG(ALWAY, "dcmd9005-> pFileInfo->szServerID = [%s]\n", pFileInfo->szServerID);
					infLOG(ALWAY, "dcmd9005-> pFileInfo->szSrcPath = [%s]\n", pFileInfo->szSrcPath);
					infLOG(ALWAY, "dcmd9005-> pFileInfo->szFileName = [%s]\n", pFileInfo->szFileName);
					infLOG(ALWAY, "dcmd9005-> pFileInfo->szDownFileName = [%s]\n", pFileInfo->szDownFileName);
					infLOG(ALWAY, "dcmd9005-> pFileInfo->szDownPath = [%s]\n", pFileInfo->szDownPath);

				}
				mysql_free_result(res);
			}
		}
	}



	if( deal_type ) // 구매 했는지 검사 1 이면 deal num
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update zangsi.T_DEAL_IP set cnt = cnt + 1 "
						 " where deal_no = %lu ; /* dcmd 9005 */"
						 , pFileInfo->dwDealID);

		if(mysql_query(con, szQuery))
		{
			//infLOG(ERROR, "dcom9005[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			infLOG(ERROR, "dcom9005[SQL]: %s\n", szQuery);
		}
	}

	if( bCloseDB )
		db_disconnect(con);
/*
	if( bCloseOpDB )
		db_disconnect(conOp);

*/
	unsigned long  dwSendLen = 0;

	req_header.nCmd = 9005;
	req_header.nDataCnt = 1;
	req_header.nDataSize = sizeof(FILEINFO);

	dwSendLen = HEADER_SIZE + req_header.nDataCnt * req_header.nDataSize ;

	pSendData = new char[dwSendLen];

	memcpy( pSendData , &req_header,HEADER_SIZE);

	if( req_header.nDataCnt * req_header.nDataSize > 0 )
		memcpy( pSendData + HEADER_SIZE, pFileInfo , req_header.nDataCnt * req_header.nDataSize);

	#ifdef __DEBUG
	printf(" ] dcmdHeader Check )\n"
		   "   nCmd = ( %d ) \n"
		   "   dcmdHeader.nDataCnt * dcmdHeader.nDataSize = ( %d ) * ( %d ) \n"
		   ,req_header.nCmd,req_header.nDataCnt,req_header.nDataSize);
	#endif

	return dwSendLen;
}
