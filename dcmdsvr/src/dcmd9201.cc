/******************************************************************************
 *   서브시스템 : FDN서버
 *   프로그램명 : dcmd9201.cc
 *         기능 : 중복접속제거
 *         설명 : 여러군데서 한 아이피로 중복 다운로드 하는 회원에게 끊기 소켓 보내기.
 *     수정이력 :
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#include <mysql.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "apdefine.h"
#include "comcomm.h"
#include "commydb.h"
#include "com9201.h"
#include "dcmd9201.h"

//#define  _DEBUG_

// db 제어 서버
#include <unistd.h>     /* for close() getpid()*/
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include "dcmdsock.h" //sock send recv
// db 제어 서버

#include "mysql_pool.h"
#include "MysqlDB.h"

extern CMysqlPool * m_g_clMysqlPool;
extern CMysqlPool* m_g_clMysqlPoolDnLog;


//******************************************************************************
//  COM9201 main
//
//  input : pCom9201_r->proc_flag => 1=wedisk, 2=mydata
//
//  return:  1(정상)
//          -1(DB오류)
//******************************************************************************
long dcmd9201(int sock , LPHEADER pHeader , char* pRecvData , char* &pSendData)//CCOM9201_R pCom9201_r)
{

	LPCCOM9201_R pCom9201_r = (LPCCOM9201_R)pRecvData;

	HEADER req_header;
	memcpy(&req_header , pHeader,HEADER_SIZE);
	//infLOG(ALWAY, "CMD9201 |  \n");



/*
	 다중 다운로드 관련 하여 수정 - 20081224.
	 dnmserver 서버와 연동
	 사용하지 않을시에는 [시작] 부터 [끝]  까지 주석 처리 하면 됩니다.
*/

//[시작]

	char szQuery[10000];
	MYSQL       *con=NULL;
	MYSQL_RES *res;
	MYSQL_ROW  row;

	#ifdef _DEBUG_
	printf("com9201-> start-------------------------------\n");
	printf("com9201-> user_id  (%s)\n" , pCom9201_r->user_id  );
	printf("com9201-> conn_ip  (%s)\n" , pCom9201_r->conn_ip  );
	printf("com9201-> cer_key  (%s)\n" , pCom9201_r->cer_key  );
	#endif
/*
	infLOG(ALWAY, "dcmd9002 - start\n"
	              "dcmd9002 - conn_ip(%s)\n"
	              "dcmd9002 - user_id  (%s)\n"
	              "dcmd9002 - cer_key  (%s)\n"
	              , pCom9201_r->conn_ip, pCom9201_r->user_id, pCom9201_r->cer_key);
*/

	if(strlen(pCom9201_r->user_id) <= 0 && strlen(pCom9201_r->cer_key) <= 0)
	{
		#ifdef _DEBUG_
		printf(">com9201 user_id : %d, cer_key : %d\n", strlen(pCom9201_r->user_id), strlen(pCom9201_r->cer_key));
		#endif
		req_header.nCmd = 9201 ;
		pSendData = new char[HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);

		return HEADER_SIZE;
	}

/*
	// DB 연결
	bool bCloseDB = false;


	CMysqlCon MysqlCon(m_g_clMysqlPool,m_g_clMysqlPool->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );

	con = MysqlCon.GetMysqlCon();

	if (con == NULL )
	{
    	infLOG(ERROR, "GetMysqlCon is null \n");
		req_header.nCmd = -1 ;


		int nRetry = 0;
		while (!(con=db_connect(OSP_DB_NAME		,OSP_DB_IP_PUB		,OSP_DB_DCMD_USER	,OSP_DB_DCMD_PASS )) && nRetry < 5 )
		{
			nRetry++;
			sleep(1);
			#ifdef __DEBUG_
			printf(" ] DB 접속 재시도 \n");
			#endif
		}

		if( nRetry >= 5)
		{

			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);

	       	return HEADER_SIZE;

	    }
	    bCloseDB = true;


	}


*/

	// 다중 다운로드 관련 하여 수정 - 20081224

	CMysqlCon MysqlConDnLog;
	MysqlConDnLog.ConnectPool(m_g_clMysqlPoolDnLog,m_g_clMysqlPoolDnLog->GetPrimaryKey());//getpid() + m_g_clMysqlPool->GetPrimaryKey() );

	MYSQL* pDCon = MysqlConDnLog.GetMysqlCon();

	if( pDCon == NULL )
	{
		infLOG(ERROR,"9201 치명적인(Critical Error) 오류 입니다.\n");

		req_header.nCmd = 9201 ;
		pSendData = new char[HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);

		return HEADER_SIZE;
	}
	//--------------------------------------------------------------------------
	// 다운로드사용자 처리
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));

	if(strlen(pCom9201_r->cer_key) > 0)
	{
		sprintf(szQuery, "select conn_ip, id , server_id from T_CONTENTS_CERKEY_UPDN "
		                 " where cer_key = '%s' and updn_flag = 'DN'  "
		                 , pCom9201_r->cer_key
		                 );
	}
	else if(strlen(pCom9201_r->user_id) > 0 )
	{
		sprintf(szQuery, "select conn_ip, id , server_id from zangsi.T_CONTENTS_UPDN "
		                 " where user_id = '%s' and updn_flag = 'DN'  "
		                 , pCom9201_r->user_id
		                 );
	}

	#ifdef _DEBUG_
	printf(">com9201 szQuery : [%s]\n", szQuery);
	#endif

	//infLOG(ALWAY, "dcmd9201[SQL]: %s\n", szQuery);

	if (mysql_query(pDCon, szQuery))
	{
		infLOG(ERROR, "com9201[ERR]: [ %s ] [%d](%s)\n",szQuery, mysql_errno(pDCon), mysql_error(pDCon));
		//DCon.CloseDB();
		/*
		if( bCloseDB )
			db_disconnect(con);
		*/
		infLOG(ERROR, "dcmd9201[SQL]: %s\n", szQuery);
		req_header.nCmd = -1 ;
		pSendData = new char [HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);
		return HEADER_SIZE;

		return -1;

    }



	if (!(res = mysql_store_result(pDCon)))
	{
	    infLOG(ERROR, "daem9201_process: mysql_store_result error...\n");
		infLOG(ERROR, "[%d](%s)",mysql_errno(pDCon), mysql_error(pDCon));
		//DCon.CloseDB();
		/*
		if( bCloseDB )
			db_disconnect(pDCon);
		*/
		return -1;

	}

 	if (mysql_num_rows(res)==0 || strcmp(pCom9201_r->user_id , "운영팀") == 0)
 	{
		mysql_free_result(res);

/*
		infLOG(ALWAY, "daem9201_process: 처리할 자료가 없습니다.\n "
		 			  " insert ( %s ) ( %s )\n",pCom9201_r->user_id, pCom9201_r->conn_ip  );
*/
		#ifdef __DEBUG
		printf("daem9201_process: 처리할 자료가 없습니다.\n "
		 			  " insert ( %s ) ( %s )\n",pCom9201_r->user_id, pCom9201_r->conn_ip  );
		#endif

		sprintf(szQuery, " REPLACE zangsi.T_CONTENTS_UPDN (updn_flag, user_id , cont_gu,server_id ,conn_ip,id , reg_date,reg_time ) "
						 " VALUES( 'DN', '%s','','', '%s' ,0 "                  , pCom9201_r->user_id , pCom9201_r->conn_ip  );
		strcat( szQuery , " , date_format(now(),'%Y%m%d') , date_format(now(),'%H%i%s') )               " );


		if (mysql_query(pDCon, szQuery))
		{
			infLOG(ERROR, "com9201[ERR]: [ %s ] [%d](%s)\n",szQuery, mysql_errno(pDCon), mysql_error(pDCon));


			#ifdef __DEBUG
			printf("com9201[ERR]: [ %s ] [%d](%s)\n",szQuery, mysql_errno(pDCon), mysql_error(pDCon));
			#endif
			//DCon.CloseDB();
			/*
			if( bCloseDB )
				db_disconnect(pDCon);
			*/
			infLOG(ERROR, "dcmd9201[SQL]: %s\n", szQuery);
			req_header.nCmd = -1 ;
			pSendData = new char [HEADER_SIZE];
			memcpy(pSendData,&req_header,HEADER_SIZE);
			return HEADER_SIZE;


			return -1;

	    }

	    //DCon.CloseDB();
	    /*
		if( bCloseDB )
			db_disconnect(pDCon);
		*/
		//infLOG(ALWAY, "com9201 : [ %s ]\n",szQuery);
		//20100825 - no.582
		req_header.nCmd = 9201 ;
		pSendData = new char[HEADER_SIZE];
		memcpy(pSendData,&req_header,HEADER_SIZE);

		return HEADER_SIZE;
	}

    //infLOG(ALWAY, "daem9201_process: select_cnt(%d)\n", mysql_num_rows(res));

	int nRowcnt = 0;

	while((row = mysql_fetch_row(res)))
	{


		#ifdef __DEBUG
		printf(" [ %d ] old_ip ( %s ) new_ip ( %s ) contents id ( %lu ) user_id ( %s ) \n",nRowcnt,getstr(row,0),pCom9201_r->conn_ip,(unsigned long)getint(row,1),getstr(row,2));
		#endif




		if( strcmp(getstr(row,0),pCom9201_r->conn_ip) != 0 && strcmp(getstr(row,0),REMOTE_SERVER_IP) != 0  )
		{

			//infLOG(ALWAY,"dcmd9201 | user_id [ %s ] ip [ %s ]\n",pCom9201_r->user_id ,pCom9201_r->conn_ip);
			infLOG(ALWAY," [ %s ] user_id [ %d ] old_ip ( %s ) new_ip ( %s ) contents id ( %lu ) server_id ( %s ) \n",pCom9201_r->user_id,nRowcnt,getstr(row,0),pCom9201_r->conn_ip,(unsigned long)getint(row,1),getstr(row,2));

			nRowcnt++;



			PACKET101 Packet101_r;
			memset(&Packet101_r,0x00,sizeof(PACKET101));
			strcpyA(Packet101_r.szCmd ,"1");
			if( sizeof(pCom9201_r->user_id) <12 )
			{
				memcpy(Packet101_r.szUserID,pCom9201_r->user_id,sizeof(pCom9201_r->user_id));
			}
			else
			{
				memcpy(Packet101_r.szUserID,pCom9201_r->user_id,12 );
			}
/*
			if(strlen(pCom9201_r->cer_key) > 0)
				strcpyA(Packet101_r.szCerKey , pCom9201_r->cer_key);
*/
			strcpyA(Packet101_r.szRemoteIP , getstr(row,0));
			sprintf(Packet101_r.szMsg,"IP 가 %s 인 사용자의 다운로드 요청으로 인하여 접속을 종료 합니다.\n" , pCom9201_r->conn_ip);

			struct sockaddr_in disSerAddr;

			int dis_socket;
			if ( ( dis_socket = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0 )
			{
				infLOG(ERROR, "COM9201[ERR]: 소켓 생성 오류 입니다.   ( %s ) \n",Packet101_r.szUserID);
		       	mysql_free_result(res);
		       	//DCon.CloseDB();
		       	/*
		       	if( bCloseDB )
					db_disconnect(pDCon);
				*/
		       	return(-1);
			}

			//comhead.h 에 REMOTE_SERVER_IP | REMOTE_CLIENT_PORT 정의 되어 있음.
			memset(&disSerAddr,0x00,sizeof(disSerAddr));
			disSerAddr.sin_family      = AF_INET;
			//disSerAddr.sin_addr.s_addr = inet_addr(CMD_SERVER_IP);
			disSerAddr.sin_addr.s_addr = inet_addr(REMOTE_SERVER_IP);
			disSerAddr.sin_port        = htons(REMOTE_CLIENT_PORT);
			//infLOG(ALWAY,"dn 끊기 소켓 생성 및 접속 ( %s ) ( %s ) ( %d )\n",Packet101_r.szRemoteIP,REMOTE_SERVER_IP,REMOTE_CLIENT_PORT);
			//infLOG(ALWAY,"dn 끊기 소켓  접속 ( %s ) ( %s ) ( %s )\n",Packet101_r.szUserID,Packet101_r.szRemoteIP,REMOTE_SERVER_IP);

			// connect client
			if( connect(dis_socket,(struct sockaddr * )&disSerAddr,sizeof(disSerAddr)) < 0 )
			{
				//infLOG(ERROR, "COM9201[ERR]: 원격 사용자 서버 접속 오류. ( %s ) \n",Packet101_r.szUserID);

				mysql_free_result(res);
				//DCon.CloseDB();
				/*
		       	if( bCloseDB )
					db_disconnect(pDCon);
				*/
		       	return(-1);
			}
			#ifdef __DEBUG
			printf("데이터 확인 \n");
			printf("Header.nCmd ( %s )\nHeader.szUserID ( %s ) \nDATA.szRemoteIP(%s)\nDATA,szMsg( %s )\n"
					,Packet101_r.szCmd,Packet101_r.szUserID,Packet101_r.szRemoteIP,Packet101_r.szMsg);
			printf(" 사이즈 ( %d ) \n",sizeof(PACKET101));
			#endif


			if(SendData(dis_socket,(char*)&Packet101_r, sizeof(PACKET101) ) < 0)
			{
				//			   012345678901234]
				int err  =errno;
				infLOG(ERROR, "RequestDisconnectUser send ] sendData  (%d)\n",err);
				#ifdef __DEBUG
				printf("RequestDisconnectUser send ] sendData Error \n");
				#endif



				mysql_free_result(res);
				//DCon.CloseDB();
				/*
				if( bCloseDB )
					db_disconnect(pDCon);
				*/
				close(dis_socket);
				return -1;


			}
			#ifdef __DEBUG
			printf("dn 끊기 소켓 접속 종료 ( %s )\n",Packet101_r.szRemoteIP);
			#endif
			close(dis_socket);
		}
		//
	}
	mysql_free_result(res);
	//DCon.CloseDB();
	/*
	if( bCloseDB )
		db_disconnect(pDCon);
	*/

	#ifdef _DEBUG_
	printf("com9201-> end\n");
	#endif


//[끝]



	req_header.nCmd = 9201 ;
	pSendData = new char[HEADER_SIZE];
	memcpy(pSendData,&req_header,HEADER_SIZE);

	return HEADER_SIZE;

}
