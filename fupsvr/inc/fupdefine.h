#ifndef _FUP_DEFINE
#define _FUP_DEFINE


#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif



#include "fups4001.h"
#include "fups4002.h"
#include "fups4003.h"
#include "fups4004.h"
#include "com9009.h"
#include <pthread.h>

#include <map>
using namespace std;


#define FT_SELL 	11
#define FT_GURU 	12
                	
#define C_START 	2
#define C_END 		1
#define C_MIDDLE 	0

#define F_START 	0
#define F_MIDDLE 	1
#define F_END 		2


#define FT_FOLDER 	0x001 // Folder 표시
#define FT_FILE 	0x002  // File 표시

#define FT_ZIP 1
#define FT_NOTZIP 0

#define FT_MYDISK 	0x003 
#define FT_WEDISK 	0x004
//File Up/Down

#define NORMAL_UPLOAD	 0
#define RECONNECT_UPLOAD 1
#define RE_UPLOAD 		 2


#define RS_EOL 	5
#define RS_OK  	6
#define RS_ERR 	7

#define END -2                                                                                                                            
#define MAXPENDING 5    /* Maximum outstanding connection requests */


#define RS_FILE_REQUEST_FILE 				500 // 파일의 해더 정보 (준비)   
#define RS_FILE_DATA_TRANSFER				501 /// 파일의 실제 DATA  (전송)
#define RS_FILE_REQUEST_NEXT_FILE 			502
#define RS_FILE_REQUEST_NEXT_FILEINFO 		503
                                        	
#define RS_FILE_CHECK_ID_OK 				505
#define RS_FILE_CHECK_ID_FAIL 				506
#define RS_FILE_REQUEST_FILE_FILINFO 		507
#define RS_FILE_DATA_SIGN_CHECK 			508
#define RS_FILE_REQUEST_CONTINUE			509
                                        	
#define RS_FILE_CHECK_ID 					512
#define RS_FILE_REQUEST_ID_DISCONNECT 		513
#define RS_FILE_REQUEST_ID_DISCONNECT_OK	514
#define RS_FILE_END_FAIL 					517
#define RS_FILE_REQUEST_LIST 				518
#define RS_FILE_REQUEST_UPSCOPY 			519
#define RS_FILE_REQUEST_SERVER_FILE_LIST	520


#define RS_SPEED_CHECK 		600
#define RS_SPEED_CHECK_OK 	601

#define RS_MYDISK_FILE_REQUEST_LIST 			2001
#define RS_MYDISK_FILE_REQUEST_FILE 			2002
#define RS_MYDISK_FILE_DATA_TRANSFER 			2003
#define RS_MYDISK_FILE_REQUEST_FILE_FILINFO 	2004
#define RS_MYDISK_FILE_END_FAIL 				2005
#define RS_MYDISK_FILE_REQUEST_NEXT_FILEINFO 	2006
#define RS_MYDISK_FILE_REQUEST_NEXT_FILE 		2007
#define RS_MYDISK_FILE_REQUEST_SERVER_FILE_LIST	2009

//#define RS_FILE_REQUEST_HARD_SIZE 4002

#define RS_GURU_FILE_UP 			4002
#define RS_GURUFILE_REQUEST_LIST 	40021

#define RS_CMD_REQUEST_USER_LIST 	10051

#define RS_ADMIN_REQUEST_ID_DISCONNECT			10513
#define RS_ADMIN_REQUEST_ID_DISCONNECT_OK		10514

//T_CONTDATA_MYINFO, T_CONTDATA_MYFILE 내자료실 파일


#define RECVBUF 1024*32//32
#define SENDBUF 1024*32//32

#define SOCKBUF 1024*4




/*
typedef struct _GURUFILEINFO
{
	int nType; //0 is file 1 is directroy
	int nFlag; // 0 is start 1 is middle 2 is end
	char szFilePath[512];
	char szFileName[255];
	char szServerFilePath[512];
	char szServerFileName[255];
	double dFileSize;

}GURUFILEINFO,*LPGURUFILEINFO;
*/

typedef struct _GURUFOLDERPATH
{
	char szFolderName[12];
	char szFolderPath[512];
}GURUFOLDERPATH,LPGURUFOLDERPATH;

