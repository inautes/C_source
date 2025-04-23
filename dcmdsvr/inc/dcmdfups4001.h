/******************************************************************************
 *   서브시스템 : CMD서버
 *   프로그램명 : fups4001.h
 *         기능 : 각서버군에서 적정서버를 찾아 IP와 Port를 전송한다.
 *         설명 :
 *       작성자 : LEE
 *       작성일 : 2004/02/16
 *     수정이력 :
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#ifndef	_dcmdfups4001_H_
#define	_dcmdfups4001_H_
#include "comhead.h"

typedef struct _tagDSP_REQUEST_INFO_
{
	unsigned long dwId;
	char	szSrcName[8];
	char	szSrcIP[16];
	char	szDestName[8];
	char	szDestIP[16];
	int		nDestPort;

}DSP_R_INFO,*LPDSP_R_INFO;
/*  
typedef struct _CFUPS4001
{   
	// T_CONTENTS_INFO(컨텐츠정보)
	int     file_resoX              ;  // 해상도X
	int     file_resoY              ;  // 해상도Y
	int     dsp_file_cnt                ;  // 조회건수
	int     down_cnt                ;  // 다운로드건수
	int     price_amt               ;  // 희망가격
	int     won_mega                ;  // 원당메가



	unsigned long id                ;  // 컨테츠ID
	double  file_size               ;  // 파일크기
	
	char    title           [ 200+1];  // 제목
	char    descript        [4799+1];  // 내용
	char	  default_hash	   [40+1];  //해쉬코드
	char	  audio_hash		   [32+1];  //해쉬코드
	char	  video_hash		  [128+1];  //해쉬코드
	char	  copyright_yn		[1+1];  //해쉬코드	
	char    keyword           [80+1];  // 키워드
	char    sect_code         [ 2+1];  // 분류코드
	char    share_meth        [ 2+1];  // 공유방법
	char    disp_end_date     [ 8+1];  // 게시종료일자
	char    disp_end_time     [ 6+1];  // 게시종료시간
	char    disp_stat         [ 1+1];  // 게시상태
	char    file_del_yn       [ 1+1];  // 파일삭제여부
	// T_CONTENTS_FILE(컨텐츠파일정보)
	char    folder_yn         [ 1+1];  // 폴더여부
	char    server_id         [ 5+1];  // 서버ID
	char    file_path        [255+1];  // 파일경로
	char    file_name1       [255+1];  // 로컬파일이름
	char    file_name2       [255+1];  // 서버파일이름
	char    file_type         [ 5+1];  // 파일타입
	char    up_st_date        [ 8+1];  // 업로드시작일자
	char    up_st_time        [ 6+1];  // 업로드시작시간
	char    reg_user          [12+1];  // 등록자
	char    reg_date          [ 8+1];  // 등록일자
	char    reg_time          [ 6+1];  // 등록시간
	char	adult_yn		  [ 1+1];  // 성인자료여부
	char	temp			  [ 1+1];  //

	
}CFUPS4001,*LPCFUPS4001;
*/
/*
typedef struct _CFUPS4001
{
	// T_CONTENTS_INFO(컨텐츠정보)
	char    title           [ 200+1];  // 제목
	char    descript        [5000+1];  // 내용
	char    keyword           [80+1];  // 키워드
	char    sect_code         [ 2+1];  // 분류코드
	char    share_meth        [ 2+1];  // 공유방법
	int     price_amt               ;  // 희망가격
	int     won_mega                ;  // 원당메가
	char    disp_end_date     [ 8+1];  // 게시종료일자
	char    disp_end_time     [ 6+1];  // 게시종료시간
	char    disp_stat         [ 1+1];  // 게시상태
	char    file_del_yn       [ 1+1];  // 파일삭제여부
	// T_CONTENTS_FILE(컨텐츠파일정보)
	char    folder_yn         [ 1+1];  // 폴더여부
	char    server_id         [ 5+1];  // 서버ID
	char    file_path        [255+1];  // 파일경로
	char    file_name1       [255+1];  // 서버파일이름
	char    file_name2       [255+1];  // 로컬파일이름
	char    file_type         [ 5+1];  // 파일타입
	int     file_resoX              ;  // 해상도X
	int     file_resoY              ;  // 해상도Y
	int     qury_cnt                ;  // 조회건수
	int     down_cnt                ;  // 다운로드건수
	char    up_st_date        [ 8+1];  // 업로드시작일자
	char    up_st_time        [ 6+1];  // 업로드시작시간
	char    reg_user          [12+1];  // 등록자
	char    reg_date          [ 8+1];  // 등록일자
	char    reg_time          [ 6+1];  // 등록시간
	unsigned long id                ;  // 컨테츠ID
	double  file_size               ;  // 파일크기
}Cfups4001,*LPCfups4001;
*/
//extern long fups4001(LPCfups4001 pfups4001);
long dcmdfups4001(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData);//CFUPS4001 PFUPS4001);
void setLogFups4001(char* pData);
void *DspRequestThreadMain(void *threadArgs);
#endif

/******************************************************************************
 * End of file...
 *****************************************************************************/
