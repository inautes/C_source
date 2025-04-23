#include "CLog64.h"
#include <stdarg.h>

CLog64::CLog64()
{
	memset(m_szLogPath,0x00,sizeof(m_szLogPath));
	memset(m_szLogName,0x00,sizeof(m_szLogName));
	memset(m_szFullLogPath,0x00,sizeof(m_szFullLogPath));
}

CLog64::CLog64(char* pLogPath,char* pLogFileName)
{
	memset(m_szLogPath,0x00,sizeof(m_szLogPath));
	memset(m_szLogName,0x00,sizeof(m_szLogName));
	memset(m_szFullLogPath,0x00,sizeof(m_szFullLogPath));
	initLog(pLogPath,pLogFileName);
}


CLog64::~CLog64()
{

}

int CLog64::initLog(char* pLogPath,char* pLogName)
{
	if( pLogPath != NULL && pLogName != NULL)
	{
		strcpy(m_szLogPath,pLogPath);
		strcpy(m_szLogName,pLogName);
		strcpy(m_szFullLogPath,m_szLogPath);
		strcat(m_szFullLogPath,"/");
		strcat(m_szFullLogPath,pLogName);
		
		setLog("create log file to [ %s ] \n",m_szFullLogPath);
		
		return 1;
	}
	return -1;
		
}

int CLog64::setLog( char *pformat, ...)
{
	char    szBuffer[4048];
	char	szCurTime[20];
	
	va_list args;
	FILE 	*fp = NULL;

	memset(szBuffer, 0x00, sizeof(szBuffer));
	va_start(args, pformat);
	vsprintf(szBuffer, pformat, args);
	va_end(args);

	
	memset(szCurTime,0x00,sizeof(szCurTime));
	

	// 시간 정보 가져 오기
	struct  timeval stNow;
	struct  timezone stZone;
	struct  tm  stCtm;

	gettimeofday (&stNow, &stZone);
	localtime_r (&stNow.tv_sec, &stCtm);
	sprintf(szCurTime, "%02d:%02d:%02d.%03d",  stCtm.tm_hour,
						stCtm.tm_min,
						stCtm.tm_sec,
						stNow.tv_usec/1000);
	


	//파일 이름 결정
	
	time_t	ct;
	struct tm  *stm;
	
	time( &ct ) ;
	stm = localtime ( &ct ) ;

		
	char szFullLogPath[4100];
	memset(szFullLogPath,0x00,sizeof(szFullLogPath));
	char	szCurDate[30];
	memset(szCurDate,0x00,sizeof(szCurDate));
	char	szLogBase[256];	
	memset(szLogBase,0x00,sizeof(szLogBase));
	    
	sprintf(szCurDate, "%04d%02d%02d", stm->tm_year + 1900
					 , stm->tm_mon  + 1
					 , stm->tm_mday );



	sprintf(szFullLogPath, "%s_%s.log", m_szFullLogPath, szCurDate);

	if ( strlen(m_szFullLogPath) == 0 ) 
		return FALSE;

	if ( (fp = fopen64(szFullLogPath, "a+")) != NULL )
	{
		fprintf(fp, "[%s`%d']%s", szCurTime, (int)getpid(),  szBuffer);
		fclose(fp);
		return 1;
	}

	return -1;
}




	

