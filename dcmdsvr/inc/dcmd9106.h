/******************************************************************************
 *   서브시스템 : FUP서버
 *   프로그램명 : com9106.h
 *         기능 : upload 시 통합 DB 에 중복파일 조회
 *         설명 : 가라 업로드를 하기 위함.
 *       작성자 : 김일오
 *       작성일 : 2015.03.12
 *     수정이력 :
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#ifndef	_DCMD9106_H_
#define	_DCMD9106_H_
#include "comhead.h"

/*
typedef struct _CCOM9106_R
{ 
	unsigned long  temp_id      ;    // nori.T_CONTENTS_TEMPLIST id
	unsigned long  seq_no       ;    // nori.T_CONTENTS_TEMPLIST seq_no

	unsigned long  file_id      ;    // nori.T_CONTENTS_TEMPLIST file_id
	char	server_group_id		[10+1];// nori.T_CONTENTS_TEMPLIST server_group_id

	unsigned int result_val     ;    // 결과 값 리턴   1 : 성공 , 0 : 실패, -1 : DB 쿼리 실패..
	unsigned int b_duplicate	;    // 중복 유무 체크..  1 : 중복 , 0 : 중복 아님.. 
	
}CCOM9106_R, *LPCCOM9106_R;

*/
long dcmd9106(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData);//CCOM9106_R pcom9106_r);
#endif

/******************************************************************************
 * End of file...
 *****************************************************************************/
