/******************************************************************************
 *   서브시스템 : DCMD서버
 *   프로그램명 : fups40051.cc
 *         기능 : 컨텐츠 등록
 *         설명 :
 *       작성자 : HCS
 *       작성일 : 2009/09/11
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
#include <unistd.h> 

#include "comcomm.h"
#include "commydb.h"
#include "apdefine.h"
//#include "/home/nori/zangsi_with_dcmd/fupsvr/inc/fups4005.h"
#include "../../fupsvr/inc/fups4005.h"
#include "dcmdfups40051.h"
#include "dcmdcomlib.h" //업로드 fupcomlib
//#define  _DEBUG_
#include "mysql_pool.h"

extern CMysqlPool * m_g_clMysqlPool;
extern CMysqlPool * m_g_clMysqlPoolCopyRight;	//유료컨테츠 DB
extern CMysqlPool * m_g_clMysqlPoolHadoop;		//통합 DB

int FlogInsertFileData(MYSQL *con, CMysqlCon MysqlCon, MYSQL *cpr_con, CMysqlCon MysqlCprCon
					, char* pDefaultHash, char* pFileName, double dwFileSize, char* pFileType
					, CFUPS4005 pFUPS4005
					, bool bIsZip, char* pMurekaStatus, char* pRegDate, char* pRegTime
					, LPMUREKA_VINFO pMurekaVInfo, int nMurekaCnt, int nMurekaIndex,unsigned long uFile_id, char* pServer_group_id); // 통합 추가

int FlogDeleteCprVideo(MYSQL *con, CMysqlCon MysqlCon, MYSQL *cpr_con, CMysqlCon MysqlCprCon
					, CFUPS4005 pFUPS4005
					, MUREKA_VINFO MurekaVInfo
					, char* pRegDate, char* pRegTime);

int FlogInsertFilterVideo(MYSQL *cpr_con, CMysqlCon MysqlCprCon
					, CFUPS4005 pFUPS4005
					, MUREKA_VINFO MurekaVInfo
					, char* pRegDate, char* pRegTime, bool bIsZip);

int FlogInsertFilterMusic(MYSQL *cpr_con, CMysqlCon MysqlCprCon
					, CFUPS4005 pFUPS4005
					, MUREKA_VINFO MurekaVInfo
					, char* pRegDate, char* pRegTime, bool bIsZip);


int FlogInsertMurekaVideo(MYSQL *con, CMysqlCon MysqlCon
					, CFUPS4005 pFUPS4005
					, LPMUREKA_VINFO pMurekaVInfo, int nMurekaCnt, int nMurekaIndex
					);


long dcmdfups40051(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData)//CFUPS4005 pfups4005)
{
	CFUPS4005 pfups4005;
	memcpy(&pfups4005, (LPCFUPS4005)pRecvData ,sizeof(CFUPS4005));
	
	// 2009/06/14 - HCS :  뮤레카 정보 받기.
	int nMurekaCnt = pfups4005.mureka_cnt;
	
	LPMUREKA_VINFO pMurekaVInfo = NULL;
	
	if(nMurekaCnt > 0)
	{
		char* pTempBuffer = new char[sizeof(MUREKA_VINFO)*nMurekaCnt];
		memcpy( pTempBuffer , pRecvData + sizeof(CFUPS4005) , sizeof(MUREKA_VINFO)*nMurekaCnt);
		pMurekaVInfo = (LPMUREKA_VINFO)pTempBuffer;
	}

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

		infLOG(ERROR, "FUPS40051 | GetMysqlCon is null \n");
		
		nRetry = 0;
		while (!(con=db_connect("nori")) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);

			infLOG(ERROR, "FUPS40051 | GetMysqlCon is null [%d](%s)\n",ErrNum, ErrMsg);
			infLOG(ERROR, "FUPS40051 | Mysql Error | Errno : %d | ErrMsg : %s \n", mysql_errno(con),mysql_error(con));			
		}
		
		if( nRetry >= 5)
		{				
			req_header.nCmd = -1 ;	
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);
					
			infLOG(ERROR, "FUPS40051 | Cannot DB Connect \n");
			
	       	return HEADER_SIZE;
	    }
	    infLOG(ERROR, "FUPS40051 | Connect DB direct\n");
	    
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

		infLOG(ERROR, "FUPS40051 | MysqlCprCon is null \n");
		
		nRetry = 0;
		while (!(cpr_con=Cpr_db_connect("nori_cpr")) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);

			infLOG(ERROR, "FUPS40051 |  MysqlCprCon is null [%d](%s)\n",ErrNum, ErrMsg);
			infLOG(ERROR, "FUPS40051 | Mysql CPR Error | Errno : %d | ErrMsg : %s \n", mysql_errno(cpr_con),mysql_error(cpr_con));			
		}
		if( nRetry >= 5)
		{				
			req_header.nCmd = -1 ;	
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);
					
			infLOG(ERROR, "FUPS40051 | Cannot CPR DB Connect \n");
			db_disconnect(con);
	       	return HEADER_SIZE;
	    }
	    infLOG(ERROR, "FUPS40051 | GetMysqlCprCon Direct Connect \n");
	    
	    bCprCloseDB = true;
	}
	//--------------------------------------------------------------------------

	// 통합db =========================================================
	MYSQL *con_hadoop=NULL;
	MYSQL_RES   *res_hadoop;
	MYSQL_ROW    row_hadoop;
	bool bHadoopCloseDB = false;
	
	CMysqlCon MysqlHadoopnCon(m_g_clMysqlPoolHadoop,m_g_clMysqlPoolHadoop->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );
	con_hadoop = MysqlHadoopnCon.GetMysqlCon();
	
	if (con_hadoop == NULL )
	{
		ErrNum = -400592;
		sprintf(ErrMsg, "MysqlHadoopnCon2 에 접속하지 못 하였습니다.\n");
		
		infLOG(ERROR, "FUPS4005 | [ %s ]\n",ErrMsg);
		
		nRetry = 0;
		while (!(con_hadoop=Hadoop_db_connect("cnfs")) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			infLOG(ERROR, "FUPS4005 | GetMysqlCon is null [%d](%s)\n",ErrNum, ErrMsg);
			infLOG(ERROR, "FUPS4005 | Mysql Error | Errno : %d | ErrMsg : %s \n", mysql_errno(con_hadoop),mysql_error(con_hadoop));			
		}
		
		if( nRetry >= 5)
		{				
			req_header.nCmd = -1 ;	
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);
			
			infLOG(ERROR, "FUPS4005 | Cannot DB Connect \n");
			
			return HEADER_SIZE;
		}
		infLOG(ALWAY, "FUPS4005 | Connect DB direct\n");
		bHadoopCloseDB = true;
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

	ReplaceSingleQuotation(pfups4005.file_name, '\'',pfups4005.file_name);
	ReplaceSingleQuotation(pfups4005.folder_name, '\'',pfups4005.folder_name); // 인탁 20090306일수
		
	memset (szQuery , 0x00, sizeof(szQuery ));
		
	memset (pfups4005.copyright_yn , 0x00, sizeof(pfups4005.copyright_yn ));
	strcpyA(pfups4005.copyright_yn,"N");
	if(strcmp(pfups4005.cont_gu, "FD") == 0)
	{
		memset(szQuery , 0x00, sizeof(szQuery ));
		
		//2009/06/15 - HCS : 뮤레카 정보중 차단된 컨텐츠가 있는지 검사.
		/* 
		클라이언트로 부터 넘겨 받은 뮤레카 정보중 차단된 컨텐츠가 있다면
		nori DB의 저작권 데이터를 수정한다.
		뮤레카 정보가 하나도 없다면 기존방식대로 처리.
		*/
		bool bIsZip = false;

		if(nMurekaCnt > 1)
			bIsZip = true;
			
		//	하둡 통합 DB 검색 ====================================================================	
		memset(szQuery, 0x00, sizeof(szQuery));

		sprintf(szQuery, " select a.file_id , a.server_group_id from cnfs.T_CONTENTS_FILELIST_UNIQ a where a.default_hash = '%s'  and a.use_yn = 'Y' and a.file_size = %.0f and a.reg_date <= date_format(date_add(now(), interval -7 day),'%Y%m%d') "
						,pfups4005.default_hash,pfups4005.file_size);
		
		bool bIsHadoop = false;
		unsigned long uFile_id=0; // %lu
		char pServer_group_id[11];	memset (pServer_group_id , 0x00, sizeof(pServer_group_id ));
		MYSQL_RES   *res_hadoop;
		MYSQL_ROW    row_hadoop;

		if (MysqlHadoopnCon.MysqlQuery(con_hadoop,szSysErrMsg, szQuery))
		{
			ErrNum = -4006;
			sprintf(ErrMsg, "T_CONTENTS_FILELIST_UNIQ 조회 오류입니다.\n");
		}else if (!(res_hadoop = mysql_store_result(con_hadoop)))
		{
			ErrNum = -4006;
			sprintf(ErrMsg, "T_CONTENTS_FILELIST_UNIQ 오류입니다.\n");
		}else if (mysql_num_rows(res_hadoop)!=0)
		{
			row_hadoop = mysql_fetch_row(res_hadoop);
			uFile_id = (unsigned long)getnum(row_hadoop,0);
			strcpyA(pServer_group_id, getstr(row_hadoop,1));
			infLOG(ALWAY,"Hadoop = uFile_id=%lu , %s\n", uFile_id,szQuery);
			bIsHadoop = true;
			//memset(szSectSub, 0x00, sizeof(szSectSub));
			//strcpyA(szSectSub, getstr(row_hadoop, 0));
		}	
		mysql_free_result(res_hadoop);

		// ====================================================================

		for(int i=0; i<nMurekaCnt; i++)
		{
			char szHashValue[32+1];
			memset(szHashValue, 0x00, sizeof(szHashValue));
			char szSize[128];
			memset(szSize, 0x00, sizeof(szSize));
			double dwSize = 0;

			//--------------------------------------------------------------------------
			//동영상 필터링
			//--------------------------------------------------------------------------
			if(pMurekaVInfo[i].nFileGubun == 2) 
			{
				infLOG(ALWAY,"dcmdfups40051> video_status = [%s]\n",pMurekaVInfo[i].video_status);
				infLOG(ALWAY,"dcmdfups40051> video_id = [%s]\n",pMurekaVInfo[i].video_id);
				infLOG(ALWAY,"dcmdfups40051> video_title = [%s]\n",pMurekaVInfo[i].video_title);
				infLOG(ALWAY,"dcmdfups40051> video_jejak_year = [%s]\n",pMurekaVInfo[i].video_jejak_year);
				infLOG(ALWAY,"dcmdfups40051> video_right_name = [%s]\n",pMurekaVInfo[i].video_right_name);
				infLOG(ALWAY,"dcmdfups40051> video_right_content_id = [%s]\n",pMurekaVInfo[i].video_right_content_id);
				infLOG(ALWAY,"dcmdfups40051> video_grade = [%s]\n",pMurekaVInfo[i].video_grade);
				infLOG(ALWAY,"dcmdfups40051> video_price = [%s]\n",pMurekaVInfo[i].video_price);
				infLOG(ALWAY,"dcmdfups40051> video_cha = [%s]\n",pMurekaVInfo[i].video_cha);
				infLOG(ALWAY,"dcmdfups40051> video_osp_jibun = [%s]\n",pMurekaVInfo[i].video_osp_jibun);
				infLOG(ALWAY,"dcmdfups40051> video_osp_etc = [%s]\n",pMurekaVInfo[i].video_osp_etc);
				infLOG(ALWAY,"dcmdfups40051> video_onair_date = [%s]\n",pMurekaVInfo[i].video_onair_date);
				infLOG(ALWAY,"dcmdfups40051> video_right_id = [%s]\n",pMurekaVInfo[i].video_right_id);
				
				strcpy(szFileType, "동영상");
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

				if(strcmp(pMurekaVInfo[i].video_status, "02") == 0)
				{// 저작물이 하나라도 있다면 관리테이블을 업데이트/인서트 한다.
					strcpyA(szMurekaStatus, pMurekaVInfo[i].video_status);
					#ifdef __DEBUG
					printf("dcmdfups40051> 차단 항목 있음.\n");
					printf("dcmdfups40051> szCopyrightUser : [%s]\n", pMurekaVInfo[i].video_right_name);
					printf("dcmdfups40051> szTitle : [%s]\n", pMurekaVInfo[i].video_title);
					printf("dcmdfups40051> szVideoHash : [%s]\n", pMurekaVInfo[i].mureka_hash);
					#endif
					
	
					int nRet = FlogDeleteCprVideo(con, MysqlCon, cpr_con, MysqlCprCon
													, pfups4005
													, pMurekaVInfo[i]
													, reg_date, reg_time);
					if(nRet < 0)
					{
						infLOG(ERROR, "FlogDeleteCprVideo 실패\n"); 														
					}

					nRet = 0;
					nRet = FlogInsertFilterVideo(cpr_con, MysqlCprCon
											, pfups4005
											, pMurekaVInfo[i]
											, reg_date, reg_time, bIsZip);
					if(nRet < 0)
					{
						infLOG(ERROR, "FlogInsertFilterVideo 실패\n"); 														
					}
				}
			}
			else if(pMurekaVInfo[i].nFileGubun == 1)
			{
				infLOG(ALWAY,"dcmdfups40051> music_status = [%s]\n",pMurekaVInfo[i].music_status);
				infLOG(ALWAY,"dcmdfups40051> music_id = [%s]\n",pMurekaVInfo[i].music_id);
				infLOG(ALWAY,"dcmdfups40051> music_title = [%s]\n",pMurekaVInfo[i].music_title);
				infLOG(ALWAY,"dcmdfups40051> music_artist = [%s]\n",pMurekaVInfo[i].music_artist);
				infLOG(ALWAY,"dcmdfups40051> music_album = [%s]\n",pMurekaVInfo[i].music_album);
				infLOG(ALWAY,"dcmdfups40051> music_prod_code = [%s]\n",pMurekaVInfo[i].music_prod_code);
				infLOG(ALWAY,"dcmdfups40051> music_price = [%s]\n",pMurekaVInfo[i].music_price);
				infLOG(ALWAY,"dcmdfups40051> music_injeob_com = [%s]\n",pMurekaVInfo[i].music_injeob_com);
				infLOG(ALWAY,"dcmdfups40051> music_injeob_music_id = [%s]\n",pMurekaVInfo[i].music_injeob_music_id);
				infLOG(ALWAY,"dcmdfups40051> video_osp_jibun = [%s]\n",pMurekaVInfo[i].video_osp_jibun);
				infLOG(ALWAY,"dcmdfups40051> video_osp_etc = [%s]\n",pMurekaVInfo[i].video_osp_etc);
				infLOG(ALWAY,"dcmdfups40051> video_onair_date = [%s]\n",pMurekaVInfo[i].video_onair_date);
				infLOG(ALWAY,"dcmdfups40051> video_right_id = [%s]\n",pMurekaVInfo[i].video_right_id);
				strcpy(szFileType, "음악");
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
					#ifdef __DEBUG
					printf("dcmdfups40051> 차단 항목 있음.\n");
					printf("dcmdfups40051> szCopyrightUser : [%s]\n", pMurekaVInfo[i].music_injeob_com);
					printf("dcmdfups40051> szTitle : [%s]\n", pMurekaVInfo[i].music_title);
					printf("dcmdfups40051> szVideoHash : [%s]\n", pMurekaVInfo[i].mureka_hash);
					#endif

					int nRet = FlogInsertFilterMusic(cpr_con, MysqlCprCon
											, pfups4005
											, pMurekaVInfo[i]
											, reg_date, reg_time, bIsZip);
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
					int nResult = FlogInsertFileData(con, MysqlCon, cpr_con, MysqlCprCon, "", "", 0, szFileType, pfups4005, false, szMurekaStatus, reg_date, reg_time,pMurekaVInfo,nMurekaCnt,i,uFile_id, pServer_group_id);
					
					if(nResult == -1)
						goto fups4005_err;
					else if(nResult == 1)
						strcpy(pfups4005.copyright_yn, "Y");
				}
				else if(pMurekaVInfo[i].nFileGubun == 2 || pMurekaVInfo[i].nFileGubun == 1 || pMurekaVInfo[i].nFileGubun == 4)
				{	
					int nResult = FlogInsertFileData(con, MysqlCon, cpr_con, MysqlCprCon, szHashValue, pMurekaVInfo[i].filename, dwSize, szFileType, pfups4005, bIsZip, szMurekaStatus, reg_date, reg_time,pMurekaVInfo,nMurekaCnt,i,uFile_id, pServer_group_id);
						if(nResult == -1)
							goto fups4005_err;
						else if(nResult == 1)
							strcpy(pfups4005.copyright_yn, "Y");
				}			
			}			
			else
			{
				int nResult = FlogInsertFileData(con, MysqlCon, cpr_con, MysqlCprCon, szHashValue, pMurekaVInfo[i].filename, dwSize, szFileType, pfups4005, bIsZip, szMurekaStatus, reg_date, reg_time,pMurekaVInfo,nMurekaCnt,i,uFile_id, pServer_group_id);
					if(nResult == -1)
						goto fups4005_err;
					else if(nResult == 1)
						strcpy(pfups4005.copyright_yn, "Y");
			}
			//--------------------------------------------------------------------------
		}
		
		//--------------------------------------------------------------------------
		//뮤레카 필터링 거치지 않은 파일 등록
		//--------------------------------------------------------------------------
		if(nMurekaCnt <= 0)
		{
			int nResult = FlogInsertFileData(con, MysqlCon, cpr_con, MysqlCprCon, "", "", 0, szFileType, pfups4005, false, szMurekaStatus, reg_date, reg_time,pMurekaVInfo,nMurekaCnt,0,uFile_id, pServer_group_id);
			
			if(nResult == -1)
				goto fups4005_err;
			else if(nResult == 1)
				strcpy(pfups4005.copyright_yn, "Y");
		}		
		//--------------------------------------------------------------------------
			
	}
	else
	{
		infLOG(ALWAY,"fups40051 등록 실패, 잘못된 cont_gu 입력 : %s\n", pfups4005.cont_gu);
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
	
	//infLOG(ALWAY,"fups4005 등록 완료\n");		
  	
  	return HEADER_SIZE;
	
//------------------------------------------------------------------------------
fups4005_err:
    infLOG(ERROR, "Exception ) fups40051-> [%d](%s)\n",ErrNum, ErrMsg);
	infLOG(ERROR, "fups40051-> [%d](%s)\n",mysql_errno(con), mysql_error(con));
	
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
* (I) 1. DB connet 정보(nori, nori_cpr)
*     2. 파일 정보(해시, 파일명, 사이즈, 파일 타입[동영상,음악])
*     3. 컨텐츠 정보
*     4. 기타 정보(압축여부, 뮤레카 결과 코드, 등록일시)
* (R) 1. 0		: 일반 컨텐츠
*	  2. 음수	: 오류
*	  3. 1	 	: 차단 컨텐츠
*****************************************************************************/
int FlogInsertFileData(MYSQL *con, CMysqlCon MysqlCon, MYSQL *cpr_con, CMysqlCon MysqlCprCon
					, char* pDefaultHash, char* pFileName, double dwFileSize, char* pFileType
					, CFUPS4005 pFUPS4005
					, bool bIsZip, char* pMurekaStatus, char* pRegDate, char* pRegTime
					, LPMUREKA_VINFO pMurekaVInfo, int nMurekaCnt, int nMurekaIndex,unsigned long uFile_id, char* pServer_group_id)
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
    sprintf(szExtHash, "%s%.0f", pFUPS4005.default_hash, pFUPS4005.file_size);
    
	if(bIsZip)
	{
		memset(pFUPS4005.default_hash, 0x00, sizeof(pFUPS4005.default_hash));
		sprintf(pFUPS4005.default_hash, "%s", pDefaultHash);

		pFUPS4005.file_size = dwFileSize;

		memset(pFUPS4005.file_name, 0x00, sizeof(pFUPS4005.file_name));
		sprintf(pFUPS4005.file_name, "%s", pFileName);
		
		ReplaceSingleQuotation(pFUPS4005.file_name, '\'',pFUPS4005.file_name);

		strcpy(szExtHash, pDefaultHash);
	}
	#ifdef _DEBUG_
	printf("FlogInsertFileData> 해시   (%s).\n", pFUPS4005.default_hash);
	printf("FlogInsertFileData> 파일명 (%s).\n", pFUPS4005.file_name);
	printf("FlogInsertFileData> 사이즈 (%.0f).\n", pFUPS4005.file_size);
	#endif

	
	//--------------------------------------------------------------------------
	// 차단 컨텐츠 조회
	//--------------------------------------------------------------------------
	unsigned long ulFileSeqNo = 0;
	unsigned long ulListId = 0;
	char szTitle[250+1];
	memset(szTitle, 0x00, sizeof(szTitle));
	
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select a.seq_no, b.list_id, b.title "
					 " from nori_cpr.T_FILTER_HASH_FILE a, nori_cpr.T_FILTER_CONT_LIST b "
					 " where a.default_hash_ext = '%s' and a.list_id = b.list_id and b.status = 'C'  "
					 , szExtHash);
	#ifdef __DEBUG
	printf("FlogInsertFileData> query = [%s]\n\n", szQuery);
	#endif
	if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		infLOG(ERROR, "fups40051->FlogInsertFileData : MysqlQuery 오류.\n");
		infLOG(ERROR, "fups40051->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
	}
	else
	{
		//차단 컨텐츠이면 
		if (!(cpr_res = mysql_store_result(cpr_con)))
		{
			infLOG(ERROR, "fups40051->FlogInsertFileData : mysql_store_result 오류.\n");
			infLOG(ERROR, "fups40051->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
		}
		else
		{
			if (mysql_num_rows(cpr_res) > 0 )
			{
				cpr_row = mysql_fetch_row(cpr_res);
				ulFileSeqNo = (unsigned long)getnum(cpr_row,0);
				ulListId = (unsigned long)getnum(cpr_row,1);
				sprintf(szTitle, "%s", getstr(cpr_row,2));
				
				memset(pFUPS4005.copyright_yn, 0x00, sizeof(pFUPS4005.copyright_yn));
				strcpy(pFUPS4005.copyright_yn, "Y");
				nResult = 1;
			}
			mysql_free_result(cpr_res);
		}
	}
	//--------------------------------------------------------------------------
	
	if(strcmp(pFUPS4005.copyright_yn, "Y") == 0)
	{	
		//--------------------------------------------------------------------------
		//중복 데이터 조회
		//--------------------------------------------------------------------------
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "select seq_no from zangsi.T_CONTFLOG_TEMPLIST "
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
			infLOG(ERROR, "fups40051->FlogInsertFileData : MysqlQuery 오류.\n");
			infLOG(ERROR, "fups40051->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
			return -1;
	  	}	
		if (!(res = mysql_store_result(con)))
		{
			infLOG(ERROR, "fups40051->FlogInsertFileData : mysql_store_result 오류.\n");
			infLOG(ERROR, "fups40051->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
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
			
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, "insert into zangsi.T_CONTFLOG_TEMPLIST "
							 " ( id , depth, folder_yn, folder_path, sect_code, file_name, file_size, file_type, reg_user, reg_date, reg_time, default_hash, audio_hash, video_hash, default_hash_ext, copyright_yn, mureka_code ,file_id ,server_group_id) "
			                 " values "
			                 " ( %lu, %d, '%s', '%s', '%s', '%s', %.0f, '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s' , %lu , '%s') "
			                 ,pFUPS4005.id
			                 ,pFUPS4005.depth
			                 ,pFUPS4005.folder_yn
			                 ,pFUPS4005.folder_name
			                 ,pFUPS4005.sect_code
			                 ,pFUPS4005.file_name
			                 ,pFUPS4005.file_size
			                 ,pFileType
			                 ,pFUPS4005.user_id
			                 ,pRegDate
			                 ,pRegTime
			                 ,pFUPS4005.default_hash
			                 ,pFUPS4005.audio_hash
			                 ,pFUPS4005.video_hash
			                 ,szExtHash
			                 ,pFUPS4005.copyright_yn
			                 ,pMurekaStatus
							 ,uFile_id
							 ,pServer_group_id );			
		 		#ifdef _DEBUG_
		 		printf("FlogInsertFileData> szQuery=[%s].\n", szQuery);
		 		#endif 
				if (MysqlCon.MysqlQuery(con,szSysErrMsg, szQuery))
				{
					infLOG(ERROR, "fups40051->FlogInsertFileData : MysqlQuery 오류.\n");
					infLOG(ERROR, "fups40051->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
					
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
									CFUPS4005 fups4005 = pFUPS4005;
									fups4005.seq_no = filelist_seq;
									
									FlogInsertMurekaVideo(con,  MysqlCon	, fups4005, pMurekaVInfo,nMurekaCnt,nMurekaIndex);
								}
							}
						}
					}
				}
				
			
		}
		//--------------------------------------------------------------------------
		
		//--------------------------------------------------------------------------
		//차단 컨텐츠 로그 기록
		//--------------------------------------------------------------------------
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " INSERT INTO nori_cpr.T_FILTER_HIST_TEMP "
					 " (cont_id, cont_gu, file_seq_no, list_id, title, sect_code, sect_sub, default_hash, audio_hash, video_hash, default_hash_ext, file_name, file_size, reg_user, reg_date, reg_time) "
	                 " values "
	                 " (%lu, 'FD', %lu, %lu, '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', %.0f, '%s', '%s', '%s') "
	                 ,pFUPS4005.id
	                 ,ulFileSeqNo
	                 ,ulListId
	                 ,szTitle
	                 ,pFUPS4005.sect_code
	                 ,pFUPS4005.sect_sub
	                 ,pFUPS4005.default_hash
	                 ,pFUPS4005.audio_hash
	                 ,pFUPS4005.video_hash
	                 ,szExtHash
	                 ,pFUPS4005.file_name
	                 ,pFUPS4005.file_size
	                 ,pFUPS4005.user_id
	                 ,pRegDate
	                 ,pRegTime);
		#ifdef _DEBUG_
		printf("FlogInsertFileData> szQuery=[%s].\n", szQuery);
		#endif 
		if (MysqlCon.MysqlQuery(cpr_con,szSysErrMsg, szQuery))
		{
			if(mysql_errno(cpr_con) != 1062 )
			{
				infLOG(ALWAY, "nResult->%d.\n",nResult);
				infLOG(ERROR, "fups40051->FlogInsertFileData : MysqlQuery 오류.\n");
				infLOG(ERROR, "fups40051->FlogInsertFileData : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
				return -1;
			}
	  	}
		//--------------------------------------------------------------------------
	}
	
	infLOG(ALWAY, "nResult2->%d.\n",nResult);
	return nResult;
}


