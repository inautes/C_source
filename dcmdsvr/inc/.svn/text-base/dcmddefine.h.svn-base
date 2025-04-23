#ifndef _DCMD_DEFINE
#define _DCMD_DEFINE


#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif


using namespace std;

#include "com9001.h"
#include "com9002.h"
#include "com9004.h"
#include "com9005.h"
#include "com9006.h"
#include "com9101.h"
#include "com9102.h"
#include "com9103.h"
#include "com9104.h"
#include "com9105.h"


//#include "fdns3004.h"
//#include "fups4001.h"
//#include "fups4003.h"
//#include "fups4002.h"

#define RS_EOL 	5
#define RS_OK 	6
#define RS_ERR 	7
#define END 	-2



#define RECVBUF 1024*32//32
#define SENDBUF 1024*32//32

#define SOCKBUF 1024*4

/*----------------------------------------------------------------------------*/
/*   정액제 시간 정보 조회
/*----------------------------------------------------------------------------*/
/*
typedef struct _DEAL_ID
{
	unsigned long dwDealNum;
}DEALID,*LPDEALID;
*/
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
{
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
	char szUserID[50];
	COM9XXX gCom9xxx;
	pthread_t threadID;
	
	THREADINFO thread;
}USERINFO,*LPUSERINFO;



#endif

