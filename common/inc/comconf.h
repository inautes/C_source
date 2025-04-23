/******************************************************************************
 *   서브시스템 : 공통모듈
 *   프로그램명 : cmdconf.h
 *         기능 : 초기화 함수 헤더화일
 *         설명 :
 *       작성자 : JDP
 *       작성일 : 2004/02/17
 *     수정이력 :
 *
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/

#ifndef _CMDCONF_
#define	_CMDCONF_

char*	infSkipSpace(char *pstr);
char*	infTailSpace(char *pstr);

/*
** SUserParm_T items
*/
int		infSetUserParm(SUserParm_T *pUser, int argc, char **agrv);
int		infSetUserparm(SUserParm_T *pUser, char *szFileName);
char*	infGetServer_zone(SUserParm_T *pUser);
void 	infSetServer_zone(SUserParm_T *pUser, char *szValue);
char*	infGetProcName(SUserParm_T *pUser);
void	infSetProcName(SUserParm_T *pUser, char *szValue);
int     infGetAppState(SUserParm_T *pUser);
void    infSetAppState(SUserParm_T *pUser, int nValue);
int     infGetMaxPsCnt(SUserParm_T *pUser);
int     infGetLogLevel(SUserParm_T *pUser);
int     infGetErrorLog(SUserParm_T *pUser);
char*	infGetLogFBase(SUserParm_T *pUser);

char*	infGetDBUserId(SUserParm_T *pUser);
char*	infGetDBPassWd(SUserParm_T *pUser);
char*	infGetDBConStr(SUserParm_T *pUser);
int		infGetDBStatus(SUserParm_T *pUser);
int 	infSetDBStatus(SUserParm_T *pUser, int nStatus);

char*	infGetRTUserId(SUserParm_T *pUser);
char*	infGetRTPassWd(SUserParm_T *pUser);
char*	infGetRTConStr(SUserParm_T *pUser);
int		infGetRTStatus(SUserParm_T *pUser);
void	infSetRTStatus(SUserParm_T *pUser, int nStatus);

char*	infGetDatFPath(SUserParm_T *pUser);
char*	infGetOutFName(SUserParm_T *pUser);
char*	infGetInpFName(SUserParm_T *pUser);

char*	infGetLocalSid(SUserParm_T *pUser);
char*	infGetOtherSys(SUserParm_T *pUser);

int		infGetIpcSemKy(SUserParm_T *pUser);   /* Semaphore Key */
int		infGetIpcMsgKy(SUserParm_T *pUser);   /* Message Queue Key */
int     infGetIpcShmKy(SUserParm_T *pUser);   /* Shared memory Key */
int		infGetIpcSemId(SUserParm_T *pUser);   /* Semaphore ID */
int		infGetIpcMsgId(SUserParm_T *pUser);   /* Message Queue ID */
int     infGetIpcShmId(SUserParm_T *pUser);   /* Shared memory ID */
void	infSetIpcSemId(SUserParm_T *pUser, int nId);   /* Semaphore ID */
void	infSetIpcMsgId(SUserParm_T *pUser, int nId);   /* Message Queue ID */
void	infSetIpcShmId(SUserParm_T *pUser, int nId);   /* Shared memory ID */

char*	infGetHostName(SUserParm_T *pUser);
char*	infGetSockAddr(SUserParm_T *pUser);
int		infGetSockPort(SUserParm_T *pUser);
int		infGetSockSize(SUserParm_T *pUser);
int		infGetRetryCnt(SUserParm_T *pUser);
int		infSetSocketId(SUserParm_T *pUser, int nValue);
int		infGetSocketId(SUserParm_T *pUser);
int		infSetListenFd(SUserParm_T *pUser, int nValue);
int		infGetListenFd(SUserParm_T *pUser);

