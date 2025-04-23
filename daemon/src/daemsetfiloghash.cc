/******************************************************************************
 *   서브시스템 : daemon프로세스
 *   프로그램명 : daemsetfiloghash.cc
 *         기능 : 필로그 해시정보 DB 입력
 *         설명 : 
 *       작성자 : HCS
 *       작성일 : 2009/10/30
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
#include <unistd.h> //for usleep();
#include <ftw.h>
#include <stdint.h>

#include "daemcom.h"
#include "commydb.h"

#define  MAX_ROWS	1
#define _DEBUG_

#define T1 0xd76aa478
#define T2 0xe8c7b756
#define T3 0x242070db
#define T4 0xc1bdceee
#define T5 0xf57c0faf
#define T6 0x4787c62a
#define T7 0xa8304613
#define T8 0xfd469501
#define T9 0x698098d8
#define T10 0x8b44f7af
#define T11 0xffff5bb1
#define T12 0x895cd7be
#define T13 0x6b901122
#define T14 0xfd987193
#define T15 0xa679438e
#define T16 0x49b40821
#define T17 0xf61e2562
#define T18 0xc040b340
#define T19 0x265e5a51
#define T20 0xe9b6c7aa
#define T21 0xd62f105d
#define T22 0x02441453
#define T23 0xd8a1e681
#define T24 0xe7d3fbc8
#define T25 0x21e1cde6
#define T26 0xc33707d6
#define T27 0xf4d50d87
#define T28 0x455a14ed
#define T29 0xa9e3e905
#define T30 0xfcefa3f8
#define T31 0x676f02d9
#define T32 0x8d2a4c8a
#define T33 0xfffa3942
#define T34 0x8771f681
#define T35 0x6d9d6122
#define T36 0xfde5380c
#define T37 0xa4beea44
#define T38 0x4bdecfa9
#define T39 0xf6bb4b60
#define T40 0xbebfbc70
#define T41 0x289b7ec6
#define T42 0xeaa127fa
#define T43 0xd4ef3085
#define T44 0x04881d05
#define T45 0xd9d4d039
#define T46 0xe6db99e5
#define T47 0x1fa27cf8
#define T48 0xc4ac5665
#define T49 0xf4292244
#define T50 0x432aff97
#define T51 0xab9423a7
#define T52 0xfc93a039
#define T53 0x655b59c3
#define T54 0x8f0ccc92
#define T55 0xffeff47d
#define T56 0x85845dd1
#define T57 0x6fa87e4f
#define T58 0xfe2ce6e0
#define T59 0xa3014314
#define T60 0x4e0811a1
#define T61 0xf7537e82
#define T62 0xbd3af235
#define T63 0x2ad7d2bb
#define T64 0xeb86d391

typedef struct md5_state_s 
{
    unsigned int count[2];	/* message length in bits, lsw first */
    unsigned int abcd[4];		/* digest buffer */
    unsigned char buf[64];		/* accumulate block */
} md5_state_t;

char* GetString(unsigned char* pStr  , int nLen );
char* GetHashFromFile(char* strFileFullPath);

static void md5_process(md5_state_t *pms, const unsigned char *pData /*[64]*/);	
void md5_init(md5_state_t *pms);
void md5_append(md5_state_t *pms, const unsigned char *pData, int nBytes);
void md5_finish(md5_state_t *pms, unsigned char digest[16]);

int daemsfh_init_process(int argc, char **argv);
int daemsfh_main_process();
int daemsfh_check_hash(unsigned long ulID);
int daemsfh_inset_hash(int nDepth, char* pFolderPath, char* pFileName, double dFileSize, char* pDefaultHash);
char* AppendSpecialChar(char* strSrcString, char cReplace,char* pResult);
int daemsfh_get_sysdate();
int daemsfh_term_process();
void daemsfh_signal(int nSignal);

int GetFileInfo( const char* file, const struct stat64 *sb, int flag);

MYSQL     *con;
MYSQL     *con_bck;

char   gsys_date  [  8+1];	//등록일
char   gsys_time  [  6+1];	//등록시간
char   ghost_name [  5+1];	//분류

