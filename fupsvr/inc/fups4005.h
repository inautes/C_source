/******************************************************************************
 *   서브시스템 : CMD서버
 *   프로그램명 : fups4005.h
 *         기능 : 각서버군에서 적정서버를 찾아 IP와 Port를 전송한다.
 *         설명 :
 *       작성자 : LEE
 *       작성일 : 2004/02/16
 *     수정이력 :
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#ifndef	_fups4005_H_
#define	_fups4005_H_

typedef struct _MUREKA_VINFO //20090813 동영상 필터링 연동
{
	int		nFileGubun;					// 0 - 음악/동영상 파일 아님, 1 - 음악파일, 2 - 동영상파일
	
	char	filename[256+1];				// 파일명 (경로 제외)

	char	mureka_hash[64+1];			// 파일에 대한 해시값		

	int		nResultCode;				// 검색결과코드
										//	0 : 정상
										//	1 : 네트워크 에러
										//	2 : File Open or Read 에러
										//	3 : 포맷 에러
										//  4 : 타임아웃 에러
										//  5 : 동영상 헤더정보에 오류 (Wedisk)
										//	99 : 기타 에러
	
	//////////////////////////////////////////////////////
	// 음악파일(nFileGubun=1) 의 경우 결과
	//////////////////////////////////////////////////////
	char	music_status[2+1];			// 상태 ( 00:무료, 01:유료, 02:차단, 03:Non-License, 04:Unknown)

	char	music_id[9+1];				// 뮤직ID
	char	music_title[200+1];			// 곡명
	char	music_artist[150+1];			// 아티스트
	char	music_album[150+1];			// 앨범
	char	music_prod_code[5+1];			// 상품코드 (정액제 : 20000)
	char	music_price[6+1];				// 금액 (정액제 : 0)
	char	music_injeob_com[60+1];		// 인접권 업체명
	char	music_injeob_music_id[20+1];	// 인접권 업체 MUSIC_ID
	//////////////////////////////////////////////////////
	//--음악파일
	//////////////////////////////////////////////////////

	//////////////////////////////////////////////////////
	// 비디오(nFileGubun=2)의 경우 결과 
	//////////////////////////////////////////////////////
	char	video_status[2+1];			// 상태 (00:무료, 01:유료, 02:차단, 03:Non-License, 04:Unknown)
	
	char	video_id[20+1];
	char	video_title[200+1];			// 제목
	char	video_jejak_year[4+1];		// 제작년도
	char	video_right_name[200+1];		// 계약사명
	char	video_right_content_id[50+1];	// 계약사 컨텐츠ID
	
	char	video_grade[2+1];				// 등급
										// 12 : 12세 이상
										// 15 : 15세 이상
										// 18 : 18세 이상
										// 1  : 전체관람가
										// 0  : 등급 미정
	
	char	video_price[6+1];				// 가격 (유료일때 가격)									
	char	video_cha[10+1];				// 회차

	char	video_osp_jibun[10+1];		// OSP 지분률
	char	video_osp_etc[256+1];			// OSP 설정 비고값

///// 20100128
	char	video_onair_date[8+1];		// 방영일/개봉일
	char	video_right_id[20+1];			// 계약사 ID
///// 20100128 /////
	//////////////////////////////////////////////////////
	// 비디오
	//////////////////////////////////////////////////////	
}MUREKA_VINFO, *LPMUREKA_VINFO;
typedef struct _CFUPS4005
{
	unsigned long id  			;  // 컨테츠 temp ID
	int		seq_no				;
	int		depth				;
	int		mureka_cnt			;	//뮤레카 패킷 갯수.
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
}CFUPS4005,*LPCFUPS4005;


typedef struct _CFUPS4005_1M_HASH
{
	char  	hash_1m				[256]; //wedisk 1mb hash
	char  	hash_1m_mureka		[256]; //mureka 1mb hash			
}CFUPS4005_1M_HASH,*LPCFUPS4005_1M_HASH;

//extern long fups4005(LPCfups4005 pfups4005);
long fups4005(CFUPS4005 PFUPS4005, LPMUREKA_VINFO pMurekaVInfo);
//20190123
long fups4005hash(CFUPS4005 PFUPS4005, LPMUREKA_VINFO pMurekaVInfo, CFUPS4005_1M_HASH PFUPS4005HASH);
long fupsflog4005(CFUPS4005 PFUPS4005, LPMUREKA_VINFO pMurekaVInfo);
#endif

/******************************************************************************
 * End of file...
 *****************************************************************************/
