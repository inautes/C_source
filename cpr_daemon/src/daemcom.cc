/*****************************************************************************
*    파   일   명 : ZzCommon.pc                                       
*    프로그램  명 : 공통함수
*    작   성   일 : 2002.11.09                                        
*    작   성   자 : JDP
*    프로그램설명 : 공통함수
*=============================================================================
*    <수정내역>    
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h> 
#include <errno.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <string.h>
#include <stdarg.h>
//#include <varargs.h>

#include "daemcom.h"

#define	_PRINT_
SUserParm gsUserParm;
/*****************************************************************************
#include <sqlca.h>
*  에러 데이터 파일 쓰기
* (I) grg(I) 1. char *pszFileName : 원본 파일명
*            2. char *pszData : WriteData 
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int	ZzErrorDataWrite(char *pszFileName, char *pszData)
{
	FILE	*fp;
	char	szFileName[2046];

	memset(szFileName, 0x00, sizeof(szFileName));
	sprintf(szFileName, "%s.log", pszFileName);

	
	/*
	** 파일 OPEN
	*/
	if ( (fp=fopen(szFileName, "a+")) == NULL ) {
		ZzLOG(ERROR, "File(%s) Open Fail\n", szFileName);
		return (-1);
	}

	fprintf(fp, "%s\n", pszData);

	/*
	** 파일 포인트 닫기
	*/
	fclose(fp);
	return (0);
}
/*****************************************************************************
 * 문자열 데이타가 수치형 문자열 데이타 인지 체크
 * arg(I) : 1. char datastrs[] : 문자열 데이타
 *          2. int datasize : 문자열 길이
 *          3. int min_size : 비교 문자열 길이
 * return : 1. '0' : 거짓
 *          2. '1' : 참
 ****************************************************************************/
int     ZzValidString(char *datastrs, int datasize, int min_size)
{
	int     numbxxxx = ZzGetStringCount(datastrs, datasize);
	if (numbxxxx < min_size) {
		return(0);
	}
	return(1);  /* 정상 */
}


/*****************************************************************************
 * 문자열에서 0x20(space)보다 큰 값을 체크
 * arg(I) : 1. char *pstr : 문자열
 *          2. int size : 문자열 길이
 * return : 1. space 보다 큰 값의 count
 ****************************************************************************/
int     ZzGetStringCount(char *pstr, int size)
{
	int    numb = 0;
	while( size-- > 0 ) {
		if ( *((unsigned char*)pstr) > 0x20 ) {
			numb++;
		}
		pstr++;
	}
	return(numb);
}


/*****************************************************************************     
* 숫자(int/long/double) 값을 컴마 문자열로 변환
* double값은 소수점 제외
* (O) char *pstr : 변환된 값
* (I) long value : long type의 숫자(int값 포함)
* (R) char *pstr : 변환된 값
*****************************************************************************/     
char  ZzGetCommaStr(char *pstr, double value)                                               
{                                 
	char	szValue[256];
	char	szComma[256];
	int	i, size;
	char	ch;
	int	nPos = 0;
	int	bSet = 0;
	int	nChk = 0;
	
	/*
	** max : 1234567890123456 => 12,345,678,901,123,568
	*/
	
	memset(szValue, 0x00, sizeof(szValue));
	memset(szComma, 0x00, sizeof(szComma));
	size = sprintf(szValue, "%.0f", value);

	for ( i=(size-1); i>=0; i-- ) {
		ch   = szValue[i];
		bSet = 0;

		if ( (nChk%3) == 0 && nPos ) { bSet = 1; }
		if ( i==0 ) {
			if ( szValue[0] == '-' ) { bSet = 0; }
		}
			
		if ( bSet ) {
			szComma[nPos] = ',';
			nPos++;
		}

		szComma[nPos] = ch;	
		nPos++;
		nChk++;
	}

	/* 컴마를 설정한 문자열을 정상적으로 	*/
	size = strlen(szComma);
	nPos = size;
	for ( i=0; i<size; i++ ) {
		nPos--;
		ch = szComma[nPos];
		pstr[i] = ch;
	}	
	pstr[i] = 0x00;

	return(*pstr);                                                                
}                                                                                  

/*****************************************************************************
 * 문자열을 long value로 변환
 * arg(I) : 1. char *pstr : 문자열
 *          2. int size : 문자열 길이
 * return : 1. long 문자열 변환값
 ****************************************************************************/
long     ZzStrToLong(char *pstr, int size)
{
        long    numb = 0L;
        char    ch;
        int     minus = 0;
        
        while ((size > 0) && (*pstr == 0x20)) 
        {
			pstr++;
			size--;
        }
        if ((size > 0) && (*pstr == '-')) 
        {
			pstr++;
			size--;
			minus = 1;
        }
        while ((size > 0) && (*pstr == '0')) 
        {
			pstr++;
			size--;
        }
        while (size-- > 0 ) 
        {
			ch = *pstr++;
			if ((ch < '0') || (ch > '9')) 
			{
				break;
			}
			numb = (numb * 10) + (ch - '0');
        }
        return((minus) ? -numb : numb);
}


/*****************************************************************************
* 마지막 문자열이 한글인 경우에 정상적인 글씨가 아닌경우는 NULL로 설정
* (I) 1. char *pStr : 문자열
* (R) 1. int '0' 정상 / '-1' : errno 
******************************************************************************/
int  ZzBrokenHangulChangeNull(char *pStr)
{
	int     nEnd = strlen(pStr) - 1;
	int     nHan = 0;
	int     i;

	if ( !(*(pStr+nEnd) & 0x80) ) return(0);

	for ( i=nEnd; i>-1; i-- )
	{
		char    ch = *(pStr+i);
		if ( ch & 0x80 )
		{
			nHan++;
		}
		else
		{
			if ( nHan % 2 )
			{
				*(pStr+nEnd) = 0x00;
				return (0);
			}
			else
			{
				return (-1);
			}
        	}
    	}
	return (-1);
}

