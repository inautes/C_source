/******************************************************************************
 *   서브시스템 : FUP서버
 *   프로그램명 : com9003.cc
 *         기능 : 내디스크사용량UPDATE
 *         설명 : 내디스크에 파일upload, 복사, 삭제시 디스크사용량을 UPDATE한다
 *       작성자 : JDP
 *       작성일 : 2004/02/16
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

#include "apdefine.h"
#include "comcomm.h"
#include "commydb.h"
#include "com9003.h"
//#define  _DEBUG_

//******************************************************************************
//* COM9003 main
//* error 발생시 pSendData를 사용하고 정상인경우 seq_no를 return한다.
//  return:  1(정상)
//          -1(DB오류)
//          -2(minus처리시 사이즈가 너무작음)
//          -3(남아있는 용량보다 사이즈가 큼)
//******************************************************************************
long com90031(CCOM9003_R pcom9003_r,char* szIP, int nPort)
{
	char szQuery[10000];
	MYSQL       *con;
	MYSQL_RES *res;
	MYSQL_ROW  row;
	double  disk_use                ;  // 파일크기
	double  disk_rem                ;  // 디스크남은용량

	
	#ifdef _DEBUG_
	printf("com9003-> start\n");
	printf("com9003-> user_id  (%s)\n"    , pcom9003_r.user_id   );
	printf("com9003-> file_size(%13.0f)\n", pcom9003_r.file_size );
	#endif

	//--------------------------------------------------------------------------
	// 서버정보용량 UPDATE
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery ));
	sprintf(szQuery, "SELECT a.disk_use               "
	                 "     , a.disk_size - a.disk_use "
	                 "  FROM T_MYDATA_INFO a   "
	                 " WHERE a.user_id = '%s'         "
	                 ,pcom9003_r.user_id
	                 );
	// DB 연결
	
	if (!(con=db_connect("zangsi")))
	{
		infLOG(ERROR, "com9003[ERR]: DB에 접속하지 못 하였습니다.\n");
		infLOG(ERROR, "com9003[SQL]: %s\n", szQuery);
       	return(-1); 
	}
	

	if (mysql_query(con, szQuery)){
		infLOG(ERROR, "com9003[ERR]: 검색시 오류가 발생했습니다.\n");
		infLOG(ERROR, "com9003[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		db_disconnect(con);
       	return(-1); 
	}

	if (!(res = mysql_store_result(con)))
	{
		infLOG(ERROR, "com9003[ERR]: 검색시 오류가 발생했습니다.\n");
		infLOG(ERROR, "com9003[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		db_disconnect(con);
	   	return(-1); 
	}

 	if (mysql_num_rows(res)==0)
 	{
		infLOG(ERROR, "com9003[ERR]: 검색시 오류가 발생하였습니다.\n");
		infLOG(ERROR, "com9003[ERR]: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		db_disconnect(con);
	   	return(-1); 
	}
	
	memset(&disk_use, 0x00, sizeof(disk_use));
	memset(&disk_rem, 0x00, sizeof(disk_rem));
	if (row = mysql_fetch_row(res))
	{
		disk_use = getnum(row, 0);
		disk_rem = getnum(row, 1);
	}
	mysql_free_result(res);

	#ifdef _DEBUG_
	printf("com9003-> disk_use(%13.0f)\n", disk_use );
	printf("com9003-> disk_rem(%13.0f)\n", disk_rem );
	#endif

	if (pcom9003_r.file_size < 0)
	{
		if ((disk_use + pcom9003_r.file_size) < 0)
		{
			    
			#ifdef _DEBUG_
			printf("com9003-> return (-2)\n");
			#endif
			db_disconnect(con);
			return (-2);
		}
	}
	else
	{
		if (pcom9003_r.file_size > 0)
		{
			if (pcom9003_r.file_size > disk_rem)
			{
				#ifdef _DEBUG_
				printf("com9003-> return (-3)\n");
				#endif
				db_disconnect(con);
				return (-3);
			}
		}
	}
	//--------------------------------------------------------------------------
	// 서버정보용량 UPDATE
	//--------------------------------------------------------------------------
	memset (szQuery , 0x00, sizeof(szQuery ));
	#ifdef _DEBUG_
	printf( "UPDATE T_MYDATA_INFO        \n"
			"   SET disk_use = disk_use + %13.0f\n"
			" WHERE user_id  = '%s'             \n"
			,pcom9003_r.file_size
			,pcom9003_r.user_id
			);
	#endif	     
	
	sprintf(szQuery, "UPDATE T_MYDATA_INFO        "
	                 "   SET disk_use = disk_use + %13.0f"
	                 " WHERE user_id  = '%s'            "
	                 ,pcom9003_r.file_size
	                 ,pcom9003_r.user_id
	                 );

		#ifdef _DEBUG_
		printf("com9003-> (End Query)\n");
		#endif
	            

	if (mysql_query(con, szQuery)){
		infLOG(ERROR, "com9003[ERR]: UPDATE T_MYDATA_INFO (%s)\n", mysql_error(con));
		infLOG(ERROR, "com9003[SQL]: %s\n", szQuery);
		db_disconnect(con);
		
		#ifdef _DEBUG_
		printf("com9003-> return (-1)\n");
		#endif
		return -1;
    }
    
   

	db_disconnect(con);
	
	 
	#ifdef _DEBUG_
	printf("com9003-> end\n");
	#endif
	return (1);
}

