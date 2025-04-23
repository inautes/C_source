/******************************************************************************
 *   서브시스템 : CMD서버
 *   프로그램명 : fups40061.cc
 *         기능 : 필로그 제휴 컨텐츠 조회 및 정보 셋팅
 *         설명 :
 				  수정사항 - 유료컨텐츠
 				  수정사항 - foldername 에 ReplaceSingleQuotation 적용
					  비고 - 적용시에 주석을 풀어주어야 합니다.
				  수정사항 - EBS 제휴 추가
 				  수정사항 - SBS 이벤트(no.677)
 				  수정사항 - MBC 이벤트 종료

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
#include "comconf.h"
#include "commydb.h"
#include "apdefine.h"
#include "../../fupsvr/inc/fups4006.h"
#include "dcmdfups40061.h"
#include "dcmdcomlib.h"


#include "mysql_pool.h"

extern CMysqlPool * m_g_clMysqlPool;
extern CMysqlPool * m_g_clMysqlPoolCopyRight;

int FlogInsertFileData(MYSQL *con, CMysqlCon MysqlCon, MYSQL *cpr_con, CMysqlCon MysqlCprCon
					, char* pDefaultHash, char* pFileName, double dwFileSize, char* pFileType
					, CFUPS4006 pfups4006
					, bool bIsZip, char* szMurekaStatus, char* pRegDate, char* pRegTime
					, LPMUREKA_VINFO pMurekaVInfo, int nMurekaCnt , int nMurekaIndex );

int FlogInsertCprData(MYSQL *con, CMysqlCon MysqlCon, MYSQL *cpr_con, CMysqlCon MysqlCprCon
					, unsigned long ulID, char* pSectCode, char* pSectSub
					, char* szDefaultHash, char* szFileName, double dwFileSize, MUREKA_VINFO MurekaVInfo);

int FlogUpdateCprData(MYSQL *cpr_con, CMysqlCon MysqlCprCon, char* szDefaultHash, char* szFileName, double dwFileSize, MUREKA_VINFO MurekaVInfo);

int FlogInsertMurekaVideo(MYSQL *con, CMysqlCon MysqlCon
					, CFUPS4006 pfups4006
					, LPMUREKA_VINFO pMurekaVInfo, int nMurekaCnt, int nMurekaIndex
					);

long dcmdfups40061(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData)
{
	CFUPS4006 pfups4006 ;
	memset(&pfups4006,0x00,sizeof(CFUPS4006));
	memcpy( &pfups4006 ,  (LPCFUPS4006)pRecvData ,sizeof(CFUPS4006));

	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);

	int nMurekaCnt = pfups4006.mureka_cnt;

	LPMUREKA_VINFO pMurekaVInfo = NULL;

	if(nMurekaCnt > 0)
	{
		char* pTempBuffer = new char[sizeof(MUREKA_VINFO)*nMurekaCnt];
		memcpy( pTempBuffer , pRecvData + sizeof(CFUPS4006) , sizeof(MUREKA_VINFO)*nMurekaCnt);
		pMurekaVInfo = (LPMUREKA_VINFO)pTempBuffer;
	}

	//infLOG(ALWAY,"FUPS40061 | 등록 시작[%lu] [%s] [ %d ]\n", pfups4006.id, pfups4006.file_name,nMurekaCnt);

	#ifdef _DEBUG_
	for(int i=0; i<nMurekaCnt; i++)
	{
		infLOG(ALWAY,"dcmdfups40061> 뮤레카 정보 조회 %d, temp_id=[%lu]\n", i, pfups4006.id);
		infLOG(ALWAY,"dcmdfups40061> nResultCode = [%d]\n",pMurekaVInfo[i].nResultCode);
		infLOG(ALWAY,"dcmdfups40061> nFileGubun = [%d]\n",pMurekaVInfo[i].nFileGubun);
		infLOG(ALWAY,"dcmdfups40061> filename = [%s]\n",pMurekaVInfo[i].filename);
		infLOG(ALWAY,"dcmdfups40061> mureka_hash = [%s]\n",pMurekaVInfo[i].mureka_hash);

		if(pMurekaVInfo[i].nFileGubun == 2)
		{
			infLOG(ALWAY,"dcmdfups40061> video_status = [%s]\n",pMurekaVInfo[i].video_status);
			infLOG(ALWAY,"dcmdfups40061> video_id = [%s]\n",pMurekaVInfo[i].video_id);
			infLOG(ALWAY,"dcmdfups40061> video_title = [%s]\n",pMurekaVInfo[i].video_title);
			infLOG(ALWAY,"dcmdfups40061> video_jejak_year = [%s]\n",pMurekaVInfo[i].video_jejak_year);
			infLOG(ALWAY,"dcmdfups40061> video_right_name = [%s]\n",pMurekaVInfo[i].video_right_name);
			infLOG(ALWAY,"dcmdfups40061> video_right_content_id = [%s]\n",pMurekaVInfo[i].video_right_content_id);
			infLOG(ALWAY,"dcmdfups40061> video_grade = [%s]\n",pMurekaVInfo[i].video_grade);
			infLOG(ALWAY,"dcmdfups40061> video_price = [%s]\n",pMurekaVInfo[i].video_price);
			infLOG(ALWAY,"dcmdfups40061> video_cha = [%s]\n",pMurekaVInfo[i].video_cha);
			infLOG(ALWAY,"dcmdfups40061> video_osp_jibun = [%s]\n",pMurekaVInfo[i].video_osp_jibun);
			infLOG(ALWAY,"dcmdfups40061> video_osp_etc = [%s]\n",pMurekaVInfo[i].video_osp_etc);
			infLOG(ALWAY,"dcmdfups40061> video_onair_date = [%s]\n",pMurekaVInfo[i].video_onair_date);
			infLOG(ALWAY,"dcmdfups40061> video_right_id = [%s]\n",pMurekaVInfo[i].video_right_id);
		}
		else if(pMurekaVInfo[i].nFileGubun == 1)
		{
			infLOG(ALWAY,"dcmdfups40061> music_status = [%s]\n",pMurekaVInfo[i].music_status);
			infLOG(ALWAY,"dcmdfups40061> music_id = [%s]\n",pMurekaVInfo[i].music_id);
			infLOG(ALWAY,"dcmdfups40061> music_title = [%s]\n",pMurekaVInfo[i].music_title);
			infLOG(ALWAY,"dcmdfups40061> music_artist = [%s]\n",pMurekaVInfo[i].music_artist);
			infLOG(ALWAY,"dcmdfups40061> music_album = [%s]\n",pMurekaVInfo[i].music_album);
			infLOG(ALWAY,"dcmdfups40061> music_prod_code = [%s]\n",pMurekaVInfo[i].music_prod_code);
			infLOG(ALWAY,"dcmdfups40061> music_price = [%s]\n",pMurekaVInfo[i].music_price);
			infLOG(ALWAY,"dcmdfups40061> music_injeob_com = [%s]\n",pMurekaVInfo[i].music_injeob_com);
			infLOG(ALWAY,"dcmdfups40061> music_injeob_music_id = [%s]\n",pMurekaVInfo[i].music_injeob_music_id);
			infLOG(ALWAY,"dcmdfups40061> video_osp_jibun = [%s]\n",pMurekaVInfo[i].video_osp_jibun);
			infLOG(ALWAY,"dcmdfups40061> video_osp_etc = [%s]\n",pMurekaVInfo[i].video_osp_etc);
			infLOG(ALWAY,"dcmdfups40061> video_onair_date = [%s]\n",pMurekaVInfo[i].video_onair_date);
			infLOG(ALWAY,"dcmdfups40061> video_right_id = [%s]\n",pMurekaVInfo[i].video_right_id);
		}
	}
	#endif

	//--------------------------------------------------------------------------
	// Wedisk DB
	//--------------------------------------------------------------------------
	bool bDefaultHash = false;
	bool bAudioHash = false;
	bool bVideoHash = false;
	bool bCloseDB = false;

	MYSQL *con=NULL;
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
	int  ErrNum = -1;         // error no

    char reg_date[8+1];
    memset(reg_date,0x00,sizeof(reg_date));

    char reg_time[6+1];
    memset(reg_time,0x00,sizeof(reg_time));

	char szCompID[6+1];
	memset(szCompID,0x00,sizeof(szCompID));

	char szMurekaStatus[5+1];
	memset(szMurekaStatus,0x00,sizeof(szMurekaStatus));
	//char szEventStatus[5+1];	memset(szEventStatus,0x00,sizeof(szEventStatus));

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
		ErrNum = -400692;
		sprintf(ErrMsg, "DB에 접속하지 못 하였습니다.\n");

		infLOG(ERROR, "FUPS40061 | GetMysqlCon is null \n");

		int nRetry = 0;
		while (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			infLOG(ERROR, "FUPS40061 | [%d](%s)\n",ErrNum, ErrMsg);
			//infLOG(ERROR, "FUPS40061 | Mysql Error | Errno : %d | ErrMsg : %s \n", mysql_errno(con),mysql_error(con));
		}

		if( nRetry >= 5)
		{
			req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);

			infLOG(ERROR, "FUPS40061 | Cannot DB Connect \n");

	       	return HEADER_SIZE;
	    }
	    infLOG(ERROR, "FUPS40061 | GetMysqlCon Direct Connect \n");

	    bCloseDB = true;
	}
	//--------------------------------------------------------------------------


	//--------------------------------------------------------------------------
	//유료 컨텐츠 DB 연결
	//--------------------------------------------------------------------------
	CMysqlCon MysqlCprCon(m_g_clMysqlPoolCopyRight,m_g_clMysqlPoolCopyRight->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );
	cpr_con = MysqlCprCon.GetMysqlCon();

	if (cpr_con == NULL )
	{
		ErrNum = -400692;
		sprintf(ErrMsg, "유료 컨텐츠DB에 접속하지 못 하였습니다.\n");

		infLOG(ERROR, "FUPS40061 | MysqlCprCon is null \n");

		int nRetry = 0;
		while (!(cpr_con=db_connect(OSP_CPR_DB_NAME		,OSP_CPR_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);

			infLOG(ERROR, "FUPS40061 |  MysqlCprCon is null [%d](%s)\n",ErrNum, ErrMsg);
			//infLOG(ERROR, "FUPS40061 | Mysql CPR Error | Errno : %d | ErrMsg : %s \n", mysql_errno(cpr_con),mysql_error(cpr_con));
		}
		if( nRetry >= 5)
		{
			req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);

			infLOG(ERROR, "FUPS40061 | Cannot CPR DB Connect \n");
			db_disconnect(con);
	       	return HEADER_SIZE;
	    }
	    infLOG(ERROR, "FUPS40061 | GetMysqlCprCon Direct Connect \n");

	    bCprCloseDB = true;
	}
	//--------------------------------------------------------------------------


	//--------------------------------------------------------------------------
	// 등록일자 얻기
	//--------------------------------------------------------------------------

	memset(szQuery,  0x00, sizeof(szQuery));
	memset(szQuery1, 0x00, sizeof(szQuery1));

	strcpyA(szQuery, "SELECT date_format(now(),'%Y%m%d')");
	strcat(szQuery, "     , date_format(now(),'%H%i%s')");

	if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
	{
		ErrNum = -400601;
		sprintf(ErrMsg, "테이베이스 오류입니다.\n");
		goto fups40061_err;
	}
	if (!(res = mysql_store_result(con)))
	{
		ErrNum = -400602;
		sprintf(ErrMsg, "테이베이스 오류입니다.\n");
		goto fups40061_err;
	}
 	if (mysql_num_rows(res)==0)
 	{
		ErrNum = -400602;
		sprintf(ErrMsg, "검색된 자료가 없습니다.\n");
		mysql_free_result(res);
		goto fups40061_err;
	}
	row = mysql_fetch_row(res);

	memset(reg_date     , 0x00, sizeof(reg_date     ));
	memset(reg_time     , 0x00, sizeof(reg_time     ));

	strcpyA(reg_date     , getstr(row, 0));
	strcpyA(reg_time     , getstr(row, 1));

	mysql_free_result(res);
	//--------------------------------------------------------------------------


	ReplaceSingleQuotation(pfups4006.file_name, '\'',pfups4006.file_name);
	ReplaceSingleQuotation(pfups4006.folder_name, '\'',pfups4006.folder_name); /// 인탁 20090306일 수정


	memset (szQuery , 0x00, sizeof(szQuery ));
	strcpyA(pfups4006.copyright_yn, "N");
	infLOG(ALWAY,"dcmdfups40061> copyright_yn 1 = [%lu][%s]\n",pfups4006.id, pfups4006.copyright_yn);

	if( strcmp(pfups4006.cont_gu, "FD") == 0)
	{
		//2009/06/15 - HCS : 뮤레카 정보중 유료 컨텐츠가 있는지 검사.
		/*
		클라이언트로 부터 넘겨 받은 뮤레카 정보중 유료 컨텐츠가 있다면
		zangsi_cpr DB의 제휴 데이터를 수정한다. zangsi DB 의 가격은 백단데몬에서 일괄 수정.
		뮤레카 정보가 하나도 없다면 기존방식대로 처리.
		*/
		bool bIsZip = false;

		if(nMurekaCnt > 1)
			bIsZip = true;

		//--------------------------------------------------------------------------
		//뮤레카 필터링 거친 파일 등록
		//--------------------------------------------------------------------------
		//for(int i=0; i < nMurekaCnt; i++) 서버가 자꾸 죽어서 임시로 처리
		if( nMurekaCnt  > 0 )
		{
			for(int i=0; i < 1; i++)
			{
				char szHashValue[32+1];
				memset(szHashValue, 0x00, sizeof(szHashValue));
				char szSize[128];
				memset(szSize, 0x00, sizeof(szSize));
				double dwSize = 0;
				memset(szMurekaStatus,0x00,sizeof(szMurekaStatus));
				//--------------------------------------------------------------------------
				//동영상 필터링
				//--------------------------------------------------------------------------
				if(pMurekaVInfo[i].nFileGubun == 2)
				{
					//strcpy(szFileType, "동영상");

					if(pMurekaVInfo[i].nResultCode == 0)
					{
						memset(szMurekaStatus, 0x00, sizeof(szMurekaStatus));
						strcpyA(szMurekaStatus, pMurekaVInfo[i].video_status);
					}
					else
					{
						memset(szMurekaStatus, 0x00, sizeof(szMurekaStatus));
						sprintf(szMurekaStatus, "%d", pMurekaVInfo[i].nResultCode);
					}

					memset(pfups4006.video_hash, 0x00, sizeof(pfups4006.video_hash));
					strcpyA(pfups4006.video_hash, pMurekaVInfo[i].mureka_hash);

					if(nMurekaCnt > 1)
					{
						memcpy(szHashValue, pfups4006.video_hash, 32);
						memcpy(szSize, pfups4006.video_hash + 33, strlen(pfups4006.video_hash));
						dwSize = atof(szSize);
					}
					else
					{
						sprintf(szHashValue, "%s", pfups4006.default_hash);
						dwSize = pfups4006.file_size;
					}

					ReplaceSingleQuotation(pMurekaVInfo[i].video_title, '\'',pMurekaVInfo[i].video_title);

					if(strcmp(pMurekaVInfo[i].video_status, "01") == 0)
					{//유료 컨텐츠.
						infLOG(ALWAY,"dcmdfups40061> 유료 항목 있음.\n");
						infLOG(ALWAY,"dcmdfups40061> szMurekaStatus (%s).\n", szMurekaStatus);
						infLOG(ALWAY,"dcmdfups40061> 관리코드 (%s).\n", pMurekaVInfo[i].video_right_content_id);
						infLOG(ALWAY,"dcmdfups40061> 제목 (%s)\n",pMurekaVInfo[i].video_title);
						infLOG(ALWAY,"dcmdfups40061> 제휴사 (%s).\n", pMurekaVInfo[i].video_right_name);
						infLOG(ALWAY,"dcmdfups40061> 금액 (%s).\n", pMurekaVInfo[i].video_price);
						infLOG(ALWAY,"dcmdfups40061> 등급 (%s).\n", pMurekaVInfo[i].video_grade);
						infLOG(ALWAY,"dcmdfups40061> 차수 (%s).\n", pMurekaVInfo[i].video_cha);

						if(pfups4006.file_size > 1024*1024*10) //20100429 - HCS : 10M 이하의 제휴 컨텐츠 존재함.
						{
							//--------------------------------------------------------------------------
							//제휴 정보 등록및 변경 처리
							//--------------------------------------------------------------------------
							int nRet = FlogInsertCprData(con, MysqlCon, cpr_con, MysqlCprCon, pfups4006.id, "FD", "FD", szHashValue, pfups4006.file_name, dwSize, pMurekaVInfo[i]);//,szEventStatus);

							if(nRet < 0)
							{
								strcpyA(ErrMsg, "제휴 데이터 인서트 오류.");
								infLOG(ERROR, "%s\n",ErrMsg);
							}
							if(nRet == 5)
								strcpy(szMurekaStatus, "30");
							else if(nRet == 6)
								strcpy(szMurekaStatus, "31");
							else if(nRet == 7)
								strcpy(szMurekaStatus, "32");
							else if(nRet == 8)
								strcpy(szMurekaStatus, "33"); //20100805. EBS
							else if(nRet == 9)
								strcpy(szMurekaStatus, "34"); //20110217. KBSI
							else if( nRet == 13 )//20160618
								strcpy(szMurekaStatus, "HI"); //cj event

							/*
							if( strlen(szEventStatus) > 0 )
							{
								strcpy(szMurekaStatus,szEventStatus);
							}
							*/


							//--------------------------------------------------------------------------
						}
					}
					else if(strcmp(pMurekaVInfo[i].video_status, "00") == 0)
					{// 무료 컨텐츠.
						infLOG(ALWAY,"dcmdfups40061> 무료 항목 있음.\n");
						infLOG(ALWAY,"dcmdfups40061> szMurekaStatus (%s).\n", szMurekaStatus);
						infLOG(ALWAY,"dcmdfups40061> 관리코드 (%s).\n", pMurekaVInfo[i].video_right_content_id);
						infLOG(ALWAY,"dcmdfups40061> 제휴사 (%s).\n", pMurekaVInfo[i].video_right_name);
						infLOG(ALWAY,"dcmdfups40061> video id (%s).\n", pMurekaVInfo[i].video_id);

						if(pfups4006.file_size > 1024*1024*10)
						{
							//--------------------------------------------------------------------------
							//제휴 서비스 취소 처리
							//--------------------------------------------------------------------------
							int nRet = FlogUpdateCprData(cpr_con, MysqlCprCon, pfups4006.default_hash, pfups4006.file_name, pfups4006.file_size, pMurekaVInfo[i]);
							if(nRet < 0)
							{
								infLOG(ERROR, "dcmdfups40061>제휴컨텐츠  뮤료 전환 업데이트 오류.\n");
							}
						}
					}
				}
				else if(pMurekaVInfo[i].nFileGubun == 0 && pMurekaVInfo[i].nResultCode != 0 && pMurekaVInfo[i].nResultCode <= 99)
				{
					memset(szMurekaStatus, 0x00, sizeof(szMurekaStatus));
					sprintf(szMurekaStatus, "%d", pMurekaVInfo[i].nResultCode);
				}



				//--------------------------------------------------------------------------
				//파일정보 등록
				//--------------------------------------------------------------------------
				if(bIsZip)
				{
					if(i == 0)
					{
						int nResult = FlogInsertFileData(con, MysqlCon, cpr_con, MysqlCprCon, "", "", 0, szFileType, pfups4006, false, szMurekaStatus, reg_date, reg_time,pMurekaVInfo,nMurekaCnt,i);

						if(nResult == -1)
							goto fups40061_err;
						else if(nResult == 1)
						{
							strcpy(pfups4006.copyright_yn, "C");
							infLOG(ALWAY,"dcmdfups40061> copyright_yn 2 = [%lu][%s]\n",pfups4006.id, pfups4006.copyright_yn);
						}
					}
					else if(pMurekaVInfo[i].nFileGubun == 2 || pMurekaVInfo[i].nFileGubun == 1 || pMurekaVInfo[i].nFileGubun == 4)
					{
						int nResult = FlogInsertFileData(con, MysqlCon, cpr_con, MysqlCprCon, szHashValue, pMurekaVInfo[i].filename, dwSize, szFileType, pfups4006, bIsZip, szMurekaStatus, reg_date, reg_time,pMurekaVInfo,nMurekaCnt,i);
							if(nResult == -1)
								goto fups40061_err;
							else if(nResult == 1)
							{
								strcpy(pfups4006.copyright_yn, "C");
								infLOG(ALWAY,"dcmdfups40061> copyright_yn 3 = [%lu][%s]\n",pfups4006.id, pfups4006.copyright_yn);
							}
					}
				}
				else
				{
					int nResult = FlogInsertFileData(con, MysqlCon, cpr_con, MysqlCprCon, szHashValue, pMurekaVInfo[i].filename, dwSize, szFileType, pfups4006, bIsZip, szMurekaStatus, reg_date, reg_time,pMurekaVInfo,nMurekaCnt,i);
						if(nResult == -1)
							goto fups40061_err;
						else if(nResult == 1)
						{
							strcpy(pfups4006.copyright_yn, "C");
							infLOG(ALWAY,"dcmdfups40061> copyright_yn 4 = [%lu][%s]\n",pfups4006.id, pfups4006.copyright_yn);
						}
				}
				//--------------------------------------------------------------------------
			}
		}

		//--------------------------------------------------------------------------
		//뮤레카 필터링 거치지 않은 파일 등록
		//--------------------------------------------------------------------------
		if(nMurekaCnt <= 0)
		{
			int nResult = FlogInsertFileData(con, MysqlCon, cpr_con, MysqlCprCon, "", "", 0, szFileType, pfups4006, false, szMurekaStatus, reg_date, reg_time,pMurekaVInfo,nMurekaCnt,0);

			if(nResult == -1)
				goto fups40061_err;
			else if(nResult == 1)
			{
				strcpy(pfups4006.copyright_yn, "C");
				infLOG(ALWAY,"dcmdfups40061> copyright_yn 5 = [%lu][%s]\n",pfups4006.id, pfups4006.copyright_yn);
			}
		}
		//--------------------------------------------------------------------------
	}
	else
	{
		infLOG(ALWAY,"FUPS40061 | 등록 실패, 잘못된 분류입니다.  분류 코드 [ %s ] \n", pfups4006.cont_gu);
		goto fups40061_err;
	}

	//infLOG(ALWAY,"FUPS40061 | 등록 완료[%lu]\n", pfups4006.id);

	if( bCloseDB )
		db_disconnect(con);

	if( bCprCloseDB )
		db_disconnect(cpr_con);

	if( strcmp(pfups4006.copyright_yn,"C") == 0)
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

  return HEADER_SIZE;

