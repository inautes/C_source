/******************************************************************************
 *   서브시스템 : FUP서버
 *   프로그램명 : com9004.cc
 *         기능 : upload 시작
 *         설명 : 서버에 업로드하기 전 모듈 체크, 판매자 정보 조회
 *       작성일 :
 *     수정이력 :
 *     수정내용 : 메인 백업 디비에서 작업하도록 변경.
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#include <mysql.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apdefine.h"
#include "comcomm.h"
#include "commydb.h"
#include "dcmd9004.h"
//#define  _DEBUG_
#include <unistd.h>     /* for close() getpid()*/
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include "dcmdsock.h" //sock send recv

#include "mysql_pool.h"

extern CMysqlPool * m_g_clMysqlPool;


//******************************************************************************
//  com9004 main
//
//  input : unsigned long temp_id (T_CONTENTS_TEM.id)
//
//  return:  1(정상)
//          -1(DB오류)
//          -2(해당 사용자는 이미 성인자료가 10건이므로 업로드할 수 없습니다.)
//******************************************************************************

long dcmd9004(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData)//unsigned long temp_id)
{
	COM9004D dcom9004data;
	memset(&dcom9004data,0x00,sizeof(COM9004D));
	memcpy(&dcom9004data , pRecvData,sizeof(COM9004D));

	COM9004D dcom9004return;
	memset(&dcom9004return,0x00,sizeof(COM9004D));
	memcpy(&dcom9004return , pRecvData,sizeof(COM9004D));

	//unsigned long temp_id;
	//memcpy(&temp_id , pRecvData,sizeof(unsigned long));

	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);

	req_header.nDataCnt = 0;
	req_header.nDataSize = 0;//sizeof(temp_id); //unsigend long


	char szQuery[10000];
	MYSQL       *con=NULL;
	MYSQL_RES *res;
	MYSQL_ROW  row;

	int  reg_count = 0;
	char cont_gu   [2 + 1];
	char adult_yn  [1 + 1];
	char reg_user  [12+ 1];
	char sect_code [2 + 1];
	char reg_date [9];
	double temp_file_size ;
	unsigned long real_cont_id = 0 ;
	char auth_num[3];
	char limit_yn[1+1];

	#ifdef _DEBUG_
	printf("com9004-> start-------------------------------\n");
	printf("com9004-> temp_id  (%lld)\n", dcom9004data.temp_id  );
	printf("com9004-> user_id  (%s)\n", dcom9004data.user_id  );
	printf("com9004-> szVersionData  (%s)\n", dcom9004data.szVersionData  );//no.767
	#endif
	//infLOG(ALWAY,"\ncom9004 - start\n"
	//             "com9004 - temp_id  (%lld)\n",  dcom9004data.temp_id   );



	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	bool bCloseDB = false;

	CMysqlCon MysqlCon(m_g_clMysqlPool,m_g_clMysqlPool->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );

	con = MysqlCon.GetMysqlCon();

	if (con == NULL )
	{
		infLOG(ERROR, "DCMD9004 | GetMysqlCon is null\n");

		int nRetry = 0;
		while (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			infLOG(ERROR, "DCMD9004 | Cannot DB Connect - GetMysqlCon is null\n");
			//infLOG(ERROR, "DCMD9004 | Mysql Error | Errno : %d | ErrMsg : %s \n\n", mysql_errno(con),mysql_error(con));
		}
		if( nRetry >= 5)
		{
			req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);
			infLOG(ERROR, "DCMD9004 | Cannot DB Connect \n");
			return HEADER_SIZE;
	    }


	    bCloseDB = true;
	    infLOG(ERROR,"DCMD9004 | Connect DB direct\n");
	}

	//--------------------------------------------------------------------------
	// 등록할 자료에 대한 상태 검사
	// 1. 위디스크가 아니면 bypass
	// 2. 성인자료가 아니면 bypass
	//--------------------------------------------------------------------------
	memset (szQuery,  0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT cont_gu, adult_yn, reg_user,sect_code,file_size ,reg_date ,real_cont_id, limit_yn"
	                 "  FROM zangsi.T_CONTENTS_TEMP"
	                 " where id =  %lld"
	                 ,  dcom9004data.temp_id );


	#ifdef _DEBUG_
	printf( "%s\n",szQuery);
	#endif

	if (mysql_query(con, szQuery))
	{
		infLOG(ERROR, "com9004[ERR]: SELECT T_CONTENTS_TEMP error\n");
		infLOG(ERROR, "com9004[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		if( bCloseDB )
			db_disconnect(con);
		infLOG(ERROR, "com9004[SQL]: %s\n", szQuery);
		req_header.nCmd = -1 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);

       	return HEADER_SIZE;
	}

	if (!(res = mysql_store_result(con)))
	{
		infLOG(ERROR, "com9004[ERR]: mysql_store_result error\n");
		infLOG(ERROR, "com9004[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		if( bCloseDB )
			db_disconnect(con);
		infLOG(ERROR, "com9004[SQL]: %s\n", szQuery);
		req_header.nCmd = -1 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);

       	return HEADER_SIZE;
	}
 	if (mysql_num_rows(res)==0)
 	{
		infLOG(ERROR, "com9004[ERR]: mysql_num_rows error\n");
		infLOG(ERROR, "com9004[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		if( bCloseDB )
			db_disconnect(con);
		infLOG(ERROR, "com9004[SQL]: %s\n", szQuery);
		req_header.nCmd = -1 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);

       	return HEADER_SIZE;
	}
	row = mysql_fetch_row(res);
	memset(cont_gu , 0x00, sizeof(cont_gu ));
	memset(adult_yn, 0x00, sizeof(adult_yn));
	memset(reg_user, 0x00, sizeof(reg_user));
	memset(sect_code, 0x00, sizeof(sect_code));
	memset(limit_yn, 0x00, sizeof(limit_yn));

	real_cont_id = 0 ;

	strcpyA(cont_gu , getstr(row, 0));
	strcpyA(adult_yn, getstr(row, 1));
	strcpyA(reg_user, getstr(row, 2));
	strcpyA(sect_code, getstr(row, 3));
	temp_file_size = (double)getnum(row,4);
	strcpyA(reg_date, getstr(row, 5));
	real_cont_id = (unsigned long)getint(row,6);
	strcpyA(limit_yn, getstr(row, 7));


	mysql_free_result(res);