/* 2001-05-25 삽입 (lhu) */
char*	infGetASvrAddr(SUserParm_T *pUser);
char*	infGetBSvrAddr(SUserParm_T *pUser);
int		infGetASvrPort(SUserParm_T *pUser);
int     infGetBSvrPort(SUserParm_T *pUser);

int     infSetSpatsSocketId(SUserParm_T *pUser, int nCnt, int nValue);
int     infGetSpatsSocketId(SUserParm_T *pUser, int nCnt);
int     infSetSpatsWorkFlag(SUserParm_T *pUser, int nCnt, int nValue);
int		infGetSpatsWorkFlag(SUserParm_T *pUser, int nCnt);
int     infSetSpatsPeerAddr(SUserParm_T *pUser, int nCnt, char *szAddr);
char*   infGetSpatsPeerAddr(SUserParm_T *pUser, int nCnt);

int     infSetSpatsASvrSockId(SUserParm_T *pUser, int nCnt, int nValue);
int		infGetSpatsASvrSockId(SUserParm_T *pUser, int nCnt);
int     infSetSpatsBSvrSockId(SUserParm_T *pUser, int nCnt, int nValue);
int		infGetSpatsBSvrSockId(SUserParm_T *pUser, int nCnt);
/* 삽입 끝 */

char*	infGetRegionNm(SUserParm_T *pUser);
char*	infGetRegionIp(SUserParm_T *pUser);

char*	infGetRegionId(SUserParm_T *pUser);
char*	infGetServerNm(SUserParm_T *pUser);
char*	infGetServerIp(SUserParm_T *pUser);
int		infGetInetPort(SUserParm_T *pUser);
char*	infGetSvrAlias(SUserParm_T *pUser);

void	infSetRegionNm(SUserParm_T *pUser, char *szValue);
void	infSetRegionIp(SUserParm_T *pUser, char *szValue);
void	infSetRegionId(SUserParm_T *pUser, char *szValue);
void	infSetServerNm(SUserParm_T *pUser, char *szValue);
void	infSetServerIp(SUserParm_T *pUser, char *szValue);
void	infSetInetPort(SUserParm_T *pUser, int    nValue);
void    infSetSvrAlias(SUserParm_T *pUser, char *szValue);

void	infSetSendRtns(SUserParm_T *pUser, int nValue);
void    infSetRecvRtns(SUserParm_T *pUser, int nValue);
void    infSetDbmsRtns(SUserParm_T *pUser, int nValue);
void    infSetExcpRtns(SUserParm_T *pUser, int nValue);
void    infSetRespRtns(SUserParm_T *pUser, int nValue);
void    infSetTempRtns(SUserParm_T *pUser, int nValue);
void    infSetJobStart(SUserParm_T *pUser, int nValue);
int		infGetSendRtns(SUserParm_T *pUser);
int     infGetRecvRtns(SUserParm_T *pUser);
int     infGetDbmsRtns(SUserParm_T *pUser);
int     infGetExcpRtns(SUserParm_T *pUser);
int     infGetRespRtns(SUserParm_T *pUser);
int     infGetTempRtns(SUserParm_T *pUser);
int     infGetJobStart(SUserParm_T *pUser);

char*	infGetTtyNames(SUserParm_T *pUser);			  /* 1. TTY NAME      */
int		infGetIoSpeeds(SUserParm_T *pUser);			  /* 2. In/Out Speeds */
int     infGetBitTypes(SUserParm_T *pUser);			  /* 3. Bit Type      */
int		infGetIsParity(SUserParm_T *pUser);			  /* 4. Parity Enable */
int		infGetRs232cId(SUserParm_T *pUser);			  /* 5. ID */
int		infSetRs232cId(SUserParm_T *pUser, int nValue); 

