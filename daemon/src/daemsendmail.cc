/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daemsendmail.cc
 *         기능 : 회원에게 메일 보내기
 *         설명 : 메일입력 회원정보 가져와 메일링 페이지 호출
 *       작성자 : HCS
 *       작성일 : 2010/03/24
 *     수정이력 : 
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
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

#include <pthread.h>        /* for POSIX threads */

#include "daemcom.h"

#define  MAX_ROWS	1
#define _DEBUG_
#define NUMBER		10
#define NEXT_LINE "\r\n"

int InitProcess(int argc, char **argv);
int MainProcess();
int TermProcess();
void Signal(int nSignal);

int Depth, is_value;

int nCloseCheck;
char szSvrIP[16+1];


struct recv_data_MemoryStruct {
	char *memory;
	size_t size;
};

struct send_data {
	char *readptr;
	int sizeleft;
};

int send_data_read_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
	struct send_data *pooh = (struct send_data *)userp;
	
	if(size*nmemb < 1)
	{
		return 0;
	}
	
	if(pooh->sizeleft) {
		*(char *)ptr = pooh->readptr[0]; /* copy one single byte */
		pooh->readptr++;                 /* advance pointer */
		pooh->sizeleft--;                /* less data left */
		return 1;                        /* we return 1 byte at a time! */
	}
	
	return -1;                         /* no more data left to deliver */
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

