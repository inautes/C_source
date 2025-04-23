/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daemschstats.cc
 *         기능 : 검색어 통계 집계
 *         설명 : 검색서버의 검색어 통계 가져와 DB에 저장
 *       작성자 : HCS
 *       작성일 : 2010/03/24
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
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h> //for usleep();
#include <regex.h>

#include <curl/curl.h> //for cURL
#include <curl/types.h> //for cURL
#include <curl/easy.h> //for cURL

#include "daemcom.h"
#include "commydb.h"

#define  MAX_ROWS	1
#define _DEBUG_
#define NUMBER		10

int GetSysdate();
int InitProcess(int argc, char **argv);
int MainProcess();
int TermProcess();
void Signal(int nSignal);
int DBWriterSum();

MYSQL     *con;

char g_szRegYear[4+1];
char g_szRegDate[4+1];
char g_szHlDate[4+1];

char g_szRegTime[2+1];
char g_szHlTime[2+1];

char g_szProcCd[4+1];

int Depth, is_value;

struct recv_data_MemoryStruct {
	char *memory;
	size_t size;
};

char* AppendSpecialChar(char* strSrcString, char cReplace,char* pResult)
{

	char strResult[130000];

	memset(strResult,0x00,sizeof(strResult));
	
	int cTemp;
	int nSpecialCount=0;
	int nFileLen = strlen(strSrcString);
	int cOldTemp ;
	
	
	for(unsigned long i=0; i<nFileLen ; i++)
	{
		if( i > 0 )
		{
			cOldTemp = strSrcString[i-1];
		}
		cTemp = strSrcString[i];
		
	
		
		if( (cTemp == '\'' && cOldTemp != '\\') || cTemp == '\\') 
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
int recv_data_WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
	register int realsize = size * nmemb;
	struct recv_data_MemoryStruct *mem = (struct recv_data_MemoryStruct *)data;
	
	mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory) 
	{
		memcpy(&(mem->memory[mem->size]), ptr, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;
	}
	return realsize;
}

int GetHTTPFilePostAPI(char** ppResult, char* url)
{
	int nRet = 0;
	CURL *curl_handle;
	CURLcode res;
	struct curl_slist *headers = NULL;

	recv_data_MemoryStruct recv;

	recv.memory = NULL;
	recv.size = 0;
	
	/* init the curl session */
	curl_handle = curl_easy_init();

	if(curl_handle == NULL)
	{
		ZzLOG(ERROR, "GetHTTPFilePostAPI: curl_easy_init failed\n");
		nRet = 1;
		return nRet;
	}
	
	do 
	{
		// SSL 인증서 hostname, 날짜 무시하고 통신하도록
		res = curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);
		if(res != CURLE_OK) 
		{
			ZzLOG(ERROR, "GetHTTPFilePostAPI: CURLOPT_SSL_VERIFYHOST failed\n"); 
			nRet = 2; 
			break; 
		}
		
		res = curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);	
		if(res != CURLE_OK) 
		{ 
			ZzLOG(ERROR, "GetHTTPFilePostAPI: CURLOPT_SSL_VERIFYPEER failed\n"); 
			nRet = 3; 
			break;
		}
	
		// multithread 에서 사용하려면 CURLOPT_NOSIGNAL 1로 세팅
		res = curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 0);
		if(res != CURLE_OK) 
		{
			ZzLOG(ERROR, "GetHTTPFilePostAPI: CURLOPT_NOSIGNAL failed\n"); 
			nRet = 11; 
			break; 
		}

		res = curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT , 60);

		/* specify URL to get */
		res = curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		if(res != CURLE_OK) 
		{ 
			ZzLOG(ERROR, "GetHTTPFilePostAPI: CURLOPT_URL failed\n"); 
			nRet = 4; 
			break; 
		}
		/* send all data to this function  */
		res = curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, recv_data_WriteMemoryCallback);
		if(res != CURLE_OK) 
		{ 
			ZzLOG(ERROR, "GetHTTPFilePostAPI: CURLOPT_WRITEFUNCTION failed\n"); 
			nRet = 9; 
			break; 
		}
		
		/* we pass our 'chunk' struct to the callback function */
		res = curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&recv);
		if(res != CURLE_OK) 
		{ 
			ZzLOG(ERROR, "GetHTTPFilePostAPI: CURLOPT_WRITEDATA failed\n"); 
			nRet = 10;
			break; 
		}

		res = curl_easy_perform(curl_handle);

		if(res != CURLE_OK) 
		{ 
			ZzLOG(ERROR, "GetHTTPFilePostAPI: CURLE_ERR num [ %d ] \n",res);
			nRet = 90;			
		}
		else
		{
			
			nRet = res;
			
			*ppResult = new char[recv.size+1];
			memset(*ppResult, 0x00, recv.size+1);
			memcpy(*ppResult, recv.memory, recv.size);
		}
		
	} while (0);

	/* cleanup curl stuff */
	curl_easy_cleanup(curl_handle);

	return nRet;
}