typedef struct _GURUFILEINFO
{
	int nFlag; // 0 is start 1 is middle 2 is end
	char	local_file_path	   	  [511+1];  // 파일경로
	char    local_file_name       [255+1];  // 로컬파일이름
	char    server_file_path      [255+1];  // 서버파일경로
	char    server_file_name      [255+1];  // 서버파일경로
	CFUPS4002 fups4002;
}GURUFILEINFO,*LPGURUFILEINFO;


typedef struct _UPSCOPY
{
	int nTypeDisk; // 디스크 타입 (Ft_MYDISK , Ft_WEDISK)
	int nType; //파일타입 (FT_FOLDER or FT_FILE)
	int nMoneyType; //11 : 판매 - 700  , 12 : 공유 - mb 당 얼마
//	long dwMoney; //돈 액수 // 메가 당, 건당
	int nReUploadFlag; 
	long dwServerPort;
	unsigned long nNumber; // 컨텐츠 번호
	char szTemp[2];
	char szServerID[10];
	char szServerIP[16];
	char szDestServerIP[16]; //복사될 서버 아이피
	//char szDestServerID[8]; //복사될 서버 아이피
	char szFolderName[256]; // 파일 날짜 .
	char szFileName[256]; // 파일이름
	char szSrcPath[512]; // 파일이 존재하는 폴더이름
	char szDownPath[512]; // 파일이 존재하는 폴더이름
	
	double dFileSize; // 파일크기
	struct _CFUPS4001 cfups4001;

}UPSCOPY, *LPUPSCOPY;



typedef struct _FILEINFO 
{
	
	//double dFileSize; // 파일크기
	int nTypeDisk; // 디스크 타입 (FG_MYDISK , FG_WEDISK)
	int nType; //파일타입 (FT_FOLDER or FT_FILE)
	int nfupsFlag; //11 : 판매 - 700  , 12 : 공유 - mb 당 얼마
//	long dwMoney; //돈 액수 // 메가 당, 건당
	int nReUploadFlag; 
	long dwServerPort;
	unsigned long nNumber; // 컨텐츠 번호
	char szCopyright_yn[2];
	char szMureka_yn[2];
	char szServerID[10];
	char szServerIP[16];
	char szFolderName[256]; // 폴더 이름
	char szFileName[256]; // 파일이름
	char szSrcPath[512]; // 파일이 존재하는 폴더이름
	char szDownPath[512]; // 파일이 존재하는 폴더이름
	char szDefault_hash	   [38];  //해쉬코드
	char szAudio_hash		   [32];  //해쉬코드
	char szVideo_hash		  [128];  //해쉬코드
	double dFileSize; // 파일크기
	struct _CFUPS4001 cfups4001;
	
}FILEINFO, *LPFILEINFO;


typedef struct _MFILEINFO 
{
	int nTypeDisk; // 디스크 타입 (FG_MYDISK , FG_WEDISK)
	int nType; //파일타입 (FT_FOLDER or FT_FILE)
	int nfupsFlag; //0 : END  1: MIDDLE  2: START
//	long dwMoney; //돈 액수 // 메가 당, 건당
	int nReUploadFlag; 
	long dwServerPort;
	unsigned long nNumber; // 컨텐츠 번호
	char szTemp[2];
	char szServerID[10];
	char szServerIP[16];
	char szFolderName[256]; // 폴더 이름
	char szFileName[256]; // 파일이름
	char szSrcPath[512]; // 파일이 존재하는 폴더이름
	char szDownPath[512]; // 파일이 존재하는 폴더이름
	double dFileSize; // 파일크기
	struct _CFUPS4003 cfups4003;
	
}MFILEINFO, *LPMFILEINFO;


typedef struct _CSERVERINFO
{
	char szIP[16];
	long dwPort;
}CSERVERINFO,*LPCSERVERINFO;



typedef struct _FILEHEAD
{
	double dCurrentSize;
	char szFullFileName[768]; //server측 패스와 이름
}FILEHEAD,*LPFILEHEAD;

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



typedef struct ThreadArgs
{
    int clntSock;                      /* Socket descriptor for client */
    char userIP[16];
    char startTime[32];
    //long startTime;
    //char startTime[100];
}THREADINFO,*LPTHREADINFO;

typedef struct _UserInfo
{
	char szUserID[50];
	pthread_t threadID;
	
	THREADINFO thread;
}USERINFO,*LPUSERINFO;




#endif
