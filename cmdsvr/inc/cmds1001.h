/******************************************************************************
 *   서브시스템 : CMD서버
 *   프로그램명 : cmds1001.h
 *         기능 : 업로드 각서버군에서 적정서버를 찾아 IP와 Port를 전송한다.
 *         설명 :
 *       작성자 : LEE
 *       작성일 : 2004/02/16
 *     수정이력 :
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#ifndef	_CMDS1001_H_
#define	_CMDS1001_H_

#include "comhead.h"
/*----------------------------------------------------------------------------*/
/*   WE디스크 조회 요청                                                       */
/*----------------------------------------------------------------------------*/
typedef struct _CCMDS1001_R
{
	int     find_flag             ;  // 검색구분 [1]WE디스크  [2]MY디스크
	unsigned long temp_id ;
	char   sect_code[ 2+2];  // 분류코드
	char   sect_sub[ 2+2];  // 분류코드
	char   adult_yn[ 2+2];  // 성인 
	double file_size;
}CCMDS1001_R,*LPCCMDS1001_R;


/*----------------------------------------------------------------------------*/
/*   WE디스크 조회 응답                                                       */
/*----------------------------------------------------------------------------*/
typedef struct _CCMDS1001_S
{
	char server_id  [ 5+1];   // 서버ID
	char server_ip  [15+1];   // 서버IP
	char tmp        [ 2  ];   // filler
	long dn_port          ;   // 다운로드port
	long up_Port          ;   // 업로드  port
	char root_path [255+1];   // MY디스크시 생성될 패스
}CCMDS1001_S,*LPCCMDS1001_S;

int cmds1001(char *pRecvHead, char *pRecvData, char* &pSendData);

int GetPriServerInfo(MYSQL *con, int nLimit, char* pServerGu, LPCCMDS1001_S pCmd1001_S);

int GetSectServerInfo(MYSQL *con, char* pServerGu, char* pSectCode, LPCCMDS1001_S pCmd1001_S);

int GetServerInfo(MYSQL *con, char* pServerGu, LPCCMDS1001_S pCmd1001_S);
#endif

/******************************************************************************
 * End of file...
 *****************************************************************************/
