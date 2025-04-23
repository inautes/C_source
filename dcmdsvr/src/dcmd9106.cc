/******************************************************************************
 *   서브시스템 : FUP서버
 *   프로그램명 : com9106.h
 *         기능 : upload 시 통합 db에 파일이 있는지 조회
 *         설명 : 가라 업로드 하기 위한 처리
 *       작성자 : 김일오
 *       작성일 : 2015.03.12
 *     수정이력 :

-- 일반 컨텐츠
T_CONTENTS_FILE -- 데이터 들어있음.
T_CONTENTS_FILELIST
T_CONTENTS_TEMPLIST

-- 개인자료실 , 해쉬정보가 데이블에 없다. 어디에 있는걸까? 
T_CONTDATA_MYFILE -- 데이터 들어있음.
T_CONTDATA_MYFILELIST -- 폴더구조
T_CONTENTS_TEMP
T_CONTENTS_TEMPLIST

-- 필로그
T_CONTFLOG_FILE    -- 데이터 들어있음
T_CONTFLOG_FILELIST
T_CONTFLOG_TEMPLIST 
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
#include "com9106.h"
#include "dcmd9106.h"


#include "mysql_pool.h"
#include "MysqlDB.h"

extern CMysqlPool *m_g_clMysqlPool;


void printv(LPCCOM9106_R pV)
{
	printf(" com9106-> start\n com9106-> temp_id  (%lu), com9106-> seq_no  (%lu)\n " , pV->temp_id  , pV->seq_no );  
}

//******************************************************************************
//  COM9106 main
//
//  input : pcom9106_r->proc_flag => 1=wedisk, 2=mydata
//
//  return:  1(정상)
//          -1(DB오류)
//******************************************************************************
long dcmd9106(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData)//CCOM9106_R pcom9106_r)
{
 	LPCCOM9106_R pcom9106_r = (LPCCOM9106_R)pRecvData; // pcom9106_r -> pcom9106_return 로 전부 변경하자.
	
	CCOM9106_R pcom9106_return; 
	memset(&pcom9106_return,0x00,sizeof(CCOM9106_R));
	memcpy(&pcom9106_return , pRecvData,sizeof(CCOM9106_R));

	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);
	int nRetry = 0;
	MYSQL       *con=NULL;
	
	#ifdef _DEBUG
	printv(pcom9106_r);
	#endif
				
	int ret =0 ;
	
	// main db 연결
	CMysqlDB main_db;
	main_db.setDebug(true);

	CMysqlCon MysqlCon;
	MysqlCon.ConnectPool(m_g_clMysqlPool,m_g_clMysqlPool->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );
	main_db.setCon(MysqlCon.GetMysqlCon());

	if ( main_db.getCon() == NULL )
	{
		infLOG(ERROR, "dcmd9106: GetMysqlCon is null\n");
				
		while (!(con=db_connect("nori")) && nRetry < 5 ) {
			nRetry++;sleep(1);
		}
		
		if( nRetry >= 5){			
			req_header.nCmd = -1 ;	
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);
			return HEADER_SIZE;
	    }
	    
	    main_db.setCon(con);
	    main_db.setUseDisconnect(true);
	}


	//start	
	if(pcom9106_r->service_type  == 1)    // wedisk
	{
		ret = main_db.num_rows(" SELECT a.file_id From zangsi.T_CONTENTS_TEMPLIST a where a.file_id is not null and a.id = %lu and a.default_hash = '%s' ",
						   pcom9106_r->temp_id,pcom9106_r->default_hash );
	}else if(pcom9106_r->service_type  == 2) // mydisk
	{
		//ret = main_db.num_rows(" SELECT a.file_id, a.server_group_id From zangsi.T_CONTENTS_TEMP a where a.file_id is not null and a.server_group_id is not null and a.id = %lu and a.seq_no = %lu; ",	   pcom9106_r->temp_id,pcom9106_r->seq_no);
	}else if(pcom9106_r->service_type  == 3) // mydata
	{
		//ret = main_db.num_rows(" SELECT a.file_id, a.server_group_id From zangsi.T_CONTENTS_TEMPLIST a where a.file_id is not null and a.server_group_id is not null and a.id = %lu and a.seq_no = %lu; ",	   pcom9106_r->temp_id,pcom9106_r->seq_no);
	}else  if(pcom9106_r->service_type  == 4) // filog
	{
		ret = main_db.num_rows(" SELECT a.file_id From zangsi.T_CONTFLOG_TEMPLIST a where a.file_id is not null and a.id = %lu and a.default_hash = '%s' ",
						   pcom9106_r->temp_id,pcom9106_r->default_hash);
	}else
	{
		// error
	}

	if ( ret < 0 )
 	{
 		main_db.free_result();
 		main_db.disconnect_db();
 		
		pcom9106_return.b_duplicate = 0;
 		infLOG(ERROR, "com9106 : ret [ %d ] query [ %s ] --> error msg : [ %s ]\n",ret, main_db.getQueryString(),main_db.getErrorMessage());
		
		req_header.nCmd = -1 ;	
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);
	    return HEADER_SIZE;
	}
	else
	{
		 if(main_db.fetch_row() != NULL)
		 {
				 pcom9106_return.file_id = (unsigned long)main_db.getnum(0);

				 main_db.free_result();

				 if(pcom9106_return.file_id != 0)
					 pcom9106_return.b_duplicate = 1;
				 else
 					 pcom9106_return.b_duplicate = 0;

				 infLOG(ALWAY, "com9106 return : file_id [ %lu ] \n",pcom9106_return.file_id);
		}
		else
		{
	 		main_db.free_result();
	 		main_db.disconnect_db();
	 		
			pcom9106_return.b_duplicate = 0;
	 		infLOG(ERROR, "com9106 : query [ %s ] --> error msg : [ %s ]\n",main_db.getQueryString(),main_db.getErrorMessage());
			
			req_header.nCmd = -1 ;	
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);
					
	    return HEADER_SIZE;			
		}
	}
	main_db.disconnect_db();

	req_header.nCmd = 9106 ;	
	req_header.nDataCnt = 1;
	req_header.nDataSize = sizeof(CCOM9106_R);
	pSendData = new char [HEADER_SIZE + sizeof(CCOM9106_R) ];
	
	memcpy(pSendData,&req_header,HEADER_SIZE);
	memcpy(pSendData + HEADER_SIZE ,&pcom9106_return, sizeof(CCOM9106_R));
	
	return HEADER_SIZE + sizeof(CCOM9106_R);
}