//------------------------------------------------------------------------------
fups40061_err:

	infLOG(ALWAY,"FUPS40061> fups40061_err [%lu] \n",pfups4006.id);

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
*	  3. 1	 	: 제휴 컨텐츠
*****************************************************************************/
int FlogInsertFileData(MYSQL *con, CMysqlCon MysqlCon, MYSQL *cpr_con, CMysqlCon MysqlCprCon
					, char* pDefaultHash, char* pFileName, double dwFileSize, char* pFileType
					, CFUPS4006 pfups4006
					, bool bIsZip, char* pMurekaStatus, char* pRegDate, char* pRegTime
					, LPMUREKA_VINFO pMurekaVInfo, int nMurekaCnt, int nMurekaIndex )
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
    sprintf(szExtHash, "%s", pfups4006.default_hash);

    char szEventFileType[6+1];	memset(szEventFileType,0x00,sizeof(szEventFileType));

	if(bIsZip)
	{
		memset(pfups4006.default_hash, 0x00, sizeof(pfups4006.default_hash));
		sprintf(pfups4006.default_hash, "%s", pDefaultHash);

		pfups4006.file_size = dwFileSize;

		memset(pfups4006.file_name, 0x00, sizeof(pfups4006.file_name));
		sprintf(pfups4006.file_name, "%s", pFileName);

		strcpy(szExtHash, pDefaultHash);
	}


	#ifdef _DEBUG_
	printf("InsertFileData> 해시   (%s).\n", pfups4006.default_hash);
	printf("InsertFileData> 파일명 (%s).\n", pfups4006.file_name);
	printf("InsertFileData> 사이즈 (%.0f).\n", pfups4006.file_size);
	#endif


	unsigned long lChiID = 0;
	long lPriceAmt = 0;
	char szCompID[6+1];
	memset(szCompID,0x00,sizeof(szCompID));

	//--------------------------------------------------------------------------
	// 유료 컨텐츠 조회
	//--------------------------------------------------------------------------
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select a.chi_id  ,c.price_amt , a.comp_cd ,c.mgr_cd , c.chapter "
					 " from zangsi_cpr.T_CPR_HASH_INFO a, zangsi_cpr.T_CPR_HASH_FILE b, zangsi_cpr.T_CPR_CONT_LIST c "
					 " where a.proc_stat = 'C' and a.chi_id = b.chi_id and a.list_id = c.list_id and c.apply_yn = 'Y' and b.file_size = %15.0f and b.default_hash = '%s' limit 1"
					 , pfups4006.file_size
					 , szExtHash);

	//infLOG(ALWAY,"fups4006->InsertFileData : 유료 조회 [ %s ] \n",szQuery);

	#ifdef __DEBUG
	printf("InsertFileData> query = [%s]\n\n", szQuery);
	#endif
	if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		infLOG(ERROR, "fups40061->FlogInsertFileData : MysqlQuery 오류.\n");
		infLOG(ERROR, "fups40061->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
	}
	else
	{
		//유료 컨텐츠이면
		if (!(cpr_res = mysql_store_result(cpr_con)))
		{
			infLOG(ERROR, "fups40061->FlogInsertFileData : mysql_store_result 오류.\n");
			infLOG(ERROR, "fups40061->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
		}
		else
		{
			if (mysql_num_rows(cpr_res) > 0 )
			{
				char szMgrCd[24];
				memset(szMgrCd,0x00,sizeof(szMgrCd));

				char szChapter[24];
				memset(szChapter,0x00,sizeof(szChapter));



				memset(pfups4006.copyright_yn, 0x00, sizeof(pfups4006.copyright_yn));
				strcpy(pfups4006.copyright_yn, "C");
				infLOG(ALWAY,"dcmdfups40061> copyright_yn f1 = [%lu][%s]\n",pfups4006.id, pfups4006.copyright_yn);

				if(cpr_row = mysql_fetch_row(cpr_res))
				{
					lChiID = (unsigned long)getnum(cpr_row,0);
					lPriceAmt = (long)getnum(cpr_row,1);
					strcpyA( szCompID , getstr(cpr_row,2));

					strcpy(szMgrCd , getstr(cpr_row,3));
					strcpy(szChapter,getstr(cpr_row,4));

				}
				nResult = 1;

				mysql_free_result(cpr_res);


				//20120612 - event 검사 - 이벤트일시에 무료 등록 여부를 판단하여 pEventStatus 에 추가

					memset(szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, "select cont_type , list_id from zangsi_cpr.T_CPR_EVENT_CONT_LIST where mgr_cd = '%s'  and chapter = %s and use_yn='Y' " , szMgrCd ,szChapter );

					infLOG(ALWAY, "%s",szQuery);


					if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
					{
						infLOG(ERROR, "%s","조회 오류.\nszQuery=[%s]\n", szQuery);
						infLOG(ERROR, "fups4006->FlogInsertFileData  [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
						//return -1;
					}

					if (!(cpr_res = mysql_store_result(cpr_con)))
					{
						infLOG(ERROR, "%s","데이터 생성 오류.\nszQuery=[%s]\n", szQuery);
						infLOG(ERROR, "fups4006->FlogInsertFileData  [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
						//mysql_free_result(cpr_res);
						//return -1;
					}


					if (mysql_num_rows(cpr_res) > 0 )
				 	{
				 		cpr_row = mysql_fetch_row(cpr_res);

						if( getstr(cpr_row,0) != NULL )
						{
							strcpy(szEventFileType,getstr(cpr_row,0));
							pMurekaStatus = szEventFileType;
						}


					}
					mysql_free_result(cpr_res);

			}
			else
			{
				if( strcmp(pfups4006.auth_num,"CPR") == 0  )//제휴 아이디
				{
					memset(pfups4006.copyright_yn, 0x00, sizeof(pfups4006.copyright_yn));
					strcpy(pfups4006.copyright_yn, "C");
					strcpy(szCompID, "CPR");
					nResult = 1;
					infLOG(ALWAY,"dcmdfups40061> copyright_yn f2 = [%lu][%s]\n",pfups4006.id, pfups4006.copyright_yn);
					infLOG(ALWAY, "fups40061->FlogInsertFileData : 제휴 아이디 등록. %s -- %s\n", pfups4006.user_id, pfups4006.auth_num);
					#ifdef _DEBUG_
					printf("fups40061->FlogInsertFileData : 제휴 아이디 등록. %s -- %s\n", pfups4006.user_id, pfups4006.auth_num);
					#endif
				}
				else
				{
					// 조회 안될때
					// strcpy(pfups4006.copyright_yn, "B");
					infLOG(ALWAY,"dcmdfups40061> copyright_yn f3 = [%lu][%s]\n",pfups4006.id, pfups4006.copyright_yn);
				}
				mysql_free_result(cpr_res);
			}

		}
	}
	//--------------------------------------------------------------------------


	if( strcmp(pfups4006.folder_yn, "N") == 0 )
	{
		memset(szQuery , 0x00, sizeof(szQuery ));
		sprintf(szQuery, "REPLACE INTO zangsi.T_CONTFLOG_TEMPLIST           "
		                 "       (id            ,seq_no        ,folder_yn  "
		                 "       ,file_name     ,file_size     ,file_type  "
		                 "       ,reg_user      ,reg_date      ,reg_time		,default_hash , audio_hash , video_hash	,copyright_yn )  "
		                 " VALUES (%lu			,%d			   ,'N'              "
		                 "			,'%s'			,%13.0f		   ,'2'			"
		                 "			,'%s'			,'%s'			   ,'%s'				,'%s' ,'%s' ,'%s' , '%s'	) "
		                 ,pfups4006.id
		                 ,pfups4006.seq_no
		                 ,pfups4006.file_name
		                 ,pfups4006.file_size
		                 ,pfups4006.user_id
		                 ,pRegDate
		                 ,pRegTime
		                 ,pfups4006.default_hash
		                 ,pfups4006.audio_hash
		                 ,pfups4006.video_hash
		                 ,pfups4006.copyright_yn );

		if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
		{
			infLOG(ERROR, "fups40061->FlogInsertFileData : MysqlQuery 오류.\n");
			infLOG(ERROR, "fups40061->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
			return -1;
	  	}

		//--------------------------------------------------------------------------
		if( pfups4006.file_size <= 0 || pfups4006.default_hash == NULL || strlen(pfups4006.default_hash) <= 0 )
			return 0;

		//--------------------------------------------------------------------------
		//중복 데이터 조회
		//--------------------------------------------------------------------------
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "select seq_no, copyright_yn from zangsi.T_CONTFLOG_TEMPLIST_SUB           "
						 " where id = %lu and  file_size = %15.0f and default_hash = '%s'  limit 1"
		                 ,pfups4006.id
		                 ,pfups4006.file_size
		                 ,pfups4006.default_hash
		                 );

	  	bool bFirst = true;
	  	bool bCprFirst = true;
	  	unsigned long ulSeqNO = 0;
	  	char szCprYn[2];
	  	memset(szCprYn, 0x00, sizeof(szCprYn));

		if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
		{
			infLOG(ERROR, "fups40061->FlogInsertFileData : MysqlQuery 오류.\n");
			infLOG(ERROR, "fups40061->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
			return -1;
	  	}
		if (!(res = mysql_store_result(con)))
		{
			infLOG(ERROR, "fups40061->FlogInsertFileData : mysql_store_result 오류.\n");
			infLOG(ERROR, "fups40061->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
			return -1;
		}
		if(row = mysql_fetch_row(res))
		{
			if (mysql_num_rows(res) > 0 )
			{
				bFirst = false;
				ulSeqNO = (unsigned long)getnum(row,0);
				sprintf(szCprYn, "%s", getstr(row,1));

				if(strcmp(szCprYn, pfups4006.copyright_yn) != 0)
				{
					memset(szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, "delete from zangsi.T_CONTFLOG_TEMPLIST_SUB where seq_no = %lu", ulSeqNO);
					if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
					{
						infLOG(ERROR, "fups40061->FlogInsertFileData : MysqlQuery 오류.\n");
						infLOG(ERROR, "fups40061->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
						return -1;
				  	}
				  	bFirst = true;
				}

			}
		}
		mysql_free_result(res);
		//--------------------------------------------------------------------------


		//--------------------------------------------------------------------------
		//중복 데이터가 없다면 파일정보 등록
		//--------------------------------------------------------------------------
		if( bFirst )
		{

				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, "insert into zangsi.T_CONTFLOG_TEMPLIST_SUB "
									 " ( id , depth, file_type , folder_path , file_name, file_size, reg_user, reg_date,reg_time, default_hash , audio_hash, video_hash, copyright_yn	 ) "
					                 " values "
					                 " ( %d , %d   , '%s'      , '%s'        , '%s'     , %13.0f   , '%s'    , '%s'    ,'%s'    , '%s'         , '%s'      , '%s'      , '%s' )   "
					                 ,pfups4006.id
					                 ,pfups4006.depth
					                 ,pMurekaStatus
					                 ,pfups4006.folder_name
					                 ,pfups4006.file_name
					                 ,pfups4006.file_size
					                 ,pfups4006.user_id
					                 ,pRegDate
					                 ,pRegTime
					                 ,pfups4006.default_hash
					                 ,pfups4006.audio_hash
					                 ,pfups4006.video_hash
					                 ,pfups4006.copyright_yn
					                 );
		 		#ifdef _DEBUG_
		 		printf("InsertFileData> szQuery=[%s].\n", szQuery);
		 		#endif
				if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
				{
					infLOG(ERROR, "fups40061->FlogInsertFileData : MysqlQuery 오류.\n");
					infLOG(ERROR, "fups40061->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);

			  	}
			  	else
			  	{
			  		//if( bIsZip == false && nMurekaCnt == 1)
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
									CFUPS4006 fups4006 = pfups4006;
									fups4006.seq_no = filelist_seq;

									FlogInsertMurekaVideo(con,  MysqlCon	, fups4006, pMurekaVInfo,nMurekaCnt ,  nMurekaIndex );
								}
							}
						}
					}
				}


		}
		//--------------------------------------------------------------------------

		//-------------------
		//제휴 데이터 입력
		//-------------------
		if(strcmp(pfups4006.copyright_yn, "C") == 0)
		{
			memset(szQuery , 0x00, sizeof(szQuery ));
			sprintf(szQuery, "select seq_no, copyright_yn from zangsi_cpr.T_CONTFLOG_TEMPLIST_SUB           "
							 " where id = %lu and depth = %d  and  file_size = %.0f and default_hash = '%s'  limit 1"
			                 ,pfups4006.id
			                 ,pfups4006.depth
			                 ,pfups4006.file_size
			                 ,pfups4006.default_hash);

			if (MysqlCprCon.MysqlQuery(cpr_con,szSysErrMsg, szQuery))
			{
				infLOG(ERROR, "fups40061->FlogInsertFileData : MysqlQuery 오류.\n");
				infLOG(ERROR, "fups40061->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
				return -1;
		  	}
			if (!(cpr_res = mysql_store_result(cpr_con)))
			{
				infLOG(ERROR, "fups40061->FlogInsertFileData : mysql_store_result 오류.\n");
				infLOG(ERROR, "fups40061->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
				return -1;
			}
			if(cpr_row = mysql_fetch_row(cpr_res))
			{
				if (mysql_num_rows(cpr_res) > 0 )
				{
					bCprFirst = false;
					ulSeqNO = (unsigned long)getnum(cpr_row,0);
					memset(szCprYn, 0x00, sizeof(szCprYn));
					strcpy(szCprYn, getstr(cpr_row,1));
					if(strcmp(szCprYn, pfups4006.copyright_yn) != 0)
					{
						memset(szQuery, 0x00, sizeof(szQuery));
						sprintf(szQuery, "delete from zangsi_cpr.T_CONTFLOG_TEMPLIST_SUB where seq_no = %lu", ulSeqNO);
						if (MysqlCprCon.MysqlQuery(cpr_con,szSysErrMsg, szQuery))
						{
							infLOG(ERROR, "fups40061->FlogInsertFileData : MysqlQuery 오류.\n");
							infLOG(ERROR, "fups40061->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
							mysql_free_result(cpr_res);
							return -1;
					  	}
					  	bCprFirst = true;
					}
				}
			}
			mysql_free_result(cpr_res);

			if( bCprFirst )
			{
				memset(szQuery , 0x00, sizeof(szQuery ));
				sprintf(szQuery, "insert into zangsi_cpr.T_CONTFLOG_TEMPLIST_SUB           "
								 " ( id, depth, file_type, folder_path , file_name, file_size, reg_user, reg_date, reg_time, default_hash, audio_hash, video_hash, copyright_yn, comp_cd, chi_id, price_amt) "
				                 " values "
				                 " ( %d, %d   , '%s'     , '%s'        , '%s'     , %13.0f   , '%s'     , '%s'   ,'%s'     , '%s'        , '%s'      , '%s'      , '%s'        , '%s'   , %lu    , %d )   "
				                 ,pfups4006.id
				                 ,pfups4006.depth
				                 ,pMurekaStatus
				                 ,pfups4006.folder_name
				                 ,pfups4006.file_name
				                 ,pfups4006.file_size
				                 ,pfups4006.user_id
				                 ,pRegDate
				                 ,pRegTime
				                 ,pfups4006.default_hash
				                 ,pfups4006.audio_hash
				                 ,pfups4006.video_hash
				                 ,pfups4006.copyright_yn
				                 ,szCompID
				                 ,lChiID
				                 ,lPriceAmt);
				#ifdef _DEBUG_
				printf("\n\nfups4006 szQuery %s \n\n",szQuery);
				#endif
				if (MysqlCprCon.MysqlQuery(cpr_con,szSysErrMsg, szQuery))
				{
					infLOG(ERROR, "fups40061->FlogInsertFileData : MysqlQuery 오류.\n");
					infLOG(ERROR, "fups40061->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
					return -1;
			 	}
			}
		}



	}
	else if( strcmp(pfups4006.folder_yn, "Y") == 0 )
	{
		if( pfups4006.depth == 0 )
		{
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "update zangsi.T_CONTFLOG_TEMPLIST           "
			                 " set file_size = %.0f, default_hash = '%s' , audio_hash = '%s' , video_hash = '%s' , copyright_yn = '%s' "
			                 " WHERE id  =  %lu and reg_user = '%s' and file_name = '%s'          "
			                 ,pfups4006.file_size
			                 ,pfups4006.default_hash
			                 ,pfups4006.audio_hash
			                 ,pfups4006.video_hash
			                 ,pfups4006.copyright_yn
			                 ,pfups4006.id
			                 ,pfups4006.user_id
			                 ,pfups4006.file_name);

			if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
			{
				infLOG(ERROR, "fups40061->FlogInsertFileData : MysqlQuery 오류.\n");
				infLOG(ERROR, "fups40061->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
				return -1;
		  	}
		}

		if( pfups4006.file_size <= 0 || pfups4006.default_hash == NULL || strlen(pfups4006.default_hash) <= 0 )
			return 0;

		//--------------------------------------------------------------------------
		//중복 데이터 조회
		//--------------------------------------------------------------------------
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "select seq_no, copyright_yn from zangsi.T_CONTFLOG_TEMPLIST_SUB           "
						 " where id = %lu and  file_size = %15.0f and default_hash = '%s' and depth = %d limit 1"
		                 ,pfups4006.id
		                 ,pfups4006.file_size
		                 ,pfups4006.default_hash
		                 ,pfups4006.depth
		                 );

	  	bool bFirst = true;
	  	bool bCprFirst = true;
	  	unsigned long ulSeqNO = 0;
	  	char szCprYn[2];
	  	memset(szCprYn, 0x00, sizeof(szCprYn));

		if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
		{
			infLOG(ERROR, "fups40061->FlogInsertFileData : MysqlQuery 오류.\n");
			infLOG(ERROR, "fups40061->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
			return -1;
	  	}
		if (!(res = mysql_store_result(con)))
		{
			infLOG(ERROR, "fups40061->FlogInsertFileData : mysql_store_result 오류.\n");
			infLOG(ERROR, "fups40061->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
			return -1;
		}
		if(row = mysql_fetch_row(res))
		{
			if (mysql_num_rows(res) > 0 )
			{
				bFirst = false;
				ulSeqNO = (unsigned long)getnum(row,0);
				sprintf(szCprYn, "%s", getstr(row,1));
				mysql_free_result(res);
				if(strcmp(szCprYn, pfups4006.copyright_yn) != 0)
				{
					memset(szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, "delete from zangsi.T_CONTFLOG_TEMPLIST_SUB where seq_no = %lu", ulSeqNO);
					if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
					{
						infLOG(ERROR, "fups40061->FlogInsertFileData : MysqlQuery 오류.\n");
						infLOG(ERROR, "fups40061->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
						return -1;
				  	}
				  	bFirst = true;
				}

			}
			else
				mysql_free_result(res);
		}
		else
			mysql_free_result(res);

		//--------------------------------------------------------------------------


		//--------------------------------------------------------------------------
		//중복 데이터가 없다면 파일정보 등록
		//--------------------------------------------------------------------------
		if( bFirst )
		{


				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, "insert into zangsi.T_CONTFLOG_TEMPLIST_SUB "
									 " ( id , depth, file_type , folder_path , file_name, file_size, reg_user, reg_date,reg_time, default_hash , audio_hash, video_hash, copyright_yn	 ) "
					                 " values "
					                 " ( %d , %d   , '%s'      , '%s'        , '%s'     , %13.0f   , '%s'    , '%s'    ,'%s'    , '%s'         , '%s'      , '%s'      , '%s' )   "
					                 ,pfups4006.id
					                 ,pfups4006.depth
					                 ,pMurekaStatus
					                 ,pfups4006.folder_name
					                 ,pfups4006.file_name
					                 ,pfups4006.file_size
					                 ,pfups4006.user_id
					                 ,pRegDate
					                 ,pRegTime
					                 ,pfups4006.default_hash
					                 ,pfups4006.audio_hash
					                 ,pfups4006.video_hash
					                 ,pfups4006.copyright_yn
					                 );
		 		#ifdef _DEBUG_
		 		printf("InsertFileData> szQuery=[%s].\n", szQuery);
		 		#endif
				if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
				{
					infLOG(ERROR, "fups40061->FlogInsertFileData : MysqlQuery 오류.\n");
					infLOG(ERROR, "fups40061->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
					//return -1;
			  	}
			  	else
			  	{
			  		//if( bIsZip == false && nMurekaCnt == 1)
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
									CFUPS4006 fups4006 = pfups4006;
									fups4006.seq_no = filelist_seq;
									FlogInsertMurekaVideo(con,  MysqlCon	, fups4006, pMurekaVInfo,nMurekaCnt ,nMurekaIndex);
								}
							}
						}
					}
				}


		}
		//--------------------------------------------------------------------------

		//-------------------
		//제휴 데이터 입력
		//-------------------
		if(strcmp(pfups4006.copyright_yn, "C") == 0)
		{
			memset(szQuery , 0x00, sizeof(szQuery ));
			sprintf(szQuery, "select seq_no, copyright_yn from zangsi_cpr.T_CONTFLOG_TEMPLIST_SUB           "
							 " where id = %lu and depth = %d  and  file_size = %.0f and default_hash = '%s'  limit 1"
			                 ,pfups4006.id
			                 ,pfups4006.depth
			                 ,pfups4006.file_size
			                 ,pfups4006.default_hash);

			if (MysqlCprCon.MysqlQuery(cpr_con,szSysErrMsg, szQuery))
			{
				infLOG(ERROR, "fups40061->FlogInsertFileData : MysqlQuery 오류.\n");
				infLOG(ERROR, "fups40061->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
				return -1;
		  	}
			if (!(cpr_res = mysql_store_result(cpr_con)))
			{
				infLOG(ERROR, "fups40061->FlogInsertFileData : mysql_store_result 오류.\n");
				infLOG(ERROR, "fups40061->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
				return -1;
			}
			if(cpr_row = mysql_fetch_row(cpr_res))
			{
				if (mysql_num_rows(cpr_res) > 0 )
				{
					bCprFirst = false;
					ulSeqNO = (unsigned long)getnum(cpr_row,0);
					memset(szCprYn, 0x00, sizeof(szCprYn));
					strcpy(szCprYn, getstr(cpr_row,1));
					if(strcmp(szCprYn, pfups4006.copyright_yn) != 0)
					{
						memset(szQuery, 0x00, sizeof(szQuery));
						sprintf(szQuery, "delete from zangsi_cpr.T_CONTFLOG_TEMPLIST_SUB where seq_no = %lu", ulSeqNO);
						if (MysqlCprCon.MysqlQuery(cpr_con,szSysErrMsg, szQuery))
						{
							infLOG(ERROR, "fups40061->FlogInsertFileData : MysqlQuery 오류.\n");
							infLOG(ERROR, "fups40061->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
							mysql_free_result(cpr_res);
							return -1;
					  	}
					  	bCprFirst = true;
					}
				}
			}
			mysql_free_result(cpr_res);

			if( bCprFirst )
			{
				memset(szQuery , 0x00, sizeof(szQuery ));
				sprintf(szQuery, "insert into zangsi_cpr.T_CONTFLOG_TEMPLIST_SUB           "
								 " ( id, depth, file_type, folder_path , file_name, file_size, reg_user, reg_date, reg_time, default_hash, audio_hash, video_hash, copyright_yn, comp_cd, chi_id, price_amt) "
				                 " values "
				                 " ( %d, %d   , '%s'     , '%s'        , '%s'     , %13.0f   , '%s'     , '%s'   ,'%s'     , '%s'        , '%s'      , '%s'      , '%s'        , '%s'   , %lu    , %d )   "
				                 ,pfups4006.id
				                 ,pfups4006.depth
				                 ,pMurekaStatus
				                 ,pfups4006.folder_name
				                 ,pfups4006.file_name
				                 ,pfups4006.file_size
				                 ,pfups4006.user_id
				                 ,pRegDate
				                 ,pRegTime
				                 ,pfups4006.default_hash
				                 ,pfups4006.audio_hash
				                 ,pfups4006.video_hash
				                 ,pfups4006.copyright_yn
				                 ,szCompID
				                 ,lChiID
				                 ,lPriceAmt);
				#ifdef _DEBUG_
				printf("\n\nfups4006 szQuery %s \n\n",szQuery);
				#endif
				if (MysqlCprCon.MysqlQuery(cpr_con,szSysErrMsg, szQuery))
				{
					infLOG(ERROR, "fups40061->FlogInsertFileData : MysqlQuery 오류.\n");
					infLOG(ERROR, "fups40061->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
					return -1;
			 	}
			}
		}
	}

	return nResult;
}

int FlogInsertCprData(MYSQL *con, CMysqlCon MysqlCon, MYSQL *cpr_con, CMysqlCon MysqlCprCon, unsigned long ulID, char* pSectCode, char* pSectSub, char* szDefaultHash, char* szFileName, double dwFileSize, MUREKA_VINFO MurekaVInfo)//,char *pEventStatus)
{
	MYSQL_RES   *cpr_res2;
	MYSQL_ROW    cpr_row2;

	int nResult = 0;

	char ErrMsg[256];    // error message
	memset(ErrMsg   , 0x00, sizeof(ErrMsg   )); // 오류메시지

	char szSysErrMsg[255];
	memset(szSysErrMsg,0x00,sizeof(szSysErrMsg));

	char szQuery[15000];
	memset(szQuery, 0x00, sizeof(szQuery));

	char szCompCd[6+1];
	memset(szCompCd, 0x00, sizeof(szCompCd));

    char reg_date[8+1];
    memset(reg_date,0x00,sizeof(reg_date));

    char reg_time[6+1];
    memset(reg_time,0x00,sizeof(reg_time));

	int nMPriceAmt = atoi(MurekaVInfo.video_price);

	char szMadultYn[2+1];
	memset(szMadultYn, 0x00, sizeof(szMadultYn));

	bool bIsChange = false;

	char szApplyYn[2+1];
	memset(szApplyYn, 0x00, sizeof(szApplyYn));

	unsigned long ulListId = 0;


	if(strlen(MurekaVInfo.video_right_content_id) <= 0)
	{
		strcpyA(ErrMsg, "관리코드 없음.");
		infLOG(ERROR, "fups40061->FlogInsertCprData (%s)\n", ErrMsg);
		return -1;
	}

	if(strlen(MurekaVInfo.video_right_name) <= 0)
	{
		strcpyA(ErrMsg, "권리사 정보 없음.");
		infLOG(ERROR, "fups40061->FlogInsertCprData (%s)\n", ErrMsg);
		return -1;
	}

	if(strlen(MurekaVInfo.video_cha) <= 0)
	{
		strcpyA(ErrMsg, "회차 정보 없음.");
		infLOG(ERROR, "fups40061->FlogInsertCprData (%s)\n", ErrMsg);
		return -1;
	}

	char szPaymentRate[6+1];
	memset(szPaymentRate, 0x00, sizeof(szPaymentRate));

	double dOspPaymentRate = atof(MurekaVInfo.video_osp_jibun);
	double dPaymentRate = 100 - dOspPaymentRate;

	sprintf(szPaymentRate, "%.2f", dPaymentRate);

	#ifdef __DEBUG
	printf("FlogInsertCprData START !!\n ");
	printf("FlogInsertCprData> szDefaultHash = [%s]\n", szDefaultHash);
	printf("FlogInsertCprData> szFileName = [%s]\n", szFileName);
	printf("FlogInsertCprData> dwFileSize = [%.0f]\n", dwFileSize);
	printf("FlogInsertCprData> MurekaVInfo.video_title = [%s]\n", MurekaVInfo.video_title);
	printf("FlogInsertCprData> MurekaVInfo.video_right_content_id = [%s]\n", MurekaVInfo.video_right_content_id);
	printf("FlogInsertCprData> MurekaVInfo.video_osp_jibun = [%s]\n", MurekaVInfo.video_osp_jibun);
	printf("FlogInsertCprData> szPaymentRate = [%s]\n", szPaymentRate);
	#endif

	strcpyA(szQuery, "SELECT date_format(now(),'%Y%m%d')");
	strcat(szQuery, "     , date_format(now(),'%H%i%s')");

	if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		strcpyA(ErrMsg, "등록일자 조회 오류.\n");
		infLOG(ERROR, "%s",ErrMsg);
		infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		return -1;
	}

	if (!(cpr_res2 = mysql_store_result(cpr_con)))
	{
		strcpyA(ErrMsg, "등록일자 데이터 생성 오류.\n");
		infLOG(ERROR, "%s",ErrMsg);
		infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -1;
	}

	if (mysql_num_rows(cpr_res2) == 0 )
 	{
		strcpyA(ErrMsg, "등록일자 오류.\n");
		infLOG(ERROR, "%s",ErrMsg);
		infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -1;
	}

	cpr_row2 = mysql_fetch_row(cpr_res2);

	strcpyA(reg_date     , getstr(cpr_row2, 0));
	strcpyA(reg_time     , getstr(cpr_row2, 1));

	mysql_free_result(cpr_res2);

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "select comp_cd from zangsi_cpr.T_CPR_COMP_INFO where comp_nm = '%s'", MurekaVInfo.video_right_name);

	#ifdef _DEBUG
	printf("FlogInsertCprData> szQuery=[%s]\n", szQuery);
	#endif

	if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		sprintf(ErrMsg, "권리자 조회 오류(%s).\n", MurekaVInfo.video_right_name);
		infLOG(ERROR, "%s",ErrMsg);
		infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		return -1;
	}

	if (!(cpr_res2 = mysql_store_result(cpr_con)))
	{
		sprintf(ErrMsg, "권리자 데이터 생성 오류(%s).\n", MurekaVInfo.video_right_name);
		infLOG(ERROR, "%s",ErrMsg);
		infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -1;
	}

	if (mysql_num_rows(cpr_res2) == 0 )
 	{
		sprintf(ErrMsg, "권리자 없음.\nszQuery=[%s]\n", szQuery);
		infLOG(ERROR, "%s",ErrMsg);
		infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -1;
	}

	cpr_row2 = mysql_fetch_row(cpr_res2);

	strcpyA(szCompCd, getstr(cpr_row2, 0));

	mysql_free_result(cpr_res2);

	//20100630 수정 - LEE
	/*
	if(strcmp(szCompCd, "010022") == 0
		|| strcmp(szCompCd, "010021") == 0)//no.677

	{
		nMPriceAmt = atoi(MurekaVInfo.video_price)/10;
		if( nMPriceAmt < 50 )
			nMPriceAmt = 50;
	}
	*/
	/*
	방송 컨텐츠일 경우 가격, 지분율, 권리자 정보가 있는지 체크하여 없다면 일반컨텐츠로 등록(??).
	체크결과가 TRUE이면 정보대로 zangsi_cpr.T_CPR_CONT_LIST에 입력.
	등록시간이 zangsi.T_MINOR_CODE의 미적용 시간대라면은 zangsi_cpr.T_CPR_CONT_LIST의 apply_yn 를 'P'로 입력
	*/


/*
	if(strcmp(szCompCd, "010022") == 0 //MBC
		|| strcmp(szCompCd, "010021") == 0 //SBS
		|| strcmp(szCompCd, "010020") == 0 //KBS
		|| strcmp(szCompCd, "010033") == 0 //20100804. EBS
		|| strcmp(szCompCd, "010037") == 0 //no.908
		|| strcmp(szCompCd, "010032") == 0
		|| strcmp(szCompCd, "010030") == 0 //no.1098
		|| strcmp(szCompCd, "010038") == 0 //no.1116
		|| strcmp(szCompCd, "010036") == 0 //no.1141
		|| strcmp(szCompCd, "010017") == 0 //no.1141
		)
	{
		*/
		#ifdef _DEBUG
		printf("FlogInsertCprData> MurekaVInfo.video_right_content_id[%s]\n", MurekaVInfo.video_right_content_id);
		printf("FlogInsertCprData> nMPriceAmt[%d]\n", nMPriceAmt);
		printf("FlogInsertCprData> MurekaVInfo.video_right_name[%s]\n", MurekaVInfo.video_right_name);
		printf("FlogInsertCprData> MurekaVInfo.video_osp_jibun[%s]\n", MurekaVInfo.video_osp_jibun);
		#endif
		if(nMPriceAmt <= 0 || strlen(MurekaVInfo.video_right_name) <= 0 || strlen(szPaymentRate) <= 0)
		{
			strcpyA(ErrMsg, "메타 정보 없음.\n");
			infLOG(ERROR, "fups40061->FlogInsertCprData (%s)\n", ErrMsg);
			return -1;
		}

		//////////////no.875/////////////////

		if(strcmp(szCompCd, "010022") == 0) //MBC
			nResult = 5;
		else if(strcmp(szCompCd, "010020") == 0) //KBS
			nResult = 6;
		else if(strcmp(szCompCd, "010021") == 0) //SBS
			nResult = 7;
		else if(strcmp(szCompCd, "010033") == 0)//20100804. EBS
			nResult = 8;
		else if(strcmp(szCompCd, "010032") == 0)//20110217. KBSI
			nResult = 9;
		else if(strcmp(szCompCd, "010030") == 0)//no.1098
			nResult = 10;
		else if(strcmp(szCompCd, "010038") == 0)//no.1116
			nResult = 11;
		else if(strcmp(szCompCd, "010036") == 0)//no.1141
			nResult = 12;
		else if ( strcmp(MurekaVInfo.video_right_content_id , "JAYE_MBN_DM_1_2_001") == 0  || strcmp(MurekaVInfo.video_right_content_id , "JAYE_MBN_DM_1_2_002" ) == 0 )//20120618
			nResult = 13;

		if(strcmp(MurekaVInfo.video_osp_etc, "NOTLOG") == 0)
		{
			strcpy(szApplyYn, "P");
		}
		else
			strcpy(szApplyYn, "Y");


		memset(szQuery, 0x00, sizeof(szQuery));
		//sprintf(szQuery, "select list_id, apply_yn from zangsi_cpr.T_CPR_CONT_LIST where mgr_cd = '%s' and comp_cd = '%s' and chapter = %s"
		sprintf(szQuery, "select list_id, apply_yn ,comp_cd  , price_amt from zangsi_cpr.T_CPR_CONT_LIST where mgr_cd = '%s' and chapter = %s"
					   , MurekaVInfo.video_right_content_id
					   //, szCompCd
					   , MurekaVInfo.video_cha);

		#ifdef _DEBUG_
		printf("FlogInsertCprData> szQuery=[%s]\n\n", szQuery);
		#endif

		if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
		{
			sprintf(ErrMsg, "관리코드 조회 오류.\nszQuery=[%s]\n", szQuery);
			infLOG(ERROR, "%s",ErrMsg);
			infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
			return -1;
		}

		if (!(cpr_res2 = mysql_store_result(cpr_con)))
		{
			sprintf(ErrMsg, "관리코드 데이터 생성 오류.\nszQuery=[%s]\n", szQuery);
			infLOG(ERROR, "%s",ErrMsg);
			infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
			mysql_free_result(cpr_res2);
			return -1;
		}

		//제휴 목록이 없을 시에 T_CPR_CONT_LIST 추가 하기 -
		if (mysql_num_rows(cpr_res2) == 0 )
	 	{
			mysql_free_result(cpr_res2);

			if(strcmp(MurekaVInfo.video_grade, "18") == 0)
				strcpy(szMadultYn, "Y");
			else
				strcpy(szMadultYn, "N");

			//20120522
			{

				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " insert into zangsi_cpr.T_CPR_CONT_LIST "
								 " (seq_no , title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, open_date, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate) "
								 " values "
								 " ( 1 ,  '%s', %s,'%s','%s','%s','%s',%d,'%s', '%s',  'sys40061','%s','%s','%s','%s')  "
								 , MurekaVInfo.video_title
								 , MurekaVInfo.video_cha
								 , MurekaVInfo.video_right_content_id
								 , szCompCd
								 , pSectCode
								 , pSectSub
								 , nMPriceAmt
								 , szMadultYn
								 , MurekaVInfo.video_onair_date
								 , reg_date
								 , reg_time
								 , szApplyYn
								 , szPaymentRate);

		 		#ifdef _DEBUG_
		 		printf("FlogInsertCprData> szQuery=[%s].\n\n", szQuery);
		 		#endif

				if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
				{
					strcpyA(ErrMsg, "관리코드 입력 오류.\n");
					infLOG(ERROR, "%s",ErrMsg);
					infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
					return -1;
				}



			}


			ulListId = mysql_insert_id(cpr_con);

			//20120522
			memset(szQuery, 0x00, sizeof(szQuery));
			/*
			sprintf(szQuery, " insert into zangsi_cpr.T_CPR_CONT_LIST_HIST "
							 " (seq_no , title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, open_date, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate) "
							 " values "
							 " ( 1 ,  '%s', %s,   '%s',    '%s',      '%s',     '%s',        %d,     '%s', '%s',  'sys40061',     '%s',     '%s',     '%s',             '%s')  "
							 , MurekaVInfo.video_title
							 , MurekaVInfo.video_cha
							 , MurekaVInfo.video_right_content_id
							 , szCompCd
							 , pSectCode
							 , pSectSub
							 , nMPriceAmt
							 , szMadultYn
							 , MurekaVInfo.video_onair_date
							 , reg_date
							 , reg_time
							 , szApplyYn
							 , szPaymentRate);
			*/
							 //20120522
			sprintf(szQuery, " insert into zangsi_cpr.T_CPR_CONT_LIST_HIST "
							 " (list_id ,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate "
							 " , udt_user , udt_date,udt_time ) "
							 " select list_id ,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate   "
							 " , 'sys40061' , date_format(now(), '%%Y%%m%%d'),  date_format(now(), '%%H%%i%%s')  "
							 " from zangsi_cpr.T_CPR_CONT_LIST where list_id = %lu "
							 , ulListId);



	 		#ifdef _DEBUG_
	 		printf("FlogInsertCprData> szQuery=[%s].\n\n", szQuery);
	 		#endif

			if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				strcpyA(ErrMsg, "관리코드 입력 오류.\n");
				infLOG(ERROR, "%s",ErrMsg);
				infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
				return -1;
			}

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi_cpr.T_CPR_CONT_LIST_WAIT "
							 " (list_id, title, chapter, mgr_cd, comp_cd, wait_time, reg_user, reg_date, reg_time, proc_stat) "
							 " select "
							 " %lu, '%s', %s, '%s', '%s', d.minor_name, 'sys40061', '%s', '%s', b.minor_name  "
							 " from zangsi_cpr.T_MINOR_CODE a, zangsi_cpr.T_MINOR_CODE b "
							 " , zangsi_cpr.T_MINOR_CODE c, zangsi_cpr.T_MINOR_CODE d  "
							 " where a.minor_code = b.minor_code and c.minor_code = d.minor_code and "
							 " a.major_code = '91' and b.major_code = '92' and "
							 " c.major_code = '91' and d.major_code = '90' and "
							 " a.minor_name = '%s' and c.minor_name = '%s' "
							 , ulListId
							 , MurekaVInfo.video_title
							 , MurekaVInfo.video_cha
							 , MurekaVInfo.video_right_content_id
							 , szCompCd
							 , reg_date
							 , reg_time
							 , szCompCd
							 , szCompCd);

	 		#ifdef _DEBUG_
	 		printf("FlogInsertCprData> szQuery=[%s].\n\n", szQuery);
	 		#endif

			if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				strcpyA(ErrMsg, "관리코드 입력 오류.\n");
				infLOG(ERROR, "%s\n%s\n",ErrMsg, szQuery);
				infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
				return -1;
			}
/*
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi.T_CPR_CONT_LIST "
							 " (title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate) "
							 " values "
							 " ( '%s', %s,'%s','%s','%s','%s',%d,'%s',  'sys40061','%s','%s','%s','%s')  "
							 , MurekaVInfo.video_title
							 , MurekaVInfo.video_cha
							 , MurekaVInfo.video_right_content_id
							 , szCompCd
							 , pSectCode
							 , pSectSub
							 , nMPriceAmt
							 , szMadultYn
							 , reg_date
							 , reg_time
							 , szApplyYn
							 , szPaymentRate);
*/
	 		#ifdef _DEBUG_
	 		printf("FlogInsertCprData> szQuery=[%s].\n\n", szQuery);
	 		#endif

			if(MysqlCon.MysqlQuery(con, szSysErrMsg, szQuery))
			{
				strcpyA(ErrMsg, "main db 관리코드 입력 오류.\n");
				infLOG(ERROR, "%s",ErrMsg);
				infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(con), mysql_error(con));
				return -1;
			}

			if(nResult >= 5 && strcmp(szApplyYn, "P") == 0)
			{
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " replace into zangsi_cpr.T_CPR_MUREKA_WAIT_TEMP "
								 " ( list_id, mgr_cd, cont_gu, id, reg_date, reg_time, default_hash, file_size, file_name)"
								 " values "
								 " (%lu, '%s', 'FD', %lu, '%s', '%s', '%s', %15.0f, '%s') "
								 , ulListId
								 , MurekaVInfo.video_right_content_id
								 , ulID
								 , reg_date
								 , reg_time
								 , szDefaultHash
								 , dwFileSize
								 , szFileName);
		 		#ifdef _DEBUG_
		 		printf("FlogInsertCprData> szQuery=[%s].\n\n", szQuery);
		 		#endif

				if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
				{
					strcpyA(ErrMsg, "T_CPR_MUREKA_WAIT_TEMP 입력 오류.\n");
					infLOG(ERROR, "%s",ErrMsg);
					infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
					return -1;
				}
			}

		}
		else
		{
			cpr_row2 = mysql_fetch_row(cpr_res2);

			ulListId = (unsigned long)getnum(cpr_row2, 0);

			char szListApplyYn[2+1];	memset(szListApplyYn, 0x00, sizeof(szListApplyYn));
			char szOldCompCd[6+1];	memset(szOldCompCd, 0x00, sizeof(szOldCompCd));
			int nOldPriceAmt = 0;

			strcpyA(szListApplyYn, getstr(cpr_row2, 1));
			strcpyA(szOldCompCd,getstr(cpr_row2,2));
			nOldPriceAmt = getint(cpr_row2,3);

			mysql_free_result(cpr_res2);

			if(strcmp(szListApplyYn, szApplyYn) != 0 || strcmp(szCompCd,szOldCompCd) != 0 )//|| nMPriceAmt != nOldPriceAmt )
			{
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " update zangsi_cpr.T_CPR_CONT_LIST set apply_yn = '%s'  , szCompCd ='%s' , seq_no = seq_no + 1  where list_id = %lu"
							   , szApplyYn
							   , szCompCd
							   , ulListId);
				/*
				sprintf(szQuery, " update zangsi_cpr.T_CPR_CONT_LIST set apply_yn = '%s' , szCompCd ='%s' , price_amt = %d ,  seq_no = seq_no + 1  where list_id = %lu"
							   , szApplyYn
							   , szCompCd
							   , nMPriceAmt
							   , ulListId);
				*/
		 		#ifdef _DEBUG_
		 		printf("FlogInsertCprData> szQuery=[%s].\n\n", szQuery);
		 		#endif

				if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
				{
					strcpyA(ErrMsg, "관리코드 업데이트 오류.\n");
					infLOG(ERROR, "%s",ErrMsg);
					infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
					return -1;
				}

				//20120522
				sprintf(szQuery, " insert into zangsi_cpr.T_CPR_CONT_LIST_HIST "
								 " (list_id ,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate "
								 " , udt_user , udt_date,udt_time ) "
								 " select list_id ,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate   "
								 " , 'sys40061' , date_format(now(), '%%Y%%m%%d'),  date_format(now(), '%%H%%i%%s') "
								 " from zangsi_cpr.T_CPR_CONT_LIST where list_id = %lu "
								 , ulListId);



				if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
				{
					strcpyA(ErrMsg, "관리코드 업데이트 오류.\n");
					infLOG(ERROR, "%s",ErrMsg);
					infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
					return -1;
				}
/*
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " update zangsi.T_CPR_CONT_LIST set apply_yn = '%s' , szCompCd ='%s' where list_id = %lu"
							   , szApplyYn
							   , szCompCd
							   ,ulListId);
*/
		 		#ifdef _DEBUG_
		 		printf("FlogInsertCprData> szQuery=[%s].\n\n", szQuery);
		 		#endif

				if(MysqlCon.MysqlQuery(con, szSysErrMsg, szQuery))
				{
					strcpyA(ErrMsg, "main DB 관리코드 업데이트 오류.\n");
					infLOG(ERROR, "%s",ErrMsg);
					infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(con), mysql_error(con));
					return -1;
				}

			}
			if(nResult >= 5 && strcmp(szApplyYn, "P") == 0)
			{
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " replace into zangsi_cpr.T_CPR_MUREKA_WAIT_TEMP "
								 " ( list_id, mgr_cd, cont_gu, id, reg_date, reg_time, default_hash, file_size, file_name)"
								 " values "
								 " (%lu, '%s', 'FD', %lu, '%s', '%s', '%s', %15.0f, '%s') "
								 , ulListId
								 , MurekaVInfo.video_right_content_id
								 , ulID
								 , reg_date
								 , reg_time
								 , szDefaultHash
								 , dwFileSize
								 , szFileName);
		 		#ifdef _DEBUG_
		 		printf("FlogInsertCprData> szQuery=[%s].\n\n", szQuery);
		 		#endif

				if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
				{
					strcpyA(ErrMsg, "T_CPR_MUREKA_WAIT_TEMP 입력 오류.\n");
					infLOG(ERROR, "%s",ErrMsg);
					infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
					return -1;
				}
			}
		}
	//}

	if(strcmp(MurekaVInfo.video_osp_etc, "NOTLOG") == 0)
		return nResult;

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select chi_id, title, cha from zangsi_cpr.T_CONT_CPR_MAP_TEMP where id = %lu and cont_gu = 'FD' ", ulID);

	#ifdef __DEBUG
	printf("FlogInsertCprData> query = [%s]\n", szQuery);
	#endif

	if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		strcpyA(ErrMsg, "T_CONT_CPR_MAP_TEMP 조회 오류.\n");
		infLOG(ERROR, "%s",ErrMsg);
		infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		return -1;
	}
	if (!(cpr_res2 = mysql_store_result(cpr_con)))
	{
		strcpyA(ErrMsg, "T_CONT_CPR_MAP_TEMP 데이터 생성 오류.\n");
		infLOG(ERROR, "%s",ErrMsg);
		infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -1;
	}
	if (mysql_num_rows(cpr_res2) <= 0 )
	{
		#ifdef __DEBUG
		printf("fups40061->FlogInsertCprData : 첫번째 파일 [%lu]\n", ulID);
		#endif
		mysql_free_result(cpr_res2);


		/*
		2009/09/25 T_CPR_HASH_INFO, FILE 조회하여 없으면 인서트후 T_CONT_CPR_MAP_TEMP에도 인서트
		*/

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " select a.chi_id"
						 " from zangsi_cpr.T_CPR_HASH_INFO a, zangsi_cpr.T_CPR_HASH_FILE b "
						 " where a.chi_id = b.chi_id and b.file_size = %15.0f and b.default_hash = '%s' "
						 " group by a.chi_id limit 1"
						 , dwFileSize
						 , szDefaultHash);

		#ifdef __DEBUG
		printf("FlogInsertCprData> query = [%s]\n", szQuery);
		#endif

		if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
		{
			strcpyA(ErrMsg, "해시 정보 조회 오류.\n");
			infLOG(ERROR, "%s",ErrMsg);
			infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
			return -1;
		}
		if (!(cpr_res2 = mysql_store_result(cpr_con)))
		{
			strcpyA(ErrMsg, "해시 정보 데이터 생성 오류.\n");
			infLOG(ERROR, "%s",ErrMsg);
			infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
			return -1;
		}
		if(mysql_num_rows(cpr_res2) <= 0)
		{//해시정보가 없으므로 인서트
			mysql_free_result(cpr_res2);

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi_cpr.T_CPR_HASH_INFO  "
							 " (title, comp_cd, list_id, sect_code, sect_sub, adult_yn, price_amt, proc_stat, reg_user, cpr_payment_rate, reg_date, reg_time) "
							 " select  "
							 " title , comp_cd , list_id, if(sect_code = 'FD', '%s', sect_code), if(sect_sub = 'FD', '%s', sect_sub), adult_yn, price_amt, 'C'      , 'sys40061', cpr_payment_rate, '%s'    , '%s'  "
							 " from zangsi_cpr.T_CPR_CONT_LIST where "
							 //"  mgr_cd = '%s' and comp_cd = '%s' and chapter = %s and apply_yn = 'Y' "
							 "  list_id = %lu and apply_yn = 'Y' "
							 , pSectCode
							 , pSectSub
							 , reg_date
							 , reg_time
							 , ulListId );
							 /*
							 , MurekaVInfo.video_right_content_id
							 , szCompCd
							 , MurekaVInfo.video_cha);
							 */
			#ifdef __DEBUG
			printf("FlogInsertCprData> query = [%s]\n", szQuery);
			#endif


			if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				strcpyA(ErrMsg, "해시 정보 인서트 오류.\n");
				infLOG(ERROR, "%s",ErrMsg);
				infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
				return -1;
			}

			unsigned long ulChiId = 0;
			ulChiId = mysql_insert_id(cpr_con);

			#ifdef __DEBUG
			printf("FlogInsertCprData> dwId = [%lu]\n", ulChiId);
			#endif

			if(ulChiId == 0)
			{
				if(nResult >= 5 && strcmp(szApplyYn, "P") == 0)
				{
					memset(szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, " replace into zangsi_cpr.T_CPR_MUREKA_WAIT_TEMP "
									 " ( list_id, mgr_cd, cont_gu, id, reg_date, reg_time, default_hash, file_size, file_name)"
									 " values "
									 " (%lu, '%s', 'FD', %lu, '%s', '%s', '%s', %15.0f, '%s') "
									 , ulListId
									 , MurekaVInfo.video_right_content_id
									 , ulID
									 , reg_date
									 , reg_time
									 , szDefaultHash
									 , dwFileSize
									 , szFileName);

			 		#ifdef _DEBUG_
			 		printf("FlogInsertCprData> szQuery=[%s].\n\n", szQuery);
			 		#endif

					if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
					{
						strcpyA(ErrMsg, "T_CPR_MUREKA_WAIT_TEMP 입력 오류.\n");
						infLOG(ERROR, "%s",ErrMsg);
						infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
						return -1;
					}
				}

				sprintf(ErrMsg, "적용된 해시없음[%s].\n", MurekaVInfo.video_right_content_id);
				infLOG(ALWAY, "fups40061->%s",ErrMsg);
				return 0;
			}


			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi_cpr.T_CPR_HASH_FILE "
							 " (chi_id, comp_cd, filter_yn, video_yn, default_hash, video_hash, file_name, file_size, reg_date, reg_time) "
							 " select "
							 " %lu    , comp_cd , 'Y'     , 'Y'     , '%s'        , '%s'      , '%s'     , %15.0f , '%s'    , '%s' "
							 " from zangsi_cpr.T_CPR_CONT_LIST where "
							 //" mgr_cd = '%s' and comp_cd = '%s' and chapter = %s and apply_yn = 'Y' "
							 " list_id  = %lu and apply_yn = 'Y' "
							 , ulChiId
							 , szDefaultHash
							 , MurekaVInfo.mureka_hash
							 , szFileName
							 , dwFileSize
							 , reg_date
							 , reg_time
							 , ulListId );
							 /*
							 , MurekaVInfo.video_right_content_id
							 , szCompCd
							 , MurekaVInfo.video_cha);
							 */
			#ifdef __DEBUG
			printf("FlogInsertCprData> query = [%s]\n", szQuery);
			#endif
			if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				strcpyA(ErrMsg, "해시 파일정보 인서트 오류.\n");
				infLOG(ERROR, "%s",ErrMsg);
				infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
				return -1;
			}
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi_cpr.T_CPR_PRICE_HIST "
							 " (chi_id , comp_cd  , mgr_cd , apply_date, apply_time, price_amt  , cpr_payment_rate  , cpr_desc          , reg_user, reg_date, reg_time) "
							 " select  "
							 " b.chi_id, a.comp_cd, a.mgr_cd, '%s'     , '%s'      , a.price_amt, a.cpr_payment_rate, 'mureka filtering', 'sys40061', '%s'    , '%s' "
							 " from zangsi_cpr.T_CPR_CONT_LIST a, zangsi_cpr.T_CPR_HASH_INFO b "
							 " where a.list_id = b.list_id and b.chi_id = %lu "
							 , reg_date
							 , reg_time
							 , reg_date
							 , reg_time
							 , ulChiId);

			#ifdef __DEBUG
			printf("FlogInsertCprData> query = [%s]\n", szQuery);
			#endif
			if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				strcpyA(ErrMsg, "해시 히스토리정보 인서트 오류.\n");
				infLOG(ERROR, "%s",ErrMsg);
				infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
				return -1;
			}
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "insert into zangsi_cpr.T_CONT_CPR_MAP_TEMP (id, chi_id, cont_gu, title, cha) "
							 " values "
							 " (%lu, %lu, 'FD', '%s', '%s')"
							 ,ulID, ulChiId, MurekaVInfo.video_title, MurekaVInfo.video_cha);

			#ifdef __DEBUG
			printf("FlogInsertCprData> query = [%s]\n", szQuery);
			#endif
			if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				strcpyA(ErrMsg, "T_CONT_CPR_MAP_TEMP 인서트 오류.\n");
				infLOG(ERROR, "%s",ErrMsg);
				infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
				return -1;
			}
		}
		else
		{
			mysql_free_result(cpr_res2);
		}
	}
	else
	{//T_CONT_CPR_MAP_TEMP에 데이터 있음.
		cpr_row2 = mysql_fetch_row(cpr_res2);

		unsigned long ulChiId = 0;
		ulChiId = (unsigned long)getnum(cpr_row2, 0);

		mysql_free_result(cpr_res2);

		char szTitle[5000];
		memset(szTitle, 0x00, sizeof(szTitle));
		strcpyA(szTitle, getstr(cpr_row2, 1));

		char szCha[10+1];
		memset(szCha, 0x00, sizeof(szCha));
		sprintf(szCha, "%s", getstr(cpr_row2, 2));

		if(strcmp(szTitle, MurekaVInfo.video_title) == 0 && strcmp(szCha, MurekaVInfo.video_cha) == 0)
		{//같은 제휴 컨텐츠 T_CPR_HASH_FILE 만 인서트
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " select a.chi_id"
							 " from zangsi_cpr.T_CPR_HASH_INFO a, zangsi_cpr.T_CPR_HASH_FILE b "
							 " where a.chi_id = b.chi_id and b.file_size = %15.0f and b.default_hash = '%s' "
							 " group by a.chi_id limit 1"
							 , dwFileSize
							 , szDefaultHash);

			#ifdef __DEBUG
			printf("FlogInsertCprData> query = [%s]\n", szQuery);
			#endif

			if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				strcpyA(ErrMsg, "해시 정보 조회 오류.\n");
				infLOG(ERROR, "%s",ErrMsg);
				infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
				return -1;
			}
			if (!(cpr_res2 = mysql_store_result(cpr_con)))
			{
				strcpyA(ErrMsg, "해시 정보 데이터 생성 오류.\n");
				infLOG(ERROR, "%s",ErrMsg);
				infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
				return -1;
			}
			if(mysql_num_rows(cpr_res2) <= 0)
			{//해시정보가 없으므로 인서트
				mysql_free_result(cpr_res2);

				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " insert into zangsi_cpr.T_CPR_HASH_FILE "
								 " (chi_id, comp_cd, filter_yn, video_yn, default_hash, video_hash, file_name, file_size, reg_date, reg_time) "
								 " select "
								 " %lu    , comp_cd , 'Y'     , 'Y'     , '%s'        , '%s'      , '%s'     , %15.0f , '%s'    , '%s' "
								 " from zangsi_cpr.T_CPR_CONT_LIST where "
								 //" mgr_cd = '%s' and comp_cd = '%s' and chapter = %s and apply_yn = 'Y' "
								 " list_id  = %lu and apply_yn = 'Y' "
								 , ulChiId
								 , szDefaultHash
								 , MurekaVInfo.mureka_hash
								 , szFileName
								 , dwFileSize
								 , reg_date
								 , reg_time
								 , ulListId );
								 /*
								 , MurekaVInfo.video_right_content_id
								 , szCompCd
								 , MurekaVInfo.video_cha);
								 */
				#ifdef __DEBUG
				printf("FlogInsertCprData> query = [%s]\n", szQuery);
				#endif
				if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
				{
					strcpyA(ErrMsg, "해시 파일정보 인서트 오류.\n");
					infLOG(ERROR, "%s",ErrMsg);
					infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
					mysql_free_result(cpr_res2);
					return -1;
				}
			}
			else
			{
				mysql_free_result(cpr_res2);
			}
		}
		else
		{
			/*
			2009/09/25 T_CPR_HASH_INFO, FILE 조회하여 없으면 인서트후 T_CONT_CPR_MAP_TEMP에도 인서트
			*/

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " select a.chi_id"
							 " from zangsi_cpr.T_CPR_HASH_INFO a, zangsi_cpr.T_CPR_HASH_FILE b "
							 " where a.chi_id = b.chi_id and b.file_size = %15.0f and b.default_hash = '%s' "
							 " group by a.chi_id limit 1"
							 , dwFileSize
							 , szDefaultHash);

			#ifdef __DEBUG
			printf("FlogInsertCprData> query = [%s]\n", szQuery);
			#endif

			if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				strcpyA(ErrMsg, "해시 정보 조회 오류.\n");
				infLOG(ERROR, "%s",ErrMsg);
				infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
				return -1;
			}
			if (!(cpr_res2 = mysql_store_result(cpr_con)))
			{
				strcpyA(ErrMsg, "해시 정보 데이터 생성 오류.\n");
				infLOG(ERROR, "%s",ErrMsg);
				infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
				return -1;
			}
			if(mysql_num_rows(cpr_res2) <= 0)
			{//해시정보가 없으므로 인서트
				mysql_free_result(cpr_res2);

				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " insert into zangsi_cpr.T_CPR_HASH_INFO  "
								 " (title, comp_cd, list_id, sect_code, sect_sub, adult_yn, price_amt, proc_stat, reg_user, cpr_payment_rate, reg_date, reg_time) "
								 " select  "
								 " title , comp_cd , list_id, if(sect_code = 'FD', '%s', sect_code), if(sect_sub = 'FD', '%s', sect_sub), adult_yn, price_amt, 'C'      , 'sys40061', cpr_payment_rate, '%s'    , '%s'  "
								 " from zangsi_cpr.T_CPR_CONT_LIST where "
								 //" mgr_cd = '%s' and comp_cd = '%s' and chapter = %s and apply_yn = 'Y' "
								 " list_id  = %lu and apply_yn = 'Y' "
								 , pSectCode
								 , pSectSub
								 , reg_date
								 , reg_time
								 , ulListId );
								 /*
								 , MurekaVInfo.video_right_content_id
								 , szCompCd
								 , MurekaVInfo.video_cha);
								 */
				#ifdef __DEBUG
				printf("FlogInsertCprData> query = [%s]\n", szQuery);
				#endif


				if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
				{
					strcpyA(ErrMsg, "해시 정보 인서트 오류.\n");
					infLOG(ERROR, "%s",ErrMsg);
					infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
					return -1;
				}

				unsigned long ulChiId = 0;
				ulChiId = mysql_insert_id(cpr_con);

				#ifdef __DEBUG
				printf("FlogInsertCprData> dwId = [%lu]\n", ulChiId);
				#endif

				if(ulChiId == 0)
				{
					if(nResult >= 5 && strcmp(szApplyYn, "P") == 0)
					{
						memset(szQuery, 0x00, sizeof(szQuery));
						sprintf(szQuery, " replace into zangsi_cpr.T_CPR_MUREKA_WAIT_TEMP "
										 " ( list_id, mgr_cd, cont_gu, id, reg_date, reg_time, default_hash, file_size, file_name)"
										 " values "
										 " (%lu, '%s', 'FD', %lu, '%s', '%s', '%s', %15.0f, '%s') "
										 , ulListId
										 , MurekaVInfo.video_right_content_id
										 , ulID
										 , reg_date
										 , reg_time
										 , szDefaultHash
										 , dwFileSize
										 , szFileName);

				 		#ifdef _DEBUG_
				 		printf("FlogInsertCprData> szQuery=[%s].\n\n", szQuery);
				 		#endif

						if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
						{
							strcpyA(ErrMsg, "T_CPR_MUREKA_WAIT_TEMP 입력 오류.\n");
							infLOG(ERROR, "%s",ErrMsg);
							infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
							return -1;
						}
					}
					sprintf(ErrMsg, "적용된 해시없음[%s].\n", MurekaVInfo.video_right_content_id);
					infLOG(ALWAY, "fups40061->%s",ErrMsg);
					return 0;
				}


				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " insert into zangsi_cpr.T_CPR_HASH_FILE "
								 " (chi_id, comp_cd, filter_yn, video_yn, default_hash, video_hash, file_name, file_size, reg_date, reg_time) "
								 " select "
								 " %lu    , comp_cd , 'Y'     , 'Y'     , '%s'        , '%s'      , '%s'     , %15.0f , '%s'    , '%s' "
								 " from zangsi_cpr.T_CPR_CONT_LIST where "
								 //" mgr_cd = '%s' and comp_cd = '%s' and chapter = %s and apply_yn = 'Y' "
								 " list_id  = %lu and apply_yn = 'Y' "
								 , ulChiId
								 , szDefaultHash
								 , MurekaVInfo.mureka_hash
								 , szFileName
								 , dwFileSize
								 , reg_date
								 , reg_time
								 , ulListId );
								 /*
								 , MurekaVInfo.video_right_content_id
								 , szCompCd
								 , MurekaVInfo.video_cha);
								 */
				#ifdef __DEBUG
				printf("FlogInsertCprData> query = [%s]\n", szQuery);
				#endif
				if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
				{
					strcpyA(ErrMsg, "해시 파일정보 인서트 오류.\n");
					infLOG(ERROR, "%s",ErrMsg);
					infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
					return -1;
				}
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " insert into zangsi_cpr.T_CPR_PRICE_HIST "
								 " (chi_id , comp_cd  , mgr_cd , apply_date, apply_time, price_amt  , cpr_payment_rate  , cpr_desc          , reg_user, reg_date, reg_time) "
								 " select  "
								 " b.chi_id, a.comp_cd, a.mgr_cd, '%s'     , '%s'      , a.price_amt, a.cpr_payment_rate, 'mureka filtering', 'sys40061', '%s'    , '%s' "
								 " from zangsi_cpr.T_CPR_CONT_LIST a, zangsi_cpr.T_CPR_HASH_INFO b "
								 " where a.list_id = b.list_id and b.chi_id = %lu "
								 , reg_date
								 , reg_time
								 , reg_date
								 , reg_time
								 , ulChiId);

				#ifdef __DEBUG
				printf("FlogInsertCprData> query = [%s]\n", szQuery);
				#endif
				if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
				{
					strcpyA(ErrMsg, "해시 히스토리정보 인서트 오류.\n");
					infLOG(ERROR, "%s",ErrMsg);
					infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
					return -1;
				}
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, "insert into zangsi_cpr.T_CONT_CPR_MAP_TEMP (id, chi_id, cont_gu, title, cha) "
								 " values "
								 " (%lu, %lu, 'FD', '%s', '%s')"
								 ,ulID, ulChiId, MurekaVInfo.video_title, MurekaVInfo.video_cha);

				#ifdef __DEBUG
				printf("FlogInsertCprData> query = [%s]\n", szQuery);
				#endif
				if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
				{
					strcpyA(ErrMsg, "T_CONT_CPR_MAP_TEMP 인서트 오류.\n");
					infLOG(ERROR, "%s",ErrMsg);
					infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
					return -1;
				}

			}
			else
			{
				mysql_free_result(cpr_res2);
			}
		}
	}

	#ifdef __DEBUG
	printf("\n\n\n\n\ndcmdfups40061 InsertCprData() 완료\n\n\n\n");
	#endif

	return nResult;

}

