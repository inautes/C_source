/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : cprdaem2001.cc
 *         기능 : 정보가 변경된 제휴건과 zangsi 디비의 데이터를 일괄 업데이트.
 *         설명 :
 *     설치위치 : 유료컨텐츠DB에 위치
 *
 *       작성자 : HCS
 *       작성일 : 2009/06/16
 *     수정이력 : 2010/09/16 - SBS이벤트(no.677):nmnt_cnt에 입력될 가격(무조건 개당 50)
				  2010/10/15 - 제휴 필드 추가. (no.741)
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#include <mysql.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h> //for sleep()
#include <math.h>

#include "daemcom.h"
#include "commydb.h"

#define S_OK 0
#define S_FAIL -1
#define S_NODATA 1
#define S_DBERR 2
#define SERVICE_ALL 1
#define SERVICE_PC 2
#define SERVICE_MOBILE 3

int cprdaem2001_process_init();
int cprdaem2001_process();
int cprdaem2001_process_term();
void cprdaem2001_signal(int nSignal);

int UpdateCpr(unsigned long ulListId, unsigned long ulSeqNo, int nChapter, char *pTitle, char *pAdultYn, char *pMgrCd, char *pCompCd, char *pSectCode, char *pSectSub, int nPriceAmt, int nMobPriceAmt, double dCprPaymentRate, char *pProcCd, char *pServiceGu);
int UpdateCont(unsigned long ulId, char *pAdultYn, char *pSectCode, int nPriceAmt, int nMbcPriceAmt, char *pFileType, char *pServiceGu, int nEvent_no, bool bEvent, int nMobPriceAmt, char *pSectCode);
int UpdateFCont(unsigned long ulId, char *pAdultYn, int nPriceAmt, int nMbcPriceAmt, char *pFileType, int nEvent_no, bool bEvent);

int UpdateCont_N(unsigned long ulId, char *pDelServiceGu);
int UpdateFCont_N(unsigned long ulId);

int CancleCpr(unsigned long ulListId, unsigned long ulSeqNo, char *pServiceGu, int nDelete);
int DeleteCont(unsigned long ulId, char *pDelServiceGu);
int DeleteFCont(unsigned long ulId);

int ContLogWrite(char *pContGu, unsigned long ulId, char *pProcCd);
int LogWrite(unsigned long ulListId, unsigned long ulSeqNo, char *pProcCd);

int Check_Event_Contents(char *cont_gu, unsigned long uContId);

MYSQL *con;
MYSQL *cpr_con;

char greg_date[8 + 1]; // 처리일자
char greg_time[6 + 1]; // 처리시작시간
unsigned long gl_start_no = 0;

//******************************************************************************
//* cprdaem2001 db 처리로직
//******************************************************************************

int cprdaem2001_process()
{
	/*
	1.zangsi_cpr.T_CPR_RESRV_CONT_LIST_SUB 에서 변경/취소건 셀렉트
	2.변경된 내용 CPR DB에 적용. 가격 변경, 제휴 취소일 경우 컨텐츠 정보 변경
	3.취소 건은 관련 컨텐츠를 숨김처리하고, 기간만료일을 7일후로 한다.
	4.변경완료후에는 처리한 zangsi_cpr.T_CPR_RESRV_CONT_LIST_SUB의 데이터를 삭제하고 zangsi_cpr.T_CPR_RESRV_CONT_LIST_SUB_HIST에 기록한다.
	*/

	int nLimtCnt = 0;

	for (;;)
	{
		MYSQL_RES *res;
		MYSQL_ROW row;

		char szQuery[1600];
		memset(szQuery, 0x00, sizeof(szQuery));

		// step 1 조회. service_gu 기준 예약 작업 변경 		A = All, P = PC, M = Mobile		 신규 등록된 제휴 정보를 조회한다. 	*/
		sprintf(szQuery, " select list_id, seq_no, chapter, title, adult_yn, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, mob_price_amt, cpr_payment_rate, proc_cd "
						 " , date_format(now(), '%%Y%%m%%d'), date_format(now(), '%%H%%i%%s'), service_gu "
						 " from zangsi_cpr.T_CPR_RESRV_CONT_LIST_SUB "
						 " where proc_date = date_format(now(), '%%Y%%m%%d') and proc_time <= date_format(now(), '%%H%%i%%s') order by reg_date, reg_time desc limit %d, 100 ; /* cprdaem2001 */ ",
				nLimtCnt);

		nLimtCnt += 100;

		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "cprdaem2001_process: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			return S_FAIL;
		}

		if (!(res = mysql_store_result(cpr_con)))
		{
			ZzLOG(ERROR, "cprdaem2001_process: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			return S_FAIL;
		}
		if (mysql_num_rows(res) == 0)
		{
			ZzLOG(ALWAY, "cprdaem2001_process: 처리할 자료없음.\n");
			mysql_free_result(res);
			return S_NODATA;
		}

		while (row = mysql_fetch_row(res))
		{
			unsigned long ulListId = 0;
			unsigned long ulSeqNo = 0;
			int nPriceAmt = 0;
			int nMobPriceAmt = 0;
			int nChapter = 1;
			double dCprPaymentRate = 0;
			char szTitle[250 + 1], szAdultYn[1 + 1], szMgrCd[50 + 1], szCompCd[6 + 1], szSectCode[2 + 1], szSectSub[2 + 1], szProcCd[1 + 1], szServiceGu[1 + 1];
			memset(szTitle, 0x00, sizeof(szTitle));
			memset(szAdultYn, 0x00, sizeof(szAdultYn));
			memset(szMgrCd, 0x00, sizeof(szMgrCd));
			memset(szCompCd, 0x00, sizeof(szCompCd));
			memset(szSectCode, 0x00, sizeof(szSectCode));
			memset(szSectSub, 0x00, sizeof(szSectSub));
			memset(szProcCd, 0x00, sizeof(szProcCd));
			memset(greg_date, 0x00, sizeof(greg_date));
			memset(greg_time, 0x00, sizeof(greg_time));
			memset(szServiceGu, 0x00, sizeof(szServiceGu));

			ulListId = (unsigned long)getnum(row, 0);
			ulSeqNo = (unsigned long)getnum(row, 1);
			nChapter = (int)getint(row, 2);
			sprintf(szTitle, "%s", getstr(row, 3));
			sprintf(szAdultYn, "%s", getstr(row, 4));
			sprintf(szMgrCd, "%s", getstr(row, 5));
			sprintf(szCompCd, "%s", getstr(row, 6));
			sprintf(szSectCode, "%s", getstr(row, 7));
			sprintf(szSectSub, "%s", getstr(row, 8));
			nPriceAmt = (int)getint(row, 9);
			nMobPriceAmt = (int)getint(row, 10);
			dCprPaymentRate = getnum(row, 11);
			sprintf(szProcCd, "%s", getstr(row, 12));
			sprintf(greg_date, "%s", getstr(row, 13));
			sprintf(greg_time, "%s", getstr(row, 14));
			sprintf(szServiceGu, "%s", getstr(row, 15));

			ReplaceSingleToBackslash(szTitle);

			ZzLOG(ALWAY, "ulListId = %lu\n", ulListId);
			ZzLOG(ALWAY, "ulSeqNo = %lu\n", ulSeqNo);
			ZzLOG(ALWAY, "nChapter = %d\n", nChapter);
			ZzLOG(ALWAY, "szTitle = %s\n", szTitle);
			ZzLOG(ALWAY, "szAdultYn = %s\n", szAdultYn);
			ZzLOG(ALWAY, "szMgrCd = %s\n", szMgrCd);
			ZzLOG(ALWAY, "szCompCd = %s\n", szCompCd);
			ZzLOG(ALWAY, "szSectCode = %s\n", szSectCode);
			ZzLOG(ALWAY, "szSectSub = %s\n", szSectSub);
			ZzLOG(ALWAY, "nPriceAmt = %d\n", nPriceAmt);
			ZzLOG(ALWAY, "dCprPaymentRate = %.2f\n", dCprPaymentRate);
			ZzLOG(ALWAY, "szProcCd = %s\n", szProcCd);
			ZzLOG(ALWAY, "greg_date = %s\n", greg_date);
			ZzLOG(ALWAY, "greg_time = %s\n", greg_time);
			ZzLOG(ALWAY, "szServiceGu = %s\n", szServiceGu);

			// step 2 처리.
			int nRes = 0;
			if (strcmp(szProcCd, "U") == 0)
			{
				nRes = UpdateCpr(ulListId, ulSeqNo, nChapter, szTitle, szAdultYn, szMgrCd, szCompCd, szSectCode, szSectSub, nPriceAmt, nMobPriceAmt, dCprPaymentRate, szProcCd, szServiceGu);
				if (nRes == S_FAIL)
				{
					LogWrite(ulListId, ulSeqNo, "F"); /*에러 처리. 정보 변경중 에러가 발생한 경우*/
				}
			}
			else if (strcmp(szProcCd, "D") == 0)
			{
				nRes = CancleCpr(ulListId, ulSeqNo, szServiceGu, 1);
				if (nRes == S_FAIL)
				{
					LogWrite(ulListId, ulSeqNo, "F"); /*에러 처리. 정보 변경중 에러가 발생한 경우*/
				}
			}
			else if (strcmp(szProcCd, "N") == 0) // 20151125 일반 으로 변경.
			{
				nRes = CancleCpr(ulListId, ulSeqNo, szServiceGu, 0);

				if (nRes == S_FAIL)
				{
					LogWrite(ulListId, ulSeqNo, "F"); /*에러 처리. 정보 변경중 에러가 발생한 경우*/
				}
			}
			else
			{
				ZzLOG(ERROR, "cprdaem2001_process: 처리종류 오류. ulListId=[%lu], ulSeqNo=[%lu], szProcCd=[%s]\n", ulListId, ulSeqNo, szProcCd);
				LogWrite(ulListId, ulSeqNo, "F"); /*에러 처리. 정보 변경중 에러가 발생한 경우*/
			}
			if (nRes == S_DBERR)
				return nRes;

			if (nRes != S_FAIL)
				LogWrite(ulListId, ulSeqNo, "U"); /*로그 기록*/

			usleep(500); // 0.05초 휴면
		}
		mysql_free_result(res);
	}
	return S_OK;
}

int Check_Event_Contents(char *cont_gu, unsigned long uContId)
{

	ZzLOG(ALWAY, "Chech_Event_Label start\n");

	int nEvent_no = 0;

	char szQuery[10000];
	memset(szQuery, 0x00, sizeof(szQuery));
	char szSysErrMsg[255];
	memset(szSysErrMsg, 0x00, sizeof(szSysErrMsg));
	MYSQL_RES *res;
	MYSQL_ROW row;
	// 1. 이벤트 컨텐츠 조회

	sprintf(szQuery, " select b.event_code From T_CONTENTS_CP_FILELIST a , N_EVENT_CONTENTS_INFO b where "
					 " a.cont_gu = '%s' AND a.id = %lu and a.mgr_cd = b.cont_code limit 1; ",
			cont_gu, uContId);

	ZzLOG(ALWAY, "Chech_Event_Label szQuery = %s\n", szQuery);

	if (mysql_query(con, szQuery))
	{
		return 0;
	}
	if (!(res = mysql_store_result(con)))
	{
		return 0;
	}
	if (mysql_num_rows(res) == 0)
	{
		return 0;
	}

	if (row = mysql_fetch_row(res))
	{
		nEvent_no = (unsigned int)getint(row, 0);
	}
	mysql_free_result(res);

	return nEvent_no;
}