int		infGetTimedSec(SUserParm_T *pUser);
int		infGetExecMode(SUserParm_T *pUser);
int		infGetWaitSecs(SUserParm_T *pUser);
int		infGetRetrySec(SUserParm_T *pUser);
int		infGetInterval(SUserParm_T *pUser);
int		infGetThrCount(SUserParm_T *pUser);
int		infGetWorkFlag(SUserParm_T *pUser);
int		infSetWorkFlag(SUserParm_T *pUser, int nValue);

int		infGetExecNumb(SUserParm_T *pUser);
int		infGetEachNumb(SUserParm_T *pUser);
int		infGetFileNumb(SUserParm_T *pUser);

char*	infGetFormatId(SUserParm_T *pUser);
char*	infGetSeqFName(SUserParm_T *pUser);

long    infGetSendNumb(SUserParm_T *pUser);
long    infGetRecvNumb(SUserParm_T *pUser);
void    infIncSendNumb(SUserParm_T *pUser);
void    infIncRecvNumb(SUserParm_T *pUser);

int		infGetThrNumb(SUserParm_T *pUser);
void	infIncThrNumb(SUserParm_T *pUser);
void	infDecThrNumb(SUserParm_T *pUser);

/*
** SEachWork_T items
*/
void    infSetSubWorkFlag(SEachWork_T *pEach, int nValue);
void	infSetSubSendRtns(SEachWork_T *pEach, int nValue);
void    infSetSubRecvRtns(SEachWork_T *pEach, int nValue);
void    infSetSubDbmsRtns(SEachWork_T *pEach, int nValue);
void    infSetSubExcpRtns(SEachWork_T *pEach, int nValue);
void    infSetSubRespRtns(SEachWork_T *pEach, int nValue);
void    infSetSubTempRtns(SEachWork_T *pEach, int nValue);
void    infSetSubSocketId(SEachWork_T *pEach, int nValue);

int     infGetSubWorkFlag(SEachWork_T *pEach);
int		infGetSubSendRtns(SEachWork_T *pEach);
int     infGetSubRecvRtns(SEachWork_T *pEach);
int     infGetSubDbmsRtns(SEachWork_T *pEach);
int     infGetSubExcpRtns(SEachWork_T *pEach);
int     infGetSubRespRtns(SEachWork_T *pEach);
int     infGetSubTempRtns(SEachWork_T *pEach);
int		infGetSubSocketId(SEachWork_T *pEach);

char*   infGetSubModuleId(SEachWork_T *pEach);
char*   infGetSubRegionId(SEachWork_T *pEach);
char*   infGetSubServerNm(SEachWork_T *pEach);
char*   infGetSubServerIp(SEachWork_T *pEach);
int     infGetSubInetPort(SEachWork_T *pEach);
int     infGetSubLoadFlag(SEachWork_T *pEach);
char*   infGetSubSvrAlias(SEachWork_T *pEach);

long    infGetSubSendNumb(SEachWork_T *pEach);
long    infGetSubRecvNumb(SEachWork_T *pEach);
void    infIncSubSendNumb(SEachWork_T *pEach);
void    infIncSubRecvNumb(SEachWork_T *pEach);

char*   infGetPvcName(SEachWork_T *pEach);
int     infGetX25Port(SEachWork_T *pEach);
int     infGetX25Stat(SEachWork_T *pEach);
int     infGetX25Init(SEachWork_T *pEach);
int     infSetX25Port(SEachWork_T *pEach, int nValue);
int     infSetX25Stat(SEachWork_T *pEach, int nValue);
int     infSetX25Init(SEachWork_T *pEach, int nValue);

/*
** SThrInfos_T items
*/
int			infGetThrStatus(SThrInfos_T *pThri);
int			infGetThrSocketId(SThrInfos_T *pThri);
char*		infGetThrPeerAddr(SThrInfos_T *pThri);
void		strcpyA(char* pOutStr, char* pInStr);
int			strcpyA(char* pOutStr, char* pInStr, int nOutLen);

#endif /* define _APCONFIG_ */

/*****************************************************************************
 * End of file...
 *****************************************************************************/
