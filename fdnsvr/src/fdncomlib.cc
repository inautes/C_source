#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h> //opendir
#include <dirent.h> //opendir
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "fdncomlib.h"
#include "fdndefine.h"
#include "apdefine.h" // for log
#include "comcomm.h" // for log
#include "fdnmain.h"
#include "comhead.h"

#include "fdncomproc.h"
////////////////////////////////////////////////////////////////////////////////
// common lib
////////////////////////////////////////////////////////////////////////////////

/*---------------------------------------------------------------------------*/
// 192.168.0.1 중 1 의 값을 가져 온다.
/*---------------------------------------------------------------------------*/
/*
int GetLastIpNum(char* pSrcIP,char* pResult)
{
	if(strlen(pSrcIP) <= 0)
		return -1;

	char* pToken = strtok(pSrcIP,".");
	if(pToken == NULL)
		return -1;

	pToken = strtok(NULL,".");
	if(pToken == NULL)
		return -1;

	pToken = strtok(NULL,".");
	if(pToken == NULL)
		return -1;

	pToken = strtok(NULL,".");
	if(pToken == NULL)
		return -1;

	strcpyA(pResult,pToken);
	return 1;

}
*/

/*-----------------------------------------------------------------------*/
// 특수문자에 cReplace 값을 추가한다. (a/3/4 , '?', result) -> a?/3?/4
/*-----------------------------------------------------------------------*/
/*======================================================================
 int AppendSpecialChar(char* strSrcString, char cReplace,char* pResult)
 ======================================================================*/