/* 	step 2: 업데이트 service_gu 기준 예약 작업 변경 		A = All, P = PC, M = Mobile		*/
int UpdateCpr(unsigned long ulListId, unsigned long ulSeqNo, int nChapter, char *pTitle, char *pAdultYn, char *pMgrCd, char *pCompCd, char *pSectCode, char *pSectSub, int nPriceAmt, int nMobPriceAmt, double dCprPaymentRate, char *pProcCd, char *pServiceGu)
{
	MYSQL_RES *res;
	MYSQL_ROW row;

	bool bIsUdtCont = false;
	bool bIsInsert = false;
	bool bIsUdtContMob = false;
	bool bIsInsertMob = false;

	char adult_yn[2 + 1];
	memset(adult_yn, 0x00, sizeof(adult_yn));
	char list_sect_code[2 + 1];
	memset(list_sect_code, 0x00, sizeof(list_sect_code));
	char list_mgr_code[50 + 1];
	memset(list_mgr_code, 0x00, sizeof(list_mgr_code));
	char list_comp_cd[20 + 1];
	memset(list_comp_cd, 0x00, sizeof(list_comp_cd));

	int nServiceGu = 0;		  // ServiceGu 분류
	int nListPriceAmt = 0;	  // PC 가격정보
	int nListMobPriceAmt = 0; // Mobile 가격정보
	int nListMobChapter = 0;  // Mobile chapter

	char szMob_adult_yn[2 + 1];
	memset(szMob_adult_yn, 0x00, sizeof(szMob_adult_yn));
	char szMob_list_sect_code[2 + 1];
	memset(szMob_list_sect_code, 0x00, sizeof(szMob_list_sect_code));
	char szMob_list_mgr_code[50 + 1];
	memset(szMob_list_mgr_code, 0x00, sizeof(szMob_list_mgr_code));
	char szMob_list_comp_cd[20 + 1];
	memset(szMob_list_comp_cd, 0x00, sizeof(szMob_list_comp_cd));
	char szMobile_chk[2];
	memset(szMobile_chk, 0x00, sizeof(szMobile_chk));

	//---------------------------------------------------------
	// 2015.08.12 테이블명 service_gu 분류별 교체
	// service_gu    A = All, P = Pc, M = Mobile
	// SERVICE_ALL = 1, SERVICE_PC = 2, SERVICE_MOBILE = 3
	//---------------------------------------------------------
	ZzLOG(ALWAY, "pServiceGu = %s", pServiceGu);

	if (strcmp(pServiceGu, "A") == 0)
	{
		nServiceGu = SERVICE_ALL;
	}
	else if (strcmp(pServiceGu, "P") == 0)
	{
		nServiceGu = SERVICE_PC;
	}
	else
	{
		nServiceGu = SERVICE_MOBILE;
	}

	//---------------------------------------------------------
	// 2015.08.12 테이블명 service_gu 분류별 교체
	// A = All, P = Pc, M = Mobile
	// T_CPR_RESRV_CONT_LIST_SUB_HIST 조회한값으로 확인
	//---------------------------------------------------------
	char szQuery[2048];
	switch (nServiceGu)
	{

		memset(szQuery, 0x00, sizeof(szQuery));

	case SERVICE_ALL: // All
	{
		ZzLOG(ALWAY, "SERVICE_ALL \n");
		ZzLOG(ALWAY, "SERVICE_ALL PC \n");
		/* PC 제휴물 상태 정보를 조회 */
		sprintf(szQuery, "SELECT a.price_amt, a.adult_yn, a.sect_code, a.mgr_cd, a.comp_cd, "
						 " ifnull(b.price_amt,-1) AS mob_price_amt, b.adult_yn AS mob_adult_yn, b.sect_code AS mob_sect_code, b.mobile_chk AS mob_mobile_chk "
						 " FROM zangsi_cpr.T_CPR_CONT_LIST a LEFT JOIN zangsi_cpr.T_CPR_MOB_CONT_LIST b ON a.list_id = b.list_id "
						 " WHERE a.list_id = %lu ; /* cprdaem2001 */ ",
				ulListId);

		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			return S_FAIL;
		}

		if (!(res = mysql_store_result(cpr_con)))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			return S_FAIL;
		}
		if (mysql_num_rows(res) == 0)
		{
			ZzLOG(ERROR, "UpdateCpr: 처리할 자료없음. ulListId=[%lu], \n", ulListId);
			ZzLOG(ERROR, "(%s)\n", szQuery);
			mysql_free_result(res);
			return S_FAIL;
		}

		row = mysql_fetch_row(res);

		nListPriceAmt = (int)getint(row, 0);
		sprintf(adult_yn, "%s", getstr(row, 1));
		sprintf(list_sect_code, "%s", getstr(row, 2));
		sprintf(list_mgr_code, "%s", getstr(row, 3));
		sprintf(list_comp_cd, "%s", getstr(row, 4));
		nListMobPriceAmt = (int)getint(row, 5);
		sprintf(szMob_adult_yn, "%s", getstr(row, 6));
		sprintf(szMob_list_sect_code, "%s", getstr(row, 7));
		sprintf(szMobile_chk, "%s", getstr(row, 8));

		if (strlen(adult_yn) == 0)
			strcpy(adult_yn, pAdultYn);
		if (strlen(list_sect_code) == 0)
			strcpy(list_sect_code, pSectCode);
		if (strlen(szMob_adult_yn) == 0)
			strcpy(szMob_adult_yn, pAdultYn);
		if (strlen(szMob_list_sect_code) == 0)
			strcpy(szMob_list_sect_code, pSectCode);

		mysql_free_result(res);

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "select max(seq_no) from zangsi_cpr.T_CPR_CONT_LIST where list_id = %lu ; /* cprdaem2001 */ ", ulListId);

		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
			ZzLOG(ERROR, "(%s)[%d](%s)\n", szQuery, mysql_errno(cpr_con), mysql_error(cpr_con));
			return S_FAIL;
		}

		if (!(res = mysql_store_result(cpr_con)))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_store_result error...\n");
			ZzLOG(ERROR, "(%s)[%d](%s)\n", szQuery, mysql_errno(cpr_con), mysql_error(cpr_con));
			return S_FAIL;
		}
		if (mysql_num_rows(res) == 0)
		{
			ZzLOG(ERROR, "UpdateCpr: 처리할 자료없음. ulListId=[%lu], \n", ulListId);
			ZzLOG(ERROR, "(%s)[%d](%s)\n", szQuery, mysql_errno(cpr_con), mysql_error(cpr_con));
			mysql_free_result(res);
			return S_FAIL;
		}

		row = mysql_fetch_row(res);
		int list_seq_no = 0;
		list_seq_no = (int)getint(row, 0);
		list_seq_no++; //신규 seq_no
		mysql_free_result(res);

		ZzLOG(ALWAY, "111111111 \n");
		if (nListPriceAmt != nPriceAmt || strcmp(adult_yn, pAdultYn) != 0 || strcmp(list_sect_code, pSectCode) != 0 || nListMobPriceAmt != nPriceAmt || strcmp(szMob_adult_yn, pAdultYn) != 0 || strcmp(szMob_list_sect_code, pSectCode) != 0)
		{
			bIsUdtCont = true;
		}

		if (strcmp(list_mgr_code, pMgrCd) != 0 || strcmp(list_comp_cd, pCompCd) != 0)
			bIsInsert = true;

		if (bIsUdtCont == false) // LOG
		{
			ZzLOG(ALWAY, "nListPriceAmt =%d, nPriceAmt = %d\n", nListPriceAmt, nPriceAmt);
			ZzLOG(ALWAY, "adult_yn =%s, pAdultYn = %s\n", adult_yn, pAdultYn);
			ZzLOG(ALWAY, "list_sect_code =%s, pSectCode = %s\n", list_sect_code, pSectCode);
			ZzLOG(ALWAY, "nListMobPriceAmt =%d, nPriceAmt = %d\n", nListMobPriceAmt, nPriceAmt);
			ZzLOG(ALWAY, "szMob_adult_yn =%s, pAdultYn = %s\n", szMob_adult_yn, pAdultYn);
			ZzLOG(ALWAY, "szMob_list_sect_code =%s, pSectCode = %s\n", szMob_list_sect_code, pSectCode);

			bIsUdtCont = true; // 무조건 업데이트 해주도록 변경.
		}

		if (tran_begin(cpr_con) != 0)
		{
			ZzLOG(ERROR, "UpdateCpr: 트렌젝션 오류.\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			return S_DBERR;
		}

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi_cpr.T_CPR_RESRV_CONT_LIST_SUB_HIST  "
						 " (list_id, resrv_seq_no, title, adult_yn, mgr_cd, comp_cd "
						 " , sect_code, sect_sub, price_amt, cpr_payment_rate, proc_date, proc_time, proc_cd "
						 " , reg_user, reg_date, reg_time, udt_user, udt_date, udt_time, proc_stat, service_gu) "
						 " select "
						 " b.list_id, b.seq_no, a.title, a.adult_yn, a.mgr_cd, a.comp_cd"
						 " , a.sect_code, a.sect_sub, a.price_amt, a.cpr_payment_rate, b.proc_date, b.proc_time, b.proc_cd "
						 " , a.reg_user, a.reg_date, a.reg_time, 'sys2001', '%s', '%s', 'H', b.service_gu "
						 " from zangsi_cpr.T_CPR_CONT_LIST a, zangsi_cpr.T_CPR_RESRV_CONT_LIST_SUB b "
						 " where a.list_id = b.list_id and b.list_id = %lu and b.seq_no = %lu ; /* cprdaem2001 */ ",
				greg_date, greg_time, ulListId, ulSeqNo);

		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return S_FAIL;
		}

		// T_CPR_CONT_LIST_HIST 에 추가
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update zangsi_cpr.T_CPR_CONT_LIST "
						 " set seq_no = %d,title = '%s', chapter ='%d' , mgr_cd = '%s' , comp_cd = '%s' , sect_code = '%s', sect_sub = '%s'"
						 " , price_amt =  %d, adult_yn = '%s' , reg_user = 'sys2001', apply_yn = 'Y', cpr_payment_rate = '%.2f' " //, reg_date = '%s', reg_time = '%s'
						 " where list_id = %lu ; /* cprdaem2001 */ ",
				list_seq_no, pTitle, nChapter, pMgrCd, pCompCd, pSectCode, pSectSub, nPriceAmt, pAdultYn, dCprPaymentRate, ulListId); //, greg_date, greg_time

		ZzLOG(ALWAY, "pc szQuery = %s\n", szQuery);

		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return S_FAIL;
		}

		// T_CPR_CONT_LIST_HIST 에 추가
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi_cpr.T_CPR_CONT_LIST_HIST "
						 " (list_id ,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate "
						 " , udt_user , udt_date,udt_time ) "
						 " values "
						 " (%lu , %d, '%s', '%d', '%s', '%s', '%s', '%s', %d, '%s', 'sys2001', '%s', '%s', 'Y', '%.2f' "
						 " , 'sys2001' , date_format(now(), '%%Y%%m%%d'),  date_format(now(), '%%H%%i%%s')  ) ; /* cprdaem2001 */ ",
				ulListId, list_seq_no, pTitle, nChapter, pMgrCd, pCompCd, pSectCode, pSectSub, nPriceAmt, pAdultYn, greg_date, greg_time, dCprPaymentRate);

		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return S_FAIL;
		}

		ZzLOG(ALWAY, "SERVICE_ALL Mobile \n");

		/* Mobile 제휴 정보를 조회 */
		if (nListMobPriceAmt != -1)
		{

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "select max(seq_no) from zangsi_cpr.T_CPR_MOB_CONT_LIST_HIST where list_id = %lu ; /* cprdaem2001 */ ", ulListId);

			if (mysql_query(cpr_con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				return S_FAIL;
			}

			if (!(res = mysql_store_result(cpr_con)))
			{
				ZzLOG(ERROR, "UpdateCpr: mysql_store_result error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				return S_FAIL;
			}
			if (mysql_num_rows(res) == 0)
			{
				ZzLOG(ERROR, "UpdateCpr: 처리할 자료없음. ulListId=[%lu], \n", ulListId);
				ZzLOG(ERROR, "(%s)\n", szQuery);
				mysql_free_result(res);
				return S_FAIL;
			}

			int nMob_list_seq_no = 0;
			nMob_list_seq_no = (int)getint(row, 0);
			nMob_list_seq_no++; //신규 seq_no

			mysql_free_result(res);

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update zangsi_cpr.T_CPR_MOB_CONT_LIST "
							 " set seq_no = %d,title = '%s', chapter ='%d' , mgr_cd = '%s' , comp_cd = '%s' , sect_code = '%s', sect_sub = '%s'"
							 " , price_amt =  %d, adult_yn = '%s' , reg_user = 'sys2001', apply_yn = 'Y', cpr_payment_rate = '%.2f' " //, reg_date = '%s', reg_time = '%s'
							 " where list_id = %lu ; /* cprdaem2001 */ ",
					nMob_list_seq_no, pTitle, nChapter, pMgrCd, pCompCd, pSectCode, pSectSub, nMobPriceAmt, pAdultYn, dCprPaymentRate, ulListId); //, greg_date, greg_time

			ZzLOG(ALWAY, "mob szQuery = %s\n", szQuery);

			if (mysql_query(cpr_con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				tran_rollback(cpr_con);
				tran_end(cpr_con);
				return S_FAIL;
			}

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi_cpr.T_CPR_MOB_CONT_LIST_HIST "
							 " (list_id ,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate "
							 " , udt_user , udt_date,udt_time , mobile_chk ) "
							 " values "
							 " (%lu , %d, '%s', '%d', '%s', '%s', '%s', '%s', %d, '%s', 'sys2001', '%s', '%s', 'Y', '%.2f' "
							 " , 'sys2001' , date_format(now(), '%%Y%%m%%d'),  date_format(now(), '%%H%%i%%s') , '%s' ) ; /* cprdaem2001 */ ",
					ulListId, nMob_list_seq_no, pTitle, nChapter, pMgrCd, pCompCd, pSectCode, pSectSub, nMobPriceAmt, pAdultYn, greg_date, greg_time, dCprPaymentRate, szMobile_chk);

			if (mysql_query(cpr_con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				tran_rollback(cpr_con);
				tran_end(cpr_con);
				return S_FAIL;
			}
		}

		break;
	}
	case SERVICE_PC: // PC
	{
		ZzLOG(ALWAY, "SERVICE_PC \n");
		/* PC 제휴물 상태 정보를 조회 */
		sprintf(szQuery, "select price_amt, adult_yn, sect_code, mgr_cd, comp_cd, chapter from zangsi_cpr.T_CPR_CONT_LIST where list_id = %lu ; /* cprdaem2001 */ ", ulListId);

		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			return S_FAIL;
		}

		if (!(res = mysql_store_result(cpr_con)))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			return S_FAIL;
		}
		if (mysql_num_rows(res) == 0)
		{
			ZzLOG(ERROR, "UpdateCpr: 처리할 자료없음. ulListId=[%lu], \n", ulListId);
			ZzLOG(ERROR, "(%s)\n", szQuery);
			mysql_free_result(res);
			return S_FAIL;
		}

		row = mysql_fetch_row(res);

		nListPriceAmt = (int)getint(row, 0);
		sprintf(adult_yn, "%s", getstr(row, 1));
		sprintf(list_sect_code, "%s", getstr(row, 2));
		sprintf(list_mgr_code, "%s", getstr(row, 3));
		sprintf(list_comp_cd, "%s", getstr(row, 4));

		if (strlen(adult_yn) == 0)
			strcpy(adult_yn, pAdultYn);
		if (strlen(list_sect_code) == 0)
			strcpy(list_sect_code, pSectCode);

		mysql_free_result(res);

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "select max(seq_no) from zangsi_cpr.T_CPR_CONT_LIST where list_id = %lu ; /* cprdaem2001 */ ", ulListId);

		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
			ZzLOG(ERROR, "(%s)[%d](%s)\n", szQuery, mysql_errno(cpr_con), mysql_error(cpr_con));
			return S_FAIL;
		}

		if (!(res = mysql_store_result(cpr_con)))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_store_result error...\n");
			ZzLOG(ERROR, "(%s)[%d](%s)\n", szQuery, mysql_errno(cpr_con), mysql_error(cpr_con));
			return S_FAIL;
		}
		if (mysql_num_rows(res) == 0)
		{
			ZzLOG(ERROR, "UpdateCpr: 처리할 자료없음. ulListId=[%lu], \n", ulListId);
			ZzLOG(ERROR, "(%s)[%d](%s)\n", szQuery, mysql_errno(cpr_con), mysql_error(cpr_con));
			mysql_free_result(res);
			return S_FAIL;
		}

		row = mysql_fetch_row(res);

		int list_seq_no = 0;
		list_seq_no = (int)getint(row, 0);
		list_seq_no++; //신규 seq_no

		ZzLOG(ALWAY, "22222 \n");
		if (nListPriceAmt != nPriceAmt || strcmp(adult_yn, pAdultYn) != 0 || strcmp(list_sect_code, pSectCode) != 0)
		{
			bIsUdtCont = true;
		}

		if (strcmp(list_mgr_code, pMgrCd) != 0 || strcmp(list_comp_cd, pCompCd) != 0)
			bIsInsert = true;

		if (tran_begin(cpr_con) != 0)
		{
			ZzLOG(ERROR, "UpdateCpr: 트렌젝션 오류.\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			return S_DBERR;
		}

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi_cpr.T_CPR_RESRV_CONT_LIST_SUB_HIST  "
						 " (list_id, resrv_seq_no, title, adult_yn, mgr_cd, comp_cd "
						 " , sect_code, sect_sub, price_amt, cpr_payment_rate, proc_date, proc_time, proc_cd "
						 " , reg_user, reg_date, reg_time, udt_user, udt_date, udt_time, proc_stat, service_gu) "
						 " select "
						 " b.list_id, b.seq_no, a.title, a.adult_yn, a.mgr_cd, a.comp_cd"
						 " , a.sect_code, a.sect_sub, a.price_amt, a.cpr_payment_rate, b.proc_date, b.proc_time, b.proc_cd "
						 " , a.reg_user, a.reg_date, a.reg_time, 'sys2001', '%s', '%s', 'H' , b.service_gu "
						 " from zangsi_cpr.T_CPR_CONT_LIST a, zangsi_cpr.T_CPR_RESRV_CONT_LIST_SUB b "
						 " where a.list_id = b.list_id and b.list_id = %lu and b.seq_no = %lu ; /* cprdaem2001 */",
				greg_date, greg_time, ulListId, ulSeqNo);

		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return S_FAIL;
		}

		// T_CPR_CONT_LIST_HIST 에 추가
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update zangsi_cpr.T_CPR_CONT_LIST "
						 " set seq_no = %d,title = '%s', chapter ='%d' , mgr_cd = '%s' , comp_cd = '%s' , sect_code = '%s', sect_sub = '%s'"
						 " , price_amt =  %d, adult_yn = '%s' , reg_user = 'sys2001', apply_yn = 'Y', cpr_payment_rate = '%.2f' " //, reg_date = '%s', reg_time = '%s'
						 " where list_id = %lu ; /* cprdaem2001 */ ",
				list_seq_no, pTitle, nChapter, pMgrCd, pCompCd, pSectCode, pSectSub, nPriceAmt, pAdultYn, dCprPaymentRate, ulListId); //, greg_date, greg_time

		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return S_FAIL;
		}

		// T_CPR_CONT_LIST_HIST 에 추가
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi_cpr.T_CPR_CONT_LIST_HIST "
						 " (list_id ,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate "
						 " , udt_user , udt_date,udt_time ) "
						 " values "
						 " (%lu , %d, '%s', '%d', '%s', '%s', '%s', '%s', %d, '%s', 'sys2001', '%s', '%s', 'Y', '%.2f' "
						 " , 'sys2001' , date_format(now(), '%%Y%%m%%d'),  date_format(now(), '%%H%%i%%s')  ) ; /* cprdaem2001 */",
				ulListId, list_seq_no, pTitle, nChapter, pMgrCd, pCompCd, pSectCode, pSectSub, nPriceAmt, pAdultYn, greg_date, greg_time, dCprPaymentRate);

		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return S_FAIL;
		}
		break;
	}
	case SERVICE_MOBILE: // Mobile
	{
		ZzLOG(ALWAY, "SERVICE_MOBILE \n");
		/* Mobile 제휴물 상태 정보를 조회 T_CPR_MOB_CONT_LIST_HIST */
		sprintf(szQuery, "select price_amt, adult_yn, sect_code, mobile_chk from zangsi_cpr.T_CPR_MOB_CONT_LIST where list_id = %lu ; /* cprdaem2001 */ ", ulListId);
		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			return S_FAIL;
		}

		if (!(res = mysql_store_result(cpr_con)))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			return S_FAIL;
		}
		if (mysql_num_rows(res) == 0)
		{
			ZzLOG(ERROR, "UpdateCpr: 처리할 자료없음. ulListId=[%lu], \n", ulListId);
			ZzLOG(ERROR, "(%s)\n", szQuery);
			mysql_free_result(res);
			return S_FAIL;
		}

		row = mysql_fetch_row(res);

		nListMobPriceAmt = (int)getint(row, 0);
		sprintf(szMob_adult_yn, "%s", getstr(row, 1));
		sprintf(szMob_list_sect_code, "%s", getstr(row, 2));
		sprintf(szMobile_chk, "%s", getstr(row, 3));

		mysql_free_result(res);

		if (strlen(szMob_adult_yn) == 0)
			strcpy(szMob_adult_yn, pAdultYn);
		if (strlen(szMob_list_sect_code) == 0)
			strcpy(szMob_list_sect_code, pSectCode);

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "select max(seq_no) from zangsi_cpr.T_CPR_MOB_CONT_LIST_HIST where list_id = %lu ; /* cprdaem2001 */ ", ulListId);

		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
			ZzLOG(ERROR, "(%s)[%d](%s)\n", szQuery, mysql_errno(cpr_con), mysql_error(cpr_con));
			return S_FAIL;
		}

		if (!(res = mysql_store_result(cpr_con)))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_store_result error...\n");
			ZzLOG(ERROR, "(%s)[%d](%s)\n", szQuery, mysql_errno(cpr_con), mysql_error(cpr_con));
			return S_FAIL;
		}
		if (mysql_num_rows(res) == 0)
		{
			ZzLOG(ERROR, "UpdateCpr: 처리할 자료없음. ulListId=[%lu], \n", ulListId);
			ZzLOG(ERROR, "(%s)[%d](%s)\n", szQuery, mysql_errno(cpr_con), mysql_error(cpr_con));
			mysql_free_result(res);
			return S_FAIL;
		}

		row = mysql_fetch_row(res);
		int nMob_list_seq_no = 0;
		nMob_list_seq_no = (int)getint(row, 0);
		nMob_list_seq_no++; //신규 seq_no
		mysql_free_result(res);

		ZzLOG(ALWAY, "33333 \n");
		if (nListMobPriceAmt != nPriceAmt || strcmp(szMob_adult_yn, pAdultYn) != 0 || strcmp(szMob_list_sect_code, pSectCode) != 0)
		{
			bIsUdtCont = true;
		}

		if (tran_begin(cpr_con) != 0)
		{
			ZzLOG(ERROR, "UpdateCpr: 트렌젝션 오류.\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			return S_DBERR;
		}

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi_cpr.T_CPR_RESRV_CONT_LIST_SUB_HIST  "
						 " (list_id, resrv_seq_no, title, adult_yn, mgr_cd, comp_cd "
						 " , sect_code, sect_sub, price_amt, cpr_payment_rate, proc_date, proc_time, proc_cd "
						 " , reg_user, reg_date, reg_time, udt_user, udt_date, udt_time, proc_stat, service_gu) "
						 " select "
						 " b.list_id, b.seq_no, a.title, a.adult_yn, a.mgr_cd, a.comp_cd"
						 " , a.sect_code, a.sect_sub, a.price_amt, a.cpr_payment_rate, b.proc_date, b.proc_time, b.proc_cd "
						 " , a.reg_user, a.reg_date, a.reg_time, 'sys2001', '%s', '%s', 'H' , b.service_gu "
						 " from zangsi_cpr.T_CPR_MOB_CONT_LIST a, zangsi_cpr.T_CPR_RESRV_CONT_LIST_SUB b "
						 " where a.list_id = b.list_id and b.list_id = %lu and b.seq_no = %lu ; /* cprdaem2001 */ ",
				greg_date, greg_time, ulListId, ulSeqNo);

		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return S_FAIL;
		}

		// T_CPR_MOB_CONT_LIST_HIST 에 추가
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update zangsi_cpr.T_CPR_MOB_CONT_LIST "
						 " set seq_no = %d,title = '%s', chapter ='%d' , mgr_cd = '%s' , comp_cd = '%s' , sect_code = '%s', sect_sub = '%s'"
						 " , price_amt =  %d, adult_yn = '%s' , reg_user = 'sys2001', apply_yn = 'Y', cpr_payment_rate = '%.2f' , mobile_chk = 'Y' " //, reg_date = '%s', reg_time = '%s'
						 " where list_id = %lu ; /* cprdaem2001 */ ",
				nMob_list_seq_no, pTitle, nChapter, pMgrCd, pCompCd, pSectCode, pSectSub, nMobPriceAmt, pAdultYn, dCprPaymentRate, ulListId); //, greg_date, greg_time

		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return S_FAIL;
		}

		// T_CPR_MOB_CONT_LIST_HIST 에 추가
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi_cpr.T_CPR_MOB_CONT_LIST_HIST "
						 " (list_id ,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate "
						 " , udt_user , udt_date,udt_time , mobile_chk ) "
						 " values "
						 " (%lu , %d, '%s', '%d', '%s', '%s', '%s', '%s', %d, '%s', 'sys2001', '%s', '%s', 'Y', '%.2f' "
						 " , 'sys2001' , date_format(now(), '%%Y%%m%%d'),  date_format(now(), '%%H%%i%%s') , '%s' ) ; /* cprdaem2001 */ ",
				ulListId, nMob_list_seq_no, pTitle, nChapter, pMgrCd, pCompCd, pSectCode, pSectSub, nMobPriceAmt, pAdultYn, greg_date, greg_time, dCprPaymentRate, szMobile_chk);

		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return S_FAIL;
		}
		break;
	}
	}

	////////////////////////////////////////////////////////////////////////

	/*
		2015.4.20 추가 작업 예정
		T_CPR_MOB_CONT_LIST 와 T_CPR_MOB_CONT_LIST_HIST 1:1 매핑이 되어야 함. 현재 안되어 있어서 T_CPR_CONT_LIST 에서 정보를 가져온다.
		가져온 정보로 모바일 서비스를 하는지 체크하고 모바일 서비스를 한다면 T_CPR_MOB_CONT_LIST, T_CPR_MOB_CONT_LIST_HIST 인서트한다.
	*/

	ZzLOG(ALWAY, "step2==\n");

	// T_CPR_HASH_INFO , FILE 정보 업데이트
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "select chi_id from zangsi_cpr.T_CPR_HASH_INFO where list_id = %lu and proc_stat = 'C' ; /* cprdaem2001 */ ", ulListId);

	if (mysql_query(cpr_con, szQuery))
	{
		ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		tran_rollback(cpr_con);
		tran_end(cpr_con);
		return S_FAIL;
	}

	if (!(res = mysql_store_result(cpr_con)))
	{
		ZzLOG(ERROR, "UpdateCpr: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		tran_rollback(cpr_con);
		tran_end(cpr_con);
		return S_FAIL;
	}

	if (mysql_num_rows(res) == 0)
	{
		ZzLOG(ALWAY, "UpdateCpr: 처리할 해시 자료없음. ulListId=[%lu], \n", ulListId);
		ZzLOG(ALWAY, "(%s)\n", szQuery);
		mysql_free_result(res);
	}
	else
	{
		while (row = mysql_fetch_row(res))
		{
			unsigned long ulChiId = 0;
			ulChiId = (unsigned long)getnum(row, 0);

			char szDesc[512];
			memset(szDesc, 0x00, sizeof(szDesc));
			if (strcmp(pServiceGu, "A") == 0)
			{
				if (bIsInsert)
				{
					sprintf(szDesc, " set a.title = '%s', a.comp_cd = '%s', b.comp_cd = '%s', a.sect_code = '%s', a.sect_sub = '%s', a.price_amt = %d, a.adult_yn = '%s', a.cpr_payment_rate = '%.2f'  ", pTitle, pCompCd, pCompCd, pSectCode, pSectSub, nPriceAmt, pAdultYn, dCprPaymentRate);
				}
				else
				{
					sprintf(szDesc, " set a.title = '%s', a.sect_code = '%s', a.sect_sub = '%s', a.price_amt = %d, a.adult_yn = '%s', a.cpr_payment_rate = '%.2f'  ", pTitle, pSectCode, pSectSub, nPriceAmt, pAdultYn, dCprPaymentRate);
				}

				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " update zangsi_cpr.T_CPR_HASH_INFO a,  zangsi_cpr.T_CPR_HASH_FILE b "
								 " %s "
								 " where a.chi_id = b.chi_id and a.chi_id = %lu ; /* cprdaem2001 */",
						szDesc, ulChiId);

				if (mysql_query(cpr_con, szQuery))
				{
					ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
					ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
					ZzLOG(ERROR, "(%s)\n", szQuery);
					mysql_free_result(res);
					tran_rollback(cpr_con);
					tran_end(cpr_con);
					return S_FAIL;
				}

				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " insert into zangsi_cpr.T_CPR_PRICE_HIST (chi_id, comp_cd, mgr_cd, apply_date, apply_time, price_amt, cpr_payment_rate, cpr_desc, reg_user, reg_date, reg_time) "
								 " values "
								 " (%lu, '%s', '%s', '%s', '%s', %d, '%.2f', '제휴 변경 예약 시스템', 'sys2001', '%s', '%s') ; /* cprdaem2001 */ ",
						ulChiId, pCompCd, pMgrCd, greg_date, greg_time, nPriceAmt, dCprPaymentRate, greg_date, greg_time);

				if (mysql_query(cpr_con, szQuery))
				{
					ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
					ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
					ZzLOG(ERROR, "(%s)\n", szQuery);
					mysql_free_result(res);
					tran_rollback(cpr_con);
					tran_end(cpr_con);
					return S_FAIL;
				}

				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " select b.default_hash, b.file_size from zangsi_cpr.T_CPR_HASH_INFO a, zangsi_cpr.T_CPR_HASH_FILE b "
								 " where a.chi_id = b.chi_id and a.chi_id = %lu and proc_stat = 'C' ; /* cprdaem2001 */ ",
						ulChiId);

				// ZzLOG(ALWAY, "UpdateCpr: [%s]\n\n", szQuery);

				if (mysql_query(cpr_con, szQuery))
				{
					ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
					ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
					ZzLOG(ERROR, "(%s)\n", szQuery);
					continue;
				}

				MYSQL_RES *local_res;
				MYSQL_ROW local_row;

				if (!(local_res = mysql_store_result(cpr_con)))
				{
					ZzLOG(ERROR, "UpdateCpr: mysql_store_result error...\n");
					ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
					ZzLOG(ERROR, "(%s)\n", szQuery);
					continue;
				}

				if (mysql_num_rows(local_res) == 0)
				{
					ZzLOG(ALWAY, "UpdateCpr: 처리할 해시 자료없음. ulChiId=[%lu], \n", ulChiId);
					ZzLOG(ALWAY, "(%s)\n", szQuery);
					continue;
				}

				while (local_row = mysql_fetch_row(local_res))
				{
					memset(szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, " update zangsi_cpr.T_CONTENTS_FILELIST_SUB  "
									 " set price_amt = %d "
									 " where default_hash = '%s' and file_size = %.0f and chi_id = %lu  ; /* cprdaem2001 */ ",
							nPriceAmt, getstr(local_row, 0), getnum(local_row, 1), ulChiId);
					if (mysql_query(cpr_con, szQuery))
					{
						ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
						ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
						ZzLOG(ERROR, "(%s)\n", szQuery);
						continue;
					}
				}
				mysql_free_result(local_res);
			}
			else if (strcmp(pServiceGu, "P") == 0)
			{
				if (bIsInsert)
				{
					sprintf(szDesc, " set a.title = '%s', a.comp_cd = '%s', b.comp_cd = '%s', a.sect_code = '%s', a.sect_sub = '%s', a.price_amt = %d, a.adult_yn = '%s', a.cpr_payment_rate = '%.2f'  ", pTitle, pCompCd, pCompCd, pSectCode, pSectSub, nPriceAmt, pAdultYn, dCprPaymentRate);
				}
				else
				{
					sprintf(szDesc, " set a.title = '%s', a.sect_code = '%s', a.sect_sub = '%s', a.price_amt = %d, a.adult_yn = '%s', a.cpr_payment_rate = '%.2f'  ", pTitle, pSectCode, pSectSub, nPriceAmt, pAdultYn, dCprPaymentRate);
				}

				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " update zangsi_cpr.T_CPR_HASH_INFO a,  zangsi_cpr.T_CPR_HASH_FILE b "
								 " %s "
								 " where a.chi_id = b.chi_id and a.chi_id = %lu ; /* cprdaem2001 */",
						szDesc, ulChiId);

				if (mysql_query(cpr_con, szQuery))
				{
					ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
					ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
					ZzLOG(ERROR, "(%s)\n", szQuery);
					mysql_free_result(res);
					tran_rollback(cpr_con);
					tran_end(cpr_con);
					return S_FAIL;
				}

				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " insert into zangsi_cpr.T_CPR_PRICE_HIST (chi_id, comp_cd, mgr_cd, apply_date, apply_time, price_amt, cpr_payment_rate, cpr_desc, reg_user, reg_date, reg_time) "
								 " values "
								 " (%lu, '%s', '%s', '%s', '%s', %d, '%.2f', '제휴 변경 예약 시스템', 'sys2001', '%s', '%s') ; /* cprdaem2001 */ ",
						ulChiId, pCompCd, pMgrCd, greg_date, greg_time, nPriceAmt, dCprPaymentRate, greg_date, greg_time);

				if (mysql_query(cpr_con, szQuery))
				{
					ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
					ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
					ZzLOG(ERROR, "(%s)\n", szQuery);
					mysql_free_result(res);
					tran_rollback(cpr_con);
					tran_end(cpr_con);
					return S_FAIL;
				}

				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " select b.default_hash, b.file_size from zangsi_cpr.T_CPR_HASH_INFO a, zangsi_cpr.T_CPR_HASH_FILE b "
								 " where a.chi_id = b.chi_id and a.chi_id = %lu and proc_stat = 'C' ; /* cprdaem2001 */ ",
						ulChiId);

				// ZzLOG(ALWAY, "UpdateCpr: [%s]\n\n", szQuery);

				if (mysql_query(cpr_con, szQuery))
				{
					ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
					ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
					ZzLOG(ERROR, "(%s)\n", szQuery);
					continue;
				}

				MYSQL_RES *local_res;
				MYSQL_ROW local_row;

				if (!(local_res = mysql_store_result(cpr_con)))
				{
					ZzLOG(ERROR, "UpdateCpr: mysql_store_result error...\n");
					ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
					ZzLOG(ERROR, "(%s)\n", szQuery);
					continue;
				}

				if (mysql_num_rows(local_res) == 0)
				{
					ZzLOG(ALWAY, "UpdateCpr: 처리할 해시 자료없음. ulChiId=[%lu], \n", ulChiId);
					ZzLOG(ALWAY, "(%s)\n", szQuery);
					continue;
				}

				while (local_row = mysql_fetch_row(local_res))
				{
					memset(szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, " update zangsi_cpr.T_CONTENTS_FILELIST_SUB  "
									 " set price_amt = %d "
									 " where default_hash = '%s' and file_size = %.0f and chi_id = %lu  ; /* cprdaem2001 */ ",
							nPriceAmt, getstr(local_row, 0), getnum(local_row, 1), ulChiId);
					if (mysql_query(cpr_con, szQuery))
					{
						ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
						ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
						ZzLOG(ERROR, "(%s)\n", szQuery);
						continue;
					}
				}
				mysql_free_result(local_res);
			}
			else // pServiceGu = M
			{
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " insert into zangsi_cpr.T_CPR_PRICE_HIST (chi_id, comp_cd, mgr_cd, apply_date, apply_time, price_amt, cpr_payment_rate, cpr_desc, reg_user, reg_date, reg_time) "
								 " values "
								 " (%lu, '%s', '%s', '%s', '%s', %d, '%.2f', '제휴 변경 예약 시스템', 'sys2001', '%s', '%s') ; /* cprdaem2001 */ ",
						ulChiId, pCompCd, pMgrCd, greg_date, greg_time, nPriceAmt, dCprPaymentRate, greg_date, greg_time);

				if (mysql_query(cpr_con, szQuery))
				{
					ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
					ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
					ZzLOG(ERROR, "(%s)\n", szQuery);
					mysql_free_result(res);
					tran_rollback(cpr_con);
					tran_end(cpr_con);
					return S_FAIL;
				}
			}

		} // 		while(row = mysql_fetch_row(res)
		mysql_free_result(res);
	}

	////////////////////////////////////////////////////////////////
	ZzLOG(ALWAY, "step3==\n");
	if (tran_commit(cpr_con) != 0)
	{
		ZzLOG(ERROR, "UpdateCpr: 트렌젝션 commit 오류.\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		tran_rollback(cpr_con);
		tran_end(cpr_con);
		return S_DBERR;
	}

	tran_end(cpr_con);

	if (!(con = db_connect_to_main("zangsi")))
	{
		ZzLOG(ERROR, "UpdateCpr: zangsi DB에 접속하지 못 하였습니다...\n");
		return S_DBERR;
	}

	ZzLOG(ALWAY, "bIsUdtCont = (%d)\n", bIsUdtCont);
	// T_CONTENTS_INFO 와 관련된 값이 변경되었을 경우 사용 데이터 변경.
	if (bIsUdtCont)
	{
		unsigned long ulTempListId = 0;
		ulTempListId = ulListId;

		if (strcmp(pServiceGu, "A") == 0 || strcmp(pServiceGu, "P") == 0)
		{
			ZzLOG(ALWAY, "step3 pServiceGu = %s ==\n", pServiceGu);

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " select c.cont_gu, c.id, sum(a.price_amt) "
							 " from zangsi_cpr.T_CPR_CONT_LIST a, zangsi_cpr.T_CPR_HASH_INFO b, zangsi_cpr.T_CPR_CONT_MAP_SUB c "
							 " where a.list_id = %lu and a.list_id = b.list_id and b.chi_id = c.chi_id and b.proc_stat = 'C' "
							 " group by c.cont_gu, c.id ; /* cprdaem2001 */ ",
					ulTempListId);

			if (mysql_query(cpr_con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				db_disconnect(con);
				return S_NODATA;
			}

			if (!(res = mysql_store_result(cpr_con)))
			{
				ZzLOG(ERROR, "UpdateCpr: mysql_store_result error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				db_disconnect(con);
				return S_NODATA;
			}
			if (mysql_num_rows(res) == 0)
			{
				ZzLOG(ALWAY, "UpdateCpr: 처리할 컨텐츠 자료없음. ulListId=[%lu], \n", ulTempListId);
				ZzLOG(ALWAY, "(%s)\n", szQuery);
				mysql_free_result(res);
			}
			else
			{
				while (row = mysql_fetch_row(res))
				{
					MYSQL_RES *res2;
					MYSQL_ROW row2;

					char szContGu[2 + 1];
					memset(szContGu, 0x00, sizeof(szContGu));
					sprintf(szContGu, "%s", getstr(row, 0));

					unsigned long ulId = 0;
					ulId = (unsigned long)getnum(row, 1);

					//" select c.cont_gu, c.id, sum(a.price_amt), sum(if(a.comp_cd in('010022', '010021'), 50, a.price_amt)) "//no.677
					memset(szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, " select c.cont_gu, c.id, sum(a.price_amt ) ,sum(if ( d.event_amt is null , a.price_amt ,d.event_amt)), d.cont_type" //, sum(if(a.comp_cd in('010022', '010021'), 50, a.price_amt)) "
									 " from zangsi_cpr.T_CPR_CONT_LIST a left outer join zangsi_cpr.T_CPR_EVENT_CONT_LIST d "
									 " on a.mgr_cd = d.mgr_cd and d.use_yn='Y' "
									 " , zangsi_cpr.T_CPR_HASH_INFO b, zangsi_cpr.T_CPR_CONT_MAP_SUB c "
									 " where a.list_id = b.list_id and b.chi_id = c.chi_id and b.proc_stat = 'C' and c.id = %lu and c.cont_gu = '%s' "
									 " group by c.cont_gu, c.id ; /* cprdaem2001 */ ",
							ulId, szContGu);

					if (mysql_query(cpr_con, szQuery))
					{
						mysql_free_result(res);
						ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
						ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
						ZzLOG(ERROR, "(%s)\n", szQuery);
						db_disconnect(con);
						return S_NODATA;
					}

					if (!(res2 = mysql_store_result(cpr_con)))
					{
						mysql_free_result(res);
						ZzLOG(ERROR, "UpdateCpr: mysql_store_result error...\n");
						ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
						ZzLOG(ERROR, "(%s)\n", szQuery);
						db_disconnect(con);
						return S_NODATA;
					}
					if (mysql_num_rows(res2) == 0)
					{
						ZzLOG(ALWAY, "UpdateCpr: 처리할 컨텐츠 자료없음. ulListId=[%lu], \n", ulTempListId);
						ZzLOG(ALWAY, "(%s)\n", szQuery);
						mysql_free_result(res2);
						continue;
					}
					while (row2 = mysql_fetch_row(res2))
					{
						int nContPriceAmt = 0;
						nContPriceAmt = (int)getint(row2, 2);

						int nMbcPriceAmt = 0;
						nMbcPriceAmt = (int)getint(row2, 3);

						char szFileType[12];
						memset(szFileType, 0x00, sizeof(szFileType));

						char *pFileType = NULL;
						if (getstr(row2, 4) != NULL && strlen(getstr(row2, 4)) > 0)
						{
							strcpy(szFileType, getstr(row2, 4));
							pFileType = szFileType;
						}

						bool bEvent = false;
						int nEvent_no = 0;
						//						nEvent_no = Check_Event_Contents(szContGu,ulId);
						if (strcmp(pCompCd, "010022") == 0 && nPriceAmt == 300)
						{
							bEvent = true;
							nEvent_no = 138;
						}

						int nRes = 0;
						if (strcmp(szContGu, "WE") == 0)
							nRes = UpdateCont(ulId, pAdultYn, pSectCode, nContPriceAmt, nMbcPriceAmt, pFileType, pServiceGu, nEvent_no, bEvent, nMobPriceAmt, pSectSub);
						else if (strcmp(szContGu, "FD") == 0)
							nRes = UpdateFCont(ulId, pAdultYn, nContPriceAmt, nMbcPriceAmt, pFileType, nEvent_no, bEvent);

						if (nRes == S_OK)
							usleep(500); //컨텐츠 1개 처리후 0.0005초 휴면
						else if (nRes != S_OK)
							ContLogWrite(szContGu, ulId, "U");

						if (nRes == S_DBERR)
						{
							mysql_free_result(res);
							mysql_free_result(res2);
							db_disconnect(con);
							return nRes;
						}
					}
					mysql_free_result(res2);
				}
				mysql_free_result(res);
			}
		}
		else
		{
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " select c.cont_gu, c.id, sum(a.price_amt) "
							 " from zangsi_cpr.T_CPR_MOB_CONT_LIST a, zangsi_cpr.T_CPR_HASH_INFO b, zangsi_cpr.T_CPR_CONT_MAP_SUB c "
							 " where a.list_id = %lu and a.list_id = b.list_id and b.chi_id = c.chi_id and b.proc_stat = 'C' "
							 " group by c.cont_gu, c.id ; /* cprdaem2001 */ ",
					ulTempListId);

			if (mysql_query(cpr_con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				db_disconnect(con);
				return S_NODATA;
			}

			if (!(res = mysql_store_result(cpr_con)))
			{
				ZzLOG(ERROR, "UpdateCpr: mysql_store_result error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				db_disconnect(con);
				return S_NODATA;
			}
			if (mysql_num_rows(res) == 0)
			{
				ZzLOG(ALWAY, "UpdateCpr: 처리할 컨텐츠 자료없음. ulListId=[%lu], \n", ulTempListId);
				ZzLOG(ALWAY, "(%s)\n", szQuery);
				mysql_free_result(res);
			}
			else
			{
				while (row = mysql_fetch_row(res))
				{
					MYSQL_RES *res2;
					MYSQL_ROW row2;

					char szContGu[2 + 1];
					memset(szContGu, 0x00, sizeof(szContGu));
					sprintf(szContGu, "%s", getstr(row, 0));

					unsigned long ulId = 0;
					ulId = (unsigned long)getnum(row, 1);

					//" select c.cont_gu, c.id, sum(a.price_amt), sum(if(a.comp_cd in('010022', '010021'), 50, a.price_amt)) "//no.677
					memset(szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, " select c.cont_gu, c.id, sum(a.price_amt ) ,sum(if ( d.event_amt is null , a.price_amt ,d.event_amt)), d.cont_type" //, sum(if(a.comp_cd in('010022', '010021'), 50, a.price_amt)) "
									 " from zangsi_cpr.T_CPR_MOB_CONT_LIST a left outer join zangsi_cpr.T_CPR_EVENT_CONT_LIST d "
									 " on a.mgr_cd = d.mgr_cd and d.use_yn='Y' "
									 " , zangsi_cpr.T_CPR_HASH_INFO b, zangsi_cpr.T_CPR_CONT_MAP_SUB c "
									 " where a.list_id = b.list_id and b.chi_id = c.chi_id and b.proc_stat = 'C' and c.id = %lu and c.cont_gu = '%s' "
									 " group by c.cont_gu, c.id ; /* cprdaem2001 */ ",
							ulId, szContGu);

					if (mysql_query(cpr_con, szQuery))
					{
						mysql_free_result(res);
						ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
						ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
						ZzLOG(ERROR, "(%s)\n", szQuery);
						db_disconnect(con);
						return S_NODATA;
					}

					if (!(res2 = mysql_store_result(cpr_con)))
					{
						mysql_free_result(res);
						ZzLOG(ERROR, "UpdateCpr: mysql_store_result error...\n");
						ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
						ZzLOG(ERROR, "(%s)\n", szQuery);
						db_disconnect(con);
						return S_NODATA;
					}
					if (mysql_num_rows(res2) == 0)
					{
						ZzLOG(ALWAY, "UpdateCpr: 처리할 컨텐츠 자료없음. ulListId=[%lu], \n", ulTempListId);
						ZzLOG(ALWAY, "(%s)\n", szQuery);
						mysql_free_result(res2);
						continue;
					}
					while (row2 = mysql_fetch_row(res2))
					{
						int nContPriceAmt = 0;
						nContPriceAmt = (int)getint(row2, 2);

						int nMbcPriceAmt = 0;
						nMbcPriceAmt = (int)getint(row2, 3);

						char szFileType[12];
						memset(szFileType, 0x00, sizeof(szFileType));

						char *pFileType = NULL;
						if (getstr(row2, 4) != NULL && strlen(getstr(row2, 4)) > 0)
						{
							strcpy(szFileType, getstr(row2, 4));
							pFileType = szFileType;
						}

						bool bEvent = false;
						int nEvent_no = 0;
						if (strcmp(pCompCd, "010022") == 0 && nPriceAmt == 300)
						{
							bEvent = true;
							nEvent_no = 138;
						}

						int nRes = 0;
						if (strcmp(szContGu, "WE") == 0)
							nRes = UpdateCont(ulId, pAdultYn, pSectCode, nContPriceAmt, nMbcPriceAmt, pFileType, pServiceGu, nEvent_no, bEvent, nMobPriceAmt, pSectSub);
						else if (strcmp(szContGu, "FD") == 0)
							nRes = UpdateFCont(ulId, pAdultYn, nContPriceAmt, nMbcPriceAmt, pFileType, nEvent_no, bEvent);

						if (nRes == S_OK)
							usleep(500); //컨텐츠 1개 처리후 0.0005초 휴면
						else if (nRes != S_OK)
							ContLogWrite(szContGu, ulId, "U");

						if (nRes == S_DBERR)
						{
							mysql_free_result(res);
							mysql_free_result(res2);
							db_disconnect(con);
							return nRes;
						}
					}
					mysql_free_result(res2);
				}
				mysql_free_result(res);
			}
		}
	}

	db_disconnect(con);
	return S_OK;
}

int UpdateCont(unsigned long ulId, char *pAdultYn, char *pSectCode, int nPriceAmt, int nMbcPriceAmt, char *pFileType, char *pServiceGu, int nEvent_no, bool bEvent, int nMobPriceAmt, char *pSectSub)
{
	/*
	1.T_CONTENTS_INFO price_amt, adult_yn, sect_code 업데이트
	2.T_CONTENTS_VIR_ID adult_yn, sect_code 업데이트
	3.T_CONTENTS_CREATE 인서트
	4.T_MEMO_INFO 인서트
	*/

	MYSQL_RES *res;
	MYSQL_ROW row;

	int nRealPriceAmt = 0;
	int nRealPriceAmt_mob = 0;

	// sect code 를 가져온다.
	char szQuery[16384];
	char szSectSub[2 + 1];
	char szUserId[16 + 1];
	char szRealSectCode[2 + 1];
	char szRealAdultYn[1 + 1];
	char szTitle[2048];

	memset(szQuery, 0x00, sizeof(szQuery));
	memset(szSectSub, 0x00, sizeof(szSectSub));
	memset(szUserId, 0x00, sizeof(szUserId));
	memset(szRealSectCode, 0x00, sizeof(szRealSectCode));
	memset(szRealAdultYn, 0x00, sizeof(szRealAdultYn));
	memset(szTitle, 0x00, sizeof(szTitle));

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select a.reg_user, a.price_amt, a.sect_code, a.adult_yn, a.title , d.price_amt as mob_price_amt, a.sect_sub "
					 " FROM zangsi.T_CONTENTS_INFO a LEFT JOIN zangsi.T_MOB_CONTENTS_INFO d ON a.id = d.id, zangsi.T_CONTENTS_FILE b, zangsi.T_CONTENTS_VIR_ID c "
					 " where a.id = b.id and a.id = c.id and a.id = %lu and a.del_yn = 'N' and c.copyright_yn in ('C','H') ; /* cprdaem2001 */ ",
			ulId);

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return S_FAIL;
	}

	if (!(res = mysql_store_result(con)))
	{
		ZzLOG(ERROR, "UpdateCont: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return S_FAIL;
	}
	if (mysql_num_rows(res) == 0)
	{
		ZzLOG(ALWAY, "UpdateCont: 처리할 자료없음. ulId=[%lu], \n", ulId);
		ZzLOG(ALWAY, "(%s)\n", szQuery);
		mysql_free_result(res);
		return S_NODATA;
	}

	row = mysql_fetch_row(res);

	sprintf(szUserId, "%s", getstr(row, 0));
	nRealPriceAmt = (int)getint(row, 1);
	sprintf(szRealSectCode, "%s", getstr(row, 2));
	sprintf(szRealAdultYn, "%s", getstr(row, 3));
	sprintf(szTitle, "%s", getstr(row, 4));
	nRealPriceAmt_mob = (int)getint(row, 5);
	sprintf(szSectSub, "%s", getstr(row, 6));

	mysql_free_result(res);

	if (strcmp(pServiceGu, "A") == 0)
	{
		if (nRealPriceAmt == nPriceAmt && nRealPriceAmt_mob == nPriceAmt && strcmp(szRealSectCode, pSectCode) == 0 && strcmp(szSectSub, pSectSub) == 0 && strcmp(szRealAdultYn, pAdultYn) == 0)
		{
			ZzLOG(ALWAY, "변경할 정보가 없음.[%lu]\n", ulId);
			return S_OK;
		}

		if (nRealPriceAmt == nPriceAmt && nRealPriceAmt_mob == nPriceAmt && strcmp(szRealAdultYn, pAdultYn) == 0 && strcmp(szSectSub, pSectSub) && strcmp(szRealSectCode, "15") == 0)
		{
			ZzLOG(ALWAY, "변경할 정보가 없음.[%lu] 분류 코드 [%s] \n", ulId, szRealSectCode);
			return S_OK;
		}
	}
	else if (strcmp(pServiceGu, "M") == 0)
	{

		if (nRealPriceAmt_mob == nPriceAmt && strcmp(szRealSectCode, pSectCode) == 0 && strcmp(szSectSub, pSectSub) && strcmp(szRealAdultYn, pAdultYn) == 0)
		{
			ZzLOG(ALWAY, "변경할 정보가 없음.[%lu]\n", ulId);
			return S_OK;
		}

		if (nRealPriceAmt_mob == nPriceAmt && strcmp(szRealAdultYn, pAdultYn) == 0 && strcmp(szRealSectCode, "15") == 0)
		{
			ZzLOG(ALWAY, "변경할 정보가 없음.[%lu] 분류 코드 [%s] \n", ulId, szRealSectCode);
			return S_OK;
		}
	}
	else if (strcmp(pServiceGu, "P") == 0)
	{
		if (nRealPriceAmt == nPriceAmt && strcmp(szRealSectCode, pSectCode) == 0 && strcmp(szSectSub, pSectSub) && strcmp(szRealAdultYn, pAdultYn) == 0)
		{
			ZzLOG(ALWAY, "변경할 정보가 없음.[%lu]\n", ulId);
			return S_OK;
		}

		if (nRealPriceAmt == nPriceAmt && strcmp(szRealAdultYn, pAdultYn) == 0 && strcmp(szRealSectCode, "15") == 0)
		{
			ZzLOG(ALWAY, "변경할 정보가 없음.[%lu] 분류 코드 [%s] \n", ulId, szRealSectCode);
			return S_OK;
		}
	}

	/*
	if(strcmp(szRealSectCode, "15") == 0)
		strcpy(pSectCode, szRealSectCode);
*/

	if (tran_begin(con) != 0)
	{
		ZzLOG(ERROR, "UpdateCont: 트렌젝션 오류.\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		return S_FAIL;
	}

	memset(szQuery, 0x00, sizeof(szQuery));
	if (strcmp(szRealSectCode, pSectCode) != 0)
	{
		if (strcmp(pServiceGu, "A") == 0)
		{
			//--------------------------------------------------------------------------
			//소분류 일반코드값 조회
			//--------------------------------------------------------------------------
			sprintf(szQuery, " select minor_code from zangsi.T_MINOR_CODE "
							 " where major_code = '9%s' and minor_name = '일반' ; /* cprdaem2001 */ ",
					pSectCode);

			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				return S_FAIL;
			}

			if (!(res = mysql_store_result(con)))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_store_result error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				return S_FAIL;
			}
			if (mysql_num_rows(res) == 0)
			{
				ZzLOG(ALWAY, "UpdateCont: 처리할 자료없음. ulId=[%lu], \n", ulId);
				ZzLOG(ALWAY, "(%s)\n", szQuery);
				mysql_free_result(res);
				return S_NODATA;
			}
			row = mysql_fetch_row(res);
			if (strcmp(pSectCode, "11") != 0)
			{
				sprintf(szSectSub, "%s", getstr(row, 0));
				ZzLOG(ALWAY, "UpdateCont: SectCode not 11, SectSub=[%s], \n", szSectSub);
			}
			else
			{
				ZzLOG(ALWAY, "UpdateCont: SectCode == 11, SectSub=[%s], \n", szSectSub);
			}
			mysql_free_result(res);

			memset(szQuery, 0x00, sizeof(szQuery));

			ZzLOG(ALWAY, "bEvent: [%d]:%d \n\n", bEvent, nEvent_no);

			sprintf(szQuery, " update zangsi.T_CONTENTS_INFO "
							 " set price_amt = %d, sect_code = '%s', sect_sub = '%s' , adult_yn = '%s', nmnt_cnt = %d , disp_cnt_inc = %d " // MBC 이벤트 : nEvent_no
							 " where id = %lu ; /* cprdaem2001 */ ",
					nPriceAmt, pSectCode, szSectSub, pAdultYn, nMbcPriceAmt, nEvent_no, ulId); // nEvent_no

			ZzLOG(ALWAY, "UpdateCont: [%s]\n\n", szQuery);

			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				tran_rollback(con);
				tran_end(con);
				return S_FAIL;
			}

			if (pFileType != NULL)
			{
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " update zangsi.T_CONTENTS_FILE "
								 " set file_type = '%s'" // 20100630 mbc
								 " where id = %lu ; /* cprdaem2001 */ ",
						pFileType, ulId); // 20100630 mbc
				ZzLOG(ALWAY, "UpdateCont: [%s]\n\n", szQuery);

				if (mysql_query(con, szQuery))
				{
					ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
					ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
					ZzLOG(ERROR, "(%s)\n", szQuery);
					tran_rollback(con);
					tran_end(con);
					return S_FAIL;
				}
			} // if( pFileType != NULL )

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update zangsi.T_MOB_CONTENTS_INFO "
							 " set price_amt = %d "
							 " where id = %lu ; /* cprdaem2001 */ ",
					nMobPriceAmt, ulId); // 20100630 mbc
			ZzLOG(ALWAY, "UpdateCont: [%s]\n\n", szQuery);

			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				tran_rollback(con);
				tran_end(con);
				return S_FAIL;
			}

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update zangsi.T_CONTENTS_VIR_ID "
							 " set sect_code = '%s', sect_sub='%s', adult_yn = '%s' "
							 " where id = %lu ; /* cprdaem2001 */ ",
					pSectCode, szSectSub, pAdultYn, ulId);

			ZzLOG(ALWAY, "UpdateCont: [%s]\n\n", szQuery);

			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				tran_rollback(con);
				tran_end(con);
				return S_FAIL;
			}

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update zangsi.T_CONTENTS_VIR_ID2 "
							 " set sect_code = '%s', sect_sub='%s' ,adult_yn = '%s' "
							 " where id = %lu ; /* cprdaem2001 */ ",
					pSectCode, szSectSub, pAdultYn, ulId);

			ZzLOG(ALWAY, "UpdateCont: [%s]\n\n", szQuery);

			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				tran_rollback(con);
				tran_end(con);
				return S_FAIL;
			}
		}
		else if (strcmp(pServiceGu, "P") == 0)
		{
			//--------------------------------------------------------------------------
			//소분류 일반코드값 조회
			//--------------------------------------------------------------------------
			sprintf(szQuery, " select minor_code from zangsi.T_MINOR_CODE "
							 " where major_code = '9%s' and minor_name = '일반' ; /* cprdaem2001 */ ",
					pSectCode);

			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				return S_FAIL;
			}

			if (!(res = mysql_store_result(con)))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_store_result error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				return S_FAIL;
			}
			if (mysql_num_rows(res) == 0)
			{
				ZzLOG(ALWAY, "UpdateCont: 처리할 자료없음. ulId=[%lu], \n", ulId);
				ZzLOG(ALWAY, "(%s)\n", szQuery);
				mysql_free_result(res);
				return S_NODATA;
			}
			row = mysql_fetch_row(res);
			if (strcmp(pSectCode, "11") != 0)
			{
				sprintf(szSectSub, "%s", getstr(row, 0));
				ZzLOG(ALWAY, "UpdateCont: SectCode not 11, SectSub=[%s], \n", szSectSub);
			}
			else
			{
				ZzLOG(ALWAY, "UpdateCont: SectCode == 11, SectSub=[%s], \n", szSectSub);
			}
			mysql_free_result(res);

			memset(szQuery, 0x00, sizeof(szQuery));

			ZzLOG(ALWAY, "bEvent: [%d]:%d \n\n", bEvent, nEvent_no);

			sprintf(szQuery, " update zangsi.T_CONTENTS_INFO "
							 " set price_amt = %d, sect_code = '%s', sect_sub = '%s' , adult_yn = '%s', nmnt_cnt = %d , disp_cnt_inc = %d " // MBC 이벤트
							 " where id = %lu ; /* cprdaem2001 */ ",
					nPriceAmt, pSectCode, szSectSub, pAdultYn, nMbcPriceAmt, nEvent_no, ulId); // nEvent_no

			ZzLOG(ALWAY, "UpdateCont: [%s]\n\n", szQuery);

			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				tran_rollback(con);
				tran_end(con);
				return S_FAIL;
			}

			if (pFileType != NULL)
			{
				memset(szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " update zangsi.T_CONTENTS_FILE "
								 " set file_type = '%s'" // 20100630 mbc
								 " where id = %lu ; /* cprdaem2001 */ ",
						pFileType, ulId); // 20100630 mbc
				ZzLOG(ALWAY, "UpdateCont: [%s]\n\n", szQuery);

				if (mysql_query(con, szQuery))
				{
					ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
					ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
					ZzLOG(ERROR, "(%s)\n", szQuery);
					tran_rollback(con);
					tran_end(con);
					return S_FAIL;
				}
			} // if( pFileType != NULL )

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update zangsi.T_CONTENTS_VIR_ID "
							 " set sect_code = '%s', sect_sub='%s', adult_yn = '%s' "
							 " where id = %lu ; /* cprdaem2001 */ ",
					pSectCode, szSectSub, pAdultYn, ulId);

			ZzLOG(ALWAY, "UpdateCont: [%s]\n\n", szQuery);

			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				tran_rollback(con);
				tran_end(con);
				return S_FAIL;
			}

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update zangsi.T_CONTENTS_VIR_ID2 "
							 " set sect_code = '%s', sect_sub='%s' ,adult_yn = '%s' "
							 " where id = %lu ; /* cprdaem2001 */ ",
					pSectCode, szSectSub, pAdultYn, ulId);

			ZzLOG(ALWAY, "UpdateCont: [%s]\n\n", szQuery);

			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				tran_rollback(con);
				tran_end(con);
				return S_FAIL;
			}
		}
		else
		{

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update zangsi.T_MOB_CONTENTS_INFO "
							 " set price_amt = %d "
							 " where id = %lu ; /* cprdaem2001 */ ",
					nMobPriceAmt, ulId); // 20100630 mbc
			ZzLOG(ALWAY, "UpdateCont: [%s]\n\n", szQuery);

			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				tran_rollback(con);
				tran_end(con);
				return S_FAIL;
			}
		}

	} // if( strcmp(szRealSectCode , pSectCode) != 0  )
	else
	{
		if (strcmp(pServiceGu, "A") == 0)
		{

			memset(szQuery, 0x00, sizeof(szQuery));

			ZzLOG(ALWAY, "bEvent: [%d]:%d \n\n", bEvent, nEvent_no);

			sprintf(szQuery, " update zangsi.T_CONTENTS_INFO "
							 " set price_amt = %d, sect_code = '%s', sect_sub = '%s', adult_yn = '%s', nmnt_cnt = %d , disp_cnt_inc = %d " // 20100630 mbc
							 " where id = %lu ; /* cprdaem2001 */ ",
					nPriceAmt, pSectCode, szSectSub, pAdultYn, nMbcPriceAmt, nEvent_no, ulId); // 20100630 // nEvent_no

			ZzLOG(ALWAY, "UpdateCont: [%s]\n\n", szQuery);

			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				tran_rollback(con);
				tran_end(con);
				return S_FAIL;
			}

			//-- 2015.09.22 service_gu = A , 모바일 업데이트
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update zangsi.T_MOB_CONTENTS_INFO "
							 " set price_amt = %d "
							 " where id = %lu ; /* cprdaem2001 */ ",
					nMobPriceAmt, ulId);
			ZzLOG(ALWAY, "UpdateCont: [%s]\n\n", szQuery);

			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				tran_rollback(con);
				tran_end(con);
				return S_FAIL;
			}

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update zangsi.T_CONTENTS_VIR_ID "
							 " set sect_code = '%s', sect_sub = '%s', adult_yn = '%s' "
							 " where id = %lu ; /* cprdaem2001 */ ",
					pSectCode, szSectSub, pAdultYn, ulId);

			ZzLOG(ALWAY, "UpdateCont: [%s]\n\n", szQuery);

			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				tran_rollback(con);
				tran_end(con);
				return S_FAIL;
			}

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update zangsi.T_CONTENTS_VIR_ID2 "
							 " set sect_code = '%s', sect_sub = '%s', adult_yn = '%s' "
							 " where id = %lu ; /* cprdaem2001 */ ",
					pSectCode, szSectSub, pAdultYn, ulId);

			ZzLOG(ALWAY, "UpdateCont: [%s]\n\n", szQuery);

			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				tran_rollback(con);
				tran_end(con);
				return S_FAIL;
			}
		}
		else if (strcmp(pServiceGu, "P") == 0)
		{

			memset(szQuery, 0x00, sizeof(szQuery));

			ZzLOG(ALWAY, "bEvent: [%d]:%d \n\n", bEvent, nEvent_no);

			sprintf(szQuery, " update zangsi.T_CONTENTS_INFO "
							 " set price_amt = %d, sect_code = '%s', sect_sub = '%s', adult_yn = '%s', nmnt_cnt = %d , disp_cnt_inc = %d " // 20100630 mbc
							 " where id = %lu ; /* cprdaem2001 */ ",
					nPriceAmt, pSectCode, szSectSub, pAdultYn, nMbcPriceAmt, nEvent_no, ulId); // 20100630 // nEvent_no

			ZzLOG(ALWAY, "UpdateCont: [%s]\n\n", szQuery);

			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				tran_rollback(con);
				tran_end(con);
				return S_FAIL;
			}

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update zangsi.T_CONTENTS_VIR_ID "
							 " set sect_code = '%s', sect_sub = '%s', adult_yn = '%s' "
							 " where id = %lu ; /* cprdaem2001 */ ",
					pSectCode, szSectSub, pAdultYn, ulId);

			ZzLOG(ALWAY, "UpdateCont: [%s]\n\n", szQuery);

			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				tran_rollback(con);
				tran_end(con);
				return S_FAIL;
			}

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update zangsi.T_CONTENTS_VIR_ID2 "
							 " set sect_code = '%s', sect_sub = '%s', adult_yn = '%s' "
							 " where id = %lu ; /* cprdaem2001 */ ",
					pSectCode, szSectSub, pAdultYn, ulId);

			ZzLOG(ALWAY, "UpdateCont: [%s]\n\n", szQuery);

			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				tran_rollback(con);
				tran_end(con);
				return S_FAIL;
			}
		}
		else // pServiceGu = M
		{
			//-- 2015.09.22 service_gu = M 일 경우 업데이트
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " update zangsi.T_MOB_CONTENTS_INFO "
							 " set price_amt = %d "
							 " where id = %lu ; /* cprdaem2001 */ ",
					nMobPriceAmt, ulId);
			ZzLOG(ALWAY, "UpdateCont: [%s]\n\n", szQuery);

			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
				tran_rollback(con);
				tran_end(con);
				return S_FAIL;
			}
		}
	}

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " INSERT INTO zangsi.T_CONTENTS_CREATE (cont_gu, id, udt_cd) values ('01', %lu, 'U') ; /* cprdaem2001 */ ", ulId);

	ZzLOG(ALWAY, "UpdateCont: [%s]\n\n", szQuery);

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		tran_rollback(con);
		tran_end(con);
		return S_FAIL;
	}

	if (strcmp(pServiceGu, "A") == 0 || strcmp(pServiceGu, "P") == 0)
	{
		if (nRealPriceAmt != nPriceAmt)
		{
			char szDesc[4096];
			memset(szDesc, 0x00, sizeof(szDesc));

			ReplaceSingleToDouble(szTitle);

			sprintf(szDesc, " 안녕하세요. 위디스크입니다.\r\n\r\n"
							" 회원님께서 등록하여주신 자료\r\n"
							"[ %lu번 : %s ]는 \r\n"
							"제휴사의 요청으로 기존 %d캐시에서 %d캐시로 가격이 변경되었습니다.\r\n\r\n"
							"%s님께서는 변경된 가격을 확인해 보시기 바랍니다."
							"\r\n\r\n 감사합니다. \r\n"
					//, ulId, szTitle, nRealPriceAmt, nPriceAmt, szUserId);
					,
					ulId, szTitle, nRealPriceAmt, nPriceAmt, szUserId); // 20100630 MBC

			memset(szQuery, 0x00, sizeof(szQuery));

			sprintf(szQuery, " insert into zangsi.T_SEND_MEMO (  memo_cd, descript, send_user, del_yn, send_date, send_time ) "
							 " values (  '05' "

							 " ,'%s' "

							 " , '운영팀' ,'N', '%s', '%s' ) ; /* cprdaem2001 */ ",
					szDesc, greg_date, greg_time);

			ZzLOG(ALWAY, "[%s]\n\n", szQuery);
			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
			}

			memset(szQuery, 0x00, sizeof(szQuery));
			strcpy(szQuery, "SELECT last_insert_id() as send_seq_no ; /* cprdaem2001 */ ");

			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
			}

			MYSQL_RES *myres = mysql_store_result(con);
			MYSQL_ROW myrow = mysql_fetch_row(myres);

			double send_seq_no = getnum(myrow, 0);

			mysql_free_result(myres);

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi.T_RECV_MEMO "
							 "  (memo_seq_no, recv_user, recv_date, recv_time, read_yn, del_yn, save_yn)  "
							 " values "
							 "  (%.0f, '%s' , '%s', '%s' , 'N', 'N', 'N') ; /* cprdaem2001 */ ",
					send_seq_no, szUserId, greg_date, greg_time);

			ZzLOG(ALWAY, "[%s]\n\n", szQuery);
			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "UpdateCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
			}
		}
	}

	if (tran_commit(con) != 0)
	{
		ZzLOG(ERROR, "UpdateCont: 트렌젝션 commit 오류.\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		tran_rollback(con);
		tran_end(con);
		return S_DBERR;
	}
	tran_end(con);

	return 0;
}

