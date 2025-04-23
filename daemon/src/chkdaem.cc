/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : chkdaem.cc
 *         기능 : cmd upload download 서버 접속자수 체크
 *         설명 : cmd upload download 서버 접속자수 체크
                  
           실행 : crontab -e 
                  0,10,20,30,40,50 * * * * /home/ezwon/zangsi/bin/chkdaemon up 
                  05 * * * * /home/ezwon/zangsi/bin/chkdaemon
 *       작성자 : LEE
 *       작성일 : 2005/01/19
 *     수정이력 : HCS - 2007/12/13일 
 				   업로드 서버 동일기능 추가
 				  HCS - no.1001
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#include <mysql.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/time.h>
//#include <time.h>

#include "chkdaem.h"
#include "daemcom.h"
#include "commydb.h"
#include "comhead.h"
//#include "apdefine.h"
//#include "comcomm.h"

#include "chkdaem_socket.h"


//#define __DEBUG

/*===========================================================================*/
/* 전역변수 선언															 */
/*===========================================================================*/
MYSQL     *con;
MYSQL_RES *res;
MYSQL_ROW  row;
	
	
static char szRecvPhoneNum[20][20];
static bool bIsCheck = false;
char g_szServerID[5+1];
//static bool bCloseMain;
void upload_checker()
{

	time_t			curtime;
	struct tm		*stm;
	time( &curtime );
	stm = (struct tm *) localtime(&curtime);

	localtime_r(&curtime, stm);

	char szCurTime[100];
	memset(szCurTime,0x00,sizeof(szCurTime));
	sprintf(szCurTime ,"%04d년%02d월%02d일%02d시%02d분%02d초"
						,stm->tm_year+1900
						,  stm->tm_mon + 1
						,  stm->tm_mday
						,  stm->tm_hour
						,  stm->tm_min
						,  stm->tm_sec
						);
			
	#ifdef __DEBUG					
	printf("signal alram :\n"
		   "The Signal Alram Time is %s\n\n",szCurTime);
	#endif
		
	upload_checker_run();
}


void signal_caller(int sig)
{

	
		

	time_t			curtime;
	struct tm		*stm;
	time( &curtime );
	stm = (struct tm *) localtime(&curtime);

	localtime_r(&curtime, stm);

	char szCurTime[100];
	memset(szCurTime,0x00,sizeof(szCurTime));
	sprintf(szCurTime ,"%04d년%02d월%02d일%02d시%02d분%02d초"
						,stm->tm_year+1900
						,  stm->tm_mon + 1
						,  stm->tm_mday
						,  stm->tm_hour
						,  stm->tm_min
						,  stm->tm_sec
						);
			
	#ifdef __DEBUG					
	printf("signal alram :\n"
		   "The Signal Alram Time is %s\n\n",szCurTime);
	#endif
	
	run();
	
	
//	bCloseMain = false;
	 
//	signal(SIGALRM, signal_caller);
	
	
		  
}