char* AppendSpecialChar(char* strSrcString, char cReplace,char* pResult)
{

	char strResult[512];
	char szHangul[2];
	memset(strResult,0x00,512);

	int cTemp;
	int nSpecialCount=0;
	int nFileLen = strlen(strSrcString);

	for(int i=0; i<nFileLen ; i++)
	{
		cTemp = strSrcString[i];

		if( ((cTemp >= 32 && cTemp <= 47 ) || (cTemp >= 58 && cTemp <= 64)
		    || (cTemp >= 91 && cTemp <=96)|| (cTemp >= 123 && cTemp <=126))
		    )
		{
			strResult[nSpecialCount] = (char)cReplace;
			strResult[nSpecialCount+1] = (char)cTemp;
			nSpecialCount++;

		}
		else
		{
			if(cTemp<0)//한글
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

	memcpy(pResult , strResult,strlen(strResult));

	return pResult;
}


/*-----------------------------------------------------------------------*/
// nChar이 나타나는 처음 자리수를 얻는다. (1234,2) -> 1
/*-----------------------------------------------------------------------*/
/*======================================================================
 int GetfirstCharIndex(char* srsString, char nChar)
 return   : false (-1) : true ( 0이상의 숫자)
// srcString 에서 처음 nChar이 나타나는 곳의 자릿수를 return
 ======================================================================*/
int GetFistCharIndex(char* srcString,char nChar)
{
    #ifdef __DEBUG
    printf("GetFistCharIndex( %s , %c)",srcString,nChar);
    #endif

    int  nLen;

    nLen = 0;
    /*
    if( *(srcString + nLen )== nChar)
    	return 1;
   */
    while ( *(srcString + nLen) != nChar  )
    {
    	 nLen++;
    	 if(nLen > strlen(srcString))
    	 	return -1;
    }

    #ifdef __DEBUG
    printf("     return %d",nLen);
    #endif

    return nLen  ;
}


/*-----------------------------------------------------------------------*/
// nChar이 나타나는 처음 자리수를 얻는다. (1234,2) -> 1
/*-----------------------------------------------------------------------*/

/*======================================================================
 char* GetRightString(char* srcString,int nCount)
 return   : false (NULL) : true ( 문자열 반환)
 // srcString 의 오른쪽으로부터 nCount만큼 문자열을 반환
 ======================================================================*/
bool GetRightString(char* srcString,int nCount,char* resultString)
{
	if(strlen(srcString) > 612 || nCount <=0)
		return false;
	/*
	nCount = nCount - 1;

	memcpy(resultString,(srcString+ strlen(srcString) - nCount),nCount);
	*/
	memcpy(resultString,(srcString+ strlen(srcString) - nCount),nCount);

    #ifdef __DEBUG
    printf(" GetRightString(%s , %d , %s )",srcString , nCount , resultString);
    #endif
	return true;

}


/*****************************************************************************
* 같은 문자를 새로 replace 한다 .단 길이가 같은 문자로 변환
*****************************************************************************/
void StrReplace(char *source, char *search, char *change)
{
    char    *ptr;


    int     len = strlen(source);
    int     pos = 0;


    while( (ptr = strstr(source, search)) != NULL )
    {
        pos = len - strlen(ptr);
        memcpy((char*)&source[pos], change, strlen(change));
    }
}

/*****************************************************************************
* 한개의 단어를  replace 한다 .
*****************************************************************************/
void SetWordReplace(char *source, char *search, char *change , char* pResult)
{
    char    *ptr;

    int     len = strlen(source);
    int     pos = 0;
    int		change_len = strlen(change);

	if(len > 512) //over flow error
		return ;

    if( (ptr = strstr(source, search)) != NULL )
    {
    	pos = len - strlen(ptr);
    	memcpy(pResult,source,pos);
        memcpy(&pResult[pos],change,change_len);
        memcpy(&pResult[pos + change_len],&source[pos + strlen(search)] , len -pos + strlen(search) );
    }

}


/*------------------------------------------------------------------------------
** double GetFileSize(char* path)
** return   : double(file_size)
** 파일의 크기를 구한다.
**----------------------------------------------------------------------------*/
double GetFileSize(char* path)
{
	double dResu = 0.0;
	double dResult = 0.0;

	struct stat64 statbuf;
	DIR *pdir = NULL;

	int stat = stat64(path,&statbuf);

	if(stat !=0 )
	{
		#ifdef __DEBUG
		printf("   ../int stat = stat64(path,&statbuf) : %s\n",path);
		#endif

        infLOG(ERROR, "-> Exception ) GetFileSize Error ( stat !=0 )\n");
		return -1;
	}

	if(S_ISREG (statbuf.st_mode))
	{
		// path -> 파일...
		return ((double)statbuf.st_size); // 크기
	}

	// path -> 디렉토리...
	pdir = opendir(path);//strPath);
	if(pdir == NULL)
	{
		#ifdef __DEBUG
		printf("GetFileSize( %s )  pdir == NULL\n",path);
		#endif
        infLOG(ERROR, "-> Exception ) GetFileSize Error ( pdir == NULL )\n");

		return -1;
	}

	struct dirent *pent;
	char fullpath[612];

	while( (pent = readdir(pdir)) != NULL )
	{
		memset(fullpath,0x00,sizeof(fullpath));
		strcpy(fullpath,path);//strPath);
		strcat(fullpath , "/");
		strcat(fullpath, pent->d_name);


		stat = stat64(fullpath,&statbuf);

	 	if((strcmp(".",pent->d_name) !=0 ) && (strcmp("..",pent->d_name) != 0) && (GetFistCharIndex(pent->d_name,'.') != 0))
		{

			if(S_ISDIR (statbuf.st_mode))
			{
				dResu =GetFileSize( fullpath);
				if(dResu == -1)
				{
					#ifdef __DEBUG
					printf("GetFileSize : opendir 에러 2\n");
					#endif
					infLOG(ERROR, "GetFileSize : opendir 에러 2\n");

					closedir(pdir);


					return -1;
				}
				dResult = dResult + dResu;

			}
			else
			{
				if(S_ISREG (statbuf.st_mode))
				{
					dResult = dResult + (double)(double)statbuf.st_size; // 크기

				}
				else
				{

					#ifdef __DEBUG
					printf("GetFileSize : 해당 하는 파일 형태가 없음 \n");
					#endif
					infLOG(ERROR, "GetFileSize : 해당 하는 파일 형태가 없음 \n");

					closedir(pdir);

                    infLOG(ERROR, "-> Exception ) S_ISREG GetFileSize Error ( GetFileSize == -1 )\n");
					return -1;
				}
			}
		}
	}


	closedir(pdir);

	return dResult;
}



void DieWithError(char *errorMessage)
{

    printf(errorMessage);
    exit(1);
}
void GetErrMsg(int nErrno,char* szErrMsg)
{
	int nTempErrNo = nErrno;
	strcpy(szErrMsg,strerror(nTempErrNo));

}

int CheckUser()
{
	return 0;
}

