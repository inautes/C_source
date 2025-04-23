

////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h> 

#include "fupcomlib.h"
#include "fupdefine.h"

#include "apdefine.h" // for log
#include "comcomm.h" // for log

#include "fupmain.h"
/*======================================================================
 void ReplaceSingleToDouble(char* pString)
 ex> StrReplace("te'st")
 re> output is te"st
 ======================================================================*/


void ReplaceSingleToDouble(char* pString)
{
	int cTemp;
	int nFileLen = strlen(pString);
	
	int cReplace = '"';
	int cSingle = '\'';

	
	for(int i=0; i<nFileLen ; i++)
	{
		cTemp = pString[i];
		if( cTemp == cSingle ) // 96 is ' 
		{
			pString[i] = (char)cReplace;
							
		}
	}
}


/*======================================================================
 void StrReplace(char *source, char *search, char *change)
 ex> StrReplace("a1bbc",a1","1")
 re> source = "1bbc"
 ======================================================================*/
 
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

/*======================================================================
 int GetLastIpNum(char* pSrcIP,char* pResult)
 ex> GetLastIpNum(211.255.19.223  ,pResult) 
 re> pResult = 233
 ======================================================================*/
 
int GetLastIpNum(char* pSrcIP,char* pResult)
{
	char szTemp[16];
	memcpy(szTemp,pSrcIP,16);
	
	if(strlen(szTemp) <= 0)
		return -1;
	
	char* pToken = strtok(szTemp,".");
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
	
	strcpy(pResult,pToken);
	return 1;
	
}

/*======================================================================
 char* AppendSpecialChar(char* strSrcString, char cReplace,char* pResult)
 ex> AppendSpecialChar("hahah/a",'0',pResult)
 re> pResult = haha0/a
 * cReplace charecter append to special charecter 
 ======================================================================*/
 
char* AppendSpecialChar(char* strSrcString, char cReplace,char* pResult)
{

	char strResult[768];
	char szHangul[2];
	memset(strResult,0x00,sizeof(strResult));
	
	int cTemp;
	int nSpecialCount=0;
	int nFileLen = strlen(strSrcString);
	
	//printf("%d %s len \n",strlen(strSrcString),strSrcString);
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
	
	memcpy(pResult , strResult,strlen(strResult));

	return pResult;
}
	
	
/*======================================================================
 int GetFistCharIndex(char* srcString,char nChar)
 ex> GetFistCharIndex("hahah0a0",'0',)
 		       12345678
 re> 4
 * if char cant find that return to -1
 ======================================================================*/
 
int GetFistCharIndex(char* srcString,char nChar)
{

    int  nLen;
   
    nLen = 0;
    if( *(srcString + nLen )== nChar)
    	return 1;
  
    while ( *(srcString + nLen) != nChar  ) 
    {
    	 nLen++;
    	 if(nLen > strlen(srcString))
    	 	return -1;
    }
  
    return nLen;
}
/*======================================================================
 int GetReverseIndex(char* srcString,char nChar)
 ex> GetFistCharIndex("hahah0a0",'0',)
                       12345678
 re>  7
 * if char cant find that return to -1
 ======================================================================*/
 
int GetReverseIndex(char* srcString,char nChar)
{
    int  nLen,nCount,nStop;
   
    nLen = -1;
    nCount = 0;
    nStop = strlen(srcString);
   
   
    while  (nCount < nStop) // 문자열 끝은 0 값 즉, null 값입니다. 0이면 문자열 끝...
    {
    	if(*(srcString + nCount) == nChar)
    		nLen = nCount;
    	 nCount++;
    	 if(nCount > nStop)
    	 	break;
    }

    return nLen;
	
}


/*======================================================================
 char* GetLeftString(char* srcString,int nCount)
 return   : false (NULL) : true ( 문자열 반환)
 // srcString 의 왼쪽으로부터 nCount만큼 문자열을 반환
 ======================================================================*/

/*======================================================================
 bool GetLeftString(char* srcString,int nCount,char* resultString)
 ex> GetLeftString("jindogg",3,resultString)
                    1234567
 re>  jind
 * max len is 612 ,min len is 1 
 ======================================================================*/
 
bool GetLeftString(char* srcString,int nCount,char* resultString)
{
	
	if(strlen(srcString) > 4096 || nCount <=0)
		return false;
	//nCount = nCount + 1;	
	
	memcpy(resultString,srcString,nCount);
	
	
	return true;
	
}
/*======================================================================
 char* GetRightString(char* srcString,int nCount)
 return   : false (NULL) : true ( 문자열 반환)
 // srcString 의 오른쪽으로부터 nCount만큼 문자열을 반환
 ======================================================================*/
 
 
/*======================================================================
 bool GetRightString(char* srcString,int nCount,char* resultString)
 ex> GetRightString("jindogg",3,resultString)
                     6543210
 re>  ogg
 * max len is 612 ,min len is 1 
 ======================================================================*/
 
bool GetRightString(char* srcString,int nCount,char* resultString)
{
	
	if(strlen(srcString) > 612 || nCount <=0)
		return false;
		
	//nCount = nCount - 1;
	//memcpy(resultString,(srcString+ strlen(srcString) - nCount),strlen(srcString) - ( strlen(srcString) - nCount));//nCount);
	memcpy(resultString,(srcString+ strlen(srcString) - nCount),nCount);
	
	return true;
	
}


/*======================================================================
 int MakeFolder(char* szPath)
 return   : false (1) : true ( -1)
 // 폴더 생성( root = "./ss"일때 path를 "./ss/ss/a" 라고 하면 자동으로 생성
 ======================================================================*/
