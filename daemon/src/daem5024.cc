/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daem5024.cc
 *         기능 : 컨텐츠 삭제
 *         설명 : 판매자격이 취소 된 화원중 처리일자가 일주일이 지난회원의 컨텐츠 삭제
				 
 *     설치위치 : 관리자 DB
 *
 *       작성자 : HCS
 *       작성일 : 2009/07/30
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
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h> // for usleep()

#include "daemcom.h"
#include "commydb.h"

#define  MAX_ROWS	1
#define _DEBUG_


int daem5024_init_process(int argc, char **argv);
int daem5024_main_process();
int daem5024_term_process();
void daem5024_signal(int nSignal);
int DeleteContents(char* pUserId);

MYSQL     *con_bck; // back
MYSQL_RES *res_bck;
MYSQL_ROW  row_bck;

MYSQL     *con; // main
MYSQL_RES *res;
MYSQL_ROW  row;


MYSQL     *con_sub; // sub 1

char gproc_date [  8+1];	// 처리일자
char   gsys_date  [  8+1];	//등록일
char   gsys_time  [  6+1];	//등록시간
char   gdel_date  [  8+1];	//삭제예정일 (처리일자+3)


bool IsConnectDB(MYSQL* pCon)
{
	if(!pCon)
	{
		return false;
	}
	char szQuery[10];
	memset(szQuery, 0x00, sizeof(szQuery));
	strcpy(szQuery, "select 1");

	if(mysql_query(pCon, szQuery))
	{
		return false;
	}
	if(!(res = mysql_store_result(pCon)))
	{
		return false;
	}
	mysql_free_result(res);

	return true;

}


int DeleteQuery(char *szQuery)
{
	ZzLOG(ALWAY, "Query [ %s ]\n",szQuery);

	if (mysql_query(con, szQuery)){
		ZzLOG(ERROR, "Delete Query [ %s ]\n",szQuery);
		return -1;
	}

	return 0;
}

int UpdateQuery(char *szQuery)
{
	ZzLOG(ALWAY, "Query [ %s ]\n",szQuery);

	if (mysql_query(con, szQuery)){
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
		return -1;
	}

	return 0;
}



//******************************************************************************
//* daem5024 main
//******************************************************************************
int daem5024_main_process()
{

	if(IsConnectDB(con_bck) == false)
	{
		if (!(con_bck=db_connect_backup("zangsi")))
		{
			db_disconnect(con_bck);
			ZzLOG(ERROR, "con_bck DB에 접속하지 못 하였습니다...\n");
			return(-1);
		}
	}

	char szQuery[1000];  // query string

    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "daem5024_main_process: gproc_date[%s]\n", gproc_date);
    ZzLOG(ALWAY, "---------------------------------------------------------\n");

	// perm_gu : 1-접수, 2-승인(준), 3-취소, 4-보류, 0-영구취소, 5-승인(정)

	memset(szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, " select a.user_id "
					 " from zangsi.T_PERM_UPLOAD_AUTH a, zangsi.T_CONTENTS_INFO b "
					 " where a.perm_gu in('0', '3') and a.proc_date <= '%s' and a.user_id = b.reg_user and b.del_yn = 'N' "
					 " group by a.user_id ; /* daem5024 */ "
					 , gproc_date);

	#ifdef __DEBUG
	printf("Main Query = [%s]\n", szQuery);
	#endif
	ZzLOG(ALWAY, "daem5024_main_process: [ %s ] \n", szQuery);
	if(mysql_query(con_bck, szQuery))
	{
		ZzLOG(ERROR, "daem5024_main_process: mysql_query error [%s]\n", szQuery);
		ZzLOG(ERROR, "daem5024_main_process: [%d](%s)\n", mysql_errno(con_bck), mysql_error(con_bck));
		return -1;
	}
	if(!(res_bck = mysql_store_result(con_bck)))
	{
		ZzLOG(ERROR, "daem5024_main_process: mysql_store_result error [%s]\n", szQuery);
		ZzLOG(ERROR, "daem5024_main_process: [%d](%s)\n", mysql_errno(con_bck), mysql_error(con_bck));
		return -1;
	}
	if(mysql_num_rows(res_bck) == 0)
	{
		ZzLOG(ALWAY, "daem5024_main_process: 처리할 자료 없음. [%s]\n", szQuery);
		return 0;
	}

	int nCount = 0;
	while(row_bck = mysql_fetch_row(res_bck))
	{
		char szUserId[12+1];
		memset(szUserId, 0x00, sizeof(szUserId));

		strcpy(szUserId, getstr(row_bck,0));

		//컨텐츠 삭제 처리
		#ifdef __DEBUG
		printf("----------DeleteContents(%s)----------\n", szUserId);
		#endif

		ZzLOG(ALWAY, "daem5024_main_process: %s 처리시작\n", szUserId);

		if(DeleteContents(szUserId) < 0)
		{
			mysql_free_result(res_bck);
			return -1;
		}

		ZzLOG(ALWAY, "daem5024_main_process: %s 처리완료\n", szUserId);

		nCount++;
		#ifdef __DEBUG
		printf("----------------end(%d)----------------\n", nCount);
		#endif
	}
	mysql_free_result(res_bck);

    ZzLOG(ALWAY, "daem5024_main_process: %d명 처리완료\n", nCount);
	return  0;
}


