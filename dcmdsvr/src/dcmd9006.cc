/******************************************************************************
 *   서브시스템 : FDN서버
 *   프로그램명 : com9006.cc
 *         기능 : 컨텐츠 구매정보에서 어떤 요금제로 구매했는지를 조사한다.
 *         설명 :
 *     수정이력 :
 *     수정이력 :
 *     수정내용 : 2.0일경우 정액여부 검사하지 않음.
 *                deal_type이 3이면 모든 검증 프로세스 그냥 통과.
*******************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#include <mysql.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "dcmddefine.h"
#include "apdefine.h"
#include "comcomm.h"
#include "commydb.h"
#include "dcmd9006.h"
#include "time.h"
//#define  _DEBUG_

// db 제어 서버
#include <unistd.h>     /* for close() getpid()*/
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include "dcmdsock.h" //sock send recv
// db 제어 서버
#include "mysql_pool.h"

#include "../../fdnsvr/inc/fdndefine.h"
extern CMysqlPool * m_g_clMysqlPool;

long dcmd9006(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData)
{


	char szQuery[10000];
	MYSQL     *con=NULL;
	MYSQL_RES *res;
	MYSQL_ROW  row;

	char szStartHoldTime[7];
	char szEndHoldTime[7];
	char szHoldTimeCode[2];
	long lCurrentTime = 0;
	long lCurrentTime_fix4 = 0;
	char szErrHoldTime[2];
	int nErrHoldTime = 0;
	memset(szHoldTimeCode,0x00,sizeof(szHoldTimeCode));
	memset(szEndHoldTime,0x00,sizeof(szEndHoldTime));
	memset(szStartHoldTime,0x00,sizeof(szStartHoldTime));
	memset(szErrHoldTime,0x00,sizeof(szErrHoldTime));



	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);

	HOLDTIME HoldTime ;
	memset(&HoldTime,0x00,sizeof(HOLDTIME));


	LPFILEINFO pFileInfo = (LPFILEINFO)pRecvData;

	#ifdef _DEBUG_
	printf("com9006-> start------------------------------- ( DealType %d )( DealNo %lu) \n"
	        ,pFileInfo->nDealType, pFileInfo->dwDealID);
	#endif


	if(pFileInfo->nDealType == 2 || pFileInfo->nDealType == 3)
	{//2.0 다운로드일 경우.
		#ifdef _DEBUG_
		printf("> 9006 : 정상 처리 (2.0) ");
		#endif
 		HoldTime.nHoldTimeMode         =   atoi(szHoldTimeCode);

		unsigned long  dwSendLen = 0;

		req_header.nCmd = 9006;
		req_header.nDataCnt = 1;
		req_header.nDataSize = sizeof(HoldTime);

		dwSendLen = HEADER_SIZE + req_header.nDataCnt * req_header.nDataSize ;

		pSendData = new char [dwSendLen];
		memset(pSendData , 0x00 ,sizeof(dwSendLen));

		memcpy( pSendData , &req_header,HEADER_SIZE);

		if( req_header.nDataCnt * req_header.nDataSize > 0 )
		{
			memcpy( pSendData + HEADER_SIZE, &HoldTime , req_header.nDataCnt * req_header.nDataSize);
		}

	    #ifdef _DEBUG_
	    printf("Mode %d\n",HoldTime.nHoldTimeMode      );
	    #endif

		return dwSendLen;

	}


	//infLOG(ALWAY,"\ncom9006 [ %lu ] \n",pFileInfo->dwDealID);

	// pFileInfo->dwDealID

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	bool bCloseDB = false;


	CMysqlCon MysqlCon(m_g_clMysqlPool,m_g_clMysqlPool->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );

	con = MysqlCon.GetMysqlCon();

	if (con == NULL )
	{

		#ifdef __DEBUG
		printf(" ] com9006 요청 패킷 전송 ID ( %s ) deal_type ( %n ) deal_no ( %lu )\n"
		,pHeader->szUserID,pFileInfo->nDealType,pFileInfo->dwDealID);
		#endif

		infLOG(ERROR, "DCMD9006 | GetMysqlCon is null \n");


		int nRetry = 0;
		while (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			infLOG(ERROR, "DCMD9006 | Cannot DB Connect - GetMysqlCon is null\n");
			//infLOG(ERROR, "DCMD9006 | Mysql Error | Errno : %d | ErrMsg : %s \n\n", mysql_errno(con),mysql_error(con));
		}
		if( nRetry >= 5)
		{
			req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);
			infLOG(ERROR, "DCMD9006 | Cannot DB Connect \n");
			return HEADER_SIZE;
	    }


	    bCloseDB = true;
	    infLOG(ERROR,"DCMD9006 | Connect DB direct\n");
	}
	//infLOG(ALWAY,"com9006 요청 패킷 전송 ID ( %s ) deal_no ( %lu )\n",pHeader->szUserID,pFileInfo->dwDealID);


	//----------------------------------------------------------------------
	// 구매정보 확인
	//----------------------------------------------------------------------
	memset (szQuery,  0x00, sizeof(szQuery));



	sprintf(szQuery, "SELECT fixamt_yn					"
	                 "     , substring(minor_name,1,6)   "
	                 "     , substring(minor_name,7,6)   "
	                 "     , UNIX_TIMESTAMP()            "
	                 "	   , date_format(now(),'%%H') "
	                 "	   , UNIX_TIMESTAMP(now()) - 64800 "
	                 "  FROM T_DEAL_INFO  a       "
	                 "     , T_MINOR_CODE b       "
	                 " WHERE a.deal_no    = %lu          "
	                 "   AND a.fixamt_yn  in ('2', '3','4','5','6','7','8','9')  "
	                 "   AND a.fixamt_yn  = b.minor_code "
	                 "   AND b.major_code = '25'		 "
	                 , pFileInfo->dwDealID );

	//UNIX_TIMESTAMP(date_format(date_add(now(),INTERVAL -18 HOUR),'%Y%m%d%H%i%s'))

	//infLOG(ALWAY,"\ncom9006 mysql_query [ %lu ] [ %s ] \n",pFileInfo->dwDealID,szQuery);
	#ifdef _DEBUG_
	printf("> 9006 : szQuery[%s]\n", szQuery);
	#endif

	if (mysql_query(con, szQuery))
	{
		infLOG(ERROR, "com9006[ERR]: SELECT T_DEAL_INFO error\n");
		infLOG(ERROR, "com9006[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));

		if(con!=NULL)
		{
			if( bCloseDB )
				db_disconnect(con);
		}

		req_header.nCmd = -1 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);

		infLOG(ERROR, "com9006[SQL]: %s\n", szQuery);

		return HEADER_SIZE;
	}
