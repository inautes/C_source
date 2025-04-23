/******************************************************************************
 *   서브시스템 : 공통모듈
 *   프로그램명 : cmdconf.c
 *         기능 : 초기화 함수 정의
 *         설명 :
 *       작성자 : JDP
 *       작성일 : 2004/02/17
 *     수정이력 :
 *
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/file.h>
#include <string.h>
#include <memory.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stddef.h>

#include "apdefine.h"
#include "comcomm.h"
#include "apstruct.h"
#include "comconf.h"

/*
** 전역변수
*/
static	char*			gpAppName = NULL;

/*
** 함수 프로토타입
*/
#ifndef ITEM_COUNT
#define ITEM_COUNT(xxxx)    (sizeof(xxxx)/sizeof((xxxx)[0]))
#endif

#if 0
#define _CONFIG_LOG_
#endif

/*****************************************************************************
* Input 문자열포인트(맨처음)에서 포인트의 값이 space가 아닐때까지 증가시켜
* 그 포인트를 return
* arg(I) 1. char* pdatastr : 문자열 포인트
* return 1. char* : space 문자열 포인트가 아닌 포인트
*****************************************************************************/
char	*infSkipSpace(char *pdatastr)
{
	while (*pdatastr != 0x00) {
		if ((*pdatastr != 0x20) && (*pdatastr != '\t')) {
			break;
		}
		pdatastr++;
	}
	return(pdatastr);
}

/*****************************************************************************
* Input 문자열포인트(마지막)에서 포인트의 값이 space가 아닐때까지 감소시켜
* 그 포인트를 return
* arg(I) 1. char* pdatastr : 문자열 포인트
* return 1. char* : space 문자열 포인트가 아닌 포인트
*****************************************************************************/
char	*infTailSpace(char *pdatastr)
{
	char	*pdataend = (pdatastr + strlen(pdatastr));

	while (pdataend > pdatastr) {
		char	ch = *(pdataend - 1);
		if ((ch != 0x20) && (ch != '\t') && (ch != '\r') && (ch != '\n')) {
			break;
		}
		pdataend--;
	}
	*pdataend = 0x00;
	return(pdatastr);
}

/*****************************************************************************
 * Config파일에 정의된 값을 얻음
 * arg(I) 1. char *preadstr : 파일에서 읽어온 문자열
 * arg(O) 1. char **pitemptr : ITEM명(Config요소)
 *        2. char **pdataptr : ITEM Config value
 * return 1. 성공 : '0'
 *        2. 실패 : '-1'
 ****************************************************************************/
static	int		GET_ITEMDATA(char *preadstr, char **pitemptr, char **pdataptr)
{
	char	*pitemstr, *pdatastr;
	char	*pskipstr;

	pitemstr = infSkipSpace(preadstr);

	if ((pdatastr = strchr(pitemstr, '#')) != NULL) {
		*pdatastr = 0x00;
	}
	if ((*pitemstr == '#') || (*pitemstr == 0x00)) {
		return(-1);
	}
	if ((pdatastr = strchr(pitemstr, '=')) == NULL) {
		return(-1);
	}
	*pdatastr++ = 0x00;

	*pitemptr = infTailSpace(pitemstr);

	pdatastr = infTailSpace(pdatastr);
	pskipstr = infSkipSpace(pdatastr);

	memmove(pdatastr, pskipstr, strlen(pskipstr) + 1);

	*pdataptr = pdatastr;

	return(0);
}

/*
** Config 파일에서 구성요소별로 데이타를 관리하는 구조체
*/
typedef	struct itemdata_t {
	char	itemname[10];
	int		itemposi;
	int		itemsize;
	int		(*itemfunc)(char *, struct itemdata_t *, char *);
	char   *(*viewfunc)(char *, struct itemdata_t *, char *);
	int		defvalue;
} itemdata_t;

/*****************************************************************************
 * EACHWORK을 pUser의  배열에 등록
 * arg(I) 1. SUserParm_T *pUser : Process Infomation
 *        2. char *psubworks : Serial Number
 * return 1. 실패 : '-1'
 *        2. 성공 : '1'
 *        3. '0': psubworks이 NULL이거나 이전에 등록되어 중복된 경우
 ****************************************************************************/
static  int     CpySUB_WORK(SUserParm_T *pUser, char *psubworks)
{
	int			n, nposition;
	char		szTemp[20];
	int          nEachNumb = pUser->nEachNumb;
	SEachWork_T *pEach     = pUser->asEachWork + nEachNumb;

	if (nEachNumb >= ITEM_COUNT(pUser->asEachWork)) {
		return(-1);
	}

	if (*psubworks == 0x00) {
		return(0);
	}

#ifdef _CONFIG_LOG_
	infLOG(TRACE, "CONFIG`EACHWORK=[%s]\n", psubworks);
#endif

	pEach->nThrIndex = -1;
	pEach->nSocketId = -1;

	nposition = 0;
	while (psubworks != NULL) {
		char    *pnextptr;
		psubworks = infSkipSpace(psubworks);
		if ((pnextptr = strchr(psubworks, '|')) != NULL) {
			*pnextptr++ = 0x00;
		}

		switch( nposition )
		{
		case  0 : sprintf(pEach->szModuleId, "%s", psubworks);
#ifdef _CONFIG_LOG_
				  infLOG(TRACE, "CONFIG`EACHWORK=MODULEID[%s]\n", pEach->szModuleId);
#endif
				  break;
		case  1 : sprintf(pEach->szRegionId, "%s", psubworks);
#ifdef _CONFIG_LOG_
				  infLOG(TRACE, "CONFIG`EACHWORK=SERIALNO[%s]\n", pEach->szRegionId);
#endif
				  break;
		case  2 : sprintf(pEach->szServerNm, "%s", psubworks);
#ifdef _CONFIG_LOG_
				  infLOG(TRACE, "CONFIG`EACHWORK=SERVERNM[%s]\n", pEach->szServerNm);
#endif
				  break;
		case  3 : sprintf(pEach->szServerIp, "%s", psubworks);
#ifdef _CONFIG_LOG_
				  infLOG(TRACE, "CONFIG`EACHWORK=SERVERIP[%s]\n", pEach->szServerIp);
#endif
				  break;
		case  4 : memset(szTemp, 0x00, sizeof(szTemp));
				  sprintf(szTemp, "%s", psubworks);
				  pEach->nInetPort = atoi(szTemp);
					
#ifdef _CONFIG_LOG_
				  infLOG(TRACE, "CONFIG`EACHWORK=INETPORT[%s]\n", pEach->nInetPort);
#endif
				  break;
		case  5 : sprintf(pEach->szLoadFlag, "%s", psubworks);
#ifdef _CONFIG_LOG_
				  infLOG(TRACE, "CONFIG`EACHWORK=LAODFLAG[%s]\n", pEach->szLoadFlag);
#endif
				  break;
		case  6 : sprintf(pEach->szSvrAlias, "%s", psubworks);
#ifdef _CONFIG_LOG_
				  infLOG(TRACE, "CONFIG`EACHWORK=SVRALIAS[%s]\n", pEach->szSvrAlias);
#endif
				  break;
		}

		psubworks = pnextptr;
		nposition++;
	}

    pUser->nEachNumb++;

    return(1);
}