int UpdateFCont(unsigned long ulId, char *pAdultYn, int nPriceAmt, int nMbcPriceAmt, char *pFileType, int nEvent_no, bool bEvent)
{
	/*
	1.T_CONTFLOG_INFO price_amt, adult_yn, sect_code 업데이트
	2.T_CONTFLOG_VIR_ID adult_yn, sect_code 업데이트
	4.T_MEMO_INFO 인서트
	*/

	MYSQL_RES *res;
	MYSQL_ROW row;

	char szQuery[16384];
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select a.reg_user, a.price_amt, a.adult_yn, a.title "
					 " from zangsi.T_CONTFLOG_INFO a, zangsi.T_CONTFLOG_FILE b, zangsi.T_CONTFLOG_VIR_ID c "
					 " where a.id = b.id and a.id = c.id and a.id = %lu and a.del_yn = 'N' and c.copyright_yn in ('C') ; /* cprdaem2001 */ ",
			ulId);

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "UpdateFCont: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return S_FAIL;
	}

	if (!(res = mysql_store_result(con)))
	{
		ZzLOG(ERROR, "UpdateFCont: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return S_FAIL;
	}
	if (mysql_num_rows(res) == 0)
	{
		ZzLOG(ALWAY, "UpdateFCont: 처리할 자료없음. ulId=[%lu], \n", ulId);
		ZzLOG(ALWAY, "(%s)\n", szQuery);
		mysql_free_result(res);
		return S_NODATA;
	}

	row = mysql_fetch_row(res);

	char szUserId[16 + 1];
	memset(szUserId, 0x00, sizeof(szUserId));
	sprintf(szUserId, "%s", getstr(row, 0));

	int nRealPriceAmt = 0;
	nRealPriceAmt = (int)getint(row, 1);

	char szRealAdultYn[1 + 1];
	memset(szRealAdultYn, 0x00, sizeof(szRealAdultYn));
	sprintf(szRealAdultYn, "%s", getstr(row, 2));

	char szTitle[2048];
	memset(szTitle, 0x00, sizeof(szTitle));
	sprintf(szTitle, "%s", getstr(row, 3));

	mysql_free_result(res);

	if (nRealPriceAmt == nPriceAmt && szRealAdultYn == pAdultYn)
		return S_OK;

	if (tran_begin(con) != 0)
	{
		ZzLOG(ERROR, "UpdateFCont: 트렌젝션 오류.\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		return S_FAIL;
	}

	ZzLOG(ALWAY, "bEvent: [%d]:%d \n\n", bEvent, nEvent_no);

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " update zangsi.T_CONTFLOG_INFO "
					 " set price_amt = %d, adult_yn = '%s', won_mega = %d  , disp_cnt_inc = %d" // 20100630
					 " where id = %lu ; /* cprdaem2001 */ ",
			nPriceAmt, pAdultYn, nMbcPriceAmt, nEvent_no, ulId); // 20100630

	ZzLOG(ALWAY, "UpdateFCont: [%s]\n\n", szQuery);

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "UpdateFCont: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		tran_rollback(con);
		tran_end(con);
		return S_FAIL;
	}

	if (pFileType != NULL)
	{

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update zangsi.T_CONTFLOG_FILE "
						 " set file_type  = '%s' "
						 " where id = %lu ; /* cprdaem2001 */ ",
				pFileType, ulId); // 20121203

		ZzLOG(ALWAY, "UpdateFCont: [%s]\n\n", szQuery);

		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "UpdateFCont: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(con);
			tran_end(con);
			return S_FAIL;
		}
	}
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " update zangsi.T_CONTFLOG_VIR_ID "
					 " set adult_yn = '%s' "
					 " where id = %lu ; /* cprdaem2001 */ ",
			pAdultYn, ulId);

	ZzLOG(ALWAY, "UpdateFCont: [%s]\n\n", szQuery);

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "UpdateFCont: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		tran_rollback(con);
		tran_end(con);
		return S_FAIL;
	}

	if (nRealPriceAmt != nPriceAmt)
	{
		char szDesc[4096];
		memset(szDesc, 0x00, sizeof(szDesc));

		ReplaceSingleToDouble(szTitle);

		sprintf(szDesc, " 안녕하세요. 위디스크 입니다.\r\n\r\n"
						" 회원님께서 등록하여주신 필로그 자료\r\n"
						"[ %s ]는 \r\n"
						"제휴사의 요청으로 기존 %d캐시에서 %d캐시로 가격이 변경되었습니다.\r\n\r\n"
						"%s님께서는 변경된 가격을 확인하여 보시기 바랍니다."
						"\r\n\r\n 감사합니다. \r\n"
				//, szTitle, nRealPriceAmt, nPriceAmt, szUserId);
				,
				szTitle, nRealPriceAmt, nMbcPriceAmt, szUserId); // 20100630

		memset(szQuery, 0x00, sizeof(szQuery));
		/*
		sprintf(szQuery, " insert into zangsi.T_MEMO_INFO "
						 " (user_id, memo_cd, ref_id, descript, send_user, recv_yn, recv_date, recv_time) "
						 " values "
						 " ('%s', '01', 0 , '%s', '운영팀', 'N', '%s', '%s' ) "
						 , szUserId, szDesc, greg_date, greg_time);

		ZzLOG(ALWAY, "UpdateFCont: [%s]\n\n", szQuery);

		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "UpdateFCont: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
		}
		*/

		sprintf(szQuery, " insert into zangsi.T_SEND_MEMO (  memo_cd,  descript, send_user,del_yn, send_date, send_time ) "
						 " values (  '05' "

						 " ,'%s' "

						 " , '운영팀' ,'N', '%s', '%s' ) ; /* cprdaem2001 */ ",
				szDesc, greg_date, greg_time);

		ZzLOG(ALWAY, "[%s]\n\n", szQuery);
		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "UpdateFCont: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
		}
		memset(szQuery, 0x00, sizeof(szQuery));
		strcpy(szQuery, "SELECT last_insert_id() as send_seq_no");

		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "UpdateFCont: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
		}

		MYSQL_RES *myres = mysql_store_result(con);
		MYSQL_ROW myrow = mysql_fetch_row(myres);

		double send_seq_no = getnum(myrow, 0);

		mysql_free_result(myres);

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi.T_RECV_MEMO "
						 "  (memo_seq_no, recv_user, recv_date, recv_time, read_yn, del_yn, save_yn)  "
						 " values "
						 "  (%.0f, '%s' , '%s', '%s' , 'N', 'N', 'N') ; /* cprdaem2001 */ ",
				send_seq_no, szUserId, greg_date, greg_time);

		ZzLOG(ALWAY, "[%s]\n\n", szQuery);
		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "UpdateFCont: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
		}
	}

	if (tran_commit(con) != 0)
	{
		ZzLOG(ERROR, "UpdateFCont: 트렌젝션 commit 오류.\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		tran_rollback(con);
		tran_end(con);
		return S_DBERR;
	}
	tran_end(con);

	return 0;
}