char* CopySchData(const char *buf, int start, int end)
{
	int nDataSize = 0;
	nDataSize = end - start;
	
	if(nDataSize <= 0)
	{
		return NULL;
	}
	
	char* pRetData = new char[nDataSize+1];
	memset(pRetData, 0x00, nDataSize+1);
	
	memcpy(pRetData, buf+start, nDataSize);
	
	return pRetData;
}

int DBWriter(char* pServerId, char* pType, char* pResult, int nRanking, char* pKeyWord, int nCount, char* pCodeDiv, double dPercnt, char* pChangeValue, int nTotCnt )
{
	MYSQL_RES *res;
	MYSQL_ROW  row;

    char szQuery[10000];
    memset(szQuery, 0x00, sizeof(szQuery));
	
	char szKeyWord[128];
	memset(szKeyWord, 0x00, sizeof(szKeyWord));
	
	AppendSpecialChar(pKeyWord, '\\', szKeyWord);
	    
    if(strcmp(pType, "DAY") == 0)
	{
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "INSERT INTO zangsi_sum.T_RANK_KEYWORD_DAY values ('%s', '%s%s', '%s', %d, '%s', %d, '%s', %.2f, '%s', %d)"
						 , pServerId, g_szRegYear, g_szRegDate, pResult, nRanking, szKeyWord, nCount, pCodeDiv, dPercnt, pChangeValue, nTotCnt);
	}
    else if(strcmp(pType, "TIME") == 0)
	{
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "INSERT INTO zangsi_sum.T_RANK_KEYWORD_TIME values ('%s', '%s%s', '%s', '%s', %d, '%s', %d, '%s', %.2f, '%s', %d)"
						 , pServerId, g_szRegYear, g_szRegDate, g_szRegTime, pResult, nRanking, szKeyWord, nCount, pCodeDiv, dPercnt, pChangeValue, nTotCnt);
	}
    else
    {
    	ZzLOG(ERROR, "DBWriter: unkown type(%s)\n", pType);
    	return -1;
    }
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "DBWriter: insert error...\n");
	    ZzLOG(ERROR, "DBWriter: szQuery=[%s]\n", szQuery);
		ZzLOG(ERROR, "DBWriter: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}

	return 0;	
}
int DBWriterSum()
{
	MYSQL_RES *res;
	MYSQL_ROW  row;

    char szQuery[10000];
    memset(szQuery, 0x00, sizeof(szQuery));
	
    if(strcmp(g_szProcCd, "DAY") == 0)
	{
		ZzLOG(ALWAY, "DBWriterSum: g_szProcCd=[%s]\n", g_szProcCd);

		return 0;
	}
    else if(strcmp(g_szProcCd, "TIME") == 0)
	{
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "INSERT INTO zangsi_sum.T_RANK_KEYWORD_TIME_SUM (stat_date, stat_time, result, keyword, count, percnt, tot_cnt) "
						 " select stat_date, stat_time, result, keyword, sum(count), sum(percnt), sum(tot_cnt) "
						 " from zangsi_sum.T_RANK_KEYWORD_TIME where stat_date = '%s%s' and stat_time = '%s' and result = 'SUCC' and keyword !='?' "
						 " group by stat_date, stat_time, result, keyword "
						 " order by 5 desc limit 1 "
						 , g_szRegYear , g_szRegDate, g_szRegTime);
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "DBWriterSum: insert error...\n");
		    ZzLOG(ERROR, "DBWriterSum: szQuery=[%s]\n", szQuery);
			ZzLOG(ERROR, "DBWriterSum: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
		}
		ZzLOG(ALWAY, "Query=[%s]\n", szQuery);

		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "INSERT INTO zangsi_sum.T_RANK_KEYWORD_TIME_SUM (stat_date, stat_time, result, keyword, count, percnt, tot_cnt) "
						 " select stat_date, stat_time, result, keyword, sum(count), sum(percnt), sum(tot_cnt) "
						 " from zangsi_sum.T_RANK_KEYWORD_TIME where stat_date = '%s%s' and stat_time = '%s' and result = 'FAIL' and keyword !='?' "
						 " group by stat_date, stat_time, result, keyword "
						 " order by 5 desc limit 1 "
						 , g_szRegYear , g_szRegDate, g_szRegTime);
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "DBWriterSum: insert error...\n");
		    ZzLOG(ERROR, "DBWriterSum: szQuery=[%s]\n", szQuery);
			ZzLOG(ERROR, "DBWriterSum: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
		}
		ZzLOG(ALWAY, "Query=[%s]\n", szQuery);

		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "INSERT INTO zangsi_sum.T_RANK_KEYWORD_TIME_SUM (stat_date, stat_time, result, keyword, count, percnt, tot_cnt) "
						 " select stat_date, stat_time, 'ALL', keyword, sum(count), sum(percnt), sum(tot_cnt) "
						 " from zangsi_sum.T_RANK_KEYWORD_TIME where stat_date = '%s%s' and stat_time = '%s' and keyword !='?' "
						 " group by stat_date, stat_time, keyword "
						 " order by 5 desc limit 1 "
						 , g_szRegYear , g_szRegDate, g_szRegTime);
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "DBWriterSum: insert error...\n");
		    ZzLOG(ERROR, "DBWriterSum: szQuery=[%s]\n", szQuery);
			ZzLOG(ERROR, "DBWriterSum: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
		}
		ZzLOG(ALWAY, "Query=[%s]\n", szQuery);

	}
    else
    {
    	ZzLOG(ERROR, "DBWriterSum: unkown type(%s)\n", g_szProcCd);
    	return -1;
    }

	return 0;	
}