void    infSetSubWorkFlag(SEachWork_T *pEach, int nValue) { pEach->nWorkFlag = nValue; }
void    infSetSubSendRtns(SEachWork_T *pEach, int nValue) { pEach->nSendRtns = nValue; }
void    infSetSubRecvRtns(SEachWork_T *pEach, int nValue) { pEach->nRecvRtns = nValue; }
void    infSetSubDbmsRtns(SEachWork_T *pEach, int nValue) { pEach->nDbmsRtns = nValue; }
void    infSetSubExcpRtns(SEachWork_T *pEach, int nValue) { pEach->nExcpRtns = nValue; }
void    infSetSubRespRtns(SEachWork_T *pEach, int nValue) { pEach->nRespRtns = nValue; }
void    infSetSubTempRtns(SEachWork_T *pEach, int nValue) { pEach->nTempRtns = nValue; }
void    infSetSubSocketId(SEachWork_T *pEach, int nValue) { pEach->nSocketId = nValue; }

int		infSetThreadIndex(SEachWork_T *pEach, int nValue) { pEach->nThrIndex = nValue; return (pEach->nThrIndex); }
int		infGetThreadIndex(SEachWork_T *pEach) { return (pEach->nThrIndex); }

int     infGetSubWorkFlag(SEachWork_T *pEach) { return (pEach->nWorkFlag); }
int     infGetSubSendRtns(SEachWork_T *pEach) { return (pEach->nSendRtns); }
int     infGetSubRecvRtns(SEachWork_T *pEach) { return (pEach->nRecvRtns); }
int     infGetSubDbmsRtns(SEachWork_T *pEach) { return (pEach->nDbmsRtns); }
int     infGetSubExcpRtns(SEachWork_T *pEach) { return (pEach->nExcpRtns); }
int     infGetSubRespRtns(SEachWork_T *pEach) { return (pEach->nRespRtns); }
int     infGetSubTempRtns(SEachWork_T *pEach) { return (pEach->nTempRtns); }
int     infGetSubSocketId(SEachWork_T *pEach) { return (pEach->nSocketId); }
char*   infGetSubModuleId(SEachWork_T *pEach) { return (pEach->szModuleId); }
char*   infGetSubRegionId(SEachWork_T *pEach) { return (pEach->szRegionId); }
char*   infGetSubServerNm(SEachWork_T *pEach) { return (pEach->szServerNm); }
char*   infGetSubServerIp(SEachWork_T *pEach) { return (pEach->szServerIp); }
int     infGetSubInetPort(SEachWork_T *pEach) { return (pEach->nInetPort); }
int     infGetSubLoadFlag(SEachWork_T *pEach) { return (strcmp(pEach->szLoadFlag, "Y")==0) ? 1:0; }
char*	infGetSubSvrAlias(SEachWork_T *pEach) { return (pEach->szSvrAlias); }

long	infGetSubSendNumb(SEachWork_T *pEach) { return (pEach->nSendNumb); }
long	infGetSubRecvNumb(SEachWork_T *pEach) { return (pEach->nRecvNumb); }
void	infIncSubSendNumb(SEachWork_T *pEach) { pEach->nSendNumb++; }
void	infIncSubRecvNumb(SEachWork_T *pEach) { pEach->nRecvNumb++; }

char*	infGetPvcName(SEachWork_T *pEach) { return (pEach->szPvcName); }
int		infGetX25Port(SEachWork_T *pEach) { return (pEach->nX25Port); }
int		infGetX25Stat(SEachWork_T *pEach) { return (pEach->nX25Stat); }
int		infGetX25Init(SEachWork_T *pEach) { return (pEach->nIniFlag); }
int		infSetX25Port(SEachWork_T *pEach, int nValue) { pEach->nX25Port = nValue; return (pEach->nX25Port); }
int		infSetX25Stat(SEachWork_T *pEach, int nValue) { pEach->nX25Stat = nValue; return (pEach->nX25Stat); }
int		infSetX25Init(SEachWork_T *pEach, int nValue) { pEach->nIniFlag = nValue; return (pEach->nIniFlag); }

int		infSetEachworkToUserparm(char *szModuleId, SUserParm_T *pUser)
{
    int         nEachNumb = pUser->nEachNumb;
    SEachWork_T *pEach    = pUser->asEachWork;

    while ( nEachNumb-- > 0 )
    {
        if ( strcmp(szModuleId, infGetSubModuleId(pEach)) == 0 )
		{
			if ( infGetSubLoadFlag(pEach) == 0 ) return (-1);

			infSetProcName(pUser, infGetSubModuleId(pEach));
			infSetRegionNm(pUser, infGetSubServerNm(pEach));
			infSetRegionIp(pUser, infGetSubServerIp(pEach));
			infSetRegionId(pUser, infGetSubRegionId(pEach));
			infSetServerNm(pUser, infGetSubServerNm(pEach));
			infSetServerIp(pUser, infGetSubServerIp(pEach));
			infSetInetPort(pUser, infGetSubInetPort(pEach));
			infSetSvrAlias(pUser, infGetSubSvrAlias(pEach));
			return (0);
        }
        pEach++;
    }

	return (-1);
}

/*****************************************************************************
 * USEFILES NAME을 pUser의 asUseFiles 배열에 등록
 * arg(I) 1. SUserParm_T *pUser : Process Infomation
 *        2. char* pszFileName : USE FILE NAME
 * return 1. 실패 : '-1'
 *        2. 성공 : '1'
 *        3. '0': asUseFiles이 NULL이거나 이전에 등록되어 중복된 경우
 ****************************************************************************/
static  int     CpyUSE_FILE(SUserParm_T *pUser, char *pszFileName)
{
	int          nFileNumb = pUser->nFileNumb;
	int          n;

	if (nFileNumb >= ITEM_COUNT(pUser->asUseFiles)) {
		return(-1);
	}

	if (*pszFileName == 0x00) {
		return(0);
	}

	if (strlen(pszFileName) > FILENAME_SIZE) {
		pszFileName[FILENAME_SIZE] = 0x00;
	}

	for (n=0; n<nFileNumb; n++) {
		if (strcmp(pUser->asUseFiles[n].szFileName, pszFileName) == 0) {
			return(0);
		}
	}

#ifdef _CONFIG_LOG_
	infLOG(TRACE, "CONFIG`USEFILES=[%s]\n", pszFileName);
#endif

	sprintf(pUser->asUseFiles[nFileNumb].szFileName, "%s", pszFileName);

	pUser->nFileNumb++;

    return(1);
}

/*****************************************************************************
 * USE FILE NAME을 pUser의 pszFileName배열에 등록
 * arg(I) 1. SUserParm_T *pUser : Process Infomation
 *        2. char* ppszFileName : 등록할 pszFileName명
 * return 1. 실패 : '-1'
 *        2. 정상 : '0'
 ****************************************************************************/
static  int     AddUSE_FILE(SUserParm_T *pUser, char *pszFileName)
{
	if (pszFileName != NULL) 
	{
		pszFileName = infSkipSpace(pszFileName);
		pszFileName = infTailSpace(pszFileName);

		if (CpyUSE_FILE(pUser, pszFileName) < 0) {
			return(-1);
		}
	}

	return(0);
}

/*****************************************************************************
 * SUB_WORK을 pUser의 EACHWORK 배열에 등록
 * arg(I) 1. SUserParm_T *pUser : Process Infomation
 *        2. char *psubworks : EACHWORK문자열 포인트
 * return 1. 실패 : '-1'
 *        2. 정상 : '0'
 ****************************************************************************/
static  int     AddSUB_WORK(SUserParm_T *pUser, char *psubworks)
{
	
	if (psubworks != NULL) 
	{
		psubworks = infSkipSpace(psubworks);
		psubworks = infTailSpace(psubworks);

		if (CpySUB_WORK(pUser, psubworks) < 0) {
			return(-1);
		}
	}

	return(0);
}