/*****************************************************************************
* 문자열 데이터 내에서 특정문자열을 대체문자열로 바꾼다.
* (I) 1. char *search : 특정문자열
*     2. char *change : 대체문자열
* (B) 1. char *source : 문자열 데이터
* (E) 변환전 : temp = "123456ABCD";
*     ZzStrReplace(temp, "ABCD", "EFGH");
*     변환후 : temp = "123456EFGH";
*****************************************************************************/
void  ZzStrReplace(char *source, char *search, char *change)
{
	char    *ptr;
	int     len = strlen(source);
	int     pos = 0;

	if ( (ptr = (char*)strstr(source, search)) != NULL )
	{
		pos = len - strlen(ptr);
		memcpy(&source[pos], change, strlen(change));
	}
}

/*****************************************************************************
* 문자열 전체를 소문자로 변환 (알파벳)
* (B) 1. char *pstr : 변환 전 문자열 포인트 및 변환 후 문자열 포인트
****************************************************************************/
void    ZzStrToLower(char *pstr)
{
	while (*pstr != 0x00) {
		if ((*pstr >= 'A') && (*pstr <= 'Z')) {
			*pstr = ('a' + (*pstr - 'A'));
		}
		pstr++;
	}
}
/*****************************************************************************
* 문자열 전체를 대문자로 변환 (알파벳)
* (B) 1. char *pstr : 변환 전 문자열 포인트 및 변환 후 문자열 포인트
****************************************************************************/
void    ZzStrToUpper(char *pstr)
{
	while (*pstr != 0x00) {
		if ((*pstr >= 'a') && (*pstr <= 'z')) {
			*pstr = ('A' + (*pstr - 'a'));
		}
		pstr++;
	}
}

/*****************************************************************************
* 문자열 포인트의 특정문자 카운트 수를 체크
* (I) 1. char *pStr : 문자열 포인트
*     2. char chTarget : 특정문자
* (O) 1. char *pStr : NULL 설정 문자열 포인트  
* (R) 1. int : pStr 길이
*****************************************************************************/
int		ZzCharCount(char *pStr, char chTarget)
{
	int		i;	
	int		nCount;

	for ( i=0, nCount=0; i<strlen(pStr); i++ )
	{
    	char ch = pStr[i];
        if ( ch == chTarget )
        {
			nCount++;
		}
	}
	return nCount;
}


/*****************************************************************************
* 문자열에 SPACE로 초기화한 다음 문자열을 복사한다.
* (I) 1. int size       : SPACE로 초기화 길이
*     2. char *pdatastr : 복사 대상
* (O) 1. char *memstr   : 초기화 및 복사 문자열
*****************************************************************************/
int     ZzStrSpaceSetAndCopy(char *pmemstr, int size, char *pdatastr)
{
	int length = strlen(pdatastr);

	if ( size < length ) {
		length = size;
	}

	memset(pmemstr, 0x20, size);
	memcpy(pmemstr, pdatastr, length);
	return size;
}

/*****************************************************************************
* 문자열을 포멧형태로 변환
* (I) 1. char *pdatastr : 문자열 데이터
*     2. char *pformstr : 포멧
* (O) 1. char *pwantstr : 변환 문자열
* (E) 
* char szWantStr[20];
* ZzStrChangeFormData(szWantStr, "19991231", "XXXX-XX-XX");
* 결과 szWantStr은 "1999-12-31"로 셋팅됨
*****************************************************************************/
void	ZzStrChangeFormData(char *pwantstr, char *pdatastr, char *pformstr)
{
	char    ch;

	while ( (ch=*pformstr) != 0x00 ) {
		if ( ch == 'X' ) {
			*pwantstr++ = *pdatastr++;
		} else {
			*pwantstr++ = ch;
		}
		pformstr++;
	}

	*pwantstr = 0x00;
}


/*****************************************************************************
* 문자열 데이타에서 cDest문자를 cChng문자로 변환
* (B) 1. *pstr : 문자열 데이타 
* (I) 1. int size : 문자열 데이타 
*     2. char cDest : 비교문자
*     3. char cChng : 대체문자
*****************************************************************************/
void	ZzCharChange(char *pstr, int size, char cDest, char cChng)
{
	while ( size-- > 0 ) {
		if ( *pstr == cDest ) {
			*pstr = cChng;
		}
		pstr++;
	}
}
/**************************************************************************
* 시간을 체크하여 간격을 리턴
* (I) 1.char *szTime1 : 시간 ("YYYYMMDDHHMISS")
*     2.char *szTime2 : 시간 ("YYYYMMDDHHMISS")
* (O) void
* (R) long	: 시간1과 시간2의 간격(초)
**************************************************************************/
int     ZzGetDiffTime(char *szTime1, char *szTime2)
{
	char            szTemp[50];
	time_t          time1;
	time_t          time2;
	struct tm       *stm;
	time_t  ct;

	/* 반드시 데이터 길이가 14이상이어야 함	*/
	if ( strlen(szTime1) != 14 ) return 0;
	if ( strlen(szTime2) != 14 ) return 0;

	time( &ct ) ;
	stm = localtime ( &ct ) ;

	/* 문자열을 시간으로 변환	*/
	NULLSET(szTemp); sprintf(szTemp, "%-4.4s", (char*)&szTime1[0] ); stm->tm_year = atoi(szTemp) - 1900;
	NULLSET(szTemp); sprintf(szTemp, "%-2.2s", (char*)&szTime1[4] ); stm->tm_mon  = atoi(szTemp) - 1;
	NULLSET(szTemp); sprintf(szTemp, "%-2.2s", (char*)&szTime1[6] ); stm->tm_mday = atoi(szTemp);
	NULLSET(szTemp); sprintf(szTemp, "%-2.2s", (char*)&szTime1[8] ); stm->tm_hour = atoi(szTemp);
	NULLSET(szTemp); sprintf(szTemp, "%-2.2s", (char*)&szTime1[10]); stm->tm_min  = atoi(szTemp);
	NULLSET(szTemp); sprintf(szTemp, "%-2.2s", (char*)&szTime1[10]); stm->tm_min  = atoi(szTemp);
	NULLSET(szTemp); sprintf(szTemp, "%-2.2s", (char*)&szTime1[12]); stm->tm_sec  = atoi(szTemp);
	time1 = mktime( stm );

	NULLSET(szTemp); sprintf(szTemp, "%-4.4s", (char*)&szTime2[0] ); stm->tm_year = atoi(szTemp) - 1900;
	NULLSET(szTemp); sprintf(szTemp, "%-2.2s", (char*)&szTime2[4] ); stm->tm_mon  = atoi(szTemp) - 1;
	NULLSET(szTemp); sprintf(szTemp, "%-2.2s", (char*)&szTime2[6] ); stm->tm_mday = atoi(szTemp);
	NULLSET(szTemp); sprintf(szTemp, "%-2.2s", (char*)&szTime2[8] ); stm->tm_hour = atoi(szTemp);
	NULLSET(szTemp); sprintf(szTemp, "%-2.2s", (char*)&szTime2[10]); stm->tm_min  = atoi(szTemp);
	NULLSET(szTemp); sprintf(szTemp, "%-2.2s", (char*)&szTime2[12]); stm->tm_sec  = atoi(szTemp);
	time2 = mktime( stm );

	return (int)difftime(time1, time2);
}

