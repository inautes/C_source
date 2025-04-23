#ifndef _FUP_DEFINE
#define _FUP_DEFINE


#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif



#include <map>

using namespace std;

#include <pthread.h>        /* for POSIX threads */

#include "fdns3004.h"
#include "com9006.h"
#include "com9002.h"
#include "com9009.h"
#include "com9102.h"
#include "com9201.h"



#define FT_SELL 	11
#define FT_GURU 	12


#define FT_FOLDER 	0x001 // Folder 표시
#define FT_FILE 	0x002  // File 표시

#define FT_MYDISK 	0x003
#define FT_WEDISK 	0x004
#define FT_MYFILE 	0x005
//File Up/Down




#define RS_EOL 	5
#define RS_OK 	6
#define RS_ERR 	7
#define END 	-2

#define RS_FILE_REQUEST_FILE 				500 // 파일의 해더 정보 (준비)
#define RS_FILE_DATA_TRANSFER 				501 /// 파일의 실제 DATA  (전송)
#define RS_FILE_REQUEST_NEXT_FILE 			502
#define RS_FILE_REQUEST_NEXT_FILEINFO 		503
#define RS_FILE_CHECK_ID_OK 				505
#define RS_FILE_CHECK_ID_FAIL 				506
#define RS_FILE_REQUEST_FILE_FILINFO 		507
#define RS_FILE_REQUEST_CONTINUE 			509

#define RS_FILE_CHECK_ID					512
#define RS_FILE_REQUEST_ID_DISCONNECT 		513
#define RS_FILE_REQUEST_ID_DISCONNECT_OK 	514
#define RS_FILE_DATA_MUREKA_CHECK			515
#define RS_FILE_REQUEST_LIST 				516
#define RS_FDN_REQUEST_HOLD_TIME			517
#define RS_FILE_REQUEST_FILE_WITH_HOLD_TIME 518


#define RS_GRID_DATA_TRANSFER					521
#define RS_GRID_STOP							522
#define RS_GRID_FAIL							523
#define RS_GRID_NEXT_FILE				524
#define RS_GRID_COMPLETION				525
#define RS_FILE_REQUEST_FILE_WITH_HOLD_TIME_GRID 526
#define RS_FILE_REQUEST_FILE_GRID	527
#define RS_GRID_KEEPALIVE_CHECK	528
#define RS_GRID_DATA_TRANSFER_NEXT 529




//#define RS_FILE_DATA_SIGN_CHECK 508
//#define RS_FILE_HEAD 510
//#define RS_FILE_DATA_TRANSFER_OK 511



#define RS_CMD_REQUEST_USER_LIST 		10051
#define RS_ADMIN_REQUEST_ID_DISCONNECT			10513
#define RS_ADMIN_REQUEST_ID_DISCONNECT_OK		10514

#define RS_REQUEST_CYBER_MONEY 			3001
#define RS_REQUEST_CYBER_MONEY_READY 	3002

#define RS_GURU_FILE_DN 				3004
#define RS_GURU_FILE_DN_LIST 			30041


#define RECVBUF 1024*512//32
#define SENDBUF 1024*512//32

#define SOCKBUF 1024*4

/*----------------------------------------------------------------------------*/
/*   정액제 시간 정보 조회
/*----------------------------------------------------------------------------*/
typedef struct _DEAL_ID
{
	unsigned long dwDealNum;
}DEALID,*LPDEALID;
/*----------------------------------------------------------------------------*/
/*   정액제 시간 정보 조회
/*----------------------------------------------------------------------------*/

typedef struct __TIME
{
	int nYear;
	int nMonth;
	int nDay;
	int nHour;
	int nMinute;
	int nSecond;
}TIME,*LPTIME;

typedef struct _HOLD_TIME
{
	int nHoldTimeMode; //저액제 모드
	//년 월 일 시 분 초

	TIME stHoldStartTime; //정액제 시작 시간
	TIME stHoldEndTime;   //정액제 끝날 시간

	long lHoldCurTime;   //현재 접속 시간
//	TIME stHoldCurTime;   //현재 접속 시간

}HOLDTIME,*LPHOLDTIME;


typedef struct _GURUFILEHEAD
{
	double dCurrentSize;
	char szFullFileName[768]; //server측 패스와 이름
}GURUFILEHEAD,*LPGURUFILEHEAD;
typedef struct _CFDNLIST_R
{
	unsigned long dwConID;
}CFDNLIST_R,*LPCFDNLIST_R;


typedef struct _GURUFILEINFO
{
	int nFlag;
	char	local_file_path	   	  [511+1];  // 파일경로
	char    local_file_name       [255+1];  // 로컬파일이름
	char    server_file_path      [255+1];  // 서버파일경로
	char    server_file_name      [255+1];  // 서버파일경로
	CFDNS3004_R fdn3004;
}GURUFILEINFO,*LPGURUFILEINFO;


typedef struct _SCOPYINFO
{
	unsigned long dwID; //등록된 시퀀스 번호
	char DnDate[12];
}SCOPYINFO,*LPSCOPYINFO;


typedef struct _FILEGROUPINFO
{
	int nMode; //start(1) or end(2) or nothing(1과 2을 제외한 어떤 수)
	int nFileCount;
	int nListCount;
	unsigned long dwID; //할당 받은 시퀀스 번호
	unsigned long GroupID;
	char DnDate[12];
	double dGroupTotalSize;
	double dGroupDownSize;

}FILEGROUPINFO,LPFILEGROUPINFO;


