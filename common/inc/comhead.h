/*============================================================================*/
/* PROJECT명 : CMD서버                                                        */
/* FILE   명 : comhead.h                                                      */
/* 기     능 : 통신버퍼의 헤더구조                                            */
/*============================================================================*/
#ifndef __HEADER__
#define __HEADER__

#include "gdefine.h"
/*----------------------------------------------------------------------------*/
/* Protocol Header Information                                                */
/*----------------------------------------------------------------------------*/
typedef struct _HEADER
{
	int     nCmd        ;			/* CMD 번호    */
	int     nDataCnt    ;			/* row count   */ // 데이터 패킷 카운트
	int		nDataSize   ;			/* row Size    */ // 데이터 사이즈
	int		nErrorCode  ;			/* Error번호   */
	char	szUserID[12];			/* 사용자 ID   */
} HEADER, *LPHEADER;

#define	 HEADER_SIZE	sizeof( HEADER)
#define	_HEADER_SIZE	sizeof(_HEADER)

//#define __DEBUG
/*----------------------------------------------------------------------------*/
/* Protocol Header Information                                                */
/* For Error Message Transfer to Client                                       */
/*----------------------------------------------------------------------------*/
typedef struct _ERR_HEADER
{
	HEADER	header;
	char	errmsg[256];
} ERR_HEADER;
#define	 ERR_HEADER_SIZE	sizeof( ERR_HEADER)
#define	_ERR_HEADER_SIZE	sizeof(_ERR_HEADER)

#define MODE_NORMAL	20000
#define MODE_DSP	20001


#endif /* end of __HEADER__ */