/*******************************************************************************
* 비교시간과 현재시간의 간격을 체크
* (I) 1. char *szCmpTime : 비교시간(YYYYMMDDHHMISS)
* (O)
* (R) long   : 현재 시간과 비교한 간격(초)
*******************************************************************************/
int     ZzCurDiffTime(char *szCmpTime)
{
	char            szTemp[20];
	time_t          curtime;
	struct tm       *stm;

	/* 현재의 시간을 구한다	*/
	time( &curtime );
	stm = (struct tm *) localtime(&curtime);

	/* 문자열을 시간으로 변환	*/
	NULLSET(szTemp); sprintf(szTemp, "%-4.4s", (char*)&szCmpTime[0] ); stm->tm_year = atoi(szTemp) - 1900;
	NULLSET(szTemp); sprintf(szTemp, "%-2.2s", (char*)&szCmpTime[4] ); stm->tm_mon  = atoi(szTemp) - 1;
	NULLSET(szTemp); sprintf(szTemp, "%-2.2s", (char*)&szCmpTime[6] ); stm->tm_mday = atoi(szTemp);
	NULLSET(szTemp); sprintf(szTemp, "%-2.2s", (char*)&szCmpTime[8] ); stm->tm_hour = atoi(szTemp);
	NULLSET(szTemp); sprintf(szTemp, "%-2.2s", (char*)&szCmpTime[10]); stm->tm_min  = atoi(szTemp);
	NULLSET(szTemp); sprintf(szTemp, "%-2.2s", (char*)&szCmpTime[12]); stm->tm_sec  = atoi(szTemp);
	curtime = mktime( stm );
	return (int)difftime(time(NULL), curtime);
}

/*****************************************************************************
* 현재시간과 비교할 시간의 간격을 체크(미만)
* (I) 1. time_t comptime : 비교할 시간
*     2. int timesecs : 비교시간 간격
* (R) 1. '0' : 비교시간 간격보다 초과 (거짓)
*     2. '1' : 비교시간 간격보다 이하이면 (참)
*****************************************************************************/
int    ZzIsRemainTimesecs(time_t comptime, int timesecs)
{
	if (difftime(time(NULL), comptime) <= (double)timesecs) {
		return(1);
	}
	return(0);
}

/*****************************************************************************
* DATE FORMAT CHECK
* (I) 1. char *pszStr : 날자 문자열
* (O)
* (R) 1: True;
*     0: False;
******************************************************************************/
int	ZzDateCheck(char *pdate)
{
	static int mm_days[12] = {31,0,31,30,31,30,31,31,30,31,30,31};
	int nYear;
	int nMonth;
	int nDay;
	char szYear[5];
	char szMonth[3];
	char szDay[3];

	memset(szYear, '\0',5);
	memset(szMonth,'\0',3);
	memset(szDay,  '\0',3);

	strncpy(szYear,  pdate  ,4);
	strncpy(szMonth, pdate+4,2);
	strncpy(szDay,   pdate+6,2);
	nYear  = atoi(szYear);
	nMonth = atoi(szMonth);
	nDay   = atoi(szDay);

	if  ((nYear < 1)||(nYear > 9999)||
	     (nMonth < 1)||(nMonth > 12) ) {
		return 0;
	}
	if  (nYear%4==0 && nYear%100 != 0  || nYear%400==0) {
		mm_days[1] = 29;
	} else {
		mm_days[1] = 28;
	}
	if  ((nDay < 1)||(nDay > mm_days[nMonth-1])) {
		return 0;
	}
	return 1;
}