/*****************************************************************************
* 제휴 데이터 서비스 취소 처리
* (I) 1. DB connet 정보(nori, nori_cpr)
*     3. 뮤레카 정보
*     4. 기타 정보(등록일시)
* (R) 1. 0		: 정상
*	  2. 음수	: 오류
*****************************************************************************/
int FlogDeleteCprVideo(MYSQL *con, CMysqlCon MysqlCon, MYSQL *cpr_con, CMysqlCon MysqlCprCon
					, CFUPS4005 pFUPS4005
					, MUREKA_VINFO MurekaVInfo
					, char* pRegDate, char* pRegTime)
{
	if(strlen(MurekaVInfo.video_right_content_id) <= 0)
	{
		infLOG(ERROR, "fups40051->FlogDeleteCprVideo : 관리코드 없음.\n");
		return -1;
	}

	MYSQL_RES   *cpr_res;
	MYSQL_ROW    cpr_row;

	char szSysErrMsg[255];
	memset(szSysErrMsg,0x00,sizeof(szSysErrMsg));

	char szQuery[15000];
	memset(szQuery, 0x00, sizeof(szQuery));
	
	//--------------------------------------------------------------------------
	//권리사코드 조회
	//--------------------------------------------------------------------------
	char szCompCd[6+1];
	memset(szCompCd, 0x00, sizeof(szCompCd));

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "select comp_cd from nori_cpr.T_CPR_COMP_INFO where comp_nm = '%s'", MurekaVInfo.video_right_name);

	if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		infLOG(ERROR, "fups40051->FlogDeleteCprVideo : MysqlQuery error.\n");
		infLOG(ERROR, "fups40051->FlogDeleteCprVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
		return -1;
	}
	if (!(cpr_res = mysql_store_result(cpr_con)))
	{
		infLOG(ERROR, "fups40051->FlogDeleteCprVideo : mysql_store_result error.\n");
		infLOG(ERROR, "fups40051->FlogDeleteCprVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
		mysql_free_result(cpr_res);
		return -1;
	}
	if (mysql_num_rows(cpr_res) == 0 )
 	{
		infLOG(ERROR, "fups40051->FlogDeleteCprVideo : mysql_num_rows error.\n");
		infLOG(ERROR, "fups40051->FlogDeleteCprVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
		mysql_free_result(cpr_res);
		return -1;
	}

	cpr_row = mysql_fetch_row(cpr_res);

	strcpyA(szCompCd, getstr(cpr_row, 0));

	mysql_free_result(cpr_res);
	//--------------------------------------------------------------------------
	
	
	//--------------------------------------------------------------------------
	//삭제할 정보 조회
	//--------------------------------------------------------------------------
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select b.list_id, b.seq_no, b.status "
					 " from nori_cpr.T_CPR_CONT_LIST a, nori_cpr.T_CPR_CONT_LIST_SUB b "
					 " where a.list_id = b.list_id and b.mgr_cd = '%s' and b.comp_cd = '%s' and a.chapter = %s "
				   , MurekaVInfo.video_right_content_id
				   , szCompCd
				   , MurekaVInfo.video_cha);
	if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		infLOG(ERROR, "fups40051->FlogDeleteCprVideo : T_CPR_CONT_LIST 조회 오류.\n");
		infLOG(ERROR, "fups40051->FlogDeleteCprVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
		return -1;
	}
	if (!(cpr_res = mysql_store_result(cpr_con)))
	{
		infLOG(ERROR, "fups40051->FlogDeleteCprVideo : T_CPR_CONT_LIST 데이터 생성 오류.\n");
		infLOG(ERROR, "fups40051->FlogDeleteCprVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
		mysql_free_result(cpr_res);
		return -1;
	}
	if (mysql_num_rows(cpr_res) <= 0 )
	{
		infLOG(ALWAY, "fups40051->FlogDeleteCprVideo : T_CPR_CONT_LIST 관리코드 없음.[%s]\n", MurekaVInfo.video_right_content_id);
		infLOG(ALWAY, "fups40051->FlogDeleteCprVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
		mysql_free_result(cpr_res);
		return 0;
	}
	cpr_row = mysql_fetch_row(cpr_res);
	
	unsigned long ulListId = 0;
	ulListId = (unsigned long)getnum(cpr_row, 0);
	
	unsigned long ulSeqNo = 0;
	ulSeqNo = (unsigned long)getnum(cpr_row, 1);
	
	char szStatus[1+1];
	memset(szStatus, 0x00, sizeof(szStatus));
	strcpyA(szStatus, getstr(cpr_row, 2));
	
	mysql_free_result(cpr_res);

	if(strcmp(szStatus, "C") == 0)
	{
		//--------------------------------------------------------------------------
		//트렌젝션 시작
		//--------------------------------------------------------------------------
		if (tran_begin(cpr_con)!=0)
		{
			infLOG(ERROR, "fups40051->FlogDeleteCprVideo : 트렌젝션 오류.\n");
			infLOG(ERROR, "fups40051->FlogDeleteCprVideo : [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
			return -1;
		}
		//--------------------------------------------------------------------------
		
		
		//--------------------------------------------------------------------------
		//관리코드 정보 로그 기록
		//--------------------------------------------------------------------------
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into nori_cpr.T_CPR_CONT_LIST_HIST "
						 " (list_id, sub_seq_no, title, adult_yn, grade, chapter, open_date, reg_user, reg_date, reg_time, udt_user, udt_date, udt_time, descript) "
						 " select "
						 " list_id, %lu, title, adult_yn, grade, chapter, open_date, reg_user, reg_date, reg_time, 'system', '%s', '%s', 'mureka filtering 차단' "
						 " from nori_cpr.T_CPR_CONT_LIST "
						 " where list_id = %lu "
						 , ulSeqNo
						 , pRegDate
						 , pRegTime
						 , ulListId);
		#ifdef __DEBUG
		printf("FlogDeleteCprVideo> query = [%s]\n\n", szQuery);
		#endif
		if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
		{
			infLOG(ERROR, "fups40051->FlogDeleteCprVideo : T_CPR_CONT_LIST_HIST 인서트 오류.\n");
			infLOG(ERROR, "fups40051->FlogDeleteCprVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return -1;
		}
		
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into nori_cpr.T_CPR_CONT_LIST_SUB_HIST "
						 " (list_id, sub_seq_no, status, mgr_cd, comp_cd, mgr_ext, copyrighter, sect_code, sect_sub, price_amt, cpr_payment_rate " 
						 " , cpr_date_ext, reg_user, reg_date, reg_time, udt_user, udt_date, udt_time, descript, calc_status) "
						 " select "
						 " list_id, seq_no, status, mgr_cd, comp_cd, mgr_ext, copyrighter, sect_code, sect_sub, price_amt, cpr_payment_rate " 
						 " , cpr_date_ext, reg_user, reg_date, reg_time, 'system', '%s', '%s', 'mureka filtering 차단', 'C' "
						 " from nori_cpr.T_CPR_CONT_LIST_SUB "
						 " where list_id = %lu and seq_no = %lu "
						 , pRegDate
						 , pRegTime
						 , ulListId
						 , ulSeqNo);
		#ifdef __DEBUG
		printf("FlogDeleteCprVideo> query = [%s]\n\n", szQuery);
		#endif
		if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
		{
			infLOG(ERROR, "fups40051->FlogDeleteCprVideo : T_CPR_CONT_LIST_SUB_HIST 인서트 오류.\n");
			infLOG(ERROR, "fups40051->FlogDeleteCprVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return -1;
		}
		//--------------------------------------------------------------------------
		

		//--------------------------------------------------------------------------
		//관리코드 정보 삭제
		//--------------------------------------------------------------------------
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " delete from nori_cpr.T_CPR_CONT_LIST where list_id = %lu ", ulListId);
		#ifdef __DEBUG
		printf("FlogDeleteCprVideo> query = [%s]\n\n", szQuery);
		#endif
		if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
		{
			infLOG(ERROR, "fups40051->FlogDeleteCprVideo : T_CPR_CONT_LIST 삭제 오류.\n");
			infLOG(ERROR, "fups40051->FlogDeleteCprVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return -1;
		}

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " delete from nori_cpr.T_CPR_CONT_LIST_SUB where list_id = %lu and seq_no = %lu ", ulListId, ulSeqNo);
		#ifdef __DEBUG
		printf("FlogDeleteCprVideo> query = [%s]\n\n", szQuery);
		#endif
		if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
		{
			infLOG(ERROR, "fups40051->FlogDeleteCprVideo : T_CPR_CONT_LIST 삭제 오류.\n");
			infLOG(ERROR, "fups40051->FlogDeleteCprVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return -1;
		}
		//--------------------------------------------------------------------------
		
		
		//--------------------------------------------------------------------------
		//삭제할 해시정보 조회
		//--------------------------------------------------------------------------
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " select seq_no "
						 " from nori_cpr.T_CPR_HASH_FILE "
						 " where list_id = %lu "
						 , ulListId);

		if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
		{
			infLOG(ERROR, "fups40051->FlogDeleteCprVideo : 해시 파일정보 조회 오류.\n");
			infLOG(ERROR, "fups40051->FlogDeleteCprVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return -1;
		}
		if (!(cpr_res = mysql_store_result(cpr_con)))
		{
			infLOG(ERROR, "fups40051->FlogDeleteCprVideo : 데이터 생성 오류.\n");
			infLOG(ERROR, "fups40051->FlogDeleteCprVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
			mysql_free_result(cpr_res);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return -1;
		}
		if (mysql_num_rows(cpr_res) <= 0 )
		{
			infLOG(ALWAY, "해시 정보 없음.[%lu]\n", ulListId);
			mysql_free_result(cpr_res);
		}
		else
		{
			while(cpr_row = mysql_fetch_row(cpr_res))
			{
				MYSQL_RES   *res;
				MYSQL_ROW    row;
				
				unsigned long ulFileSeqNo = 0, ulHistSeqNo = 1;
				ulFileSeqNo = (unsigned long)getnum(cpr_row, 0);
				
				//--------------------------------------------------------------------------
				//해시정보 로그 시퀀스 발번
				//--------------------------------------------------------------------------
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " select max(seq_no) +1 "
								 " from nori_cpr.T_CPR_HASH_FILE_HIST "
								 " where file_seq_no = %lu "
								 , ulFileSeqNo);
				if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
				{
					infLOG(ERROR, "fups40051->FlogDeleteCprVideo : T_CPR_HASH_FILE_HIST seq_no 조회 오류(%lu).\n", ulFileSeqNo);
					infLOG(ERROR, "fups40051->FlogDeleteCprVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
					tran_rollback(cpr_con);
					tran_end(cpr_con);
					return -1;
				}
				if (!(res = mysql_store_result(cpr_con)))
				{
					infLOG(ERROR, "fups40051->FlogDeleteCprVideo : 데이터 생성 오류.\n");
					infLOG(ERROR, "fups40051->FlogDeleteCprVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
					mysql_free_result(cpr_res);
					tran_rollback(cpr_con);
					tran_end(cpr_con);
					return -1;
				}
				if (mysql_num_rows(cpr_res) <= 0 )
				{
					ulHistSeqNo = 1;
					mysql_free_result(res);
				}
				else
				{
					row = mysql_fetch_row(res);
					ulHistSeqNo = (unsigned long)getnum(row,0);
					mysql_free_result(res);
				}
				//--------------------------------------------------------------------------
				
				
				//--------------------------------------------------------------------------
				//해시정보 로그 기록
				//--------------------------------------------------------------------------
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " insert into nori_cpr.T_CPR_HASH_FILE_HIST "
								 " (file_seq_no, seq_no, list_id, default_hash, audio_hash, video_hash, default_hash_ext, file_name, file_size, status, reg_user, reg_date, reg_time, udt_user, udt_date, udt_time, descript, proc_cd) "
								 " select "
								 " seq_no, %lu, list_id, default_hash, audio_hash, video_hash, default_hash_ext, file_name, file_size, status, reg_user, reg_date, reg_time, 'system', '%s', '%s', 'mureka filtering 차단', 'D' "
								 " from nori_cpr.T_CPR_HASH_FILE "
								 " where seq_no = %lu "
								 , ulHistSeqNo
								 , pRegDate
								 , pRegTime
								 , ulFileSeqNo);
				#ifdef __DEBUG
				printf("FlogDeleteCprVideo> query = [%s]\n\n", szQuery);
				#endif
				if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
				{
					infLOG(ERROR, "fups40051->FlogDeleteCprVideo : T_CPR_HASH_FILE_HIST 인서트 오류(%lu).\n", ulFileSeqNo);
					infLOG(ERROR, "fups40051->FlogDeleteCprVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
					mysql_free_result(cpr_res);
					tran_rollback(cpr_con);
					tran_end(cpr_con);
					return -1;
				}
				//--------------------------------------------------------------------------
				
				
				//--------------------------------------------------------------------------
				//해시정보 삭제
				//--------------------------------------------------------------------------
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " delete from nori_cpr.T_CPR_HASH_FILE where seq_no = %lu ", ulFileSeqNo);
				#ifdef __DEBUG
				printf("FlogDeleteCprVideo> query = [%s]\n\n", szQuery);
				#endif
				if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
				{
					infLOG(ERROR, "fups40051->FlogDeleteCprVideo : T_CPR_HASH_FILE 삭제 오류(%lu).\n", ulFileSeqNo);
					infLOG(ERROR, "fups40051->FlogDeleteCprVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
					mysql_free_result(cpr_res);
					tran_rollback(cpr_con);
					tran_end(cpr_con);
					return -1;
				}
				//--------------------------------------------------------------------------
			}
			mysql_free_result(cpr_res);
		}
		
		//--------------------------------------------------------------------------
		//메인디비 관리코드 정보 삭제
		//--------------------------------------------------------------------------
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " delete from zangsi.T_CPR_CONT_LIST where list_id = %lu ", ulListId);
		#ifdef __DEBUG
		printf("FlogDeleteCprVideo> query = [%s]\n\n", szQuery);
		#endif
		if(MysqlCprCon.MysqlQuery(con, szSysErrMsg, szQuery))
		{
			infLOG(ERROR, "fups40051->FlogDeleteCprVideo : T_CPR_CONT_LIST 삭제 오류.\n");
			infLOG(ERROR, "fups40051->FlogDeleteCprVideo : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return -1;
		}

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " delete from nori_cpr.T_CPR_CONT_LIST_SUB where list_id = %lu and seq_no = %lu ", ulListId, ulSeqNo);
		#ifdef __DEBUG
		printf("FlogDeleteCprVideo> query = [%s]\n\n", szQuery);
		#endif
		if(MysqlCprCon.MysqlQuery(con, szSysErrMsg, szQuery))
		{
			infLOG(ERROR, "fups40051->FlogDeleteCprVideo : T_CPR_CONT_LIST_SUB 삭제 오류.\n");
			infLOG(ERROR, "fups40051->FlogDeleteCprVideo : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return -1;
		}
		//--------------------------------------------------------------------------


		//--------------------------------------------------------------------------
		//트렌젝션 적용
		//--------------------------------------------------------------------------
		if (tran_commit(cpr_con)!=0)
		{
			infLOG(ERROR, "fups40051->FlogDeleteCprVideo : 트렌젝션 commit 오류.\n");
			infLOG(ERROR, "fups40051->FlogDeleteCprVideo : [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return -1;
		}

		tran_end(cpr_con);
		//--------------------------------------------------------------------------
	}

	#ifdef __DEBUG
	printf("\n\n\n\n\ndcmdfups4006 FlogDeleteCprVideo() 완료\n\n\n\n");
	#endif
	
	return 0;
}

/*****************************************************************************
* 필터 데이터 등록(동영상)
* (I) 1. DB connet 정보(nori_cpr)
*     2. 파일 정보
*     3. 뮤레카 정보
*	  4. 기타 정보(등록일시, 압축 여부)
* (R) 1. 0		: 정상
*	  2. 음수	: 오류
*****************************************************************************/
int FlogInsertFilterVideo(MYSQL *cpr_con, CMysqlCon MysqlCprCon
					, CFUPS4005 pFUPS4005
					, MUREKA_VINFO MurekaVInfo
					, char* pRegDate, char* pRegTime, bool bIsZip)
{
	MYSQL_RES   *cpr_res;
	MYSQL_ROW    cpr_row;

	char szSysErrMsg[255];
	memset(szSysErrMsg,0x00,sizeof(szSysErrMsg));

	char szQuery[15000];
	memset(szQuery, 0x00, sizeof(szQuery));
	

	//--------------------------------------------------------------------------
	//압축 파일일 경우 확장해시는 뮤레카 해시로 대체
	//--------------------------------------------------------------------------
	char szExtHash[50+1];
	memset(szExtHash, 0x00, sizeof(szExtHash));
	
	char szFileName[512+1];
	memset(szFileName, 0x00, sizeof(szFileName));
	
	double dFileSize = 0;

	char szSize[128];
	memset(szSize, 0x00, sizeof(szSize));

	if(bIsZip)
	{	
		sprintf(szExtHash, "%s", MurekaVInfo.mureka_hash);
		sprintf(szFileName, "%s", MurekaVInfo.filename);
		memcpy(szSize, pFUPS4005.video_hash + 33, strlen(pFUPS4005.video_hash));
		dFileSize = atof(szSize);
	}
	else
	{
		sprintf(szExtHash, "%s%.0f", pFUPS4005.default_hash, pFUPS4005.file_size);
		//sprintf(szFileName, "%s", pFUPS4005.file_name);
		ReplaceSingleQuotation(szFileName, '\'',pFUPS4005.file_name);
		dFileSize = pFUPS4005.file_size;
	}	
	//--------------------------------------------------------------------------
	
	
	
	//--------------------------------------------------------------------------
	//성인 유무 설정
	//--------------------------------------------------------------------------
	char szAdultYn[1+1];
	memset(szAdultYn, 0x00, sizeof(szAdultYn));
	
	if(strcmp(MurekaVInfo.video_grade, "18") == 0)
		strcpy(szAdultYn, "Y");
	else
		strcpy(szAdultYn, "N");
	//--------------------------------------------------------------------------
	
	
	
	//--------------------------------------------------------------------------
	//권리사코드 조회
	//--------------------------------------------------------------------------
	char szCompCd[6+1];
	memset(szCompCd, 0x00, sizeof(szCompCd));

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "select comp_cd from nori_cpr.T_CPR_COMP_INFO where comp_nm = '%s'", MurekaVInfo.video_right_name);

	if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		infLOG(ERROR, "fups40051->FlogInsertFilterVideo : MysqlQuery error.\n");
		infLOG(ERROR, "fups40051->FlogInsertFilterVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
		return -1;
	}
	if (!(cpr_res = mysql_store_result(cpr_con)))
	{
		infLOG(ERROR, "fups40051->FlogInsertFilterVideo : mysql_store_result error.\n");
		infLOG(ERROR, "fups40051->FlogInsertFilterVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
		mysql_free_result(cpr_res);
		return -1;
	}
	if (mysql_num_rows(cpr_res) == 0 )
 	{
		infLOG(ALWAY, "fups40051->FlogInsertFilterVideo : 권리사 정보 없음.\n");
		infLOG(ALWAY, "fups40051->FlogInsertFilterVideo : [%s]\n", szQuery);
	}
	else
	{	
		cpr_row = mysql_fetch_row(cpr_res);
		strcpyA(szCompCd, getstr(cpr_row, 0));
	}
	mysql_free_result(cpr_res);
	//--------------------------------------------------------------------------


	//--------------------------------------------------------------------------
	//트렌젝션 시작
	//--------------------------------------------------------------------------
	if (tran_begin(cpr_con)!=0)
	{
		infLOG(ERROR, "fups4005->FlogInsertFilterVideo : 트렌젝션 오류.\n");
		infLOG(ERROR, "fups4005->FlogInsertFilterVideo : [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		return -1;
	}
	//--------------------------------------------------------------------------

	
	//--------------------------------------------------------------------------
	//필터 목록 조회
	//--------------------------------------------------------------------------
	unsigned ulListId = 0;
	bool bIsInsert = false;
	bool bIsUpdate = false;
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select a.seq_no, b.list_id, b.status "
					 " from nori_cpr.T_FILTER_HASH_FILE a, nori_cpr.T_FILTER_CONT_LIST b "
					 " where a.default_hash_ext = '%s' and a.list_id = b.list_id  "
					 , szExtHash);

	if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		infLOG(ERROR, "fups40051->FlogInsertFilterVideo : MysqlQuery error.\n");
		infLOG(ERROR, "fups40051->FlogInsertFilterVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
		tran_rollback(cpr_con);
		tran_end(cpr_con);
		return -1;
	}
	if (!(cpr_res = mysql_store_result(cpr_con)))
	{
		infLOG(ERROR, "fups40051->FlogInsertFilterVideo : mysql_store_result error.\n");
		infLOG(ERROR, "fups40051->FlogInsertFilterVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
		tran_rollback(cpr_con);
		tran_end(cpr_con);
		mysql_free_result(cpr_res);
		return -1;
	}
	if (mysql_num_rows(cpr_res) == 0 )
 	{
		mysql_free_result(cpr_res);
		infLOG(ALWAY, "fups40051->FlogInsertFilterVideo : 필터 목록에 없음.\n");
		infLOG(ALWAY, "fups40051->FlogInsertFilterVideo : [%s]\n", szQuery);
		
		if(strlen(MurekaVInfo.video_right_content_id) > 0)
		{

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " select status, list_id "
							 " from nori_cpr.T_FILTER_CONT_LIST "
							 " where mgr_cd = '%lu' "
							 , MurekaVInfo.video_right_content_id);
			if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				infLOG(ERROR, "fups40051->FlogInsertFilterVideo : MysqlQuery error.\n");
				infLOG(ERROR, "fups40051->FlogInsertFilterVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
				tran_rollback(cpr_con);
				tran_end(cpr_con);
				return -1;
			}
			if (!(cpr_res = mysql_store_result(cpr_con)))
			{
				infLOG(ERROR, "fups40051->FlogInsertFilterVideo : mysql_store_result error.\n");
				infLOG(ERROR, "fups40051->FlogInsertFilterVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
				tran_rollback(cpr_con);
				tran_end(cpr_con);
				return -1;
			}
			if (mysql_num_rows(cpr_res) == 0 )
		 	{
				mysql_free_result(cpr_res);
				bIsInsert = true;
			}
			else
			{
				cpr_row = mysql_fetch_row(cpr_res);
				char szStatus[1+1];
				memset(szStatus, 0x00, sizeof(szStatus));
				sprintf(szStatus, "%s", getstr(cpr_row, 0));
				ulListId = (unsigned long)getnum(cpr_row,1);
				mysql_free_result(cpr_res);
				
				if(strcmp(szStatus, "C") != 0)
				{
					bIsUpdate = true;
				}
			}	
		}
		else
		{
			bIsInsert = true;
		}		
		
		if(bIsInsert == true || bIsUpdate == true)
		{
			memset(szQuery, 0x00, sizeof(szQuery));
			if(bIsUpdate)
			{
				//--------------------------------------------------------------------------
				//차단 컨텐츠 목록 상태값 차단으로 변경
				//--------------------------------------------------------------------------
				sprintf(szQuery, " update nori_cpr.T_FILTER_CONT_LIST "
								 " set status = 'C' "
								 " where list_id = '%lu' "
								 , ulListId);
				//--------------------------------------------------------------------------
			}
			
			else if(bIsInsert)
			{
				//--------------------------------------------------------------------------
				//차단 컨텐츠 목록 등록
				//--------------------------------------------------------------------------
				sprintf(szQuery, " INSERT INTO nori_cpr.T_FILTER_CONT_LIST "
								 " (title, mgr_cd, mgr_ext, sect_code, adult_yn, grade, status, chapter, open_date, reg_user, reg_date, reg_time, copyrighter) "
								 " VALUES "
								 " ('%s', '%s', '%s%s', '%s', '%s', '%s', 'C', %s, '%s', 'system', '%s', '%s', '%s') "
								 , MurekaVInfo.video_title
								 , MurekaVInfo.video_right_content_id
								 , MurekaVInfo.video_right_content_id, szCompCd
								 , pFUPS4005.sect_code
								 , szAdultYn
								 , MurekaVInfo.video_grade
								 , MurekaVInfo.video_cha
								 , MurekaVInfo.video_onair_date
								 , pRegDate
								 , pRegTime
								 , MurekaVInfo.video_right_name);
				//--------------------------------------------------------------------------
			}
			else
			{
				infLOG(ERROR, "fups40051->FlogInsertFilterVideo : flag error.(%d--%d)\n", bIsUpdate, bIsInsert );
				return -1;
			}	
			if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				infLOG(ERROR, "fups40051->FlogInsertFilterVideo : MysqlQuery error.\n");
				infLOG(ERROR, "fups40051->FlogInsertFilterVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
				tran_rollback(cpr_con);
				tran_end(cpr_con);
				return -1;
			}
			if(bIsInsert)
				ulListId = mysql_insert_id(cpr_con);
		}
		

		//--------------------------------------------------------------------------
		//파일 해쉬 정보 등록
		//--------------------------------------------------------------------------
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " INSERT INTO nori_cpr.T_FILTER_HASH_FILE "
						 " (list_id, default_hash, audio_hash, video_hash, default_hash_ext, file_name, file_size, reg_user, reg_date, reg_time) "
						 " VALUES "
						 " (%lu, '%s', '%s', '%s', '%s', '%s', %.0f, 'system', '%s', '%s') "
						 , ulListId, pFUPS4005.default_hash, pFUPS4005.audio_hash, MurekaVInfo.mureka_hash, szExtHash, szFileName, dFileSize, pRegDate, pRegTime);
		if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
		{
			infLOG(ERROR, "fups40051->FlogInsertFilterVideo : MysqlQuery error.\n");
			infLOG(ERROR, "fups40051->FlogInsertFilterVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return -1;
		}
		//--------------------------------------------------------------------------
		
	}
	else
	{
		cpr_row = mysql_fetch_row(cpr_res);
		ulListId = (unsigned long)getnum(cpr_row,1);
		char szStatus[1+1];
		memset(szStatus, 0x00, sizeof(szStatus));
		sprintf(szStatus, "%s", getstr(cpr_row, 2));
		mysql_free_result(cpr_res);
		
		if(strcmp(szStatus, "C") != 0)
		{
			//--------------------------------------------------------------------------
			//차단으로 상태값 변경
			//--------------------------------------------------------------------------
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update nori_cpr.T_FILTER_CONT_LIST "
							 " set status = 'C' "
							 " where list_id = '%s' "
							 , ulListId);
			if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				infLOG(ERROR, "fups40051->FlogInsertFilterVideo : MysqlQuery error.\n");
				infLOG(ERROR, "fups40051->FlogInsertFilterVideo : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
				tran_rollback(cpr_con);
				tran_end(cpr_con);
				return -1;
			}
		}
	}	

	//--------------------------------------------------------------------------
	//트렌젝션 적용
	//--------------------------------------------------------------------------
	if (tran_commit(cpr_con)!=0)
	{
		infLOG(ERROR, "fups4005->FlogInsertFilterVideo : 트렌젝션 commit 오류.\n");
		infLOG(ERROR, "fups4005->FlogInsertFilterVideo : [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		tran_rollback(cpr_con);
		tran_end(cpr_con);
		return -1;
	}

	tran_end(cpr_con);
	//--------------------------------------------------------------------------

	#ifdef __DEBUG
	printf("\n\n\n\n\ndcmdfups4006 FlogInsertFilterVideo() 완료\n\n\n\n");
	#endif
	
	return 0;
}

/*****************************************************************************
* 필터 데이터 등록(음악)
* (I) 1. DB connet 정보(nori_cpr)
*     2. 파일 정보
*     3. 뮤레카 정보
*	  4. 기타 정보(등록일시, 압축 여부)
* (R) 1. 0		: 정상
*	  2. 음수	: 오류
*****************************************************************************/
int FlogInsertFilterMusic(MYSQL *cpr_con, CMysqlCon MysqlCprCon
					, CFUPS4005 pFUPS4005
					, MUREKA_VINFO MurekaVInfo
					, char* pRegDate, char* pRegTime, bool bIsZip)
{
	MYSQL_RES   *cpr_res;
	MYSQL_ROW    cpr_row;

	char szSysErrMsg[255];
	memset(szSysErrMsg,0x00,sizeof(szSysErrMsg));

	char szQuery[15000];
	memset(szQuery, 0x00, sizeof(szQuery));
	

	//--------------------------------------------------------------------------
	//압축 파일일 경우 확장해시는 뮤레카 해시로 대체
	//--------------------------------------------------------------------------
	char szExtHash[50+1];
	memset(szExtHash, 0x00, sizeof(szExtHash));
	
	char szFileName[512+1];
	memset(szFileName, 0x00, sizeof(szFileName));
	
	double dFileSize = 0;

	char szSize[128];
	memset(szSize, 0x00, sizeof(szSize));

	if(bIsZip)
	{	
		sprintf(szExtHash, "%s", MurekaVInfo.mureka_hash);
		sprintf(szFileName, "%s", MurekaVInfo.filename);
		memcpy(szSize, pFUPS4005.video_hash + 33, strlen(pFUPS4005.video_hash));
		dFileSize = atof(szSize);
	}
	else
	{
		sprintf(szExtHash, "%s%.0f", pFUPS4005.default_hash, pFUPS4005.file_size);
		sprintf(szFileName, "%s", pFUPS4005.file_name);
		dFileSize = pFUPS4005.file_size;
	}	
	//--------------------------------------------------------------------------
	
	char szTitle[250+1];
	memset(szTitle, 0x00, sizeof(szTitle));
	if(strlen(MurekaVInfo.music_title) <= 0)
		strcpy(szTitle, szFileName);
	else
		strcpy(szTitle, MurekaVInfo.music_album);
		
	char szCopyRighter[250+1];
	memset(szCopyRighter, 0x00, sizeof(szCopyRighter));
	if(strlen(MurekaVInfo.music_injeob_com) <= 0)
		strcpy(szCopyRighter, MurekaVInfo.music_artist);
	else
		strcpy(szCopyRighter, MurekaVInfo.music_injeob_com);
	
	
	
	//--------------------------------------------------------------------------
	//성인 유무 설정
	//--------------------------------------------------------------------------
	char szAdultYn[1+1];
	memset(szAdultYn, 0x00, sizeof(szAdultYn));
	strcpy(szAdultYn, "N");
	//--------------------------------------------------------------------------
	
	
	
	//--------------------------------------------------------------------------
	//권리사코드 조회
	//--------------------------------------------------------------------------
	char szCompCd[6+1];
	memset(szCompCd, 0x00, sizeof(szCompCd));

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "select comp_cd from nori_cpr.T_CPR_COMP_INFO where comp_nm = '%s'", MurekaVInfo.music_injeob_com);

	if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		infLOG(ERROR, "fups40051->FlogInsertFilterMusic : MysqlQuery error.\n");
		infLOG(ERROR, "fups40051->FlogInsertFilterMusic : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
		return -1;
	}
	if (!(cpr_res = mysql_store_result(cpr_con)))
	{
		infLOG(ERROR, "fups40051->FlogInsertFilterMusic : mysql_store_result error.\n");
		infLOG(ERROR, "fups40051->FlogInsertFilterMusic : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
		mysql_free_result(cpr_res);
		return -1;
	}
	if (mysql_num_rows(cpr_res) == 0 )
 	{
		infLOG(ALWAY, "fups40051->FlogInsertFilterMusic : 권리사 정보 없음.\n");
		infLOG(ALWAY, "fups40051->FlogInsertFilterMusic : [%s]\n", szQuery);
	}
	else
	{	
		cpr_row = mysql_fetch_row(cpr_res);
		strcpyA(szCompCd, getstr(cpr_row, 0));
	}
	mysql_free_result(cpr_res);
	//--------------------------------------------------------------------------


	//--------------------------------------------------------------------------
	//트렌젝션 시작
	//--------------------------------------------------------------------------
	if (tran_begin(cpr_con)!=0)
	{
		infLOG(ERROR, "fups4005->FlogInsertFilterMusic : 트렌젝션 오류.\n");
		infLOG(ERROR, "fups4005->FlogInsertFilterMusic : [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		return -1;
	}
	//--------------------------------------------------------------------------

	
	//--------------------------------------------------------------------------
	//필터 목록 조회
	//--------------------------------------------------------------------------
	unsigned ulListId = 0;
	bool bIsInsert = false;
	bool bIsUpdate = false;
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select a.seq_no, b.list_id, b.status "
					 " from nori_cpr.T_FILTER_HASH_FILE a, nori_cpr.T_FILTER_CONT_LIST b "
					 " where a.default_hash_ext = '%s' and a.list_id = b.list_id  "
					 , szExtHash);

	if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		infLOG(ERROR, "fups40051->FlogInsertFilterMusic : MysqlQuery error.\n");
		infLOG(ERROR, "fups40051->FlogInsertFilterMusic : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
		tran_rollback(cpr_con);
		tran_end(cpr_con);
		return -1;
	}
	if (!(cpr_res = mysql_store_result(cpr_con)))
	{
		infLOG(ERROR, "fups40051->FlogInsertFilterMusic : mysql_store_result error.\n");
		infLOG(ERROR, "fups40051->FlogInsertFilterMusic : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
		tran_rollback(cpr_con);
		tran_end(cpr_con);
		mysql_free_result(cpr_res);
		return -1;
	}
	if (mysql_num_rows(cpr_res) == 0 )
 	{
		mysql_free_result(cpr_res);
		infLOG(ALWAY, "fups40051->FlogInsertFilterMusic : 필터 목록에 없음.\n");
		infLOG(ALWAY, "fups40051->FlogInsertFilterMusic : [%s]\n", szQuery);
		
		if(strlen(MurekaVInfo.music_album) > 0)
		{

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " select status, list_id "
							 " from nori_cpr.T_FILTER_CONT_LIST "
							 " where mgr_cd = '%s' "
							 , MurekaVInfo.music_album);
			if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				infLOG(ERROR, "fups40051->FlogInsertFilterMusic : MysqlQuery error.\n");
				infLOG(ERROR, "fups40051->FlogInsertFilterMusic : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
				tran_rollback(cpr_con);
				tran_end(cpr_con);
				return -1;
			}
			if (!(cpr_res = mysql_store_result(cpr_con)))
			{
				infLOG(ERROR, "fups40051->FlogInsertFilterMusic : mysql_store_result error.\n");
				infLOG(ERROR, "fups40051->FlogInsertFilterMusic : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
				tran_rollback(cpr_con);
				tran_end(cpr_con);
				return -1;
			}
			if (mysql_num_rows(cpr_res) == 0 )
		 	{
				mysql_free_result(cpr_res);
				bIsInsert = true;
			}
			else
			{
				cpr_row = mysql_fetch_row(cpr_res);
				char szStatus[1+1];
				memset(szStatus, 0x00, sizeof(szStatus));
				sprintf(szStatus, "%s", getstr(cpr_row, 0));
				ulListId = (unsigned long)getnum(cpr_row,1);
				mysql_free_result(cpr_res);
				
				if(strcmp(szStatus, "C") != 0)
				{
					bIsUpdate = true;
				}
			}	
		}
		else
		{
			bIsInsert = true;
		}		
		
		if(bIsInsert == true || bIsUpdate == true)
		{
			memset(szQuery, 0x00, sizeof(szQuery));
			if(bIsUpdate)
			{
				//--------------------------------------------------------------------------
				//차단 컨텐츠 목록 상태값 차단으로 변경
				//--------------------------------------------------------------------------
				sprintf(szQuery, " update nori_cpr.T_FILTER_CONT_LIST "
								 " set status = 'C' "
								 " where list_id = '%lu' "
								 , ulListId);
				//--------------------------------------------------------------------------
			}
			
			else if(bIsInsert)
			{
				//--------------------------------------------------------------------------
				//차단 컨텐츠 목록 등록
				//--------------------------------------------------------------------------
				sprintf(szQuery, " INSERT INTO nori_cpr.T_FILTER_CONT_LIST "
								 " (title, mgr_cd, mgr_ext, sect_code, adult_yn, status, reg_user, reg_date, reg_time, copyrighter) "
								 " VALUES "
								 " ('%s', '%s', '%s%s', '%s', '%s', 'C', 'system', '%s', '%s', '%s') "
								 , szTitle
								 , MurekaVInfo.music_id
								 , MurekaVInfo.music_id, szCompCd
								 , pFUPS4005.sect_code
								 , szAdultYn
								 , pRegDate
								 , pRegTime
								 , szCopyRighter);
				//--------------------------------------------------------------------------
			}
			else
			{
				infLOG(ERROR, "fups40051->FlogInsertFilterMusic : flag error.(%d--%d)\n", bIsUpdate, bIsInsert );
				return -1;
			}	
			if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				infLOG(ERROR, "fups40051->FlogInsertFilterMusic : MysqlQuery error.\n");
				infLOG(ERROR, "fups40051->FlogInsertFilterMusic : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
				tran_rollback(cpr_con);
				tran_end(cpr_con);
				mysql_free_result(cpr_res);
				return -1;
			}
			if(bIsInsert)
				ulListId = mysql_insert_id(cpr_con);
		}
		

		//--------------------------------------------------------------------------
		//파일 해쉬 정보 등록
		//--------------------------------------------------------------------------
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " INSERT INTO nori_cpr.T_FILTER_HASH_FILE "
						 " (list_id, default_hash, audio_hash, video_hash, default_hash_ext, file_name, file_size, reg_user, reg_date, reg_time) "
						 " VALUES "
						 " (%lu, '%s', '%s', '%s', '%s', '%s', %.0f, 'system', '%s', '%s') "
						 , ulListId, pFUPS4005.default_hash, pFUPS4005.audio_hash, MurekaVInfo.mureka_hash, szExtHash, szFileName, dFileSize, pRegDate, pRegTime);
		if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
		{
			infLOG(ERROR, "fups40051->FlogInsertFilterMusic : MysqlQuery error.\n");
			infLOG(ERROR, "fups40051->FlogInsertFilterMusic : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			mysql_free_result(cpr_res);
			return -1;
		}
		//--------------------------------------------------------------------------
		
	}
	else
	{
		cpr_row = mysql_fetch_row(cpr_res);
		ulListId = (unsigned long)getnum(cpr_row,1);
		char szStatus[1+1];
		memset(szStatus, 0x00, sizeof(szStatus));
		sprintf(szStatus, "%s", getstr(cpr_row, 2));
		mysql_free_result(cpr_res);
		
		if(strcmp(szStatus, "C") != 0)
		{
			//--------------------------------------------------------------------------
			//차단으로 상태값 변경
			//--------------------------------------------------------------------------
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update nori_cpr.T_FILTER_CONT_LIST "
							 " set status = 'C' "
							 " where list_id = '%lu' "
							 , ulListId);
			if(MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				infLOG(ERROR, "fups40051->FlogInsertFilterMusic : MysqlQuery error.\n");
				infLOG(ERROR, "fups40051->FlogInsertFilterMusic : [%d](%s)[%s]\n",mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
				tran_rollback(cpr_con);
				tran_end(cpr_con);
				mysql_free_result(cpr_res);
				return -1;
			}
		}
	}	

	//--------------------------------------------------------------------------
	//트렌젝션 적용
	//--------------------------------------------------------------------------
	if (tran_commit(cpr_con)!=0)
	{
		infLOG(ERROR, "fups4005->FlogInsertFilterMusic : 트렌젝션 commit 오류.\n");
		infLOG(ERROR, "fups4005->FlogInsertFilterMusic : [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		tran_rollback(cpr_con);
		tran_end(cpr_con);
		return -1;
	}

	tran_end(cpr_con);
	//--------------------------------------------------------------------------

	#ifdef __DEBUG
	printf("\n\n\n\n\ndcmdfups4006 FlogInsertFilterMusic() 완료\n\n\n\n");
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
					, CFUPS4005 pFUPS4005
					, LPMUREKA_VINFO pMurekaVInfo, int nMurekaCnt, int nMurekaIndex
					 )
{
	infLOG(ALWAY, "fups4005->InsertMurekaVideo IN | temp_id [ %lu ] seq [ %lu ] \n",pFUPS4005.id,pFUPS4005.seq_no);
	
		
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
		
		if( pMurekaVInfo[i].nFileGubun == 4 ) //바이러스
		{
			sprintf(szQuery,"insert into zangsi.T_CONTFLOG_TEMPLIST_MUREKA "
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
		   infLOG(ALWAY, "pMurekaVInfo[i].video_right_content_id 3[ %s ] \n",pMurekaVInfo[i].video_right_content_id);
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
							  , pFUPS4005.seq_no, pFUPS4005.id ,pMurekaVInfo[i].nFileGubun,pMurekaVInfo[i].nResultCode,pMurekaVInfo[i].video_status
							  ,pMurekaVInfo[i].video_id,pMurekaVInfo[i].video_title,pMurekaVInfo[i].video_jejak_year,pMurekaVInfo[i].video_right_name
							  ,pMurekaVInfo[i].video_right_content_id,pMurekaVInfo[i].video_grade,pMurekaVInfo[i].video_price,pMurekaVInfo[i].video_cha,pMurekaVInfo[i].video_osp_jibun,pMurekaVInfo[i].video_osp_etc
							  ,pMurekaVInfo[i].video_onair_date,pMurekaVInfo[i].video_right_id ,pMurekaVInfo[i].mureka_hash ,pMurekaVInfo[i].filename);			
		   infLOG(ALWAY, "pMurekaVInfo[i].video_right_content_id 4[ %s ] \n",pMurekaVInfo[i].video_right_content_id);
		}
		if( pMurekaVInfo[i].nFileGubun == 4 || pMurekaVInfo[i].nFileGubun == 2 )
		{
			if(MysqlCon.MysqlQuery(con, szSysErrMsg, szQuery))
			{
				infLOG(ERROR, "fups4005->InsertMurekaVideo : MysqlQuery error.\n");
				infLOG(ERROR, "fups4005->InsertMurekaVideo : [%d](%s)[%s]\n",mysql_errno(con), mysql_error(con), szQuery);
			}
		}
	}

	
	infLOG(ALWAY, "fups4005->InsertMurekaVideo OUT | temp_id [ %lu ] seq [ %lu ] \n",pFUPS4005.id,pFUPS4005.seq_no);
	return 0;
}
