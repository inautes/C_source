/******************************************************************************
 *   서브시스템 : 공통모듈
 *   프로그램명 : apstruct.h
 *         기능 : 구조체 정의 헤더화일
 *         설명 :
 *       작성자 : JDP
 *       작성일 : 2004/02/17
 *     수정이력 :
 *
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/

#ifndef	_APSTRUCT_
#define _APSTRUCT_

#define	MAX_EACHWORK	15
#define	MAX_USEFILES	15
#define	MAX_MULTITHR	15

#define EACHWORK_SIZE	132
#define FILENAME_SIZE	132
#define	MAX_SPATSUSERS  1000

typedef enum {
    STAT_PGM_INIT = 1,
    STAT_PGM_TERM,
    STAT_NET_INIT,
    STAT_NET_TERM,
    STAT_NET_RECV,
    STAT_NET_SEND,
    STAT_TCP_INIT,
    STAT_TCP_TERM,
    STAT_TCP_RECV,
    STAT_TCP_SEND,
    STAT_X25_INIT,
    STAT_X25_TERM,
    STAT_X25_RECV,
    STAT_X25_SEND,
    STAT_MSG_INIT,
    STAT_MSG_TERM,
    STAT_MSG_RECV,
    STAT_MSG_SEND,
    STAT_ORA_INIT,
    STAT_ORA_READ,
    STAT_ORA_WRIT,
    STAT_ORA_TERM
} SEachStatus;

/*
** 에러 체크 구조체
*/
typedef struct {
	int			nCode;				/* 코드   */
	char		szMsg[1048];		/* 메시지 */
} SErrCheck_T;

/*
** Thread Infomation
*/
typedef struct {
    int			nStatus;    	/* 쓰레드 상태 */
    int			nSocketId;  	/* 클라이언트 통신포트 */
    char		szPeerAddr[20]; /* 클라이언트의 IpAddr */
} SThrInfos_T;

/*
** Sub Work Infomation
*/
typedef struct {
	int			nWorkFlag;		/* Sub Process Status   */
	int			nSendRtns;		/* Send thread Status */
	int			nRecvRtns;		/* Recv thread Status  */
	int			nDbmsRtns;		/* DBMS thread Status  */
	int			nExcpRtns;		/* Exception thread Status  */
	int			nRespRtns;		/* Response thread Status  */
	int			nTempRtns;		/* Temp thread Status  */
	int			nSocketId;		/* Socket ID */
	int			nThrIndex;		/* thread index */

	long		nSendNumb;		/* 송신 건수 */
	long		nRecvNumb;		/* 수신 건수 */
	
	/*
	** TCP/IP 
	*/
	char		szModuleId[80];	/* 0. Module Id */
	char		szRegionId[20];	/* 1. Region Id */
	char		szServerNm[20];	/* 2. Server Name */
	char		szServerIp[20];	/* 3. Server IP Address */
	int			 nInetPort;		/* 4. Network Port Number */
	char		szLoadFlag[10];	/* 5. Process Load (Y/N)  */
	char		szSvrAlias[80];	/* 6. Remarks Name */

	/*
	** RS232C
	*/
	char		szTtyName[80];	/* 1. TTY NAME      */
	int			nIoSpeed;		/* 2. In/Out Speeds */
	int			nBitTypes;		/* 3. Bit Type      */
	int			nIsParity;		/* 4. Parity Enable */
 	int			nRs232cId;		/* 5. ID */

	/*
	** X25
	*/
#ifdef _DIGITAL_
	x25rdoob_t	x25rdoob;   /* X25ReadOOB status */
#endif
	char		szPvcName[80];
	int			nX25Port;
	int			nX25Stat;
	int			nIniFlag;
	int			nCurStat;
	time_t		tCurTime;
	int			nPreStat;
	time_t		tPreTime;
	char		szErrMsg[1024];
} SEachWork_T;

/*
** 프로그램에서 사용하는 파일명
*/
typedef struct {
    int    		nFileMode;
    char   		szFileName[FILENAME_SIZE+1];
} SUseFiles_T;

/*
** 2001-05-25 생성(lhu)
** 택배시스템에서 스레드를 위해....
*/
typedef struct {
	int		nWorkFlag;			/* 스레드 구동여부 1 : 구동 , -1 : 구동안됨 */
	int		nSocketId;			/* 클라이언트 소켓 아이디 */
	char	szPeerAddr[30];   	/* 클라이언트 어드레스 */
	int		nASvrSockId;		/* A서버로 접속했을때의 소켓 ID */
	int		nBSvrSockId;		/* B서버로 접속했을때의 소켓 ID */
} SSpatsInfo_T;