//	infLOG(ERROR, "com9004 값 확인 : cont_gu (%s) ,adult_yn (%s)\n",cont_gu,adult_yn);

	//infLOG(ALWAY, "com9004 값 확인 : temp_id ( %lld ) reg_user ( %s )  cont_gu (%s) ,adult_yn (%s) \r\ntemp_file_size (%f ) dcom9004data.file_size (%f) real_cont_id (%lu)\n",dcom9004data.temp_id,reg_user,cont_gu,adult_yn,temp_file_size,dcom9004data.file_size,real_cont_id);



	strcpyA( dcom9004data.user_id,reg_user);

	memset (szQuery,  0x00, sizeof(szQuery));
	sprintf(szQuery, " select auth_num from zangsi.T_PERM_UPLOAD_AUTH "
						 " where user_id = '%s'  "
						 , dcom9004data.user_id);



	#ifdef _DEBUG_
	printf( "%s\n",szQuery);
	#endif

	if (mysql_query(con, szQuery))
	{
		infLOG(ERROR, "com9004[ERR]: SELECT T_PERM_UPLOAD_AUTH error\n");
		infLOG(ERROR, "com9004[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		if( bCloseDB )
			db_disconnect(con);
		infLOG(ERROR, "com9004[SQL]: %s\n", szQuery);
		req_header.nCmd = -1 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);

       	return HEADER_SIZE;
	}

	if (!(res = mysql_store_result(con)))
	{
		infLOG(ERROR, "com9004[ERR]: T_PERM_UPLOAD_AUTH mysql_store_result error\n");
		infLOG(ERROR, "com9004[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		if( bCloseDB )
			db_disconnect(con);
		infLOG(ERROR, "com9004[SQL]: %s\n", szQuery);
		req_header.nCmd = -1 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);

       	return HEADER_SIZE;
	}
 	if (mysql_num_rows(res)==0)
 	{
 		infLOG(ERROR, "com9004[SQL]: [ %s ] \n", szQuery);
		infLOG(ERROR, "com9004[ERR]: T_PERM_UPLOAD_AUTH 업로드 값이 없습니다.\n");
		infLOG(ERROR, "com9004[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
	}
	else
	{
		row = mysql_fetch_row(res);

		memset(auth_num , 0x00, sizeof(auth_num ));
		strcpyA(auth_num , getstr(row, 0));
		strcpyA(dcom9004return.auth_num, auth_num);

	}
	mysql_free_result(res);



	#ifdef _DEBUG_
	printf( "\n업로드 권한 : %s\n",dcom9004return.auth_num);
	#endif

	//infLOG(ALWAY,"\n업로드 권한 : %s\n",dcom9004return.auth_num);






	if (strcmp(cont_gu, "WE")!=0)
	{
		if( bCloseDB )
			db_disconnect(con);
		//infLOG(ALWAY, "com9004[SQL]: %s\n", szQuery);
		if( strcmp(cont_gu, "FD") == 0 )
		{
			req_header.nCmd = -4 ;
		}
		else
		{
			req_header.nCmd = 9004 ;
		}

		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);

       	return HEADER_SIZE;
	}

	//infLOG(ALWAY, "\ncom9004[SQL]: dcom9004data.file_size  ( %f ) | temp_file_size ( %f ) adult_yn ( %s ) real_cont_id (%lu )\n", dcom9004data.file_size , temp_file_size,adult_yn,real_cont_id);

	//파일 사이즈 변경 했는지 검사
	if(  dcom9004data.file_size != temp_file_size )
	{

		infLOG(ERROR, "com9004[ERR]: 템프 %lld 번 파일 사이즈 변경. 검사 하세요.[%ld][%ld]\n\n", dcom9004data.temp_id, dcom9004data.file_size, temp_file_size );
		if( bCloseDB )
			db_disconnect(con);

		infLOG(ALWAY, "com9004[SQL]: %s\n", szQuery);
		req_header.nCmd = -3 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);

       	return HEADER_SIZE;

	}
	/*
		infLOG(ALWAY,"dcom9004 확인 : \n"
			" long long temp_id = %lld      \n"
			" double file_size	= %15.0f    \n"
			" char user_id[16]  = %s        \n"
			" char auth_num[3]  = %s        \n"
			,	dcom9004return.temp_id ,dcom9004return.file_size	,dcom9004return.user_id , dcom9004return.auth_num );
	*/

	//제휴 아이디는 성인 검사 하지 않음.
	if( strcmp(dcom9004return.auth_num,"CPR") == 0 ) //제휴 아이디
	{
		req_header.nCmd = 9004 ;
		req_header.nDataCnt = 1;
		req_header.nDataSize = sizeof(COM9004D);//sizeof(temp_id); //unsigend long
		pSendData = new char [HEADER_SIZE + sizeof(COM9004D) ];

		memcpy(pSendData,&req_header,HEADER_SIZE);
		memcpy(pSendData + HEADER_SIZE ,&dcom9004return, sizeof(COM9004D));

       	return HEADER_SIZE + sizeof(COM9004D);

	}

	if (strcmp(adult_yn, "Y")!=0)
	{
		if( bCloseDB )
			db_disconnect(con);
		//infLOG(ALWAY, "com9004[SQL]: %s\n", szQuery);
		req_header.nCmd = 9004 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);

       	return HEADER_SIZE;
	}