/*****************************************************************************
 * Config파일의 EACHWORK을 Setting하기 위한 함수
 * arg(I) 1. char *pUser : pUser 명
 *        2. itemdata_t *itemdata : 구성요소 구조체
 *        3. char *pdatastr : 구성요소 구조체에 등록할 값(EACHWORK)
 * return 1. 실패 : '-1'
 *        2. 정상 : '0'
 *****************************************************************************/
static  int     ITEM_ADDSUB(char *pUser,itemdata_t *itemdata,char *pdatastr)
{
    int         rc;
    if (pdatastr == NULL) {
        return(0);
    }
    if ((rc = AddSUB_WORK((SUserParm_T *)pUser, pdatastr)) < 0) {
        sprintf(pdatastr, "너무 많은 EACHWORK을 지정하였습니다(최대=%d)", MAX_EACHWORK);
    }
    return(rc);
}

/*****************************************************************************
 * Config파일의 EACHWORK을 Setting하기 위한 함수(임시로 만듬)
 * arg(I) 1. char *pUser : pUser 명
 *        2. itemdata_t *itemdata : 구성요소 구조체
 *        3. char *pdatastr : 구성요소 구조체에 등록할 값(EACHWORK)
 * return 1. char* : 구성된 EACHWORK문자열
 *****************************************************************************/
static  char   *VIEW_ADDSUB(char *pUser,itemdata_t *itemdata,char *pdatastr)
{
    return("");
}

/*****************************************************************************
 * Config파일의 USEFILES을 Setting하기 위한 함수
 * arg(I) 1. char *pUser : pUser 명
 *        2. itemdata_t *itemdata : 구성요소 구조체
 *        3. char *pdatastr : 구성요소 구조체에 등록할 값(USEFILES)
 * return 1. 실패 : '-1'
 *        2. 정상 : '0'
 *****************************************************************************/
static  int     ITEM_ADDUSE(char *pUser,itemdata_t *itemdata,char *pdatastr)
{
    int         rc;
    if (pdatastr == NULL) {
        return(0);
    }
    if ((rc = AddUSE_FILE((SUserParm_T *)pUser, pdatastr)) < 0) {
        sprintf(pdatastr, "너무 많은 USEFILES을 지정하였습니다(최대=%d)", MAX_USEFILES);
    }
    return(rc);
}

/*****************************************************************************
 * Config파일의 USEFILES을 Setting하기 위한 함수(임시로 만듬)
 * arg(I) 1. char *pUser : pUser 명
 *        2. itemdata_t *itemdata : 구성요소 구조체
 *        3. char *pdatastr : 구성요소 구조체에 등록할 값(USEFILES)
 * return 1. char* : 구성된 USEFILES문자열
 *****************************************************************************/
static  char   *VIEW_ADDUSE(char *pUser,itemdata_t *itemdata,char *pdatastr)
{
    return("");
}

/*****************************************************************************
 * PVC_NAME을 pUser의  배열에 등록
 * arg(I) 1. SUserParm_T *pUser : Process Infomation
 *        2. char *psubworks : Serial Number
 * return 1. 실패 : '-1'
 *        2. 성공 : '1'
 *        3. '0': psubworks이 NULL이거나 이전에 등록되어 중복된 경우
 ****************************************************************************/
static  int     CpyPVC_NAME(SUserParm_T *pUser, char *pszPvcName)
{
	int			n, nposition;
	int          nEachNumb = pUser->nEachNumb;
	SEachWork_T *pEach     = pUser->asEachWork + nEachNumb;

	if (nEachNumb >= ITEM_COUNT(pUser->asEachWork)) {
		return(-1);
	}

	if (*pszPvcName == 0x00) {
		return(0);
	}

	pEach->nX25Port = -1;
	pEach->nX25Stat = -1;
	pEach->nIniFlag =  0;
	pszPvcName = infSkipSpace(pszPvcName);
	sprintf(pEach->szPvcName, "%s", pszPvcName);

#ifdef _CONFIG_LOG_
	infLOG(TRACE, "CONFIG`PVC_NAME=[%s]\n", pEach->szPvcName);
#endif

    pUser->nEachNumb++;

    return(1);
}

/*****************************************************************************
 * PVC_NAME을 pUser의 PVC_NAMES 배열에 등록
 * arg(I) 1. SUserParm_T *pUser : Process Infomation
 *        2. char *pszPvcName : PVC_NAME문자열 포인트
 * return 1. 실패 : '-1'
 *        2. 정상 : '0'
 ****************************************************************************/
static  int     AddPVC_NAME(SUserParm_T *pUser, char *pszPvcName)
{
	
	if (pszPvcName != NULL) 
	{
		pszPvcName = infSkipSpace(pszPvcName);
		pszPvcName = infTailSpace(pszPvcName);

		if (CpyPVC_NAME(pUser, pszPvcName) < 0) {
			return(-1);
		}
	}

	return(0);
}

/*****************************************************************************
 * Config파일의 PVC_NAME을 Setting하기 위한 함수
 * arg(I) 1. char *pUser : pUser 명
 *        2. itemdata_t *itemdata : 구성요소 구조체
 *        3. char *pdatastr : 구성요소 구조체에 등록할 값(USEFILES)
 * return 1. 실패 : '-1'
 *        2. 정상 : '0'
 *****************************************************************************/
static  int     ITEM_ADDPVC(char *pUser,itemdata_t *itemdata,char *pdatastr)
{
    int         rc;
    if (pdatastr == NULL) {
        return(0);
    }
    if ((rc = AddPVC_NAME((SUserParm_T *)pUser, pdatastr)) < 0) {
        sprintf(pdatastr, "너무 많은 PVC_NAME을 지정하였습니다(최대=%d)", MAX_EACHWORK);
    }
    return(rc);
}

/*****************************************************************************
 * Config파일의 USEFILES을 Setting하기 위한 함수(임시로 만듬)
 * arg(I) 1. char *pUser : pUser 명
 *        2. itemdata_t *itemdata : 구성요소 구조체
 *        3. char *pdatastr : 구성요소 구조체에 등록할 값(USEFILES)
 * return 1. char* : 구성된 USEFILES문자열
 *****************************************************************************/
static  char   *VIEW_ADDPVC(char *pUser,itemdata_t *itemdata,char *pdatastr)
{
    return("");
}

/*****************************************************************************
 * Config파일의 각 ITEM에 대한 문자열 데이타를 Setting
 * arg(I) 1. char *pUser : pUser 명
 *        2. itemdata_t *itemdata : 구성요소 구조체
 *        3. char *pdatastr : 구성요소 구조체에 등록할 값
 * return 1. '0'으로 Fixed
 *****************************************************************************/
static	int		ITEM_STRING(char *pUser,itemdata_t *itemdata,char *pdatastr)
{
	int		datasize;
	if (pdatastr == NULL) {
		return(0);
	}
	if ((datasize = strlen(pdatastr)) >= itemdata->itemsize) {
		datasize = (itemdata->itemsize - 1);
	}

	memcpy(pUser + itemdata->itemposi, pdatastr, datasize);
	return(0);
}

/*****************************************************************************
 * Config파일의 각 ITEM에 대한 문자열 데이타를 얻음
 * arg(I) 1. char *pUser : pUser 명
 *        2. itemdata_t *itemdata : 구성요소 구조체
 *        3. char *pdatastr : 구성요소 구조체에 등록할 값
 * return 1. char* : 구성된 문자열 포인트
 *****************************************************************************/
static	char   *VIEW_STRING(char *pUser,itemdata_t *itemdata,char *pdatastr)
{
	return(pUser + itemdata->itemposi);
}