/*****************************************************************************
* 시스템시간을 포멧형태로 얻음
*****************************************************************************/
int	ZzGetSysTime(char *pdatastr, char *pformstr)
{
	char	*pdatasav = pdatastr;
	time_t      ct;
	struct tm  *stm;
	long		value;
	int			size, loop, real;

	time( &ct ) ;
	stm = localtime ( &ct ) ;

	while (pformstr != NULL) {
		if    (memcmp(pformstr, "YYYY", 4) == 0) { value = stm->tm_year+1900;}
		else if (memcmp(pformstr, "YY", 2) == 0) { value = stm->tm_year;	}
		else if (memcmp(pformstr, "MM", 2) == 0) { value = stm->tm_mon + 1;	}
		else if (memcmp(pformstr, "DD", 2) == 0) { value = stm->tm_mday;	}
		else if (memcmp(pformstr, "HH", 2) == 0) { value = stm->tm_hour;	}
		else if (memcmp(pformstr, "MI", 2) == 0) { value = stm->tm_min;		}
		else if (memcmp(pformstr, "SS", 2) == 0) { value = stm->tm_sec;		}
		else {
			*pdatastr++ = *pformstr++;
			continue;
		}
		size = 2;
		if (memcmp(pformstr, "YYYY", 4) == 0) size = 4;

		memset(pdatastr, '0', size);

		real = 0;
		loop = size;
		pdatastr += (size - 1);
		while( loop-- > 0 ) {
			*pdatastr = (char)((value % 10) + '0');
			value /= 10;
			real++;
			if ( value <= 0 ) break;
			pdatastr--;
		}
		pdatastr += real;
		pformstr += size;
	}
	pdatastr = NULL;

	return(pdatastr - pdatasav);
}

/*****************************************************************************
* 시스템일자을 얻음 (YYYY/MM/DD)
* (I) 
* (O)
* (R) 1. char *pszStr : 문자열
******************************************************************************/
char *ZzGetCurDate(char *pszStr)
{
	struct  timeval stNow;
	struct  timezone stZone;
	struct  tm  stCtm;

	gettimeofday (&stNow, &stZone);
	localtime_r (&stNow.tv_sec, &stCtm);
	sprintf(pszStr, "%04d/%02d/%02d", stCtm.tm_year + 1900,
					  stCtm.tm_mon + 1,
					  stCtm.tm_mday);

	return(pszStr);
}
/*****************************************************************************
* 시스템시간을  얻음 (HH:MI:SS:SEC)
* (I) 
* (O)
* (R) 1. char *pszStr : 문자열
******************************************************************************/
char	*ZzGetCurTime(char *pszStr)
{
	struct  timeval stNow;
	struct  timezone stZone;
	struct  tm  stCtm;

	gettimeofday (&stNow, &stZone);
	localtime_r (&stNow.tv_sec, &stCtm);
	sprintf(pszStr, "%02d:%02d:%02d.%03d",  stCtm.tm_hour,
						stCtm.tm_min,
						stCtm.tm_sec,
						stNow.tv_usec/1000);

	return(pszStr);
}

/*****************************************************************************
* 날짜와 날짜사이의 일자수를 구한다.
* (I) char *szStartymd : 시작일자 
*     char *szEndymd   : 종료일자    
* (R) int : 날짜사이의 일자수
*****************************************************************************/
int ZzCalcBeginOfEnd(char *szStartymd, char *szEndymd)
{
    int dStart;
    int dEnd ;

    dStart = ZzCalcSolarDay(szStartymd, 1);
    dEnd   = ZzCalcSolarDay(szEndymd, 1);

    if( (dStart == 0) || (dEnd == 0) ) 
    {
        return (0);
    } else
    {   
        return( dEnd - dStart + 1); 
    }

}
int ZzCalcSolarDay(char *szStartymd, int iFlag)
{
	static int mm_days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

	int nYear;
	int nMonth;
	int nDay;
	int iCnt;
	int days;
	char szYear[5];
	char szMonth[3];
	char szDay[3];
    
    memset(szYear , 0x00 , sizeof(szYear));
    memset(szMonth , 0x00 , sizeof(szMonth));
    memset(szDay , 0x00 , sizeof(szDay));
    
	strncpy(szYear,  szStartymd  ,4);
	strncpy(szMonth, szStartymd+4,2);
	strncpy(szDay,   szStartymd+6,2);

	nYear  = atoi(szYear);
	nMonth = atoi(szMonth);
	nDay   = atoi(szDay);

	days = 0;

	if  ((nYear < 1)||(nYear > 9999)|| (nMonth < 1)||(nMonth > 12) ) 
	{
		return 0;
	}
	
	if  (nYear%4==0 && nYear%100 != 0  || nYear%400==0) 
	{
		mm_days[1] = 29;
	} else 
	{
		mm_days[1] = 28;
	}

	if  ((nDay < 1)||(nDay > mm_days[nMonth-1])) {
		return 0;
	}

   if (iFlag == 1)
      days = ( (nYear - 1) * 365 )  + ( (nYear - 1) / 4 ) - ( (nYear - 1) / 100 ) 
                                    + ( (nYear - 1) / 400 );

   for (iCnt = 1; iCnt<= (nMonth - 1); ++iCnt)
      days  = days + mm_days[iCnt-1];

    return ( days + nDay  );

}


int ZzSolarDateCount(char *psdate, int iCount)
{
	static int mm_days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

	int nYear;
	int nMonth;
	int nDay;
	int iCnt;
    int nYmd;
	int tmp_day;
	int tmp_yy;

    iCnt 	= 1;
    tmp_day = iCount;
    tmp_yy  = iCount / 365;
 	
    for(nYear=2; nYear <= tmp_yy ; ++nYear)
    {
    
     	if  (nYear % 400 == 0 || nYear % 100 != 0 && nYear % 4 == 0)
    	{
    		tmp_day = tmp_day - 366;
    	    if (tmp_day <= 366)
    	    {
    	    	tmp_day = tmp_day + 1;
    	    	break;
    	    }
    	
    	} else
    	{
    		tmp_day = tmp_day - 365;
	        if (tmp_day <= 365) 
	        {
	        	break;
    	    }
    	} 
    
    }

   	if  (nYear % 400 == 0 || nYear % 100 != 0 && nYear % 4 == 0)
	{
		mm_days[1] = 29;
	} else 
	{
		mm_days[1] = 28;
	}
	
	for(iCnt=0; iCnt <12; ++iCnt)
	{
		if (tmp_day > mm_days[iCnt])
		{
			tmp_day = tmp_day - mm_days[iCnt];
		} else 
		{
			break;
		}
	}
    
    nMonth = iCnt+1;
    
    if (tmp_day == 0)
    {
        nDay = 1;
    } else
    { 
    	nDay = tmp_day; 
    }

    nYmd = (nYear * 10000 + nMonth * 100 + nDay);
    sprintf(psdate, "%d", nYmd);

	return 1;
}