//	infLOG(ALWAY," ] com9006 mysql_store_result [ %lu ] \n",pFileInfo->dwDealID);

	if (!(res = mysql_store_result(con)))
	{
		infLOG(ERROR, "com9006[ERR]: mysql_store_result error\n");
		infLOG(ERROR, "com9006[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		if( bCloseDB )
			db_disconnect(con);

		infLOG(ERROR, "com9006[SQL]: %s\n", szQuery);

		req_header.nCmd = -1 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);

		return HEADER_SIZE;
	}

	unsigned long  dwSendLen = 0;

	req_header.nCmd = 9006;
	req_header.nDataCnt = 1;
	req_header.nDataSize = sizeof(HoldTime);

	dwSendLen = HEADER_SIZE + req_header.nDataCnt * req_header.nDataSize ;

//	infLOG(ALWAY," ] com9006 mysql_num_rows [ %lu ] \n",pFileInfo->dwDealID);

 	if (mysql_num_rows(res)==0)
 	{
 		//정상 처리 (일반구매, 정액제(24시간)
//		infLOG(ALWAY," ] com9006 mysql_num_rows [ %lu ] is zero \n",pFileInfo->dwDealID);
		#ifdef _DEBUG_
		printf("> 9006 : 정상 처리 (일반구매, 정액제(24시간) DealType(%d) \n", pFileInfo->nDealType);
		#endif
 		HoldTime.nHoldTimeMode         =   atoi(szHoldTimeCode);
	}
	else
	{
//		infLOG(ALWAY," ] com9006 mysql_fetch_row [ %lu ] is zero \n",pFileInfo->dwDealID);

		row = mysql_fetch_row(res);

		strcpyA(szHoldTimeCode ,  getstr(row, 0));
		strcpyA(szStartHoldTime  , getstr(row, 1) );
		strcpyA(szEndHoldTime ,  getstr(row, 2));

		lCurrentTime = (long)(getnum(row, 3));

		strcpyA( szErrHoldTime , getstr(row,4));

		lCurrentTime_fix4 = (long)(getnum(row,5));

		nErrHoldTime = atoi(szErrHoldTime) ;


		char szTemp[2];

//		infLOG(ALWAY," ] com9006 make time [ %lu ]  \n",pFileInfo->dwDealID);

		if(strcmp(szHoldTimeCode, "8") == 0) //point??
		{
			HoldTime.nHoldTimeMode = 0;
		}
		else
		{


			for(int i = 0 ; i < 3 ; i ++)
			{
				memset(szTemp,0x00,sizeof(szTemp));
				if(i == 0)
				{

					memcpy(szTemp,szStartHoldTime,2);
					HoldTime.stHoldStartTime.nHour   =  atoi(szTemp);

					memset(szTemp,0x00,sizeof(szTemp));
					memcpy(szTemp,szEndHoldTime,2);
					HoldTime.stHoldEndTime.nHour   =   atoi(szTemp);
				}
				else if( i == 1)
				{
					memcpy(szTemp,szStartHoldTime+2,2);
					HoldTime.stHoldStartTime.nMinute =   atoi(szTemp);

					memset(szTemp,0x00,sizeof(szTemp));
					memcpy(szTemp,szEndHoldTime+2,2);
					HoldTime.stHoldEndTime.nMinute =   atoi(szTemp);
				}
				else if (i == 2 )
				{
					memcpy(szTemp,szStartHoldTime+4,2);
					HoldTime.stHoldStartTime.nSecond =   atoi(szTemp);

					memset(szTemp,0x00,sizeof(szTemp));
					memcpy(szTemp,szEndHoldTime+4,2);
					HoldTime.stHoldEndTime.nSecond =   atoi(szTemp);
				}

			}

	//		infLOG(ALWAY," ] com9006 check time [ %lu ]  \n",pFileInfo->dwDealID);

			HoldTime.nHoldTimeMode         =   atoi(szHoldTimeCode);

			struct tm		*stm;
			stm = (struct tm *) localtime(&lCurrentTime);

			if(  HoldTime.nHoldTimeMode == 4 )
			{
				if( nErrHoldTime >= 18 && nErrHoldTime <= 24 )
				{
					HoldTime.stHoldStartTime.nHour = 00;
					HoldTime.stHoldStartTime.nMinute = 0;
					HoldTime.stHoldStartTime.nSecond = 0;

					HoldTime.stHoldEndTime.nHour = 12;
					HoldTime.stHoldEndTime.nMinute = 0 ;
					HoldTime.stHoldEndTime.nSecond = 0;

					// 정액제가 4 ( 야간 ) 일경우  정액제 구간과 현재 시간을 -18 시 씩 해준다.
					stm = (struct tm *) localtime(&lCurrentTime_fix4);
				}
				if( nErrHoldTime >= 0 && nErrHoldTime <= 6 )
				{

					HoldTime.stHoldStartTime.nHour = 0;
					HoldTime.stHoldStartTime.nMinute = 0;
					HoldTime.stHoldStartTime.nSecond = 0;

					HoldTime.stHoldEndTime.nHour = 6;
					HoldTime.stHoldEndTime.nMinute = 0 ;
					HoldTime.stHoldEndTime.nSecond = 0;

				}
			}

			#ifdef __DEBUG
			printf(" pHoldTime Check ( %d ) \n",HoldTime.nHoldTimeMode);
			#endif
			/*
			if( HoldTime.nHoldTimeMode  == 4)
			{
				HoldTime.nHoldTimeMode = 2;
				#ifdef __DEBUG
				printf(" pHoldTime Check Again ( %d ) \n",HoldTime.nHoldTimeMode);
				#endif
			}
			*/




			//HoldTime.lHoldCurTime          =   lCurrentTime;
			HoldTime.lHoldCurTime = stm->tm_hour*60*60 + stm->tm_min*60 + stm->tm_sec;

			#ifdef _DEBUG_
			printf( "최근 시간 %d 시 %d 분 %d 초  ( %d ) \n",stm->tm_hour,stm->tm_min,stm->tm_sec,HoldTime.lHoldCurTime);
			printf( "정액제 ] deal_id ( %lu ) Mode %d  %d 년  %d 월   %d 일  %d 시  %d 분  %d 초 (%ld)\n"
			        "정액제 ] deal_id ( %lu ) Mode %d  %d 년  %d 월   %d 일  %d 시  %d 분  %d 초 (%ld)\n"
		            ,pFileInfo->dwDealID,HoldTime.nHoldTimeMode ,HoldTime.stHoldStartTime.nYear
		            ,HoldTime.stHoldStartTime.nMonth ,HoldTime.stHoldStartTime.nDay
		            ,HoldTime.stHoldStartTime.nHour  ,HoldTime.stHoldStartTime.nMinute
		            ,HoldTime.stHoldStartTime.nSecond,HoldTime.lHoldCurTime
		            ,pFileInfo->dwDealID,HoldTime.nHoldTimeMode ,HoldTime.stHoldEndTime.nYear
		            ,HoldTime.stHoldEndTime.nMonth ,HoldTime.stHoldEndTime.nDay
		            ,HoldTime.stHoldEndTime.nHour  ,HoldTime.stHoldEndTime.nMinute
		            ,HoldTime.stHoldEndTime.nSecond,HoldTime.lHoldCurTime );

			#endif

//		infLOG(ALWAY, "최근 시간 %d 시 %d 분 %d 초  ( %d ) \n",stm->tm_hour,stm->tm_min,stm->tm_sec,HoldTime.lHoldCurTime);
			/*infLOG(ALWAY, "정액제 ] deal_id ( %lu ) Mode (%d)(%s)"
						  "%d 시  %d 분  %d 초 (%ld)\n"
		            ,pFileInfo->dwDealID, HoldTime.nHoldTimeMode, szHoldTimeCode
		            ,HoldTime.stHoldStartTime.nHour, HoldTime.stHoldStartTime.nMinute ,HoldTime.stHoldStartTime.nSecond, HoldTime.lHoldCurTime);

			infLOG(ALWAY, "정액제 ] deal_id ( %lu ) Mode (%d)(%s)"
			              "%d 시  %d 분  %d 초 (%ld)\n"
		            ,pFileInfo->dwDealID, HoldTime.nHoldTimeMode, szHoldTimeCode
		            ,HoldTime.stHoldEndTime.nHour, HoldTime.stHoldEndTime.nMinute, HoldTime.stHoldEndTime.nSecond, HoldTime.lHoldCurTime);
		   */
		}
	}
	//infLOG(ALWAY," ] com9006 mysql_free_result [ %lu ]  \n",pFileInfo->dwDealID);

	mysql_free_result(res);
	//infLOG(ALWAY," ] com9006 //db_disconnect [ %lu]  \n",pFileInfo->dwDealID);
	if( bCloseDB )
		db_disconnect(con);

	pSendData = new char [dwSendLen];
	memset(pSendData , 0x00 ,sizeof(dwSendLen));

	//infLOG(ALWAY," ] com9006 [ %lu ] pSendData [ %d ] [ %d ]*[ %d ]  \n",pFileInfo->dwDealID , dwSendLen , req_header.nDataCnt , req_header.nDataSize);

	memcpy( pSendData , &req_header,HEADER_SIZE);

	if( req_header.nDataCnt * req_header.nDataSize > 0 )
	{
		//infLOG(ALWAY," ] com9006 [ %lu ] memcpy body [ %d ] [ %d ]*[ %d ]  \n",pFileInfo->dwDealID , dwSendLen , req_header.nDataCnt , req_header.nDataSize);
		memcpy( pSendData + HEADER_SIZE, &HoldTime , req_header.nDataCnt * req_header.nDataSize);
	}


    #ifdef _DEBUG_
    printf("Mode %d\n",HoldTime.nHoldTimeMode      );
    #endif
	//infLOG(ALWAY," ] com9006 end [ %lu ]   \n",pFileInfo->dwDealID );

	return dwSendLen;
}