int CancleCpr(unsigned long ulListId, unsigned long ulSeqNo, char *pServiceGu, int nDelete)
{

	MYSQL_RES *res;
	MYSQL_ROW row;

	char szQuery[2048];
	memset(szQuery, 0x00, sizeof(szQuery));

	//---------------------------------------------------------
	// 2015.08.12  service_gu 분류
	// service_gu    A = All, P = Pc, M = Mobile
	// SERVICE_ALL = 1, SERVICE_PC = 2, SERVICE_MOBILE = 3
	//---------------------------------------------------------

	if (strcmp(pServiceGu, "A") == 0 || strcmp(pServiceGu, "P") == 0)
	{
		sprintf(szQuery, "select list_id from zangsi_cpr.T_CPR_CONT_LIST where list_id = %lu ; /* cprdaem2001 */ ", ulListId); //
	}
	else
	{
		sprintf(szQuery, "select list_id from zangsi_cpr.T_CPR_MOB_CONT_LIST where list_id = %lu ; /* cprdaem2001 */ ", ulListId); //
	}

	if (mysql_query(cpr_con, szQuery))
	{
		ZzLOG(ERROR, "CancleCpr: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return S_FAIL;
	}

	if (!(res = mysql_store_result(cpr_con)))
	{
		ZzLOG(ERROR, "CancleCpr: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return S_FAIL;
	}
	if (mysql_num_rows(res) == 0)
	{
		ZzLOG(ERROR, "CancleCpr: 처리할 자료없음. ulListId=[%lu], \n", ulListId);
		ZzLOG(ERROR, "(%s)\n", szQuery);
		mysql_free_result(res);
		return S_FAIL;
	}
	mysql_free_result(res);

	if (tran_begin(cpr_con) != 0)
	{
		ZzLOG(ERROR, "CancleCpr: 트렌젝션 오류.\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		return S_DBERR;
	}

	if (strcmp(pServiceGu, "A") == 0 || strcmp(pServiceGu, "P") == 0)
	{
		sprintf(szQuery, " insert into zangsi_cpr.T_CPR_RESRV_CONT_LIST_SUB_HIST  "
						 " (list_id, resrv_seq_no, title, adult_yn, mgr_cd, comp_cd "
						 " , sect_code, sect_sub, price_amt, cpr_payment_rate, proc_date, proc_time, proc_cd "
						 " , reg_user, reg_date, reg_time, udt_user, udt_date, udt_time, proc_stat, service_gu) "
						 " select "
						 " b.list_id, b.seq_no, a.title, a.adult_yn, a.mgr_cd, a.comp_cd"
						 " , a.sect_code, a.sect_sub, a.price_amt, a.cpr_payment_rate, b.proc_date, b.proc_time, b.proc_cd "
						 " , a.reg_user, a.reg_date, a.reg_time, 'sys2001', '%s', '%s', 'H', b.service_gu "
						 " from zangsi_cpr.T_CPR_CONT_LIST a, zangsi_cpr.T_CPR_RESRV_CONT_LIST_SUB b "
						 " where a.list_id = b.list_id and b.list_id = %lu and b.seq_no = %lu ; /* cprdaem2001 */ ",
				greg_date, greg_time, ulListId, ulSeqNo);
	}
	else
	{
		sprintf(szQuery, " insert into zangsi_cpr.T_CPR_RESRV_CONT_LIST_SUB_HIST  "
						 " (list_id, resrv_seq_no, title, adult_yn, mgr_cd, comp_cd "
						 " , sect_code, sect_sub, price_amt, cpr_payment_rate, proc_date, proc_time, proc_cd "
						 " , reg_user, reg_date, reg_time, udt_user, udt_date, udt_time, proc_stat, service_gu) "
						 " select "
						 " b.list_id, b.seq_no, a.title, a.adult_yn, a.mgr_cd, a.comp_cd"
						 " , a.sect_code, a.sect_sub, a.price_amt, a.cpr_payment_rate, b.proc_date, b.proc_time, b.proc_cd "
						 " , a.reg_user, a.reg_date, a.reg_time, 'sys2001', '%s', '%s', 'H', b.service_gu "
						 " from zangsi_cpr.T_CPR_MOB_CONT_LIST a, zangsi_cpr.T_CPR_RESRV_CONT_LIST_SUB b "
						 " where a.list_id = b.list_id and b.list_id = %lu and b.seq_no = %lu ; /* cprdaem2001 */ ",
				greg_date, greg_time, ulListId, ulSeqNo);
	}

	if (mysql_query(cpr_con, szQuery))
	{
		ZzLOG(ERROR, "CancleCpr: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		tran_rollback(cpr_con);
		tran_end(cpr_con);
		return S_FAIL;
	}

	///////////////////////////////
	if (strcmp(pServiceGu, "A") == 0 || strcmp(pServiceGu, "P") == 0) //
	{
		// T_CPR_HASH_INFO , FILE 정보 업데이트
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update zangsi_cpr.T_CPR_HASH_INFO set proc_stat = 'N' where list_id = %lu and proc_stat = 'C' ; /* cprdaem2001 */ ", ulListId);

		ZzLOG(ALWAY, "CancleCpr: [%s]\n\n", szQuery);

		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "CancleCpr: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return S_FAIL;
		}
	}
	// T_CPR_CONT_LIST_HIST 에 추가
	if (strcmp(pServiceGu, "A") == 0) //
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update  zangsi_cpr.T_CPR_CONT_LIST set apply_yn = 'N' , seq_no = seq_no +1  "
						 " where list_id = %lu ; /* cprdaem2001 */ ",
				ulListId);
		// ZzLOG(ALWAY, "UpdateCpr: [%s]\n\n", szQuery);

		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return S_FAIL;
		}

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update  zangsi_cpr.T_CPR_MOB_CONT_LIST set apply_yn = 'N' , seq_no = seq_no +1  "
						 " where list_id = %lu ; /* cprdaem2001 */ ",
				ulListId);
		// ZzLOG(ALWAY, "UpdateCpr: [%s]\n\n", szQuery);

		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return S_FAIL;
		}
	}
	else if (strcmp(pServiceGu, "P") == 0)
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update  zangsi_cpr.T_CPR_CONT_LIST set apply_yn = 'N' , seq_no = seq_no +1  "
						 " where list_id = %lu ; /* cprdaem2001 */ ",
				ulListId);
		// ZzLOG(ALWAY, "UpdateCpr: [%s]\n\n", szQuery);

		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return S_FAIL;
		}
	}
	else
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update  zangsi_cpr.T_CPR_MOB_CONT_LIST set apply_yn = 'N' , seq_no = seq_no +1  "
						 " where list_id = %lu ; /* cprdaem2001 */ ",
				ulListId);
		// ZzLOG(ALWAY, "UpdateCpr: [%s]\n\n", szQuery);

		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return S_FAIL;
		}
	}

	if (strcmp(pServiceGu, "A") == 0 || strcmp(pServiceGu, "P") == 0) //
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi_cpr.T_CPR_CONT_LIST_HIST "
						 " (list_id ,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate "
						 " , udt_user , udt_date,udt_time ) "
						 " select list_id ,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate   "
						 " , 'sys2001' , date_format(now(), '%%Y%%m%%d'),  date_format(now(), '%%H%%i%%s')  "
						 " from zangsi_cpr.T_CPR_CONT_LIST where list_id = %lu ; /* cprdaem2001 */ ",
				ulListId);
	}

	if (mysql_query(cpr_con, szQuery))
	{
		ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		tran_rollback(cpr_con);
		tran_end(cpr_con);
		return S_FAIL;
	}
	// =============

	if (strcmp(pServiceGu, "A") == 0 || strcmp(pServiceGu, "M") == 0) //
	{
		// T_CPR_MOB_CONT_LIST_HIST 에 추가
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update  zangsi_cpr.T_CPR_MOB_CONT_LIST set apply_yn = 'N' , seq_no = seq_no +1  "
						 " where list_id = %lu ; /* cprdaem2001 */ ",
				ulListId);

		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return S_FAIL;
		}

		memset(szQuery, 0x00, sizeof(szQuery));
		// T_CPR_MOB_CONT_LIST_HIST 에 추가 		mobile_chk 추가
		sprintf(szQuery, " insert into zangsi_cpr.T_CPR_MOB_CONT_LIST_HIST "
						 " (list_id ,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate "
						 " , udt_user , mobile_chk,udt_date,udt_time ) "
						 " select list_id ,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate   "
						 " , 'sys2001' , mobile_chk,date_format(now(), '%%Y%%m%%d'),  date_format(now(), '%%H%%i%%s') "
						 " from zangsi_cpr.T_CPR_MOB_CONT_LIST where list_id = %lu ; /* cprdaem2001 */ ",
				ulListId);

		if (mysql_query(cpr_con, szQuery))
		{
			ZzLOG(ERROR, "UpdateCpr: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(cpr_con);
			tran_end(cpr_con);
			return S_FAIL;
		}
	}

	if (tran_commit(cpr_con) != 0)
	{
		ZzLOG(ERROR, "CancleCpr: 트렌젝션 commit 오류.\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		tran_rollback(cpr_con);
		tran_end(cpr_con);
		return S_DBERR;
	}

	tran_end(cpr_con);

	if (!(con = db_connect_to_main("zangsi")))
	{
		ZzLOG(ERROR, "CancleCpr: zangsi DB에 접속하지 못 하였습니다...\n");
		return S_DBERR;
	}

	memset(szQuery, 0x00, sizeof(szQuery));
	if (strcmp(pServiceGu, "A") == 0 || strcmp(pServiceGu, "P") == 0) //
	{
		sprintf(szQuery, " select c.cont_gu, c.id "
						 " from zangsi_cpr.T_CPR_CONT_LIST a, zangsi_cpr.T_CPR_HASH_INFO b, zangsi_cpr.T_CPR_CONT_MAP_SUB c "
						 " where a.list_id = %lu and a.list_id = b.list_id and b.chi_id = c.chi_id and b.proc_stat = 'N' "
						 " group by c.cont_gu, c.id ; /* cprdaem2001 */ ",
				ulListId);
	}
	else
	{
		sprintf(szQuery, " select c.cont_gu, c.id "
						 " from zangsi_cpr.T_CPR_MOB_CONT_LIST a, zangsi_cpr.T_CPR_HASH_INFO b, zangsi_cpr.T_CPR_CONT_MAP_SUB c "
						 " where a.list_id = %lu and a.list_id = b.list_id and b.chi_id = c.chi_id and b.proc_stat = 'C' "
						 " group by c.cont_gu, c.id ; /* cprdaem2001 */ ",
				ulListId);
	}

	if (mysql_query(cpr_con, szQuery))
	{
		ZzLOG(ERROR, "CancleCpr: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		db_disconnect(con);
		return S_NODATA;
	}

	if (!(res = mysql_store_result(cpr_con)))
	{
		ZzLOG(ERROR, "CancleCpr: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		db_disconnect(con);
		return S_NODATA;
	}
	if (mysql_num_rows(res) == 0)
	{
		ZzLOG(ALWAY, "CancleCpr: 처리할 컨텐츠 자료없음. ulListId=[%lu], \n", ulListId);
		ZzLOG(ALWAY, "(%s)\n", szQuery);
		mysql_free_result(res);
		db_disconnect(con);
		return S_NODATA;
	}

	while (row = mysql_fetch_row(res))
	{
		char szContGu[2 + 1];
		memset(szContGu, 0x00, sizeof(szContGu));
		sprintf(szContGu, "%s", getstr(row, 0));

		unsigned long ulId = 0;
		ulId = (unsigned long)getnum(row, 1);

		int nRes = 0;

		if (nDelete == 1)
		{ // 심사중 전환..
			if (strcmp(szContGu, "WE") == 0)
				nRes = DeleteCont(ulId, pServiceGu);
			else if (strcmp(szContGu, "FD") == 0)
				nRes = DeleteFCont(ulId);
		}
		else
		{ // 비제휴 전환
			if (strcmp(szContGu, "WE") == 0)
				nRes = UpdateCont_N(ulId, pServiceGu);
			else if (strcmp(szContGu, "FD") == 0)
				nRes = UpdateFCont_N(ulId);
		}

		if (nRes == S_OK)
			usleep(100); //컨텐츠 1개 처리후 0.0005초 휴면
		else if (nRes != S_OK)
			ContLogWrite(szContGu, ulId, "D");

		if (nRes == S_DBERR)
		{
			mysql_free_result(res);
			db_disconnect(con);
			return nRes;
		}
	}
	mysql_free_result(res);

	db_disconnect(con);
	return 0;
}

int DeleteCont(unsigned long ulId, char *pDelServiceGu)
{
	/*
	1.T_CONTENTS_INFO disp_end_date 업데이트
	2.T_CONTENTS_VIR_ID copyright_yn 업데이트
	3.T_CONTENTS_CREATE 인서트
	4.T_MEMO_INFO 인서트
	*/

	MYSQL_RES *res;
	MYSQL_ROW row;

	char szQuery[16384];
	memset(szQuery, 0x00, sizeof(szQuery));
	char szUserId[16 + 1];
	memset(szUserId, 0x00, sizeof(szUserId));
	char szTitle[2048];
	memset(szTitle, 0x00, sizeof(szTitle));

	if (strcmp(pDelServiceGu, "A") == 0 || strcmp(pDelServiceGu, "P") == 0) //
	{
		sprintf(szQuery, " select a.reg_user, a.title "
						 " from zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_FILE b, zangsi.T_CONTENTS_VIR_ID c "
						 " where a.id = b.id and a.id = c.id and a.id = %lu and a.del_yn = 'N' and c.copyright_yn in ('C','H') ; /* cprdaem2001 */ ",
				ulId);

		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "DeleteCont: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			return S_FAIL;
		}

		if (!(res = mysql_store_result(con)))
		{
			ZzLOG(ERROR, "DeleteCont: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			return S_FAIL;
		}
		if (mysql_num_rows(res) == 0)
		{
			ZzLOG(ALWAY, "DeleteCont: 처리할 자료없음. ulId=[%lu], \n", ulId);
			ZzLOG(ALWAY, "(%s)\n", szQuery);
			mysql_free_result(res);
			return S_NODATA;
		}

		row = mysql_fetch_row(res);
		sprintf(szUserId, "%s", getstr(row, 0));
		sprintf(szTitle, "%s", getstr(row, 1));
		mysql_free_result(res);

		if (tran_begin(con) != 0)
		{
			ZzLOG(ERROR, "DeleteCont: 트렌젝션 오류.\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
			return S_FAIL;
		}

		// DeleteCont
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update zangsi.T_CONTENTS_INFO "
						 " set disp_end_date = date_format(date_add(now(), INTERVAL 7 DAY),'%%Y%%m%%d')"
						 " where id = %lu ; /* cprdaem2001 */ ",
				ulId);

		ZzLOG(ALWAY, "DeleteCont: [%s]\n\n", szQuery);

		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "DeleteCont: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(con);
			tran_end(con);
			return S_FAIL;
		}

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update zangsi.T_CONTENTS_VIR_ID "
						 " set copyright_yn = 'X' "
						 " where id = %lu ; /* cprdaem2001 */ ",
				ulId);

		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "DeleteCont: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(con);
			tran_end(con);
			return S_FAIL;
		}

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update zangsi.T_CONTENTS_VIR_ID2 "
						 " set copyright_yn = 'X' "
						 " where id = %lu ; /* cprdaem2001 */ ",
				ulId);

		ZzLOG(ALWAY, "DeleteCont: [%s]\n\n", szQuery);

		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "DeleteCont: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(con);
			tran_end(con);
			return S_FAIL;
		}

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " INSERT INTO zangsi.T_CONTENTS_CREATE (cont_gu, id, udt_cd) values ('01', %lu, 'D') ; /* cprdaem2001 */ ", ulId);

		ZzLOG(ALWAY, "DeleteCont: [%s]\n\n", szQuery);

		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "DeleteCont: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(con);
			tran_end(con);
			return S_FAIL;
		}
	}
	else // pDelServiceGu = M
	{
		sprintf(szQuery, " select a.reg_user, a.title "
						 " from zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_FILE b, zangsi.T_CONTENTS_VIR_ID c "
						 " where a.id = b.id and a.id = c.id and a.id = %lu and a.del_yn = 'N' and c.copyright_yn in ('C','H') and c.mob_service_yn = 'Y' ; /* cprdaem2001 */ ",
				ulId);

		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "DeleteCont: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			return S_FAIL;
		}

		if (!(res = mysql_store_result(con)))
		{
			ZzLOG(ERROR, "DeleteCont: mysql_store_result error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			return S_FAIL;
		}
		if (mysql_num_rows(res) == 0)
		{
			ZzLOG(ALWAY, "DeleteCont: 처리할 자료없음. ulId=[%lu], \n", ulId);
			ZzLOG(ALWAY, "(%s)\n", szQuery);
			mysql_free_result(res);
			return S_NODATA;
		}

		row = mysql_fetch_row(res);
		sprintf(szUserId, "%s", getstr(row, 0));
		sprintf(szTitle, "%s", getstr(row, 1));
		mysql_free_result(res);

		if (tran_begin(con) != 0)
		{
			ZzLOG(ERROR, "DeleteCont: 트렌젝션 오류.\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
			return S_FAIL;
		}

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update zangsi.T_CONTENTS_VIR_ID "
						 " set mob_service_yn = 'N' "
						 " where id = %lu ; /* cprdaem2001 */ ",
				ulId);

		ZzLOG(ALWAY, "DeleteCont: [%s]\n\n", szQuery);

		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "DeleteCont: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(con);
			tran_end(con);
			return S_FAIL;
		}

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " update zangsi.T_CONTENTS_VIR_ID2 "
						 " set mob_service_yn = 'N' "
						 " where id = %lu ; /* cprdaem2001 */ ",
				ulId);

		ZzLOG(ALWAY, "DeleteCont: [%s]\n\n", szQuery);

		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "DeleteCont: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(con);
			tran_end(con);
			return S_FAIL;
		}

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " INSERT INTO zangsi.T_CONTENTS_CREATE (cont_gu, id, udt_cd) values ('01', %lu, 'I') ; /* cprdaem2001 */ ", ulId);

		ZzLOG(ALWAY, "DeleteCont: [%s]\n\n", szQuery);

		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "DeleteCont: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
			tran_rollback(con);
			tran_end(con);
			return S_FAIL;
		}
	}

	char szDesc[4096];
	memset(szDesc, 0x00, sizeof(szDesc));

	ReplaceSingleToDouble(szTitle);
	// 2015.09.04 제휴 종료시 service_gu = A,P일 경우 보내는 메세지
	if (strcmp(pDelServiceGu, "A") == 0 || strcmp(pDelServiceGu, "P") == 0) //
	{
		sprintf(szDesc, " 안녕하세요. 위디스크 운영팀 입니다.\r\n\r\n"
						" 회원님께서 등록하여주신 자료\r\n"
						"[ %lu번 : %s ]는 \r\n"
						"제휴사의 요청으로 제휴서비스가 종료되었습니다.\r\n"
						"해당자료는 %s님께서만 확인하실 수 있도록 심사 중 조치를 하였으니 \"자료찾기 > 내가올린자료\"에서 확인하여 보시기 바랍니다."
						"\r\n\r\n 감사합니다. \r\n",
				ulId, szTitle, szUserId);

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi.T_SEND_MEMO (  memo_cd,  descript,send_user, del_yn, send_date, send_time ) "
						 " values (  '05' "
						 " ,'%s' "
						 " , '운영팀' ,'N', '%s', '%s' ) ; /* cprdaem2001 */ ",
				szDesc, greg_date, greg_time);

		ZzLOG(ALWAY, "[%s]\n\n", szQuery);
		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "DeleteCont: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
		}

		memset(szQuery, 0x00, sizeof(szQuery));
		strcpy(szQuery, "SELECT last_insert_id() as send_seq_no");

		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "DeleteCont: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
		}

		MYSQL_RES *myres = mysql_store_result(con);
		MYSQL_ROW myrow = mysql_fetch_row(myres);

		double send_seq_no = getnum(myrow, 0);

		mysql_free_result(myres);

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi.T_RECV_MEMO "
						 "  (memo_seq_no, recv_user, recv_date, recv_time, read_yn, del_yn, save_yn)  "
						 " values "
						 "  (%.0f, '%s' , '%s', '%s' , 'N', 'N', 'N') ; /* cprdaem2001 */ ",
				send_seq_no, szUserId, greg_date, greg_time);

		ZzLOG(ALWAY, "[%s]\n\n", szQuery);
		if (mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "DeleteCont: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "(%s)\n", szQuery);
		}
	}
	// 2015.09.04 제휴 종료시 service_gu = M일 경우
	/*	else
		{
			메세지 수정예정 . 운영팀과 협의
			sprintf(szDesc, " 안녕하세요. 위디스크 운영팀 입니다.\r\n\r\n"
							" 회원님께서 등록하여주신 자료\r\n"
							"[ %lu번 : %s ]는 \r\n"
							"제휴사의 요청으로 제휴서비스가 종료되었습니다.\r\n"
							"해당자료는 %s님께서만 확인하실 수 있도록 심사 중 조치를 하였으니 \"자료찾기 > 내가올린자료\"에서 확인하여 보시기 바랍니다."
							"\r\n\r\n 감사합니다. \r\n"
							, ulId, szTitle, szUserId);

			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery," insert into zangsi.T_SEND_MEMO (  memo_cd,  descript,send_user, del_yn, send_date, send_time ) "
							" values (  '05' "
							" ,'%s' "
							" , '운영팀' ,'N', '%s', '%s' ) "
							, szDesc, greg_date, greg_time);

			ZzLOG(ALWAY, "[%s]\n\n", szQuery);
			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "DeleteCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
			}

			memset(szQuery , 0x00, sizeof(szQuery ));
			strcpy( szQuery, "SELECT last_insert_id() as send_seq_no" );

			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "DeleteCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
			}

			MYSQL_RES* myres = mysql_store_result(con);
			MYSQL_ROW myrow = mysql_fetch_row(myres);

			double send_seq_no  = getnum(myrow,0 );

			mysql_free_result(myres);


			memset(szQuery , 0x00, sizeof(szQuery ));
			sprintf(szQuery," insert into zangsi.T_RECV_MEMO "
							"  (memo_seq_no, recv_user, recv_date, recv_time, read_yn, del_yn, save_yn)  "
							" values "
							"  (%.0f, '%s' , '%s', '%s' , 'N', 'N', 'N') "
							,send_seq_no,szUserId,greg_date, greg_time);

			ZzLOG(ALWAY, "[%s]\n\n", szQuery);
			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "DeleteCont: mysql_query error...\n");
				ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
				ZzLOG(ERROR, "(%s)\n", szQuery);
			}


		}
	*/

	if (tran_commit(con) != 0)
	{
		ZzLOG(ERROR, "DeleteCont: 트렌젝션 commit 오류.\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		tran_rollback(con);
		tran_end(con);
		return S_DBERR;
	}
	tran_end(con);

	return 0;
}