/*****************************************************************************
* 입력일자에 대한 일수 후의 날짜를 구한다.
* (O) char *psdate : 계산된 날짜값
* (I) char *sDate  : 입력날짜
*     int  iDay    : 일수
* (R) 
*****************************************************************************/
int ZzInDayOfDateCalc(char *psdate, char *sDate, int iDay)
{
   int iDateCnt;

   iDateCnt = ZzCalcSolarDay(sDate, 1);
   iDateCnt = iDateCnt + iDay;
   ZzSolarDateCount(psdate,  iDateCnt);
   
   return 1;
}

/*****************************************************************************
* 날짜에 대한 요일을 구한다
* (I) char *sDate : 입력 날짜값
* (R) int  : 요일
*          0 : 일요일
*          1 : 월요일
*          2 : 화요일
*          3 : 수요일
*          4 : 목요일
*          5 : 금요일
*          6 : 토요일
*****************************************************************************/
int ZzInDayOfWeek(char *sDate)
{
	int  iWeek;
    
   	iWeek = ZzCalcSolarDay(sDate, 1)%7;
    return  iWeek; 
}
/*****************************************************************************
* 시스템 환경으로부터 값을 없음
* (O) char *pszValue : 데이터 값
* (I) char *pstr : 환경에 정의된 매크로
* (R) int : 데이터 길이
*****************************************************************************/
int     ZzGetEnvironmentValue(char *pszValue, char *pstr)
{
	char *pData = NULL;

	pData = getenv(pstr);

	if ( pData != NULL ) {
		return sprintf(pszValue, "%s", pData);
	}
	return 0;
}



/*****************************************************************************
* 전역변수 초기화
* (I) void
* (R) void
*****************************************************************************/
void ZzInitGlobalVariable(char *sSvrname)
{
	/*
	** 전역변수 초기화 (NULL)
	*/
	memset(&gsUserParm, 0x00, sizeof(SUserParm));

	/*
	** 전역변수 초기값 설정   
	*/
	sprintf(gsUserParm.szProcName, "%s", sSvrname);    	        /* 프로세스 명칭 */
	sprintf(gsUserParm.szFilePath, "%s/", getenv("LOG_FILE_PATH"));	/* 디렉토리 */
	sprintf(gsUserParm.szLogFBase, "%s/", getenv("LOG_FILE_PATH"));	/* 로그파일 디렉토리 */


}
/*****************************************************************************
* 전역변수 초기화
* (I) void
* (R) void
*****************************************************************************/
void ZzInitGlobalVariable2(char *sSvrname, char *sFilePath)
{
        /*
        ** 전역변수 초기화 (NULL)
        */
        memset(&gsUserParm, 0x00, sizeof(SUserParm));

        /*
        ** 전역변수 초기값 설정
        */
        sprintf(gsUserParm.szProcName, "%s", sSvrname);   /* 프로세스 명칭 */
        sprintf(gsUserParm.szFilePath, "%s/", sFilePath);  /* 디렉토리 */
        sprintf(gsUserParm.szLogFBase, "%s/", sFilePath);  /* 로그파일 디렉토리 */

}
/*****************************************************************************
* 로그 파일명을 얻음
* (I) void
* (R) char * : 로그 파일명
*****************************************************************************/
void	ZzGetLogFileName(char *szFilePath, char *szProcName, char *szLogFile, char *szErrFile)
{
	char	szCurDate[30];
	char	szLogBase[256];	
	time_t	ct;
	struct tm  *stm;

	time( &ct ) ;
	stm = localtime ( &ct ) ;

	NULLSET(szCurDate);
	NULLSET(szLogBase);
	    
	sprintf(szCurDate, "%04d%02d%02d", stm->tm_year + 1900
					 , stm->tm_mon  + 1
					 , stm->tm_mday );


	sprintf(szLogBase, "%s%s", szFilePath, szProcName);

	sprintf(szLogFile, "%s_%s.log", szLogBase, szCurDate);
	sprintf(szErrFile, "%s_%s.err", szLogBase, szCurDate);
}

/*****************************************************************************
* Trace Log Write       : ZzLOG(int nLevel, char* pformat, ...)
* 화일로 로그처리
* (I) 1. int nLevel     : Log Level
*     2. char * pformat : Input Data Format
*     3. ...            : Input Data
* (R) 1. 정상 : '0'
*	  2. 오류 : 음수 
*****************************************************************************/
int	ZzLOG(int nLevel, char *pformat, ...)
{
	char    szBuffer[4048];
	char    szLogFile[126];
	char    szErrFile[126];
	char	szCurTime[20];
	char	*pLevel = NULL;
	va_list args;
	FILE 	*fp = NULL;

	memset(szBuffer, 0x00, sizeof(szBuffer));
	va_start(args, pformat);
	vsprintf(szBuffer, pformat, args);
	va_end(args);

	switch ( nLevel )
	{
	case TRACE : pLevel = "TRACE"; break; /* 일반 사항인 경우 */
	case CHECK : pLevel = "CHECK"; break; /* 체크 사항인 경우 */
	case ERROR : pLevel = "ERROR"; break; /* 에러 사항인 경우 */
	case ALWAY : pLevel = "ALWAY"; break; /* 항상             */
	}

	NULLSET( szLogFile );
	NULLSET( szErrFile );
	NULLSET( szCurTime );

	ZzGetCurTime(szCurTime);
	ZzGetLogFileName(gsUserParm.szLogFBase, gsUserParm.szProcName, szLogFile, szErrFile);

	if ( strlen(szLogFile) == 0 ) return FALSE;

	if ( (fp = fopen(szLogFile, "a+")) != NULL )
	{
		fprintf(fp, "[%s`%d'%s]%s", szCurTime, (int)getpid(), pLevel, szBuffer);
		fclose(fp);
	}


	/******************************
	** 필요시
	if ( nLevel == ERROR && (fp = fopen(szErrFile, "a+")) != NULL )
	{
		fprintf(fp, "%s`%d`%s`%s", szCurTime, (int)getpid(), pLevel, szBuffer);
		fclossse(fp);
	}
	******************************/
        
	return 1;
}