int XmlParser(char* pServerId, char* pType, char* pResult, char* pXmlData)
{
	int i;
	int result;
	int start=0, end=0;
	
	regex_t *compiled;
	regmatch_t *matchptr;
	size_t nmatch;
	
	if( (compiled = (regex_t*)malloc(sizeof(regex_t))) == NULL ) 
	{
		ZzLOG(ERROR, "XmlParser: regex_t malloc error\n" );
		return -1;
	}
	
	char szRex[128];
	memset(szRex, 0x00, sizeof(szRex));
	
	strcpy(szRex, "<totcnt>(.*)</totcnt>|ranking id=\"(.*)\".count=\"(.*)\".code_div=\"(.*)\".percnt=\"(.*)\".hlword=\"(.*)\"|CDATA(.*).<");
	
	
	if( regcomp( compiled, szRex, REG_EXTENDED | REG_ICASE | REG_NEWLINE ) != 0 ) 
	{
		ZzLOG(ERROR, "XmlParser: regcomp error\n" );
		return -1;
	}
	
	nmatch = compiled->re_nsub+1;
	
	if( (matchptr = (regmatch_t*)malloc(sizeof(regmatch_t)*nmatch)) == NULL ) 
	{
		ZzLOG(ERROR, "XmlParser: regmatch_t malloc error\n" );
		return -1;
	}
	
	int nTotalCnt = 0;
	int nRanking = 0;
	int nCount = 0;
	double dPercnt = 0;
	char szChange[16];
	memset(szChange, 0x00, sizeof(szChange));
	char szKeyWord[128];
	memset(szKeyWord, 0x00, sizeof(szKeyWord));
	
	//20100803. hcs. code_div 추가
	char szCodeDiv[2+1];
	memset(szCodeDiv, 0x00, sizeof(szCodeDiv));
	
	
	int nInsertCnt = 0;
	
	while( (result = regexec( compiled, pXmlData+start, nmatch, matchptr, 0)) == 0 ) 
	{
		char* pTag = CopySchData( pXmlData, start+matchptr[0].rm_so, start+matchptr[0].rm_eo );

		if(strstr(pTag, "totcnt") != NULL)
		{
			char* pTotCnt = CopySchData( pXmlData, start+matchptr[1].rm_so, start+matchptr[1].rm_eo );
			nTotalCnt = 0;
			nTotalCnt = atoi(pTotCnt);
			
			if(pTag) delete[] pTag;
			if(pTotCnt) delete[] pTotCnt;
		}
		else if(strstr(pTag, "ranking id") != NULL)
		{
			char* pRanking = CopySchData( pXmlData, start+matchptr[2].rm_so, start+matchptr[2].rm_eo );
			nRanking = 0;
			nRanking = atoi(pRanking);
			
			char* pCount = CopySchData( pXmlData, start+matchptr[3].rm_so, start+matchptr[3].rm_eo );
			nCount = 0;
			nCount = atoi(pCount);

			//20100803. hcs. code_div 추가
			char* pCodeDiv = CopySchData( pXmlData, start+matchptr[4].rm_so, start+matchptr[4].rm_eo );
			memset(szCodeDiv, 0x00, sizeof(szCodeDiv));
			memcpy(szCodeDiv, pCodeDiv, strlen(pCodeDiv));
			
			char* pPercnt = CopySchData( pXmlData, start+matchptr[5].rm_so, start+matchptr[5].rm_eo );
			dPercnt = 0;
			dPercnt = atof(pPercnt);

			char* pChange = CopySchData( pXmlData, start+matchptr[6].rm_so, start+matchptr[6].rm_eo );
			memset(szChange, 0x00, sizeof(szChange));
			sprintf(szChange, "%s", pChange);
			
			if(pRanking) delete[] pRanking;
			if(pCount) delete[] pCount;
			if(pCodeDiv) delete[] pCodeDiv;
			if(pPercnt) delete[] pPercnt;
			if(pChange) delete[] pChange;
			if(pTag) delete[] pTag;
			
		}
		else if(strstr(pTag, "CDATA") != NULL)
		{
			char* pKeyword = CopySchData( pXmlData, start+matchptr[7].rm_so, start+matchptr[7].rm_eo );
			memset(szKeyWord, 0x00, sizeof(szKeyWord));
			
			memcpy(szKeyWord, pKeyword+1, strlen(pKeyword)-3);
			//strcpy(szKeyWord, pKeyword);
			
			if(pKeyword) delete[] pKeyword;
				
			DBWriter(pServerId, pType, pResult, nRanking, szKeyWord, nCount, szCodeDiv, dPercnt, szChange, nTotalCnt);
			nInsertCnt++;
			
		}	
		
		start += matchptr[0].rm_eo;
	}
	
	regfree( compiled );
	ZzLOG(ALWAY, "XmlParser: 인서트 건수 = %d\n", nInsertCnt);
	
	return 0;
}

