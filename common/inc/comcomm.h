/******************************************************************************
 *   서브시스템 : 공통모듈
 *   프로그램명 : cmdcomm.h
 *         기능 : 공통함수 헤더화일
 *         설명 :
 *       작성자 : JDP
 *       작성일 : 2004/02/17
 *     수정이력 :
 *
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/

#ifndef _CMDCOMM_
#define	_CMDCOMM_

#include <stdio.h>
#include <stdint.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <time.h>
#include "apstruct.h"
#include "comconf.h"
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <iconv.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/vfs.h>

int		lockfile(int fd);
int		check_single_running(char *pos); // 중복 실행 체크
char*	Func_getHostname();

char*	ReplaceSingleQuotation(char* strSrcString, char cReplace,char* pResult);
void	ReplaceSingleToBackslash(char* pString);

int64_t atoi64(const char* str);

int		E_dump(int err_code, char *err_mesg, char* &pSendData);
void	infCpyUserParm(SUserParm_T *pUser);
int		infLOG(int nLevel, char *pformat, ...); /* infLOG(); */
int		DLOG(int nLevel, char *pformat, ...);
void    infToFormatString(char *pwantstr, char *pDataStr, char *pFormStr);
int		infGetSysTime(char *pDataStr, char *pFormStr);
int		infGetSysDate(char *pDataStr, char *pFormStr);
int		infTimeToFormatStr(char *pDataStr, char *pFormStr, struct tm *stm);
int		infGetStrTime(char *pDataStr, char *pFormStr, time_t chngtime);
int		infGetStrDate(char *pDataStr, char *pFormStr, time_t chngtime);

char*	infGetCurTime(char *pszStr);
char*	infGetCurDate(char *pszStr);
int		infDateFormatCheck(char *pDate);

void	infChangeChar(char *pStr, int nSize, char cDest, char cChng);
int		infStrSpaceSetAndCopy(char *pDes, int nSize, char *pSrc);
int		infStrPointCopy(char *pdes, int nSize, char *psrc, int pos);
void    infStrReplace(char *source, char *search, char *change);
void    infStrReplace(char* original, char* search, char* replace, char* result);
void	infStrToUpper(char *pStr);
void	infStrToLower(char *pStr);
int		infLongToStr(char *pStr, long lValue, int nSize);
int	    infLongToFillStr(char *pStr, long lValue, int nSize, char chFill);

int     infTrimLeft(char *pStr, char chTarget);
int     infTrimRight(char *pStr, char chTarget);
char*	infHeadTrim(char *pStr);
char*	infTailTrim(char *pStr);
int     infIsElapseTimesecs(time_t comptime, int timesecs);
int     infIsRemainTimesecs(time_t comptime, int timesecs);
int		infIsIntervalTimeOver(char *szCmpTime, int nInterva);
int     infGetDiffTime(char *szTime1, char *szTime2);
int     infCurDiffTime(char *szCmpTime);

int		infAddDelimiter(char *pDes, char chDelimiter, char *pSrc);
int		infGetDelimitCount(char *pStr, char chDelimiter);
int		infGetDelimitData(char *pStr, char chDelimiter, char *pdat, int position);
void	infSleep(int nSeconds);
void	infWaits(int nSeconds);
void	infSetError(SErrCheck_T *pErr, int nCode, char *szMsg);
int		infGetRunningProcessCount(char *szProcName);
char*   infGetStrError(int nErrno);

int     infCheckOracleDB(int sqlcode);

void		infReverseStr(char *pStr);
short int	infReverseStrToShortInt(char *pStr);
int			infReverseIntToStr(char *pStr, int val);
short int	infReverseShortInt(short int val);
int			infReverseInt(int val);

int		infStrToInt(char *pStr, int nSize);
int		infStrToInt(char *pStr, int nSize);
long	infStrToLong(char *pStr, int nSize);

int  	infBrokenHangulChangeNull(char *pStr);

int     infInitSequence(char *szFileName, int nMaxCount);
long    infNxtSequence(int nGetEntry, long lMaxValue);
long    infGetSequence(int nGetEntry, long lMaxValue);
void    infSetSequence(int nSetEntry, long lSetValue);
int     infTermSequence(char *szFileName, int nMaxCount);

long    infStringToLong(char *pStr, int nSize);
int		infNumNumber(char *pStr, int nSize);
int     infNumString(char *pStr, int nSize);
int     infValidNumber(char *pDataStr, int nDataSize, int nBgnNumber, int nEndNumber);
int     infValidString(char *pDataStr, int nDataSize, int nMinSize);
int     infControlCharToSpace(char *pStr, int nSize);

int     infGetRealSize(char *pDataStr, int nDataSize);

void strcpyB(char* pOutStr, char* pInStr);


int		infGetBatchTotCnt(SBatchCnt_T *pBatch);
int		infGetBatchSucCnt(SBatchCnt_T *pBatch);
int		infGetBatchErrCnt(SBatchCnt_T *pBatch);
int		infGetBatchSkpCnt(SBatchCnt_T *pBatch);
void    infIncBatchTotCnt(SBatchCnt_T *pBatch);
void    infIncBatchSucCnt(SBatchCnt_T *pBatch);
void    infIncBatchErrCnt(SBatchCnt_T *pBatch);
void    infIncBatchSkpCnt(SBatchCnt_T *pBatch);
void    infDecBatchTotCnt(SBatchCnt_T *pBatch);
void    infDecBatchSucCnt(SBatchCnt_T *pBatch);
void    infDecBatchErrCnt(SBatchCnt_T *pBatch);
void    infDecBatchSkpCnt(SBatchCnt_T *pBatch);


char *replaceAll(char *s, const char *olds, const char *news);


#endif	/* _CMDCOMM_ */

/*****************************************************************************
 * End of file...
 *****************************************************************************/
