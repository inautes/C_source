/*****************************************************************************
 * 파 일 명 : daemcom.h
 * 설    명 : 공통모듈
 * 작 성 자 : JDP
 * 작 성 일 : 2002.09.30
 * 주의사항 :
 *****************************************************************************/
#ifndef	_DAEMCOM_H_
#define	_DAEMCOM_H_


#define SUCC        1
#define FAIL        -1

#define TRUE        1
#define FALSE       0

/*============================================================================*/
/* 로그레벨정의                                                               */
/*============================================================================*/
#define TRACE		1
#define CHECK		2
#define ERROR		3
#define ALWAY		9

/*============================================================================*/
/* 구조체정의                                                                 */
/*============================================================================*/

typedef struct {

    char    szProcName[256];   /* 프로세스 명칭     */
    char    szFilePath[256];   /* 디렉토리          */
    char    szLogFBase[256];   /* 로그파일 디렉토리 */
    char    szBgnTime [20];    /* 시작시간          */
    char    szEndTime [20];    /* 종료시간          */

} SUserParm;

typedef struct {
    int     nFlag;
    char    szSource[125];
    char    szOutput[125];
    char    szRemark[125];
} SConvertType;

/*============================================================================*/
/* DEFINE함수 정의 */
/*    define NULL   		0x00 */
/*============================================================================*/

/* NULL SET은 사용하지 마시기 바랍니다 */
/*************************************************************/
#define NULLSET(a)  memset((char*)&a, 0x00, sizeof(a))
/*************************************************************/

#define MEMCPY(a, b) {	\
		memset((char*)a, 0x00, sizeof(a));	\
		memcpy((char*)a, (char*)b, strlen(b));	\
					}

/*===========================================================================*/
/*            .VARIABLE INITIALIZE                                          */
/*============================================================================*/
#define INITV(x)        { (x).arr[0] = 0; (x).len = 0; }
#define INITINT(x)      x = 0
#define INITSTR(x)      memset(x, 0x00, sizeof(x))
#define INITVAR(x)      memset(&x, 0x00, sizeof(x))
#define INITHOST(x,y)   memset(x, 0x00, sizeof(y))


/*****************************************************************************/
int	 ZzErrorDataWrite(char *pszFileName, char *pszData);
int  ZzValidString(char *datastrs, int datasize, int min_size);
int  ZzGetStringCount(char *pstr, int size);
char ZzGetCommaStr(char *pstr, double value);
long ZzStrToLong(char *pstr, int size);
int  ZzBrokenHangulChangeNull(char *pStr);
void ZzStrReplace(char *source, char *search, char *change);
void ZzStrToLower(char *pstr);
void ZzStrToUpper(char *pstr);
int  ZzStrSpaceSetAndCopy(char *pmemstr, int size, char *pdatastr);
void ZzStrChangeFormData(char *pwantstr, char *pdatastr, char *pformstr);
void ZzCharChange(char *pstr, int size, char cDest, char cChng);
int ZzGetDiffTime(char *szTime1, char *szTime2);
int ZzCurDiffTime(char *szCmpTime);
int  ZzIsRemainTimesecs(time_t comptime, int timesecs);
int	 ZzDateCheck(char *pdate);
int  ZzGetSysTime(char *pdatastr, char *pformstr);
char *ZzGetCurDate(char *pszStr);
char *ZzGetCurTime(char *pszStr);
int ZzCalcBeginOfEnd(char *szStartymd, char *szEndymd);
int ZzCalcSolarDay(char *szStartymd, int iFlag);
int ZzSolarDateCount(char *psdate, int iCount);
int ZzInDayOfDateCalc(char *psdate, char *sDate, int iDay);
int ZzInDayOfWeek(char *sDate);
int ZzGetEnvironmentValue(char *pszValue, char *pstr);
void ZzInitGlobalVariable(char *sSvrname);
void ZzInitGlobalVariable2(char *sSvrname, char *sFilePath);
void ZzGetLogFileName(char *szFilePath, char *szProcName, char *szLogFile, char *szErrFile);
int ZzLOG(int nLevel, char *pformat, ...);
int ZzPRT(int nLevel, char *pformat, ...);
int	rmLOG(int nLevel, char *pformat, ...);
long ZzGetAtoL(char *str, int size);
int Zzantoi(char *str, int size);
int Zzantoip(int *ip, char *str, int size);
int Zzdtoan(char *str, double  dvalue,  int size, int idp);
double Zzantodr(char *str,  int size, int idp);
int Zzantod(double *dvalue, char *str,  int size, int idp);
int Zzitoa(int ivalue, char *str);
int Zzantodbl( char *str, double *dvalue,  int size);
int	ZzHeadTrim(char *pstr, char chTarget);
int	ZzTailTrim(char *pstr, char chTarget);
char *ZzLTrim(char *pstr);
char *ZzRTrim(char *pstr);
int	ZzAddDelimiter(char *pDes, char chDelimiter, char *pSrc);
int	ZzGetDelimitCount(char *pstr, char delimit);
int	ZzGetDelimitData(char *pstr, char delimit, char *pdat, int position);
int	ZzGetDelimitFullData(char *pstr, char delimit, char *pdat, int position);
int ZzGetNumberCount(char *pstr, int size);
int ZzValidNumber(char datastrs[], int datasize, int beg_numb, int end_numb);
void ZzLongToTime(char *pszTime, long nValue);
int	ZzCharCount(char *pStr, char chTarget);
int GetFistCharIndex(char* srcString,char nChar);
int GetReverseIndex(char* srcString,char nChar);
void ReplaceSingleToDouble(char* pString);
char* Func_getHostname();
#endif
