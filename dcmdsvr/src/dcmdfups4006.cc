/******************************************************************************
 *   ����ý��� : CMD����
 *   ���α׷��� : fups4006.cc
 *         ��� : ���� ������ ��ȸ �� ���� ����
 *         ���� :
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
#include "../../fupsvr/inc/fups4006.h" //fup server �� fups4006.h �� ����ü�� ����Ѵ�.
#include "dcmdfups4006.h"
#include "dcmdcomlib.h"
#include "mysql_pool.h"

extern CMysqlPool *m_g_clMysqlPool;
extern CMysqlPool *m_g_clMysqlPoolCopyRight;

// 20190124 1m hash
int InsertFileData(MYSQL *con, CMysqlCon MysqlCon, MYSQL *cpr_con, CMysqlCon MysqlCprCon, char *pDefaultHash, char *pFileName, double dwFileSize, char *pFileType, CFUPS4006 pfups4006, bool bIsZip, char *szMurekaStatus, char *pRegDate, char *pRegTime
				   //					, LPMUREKA_VINFO pMurekaVInfo, int nMurekaCnt, int nMurekaIndex);
				   ,
				   LPMUREKA_VINFO pMurekaVInfo, int nMurekaCnt, int nMurekaIndex, CFUPS4006_1M_HASH pFUPSHASH);

int InsertCprData(MYSQL *con, CMysqlCon MysqlCon, MYSQL *cpr_con, CMysqlCon MysqlCprCon, unsigned long ulID, char *pSectCode, char *pSectSub, char *szDefaultHash, char *szFileName, double dwFileSize, MUREKA_VINFO MurekaVInfo);

int UpdateCprData(MYSQL *cpr_con, CMysqlCon MysqlCprCon, char *szDefaultHash, char *szFileName, double dwFileSize, MUREKA_VINFO MurekaVInfo);

int InsertMurekaVideo(MYSQL *con, CMysqlCon MysqlCon, CFUPS4006 pfups4006, LPMUREKA_VINFO pMurekaVInfo, int nMurekaCnt, int nMurekaIndex);

void MurekaLog(int nMurekaCnt, LPMUREKA_VINFO pMurekaVInfo);
void InsertHoldLog(MYSQL *cpr_con, CMysqlCon MysqlCprCon, int nRet, LPCFUPS4006 pfups4006, LPMUREKA_VINFO pMurekaVInfo, char *pRegDate, char *pRegTime);

// 20190123 1mbhash
int Insert1mbHash(MYSQL *con, CMysqlCon MysqlCon, CFUPS4006 pfups4006, char *pRegDate, char *pRegTime, CFUPS4006_1M_HASH pfups4006hash);

void InsertHoldLog(MYSQL *cpr_con, CMysqlCon MysqlCprCon, int nRet, LPCFUPS4006 pfups4006, LPMUREKA_VINFO pMurekaVInfo, char *pRegDate, char *pRegTime)
{
	int err_type = 0;
	char szQuery[10000];
	memset(szQuery, 0x00, sizeof(szQuery));
	char szSysErrMsg[255];
	memset(szSysErrMsg, 0x00, sizeof(szSysErrMsg));

	infLOG(ERROR, "InsertHoldLog Ret(%d)\n", nRet);

	if (nRet == -50)
		err_type = 1; // �Ǹ��� ���� ���� "B"
	if (nRet == -51)
		err_type = 2; // �����ε� ������ ���� "B"
	if (nRet == -52)
		err_type = 3; // �������� �ν�Ʈ ���� "B"
	if (nRet == -53)
		err_type = 4; // �����Ͱ� ����, ��Ŷ ����.
	if (nRet == -54)
		err_type = 5; // ��ȸ ������ 0 �϶�
	if (nRet == -55)
		err_type = 6; // ��ȸ ������ 0 �϶�
	if (nRet == -44)
		err_type = 7; // ��Ÿ ��ȸ ���� "B"

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "insert into zangsi_cpr.T_CONTENTS_UPLOAD_HOLD "
					 " ( err_type, sect_code, default_hash "
					 " ,file_size, mureka_hash, file_gu "
					 " ,mureka_status, result_code, right_content_id "
					 " ,right_name, price_amt, chapter "
					 " ,reg_user, reg_date, reg_time , tmp_id )"
					 " values ( %d ,'%s', '%s', %.0f, '%s', '%d', '%s', '%d', '%s', '%s',%d, %d,'%s','%s', '%s' , %lu ) ; /* dcmd4006 */",
			err_type, pfups4006->sect_code, pfups4006->default_hash, pfups4006->file_size, pMurekaVInfo->mureka_hash, pMurekaVInfo->nFileGubun, pMurekaVInfo->video_status, pMurekaVInfo->nResultCode, pMurekaVInfo->video_right_id, pMurekaVInfo->video_right_name, pMurekaVInfo->video_price, pMurekaVInfo->video_cha, pfups4006->user_id, pRegDate, pRegTime, pfups4006->id);

	infLOG(ALWAY, "szQuery = %s \n", szQuery);

	if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		infLOG(ERROR, "T_CONTENTS_UPLOAD_HOLD Insert ����.\n");
		return;
	}
}