int SendPhoneMsg(char* pPhoneNum, char* pMsg,char* pErrMsg)
{
    
	
	char szQuery[1000];  // query string
	int  ErrNum;         // error no
	int  nRowcnt;        // select row count


	/*--------------------------------------------------------------------*/
	/* start!!!                                                           */
	/*--------------------------------------------------------------------*/
	#ifdef _DEBUG_
	printf("Run send phone message Program ..... \n");
	#endif
	
	
	
	// 서버 목록 조회 
	memset (szQuery, 0x00, sizeof(szQuery ));
	
	sprintf(szQuery, "INSERT INTO zangsi.em_tran (                            "
	                 "       tran_id, tran_phone, tran_callback,              "
	                 "       tran_status ,  tran_date, tran_msg )             "
	                 "	VALUES('개발팀', '%s','016-244-7950', '1', now(), '%s')"
	                 ,pPhoneNum,pMsg);
	           
	#ifdef __DEBUG
	printf("INSERT INTO zangsi.em_tran (                                "
            "       tran_id, tran_phone, tran_callback,                 "
            "       tran_status ,  tran_date, tran_msg )                "
            "	VALUES('개발팀', '%s','016-244-7950', '1', now(), '%s')\n"
            ,pPhoneNum,pMsg);
	                 
	#endif


	

	ZzLOG(ALWAY,"> Query [ %s ] \n",szQuery);
	
	if (mysql_query(con, szQuery)){
		ErrNum = -100502;
		sprintf(pErrMsg, "검색시 오류가 발생하였습니다.\n");
		#ifdef _DEBUG_
		printf("> ERRMSG: [%d](%s)",ErrNum, pErrMsg);
		printf("> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
		#endif
		ZzLOG(ERROR,"> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
		
		
		
       	return(ErrNum); 
	}

	
      
	
	#ifdef _DEBUG_
	printf("end sucess Send Phone Message \n");
	#endif

	return (1);
}

int GetServerInfo(LPSERVER_INFO pServerInfo, int *nServercount ,char* pErrMsg, int nMode)
{
	char szQuery[1000];  // query string
	int  ErrNum;         // error no
	int  nRowcnt;        // select row count


	/*--------------------------------------------------------------------*/
	/* start!!!                                                           */
	/*--------------------------------------------------------------------*/
	#ifdef _DEBUG_
	printf("Run Process Check Program ..... \n");
	#endif
	
	
	
	// 서버 목록 조회 
	if(nMode == DOWNLOAD)
	{
		memset (szQuery, 0x00, sizeof(szQuery ));
		sprintf(szQuery, "select a.server_id,   a.server_ip,                 		"
		                 "       a.server_port,	a.dnsvr_port,                		"
		                 "       a.upsvr_port,  a.server_gu                  		"
		                 "  from zangsi.T_SERVER_INFO a                      		"
		                 "	where a.server_gu = '01' and user_cnt <> 300 and admin_open_yn = 'Y' 		" /*,'02','03','06')        "*/
	//	                 "order by a.server_id limit 10                              		"
		                 );
	}
	else if(nMode == UPLOAD)
	{
		memset (szQuery, 0x00, sizeof(szQuery ));
		sprintf(szQuery, "select a.server_id,   a.server_ip,                 		"
		                 "       a.server_port,	a.dnsvr_port,                		"
		                 "       a.upsvr_port,  a.server_gu                  		"
		                 "  from zangsi.T_SERVER_INFO a                      		"
		                 "	where a.server_gu = '01' and user_cnt <> 300 and upload_yn = 'Y' 		" /*,'02','03','06')        "*/
//		                 "order by a.server_id limit 10                              		"
		                 );
	}                 

	ZzLOG(ALWAY,"> Query [ %s ] \n",szQuery);
	

	if (mysql_query(con, szQuery))
	{
		ErrNum = -100502;
		sprintf(pErrMsg, "검색시 오류가 발생하였습니다.\n");
		#ifdef _DEBUG_
		printf("> ERRMSG: [%d](%s)",ErrNum, pErrMsg);
		printf("> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
		#endif
		ZzLOG(ERROR,"> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
		
		
		
       	return(ErrNum); 
	}

	//질의 결과를 얻는다
	if (!(res = mysql_store_result(con))) 
	{
		ErrNum = -100503;
		sprintf(pErrMsg, "검색시 오류가 발생하였습니다.\n");
	
		#ifdef _DEBUG_
		printf("> pErrMsg: [%d](%s)",ErrNum, pErrMsg);
		printf("> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
		#endif
	
		ZzLOG(ERROR,"> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
	
       	return(ErrNum); 
	}


	nRowcnt = 0;
 	if (mysql_num_rows(res)!=0)
 	{
		while((row = mysql_fetch_row(res))) 
		{
			#ifdef _DEBUG_
			printf("t_cmds1005s[nRowcnt].server_id(%s)\n", getstr(row, 0));
			printf("t_cmds1005s[nRowcnt].server_ip(%s)\n", getstr(row, 1));
			printf("t_cmds1005s[nRowcnt].my_port  (%d)\n", getint(row, 2));
			printf("t_cmds1005s[nRowcnt].dn_port  (%d)\n", getint(row, 3));
			printf("t_cmds1005s[nRowcnt].up_port  (%d)\n", getint(row, 4));
			printf("t_cmds1005s[nRowcnt].szServerCode  (%s)\n\n", getstr(row, 5));
			//printf("t_cmds1005s[nRowcnt].root_path(%s)\n", getstr(row, 4));
			#endif
			
			strcpy(pServerInfo[nRowcnt].server_id, getstr(row, 0));
			strcpy(pServerInfo[nRowcnt].server_ip, getstr(row, 1));
				   pServerInfo[nRowcnt].my_port  = getint(row, 2);
			       pServerInfo[nRowcnt].dn_port  = getint(row, 3);
			       pServerInfo[nRowcnt].up_port  = getint(row, 4);
			strcpy(pServerInfo[nRowcnt].szServerCode, getstr(row, 5));
			       
			
			
			nRowcnt++;
			
			*nServercount = nRowcnt;
			// buffer크기 보다 큰경우
			if (nRowcnt >= MAX_ROWS)
			{
			    #ifdef __DEBUG
				printf("Error : Server Count overflow Error\n");
				#endif
				
				mysql_free_result(res);
				


				return -1;
			}
		}
	}
	
	mysql_free_result(res);
	

	
	#ifdef _DEBUG_
	printf("end sucess GetServerInfo \n");
	#endif

	return (1);
}

int GetCashServerInfo(LPSERVER_INFO pServerInfo, int *nServercount ,char* pErrMsg)
{
	char szQuery[1000];  // query string
	int  ErrNum;         // error no
	int  nRowcnt;        // select row count


	/*--------------------------------------------------------------------*/
	/* start!!!                                                           */
	/*--------------------------------------------------------------------*/
	#ifdef _DEBUG_
	printf("Run Process Check Program ..... \n");
	#endif
	
	
	
	// 서버 목록 조회 
	memset (szQuery, 0x00, sizeof(szQuery ));
	sprintf(szQuery, "select a.server_id,   a.server_ip,                 		"
	                 "       a.server_port,	a.dnsvr_port,                		"
	                 "       a.upsvr_port,  a.server_gu                  		"
	                 "  from zangsi.T_SERVER_INFO a                      		"
	                 "	where a.server_gu = '01' and user_cnt = 300 and admin_open_yn = 'Y' 		" /*,'02','03','06')        "*/
//	                 "order by a.server_id limit 10                              		"
	                 );

	ZzLOG(ALWAY,"> Query [ %s ] \n",szQuery);
	

	if (mysql_query(con, szQuery))
	{
		ErrNum = -100502;
		sprintf(pErrMsg, "검색시 오류가 발생하였습니다.\n");
		#ifdef _DEBUG_
		printf("> ERRMSG: [%d](%s)",ErrNum, pErrMsg);
		printf("> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
		#endif
		ZzLOG(ERROR,"> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
		
		
		
       	return(ErrNum); 
	}

	//질의 결과를 얻는다
	if (!(res = mysql_store_result(con))) 
	{
		ErrNum = -100503;
		sprintf(pErrMsg, "검색시 오류가 발생하였습니다.\n");
	
		#ifdef _DEBUG_
		printf("> pErrMsg: [%d](%s)",ErrNum, pErrMsg);
		printf("> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
		#endif
	
		ZzLOG(ERROR,"> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
	
       	return(ErrNum); 
	}


	nRowcnt = 0;
 	if (mysql_num_rows(res)!=0)
 	{
		while((row = mysql_fetch_row(res))) 
		{
			#ifdef _DEBUG_
			printf("t_cmds1005s[nRowcnt].server_id(%s)\n", getstr(row, 0));
			printf("t_cmds1005s[nRowcnt].server_ip(%s)\n", getstr(row, 1));
			printf("t_cmds1005s[nRowcnt].my_port  (%d)\n", getint(row, 2));
			printf("t_cmds1005s[nRowcnt].dn_port  (%d)\n", getint(row, 3));
			printf("t_cmds1005s[nRowcnt].up_port  (%d)\n", getint(row, 4));
			printf("t_cmds1005s[nRowcnt].szServerCode  (%s)\n\n", getstr(row, 5));
			//printf("t_cmds1005s[nRowcnt].root_path(%s)\n", getstr(row, 4));
			#endif
			
			strcpy(pServerInfo[nRowcnt].server_id, getstr(row, 0));
			strcpy(pServerInfo[nRowcnt].server_ip, getstr(row, 1));
				   pServerInfo[nRowcnt].my_port  = getint(row, 2);
			       pServerInfo[nRowcnt].dn_port  = getint(row, 3);
			       pServerInfo[nRowcnt].up_port  = getint(row, 4);
			strcpy(pServerInfo[nRowcnt].szServerCode, getstr(row, 5));
			       
			
			
			nRowcnt++;

			strcpy(pServerInfo[nRowcnt].server_id, getstr(row, 0));
			strcpy(pServerInfo[nRowcnt].server_ip, getstr(row, 1));
				   pServerInfo[nRowcnt].my_port  = getint(row, 2);
			       pServerInfo[nRowcnt].dn_port  = 5002;
			       pServerInfo[nRowcnt].up_port  = getint(row, 4);
			strcpy(pServerInfo[nRowcnt].szServerCode, getstr(row, 5));
			       
			
			
			nRowcnt++;

			strcpy(pServerInfo[nRowcnt].server_id, getstr(row, 0));
			strcpy(pServerInfo[nRowcnt].server_ip, getstr(row, 1));
				   pServerInfo[nRowcnt].my_port  = getint(row, 2);
			       pServerInfo[nRowcnt].dn_port  = 5004;
			       pServerInfo[nRowcnt].up_port  = getint(row, 4);
			strcpy(pServerInfo[nRowcnt].szServerCode, getstr(row, 5));
			       
			
			
			nRowcnt++;
			
			*nServercount = nRowcnt;
			// buffer크기 보다 큰경우
			if (nRowcnt >= MAX_ROWS)
			{
			    #ifdef __DEBUG
				printf("Error : Server Count overflow Error\n");
				#endif
				
				mysql_free_result(res);
				


				return -1;
			}
		}
	}
	
	mysql_free_result(res);
	

	
	#ifdef _DEBUG_
	printf("end sucess GetServerInfo \n");
	#endif

	return (1);
}

int CheckServer(LPSERVER_INFO pServerInfo , int nServerCount,int nMode)
{

	if(nMode == UPLOAD)
		ZzLOG(ALWAY, " [checking server] 업로드 서버 채크 \n");  
	else
		ZzLOG(ALWAY, " [checking server] 다운로드 서버 채크 \n");  
			

	char szServerIP[16] ;
	char szTempServerID[3];
	char szServerID[5];

	memset(szServerID,0x00,sizeof(szServerID));			
	memset(szTempServerID,0x00,sizeof(szTempServerID));			
	memset(szServerIP,0x00,sizeof(szServerIP));
	
	unsigned int nDNPort ;
	unsigned int nUPPort ;
	unsigned int nCMDPort ;
	unsigned int nWEBPort ;


    char szPhoneSendErrMsg[256];
    char szPhoneMsg[255];
    
	ZzLOG(ALWAY, " [검사할 서버 댓수 : %d]  \n", nServerCount);  

	for(int i=0;i < nServerCount; i++)
	{
		memset(szServerIP,0x00,sizeof(szServerIP));
		nDNPort = 0;
		nUPPort = 0;
		
		memset(szServerID,0x00,sizeof(szServerID));			
		memset(szTempServerID,0x00,sizeof(szTempServerID));			
		memset(szServerIP,0x00,sizeof(szServerIP));
		
		
		strcpy( szServerID , pServerInfo[i].server_id);
		strcpy( szServerIP , pServerInfo[i].server_ip);
		
		memcpy(szTempServerID, pServerInfo[i].server_id,3);
	
	    memset(szPhoneMsg,0x00,255);
	    memset(szPhoneSendErrMsg,0x00,256);
        
        #ifdef __DEBUG
        printf(" %s : %s\n",szServerID,szServerIP);  
        #endif
        ZzLOG(ALWAY," %s : %s\n",szServerID,szServerIP);  
        
    	
    	
		//포트 결정
		//if( strcmp(szTempServerID,"WES") == 0 || strcmp(szTempServerID,"MYS") == 0 || strcmp(szTempServerID,"WS") == 0)
		{
			//wedisk
			nDNPort = pServerInfo[i].dn_port;
			nUPPort = pServerInfo[i].up_port;

            if( nMode == UPLOAD )
            {
				if( ConnectServer(szServerID,szServerIP,nUPPort,FILE_UP_SERVER) <= 0)
				{
			        sprintf( szPhoneMsg ,"%s 업로드 서버가 죽었습니다.", pServerInfo[i].server_id);
			        ZzLOG(ERROR, szPhoneMsg);  	    
			        
					char szQuery[1024];
						        
			        // db update
			        memset(szQuery,0x00,sizeof(szQuery));
					sprintf(szQuery,"UPDATE zangsi.T_SERVER_INFO set upload_yn = 'N' where server_id = '%s'",pServerInfo[i].server_id);
	

					#ifdef __DEBUG
					printf("> Query [ %s ] \n",szQuery);
					#endif
					ZzLOG(ALWAY,"> Query [ %s ] \n",szQuery);
	
					if (mysql_query(con, szQuery))
					{
						if (mysql_query(con, szQuery))
						{
							#ifdef _DEBUG_
							printf("> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
							#endif
							
							ZzLOG(ERROR,"> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));					
		
					       	return(-100502); 
						}
					}
					
				}            	
            }
            else if(nMode == DOWNLOAD )
            {
				if( ConnectServer(szServerID,szServerIP,nDNPort,FILE_DN_SERVER) == 0)
				{
			        sprintf( szPhoneMsg ,"%s 다운로드 서버가 죽었습니다.", pServerInfo[i].server_id);
			        ZzLOG(ERROR, szPhoneMsg);  
			        // db update
			        
			        char szQuery[1024];
					memset(szQuery,0x00,sizeof(szQuery));

					sprintf(szQuery,"UPDATE zangsi.T_SERVER_INFO set upload_yn = 'N' , admin_open_yn = 'N' where server_id = '%s'",pServerInfo[i].server_id);
	
					#ifdef __DEBUG
					printf("> Query [ %s ] \n",szQuery);
					#endif
					ZzLOG(ALWAY,"> Query [ %s ] \n",szQuery);
	
					if (mysql_query(con, szQuery))
					{
						if (mysql_query(con, szQuery))
						{
							#ifdef _DEBUG_
							printf("> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
							#endif
							
							ZzLOG(ERROR,"> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));					
				
					       	return(-100502); 
						}			
					}			
				    
				}
			}
			else
			{
				if( ConnectServer(szServerID,szServerIP,nDNPort,FILE_CASH_SERVER) == 0)
				{
			        sprintf( szPhoneMsg ,"%s 다운로드 서버가 죽었습니다.", pServerInfo[i].server_id);
			        ZzLOG(ERROR, szPhoneMsg);  
			        // db update
				}
			}	
		}
	}
	
	return 1;
		
		
}


//0 is Connect Error , -1 is socket error -1 is Socket Error , 1 over is sucessed
int ConnectServer(char* pServerID ,char* pServerIP,unsigned int nPort ,int nServerFlag)
{
	int nSocket = Connect(pServerIP,nPort);
    	
	//0 is Connect Error , -1 is connect error -2 is Socket Error , 1 over is sucessed
	
	if( nSocket <= 0 ) //dont connect
	{
		//문자 메세지 보내기
		#ifdef __DEBUG
		printf("Error : Socket Error\n");
		#endif		
		close(nSocket);
		return 0;
	}
	else
	{
	    int nResult = WaitRequest(pServerID,nSocket,nServerFlag);
	    if( nServerFlag == WEB_SERVER && (nResult == 0 || nResult == -1) )
	    {
	    	close(nSocket);
	        return nResult;
	    }
	}
	
	close(nSocket);
	
	return 1;
}
						
int RunFileProc(char* pServerID,int nSocket)
{
	char szQuery[1024];

   	HEADER head,recv_head;
	
	memset(&head,0x00,sizeof(HEADER));
	memset(&recv_head,0x00,sizeof(HEADER));

	head.nCmd = RS_CMD_REQUEST_USER_LIST;
	head.nDataCnt = 0;
	head.nDataSize = 0;
	head.nErrorCode = 0;
	
	strcpy(head.szUserID ,"서버채크");

	int nUserCnt = 0;
	long dwRecvLen =  0;

	char* pRecvData = NULL;	
		
	if(SendData( nSocket,(char*)&head,HEADER_SIZE )<0)
	{
		#ifdef __DEBUG
		printf("Error : EOL send Error\n");
		#endif
				
		return -1;
	}

	while(1)
	{
		memset(&recv_head,0x00,sizeof(HEADER));
		
		if(RecvData( nSocket,(char*)&recv_head,HEADER_SIZE) < 0)
		{
			#ifdef __DEBUG
			printf("Error : EOL recv Error\n");
			#endif
						
			return -1;
		}
		dwRecvLen = (recv_head.nDataCnt) * (recv_head.nDataSize);
		
		if( pRecvData != NULL)
		{
			delete[] pRecvData;
			pRecvData = NULL;
		}
			
		if( dwRecvLen > 0 )
		{
			pRecvData = new char[dwRecvLen];
			memset(pRecvData,0x00,sizeof(pRecvData));
			
			if(RecvData(nSocket, pRecvData, dwRecvLen )<= 0) //에러 나왔을때...
			{			
				if(pRecvData != NULL)
				{
					delete[] pRecvData;
					pRecvData = NULL;
				}
				return -1;
			}
			
		}
		#ifdef __DEBUG
		printf("> Check cmd [ %d ] \n",recv_head.nCmd );
		#endif
		ZzLOG(ALWAY,"> Check cmd [ %d ] \n",recv_head.nCmd );
		
		switch( recv_head.nCmd  )
		{
			case RS_EOL:
			{
				#ifdef __DEBUG
				printf("Sucessed : Close Socket ( %d) \n",nSocket);
				#endif  
				
				if( pRecvData != NULL)
				{
					delete[] pRecvData;
					pRecvData = NULL;
				}
			
				return 1;
				
				
			}
			case RS_CMD_REQUEST_USER_LIST :
			{
				//body recv
				
				nUserCnt = recv_head.nDataCnt - 1;
					
				memset(&head,0x00,sizeof(HEADER));
				
				head.nCmd = RS_EOL;
				head.nDataCnt = 0;
				head.nDataSize = 0;
				head.nErrorCode = 0;		
				
		
				if(SendData( nSocket,(char*)&head,HEADER_SIZE )<0)
				{
					#ifdef __DEBUG
					printf("Error : EOL send Error\n");
					#endif
					if( pRecvData != NULL)
					{
						delete[] pRecvData;
						pRecvData = NULL;
					}
					return -1;
				}
						
				// DB 처리

				memset(szQuery,0x00,sizeof(szQuery));
				sprintf(szQuery,"UPDATE zangsi.T_SERVER_INFO set dn_user = %d where server_id = '%s'",nUserCnt,pServerID);

				#ifdef __DEBUG
				printf("> Query [ %s ] \n",szQuery);
				#endif
				ZzLOG(ALWAY,"> Query [ %s ] \n",szQuery);

				if (mysql_query(con, szQuery))
				{
					#ifdef _DEBUG_
					printf("> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
					#endif
					
					ZzLOG(ERROR,"> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));					
					if( pRecvData != NULL)
					{
						delete[] pRecvData;
						pRecvData = NULL;
					}
			       	return(-100502); 
				}
				break;
			}
			default :
			{
				
			
				#ifdef __DEBUG
				printf("Failed : Close Socket ( %d) \n",nSocket);
				#endif  
				
				if( pRecvData != NULL)
				{
					delete[] pRecvData;
					pRecvData = NULL;
				}
				return -1;
			}
		}
	}
	
	if( pRecvData != NULL)
	{
		delete[] pRecvData;
		pRecvData = NULL;
	}
	
	return 1;
}	
				
int RunWebProc(int nSocket)
{
    char szHeader[100];
    memset(szHeader,0x00,sizeof(szHeader));
    
    char szRecvBuffer[1024];
    memset(szRecvBuffer,0x00,sizeof(szRecvBuffer));
    
    sprintf(szHeader,"GET /util/isalive.jsp\r\n");
   
   	if(SendData( nSocket,(char*)&szHeader,strlen(szHeader) )<0)
	{
		#ifdef __DEBUG
		printf("Error : EOL send Error\n");
		#endif
				
		close(nSocket);
		return -1;
	}
	
	
	
	if(RecvData( nSocket,(char*)&szRecvBuffer,sizeof(szRecvBuffer)) < 0)
	{
		#ifdef __DEBUG
		printf("Error : EOL recv Error\n");
		#endif
				
		close(nSocket);		
		return -1;
	}
	
	#ifdef __DEBUG
	printf("RecvData :%s (%d)\n",szRecvBuffer,strlen(szRecvBuffer));
	#endif


	if( strcmp( szRecvBuffer ,"OK") != 0 && strcmp( szRecvBuffer ,"\nOK") !=0 
	    &&strcmp( szRecvBuffer ,"\r\nOK") !=0 && strcmp( szRecvBuffer ,"OK") !=0 
	    )
	{
		#ifdef __DEBUG
		printf("Error : Server Is Not OK\n");
		#endif
			    
	    close(nSocket); 
	    return 1;
	}
	
	
	#ifdef __DEBUG
	printf("Sucessed : Close Socket ( %d) \n",nSocket);
	#endif  
	

	close(nSocket); 
	return 1;
}	

int RunUpFileProc(char* pServerID,int nSocket)
{
	char szQuery[1024];

   	HEADER head,recv_head;
	
	memset(&head,0x00,sizeof(HEADER));
	memset(&recv_head,0x00,sizeof(HEADER));

	head.nCmd = RS_CMD_REQUEST_USER_LIST;
	head.nDataCnt = 0;
	head.nDataSize = 0;
	head.nErrorCode = 0;
	
	strcpy(head.szUserID ,"서버채크");

	int nUserCnt = 0;
	long dwRecvLen =  0;

	char* pRecvData = NULL;	
		
	if(SendData( nSocket,(char*)&head,HEADER_SIZE )<0)
	{
		#ifdef __DEBUG
		printf("Error : EOL send Error\n");
		#endif
				
		return -1;
	}

	while(1)
	{
		memset(&recv_head,0x00,sizeof(HEADER));
		
		if(RecvData( nSocket,(char*)&recv_head,HEADER_SIZE) < 0)
		{
			#ifdef __DEBUG
			printf("Error : EOL recv Error\n");
			#endif
						
			return -1;
		}
		dwRecvLen = (recv_head.nDataCnt) * (recv_head.nDataSize);
		
		if( pRecvData != NULL)
		{
			delete[] pRecvData;
			pRecvData = NULL;
		}
			
		if( dwRecvLen > 0 )
		{
			pRecvData = new char[dwRecvLen];
			memset(pRecvData,0x00,sizeof(pRecvData));
			
			if(RecvData(nSocket, pRecvData, dwRecvLen )<= 0) //에러 나왔을때...
			{			
				if(pRecvData != NULL)
				{
					delete[] pRecvData;
					pRecvData = NULL;
				}
				return -1;
			}
			
		}
		#ifdef __DEBUG
		printf("> Check cmd [ %d ] \n",recv_head.nCmd );
		#endif
		ZzLOG(ALWAY,"> Check cmd [ %d ] \n",recv_head.nCmd );
		
		switch( recv_head.nCmd  )
		{
			case RS_EOL:
			{
				#ifdef __DEBUG
				printf("Sucessed : Close Socket ( %d) \n",nSocket);
				#endif  
				
				if( pRecvData != NULL)
				{
					delete[] pRecvData;
					pRecvData = NULL;
				}
			
				return 1;
				
				
			}
			case RS_CMD_REQUEST_USER_LIST :
			{
				//body recv
				
				nUserCnt = recv_head.nDataCnt - 1;
					
				memset(&head,0x00,sizeof(HEADER));
				
				head.nCmd = RS_EOL;
				head.nDataCnt = 0;
				head.nDataSize = 0;
				head.nErrorCode = 0;		
				
		
				if(SendData( nSocket,(char*)&head,HEADER_SIZE )<0)
				{
					#ifdef __DEBUG
					printf("Error : EOL send Error\n");
					#endif
					if( pRecvData != NULL)
					{
						delete[] pRecvData;
						pRecvData = NULL;
					}
					return -1;
				}
						
				// DB 처리

				memset(szQuery,0x00,sizeof(szQuery));
				if(nUserCnt == 0)
					sprintf(szQuery,"UPDATE zangsi.T_SERVER_INFO set up_user = %d, up_size = 0 where server_id = '%s'",nUserCnt,pServerID);					
				else
					sprintf(szQuery,"UPDATE zangsi.T_SERVER_INFO set up_user = %d where server_id = '%s'",nUserCnt,pServerID);

				#ifdef __DEBUG
				printf("> Query [ %s ] \n",szQuery);
				#endif
				ZzLOG(ALWAY,"> Query [ %s ] \n",szQuery);

				if (mysql_query(con, szQuery))
				{
					#ifdef _DEBUG_
					printf("> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
					#endif
					
					ZzLOG(ERROR,"> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));					
					if( pRecvData != NULL)
					{
						delete[] pRecvData;
						pRecvData = NULL;
					}
			       	return(-100502); 
				}
				break;
			}
			default :
			{
				
			
				#ifdef __DEBUG
				printf("Failed : Close Socket ( %d) \n",nSocket);
				#endif  
				
				if( pRecvData != NULL)
				{
					delete[] pRecvData;
					pRecvData = NULL;
				}
				return -1;
			}
		}
	}
	
	if( pRecvData != NULL)
	{
		delete[] pRecvData;
		pRecvData = NULL;
	}
	
	return 1;
}	

int RunCashFileProc(char* pServerID,int nSocket)
{
	char szQuery[1024];

   	HEADER head,recv_head;
	
	memset(&head,0x00,sizeof(HEADER));
	memset(&recv_head,0x00,sizeof(HEADER));

	head.nCmd = RS_CMD_REQUEST_USER_LIST;
	head.nDataCnt = 0;
	head.nDataSize = 0;
	head.nErrorCode = 0;
	
	strcpy(head.szUserID ,"캐시 서버 체크");

	int nUserCnt = 0;
	long dwRecvLen =  0;

	char* pRecvData = NULL;	
		
	if(SendData( nSocket,(char*)&head,HEADER_SIZE )<0)
	{
		#ifdef __DEBUG
		printf("Error : EOL send Error\n");
		#endif
				
		return -1;
	}

	while(1)
	{
		memset(&recv_head,0x00,sizeof(HEADER));
		
		if(RecvData( nSocket,(char*)&recv_head,HEADER_SIZE) < 0)
		{
			#ifdef __DEBUG
			printf("Error : EOL recv Error\n");
			#endif
						
			return -1;
		}
		dwRecvLen = (recv_head.nDataCnt) * (recv_head.nDataSize);
		
		if( pRecvData != NULL)
		{
			delete[] pRecvData;
			pRecvData = NULL;
		}
			
		if( dwRecvLen > 0 )
		{
			pRecvData = new char[dwRecvLen];
			memset(pRecvData,0x00,sizeof(pRecvData));
			
			if(RecvData(nSocket, pRecvData, dwRecvLen )<= 0) //에러 나왔을때...
			{			
				if(pRecvData != NULL)
				{
					delete[] pRecvData;
					pRecvData = NULL;
				}
				return -1;
			}
			
		}
		#ifdef __DEBUG
		printf("> Check cmd [ %d ] \n",recv_head.nCmd );
		#endif
		ZzLOG(ALWAY,"> Check cmd [ %d ] \n",recv_head.nCmd );
		
		switch( recv_head.nCmd  )
		{
			case RS_EOL:
			{
				#ifdef __DEBUG
				printf("Sucessed : Close Socket ( %d) \n",nSocket);
				#endif  
				
				if( pRecvData != NULL)
				{
					delete[] pRecvData;
					pRecvData = NULL;
				}
			
				return 1;
				
				
			}
			case RS_CMD_REQUEST_USER_LIST :
			{
				//body recv
				
				nUserCnt = recv_head.nDataCnt - 1;
					
				memset(&head,0x00,sizeof(HEADER));
				
				head.nCmd = RS_EOL;
				head.nDataCnt = 0;
				head.nDataSize = 0;
				head.nErrorCode = 0;		
				
		
				if(SendData( nSocket,(char*)&head,HEADER_SIZE )<0)
				{
					#ifdef __DEBUG
					printf("Error : EOL send Error\n");
					#endif
					if( pRecvData != NULL)
					{
						delete[] pRecvData;
						pRecvData = NULL;
					}
					return -1;
				}
						
				// DB 처리
				memset(szQuery,0x00,sizeof(szQuery));
				
				if(strcmp(g_szServerID, pServerID) != 0)
				{
					strcpy(g_szServerID, pServerID);
					sprintf(szQuery,"UPDATE zangsi.T_SERVER_INFO set dn_user = %d where server_id = '%s'",nUserCnt,pServerID);
				}
				else
				{
					sprintf(szQuery,"UPDATE zangsi.T_SERVER_INFO set dn_user = dn_user + %d where server_id = '%s'",nUserCnt,pServerID);
				}	

				#ifdef __DEBUG
				printf("> Query [ %s ] \n",szQuery);
				#endif
				ZzLOG(ALWAY,"> Query [ %s ] \n",szQuery);

				if (mysql_query(con, szQuery))
				{
					#ifdef _DEBUG_
					printf("> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
					#endif
					
					ZzLOG(ERROR,"> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));					
					if( pRecvData != NULL)
					{
						delete[] pRecvData;
						pRecvData = NULL;
					}
			       	return(-100502); 
				}
				break;
			}
			default :
			{
				
			
				#ifdef __DEBUG
				printf("Failed : Close Socket ( %d) \n",nSocket);
				#endif  
				
				if( pRecvData != NULL)
				{
					delete[] pRecvData;
					pRecvData = NULL;
				}
				return -1;
			}
		}
	}
	
	if( pRecvData != NULL)
	{
		delete[] pRecvData;
		pRecvData = NULL;
	}
	
	return 1;
}	
						

int WaitRequest(char* pServerID,int nSocket,int nServerFlag)
{
    if( nSocket < 0 )
		return -2;
		
    switch(nServerFlag)
    {
    	case FILE_UP_SERVER:
    	{
    		if(bIsCheck)
    			return -1;
    		else
    			return RunUpFileProc(pServerID,nSocket);
    	}
        case FILE_DN_SERVER:
        {
        	if(bIsCheck)
        		return -1;
        	else
        		return RunFileProc(pServerID,nSocket);
        }
        case FILE_CASH_SERVER:
        {
        	if(bIsCheck)
        		return -1;
        	else
        		return RunCashFileProc(pServerID,nSocket);
        }
        case CMD_SERVER:
        {
            return RunFileProc(pServerID,nSocket);
        }    	
        break;
        case WEB_SERVER:
        {
            return RunWebProc(nSocket);
        }
        break;
        default :
            return -2;
    }

	
	return 1;
		
}


int upload_checker_run()
{
	ZzLOG(ALWAY,"> 프로그램 시작\n");
	
	
	// DB 연결
	if (!(con=db_connect("zangsi")))
	{
		
		#ifdef _DEBUG_
		printf("> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
		#endif
		ZzLOG(ERROR,"> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
	   	return(-1); 

	}
	
		
	SERVER_INFO s_info[MAX_ROWS];
	memset(s_info,0x00,sizeof(s_info));
	
	char szErrMsg[256];
	memset(szErrMsg,0x00,sizeof(szErrMsg));
	
	int nServerCount = 0;
	// 서버 정보 조회 
	if( GetServerInfo(s_info, &nServerCount , szErrMsg, UPLOAD) < 0)
	{
		#ifdef __DEBUG
		printf("%s\n",szErrMsg);
		#endif
		
		ZzLOG(ERROR,"> %s\n",szErrMsg);
		
		if(con != NULL)
			db_disconnect(con);
				
		return -1;
	}
	// 서버 채크

	CheckServer(s_info,nServerCount,UPLOAD);
	
	if(con != NULL)
		db_disconnect(con);
	
	ZzLOG(ALWAY,"> 프로그램 종료\n");	
	
}


int run()
{
	ZzLOG(ALWAY,"> 프로그램 시작\n");
	
	
	// DB 연결
	if (!(con=db_connect("zangsi")))
	{
		
		#ifdef _DEBUG_
		printf("> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
		#endif
		ZzLOG(ERROR,"> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
	   	return(-1); 

	}
	
		
	SERVER_INFO s_info[MAX_ROWS];
	memset(s_info,0x00,sizeof(s_info));
	
	char szErrMsg[256];
	memset(szErrMsg,0x00,sizeof(szErrMsg));
	
	int nServerCount = 0;
	// 서버 정보 조회 
	if( GetServerInfo(s_info, &nServerCount , szErrMsg, DOWNLOAD) < 0)
	{
		#ifdef __DEBUG
		printf("%s\n",szErrMsg);
		#endif
		
		ZzLOG(ERROR,"> %s\n",szErrMsg);
		
		if(con != NULL)
			db_disconnect(con);
				
		return -1;
	}
	// 서버 채크

	CheckServer(s_info,nServerCount,DOWNLOAD);
	
	if(con != NULL)
		db_disconnect(con);
	
	ZzLOG(ALWAY,"> 프로그램 종료\n");	
	
}

int cash_checker()
{
	ZzLOG(ALWAY,"> 프로그램 시작\n");
	
	
	// DB 연결
	if (!(con=db_connect("zangsi")))
	{
		
		#ifdef _DEBUG_
		printf("> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
		#endif
		ZzLOG(ERROR,"> DB ERR: [%d](%s)",mysql_errno(con), mysql_error(con));
	   	return(-1); 

	}
	
		
	SERVER_INFO s_info[MAX_ROWS];
	memset(s_info,0x00,sizeof(s_info));
	
	char szErrMsg[256];
	memset(szErrMsg,0x00,sizeof(szErrMsg));
	
	int nServerCount = 0;
	// 서버 정보 조회 
	if( GetCashServerInfo(s_info, &nServerCount , szErrMsg) < 0)
	{
		#ifdef __DEBUG
		printf("%s\n",szErrMsg);
		#endif
		
		ZzLOG(ERROR,"> %s\n",szErrMsg);
		
		if(con != NULL)
			db_disconnect(con);
				
		return -1;
	}
	// 서버 채크

	CheckServer(s_info,nServerCount,CASH);
	
	if(con != NULL)
		db_disconnect(con);
	
	ZzLOG(ALWAY,"> 프로그램 종료\n");	
	
}

int SetTimer(long second)
{
	struct itimerval value;
    value.it_interval.tv_sec = second;
    value.it_interval.tv_usec = 0;

    value.it_value = value.it_interval;


	if(signal(SIGALRM, signal_caller) == SIG_ERR)  
	{
		
		printf("ERROR : time signal setup Error\n");
		
		return -1;
	}
	
	setitimer(ITIMER_REAL, &value, NULL);
	
	return 1;
}




/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  signal_exception(int nSignal)
{
    #ifdef __DEBUG
    printf("signal exception %d",nSignal);
    #endif
}

int process_init(int argc, char **argv)
{	
    ZzInitGlobalVariable2("chk_daemon", "/logs/daemon"); 
 
	#ifdef __DEBUG
	printf("start Checking Server..\n");
	#endif
	
	return 1;
	

}

//******************************************************************************
//* chkdaemon main
//******************************************************************************
int main(int argc,char** argv)
{
	
	
	signal(SIGTERM, signal_exception);
	signal(SIGINT,  signal_exception);
	signal(SIGQUIT, signal_exception);
	signal(SIGKILL, signal_exception);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
	
	memset(g_szServerID, 0x00, sizeof(g_szServerID));
      
  	if ( process_init(argc, argv) == 1 ) 
  	{
  		
  		if( argc == 2 && strcmp(argv[1],"up") == 0 )
  		{
  			bIsCheck = false;
  			upload_checker(); 			
  		}
  		else if( argc == 2 && strcmp(argv[1],"dn") == 0 )
  		{
  			bIsCheck = false;
			signal_caller(1);	
		}
		else if( argc == 2 && strcmp(argv[1],"cash") == 0 )
		{
			bIsCheck = false;
			cash_checker();
		}
  		else if( argc == 3 && strcmp(argv[1],"up") == 0 && strcmp(argv[2],"check") == 0)
  		{
  			bIsCheck = true;
  			upload_checker(); 			
  		}
  		else if( argc == 3 && strcmp(argv[1],"dn") == 0 && strcmp(argv[2],"check") == 0)
  		{
  			bIsCheck = true;
			signal_caller(1);	
		}
		else
		{
			printf("==========================사용법==========================\n");
			printf("\n각 컨텐츠 서버의\n접속자수를 업데이트하고자 할 때 : ./chkdaemon up OR dn \n\n");
			printf("\n각 컨텐츠 서버의\n프로세스 동작여부를 확인하고자 할 때 : ./chkdaemon up check OR dn check\n\n");
			printf("**로그 위치 : /logs/daemon/.\n");
			printf("==========================================================\n");
			
			
		}
		
		

	} 
	
	/*	
	bCloseMain = true;
	SetTimer(atoi(argv[1])); //60초
	while(bCloseMain)
	{
		sleep(60);
	}
    */
    
    return -1;
}
