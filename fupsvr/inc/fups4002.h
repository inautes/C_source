/******************************************************************************
 *   서브시스템 : CMD서버
 *   프로그램명 : fups4002.h
 *         기능 : 무료자료실 등록
 *         설명 :
 *       작성자 : LEE
 *       작성일 : 2004/02/16
 *     수정이력 :
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#ifndef	_fups4002_H_
#define	_fups4002_H_

// T_CONTENTS_INFO(무료자료실 파일정보)
/*
typedef struct _CFUPS4002
{
	unsigned long id                ;  // 컨테츠ID
	char    folder_yn         [ 1+1];  // 폴더여부
	char	temp			  [ 1+1];  //
	char    file_type         [ 5+1];  // 파일타입
	char    server_id         [ 5+1];  // 서버ID
	char    file_path1        [255+1];  // 서버파일경로
	char	file_path2		  [255+1];  // 로컬파일경로
	char    file_name1        [255+1];  // 서버파일이름
	char    file_name2        [255+1];  // 로컬파일이름
	char	file_db_name1	  [255+1];  //서버측 DB저장 이름
//	char	file_db_name2	  [255+1];  //로컬측 DB저장 이름
	double  file_size               ;  // 파일크기
}CFUPS4002,*LPCFUPS4002;	

*/

typedef struct _CFUPS4002
{
	unsigned long id                ;  // 컨테츠ID
	char    folder_yn         [ 1+1];  // 폴더여부
	char	temp			  [ 1+1];  //
	char    file_type         [ 5+1];  // 파일타입
	char    server_id         [ 5+1];  // 서버ID
	char    sfile_path       [255+1];  // 파일경로
	char    sfile_name       [255+1];  // 서버파일이름
	char    lfile_name       [255+1];  // 로컬파일이름
	double  file_size               ;
	double  total_file_size         ;  // 파일크기
}CFUPS4002,*LPCFUPS4002;
	
long fups4002(CFUPS4002 PFUPS4002,char* pErrMsg);
#endif

/******************************************************************************
 * End of file...
 *****************************************************************************/