long dcmdfups4006(int sock, LPHEADER pHeader, char *pRecvData, char *&pSendData)
{

	CFUPS4006 pfups4006;
	memset(&pfups4006, 0x00, sizeof(CFUPS4006));
	memcpy(&pfups4006, (LPCFUPS4006)pRecvData, sizeof(CFUPS4006));

	// 20190124 1mb hash
	long recv_body_size = 0;
	recv_body_size = sizeof(CFUPS4006);
	CFUPS4006_1M_HASH Fups1MHash;
	memset(&Fups1MHash, 0x00, sizeof(CFUPS4006_1M_HASH));
	if (pHeader->nCmd == 40062) // hash
	{
		infLOG(ALWAY, "FUPS40062 | cmd 40062 \n");
		memcpy(&Fups1MHash, pRecvData + recv_body_size, sizeof(CFUPS4006_1M_HASH));
		recv_body_size += sizeof(CFUPS4006_1M_HASH);
	}

	infLOG(ALWAY, "FUPS40062 | check hash [ %s ] [ %s ] \n", Fups1MHash.hash_1m, Fups1MHash.hash_1m_mureka);

	HEADER req_header;
	memcpy(&req_header, pHeader, HEADER_SIZE);

	int nMurekaCnt = pfups4006.mureka_cnt;
	LPMUREKA_VINFO pMurekaVInfo = NULL;

	if (nMurekaCnt > 0)
	{
		char *pTempBuffer = new char[sizeof(MUREKA_VINFO) * nMurekaCnt];
		memcpy(pTempBuffer, pRecvData + recv_body_size, sizeof(MUREKA_VINFO) * nMurekaCnt);
		pMurekaVInfo = (LPMUREKA_VINFO)pTempBuffer;
	}

	infLOG(ALWAY, "FUPS4006 | ��� ����[%lu] [%s] [ %d ] \n", pfups4006.id, pfups4006.file_name, nMurekaCnt);

	MurekaLog(nMurekaCnt, pMurekaVInfo);

	//--------------------------------------------------------------------------
	// Wedisk DB
	//--------------------------------------------------------------------------
	bool bDefaultHash = false;
	bool bAudioHash = false;
	bool bVideoHash = false;
	bool bCloseDB = false;
	MYSQL *con = NULL;
	MYSQL_RES *res;
	MYSQL_ROW row;

	//--------------------------------------------------------------------------
	// ���޵��
	//--------------------------------------------------------------------------
	bool bCprCloseDB = false;
	MYSQL *cpr_con = NULL;
	MYSQL_RES *cpr_res;
	MYSQL_ROW cpr_row;
	//--------------------------------------------------------------------------

	int ErrNum = -40061; // error no
	char szQuery[10000], szQuery1[10000];
	memset(szQuery, 0x00, sizeof(szQuery));
	memset(szQuery1, 0x00, sizeof(szQuery1));
	char szSysErrMsg[255];
	memset(szSysErrMsg, 0x00, sizeof(szSysErrMsg));
	char ErrMsg[256];
	memset(ErrMsg, 0x00, sizeof(ErrMsg)); // �����޽���
	char reg_date[8 + 1];
	memset(reg_date, 0x00, sizeof(reg_date));
	char reg_time[6 + 1];
	memset(reg_time, 0x00, sizeof(reg_time));
	char szCompID[6 + 1];
	memset(szCompID, 0x00, sizeof(szCompID));
	char szMurekaStatus[5 + 1];
	memset(szMurekaStatus, 0x00, sizeof(szMurekaStatus)); // (00:����, 01:����, 02:����, 03:Non-License, 04:Unknown)
	char szFileType[12 + 1];
	memset(szFileType, 0x00, sizeof(szFileType));

	//--------------------------------------------------------------------------
	// DB ����
	CMysqlCon MysqlCon(m_g_clMysqlPool, m_g_clMysqlPool->GetPrimaryKey());
	con = MysqlCon.GetMysqlCon();

	if (con == NULL)
	{
		ErrNum--;

		sprintf(ErrMsg, "DB�� �������� �� �Ͽ����ϴ�.\n");
		infLOG(ERROR, "FUPS4006 | GetMysqlCon is null \n");

		int nRetry = 0;
		while (!(con = db_connect(OSP_DB_NAME, OSP_DB_IP_PUB, OSP_DB_DCMD_USER, OSP_DB_DCMD_PASS)) && nRetry < 5)
		{
			nRetry++;
			sleep(1);
			infLOG(ERROR, "FUPS4006 | [%d](%s)\n", ErrNum, ErrMsg);
			// infLOG(ERROR, "FUPS4006 | Mysql Error | Errno : %d | ErrMsg : %s \n", mysql_errno(con),mysql_error(con));
		}
		if (nRetry >= 5)
		{
			req_header.nCmd = -1;
			pSendData = new char[HEADER_SIZE];
			memcpy(pSendData, &req_header, HEADER_SIZE);

			infLOG(ERROR, "FUPS4006 | Cannot DB Connect \n");
			if (pMurekaVInfo)
				delete[] pMurekaVInfo;
			return HEADER_SIZE;
		}
		infLOG(ERROR, "FUPS4006 | GetMysqlCon Direct Connect \n");

		bCloseDB = true;
	}

	//--------------------------------------------------------------------------
	//���� ������ DB ����
	CMysqlCon MysqlCprCon(m_g_clMysqlPoolCopyRight, m_g_clMysqlPoolCopyRight->GetPrimaryKey());
	cpr_con = MysqlCprCon.GetMysqlCon();

	if (cpr_con == NULL)
	{
		ErrNum--;
		sprintf(ErrMsg, "���� ������DB�� �������� �� �Ͽ����ϴ�.\n");
		infLOG(ERROR, "FUPS4006 | MysqlCprCon is null \n");
		int nRetry = 0;
		while (!(cpr_con = db_connect(OSP_CPR_DB_NAME, OSP_CPR_DB_IP_PUB, OSP_DB_DCMD_USER, OSP_DB_DCMD_PASS)) && nRetry < 5)
		{
			nRetry++;
			sleep(1);

			infLOG(ERROR, "FUPS4006 |  MysqlCprCon is null [%d](%s)\n", ErrNum, ErrMsg);
			// infLOG(ERROR, "FUPS4006 | Mysql CPR Error | Errno : %d | ErrMsg : %s \n", mysql_errno(cpr_con),mysql_error(cpr_con));
		}
		if (nRetry >= 5)
		{
			req_header.nCmd = -1;
			pSendData = new char[HEADER_SIZE];
			memcpy(pSendData, &req_header, HEADER_SIZE);

			infLOG(ERROR, "FUPS4006 | Cannot CPR DB Connect \n");
			db_disconnect(con);
			if (pMurekaVInfo)
				delete[] pMurekaVInfo;
			return HEADER_SIZE;
		}
		infLOG(ERROR, "FUPS4006 | GetMysqlCprCon Direct Connect \n");

		bCprCloseDB = true;
	}

	//--------------------------------------------------------------------------
	// ������� ���
	memset(szQuery, 0x00, sizeof(szQuery));
	memset(szQuery1, 0x00, sizeof(szQuery1));

	strcpyA(szQuery, "SELECT date_format(now(),'%Y%m%d')");
	strcat(szQuery, " , date_format(now(),'%H%i%s') ");

	if (MysqlCon.MysqlQuery(con, szSysErrMsg, szQuery))
	{
		ErrNum--;
		sprintf(ErrMsg, "���̺��̽� �����Դϴ�.\n");
		goto fups4006_err;
	}
	if (!(res = mysql_store_result(con)))
	{
		ErrNum--;
		sprintf(ErrMsg, "���̺��̽� �����Դϴ�.\n");
		goto fups4006_err;
	}
	if (mysql_num_rows(res) == 0)
	{
		ErrNum--;
		sprintf(ErrMsg, "�˻��� �ڷᰡ �����ϴ�.\n");
		mysql_free_result(res);
		goto fups4006_err;
	}
	row = mysql_fetch_row(res);

	memset(reg_date, 0x00, sizeof(reg_date));
	memset(reg_time, 0x00, sizeof(reg_time));

	strcpyA(reg_date, getstr(row, 0));
	strcpyA(reg_time, getstr(row, 1));

	mysql_free_result(res);
	//--------------------------------------------------------------------------

	ReplaceSingleQuotation(pfups4006.file_name, '\'', pfups4006.file_name);
	ReplaceSingleQuotation(pfups4006.folder_name, '\'', pfups4006.folder_name);

	memset(szQuery, 0x00, sizeof(szQuery));
	strcpyA(pfups4006.copyright_yn, "N");

	infLOG(ALWAY, "dcmdfups4006> copyright_yn 1 = [%lu][%s]\n", pfups4006.id, pfups4006.copyright_yn);

	infLOG(ALWAY, "FUPS4006 | cont_gu [ %s ] \n", pfups4006.cont_gu);

	if (strcmp(pfups4006.cont_gu, "WE") == 0)
	{
		// 2009/06/15 - HCS : �·�ī ������ ���� �������� �ִ��� �˻�.
		/*
		Ŭ���̾�Ʈ�� ���� �Ѱ� ���� �·�ī ������ ���� �������� �ִٸ�
		zangsi_cpr DB�� ���� �����͸� �����Ѵ�. zangsi DB �� ������ ��ܵ��󿡼� �ϰ� ����.
		�·�ī ������ �ϳ��� ���ٸ� ������Ĵ�� ó��.
		*/
		bool bIsZip = false;

		if (nMurekaCnt > 1)
			bIsZip = true;

		//-----------------------------------------------------------------------/---
		//�·�ī ���͸� ��ģ ���� ���
		//--------------------------------------------------------------------------
		infLOG(ALWAY, "FUPS4006 | nMurekaCnt [ %d ] \n", nMurekaCnt);
		if (nMurekaCnt > 0)
		{
			for (int i = 0; i < 1; i++)
			{
				char szHashValue[32 + 1];
				memset(szHashValue, 0x00, sizeof(szHashValue));
				char szSize[128];
				memset(szSize, 0x00, sizeof(szSize));
				double dwSize = 0;
				memset(szMurekaStatus, 0x00, sizeof(szMurekaStatus));

				//--------------------------------------------------------------------------
				//������ ���͸�
				//--------------------------------------------------------------------------
				infLOG(ALWAY, "dcmdfups4006> �·�ī ���� ��ȸ %d, temp_id=[%lu]\n", i, pfups4006.id);
				infLOG(ALWAY, "dcmdfups4006> nResultCode = [%d]\n", pMurekaVInfo[i].nResultCode);
				infLOG(ALWAY, "dcmdfups4006> nFileGubun = [%d]\n", pMurekaVInfo[i].nFileGubun);
				infLOG(ALWAY, "dcmdfups4006> filename = [%s]\n", pMurekaVInfo[i].filename);
				infLOG(ALWAY, "dcmdfups4006> mureka_hash = [%s]\n", pMurekaVInfo[i].mureka_hash);

				// nFileGubun  ( 0 - ����/������ ���� �ƴ�, 1 - ��������, 2 - ����������, 4 - ���̷���	)
				// nResultCode �˻�����ڵ� ( 0 : ����	1 : ��Ʈ��ũ ����	2 : File Open or Read ����	3 : ���� ����  4 : Ÿ�Ӿƿ� ���� 5 : ������ ��������� ���� (Wedisk) 99 : ��Ÿ ���� )
				if (pMurekaVInfo[i].nFileGubun == 2 || (pMurekaVInfo[i].nFileGubun == 8))
				{
					// strcpy(szFileType, "������");

					if (pMurekaVInfo[i].nResultCode == 0)
					{
						memset(szMurekaStatus, 0x00, sizeof(szMurekaStatus));
						strcpyA(szMurekaStatus, pMurekaVInfo[i].video_status); //  ���� (00:����, 01:����, 02:����, 03:Non-License, 04:Unknown)
					}
					else
					{
						memset(szMurekaStatus, 0x00, sizeof(szMurekaStatus));
						sprintf(szMurekaStatus, "%d", pMurekaVInfo[i].nResultCode);
					}

					memset(pfups4006.video_hash, 0x00, sizeof(pfups4006.video_hash));
					strcpyA(pfups4006.video_hash, pMurekaVInfo[i].mureka_hash);

					if (nMurekaCnt > 1)
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

					ReplaceSingleQuotation(pMurekaVInfo[i].video_title, '\'', pMurekaVInfo[i].video_title);
					ReplaceSingleQuotation(pMurekaVInfo[i].filename, '\'', pMurekaVInfo[i].filename);

					if (strcmp(pMurekaVInfo[i].video_status, "01") == 0) // ���� (00:����, 01:����, 02:����, 03:Non-License, 04:Unknown)
					{
						//���� ������.
						infLOG(ALWAY, "dcmdfups4006> ���� �׸� ����.\n");
						infLOG(ALWAY, "dcmdfups4006> szMurekaStatus (%s).\n", szMurekaStatus);
						infLOG(ALWAY, "dcmdfups4006> �����ڵ� (%s).\n", pMurekaVInfo[i].video_right_content_id);
						infLOG(ALWAY, "dcmdfups4006> ���� (%s)\n", pMurekaVInfo[i].video_title);
						infLOG(ALWAY, "dcmdfups4006> ���޻� (%s).\n", pMurekaVInfo[i].video_right_name);
						infLOG(ALWAY, "dcmdfups4006> �ݾ� (%s).\n", pMurekaVInfo[i].video_price);
						infLOG(ALWAY, "dcmdfups4006> ��� (%s).\n", pMurekaVInfo[i].video_grade);
						infLOG(ALWAY, "dcmdfups4006> ���� (%s).\n", pMurekaVInfo[i].video_cha);

						if (pfups4006.file_size > 1024 * 1024 * 10 || (pMurekaVInfo[i].nFileGubun == 8 && pfups4006.file_size != 0)) // 20100429 - HCS : 10M ������ ���� ������ ������.
						{
							char szSectSub[2 + 1];
							//--------------------------------------------------------------------------
							//�Һз� �Ϲ��ڵ尪 ��ȸ
							//--------------------------------------------------------------------------
							memset(szQuery, 0x00, sizeof(szQuery));
							sprintf(szQuery, " select minor_code from zangsi.T_MINOR_CODE "
											 " where major_code = '9%s' and minor_name = '�Ϲ�' ",
									pfups4006.sect_code);

							if (MysqlCon.MysqlQuery(con, szSysErrMsg, szQuery))
							{
								ErrNum--;
								sprintf(ErrMsg, "T_MINOR_CODE ��ȸ �����Դϴ�.\n");
								goto fups4006_err;
							}
							if (!(res = mysql_store_result(con)))
							{
								ErrNum--;
								sprintf(ErrMsg, "T_MINOR_CODE �����Դϴ�.\n");
								goto fups4006_err;
							}
							if (mysql_num_rows(res) == 0)
							{
								memset(szSectSub, 0x00, sizeof(szSectSub));
								strcpy(szSectSub, "99");
							}
							else
							{
								row = mysql_fetch_row(res);
								memset(szSectSub, 0x00, sizeof(szSectSub));
								strcpyA(szSectSub, getstr(row, 0));
							}
							mysql_free_result(res);
							//--------------------------------------------------------------------------

							//--------------------------------------------------------------------------
							//���� ���� ��Ϲ� ���� ó��
							//--------------------------------------------------------------------------
							int nRet = InsertCprData(con, MysqlCon, cpr_con, MysqlCprCon, pfups4006.id, pfups4006.sect_code, szSectSub, szHashValue, pfups4006.file_name, dwSize, pMurekaVInfo[i]); //,szEventStatus);
							infLOG(ALWAY, "dcmdfups4006> InsertCprData - result [ %d ]\n", nRet);

							if (nRet < 0)
							{
								InsertHoldLog(cpr_con, MysqlCprCon, nRet, &pfups4006, &pMurekaVInfo[i], reg_date, reg_time);

								strcpyA(ErrMsg, "���� ������ �μ�Ʈ ����.");
								infLOG(ERROR, "%s (%d)\n", ErrMsg, nRet);
								strcpy(pfups4006.copyright_yn, "B");
							}
							else if (nRet == 5)
								strcpy(szMurekaStatus, "30");
							else if (nRet == 6)
								strcpy(szMurekaStatus, "31");
							else if (nRet == 7)
								strcpy(szMurekaStatus, "32");
							else if (nRet == 8)
								strcpy(szMurekaStatus, "33");
							else if (nRet == 9)
								strcpy(szMurekaStatus, "34");
							else if (nRet == 10)
								strcpy(szMurekaStatus, "35");
							else if (nRet == 13)
								strcpy(szMurekaStatus, "38");
							else if (nRet == 14)
								strcpy(szMurekaStatus, "39");
							else if (nRet == 15)
								strcpy(szMurekaStatus, "40");
							else if (nRet == 16)
								strcpy(szMurekaStatus, "41");
							else if (nRet == 17)
								strcpy(szMurekaStatus, "42");

							//--------------------------------------------------------------------------
						}
					}
					else if (strcmp(pMurekaVInfo[i].video_status, "00") == 0)
					{ // ���� ������.
						infLOG(ALWAY, "dcmdfups4006> ���� �׸� ����.\n");
						infLOG(ALWAY, "dcmdfups4006> szMurekaStatus (%s).\n", szMurekaStatus);
						infLOG(ALWAY, "dcmdfups4006> �����ڵ� (%s).\n", pMurekaVInfo[i].video_right_content_id);
						infLOG(ALWAY, "dcmdfups4006> ���޻� (%s).\n", pMurekaVInfo[i].video_right_name);
						infLOG(ALWAY, "dcmdfups4006> video id (%s).\n", pMurekaVInfo[i].video_id);

						if (pfups4006.file_size > 1024 * 1024 * 10)
						{
							//--------------------------------------------------------------------------
							//���� ���� ��� ó�� (���� ��ȯ)
							//--------------------------------------------------------------------------
							int nRet = UpdateCprData(cpr_con, MysqlCprCon, pfups4006.default_hash, pfups4006.file_name, pfups4006.file_size, pMurekaVInfo[i]);
							if (nRet < 0)
							{
								infLOG(ERROR, "dcmdfups4006>UpdateCprData ERROR (nRet).\n", nRet);
							}
						}
					}
					else
					{
						// infLOG(ALWAY,"dcmdfups4006> ������� | mureka ��� [%s].\n",pMurekaVInfo[i].video_status);
					}
				}
				else if (pMurekaVInfo[i].nFileGubun == 0 && pMurekaVInfo[i].nResultCode != 0 && pMurekaVInfo[i].nResultCode <= 99)
				{
					infLOG(ALWAY, "dcmdfups4006> ������� result [ %d ]  | pMurekaVInfo[i].nFileGubun == [ %d ] && pMurekaVInfo[i].nResultCode != 0 && pMurekaVInfo[i].nResultCode <= 99\n", pMurekaVInfo[i].nResultCode, pMurekaVInfo[i].nFileGubun);

					memset(szMurekaStatus, 0x00, sizeof(szMurekaStatus));
					sprintf(szMurekaStatus, "%d", pMurekaVInfo[i].nResultCode);
				}
				else if (pMurekaVInfo[i].nResultCode != 0 && pMurekaVInfo[i].nResultCode != 99 && pMurekaVInfo[i].nResultCode != 96 && pMurekaVInfo[i].nResultCode != 97)
				{
					infLOG(ALWAY, "dcmdfups4006> ������� | pMurekaVInfo[i].nFileGubun == [ %d ] \n", pMurekaVInfo[i].nFileGubun);
					memset(szMurekaStatus, 0x00, sizeof(szMurekaStatus));
					// sprintf(szMurekaStatus, "%d", pMurekaVInfo[i].nResultCode);
					strcpy(szMurekaStatus, "2");
				}
				else
				{
					infLOG(ALWAY, "dcmdfups4006> ������� result [ %d ]  | pMurekaVInfo[i].nFileGubun == [ %d ] \n", pMurekaVInfo[i].nResultCode, pMurekaVInfo[i].nFileGubun);
				}

				//--------------------------------------------------------------------------
				//�������� ���
				//--------------------------------------------------------------------------
				if (bIsZip)
				{
					if (i == 0)
					{
						int nResult = InsertFileData(con, MysqlCon, cpr_con, MysqlCprCon, "", "", 0, szFileType, pfups4006, false, szMurekaStatus, reg_date, reg_time, pMurekaVInfo, nMurekaCnt, i, Fups1MHash);

						if (nResult == -1)
							goto fups4006_err;
						else if (nResult == 1)
						{
							strcpy(pfups4006.copyright_yn, "C");
							infLOG(ALWAY, "dcmdfups4006> copyright_yn 2 = [%lu][%s]\n", pfups4006.id, pfups4006.copyright_yn);
						}
					}
					else if (pMurekaVInfo[i].nFileGubun == 2 || pMurekaVInfo[i].nFileGubun == 1 || pMurekaVInfo[i].nFileGubun == 4 || (pMurekaVInfo[i].nFileGubun == 8))
					{
						int nResult = InsertFileData(con, MysqlCon, cpr_con, MysqlCprCon, szHashValue, pMurekaVInfo[i].filename, dwSize, szFileType, pfups4006, bIsZip, szMurekaStatus, reg_date, reg_time, pMurekaVInfo, nMurekaCnt, i, Fups1MHash);
						if (nResult == -1)
							goto fups4006_err;
						else if (nResult == 1)
						{
							strcpy(pfups4006.copyright_yn, "C");
							infLOG(ALWAY, "dcmdfups4006> copyright_yn 3 = [%lu][%s]\n", pfups4006.id, pfups4006.copyright_yn);
						}
					}
				}
				else
				{
					int nResult = InsertFileData(con, MysqlCon, cpr_con, MysqlCprCon, szHashValue, pMurekaVInfo[i].filename, dwSize, szFileType, pfups4006, bIsZip, szMurekaStatus, reg_date, reg_time, pMurekaVInfo, nMurekaCnt, i, Fups1MHash);
					if (nResult == -1)
						goto fups4006_err;
					else if (nResult == 1)
					{
						strcpy(pfups4006.copyright_yn, "C");
						infLOG(ALWAY, "dcmdfups4006> copyright_yn 4 = [%lu][%s]\n", pfups4006.id, pfups4006.copyright_yn);
					}
				}
				//--------------------------------------------------------------------------

				if (strcmp(pMurekaVInfo[i].video_status, "01") == 0 && atoi(pMurekaVInfo[i].video_price) > 5000)
				{
					strcpy(pfups4006.copyright_yn, "X");
					infLOG(ALWAY, "dcmdfups4006> copyright_yn 5 = [%lu][%s]\n", pfups4006.id, pfups4006.copyright_yn);
				}
			}
		}

		//--------------------------------------------------------------------------
		//�·�ī ���͸� ��ġ�� ���� ���� ���
		//--------------------------------------------------------------------------
		if (nMurekaCnt <= 0)
		{
			int nResult = InsertFileData(con, MysqlCon, cpr_con, MysqlCprCon, "", "", 0, szFileType, pfups4006, false, szMurekaStatus, reg_date, reg_time, pMurekaVInfo, nMurekaCnt, 0, Fups1MHash);

			if (nResult == -1)
				goto fups4006_err;
			else if (nResult == 1)
			{
				strcpy(pfups4006.copyright_yn, "C");
				infLOG(ALWAY, "dcmdfups4006> copyright_yn 6 = [%lu][%s]\n", pfups4006.id, pfups4006.copyright_yn);
			}
		}
		//--------------------------------------------------------------------------
	}
	else
	{
		infLOG(ALWAY, "FUPS4006 | ��� ����, �߸��� cont_gu �Է� : %s\n", pfups4006.cont_gu);
		goto fups4006_err;
	}

	infLOG(ALWAY, "FUPS4006 | ��� �Ϸ�[%lu]\n", pfups4006.id);

	if (bCloseDB)
		db_disconnect(con);

	if (bCprCloseDB)
		db_disconnect(cpr_con);

	if (strcmp(pfups4006.copyright_yn, "C") == 0 || strcmp(pfups4006.copyright_yn, "X") == 0)
	{
		req_header.nCmd = 1;
	}
	else
	{
		req_header.nCmd = 0;
	}

	pSendData = new char[HEADER_SIZE];
	memcpy(pSendData, &req_header, HEADER_SIZE);

	if (pMurekaVInfo)
		delete[] pMurekaVInfo;

	return HEADER_SIZE;

//------------------------------------------------------------------------------
fups4006_err:

	if (bCloseDB)
		db_disconnect(con);

	if (bCprCloseDB)
		db_disconnect(cpr_con);

	req_header.nCmd = -1;
	pSendData = new char[HEADER_SIZE];
	memcpy(pSendData, &req_header, HEADER_SIZE);

	if (pMurekaVInfo)
		delete[] pMurekaVInfo;

	return HEADER_SIZE;
}
/*****************************************************************************
 * ���� ������ ���
 * (I) 1. DB connet ����(zangsi, zangsi_cpr)
 *     2. ���� ����(�ؽ�, ���ϸ�, ������, ���� Ÿ��[������,����])
 *     3. ������ ����
 *     4. ��Ÿ ����(���࿩��, �·�ī ��� �ڵ�, ����Ͻ�)
 * (R) 1. 0		: �Ϲ� ������
 *	  2. ����	: ����
 *	  3. 1	 	: ���� ������
 *****************************************************************************/
