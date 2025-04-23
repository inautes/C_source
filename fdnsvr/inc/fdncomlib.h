#ifndef _FDN_COM_LIB_PROC
#define _FDN_COM_LIB_PROC


#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

int GetLastIpNum(char* pSrcIP,char* pResult);

char* AppendSpecialChar(char* strSrcString, char cReplace,char* pResult);
int GetFistCharIndex(char* srcString,char nChar);
bool GetRightString(char* srcString,int nCount,char* resultString);
void StrReplace(char *source, char *search, char *change);
double GetFileSize(char* path);
void SetWordReplace(char *source, char *search, char *change , char* pResult);


void DieWithError(char *errorMessage);

void GetErrMsg(int nErrno,char* szErrMsg);

#endif