/*****************************************************************************
 * Config파일의 각 ITEM에 대한 숫자형 데이타를 Setting
 * arg(I) 1. char *pUser : pUser 명
 *        2. itemdata_t *itemdata : 구성요소 구조체
 *        3. char *pdatastr : 구성요소 구조체에 등록할 값
 * return 1. 성공 : '0'
 *        2. 실패 : '-1'
 *****************************************************************************/
static	int		ITEM_NUMBER(char *pUser,itemdata_t *itemdata,char *pdatastr)
{
	union {
		char	*ptr_c;
		int		*ptr_i;
		long	*ptr_l;
		short	*ptr_s;
	} x;
	long	value = ((pdatastr == NULL) ? itemdata->defvalue : atol(pdatastr));
#if 0
	if (value < itemdata->defvalue) {
		return(0);
	}
#endif
	x.ptr_c = (pUser + itemdata->itemposi);
	if (itemdata->itemsize == sizeof(int)) {
		*(x.ptr_i) = value;
	} else if (itemdata->itemsize == sizeof(long)) {
		*(x.ptr_l) = value;
	} else if (itemdata->itemsize == sizeof(short)) {
		*(x.ptr_s) = value;
	} else {
		return(-1);
	}
	return(0);
}

/*****************************************************************************
 * Config파일의 각 ITEM에 대한 숫자형 데이타를 얻음
 * arg(I) 1. char *pUser : pUser 명
 *        2. itemdata_t *itemdata : 구성요소 구조체
 *        3. char *pdatastr : 구성요소 구조체에 등록할 값
 * return 1. char* : 구성된 문자열 포인트
 *****************************************************************************/
static	char   *VIEW_NUMBER(char *pUser,itemdata_t *itemdata,char *pdatastr)
{
	union {
		char	*ptr_c;
		int		*ptr_i;
		long	*ptr_l;
		short	*ptr_s;
	} x;
	long	value;
	x.ptr_c = (pUser + itemdata->itemposi);
	if (itemdata->itemsize == sizeof(int)) {
		value = (long)(*(x.ptr_i));
	} else if (itemdata->itemsize == sizeof(long)) {
		value = (long)(*(x.ptr_l));
	} else if (itemdata->itemsize == sizeof(short)) {
		value = (long)(*(x.ptr_s));
	} else {
		value = 0;
	}
	if ( value ) sprintf(pdatastr, "%ld", value);
	return(pdatastr);
}

/*****************************************************************************
 * Config파일의 등록된 ITEM이 아닌 경우 사용함수
 * arg(I) 1. char *pUser : pUser 명
 *        2. itemdata_t *itemdata : 구성요소 구조체
 *        3. char *pdatastr : 구성요소 구조체에 등록할 값
 * return 1. '0'으로 Fixed
 *****************************************************************************/
static	int		ITEM_NOTDEF(char *pUser,itemdata_t *itemdata,char *pdatastr)
{
	fprintf(stdout, "Invalid Line:[%s = %s]\n", itemdata->itemname, pdatastr);
	return(0);
}

/*****************************************************************************
 * Config파일의 등록된 ITEM이 아닌 경우 사용함수
 * arg(I) 1. char *pUser : pUser 명
 *        2. itemdata_t *itemdata : 구성요소 구조체
 *        3. char *pdatastr : 구성요소 구조체에 등록할 값
 * return 1. NULL값으로 Fixed
 *****************************************************************************/
static	char   *VIEW_NOTDEF(char *pUser,itemdata_t *itemdata,char *pdatastr)
{
	return("");
}

/*****************************************************************************
 * Config파일의 데이타를 Setting Define문으로 각 ITEM의 데이타 및 함수명을 지정
 * arg(I) 1. itemname : ITEM 명
 *        2. itemdata : ITEM 데이타
 *        3. funcbasc : ITEM의 데이타를 구성하기 위한 함수명
 *        4. defval   : ITEM 데이타의 Default 값
 ****************************************************************************/


#define	SET_ITEMDATA(itemname,itemdata,funcbase,defval)	{	\
			itemname, 										\
			offsetof(SUserParm_T, itemdata),				\
			sizeof(((SUserParm_T *)0)->itemdata),			\
			ITEM_##funcbase,								\
			VIEW_##funcbase,								\
			defval											\
}
/*
#define	SET_ITEMDATA(itemname,itemdata,funcbase,defval)	{	\
			itemname, 										\
			offsetof(SUserParm_T, itemdata),				\
			sizeof(((SUserParm_T *)0)->##itemdata),			\
			ITEM_##funcbase,								\
			VIEW_##funcbase,								\
			defval											\
}*/
/*
** ITEM데이타가 아닌 경우의 값을 지정, 전역변수 itemdata_not의 초기값
*/
static	itemdata_t	itemdata_not[] = {
	SET_ITEMDATA("????????", szProcName, NOTDEF,    0),
};
/*
** ITEM데이타의 초기값을 설정, 전역변수 itemdata_tab
*/
static	itemdata_t	itemdata_tab[] = {
	SET_ITEMDATA("PROCNAME", szProcName, STRING, 0x00),
	SET_ITEMDATA("APPSTATE",  nAppState, NUMBER,    1),
	SET_ITEMDATA("MAXPSCNT",  nMaxPsCnt, NUMBER,    1),
	SET_ITEMDATA("LOGLEVEL",  nLogLevel, NUMBER,    0),
	SET_ITEMDATA("ERRORLOG",  nErrorLog, NUMBER,    0),
	SET_ITEMDATA("LOGFBASE", szLogFBase, STRING, 0x00),
	SET_ITEMDATA("SER_ZONE", szServer_zone, STRING, 0x00),

	SET_ITEMDATA("DBUSERID", szDBUserId, STRING, 0x00),
	SET_ITEMDATA("DBPASSWD", szDBPassWd, STRING, 0x00),
	SET_ITEMDATA("DBCONSTR", szDBConStr, STRING, 0x00),

	SET_ITEMDATA("RTUSERID", szRTUserId, STRING, 0x00),
	SET_ITEMDATA("RTPASSWD", szRTPassWd, STRING, 0x00),
	SET_ITEMDATA("RTCONSTR", szRTConStr, STRING, 0x00),

	SET_ITEMDATA("DATFPATH", szDatFPath, STRING, 0x00),
	SET_ITEMDATA("OUTFNAME", szOutFName, STRING, 0x00),
	SET_ITEMDATA("INPFNAME", szInpFName, STRING, 0x00),
	SET_ITEMDATA("SEQFNAME", szSeqFName, STRING, 0x00),

	SET_ITEMDATA("LOCALSID", szLocalSid, STRING, 0x00),
	SET_ITEMDATA("OTHERSYS", szOtherSys, STRING, 0x00),

	SET_ITEMDATA("IPCSEMKY",  nIpcSemKy, NUMBER,   -1),
	SET_ITEMDATA("IPCMSGKY",  nIpcMsgKy, NUMBER,   -1),
	SET_ITEMDATA("IPCSHMKY",  nIpcShmKy, NUMBER,   -1),
	SET_ITEMDATA("IPCSEMID",  nIpcSemId, NUMBER,   -1),
	SET_ITEMDATA("IPCMSGID",  nIpcMsgId, NUMBER,   -1),
	SET_ITEMDATA("IPCSHMID",  nIpcShmId, NUMBER,   -1),

	SET_ITEMDATA("HOSTNAME", szHostName, STRING, 0x00),
	SET_ITEMDATA("SOCKADDR", szSockAddr, STRING, 0x00),
	SET_ITEMDATA("SOCKPORT",  nSockPort, NUMBER,    0),
	SET_ITEMDATA("SOCKSIZE",  nSockSize, NUMBER, 4096),
	SET_ITEMDATA("SOCKETID",  nSocketId, NUMBER,   -1),
	SET_ITEMDATA("LISTENFD",  nListenFd, NUMBER,   -1),

	SET_ITEMDATA("REGIONID", szRegionId, STRING, 0x00),
	SET_ITEMDATA("REGIONNM", szRegionNm, STRING, 0x00),
	SET_ITEMDATA("REGIONIP", szRegionIp, STRING, 0x00),
	SET_ITEMDATA("SERVERNM", szServerNm, STRING, 0x00),
	SET_ITEMDATA("SERVERIP", szServerIp, STRING, 0x00),
	SET_ITEMDATA("INETPORT",  nInetPort, NUMBER,    0),
	SET_ITEMDATA("SVRALIAS", szSvrAlias, STRING, 0x00),

	SET_ITEMDATA("TTY_NAME",  szTtyName, STRING, 0x00),
	SET_ITEMDATA("IO_SPEED",  nIoSpeeds, NUMBER, 4800),
	SET_ITEMDATA("BIT_TYPE",  nBitTypes, NUMBER,    7),
	SET_ITEMDATA("ISPARITY",  nIsParity, NUMBER,    1),
	SET_ITEMDATA("RS232CID",  nRs232cId, NUMBER,   -1),

	SET_ITEMDATA("EXECMODE", szExecMode, STRING, 0x00),
	SET_ITEMDATA("TIMEDSEC",  nTimedSec, NUMBER,    0),
	SET_ITEMDATA("RETRYCNT",  nRetryCnt, NUMBER,    3),
	SET_ITEMDATA("WAITSECS",  nWaitSecs, NUMBER,    5),
	SET_ITEMDATA("RETRYSEC",  nRetrySec, NUMBER,    5),
	SET_ITEMDATA("INTERVAL",  nInterval, NUMBER,    5),
	SET_ITEMDATA("THRCOUNT",  nThrCount, NUMBER,    0),

	SET_ITEMDATA("FORMATID", szFormatId, STRING, 0x00),

	SET_ITEMDATA("EACHWORK", asEachWork, ADDSUB, 0x00),
	SET_ITEMDATA("USEFILES", asUseFiles, ADDUSE, 0x00),
	SET_ITEMDATA("PVC_NAME", asEachWork, ADDPVC, 0x00),
	SET_ITEMDATA("ASVRADDR", szASvrAddr , STRING, 0x00),
	SET_ITEMDATA("ASVRPORT", nASvrPort , NUMBER, 0x00),
	SET_ITEMDATA("BSVRADDR", szBSvrAddr , STRING, 0x00),
	SET_ITEMDATA("BSVRPORT", nBSvrPort , NUMBER, 0x00),
};
/*****************************************************************************
 * 전역변수(itemdata_tab)에서 ITEM의 Data포인트를 얻음
 * arg(I) 1. char itemname[] : 찾고자 한 ITEM명
 * return 1. 성공 : itemdata_t *itemdata
 *        2. 실패 : 전역변수(itemdata_not)
 ****************************************************************************/