int InsertFileData(MYSQL *con, CMysqlCon MysqlCon, MYSQL *cpr_con, CMysqlCon MysqlCprCon, char *pDefaultHash, char *pFileName, double dwFileSize, char *pFileType, CFUPS4006 pfups4006, bool bIsZip, char *pMurekaStatus, char *pRegDate, char *pRegTime, LPMUREKA_VINFO pMurekaVInfo, int nMurekaCnt, int nMurekaIndex, CFUPS4006_1M_HASH pFUPSHASH)
{

	int nResult = 0;
	MYSQL_RES *cpr_res;
	MYSQL_ROW cpr_row;

	MYSQL_RES *res;
	MYSQL_ROW row;

	char szSysErrMsg[255];
	memset(szSysErrMsg, 0x00, sizeof(szSysErrMsg));

	char szQuery[10000], szQuery1[10000];
	memset(szQuery, 0x00, sizeof(szQuery));
	memset(szQuery1, 0x00, sizeof(szQuery1));

	char szExtHash[50 + 1];
	memset(szExtHash, 0x00, sizeof(szExtHash));
	sprintf(szExtHash, "%s", pfups4006.default_hash);

	char szEventFileType[6 + 1];
	memset(szEventFileType, 0x00, sizeof(szEventFileType));

	if (bIsZip)
	{
		memset(pfups4006.default_hash, 0x00, sizeof(pfups4006.default_hash));
		sprintf(pfups4006.default_hash, "%s", pDefaultHash);

		pfups4006.file_size = dwFileSize;

		memset(pfups4006.file_name, 0x00, sizeof(pfups4006.file_name));
		sprintf(pfups4006.file_name, "%s", pFileName);

		strcpy(szExtHash, pDefaultHash);
	}

#ifdef _DEBUG_
	printf("InsertFileData> �ؽ�   (%s).\n", pfups4006.default_hash);
	printf("InsertFileData> ���ϸ� (%s).\n", pfups4006.file_name);
	printf("InsertFileData> ������ (%.0f).\n", pfups4006.file_size);
#endif

	infLOG(ALWAY, "dcmdfups4006> InsertFileData = [%lu][%s]\n", pfups4006.id, pfups4006.copyright_yn);

	unsigned long lChiID = 0;
	long lPriceAmt = 0;
	long lMobilePriceAmt = 0; // 20130410
	char szCompID[6 + 1];
	memset(szCompID, 0x00, sizeof(szCompID));
	char szMobServiceYn[1 + 1];
	memset(szMobServiceYn, 0x00, sizeof(szMobServiceYn));
	strcpy(szMobServiceYn, "N");

	if (pfups4006.file_size < 0 || pfups4006.default_hash == NULL || strlen(pfups4006.default_hash) <= 0)
		return -53;

	//--------------------------------------------------------------------------
	// ���� ������ ��ȸ
	//--------------------------------------------------------------------------
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select a.chi_id  ,c.price_amt , a.comp_cd  ,c.mgr_cd , c.chapter  ,if( d.price_amt is null , 0 , d.price_amt )  , if( d.apply_yn is null , 'N' , d.apply_yn) "
					 " from zangsi_cpr.T_CPR_HASH_INFO a, zangsi_cpr.T_CPR_HASH_FILE b, zangsi_cpr.T_CPR_CONT_LIST c  "
					 " left outer join zangsi_cpr.T_CPR_MOB_CONT_LIST d on c.mgr_cd = d.mgr_cd and d.apply_yn='Y' " // 20130410
					 " where a.proc_stat = 'C' and a.chi_id = b.chi_id and a.list_id = c.list_id and c.apply_yn = 'Y' and b.file_size = %15.0f and b.default_hash = '%s' limit 1",
			pfups4006.file_size, szExtHash);

	infLOG(ALWAY, "fups4006->InsertFileData : ���� ��ȸ [ %s ] \n", szQuery);
#ifdef __DEBUG
	printf("InsertFileData> query = [%s]\n\n", szQuery);