/*
** 프로세스 구성 요소 및 전역변수
*/
typedef struct {
	char		szProcName[80];	/* Application Process Name */
	int			nAppState;		/* Application Process Exit */
	int			nMaxPsCnt;		/* 동일 프로세스 실행 가능한 최대 수 */
	int			nLogLevel;		/* Log Level */
	int			nErrorLog;		/* Error Log File Create */
	char		szLogFBase[80];	/* Log File Base */
	char		szDBUserId[16];	/* DATABASE UserID */
	char		szDBPassWd[16];	/* DATABASE Password */
	char        szServer_zone[80]; /* szServer_zone_id */
	char		szDBConStr[80];	/* UserId/PassWd@ConStr */
	int			nDBStatus;		/* DATABASE Connect Status */
	char		szRTUserId[16];	/* Remote DATABASE UserID */
	char		szRTPassWd[16];	/* Remote DATABASE Password */
	char		szRTConStr[16];	/* Remote DATABASE Connect String */
	int			nRTStatus;		/* Remote DATABASE Connect Status */

	char		szDatFPath[132];	/* DATA FILE PATH */
	char		szOutFName[132];	/* OUTPUT FILE NAME */
	char		szInpFName[132];	/* INPUT FILE NAME */
	char        szSeqFName[132];  /* 일련번호 관리 파일명 */
	
	char		szLocalSid[80];	/* Local Server ID */
	char		szOtherSys[80];			/* 상대시스템명 혹은 ID */

	/*
	** IPC Infomation
	*/
	int			nIpcSemKy;		/* Semaphore Key */
	int			nIpcMsgKy;		/* Message Queue Key */
	int			nIpcShmKy;		/* Shared memory Key */
	int			nIpcSemId;		/* Semaphore ID */
	int			nIpcMsgId;		/* Message Queue ID */
	int			nIpcShmId;		/* Shared memory ID */

	/*
	** TCP/IP INTERFACE SYSTEM ( APPLICATION IS SERVER )
	*/
	char		szHostName[80];	/* Host Name */
	char		szSockAddr[20];	/* Server IP Address */
	int			nSockPort;		/* Socket Port No */
	int			nSockSize;		/* Socket Buffer Size */
 	int			nSocketId;		/* Socket Connect ID */
 	int			nListenFd;		/* Socket Connect listen ID */

	int			nSendRtns;		/* Send thread Status */
	int			nRecvRtns;		/* Recv thread Status  */
	int			nDbmsRtns;		/* Temp thread Status  */
	int			nExcpRtns;		/* Exception thread Status  */
	int			nRespRtns;		/* Response thread Status  */
	int			nTempRtns;		/* Temp thread Status  */
	int			nJobStart;		/* 개시전문 thread Status  */

	/*
	** TCP/IP INTERFACE SYSTEM ( APPLICATION IS CLIENT )
	*/
	char		szRegionId[80];	/* 1. Serial Number */
	char		szRegionNm[80];	/* 2. Region Name */
	char		szRegionIp[20];	/* 3. Region Ip Address */
	char		szServerNm[20];	/* 4. Server Name */
	char		szServerIp[20];	/* 5. Server IP Address */
	int			 nInetPort;		/* 6. Network Port Number */
	char		szSvrAlias[80];	/* 7. Remarks Name */

	/*
	** RS232C
	*/
	char		szTtyName[80];	/* 1. TTY NAME      */
	int			nIoSpeeds;		/* 2. In/Out Speeds */
	int			nBitTypes;		/* 3. Bit Type      */
	int			nIsParity;		/* 4. Parity Enable */
 	int			nRs232cId;		/* 5. ID */

	/*
	** COMMON
	*/
	char		szExecMode[80];	/* Execute Mode */
	int			nTimedSec;		/* Socket Timount sec */
	int			nRetryCnt;		/* Socket Send or Read Retry Count */
	int			nWaitSecs;		/* Wait Time */
	int			nRetrySec;		/* Reconnect Time */
	int			nInterval;		/* Interval Time */
	int			nThrCount;		/* THREAD MAX COUNT */
	int			nWorkFlag;		/* Process Status   */

	char		szFormatId[80];	/* 송수신 Format ID */
	long		nSendNumb;		/* 송신 건수 */
	long		nRecvNumb;		/* 수신 건수 */

	int			nExecNumb;
	int			nEachNumb;
	SEachWork_T	asEachWork[MAX_EACHWORK];

	int			nFileNumb;
	SUseFiles_T	asUseFiles[MAX_USEFILES];

	int			nThrNumb;				/* Active Thread Count */
	SThrInfos_T	asThrInfos[MAX_MULTITHR];
/*
** 2001-06- 생성(lhu)
** 택배시스템에서 사용하기 위해 and EDI서버에서 DB 서버 AB를 선택하기 위해
*/
	SSpatsInfo_T	asSpats[MAX_SPATSUSERS];
	char			szASvrAddr[20];	 /* A서버 어드레스  */
	int				nASvrPort;
	char			szBSvrAddr[20];	 /* B서버 어드레스  */
	int				nBSvrPort;
} SUserParm_T;

/*
** SOCKET STANDARD FORMAT
*/
typedef struct {
	short int	iFlag;			/* 송수신 플래그 */
	short int	iLength;		/* 송수신 데이터 길이(Socket Data Length)   */
} SPacketHead_T;				/* 4 Byte */
 
typedef struct {
	char		szDate[8];		/* 처리일자 YYYYMMDD */
	char		szTime[6];		/* 처리시간 HHMMSS */
	short int	iIngType;		/* 처리종류 : 0x00(정상) */
	char		szReserved[12];	/* 예약 */
} SPacketBody_T;				/* 28 Byte */
	
typedef struct {
	char	szFormatId	[8];	/* Packet 구성 ID    */
	char	chType		;		/* 송수신 코드 : 'S'=전문개시 'Y':전문응답 'D'=데이터송신 'A'=응답(정상) 'N':응답(에러) */ 
	char	szReserved	[7];	/* 예약 */
	char	szBuffer[4000];
} SPacketData_T;

typedef struct {
	SPacketHead_T	stHead;	
	SPacketBody_T	stBody;	
	SPacketData_T	stData;	
} SPacket_T;

/*
** BATCH PROCESS COUNT
*/
typedef struct {
	int		nTotal;
	int		nSuccess;
	int		nError;
	int		nSkip;
} SBatchCnt_T;
	
#endif /* define _APSTRUCT_ */

/*****************************************************************************
 * End of file...
 *****************************************************************************/