unsigned long id;
char   gfile_path [  255+1];	//분류
char   gfile_name1 [  30+1];	//분류
char   gfolder_yn [  1+1];	//분류
unsigned long glist_seq_no;
unsigned long gsub_seq_no;





//******************************************************************************
//* daemsfh main
//******************************************************************************
int daemsfh_main_process()
{
    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "daemsfh_main_process: gsys_date[%s]\n", gsys_date);  
    ZzLOG(ALWAY, "daemsfh_main_process: gsys_time [%s]\n", gsys_time );  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    
	MYSQL_RES *res;
	MYSQL_ROW  row;

    char szQuery[1000];
    memset(szQuery, 0x00, sizeof(szQuery));
    
    
    int nCount = 0;

   	sprintf(szQuery, " select a.id, b.file_path, b.file_name1, b.folder_yn "
   					 " from zangsi.T_CONTFLOG_INFO a, zangsi.T_CONTFLOG_FILE b "
   					 " where a.id = b.id and b.server_id = '%s' "
   					 , ghost_name);
	
#ifdef __DEBUG
ZzLOG(ALWAY,"Query = [%s]\n", szQuery);
#endif    					 
	if (mysql_query(con_bck, szQuery))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daemsfh_main_process: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con_bck), mysql_error(con_bck));
		return -1;
	}
	
	if (!(res = mysql_store_result(con_bck)))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daemsfh_main_process: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con_bck), mysql_error(con_bck));
		return -1;
	}
 	if(mysql_num_rows(res)==0)
 	{
	    ZzLOG(ALWAY, "daemsfh_main_process: 처리할 자료가 없습니다.[%d]건 성공\n", nCount);
		mysql_free_result(res);
		return 0;
	}
	while(row = mysql_fetch_row(res))
	{
		id = (unsigned long)getint(row,0);
		
		memset(gfile_path, 0x00, sizeof(gfile_path));
		strcpy(gfile_path, getstr(row, 1));
		
		memset(gfile_name1, 0x00, sizeof(gfile_name1));
		strcpy(gfile_name1, getstr(row, 2));
		
		memset(gfolder_yn, 0x00, sizeof(gfolder_yn));
		strcpy(gfolder_yn, getstr(row, 3));

		char szPathBuf[300];
		memset(szPathBuf, 0x00, sizeof(szPathBuf));
		
		sprintf(szPathBuf, "%s/%s", gfile_path, gfile_name1);

		int nRes = daemsfh_check_hash(id);

		if(nRes == 0)
		{
			glist_seq_no = 0;
			gsub_seq_no = 0;
			ZzLOG(ALWAY, "daemsfh_main_process: 처리 시작[%lu]\n", id);
			if(ftw64(szPathBuf, GetFileInfo, 1) < 0)
			{
				ZzLOG(ERROR,"daemsfh_main_process: %lu 컨텐츠 파일정보 얻기 실패(%s)\n", id, szPathBuf);
				continue;
			}
			ZzLOG(ALWAY, "daemsfh_main_process: 처리 완료[%lu]\n", id);
			nCount++;
		}
		else if(nRes < 0)
		{
			mysql_free_result(res);
			ZzLOG(ALWAY, "daemsfh_main_process: [%d]건 성공\n", nCount);
			return -1;
		}	
	}
	mysql_free_result(res);

	
	ZzLOG(ALWAY, "daemsfh_main_process: [%d]건 성공\n", nCount);
	return 0;
}

