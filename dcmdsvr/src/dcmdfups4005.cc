/******************************************************************************
 *   서브시스템 : CMD서버
 *   프로그램명 : fups4005.cc
 *         기능 : 컨텐츠 등록시 뮤레카 및 차단 해쉬 검색
 *         설명 :
		  수정사항 - 유료컨텐츠
		  수정사항 - foldername 에 ReplaceSingleQuotation 적용
			  비고 - 적용시에 주석을 풀어주어야 합니다.
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#include <mysql.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "comcomm.h"
#include "commydb.h"
#include "apdefine.h"
//#include "/home/ezwon/zangsi_with_dcmd/fupsvr/inc/fups4005.h"
#include "../../fupsvr/inc/fups4005.h"
#include "dcmdfups4005.h"
#include "dcmdcomlib.h" //업로드 fupcomlib
//#define  _DEBUG_

#include "mysql_pool.h"
#include "comhead.h"

extern CMysqlPool * m_g_clMysqlPool;
extern CMysqlPool * m_g_clMysqlPoolCopyRight;	//유료컨테츠 DB

//20190124 add cfups4005_1m_hash

int InsertFileData(MYSQL *con, CMysqlCon MysqlCon, MYSQL *cpr_con, CMysqlCon MysqlCprCon
					, char* pDefaultHash, char* pFileName, double dwFileSize, char* pFileType
					, CFUPS4005 pFUPS4005
					, bool bIsZip, char* pMurekaStatus, char* pRegDate, char* pRegTime
//					, LPMUREKA_VINFO pMurekaVInfo, int nMurekaCnt, int nMurekaIndex);
					, LPMUREKA_VINFO pMurekaVInfo, int nMurekaCnt, int nMurekaIndex , CFUPS4005_1M_HASH pFUPSHASH);



int UpdateCopyRight(MYSQL *con, CMysqlCon MysqlCon, char* sect_code, char* sect_sub, char* reg_date, char* reg_time, char* szDefaultHash, char* szFileName, double dwFileSize, MUREKA_VINFO MurekaVInfo);

int InsertMurekaVideo(MYSQL *con, CMysqlCon MysqlCon
					, CFUPS4005 pFUPS4005
					, LPMUREKA_VINFO pMurekaVInfo, int nMurekaCnt, int nMurekaIndex
					);
void MurekaLog2(int nMurekaCnt, LPMUREKA_VINFO pMurekaVInfo);

//20190123 1mbhash
int Insert1mbHash(MYSQL *con, CMysqlCon MysqlCon,CFUPS4005 pfups4005, char* pRegDate, char* pRegTime ,CFUPS4005_1M_HASH pfups4005hash);
	
long dcmdfups4005(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData)//CFUPS4005 pfups4005)
{
	
	
	
	CFUPS4005 pfups4005;
	memcpy(&pfups4005, (LPCFUPS4005)pRecvData ,sizeof(CFUPS4005));

	
	//20190124 1mb hash
	long recv_body_size = 0;
	recv_body_size = sizeof(CFUPS4005);
	CFUPS4005_1M_HASH Fups1MHash;
	memset( &Fups1MHash, 0x00, sizeof(CFUPS4005_1M_HASH));	
	if( pHeader->nCmd == 40052 ) //hash 
	{
			infLOG(ALWAY,"FUPS40052 | cmd 40052 \n");
			memcpy( &Fups1MHash ,  pRecvData +recv_body_size ,sizeof(CFUPS4005_1M_HASH));
			recv_body_size += sizeof(CFUPS4005_1M_HASH);
			
	}

	infLOG(ALWAY,"FUPS40052 | check hash [ %s ] [ %s ] \n",Fups1MHash.hash_1m,Fups1MHash.hash_1m_mureka);

	
	
	
	int nMurekaCnt = pfups4005.mureka_cnt;
	LPMUREKA_VINFO pMurekaVInfo = NULL;

	if(nMurekaCnt > 0)
	{
		char* pTempBuffer = new char[sizeof(MUREKA_VINFO)*nMurekaCnt];
		memcpy( pTempBuffer , pRecvData + recv_body_size , sizeof(MUREKA_VINFO)*nMurekaCnt);
		pMurekaVInfo = (LPMUREKA_VINFO)pTempBuffer;
	}

	infLOG(ALWAY,"FUPS4005 | 시작[%lu] [%s] cnt [ %d ]\n", pfups4005.id, pfups4005.file_name,nMurekaCnt);
	MurekaLog2(nMurekaCnt,pMurekaVInfo);

	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);

	//--------------------------------------------------------------------------
	// Wedisk DB
	//--------------------------------------------------------------------------
	bool bDefaultHash = false;
	bool bAudioHash = false;
	bool bVideoHash = false;
	bool bCloseDB = false;
	bool bMasterCloseDB = false;

	MYSQL *con=NULL;
	MYSQL *conMaster=NULL;
	MYSQL_RES   *res;
	MYSQL_ROW    row;
	//--------------------------------------------------------------------------


	//--------------------------------------------------------------------------
	// 유료 컨텐츠 DB
	//--------------------------------------------------------------------------
	bool bCprCloseDB = false;

	MYSQL *cpr_con=NULL;
	MYSQL_RES   *cpr_res;
	MYSQL_ROW    cpr_row;
	//--------------------------------------------------------------------------

	char szSysErrMsg[255];
	memset(szSysErrMsg,0x00,sizeof(szSysErrMsg));

	char szQuery[10000], szQuery1[10000];
	memset (szQuery , 0x00, sizeof(szQuery ));
	memset (szQuery1 , 0x00, sizeof(szQuery1 ));

	char ErrMsg[256];    // error message
	memset(ErrMsg   , 0x00, sizeof(ErrMsg   )); // 오류메시지
	int  ErrNum;         // error no

    char reg_date[8+1];
    memset(reg_date,0x00,sizeof(reg_date));

    char reg_time[6+1];
    memset(reg_time,0x00,sizeof(reg_time));

	char szFilterYN[4+1];
	memset(szFilterYN,0x00,sizeof(szFilterYN));

	char szMurekaStatus[4+1];
	memset(szMurekaStatus,0x00,sizeof(szMurekaStatus));

	char szFileType[12+1];
	memset(szFileType,0x00,sizeof(szFileType));
	int nRetry = 0;
	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	CMysqlCon MysqlCon(m_g_clMysqlPool,m_g_clMysqlPool->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );

	con = MysqlCon.GetMysqlCon();

	if (con == NULL )
	{
		ErrNum = -400592;
		sprintf(ErrMsg, "DB에 접속하지 못 하였습니다.\n");

		infLOG(ERROR, "FUPS4005 | GetMysqlCon is null \n");

		int nRetry = 0;
		while (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);

			infLOG(ERROR, "FUPS4005 | GetMysqlCon is null [%d](%s)\n",ErrNum, ErrMsg);
			//infLOG(ERROR, "FUPS4005 | Mysql Error | Errno : %d | ErrMsg : %s \n", mysql_errno(con),mysql_error(con));
		}

		if( nRetry >= 5)
		{
			req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);

			infLOG(ERROR, "FUPS4005 | Cannot DB Connect \n");

			return HEADER_SIZE;
		}
		infLOG(ERROR, "FUPS4005 | Connect DB direct\n");

		bCloseDB = true;
	}


	//

	//--------------------------------------------------------------------------
	// 등록일자 얻기
	//--------------------------------------------------------------------------

	memset(szQuery,  0x00, sizeof(szQuery));
	memset(szQuery1, 0x00, sizeof(szQuery1));
	strcpyA(szQuery, "SELECT date_format(now(),'%Y%m%d')");
	strcat(szQuery, "     , date_format(now(),'%H%i%s')");

	if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
	{
		ErrNum = -400501;
		sprintf(ErrMsg, "테이베이스 오류입니다.\n");
		goto fups4005_err;
	}

	if (!(res = mysql_store_result(con)))
	{
		ErrNum = -400502;
		sprintf(ErrMsg, "테이베이스 오류입니다.\n");
		goto fups4005_err;
	}
	if (mysql_num_rows(res)==0)
	{
		ErrNum = -400502;
		sprintf(ErrMsg, "검색된 자료가 없습니다.\n");
		mysql_free_result(res);
		goto fups4005_err;
	}
	row = mysql_fetch_row(res);

	memset(reg_date     , 0x00, sizeof(reg_date     ));
	memset(reg_time     , 0x00, sizeof(reg_time     ));

	strcpyA(reg_date     , getstr(row, 0));
	strcpyA(reg_time     , getstr(row, 1));

	mysql_free_result(res);

	///
	ReplaceSingleQuotation(pfups4005.file_name, '\'',pfups4005.file_name);
	ReplaceSingleQuotation(pfups4005.folder_name, '\'',pfups4005.folder_name); // 인탁 20090306일수

	memset (szQuery , 0x00, sizeof(szQuery ));

	memset (pfups4005.copyright_yn , 0x00, sizeof(pfups4005.copyright_yn ));
	sprintf(pfups4005.copyright_yn, "%s","N");

	if(strcmp(pfups4005.cont_gu, "WE") == 0)
	{
		memset(szQuery , 0x00, sizeof(szQuery ));

		//2009/06/15 - HCS : 뮤레카 정보중 차단된 컨텐츠가 있는지 검사.
		/*
		클라이언트로 부터 넘겨 받은 뮤레카 정보중 차단된 컨텐츠가 있다면
		zangsi DB의 저작권 데이터를 수정한다.
		뮤레카 정보가 하나도 없다면 기존방식대로 처리.
		*/
		bool bIsZip = false;

		if(nMurekaCnt > 1)
			bIsZip = true;

		if( nMurekaCnt  > 0 )
		{
			//for(int i=0; i < nMurekaCnt; i++)
			// 파일 정보를 하나씩 보내고 있음.

			//for(int i=0; i < 1; i++)
			//{
				int i=0;
				char szHashValue[32+1];
				memset(szHashValue, 0x00, sizeof(szHashValue));
				char szSize[128];
				memset(szSize, 0x00, sizeof(szSize));
				double dwSize = 0;

				//--------------------------------------------------------------------------
				//동영상 필터링
				//--------------------------------------------------------------------------
				if(pMurekaVInfo[i].nFileGubun == 2 || (pMurekaVInfo[i].nFileGubun == 8))
				{
					//20120305 - kbs

					//strcpy(szFileType, "동영상");
					if(pMurekaVInfo[i].nResultCode == 0)
					{
						memset(szMurekaStatus, 0x00, sizeof(szMurekaStatus));
						strcpyA(szMurekaStatus, pMurekaVInfo[i].video_status); // 상태 (00:무료, 01:유료, 02:차단, 03:Non-License, 04:Unknown)
					}
					else
					{
						memset(szMurekaStatus, 0x00, sizeof(szMurekaStatus));
						sprintf(szMurekaStatus, "%d", pMurekaVInfo[i].nResultCode);
					}

					memset(pfups4005.video_hash, 0x00, sizeof(pfups4005.video_hash));
					strcpyA(pfups4005.video_hash, pMurekaVInfo[i].mureka_hash);

					if(nMurekaCnt > 1)
					{
						memcpy(szHashValue, pfups4005.video_hash, 32);
						memcpy(szSize, pfups4005.video_hash + 33, strlen(pfups4005.video_hash));
						dwSize = atof(szSize);
					}
					else
					{
						sprintf(szHashValue, "%s", pfups4005.default_hash);
						dwSize = pfups4005.file_size;
					}

					if(strcmp(pMurekaVInfo[i].video_status, "02") == 0 )
					{// 저작물이 하나라도 있다면 관리테이블을 업데이트/인서트 한다.

						//delete
						if( strcmp(pfups4005.sect_code ,"01") != 0  ) //||  strcmp(pMurekaVInfo[i].video_right_name,"씨제이이앤엠") != 0 )
						{
							sprintf(szMurekaStatus, "%s", pMurekaVInfo[i].video_status);
							infLOG(ALWAY,"dcmdfups4005> 차단 항목 있음.\n");
							infLOG(ALWAY,"dcmdfups4005> 저작권자 : [%s]\n", pMurekaVInfo[i].video_right_name);
							infLOG(ALWAY,"dcmdfups4005> 제목 : [%s]\n", pMurekaVInfo[i].video_title);
							infLOG(ALWAY,"dcmdfups4005> 뮤레카해시 : [%s]\n", pMurekaVInfo[i].mureka_hash);


							int nRet = UpdateCopyRight(con, MysqlCon, pfups4005.sect_code, pfups4005.sect_sub, reg_date, reg_time, pfups4005.default_hash, pfups4005.file_name, pfups4005.file_size, pMurekaVInfo[i]);

							if(nRet < 0)
							{
								infLOG(ERROR, "UpdateCopyRight 실패\n");
							}
						}

					}
				}
				else if(pMurekaVInfo[i].nFileGubun == 1)
				{
					//strcpy(szFileType, "음악");
					if(pMurekaVInfo[i].nResultCode == 0)
					{
						memset(szMurekaStatus, 0x00, sizeof(szMurekaStatus));
						strcpyA(szMurekaStatus, pMurekaVInfo[i].music_status);
					}
					else
					{
						memset(szMurekaStatus, 0x00, sizeof(szMurekaStatus));
						sprintf(szMurekaStatus, "%d", pMurekaVInfo[i].nResultCode);
					}

					memset(pfups4005.video_hash, 0x00, sizeof(pfups4005.video_hash));
					strcpyA(pfups4005.video_hash, pMurekaVInfo[i].mureka_hash);

					if(nMurekaCnt > 1)
					{
						strcpy(szHashValue, pfups4005.video_hash);
						memcpy(szSize, pfups4005.video_hash + 33, strlen(pfups4005.video_hash));
						dwSize = atof(szSize);
					}
					else
					{
						sprintf(szHashValue, "%s", pfups4005.default_hash);
						dwSize = pfups4005.file_size;
					}

					if(strcmp(pMurekaVInfo[i].music_status, "02") == 0)
					{// 저작물이 하나라도 있다면 관리테이블을 업데이트/인서트 한다.
						strcpyA(szMurekaStatus, pMurekaVInfo[i].video_status);
						infLOG(ALWAY,"dcmdfups4005> 차단 항목 있음.\n");
						infLOG(ALWAY,"dcmdfups4005> 저작권자 : [%s]\n", pMurekaVInfo[i].music_injeob_com);
						infLOG(ALWAY,"dcmdfups4005> 가수 : [%s]\n", pMurekaVInfo[i].music_artist);
						infLOG(ALWAY,"dcmdfups4005> 곡명 : [%s]\n", pMurekaVInfo[i].music_title);
						infLOG(ALWAY,"dcmdfups4005> 뮤레카 해시 : [%s]\n", pMurekaVInfo[i].mureka_hash);


						int nRet = UpdateCopyRight(con, MysqlCon, pfups4005.sect_code, pfups4005.sect_sub, reg_date, reg_time, pfups4005.default_hash, pfups4005.file_name, pfups4005.file_size, pMurekaVInfo[i]);

						if(nRet < 0)
						{
							infLOG(ERROR, "UpdateCopyRight 실패\n");
						}
					}
				}
				//--------------------------------------------------------------------------
				//파일정보 등록
				//--------------------------------------------------------------------------
				if(bIsZip)
				{
					if(i == 0)
					{
						int nResult = InsertFileData(con, MysqlCon, con, MysqlCon, "", "", 0, szFileType, pfups4005, false, szMurekaStatus, reg_date, reg_time,pMurekaVInfo,nMurekaCnt,i, Fups1MHash);

						if(nResult == -1)
							goto fups4005_err;
						else if(nResult == 1)
							strcpy(pfups4005.copyright_yn, "Y");
					}
					else if(pMurekaVInfo[i].nFileGubun == 2 || pMurekaVInfo[i].nFileGubun == 1 || pMurekaVInfo[i].nFileGubun == 4 || (pMurekaVInfo[i].nFileGubun == 8))
					{
							   
						int nResult = InsertFileData(con, MysqlCon, con, MysqlCon, szHashValue, pMurekaVInfo[i].filename, dwSize, szFileType, pfups4005, bIsZip, szMurekaStatus, reg_date, reg_time,pMurekaVInfo,nMurekaCnt,i,  Fups1MHash);
							if(nResult == -1)
								goto fups4005_err;
							else if(nResult == 1)
								strcpy(pfups4005.copyright_yn, "Y");
					}
				}
				else
				{
					int nResult = InsertFileData(con, MysqlCon, con, MysqlCon, szHashValue, pMurekaVInfo[i].filename, dwSize, szFileType, pfups4005, bIsZip, szMurekaStatus, reg_date, reg_time,pMurekaVInfo,nMurekaCnt,i,  Fups1MHash);
						if(nResult == -1)
							goto fups4005_err;
						else if(nResult == 1)
							strcpy(pfups4005.copyright_yn, "Y");
				}
				//--------------------------------------------------------------------------
			//}
		}


		//--------------------------------------------------------------------------
		//뮤레카 필터링 거치지 않은 파일 등록
		//--------------------------------------------------------------------------
		if(nMurekaCnt <= 0)
		{
			int nResult = InsertFileData(con, MysqlCon, con, MysqlCon, "", "", 0, szFileType, pfups4005, false, szMurekaStatus, reg_date, reg_time,pMurekaVInfo,nMurekaCnt,0,Fups1MHash);

			if(nResult == -1)
				goto fups4005_err;
			else if(nResult == 1)
				strcpy(pfups4005.copyright_yn, "Y");
		}

		//--------------------------------------------------------------------------

	}
	else
	{
		infLOG(ALWAY,"fups4005 등록 실패, 잘못된 cont_gu 입력 : %s\n", pfups4005.cont_gu);
		goto fups4005_err;
	}

	if( bCloseDB )
		db_disconnect(con);

	if( bCprCloseDB )
		db_disconnect(cpr_con);

	if( strcmp(pfups4005.copyright_yn,"Y") == 0 )
	{
		req_header.nCmd = 1 ;
	}
	else
	{
		req_header.nCmd = 0 ;
	}


	pSendData = new char[HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

	if(pMurekaVInfo)
		delete[] pMurekaVInfo;

	infLOG(ALWAY,"fups4005 완료\n");

  	return HEADER_SIZE;

//------------------------------------------------------------------------------
fups4005_err:

	if( bCloseDB )
		db_disconnect(con);

	if( bCprCloseDB )
		db_disconnect(cpr_con);

	req_header.nCmd = -1 ;
	pSendData = new char [HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

	if(pMurekaVInfo)
		delete[] pMurekaVInfo;

 	return HEADER_SIZE;
}

/*****************************************************************************
* 파일 데이터 등록
* (I) 1. DB connet 정보(zangsi, zangsi_cpr)
*     2. 파일 정보(해시, 파일명, 사이즈, 파일 타입[동영상,음악])
*     3. 컨텐츠 정보
*     4. 기타 정보(압축여부, 뮤레카 결과 코드, 등록일시)
* (R) 1. 0		: 일반 컨텐츠
*	  2. 음수	: 오류
*	  3. 1	 	: 차단 컨텐츠
*****************************************************************************/
int InsertFileData(MYSQL *con, CMysqlCon MysqlCon, MYSQL *cpr_con, CMysqlCon MysqlCprCon
					, char* pDefaultHash, char* pFileName, double dwFileSize, char* pFileType
					, CFUPS4005 pFUPS4005
					, bool bIsZip, char* pMurekaStatus, char* pRegDate, char* pRegTime
					, LPMUREKA_VINFO pMurekaVInfo, int nMurekaCnt, int nMurekaIndex,CFUPS4005_1M_HASH pFUPSHASH)
{

	int nResult = 0;
	MYSQL_RES   *cpr_res;
	MYSQL_ROW    cpr_row;

	MYSQL_RES   *res;
	MYSQL_ROW    row;

	char szSysErrMsg[255];
	memset(szSysErrMsg,0x00,sizeof(szSysErrMsg));

	char szQuery[10000], szQuery1[10000];
	memset (szQuery , 0x00, sizeof(szQuery ));
	memset (szQuery1 , 0x00, sizeof(szQuery1 ));

	char szExtHash[50+1];
	memset(szExtHash, 0x00, sizeof(szExtHash));

    sprintf(szExtHash, "%s", pFUPS4005.default_hash);

	if(bIsZip)
	{
		memset(pFUPS4005.default_hash, 0x00, sizeof(pFUPS4005.default_hash));
		sprintf(pFUPS4005.default_hash, "%s", pDefaultHash);

		pFUPS4005.file_size = dwFileSize;

		memset(pFUPS4005.file_name, 0x00, sizeof(pFUPS4005.file_name));
		sprintf(pFUPS4005.file_name, "%s", pFileName);
	}


	#ifdef _DEBUG_
	printf("InsertFileData> 해시   (%s).\n", pFUPS4005.default_hash);
	printf("InsertFileData> 파일명 (%s).\n", pFUPS4005.file_name);
	printf("InsertFileData> 사이즈 (%.0f).\n", pFUPS4005.file_size);
	#endif


	//--------------------------------------------------------------------------
	// 차단 컨텐츠 조회
	//--------------------------------------------------------------------------
	unsigned long ulID = 0;

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select a.id as id "
					 " from zangsi.T_CONTENTS_COPYRIGHT_HASH_INFO a ,zangsi.T_CONTENTS_COPYRIGHT_HASH_FILE b "
					 " where a.proc_stat = 'C' and a.id = b.info_id  and b.file_size  = %.0f and b.default_hash = '%s' and b.filter_yn = 'Y' "
					 , pFUPS4005.file_size, szExtHash);
	#ifdef __DEBUG
	printf("InsertFileData> query = [%s]\n\n", szQuery);
	#endif
	if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		infLOG(ERROR, "fups4005->InsertFileData : MysqlQuery 오류.\n");
		infLOG(ERROR, "fups4005->InsertFileData : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
	}
	else
	{
		//차단 컨텐츠이면
		if (!(cpr_res = mysql_store_result(cpr_con)))
		{
			infLOG(ERROR, "fups4005->InsertFileData : mysql_store_result 오류.\n");
			infLOG(ERROR, "fups4005->InsertFileData : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
		}
		else
		{
			if (mysql_num_rows(cpr_res) > 0 )
			{
				cpr_row = mysql_fetch_row(cpr_res);
				ulID = (unsigned long)getnum(cpr_row,0);

				memset(pFUPS4005.copyright_yn, 0x00, sizeof(pFUPS4005.copyright_yn));
				strcpy(pFUPS4005.copyright_yn, "Y");
				nResult = 1;
			}
			mysql_free_result(cpr_res);
		}
	}
	//--------------------------------------------------------------------------



	if(strcmp(pFUPS4005.copyright_yn, "Y") == 0 &&  strcmp(pFUPS4005.folder_yn, "N") == 0)
	{

		memset(szQuery , 0x00, sizeof(szQuery ));
		sprintf(szQuery, "REPLACE INTO zangsi.T_CONTENTS_TEMPLIST           "
		                 "       (id            ,seq_no        ,folder_yn  "
		                 "       ,file_name     ,file_size     ,file_type  "
		                 "       ,reg_user      ,reg_date      ,reg_time		,default_hash , audio_hash , video_hash	,copyright_yn )  "
		                 " VALUES (%ld			,%d			   ,'N'              "
		                 "			,'%s'			,%13.0f		   ,'2'			"
		                 "			,'%s'			,'%s'			   ,'%s'				,'%s' ,'%s' ,'%s' , '%s'	)"
		                 ,pFUPS4005.id
		                 ,pFUPS4005.seq_no
		                 ,pFUPS4005.file_name
		                 ,pFUPS4005.file_size
		                 ,pFUPS4005.user_id
		                 ,pRegDate
		                 ,pRegTime
		                 ,pFUPS4005.default_hash
		                 ,pFUPS4005.audio_hash
		                 ,pFUPS4005.video_hash
		                 ,pFUPS4005.copyright_yn );

		if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
		{
			infLOG(ERROR, "fups4005->InsertFileData : MysqlQuery 오류.\n");
			infLOG(ERROR, "fups4005->InsertFileData : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
			//return -1;
	  	}

		infLOG(ALWAY,"fups4005->InsertFileData [ %s ] \n",szQuery);

		//--------------------------------------------------------------------------
		//중복 데이터 조회
		//--------------------------------------------------------------------------
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "select seq_no from zangsi.T_CONTENTS_TEMPLIST_SUB "
						 " where id = %lu and file_size = %15.0f and default_hash = '%s' limit 1"
		                 ,pFUPS4005.id
		                 ,pFUPS4005.file_size
		                 ,pFUPS4005.default_hash
		                 );

	  	bool bFirst = true;
	  	unsigned long ulSeqNO = 0;

		if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
		{
			infLOG(ERROR, "fups4005->InsertFileData : MysqlQuery 오류.\n");
			infLOG(ERROR, "fups4005->InsertFileData : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
			return -1;
	  	}
		if (!(res = mysql_store_result(con)))
		{
			infLOG(ERROR, "fups4005->InsertFileData : mysql_store_result 오류.\n");
			infLOG(ERROR, "fups4005->InsertFileData : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
			return -1;
		}
		if(row = mysql_fetch_row(res))
		{
			if (mysql_num_rows(res) > 0 )
			{
				bFirst = false;
				ulSeqNO = (unsigned long)getnum(row,0);
			}
		}
		mysql_free_result(res);
		//--------------------------------------------------------------------------


		//--------------------------------------------------------------------------
		//중복 데이터가 없다면 파일정보 등록
		//--------------------------------------------------------------------------
		if( bFirst )
		{


			memset(szQuery , 0x00, sizeof(szQuery ));
			sprintf(szQuery, "insert into zangsi.T_CONTENTS_TEMPLIST_SUB           "
							 " ( id ,depth,file_type , folder_path , file_name,file_size,reg_user,reg_date,reg_time,default_hash , audio_hash , video_hash,copyright_yn	 ) "
			                 " values "
			                 " ( %d , %d   , '%s'       , '%s'     , '%s'      , %15.0f , '%s'     , '%s'   ,'%s'    ,'%s'        ,'%s'         ,'%s'       ,'%s'  )   "
			                 ,pFUPS4005.id
			                 ,pFUPS4005.depth
			                 ,pMurekaStatus
			                 ,pFUPS4005.folder_name
			                 ,pFUPS4005.file_name
			                 ,pFUPS4005.file_size
			                 ,pFUPS4005.user_id
			                 ,pRegDate
			                 ,pRegTime
			                 ,pFUPS4005.default_hash
			                 ,pFUPS4005.audio_hash
			                 ,pFUPS4005.video_hash
			                 ,pFUPS4005.copyright_yn
			                 );
	 		#ifdef _DEBUG_
	 		printf("InsertFileData> szQuery=[%s].\n", szQuery);
	 		#endif
			if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
			{
				infLOG(ERROR, "fups4005->InsertFileData : MysqlQuery 오류.\n");
				infLOG(ERROR, "fups4005->InsertFileData : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
				//return -1;
		  	}
		  	else
		  	{
		  		//if( bIsZip == false && nMurekaCnt == 1)
				//20190123 1mb hash 
				Insert1mbHash(con,  MysqlCon	, pFUPS4005,pRegDate,pRegTime,pFUPSHASH);

		  		if(  nMurekaCnt > 0  )
		  		{
				  	sprintf( szQuery, "%s", "SELECT last_insert_id() as filelist_seq" );

					if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
					{
						infLOG(ERROR, "query :[ %s ]\n\n",szQuery);
					}
					else
					{
						if (!(res = mysql_store_result(con)))
						{
							infLOG(ERROR, "query :[ %s ]\n\n",szQuery);
						}
						else
						{
							unsigned long filelist_seq  = 0;
							if(row = mysql_fetch_row(res))
							{
								filelist_seq  = (unsigned long) getnum(row,0 );		//사이버머니보유액
							}
							mysql_free_result(res);

							if( filelist_seq > 0 )
							{
								CFUPS4005 fups4005 = pFUPS4005;
								fups4005.seq_no = filelist_seq;

								InsertMurekaVideo(con,  MysqlCprCon	, fups4005, pMurekaVInfo,nMurekaCnt ,nMurekaIndex);
							}
						}
					}
				}
			}



		}
		//--------------------------------------------------------------------------
	}

	if(strcmp(pFUPS4005.copyright_yn, "Y") == 0 &&  strcmp(pFUPS4005.folder_yn, "Y") == 0)
	{
		if( pFUPS4005.depth == 0 )
		{
			sprintf(szQuery, "update zangsi.T_CONTENTS_TEMPLIST           "
			                 " set file_size = %13.0f, default_hash = '%s' , audio_hash = '%s' , video_hash = '%s' , copyright_yn = '%s' "
			                 " WHERE id  =  %ld and reg_user = '%s' and file_name = '%s'  "
			                 ,pFUPS4005.file_size
			                 ,pFUPS4005.default_hash
			                 ,pFUPS4005.audio_hash
			                 ,pFUPS4005.video_hash
			                 ,pFUPS4005.copyright_yn
			                 ,pFUPS4005.id
			                 ,pFUPS4005.user_id
			                 ,pFUPS4005.file_name
			                );

			if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
			{
				infLOG(ERROR, "fups4005->InsertFileData : MysqlQuery 오류.\n");
				infLOG(ERROR, "fups4005->InsertFileData : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
				return -1;
		  	}
		  	infLOG(ALWAY,"fups4005->InsertFileData [ %s ] \n",szQuery);

		}


		//--------------------------------------------------------------------------
		//중복 데이터 조회
		//--------------------------------------------------------------------------
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "select seq_no from zangsi.T_CONTENTS_TEMPLIST_SUB "
						 " where id = %lu and file_size = %15.0f and default_hash = '%s' and depth = %d limit 1"
		                 ,pFUPS4005.id
		                 ,pFUPS4005.file_size
		                 ,pFUPS4005.default_hash
		                 ,pFUPS4005.depth
		                 );

	  	bool bFirst = true;
	  	unsigned long ulSeqNO = 0;

		if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
		{
			infLOG(ERROR, "fups4005->InsertFileData : MysqlQuery 오류.\n");
			infLOG(ERROR, "fups4005->InsertFileData : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
			return -1;
	  	}
		if (!(res = mysql_store_result(con)))
		{
			infLOG(ERROR, "fups4005->InsertFileData : mysql_store_result 오류.\n");
			infLOG(ERROR, "fups4005->InsertFileData : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
			return -1;
		}
		if(row = mysql_fetch_row(res))
		{
			if (mysql_num_rows(res) > 0 )
			{
				bFirst = false;
				ulSeqNO = (unsigned long)getnum(row,0);
			}
		}
		mysql_free_result(res);
		//--------------------------------------------------------------------------


		//--------------------------------------------------------------------------
		//중복 데이터가 없다면 파일정보 등록
		//--------------------------------------------------------------------------
		if( bFirst )
		{

				memset(szQuery , 0x00, sizeof(szQuery ));
				sprintf(szQuery, "insert into zangsi.T_CONTENTS_TEMPLIST_SUB           "
								 " ( id ,depth,file_type , folder_path , file_name,file_size,reg_user,reg_date,reg_time,default_hash , audio_hash , video_hash,copyright_yn	 ) "
				                 " values "
				                 " ( %d , %d   , '%s'       , '%s'     , '%s'      , %15.0f , '%s'     , '%s'   ,'%s'    ,'%s'        ,'%s'         ,'%s'       ,'%s'  )   "
				                 ,pFUPS4005.id
				                 ,pFUPS4005.depth
				                 ,pMurekaStatus
				                 ,pFUPS4005.folder_name
				                 ,pFUPS4005.file_name
				                 ,pFUPS4005.file_size
				                 ,pFUPS4005.user_id
				                 ,pRegDate
				                 ,pRegTime
				                 ,pFUPS4005.default_hash
				                 ,pFUPS4005.audio_hash
				                 ,pFUPS4005.video_hash
				                 ,pFUPS4005.copyright_yn
				                 );
		 		#ifdef _DEBUG_
		 		printf("InsertFileData> szQuery=[%s].\n", szQuery);
		 		#endif
				if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
				{
					infLOG(ERROR, "fups4005->InsertFileData : MysqlQuery 오류.\n");
					infLOG(ERROR, "fups4005->InsertFileData : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
					//return -1;
			  	}
			  	else
			  	{
			  		//if( bIsZip == false && nMurekaCnt == 1)
					//20190123 1mb hash 
					Insert1mbHash(con,  MysqlCon	, pFUPS4005,pRegDate,pRegTime , pFUPSHASH);
			  		if(  nMurekaCnt > 0  )
			  		{

					  	sprintf( szQuery, "%s", "SELECT last_insert_id() as filelist_seq" );

						if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
						{
							infLOG(ERROR, "query :[ %s ]\n\n",szQuery);
						}
						else
						{
							if (!(res = mysql_store_result(con)))
							{
								infLOG(ERROR, "query :[ %s ]\n\n",szQuery);
							}
							else
							{
								unsigned long filelist_seq  = 0;
								if(row = mysql_fetch_row(res))
								{
									filelist_seq  = (unsigned long) getnum(row,0 );		//사이버머니보유액
								}
								mysql_free_result(res);

								if( filelist_seq > 0 )
								{
									CFUPS4005 fups4005 = pFUPS4005;
									fups4005.seq_no = filelist_seq;

									InsertMurekaVideo(con,  MysqlCprCon	, fups4005, pMurekaVInfo,nMurekaCnt,nMurekaIndex);
								}
							}
						}
					}
				}

		}
		//--------------------------------------------------------------------------
	}

	return nResult;
}


int UpdateCopyRight(MYSQL *con, CMysqlCon MysqlCon, char* sect_code, char* sect_sub, char* reg_date, char* reg_time, char* szDefaultHash, char* szFileName, double dwFileSize, MUREKA_VINFO MurekaVInfo)
{
	MYSQL_RES   *res;
	MYSQL_ROW    row;

	int nReturn = 0;
	char ErrMsg[256];    // error message
	memset(ErrMsg   , 0x00, sizeof(ErrMsg   )); // 오류메시지

	char szSysErrMsg[255];
	memset(szSysErrMsg,0x00,sizeof(szSysErrMsg));

	char szQuery[10000];
	memset(szQuery, 0x00, sizeof(szQuery));


	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select a.id as id, a.proc_stat, b.filter_yn "
					 " from zangsi.T_CONTENTS_COPYRIGHT_HASH_INFO a ,zangsi.T_CONTENTS_COPYRIGHT_HASH_FILE b "
					 " where a.id = b.info_id and "
					 " b.file_size  = %15.0f and (b.default_hash = '%s')"
					 , dwFileSize, szDefaultHash);
	if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
	{
		strcpyA(ErrMsg, "데이터 조회 오류 입니다.");
		infLOG(ERROR, "%s \nquery :[ %s ]\n",ErrMsg,szQuery);
		infLOG(ERROR, "fups4005->UpdateCopyRight [%d](%s)\n",mysql_errno(con), mysql_error(con));
		#ifdef __DEBUG
		printf("dcmdfups4005>UpdateCopyRight %s \nquery : [%s]\n",ErrMsg,szQuery);
		printf("fups4005->UpdateCopyRight [%d](%s)\n",mysql_errno(con), mysql_error(con));
		#endif
		return -1;
	}
	if (!(res = mysql_store_result(con)))
	{
		strcpyA(ErrMsg, "데이터 생성 오류 입니다.\n");
		infLOG(ERROR, "%s",ErrMsg);
		infLOG(ERROR, "fups4005->UpdateCopyRight [%d](%s)\n",mysql_errno(con), mysql_error(con));
		#ifdef __DEBUG
		printf("dcmdfups4005>UpdateCopyRight %s \nquery : [%s]\n\n",ErrMsg,szQuery);
		printf("fups4005->UpdateCopyRight [%d](%s)\n",mysql_errno(con), mysql_error(con));
		#endif
		return -1;

	}
	if (mysql_num_rows(res) > 0)
	{
		row = mysql_fetch_row(res);
		if(strcmp(getstr(row,1), "C") != 0 || strcmp(getstr(row,2), "Y") != 0)
		{
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "update zangsi.T_CONTENTS_COPYRIGHT_HASH_INFO a, zangsi.T_CONTENTS_COPYRIGHT_HASH_FILE b"
							 " set a.proc_stat = 'C', b.filter_yn = 'Y' "
							 " where a.id = %ld and a.id = b.info_id "
						   , (unsigned long)getnum(row,0));

			if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
			{
				strcpyA(ErrMsg, "필터링 처리 업데이트 오류 입니다.");
				infLOG(ERROR, "%s \nquery :[ %s ]\n",ErrMsg,szQuery);
				infLOG(ERROR, "fups4005->UpdateCopyRight [%d](%s)\n",mysql_errno(con), mysql_error(con));
				#ifdef __DEBUG
				printf("dcmdfups4005>UpdateCopyRight %s \nquery : [%s]\n\n",ErrMsg,szQuery);
				printf("fups4005->UpdateCopyRight [%d](%s)\n",mysql_errno(con), mysql_error(con));
				#endif
				return -1;
			}
			#ifdef __DEBUG
			printf("dcmdfups4005>UpdateCopyRight %s의 처리필드는 %s \n", szFileName, getstr(row,1) );
			#endif
			nReturn = 0;
		}
	}
	else
	{//차단된 컨텐츠인데 zangsi DB에는 데이터 없음. zangsi DB에 데이터 인서트.
		nReturn = 1;
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi.T_CONTENTS_COPYRIGHT_HASH_INFO "
		                 " (copyright_user, title, sect_code, sect_sub, proc_stat, reg_user, reg_date, reg_time) "
		                 " values "
		                 " ('%s'          , '%s' , '%s'     , '%s'    , 'C'      , 'system', '%s'    , '%s') "
		                 , (MurekaVInfo.nFileGubun == 2 ? MurekaVInfo.video_right_name : MurekaVInfo.music_artist)
		                 , (MurekaVInfo.nFileGubun == 2 ? MurekaVInfo.video_title : MurekaVInfo.music_title)
		                 , sect_code
		                 , sect_sub
		                 , reg_date
		                 , reg_time);


		if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
		{
			strcpyA(ErrMsg, "뮤레카 차단 정보 인서트 실패.");

			infLOG(ERROR, "%s \nquery :[ %s ]\n",ErrMsg,szQuery);
			infLOG(ERROR, "fups4005->UpdateCopyRight [%d](%s)\n",mysql_errno(con), mysql_error(con));
			#ifdef __DEBUG
			printf("dcmdfups4005>UpdateCopyRight %s \nquery : [%s]\n\n",ErrMsg,szQuery);
			printf("fups4005->UpdateCopyRight [%d](%s)\n",mysql_errno(con), mysql_error(con));
			#endif
			mysql_free_result(res);
			return -1;
		}
		#ifdef __DEBUG
		printf("dcmdfups4005>UpdateCopyRight szQuery =[%s]\n", szQuery);
		#endif

		unsigned long dwId = 0;
		dwId = mysql_insert_id(con);

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi.T_CONTENTS_COPYRIGHT_HASH_FILE "
		                 " (info_id, filter_yn, mureka_yn, default_hash, video_hash, file_name, file_size, reg_user, reg_date, reg_time) "
		                 " values "
		                 " ('%lu'   , 'Y'     , '%s'      , '%s'       , '%s'      , '%s'     , '%15.0f', 'system', '%s'    , '%s') "
		                 , dwId
		                 , (strcmp(sect_code, "07") == 0 ? "N" : "Y")
		                 , szDefaultHash
		                 , MurekaVInfo.mureka_hash
		                 , szFileName
		                 , dwFileSize
		                 , reg_date
		                 , reg_time);

		if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
		{
			strcpyA(ErrMsg, "뮤레카 차단 정보 인서트 실패.");

			infLOG(ERROR, "%s \nquery :[ %s ]\n",ErrMsg,szQuery);
			infLOG(ERROR, "fups4005->UpdateCopyRight [%d](%s)\n",mysql_errno(con), mysql_error(con));
			#ifdef __DEBUG
			printf("dcmdfups4005>UpdateCopyRight %s \nquery : [%s]\n\n",ErrMsg,szQuery);
			#endif
			mysql_free_result(res);
			return -1;
		}
		#ifdef __DEBUG
		printf("dcmdfups4005>UpdateCopyRight szQuery =[%s]\n", szQuery);
		#endif
		mysql_free_result(res);
	}

	return nReturn;
}




/*****************************************************************************
* 동영상 필터링 정보 및 결과 등록
* (I) 1. DB connet 정보(nori)
*     2. 파일 정보
*     3. 뮤레카 정보
*	  4. 기타 정보(등록일시, 압축 여부)
* (R) 1. 0		: 정상
*	  2. 음수	: 오류
*****************************************************************************/
int InsertMurekaVideo(MYSQL *con, CMysqlCon MysqlCon
					, CFUPS4005 pFUPS4005
					, LPMUREKA_VINFO pMurekaVInfo, int nMurekaCnt, int nMurekaIndex
					 )
{
	//infLOG(ALWAY, "fups4005->InsertMurekaVideo IN | temp_id [ %lu ] seq [ %lu ] \n",pFUPS4005.id,pFUPS4005.seq_no);
	infLOG(ALWAY, "fups4005: nMurekaCnt 2 [ %d ] \n",nMurekaCnt);

	MYSQL_RES   *res;
	MYSQL_ROW    row;

	char szSysErrMsg[255];
	memset(szSysErrMsg,0x00,sizeof(szSysErrMsg));

	char szQuery[15000];
	memset(szQuery, 0x00, sizeof(szQuery));

	//--------------------------------------------------------------------------
	//기존 항목이 있는지 확인
	//--------------------------------------------------------------------------
	memset(szQuery, 0x00, sizeof(szQuery));

	//mureak 정보 추가
	//for( int i =0 ; i < nMurekaCnt ; i++ )
	int  i= nMurekaIndex;
	if( nMurekaCnt > 0 && i < nMurekaCnt )
	{
		if( pMurekaVInfo[i].filename == NULL || strlen(pMurekaVInfo[i].filename) <= 0)
			return -1;

		ReplaceSingleQuotation(pMurekaVInfo[i].filename, '\'',pMurekaVInfo[i].filename);

		if( pMurekaVInfo[i].nFileGubun == 4 ) //바이러스
		{
			sprintf(szQuery,"insert into zangsi.T_CONTENTS_TEMPLIST_MUREKA "
						" (  seq_no , id , file_gubun , result_code , video_status , video_id , video_title , video_jejak_year , video_right_name "
						" , video_right_content_id , video_grade , video_price , video_cha , video_osp_jibun , video_osp_etc "
						" , video_onair_date , video_right_id , virus_type , virus_name ,mureka_hash ,file_name ) "
						" values ( %lu , %lu , %d , %d , '%s', '%s', '%s', '%s', '%s' "
						" , '%s' , '%s'  , '%s'  , '%s'  , '%s'  , '%s' "
						" , '%s'  , '%s'  , '%s'  , '%s' ,'%s','%s' )"
						  , pFUPS4005.seq_no, pFUPS4005.id ,pMurekaVInfo[i].nFileGubun,pMurekaVInfo[i].nResultCode,pMurekaVInfo[i].video_status
						  ,pMurekaVInfo[i].video_id,pMurekaVInfo[i].video_title,pMurekaVInfo[i].video_jejak_year,pMurekaVInfo[i].video_right_name
						  ,pMurekaVInfo[i].video_right_content_id,pMurekaVInfo[i].video_grade,pMurekaVInfo[i].video_price,pMurekaVInfo[i].video_cha,pMurekaVInfo[i].video_osp_jibun,pMurekaVInfo[i].video_osp_etc
						  ,pMurekaVInfo[i].video_onair_date,pMurekaVInfo[i].video_right_id,pMurekaVInfo[i].video_right_name,pMurekaVInfo[i].video_title ,pMurekaVInfo[i].mureka_hash,pMurekaVInfo[i].filename);
		}
		else if( pMurekaVInfo[i].nFileGubun == 2 || (pMurekaVInfo[i].nFileGubun == 8)) //동영상
		{
			sprintf(szQuery,"insert into zangsi.T_CONTENTS_TEMPLIST_MUREKA "
							" ( seq_no , id , file_gubun , result_code , video_status , video_id , video_title , video_jejak_year , video_right_name "
							" , video_right_content_id , video_grade , video_price , video_cha , video_osp_jibun , video_osp_etc "
							" , video_onair_date , video_right_id ,mureka_hash ,file_name ) "
							" values ( %lu , %lu , %d , %d , '%s', '%s', '%s', '%s', '%s' "
							" , '%s' , '%s'  , '%s'  , '%s'  , '%s'  , '%s' "
							" , '%s'  , '%s'  ,'%s','%s' )"
							  , pFUPS4005.seq_no, pFUPS4005.id ,pMurekaVInfo[i].nFileGubun,pMurekaVInfo[i].nResultCode,pMurekaVInfo[i].video_status
							  ,pMurekaVInfo[i].video_id,pMurekaVInfo[i].video_title,pMurekaVInfo[i].video_jejak_year,pMurekaVInfo[i].video_right_name
							  ,pMurekaVInfo[i].video_right_content_id,pMurekaVInfo[i].video_grade,pMurekaVInfo[i].video_price,pMurekaVInfo[i].video_cha,pMurekaVInfo[i].video_osp_jibun,pMurekaVInfo[i].video_osp_etc
							  ,pMurekaVInfo[i].video_onair_date,pMurekaVInfo[i].video_right_id ,pMurekaVInfo[i].mureka_hash ,pMurekaVInfo[i].filename);
		}

		infLOG(ALWAY, "fups4005->InsertMurekaVideo QUERY 2 [ %s ]\n",szQuery);

		if( pMurekaVInfo[i].nFileGubun == 4 || pMurekaVInfo[i].nFileGubun == 2 || (pMurekaVInfo[i].nFileGubun == 8) )
		{
			if(MysqlCon.MysqlQuery(con, szSysErrMsg, szQuery))
			{
				infLOG(ERROR, "fups4005->InsertMurekaVideo : MysqlQuery error.\n");
				infLOG(ERROR, "fups4005->InsertMurekaVideo : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
			}
		}
	}


	//infLOG(ALWAY, "fups4005->InsertMurekaVideo OUT | temp_id [ %lu ] seq [ %lu ] \n",pFUPS4005.id,pFUPS4005.seq_no);
	return 0;
}

// ============================================================================================================
void MurekaLog2(int nMurekaCnt, LPMUREKA_VINFO pMurekaVInfo)
{
#ifdef _DEBUG_
	for(int i=0; i<nMurekaCnt; i++)
	{
		infLOG(ALWAY,"dcmdfups4005> nResultCode = [%d]\n",pMurekaVInfo[i].nResultCode);
		infLOG(ALWAY,"dcmdfups4005> nFileGubun = [%d]\n",pMurekaVInfo[i].nFileGubun);
		infLOG(ALWAY,"dcmdfups4005> filename = [%s]\n",pMurekaVInfo[i].filename);
		infLOG(ALWAY,"dcmdfups4005> mureka_hash = [%s]\n",pMurekaVInfo[i].mureka_hash);

		if(pMurekaVInfo[i].nFileGubun == 2)
		{
			infLOG(ALWAY,"dcmdfups4005> video_status = [%s]\n",pMurekaVInfo[i].video_status);
			infLOG(ALWAY,"dcmdfups4005> video_id = [%s]\n",pMurekaVInfo[i].video_id);
			infLOG(ALWAY,"dcmdfups4005> video_title = [%s]\n",pMurekaVInfo[i].video_title);
			infLOG(ALWAY,"dcmdfups4005> video_jejak_year = [%s]\n",pMurekaVInfo[i].video_jejak_year);
			infLOG(ALWAY,"dcmdfups4005> video_right_name = [%s]\n",pMurekaVInfo[i].video_right_name);
			infLOG(ALWAY,"dcmdfups4005> video_right_content_id = [%s]\n",pMurekaVInfo[i].video_right_content_id);
			infLOG(ALWAY,"dcmdfups4005> video_grade = [%s]\n",pMurekaVInfo[i].video_grade);
			infLOG(ALWAY,"dcmdfups4005> video_price = [%s]\n",pMurekaVInfo[i].video_price);
			infLOG(ALWAY,"dcmdfups4005> video_cha = [%s]\n",pMurekaVInfo[i].video_cha);
			infLOG(ALWAY,"dcmdfups4005> video_osp_jibun = [%s]\n",pMurekaVInfo[i].video_osp_jibun);
			infLOG(ALWAY,"dcmdfups4005> video_osp_etc = [%s]\n",pMurekaVInfo[i].video_osp_etc);
			infLOG(ALWAY,"dcmdfups4005> video_onair_date = [%s]\n",pMurekaVInfo[i].video_onair_date);
			infLOG(ALWAY,"dcmdfups4005> video_right_id = [%s]\n",pMurekaVInfo[i].video_right_id);
		}
		else if(pMurekaVInfo[i].nFileGubun == 1)
		{
			infLOG(ALWAY,"dcmdfups4005> music_status = [%s]\n",pMurekaVInfo[i].music_status);
			infLOG(ALWAY,"dcmdfups4005> music_id = [%s]\n",pMurekaVInfo[i].music_id);
			infLOG(ALWAY,"dcmdfups4005> music_title = [%s]\n",pMurekaVInfo[i].music_title);
			infLOG(ALWAY,"dcmdfups4005> music_artist = [%s]\n",pMurekaVInfo[i].music_artist);
			infLOG(ALWAY,"dcmdfups4005> music_album = [%s]\n",pMurekaVInfo[i].music_album);
			infLOG(ALWAY,"dcmdfups4005> music_prod_code = [%s]\n",pMurekaVInfo[i].music_prod_code);
			infLOG(ALWAY,"dcmdfups4005> music_price = [%s]\n",pMurekaVInfo[i].music_price);
			infLOG(ALWAY,"dcmdfups4005> music_injeob_com = [%s]\n",pMurekaVInfo[i].music_injeob_com);
			infLOG(ALWAY,"dcmdfups4005> music_injeob_music_id = [%s]\n",pMurekaVInfo[i].music_injeob_music_id);
			infLOG(ALWAY,"dcmdfups4005> video_osp_jibun = [%s]\n",pMurekaVInfo[i].video_osp_jibun);
			infLOG(ALWAY,"dcmdfups4005> video_osp_etc = [%s]\n",pMurekaVInfo[i].video_osp_etc);
			infLOG(ALWAY,"dcmdfups4005> video_onair_date = [%s]\n",pMurekaVInfo[i].video_onair_date);
			infLOG(ALWAY,"dcmdfups4005> video_right_id = [%s]\n",pMurekaVInfo[i].video_right_id);
		}

	}
	#endif
}


//20190123
int Insert1mbHash(MYSQL *con, CMysqlCon MysqlCon,CFUPS4005 pfups4005, char* pRegDate, char* pRegTime ,CFUPS4005_1M_HASH pfups4005hash )
{

	infLOG(ALWAY, "fups4005: Insert1mbHash id  [ %d ] \n",pfups4005.id);

	MYSQL_RES   *res;
	MYSQL_ROW    row;

	char szSysErrMsg[255];
	memset(szSysErrMsg,0x00,sizeof(szSysErrMsg));

	char szQuery[10000];
	memset(szQuery, 0x00, sizeof(szQuery));


	if( strlen(pfups4005hash.hash_1m) > 0 && strlen(pfups4005hash.hash_1m_mureka) > 0 )
	{
		sprintf(szQuery,"insert into zangsi.T_CONT_TEMPLIST_HASH "
						" (  id , hash_1m , hash_1m_mureka , reg_date , reg_time , depth , default_hash, file_size )"
						" values ( %lu , '%s', '%s', '%s', '%s' ,%d,'%s',%15.0f)"
						,pfups4005.id,pfups4005hash.hash_1m,pfups4005hash.hash_1m_mureka , pRegDate,pRegTime ,pfups4005.depth,pfups4005.default_hash,pfups4005.file_size );
						
	}	
	else if( strlen(pfups4005hash.hash_1m) > 0 && strlen(pfups4005hash.hash_1m_mureka) <= 0 )
	{
		sprintf(szQuery,"insert into zangsi.T_CONT_TEMPLIST_HASH "
						" (  id , hash_1m ,  reg_date , reg_time, depth , default_hash, file_size )"
						" values ( %lu , '%s', '%s', '%s' ,%d,'%s',%15.0f)"
						,pfups4005.id,pfups4005hash.hash_1m , pRegDate,pRegTime,pfups4005.depth,pfups4005.default_hash,pfups4005.file_size );
						
	}	
	else if( strlen(pfups4005hash.hash_1m) <= 0 && strlen(pfups4005hash.hash_1m_mureka) > 0 )
	{
		sprintf(szQuery,"insert into zangsi.T_CONT_TEMPLIST_HASH "
						" (  id , hash_1m_mureka ,  reg_date , reg_time , depth , default_hash, file_size )"
						" values ( %lu , '%s', '%s', '%s' ,%d,'%s',%15.0f)"
						,pfups4005.id,pfups4005hash.hash_1m_mureka , pRegDate,pRegTime,pfups4005.depth,pfups4005.default_hash,pfups4005.file_size );
						
	}	
	else
	{
		infLOG(ALWAY, "fups4005->Insert1mbHash 1mb hash is empty [ %lu ]\n",pfups4005.id);
		return -1;
	}
	
	infLOG(ALWAY, "fups4005->Insert1mbHash QUERY [ %s ]\n",szQuery);
	if(MysqlCon.MysqlQuery(con, szSysErrMsg, szQuery))
	{
		infLOG(ERROR, "fups4005->InsertMurekaVideo : MysqlQuery error.\n");
		infLOG(ERROR, "fups4005->InsertMurekaVideo : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
	}

	return 0;
}






