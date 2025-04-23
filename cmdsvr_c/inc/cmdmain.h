/*
//******************************************************************************
 *   서브시스템 : cmd서버
 *   프로그램명 : cmdmain.h
 *         기능 : cmdmain의 메인 헤더
 *         설명 :
 *       작성자 : LEE
 *       작성일 : 2004/02/16
 *     수정이력 :
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
********************************************************************************
*/
#ifndef	_CMDMAIN_H_
#define	_CMDMAIN_H_

#define _FILE_OFFSET_BITS 64

#ifndef	_THREAD_MODULE_
#define _THREAD_MODULE_
#endif

#include <map>
using namespace std;
////////////////////////////////////////////////////////////////////
// define header
////////////////////////////////////////////////////////////////////

//탐새기
//#define RS_CMD_REQUEST_WESERVERINFO 200
//#define RS_CMD_REQUEST_MYSERVERINFO 201
//#define RS_SET_CONNECT_COUNT 10052

#define RS_EOL 5
#define RS_ERR 7

//서버 정보
#define RS_CMD_REQUEST_SERVERINFO_OK 202
#define RS_CMD_REQUEST_SERVERINFO_FAIL 203

#define RS_CMD_REQUEST_USER_LIST 10051

#define RS_CMD_REQUEST_FUP_WESERVERINFO				1001
#define RS_CMD_REQUEST_FUP_MYSERVERINFO				1101

//#define RS_CMD_REQUEST_FDN_CONTENTES_INFO				1002

#define FT_FOLDER 0x001 // Folder 표시
#define FT_FILE 0x002  // File 표시

#define FT_MYDISK 0x003 
#define FT_WEDISK 0x004


typedef struct _User_List  //10051 struct
{
	int nNumber;
	char szUserIP[16];
	char szUserID[12+4];
	char szStartTime[30+2];		
}USERLIST,*LPUSERLIST;

#define MAX_THREAD_POOL 1000
//#define MAX_THREAD_NUM 50



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

void *ThreadMain(void *threadArgs);
extern	multimap<int,USERINFO> m_UserList;

void* mon_thread(void *data);

/******************************************************************************
** 전역변수
*******************************************************************************/
extern	char	errMsg[256];			//에러메시지

extern	pthread_cond_t *mycond;
extern	pthread_cond_t *accept_thread;

extern	pthread_cond_t async_cond;
extern	pthread_mutex_t mutex_lock;
extern	pthread_mutex_t async_mutex;

/******************************************************************************
** 함수 프로토타입 정의
*******************************************************************************
** normal function
*******************************************************************************/
void WaitForRequest(int& Socket);  // Wait Request From Client 
long processed(int& Socket,char* pHead,char* pData,char* &pSendData);
int RecvData(int nSockID,char* RecvBuffer,int nRecvLen);  // recv function
int SendData(int nSockID,char* SendBuffer,int nSendLen); // send function
void DErrorMsg(char* msg); //for debug print

/******************************************************************************
** process function
*******************************************************************************/
//int CmdRequestWeServerInfo(int& Socket,LPPACKET pPacket); // get hdd info and send hdd info data
//int CmdRequestMyServerInfo(int& Socket,LPPACKET pPacket); // get hdd info and send hdd info data
//int CmdRequestEol(int& Socket,LPPACKET pPacket);
//#ifdef	_PRINT_
//	fprintf(stdout, "ERROR::socket...%s/%d\n", strerror(errno), errno);
//#endif

int cmds1001(char *pRecvHead, char *pRecvData, char* &pSendData);
int cmds1101(char *pRecvHead, char *pRecvData, char* &pSendData);
int cmds1002(char *pRecvHead, char *pRecvData, char* &pSendData);
int cmds1003(char *pRecvHead, char *pRecvData, char* &pSendData);
int cmds1004(char *pRecvHead, char *pRecvData, char* &pSendData);
int cmds10041(char *pRecvHead, char *pRecvData, char* &pSendData);
int cmds1011(char *pRecvHead, char *pRecvData, char* &pSendData);
int cmds1012(char *pRecvHead, char *pRecvData, char* &pSendData);
int cmds1005(char *pRecvHead, char *pRecvData, char* &pSendData); //admin 서버 목록 조회
int RequestUserList(int& Socket,char *pRecvHead, char *pRecvData, char* &pSendData); //cmd 사용자 목록 요청 10051
//int cmds10051(char *pRecvHead, char *pRecvData, char* &pSendData); //cmd 사용자 목록 요청
int	E_dump(int err_code, char *err_mesg, char* &pSendData);

#endif

/******************************************************************************
 * End of file...
 *****************************************************************************/

