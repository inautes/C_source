/******************************************************************************
 *   서브시스템 : CMD서버
 *   프로그램명 : fups4006.h
 *         기능 : 각서버군에서 적정서버를 찾아 IP와 Port를 전송한다.
 *         설명 :
 *       작성자 : LEE
 *       작성일 : 2004/02/16
 *     수정이력 :
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#ifndef	_dcmdfups4006_H_
#define	_dcmdfups4006_H_
#include "comhead.h"
/* 4006 과 동기화
typedef struct _CFUPS4007
{
	unsigned long id  			;  // 컨테츠 temp ID
	int		seq_no				;
	int		depth				;
	double  file_size 			;  // 파일크기
	char	sect_code[3]		;
	char	sect_sub[3]			;
	char	folder_yn[2]		;
	char	user_id[16]			;
	char 	folder_name[256]	;
	char	file_name[256]		;
	char	default_hash	   [38];  //해쉬코드
	char	audio_hash		   [32];  //해쉬코드
	char	video_hash		  [128];  //해쉬코드
	char	copyright_yn		[2];  //해쉬 조회 여부
	char	mureka_yn			[2];  //뮤레카 조회 여부
	char    cont_gu				[3];  //WE : wedisk FD : fillog
	char    auth_num			[3];  //업로더 권한
		
}CFUPS4007,*LPCFUPS4007;

*/
//extern long fups4006(LPCfups4006 pfups4006);
long dcmdfups4006(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData);//CFUPS4006 PFUPS4006);
#endif

/******************************************************************************
 * End of file...
 *****************************************************************************/