static	itemdata_t	*FND_ITEMDATA(char itemname[])
{
	itemdata_t	*itemdata;
	int			totcount;
	itemdata = &itemdata_tab[0];
	totcount = ITEM_COUNT(itemdata_tab);
	while (totcount-- > 0) {
		if (strcmp(itemdata->itemname, itemname) == 0) {
			return(itemdata);
		}
		itemdata++;
	}
	return(itemdata_not);
}

/*****************************************************************************
* Config파일로 부터 ITEM의 초기값을 설정
* (B) SUserParm_T *pUser
* (R) int : '0'  => 정상
*           음수 => 실패
*****************************************************************************/
int		infSetUserParm(SUserParm_T *pUser, int argc, char **argv)
{
	char	szFileName[256];
	char	*pPath;

	

	gpAppName = argv[0];
	infCpyUserParm(pUser);
/*
	if ( argc == 2 ) {
		sprintf(szFileName, "%s", argv[1]);
	} else {
*/
		if ( (pPath = getenv("CFG_DIR")) != NULL ) {
			sprintf(szFileName, "%s/%s.cfg", pPath, argv[0]);
		} else {
			sprintf(szFileName, "%s.cfg", argv[0]);
		}
/*
	}
*/	
#if 0
fprintf(stdout, "CONFIG FILE NAME[%s]\n", szFileName);
#endif
	return infSetUserparm(pUser, szFileName);
}

int		infSetUserparm(SUserParm_T *pUser, char *szFileName)
{
	FILE		*fp;
	itemdata_t	*itemdata;
	char		readbuff[1024];
	int			totcount;
	int			rc = 0;

	memset(pUser, 0x00, sizeof(SUserParm_T));
	
	if ( (fp = fopen(szFileName, "r")) == NULL ) {
		fprintf(stdout, "에러: 초기화 화일 열기에러(%s)\n", szFileName);
		exit(-1);
		return RETNG;
	}
	
	totcount = ITEM_COUNT(itemdata_tab);
	itemdata = itemdata_tab;
	while (totcount-- > 0) {
		(*itemdata->itemfunc)((char *)pUser, itemdata, NULL);
		itemdata++;
	}

	while(1) {
		char	*pitemstr, *pdatastr;
		memset(readbuff, 0x00, sizeof(readbuff));
		if ( fgets(readbuff, sizeof(readbuff), fp) == NULL ) break;
		
		if ( GET_ITEMDATA(readbuff, &pitemstr, &pdatastr) != 0 ) {
			continue;
		}

		itemdata = FND_ITEMDATA(pitemstr);
		rc = (*itemdata->itemfunc)((char *)pUser, itemdata, pdatastr);
		if ( rc != 0 ) {
			memmove(readbuff, pdatastr, strlen(pdatastr) + 1);
			break;
		}			
	}

	fclose(fp);

	totcount = ITEM_COUNT(itemdata_tab);
    itemdata = itemdata_tab;
    while (totcount-- > 0) {
   		char*	pdatastr, numbstrs[sizeof(SUserParm_T)];
   		pdatastr = (*itemdata->viewfunc)((char *)pUser, itemdata, numbstrs);
   		if (*pdatastr != 0x00) {
#ifdef _CONFIG_LOG_
			infLOG(TRACE, "CONFIG`%s=[%s]\n", itemdata->itemname, pdatastr);
#endif
   		}

   		itemdata++;
    }

	return RETOK;
}

/*****************************************************************************
* 통합서버의 Server_zone_id를 얻음
* (I) SUserParm_T *pUser
* (R) char * : Process Name
*****************************************************************************/
char	*infGetServer_zone(SUserParm_T *pUser)
{
	return (char*)pUser->szServer_zone;
}

void 	 infSetServer_zone(SUserParm_T *pUser, char *szValue)
{
	sprintf(pUser->szServer_zone, "%s", szValue);
}



/*****************************************************************************
* Application Process Name을 얻음
* (I) SUserParm_T *pUser
* (R) char * : Process Name
*****************************************************************************/
char	*infGetProcName(SUserParm_T *pUser)
{
#if 1
	return (char*)pUser->szProcName;
#endif
	return (char*)gpAppName;
}

void 	 infSetProcName(SUserParm_T *pUser, char *szValue)
{
	sprintf(pUser->szProcName, "%s", szValue);
}

