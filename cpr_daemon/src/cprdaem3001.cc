/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : cprdaem3001.cc
 *         기능 : 서비스 취소 처리
 *
 *       작성자 : HCS
 *       작성일 : 2010-04-30
 *       비 고  : 
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
#include <sys/stat.h>
#include <unistd.h> //for sleep()

#include "daemcom.h"
#include "commydb.h"

#define  MAX_ROWS	1

int cprdaem3001_init_process(int argc, char **argv);
int cprdaem3001_main_process();
int cprdaem3001_term_process();
void cprdaem3001_signal(int nSignal);

int ContentsProc(unsigned long ulId);
int ContflogProc(unsigned long ulId);
int WriteCopyright();


MYSQL     *con;
MYSQL	  *cpr_con;

char gszCompCd[6+1];
/*****************************************************************************
 * 홑따옴표를 특정 문자로 대체. 
 * arg(I) : 1. strSrcString : 입력 문자열
			2. cReplace		: 대체 문자
			3. pResult		: 결과 문자열
 * return : 1. char			: 결과 문자열
 ****************************************************************************/
char* ReplaceSingleQuotation(char* strSrcString, char cReplace,char* pResult)
{
	char strResult[130000];
	memset(strResult,0x00,sizeof(strResult));
	int cTemp = 0;
	int nSpecialCount=0;
	int nFileLen = strlen(strSrcString);
	int cOldTemp = 0;
	for(unsigned long i=0; i<nFileLen ; i++)
	{
		if( i > 0 )
		{
			cOldTemp = strSrcString[i-1];
		}
		cTemp = strSrcString[i];
		if( cTemp == '\'' && cOldTemp != '\\' ) 
		{
			strResult[nSpecialCount] = (char)cReplace;
			nSpecialCount++;
			strResult[nSpecialCount] = (char)cTemp;
		}
		else
		{
			if(cTemp<0)//한글 (is korean)
			{
				char* pHan = &strSrcString[i];
				memcpy(&strResult[nSpecialCount] ,pHan,2);
				nSpecialCount++;
				i=i+1;
			}
			else
			{
				strResult[nSpecialCount] = (char)cTemp;		
			}
		}
		nSpecialCount++;
	}
	strcpy(pResult,strResult);
	return pResult;
}
//******************************************************************************
//* cprdaem3001 main
//******************************************************************************
int cprdaem3001_main_process()
{
	char szQuery[15000];  // query string
	
	int nContCnt = 0;
	int nFlogCnt = 0;
	
	MYSQL_RES *cpr_res;
	MYSQL_ROW cpr_row;
	
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select comp_nm from zangsi_cpr.T_CPR_COMP_INFO where comp_cd = '%s' ", gszCompCd);

	if(mysql_query(cpr_con, szQuery))
	{
		ZzLOG(ERROR, "cprdaem3001_main_process: mysql_query error\n");
		ZzLOG(ERROR, "cprdaem3001_main_process: [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "cprdaem3001_main_process: szQuery=[%s]\n", szQuery);
		return -1;			
	}
	
	
	
	if(!(cpr_res = mysql_store_result(cpr_con)))
	{
		ZzLOG(ERROR, "cprdaem3001_main_process: mysql_store_result error\n");
		ZzLOG(ERROR, "cprdaem3001_main_process: [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "cprdaem3001_main_process: szQuery=[%s]\n", szQuery);
		return -1;			
	}
	if(mysql_num_rows(cpr_res)==0)
	{
		ZzLOG(ERROR, "cprdaem3001_main_process: 권리사 코드 오류 [%s].\n", gszCompCd);
		return -1;
	}	
	cpr_row = mysql_fetch_row(cpr_res);
	
	char szCompNm[40+1];
	memset(szCompNm, 0x00, sizeof(szCompNm));
	strcpy(szCompNm, getstr(cpr_row, 0));
	
	mysql_free_result(cpr_res);
	
	ZzLOG(ALWAY, "\n");
	ZzLOG(ALWAY, "cprdaem3001_main_process: %s 서비스 취소 처리 시작!!\\n\n", szCompNm);

	
	//----------------------------------------------------------------------
	// 차단 리스트 등록
	//----------------------------------------------------------------------
	/*
	if(WriteCopyright() < 0)
		return -1;
	*/
	
	//20120521
	//히스토리에 데이터 이관
	
	//----------------------------------------------------------------------
	// 제휴 정보 서비스 취소 처리
	//----------------------------------------------------------------------
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " update zangsi_cpr.T_CPR_CONT_LIST set apply_yn = 'N',seq_no =seq_no +1 where comp_cd = '%s' ", gszCompCd);

	if(mysql_query(cpr_con, szQuery))
	{
		ZzLOG(ERROR, "cprdaem3001_main_process: mysql_query error\n");
		ZzLOG(ERROR, "cprdaem3001_main_process: [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "cprdaem3001_main_process: szQuery=[%s]\n", szQuery);
		return -1;			
	}

	memset(szQuery, 0x00, sizeof(szQuery));
		
	sprintf(szQuery, " insert into zangsi_cpr.T_CPR_CONT_LIST_HIST "
						 " (list_id ,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate "
						 " , udt_user , udt_date,udt_time ) "
						 " select list_id ,seq_no,title, chapter, mgr_cd, comp_cd, sect_code, sect_sub, price_amt, adult_yn, reg_user, reg_date, reg_time, apply_yn, cpr_payment_rate   "
						 " , 'sys3001' , date_format(now(), '%%Y%%m%%d'),  date_format(now(), '%%H%%i%%s')   "
						" from zangsi_cpr.T_CPR_CONT_LIST  where comp_cd = '%s' "
						 , gszCompCd);
						 	
	

	if(mysql_query(cpr_con, szQuery))
	{
		ZzLOG(ERROR, "cprdaem3001_main_process: mysql_query error\n");
		ZzLOG(ERROR, "cprdaem3001_main_process: [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "cprdaem3001_main_process: szQuery=[%s]\n", szQuery);
		return -1;			
	}


	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " update zangsi_cpr.T_CPR_CONT_LIST a, zangsi_cpr.T_CPR_HASH_INFO b " 
					 " set b.proc_stat = 'N' " 
					 " where a.comp_cd = '%s' and a.list_id = b.list_id "
					 , gszCompCd);

	if(mysql_query(cpr_con, szQuery))
	{
		ZzLOG(ERROR, "cprdaem3001_main_process: mysql_query error\n");
		ZzLOG(ERROR, "cprdaem3001_main_process: [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "cprdaem3001_main_process: szQuery=[%s]\n", szQuery);
		return -1;			
	}

	
	//----------------------------------------------------------------------


	//----------------------------------------------------------------------
	// 컨텐츠 처리
	//----------------------------------------------------------------------
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select c.cont_gu,  c.id " 
					 " from zangsi_cpr.T_CPR_CONT_LIST a, zangsi_cpr.T_CPR_HASH_INFO b , zangsi_cpr.T_CPR_CONT_MAP_SUB c " 
					 " where a.comp_cd = '%s' and a.list_id = b.list_id and b.chi_id = c.chi_id "
					 " group by c.cont_gu, c.id "
					 , gszCompCd);
	if(mysql_query(cpr_con, szQuery))
	{
		ZzLOG(ERROR, "cprdaem3001_main_process: mysql_query error\n");
		ZzLOG(ERROR, "cprdaem3001_main_process: [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "cprdaem3001_main_process: szQuery=[%s]\n", szQuery);
		return -1;			
	}
	if(!(cpr_res = mysql_store_result(cpr_con)))
	{
		ZzLOG(ERROR, "cprdaem3001_main_process: mysql_store_result error\n");
		ZzLOG(ERROR, "cprdaem3001_main_process: [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "cprdaem3001_main_process: szQuery=[%s]\n", szQuery);
		return -1;			
	}
	if(mysql_num_rows(cpr_res)==0)
	{
		ZzLOG(ERROR, "cprdaem3001_main_process: 컨텐츠 없음.[%s].\n", szQuery);
		return -1;
	}	
	while(cpr_row = mysql_fetch_row(cpr_res))
	{
		int nRes = 0;
		if(strcmp(getstr(cpr_row,0), "WE") == 0)
		{	
			nRes = ContentsProc((unsigned long)getnum(cpr_row,1));
		}
		else
		{
			nRes = ContflogProc((unsigned long)getnum(cpr_row,1));
		}	
		if(nRes < 0)
			return -1;
		else if(nRes == 1)
			nContCnt++;
		else if(nRes ==2)
			nFlogCnt++;
			
	}
	mysql_free_result(cpr_res);
	//----------------------------------------------------------------------
	
	
    ZzLOG(ALWAY, "cprdaem3001_main_process: 일반 컨텐츠 %d 건 처리.\n", nContCnt);
    ZzLOG(ALWAY, "cprdaem3001_main_process: 필로그 컨텐츠 %d 건 처리.\n", nFlogCnt);
	return  0;
}

int ContentsProc(unsigned long ulId)
{
	char szQuery[15000];  // query string


	MYSQL_RES *res;
	MYSQL_ROW row;
	
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select a.id "
					 " from zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_FILE b "
					 " where a.id = b.id and a.id = %lu and a.del_yn = 'N' "
					 , ulId);
	if(mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "ContentsProc: mysql_query error\n");
		ZzLOG(ERROR, "ContentsProc: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "ContentsProc: szQuery=[%s]\n", szQuery);
		tran_rollback(con);
		tran_end(con);
		
		return -1;			
	}
	if(!(res = mysql_store_result(con)))
	{
		ZzLOG(ERROR, "ContentsProc: mysql_store_result error\n");
		ZzLOG(ERROR, "ContentsProc: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "ContentsProc: szQuery=[%s]\n", szQuery);
		tran_rollback(con);
		tran_end(con);
		return -1;			
	}
	if(mysql_num_rows(res)==0)
	{
		mysql_free_result(res);
		return 0;		
	}	
	mysql_free_result(res);

	if (tran_begin(con)!=0)
	{
		ZzLOG(ERROR, "ContentsProc: 트렌젝션 오류.\n");
		ZzLOG(ERROR, "ContentsProc: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " update zangsi.T_CONTENTS_INFO set disp_end_date = date_format(date_add(now(), interval 7 day), '%%Y%%m%%d') where id = %lu", ulId);

	if(mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "ContentsProc: mysql_query error\n");
		ZzLOG(ERROR, "ContentsProc: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "ContentsProc: szQuery=[%s]\n", szQuery);
		tran_rollback(con);
		tran_end(con);
		return -1;
	}

	
	
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " update zangsi.T_CONTENTS_VIR_ID set copyright_yn = 'X' where id = %lu", ulId);

	if(mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "ContentsProc: mysql_query error\n");
		ZzLOG(ERROR, "ContentsProc: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "ContentsProc: szQuery=[%s]\n", szQuery);
		tran_rollback(con);
		tran_end(con);
		return -1;
	}

	

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " update zangsi.T_CONTENTS_VIR_ID2 set copyright_yn = 'X' where id = %lu", ulId);

	if(mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "ContentsProc: mysql_query error\n");
		ZzLOG(ERROR, "ContentsProc: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "ContentsProc: szQuery=[%s]\n", szQuery);
		tran_rollback(con);
		tran_end(con);
		return -1;
	}

	

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " insert into zangsi.T_CONTENTS_CREATE (id, cont_gu, udt_cd) values (%lu, '01', 'D') ", ulId);

	if(mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "ContentsProc: mysql_query error\n");
		ZzLOG(ERROR, "ContentsProc: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "ContentsProc: szQuery=[%s]\n", szQuery);
		tran_rollback(con);
		tran_end(con);
		return -1;
	}

	

	if (tran_commit(con)!=0)
	{
		ZzLOG(ERROR, "ContentsProc: 트렌젝션 commit 오류.\n");
		ZzLOG(ERROR, "ContentsProc: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		tran_rollback(cpr_con);
		tran_end(cpr_con);
		return -1;
	}
	
	tran_end(cpr_con);
	
	usleep(10000);
	
	return 1;
	
}

int ContflogProc(unsigned long ulId)
{
	char szQuery[15000];  // query string


	MYSQL_RES *res;
	MYSQL_ROW row;
	
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select a.id "
					 " from zangsi.T_CONTFLOG_INFO a, zangsi.T_CONTFLOG_FILE b "
					 " where a.id = b.id and a.id = %lu and a.del_yn = 'N' "
					 , ulId);
	if(mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "ContflogProc: mysql_query error\n");
		ZzLOG(ERROR, "ContflogProc: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "ContflogProc: szQuery=[%s]\n", szQuery);
		tran_rollback(con);
		tran_end(con);
		
		return -1;			
	}
	if(!(res = mysql_store_result(con)))
	{
		ZzLOG(ERROR, "ContflogProc: mysql_store_result error\n");
		ZzLOG(ERROR, "ContflogProc: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "ContflogProc: szQuery=[%s]\n", szQuery);
		tran_rollback(con);
		tran_end(con);
		return -1;			
	}
	if(mysql_num_rows(res)==0)
	{
		mysql_free_result(res);
		return 0;		
	}	
	mysql_free_result(res);

	if (tran_begin(con)!=0)
	{
		ZzLOG(ERROR, "ContflogProc: 트렌젝션 오류.\n");
		ZzLOG(ERROR, "ContflogProc: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " update zangsi.T_CONTFLOG_INFO set disp_end_date = date_format(date_add(now(), interval 7 day), '%%Y%%m%%d') where id = %lu", ulId);

	if(mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "ContentsProc: mysql_query error\n");
		ZzLOG(ERROR, "ContentsProc: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "ContentsProc: szQuery=[%s]\n", szQuery);
		tran_rollback(con);
		tran_end(con);
		return -1;
	}
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " update zangsi.T_CONTFLOG_VIR_ID set copyright_yn = 'X' where id = %lu", ulId);

	if(mysql_query(con, szQuery))
	{
		ZzLOG(ERROR, "ContflogProc: mysql_query error\n");
		ZzLOG(ERROR, "ContflogProc: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		ZzLOG(ERROR, "ContflogProc: szQuery=[%s]\n", szQuery);
		tran_rollback(con);
		tran_end(con);
		return -1;
	}

	if (tran_commit(con)!=0)
	{
		ZzLOG(ERROR, "ContflogProc: 트렌젝션 commit 오류.\n");
		ZzLOG(ERROR, "ContflogProc: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		tran_rollback(cpr_con);
		tran_end(cpr_con);
		return -1;
	}
	
	tran_end(cpr_con);
	
	usleep(10000);
	
	return 2;
	
}
int WriteCopyright()
{
	char szQuery[15000];  // query string
	
	MYSQL_RES *cpr_res;
	MYSQL_ROW cpr_row;
	
	int nCnt = 0;

	
	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select comp_nm from zangsi_cpr.T_CPR_COMP_INFO where comp_cd = '%s' ", gszCompCd);
						
	if(mysql_query(cpr_con, szQuery))
	{
		ZzLOG(ERROR, "WriteCopyright: mysql_query error\n");
		ZzLOG(ERROR, "WriteCopyright: [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "WriteCopyright: szQuery=[%s]\n", szQuery);
		return -1;			
	}
	if(!(cpr_res = mysql_store_result(cpr_con)))
	{
		ZzLOG(ERROR, "WriteCopyright: mysql_store_result error\n");
		ZzLOG(ERROR, "WriteCopyright: [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "WriteCopyright: szQuery=[%s]\n", szQuery);
		return -1;			
	}
	if(mysql_num_rows(cpr_res)==0)
	{
		ZzLOG(ERROR, "WriteCopyright: 권리사 코드 오류 [%s].\n", gszCompCd);
		return -1;
	}	
	cpr_row = mysql_fetch_row(cpr_res);
	
	char szCompNm[40+1];
	memset(szCompNm, 0x00, sizeof(szCompNm));
	strcpy(szCompNm, getstr(cpr_row, 0));
	
	mysql_free_result(cpr_res);

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select "
					 " a.title, a.sect_code, b.sect_sub, "
					 " c.default_hash, c.file_name, c.file_size, "
					 " date_format(now(), '%%Y%%m%%d'), date_format(now(), '%%H%%i%%s') "
					 " from zangsi_cpr.T_CPR_CONT_LIST a, zangsi_cpr.T_CPR_HASH_INFO b, zangsi_cpr.T_CPR_HASH_FILE c "
					 " where a.list_id = b.list_id and a.comp_cd = b.comp_cd and b.chi_id = c.chi_id "
					 " and a.comp_cd = '%s' " 
					 " and a.apply_yn = 'Y' and b.proc_stat = 'C' and length(c.default_hash) > 0 "
					 " group by c.chf_id "
					 , gszCompCd);
						
	if(mysql_query(cpr_con, szQuery))
	{
		ZzLOG(ERROR, "WriteCopyright: mysql_query error\n");
		ZzLOG(ERROR, "WriteCopyright: [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "WriteCopyright: szQuery=[%s]\n", szQuery);
		return -1;			
	}
	if(!(cpr_res = mysql_store_result(cpr_con)))
	{
		ZzLOG(ERROR, "WriteCopyright: mysql_store_result error\n");
		ZzLOG(ERROR, "WriteCopyright: [%d](%s)\n",mysql_errno(cpr_con), mysql_error(cpr_con));
		ZzLOG(ERROR, "WriteCopyright: szQuery=[%s]\n", szQuery);
		return -1;			
	}
	if(mysql_num_rows(cpr_res)==0)
	{
		mysql_free_result(cpr_res);
		ZzLOG(ALWAY, "WriteCopyright: 결과 없음.[%s].\n", szQuery);
		return 1;
	}	
	while(cpr_row = mysql_fetch_row(cpr_res))
	{
		char szTitle[255+1];
		memset(szTitle, 0x00, sizeof(szTitle));
		strcpy(szTitle, getstr(cpr_row, 0));

		char szSectCode[2+1];
		memset(szSectCode, 0x00, sizeof(szSectCode));
		strcpy(szSectCode, getstr(cpr_row, 1));		

		char szSectSub[2+1];
		memset(szSectSub, 0x00, sizeof(szSectSub));
		strcpy(szSectSub, getstr(cpr_row, 2));		

		char szDefaultHash[32+1];
		memset(szDefaultHash, 0x00, sizeof(szDefaultHash));
		strcpy(szDefaultHash, getstr(cpr_row, 3));

		char szFileName[255+1];
		memset(szFileName, 0x00, sizeof(szFileName));
		strcpy(szFileName, getstr(cpr_row, 4));
		ReplaceSingleQuotation(szFileName, '\'',szFileName);

		double dFileSize = 0;
		dFileSize = getnum(cpr_row, 5);

		char szRegDate[8+1];
		memset(szRegDate, 0x00, sizeof(szRegDate));
		strcpy(szRegDate, getstr(cpr_row, 6));

		char szRegTime[6+1];
		memset(szRegTime, 0x00, sizeof(szRegTime));
		strcpy(szRegTime, getstr(cpr_row, 6));
		
		MYSQL_RES *res;
		MYSQL_ROW row;

		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " select a.id as id, a.proc_stat, b.filter_yn "
						 " from zangsi.T_CONTENTS_COPYRIGHT_HASH_INFO a ,zangsi.T_CONTENTS_COPYRIGHT_HASH_FILE b "
						 " where a.id = b.info_id and "
						 " b.file_size  = %15.0f and b.default_hash = '%s'"
						 , dFileSize, szDefaultHash);
		
		if(mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "WriteCopyright: mysql_query error\n");
			ZzLOG(ERROR, "WriteCopyright: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "WriteCopyright: szQuery=[%s]\n", szQuery);
			return -1;			
		}
		if(!(res = mysql_store_result(con)))
		{
			ZzLOG(ERROR, "WriteCopyright: mysql_store_result error\n");
			ZzLOG(ERROR, "WriteCopyright: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "WriteCopyright: szQuery=[%s]\n", szQuery);
			return -1;			
		}
		if(mysql_num_rows(res) > 0)
		{
			while(row = mysql_fetch_row(res))
			{
				if(strcmp(getstr(row,1), "C") != 0)
				{
					memset(szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, " update zangsi.T_CONTENTS_COPYRIGHT_HASH_INFO set proc_stat = 'C' where id = %lu ", (unsigned long)getnum(row,0));
					if(mysql_query(con, szQuery))
					{
						ZzLOG(ERROR, "WriteCopyright: mysql_query error\n");
						ZzLOG(ERROR, "WriteCopyright: [%d](%s)\n",mysql_errno(con), mysql_error(con));
						ZzLOG(ERROR, "WriteCopyright: szQuery=[%s]\n", szQuery);
						return -1;			
					}
					nCnt++;
					usleep(10000);
				}
			}
			mysql_free_result(res);
			continue;
		}	
		mysql_free_result(res);
		
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi.T_CONTENTS_COPYRIGHT_HASH_INFO "
		                 " (copyright_user, title, sect_code, sect_sub, proc_stat, reg_user, reg_date, reg_time) "
		                 " values "
		                 " ('%s'          , '%s' , '%s'     , '%s'    , 'C'      , 'system', '%s'    , '%s') "
		                 , szCompNm
		                 , szTitle
		                 , szSectCode
		                 , szSectSub
		                 , szRegDate
		                 , szRegTime);
	
		if(mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "WriteCopyright: mysql_query error\n");
			ZzLOG(ERROR, "WriteCopyright: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "WriteCopyright: szQuery=[%s]\n", szQuery);
			return -1;			
		}
	

		

		
		unsigned long ulHashId = 0;
		ulHashId = mysql_insert_id(con);
		
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi.T_CONTENTS_COPYRIGHT_HASH_FILE "
		                 " (info_id, filter_yn, mureka_yn, default_hash, file_name, file_size, reg_user, reg_date, reg_time) "
		                 " values "
		                 " ('%lu'   , 'Y'     , 'Y'      , '%s'       , '%s'     , '%15.0f', 'system', '%s'    , '%s') "
		                 , ulHashId
		                 , szDefaultHash
		                 , szFileName
		                 , dFileSize
		                 , szRegDate
		                 , szRegTime);
	
		if(mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "WriteCopyright: mysql_query error\n");
			ZzLOG(ERROR, "WriteCopyright: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			ZzLOG(ERROR, "WriteCopyright: szQuery=[%s]\n", szQuery);
			return -1;			
		}
	
		nCnt++;
		usleep(10000);
	}
	mysql_free_result(cpr_res);

	ZzLOG(ALWAY, "WriteCopyright: 차단 해시 입력 %d 건 처리.\n", nCnt);
	return 0;
}

/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int cprdaem3001_init_process(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
    
    #ifdef _DEBUG_
    ZzInitGlobalVariable2("D_cprdaem3001_", "/logs/daemon"); 
	#else
    ZzInitGlobalVariable2("cprdaem3001_", "/logs/daemon"); 
    #endif

    ZzLOG(ALWAY, "[cprdaem3001]*****************프로그램 시작*****************\n");  
    ZzLOG(ALWAY, "[cprdaem3001] 서비스 취소 처리.\n");  

    // 파라미터 값 설정 및 초기화
	if(argc != 2)
		goto arg_error;
			
	strcpy(gszCompCd, argv[1]);
	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------

	if(!(con=db_connect_to_main("zangsi")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
	   	return(-1); 
	}

	if (!(cpr_con=db_connect_cprdb("zangsi_cpr")))
	{
		ZzLOG(ERROR, "cpr DB에 접속하지 못 하였습니다...\n");
	   	return(-1); 
	}
    return (0);


arg_error:
    ZzLOG(ERROR, "usage : %s comp_cd\n", argv[0]);
    ZzLOG(ERROR, "        comp_cd(권리사 코드)\n");

    ZzPRT(ERROR, "usage : %s comp_cd\n", argv[0]);
    ZzPRT(ERROR, "        comp_cd(권리사 코드)\n");
    return -1;
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int cprdaem3001_term_process()
{
    // DB close
	db_disconnect(con);
	db_disconnect(cpr_con);	
    ZzLOG(ALWAY, "[cprdaem3001]*****************프로그램 종료*****************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  cprdaem3001_signal(int nSignal)
{
    cprdaem3001_term_process();
}

/*****************************************************************************
*  프로그램 메인 
*****************************************************************************/
int main(int argc, char **argv)
{                
	char    szTemp[1024];
	int     rc;
                 
	/*       
	** SIGNAL 정의
	*/       
/*
	signal(SIGTERM, cprdaem3001_signal);
	signal(SIGINT,  cprdaem3001_signal);
	signal(SIGQUIT, cprdaem3001_signal);
	signal(SIGKILL, cprdaem3001_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
*/
	if ( cprdaem3001_init_process(argc, argv) == 0 ) {
		/* 프로그램 메인루틴 */
		rc = cprdaem3001_main_process();
		/* 프로그램 종료루틴 */
	}
	cprdaem3001_term_process();
	return(0);
}
/*****************************************************************************
*  End of file...
*****************************************************************************/