/*****************************************************************************
* Trace Log Write       : ZzPRT(int nLevel, char* pformat, ...)
* 화일로 로그처리
* (I) 1. int nLevel     : Log Level
*     2. char * pformat : Input Data Format
*     3. ...            : Input Data
* (R) 1. 정상 : '0'
*	  2. 오류 : 음수 
*****************************************************************************/
int	ZzPRT(int nLevel, char *pformat, ...)
{
	char    szBuffer[4048];
	char	szCurTime[20];
	char	*pLevel = NULL;
	va_list args;

#ifdef	_PRINT_
	memset(szBuffer, 0x00, sizeof(szBuffer));
	va_start(args, pformat);
	vsprintf(szBuffer, pformat, args);
	va_end(args);

	switch ( nLevel )
	{
	case TRACE : pLevel = "TRACE"; break; /* 일반 사항인 경우 */
	case CHECK : pLevel = "CHECK"; break; /* 체크 사항인 경우 */
	case ERROR : pLevel = "ERROR"; break; /* 에러 사항인 경우 */
	case ALWAY : pLevel = "ALWAY"; break; /* 항상             */
	}

	NULLSET( szCurTime );

	ZzGetCurTime(szCurTime);

	fprintf(stdout, "%s-%s_%d[%s]%s", szCurTime, gsUserParm.szProcName, (int)getpid(), pLevel, szBuffer);
#endif

	return 0;
}





/*****************************************************************************
* 문자열의 값을 LONG값으로 변환
* (I) 1. char *str : 문자열
*     2. int size : 변환하고자하는 문자열의 길이
* (O)
* (R) 1. long : 변환 LONG 값
******************************************************************************/
long    ZzGetAtoL(char *str, int size)
{
	char	szTemp[128];

	memset(szTemp, 0x00, sizeof(szTemp));
	memcpy(szTemp, str, size);

	return atol(szTemp);
}


/*****************************************************************************
* 문자열에서 지정된 길이 만큼을 정수화 하여 반환한다
* (I) 1. char *str : 문자열
*     2. int size : 변환하고자하는 문자열의 길이
* (O)
* (R) 1. int : 변환 Integer 값
******************************************************************************/
int  Zzantoi(char *str, int size)
{
	char	szTemp[128];

	memset(szTemp, 0x00, sizeof(szTemp));
	memcpy(szTemp, str, size);

	return atoi(szTemp);
}

/*****************************************************************************
* 문자열에서 지정된 길이 만큼을 정수화 하여 전달인수에 갑을 MOVE한다
* (I) 1. int  *ip  : 반환될 정수값
*     2. char *str : 문자열
*     3. int  size : 변환하고자하는 문자열의 길이
* (O)
* (R) 1. int : 변환 Integer 값
******************************************************************************/
int Zzantoip(int *ip, char *str, int size)
{
	char	szTemp[128];

	memset(szTemp, 0x00, sizeof(szTemp));
	memcpy(szTemp, str, size);

	*ip = atoi(szTemp);

	return  atoi(szTemp);
}


/*****************************************************************************
* 지정된 부동소수점을 문자열로 치완한다
* (I) 1. char *str : 문자열
*     2. double  d : 변환활 실수값 
*     3. int  size : 변환하고자하는 문자열의 길이
*     4. int   idp : 소숫점이하 길이
* (O)
* (R) 1. int :  Integer 
*
* ex) struct { char a[17];char b;   } s;      
*     double d;   
*     d=123456789.1234;
*     Zzdtoan( s.a, d, sizeof(s.a), 2);
*     s.a='00000012345678912'
*
*     double floor(double x) -> 소숫점이하버림
*     double ceil(double x)  -> 소숫점이하올림
*     double pow10(int p)    -> 10 p승
******************************************************************************/
int Zzdtoan(char *str, double  dvalue,  int size, int idp)
{
	double expnum;
	double conval;
 	
 	conval = 0.0;
 	expnum = 0.0;
	
 	/* 소숫점이하 자릿수 */
	expnum = pow(10 , idp);
	conval = dvalue * expnum;
    gcvt(floor(conval) , size , str);

    return 1;

}

