/******************************************************************************
 *   서브시스템 : 공통모듈
 *   프로그램명 : apdefine.h
 *         기능 : DEFINE문 정의 헤더화일
 *         설명 :
 *       작성자 : 임성은
 *       작성일 : 2000/03/06
 *     수정이력 :
 *
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/

#ifndef	_DEFINE_SECTION_
#define	_DEFINE_SECTION_

/*
** 구조체 멤버 SIZE 정의
#pragma nomember_alignment
*/

#ifndef JOB_OFF
#define JOB_OFF		0
#endif

#ifndef JOB_ON
#define JOB_ON		1
#endif

#ifndef RETRY
#define RETRY	99
#endif

#ifndef RETOK
#define RETOK	0		/* 정상, 성공 */
#endif

#ifndef RETNG
#define RETNG	-1		/* 오류, 실패 */
#endif

#ifndef RET_OK
#define RET_OK	0		/* 정상, 성공 */
#endif

#ifndef RET_NG
#define RET_NG	-1		/* 오류, 실패 */
#endif

#ifndef RC_SUCCESS
#define RC_SUCCESS	0		/* 정상, 성공 */
#endif

#ifndef RC_FAIL
#define RC_FAIL		-1		/* 오류, 실패 */
#endif

#ifndef RC_ERROR
#define RC_ERROR	-1		/* 오류, 실패 */
#endif

#ifndef SUCCESS
#define SUCCESS		0		/* 정상, 성공 */
#endif

#ifndef FAIL
#define FAIL		-1		/* 오류, 실패 */
#endif

#ifndef BOOL
#define BOOL	int
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

/*
** LOG LEVEL
*/
#ifndef PRINT
#define PRINT     	1           /* 화면으로 출력 */
#endif

#ifndef TRACE
#define TRACE     	2           /* 일반 사항인 경우 */
#endif

#ifndef STATE
#define STATE     	3           /* 일반 사항인 경우 */
#endif

#ifndef CHECK
#define CHECK		4           /* 체크 사항인 경우 */
#endif

#ifndef ERROR
#define ERROR		8           /* 에러 사항인 경우 */
#endif

#ifndef ALWAY
#define ALWAY		9           /* 항상             */
#endif

/*
** 쓰레드 상태 정의
*/
#ifndef THR_INI
#define THR_INI		0	/* 쓰레드가 없음 */
#endif

#ifndef THR_END
#define THR_END		0	/* 쓰레드가 없음 */
#endif

#ifndef THR_RUN
#define THR_RUN		1	/* 쓰레드 실행중 */
#endif

#ifndef THR_EMPTY
#define THR_EMPTY		0	/* 쓰레드가 없음 */
#endif

#ifndef THR_ACTIVE
#define THR_ACTIVE		1	/* 쓰레드 실행중 */
#endif

#ifndef THR_EXIT
#define THR_EXIT		9	/* 쓰레드 종료 */
#endif

#ifndef MAX_THREADS
#define MAX_THREADS		50	/* 최대 생성가능 쓰레드 수 */
#endif

#ifndef SOCKET_SIZE
#define SOCKET_SIZE		4096	/* 송수신 버퍼 SIZE */
#endif

#ifndef MAX_BUF_LEN
#define MAX_BUF_LEN		4096	/* 송수신 버퍼 SIZE */
#endif

#ifndef RETRY_CNT
#define RETRY_CNT   3       /* 송신 ERROR COUNT */
#endif

#ifndef TMP_SIZE
#define TMP_SIZE	256
#endif

#ifndef SEQUENCE_MAXCOUNT
#define SEQUENCE_MAXCOUNT 10	/* MAX SEQ */
#endif

/*
** 오류코드
*/
#define	ERROR_HEADER	0x10	/* 잘못된 헤더 형식	*/
#define	ERROR_BUSY		0x20	/* 송수신 장애		*/
#define	ERROR_TIMEOUT	0x30	/* 시간 초과		*/
#define	ERROR_FORMAT	0x40	/* 포멧 오류		*/
#define	ERROR_ETC		0x90	/* 기타 오류		*/


/*
** 제어코드
*/
#define ASCII_NULL      0x00
#define ASCII_CTRL_A    0x01
#define ASCII_CTRL_B    0x02
#define ASCII_CTRL_C    0x03
#define ASCII_CTRL_D    0x04
#define ASCII_CTRL_E    0x05
#define ASCII_CTRL_F    0x06
#define ASCII_CTRL_G    0x07
#define ASCII_CTRL_K    0x0B
#define ASCII_CTRL_V    0x16
#define ASCII_ESC       0x1B
#define ASCII_LF        0x0D
#define ASCII_CR        0x0A
#define ASCII_SPACE     0x30
#define ASCII_TILT      0x7E

/*
** DB 상태
*/
#ifndef DB_CONNECT
#define DB_CONNECT      1
#endif

#ifndef DB_RELEASE
#define DB_RELEASE      0
#endif

/*
** 매크로 함수
*/

#define MEMSETS(des) 	memset( &des, 0x00, sizeof(des) )	/* 구조체 */
#define MEMSETP(des) 	memset( des,  0x00, sizeof(des) )	/* 포인트 */
#define NULLSET(des) 	memset( des,  0x00, sizeof(des) )	/* 문자형 */
#define ZEROSET(des) 	des = 0;							/* 수치형 */

#define MEMCMP(des, src)	memcmp((char*)des, (char*)src, strlen(src))
#define MEMCPY(des, src)	memcpy((char*)des, (char*)src, strlen(src))
#define STRCPY(des, src) 	strcpy((char*)des, (char*)src)
#define SPRINT(des, src) 	sprintf((char*)des, "%s", (char*)src)

#define FILLMEMORY(str, val)    memset(str, val, sizeof(str))
#define COPYMEMORY(str, des)    memcpy(str, des, strlen(des))
#define LONGTOMEM9(str, val)    infLongToFillStr(str, val, sizeof(str), '0')

#endif

/*****************************************************************************
 * End of file...
 *****************************************************************************/
