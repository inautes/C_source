/******************************************************************************
 *   서브시스템 : FDN
 *   프로그램명 : dcmd9009.cc
 *         기능 : 
 *         설명 : 컨텐츠 서버 다운로드 카운트
 *       작성자 : 
 *       작성일 : 
 *     수정이력 :
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#ifndef	_DCMD9009_H_
#define	_DCMD9009_H_
#include "comhead.h"
/*
typedef struct _CCOM9009_R
{
	char    server_id         [ 5+1];	// 서버ID
	int		nConnectCnt;
	int		nMode;				// 0 : dn , 1 : up 
	unsigned long uDisksize;
	unsigned long uUsesize;
	unsigned long uFreesize;
}CCOM9009_R, *LPCCOM9009_R;

/*
typedef struct _CCOM9009_S
{
}CCOM9009_S, *LPCCOM9009_S;
*/
long dcmd9009(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData);//CCOM9009_R pcom9009_r,int nFlag);
#endif

/******************************************************************************
 * End of file...
 *****************************************************************************/