#endif
	if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		infLOG(ERROR, "fups4006->InsertFileData : MysqlQuery ����.\n");
		infLOG(ERROR, "fups4006->InsertFileData : [%d](%s)[%s]\n", mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
	}
	else
	{
		//���� �������̸�
		if (!(cpr_res = mysql_store_result(cpr_con)))
		{
			infLOG(ERROR, "fups4006->InsertFileData : mysql_store_result ����.\n");
			infLOG(ERROR, "fups4006->InsertFileData : [%d](%s)[%s]\n", mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
		}
		else
		{
			// �Ϲ� ������
			if (mysql_num_rows(cpr_res) > 0)
			{
				char szMgrCd[24];
				memset(szMgrCd, 0x00, sizeof(szMgrCd));
				char szChapter[24];
				memset(szChapter, 0x00, sizeof(szChapter));

				memset(pfups4006.copyright_yn, 0x00, sizeof(pfups4006.copyright_yn));
				strcpy(pfups4006.copyright_yn, "C");
				infLOG(ALWAY, "dcmdfups4006> copyright_yn f1 = [%lu][%s]\n", pfups4006.id, pfups4006.copyright_yn);

				if (cpr_row = mysql_fetch_row(cpr_res))
				{

					if (pMurekaVInfo == NULL)
					{
						infLOG(ERROR, "dcmdfups4006> pMurekaVInfo is NULL \n");
						return -1;
					}
					lChiID = (unsigned long)getnum(cpr_row, 0);
					lPriceAmt = (long)getnum(cpr_row, 1);
					strcpyA(szCompID, getstr(cpr_row, 2));

					strcpy(szMgrCd, getstr(cpr_row, 3));
					strcpy(szChapter, getstr(cpr_row, 4));

					lMobilePriceAmt = (long)getnum(cpr_row, 5); // 20130410
					strcpy(szMobServiceYn, getstr(cpr_row, 6)); // 20130410

					infLOG(ALWAY, "dcmdfups4006> pMurekaVInfo->video_right_name = %s , szCompID = %s \n", pMurekaVInfo->video_right_name, szCompID);
					infLOG(ALWAY, "dcmdfups4006> pMurekaVInfo->video_price  = [%s], lPriceAmt = [%d] \n", pMurekaVInfo->video_price, lPriceAmt);
					infLOG(ALWAY, "dcmdfups4006> pMurekaVInfo->video_right_content_id = [%s], szMgrCd = [%s] \n", pMurekaVInfo->video_right_content_id, szMgrCd);

					////// 20170221
					if (strcmp(pMurekaVInfo->video_right_content_id, "") == 0)
					{
						infLOG(ERROR, "dcmdfups4006> pMurekaVInfo->video_right_content_id is NULL = [%lu][ %s ] \n", pfups4006.id, szMgrCd);

						strcpy(pMurekaVInfo->video_right_content_id, szMgrCd);
						if (atoi(pMurekaVInfo->video_price) == 0)
						{
							infLOG(ERROR, "dcmdfups4006> pMurekaVInfo->video_price = [%d]->[%d] \n", pMurekaVInfo->video_price, lPriceAmt);
							strcpy(pMurekaVInfo->video_price, getstr(cpr_row, 1));
						}
					}
				}
				nResult = 1;

				mysql_free_result(cpr_res);

				// pMurekaVInfo->video_right_name szCompID // ���޻簡 �ٸ����. ��� üũ �ұ�?
				// select * from T_CPR_COMP_INFO a where a.comp_cd = '010022';

				// 20170222 ���� ������Ʈ : ��� ���� ��

				if (atoi(pMurekaVInfo->video_price) != lPriceAmt)
				{
					infLOG(ERROR, "dcmdfups4006> video_price = [%s] , lPriceAmt = [%d] , lMobilePriceAmt = [ %d] \n", pMurekaVInfo->video_price, lPriceAmt, lMobilePriceAmt);

					lPriceAmt = atoi(pMurekaVInfo->video_price);
					lMobilePriceAmt = atoi(pMurekaVInfo->video_price);

					memset(szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, " update zangsi_cpr.T_CPR_HASH_INFO a, zangsi_cpr.T_CPR_HASH_FILE b, zangsi_cpr.T_CPR_CONT_LIST c "
									 " LEFT OUTER JOIN zangsi_cpr.T_CPR_MOB_CONT_LIST d ON c.mgr_cd = d.mgr_cd AND d.apply_yn='Y' "
									 " set a.price_amt = %d, c.price_amt = %d, d.price_amt = %d, a.reg_user = 'dcmd' ,c.reg_user ='dcmd' , c.seq_no = c.seq_no + 1, d.seq_no = d.seq_no + 1, d.reg_user ='dcmd' "
									 " WHERE a.proc_stat = 'C' AND a.chi_id = b.chi_id AND a.list_id = c.list_id AND c.apply_yn = 'Y' AND b.file_size = %15.0f AND b.default_hash = '%s'; ",
							lPriceAmt, lPriceAmt, lPriceAmt, pfups4006.file_size, szExtHash);

					infLOG(ALWAY, "%s", szQuery);

					if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
					{
						infLOG(ERROR, "dcmdfups4006> pfups4006.copyright_yn err 1 = %s\n", pfups4006.copyright_yn);
						memset(pfups4006.copyright_yn, 0x00, sizeof(pfups4006.copyright_yn));
						strcpy(pfups4006.copyright_yn, "B");
						return -52;
					}

					memset(szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, " INSERT INTO zangsi_cpr.T_CPR_CONT_LIST_HIST "
									 " (list_id,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate, udt_user, udt_date,udt_time) "
									 " SELECT c.list_id, c.seq_no ,c.title, c.chapter, c.mgr_cd, c.comp_cd, c.sect_code, c.sect_sub, c.price_amt, c.adult_yn, c.reg_user, c.reg_date, c.reg_time, c.apply_yn, c.cpr_payment_rate, 'dcmd', DATE_FORMAT(NOW(), '%%Y%%m%%d'), DATE_FORMAT(NOW(), '%%H%%i%%s') "
									 " FROM  zangsi_cpr.T_CPR_HASH_INFO a, zangsi_cpr.T_CPR_HASH_FILE b, zangsi_cpr.T_CPR_CONT_LIST c "
									 " WHERE a.proc_stat = 'C' AND a.chi_id = b.chi_id AND a.list_id = c.list_id AND c.apply_yn = 'Y' AND b.file_size = %15.0f AND b.default_hash = '%s'; ",
							pfups4006.file_size, szExtHash);

					if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
					{
						infLOG(ERROR, "dcmdfups4006> T_CPR_CONT_LIST_HIST insert error \n");
						memset(pfups4006.copyright_yn, 0x00, sizeof(pfups4006.copyright_yn));
						// strcpy(pfups4006.copyright_yn, "B");
						return -44;
					}
					//

					memset(szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, " INSERT INTO zangsi_cpr.T_CPR_MOB_CONT_LIST_HIST "
									 " (list_id,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate, udt_user, udt_date,udt_time) "
									 " SELECT c.list_id, c.seq_no ,c.title, c.chapter, c.mgr_cd, c.comp_cd, c.sect_code, c.sect_sub, c.price_amt, c.adult_yn, c.reg_user, c.reg_date, c.reg_time, c.apply_yn, c.cpr_payment_rate, 'dcmd', DATE_FORMAT(NOW(), '%%Y%%m%%d'), DATE_FORMAT(NOW(), '%%H%%i%%s') "
									 " FROM  zangsi_cpr.T_CPR_HASH_INFO a, zangsi_cpr.T_CPR_HASH_FILE b, zangsi_cpr.T_CPR_MOB_CONT_LIST c "
									 " WHERE a.proc_stat = 'C' AND a.chi_id = b.chi_id AND a.list_id = c.list_id AND c.apply_yn = 'Y' AND b.file_size = %15.0f AND b.default_hash = '%s'; ",
							pfups4006.file_size, szExtHash);

					if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
					{
						infLOG(ERROR, "dcmdfups4006> T_CPR_MOB_CONT_LIST_HIST insert error \n");
						memset(pfups4006.copyright_yn, 0x00, sizeof(pfups4006.copyright_yn));
						// strcpy(pfups4006.copyright_yn, "B");
						return -44;
					}
					//
				}

				// 20120612 - event �˻� - �̺�Ʈ�Ͻÿ� ���� ��� ���θ� �Ǵ��Ͽ� pEventStatus �� �߰�

				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, "select cont_type , list_id from zangsi_cpr.T_CPR_EVENT_CONT_LIST where mgr_cd = '%s'  and chapter = %s and use_yn='Y' ", szMgrCd, szChapter);

				infLOG(ALWAY, "%s", szQuery);

				if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
				{
					infLOG(ERROR, "%s", "��ȸ ����.\nszQuery=[%s]\n", szQuery);
					infLOG(ERROR, "fups4006->FlogInsertFileData  [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
					return -44;
				}
				else
				{
					if (!(cpr_res = mysql_store_result(cpr_con)))
					{
						infLOG(ERROR, "%s", "������ ���� ����.\nszQuery=[%s]\n", szQuery);
						infLOG(ERROR, "fups4006->FlogInsertFileData  [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
						// mysql_free_result(cpr_res);
						return -44;
					}
					else
					{
						if (mysql_num_rows(cpr_res) > 0)
						{
							cpr_row = mysql_fetch_row(cpr_res);

							if (getstr(cpr_row, 0) != NULL)
							{

								strcpy(szEventFileType, getstr(cpr_row, 0));
								pMurekaStatus = szEventFileType;
							}

							infLOG(ALWAY, "szEventFileType [ %s ]\n", pMurekaStatus);
						}
						mysql_free_result(cpr_res);
					}
				}
			}
			else
			{
				if (strcmp(pfups4006.auth_num, "CPR") == 0 && pfups4006.file_size > 1024 * 1024 * 1) //���� ���̵�
				{
					memset(pfups4006.copyright_yn, 0x00, sizeof(pfups4006.copyright_yn));
					strcpy(pfups4006.copyright_yn, "C");
					infLOG(ALWAY, "dcmdfups4006> copyright_yn f2 = [%lu][%s]\n", pfups4006.id, pfups4006.copyright_yn);

					strcpy(szCompID, "CPR");
					nResult = 1;
					infLOG(ALWAY, "fups4006->InsertFileData : ���� ���̵� ���. %s -- %s\n", pfups4006.user_id, pfups4006.auth_num);
				}
				else
				{
					// infLOG(ALWAY,"�Ϲ� ������. \n");
					// memset(pfups4006.copyright_yn, 0x00, sizeof(pfups4006.copyright_yn));
					// strcpy(pfups4006.copyright_yn, "B");
					infLOG(ALWAY, "dcmdfups4006> copyright_yn f3 = [%lu][%s]\n", pfups4006.id, pfups4006.copyright_yn);
				}
				mysql_free_result(cpr_res);
			}
		}
	}
	//--------------------------------------------------------------------------

	if (strcmp(pfups4006.folder_yn, "N") == 0)
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "REPLACE INTO zangsi.T_CONTENTS_TEMPLIST           "
						 "       (id            ,seq_no        ,folder_yn  "
						 "       ,file_name     ,file_size     ,file_type  "
						 "       ,reg_user      ,reg_date      ,reg_time		,default_hash , audio_hash , video_hash	,copyright_yn )  "
						 " VALUES (%lu			,%d			   ,'N'              "
						 "			,'%s'			,%13.0f		   ,'2'			"
						 "			,'%s'			,'%s'			   ,'%s'				,'%s' ,'%s' ,'%s' , '%s'	) ",
				pfups4006.id, pfups4006.seq_no, pfups4006.file_name, pfups4006.file_size, pfups4006.user_id, pRegDate, pRegTime, pfups4006.default_hash, pfups4006.audio_hash, pfups4006.video_hash, pfups4006.copyright_yn);

		if (MysqlCon.MysqlQuery(con, szSysErrMsg, szQuery))
		{
			infLOG(ERROR, "fups4006->InsertFileData : MysqlQuery ����.\n");
			infLOG(ERROR, "fups4006->InsertFileData : [%d](%s)[%s]\n", mysql_errno(con), mysql_error(con), szQuery);
			return -44;
		}
		infLOG(ALWAY, "fups4006->InsertFileData [ %s ] \n", szQuery);

		//--------------------------------------------------------------------------
		//�ߺ� ������ ��ȸ
		//--------------------------------------------------------------------------
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "select seq_no, copyright_yn from zangsi.T_CONTENTS_TEMPLIST_SUB           "
						 " where id = %lu and  file_size = %15.0f and default_hash = '%s' and depth = %d and file_name = '%s' and folder_path = '%s' limit 1",
				pfups4006.id, pfups4006.file_size, pfups4006.default_hash, pfups4006.depth, pfups4006.file_name, pfups4006.folder_name);

		bool bFirst = true;
		bool bCprFirst = true;
		unsigned long ulSeqNO = 0;
		char szCprYn[2];
		memset(szCprYn, 0x00, sizeof(szCprYn));

		if (MysqlCon.MysqlQuery(con, szSysErrMsg, szQuery))
		{
			infLOG(ERROR, "fups4006->InsertFileData : MysqlQuery ����.\n");
			infLOG(ERROR, "fups4006->InsertFileData : [%d](%s)[%s]\n", mysql_errno(con), mysql_error(con), szQuery);
			return -44;
		}
		if (!(res = mysql_store_result(con)))
		{
			infLOG(ERROR, "fups4006->InsertFileData : mysql_store_result ����.\n");
			infLOG(ERROR, "fups4006->InsertFileData : [%d](%s)[%s]\n", mysql_errno(con), mysql_error(con), szQuery);
			return -44;
		}
		if (row = mysql_fetch_row(res))
		{
			if (mysql_num_rows(res) > 0)
			{
				bFirst = false;
				ulSeqNO = (unsigned long)getnum(row, 0);
				sprintf(szCprYn, "%s", getstr(row, 1));

				mysql_free_result(res);

				if (strcmp(szCprYn, pfups4006.copyright_yn) != 0)
				{
					memset(szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, "delete from zangsi.T_CONTENTS_TEMPLIST_SUB where seq_no = %lu", ulSeqNO);
					if (MysqlCon.MysqlQuery(con, szSysErrMsg, szQuery))
					{
						infLOG(ERROR, "fups4006->InsertFileData : MysqlQuery ����.\n");
						infLOG(ERROR, "fups4006->InsertFileData : [%d](%s)[%s]\n", mysql_errno(con), mysql_error(con), szQuery);
						return -44;
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
		//�ߺ� �����Ͱ� ���ٸ� �������� ���
		//--------------------------------------------------------------------------
		if (bFirst)
		{

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "insert into zangsi.T_CONTENTS_TEMPLIST_SUB "
							 " ( id , depth, folder_yn, file_type , folder_path , file_name, file_size, reg_user, reg_date,reg_time, default_hash , audio_hash, video_hash, copyright_yn	 ) "
							 " values "
							 " ( %d , %d , '%s'  , '%s'      , '%s'        , '%s'     , %15.0f   , '%s'    , '%s'    ,'%s'    , '%s'         , '%s'      , '%s'      , '%s' )   ",
					pfups4006.id, pfups4006.depth, pfups4006.folder_yn, pMurekaStatus, pfups4006.folder_name, pfups4006.file_name, pfups4006.file_size, pfups4006.user_id, pRegDate, pRegTime, pfups4006.default_hash, pfups4006.audio_hash, pfups4006.video_hash, pfups4006.copyright_yn);

			infLOG(ALWAY, "szQuery = %s\n", szQuery);
#ifdef _DEBUG_
			printf("InsertFileData> szQuery=[%s].\n", szQuery);
#endif
			if (MysqlCon.MysqlQuery(con, szSysErrMsg, szQuery))
			{
				infLOG(ERROR, "fups4006->InsertFileData : MysqlQuery ����.\n");
				infLOG(ERROR, "fups4006->InsertFileData : [%d](%s)[%s]\n", mysql_errno(con), mysql_error(con), szQuery);
				return -44;
			}
			else
			{
				// if( bIsZip == false && nMurekaCnt == 1)
				// 20190123 1mb hash
				Insert1mbHash(con, MysqlCon, pfups4006, pRegDate, pRegTime, pFUPSHASH);
				if (nMurekaCnt > 0)
				{
					sprintf(szQuery, "%s", "SELECT last_insert_id() as filelist_seq");

					if (MysqlCon.MysqlQuery(con, szSysErrMsg, szQuery))
					{
						infLOG(ERROR, "query :[ %s ]\n\n", szQuery);
					}
					else
					{
						if (!(res = mysql_store_result(con)))
						{
							infLOG(ERROR, "query :[ %s ]\n\n", szQuery);
						}
						else
						{
							unsigned long filelist_seq = 0;
							if (row = mysql_fetch_row(res))
							{
								filelist_seq = (unsigned long)getnum(row, 0); //���̹��ӴϺ�����
							}
							mysql_free_result(res);

							if (filelist_seq > 0)
							{
								CFUPS4006 fups4006 = pfups4006;
								fups4006.seq_no = filelist_seq;

								InsertMurekaVideo(con, MysqlCon, fups4006, pMurekaVInfo, nMurekaCnt, nMurekaIndex);
							}
						}
					}
				}
			}
		}
		//--------------------------------------------------------------------------

		infLOG(ALWAY, "pfups4006.copyright_yn v0 [ %s ] \n", pfups4006.copyright_yn);

		//-------------------
		//���� ������ �Է�
		//-------------------
		if (strcmp(pfups4006.copyright_yn, "C") == 0)
		{
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "select seq_no, copyright_yn from zangsi_cpr.T_CONTENTS_TEMPLIST_SUB           "
							 " where id = %lu and depth = %d  and  file_size = %.0f and default_hash = '%s'  limit 1",
					pfups4006.id, pfups4006.depth, pfups4006.file_size, pfups4006.default_hash);

			if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				infLOG(ERROR, "fups4006->InsertFileData : MysqlQuery ����.\n");
				infLOG(ERROR, "fups4006->InsertFileData : [%d](%s)[%s]\n", mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
				return -44;
			}
			if (!(cpr_res = mysql_store_result(cpr_con)))
			{
				infLOG(ERROR, "fups4006->InsertFileData : mysql_store_result ����.\n");
				infLOG(ERROR, "fups4006->InsertFileData : [%d](%s)[%s]\n", mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
				return -44;
			}
			if (cpr_row = mysql_fetch_row(cpr_res))
			{
				if (mysql_num_rows(cpr_res) > 0)
				{
					bCprFirst = false;
					ulSeqNO = (unsigned long)getnum(cpr_row, 0);
					memset(szCprYn, 0x00, sizeof(szCprYn));
					strcpy(szCprYn, getstr(cpr_row, 1));
					if (strcmp(szCprYn, pfups4006.copyright_yn) != 0)
					{
						memset(szQuery, 0x00, sizeof(szQuery));
						sprintf(szQuery, "delete from zangsi_cpr.T_CONTENTS_TEMPLIST_SUB where seq_no = %lu", ulSeqNO);
						if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
						{
							infLOG(ERROR, "fups4006->InsertFileData : MysqlQuery ����.\n");
							infLOG(ERROR, "fups4006->InsertFileData : [%d](%s)[%s]\n", mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
							mysql_free_result(cpr_res);
							return -44;
						}
						bCprFirst = true;
					}
				}
			}
			mysql_free_result(cpr_res);

			if (bCprFirst)
			{
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, "insert into zangsi_cpr.T_CONTENTS_TEMPLIST_SUB           "
								 " ( id, depth,folder_yn, file_type, folder_path , file_name, file_size, reg_user, reg_date, reg_time, default_hash, audio_hash, video_hash, copyright_yn, comp_cd, chi_id, price_amt , mob_price_amt,mob_service_yn) "
								 " values "
								 " ( %d , %d , '%s'  , '%s'    , '%s'        , '%s'     , %13.0f   , '%s'     , '%s'   ,'%s'     , '%s'        , '%s'      , '%s'      , '%s'        , '%s'   , %lu    , %d , %d ,'%s' )   ",
						pfups4006.id, pfups4006.depth, pfups4006.folder_yn, pMurekaStatus, pfups4006.folder_name, pfups4006.file_name, pfups4006.file_size, pfups4006.user_id, pRegDate, pRegTime, pfups4006.default_hash, pfups4006.audio_hash, pfups4006.video_hash, pfups4006.copyright_yn, szCompID, lChiID, lPriceAmt, lMobilePriceAmt, szMobServiceYn); // 20130410

				infLOG(ALWAY, "fups4006->T_CONTENTS_TEMPLIST_SUB [ %s ] \n", szQuery);

				if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
				{
					infLOG(ERROR, "fups4006->InsertFileData : MysqlQuery ����.\n");
					infLOG(ERROR, "fups4006->InsertFileData : [%d](%s)[%s]\n", mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
					return -44;
				}
			}
		}
	}
	else if (strcmp(pfups4006.folder_yn, "Y") == 0)
	{
		if (pfups4006.depth == 0)
		{
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "update zangsi.T_CONTENTS_TEMPLIST           "
							 " set file_size = %.0f, default_hash = '%s' , audio_hash = '%s' , video_hash = '%s' , copyright_yn = '%s' "
							 " WHERE id  =  %lu and reg_user = '%s' and file_name = '%s'          ",
					pfups4006.file_size, pfups4006.default_hash, pfups4006.audio_hash, pfups4006.video_hash, pfups4006.copyright_yn, pfups4006.id, pfups4006.user_id, pfups4006.file_name);

			if (MysqlCon.MysqlQuery(con, szSysErrMsg, szQuery))
			{
				infLOG(ERROR, "fups4006->InsertFileData : MysqlQuery ����.\n");
				infLOG(ERROR, "fups4006->InsertFileData : [%d](%s)[%s]\n", mysql_errno(con), mysql_error(con), szQuery);
				return -44;
			}
			infLOG(ALWAY, "fups4006->InsertFileData [ %s ] \n", szQuery);
		}

		//--------------------------------------------------------------------------
		//�ߺ� ������ ��ȸ
		//--------------------------------------------------------------------------
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "select seq_no, copyright_yn from zangsi.T_CONTENTS_TEMPLIST_SUB           "
						 " where id = %lu and  file_size = %15.0f and default_hash = '%s' and depth = %d and file_name = '%s' and folder_path = '%s'  limit 1",
				pfups4006.id, pfups4006.file_size, pfups4006.default_hash, pfups4006.depth, pfups4006.file_name, pfups4006.folder_name);

		bool bFirst = true;
		bool bCprFirst = true;
		unsigned long ulSeqNO = 0;
		char szCprYn[2];
		memset(szCprYn, 0x00, sizeof(szCprYn));

		if (MysqlCon.MysqlQuery(con, szSysErrMsg, szQuery))
		{
			infLOG(ERROR, "fups4006->InsertFileData : MysqlQuery ����.\n");
			infLOG(ERROR, "fups4006->InsertFileData : [%d](%s)[%s]\n", mysql_errno(con), mysql_error(con), szQuery);
			return -44;
		}
		if (!(res = mysql_store_result(con)))
		{
			infLOG(ERROR, "fups4006->InsertFileData : mysql_store_result ����.\n");
			infLOG(ERROR, "fups4006->InsertFileData : [%d](%s)[%s]\n", mysql_errno(con), mysql_error(con), szQuery);
			return -44;
		}
		if (row = mysql_fetch_row(res))
		{
			if (mysql_num_rows(res) > 0)
			{
				bFirst = false;
				ulSeqNO = (unsigned long)getnum(row, 0);
				sprintf(szCprYn, "%s", getstr(row, 1));
				mysql_free_result(res);

				if (strcmp(szCprYn, pfups4006.copyright_yn) != 0)
				{
					memset(szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, "delete from zangsi.T_CONTENTS_TEMPLIST_SUB where seq_no = %lu", ulSeqNO);
					if (MysqlCon.MysqlQuery(con, szSysErrMsg, szQuery))
					{
						infLOG(ERROR, "fups4006->InsertFileData : MysqlQuery ����.\n");
						infLOG(ERROR, "fups4006->InsertFileData : [%d](%s)[%s]\n", mysql_errno(con), mysql_error(con), szQuery);
						return -44;
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
		//�ߺ� �����Ͱ� ���ٸ� �������� ���
		//--------------------------------------------------------------------------
		if (bFirst)
		{

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "insert into zangsi.T_CONTENTS_TEMPLIST_SUB "
							 " ( id , depth,folder_yn, file_type , folder_path , file_name, file_size, reg_user, reg_date,reg_time, default_hash , audio_hash, video_hash, copyright_yn	 ) "
							 " values "
							 " ( %d , %d  , '%s'  , '%s'      , '%s'        , '%s'     , %15.0f   , '%s'    , '%s'    ,'%s'    , '%s'         , '%s'      , '%s'      , '%s' )   ",
					pfups4006.id, pfups4006.depth, pfups4006.folder_yn, pMurekaStatus, pfups4006.folder_name, pfups4006.file_name, pfups4006.file_size, pfups4006.user_id, pRegDate, pRegTime, pfups4006.default_hash, pfups4006.audio_hash, pfups4006.video_hash, pfups4006.copyright_yn);
			infLOG(ALWAY, "szQuery 3= %s\n", szQuery);
#ifdef _DEBUG_
			printf("InsertFileData> szQuery=[%s].\n", szQuery);
#endif
			if (MysqlCon.MysqlQuery(con, szSysErrMsg, szQuery))
			{
				infLOG(ERROR, "fups4006->InsertFileData : MysqlQuery ����.\n");
				infLOG(ERROR, "fups4006->InsertFileData : [%d](%s)[%s]\n", mysql_errno(con), mysql_error(con), szQuery);
				return -44;
			}
			else
			{
				// if( bIsZip == false && nMurekaCnt == 1)
				// 20190123 1mb hash
				Insert1mbHash(con, MysqlCon, pfups4006, pRegDate, pRegTime, pFUPSHASH);

				if (nMurekaCnt > 0)
				{
					sprintf(szQuery, "%s", "SELECT last_insert_id() as filelist_seq");

					if (MysqlCon.MysqlQuery(con, szSysErrMsg, szQuery))
					{
						infLOG(ERROR, "query :[ %s ]\n\n", szQuery);
					}
					else
					{
						if (!(res = mysql_store_result(con)))
						{
							infLOG(ERROR, "query :[ %s ]\n\n", szQuery);
						}
						else
						{
							unsigned long filelist_seq = 0;
							if (row = mysql_fetch_row(res))
							{
								filelist_seq = (unsigned long)getnum(row, 0); //���̹��ӴϺ�����
							}
							mysql_free_result(res);

							if (filelist_seq > 0)
							{
								CFUPS4006 fups4006 = pfups4006;
								fups4006.seq_no = filelist_seq;
								InsertMurekaVideo(con, MysqlCon, fups4006, pMurekaVInfo, nMurekaCnt, nMurekaIndex);
							}
						}
					}
				}
			}
		}
		//--------------------------------------------------------------------------

		infLOG(ALWAY, "pfups4006.copyright_yn v1 [ %s ] \n", pfups4006.copyright_yn);

		//-------------------
		//���� ������ �Է�
		//-------------------
		if (strcmp(pfups4006.copyright_yn, "C") == 0)
		{
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "select seq_no, copyright_yn from zangsi_cpr.T_CONTENTS_TEMPLIST_SUB           "
							 " where id = %lu and depth = %d  and  file_size = %.0f and default_hash = '%s'  limit 1",
					pfups4006.id, pfups4006.depth, pfups4006.file_size, pfups4006.default_hash);

			if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				infLOG(ERROR, "fups4006->InsertFileData : MysqlQuery ����.\n");
				infLOG(ERROR, "fups4006->InsertFileData : [%d](%s)[%s]\n", mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
				return -44;
			}
			if (!(cpr_res = mysql_store_result(cpr_con)))
			{
				infLOG(ERROR, "fups4006->InsertFileData : mysql_store_result ����.\n");
				infLOG(ERROR, "fups4006->InsertFileData : [%d](%s)[%s]\n", mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
				return -44;
			}
			if (cpr_row = mysql_fetch_row(cpr_res))
			{
				if (mysql_num_rows(cpr_res) > 0)
				{
					bCprFirst = false;
					ulSeqNO = (unsigned long)getnum(cpr_row, 0);
					memset(szCprYn, 0x00, sizeof(szCprYn));
					strcpy(szCprYn, getstr(cpr_row, 1));
					if (strcmp(szCprYn, pfups4006.copyright_yn) != 0)
					{
						memset(szQuery, 0x00, sizeof(szQuery));
						sprintf(szQuery, "delete from zangsi_cpr.T_CONTENTS_TEMPLIST_SUB where seq_no = %lu", ulSeqNO);
						if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
						{
							infLOG(ERROR, "fups4006->InsertFileData : MysqlQuery ����.\n");
							infLOG(ERROR, "fups4006->InsertFileData : [%d](%s)[%s]\n", mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
							mysql_free_result(cpr_res);
							return -44;
						}
						bCprFirst = true;
					}
				}
			}
			mysql_free_result(cpr_res);

			if (bCprFirst)
			{
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, "insert into zangsi_cpr.T_CONTENTS_TEMPLIST_SUB           "
								 " ( id, depth, folder_yn, file_type, folder_path , file_name, file_size, reg_user, reg_date, reg_time, default_hash, audio_hash, video_hash, copyright_yn, comp_cd, chi_id, price_amt , mob_price_amt,mob_service_yn ) "
								 " values "
								 " ( %d, %d  , '%s'  , '%s'     , '%s'        , '%s'     , %13.0f   , '%s'     , '%s'   ,'%s'     , '%s'        , '%s'      , '%s'      , '%s'        , '%s'   , %lu    , %d  , %d ,'%s' )   ",
						pfups4006.id, pfups4006.depth, pfups4006.folder_yn, pMurekaStatus, pfups4006.folder_name, pfups4006.file_name, pfups4006.file_size, pfups4006.user_id, pRegDate, pRegTime, pfups4006.default_hash, pfups4006.audio_hash, pfups4006.video_hash, pfups4006.copyright_yn, szCompID, lChiID, lPriceAmt, lMobilePriceAmt, szMobServiceYn); // 20130410

				infLOG(ALWAY, "fups4006->T_CONTENTS_TEMPLIST_SUB [ %s ] \n", szQuery);

				if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
				{
					infLOG(ERROR, "fups4006->InsertFileData : MysqlQuery ����.\n");
					infLOG(ERROR, "fups4006->InsertFileData : [%d](%s)[%s]\n", mysql_errno(cpr_con), mysql_error(cpr_con), szQuery);
					return -44;
				}
			}
		}
	}

	return nResult;
}

int InsertCprData(MYSQL *con, CMysqlCon MysqlCon, MYSQL *cpr_con, CMysqlCon MysqlCprCon, unsigned long ulID, char *pSectCode, char *pSectSub, char *szDefaultHash, char *szFileName, double dwFileSize, MUREKA_VINFO MurekaVInfo) //,char* pEventStatus)
{
	MYSQL_RES *cpr_res2;
	MYSQL_ROW cpr_row2;

	int nResult = 0;

	char ErrMsg[256];
	memset(ErrMsg, 0x00, sizeof(ErrMsg)); // �����޽���
	char szSysErrMsg[255];
	memset(szSysErrMsg, 0x00, sizeof(szSysErrMsg));
	char szQuery[15000];
	memset(szQuery, 0x00, sizeof(szQuery));
	char szCompCd[6 + 1];
	memset(szCompCd, 0x00, sizeof(szCompCd));
	char reg_date[8 + 1];
	memset(reg_date, 0x00, sizeof(reg_date));
	char reg_time[6 + 1];
	memset(reg_time, 0x00, sizeof(reg_time));
	char szMadultYn[2 + 1];
	memset(szMadultYn, 0x00, sizeof(szMadultYn));
	char szApplyYn[2 + 1];
	memset(szApplyYn, 0x00, sizeof(szApplyYn));

	int nMPriceAmt = atoi(MurekaVInfo.video_price);
	bool bIsChange = false;
	unsigned long ulListId = 0;

	char szPaymentRate[6 + 1];
	memset(szPaymentRate, 0x00, sizeof(szPaymentRate));

	double dOspPaymentRate = atof(MurekaVInfo.video_osp_jibun);
	double dPaymentRate = 100 - dOspPaymentRate;

	sprintf(szPaymentRate, "%.2f", dPaymentRate);

	if (strlen(MurekaVInfo.video_right_content_id) <= 0 ||
		strlen(MurekaVInfo.video_right_name) <= 0 ||
		strlen(MurekaVInfo.video_cha) <= 0 ||
		nMPriceAmt <= 0 ||
		strlen(szPaymentRate) <= 0)
	{
		strcpyA(ErrMsg, "������ ����.");
		infLOG(ERROR, "fups4006->InsertCprData (%s)\n", ErrMsg);
		return -51;
	}

	strcpyA(szQuery, "SELECT date_format(now(),'%Y%m%d')");
	strcat(szQuery, "     , date_format(now(),'%H%i%s')");

	if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		strcpyA(ErrMsg, "������� ��ȸ ����.\n");
		infLOG(ERROR, "%s", ErrMsg);
		infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		return -44;
	}

	if (!(cpr_res2 = mysql_store_result(cpr_con)))
	{
		strcpyA(ErrMsg, "������� ������ ���� ����.\n");
		infLOG(ERROR, "%s", ErrMsg);
		infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -44;
	}

	if (mysql_num_rows(cpr_res2) == 0)
	{
		strcpyA(ErrMsg, "������� ����.\n");
		infLOG(ERROR, "%s", ErrMsg);
		infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -44;
	}

	cpr_row2 = mysql_fetch_row(cpr_res2);

	strcpyA(reg_date, getstr(cpr_row2, 0));
	strcpyA(reg_time, getstr(cpr_row2, 1));

	mysql_free_result(cpr_res2);

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "select comp_cd from zangsi_cpr.T_CPR_COMP_INFO where comp_nm = '%s'", MurekaVInfo.video_right_name);

	if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		sprintf(ErrMsg, "�Ǹ��� ��ȸ ����(%s).\n", MurekaVInfo.video_right_name);
		infLOG(ERROR, "%s", ErrMsg);
		infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		return -50;
	}

	if (!(cpr_res2 = mysql_store_result(cpr_con)))
	{
		sprintf(ErrMsg, "�Ǹ��� ��ȸ ����(%s).\n", MurekaVInfo.video_right_name);
		infLOG(ERROR, "%s", ErrMsg);
		infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -50;
	}

	if (mysql_num_rows(cpr_res2) == 0)
	{
		sprintf(ErrMsg, "�Ǹ��� ����.\n szQuery=[%s]\n", szQuery);
		infLOG(ERROR, "%s", ErrMsg);
		infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -50;
	}

	cpr_row2 = mysql_fetch_row(cpr_res2);
	strcpyA(szCompCd, getstr(cpr_row2, 0));
	mysql_free_result(cpr_res2);

	/*
	��� �������� ��� ����, ������, �Ǹ��� ������ �ִ��� üũ�Ͽ� ���ٸ� �Ϲ��������� ���(??).
	üũ����� TRUE�̸� ������� zangsi_cpr.T_CPR_CONT_LIST�� �Է�.
	��Ͻð��� zangsi.T_MINOR_CODE�� ������ �ð������� zangsi_cpr.T_CPR_CONT_LIST�� apply_yn �� 'P'�� �Է�
	*/

	//////////////no.875/////////////////

	if (strcmp(szCompCd, "010022") == 0) // MBC
		nResult = 5;
	else if (strcmp(szCompCd, "010020") == 0) // KBS
		nResult = 6;
	else if (strcmp(szCompCd, "010021") == 0) // SBS
		nResult = 7;
	else if (strcmp(szCompCd, "010033") == 0) // EBS
		nResult = 8;
	else if (strcmp(szCompCd, "010032") == 0) // KBSi
		nResult = 9;
	else if (strcmp(szCompCd, "010030") == 0) // MBCP
		nResult = 10;
	else if (strcmp(szCompCd, "010038") == 0) //
		nResult = 11;
	else if (strcmp(szCompCd, "010036") == 0) //
		nResult = 12;
	else if (strcmp(szCompCd, "010037") == 0) // iMBC
		nResult = 13;
	else if (strcmp(szCompCd, "010071") == 0) // MBCM
		nResult = 14;
	else if (strcmp(szCompCd, "010017") == 0) // CJETV
		nResult = 15;
	else if (strcmp(szCompCd, "010041") == 0) // CJENM
		nResult = 16;
	else if (strcmp(szCompCd, "010055") == 0) // MBCV
		nResult = 17;

	/////////////////////////////////////

	if (strcmp(MurekaVInfo.video_osp_etc, "NOTLOG") == 0)
	{
		strcpy(szApplyYn, "P");
	}
	else
		strcpy(szApplyYn, "Y");

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "select list_id, apply_yn ,comp_cd  , price_amt from zangsi_cpr.T_CPR_CONT_LIST where mgr_cd = '%s' and chapter = %s", MurekaVInfo.video_right_content_id, MurekaVInfo.video_cha);

	infLOG(ALWAY, "fups4006->szQuery 1[ %s ] \n", szQuery);

	if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		sprintf(ErrMsg, "�����ڵ� ��ȸ ����.\nszQuery=[%s]\n", szQuery);
		infLOG(ERROR, "%s", ErrMsg);
		infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		return -44;
	}

	if (!(cpr_res2 = mysql_store_result(cpr_con)))
	{
		sprintf(ErrMsg, "�����ڵ� ��ȸ ����.\nszQuery=[%s]\n", szQuery);
		infLOG(ERROR, "%s", ErrMsg);
		infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -44;
	}

	//���� ���� ������
	if (mysql_num_rows(cpr_res2) == 0)
	{
		mysql_free_result(cpr_res2);

		if (strcmp(MurekaVInfo.video_grade, "18") == 0)
			strcpy(szMadultYn, "Y");
		else
			strcpy(szMadultYn, "N");

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi_cpr.T_CPR_CONT_LIST "
						 " (seq_no ,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, open_date, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate) "
						 " values "
						 " ( 1, '%s', %s,   '%s',    '%s',      '%s',     '%s',        %d,     '%s', '%s',  'sys4006',     '%s',     '%s',     '%s',             '%s')  ",
				MurekaVInfo.video_title, MurekaVInfo.video_cha, MurekaVInfo.video_right_content_id, szCompCd, pSectCode, pSectSub, nMPriceAmt, szMadultYn, MurekaVInfo.video_onair_date, reg_date, reg_time, szApplyYn, szPaymentRate);

		infLOG(ALWAY, "fups4006->szQuery 2[ %s ] \n", szQuery);

		if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
		{
			strcpyA(ErrMsg, "�����ڵ� INSERT ����.\n");
			infLOG(ERROR, "%s", ErrMsg);
			infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			return -44;
		}
		ulListId = mysql_insert_id(cpr_con);

		sprintf(szQuery, " insert into zangsi_cpr.T_CPR_CONT_LIST_HIST "
						 " (list_id ,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate "
						 " , udt_user , udt_date,udt_time ) "
						 " select list_id ,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate   "
						 " , 'sys4006' , date_format(now(), '%%Y%%m%%d'),  date_format(now(), '%%H%%i%%s')  "
						 " from zangsi_cpr.T_CPR_CONT_LIST where list_id = %lu ",
				ulListId);

		infLOG(ALWAY, "fups4006->szQuery 3[ %s ] \n", szQuery);

		if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
		{
			strcpyA(ErrMsg, "�����ڵ� INSERT ����.\n");
			infLOG(ERROR, "%s", ErrMsg);
			infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			return -44;
		}

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi_cpr.T_CPR_CONT_LIST_WAIT "
						 " (list_id, title, chapter, mgr_cd, comp_cd, wait_time, reg_user, reg_date, reg_time, proc_stat) "
						 " select "
						 " %lu, '%s', %s, '%s', '%s', d.minor_name, 'sys4006', '%s', '%s', b.minor_name  "
						 " from zangsi_cpr.T_MINOR_CODE a, zangsi_cpr.T_MINOR_CODE b "
						 " , zangsi_cpr.T_MINOR_CODE c, zangsi_cpr.T_MINOR_CODE d  "
						 " where a.minor_code = b.minor_code and c.minor_code = d.minor_code and "
						 " a.major_code = '91' and b.major_code = '92' and "
						 " c.major_code = '91' and d.major_code = '90' and "
						 " a.minor_name = '%s' and c.minor_name = '%s' ",
				ulListId, MurekaVInfo.video_title, MurekaVInfo.video_cha, MurekaVInfo.video_right_content_id, szCompCd, reg_date, reg_time, szCompCd, szCompCd);

		if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
		{
			strcpyA(ErrMsg, "�����ڵ� �Է� ����.\n");
			infLOG(ERROR, "%s\n%s\n", ErrMsg, szQuery);
			infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			return -44;
		}

		if (nResult >= 5 && strcmp(szApplyYn, "P") == 0)
		{
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " replace into zangsi_cpr.T_CPR_MUREKA_WAIT_TEMP "
							 " ( list_id, mgr_cd, cont_gu, id, reg_date, reg_time, default_hash, file_size, file_name)"
							 " values "
							 " (%lu, '%s', 'WE', %lu, '%s', '%s', '%s', %15.0f, '%s') ",
					ulListId, MurekaVInfo.video_right_content_id, ulID, reg_date, reg_time, szDefaultHash, dwFileSize, szFileName);