int MakeFolder(char* szPath)
{	
	char szPathTemp[768];
	memset(szPathTemp,0x00,sizeof(szPathTemp));
	strcpy(szPathTemp , szPath);	
	
	int stat;
	stat = mkdir(szPathTemp,S_IFDIR|S_IRWXU);

	if(stat != 0)  //0 is success -1 is error
	{
		if(errno == EACCES) // 이미 있는 디렉토리
			return -1;
		else //	if(errno == ENOENT)
		{
			char szNextPath[768];
			memset(szNextPath,0x00,sizeof(szNextPath));
			
			int nLen = GetReverseIndex(szPathTemp , '/');
			//get count 0~ max
			//nLen = nLen - 1; // '/' delete
			
			//get string 1~max
			if(GetLeftString(szPathTemp,nLen,szNextPath) == false)
			{
				return 1;
			}
			
			MakeFolder(szNextPath);
			stat = mkdir(szPathTemp,S_IFDIR|S_IRWXU);
			if(stat == 0)
				chmod( szPathTemp, 0755);
			//directory mode is R W X

		}
	}
	else
	{
		chmod( szPathTemp, 0755);
	}
   
	
	return 1;
	
}


int DeleteFile(int nType,char* path)
{
		

	char strSystemQuery[612];
	if(nType == FT_FILE)
	{

		#ifdef __DEBUG
	//			01234567890123456789]	
		printf("DeleteFile  		] \n");	
		#endif					

				
		#ifdef __DEBUG
		printf("DeleteFile  		] file : %s\n",path);
		#endif
		
		if(remove(path) != 0)
		{
			#ifdef __DEBUG
		//			01234567890123456789]				
			printf("DeleteFile  		] File Delete Failed ( %s ) \n",path);			
			#endif			
			infLOG(ERROR, "DeleteFile  		]  file delete failed ( %s ) \n",path);			
			//ENOENT : 파일이나 디렉토리를 찾을 수 없다.
			// EACCESS : 엑세스가 불가능하다.
			return -1;
		}
	}
	else if(nType == FT_FOLDER)
	{
		#ifdef __DEBUG
	//			01234567890123456789]				
		printf("DeleteFile  		] folder Delete ( %s ) \n",path);			
		#endif			


		if(strlen(path) < 34) // 위디스크 및 내디스크 폴더 최소 길이 검사
		{
			#ifdef __DEBUG
		//			01234567890123456789]				
			printf("DeleteFile  		] File Delete Failed ( %s ) Over Length \n",path);			
			#endif			
			infLOG(ERROR, "DeleteFile  		]  file delete failed ( %s ) Over Length\n",path);	
			return -1;
		}
		char szResult[768];
		
		memset(strSystemQuery,0x00,sizeof(strSystemQuery));
		strcpy(strSystemQuery,"rm -r -f ");		
		memset(szResult,0x00,sizeof(szResult));
		AppendSpecialChar(path,'\\',szResult);
		strcat(strSystemQuery,szResult);
		
		int stat = system(strSystemQuery);

		if( stat != 0) //err
		{
			#ifdef __DEBUG
		//			01234567890123456789]				
			printf("DeleteFile  		] Run Sytem Failed ( %s ) \n",strSystemQuery);			
			#endif			
			infLOG(ERROR, "DeleteFile  		]  Run Sytem Failed ( %s ) \n",strSystemQuery);	
							
			return -1;
		
		}
		
	}
	return 1;
}



void GetErrMsg(int nErrno,char* szErrMsg)
{
	int nTempErrNo = nErrno;
	strcpy(szErrMsg,strerror(nTempErrNo));	
	
			
}
/*
int CheckUser()
{
	struct  timeval stNow;
	struct  timezone stZone;
	struct  tm  stCtm;
	
	int nCurTime = 0;
	char szCurTime[256];
	char szTempTime[256];
	
	memset(szCurTime, 0x00, sizeof(szCurTime));
	memset(szTempTime, 0x00, sizeof(szTempTime));
	
	gettimeofday (&stNow, &stZone);
	localtime_r (&stNow.tv_sec, &stCtm);
		
	sprintf(szCurTime, "%02d", stCtm.tm_min);
	
	sprintf(szTempTime, "%02d:%02d:%02d", stCtm.tm_hour, stCtm.tm_min, stCtm.tm_sec);
    #ifdef __DEBUG
	printf("---------->time : %s\n", szCurTime);
	printf("---------->time : %s\n", szTempTime);
	#endif

	
//	if(strcmp(szCurTime,"00") == 0)
	{
		
		LPUSERLIST pData = new USERLIST[2000];
		memset(pData,0x00,sizeof(USERLIST)*2000);
	
	
		multimap<int,USERINFO>::iterator mi;
		mi = m_UserList.begin();
		
		int nCount =0;
		
		while(mi != m_UserList.end())
		{
			
			
			mi++;
			nCount++;
			if(nCount >= 1999)
			{
				infLOG(ERROR,"CheckUser Failed");	
				return -1;
			}
			
		}

		delete[] pData;
		
		CCOM9205_R com9205_r;
		
		memset(&com9205_r, 0x00, sizeof(CCOM9205_R));
		
//		sprintf(com9205_r.server_id, "%s", getenv("HOSTNAME"));
		strcpy(com9205_r.server_id, getenv("HOSTNAME"));
		com9205_r.nCount = nCount;
	    #ifdef __DEBUG
		printf("---------------->server_id = %s nCount = %d\n", com9205_r.server_id, com9205_r.nCount);
		#endif
	
		if(com9205(com9205_r) < 0)
		{
			infLOG(ERROR,"CheckUser Failed");	
			return -1;
		}
		
	}	
 
}
*/