int DeleteFCont(unsigned long ulId)
{
	/*
	1.T_CONTFLOG_INFO price_amt, adult_yn, sect_code 업데이트
	2.T_CONTFLOG_VIR_ID adult_yn, sect_code 업데이트
	4.T_MEMO_INFO 인서트
	*/

	MYSQL_RES *res;
	MYSQL_ROW row;

	char szQuery[16384];
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select a.reg_user, a.title "
					 " from zangsi.T_CONTFLOG_INFO a, zangsi.T_CONTFLOG_FILE b, zangsi.T_CONTFLOG_VIR_ID c "
					 " where a.id = b.id and a.id = c.id and a.id = %lu and a.del_yn = 'N' and c.copyright_yn in ('C','H') ; /* cprdaem2001 */ ",
			ulId);

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "DeleteFCont: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return S_FAIL;
	}

	if (!(res = mysql_store_result(con)))
	{
		ZzLOG(ERROR, "DeleteFCont: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return S_FAIL;
	}
	if (mysql_num_rows(res) == 0)
	{
		ZzLOG(ALWAY, "DeleteFCont: 처리할 자료없음. ulId=[%lu], \n", ulId);
		ZzLOG(ALWAY, "(%s)\n", szQuery);
		mysql_free_result(res);
		return S_NODATA;
	}

	row = mysql_fetch_row(res);

	char szUserId[16 + 1];
	memset(szUserId, 0x00, sizeof(szUserId));
	sprintf(szUserId, "%s", getstr(row, 0));

	char szTitle[2048];
	memset(szTitle, 0x00, sizeof(szTitle));
	sprintf(szTitle, "%s", getstr(row, 1));

	mysql_free_result(res);

	if (tran_begin(con) != 0)
	{
		ZzLOG(ERROR, "DeleteFCont: 트렌젝션 오류.\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		return S_FAIL;
	}

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " update zangsi.T_CONTFLOG_INFO "
					 " set disp_end_date = date_format(date_add(now(), INTERVAL 7 DAY),'%%Y%%m%%d')"
					 " where id = %lu ; /* cprdaem2001 */ ",
			ulId);

	ZzLOG(ALWAY, "DeleteFCont: [%s]\n\n", szQuery);

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "DeleteFCont: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		tran_rollback(con);
		tran_end(con);
		return S_FAIL;
	}

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " update zangsi.T_CONTFLOG_VIR_ID "
					 " set copyright_yn = 'X' "
					 " where id = %lu ; /* cprdaem2001 */ ",
			ulId);

	ZzLOG(ALWAY, "DeleteFCont: [%s]\n\n", szQuery);

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "DeleteFCont: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		tran_rollback(con);
		tran_end(con);
		return S_FAIL;
	}

	char szDesc[4096];
	memset(szDesc, 0x00, sizeof(szDesc));

	ReplaceSingleToDouble(szTitle);

	sprintf(szDesc, " 안녕하세요. 위디스크 운영팀 입니다.\r\n\r\n"
					" 회원님께서 등록하여주신 필로그 자료\r\n"
					"[ %s ]는 \r\n"
					"제휴사의 요청으로 제휴서비스가 종료되었습니다.\r\n"
					"해당 필로그자료는 %s님께서만 확인하실 수 있도록 심사 중 조치를 하였으니 필로그 자료실에서 확인하여 보시기 바랍니다."
					"\r\n\r\n 감사합니다. \r\n",
			szTitle, szUserId);

	memset(szQuery, 0x00, sizeof(szQuery));
	/*
	sprintf(szQuery, " insert into zangsi.T_MEMO_INFO "
					 " (user_id, memo_cd, ref_id, descript, send_user, recv_yn, recv_date, recv_time) "
					 " values "
					 " ('%s', '01', 0 , '%s', '운영팀', 'N', '%s', '%s' ) "
					 , szUserId, szDesc, greg_date, greg_time);

	ZzLOG(ALWAY, "DeleteFCont: [%s]\n\n", szQuery);

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "DeleteFCont: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
	}
	*/

	sprintf(szQuery, " insert into zangsi.T_SEND_MEMO (  memo_cd, descript, send_user, del_yn, send_date, send_time ) "
					 " values (  '05' "

					 " ,'%s' "

					 " , '운영팀' ,'N', '%s', '%s' ) ; /* cprdaem2001 */ ",
			szDesc, greg_date, greg_time);

	ZzLOG(ALWAY, "[%s]\n\n", szQuery);
	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "DeleteFCont: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
	}

	memset(szQuery, 0x00, sizeof(szQuery));
	strcpy(szQuery, "SELECT last_insert_id() as send_seq_no");

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "DeleteFCont: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
	}

	MYSQL_RES *myres = mysql_store_result(con);
	MYSQL_ROW myrow = mysql_fetch_row(myres);

	double send_seq_no = getnum(myrow, 0);

	mysql_free_result(myres);

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " insert into zangsi.T_RECV_MEMO "
					 "  (memo_seq_no, recv_user, recv_date, recv_time, read_yn, del_yn, save_yn)  "
					 " values "
					 "  (%.0f, '%s' , '%s', '%s' , 'N', 'N', 'N') ; /* cprdaem2001 */ ",
			send_seq_no, szUserId, greg_date, greg_time);

	ZzLOG(ALWAY, "[%s]\n\n", szQuery);
	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "DeleteFCont: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
	}

	if (tran_commit(con) != 0)
	{
		ZzLOG(ERROR, "DeleteFCont: 트렌젝션 commit 오류.\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		tran_rollback(con);
		tran_end(con);
		return S_DBERR;
	}
	tran_end(con);

	return 0;
}