// 업로드 자격 취소자의 경우 해당 컨텐츠를 자동 연장 하지 않는다.
// 게시물을 모두 내림. 유일 해쉬 포함.
int DeleteContents(char* pUserId)
{

	if(IsConnectDB(con) == false)
	{
		if (!(con=db_connect_local("zangsi")))
		{
			db_disconnect(con);
			ZzLOG(ERROR, "con_bck DB에 접속하지 못 하였습니다...\n");
			return(-1);
		}
	}


	MYSQL_RES *delete_res=NULL;
	MYSQL_ROW  delete_row=NULL;

	char szQuery[1000];
	memset(szQuery, 0x00, sizeof(szQuery));

	bool bFail = false;

	int nCount = 0;
	int nFlogCount = 0;

	//컨텐츠 삭제처리
	while(1)
	{
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " select id ,memo_cnt from zangsi.T_CONTENTS_INFO where reg_user = '%s' and del_yn = 'N' ; /* daem5024 */  ", pUserId);

		if(mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "DeleteContents: mysql_query error [%s]\n", szQuery);
			ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
			return -1;
		}
		if(!(delete_res = mysql_store_result(con)))
		{
			mysql_free_result(delete_res);
			ZzLOG(ERROR, "DeleteContents: mysql_store_result error [%s]\n", szQuery);
			ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
			return -1;
		}
		if(mysql_num_rows(delete_res) == 0)
		{
			mysql_free_result(delete_res);

			if(nCount == 0)
			{
				ZzLOG(ALWAY, "DeleteContents: %s는 보유 컨텐츠가 없습니다. [%s]\n", pUserId, szQuery);
				return 0;
			}
			else
			{
				ZzLOG(ALWAY, "DeleteContents: 일반 컨텐츠 - %s %d건 처리완료\n", pUserId, nCount);
				break;
			}
		}


		while(delete_row = mysql_fetch_row(delete_res))
		{

			if(tran_begin(con)!=0)
			{
				mysql_free_result(delete_res);
			    ZzLOG(ERROR, "DeleteContents: tran_begin: 테이베이스 오류입니다.\n");
				ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
				return -1;
			}

			unsigned long ulID = (unsigned long)getnum(delete_row,0);
			unsigned long memo_cnt = (unsigned long)getnum(delete_row,1);


			//--------------------------------------------------------------------------
			// T_CONTENTS_ADMDEL insert
			//
			//--------------------------------------------------------------------------
			bool bBest_con=false;
			memset (szQuery, 0x00, sizeof(szQuery));
			//성지 순례 컨텐츠인지 확인
			{
				//성지순례 중인 것 중 삭제 되는컨텐츠

				memset (szQuery, 0x00, sizeof(szQuery));
				sprintf(szQuery, " SELECT a.id ,a.reg_user ,b.best_gu "
						         "  FROM zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_BEST b  "
						         " WHERE a.id = %lu "
						         "   AND a.id = b.id and b.best_gu in('HK' ,'HS','HD') ; /* daem5024 */  "
						         ,ulID
						         );

				 #ifdef __DEBUG
				 printf("%s\n\n",szQuery);
				 #endif
				ZzLOG(ALWAY,"Best에 등록된 컨텐츠 처리 [ %s ]\n",szQuery);
				if (mysql_query(con, szQuery)){
					ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
				    ZzLOG(ERROR, "daem5002_del_end_date: INSERT T_CONTENTS_ADMDEL error...\n");
				    bFail = true;
						break;
					//if (mysql_errno(con) != 1062)
			    }

				MYSQL_RES*	best_res = mysql_store_result(con);
				MYSQL_ROW	best_row = NULL;
				unsigned long cont_id=0;
				char szRegUser[24];
				memset(szRegUser,0x00,sizeof(szRegUser));
				char best_gu[10];
				memset(best_gu,0x00,sizeof(best_gu));

				if( (best_row = mysql_fetch_row(best_res)) != NULL )
				{
					bBest_con = true;
					cont_id = (unsigned long)getnum(best_row,0);
					strcpy(szRegUser ,getstr(best_row,1));
					strcpy(best_gu ,getstr(best_row,2));

					memset (szQuery, 0x00, sizeof(szQuery));

					//----------------------------------------------------------------------
					// 검색엔진의 색인정보 삭제
					//----------------------------------------------------------------------
					memset (szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_CREATE "
					                "        (id, cont_gu, udt_cd) "
					                 "SELECT id     , '01' , 'D'         "
					                 "  FROM zangsi.T_CONTENTS_INFO   "
					                 " WHERE id = %lu ; /* daem5024 */ "
					                , cont_id);
					if (mysql_query(con, szQuery)){
					    ZzLOG(ERROR, "daem5002_delete_data: INSERT T_CONTENTS_CREATE error...\n");
						ZzLOG(ERROR, "daem5002_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
						bFail = true;
						break;
				    }

					if( strcmp(best_gu,"HK") == 0 )
					{
						memset (szQuery, 0x00, sizeof(szQuery));
						sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_ADMDEL"
						                 "     ( cont_gu  , id                "
						                 "     , del_desc , reg_user          "
						                 "     , reg_date , reg_time		 "
						                 "     , proc_yn )         "//20130523 - 성지순례 정보만 남기기
						                 "SELECT '01'     , id                "
						                 "     , '게시기간 만료' , 'sys5024_1'    "
						                 "     , '%s'     ,'%s'               "
						                 "     , 'T' " //20130523 - 성지순례 정보만 남기기
						                 "  FROM zangsi.T_CONTENTS_INFO "
						                 " WHERE id = %lu ; /* daem5024 */ "
						                 ,gsys_date
						                 ,gsys_time
						                 ,cont_id
						                 );


						 #ifdef __DEBUG
						 printf("%s\n\n",szQuery);
						 #endif

						if (mysql_query(con, szQuery)){
							ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
						    ZzLOG(ERROR, "daem5002_del_end_date: INSERT T_CONTENTS_ADMDEL error...\n");
						    if( mysql_errno(con) != 1062 )
							{
								bFail = true;
								break;
							}
					    }


						memset (szQuery, 0x00, sizeof(szQuery));
						//20100824 - no.575
						sprintf(szQuery, " update zangsi.T_CONTENTS_VIR_ID2  "
						                 " set del_yn = 'Y' , copyright_yn= 'T' "
						                 " where id = %lu ; /* daem5024 */ "
						                 ,cont_id  );
						 #ifdef __DEBUG
						 printf("%s\n\n",szQuery);
						 #endif

						if (mysql_query(con, szQuery)){
							ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
						    ZzLOG(ERROR, "daem5002_del_end_date: UPDATE T_CONTENTS_INFO error...\n");
						    bFail = true;
						break;
					    }

						memset (szQuery, 0x00, sizeof(szQuery));
						//20100824 - no.575
						sprintf(szQuery, " update zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_VIR_ID b "
						                 " set a.disp_stat = 'N', a.del_yn = 'Y', b.del_yn = 'Y' "
						                 " , b.copyright_yn= 'T'  "
						                 " where a.id = b.id and a.id = %lu ; /* daem5024 */ "
						                 ,cont_id
						                 );
						 #ifdef __DEBUG
						 printf("%s\n\n",szQuery);
						 #endif

						if (mysql_query(con, szQuery)){
							ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
						    ZzLOG(ERROR, "daem5002_del_end_date: UPDATE T_CONTENTS_INFO error...\n");
						    bFail = true;
						break;
					    }

					}
					else
					{
						memset (szQuery, 0x00, sizeof(szQuery));
						sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_ADMDEL"
						                 "     ( cont_gu  , id                "
						                 "     , del_desc , reg_user          "
						                 "     , reg_date , reg_time		 "
						                 "     , proc_yn )         "//20130523 - 성지순례 정보만 남기기
						                 "SELECT '01'     , id                "
						                 "     , '게시기간 만료' , 'sys5024_2'    "
						                 "     , '%s'     ,'%s'               "
						                 "     , 'S' " //20130523 - 성지순례 정보만 남기기
						                 "  FROM zangsi.T_CONTENTS_INFO "
						                " WHERE id = %lu ; /* daem5024 */ "
						                 ,gsys_date
						                 ,gsys_time
						                 ,cont_id
						                 );

						memset (szQuery, 0x00, sizeof(szQuery));
						//20100824 - no.575
						sprintf(szQuery, " update zangsi.T_CONTENTS_VIR_ID2  "
						                 " set del_yn = 'Y' , copyright_yn= 'S' "
						                 " where id = %lu ; /* daem5024 */ "
						                 ,cont_id  );
						 #ifdef __DEBUG
						 printf("%s\n\n",szQuery);
						 #endif

						if (mysql_query(con, szQuery)){
							ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
						    ZzLOG(ERROR, "daem5002_del_end_date: UPDATE T_CONTENTS_INFO error...\n");
						    bFail = true;
						break;
					    }

						memset (szQuery, 0x00, sizeof(szQuery));
						//20100824 - no.575
						sprintf(szQuery, " update zangsi.T_CONTENTS_INFO a, zangsi.T_CONTENTS_VIR_ID b "
						                 " set a.disp_stat = 'N', a.del_yn = 'Y', b.del_yn = 'Y' "
						                 " , b.copyright_yn= 'S'  "
						                 " where a.id = b.id and a.id = %lu ; /* daem5024 */ "
						                 ,cont_id
						                 );
						 #ifdef __DEBUG
						 printf("%s\n\n",szQuery);
						 #endif

						if (mysql_query(con, szQuery)){
							ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
						    ZzLOG(ERROR, "daem5002_del_end_date: UPDATE T_CONTENTS_INFO error...\n");
						    bFail = true;
						break;
					    }

					}



					cont_id = 0;
					memset(szRegUser,0x00,sizeof(szRegUser));
					memset(best_gu,0x00,sizeof(best_gu));
				}

				mysql_free_result(best_res);
			}
			//메모가 50개 이상인지

			memset (szQuery, 0x00, sizeof(szQuery));
			//일반
			if( bBest_con == false)
			{
				if( memo_cnt >= 50 )
				{

					sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_ADMDEL"
					                 " ( cont_gu, id, del_desc       , reg_user, reg_date , reg_time ,proc_yn) "
					                 "VALUES "
					                 " ( '01'   ,%ld, '판매자격 취소', 'sys5024_3', '%s'     ,'%s'     ,'T' ) ; /* daem5024 */ "
					                 , ulID
					                 ,gsys_date
					                 ,gsys_time);
				}
				else
				{
					sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_ADMDEL"
					                 " ( cont_gu, id, del_desc       , reg_user, reg_date , reg_time) "
					                 "VALUES "
					                 " ( '01'   ,%ld, '판매자격 취소', 'sys5024_4', '%s'     ,'%s'     ) ; /* daem5024 */ "
					                 , ulID
					                 ,gsys_date
					                 ,gsys_time);
				}

				if(mysql_query(con, szQuery))
				{
					ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
				    ZzLOG(ERROR, "DeleteContents: INSERT T_CONTENTS_ADMDEL error...\n");
					if( mysql_errno(con) != 1062 )
							{
								bFail = true;
								break;
							}
			    }
				 #ifdef __DEBUG
				 printf("\n>T_CONTENTS_ADMDEL : %s\n\n",szQuery);
				 #endif
				if( memo_cnt < 50 )
				{
					//----------------------------------------------------------------------
					// 검색엔진의 색인정보 삭제
					//----------------------------------------------------------------------
					memset (szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, " INSERT INTO zangsi.T_CONTENTS_CREATE "
					                 " (id , cont_gu, udt_cd) "
					                 " VALUES "
					                 " (%ld, '01'   , 'D'   ) ; /* daem5024 */ "
					                 , ulID);

					if(mysql_query(con, szQuery))
					{
						ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
					    ZzLOG(ERROR, "DeleteContents: INSERT T_CONTENTS_CREATE error...\n");
					    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
						bFail = true;
						break;
				    }
					 #ifdef __DEBUG
					 printf("\n>T_CONTENTS_CREATE : %s\n\n",szQuery);
					 #endif

					//----------------------------------------------------------------------
					// 이미지 삭제 정보 추가
					//----------------------------------------------------------------------
					memset (szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, " INSERT INTO zangsi.T_IMG_DEL "
					                 " (cont_gu, seq_no, filog_cn, file_path1, file_path2, file_name, reg_user, reg_date, reg_time)"
					                 " SELECT "
					                 "  'WE', img_no, 0, '/zangsi/project/zangsi/files_zangsi/contents/img',"
					                 "  img_spath, img_sname, reg_user, '%s', '%s'"
					                 " FROM zangsi.T_CONTENTS_IMG "
					                 " WHERE cont_gu = 'WE' and id = %ld ; /* daem5024 */ "
					                 , gsys_date
					                 , gsys_time
					                 , ulID);

					ZzLOG(ALWAY, " 3 Query [ %s ] \n",szQuery);

					if(mysql_query(con, szQuery))
					{
						ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
					    ZzLOG(ERROR, "DeleteContents: INSERT T_IMG_DEL error...\n");
					    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
						if( mysql_errno(con) != 1062 )
							{
								bFail = true;
								break;
							}
				    }
					 #ifdef __DEBUG
					 printf("\n>T_IMG_DEL : %s\n\n",szQuery);
					 #endif

					//----------------------------------------------------------------------
					// 이미지 정보 삭제
					//----------------------------------------------------------------------
					memset (szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, " DELETE "
									 " FROM zangsi.T_CONTENTS_IMG "
									 " WHERE cont_gu = 'WE' and id = %ld ; /* daem5024 */ "
									 , ulID);

					if(mysql_query(con, szQuery))
					{
						ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
					    ZzLOG(ERROR, "DeleteContents: DELETE T_CONTENTS_IMG error...\n");
					    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
						bFail = true;
						break;
				    }
					 #ifdef __DEBUG
					 printf("\n>T_CONTENTS_IMG : %s\n\n",szQuery);
					 #endif

					 memset (szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, "UPDATE zangsi.T_CONTENTS_VIR_ID"
					                 "   SET del_yn        = 'Y'   "
					                 " WHERE id = %ld  ; /* daem5024 */ "
					                 , ulID);

					if (mysql_query(con, szQuery))
					{
						ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
					    ZzLOG(ERROR, "DeleteContents: UPDATE T_CONTENTS_INFO error...\n");
					    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
						bFail = true;
						break;
				    }
					 #ifdef __DEBUG
					 printf("\n>T_CONTENTS_INFO : %s\n\n",szQuery);
					 #endif

					memset (szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, "UPDATE zangsi.T_CONTENTS_VIR_ID2"
					                 "   SET del_yn        = 'Y'   "
					                 " WHERE id = %ld  ; /* daem5024 */ "
					                 , ulID);

					if (mysql_query(con, szQuery))
					{
						ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
					    ZzLOG(ERROR, "DeleteContents: UPDATE T_CONTENTS_INFO error...\n");
					    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
						bFail = true;
						break;
				    }
				 }
				 else
			  	 {
			  	 	memset (szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, "INSERT INTO  zangsi.T_CONTENTS_BEST ( best_gu , id ,reg_date,reg_time ) "
					                 "   values  ( 'HK',%lu,'%s','%s' ) ; /* daem5024 */ "
					                 , ulID
					                 ,gsys_date
					                 ,gsys_time
					                 );

					if (mysql_query(con, szQuery))
					{
						ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
					    ZzLOG(ERROR, "DeleteContents: UPDATE T_CONTENTS_INFO error...\n");
					    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
						if( mysql_errno(con) != 1062 )
							{
								bFail = true;
								break;
							}
				    }

			 		memset (szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, "UPDATE zangsi.T_CONTENTS_VIR_ID"
					                 "   SET del_yn        = 'Y'  ,copyright_yn='T'  "
					                 " WHERE id = %ld  ; /* daem5024 */  "
					                 , ulID);

					if (mysql_query(con, szQuery))
					{
						ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
					    ZzLOG(ERROR, "DeleteContents: UPDATE T_CONTENTS_INFO error...\n");
					    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
						bFail = true;
						break;
				    }
					 #ifdef __DEBUG
					 printf("\n>T_CONTENTS_INFO : %s\n\n",szQuery);
					 #endif

					memset (szQuery, 0x00, sizeof(szQuery));
					sprintf(szQuery, "UPDATE zangsi.T_CONTENTS_VIR_ID2"
					                 "   SET del_yn        = 'Y'  ,copyright_yn='T' "
					                 " WHERE id = %ld  ; /* daem5024 */ "
					                 , ulID);

					if (mysql_query(con, szQuery))
					{
						ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
					    ZzLOG(ERROR, "DeleteContents: UPDATE T_CONTENTS_INFO error...\n");
					    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
						bFail = true;
						break;
				    }
			 	}
			 }



 			//--------------------------------------------------------------------------
			// T_CONTENTS_DEL insert
			//--------------------------------------------------------------------------
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_DEL "
				                 "     ( cont_gu      , title       "
				                 "     , id           , folder_yn   "
				                 "     , server_id    , del_date    "
				                 "     , file_path    , file_name1  "
				                 "     , file_name2   , file_size   "
				                 "     , file_type    , reg_user    "
				                 "     , reg_date     , reg_time)   "
				                 "SELECT 'WE'         ,a.title      "
				                 "     , a.id         , b.folder_yn "
				                 "     , b.server_id  , '%s'        "
				                 "     , b.file_path  , b.file_name1"
				                 "     , b.file_name2 , b.file_size "
				                 "     , b.file_type  , b.reg_user  "
				                 "     , b.reg_date   , b.reg_time  "
				                 "  FROM zangsi.T_CONTENTS_INFO a   "
				                 "     , zangsi.T_CONTENTS_FILE b   "
				                 " WHERE b.file_resoX    = 0        "
				                 "   AND a.id            = b.id     "
				                 "	and a.id = %lu "
				                 ,gdel_date
				                 ,ulID
				                 );

			ZzLOG(ALWAY, "[%s]\n\n", szQuery);
			if (mysql_query(con, szQuery)){
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "daem5024: T_CONTENTS_DEL error...\n");
			    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
		    	if( mysql_errno(con) != 1062 )
						{
							bFail = true;
							break;
						}
		    }




			//--------------------------------------------------------------------------
			// 컨텐츠 삭제 처리
			//--------------------------------------------------------------------------
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "UPDATE zangsi.T_CONTENTS_INFO"
			                 "   SET disp_stat     = 'N'   "
			                 "     , del_yn        = 'Y'   "
			                 " WHERE id = %ld  ; /* daem5024 */ "
			                 , ulID);

			ZzLOG(ALWAY, "[%s]\n\n", szQuery);
			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "DeleteContents: UPDATE T_CONTENTS_INFO error...\n");
			    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
				bFail = true;
				break;
		    }
			 #ifdef __DEBUG
			 printf("\n>T_CONTENTS_INFO : %s\n\n",szQuery);
			 #endif

			if(commit(con) != 0)
			{
			    ZzLOG(ERROR, "[DeleteContents] commit error...\n");
				ZzLOG(ERROR, "[DeleteContents] [%d](%s)\n", mysql_errno(con), mysql_error(con));
				tran_rollback(con);
				tran_end(con);
				return -1;
			}
			if(tran_end(con) != 0)
			{
			    ZzLOG(ERROR, "[DeleteContents] tran_end error...\n");
				ZzLOG(ERROR, "[DeleteContents] [%d](%s)\n", mysql_errno(con), mysql_error(con));
				tran_rollback(con);
				tran_end(con);
				return -1;
			}

			usleep(100);

		    nCount++;
		}
		mysql_free_result(delete_res);
		if(bFail)
		{
			tran_rollback(con);
			tran_end(con);
			return -1;
		}
	}



	//필로그 자료 삭제 처리

	while(1)
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " SELECT a.id, b.file_size "
						 " FROM zangsi.T_CONTFLOG_INFO a     "
						 "    , zangsi.T_CONTFLOG_FILE b     "
						 " where a.reg_user = '%s' and a.del_yn = 'N' and a.id = b.id ; /* daem5024 */ "
						 , pUserId);

		if(mysql_query(con, szQuery))
		{
			ZzLOG(ERROR, "DeleteContents: mysql_query error [%s]\n", szQuery);
			ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
			return -1;
		}
		if(!(delete_res = mysql_store_result(con)))
		{
			mysql_free_result(delete_res);
			ZzLOG(ERROR, "DeleteContents: mysql_store_result error [%s]\n", szQuery);
			ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
			return -1;
		}
		if(mysql_num_rows(delete_res) == 0)
		{
			mysql_free_result(delete_res);
			if(nFlogCount == 0)
			{
				ZzLOG(ALWAY, "DeleteContents: %s는 보유 필로그 자료가 없습니다.\n", pUserId);
				return 0;
			}
			else
			{
				ZzLOG(ALWAY, "DeleteContents: 필로그 컨텐츠 - %s %d건 처리완료\n", pUserId, nFlogCount);
				break;
			}

		}

		while(delete_row = mysql_fetch_row(delete_res))
		{
			if(tran_begin(con)!=0)
			{
				mysql_free_result(delete_res);
			    ZzLOG(ERROR, "DeleteContents: tran_begin: 테이베이스 오류입니다.\n");
				ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
				return -1;
			}

			unsigned long ulID = (unsigned long)getnum(delete_row,0);
			double dFileSize = (double)getnum(delete_row,1);

			//--------------------------------------------------------------------------
			// T_CONTENTS_ADMDEL insert
			//
			//--------------------------------------------------------------------------
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_ADMDEL"
			                 " ( cont_gu, id, del_desc       , reg_user, reg_date , reg_time) "
			                 "VALUES "
			                 " ( 'FD'   ,%ld, '판매자격 취소', 'sys5024_5', '%s'     ,'%s'     ) ; /* daem5024 */  "
			                 , ulID
			                 ,gsys_date
			                 ,gsys_time);

			if(mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "DeleteContents: INSERT T_CONTENTS_ADMDEL flog error...\n");
			    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
				bFail = true;
				break;
		    }
			 #ifdef __DEBUG
			 printf("\n>T_CONTENTS_ADMDEL flog : %s\n\n",szQuery);
			 #endif


			//----------------------------------------------------------------------
			// 이미지 삭제 정보 추가
			//----------------------------------------------------------------------
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " INSERT INTO zangsi.T_IMG_DEL "
			                 " (cont_gu, seq_no, filog_cn, file_path1, file_path2, file_name, reg_user, reg_date, reg_time)"
			                 " SELECT "
			                 "  'FD', img_no, 0, '/zangsi/project/zangsi/files_zangsi/contents/img',"
			                 "  img_spath, img_sname, reg_user, '%s', '%s'"
			                 " FROM zangsi.T_CONTFLOG_IMG "
			                 " WHERE cont_gu = 'FD' and id = %ld ; /* daem5024 */  "
			                 , gsys_date
			                 , gsys_time
			                 , ulID);

			ZzLOG(ALWAY, " 4 Query [ %s ] \n",szQuery);

			if(mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "DeleteContents: INSERT T_IMG_DEL error...\n");
			    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
				bFail = true;
				break;
		    }
			 #ifdef __DEBUG
			 printf("\n>T_IMG_DEL : %s\n\n",szQuery);
			 #endif

			//----------------------------------------------------------------------
			// 이미지 정보 삭제
			//----------------------------------------------------------------------
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " DELETE "
							 " FROM zangsi.T_CONTFLOG_IMG "
							 " WHERE cont_gu = 'FD' and id = %ld ; /* daem5024 */  "
							 , ulID);

			if(mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "DeleteContents: DELETE T_CONTENTS_IMG error...\n");
			    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
				bFail = true;
				break;
		    }
			 #ifdef __DEBUG
			 printf("\n>T_CONTENTS_IMG : %s\n\n",szQuery);
			 #endif

			//--------------------------------------------------------------------------
			// T_CONTENTS_DEL insert
			//--------------------------------------------------------------------------
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_DEL "
			                 "     ( cont_gu                    "
			                 "     , id           , folder_yn   "
			                 "     , server_id    , del_date    "
			                 "     , file_path    , file_name1  "
			                 "     , file_name2   , file_size   "
			                 "     , file_type    , reg_user    "
			                 "     , reg_date     , reg_time)   "
			                 "SELECT 'FD'                       "
			                 "     , a.id         , b.folder_yn "
			                 "     , b.server_id  , '%s'        "
			                 "     , b.file_path  , b.file_name1"
			                 "     , b.file_name2 , b.file_size "
			                 "     , b.file_type  , b.reg_user  "
			                 "     , b.reg_date   , b.reg_time  "
			                 "  FROM zangsi.T_CONTFLOG_INFO a   "
			                 "     , zangsi.T_CONTFLOG_FILE b   "
			                 " WHERE a.id = b.id and a.id = %ld ; /* daem5024 */  "
			                 , gdel_date
			                 , ulID);

			ZzLOG(ALWAY, "[%s]\n\n", szQuery);
			if(mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "DeleteContents: INSERT T_CONTENTS_DEL flog error...\n");
			    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
				bFail = true;
				break;
		    }
			 #ifdef __DEBUG
			 printf("\n>T_CONTENTS_DEL flog : %s\n\n",szQuery);
			 #endif

			//--------------------------------------------------------------------------
			// 컨텐츠 삭제 처리
			//--------------------------------------------------------------------------
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "UPDATE zangsi.T_CONTFLOG_INFO"
			                 "   SET disp_stat     = 'N'   "
			                 "     , del_yn        = 'Y'   "
			                 " WHERE id = %ld  ; /* daem5024 */  "
			                 , ulID);
			ZzLOG(ALWAY, "[%s]\n\n", szQuery);
			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "DeleteContents: UPDATE T_CONTFLOG_INFO error...\n");
			    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
				bFail = true;
				break;
		    }
			 #ifdef __DEBUG
			 printf("\n>T_CONTFLOG_INFO : %s\n\n",szQuery);
			 #endif

			//--------------------------------------------------------------------------
			//사용자 용량 업데이트
			//--------------------------------------------------------------------------
			memset (szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, "UPDATE zangsi.T_PERM_UPLOAD_AUTH			"
			                 "   SET disk_use    = disk_use    - %15.0f "
		                 	 " WHERE user_id   = '%s'   ; /* daem5024 */   "
		                 	 ,dFileSize
			                 ,pUserId
			                 );

			if (mysql_query(con, szQuery))
			{
				ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
			    ZzLOG(ERROR, "DeleteContents: UPDATE T_PERM_UPLOAD_AUTH flog error...\n");
			    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
				bFail = true;
				break;
		    }
			 #ifdef __DEBUG
			 printf("\n>T_PERM_UPLOAD_AUTH : %s\n\n",szQuery);
			 #endif

			if(commit(con) != 0)
			{
			    ZzLOG(ERROR, "[DeleteContents] commit error...\n");
				ZzLOG(ERROR, "[DeleteContents] [%d](%s)\n", mysql_errno(con), mysql_error(con));
				tran_rollback(con);
				tran_end(con);
				return -1;
			}

			if(tran_end(con) != 0)
			{
			    ZzLOG(ERROR, "[DeleteContents] tran_end error...\n");
				ZzLOG(ERROR, "[DeleteContents] [%d](%s)\n", mysql_errno(con), mysql_error(con));
				tran_rollback(con);
				tran_end(con);
				return -1;
			}

		    nFlogCount++;
		}
		mysql_free_result(delete_res);
		if(bFail)
		{
			tran_rollback(con);
			tran_end(con);
			return -1;
		}
	}

	//쪽지 보내기
	char szDesc[1500];
	memset(szDesc, 0x00, sizeof(szDesc));
	sprintf(szDesc, "위디스크 운영팀에서 알려드립니다.\r\n\r\n"
					"%s 회원님의 판매등록자격이 취소됨에 따라, %s 회원님께서 "
					"기존에 올리셨던 컨텐츠들이 일괄 삭제 되었습니다. 이는 %s "
					"회원님의 컨텐츠가 관리되지 못하고 방치될 경우, 회원간 분쟁 "
					"발생이나, 저작권 문제가 발생할 수 있는 최소한의 가능성을 "
					"예방하기 위함입니다.\r\n"
					"이용에 오해 없으시길 바랍니다.\r\n\r\n"
					"감사합니다."
					 , pUserId, pUserId, pUserId);

	char szMemoQuery[3000];
	memset(szMemoQuery, 0x00, sizeof(szMemoQuery));