#ifdef _DEBUG_
			printf("InsertCprData> szQuery=[%s].\n\n", szQuery);
#endif

			if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				strcpyA(ErrMsg, "T_CPR_MUREKA_WAIT_TEMP �Է� ����.\n");
				infLOG(ERROR, "%s", ErrMsg);
				infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
				return -44;
			}
		}
	}
	else
	{
		cpr_row2 = mysql_fetch_row(cpr_res2);

		ulListId = (unsigned long)getnum(cpr_row2, 0);

		char szListApplyYn[2 + 1];
		char szOldCompCd[6 + 1];
		memset(szOldCompCd, 0x00, sizeof(szOldCompCd));
		int nOldPriceAmt = 0;

		strcpyA(szListApplyYn, getstr(cpr_row2, 1));
		strcpyA(szOldCompCd, getstr(cpr_row2, 2));
		nOldPriceAmt = getint(cpr_row2, 3);

		mysql_free_result(cpr_res2);

		if (strcmp(szListApplyYn, szApplyYn) != 0 || strcmp(szCompCd, szOldCompCd) != 0) //|| nMPriceAmt != nOldPriceAmt )
		{

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update zangsi_cpr.T_CPR_CONT_LIST set apply_yn = '%s', comp_cd ='%s'  ,  seq_no = seq_no + 1  where list_id = %lu ", szApplyYn, szCompCd, ulListId);

			infLOG(ALWAY, "fups4006->szQuery 4[ %s ] \n", szQuery);

			if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				strcpyA(ErrMsg, "�����ڵ� UPDATE ����.\n");
				infLOG(ERROR, "%s", ErrMsg);
				infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
				return -44;
			}

			// 20120522
			sprintf(szQuery, " insert into zangsi_cpr.T_CPR_CONT_LIST_HIST "
							 " (list_id ,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate "
							 " , udt_user , udt_date,udt_time ) "
							 " select list_id ,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate   "
							 " , 'sys4006' , date_format(now(), '%%Y%%m%%d'),  date_format(now(), '%%H%%i%%s') "
							 " from zangsi_cpr.T_CPR_CONT_LIST where list_id = %lu ",
					ulListId);

			infLOG(ALWAY, "fups4006->szQuery 5:[ %s ] \n", szQuery);

			if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				strcpyA(ErrMsg, "�����ڵ� INSERT ����.\n");
				infLOG(ERROR, "%s", ErrMsg);
				infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
				return -44;
			}
		}
		if (nResult >= 5 && strcmp(szApplyYn, "P") == 0)
		{
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " replace into zangsi_cpr.T_CPR_MUREKA_WAIT_TEMP "
							 " ( list_id, mgr_cd, cont_gu, id, reg_date, reg_time, default_hash, file_size, file_name)"
							 " values "
							 " (%lu, '%s', 'WE', %lu, '%s', '%s', '%s', %15.0f, '%s') ",
					ulListId, MurekaVInfo.video_right_content_id, ulID, reg_date, reg_time, szDefaultHash, dwFileSize, szFileName);

			infLOG(ALWAY, "fups4006->szQuery 6:[ %s ] \n", szQuery);

			if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				strcpyA(ErrMsg, "T_CPR_MUREKA_WAIT_TEMP �Է� ����.\n");
				infLOG(ERROR, "%s", ErrMsg);
				infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
				return -44;
			}
		}
	}

	if (strcmp(MurekaVInfo.video_osp_etc, "NOTLOG") == 0)
		return nResult;

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select chi_id, title, cha from zangsi_cpr.T_CONT_CPR_MAP_TEMP where id = %lu and cont_gu = 'WE' ", ulID);

