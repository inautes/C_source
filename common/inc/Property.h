#define MAX_LENGTH 1024

class Property {

public:

	Property();
	virtual ~Property();
	bool GetStrProperty(char *pszCate, char *pszString, char *pszDest);
	bool GetIntProperty(char *pszCate, char *pszString, int &nDest);
	void strReplace(char *pszSource, char *pszSearch, char *pszChange);
	int strMid(char *pszTar, char *pData, int idx, int len);
	
	void SetProcName(char *pProcName);



};
//void strReplace(char *source, char *search, char *change);
//int strMid(char *tar, char *pData, int idx, int len);
//bool GetProperty(char *szCate, char *szString, char *Dest);