int ContLogWrite(char *pContGu, unsigned long ulId, char *pProcCd)
{
	MYSQL_RES *res;
	MYSQL_ROW row;

	char szQuery[2048];
	memset(szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery, "insert into zangsi_cpr.T_CPR_RESRV_CONT_FILE_LOG values ('%s', %lu, '%s', '%s', '%s') ; /* cprdaem2001 */ ", pContGu, ulId, pProcCd, greg_date, greg_time);

	if (mysql_query(cpr_con, szQuery))
	{
		ZzLOG(ERROR, "ContLogWrite: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return S_FAIL;
	}

	return S_OK;
}

int LogWrite(unsigned long ulListId, unsigned long ulSeqNo, char *pProcCd)
{
	MYSQL_RES *res;
	MYSQL_ROW row;

	char szQuery[2048];
	memset(szQuery, 0x00, sizeof(szQuery));

	sprintf(szQuery, " insert into zangsi_cpr.T_CPR_RESRV_CONT_LIST_SUB_HIST "
					 " (list_id, resrv_seq_no, title, adult_yn, grade, chapter, open_date, mgr_cd, comp_cd, mgr_ext, copyrighter "
					 " , sect_code, sect_sub, price_amt, cpr_payment_rate, cpr_date_ext, proc_date, proc_time, proc_cd "
					 ", reg_user, reg_date, reg_time, udt_user, udt_date, udt_time, proc_stat) "
					 " select "
					 " list_id, seq_no, title, adult_yn, grade, chapter, open_date, mgr_cd, comp_cd, mgr_ext, copyrighter "
					 " , sect_code, sect_sub, price_amt, cpr_payment_rate, cpr_date_ext, proc_date, proc_time, proc_cd "
					 ", reg_user, reg_date, reg_time, 'sys2001', '%s', '%s', '%s' "
					 " from zangsi_cpr.T_CPR_RESRV_CONT_LIST_SUB "
					 " where list_id = %lu and seq_no = %lu ; /* cprdaem2001 */ ",
			greg_date, greg_time, pProcCd, ulListId, ulSeqNo);

	if (mysql_query(cpr_con, szQuery))
	{
		ZzLOG(ERROR, "LogWrite: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return S_FAIL;
	}

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " delete from zangsi_cpr.T_CPR_RESRV_CONT_LIST_SUB where list_id = %lu and seq_no = %lu ; /* cprdaem2001 */ ", ulListId, ulSeqNo);

	if (mysql_query(cpr_con, szQuery))
	{
		ZzLOG(ERROR, "LogWrite: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return S_FAIL;
	}

	return S_OK;
}

/*****************************************************************************
 * 프로그램 시작루틴
 * 전역변수 초기화 및 데이타베이스 연결
 * (I) void
 * (R) int : 정상(0)/오류(-1)
 *****************************************************************************/
int cprdaem2001_process_init()
{
	MYSQL_RES *cpr_res;
	MYSQL_ROW cpr_row;

	char szQuery[1000]; // query string
	memset(szQuery, 0x00, sizeof(szQuery));

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------

	ZzLOG(ALWAY, "zangsi_cpr DB에 접속 합니다....\n");

	if (!(cpr_con = db_connect_cprdb("zangsi_cpr")))
	{
		ZzLOG(ERROR, "zangsi_cpr DB에 접속하지 못 하였습니다...\n");
		return S_DBERR;
	}

	return S_OK;
}

/***************************************************************************
 * 프로그램 종료루틴
 * 데이터베이스 종료 및 처리결과를 로그파일에 정의
 * (I) void
 * (R) int : 정상(0)/오류(-1)
 ****************************************************************************/
int cprdaem2001_process_term()
{
	// DB close
	db_disconnect(cpr_con);

	return (0);
}

/*****************************************************************************
 * 프로그램 시그널 처리
 * (I) void
 * (R) void
 *****************************************************************************/
void cprdaem2001_signal(int nSignal)
{
	cprdaem2001_process_term();
}

/*****************************************************************************
 *  프로그램 메인
 *****************************************************************************/

int main(int argc, char **argv)
{
	int rc;

	int nSleepTime = atoi(argv[1]);

	signal(SIGTERM, cprdaem2001_signal);
	signal(SIGINT, cprdaem2001_signal);
	signal(SIGQUIT, cprdaem2001_signal);
	signal(SIGKILL, cprdaem2001_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

#ifdef _DEBUG_
	ZzInitGlobalVariable2("D_cprdaem2001", "/logs/daemon");
#else
	ZzInitGlobalVariable2("cprdaem2001", "/logs/daemon");
#endif

	ZzLOG(ALWAY, "[cprdaem2001]*****************프로그램 시작*****************\n");

	// 프로그램 메인루틴
	for (;;)
	{

		if (cprdaem2001_process_init() == S_DBERR)
		{
			ZzLOG(ERROR, "DB error. 프로그램을 종료합니다.\n");
			return -1;
		}

		ZzLOG(ALWAY, "cprdaem2001_process 처리시작.\n");
		rc = cprdaem2001_process();

		cprdaem2001_process_term();

		if (rc == S_OK || rc == S_NODATA)
		{
			ZzLOG(ALWAY, "cprdaem2001_process 처리완료. %d초간 휴면합니다.\n", nSleepTime);
			sleep(nSleepTime);
		}
		else if (rc == S_FAIL)
		{
			ZzLOG(ERROR, "cprdaem2001_process 처리실패. 60초간 휴면합니다.\n");
			sleep(60);
		}
		else if (rc == S_DBERR)
		{
			ZzLOG(ERROR, "DB error. 프로그램을 종료합니다.\n");
			return -1;
		}
	}
	ZzLOG(ALWAY, "[cprdaem2001]*****************프로그램 종료*****************\n");

	// 프로그램 종료루틴
	return (0);
}

// 2015.11.25

int UpdateCont_N(unsigned long ulId, char *pDelServiceGu)
{
	/*
	1.T_CONTENTS_INFO disp_end_date 업데이트
	2.T_CONTENTS_VIR_ID copyright_yn 업데이트
	3.T_CONTENTS_CREATE 인서트
	4.T_MEMO_INFO 인서트
	*/

	MYSQL_RES *res;
	MYSQL_ROW row;

	char szQuery[16384];
	memset(szQuery, 0x00, sizeof(szQuery));
	char szUserId[16 + 1];
	memset(szUserId, 0x00, sizeof(szUserId));
	char szTitle[2048];
	memset(szTitle, 0x00, sizeof(szTitle));
	double ufilesize = 0;

	sprintf(szQuery, " select a.reg_user, a.title ,b.file_size "
					 " from zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_FILE b, zangsi.T_CONTENTS_VIR_ID c "
					 " where a.id = b.id and a.id = c.id and a.id = %lu and a.del_yn = 'N' and c.copyright_yn in ('C','H') ; /* cprdaem2001 */ ",
			ulId);

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "DeleteCont: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return S_FAIL;
	}

	if (!(res = mysql_store_result(con)))
	{
		ZzLOG(ERROR, "DeleteCont: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return S_FAIL;
	}
	if (mysql_num_rows(res) == 0)
	{
		ZzLOG(ALWAY, "DeleteCont: 처리할 자료없음. ulId=[%lu], \n", ulId);
		ZzLOG(ALWAY, "(%s)\n", szQuery);
		mysql_free_result(res);
		return S_NODATA;
	}

	row = mysql_fetch_row(res);
	sprintf(szUserId, "%s", getstr(row, 0));
	sprintf(szTitle, "%s", getstr(row, 1));
	ufilesize = (double)getnum(row, 2);
	mysql_free_result(res);

	if (tran_begin(con) != 0)
	{
		ZzLOG(ERROR, "DeleteCont: 트렌젝션 오류.\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		return S_FAIL;
	}

	int nPrice = 0;
	nPrice = (int)round(ufilesize / 1024 / 1024 / 100) * 10;
	if (nPrice < 10)
		nPrice = 10;

	// UpdateCont_N
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " update zangsi.T_CONTENTS_INFO "
					 " set price_amt = %d "
					 " where id = %lu ; /* cprdaem2001 */ ",
			nPrice, ulId);

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "DeleteCont: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		tran_rollback(con);
		tran_end(con);
		return S_FAIL;
	}

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " update zangsi.T_MOB_CONTENTS_INFO "
					 " set price_amt = %d "
					 " where id = %lu ; /* cprdaem2001 */ ",
			nPrice, ulId);

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "DeleteCont: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		tran_rollback(con);
		tran_end(con);
		return S_FAIL;
	}

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " update zangsi.T_CONTENTS_VIR_ID "
					 " set copyright_yn = 'N' "
					 " where id = %lu ; /* cprdaem2001 */ ",
			ulId);

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "DeleteCont: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		tran_rollback(con);
		tran_end(con);
		return S_FAIL;
	}

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " update zangsi.T_CONTENTS_VIR_ID2 "
					 " set copyright_yn = 'N' "
					 " where id = %lu ; /* cprdaem2001 */ ",
			ulId);

	ZzLOG(ALWAY, "DeleteCont: [%s]\n\n", szQuery);

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "DeleteCont: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		tran_rollback(con);
		tran_end(con);
		return S_FAIL;
	}

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " INSERT INTO zangsi.T_CONTENTS_CREATE (cont_gu, id, udt_cd) values ('01', %lu, 'I') ; /* cprdaem2001 */ ", ulId);

	ZzLOG(ALWAY, "DeleteCont: [%s]\n\n", szQuery);

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "DeleteCont: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		tran_rollback(con);
		tran_end(con);
		return S_FAIL;
	}

	if (tran_commit(con) != 0)
	{
		ZzLOG(ERROR, "DeleteCont: 트렌젝션 commit 오류.\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		tran_rollback(con);
		tran_end(con);
		return S_DBERR;
	}
	tran_end(con);

	return 0;
}