/*

	if( real_cont_id  >  0 )
	{
		if( bCloseDB )
			db_disconnect(con);
		infLOG(ALWAY, "com9004[SQL]: %s\n", szQuery);
		req_header.nCmd = 9004 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);

       	return HEADER_SIZE;
	}
*/



	//--------------------------------------------------------------------------
	// 등록할 자료에 대한 상태 검사
	// 성인 자료 갯수 검사
	//--------------------------------------------------------------------------
	memset (szQuery,  0x00, sizeof(szQuery));

	/*
	sprintf(szQuery, "SELECT count(id) "
	                 "  FROM T_CONTENTS_INFO"
	                 " WHERE reg_user = '%s'"
	                 "   AND adult_yn = 'Y'"
	                 "   AND del_yn   = 'N'"
	                 , reg_user);
	 */

	/*
    sprintf(szQuery, "select count(id)                                        "
                     " from T_CONTENTS_LIMIT                         		    "
                     "  where reg_user = '%s'                                 "
                     //"	 and sect_code = '%s'                               "
                     "	 and adult_yn = 'Y'                                 "
                     //"	 and (TO_DAYS(curdate()) - TO_DAYS(reg_date)) <= 30 "
                     "	 and reg_date = '%s' "
                     ,reg_user,reg_date);
                     //,sect_code);
    */

 //2009-01-30 성인 개수제한 하지않음. -- HCS