/*
	sprintf(szMemoQuery, " insert into zangsi.T_MEMO_INFO "
						 " (user_id, memo_cd, ref_id, descript, send_user, recv_yn, recv_date, recv_time) "
						 " values "
						 " ('%s', '01', 0, '%s', '운영팀', 'N', '%s', '%s')  "
						 , pUserId, szDesc, gsys_date, gsys_time);

	if (mysql_query(con, szMemoQuery))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szMemoQuery);
	    ZzLOG(ERROR, "DeleteContents: insert T_MEMO_INFO error...\n");
	    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
		return -1;
    }
*/

	sprintf(szQuery, " insert into zangsi.T_SEND_MEMO (  memo_cd,  descript,send_user, del_yn, send_date, send_time ) "
						" values (  '05' "

						" ,'%s' "

						" , '운영팀' ,'N', '%s', '%s' ) ; /* daem5024 */  "
						, szDesc, gsys_date, gsys_time);

	ZzLOG(ALWAY, "[%s]\n\n", szQuery);
	if (mysql_query(con, szMemoQuery))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szMemoQuery);
	    ZzLOG(ERROR, "DeleteContents: insert T_MEMO_INFO error...\n");
	    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
		return -1;
    }

	memset(szQuery , 0x00, sizeof(szQuery ));
	strcpy( szQuery, "SELECT last_insert_id() as send_seq_no" );

	if (mysql_query(con, szMemoQuery))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szMemoQuery);
	    ZzLOG(ERROR, "DeleteContents: insert T_MEMO_INFO error...\n");
	    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
		return -1;
    }

	MYSQL_RES* myres = mysql_store_result(con);
	MYSQL_ROW myrow = mysql_fetch_row(myres);

	double send_seq_no  = getnum(myrow,0 );

	mysql_free_result(myres);


	memset(szQuery , 0x00, sizeof(szQuery ));
	sprintf(szQuery," insert into zangsi.T_RECV_MEMO "
					"  (memo_seq_no, recv_user, recv_date, recv_time, read_yn, del_yn, save_yn)  "
					" values "
					"  (%.0f, '%s' , '%s', '%s' , 'N', 'N', 'N') ; /* daem5024 */ "
					,send_seq_no,pUserId,gsys_date, gsys_time);

	ZzLOG(ALWAY, "[%s]\n\n", szQuery);
	if (mysql_query(con, szMemoQuery))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szMemoQuery);
	    ZzLOG(ERROR, "DeleteContents: insert T_MEMO_INFO error...\n");
	    ZzLOG(ERROR, "DeleteContents: [%d](%s)\n", mysql_errno(con), mysql_error(con));
		return -1;
    }


	return 0;
}