/*****************************************************************************
* 현재 디렉토리를 recursive하게 탐색
* (I) 1. const char *file : ftw()에서 받아온 파일패스
*	  2. const struct stat *sb : 탐색된 파일/폴더의 정보를 담은 구조체
	  3. int flag : FTW_F - 파일.
	  				FTW_D - 폴더.
	  				FTW_NS - stat구조체가 읽어들일 수없는 파일.
* (R) 	 		
*****************************************************************************/
int GetFileInfo( const char* file, const struct stat64 *sb, int flag)
{
	char *pErrMsg = NULL;
	char szBuffer[512];
	memset(szBuffer, 0x00, sizeof(szBuffer));
	
	char szFolderPath[255+1];
	memset(szFolderPath, 0x00, sizeof(szFolderPath));
	
	char szFullPath[512];
	memset(szFullPath, 0x00, sizeof(szFullPath));
	
	strcpy(szFullPath, file);

	char szFilePath[255+1];
	memset(szFilePath, 0x00, sizeof(szFilePath));
	
	sprintf(szFilePath, "%s/", gfile_path);
	
	
	int nDepth = 0;
	
	char szSubFilePath[512];
	memset(szSubFilePath,0x00,sizeof(szSubFilePath));			

	char szTempSubFilePath[512];
	memset(szTempSubFilePath,0x00,sizeof(szTempSubFilePath));			
	
	switch(flag)
	{
		case FTW_F: // 파일
		{
			char* pDestTemp = strrchr(file, '/');
			char* pDest = strtok(pDestTemp,"/");
			
			char szDest[512];
			memset(szDest, 0x00, sizeof(szDest));
			strcpy(szDest, pDest);
			
			double dFileSize = sb->st_size;
			
			if(strcmp(gfolder_yn, "Y") == 0)
			{	
				int nMoveLen = strlen(szFilePath);
				int nDestLen = strlen(szFullPath);
					
				if( strstr( szFullPath ,szFilePath ) != NULL  && nDestLen - nMoveLen > 0 )
				{
					memcpy(szTempSubFilePath,szFullPath + nMoveLen ,  nDestLen - nMoveLen );

					int nSubDestLen = strlen(szDest);
					int nSubMoveLen = strlen(szTempSubFilePath) - nSubDestLen;
					
					memcpy(szSubFilePath, szTempSubFilePath,  nSubMoveLen);
					
					char* pTemp = strtok(szSubFilePath,"/");
		
					while(pTemp!=NULL )
					{
						nDepth ++ ;
						pTemp = strtok(NULL,"/");
						
						if(  pTemp != NULL)
						{
							strcat(szFolderPath ,pTemp);
							strcat(szFolderPath ,"/");
						}
					}
					if( nDepth > 0 )
						nDepth--;
				}
			}
			
			/*
			DB 등록 함수
			*/
			char szDefaultHash[32+1];
			memset(szDefaultHash, 0x00, sizeof(szDefaultHash));
			
			strcpy(szDefaultHash, GetHashFromFile((char*)file));
			
			daemsfh_inset_hash(nDepth, szFolderPath, szDest, dFileSize, szDefaultHash);
			
			/*
			ZzLOG(ALWAY, "GetFileInfo: Depth - (%d)\n", nDepth);
			ZzLOG(ALWAY, "GetFileInfo: FolderPath - (%s)\n", szFolderPath);
			ZzLOG(ALWAY, "GetFileInfo: FileName - (%s)\n", szDest);
			ZzLOG(ALWAY, "GetFileInfo: FileSize - (%15.0f)\n\n", dFileSize);
			*/
			break;
		}
		case FTW_D: // 폴더
		{
			break;
		}
		case FTW_NS:
		{
			ZzLOG(ERROR, "GetFileInfo: FTW_NS(%s)\n", file);
			break;	
		}	
		case FTW_DNR:
		{
			ZzLOG(ERROR, "GetFileInfo: FTW_DNR(%s)\n", file);
			break;	
		}	
		default:
		{
			ZzLOG(ERROR, "GetFileInfo: UNKOWN TYPE(%s)\n", file);
			return -1;
		}
	}
	return 0;
}	

int daemsfh_check_hash(unsigned long ulID)
{
	MYSQL_RES *res;
	MYSQL_ROW  row;
	
	char szQuery[1000];
	memset(szQuery, 0x00, sizeof(szQuery));

   	sprintf(szQuery, " select id "
   					 " from zangsi.T_CONTFLOG_FILELIST "
   					 " where id = %lu limit 1 "
   					 , id);
   					 
	if (mysql_query(con_bck, szQuery))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daemsfh_check_hash: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con_bck), mysql_error(con_bck));
		return -1;
	}
	if (!(res = mysql_store_result(con_bck)))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daemsfh_check_hash: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con_bck), mysql_error(con_bck));
		mysql_free_result(res);
		return -1;
	}
 	if (mysql_num_rows(res) > 0)
 	{
	    ZzLOG(ALWAY, "daemsfh_check_hash: 이미 등록된 파일정보가 있습니다.(%lu)\n", ulID);
		mysql_free_result(res);
		return 1;
	}
	
	mysql_free_result(res);
	
	return 0;
	
}