int			infGetAppState(SUserParm_T *pUser)				{ return (pUser->nAppState); }
void		infSetAppState(SUserParm_T *pUser, int nValue)	{ pUser->nAppState = nValue; }

/*****************************************************************************
* 동일 프로세스 실행 가능한 최대 수 
* (I) SUserParm_T *pUser
* (R) int	: 카운트 수
*****************************************************************************/
int		infGetMaxPsCnt(SUserParm_T *pUser)
{
	return pUser->nMaxPsCnt;
}

/*****************************************************************************
* Log Level을 얻음
* (I) SUserParm_T *pUser
* (R) int : Log Level
*****************************************************************************/
int		infGetLogLevel(SUserParm_T *pUser)
{
	return pUser->nLogLevel;
}

/*****************************************************************************
* Error Log File 생성 여부값을 얻음
* (I) SUserParm_T *pUser
* (R) int : Error Log File 생성여부 (1:생성, 0:안함)
*****************************************************************************/
int		infGetErrorLog(SUserParm_T *pUser)
{
	return pUser->nErrorLog;
}

/*****************************************************************************
* Log File Base를 얻음
* (I) SUserParm_T *pUser
* (R) char * : Log File Base
*****************************************************************************/
char	*infGetLogFBase(SUserParm_T *pUser)
{
	return (char*)pUser->szLogFBase;
}

/*****************************************************************************
* DATABASE User ID를 얻음
* (I) SUserParm_T *pUser
* (R) char * : DATABASE User ID
*****************************************************************************/
char	*infGetDBUserId(SUserParm_T *pUser)
{
	return (char*)pUser->szDBUserId;
}
/*****************************************************************************
* DATABASE Password를 얻음
* (I) SUserParm_T *pUser
* (R) char * : DATABASE Password
*****************************************************************************/
char	*infGetDBPassWd(SUserParm_T *pUser)
{
	return (char*)pUser->szDBPassWd;
}
/*****************************************************************************
* UserId/PassWd@ConStr 얻음
* (I) SUserParm_T *pUser
* (R) char * : UserId/PassWd@ConStr
*****************************************************************************/
char	*infGetDBConStr(SUserParm_T *pUser)
{
	return (char*)pUser->szDBConStr;
}
/*****************************************************************************
* DATABASE Connect Status를 얻음
* (I) SUserParm_T *pUser 
* (R) int : '1' : DB_CONNECT
*           '0' : DB_RELEASE
*****************************************************************************/
int		infGetDBStatus(SUserParm_T *pUser)
{
	return pUser->nDBStatus;
}

/*****************************************************************************
* DATABASE Connect Status를 설정
* (I) 1. SUserParm_T *pUser 
*     2. int : '1' : DB_CONNECT
*              '0' : DB_RELEASE
* (R) void
*****************************************************************************/
int		infSetDBStatus(SUserParm_T *pUser, int nStatus)
{
	pUser->nDBStatus = nStatus;
	return pUser->nDBStatus;
}

/*****************************************************************************
* Remote DATABASE User ID를 얻음
* (I) SUserParm_T *pUser
* (R) char * : DATABASE User ID
*****************************************************************************/
char	*infGetRTUserId(SUserParm_T *pUser)
{
	return (char*)pUser->szRTUserId;
}

/*****************************************************************************
* Remote DATABASE Password를 얻음
* (I) SUserParm_T *pUser
* (R) char * : DATABASE Password
*****************************************************************************/
char	*infGetRTPassWd(SUserParm_T *pUser)
{
	return (char*)pUser->szRTPassWd;
}

/*****************************************************************************
* Remote DATABASE Connect String 얻음
* (I) SUserParm_T *pUser
* (R) char * : DATABASE Connect String
*****************************************************************************/
char	*infGetRTConStr(SUserParm_T *pUser)
{
	return (char*)pUser->szRTConStr;
}

/*****************************************************************************
* Remote DATABASE Connect Status를 얻음
* (I) SUserParm_T *pUser 
* (R) int : '1' : DB_CONNECT
*           '0' : DB_RELEASE
*****************************************************************************/
int		infGetRTStatus(SUserParm_T *pUser)
{
	return pUser->nRTStatus;
}

/*****************************************************************************
* Remote DATABASE Connect Status를 설정
* (I) 1. SUserParm_T *pUser 
*     2. int : '1' : DB_CONNECT
*              '0' : DB_RELEASE
* (R) void
*****************************************************************************/
void	infSetRTStatus(SUserParm_T *pUser, int nStatus)
{
	pUser->nRTStatus = nStatus;
}

/*****************************************************************************
* DATA FILE PATH얻음
* (I) SUserParm_T *pUser
* (R) char * : DATA FILE PATH
*****************************************************************************/
char	*infGetDatFPath(SUserParm_T *pUser)
{
	return (char*)pUser->szDatFPath;
}

/*****************************************************************************
* OUTPUT FILE NAME String 얻음
* (I) SUserParm_T *pUser
* (R) char * : OUTPUT FILE NAME String
*****************************************************************************/
char	*infGetOutFName(SUserParm_T *pUser)
{
	return (char*)pUser->szOutFName;
}

/*****************************************************************************
* INPUT FILE NAME String 얻음
* (I) SUserParm_T *pUser
* (R) char * : INPUT FILE NAME String
*****************************************************************************/
char	*infGetInpFName(SUserParm_T *pUser)
{
	return (char*)pUser->szInpFName;
}

/*****************************************************************************
* 일련번호 화일명를 얻음
* (I) SUserParm_T *pUser
* (R) char * : 일련번호관리 화일명
*****************************************************************************/
char	*infGetSeqFName(SUserParm_T *pUser)
{
	return pUser->szSeqFName;
}

/*****************************************************************************
* LOCAL SERVER ID
* (I) SUserParm_T *pUser
* (R) char * : LOCAL SERVER ID String
*****************************************************************************/
char	*infGetLocalSid(SUserParm_T *pUser)
{
	return (char*)pUser->szLocalSid;
}

/*****************************************************************************
* 상대시스템명
* (I) SUserParm_T *pUser
* (R) char * : 상대시스템명
*****************************************************************************/
char	*infGetOtherSys(SUserParm_T *pUser)
{
	return (char*)pUser->szOtherSys;
}

/*****************************************************************************
* IPC 관련 변수 
*****************************************************************************/
int     infGetIpcSemKy(SUserParm_T *pUser) { return pUser->nIpcSemKy; } /* Semaphore Key */
int     infGetIpcMsgKy(SUserParm_T *pUser) { return pUser->nIpcMsgKy; } /* Message Queue Key */
int     infGetIpcShmKy(SUserParm_T *pUser) { return pUser->nIpcShmKy; } /* Shared memory Key */
int     infGetIpcSemId(SUserParm_T *pUser) { return pUser->nIpcSemId; } /* Semaphore ID */
int		infGetIpcMsgId(SUserParm_T *pUser) { return pUser->nIpcMsgId; } /* Message Queue ID */
int		infGetIpcShmId(SUserParm_T *pUser) { return pUser->nIpcShmId; } /* Shared memory ID */
void    infSetIpcSemId(SUserParm_T *pUser, int nId) { pUser->nIpcSemId = nId; }   /* Semaphore ID */
void    infSetIpcMsgId(SUserParm_T *pUser, int nId) { pUser->nIpcMsgId = nId; }   /* Message Queue ID */
void    infSetIpcShmId(SUserParm_T *pUser, int nId) { pUser->nIpcShmId = nId; }   /* Shared memory ID */