/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5024_get_sysdate()
{
	char szQuery[1000];		// query string
	char sztemp [100];      // query temp

	memset(szQuery, 0x00, sizeof(szQuery));
	if (strcmp(gproc_date, "00000000")==0) {
		strcpy(szQuery, "SELECT date_format(date_add(now(), INTERVAL -7 DAY),'%Y%m%d')");
		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL  4 DAY),'%Y%m%d')");
		strcat(szQuery, "     , date_format(now(),'%Y%m%d')");
		strcat(szQuery, "     , date_format(now(),'%H%i%s'); /* daem5024 */ " );
	}
	else
	{
		sprintf(szQuery, " SELECT "
						 " date_format('%s','%%Y%%m%%d'), "
						 " date_format(date_add(now(), INTERVAL  4 DAY),'%%Y%%m%%d'), "
						 " date_format(now(),'%%Y%%m%%d'), "
						 " date_format(now(),'%%H%%i%%s') ; /* daem5024 */  "
						 , gproc_date);
	}


	ZzLOG(ERROR, "Query [ %s ]\n",szQuery);

	if (mysql_query(con, szQuery)){
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "sysdate: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}

	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "sysdate: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "sysdate: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}

	memset(gproc_date, 0x00, sizeof(gproc_date));
	memset(gdel_date , 0x00, sizeof(gdel_date ));
	memset(gsys_date , 0x00, sizeof(gsys_date ));
	memset(gsys_time , 0x00, sizeof(gsys_time ));

	if(row = mysql_fetch_row(res))
	{
		strcpy(gproc_date,   getstr(row, 0));
		strcpy(gdel_date ,   getstr(row, 1));
		strcpy(gsys_date ,   getstr(row, 2));
		strcpy(gsys_time ,   getstr(row, 3));
	}