int GetHTTPFilePostAPI(char** ppResult, char* url, char* postdata)
{
	int nRet = 0;
	CURL *curl_handle;
	CURLcode res;
	struct curl_slist *headers = NULL;

	send_data send;
	recv_data_MemoryStruct recv;

	send.readptr = postdata;
	send.sizeleft = strlen(postdata);

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
		res = curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
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
		/*
		// Now specify we want to POST data 
		res = curl_easy_setopt(curl_handle, CURLOPT_POST, 1);    
		if(res != CURLE_OK) 
		{ 
			ZzLOG(ERROR, "GetHTTPFilePostAPI: CURLOPT_POST failed\n"); 
			nRet = 5; 
			break; 
		}
		
		// Set the expected POST size 
		res = curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, send.sizeleft);
		if(res != CURLE_OK) 
		{ 
			ZzLOG(ERROR, "GetHTTPFilePostAPI: CURLOPT_POSTFIELDSIZE failed\n"); 
			nRet = 6; 
			break; 
		}
		
		// we want to use our own read function 
		res = curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, send_data_read_callback);    
		if(res != CURLE_OK) 
		{ 
			ZzLOG(ERROR, "GetHTTPFilePostAPI: CURLOPT_READFUNCTION failed\n"); 
			nRet = 7; 
			break; 
		}
		
		// pointer to pass to our read function
		res = curl_easy_setopt(curl_handle, CURLOPT_READDATA, &send);
		if(res != CURLE_OK) 
		{ 
			ZzLOG(ERROR, "GetHTTPFilePostAPI: CURLOPT_READDATA failed\n"); 
			nRet = 8; 
			break; 
		}
		*/
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

int FileWriter(char* pPath, char* pBuffer)
{
	FILE* pFHandle = NULL;
	
	if(( pFHandle = fopen(pPath,"a+t") ) == NULL)
	{
		ZzLOG(ERROR, "FileWriter: %s, %s\n", pPath, pBuffer);
		return -1;
	}	
	fseek(pFHandle,0l,SEEK_END);
	fwrite(pBuffer,strlen(pBuffer),1,pFHandle);
	fwrite(NEXT_LINE,strlen(NEXT_LINE),1,pFHandle);
	fclose(pFHandle);
	
	return 0;
}
int SendMail(char* pUserId, char* pEmail)
{
	int nRet = -1;
	char* pReturnData = NULL;

	char szUrl[512];
	memset(szUrl, 0x00, sizeof(szUrl));

	char szPostData[512];
	memset(szPostData, 0x00, sizeof(szPostData));
	
	//http://www.wedisk.co.kr/mail/mail_send_proc.jsp?userid=xxx&email=xxxx

	sprintf(szUrl, "http://%s/mail/mail_send2_proc.jsp", szSvrIP);
	

	sprintf(szPostData, "?userid=%s&email=%s", pUserId, pEmail);
	
	strcat(szUrl, szPostData);
	
	
	if(GetHTTPFilePostAPI(&pReturnData, szUrl, szPostData) > 0)
		return nRet;
	
	if(strstr(pReturnData, "OK") != NULL)
		nRet = 0;
		
	
	if(pReturnData)
		delete[] pReturnData;
		
	usleep(100);
	
	return nRet;
}
int Processer(char* pFilePath, char* pSLogPath, char* pFLogPath)
{
	ZzLOG(ALWAY, "Processer: pFilePath=[%s]\n", pFilePath);
	ZzLOG(ALWAY, "Processer: pSLogPath=[%s]\n", pSLogPath);
	ZzLOG(ALWAY, "Processer: pFLogPath=[%s]\n", pFLogPath);
	FILE* pFHandle = NULL;
	
	if(( pFHandle = fopen(pFilePath,"rt") ) == NULL)
	{
		ZzLOG(ERROR, "SendMail: %s open fail\n", pFilePath);
		return -1;
	}
	
	char* pToken=NULL;
	char szStringBuffer[512];
	memset(szStringBuffer,0x00,sizeof(szStringBuffer));	
	
	char szUserId[18];
	memset(szUserId,0x00,sizeof(szUserId));	
	
	char szEmail[128];
	memset(szEmail,0x00,sizeof(szEmail));	
	
	char szMsg[128];
	memset(szMsg, 0x00, sizeof(szMsg));
		
	char szDate[8+1];
	memset(szDate, 0x00, sizeof(szDate));
		
	char szTime[6+1];
	memset(szTime, 0x00, sizeof(szTime));
	
	while(fgets(szStringBuffer,sizeof(szStringBuffer),pFHandle)!=NULL)
	{
		memset(szUserId,0x00,sizeof(szUserId));	
		memset(szEmail,0x00,sizeof(szEmail));	
		memset(szMsg, 0x00, sizeof(szMsg));
		memset(szDate, 0x00, sizeof(szDate));
		memset(szTime, 0x00, sizeof(szTime));

		pToken = strtok(szStringBuffer," ");
		
		if(pToken)
			strcpy( szUserId ,pToken);
		
		pToken = strtok( NULL , NEXT_LINE);

		if(pToken)
			strcpy( szEmail ,pToken);


		if(strlen(szUserId) > 0 && strlen(szEmail) > 0)
		{
			struct  timeval stNow;
			struct  timezone stZone;
			struct  tm  stCtm;
		
			gettimeofday (&stNow, &stZone);
			localtime_r (&stNow.tv_sec, &stCtm);

			sprintf(szDate, "%04d%02d%02d", stCtm.tm_year + 1900, stCtm.tm_mon + 1, stCtm.tm_mday);

			sprintf(szTime, "%02d%02d%02d",  stCtm.tm_hour, stCtm.tm_min, stCtm.tm_sec);
		
			
			int nRet = SendMail(szUserId, szEmail);
			if(nRet == 0)
			{
				sprintf(szMsg, "%s, %s, %s, %s", szUserId, szEmail, szDate, szTime);	
				if(FileWriter(pSLogPath, szMsg) < 0)
					return -1;
			}
			else
			{
				sprintf(szMsg, "%s, %s, %s, %s", szUserId, szEmail, szDate, szTime);	
				if(FileWriter(pFLogPath, szMsg) < 0)
					return -1;
			}	
		}
	}
	fclose(pFHandle);
	
	return 0;
}
void *ThreadMain(void * nNum)
{
	char szInfoFilePath[512];
	memset(szInfoFilePath, 0x00, sizeof(szInfoFilePath));
	
	sprintf(szInfoFilePath, "%s/zangsi_with_dcmd/infofile/%d_info.txt", getenv("HOME"), (int) nNum);
	
	char szSLogPath[512];
	memset(szSLogPath, 0x00, sizeof(szSLogPath));
	
	sprintf(szSLogPath, "%s/zangsi_with_dcmd/logfile/%d_success.log", getenv("HOME"), (int) nNum);
	
	char szFLogPath[512];
	memset(szFLogPath, 0x00, sizeof(szFLogPath));
	
	sprintf(szFLogPath, "%s/zangsi_with_dcmd/logfile/%d_fail.log", getenv("HOME"), (int) nNum);
	
	

	Processer(szInfoFilePath, szSLogPath, szFLogPath);
	nCloseCheck--;
	ZzLOG(ALWAY, "ThreadMain: %d 쓰레드 종료. nCloseCheck=[%d]\n", (int) nNum, nCloseCheck);
	
	return NULL;
}
//******************************************************************************
//* daemsendmail main
//******************************************************************************
int MainProcess()
{
    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "MainProcess START !! \n");  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");
	
    pthread_t threadID;
    
	for(int i=1;i<=nCloseCheck;i++)
	{
	    if (pthread_create(&threadID, NULL, ThreadMain, (void *)i) != 0)
		{
			ZzLOG(ERROR, "MainProcess: %d pthread failed\n", i);
		}
	}
	
	while(1)
	{
		if(nCloseCheck <= 0)
			break;
			
		sleep(10);
	}
	

	
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
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("daemsendmail", "/home/ezwon/zangsi_with_dcmd/daemon/log"); 

	if(argc < 3 || argc > 4)
		goto arg_error;

	nCloseCheck = 0;	
	
	sprintf(szSvrIP, "%s", argv[1]);

	nCloseCheck = atoi(argv[2]);
	
	

    ZzLOG(ALWAY, "[daemsendmail]*****************프로그램 시작[%d]*****************\n", nCloseCheck);  
    
	
    return (0);

arg_error:
    ZzLOG(ERROR, "usage : %s [IP or DOMAIN] [쓰레드 개수]\n", argv[0]);
    printf("ERROR!! usage : %s [IP or DOMAIN] [쓰레드 개수]\n", argv[0]);
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
	
    ZzLOG(ALWAY, "[daemsendmail]*****************프로그램 종료*****************\n\n");

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