#ifdef __DEBUG
	printf("InsertCprData> query = [%s]\n", szQuery);
#endif

	if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		strcpyA(ErrMsg, "T_CONT_CPR_MAP_TEMP ��ȸ ����.\n");
		infLOG(ERROR, "%s", ErrMsg);
		infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		return -44;
	}
	if (!(cpr_res2 = mysql_store_result(cpr_con)))
	{
		strcpyA(ErrMsg, "T_CONT_CPR_MAP_TEMP ������ ���� ����.\n");
		infLOG(ERROR, "%s", ErrMsg);
		infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -44;
	}
	if (mysql_num_rows(cpr_res2) <= 0)
	{
#ifdef __DEBUG
		printf("fups4006->InsertCprData : ù��° ���� [%lu]\n", ulID);
#endif
		mysql_free_result(cpr_res2);

		/* 2009/09/25 T_CPR_HASH_INFO, FILE ��ȸ�Ͽ� ������ �μ�Ʈ�� T_CONT_CPR_MAP_TEMP���� �μ�Ʈ	*/

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " select a.chi_id"
						 " from zangsi_cpr.T_CPR_HASH_INFO a, zangsi_cpr.T_CPR_HASH_FILE b "
						 " where a.chi_id = b.chi_id and b.file_size = %15.0f and b.default_hash = '%s' "
						 " group by a.chi_id limit 1",
				dwFileSize, szDefaultHash);

		if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
		{
			strcpyA(ErrMsg, "�ؽ� ���� ��ȸ ����.\n");
			infLOG(ERROR, "%s", ErrMsg);
			infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			return -44;
		}
		if (!(cpr_res2 = mysql_store_result(cpr_con)))
		{
			strcpyA(ErrMsg, "�ؽ� ���� ������ ���� ����.\n");
			infLOG(ERROR, "%s", ErrMsg);
			infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			return -44;
		}
		if (mysql_num_rows(cpr_res2) <= 0)
		{ //�ؽ������� �����Ƿ� �μ�Ʈ
			mysql_free_result(cpr_res2);

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi_cpr.T_CPR_HASH_INFO  "
							 " (title, comp_cd, list_id, sect_code, sect_sub, adult_yn, price_amt, proc_stat, reg_user, cpr_payment_rate, reg_date, reg_time) "
							 " select  "
							 " title , comp_cd , list_id, if(sect_code = 'FD', '%s', sect_code), if(sect_sub = 'FD', '%s', sect_sub), adult_yn, price_amt, 'C'      , 'sys4006', cpr_payment_rate, '%s'    , '%s'  "
							 " from zangsi_cpr.T_CPR_CONT_LIST where mgr_cd = '%s' and comp_cd = '%s' and chapter = %s and apply_yn = 'Y' ",
					pSectCode, pSectSub, reg_date, reg_time, MurekaVInfo.video_right_content_id, szCompCd, MurekaVInfo.video_cha);

			infLOG(ALWAY, "fups4006->szQuery 7:[ %s ] \n", szQuery);

			if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				strcpyA(ErrMsg, "�ؽ� ���� �μ�Ʈ ����.\n");
				infLOG(ERROR, "%s", ErrMsg);
				infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
				return -44;
			}

			unsigned long ulChiId = 0;
			ulChiId = mysql_insert_id(cpr_con);

			if (ulChiId == 0)
			{
				if (nResult >= 5 && strcmp(szApplyYn, "P") == 0)
				{
					memset(szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, " replace into zangsi_cpr.T_CPR_MUREKA_WAIT_TEMP "
									 " ( list_id, mgr_cd, cont_gu, id, reg_date, reg_time, default_hash, file_size, file_name)"
									 " values "
									 " (%lu, '%s', 'WE', %lu, '%s', '%s', '%s', %15.0f, '%s') ",
							ulListId, MurekaVInfo.video_right_content_id, ulID, reg_date, reg_time, szDefaultHash, dwFileSize, szFileName);

					infLOG(ALWAY, "fups4006->szQuery 8:[ %s ] \n", szQuery);

					if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
					{
						strcpyA(ErrMsg, "T_CPR_MUREKA_WAIT_TEMP �Է� ����.\n");
						infLOG(ERROR, "%s", ErrMsg);
						infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
						return -44;
					}
				}

				sprintf(ErrMsg, "����� �ؽþ���[%s].\n", MurekaVInfo.video_right_content_id);
				infLOG(ALWAY, "fups4006->%s", ErrMsg);
				return 0;
			}

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi_cpr.T_CPR_HASH_FILE "
							 " (chi_id, comp_cd, filter_yn, video_yn, default_hash, video_hash, file_name, file_size, reg_date, reg_time) "
							 " select "
							 " %lu    , comp_cd , 'Y'     , 'Y'     , '%s'        , '%s'      , '%s'     , %15.0f , '%s'    , '%s' "
							 " from zangsi_cpr.T_CPR_CONT_LIST where mgr_cd = '%s' and comp_cd = '%s' and chapter = %s and apply_yn = 'Y' ",
					ulChiId, szDefaultHash, MurekaVInfo.mureka_hash, szFileName, dwFileSize, reg_date, reg_time, MurekaVInfo.video_right_content_id, szCompCd, MurekaVInfo.video_cha);

			infLOG(ALWAY, "fups4006->szQuery 9:[ %s ] \n", szQuery);

			if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				strcpyA(ErrMsg, "�ؽ� �������� �μ�Ʈ ����.\n");
				infLOG(ERROR, "%s", ErrMsg);
				infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
				return -44;
			}
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi_cpr.T_CPR_PRICE_HIST "
							 " (chi_id , comp_cd  , mgr_cd , apply_date, apply_time, price_amt  , cpr_payment_rate  , cpr_desc          , reg_user, reg_date, reg_time) "
							 " select  "
							 " b.chi_id, a.comp_cd, a.mgr_cd, '%s'     , '%s'      , a.price_amt, a.cpr_payment_rate, 'mureka filtering', 'sys4006', '%s'    , '%s' "
							 " from zangsi_cpr.T_CPR_CONT_LIST a, zangsi_cpr.T_CPR_HASH_INFO b "
							 " where a.list_id = b.list_id and b.chi_id = %lu ",
					reg_date, reg_time, reg_date, reg_time, ulChiId);

			infLOG(ALWAY, "fups4006->szQuery 10:[ %s ] \n", szQuery);

			if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				strcpyA(ErrMsg, "�ؽ� �����丮���� �μ�Ʈ ����.\n");
				infLOG(ERROR, "%s", ErrMsg);
				infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
				return -44;
			}
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "insert into zangsi_cpr.T_CONT_CPR_MAP_TEMP (id, chi_id, cont_gu, title, cha) "
							 " values "
							 " (%lu, %lu, 'WE', '%s', '%s')",
					ulID, ulChiId, MurekaVInfo.video_title, MurekaVInfo.video_cha);

			infLOG(ALWAY, "fups4006->szQuery 11:[ %s ] \n", szQuery);

			if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				strcpyA(ErrMsg, "T_CONT_CPR_MAP_TEMP �μ�Ʈ ����.\n");
				infLOG(ERROR, "%s", ErrMsg);
				infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
				return -44;
			}
		}
		else
		{
			mysql_free_result(cpr_res2);
		}
	}
	else
	{ // T_CONT_CPR_MAP_TEMP�� ������ ����.
		cpr_row2 = mysql_fetch_row(cpr_res2);

		unsigned long ulChiId = 0;
		ulChiId = (unsigned long)getnum(cpr_row2, 0);

		mysql_free_result(cpr_res2);

		char szTitle[5000];
		memset(szTitle, 0x00, sizeof(szTitle));
		strcpyA(szTitle, getstr(cpr_row2, 1));

		char szCha[10 + 1];
		memset(szCha, 0x00, sizeof(szCha));
		sprintf(szCha, "%s", getstr(cpr_row2, 2));

		if (strcmp(szTitle, MurekaVInfo.video_title) == 0 && strcmp(szCha, MurekaVInfo.video_cha) == 0)
		{ //���� ���� ������ T_CPR_HASH_FILE �� �μ�Ʈ
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " select a.chi_id"
							 " from zangsi_cpr.T_CPR_HASH_INFO a, zangsi_cpr.T_CPR_HASH_FILE b "
							 " where a.chi_id = b.chi_id and b.file_size = %15.0f and b.default_hash = '%s' "
							 " group by a.chi_id limit 1",
					dwFileSize, szDefaultHash);

			if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				strcpyA(ErrMsg, "�ؽ� ���� ��ȸ ����.\n");
				infLOG(ERROR, "%s", ErrMsg);
				infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
				return -44;
			}
			if (!(cpr_res2 = mysql_store_result(cpr_con)))
			{
				strcpyA(ErrMsg, "�ؽ� ���� ������ ���� ����.\n");
				infLOG(ERROR, "%s", ErrMsg);
				infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
				return -44;
			}
			if (mysql_num_rows(cpr_res2) <= 0)
			{ //�ؽ������� �����Ƿ� �μ�Ʈ
				mysql_free_result(cpr_res2);

				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " insert into zangsi_cpr.T_CPR_HASH_FILE "
								 " (chi_id, comp_cd, filter_yn, video_yn, default_hash, video_hash, file_name, file_size, reg_date, reg_time) "
								 " select "
								 " %lu    , comp_cd , 'Y'     , 'Y'     , '%s'        , '%s'      , '%s'     , %15.0f , '%s'    , '%s' "
								 " from zangsi_cpr.T_CPR_CONT_LIST where mgr_cd = '%s' and comp_cd = '%s' and chapter = %s and apply_yn = 'Y' ",
						ulChiId, szDefaultHash, MurekaVInfo.mureka_hash, szFileName, dwFileSize, reg_date, reg_time, MurekaVInfo.video_right_content_id, szCompCd, MurekaVInfo.video_cha);

				infLOG(ALWAY, "fups4006->szQuery a1:[ %s ] \n", szQuery);

				if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
				{
					strcpyA(ErrMsg, "�ؽ� �������� �μ�Ʈ ����.\n");
					infLOG(ERROR, "%s", ErrMsg);
					infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
					mysql_free_result(cpr_res2);
					return -44;
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
			2009/09/25 T_CPR_HASH_INFO, FILE ��ȸ�Ͽ� ������ �μ�Ʈ�� T_CONT_CPR_MAP_TEMP���� �μ�Ʈ
			*/

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " select a.chi_id"
							 " from zangsi_cpr.T_CPR_HASH_INFO a, zangsi_cpr.T_CPR_HASH_FILE b "
							 " where a.chi_id = b.chi_id and b.file_size = %15.0f and b.default_hash = '%s' "
							 " group by a.chi_id limit 1",
					dwFileSize, szDefaultHash);