int GetXml(char* pServerId, char* pServerIp, char* pTypeName, char* pResult)
{
	char szUrl[1024];
	memset(szUrl,0x00,sizeof(szUrl));
	
	char szData[1024];
	memset(szData,0x00,sizeof(szData));
	
	sprintf(szUrl, "http://%s:7578/srch_statisxml", pServerIp);
	
	if(strcmp(g_szProcCd, "DAY") == 0)
		sprintf(szData, "?w=%s&cycle=d&date=%s&hl_date=%s&base64=1&outmax=1000", pTypeName, g_szRegDate, g_szHlDate);
	else
		sprintf(szData, "?w=%s&cycle=h&date=%s&hl_date=%s&base64=1&outmax=1000", pTypeName, g_szRegTime, g_szHlTime);
	
	strcat(szUrl, szData);

	char* pXmlData = NULL;
	
	int nRet = GetHTTPFilePostAPI(&pXmlData, szUrl);
	if(nRet != 0)
	{
		ZzLOG(ERROR, "MainProcess: GetHTTPFilePostAPI()실패...nRet = %d\n", nRet);
		ZzLOG(ERROR, "MainProcess: szUrl=[%s]\n", szUrl);

		if(pXmlData)
			delete[] pXmlData;

		return -1;
	}
	
	/*
	쿼리 결과 파싱하여 DB에 저장
	*/
	ZzLOG(ALWAY, "GetXml: 파싱 시작.\n");
	ZzLOG(ALWAY, "GetXml: %s\n", szUrl);
	
	XmlParser(pServerId, g_szProcCd, pResult, pXmlData);
	
	ZzLOG(ALWAY, "GetXml: 파싱 종료.\n");
	if(pXmlData)
		delete[] pXmlData;
		
	return 0;
}