/*****************************************************************************
* Host Name를 얻음
* (I) SUserParm_T *pUser
* (R) char * : Host Name
*****************************************************************************/
char 	*infGetHostName(SUserParm_T *pUser)
{
	return pUser->szHostName;
}

/*****************************************************************************
* Server Ip Address를 얻음
* (I) SUserParm_T *pUser
* (R) char * : Server Ip Address
*****************************************************************************/
char 	*infGetSockAddr(SUserParm_T *pUser)
{
	return pUser->szSockAddr;
}
/*****************************************************************************
* Server Ip Address를 얻음 ( A SERVER )
* (I) SUserParm_T *pUser
* (R) char * : Server Ip Address
*****************************************************************************/
char 	*infGetASvrAddr(SUserParm_T *pUser)
{
	return pUser->szASvrAddr;
}
/*****************************************************************************
* Server Ip Address를 얻음 ( B SERVER )
* (I) SUserParm_T *pUser
* (R) char * : Server Ip Address
*****************************************************************************/
char 	*infGetBSvrAddr(SUserParm_T *pUser)
{
	return pUser->szBSvrAddr;
}

/*****************************************************************************
* Socket Port No를 얻음
* (I) SUserParm_T *pUser
* (R) int : Socket Port No
*****************************************************************************/
int		infGetSockPort(SUserParm_T *pUser)
{
	return pUser->nSockPort;
}

/*****************************************************************************
* Socket Port No를 얻음 ( A SERVER )
* (I) SUserParm_T *pUser
* (R) int : Socket Port No
*****************************************************************************/
int		infGetASvrPort(SUserParm_T *pUser)
{
	return pUser->nASvrPort;
}

/*****************************************************************************
* Socket Port No를 얻음 ( B SERVER )
* (I) SUserParm_T *pUser
* (R) int : Socket Port No
*****************************************************************************/
int		infGetBSvrPort(SUserParm_T *pUser)
{
	return pUser->nBSvrPort;
}

/*****************************************************************************
* Socket Buffer Size를 얻음
* (I) SUserParm_T *pUser
* (R) int : Socket Buffer Size
*****************************************************************************/
int		infGetSockSize(SUserParm_T *pUser)
{
	return pUser->nSockSize;
}

/*
** 2001-05-25 삽입 (lhu)
** 택배시스템 
*/
/*****************************************************************************
* SUserParm의 SSpatssInfo_T형의 구조체에 각 값을 세팅 또는 구해온다.
* 스레드 사용을 위해...
* (I) 1.SUserParm_T *pUser
*     2.int  nCnt 구조체 배열의 위치
*****************************************************************************/

int		infSetSpatsSocketId(SUserParm_T *pUser, int nCnt, int nValue) 
{ 
	pUser->asSpats[nCnt].nSocketId = nValue; 
	return (pUser->asSpats[nCnt].nSocketId) ; 
}
int		infGetSpatsSocketId(SUserParm_T *pUser, int nCnt) 
{ 
	return (pUser->asSpats[nCnt].nSocketId) ; 
}
int		infSetSpatsWorkFlag(SUserParm_T *pUser, int nCnt, int nValue) 
{ 
	pUser->asSpats[nCnt].nWorkFlag = nValue; 
	return (pUser->asSpats[nCnt].nWorkFlag) ; 
}
int		infGetSpatsWorkFlag(SUserParm_T *pUser, int nCnt) 
{ 
	return (pUser->asSpats[nCnt].nWorkFlag) ; 
}
int		infSetSpatsPeerAddr(SUserParm_T *pUser, int nCnt, char *szAddr) 
{ 
	int nLen;
	memset(pUser->asSpats[nCnt].szPeerAddr,	0x00,	sizeof(pUser->asSpats[nCnt].szPeerAddr));
	nLen = sprintf(pUser->asSpats[nCnt].szPeerAddr,"%s",szAddr); ; 
	return (nLen) ; 
}
char*	infGetSpatsPeerAddr(SUserParm_T *pUser, int nCnt) 
{ 
	return (pUser->asSpats[nCnt].szPeerAddr) ; 
}
int		infSetSpatsASvrSockId(SUserParm_T *pUser, int nCnt, int nValue) 
{ 
	pUser->asSpats[nCnt].nASvrSockId = nValue; 
	return (pUser->asSpats[nCnt].nASvrSockId) ; 
}
int		infSetSpatsBSvrSockId(SUserParm_T *pUser, int nCnt, int nValue) 
{ 
	pUser->asSpats[nCnt].nBSvrSockId = nValue; 
	return (pUser->asSpats[nCnt].nBSvrSockId) ; 
}
int		infGetSpatsASvrSockId(SUserParm_T *pUser, int nCnt) 
{ 
	return (pUser->asSpats[nCnt].nASvrSockId) ; 
}
int		infGetSpatsBSvrSockId(SUserParm_T *pUser, int nCnt) 
{ 
	return (pUser->asSpats[nCnt].nBSvrSockId) ; 
}
/*
** 삽입 끝
*/

int    	infSetSocketId(SUserParm_T *pUser, int nValue) { pUser->nSocketId = nValue; return (pUser->nSocketId); }
int     infGetSocketId(SUserParm_T *pUser) { return (pUser->nSocketId); }
int     infSetListenFd(SUserParm_T *pUser, int nValue) { pUser->nListenFd = nValue; return (pUser->nListenFd); }
int     infGetListenFd(SUserParm_T *pUser) { return (pUser->nListenFd); }

/*****************************************************************************
* Socket Connect ID/송수신 상태 설정 및 얻음
* (I) SUserParm_T *pUser
* (R) int : Socket Connect ID/송수신 상태
*****************************************************************************/
void    infSetSendRtns(SUserParm_T *pUser, int nValue) { pUser->nSendRtns = nValue; }
void    infSetRecvRtns(SUserParm_T *pUser, int nValue) { pUser->nRecvRtns = nValue; }
void    infSetDbmsRtns(SUserParm_T *pUser, int nValue) { pUser->nDbmsRtns = nValue; }
void    infSetExcpRtns(SUserParm_T *pUser, int nValue) { pUser->nExcpRtns = nValue; }
void    infSetRespRtns(SUserParm_T *pUser, int nValue) { pUser->nRespRtns = nValue; }
void    infSetTempRtns(SUserParm_T *pUser, int nValue) { pUser->nTempRtns = nValue; }
void    infSetJobStart(SUserParm_T *pUser, int nValue) { pUser->nJobStart = nValue; }

int     infGetSendRtns(SUserParm_T *pUser) { return (pUser->nSendRtns); }
int     infGetRecvRtns(SUserParm_T *pUser) { return (pUser->nRecvRtns); }
int     infGetDbmsRtns(SUserParm_T *pUser) { return (pUser->nDbmsRtns); }
int     infGetExcpRtns(SUserParm_T *pUser) { return (pUser->nExcpRtns); }
int     infGetRespRtns(SUserParm_T *pUser) { return (pUser->nRespRtns); }
int     infGetTempRtns(SUserParm_T *pUser) { return (pUser->nTempRtns); }
int     infGetJobStart(SUserParm_T *pUser) { return (pUser->nJobStart); }

long    infGetSendNumb(SUserParm_T *pUser) { return (pUser->nSendNumb); }
long    infGetRecvNumb(SUserParm_T *pUser) { return (pUser->nRecvNumb); }

void    infIncSendNumb(SUserParm_T *pUser) { pUser->nSendNumb++; }
void    infIncRecvNumb(SUserParm_T *pUser) { pUser->nRecvNumb++; }