typedef struct _FILEINFO
{
	int nTypeDisk; // 디스크 타입 (FG_MYDISK , FG_WEDISK)
	int nType; //파일타입 (FT_FOLDER or FT_FILE)
	int nDealType;//11 : 판매 - 700  , 12 : 공유 - mb 당 얼마
	unsigned long dwDealID; //deal number
	char szServerID[10];
	char szServerIP[16];
	long dwServerPort;
	unsigned long nNumber; // 컨텐츠 번호,서버 이름
	char szUserID[12+1];
	char szFileOwnerID[12+1];
	//char szSaveDate[256]; // 파일 날짜 .
	char szDownFileName[256];
	char szFileName[256]; // 파일이름
	char szSrcPath[512]; // 파일이 존재하는 폴더이름
	char szDownPath[512]; // 파일이 존재하는 폴더이름
	char szCerKey[48];
//	FILEGROUPINFO GroupInfo;
	double dFileSize; // 파일크기
}FILEINFO, *LPFILEINFO;  // 구룹 정보

//20120723
typedef struct _DN_DEAL_INFO_MUREKA
{
	unsigned long dwDealNo;
	unsigned long dwContId;
	unsigned long dwFileSeq;
	char	szContGu[3];
	char	szDealDate[16];
	char	szDefaultHash[40];
	char	szVideoHash[128];
	double	dFileSize;
	char	szCopyrightYn[2];
	char	szDnUser[13];
	char	szUpUser[13];
	char	szTemp[1];
//deal_no:**:cont_id:**:fileslit_seq:**:deal_date(20120612000000):**:default_hash:**:video_hash:**:FileSize:**:copyright_yn:**:DnUser:**:UpUser
}DN_DEAL_INFO_MUREKA,*LPDN_DEAL_INFO_MUREKA;
//20120723
typedef struct _DN_DEAL_INFO_MUREKA_KEY
{
	unsigned long dwDealNo;
	char	szContGu[3];
	unsigned long dwContId;
	unsigned long dwFileSeq;
	char	szFilteringKey[20];
	char	szStatus[3];
	char	szTemp[6];
//deal_no:**:cont_id:**:fileslit_seq:**:deal_date(20120612000000):**:default_hash:**:video_hash:**:FileSize:**:copyright_yn:**:DnUser:**:UpUser
}DN_DEAL_INFO_MUREKA_KEY,*LPDN_DEAL_INFO_MUREKA_KEY;


typedef struct _DNSCOPY
{
	int nTypeDisk; // 디스크 타입 (FG_MYDISK , FG_WEDISK)
	int nType; //파일타입 (FT_FOLDER or FT_FILE)
	int nMoneyType;//11 : 판매 - 700  , 12 : 공유 - mb 당 얼마
	long dwMoney; //돈 액수 // 메가 당, 건당
	char szServerID[10];
	char szServerIP[16];
	char szDestServerIP[16]; //복사될 서버 아이피
	//char szDestServerID[8]; //복사될 서버 아이피
	long dwServerPort;
	unsigned long nNumber; // 컨텐츠 번호,서버 이름
	char szUserID[12+1];
	char szFileOwnerID[12+1];
	char szDestName[256]; // 파일 날짜 .
	char szFileName[256]; // 파일이름
	char szSrcPath[512]; // 파일이 존재하는 폴더이름
	char szDownPath[512]; // 파일이 존재하는 폴더이름
	FILEGROUPINFO GroupInfo;
	double dFileSize; // 파일크기
}DNSCOPY, *LPDNSCOPY;

typedef struct _FILEHEAD
{
	unsigned long dwID;
	double dCurrentSize;
	char DnDate[12];
	char szFullFileName[768]; //server측 패스와 이름
}FILEHEAD,*LPFILEHEAD;


typedef struct _CSERVERINFO
{
	char szIP[16];
	long dwPort;
}CSERVERINFO,*LPCSERVERINFO;

typedef struct _User_List  //10051 struct
{
	int nNumber;
	char szUserIP[16];
	char szUserID[12+4];
	char szStartTime[30+2];
}USERLIST,*LPUSERLIST;

typedef struct _DIS_USER
{
	char szUserID[12];
}DISUSER,*LPDISUSER;



typedef struct _Com9xxx
{//164
	int nStatus ; // 0 init , 1 use
	CCOM9002_R com9002_R;
	CCOM9102_R com9102_R;
}COM9XXX,*LPCOM9XXX;


/* Structure of arguments to pass to client thread */
typedef struct ThreadArgs
{
    int clntSock;                      /* Socket descriptor for client */
    char userIP[16];
    char startTime[32];
}THREADINFO,*LPTHREADINFO;

typedef struct _UserInfo
{
	char szUserID[52];
	COM9XXX gCom9xxx; //164

	time_t			curtime;
	int  			nRealDown; // 0 (grid) , 1 (server)
	unsigned long 	temp_id;	// 컨테츠ID(T_CONTENTS_TEMP.id)
	char   	 		server_id[ 10];	// 서버ID
	char   	 		conn_ip[19+1];	// 접속ID

	pthread_t threadID;
	THREADINFO thread;
}USERINFO,*LPUSERINFO;


#endif

