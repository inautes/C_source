
/********************************************************************************
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
#include "dcmd5160.h"

#include "mysql_pool.h"
#include "MysqlDB.h"
#include "../../fdnsvr/inc/fdndefine.h" //

extern CMysqlPool *m_g_clMysqlPool;
extern CMysqlPool *m_g_clMysqlPoolHadoop;

void LogView(LPFILEINFO pRecv)
{

	infLOG(ALWAY,"nTypeDisk   	%d\n",pRecv->nTypeDisk	);
	infLOG(ALWAY,"nType			%d\n",pRecv->nType		);
	infLOG(ALWAY,"nDealType   	%d\n",pRecv->nDealType	);
	infLOG(ALWAY,"DealID	    	%d\n",pRecv->dwDealID	);
	infLOG(ALWAY,"szServerID   	%s\n",pRecv->szServerID);
	infLOG(ALWAY,"szServerIP   	%s\n",pRecv->szServerIP);
	infLOG(ALWAY,"dwServerPort   	%d\n",pRecv->dwServerPort);
	infLOG(ALWAY,"nNumber			%lu\n",pRecv->nNumber);
	infLOG(ALWAY,"szUserID      	%s\n", pRecv->szUserID     );
	infLOG(ALWAY,"szFileOwnerID 	%s\n", pRecv->szFileOwnerID);
	infLOG(ALWAY,"szDownFileName	%s\n", pRecv->szDownFileName);
	infLOG(ALWAY,"szFileName    	%s\n", pRecv->szFileName   );
	infLOG(ALWAY,"szSrcPath     	%s\n", pRecv->szSrcPath    );
	infLOG(ALWAY,"szDownPath    	%s\n", pRecv->szDownPath   );
	infLOG(ALWAY,"dFileSize     	%.0f\n",pRecv->dFileSize);

}
//******************************************************************************
//  COM5160 main
//******************************************************************************
long dcmd5160(int sock , LPHEADER pRecvHead , char* pRecvData , char* &pSendData)//CCOM9106_R pcom9106_r)
{
	if( pRecvHead == NULL ) infLOG(ERROR, "pRecvHead == NULL\n");
	if( pRecvData == NULL )	infLOG(ERROR, "pRecvData == NULL\n");

	LPHEADER pHeader = (LPHEADER)pRecvHead;
	ERR_HEADER errheader; 	memset(&errheader,0x00,sizeof(ERR_HEADER));
	HEADER headers; 		memset(&headers,0x00,sizeof(HEADER));

	LPFILEINFO pRecv = (LPFILEINFO)pRecvData;

	#ifdef __DEBUG
	LogView(pRecv);
	#endif

	int nRetry = 0;

// main db 연결
	CMysqlDB main_db;
	main_db.setDebug(true);
	MYSQL       *con=NULL;

	CMysqlCon MysqlCon;
	MysqlCon.ConnectPool(m_g_clMysqlPool,m_g_clMysqlPool->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );
	main_db.setCon(MysqlCon.GetMysqlCon());

	if ( main_db.getCon() == NULL )
	{
		infLOG(ERROR, "dcmd5160: GetMysqlCon is null\n");
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_FILE_REQUEST_LIST;

		while (!(con=db_connect("nori")) && nRetry < 5 ) {
			nRetry++;sleep(1);
		}

		if( nRetry >= 5){
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&errheader,HEADER_SIZE);
			infLOG(ALWAY,"ERROR 1\n");
			return HEADER_SIZE;
	    }

	    main_db.setCon(con);
	    main_db.setUseDisconnect(true);
	}

// 통합 db 연결
	nRetry = 0;

	CMysqlDB hadoop_db;
	hadoop_db.setDebug(true);
	MYSQL       *con_hadoop=NULL;

	CMysqlCon MysqlConHadoop;
	MysqlConHadoop.ConnectPool(m_g_clMysqlPoolHadoop,m_g_clMysqlPoolHadoop->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );
	hadoop_db.setCon(MysqlConHadoop.GetMysqlCon());

	if ( hadoop_db.getCon() == NULL )
	{
		infLOG(ERROR, "dcmd5160: GetMysqlCon is null\n");
		errheader.header.nCmd = RS_ERR;
		errheader.header.nErrorCode = -RS_FILE_REQUEST_LIST;

		while (!(con_hadoop=Hadoop_db_connect("cnfs")) && nRetry < 5 ){
			nRetry++;sleep(1);
		}

		if( nRetry >= 5){
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&errheader,HEADER_SIZE);
			infLOG(ALWAY,"ERROR 2\n");
			return HEADER_SIZE;
	    }

	    hadoop_db.setCon(con_hadoop);
	    hadoop_db.setUseDisconnect(true);
	}

	infLOG(ALWAY,"stemp 1\n");


	///////////////////////////////////
	// ID 체크 : 필요 없는듯 한데... 
	if(strstr(pRecv->szUserID, "'") != NULL || strstr(pRecv->szUserID, "\"") != NULL || strstr(pRecv->szUserID, " ") != NULL)
	{
		//infLOG(ERROR, "DCMD9005 | 잘못된 user_id입니다. [%s]", pRecv->szUserID);

		ReplaceSingleQuotation(user_id, '\'',user_id);

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " INSERT INTO zangsi.T_HACKING_USER (server_gu, deal_no, user_id, reg_date, reg_time) "
						 " VALUES "
						 "('DCMD', %lu, '%s', date_format(now(),'%Y%m%d'), date_format(now(),'%H%i%s')) "
						 , pRecv->dwDealID
						 , pRecv->szUserID);

		infLOG(ALWAY,"szQuery = %s\n",szQuery);

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

	///////////////////////////
	// 구매정보 확인
	memset (szQuery,  0x00, sizeof(szQuery));
	if( deal_type ) // 구매 했는지 검사 1 이면 deal num
	{
		sprintf(szQuery, "SELECT a.id FROM zangsi.T_DEAL_INFO a  where a.deal_no  =  %lu	and a.buy_user = '%s';	"
		                 , pRecv->dwDealID , pRecv->szUserID);

		infLOG(ALWAY,"szQuery = %s\n",szQuery);

		if (mysql_query(con, szQuery))
		{
			strcpyA(err_req_header.errmsg,"5160 에러 : DB 에러 입니다.\n");
			if( bCloseDB )
				db_disconnect(con);
		
			err_req_header.header.nCmd = -2 ;
			pSendData = new char[ERR_HEADER_SIZE];
			memcpy(pSendData,&err_req_header,ERR_HEADER_SIZE);

			return ERR_HEADER_SIZE;

		}

		if (!(res = mysql_store_result(con)))
		{
			strcpyA(err_req_header.errmsg,"5160 에러 : DB 에러 입니다.\n");
			if( bCloseDB )
				db_disconnect(con);

			req_header.nCmd = -2 ;
			pSendData = new char [ERR_HEADER_SIZE];
			memcpy(pSendData,&req_header,ERR_HEADER_SIZE);

			return ERR_HEADER_SIZE;
		}
	}

///////////////////////////
	int ret = 0;
	// 다운로드 목록 만들기.
	char szQuery[1000];  // query string
	memset (szQuery, 0x00, sizeof(szQuery));

	infLOG(ALWAY,"pRecv->szUserID = %s, pRecv->nType = %d\n",pRecv->szUserID,pRecv->nType);

	//	if(strcmp(folder_yn ,"N") == 0)
	if( pRecv->nType == FT_FILE)
	{ // -- 파일일때

		if( pRecv->nDealType == 0) // 관리자
		{
			if(pRecv->nTypeDisk == 0x003)
			{ // FD 필로그
				sprintf(szQuery," SELECT a.server_id,c.server_ip, a.file_path,a.file_name1,a.file_name2, b.default_hash, b.file_size, b.hdfs_status , c.dnsvr_port , ifnull(d.file_id,0) , ifnull(d.uniq_id,0), d.use_yn "
								" FROM zangsi.T_CONTFLOG_FILE a, zangsi.T_SERVER_INFO c, zangsi.T_CONTFLOG_FILELIST_SUB b left join  zangsi.T_CONTENTS_FILELIST_REAL d on b.file_id = d.file_id AND d.use_yn = 'Y' "
								" WHERE a.server_id = c.server_id and a.id = b.id  and a.id = %lu ; "
							,pRecv->nNumber );
			}
			else if(pRecv->nTypeDisk == 0x005)
			{ // MD 내디스크
			}
			else if(pRecv->nTypeDisk == 0x004)
			{ // WE 일반
				sprintf(szQuery," SELECT a.server_id, c.server_ip, a.file_path,a.file_name1,a.file_name2, b.default_hash, b.file_size, b.hdfs_status , c.dnsvr_port , ifnull(d.file_id,0) , ifnull(d.uniq_id,0), d.use_yn "
								" FROM zangsi.T_CONTENTS_FILE a, zangsi.T_SERVER_INFO c, zangsi.T_CONTENTS_FILELIST_SUB b left join  zangsi.T_CONTENTS_FILELIST_REAL d on b.file_id = d.file_id AND d.use_yn = 'Y' "  
								" WHERE a.server_id = c.server_id and a.id = b.id  and a.id = %lu ; "
							,pRecv->nNumber );
			}
		}
		else if( pRecv->nDealType == 1) // 일반
		{
			if(pRecv->nTypeDisk == 0x003)
			{ // FD 필로그
				sprintf(szQuery," SELECT a.server_id,c.server_ip, a.file_path,a.file_name1,a.file_name2, b.default_hash, b.file_size, b.hdfs_status , c.dnsvr_port , ifnull(d.file_id,0) , ifnull(d.uniq_id,0), d.use_yn "
								 " FROM zangsi.T_CONTFLOG_FILE a, zangsi.T_SERVER_INFO c, zangsi.T_DEAL_INFO e, zangsi.T_CONTFLOG_FILELIST_SUB b left join  zangsi.T_CONTENTS_FILELIST_REAL d on b.file_id = d.file_id AND d.use_yn = 'Y' " 
								 " WHERE a.server_id = c.server_id and a.id = b.id   and a.id = e.id and e.cont_gu='WE' and e.deal_no = %lu ; "
								,pRecv->nNumber );
			}
			else if(pRecv->nTypeDisk == 0x005)
			{ // MD 내디스크
			}
			else if(pRecv->nTypeDisk == 0x004)
			{ // WE 일반
				sprintf(szQuery," SELECT a.server_id,c.server_ip, a.file_path,a.file_name1,a.file_name2, b.default_hash, b.file_size, b.hdfs_status , c.dnsvr_port , ifnull(d.file_id,0) , ifnull(d.uniq_id,0), d.use_yn " 
								" FROM zangsi.T_CONTENTS_FILE a, zangsi.T_SERVER_INFO c, zangsi.T_DEAL_INFO e, zangsi.T_CONTENTS_FILELIST_SUB b left join  zangsi.T_CONTENTS_FILELIST_REAL d on b.file_id = d.file_id AND d.use_yn = 'Y' " 
								" WHERE a.server_id = c.server_id and a.id = b.id  and a.id = e.id and e.cont_gu='FD' and e.deal_no = %lu ; "
							,	pRecv->nNumber );
			}
		}
		else
		{
			infLOG(ERROR, "com5160 : error pRecv->nDealType = %d\n",pRecv->nDealType);
		}


		ret = main_db.num_rows(szQuery);

		if ( ret <=0 )
		{
			main_db.free_result();
			main_db.disconnect_db();

			infLOG(ERROR, "com5160 : query [ %s ] \n 1. --> error msg : [ %s ]\n",main_db.getQueryString(),main_db.getErrorMessage());

			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_FILE_REQUEST_LIST;
			strcpy(errheader.errmsg,"목록을 가져올수 없습니다. (0)");
			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

			return ERR_HEADER_SIZE;
		}
		else
		{
			 while(main_db.fetch_row() != NULL)
			 {
				infLOG(ALWAY,"stemp 6\n");

				// 파일 하나
				unsigned long uFile_id = 0;
				unsigned long uUniq_id = 0;
				char szHdfs_status[10];		memset(szHdfs_status,0x00,sizeof(szHdfs_status));
				char szFullPath[1024];		memset(szFullPath,0x00,sizeof(szFullPath));
				char szUse_yn[2];			memset(szUse_yn,0x00,sizeof(szUse_yn));

				strcpy(pRecv->szDownFileName,main_db.getstr(4));	// 저장 파일명
				pRecv->dFileSize = (unsigned int) main_db.getnum(6);
				strcpy(szHdfs_status,main_db.getstr(7));
				pRecv->dwServerPort = (long)main_db.getnum(8);
				uFile_id = (unsigned long)main_db.getnum(9);
				uUniq_id = (unsigned long)main_db.getnum(10);
				strcpy(szUse_yn,main_db.getstr(11));

				strcpy(pRecv->szDownPath,""); // 클라이언트의 경로를 가진다.

				if( strcmp(szHdfs_status,"H") == 0 && uUniq_id > 0 )
				{	// 통합 서버에서 조회
					memset (szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery," SELECT a.server_id, b.server_ip, c.file_path, c.file_name, c.file_size, c.default_hash, b.dnsvr_port "
									" FROM cnfs.T_CONTENTS_MAP a, cnfs.T_SERVER_INFO b, cnfs.T_CONTENTS_FILELIST_UNIQ c "
									" WHERE a.server_id = b.server_id AND b.down_yn = 'Y' and c.uniq_id = a.uniq_id AND a.uniq_id = %lu "
									" ORDER BY b.dn_user; ", uFile_id );

					ret = hadoop_db.num_rows(szQuery);

					memset (szQuery, 0x00, sizeof(szQuery));

					if ( ret <=0 )
					{
						hadoop_db.free_result();
						hadoop_db.disconnect_db();

						infLOG(ERROR, "com5160 : query [ %s ] \n 1. --> error msg : [ %s ]\n",hadoop_db.getQueryString(),hadoop_db.getErrorMessage());

						pSendData = new char[sizeof(ERR_HEADER)];
						memset(pSendData,0x00,sizeof(ERR_HEADER));
						errheader.header.nCmd = RS_ERR;
						errheader.header.nErrorCode = -RS_FILE_REQUEST_LIST;
						strcpy(errheader.errmsg,"목록을 가져올수 없습니다. (0)");
						memcpy(pSendData, &errheader, sizeof(ERR_HEADER));
						return ERR_HEADER_SIZE;
					}
					else
					{

						 while(hadoop_db.fetch_row() != NULL)
						 {
							strcpy(pRecv->szServerID,hadoop_db.getstr(0));
							strcpy(pRecv->szServerIP,hadoop_db.getstr(1));
							strcpy(pRecv->szFileName,hadoop_db.getstr(3));
							pRecv->dwServerPort = (long)hadoop_db.getnum(6);

							pRecv->nNumber = uFile_id; // 2015.11.06 cont_id -> file_id로 변경.

							strcpy(szFullPath,hadoop_db.getstr(2)); // 소스 폴더 경로

							if( strcmp(hadoop_db.getstr(5),"") == 0)
							{
									strcat(szFullPath,"|");
									strcat(szFullPath,hadoop_db.getstr(5)); // default_hash
									strcat(szFullPath,"|");
							}
							ReplaceSingleQuotation(szFullPath, '\'',szFullPath);
							strcpy(pRecv->szSrcPath,szFullPath);	// 소스 폴더 경로
							infLOG(ALWAY,"pRecv->szSrcPath = %s \n",pRecv->szSrcPath);
						}
					}
		 			hadoop_db.free_result();
					hadoop_db.disconnect_db();


				}
				else if( strcmp(szHdfs_status,"C") == 0 && uFile_id > 1 )
				{
					memset (szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery," SELECT a.server_id, b.server_ip, a.file_path, a.file_name, a.file_size, a.default_hash, b.dnsvr_port "
									 " FROM T_CONTENTS_FILELIST_REAL a, T_SERVER_INFO b "
									 " WHERE a.server_id = b.server_id AND a.use_yn = 'Y' AND b.admin_open_yn = 'Y' AND a.file_id = %lu "
									 " ORDER BY b.dn_user LIMIT 1; ", uFile_id ); 

					ret = hadoop_db.num_rows(szQuery);

					if ( ret <=0 )
					{
						hadoop_db.free_result();
						hadoop_db.disconnect_db();

						pSendData = new char[sizeof(ERR_HEADER)];
						memset(pSendData,0x00,sizeof(ERR_HEADER));
						errheader.header.nCmd = RS_ERR;
						errheader.header.nErrorCode = -RS_FILE_REQUEST_LIST;
						strcpy(errheader.errmsg,"목록을 가져올수 없습니다. (1)");
						memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

						return ERR_HEADER_SIZE;
					}
					else
					{

						 while(hadoop_db.fetch_row() != NULL)
						 {
							strcpy(pRecv->szServerID,hadoop_db.getstr(0));
							strcpy(pRecv->szServerIP,hadoop_db.getstr(1));
							strcpy(pRecv->szFileName,hadoop_db.getstr(3));
							pRecv->dwServerPort = (long)hadoop_db.getnum(6);

							pRecv->nNumber = uFile_id; // 2015.11.06 cont_id -> file_id로 변경.

							strcpy(szFullPath,hadoop_db.getstr(2)); // 소스 폴더 경로

							if( strcmp(hadoop_db.getstr(5),"") == 0)
							{
									strcat(szFullPath,"|");
									strcat(szFullPath,hadoop_db.getstr(5)); // default_hash
									strcat(szFullPath,"|");
							}
							ReplaceSingleQuotation(szFullPath, '\'',szFullPath);
							strcpy(pRecv->szSrcPath,szFullPath);	// 소스 폴더 경로
							infLOG(ALWAY,"pRecv->szSrcPath = %s \n",pRecv->szSrcPath);
						}
					}
		 			hadoop_db.free_result();
					hadoop_db.disconnect_db();

				}
				else
				{
					infLOG(ALWAY,"stemp 88\n");
					strcpy(pRecv->szServerID,main_db.getstr(0));
					strcpy(pRecv->szServerIP,main_db.getstr(1));
					strcpy(pRecv->szFileName,main_db.getstr(3)); // 소스 파일명

					strcpy(szFullPath,main_db.getstr(2)); // 소스 폴더 경로

					if( strcmp(main_db.getstr(5),"") == 0)
					{
						strcat(szFullPath,"|");
						strcat(szFullPath,main_db.getstr(5)); // default_hash
						strcat(szFullPath,"|");
					}
					ReplaceSingleQuotation(szFullPath, '\'',szFullPath);
					strcpy(pRecv->szSrcPath,szFullPath); // 소스 폴더 경로
					infLOG(ALWAY,"pRecv->szSrcPath = %s \n",pRecv->szSrcPath);
				}
			 }
 			main_db.free_result();

			if( pRecv->nDealType ) // 구매 했는지 검사 1 이면 deal num
			{
				ret = main_db.query("  update zangsi.T_DEAL_IP set cnt = cnt + 1 "
								" where deal_no = %lu"
								, pRecv->dwDealID);

				if( ret != 0 )
					 infLOG(ERROR, "com9005 cnt+1 : query [ %s ] \nerror msg : [ %s ]\n",main_db.getQueryString(),main_db.getErrorMessage());

			}

			main_db.disconnect_db();
		}

		headers.nCmd = RS_FILE_REQUEST_LIST;
		headers.nDataCnt = 1;
		headers.nDataSize = sizeof(FILEINFO);
		headers.nErrorCode = 0;

		pSendData = new char[sizeof(HEADER) + headers.nDataCnt * headers.nDataSize];
		memset(pSendData,0x00,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize);
		memcpy(pSendData,&headers,sizeof(HEADER)); //head
		memcpy(pSendData+sizeof(HEADER),pRecv,headers.nDataCnt * headers.nDataSize);
	}
//========================================================================================================
//	else if (strcmp(folder_yn ,"Y") == 0)

//	char szDownFileName[256]; // 저장 파일명
//	char szDownPath[512]; // 저장 폴더 경로
//	char szFileName[256]; // 소스 파일명
//	char szSrcPath[512];  // 소스 폴더 경로

	memset (szQuery, 0x00, sizeof(szQuery));

	if( pRecv->nType == FT_FOLDER)
	{// -- 폴더일때

		if( pRecv->nDealType == 0) // 관리자
		{
			if(pRecv->nTypeDisk == 0x003)
			{ // FD 필로그

				sprintf(szQuery," SELECT a.server_id,c.server_ip, CONCAT(a.file_path,'/',a.file_name1),b.file_name, a.file_name2,b.folder_path, b.file_name, b.default_hash , b.file_size, b.hdfs_status , c.dnsvr_port, ifnull(d.file_id,0) , ifnull(d.uniq_id,0), d.use_yn"
								" FROM zangsi.T_CONTFLOG_FILE a, zangsi.T_SERVER_INFO c, zangsi.T_CONTFLOG_FILELIST_SUB b left join  zangsi.T_CONTENTS_FILELIST_REAL d on b.file_id = d.file_id AND d.use_yn = 'Y' "
								" WHERE a.server_id = c.server_id and a.id = b.id and a.id = %lu order by b.file_name "
								,pRecv->nNumber );


			}
			else if(pRecv->nTypeDisk == 0x005)
			{ // MD 내디스크
	
			}
			else if(pRecv->nTypeDisk == 0x004)
			{ // WE 일반
				sprintf(szQuery," SELECT a.server_id,c.server_ip, CONCAT(a.file_path,'/',a.file_name1),b.file_name, a.file_name2,b.folder_path, b.file_name, b.default_hash , b.file_size, b.hdfs_status , c.dnsvr_port, ifnull(d.file_id,0) , ifnull(d.uniq_id,0), d.use_yn"
								" FROM zangsi.T_CONTENTS_FILE a, zangsi.T_SERVER_INFO c, zangsi.T_CONTENTS_FILELIST_SUB b left join  zangsi.T_CONTENTS_FILELIST_REAL d on b.file_id = d.file_id AND d.use_yn = 'Y' "
								" WHERE a.server_id = c.server_id and a.id = b.id and a.id = %lu order by b.file_name "
								,pRecv->nNumber );
			}
		}
		else if( pRecv->nDealType == 1) // 일반
		{
			if(pRecv->nTypeDisk == 0x003)
			{ // FD 필로그
				sprintf(szQuery," SELECT a.server_id,c.server_ip, CONCAT(a.file_path,'/',a.file_name1),b.file_name, a.file_name2,b.folder_path, b.file_name, b.default_hash , b.file_size, b.hdfs_status , c.dnsvr_port, ifnull(d.file_id,0) , ifnull(d.uniq_id,0), d.use_yn"
								" FROM zangsi.T_CONTFLOG_FILE a, zangsi.T_SERVER_INFO c, zangsi.T_DEAL_INFO e, zangsi.T_CONTFLOG_FILELIST_SUB b left join  zangsi.T_CONTENTS_FILELIST_REAL d on b.file_id = d.file_id AND d.use_yn = 'Y' "
								" WHERE a.server_id = c.server_id and a.id = b.id and a.id = e.id and e.cont_gu='FD' and e.deal_no = %lu  order by b.file_name "
								,pRecv->nNumber );


			}
			else if(pRecv->nTypeDisk == 0x005)
			{ // MD 내디스크

			}
			else if(pRecv->nTypeDisk == 0x004)
			{ // WE 일반
				sprintf(szQuery," SELECT a.server_id,c.server_ip, CONCAT(a.file_path,'/',a.file_name1),b.file_name, a.file_name2,b.folder_path, b.file_name, b.default_hash , b.file_size, b.hdfs_status , c.dnsvr_port, ifnull(d.file_id,0) , ifnull(d.uniq_id,0), d.use_yn"
								" FROM zangsi.T_CONTENTS_FILE a, zangsi.T_SERVER_INFO c, zangsi.T_DEAL_INFO e, zangsi.T_CONTENTS_FILELIST_SUB b left join  zangsi.T_CONTENTS_FILELIST_REAL d on b.file_id = d.file_id AND d.use_yn = 'Y' " 
								" WHERE a.id = d.id and a.server_id = c.server_id and a.id = b.id and d.deal_no = %lu and d.cont_gu='WE' order by b.file_name "
								,pRecv->nNumber );
			}
		}
		else
		{
			infLOG(ERROR, "com5160 : error pRecv->nDealType = %d\n",pRecv->nDealType);
		}


		ret = main_db.num_rows(szQuery);

		if ( ret <=0 )
		{
			main_db.free_result();
			main_db.disconnect_db();

			infLOG(ERROR, "com5160 : query [ %s ] \n 1. --> error msg : [ %s ]\n",main_db.getQueryString(),main_db.getErrorMessage());

			pSendData = new char[sizeof(ERR_HEADER)];
			memset(pSendData,0x00,sizeof(ERR_HEADER));
			errheader.header.nCmd = RS_ERR;
			errheader.header.nErrorCode = -RS_FILE_REQUEST_LIST;
			strcpy(errheader.errmsg,"목록을 가져올수 없습니다. (2)");
			memcpy(pSendData, &errheader, sizeof(ERR_HEADER));

			return ERR_HEADER_SIZE;
		}
		else
		{
			LPFILEINFO pResult = new FILEINFO[2001]; //2000개의 목록을 보낼수 있다.
			memset(pResult,0x00,sizeof(FILEINFO)*2001);
			int nFileCount = 0;

			 while(main_db.fetch_row() != NULL)
			 {
				memcpy(&pResult[nFileCount],pRecvData,sizeof(FILEINFO));
				// 폴더
				unsigned long uFile_id = 0;
				unsigned long uUniq_id = 0;
				char szHdfs_status[10];		memset(szHdfs_status,0x00,sizeof(szHdfs_status));
				char szFullPath[1024];		memset(szFullPath,0x00,sizeof(szFullPath));
				char szUse_yn[2];			memset(szUse_yn,0x00,sizeof(szUse_yn));

				pResult[nFileCount].dFileSize = (double) main_db.getnum(8);

				strcpy(szHdfs_status,main_db.getstr(9));
				pResult[nFileCount].dwServerPort = (long)main_db.getnum(10);
				uFile_id = (unsigned long) main_db.getnum(11);
				uUniq_id = (unsigned long) main_db.getnum(12);
				strcpy(szUse_yn,main_db.getstr(13));			

				if(strcmp(main_db.getstr(5),"") == 0)
				 	strcpy(pResult[nFileCount].szDownPath,main_db.getstr(4)); //
				else
					sprintf(pResult[nFileCount].szDownPath,"%s/%s", main_db.getstr(4),main_db.getstr(5));

				strcpy(pResult[nFileCount].szDownFileName,main_db.getstr(6));	// 저장 파일명


				if( strcmp(szHdfs_status,"H") == 0 )
				{
					memset (szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery," SELECT a.server_id, b.server_ip, c.file_path, c.file_name, c.file_size, c.default_hash, b.dnsvr_port "
									" FROM cnfs.T_CONTENTS_MAP a, cnfs.T_SERVER_INFO b, cnfs.T_CONTENTS_FILELIST_UNIQ c "
									" WHERE a.server_id = b.server_id AND b.down_yn = 'Y' and c.uniq_id = a.uniq_id AND a.uniq_id = %lu "
									" ORDER BY b.dn_user; ", uFile_id );

					if ( ret <=0 )
					{
						hadoop_db.free_result();
						hadoop_db.disconnect_db();

						pSendData = new char[sizeof(ERR_HEADER)];
						memset(pSendData,0x00,sizeof(ERR_HEADER));
						errheader.header.nCmd = RS_ERR;
						errheader.header.nErrorCode = -RS_FILE_REQUEST_LIST;
						strcpy(errheader.errmsg,"목록을 가져올수 없습니다. (3)");
						memcpy(pSendData, &errheader, sizeof(ERR_HEADER));
						delete[] pResult;
						return ERR_HEADER_SIZE;

					}
					else
					{

						 while(hadoop_db.fetch_row() != NULL)
						 {
							strcpy(pResult[nFileCount].szServerID,hadoop_db.getstr(0));
							strcpy(pResult[nFileCount].szServerIP,hadoop_db.getstr(1));
							strcpy(pResult[nFileCount].szFileName,hadoop_db.getstr(3));
							pResult[nFileCount].dwServerPort = (long)hadoop_db.getnum(6);

							pResult[nFileCount].nNumber = uFile_id; // 2015.11.06 cont_id -> file_id로 변경.

							strcpy(szFullPath,hadoop_db.getstr(2)); // 원본 파일

							if( strcmp(hadoop_db.getstr(5),"") == 0)
							{
									strcat(szFullPath,"|");
									strcat(szFullPath,hadoop_db.getstr(5)); // default_hash
									strcat(szFullPath,"|");
							}
							ReplaceSingleQuotation(szFullPath, '\'',szFullPath);

							strcpy(pResult[nFileCount].szSrcPath,szFullPath);
							infLOG(ALWAY,"pResult[%d].szSrcPath = %s \n",nFileCount,pResult[nFileCount].szSrcPath);
						}
					}
		 			hadoop_db.free_result();
					hadoop_db.disconnect_db();

				}
				else
				{
					strcpy(pResult[nFileCount].szServerID,main_db.getstr(0));
					strcpy(pResult[nFileCount].szServerIP,main_db.getstr(1));
					strcpy(pResult[nFileCount].szFileName,main_db.getstr(3)); // 소스 파일명
					// pResult[nFileCount].dFileSize = (double) main_db.getnum(6);

					strcpy(szFullPath,main_db.getstr(2)); // file_path

					if(strcmp(main_db.getstr(5),"") == 0)
						strcpy(szFullPath,main_db.getstr(2)); //
					else
						sprintf(szFullPath,"%s/%s", main_db.getstr(2),main_db.getstr(5));

					if( strcmp(main_db.getstr(6),"") == 0)
					{
						strcat(szFullPath,"|");
						strcat(szFullPath,main_db.getstr(6)); // default_hash
						strcat(szFullPath,"|");
					}
					ReplaceSingleQuotation(szFullPath, '\'',szFullPath);
					strcpy(pResult[nFileCount].szSrcPath,szFullPath);
					infLOG(ALWAY,"pResult[%d].szSrcPath = %s \n",nFileCount,pResult[nFileCount].szSrcPath);
				}

				 //memcpy(&pResult[nFileCount],pRecv,sizeof(FILEINFO));
				 nFileCount++;


			 }
 			main_db.free_result();

			if( pRecv->nDealType ) // 구매 했는지 검사 1 이면 deal num
			{
				ret = main_db.query("  update zangsi.T_DEAL_IP set cnt = cnt + 1 "
								" where deal_no = %lu"
								, pRecv->dwDealID);

				if( ret != 0 )
					 infLOG(ERROR, "com9005 cnt+1 : query [ %s ] \nerror msg : [ %s ]\n",main_db.getQueryString(),main_db.getErrorMessage());

			}


			main_db.disconnect_db();

			headers.nDataCnt = nFileCount;

			if( headers.nDataCnt< 0 ) //파일 리스트 갯수를 얻는다.
			{
				pSendData = new char[sizeof(ERR_HEADER)];
				memset(pSendData,0x00,sizeof(ERR_HEADER));
				errheader.header.nCmd = RS_ERR;
				errheader.header.nErrorCode = -RS_FILE_REQUEST_LIST;
				strcpy(errheader.errmsg,"목록을 가져올수 없습니다. (5)");
				memcpy(pSendData, &errheader, sizeof(ERR_HEADER));
				delete[] pResult;
				return ERR_HEADER_SIZE;
			}
			else if(headers.nDataCnt == 0)
			{
				headers.nCmd = RS_EOL;

				headers.nDataCnt =0;
				headers.nDataSize = 0;
				headers.nErrorCode = 0;

				pSendData = new char[sizeof(HEADER) + headers.nDataCnt * headers.nDataSize];

				memset(pSendData,0x00,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize);
				memcpy(pSendData,&headers,sizeof(HEADER)); //head
				delete[] pResult;
				return 1;
			}
			else if(headers.nDataCnt >= 2000)
			{
				pSendData = new char[sizeof(ERR_HEADER)];
				memset(pSendData,0x00,sizeof(ERR_HEADER));
				errheader.header.nCmd = RS_ERR;
				errheader.header.nErrorCode = -RS_FILE_REQUEST_LIST;
				strcpy(errheader.errmsg,"한폴더 안에 2000개 이상의 목록을 작성 할수 없습니다..");
				memcpy(pSendData, &errheader, sizeof(ERR_HEADER));
				delete[] pResult;
				infLOG(ERROR, " ]FileRequestList: 목록 초과 작성 (%d)", headers.nDataCnt);
				return ERR_HEADER_SIZE;

			}
			headers.nCmd = RS_FILE_REQUEST_LIST;


			headers.nDataSize = sizeof(FILEINFO);
			headers.nErrorCode = 0;

			pSendData = new char[sizeof(HEADER) + headers.nDataCnt * headers.nDataSize];

			memset(pSendData,0x00,sizeof(HEADER) + headers.nDataCnt * headers.nDataSize);
			memcpy(pSendData,&headers,sizeof(HEADER)); //head
			memcpy(pSendData+sizeof(HEADER),pResult,headers.nDataCnt * headers.nDataSize);
			delete[] pResult;

		}

	}

	// 데이터가 없으면...
	// return -RS_FILE_DATA_TRANSFER;
	infLOG(ALWAY,"end ==== 5160\n");
	return (HEADER_SIZE + (headers.nDataCnt * headers.nDataSize));
}