#ifdef __DEBUG
			printf("InsertCprData> query = [%s]\n", szQuery);
#endif

			if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
			{
				strcpyA(ErrMsg, "�ؽ� ���� ��ȸ ����.\n");
				infLOG(ERROR, "%s", ErrMsg);
				infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
				return -44;
			}
			if (!(cpr_res2 = mysql_store_result(cpr_con)))
			{
				strcpyA(ErrMsg, "�ؽ� ���� ������ ���� ����.\n");
				infLOG(ERROR, "%s", ErrMsg);
				infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
				return -44;
			}
			if (mysql_num_rows(cpr_res2) <= 0)
			{ //�ؽ������� �����Ƿ� �μ�Ʈ
				mysql_free_result(cpr_res2);

				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " insert into zangsi_cpr.T_CPR_HASH_INFO  "
								 " (title, comp_cd, list_id, sect_code, sect_sub, adult_yn, price_amt, proc_stat, reg_user, cpr_payment_rate, reg_date, reg_time) "
								 " select  "
								 " title , comp_cd , list_id, if(sect_code = 'FD', '%s', sect_code), if(sect_sub = 'FD', '%s', sect_sub), adult_yn, price_amt, 'C'      , 'sys4006', cpr_payment_rate, '%s'    , '%s'  "
								 " from zangsi_cpr.T_CPR_CONT_LIST where mgr_cd = '%s' and comp_cd = '%s' and chapter = %s and apply_yn = 'Y' ",
						pSectCode, pSectSub, reg_date, reg_time, MurekaVInfo.video_right_content_id, szCompCd, MurekaVInfo.video_cha);

				infLOG(ALWAY, "fups4006->szQuery a2:[ %s ] \n", szQuery);

				if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
				{
					strcpyA(ErrMsg, "�ؽ� ���� �μ�Ʈ ����.\n");
					infLOG(ERROR, "%s", ErrMsg);
					infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
					return -44;
				}

				unsigned long ulChiId = 0;
				ulChiId = mysql_insert_id(cpr_con);

				if (ulChiId == 0)
				{
					if (nResult >= 5 && strcmp(szApplyYn, "P") == 0)
					{
						memset(szQuery, 0x00, sizeof(szQuery));
						sprintf(szQuery, " replace into zangsi_cpr.T_CPR_MUREKA_WAIT_TEMP "
										 " ( list_id, mgr_cd, cont_gu, id, reg_date, reg_time, default_hash, file_size, file_name)"
										 " values "
										 " (%lu, '%s', 'WE', %lu, '%s', '%s', '%s', %15.0f, '%s') ",
								ulListId, MurekaVInfo.video_right_content_id, ulID, reg_date, reg_time, szDefaultHash, dwFileSize, szFileName);

						infLOG(ALWAY, "fups4006->szQuery a3:[ %s ] \n", szQuery);

						if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
						{
							strcpyA(ErrMsg, "T_CPR_MUREKA_WAIT_TEMP �Է� ����.\n");
							infLOG(ERROR, "%s", ErrMsg);
							infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
							return -44;
						}
					}
					sprintf(ErrMsg, "����� �ؽþ���[%s].\n", MurekaVInfo.video_right_content_id);
					infLOG(ALWAY, "fups4006->%s", ErrMsg);
					return 0;
				}

				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " insert into zangsi_cpr.T_CPR_HASH_FILE "
								 " (chi_id, comp_cd, filter_yn, video_yn, default_hash, video_hash, file_name, file_size, reg_date, reg_time) "
								 " select "
								 " %lu    , comp_cd , 'Y'     , 'Y'     , '%s'        , '%s'      , '%s'     , %15.0f , '%s'    , '%s' "
								 " from zangsi_cpr.T_CPR_CONT_LIST where mgr_cd = '%s' and comp_cd = '%s' and chapter = %s and apply_yn = 'Y' ",
						ulChiId, szDefaultHash, MurekaVInfo.mureka_hash, szFileName, dwFileSize, reg_date, reg_time, MurekaVInfo.video_right_content_id, szCompCd, MurekaVInfo.video_cha);

				infLOG(ALWAY, "fups4006->szQuery a4:[ %s ] \n", szQuery);

				if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
				{
					strcpyA(ErrMsg, "�ؽ� �������� �μ�Ʈ ����.\n");
					infLOG(ERROR, "%s", ErrMsg);
					infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
					return -44;
				}
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " insert into zangsi_cpr.T_CPR_PRICE_HIST "
								 " (chi_id , comp_cd  , mgr_cd , apply_date, apply_time, price_amt  , cpr_payment_rate  , cpr_desc          , reg_user, reg_date, reg_time) "
								 " select  "
								 " b.chi_id, a.comp_cd, a.mgr_cd, '%s'     , '%s'      , a.price_amt, a.cpr_payment_rate, 'mureka filtering', 'sys4006', '%s'    , '%s' "
								 " from zangsi_cpr.T_CPR_CONT_LIST a, zangsi_cpr.T_CPR_HASH_INFO b "
								 " where a.list_id = b.list_id and b.chi_id = %lu ",
						reg_date, reg_time, reg_date, reg_time, ulChiId);

				infLOG(ALWAY, "fups4006->szQuery a5:[ %s ] \n", szQuery);

				if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
				{
					strcpyA(ErrMsg, "�ؽ� �����丮���� �μ�Ʈ ����.\n");
					infLOG(ERROR, "%s", ErrMsg);
					infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
					return -44;
				}
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, "insert into zangsi_cpr.T_CONT_CPR_MAP_TEMP (id, chi_id, cont_gu, title, cha) "
								 " values "
								 " (%lu, %lu, 'WE', '%s', '%s')",
						ulID, ulChiId, MurekaVInfo.video_title, MurekaVInfo.video_cha);

				infLOG(ALWAY, "fups4006->szQuery a6:[ %s ] \n", szQuery);

				if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
				{
					strcpyA(ErrMsg, "T_CONT_CPR_MAP_TEMP �μ�Ʈ ����.\n");
					infLOG(ERROR, "%s", ErrMsg);
					infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
					return -44;
				}
			}
			else
			{
				mysql_free_result(cpr_res2);
			}
		}
	}

	return nResult;
}

int UpdateCprData(MYSQL *cpr_con, CMysqlCon MysqlCprCon, char *szDefaultHash, char *szFileName, double dwFileSize, MUREKA_VINFO MurekaVInfo)
{
	MYSQL_RES *cpr_res2;
	MYSQL_ROW cpr_row2;

	char ErrMsg[256];					  // error message
	memset(ErrMsg, 0x00, sizeof(ErrMsg)); // �����޽���

	char szSysErrMsg[255];
	memset(szSysErrMsg, 0x00, sizeof(szSysErrMsg));

	if (strlen(MurekaVInfo.video_right_content_id) <= 0 ||
		strlen(MurekaVInfo.video_right_name) <= 0 ||
		strlen(MurekaVInfo.video_cha) <= 0)
	{
		strcpyA(ErrMsg, "������ ����.");
		infLOG(ERROR, "fups4006->InsertCprData (%s)\n", ErrMsg);
		return -51;
	}

	char szQuery[15000];
	memset(szQuery, 0x00, sizeof(szQuery));

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "select comp_cd from zangsi_cpr.T_CPR_COMP_INFO where comp_nm = '%s'", MurekaVInfo.video_right_name);

#ifdef __DEBUG
	printf("UpdateCprData> query = [%s]\n", szQuery);
