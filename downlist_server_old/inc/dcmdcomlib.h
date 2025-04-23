#ifndef _DCMD_COM_LIB_PROC
#define _DCMD_COM_LIB_PROC


#ifndef _FILE_OFFSET_BITS 
#define _FILE_OFFSET_BITS 64
#endif

int MakeFolder(char* szPath) ;
void StrReplace(char *source, char *search, char *change);
bool GetRightString(char* srcString,int nCount,char* resultString) ;
bool GetLeftString(char* srcString,int nCount,char* resultString) ;
int GetReverseIndex(char* srcString,char nChar) ;
int GetFistCharIndex(char* srcString,char nChar);
char* AppendSpecialChar(char* strSrcString, char cReplace,char* pResult) ;
int GetLastIpNum(char* pSrcIP,char* pResult);
void ReplaceSingleToDouble(char* pString);

#endif