/*
     if( strcmp(limit_yn , "Y") == 0 && (strcmp(sect_code , "11") == 0 ||  (strcmp(sect_code , "03") == 0 )))
     {
		sprintf(szQuery, "select count(id)          "
							" from T_CONTENTS_LIMIT "
							"  where reg_user = '%s' "
							"	 and adult_yn = 'Y'  "
							"   and sect_code in ('11','03') "
							,reg_user );


	    strcat(szQuery, " and reg_date = date_format(now(),'%Y%m%d') " );



		infLOG(ALWAY, "com9004[SQL]: %s\n", szQuery);

		if (mysql_query(con, szQuery)){
			infLOG(ERROR, "com9004[ERR]: SELECT T_CONTENTS_INFO error\n");
			infLOG(ERROR, "com9004[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			if( bCloseDB )
				db_disconnect(con);
			infLOG(ERROR, "com9004[SQL]: %s\n", szQuery);
			req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);

	       	return HEADER_SIZE;
		}

		if (!(res = mysql_store_result(con)))
		{
			infLOG(ERROR, "com9004[ERR]: mysql_store_result error\n");
			infLOG(ERROR, "com9004[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			if( bCloseDB )
				db_disconnect(con);
			infLOG(ERROR, "com9004[SQL]: %s\n", szQuery);
			req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);

	       	return HEADER_SIZE;
		}
	 	if (mysql_num_rows(res)==0)
	 	{
			infLOG(ALWAY, "com9004[ERR]: mysql_num_rows error\n");
			infLOG(ALWAY, "com9004[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			mysql_free_result(res);
			if( bCloseDB )
				db_disconnect(con);
			infLOG(ALWAY, "com9004[SQL]: %s\n", szQuery);
			req_header.nCmd = 9004 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);

	       	return HEADER_SIZE;
		}
		row = mysql_fetch_row(res);
		reg_count = 0;
		reg_count = getint(row, 0);

		mysql_free_result(res);
	}
*/
	if( bCloseDB )
		db_disconnect(con);
/*
   #ifdef __DEBUG
   printf("com9004-> 성인 컨텐츠 갯수 (%d)\n",reg_count);
   #endif

	if ( reg_count >= 2 )
	{
		#ifdef _DEBUG_
		printf("com9004-> end _return value( -2 )\n");
		#endif
		infLOG(ERROR, "com9004[SQL]: 성인물 컨텐츠는 하루에 두건만 등록 가능 합니다.\n");
		req_header.nCmd = -2 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);

       	return HEADER_SIZE;
	}
*/
	#ifdef _DEBUG_
	printf("com9004-> end _return value( 1 )\n");
	#endif
	req_header.nCmd = 9004 ;
	pSendData = new char[HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);


	return HEADER_SIZE ;
}