/*****************************************************************************
* 지정된 문자열을 부동 소숫점으로 변환하여 리턴한다.
* (I) 1. char *str : 변환활문자열
*     2. int  size : 변환하고자하는 문자열의 길이
*     3. int   idp : 소숫점이하 길이
* (O) 1. double :  Double
* (R) 1. double :  Double
*
* ex) struct { char a[17]; a='1234567890123412'             
*			   char b   ; } 
*	  s;     
*	  double d;    
*	  d = Zzantodr( s.a, sizeof(s.a),2);
*	  d = 12345678901234.12
*
* 주의사항 : Return 값을 다시 확인할것 ( 2002.10.08)
*             
******************************************************************************/
double Zzantodr(char *str,  int size, int idp)
{
	char   *endptr;
	double expnum;
	double conval;
	
	expnum = 0.0;
 	conval = 0.0;
 	
 	/* 소숫점이하 자릿수 */
	/**/ 
	expnum = pow(10 , idp);
   
	conval = strtod(str,&endptr) /expnum;
    
	return (conval);
}
/*****************************************************************************
* 지정된 문자열을 부동 소숫점으로 변환한다
* (I) 1. char *str : 변환활문자열
*     2. int  size : 변환하고자하는 문자열의 길이
*     3. int   idp : 소숫점이하 길이
* (O) 1. integer
* (R) 1. integer
*
* ex)  struct { char a[17]; a='1234567890123412'             
*				char b;    } s;    
*				double d;    
*		Zzantod( &d, s.a, sizeof(s.a),2);
*		d=12345678901234.12
*
* 주의사항 : Return 값을 다시 확인할것 ( 2002.10.08)
******************************************************************************/
int Zzantod(double *dvalue, char *str,  int size, int idp)
{
	char   *endptr;
	double expnum;
	
	expnum = 0.0;
 	*dvalue = 0.0;
 	/* 소숫점이하 자릿수 */
	expnum = pow(10 , idp);
	*dvalue  = strtod(str,&endptr) /expnum;
    

	return 1;
}

/*****************************************************************************
* 지정된 문자열을 정수로한다
* (I) 1. char *str : 변환활문자열
*     2. int  size : 변환하고자하는 문자열의 길이
* (O) 1. integer
* (R) 1. integer
*
* 주의사항 : Return 값을 다시 확인할것 ( 2002.10.08)
******************************************************************************/
int Zzitoa(int ivalue, char *str)
{
    sprintf(str , "%d" , ivalue );

	return 1;
}

/*****************************************************************************
* String Convert to double
* (I) 1. char *str : 변환활문자열
*     2. int  size : 변환하고자하는 문자열의 길이
*     3. int   idp : 소숫점이하 길이
* (O) 1. integer
* (R) 1. integer
*
******************************************************************************/
int Zzantodbl( char *str, double *dvalue,  int size)
{
	char   *endptr;

	*dvalue  = strtod(str,&endptr) ;

	return 1;
}


/*****************************************************************************
* 문자열 포인트의 특정문자를 NULL로 설정 (앞부터)
* (I) 1. char *pstr : 문자열 포인트
*     2. char chTarget : 특정문자
* (O) 1. char *pstr : NULL 설정 문자열 포인트  
* (R) 1. int : pstr 길이
*****************************************************************************/
int	ZzHeadTrim(char *pstr, char chTarget)
{
	int	size = strlen(pstr);
	int     i, pos, flag;

	for ( i=0, pos=0, flag=0; i<size; i++ ) {
		char ch = pstr[i];
		if ( ch != chTarget ) {
			if ( flag == 0 ) flag = 1;
		}
		if ( flag == 1 ) pstr[pos++] = ch;
	}

	pstr[pos] = 0x00;

	return strlen(pstr);
}

/*****************************************************************************
* 문자열 포인트의 특정문자를 NULL로 설정 (뒷부터)
* (I) 1. char *pstr : 문자열 포인트
*     2. char chTarget : 특정문자
* (O) 1. char *pstr : NULL 설정 문자열 포인트  
* (R) 1. int : pstr 길이
*****************************************************************************/
int	ZzTailTrim(char *pstr, char chTarget)
{
	int     size = strlen(pstr) - 1;
	int     i;

	for ( i=size; i>=0; i-- ) {
		if ( pstr[i] != chTarget ) {
			break;
		}
		pstr[i] = 0x00;
	}

	return strlen(pstr);
}

/*****************************************************************************
* 문자열 포인트의 특정문자를 NULL로 설정 (앞부터)
* (I) 1. char *pstr : 문자열 포인트
* (R) 1. char *     : NULL 설정 문자열 포인트  
*****************************************************************************/
char    *ZzLTrim(char *pstr)
{
	int    size = strlen(pstr);
	int    i, pos;

	for ( i=0, pos=0; i<size; i++ ) {
		char    ch = pstr[i];
		if ( pos == 0 ) {
			if ( ch == 0x20 || ch =='\t' || ch == '\r' || ch == '\n' ) {
				continue;
			}
		}
		pstr[pos++] = ch;
	}
	pstr[pos] = 0x00;
	return (pstr);
}

/*****************************************************************************
* 문자열 포인트의 특정문자를 NULL로 설정 (뒤부터)
* (I) 1. char *pstr : 문자열 포인트
* (R) 1. char *     : NULL 설정 문자열 포인트  
*****************************************************************************/
char    *ZzRTrim(char *pstr)
{
	int     size = strlen(pstr) - 1;
	int     i;

	for( i=size; i>=0; i-- ) {
		char    ch = pstr[i];
		if ((ch != 0x20) && (ch != '\t') && (ch != '\r') && (ch != '\n')) {
			break;
		}
		pstr[i] = 0x00;
	}
	return (pstr);
}

/*****************************************************************************
 * 문자열데이타에 식별자문자를 마지막포인트에 첨가
 * (I) 1. char chDelimiter : 식별자 문자
 *     2. char *pSrc : 복사할 문자열데이타
 * (O) 1. char *pDec : 식별자문자를 첨가한 문자열 데이타
 * (R) 1. int : 문자열 길이
 *****************************************************************************/ 
int	ZzAddDelimiter(char *pDes, char chDelimiter, char *pSrc)
{
	int	nLength = strlen(pSrc);

	memcpy(pDes, pSrc, nLength);
	*(pDes + nLength) = chDelimiter;
	return(nLength + 1);
}

/*****************************************************************************
* 문자열에서 구분자의 갯수를 구한다.
* (I) 1. char* pstr   : 문자열 데이터
*     2. char delimit : 구분자
* (R) 1. int		  : 구분자 갯수
*****************************************************************************/
int	ZzGetDelimitCount(char *pstr, char delimit)
{
	int	size  = strlen(pstr);
	int	count = 0;

	while ( size-- > 0 ) {
		if ( *pstr == delimit ) {
			count++;
		}
		pstr++;
	}

	return count;
}

