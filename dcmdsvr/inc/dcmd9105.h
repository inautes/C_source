/******************************************************************************
 *   서브시스템 : FUP서버
 *   프로그램명 : com9105.h
 *         기능 : upload 시 T_CONTENTS_TEMP 에 서버 파일 정보 저장
 *         설명 : wedisk, mydata 업로드 시작시 temp 에 서버쪽 파일 정보 저장
 *       작성자 : LEE
 *       작성일 : 2004/11/24
 *     수정이력 :
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#ifndef	_DCMD9105_H_
#define	_DCMD9105_H_
#include "comhead.h"
/*
typedef struct _CCOM9105_R
{ 
	unsigned long id                ;  // 컨테츠ID(T_CONTENTS_TEMP.id)
	char    user_id           [12+1];  // 사용자
	char    temp              [   3];
	char	sfile_path		  [ 256];
	char	sfile_name		  [ 256];	
	char	server_id		  [ 5+1];
	char	temp2			  [	  2];
	
}CCOM9105_R, *LPCCOM9105_R;

/*
typedef struct _CCOM9105_S
{
}Ccom9105_S, *LPCCOM9105_S;
*/
long dcmd9105(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData);//CCOM9105_R pcom9105_r);
#endif

/******************************************************************************
 * End of file...
 *****************************************************************************/
