#include "Property.h"
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h> 
#include <sys/stat.h> 
#include <time.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <signal.h>

#define INI_DEST "property"

FILE *fp_property = 0;




Property::Property()
{
	char	szFileName[256];
	char	*pPath;

	
	if ( (pPath = getenv("CFG_DIR")) != NULL ) {
			sprintf(szFileName, "%s/%s.cfg", pPath,INI_DEST);
	} 
	else 
	{
		sprintf(szFileName, "%s.cfg", INI_DEST);
	}


	if(fp_property!=0)
	{
		fclose(fp_property);	
	}
	
	fp_property = fopen(szFileName, "r");

}

Property::~Property()
{
	if(fp_property!=0)
	{
		fclose(fp_property);	
		fp_property = 0;
	}
}
void Property::SetProcName(char *pProcName)
{ 
	
	char	szFileName[256];
	char	*pPath;

	if ( (pPath = getenv("CFG_DIR")) != NULL ) 
	{
		sprintf(szFileName, "%s/%s.cfg", pPath, pProcName);
	} 
	else 
	{
		sprintf(szFileName, "%s.cfg", pProcName);
	}
	
	if(fp_property!=0)
	{
		fclose(fp_property);	
	}

	
	fp_property = fopen(szFileName, "r");
	
	


}
bool Property::GetStrProperty(char *pszCate, char *pszString, char *pszDest)
{
	char *ptr;
	char szBuf[MAX_LENGTH] = {0,};
	char szTemp[MAX_LENGTH] = {0,};	
	int nStrlen = 0;
	int nTlen = 0;

	
	
	if(fp_property)
	{
		fseek(fp_property,0,0);
		
		while(fgets(szBuf,MAX_LENGTH,fp_property))
		{
			
			strReplace(szBuf," ","");
			strReplace(szBuf,"\r\n","");
			strReplace(szBuf,"\r","");			
			strReplace(szBuf,"\n","");
			strReplace(szBuf,"	","");

			ptr = strstr(szBuf, pszCate);
			
			if(ptr)
			{
				while(fgets(szBuf,MAX_LENGTH,fp_property))
				{
					
					strReplace(szBuf," ","");
					strReplace(szBuf,"\r","");
					strReplace(szBuf,"\n","");
					strReplace(szBuf,"\r\n","");
					strReplace(szBuf,"	","");
					ptr = strstr(szBuf, pszString);
					if(ptr)
					{
						//char *pCom	= strstr(szBuf,"#");
						//if(pCom > 0)
						//	pCom[0] = 0;
						
						nStrlen = strlen(pszString);

						nTlen = strlen(szBuf);					
						strMid(szTemp, szBuf, nStrlen + 1, nTlen - (nStrlen + 1));					
						strncpy(pszDest, szTemp, strlen(szTemp));					
						memset(szTemp, 0x00, sizeof(szTemp));
						break;
					}
				}
				break;				
			}			
		}

		return true;
	}
	return false;
}

bool Property::GetIntProperty(char *pszCate, char *pszString, int &nDest)
{
	char *ptr;
	char szBuf[MAX_LENGTH] = {0,};
	char szTemp[MAX_LENGTH] = {0,};	
	int nStrlen = 0;
	int nTlen = 0;
	
	if(fp_property)
	{
		fseek(fp_property,0,0);

		while(fgets(szBuf,MAX_LENGTH,fp_property))
		{
			strReplace(szBuf," ","");
			strReplace(szBuf,"\r\n","");
			strReplace(szBuf,"\r","");
			strReplace(szBuf,"\n","");		
			strReplace(szBuf,"	","");
					
			ptr = strstr(szBuf, pszCate);
			if(ptr)
			{
				while(fgets(szBuf,MAX_LENGTH,fp_property))
				{
					strReplace(szBuf," ","");
					strReplace(szBuf,"\r\n","");
					strReplace(szBuf,"\r","");
					strReplace(szBuf,"\n","");
					strReplace(szBuf,"	","");
		
					ptr = strstr(szBuf, pszString);
					if(ptr)
					{

						//char *pCom	= strstr(szBuf,"#");
						//if(pCom > 0)
						//	pCom[0] = 0;
						//

						nStrlen = strlen(pszString);
						nTlen = strlen(szBuf);					
						strMid(szTemp, szBuf, nStrlen + 1, nTlen - (nStrlen + 1));	
						
						nDest = atoi(szTemp);
						memset(szTemp, 0x00, sizeof(szTemp));
						break;
					}
				}
				break;				
			}			
		}

		return true;
	}
	return false;
}

void  Property::strReplace(char *source, char *search, char *change)
{
	char    *ptr;
		
	while(true)
	{
		char * pData = source; 
		
		int len = strlen(source);
		int pos = 0;
		
		if ((ptr = strstr(pData, search)) != 0 )
		{
			int nLen = (source - ptr) + strlen(source);
			int nMov =   strlen(search) - strlen(change); 
			memmove(ptr,ptr+nMov,nLen);
			memcpy(ptr, change, strlen(change));
			pData = ptr + strlen(change);
		}
		else
			break;
	}

}

int Property::strMid(char *tar, char *pData, int idx, int len)
{
	int slen = strlen(pData);
	int tlen = strlen(tar);
	if(slen < idx + len) return -1;
	if(len <= 0) return len;
	strncpy(tar, pData + idx, len);
	return len;
}