int daemsfh_inset_hash(int nDepth, char* pFolderPath, char* pFileName, double dFileSize, char* pDefaultHash)
{
	
	MYSQL_RES *res;
	MYSQL_ROW  row;
	char szQuery[10000];
	memset(szQuery, 0x00, sizeof(szQuery));
	
	if(nDepth == 0)
	{
		memset(szQuery, 0x00, sizeof(szQuery));
	   	sprintf(szQuery, " insert into zangsi.T_CONTFLOG_FILELIST "
	   					 " (id, seq_no, folder_yn, file_name, file_size, reg_user, , reg_date, reg_time, default_hash, copyright_yn) "
	   					 " select "
	   					 " a.id, %lu, a.folder_yn, \"%s\", %15.0f, a.reg_user, a.reg_date, a.reg_time, \"%s\", b.copyright_yn "
	   					 " from zangsi.T_CONTFLOG_FILE a, zangsi.T_CONTFLOG_VIR_ID b "
	   					 " where a.id = b.id and a.id = %lu  "
	   					 , glist_seq_no
	   					 , pFileName
	   					 , dFileSize
	   					 , pDefaultHash
	   					 , id);
		
		ZzLOG(ALWAY, "Query [ %s ]\n",szQuery);	
		/*
		if (mysql_query(con_bck, szQuery))
		{
			ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
		    ZzLOG(ERROR, "daemsfh_inset_hash: mysql_query error...\n");
			ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con_bck), mysql_error(con_bck));
			return -1;
		}
		*/
		glist_seq_no++;
	}
	
	memset(szQuery, 0x00, sizeof(szQuery));
   	sprintf(szQuery, " insert into zangsi.T_CONTFLOG_FILELIST_SUB "
   					 " (seq_no, id, depth, folder_yn, folder_path, file_name, file_size, reg_user, reg_date, reg_time, default_hash, copyright_yn) "
   					 " select "
   					 " %lu, a.id, %d, a.folder_yn, \"%s\", \"%s\", %15.0f, a.reg_user, a.reg_date, a.reg_time, \"%s\", b.copyright_yn "
   					 " from zangsi.T_CONTFLOG_FILE a, zangsi.T_CONTFLOG_VIR_ID b "
   					 " where a.id = b.id and a.id = %lu  "
   					 , gsub_seq_no
   					 , nDepth
   					 , pFolderPath
   					 , pFileName
   					 , dFileSize
   					 , pDefaultHash
   					 , id);

	ZzLOG(ALWAY, "Query [ %s ]\n",szQuery);
	/*
	if (mysql_query(con_bck, szQuery))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "daemsfh_inset_hash: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con_bck), mysql_error(con_bck));
		return -1;
	}
	*/
	gsub_seq_no++;

	return 0;
}