//******************************************************************************
//* daemschstats main
//******************************************************************************
int MainProcess()
{
    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "MainProcess START !! year=[%s] date=[%s] hl_date=[%s] \n", g_szRegYear, g_szRegDate, g_szHlDate);  
    ZzLOG(ALWAY, "                     time=[%s] hl_time=[%s] type=[%s]\n", g_szRegTime, g_szHlTime, g_szProcCd);  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");
	
	/*
	검색 서버별로 집계 쿼리
	*/
	MYSQL_RES *res;
	MYSQL_ROW  row;

    char szQuery[10000];
    memset(szQuery, 0x00, sizeof(szQuery));
    
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "select server_id, server_ip, type, type_name, result from zangsi_sum.T_RANK_KEYWORD_CFG where type = '%s'", g_szProcCd);
	
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "MainProcess: SELECT mysql_query error...\n");
		ZzLOG(ERROR, "MainProcess: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) 
	{
	    ZzLOG(ERROR, "MainProcess: SELECT mysql_store_result error...\n");
		ZzLOG(ERROR, "MainProcess: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)	
 	{
	    ZzLOG(ERROR, "MainProcess: Data not found...\n");
		return -1;
	}

	while(row = mysql_fetch_row(res))
	{
		GetXml(getstr(row,0), getstr(row,1), getstr(row,3), getstr(row,4));
	}
	mysql_free_result(res);
	
	DBWriterSum();
	
	return 0;
}



int GetSysdate()
{
	char szQuery[1000];		// query string
	char szTimeQuery[500];		// query string
	

	MYSQL_RES *res;
	MYSQL_ROW  row;

	memset(szQuery, 0x00, sizeof(szQuery));
	memset(szTimeQuery, 0x00, sizeof(szTimeQuery));
	
	
	if(strcmp(g_szRegDate, "0000") == 0)
	{
		if(strcmp(g_szProcCd, "DAY") == 0)
			strcpy(szQuery, "SELECT date_format(now(),'%Y') , date_format(date_add(now(), interval -1 day),'%m%d'), date_format(date_add(now(), interval -2 day),'%m%d')");
		else
			strcpy(szQuery, "SELECT date_format(now(),'%Y') , date_format(now(),'%m%d'), date_format(date_add(now(), interval -1 day),'%m%d')");
	}
	else
		sprintf(szQuery, "SELECT date_format(now(),'%%Y'), date_format(date_add(%s, interval -1 day),'%%m%%d')", g_szRegDate);
	
	if(strcmp(g_szRegTime, "TT") == 0)
		strcpy(szTimeQuery, ", date_format(date_add(now(), interval -1 hour), '%H'), date_format(date_add(now(), interval -2 hour), '%H')");
	else
		sprintf(szTimeQuery, " '%s', concat(if(length(%s - 1) = 1, '0', ''), %s - 1)", g_szRegTime, g_szRegTime);
		
	strcat(szQuery, szTimeQuery);
	
	//printf("%s\n", szQuery);

	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "GetSysdate: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "GetSysdate: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "GetSysdate: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
	
	row = mysql_fetch_row(res);
	
	memset(g_szRegYear, 0x00, sizeof(g_szRegYear));
	memset(g_szRegDate, 0x00, sizeof(g_szRegDate));
	memset(g_szHlDate, 0x00, sizeof(g_szHlDate));
	memset(g_szRegTime, 0x00, sizeof(g_szRegTime));
	memset(g_szHlTime, 0x00, sizeof(g_szHlTime));

	strcpy(g_szRegYear ,   getstr(row, 0));
	strcpy(g_szRegDate ,   getstr(row, 1));
	strcpy(g_szHlDate ,   getstr(row, 2));
	strcpy(g_szRegTime ,   getstr(row, 3));
	

	if(strcmp(g_szRegTime, "00") == 0)
		strcpy(g_szHlTime ,   "23");
	else
		strcpy(g_szHlTime ,   getstr(row, 4));
	
	if(strcmp(g_szProcCd, "TIME") == 0 && strcmp(g_szRegTime, "23") == 0)
		strcpy(g_szRegDate, g_szHlDate);
	
	mysql_free_result(res);
	

	return 0;
}

/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int InitProcess(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
#ifdef __DEBUG
    ZzInitGlobalVariable2("D_daemschstats", "/logs/daemon"); 
#else
    ZzInitGlobalVariable2("daemschstats", "/logs/daemon"); 
#endif    

	memset(g_szRegYear, 0x00, sizeof(g_szRegYear));
	memset(g_szRegDate, 0x00, sizeof(g_szRegDate));
	

    ZzLOG(ALWAY, "[daemschstats]*****************프로그램 시작*****************\n");  
    
    if(argc < 4)
    	goto arg_error;

	strcpy(g_szRegDate, argv[1]);
	strcpy(g_szRegTime, argv[2]);
	strcpy(g_szProcCd, argv[3]);
	
	//printf("%s %s %s %s \n", argv[0], argv[1],argv[2], argv[3]);
	
	if(strcmp(g_szProcCd, "DAY") != 0 && strcmp(g_szProcCd, "TIME") != 0) 
		goto arg_error;
	
	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(con=db_connect_backup("zangsi_sum")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1); 
	}
	
	
	
	ret=GetSysdate();
	if (ret < 0)
	{
		db_disconnect(con);
		return -1;
	}

	
    return (0);

arg_error:
    ZzLOG(ERROR, "usage : %s MMDD TT DAY|TIME\n", argv[0]);
    ZzLOG(ERROR, "        MMDD TT : 처리일자 (0000 TT:시스템일자), DAY|TIME : 처리 코드\n");

    ZzPRT(ERROR, "usage : %s MMDD TT\n", argv[0]);
    ZzPRT(ERROR, "        MMDD TT : 처리일자 (0000 TT:시스템일자), DAY|TIME : 처리 코드\n");
    return -1;
    
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int TermProcess()
{
    // DB close
	db_disconnect(con);
	
    ZzLOG(ALWAY, "[daemschstats]*****************프로그램 종료*****************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  Signal(int nSignal)
{
    TermProcess();
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
	signal(SIGTERM, Signal);
	signal(SIGINT,  Signal);
	signal(SIGQUIT, Signal);
	signal(SIGKILL, Signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( InitProcess(argc, argv) == 0 ) 
	{
		/* 프로그램 메인루틴 */
		rc = MainProcess();
		/* 프로그램 종료루틴 */                    
		TermProcess();
	}
	return(0);
}                
/*****************************************************************************
*  End of file...
*****************************************************************************/