#endif

	if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		strcpyA(ErrMsg, "T_CPR_COMP_INFO ��ȸ ����.\n");
		infLOG(ERROR, "%s", ErrMsg);
		infLOG(ERROR, "fups4006->UpdateCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		return -50;
	}
	if (!(cpr_res2 = mysql_store_result(cpr_con)))
	{
		strcpyA(ErrMsg, "T_CPR_COMP_INFO ������ ���� ����.\n");
		infLOG(ERROR, "%s", ErrMsg);
		infLOG(ERROR, "fups4006->UpdateCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -50;
	}
	if (mysql_num_rows(cpr_res2) <= 0)
	{
		infLOG(ERROR, "T_CPR_COMP_INFO ȸ���ڵ� ����.[%s]\n", MurekaVInfo.video_right_name);
		infLOG(ERROR, "fups4006->UpdateCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -50;
	}
	cpr_row2 = mysql_fetch_row(cpr_res2);

	char szCompCd[6 + 1];
	memset(szCompCd, 0x00, sizeof(szCompCd));

	strcpyA(szCompCd, getstr(cpr_row2, 0));

	mysql_free_result(cpr_res2);

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select list_id,  apply_yn from zangsi_cpr.T_CPR_CONT_LIST where mgr_cd = '%s' and comp_cd = '%s' and chapter = %s ", MurekaVInfo.video_right_content_id, szCompCd, MurekaVInfo.video_cha);

#ifdef __DEBUG
	printf("UpdateCprData> query = [%s]\n", szQuery);
#endif

	if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		strcpyA(ErrMsg, "T_CPR_CONT_LIST ��ȸ ����.\n");
		infLOG(ERROR, "%s", ErrMsg);
		infLOG(ERROR, "fups4006->UpdateCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		return -44;
	}
	if (!(cpr_res2 = mysql_store_result(cpr_con)))
	{
		strcpyA(ErrMsg, "T_CPR_CONT_LIST ������ ���� ����.\n");
		infLOG(ERROR, "%s", ErrMsg);
		infLOG(ERROR, "fups4006->UpdateCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -44;
	}
	if (mysql_num_rows(cpr_res2) <= 0)
	{
		infLOG(ERROR, "T_CPR_CONT_LIST �����ڵ� ����.[%s]\n", MurekaVInfo.video_right_content_id);
		infLOG(ERROR, "fups4006->UpdateCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -54;
	}
	cpr_row2 = mysql_fetch_row(cpr_res2);

	unsigned long ulListId = 0;
	ulListId = (unsigned long)getnum(cpr_row2, 0);

	char szApplyYn[1 + 1];
	memset(szApplyYn, 0x00, sizeof(szApplyYn));

	strcpyA(szApplyYn, getstr(cpr_row2, 1));

	mysql_free_result(cpr_res2);

	if (strcmp(szApplyYn, "Y") == 0)
	{ //���� ��Ұ� �ȴ϶��
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update zangsi_cpr.T_CPR_CONT_LIST set apply_yn = 'N' , seq_no = seq_no + 1 where list_id = %lu ", ulListId);

#ifdef __DEBUG
		printf("InsertCprData> query = [%s]\n", szQuery);
#endif
		if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
		{
			strcpyA(ErrMsg, "�ؽ� �������� �μ�Ʈ ����.\n");
			infLOG(ERROR, "%s", ErrMsg);
			infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			return -44;
		}

		// 20120522
		sprintf(szQuery, " insert into zangsi_cpr.T_CPR_CONT_LIST_HIST "
						 " (list_id ,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate "
						 " , udt_user , udt_date,udt_time ) "
						 " select list_id ,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate   "
						 " , 'sys4006' , date_format(now(), '%%Y%%m%%d'),  date_format(now(), '%%H%%i%%s')  "
						 " from zangsi_cpr.T_CPR_CONT_LIST where list_id = %lu ",
				ulListId);

		if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
		{
			strcpyA(ErrMsg, "�����ڵ� ������Ʈ ����.\n");
			infLOG(ERROR, "%s", ErrMsg);
			infLOG(ERROR, "fups4006->InsertCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			return -44;
		}
	}

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select count(b.chi_id) "
					 " from zangsi_cpr.T_CPR_CONT_LIST a, zangsi_cpr.T_CPR_HASH_INFO b "
					 " where a.list_id = b.list_id and a.list_id = %lu and b.proc_stat != 'N' "
					 " group by a.list_id ",
			ulListId);

#ifdef __DEBUG
	printf("UpdateCprData> query = [%s]\n", szQuery);
#endif

	if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
	{
		strcpyA(ErrMsg, "������ �ؽ� ��ȸ ����.\n");
		infLOG(ERROR, "%s", ErrMsg);
		infLOG(ERROR, "fups4006->UpdateCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		return -44;
	}
	if (!(cpr_res2 = mysql_store_result(cpr_con)))
	{
		strcpyA(ErrMsg, "������ �ؽ� ������ ��ȸ ����.\n");
		infLOG(ERROR, "%s", ErrMsg);
		infLOG(ERROR, "fups4006->UpdateCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -44;
	}
	if (mysql_num_rows(cpr_res2) <= 0)
	{
		infLOG(ERROR, "������ �ؽ� �����ڵ� ����.[%s][%lu]\n", MurekaVInfo.video_right_content_id, ulListId);
		infLOG(ERROR, "fups4006->UpdateCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		mysql_free_result(cpr_res2);
		return -55;
	}
	cpr_row2 = mysql_fetch_row(cpr_res2);

	int nCount = 0;
	nCount = (int)getint(cpr_row2, 0);

	mysql_free_result(cpr_res2);

	if (nCount > 0)
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update "
						 " zangsi_cpr.T_CPR_CONT_LIST a, zangsi_cpr.T_CPR_HASH_INFO b "
						 " set b.proc_stat = 'N' "
						 " where a.list_id = b.list_id and a.list_id = %lu and b.proc_stat != 'N' ",
				ulListId);
#ifdef __DEBUG
		printf("UpdateCprData> query = [%s]\n", szQuery);
#endif

		if (MysqlCprCon.MysqlQuery(cpr_con, szSysErrMsg, szQuery))
		{
			strcpyA(ErrMsg, "������ �ؽ� ��ȸ ����.\n");
			infLOG(ERROR, "%s", ErrMsg);
			infLOG(ERROR, "fups4006->UpdateCprData [%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			return -44;
		}
	}

#ifdef __DEBUG
	printf("\n\n\n\n\ndcmdfups4006 UpdateCprData() �Ϸ�\n\n\n\n");
#endif

	return 0;
}

/*****************************************************************************
 * ������ ���͸� ���� �� ��� ���
 * (I) 1. DB connet ����(nori)
 *     2. ���� ����
 *     3. �·�ī ����
 *	  4. ��Ÿ ����(����Ͻ�, ���� ����)
 * (R) 1. 0		: ����
 *	  2. ����	: ����
 *****************************************************************************/
int InsertMurekaVideo(MYSQL *con, CMysqlCon MysqlCon, CFUPS4006 pfups4006, LPMUREKA_VINFO pMurekaVInfo, int nMurekaCnt, int nMurekaIndex)
{
	infLOG(ALWAY, "fups4006: nMurekaCnt [ %d ] \n", nMurekaCnt);

	MYSQL_RES *res;
	MYSQL_ROW row;

	char szSysErrMsg[255];
	memset(szSysErrMsg, 0x00, sizeof(szSysErrMsg));

	char szQuery[10000];
	memset(szQuery, 0x00, sizeof(szQuery));

	//--------------------------------------------------------------------------
	//���� �׸��� �ִ��� Ȯ��
	//--------------------------------------------------------------------------
	// mureak ���� �߰�
	// for( int i =0 ; i < nMurekaCnt ; i++ )
	int i = nMurekaIndex;
	if (nMurekaCnt > 0 && i < nMurekaCnt)
	{
		if (pMurekaVInfo[i].filename == NULL || strlen(pMurekaVInfo[i].filename) <= 0)
			return -1;

		// ReplaceSingleQuotation(pMurekaVInfo[i].filename, '\'',pMurekaVInfo[i].filename);
		// ReplaceSingleQuotation(pMurekaVInfo[i].video_title, '\'',pMurekaVInfo[i].video_title);

		if (pMurekaVInfo[i].nFileGubun == 4) //���̷���
		{
			sprintf(szQuery, "insert into zangsi.T_CONTENTS_TEMPLIST_MUREKA "
							 " (  seq_no , id , file_gubun , result_code , video_status , video_id , video_title , video_jejak_year , video_right_name "
							 " , video_right_content_id , video_grade , video_price , video_cha , video_osp_jibun , video_osp_etc "
							 " , video_onair_date , video_right_id , virus_type , virus_name ,mureka_hash ,file_name ) "
							 " values ( %lu , %lu , %d , %d , '%s', '%s', '%s', '%s', '%s' "
							 " , '%s' , '%s'  , '%s'  , '%s'  , '%s'  , '%s' "
							 " , '%s'  , '%s'  , '%s'  , '%s' ,'%s','%s' )",
					pfups4006.seq_no, pfups4006.id, pMurekaVInfo[i].nFileGubun, pMurekaVInfo[i].nResultCode, pMurekaVInfo[i].video_status, pMurekaVInfo[i].video_id, pMurekaVInfo[i].video_title, pMurekaVInfo[i].video_jejak_year, pMurekaVInfo[i].video_right_name, pMurekaVInfo[i].video_right_content_id, pMurekaVInfo[i].video_grade, pMurekaVInfo[i].video_price, pMurekaVInfo[i].video_cha, pMurekaVInfo[i].video_osp_jibun, pMurekaVInfo[i].video_osp_etc, pMurekaVInfo[i].video_onair_date, pMurekaVInfo[i].video_right_id, pMurekaVInfo[i].video_right_name, pMurekaVInfo[i].video_title, pMurekaVInfo[i].mureka_hash, pMurekaVInfo[i].filename);
		}
		else if (pMurekaVInfo[i].nFileGubun == 2 || (pMurekaVInfo[i].nFileGubun == 8)) //������
		{
			sprintf(szQuery, "insert into zangsi.T_CONTENTS_TEMPLIST_MUREKA "
							 " ( seq_no , id , file_gubun , result_code , video_status , video_id , video_title , video_jejak_year , video_right_name "
							 " , video_right_content_id , video_grade , video_price , video_cha , video_osp_jibun , video_osp_etc "
							 " , video_onair_date , video_right_id ,mureka_hash ,file_name ) "
							 " values ( %lu , %lu , %d , %d , '%s', '%s', '%s', '%s', '%s' "
							 " , '%s' , '%s'  , '%s'  , '%s'  , '%s'  , '%s' "
							 " , '%s'  , '%s'  ,'%s','%s' )",
					pfups4006.seq_no, pfups4006.id, pMurekaVInfo[i].nFileGubun, pMurekaVInfo[i].nResultCode, pMurekaVInfo[i].video_status, pMurekaVInfo[i].video_id, pMurekaVInfo[i].video_title, pMurekaVInfo[i].video_jejak_year, pMurekaVInfo[i].video_right_name, pMurekaVInfo[i].video_right_content_id, pMurekaVInfo[i].video_grade, pMurekaVInfo[i].video_price, pMurekaVInfo[i].video_cha, pMurekaVInfo[i].video_osp_jibun, pMurekaVInfo[i].video_osp_etc, pMurekaVInfo[i].video_onair_date, pMurekaVInfo[i].video_right_id, pMurekaVInfo[i].mureka_hash, pMurekaVInfo[i].filename);
		}
		else if (pMurekaVInfo[i].nResultCode != 0)
		{
			sprintf(szQuery, "insert into zangsi.T_CONTENTS_TEMPLIST_MUREKA "
							 " ( seq_no , id , file_gubun , result_code , video_status , video_id , video_title , video_jejak_year , video_right_name "
							 " , video_right_content_id , video_grade , video_price , video_cha , video_osp_jibun , video_osp_etc "
							 " , video_onair_date , video_right_id ,mureka_hash ,file_name ) "
							 " values ( %lu , %lu , %d , %d , '%s', '%s', '%s', '%s', '%s' "
							 " , '%s' , '%s'  , '%s'  , '%s'  , '%s'  , '%s' "
							 " , '%s'  , '%s'  ,'%s','%s' )",
					pfups4006.seq_no, pfups4006.id, pMurekaVInfo[i].nFileGubun, pMurekaVInfo[i].nResultCode, pMurekaVInfo[i].video_status, pMurekaVInfo[i].video_id, pMurekaVInfo[i].video_title, pMurekaVInfo[i].video_jejak_year, pMurekaVInfo[i].video_right_name, pMurekaVInfo[i].video_right_content_id, pMurekaVInfo[i].video_grade, pMurekaVInfo[i].video_price, pMurekaVInfo[i].video_cha, pMurekaVInfo[i].video_osp_jibun, pMurekaVInfo[i].video_osp_etc, pMurekaVInfo[i].video_onair_date, pMurekaVInfo[i].video_right_id, pMurekaVInfo[i].mureka_hash, pMurekaVInfo[i].filename);
		}

		infLOG(ALWAY, "fups4006->InsertMurekaVideo QUERY [ %s ]\n", szQuery);

		if (pMurekaVInfo[i].nFileGubun == 4 || pMurekaVInfo[i].nFileGubun == 2 || (pMurekaVInfo[i].nFileGubun == 8) || pMurekaVInfo[i].nResultCode != 0)
		{
			if (MysqlCon.MysqlQuery(con, szSysErrMsg, szQuery))
			{
				infLOG(ERROR, "fups4006->InsertMurekaVideo : MysqlQuery error.\n");
				infLOG(ERROR, "fups4006->InsertMurekaVideo : [%d](%s)[%s]\n", mysql_errno(con), mysql_error(con), szQuery);
			}
		}
	}

	// infLOG(ALWAY, "fups4006->InsertMurekaVideo OUT | temp_id [ %lu ] seq [ %lu ] \n",pfups4006.id,pfups4006.seq_no);
	return 0;
}

// ============================================================================================================
void MurekaLog(int nMurekaCnt, LPMUREKA_VINFO pMurekaVInfo)
{

	for (int i = 0; i < nMurekaCnt; i++)
	{
		infLOG(ALWAY, "dcmdfups4006> nResultCode = [%d]\n", pMurekaVInfo[i].nResultCode);
		infLOG(ALWAY, "dcmdfups4006> nFileGubun = [%d]\n", pMurekaVInfo[i].nFileGubun);
		infLOG(ALWAY, "dcmdfups4006> filename = [%s]\n", pMurekaVInfo[i].filename);
		infLOG(ALWAY, "dcmdfups4006> mureka_hash = [%s]\n", pMurekaVInfo[i].mureka_hash);

		if (pMurekaVInfo[i].nFileGubun == 2 || (pMurekaVInfo[i].nFileGubun == 8))
		{
			infLOG(ALWAY, "dcmdfups4006> video_status = [%s]\n", pMurekaVInfo[i].video_status);
			infLOG(ALWAY, "dcmdfups4006> video_id = [%s]\n", pMurekaVInfo[i].video_id);
			infLOG(ALWAY, "dcmdfups4006> video_title = [%s]\n", pMurekaVInfo[i].video_title);
			infLOG(ALWAY, "dcmdfups4006> video_jejak_year = [%s]\n", pMurekaVInfo[i].video_jejak_year);
			infLOG(ALWAY, "dcmdfups4006> video_right_name = [%s]\n", pMurekaVInfo[i].video_right_name);
			infLOG(ALWAY, "dcmdfups4006> video_right_content_id = [%s]\n", pMurekaVInfo[i].video_right_content_id);
			infLOG(ALWAY, "dcmdfups4006> video_grade = [%s]\n", pMurekaVInfo[i].video_grade);
			infLOG(ALWAY, "dcmdfups4006> video_price = [%s]\n", pMurekaVInfo[i].video_price);
			infLOG(ALWAY, "dcmdfups4006> video_cha = [%s]\n", pMurekaVInfo[i].video_cha);
			infLOG(ALWAY, "dcmdfups4006> video_osp_jibun = [%s]\n", pMurekaVInfo[i].video_osp_jibun);
			infLOG(ALWAY, "dcmdfups4006> video_osp_etc = [%s]\n", pMurekaVInfo[i].video_osp_etc);
			infLOG(ALWAY, "dcmdfups4006> video_onair_date = [%s]\n", pMurekaVInfo[i].video_onair_date);
			infLOG(ALWAY, "dcmdfups4006> video_right_id = [%s]\n", pMurekaVInfo[i].video_right_id);
		}
		else if (pMurekaVInfo[i].nFileGubun == 1)
		{
			infLOG(ALWAY, "dcmdfups4006> music_status = [%s]\n", pMurekaVInfo[i].music_status);
			infLOG(ALWAY, "dcmdfups4006> music_id = [%s]\n", pMurekaVInfo[i].music_id);
			infLOG(ALWAY, "dcmdfups4006> music_title = [%s]\n", pMurekaVInfo[i].music_title);
			infLOG(ALWAY, "dcmdfups4006> music_artist = [%s]\n", pMurekaVInfo[i].music_artist);
			infLOG(ALWAY, "dcmdfups4006> music_album = [%s]\n", pMurekaVInfo[i].music_album);
			infLOG(ALWAY, "dcmdfups4006> music_prod_code = [%s]\n", pMurekaVInfo[i].music_prod_code);
			infLOG(ALWAY, "dcmdfups4006> music_price = [%s]\n", pMurekaVInfo[i].music_price);
			infLOG(ALWAY, "dcmdfups4006> music_injeob_com = [%s]\n", pMurekaVInfo[i].music_injeob_com);
			infLOG(ALWAY, "dcmdfups4006> music_injeob_music_id = [%s]\n", pMurekaVInfo[i].music_injeob_music_id);
			infLOG(ALWAY, "dcmdfups4006> video_osp_jibun = [%s]\n", pMurekaVInfo[i].video_osp_jibun);
			infLOG(ALWAY, "dcmdfups4006> video_osp_etc = [%s]\n", pMurekaVInfo[i].video_osp_etc);
			infLOG(ALWAY, "dcmdfups4006> video_onair_date = [%s]\n", pMurekaVInfo[i].video_onair_date);
			infLOG(ALWAY, "dcmdfups4006> video_right_id = [%s]\n", pMurekaVInfo[i].video_right_id);
		}
	}
}

// 20190123
int Insert1mbHash(MYSQL *con, CMysqlCon MysqlCon, CFUPS4006 pfups4006, char *pRegDate, char *pRegTime, CFUPS4006_1M_HASH pfups4006hash)
{

	infLOG(ALWAY, "fups4006: Insert1mbHash id  [ %d ] \n", pfups4006.id);

	MYSQL_RES *res;
	MYSQL_ROW row;

	char szSysErrMsg[255];
	memset(szSysErrMsg, 0x00, sizeof(szSysErrMsg));

	char szQuery[10000];
	memset(szQuery, 0x00, sizeof(szQuery));

	if (strlen(pfups4006hash.hash_1m) > 0 && strlen(pfups4006hash.hash_1m_mureka) > 0)
	{
		sprintf(szQuery, "insert into zangsi.T_CONT_TEMPLIST_HASH "
						 " (  id , hash_1m , hash_1m_mureka , reg_date , reg_time , depth , default_hash, file_size )"
						 " values ( %lu , '%s', '%s', '%s', '%s',%d,'%s',%15.0f)",
				pfups4006.id, pfups4006hash.hash_1m, pfups4006hash.hash_1m_mureka, pRegDate, pRegTime, pfups4006.depth, pfups4006.default_hash, pfups4006.file_size);
	}
	else if (strlen(pfups4006hash.hash_1m) > 0 && strlen(pfups4006hash.hash_1m_mureka) <= 0)
	{
		sprintf(szQuery, "insert into zangsi.T_CONT_TEMPLIST_HASH "
						 " (  id , hash_1m ,  reg_date , reg_time  , depth , default_hash, file_size )"
						 " values ( %lu , '%s', '%s', '%s',%d,'%s',%15.0f)",
				pfups4006.id, pfups4006hash.hash_1m, pRegDate, pRegTime, pfups4006.depth, pfups4006.default_hash, pfups4006.file_size);
	}
	else if (strlen(pfups4006hash.hash_1m) <= 0 && strlen(pfups4006hash.hash_1m_mureka) > 0)
	{
		sprintf(szQuery, "insert into zangsi.T_CONT_TEMPLIST_HASH "
						 " (  id , hash_1m_mureka ,  reg_date , reg_time  , depth , default_hash, file_size )"
						 " values ( %lu , '%s', '%s', '%s',%d,'%s',%15.0f)",
				pfups4006.id, pfups4006hash.hash_1m_mureka, pRegDate, pRegTime, pfups4006.depth, pfups4006.default_hash, pfups4006.file_size);
	}
	else
	{
		infLOG(ALWAY, "fups4006->Insert1mbHash 1mb hash is empty [ %lu ]\n", pfups4006.id);
		return -1;
	}

	infLOG(ALWAY, "fups4006->Insert1mbHash QUERY [ %s ]\n", szQuery);
	if (MysqlCon.MysqlQuery(con, szSysErrMsg, szQuery))
	{
		infLOG(ERROR, "fups4006->InsertMurekaVideo : MysqlQuery error.\n");
		infLOG(ERROR, "fups4006->InsertMurekaVideo : [%d](%s)[%s]\n", mysql_errno(con), mysql_error(con), szQuery);
	}

	return 0;
}