/*****************************************************************************
* 문자열에서 구분자의 위치값에 해당되는 데이터를 구한다.
* (I) 1. char* pstr   : 얻고자하는 문자열 데이터
*     2. char delimit : 구분자
* 	  3. char* pdat	  : 구분자 데이터
*	  4. int position : 구분자의 위치(0부터 시작)
* (R) 1. int          : 구분자 데이터의 길이
*****************************************************************************/
int	ZzGetDelimitData(char *pstr, char delimit, char *pdat, int position)
{
	char	data[500];
	int	size = strlen(pstr);
	int	count, point;
	int	i;

	memset(data, 0x00, sizeof(data));

	for ( i=0, count=0, point=0; i<size; i++ ) {
		char ch = pstr[i];
		if ( ch == delimit ) {
			count++;
			continue;
		}
		
		if ( position != count ) continue;

		data[point++] = ch;	
	}

	return sprintf(pdat, "%s", data);	
}

/*****************************************************************************
* 문자열에서 구분자의 위치값 이후의 모든 데이터를 구한다.
* (I) 1. char* pstr   : 얻고자하는 문자열 데이터
*     2. char delimit : 구분자
* 	  3. char* pdat	  : 구분자 데이터
*	  4. int position : 구분자의 위치(0부터 시작)
* (R) 1. int          : 구분자 데이터의 길이
*****************************************************************************/
int	ZzGetDelimitFullData(char *pstr, char delimit, char *pdat, int position)
{
	char	data[500];
	int	size = strlen(pstr);
	int	count, point;
	int	i;

	memset(data, 0x00, sizeof(data));

	for ( i=0, count=0, point=0; i<size; i++ ) {
		char ch = pstr[i];
		if ( ch == delimit ) {
			count++;
			if ( position == count ) continue;
		}
		
		if ( position > count ) continue;

		data[point++] = ch;	
	}

	return sprintf(pdat, "%s", data);	
}

/*****************************************************************************
 * 숫자 문자 체크
 * arg(I) : 1. char *pstr : 문자열
 *          2. ing size : 문자열 길이
 * return : 1. '0'이상 부터 '9'이하인 값의 count
 ****************************************************************************/
int     ZzGetNumberCount(char *pstr, int size)
{
	int     numb = 0;
	while( size-- > 0 ) {
		if ( (*pstr >= '0') && (*pstr <= '9')) {
			numb++;
		}
		pstr++;
	}
	return(numb);
}

/*****************************************************************************
 * 문자열 데이타가 수치형 문자열이며 그 숫치 값이 법위에 해당되는지 체크
 * arg(I) : 1. char datastrs[] : 문자열 데이타
 *          2. int datasize : 문자열 데이타 길이
 *          3. int beg_numb : 비교 수치
 *          4. int end_numb : 비교 수치
 * return : 1. '0' : 거짓
 *          2. '1' : 참
 ****************************************************************************/
int    ZzValidNumber(char datastrs[], int datasize, int beg_numb, int end_numb)
{
	int     numb9999 = ZzGetNumberCount(datastrs, datasize);
	if (numb9999 < datasize) {
		return(0);
	}
	if (beg_numb > 0L) {
		long    datanumb = ZzStrToLong(datastrs, datasize);
		if ((datanumb < beg_numb) || (datanumb > end_numb)) {
			return(0);
		}
	}
	return(1);  /* 정상 */
}

/*****************************************************************************
 * LONG값을 시간으로 변경
 * arg(O) : 1. char *pszTime : 문자열로 변환된 시간(HH:MM:DD)
 * arg(I) : 1. long lValue : 변환할 값
 * return : 1. void
 ****************************************************************************/
void    ZzLongToTime(char *pszTime, long nValue)
{
	int     nHH = 0;
	int     nMI = 0;
	int     nSS = 0;
	int     nTP = 0;

	nHH = (int)(nValue / 3600);
	nTP = (int)(nValue % 3600);
	nMI = (int)(nTP / 60);
	nSS = (int)(nTP % 60);

	sprintf(pszTime, "%02d:%02d:%02d", nHH, nMI, nSS); 
}

/*****************************************************************************
 * 문자열을 입력 위치 만틈 반환
 * arg(I) : 1. char* pData : 문자열 데이타
 *          2. char* pResults : 결과값
 *          3. int nStart : 문자열 데이타 길이
 *          4. int nStop : 비교 수치
 * return : 1. void
 ****************************************************************************/
void ZzGetCharFromLength(char* pData, char* pResults, int nStart, int nStop)
{
	if(strlen(pData) <= 0)
		return;
	
	if(nStop <= nStart)
		return;
		
	int j = 0;
	for(int i=nStart; i<=nStop; i++)
	{
		pResults[j] = pData[i];
		j++;
	}
} 

void ReplaceSingleToDouble(char* pString)
{
	int cTemp;
	int nFileLen = strlen(pString);
	
	int cReplace = '"';
	int cSingle = '\'';

	
	for(int i=0; i<nFileLen ; i++)
	{
		cTemp = pString[i];
		if( cTemp == cSingle ) // 96 is ' 
		{
			pString[i] = (char)cReplace;
							
		}
	}
}


/*======================================================================
 void ReplaceSingleToBackslash(char* pString)
 ex> StrReplace("te'st")
 re> output is te\'st
 ======================================================================*/


void ReplaceSingleToBackslash(char* pString)
{
	int cTemp;
	int nFileLen = strlen(pString);
	
	int cReplace = '\'';
	int cSingle = '\\\\\'';

	
	for(int i=0; i<nFileLen ; i++)
	{
		cTemp = pString[i];
		if( cTemp == cSingle ) // 96 is ' 
		{
			pString[i] = (char)cReplace;
							
		}
	}

}