int UpdateFCont_N(unsigned long ulId)
{
	/*
	1.T_CONTFLOG_INFO price_amt, adult_yn, sect_code 업데이트
	2.T_CONTFLOG_VIR_ID adult_yn, sect_code 업데이트
	4.T_MEMO_INFO 인서트
	*/

	MYSQL_RES *res;
	MYSQL_ROW row;

	char szQuery[16384];
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select a.reg_user, a.title ,b.file_size "
					 " from zangsi.T_CONTFLOG_INFO a, zangsi.T_CONTFLOG_FILE b, zangsi.T_CONTFLOG_VIR_ID c "
					 " where a.id = b.id and a.id = c.id and a.id = %lu and a.del_yn = 'N' and c.copyright_yn in ('C','H'); /* cprdaem2001 */  ",
			ulId);

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "DeleteFCont: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return S_FAIL;
	}

	if (!(res = mysql_store_result(con)))
	{
		ZzLOG(ERROR, "DeleteFCont: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		return S_FAIL;
	}
	if (mysql_num_rows(res) == 0)
	{
		ZzLOG(ALWAY, "DeleteFCont: 처리할 자료없음. ulId=[%lu], \n", ulId);
		ZzLOG(ALWAY, "(%s)\n", szQuery);
		mysql_free_result(res);
		return S_NODATA;
	}

	char szUserId[16 + 1];
	memset(szUserId, 0x00, sizeof(szUserId));
	char szTitle[2048];
	memset(szTitle, 0x00, sizeof(szTitle));
	double ufilesize = 0;

	row = mysql_fetch_row(res);
	sprintf(szUserId, "%s", getstr(row, 0));
	sprintf(szTitle, "%s", getstr(row, 1));
	ufilesize = (double)getnum(row, 2);
	mysql_free_result(res);

	int nPrice = 0;
	nPrice = (int)round(ufilesize / 1024 / 1024 / 100) * 10;
	if (nPrice < 10)
		nPrice = 10;

	if (tran_begin(con) != 0)
	{
		ZzLOG(ERROR, "DeleteFCont: 트렌젝션 오류.\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		return S_FAIL;
	}

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " update zangsi.T_CONTFLOG_INFO "
					 " set price_amt = %d "
					 " where id = %lu ; /* cprdaem2001 */ ",
			nPrice, ulId);

	ZzLOG(ALWAY, "DeleteFCont: [%s]\n\n", szQuery);

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "DeleteFCont: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		tran_rollback(con);
		tran_end(con);
		return S_FAIL;
	}

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " update zangsi.T_CONTFLOG_VIR_ID "
					 " set copyright_yn = 'N' "
					 " where id = %lu ; /* cprdaem2001 */ ",
			ulId);

	ZzLOG(ALWAY, "DeleteFCont: [%s]\n\n", szQuery);

	if (mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "DeleteFCont: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "(%s)\n", szQuery);
		tran_rollback(con);
		tran_end(con);
		return S_FAIL;
	}

	if (tran_commit(con) != 0)
	{
		ZzLOG(ERROR, "DeleteFCont: 트렌젝션 commit 오류.\n");
		ZzLOG(ERROR, "[%d](%s)\n", mysql_errno(con), mysql_error(con));
		tran_rollback(con);
		tran_end(con);
		return S_DBERR;
	}
	tran_end(con);

	return 0;
}
