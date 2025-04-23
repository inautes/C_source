/******************************************************************************
 *   서브시스템 : FUP서버
 *   프로그램명 : com9106.h
 *         기능 : upload 시 nori.T_CONTENTS_TEMPLIST 에 이미 중복 처리 결과가 들어가므로 file_id,server_group_id 를 여기서 조회한다.
 *         설명 : 가라 업로드를 위한 처리.
 *       작성자 : 김일오
 *       작성일 : 2015.03.12
 *     수정이력 :
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#ifndef	_COM9106_H_
#define	_COM9106_H_

typedef struct _CCOM9106_R
{
	unsigned int   service_type     ;    // //1=wedisk, 2=mydisk, 3=mydata 4=filog disk 5. guru?
	unsigned long  temp_id      ;    // nori.T_CONTENTS_TEMPLIST id
	unsigned long  seq_no       ;    // nori.T_CONTENTS_TEMPLIST seq_no

	unsigned int   depth ;
	char		   default_hash [38+1];

	unsigned long  file_id      ;    // nori.T_CONTENTS_TEMPLIST file_id
	char	server_group_id		[10+1];// nori.T_CONTENTS_TEMPLIST server_group_id

	int result_val     ;    // 결과 값 리턴   1 : 성공 , 0 : 실패, -1 : DB 쿼리 실패..
	unsigned int b_duplicate	;    // 중복 유무 체크..  1 : 중복 , 0 : 중복 아님..

}CCOM9106_R, *LPCCOM9106_R;


CCOM9106_R com9106(CCOM9106_R pcom9106_r,char* szIP, int nPort);
#endif

/******************************************************************************
 * End of file...
 *****************************************************************************/