int FlogUpdateCprData(MYSQL *cpr_con, CMysqlCon MysqlCprCon, char* szDefaultHash, char* szFileName, double dwFileSize, MUREKA_VINFO MurekaVInfo)
{
	MYSQL_RES   *cpr_res2;
	MYSQL_ROW    cpr_row2;

	char ErrMsg[256];    // error message
	memset(ErrMsg   , 0x00, sizeof(ErrMsg   )); // 오류메시지

	char szSysErrMsg[255];
	memset(szSysErrMsg,0x00,sizeof(szSysErrMsg));

	if(strlen(MurekaVInfo.video_right_content_id) <= 0)
	{
		strcpyA(ErrMsg, "관리코드 없음.\n");
		infLOG(ERROR, "fups40061->UpdateCprData (%s)\n", ErrMsg);
		return -1;
	}

	if(strlen(MurekaVInfo.video_right_name) <= 0)
	{
		strcpyA(ErrMsg, "권리사 정보 없음.");
		infLOG(ERROR, "fups40061->UpdateCprData (%s)\n", ErrMsg);
		return -1;
	}

	if(strlen(MurekaVInfo.video_cha) <= 0)
	{
		strcpyA(ErrMsg, "회차 정보 없음.");
		infLOG(ERROR, "fups40061->UpdateCprData (%s)\n", ErrMsg);
		return -1;
	}


	char szQuery[15000];
	memset(szQuery, 0x00, sizeof(szQuery));

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "select comp_cd from zangsi_cpr.T_CPR_COMP_INFO where comp_nm = '%s'", MurekaVInfo.video_right_name);

	#ifdef __DEBUG
	printf("UpdateCprData> query = [%s]\n", szQuery);
	#endif

	if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		strcpyA(ErrMsg, "T_CPR_CONT_LIST 조회 오류.\n");
		infLOG(ERROR, "%s",ErrMsg);
		infLOG(ERROR, "fups40061->UpdateCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		return -1;
	}
	if (!(cpr_res2 = mysql_store_result(cpr_con)))
	{
		strcpyA(ErrMsg, "T_CPR_CONT_LIST 데이터 생성 오류.\n");
		infLOG(ERROR, "%s",ErrMsg);
		infLOG(ERROR, "fups40061->UpdateCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -1;
	}
	if (mysql_num_rows(cpr_res2) <= 0 )
	{
		infLOG(ERROR, "T_CPR_CONT_LIST 회사코드 없음.[%s]\n", MurekaVInfo.video_right_name);
		infLOG(ERROR, "fups40061->UpdateCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -1;
	}
	cpr_row2 = mysql_fetch_row(cpr_res2);

	char szCompCd[6+1];
	memset(szCompCd, 0x00, sizeof(szCompCd));

	strcpyA(szCompCd, getstr(cpr_row2, 0));

	mysql_free_result(cpr_res2);

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select list_id,  apply_yn from zangsi_cpr.T_CPR_CONT_LIST where mgr_cd = '%s' and comp_cd = '%s' and chapter = %s "
					 , MurekaVInfo.video_right_content_id, szCompCd, MurekaVInfo.video_cha);

	#ifdef __DEBUG
	printf("UpdateCprData> query = [%s]\n", szQuery);
	#endif

	if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		strcpyA(ErrMsg, "T_CPR_CONT_LIST 조회 오류.\n");
		infLOG(ERROR, "%s",ErrMsg);
		infLOG(ERROR, "fups40061->UpdateCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		return -1;
	}
	if (!(cpr_res2 = mysql_store_result(cpr_con)))
	{
		strcpyA(ErrMsg, "T_CPR_CONT_LIST 데이터 생성 오류.\n");
		infLOG(ERROR, "%s",ErrMsg);
		infLOG(ERROR, "fups40061->UpdateCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -1;
	}
	if (mysql_num_rows(cpr_res2) <= 0 )
	{
		infLOG(ERROR, "T_CPR_CONT_LIST 관리코드 없음.[%s]\n", MurekaVInfo.video_right_content_id);
		infLOG(ERROR, "fups40061->UpdateCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -1;
	}
	cpr_row2 = mysql_fetch_row(cpr_res2);

	unsigned long ulListId = 0;
	ulListId = (unsigned long) getnum(cpr_row2,0);

	char szApplyYn[1+1];
	memset(szApplyYn, 0x00, sizeof(szApplyYn));

	strcpyA(szApplyYn, getstr(cpr_row2, 1));

	mysql_free_result(cpr_res2);

	if(strcmp(szApplyYn, "Y") == 0)
	{//제휴 취소가 안니라면
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update zangsi_cpr.T_CPR_CONT_LIST set apply_yn = 'N' , seq_no = seq_no + 1 where list_id = %lu "
						 , ulListId);

		#ifdef __DEBUG
		printf("FlogInsertCprData> query = [%s]\n", szQuery);
		#endif
		if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
		{
			strcpyA(ErrMsg, "해시 파일정보 인서트 오류.\n");
			infLOG(ERROR, "%s",ErrMsg);
			infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
			return -1;
		}

		//20120522
		sprintf(szQuery, " insert into zangsi_cpr.T_CPR_CONT_LIST_HIST "
						 " (list_id ,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate "
						 " , udt_user , udt_date,udt_time ) "
						 " select list_id ,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate   "
						 " , 'sys40061' , date_format(now(), '%%Y%%m%%d'),  date_format(now(), '%%H%%i%%s') "
						" from zangsi_cpr.T_CPR_CONT_LIST where list_id = %lu "
						 , ulListId);



		if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
		{
			strcpyA(ErrMsg, "관리코드 업데이트 오류.\n");
			infLOG(ERROR, "%s",ErrMsg);
			infLOG(ERROR, "fups40061->FlogInsertCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
			return -1;
		}

	}

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select count(b.chi_id) "
					 " from zangsi_cpr.T_CPR_CONT_LIST a, zangsi_cpr.T_CPR_HASH_INFO b "
					 " where a.list_id = b.list_id and a.list_id = %lu and b.proc_stat != 'N' "
					 " group by a.list_id "
					 , ulListId);

	#ifdef __DEBUG
	printf("UpdateCprData> query = [%s]\n", szQuery);
	#endif

	if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		strcpyA(ErrMsg, "미적용 해시 조회 오류.\n");
		infLOG(ERROR, "%s",ErrMsg);
		infLOG(ERROR, "fups40061->UpdateCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		return -1;
	}
	if (!(cpr_res2 = mysql_store_result(cpr_con)))
	{
		strcpyA(ErrMsg, "미적용 해시 데이터 생성 오류.\n");
		infLOG(ERROR, "%s",ErrMsg);
		infLOG(ERROR, "fups40061->UpdateCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -1;
	}
	if (mysql_num_rows(cpr_res2) <= 0 )
	{
		infLOG(ERROR, "미적용 해시 관리코드 없음.[%s][%lu]\n", MurekaVInfo.video_right_content_id, ulListId);
		infLOG(ERROR, "fups40061->UpdateCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -1;
	}
	cpr_row2 = mysql_fetch_row(cpr_res2);

	int nCount = 0;
	nCount = (int)getint(cpr_row2, 0);

	mysql_free_result(cpr_res2);

	if(nCount > 0)
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update "
						 " zangsi_cpr.T_CPR_CONT_LIST a, zangsi_cpr.T_CPR_HASH_INFO b "
						 " set b.proc_stat = 'N' "
						 " where a.list_id = b.list_id and a.list_id = %lu and b.proc_stat != 'N' "
						 , ulListId);
		#ifdef __DEBUG
		printf("UpdateCprData> query = [%s]\n", szQuery);
		#endif

		if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
		{
			strcpyA(ErrMsg, "미적용 해시 조회 오류.\n");
			infLOG(ERROR, "%s",ErrMsg);
			infLOG(ERROR, "fups40061->UpdateCprData [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
			return -1;
		}
	}



	#ifdef __DEBUG
	printf("\n\n\n\n\ndcmdfups40061 UpdateCprData() 완료\n\n\n\n");
	#endif

	return 0;

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
int FlogInsertMurekaVideo(MYSQL *con, CMysqlCon MysqlCon
					, CFUPS4006 pfups4006
					, LPMUREKA_VINFO pMurekaVInfo, int nMurekaCnt, int nMurekaIndex
					)
{
	//infLOG(ALWAY, "fups4006->InsertMurekaVideo IN | temp_id [ %lu ] seq [ %lu ] \n",pfups4006.id,pfups4006.seq_no);

	MYSQL_RES   *res;
	MYSQL_ROW    row;

	char szSysErrMsg[255];
	memset(szSysErrMsg,0x00,sizeof(szSysErrMsg));

	char szQuery[10000];
	memset(szQuery, 0x00, sizeof(szQuery));

	//--------------------------------------------------------------------------
	//기존 항목이 있는지 확인
	//--------------------------------------------------------------------------
	//mureak 정보 추가
	//for( int i =0 ; i < nMurekaCnt ; i++ )
	int  i= nMurekaIndex;
	if( nMurekaCnt > 0 && i < nMurekaCnt )
	{
		if( pMurekaVInfo[i].filename == NULL || strlen(pMurekaVInfo[i].filename) <= 0)
			return -1;

		if( pMurekaVInfo[i].nFileGubun == 4 ) //바이러스
		{
			sprintf(szQuery,"insert into zangsi.T_CONTFLOG_TEMPLIST_MUREKA "
						" (  seq_no , id , file_gubun , result_code , video_status , video_id , video_title , video_jejak_year , video_right_name "
						" , video_right_content_id , video_grade , video_price , video_cha , video_osp_jibun , video_osp_etc "
						" , video_onair_date , video_right_id , virus_type , virus_name ,mureka_hash ,file_name ) "
						" values ( %lu , %lu , %d , %d , '%s', '%s', '%s', '%s', '%s' "
						" , '%s' , '%s'  , '%s'  , '%s'  , '%s'  , '%s' "
						" , '%s'  , '%s'  , '%s'  , '%s' ,'%s','%s' )"
						  , pfups4006.seq_no, pfups4006.id ,pMurekaVInfo[i].nFileGubun,pMurekaVInfo[i].nResultCode,pMurekaVInfo[i].video_status
						  ,pMurekaVInfo[i].video_id,pMurekaVInfo[i].video_title,pMurekaVInfo[i].video_jejak_year,pMurekaVInfo[i].video_right_name
						  ,pMurekaVInfo[i].video_right_content_id,pMurekaVInfo[i].video_grade,pMurekaVInfo[i].video_price,pMurekaVInfo[i].video_cha,pMurekaVInfo[i].video_osp_jibun,pMurekaVInfo[i].video_osp_etc
						  ,pMurekaVInfo[i].video_onair_date,pMurekaVInfo[i].video_right_id,pMurekaVInfo[i].video_right_name,pMurekaVInfo[i].video_title ,pMurekaVInfo[i].mureka_hash,pMurekaVInfo[i].filename);
		}
		else if( pMurekaVInfo[i].nFileGubun == 2 ) //동영상
		{
			sprintf(szQuery,"insert into zangsi.T_CONTFLOG_TEMPLIST_MUREKA "
							" ( seq_no , id , file_gubun , result_code , video_status , video_id , video_title , video_jejak_year , video_right_name "
							" , video_right_content_id , video_grade , video_price , video_cha , video_osp_jibun , video_osp_etc "
							" , video_onair_date , video_right_id ,mureka_hash ,file_name ) "
							" values ( %lu , %lu , %d , %d , '%s', '%s', '%s', '%s', '%s' "
							" , '%s' , '%s'  , '%s'  , '%s'  , '%s'  , '%s' "
							" , '%s'  , '%s'  ,'%s','%s' )"
							  , pfups4006.seq_no, pfups4006.id ,pMurekaVInfo[i].nFileGubun,pMurekaVInfo[i].nResultCode,pMurekaVInfo[i].video_status
							  ,pMurekaVInfo[i].video_id,pMurekaVInfo[i].video_title,pMurekaVInfo[i].video_jejak_year,pMurekaVInfo[i].video_right_name
							  ,pMurekaVInfo[i].video_right_content_id,pMurekaVInfo[i].video_grade,pMurekaVInfo[i].video_price,pMurekaVInfo[i].video_cha,pMurekaVInfo[i].video_osp_jibun,pMurekaVInfo[i].video_osp_etc
							  ,pMurekaVInfo[i].video_onair_date,pMurekaVInfo[i].video_right_id ,pMurekaVInfo[i].mureka_hash ,pMurekaVInfo[i].filename);
		}
		//infLOG(ALWAY, "fups4006->InsertMurekaVideo QUERY [ %s ]\n",szQuery);
		if( pMurekaVInfo[i].nFileGubun == 4 || pMurekaVInfo[i].nFileGubun == 2 )
		{
			if(MysqlCon.MysqlQuery(con, szSysErrMsg, szQuery))
			{
				infLOG(ERROR, "fups4006->InsertMurekaVideo : MysqlQuery error.\n");
				infLOG(ERROR, "fups4006->InsertMurekaVideo : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
			}
		}
	}


	//infLOG(ALWAY, "fups4006->InsertMurekaVideo OUT | temp_id [ %lu ] seq [ %lu ] \n",pfups4006.id,pfups4006.seq_no);
	return 0;
}