/*****************************************************************************
* 지역코드를 얻음
* (I) SUserParm_T *pUser
* (R) char * : 지역코드
*****************************************************************************/
char 	*infGetRegionId(SUserParm_T *pUser) { return pUser->szRegionId; }
char 	*infGetRegionNm(SUserParm_T *pUser) { return pUser->szRegionNm; }
char 	*infGetRegionIp(SUserParm_T *pUser) { return pUser->szRegionIp; }
char 	*infGetServerNm(SUserParm_T *pUser) { return pUser->szServerNm; }
char 	*infGetServerIp(SUserParm_T *pUser) { return pUser->szServerIp; }
int		 infGetInetPort(SUserParm_T *pUser) { return pUser->nInetPort;  }
char	*infGetSvrAlias(SUserParm_T *pUser) { return pUser->szSvrAlias; }

void 	 infSetRegionId(SUserParm_T *pUser, char *szValue) { sprintf( pUser->szRegionId, "%s", szValue); }
void 	 infSetRegionNm(SUserParm_T *pUser, char *szValue) { sprintf( pUser->szRegionNm, "%s", szValue); }
void 	 infSetRegionIp(SUserParm_T *pUser, char *szValue) { sprintf( pUser->szRegionIp, "%s", szValue); }
void 	 infSetServerNm(SUserParm_T *pUser, char *szValue) { sprintf( pUser->szServerNm, "%s", szValue); }
void 	 infSetServerIp(SUserParm_T *pUser, char *szValue) { sprintf( pUser->szServerIp, "%s", szValue); }
void	 infSetInetPort(SUserParm_T *pUser, int    nValue) { pUser->nInetPort = nValue; }
void	 infSetSvrAlias(SUserParm_T *pUser, char *szValue) { sprintf( pUser->szSvrAlias, "%s", szValue); }


/*****************************************************************************
* RSC232C 정보를 얻음
* (I) SUserParm_T *pUser
* (R)
******************************************************************************/
char*   infGetTtyNames(SUserParm_T *pUser) { return pUser->szTtyName; }           /* 1. TTY NAME      */
int     infGetIoSpeeds(SUserParm_T *pUser) { return pUser->nIoSpeeds; }           /* 2. In/Out Speeds */
int     infGetBitTypes(SUserParm_T *pUser) { return pUser->nBitTypes; }           /* 3. Bit Type      */
int     infGetIsParity(SUserParm_T *pUser) { return pUser->nIsParity; }           /* 4. Parity Enable */
int     infGetRs232cId(SUserParm_T *pUser) { return pUser->nRs232cId; }           /* 5. ID */
int     infSetRs232cId(SUserParm_T *pUser, int nValue) { pUser->nRs232cId = nValue; return pUser->nRs232cId; }

/*****************************************************************************
* EXECUTE MODE을 얻음
* (I) SUserParm_T *pUser
* (R) int : EXECUTE MODE
*****************************************************************************/
int		infGetExecMode(SUserParm_T *pUser)
{
	if ( strcmp(pUser->szExecMode, "DEV") == 0 ) return (1);

	return (0);
}

/*****************************************************************************
* timeout sec 를 얻음
* (I) SUserParm_T *pUser
* (R) int : Timeout sec
*****************************************************************************/
int		infGetTimedSec(SUserParm_T *pUser) { return pUser->nTimedSec; }

/*****************************************************************************
* 송수신 에러발생시 재시도 카운트 수
* (I) SUserParm_T *pUser
* (R) int : Retry Count
*****************************************************************************/
int		infGetRetryCnt(SUserParm_T *pUser)
{
	return pUser->nRetryCnt;
}

/*****************************************************************************
* Wait Time(sec)을 얻음
* (I) SUserParm_T *pUser
* (R) int : Wait Time
*****************************************************************************/
int		infGetWaitSecs(SUserParm_T *pUser)
{
	return pUser->nWaitSecs;
}

/*****************************************************************************
* Reconnect Time을 얻음( ORACLE, TCPIP...)
* (I) SUserParm_T *pUser
* (R) int : Reconnect Time
*****************************************************************************/
int		infGetRetrySec(SUserParm_T *pUser)
{
	return pUser->nRetrySec;
}

/*****************************************************************************
* Interval Time을 얻음
* (I) SUserParm_T *pUser
* (R) int : Interval Time
*****************************************************************************/
int		infGetInterval(SUserParm_T *pUser)
{
	return pUser->nInterval;
}

/*****************************************************************************
* THREAD MAX COUNT을 얻음
* (I) SUserParm_T *pUser
* (R) int : Thread Max count
*****************************************************************************/
int		infGetThrCount(SUserParm_T *pUser)
{
	return pUser->nThrCount;
}

/*****************************************************************************
* EACHWORK pEach execute conut을 얻음
* (I) SUserParm_T *pUser
* (R) int : EACHWORK pEach execute conut
*****************************************************************************/
int		infGetExecNumb(SUserParm_T *pUser)
{
	return pUser->nExecNumb;
}

/*****************************************************************************
* EACHWORK conut을 얻음
* (I) SUserParm_T *pUser
* (R) int : EACHWORK conut
*****************************************************************************/
int		infGetEachNumb(SUserParm_T *pUser)
{
	return pUser->nEachNumb;
}

/*****************************************************************************
* USEFILES count을 얻음
* (I) SUserParm_T *pUser
* (R) int : USEFILES count
*****************************************************************************/
int		infGetFileNumb(SUserParm_T *pUser)
{
	return pUser->nFileNumb;
}

/*****************************************************************************
* 송수신 Format ID를 얻음
* (I) SUserParm_T *pUser
* (R) char * : FORMAT ID 
*****************************************************************************/
char	*infGetFormatId(SUserParm_T *pUser)
{
	return pUser->szFormatId;
}

int		infGetWorkFlag(SUserParm_T *pUser)				{ return pUser->nWorkFlag; }
int		infSetWorkFlag(SUserParm_T *pUser, int nValue)	{ pUser->nWorkFlag = nValue; return pUser->nWorkFlag; }

/*
** SThrInfos_T item 인자 값처리
*/
int		infGetThrNumb(SUserParm_T *pUser)	{ return pUser->nThrNumb;	}
void	infIncThrNumb(SUserParm_T *pUser)	{ pUser->nThrNumb++;		}
void	infDecThrNumb(SUserParm_T *pUser)	{ pUser->nThrNumb--;		}

int     infGetThrStatus(SThrInfos_T *pThri) { return pThri->nStatus;		}
int     infGetThrSocketId(SThrInfos_T *pThri) { return pThri->nSocketId;	}
char*   infGetThrPeerAddr(SThrInfos_T *pThri) { return pThri->szPeerAddr;	}

/*****************************************************************************
 * strcpy
 * 
 * 
 ****************************************************************************/
int strcpyA(char* pOutStr, char* pInStr, int nOutLen)
{
	//infLOG(ALWAY, "strcpyA: pInStr=[%s], nOutLen=[%d]\n", pInStr, nOutLen);
	if(pInStr == 0x00 || pInStr == NULL)
	{
		//infLOG(ERROR, "strcpy(%s, %s)\n", pOutStr, "NULL");
		strcpy(pOutStr, " ");
		return -1;
	}
	else if(nOutLen > 0)
	{
		int nInLen = strlen(pInStr);
		if( nOutLen < nInLen)
		{
			//infLOG(ERROR, "pOutStr[%d] < %s[%d]\n", nOutLen, pInStr, nInLen);
			strcpy(pOutStr, " ");
			return -2;
		}
	}

	strcpy(pOutStr, pInStr);
	return 0;
}




/******************************************************************************
 * End of file...
 *****************************************************************************/