char* AppendSpecialChar(char* strSrcString, char cReplace,char* pResult)
{

	char strResult[130000];

	memset(strResult,0x00,sizeof(strResult));
	
	int cTemp;
	int nSpecialCount=0;
	int nFileLen = strlen(strSrcString);
	int cOldTemp ;
	
	
	for(unsigned long i=0; i<nFileLen ; i++)
	{
		if( i > 0 )
		{
			cOldTemp = strSrcString[i-1];
		}
		cTemp = strSrcString[i];
		
	
		
		if( cTemp == '\'' && cOldTemp != '\\' ) 
		{
	

			strResult[nSpecialCount] = (char)cReplace;
			nSpecialCount++;
			strResult[nSpecialCount] = (char)cTemp;
			
				
		}
		else
		{
			if(cTemp<0)//한글 (is korean)
			{
				char* pHan = &strSrcString[i];
				memcpy(&strResult[nSpecialCount] ,pHan,2);
				nSpecialCount++;
				
	
								
				
				i=i+1;
			}
			else
			{
	
			
				strResult[nSpecialCount] = (char)cTemp;		
			}
		}
		nSpecialCount++;
		

	}
	
	strcpy(pResult,strResult);
	


	return pResult;
}
/*****************************************************************************
* DB에서 system Date를 얻는다.
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daemsfh_get_sysdate()
{
	MYSQL_RES *res;
	MYSQL_ROW  row;

	char szQuery[1000];		// query string
	char sztemp [100];      // query zangsi

	memset(szQuery, 0x00, sizeof(szQuery));
	strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d'), date_format(now(),'%H%i%s')");

	if (mysql_query(con_bck, szQuery))
	{
		ZzLOG(ERROR, "Query [ %s ]\n",szQuery);
	    ZzLOG(ERROR, "sysdate: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con_bck), mysql_error(con_bck));
		return -1;
	}
	
	if (!(res = mysql_store_result(con_bck)))
	{
	    ZzLOG(ERROR, "sysdate: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con_bck), mysql_error(con_bck));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "sysdate: mysql_nurows error...\n");
		ZzLOG(ERROR, "[%d](%s)\n",mysql_errno(con_bck), mysql_error(con_bck));
		mysql_free_result(res);
		return -1;
	}
	row = mysql_fetch_row(res);
	memset(gsys_date , 0x00, sizeof(gsys_date ));
	memset(gsys_time , 0x00, sizeof(gsys_time ));

	strcpy(gsys_date ,   getstr(row, 0));
	strcpy(gsys_time ,   getstr(row, 1));
	
	mysql_free_result(res);

	return 0;
}

/*****************************************************************************
* 프로그램 시작루틴
* 전역변수 초기화 및 데이타베이스 연결
* (I) void
* (R) int : 정상(0)/오류(-1)
*****************************************************************************/
int daemsfh_init_process(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** 전역변수 초기화
    */
#ifdef __DEBUG
    ZzInitGlobalVariable2("daemsfh", "/home/ezwon/zangsi_with_dcmd/daemon/log"); 

#else
    ZzInitGlobalVariable2("daemsfh", "/logs/daemon"); 
#endif    

    ZzLOG(ALWAY, "[daemsfh]*****************프로그램 시작*****************\n");  

	//--------------------------------------------------------------------------
	// DB 연결
	//--------------------------------------------------------------------------
	if (!(con=db_connect("zangsi")))
	{
		ZzLOG(ERROR, "DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
	   	return(-1); 
	}
	

	if (!(con_bck=db_connect_backup("zangsi")))
	{
		ZzLOG(ERROR, "search bck DB에 접속하지 못 하였습니다...\n");
		db_disconnect(con);
		db_disconnect(con_bck);
	   	return(-1); 
	}

	/* 처리일자 */
	memset(ghost_name, 0x00, sizeof(ghost_name));
	
	strcpy(ghost_name, getenv("HOSTNAME"));
	
	ret=daemsfh_get_sysdate();
	if (ret < 0)
	{
		db_disconnect(con);
		db_disconnect(con_bck);
		return -1;
	}
	
    return (0);
}

/***************************************************************************
* 프로그램 종료루틴
* 데이터베이스 종료 및 처리결과를 로그파일에 정의
* (I) void
* (R) int : 정상(0)/오류(-1)
****************************************************************************/
int daemsfh_term_process()
{
    // DB close
	db_disconnect(con);
	db_disconnect(con_bck);
	
    ZzLOG(ALWAY, "[daemsfh]*****************프로그램 종료*****************\n\n");

    return (0);
}

/*****************************************************************************
* 프로그램 시그널 처리
* (I) void
* (R) void
*****************************************************************************/
void  daemsfh_signal(int nSignal)
{
    daemsfh_term_process();
}

/*****************************************************************************
*  프로그램 메인 
*****************************************************************************/
int main(int argc, char **argv)
{                
	char    szTemp[1024];
	int     rc;
                 
	/*       
	** SIGNAL 정의
	*/       
	signal(SIGTERM, daemsfh_signal);
	signal(SIGINT,  daemsfh_signal);
	signal(SIGQUIT, daemsfh_signal);
	signal(SIGKILL, daemsfh_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
                 
	if ( daemsfh_init_process(argc, argv) == 0 ) 
	{
		/* 프로그램 메인루틴 */
		rc = daemsfh_main_process();
		/* 프로그램 종료루틴 */                    
		daemsfh_term_process();
	}
	return(0);
}                
void md5_process(md5_state_t *pms, const unsigned char *pData /*[64]*/)
{
    unsigned int
	a = pms->abcd[0], b = pms->abcd[1],
	c = pms->abcd[2], d = pms->abcd[3];
    unsigned int t;

#define A
#ifndef ARCH_IS_BIG_ENDIAN
#define ARCH_IS_BIG_ENDIAN 1	/* slower, default implementation */
#endif
#if ARCH_IS_BIG_ENDIAN

    /*
     * On big-endian machines, we must arrange the bytes in the right
     * order.  (This also works on machines of unknown byte order.)
     */
    unsigned int X[16];
    const unsigned char *xp = pData;
    int i;

    for (i = 0; i < 16; ++i, xp += 4)
	X[i] = xp[0] + (xp[1] << 8) + (xp[2] << 16) + (xp[3] << 24);

#else  /* !ARCH_IS_BIG_ENDIAN */

    /*
     * On little-endian machines, we can process properly aligned data
     * without copying it.
     */
    unsigned int xbuf[16];
    const unsigned int *X;

    if (!((pData - (const unsigned char *)0) & 3)) {
	/* data are properly aligned */
	X = (const unsigned int *)pData;
    } else {
	/* not aligned */
	memcpy(xbuf, pData, 64);
	X = xbuf;
    }
#endif

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

    /* Round 1. */
    /* Let [abcd k s i] denote the operation
       a = b + ((a + F(b,c,d) + X[k] + T[i]) <<< s). */
#define F(x, y, z) (((x) & (y)) | (~(x) & (z)))
#define SET(a, b, c, d, k, s, Ti)\
  t = a + F(b,c,d) + X[k] + Ti;\
  a = ROTATE_LEFT(t, s) + b
    /* Do the following 16 operations. */
    SET(a, b, c, d,  0,  7,  T1);
    SET(d, a, b, c,  1, 12,  T2);
    SET(c, d, a, b,  2, 17,  T3);
    SET(b, c, d, a,  3, 22,  T4);
    SET(a, b, c, d,  4,  7,  T5);
    SET(d, a, b, c,  5, 12,  T6);
    SET(c, d, a, b,  6, 17,  T7);
    SET(b, c, d, a,  7, 22,  T8);
    SET(a, b, c, d,  8,  7,  T9);
    SET(d, a, b, c,  9, 12, T10);
    SET(c, d, a, b, 10, 17, T11);
    SET(b, c, d, a, 11, 22, T12);
    SET(a, b, c, d, 12,  7, T13);
    SET(d, a, b, c, 13, 12, T14);
    SET(c, d, a, b, 14, 17, T15);
    SET(b, c, d, a, 15, 22, T16);
#undef SET

     /* Round 2. */
     /* Let [abcd k s i] denote the operation
          a = b + ((a + G(b,c,d) + X[k] + T[i]) <<< s). */
#define G(x, y, z) (((x) & (z)) | ((y) & ~(z)))
#define SET(a, b, c, d, k, s, Ti)\
  t = a + G(b,c,d) + X[k] + Ti;\
  a = ROTATE_LEFT(t, s) + b
     /* Do the following 16 operations. */
    SET(a, b, c, d,  1,  5, T17);
    SET(d, a, b, c,  6,  9, T18);
    SET(c, d, a, b, 11, 14, T19);
    SET(b, c, d, a,  0, 20, T20);
    SET(a, b, c, d,  5,  5, T21);
    SET(d, a, b, c, 10,  9, T22);
    SET(c, d, a, b, 15, 14, T23);
    SET(b, c, d, a,  4, 20, T24);
    SET(a, b, c, d,  9,  5, T25);
    SET(d, a, b, c, 14,  9, T26);
    SET(c, d, a, b,  3, 14, T27);
    SET(b, c, d, a,  8, 20, T28);
    SET(a, b, c, d, 13,  5, T29);
    SET(d, a, b, c,  2,  9, T30);
    SET(c, d, a, b,  7, 14, T31);
    SET(b, c, d, a, 12, 20, T32);
#undef SET

     /* Round 3. */
     /* Let [abcd k s t] denote the operation
          a = b + ((a + H(b,c,d) + X[k] + T[i]) <<< s). */
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define SET(a, b, c, d, k, s, Ti)\
  t = a + H(b,c,d) + X[k] + Ti;\
  a = ROTATE_LEFT(t, s) + b
     /* Do the following 16 operations. */
    SET(a, b, c, d,  5,  4, T33);
    SET(d, a, b, c,  8, 11, T34);
    SET(c, d, a, b, 11, 16, T35);
    SET(b, c, d, a, 14, 23, T36);
    SET(a, b, c, d,  1,  4, T37);
    SET(d, a, b, c,  4, 11, T38);
    SET(c, d, a, b,  7, 16, T39);
    SET(b, c, d, a, 10, 23, T40);
    SET(a, b, c, d, 13,  4, T41);
    SET(d, a, b, c,  0, 11, T42);
    SET(c, d, a, b,  3, 16, T43);
    SET(b, c, d, a,  6, 23, T44);
    SET(a, b, c, d,  9,  4, T45);
    SET(d, a, b, c, 12, 11, T46);
    SET(c, d, a, b, 15, 16, T47);
    SET(b, c, d, a,  2, 23, T48);
#undef SET

     /* Round 4. */
     /* Let [abcd k s t] denote the operation
          a = b + ((a + I(b,c,d) + X[k] + T[i]) <<< s). */
#define I(x, y, z) ((y) ^ ((x) | ~(z)))
#define SET(a, b, c, d, k, s, Ti)\
  t = a + I(b,c,d) + X[k] + Ti;\
  a = ROTATE_LEFT(t, s) + b
     /* Do the following 16 operations. */
    SET(a, b, c, d,  0,  6, T49);
    SET(d, a, b, c,  7, 10, T50);
    SET(c, d, a, b, 14, 15, T51);
    SET(b, c, d, a,  5, 21, T52);
    SET(a, b, c, d, 12,  6, T53);
    SET(d, a, b, c,  3, 10, T54);
    SET(c, d, a, b, 10, 15, T55);
    SET(b, c, d, a,  1, 21, T56);
    SET(a, b, c, d,  8,  6, T57);
    SET(d, a, b, c, 15, 10, T58);
    SET(c, d, a, b,  6, 15, T59);
    SET(b, c, d, a, 13, 21, T60);
    SET(a, b, c, d,  4,  6, T61);
    SET(d, a, b, c, 11, 10, T62);
    SET(c, d, a, b,  2, 15, T63);
    SET(b, c, d, a,  9, 21, T64);
#undef SET

     /* Then perform the following additions. (That is increment each
        of the four registers by the value it had before this block
        was started.) */
    pms->abcd[0] += a;
    pms->abcd[1] += b;
    pms->abcd[2] += c;
    pms->abcd[3] += d;
}
void md5_init(md5_state_t *pms)
{
    pms->count[0] = pms->count[1] = 0;
    pms->abcd[0] = 0x67452301;
    pms->abcd[1] = 0xefcdab89;
    pms->abcd[2] = 0x98badcfe;
    pms->abcd[3] = 0x10325476;
}
void md5_append(md5_state_t *pms, const unsigned char *pData, int nBytes)
{
    const unsigned char *p = pData;
    int left = nBytes;
    int offset = (pms->count[0] >> 3) & 63;
    unsigned int nbits = (unsigned int)(nBytes << 3);

    if (nBytes <= 0)
	return;

    /* Update the message length. */
    pms->count[1] += nBytes >> 29;
    pms->count[0] += nbits;
    if (pms->count[0] < nbits)
	pms->count[1]++;

    /* Process an initial partial block. */
    if (offset) {
	int copy = (offset + nBytes > 64 ? 64 - offset : nBytes);

	memcpy(pms->buf + offset, p, copy);
	if (offset + copy < 64)
	    return;
	p += copy;
	left -= copy;
	md5_process(pms, pms->buf);
    }

    /* Process full blocks. */
    for (; left >= 64; p += 64, left -= 64)
	md5_process(pms, p);

    /* Process a final partial block. */
    if (left)
	memcpy(pms->buf, p, left);
}
void md5_finish(md5_state_t *pms, unsigned char digest[16])
{
    static const unsigned char pad[64] = {
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    unsigned char data[8];
    int i;

    /* Save the length before padding. */
    for (i = 0; i < 8; ++i)
	data[i] = (unsigned char)(pms->count[i >> 2] >> ((i & 3) << 3));
    /* Pad to 56 bytes mod 64. */
    md5_append(pms, pad, ((55 - (pms->count[0] >> 3)) & 63) + 1);
    /* Append the length. */
    md5_append(pms, data, 8);
    for (i = 0; i < 16; ++i)
	digest[i] = (unsigned char)(pms->abcd[i >> 2] >> ((i & 3) << 3));
}
char* GetString(unsigned char* pStr , int nLen)
{
	md5_state_t state;
	static unsigned char digest[16];

	static char szResult[32];
	memset(szResult,0x00,sizeof(szResult));
	
	md5_init(&state);
	
	do 
	{
		if( nLen < 1024 )
			md5_append(&state,(const unsigned char *)pStr,nLen);
		else
			md5_append(&state,(const unsigned char *)pStr,1024);
		nLen -= 1024;
	} while( nLen <= 0 );
	
	md5_finish(&state, digest);

	sprintf(szResult,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
		,digest[0],digest[1],digest[2],digest[3],digest[4],digest[5],digest[6],digest[7]
		,digest[8],digest[9],digest[10],digest[11],digest[12],digest[13],digest[14],digest[15]);
	
	return szResult;
} /* MD5 */
char* GetHashFromFile(char* strFileFullPath  )
{
	char pFileFullPath[512];
	memset(pFileFullPath, 0x00, sizeof(pFileFullPath));

	strcpy(pFileFullPath,strFileFullPath);
	

	int len;
	unsigned char buffer[1024];
	memset( buffer,0x00,sizeof(buffer));

	struct stat64 statbuf;
	int stat = lstat64(strFileFullPath,&statbuf); 
	double dFileSize = (double)statbuf.st_size;
	


	md5_state_t state;
	static unsigned char digest[16];
	static char szResult[40];
	memset(szResult,0x00,sizeof(szResult));
	memset( digest,0x00,sizeof(digest));
	
	FILE *pFile=fopen(pFileFullPath,"rb");
	
	if(!pFile) 
	{
		ZzLOG(ERROR, "%s 열기 실패 \n", pFileFullPath);
		return NULL;
	}
	
	md5_init(&state);
	
	uint64_t n64ReadLen = 0 ;
	long nReadFileSize = 10485760;
	bool bMoveSeek = false;

	if( dFileSize >  10485760 )
	{
		
		do 
		{
			memset( buffer,0x00,sizeof(buffer));
			if( nReadFileSize < 1024 )
			{
				len=fread(buffer,1,nReadFileSize,pFile);
				md5_append(&state,(const unsigned char *)buffer,len);
			}
			else
			{
				len=fread(buffer,1,1024,pFile);
				md5_append(&state,(const unsigned char *)buffer,len);			
			}
			
			n64ReadLen += 1024 ;
			nReadFileSize -= 1024;
		} while( nReadFileSize > 0 );
	}
	else
	{

		nReadFileSize = (long)dFileSize ; 
		do
		{
			memset( buffer,0x00,sizeof(buffer));
			if( nReadFileSize < 1024 )
			{
				len=fread(buffer,1,nReadFileSize,pFile);
				md5_append(&state,(const unsigned char *)buffer,len);
			}
			else
			{
				len=fread(buffer,1,1024,pFile);
				md5_append(&state,(const unsigned char *)buffer,len);			
			}
			n64ReadLen += 1024 ;
			nReadFileSize -= 1024;
		}while(nReadFileSize > 0);
	}
	md5_finish(&state, digest);
	fclose(pFile);
	

	
	sprintf(szResult,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
		,digest[0],digest[1],digest[2],digest[3],digest[4],digest[5],digest[6],digest[7]
		,digest[8],digest[9],digest[10],digest[11],digest[12],digest[13],digest[14],digest[15]);

	return szResult;
	
}


/*****************************************************************************
*  End of file...
*****************************************************************************/