/*
	row = mysql_fetch_row(res);

	strcpy(gproc_date,   getstr(row, 0));
	strcpy(gdel_date ,   getstr(row, 1));
	strcpy(gsys_date ,   getstr(row, 2));
	strcpy(gsys_time ,   getstr(row, 3));
*/
	ZzLOG(ALWAY, "gproc_date[%s]\n", gproc_date);
	ZzLOG(ALWAY, "gdel_date [%s]\n", gdel_date);
	ZzLOG(ALWAY, "gsys_date [%s]\n", gsys_date);
	ZzLOG(ALWAY, "gsys_time [%s]\n", gsys_time);

	mysql_free_result(res);

	return 0;
}


/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daem5024_init_process(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
    ZzInitGlobalVariable2("d_", "/logs/daemon");


    ZzLOG(ALWAY, "[daem5024]*****************프로그램 시작*****************\n");
    ZzLOG(ALWAY, "[daem5024] 판매자격 취소 회원 컨텐츠 삭제처리\n");

    // 파라미터 값 설정 및 초기화
    if (argc < 2){
    	goto arg_error;
    }

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------

	if (!(con=db_connect_local("zangsi")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1);
	}

	if (!(con_bck=db_connect_backup("zangsi")))
	{
		ZzLOG(ERROR, "백업 DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con_bck);
	   	return(-1);
	}

	if (!(con_sub=db_connect_subdb("zangsi")))
	{
		ZzLOG(ERROR, "백업 DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con_sub);
	   	return(-1);
	}



	/* 처리일자 */
	memset(gproc_date, 0x00, sizeof(gproc_date));

	strcpy(gproc_date, argv[1]);

	ret=daem5024_get_sysdate();

	if (ret < 0){
		db_disconnect(con);
		return -1;
	}

    return (0);


arg_error:
    ZzLOG(ERROR, "usage : %s YYYYMMDD XXXXX\n", argv[0]);
    ZzLOG(ERROR, "        YYYYMMDD(처리일자)-> '00000000'인경우 system일자로처리\n");

    ZzPRT(ERROR, "usage : %s YYYYMMDD XXXXX\n", argv[0]);
    ZzPRT(ERROR, "        YYYYMMDD(처리일자)-> '00000000'인경우 system일자로처리\n");
    return -1;
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daem5024_term_process()
{
    // DB close
	db_disconnect(con);
	db_disconnect(con_bck);
    ZzLOG(ALWAY, "[daem5024]*****************프로그램 종료*****************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daem5024_signal(int nSignal)
{
    daem5024_term_process();
}

/*****************************************************************************
*  프로그램 메인
*****************************************************************************/
int main(int argc, char **argv)
{

	int     rc;

	if ( daem5024_init_process(argc, argv) == 0 ) {
		daem5024_main_process();

	}
	daem5024_term_process();
	return(0);
}
/*****************************************************************************
*  End of file...
*****************************************************************************/
