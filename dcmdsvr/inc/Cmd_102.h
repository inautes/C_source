// Cmd_102.h: interface for the CCmd_102 class.
// 데이터 준비 .. 전송 정보를 제공 및 src 파일 읽을 위치 파악
//////////////////////////////////////////////////////////////////////
#ifndef _CMD_102_
#define _CMD_102_


#include "CCommand.h"

class CCmd_102 : public CCommand  
{
public:
	
	
	char*	getTimeString();
	
	void	setStatus(int nStatus);
	int		getStatus();
	
	void	setContentsID(unsigned long ulID);
	unsigned long getContentsID();
	
	void	setSrcName(char* pName);
	char*	getSrcName();

	void	setDestName(char* pName);
	char*	getDestName();

	void	setSrcIp(char* pIp);
	char*	getSrcIp();

	void	setDestIp(char* pIp);
	char*	getDestIp();

	void	setDestPort(int nPort);
	int		getDestPort();

	
	char*	getData();
	void	setData(char* pData);
	
	void	printData();
		
	int		getDataLen();
	
	void	InitVariable();
	CCmd_102();
	virtual ~CCmd_102();
	
private:

	
	typedef struct _tagDATA
	{
		unsigned long ulContID;
		
		int		nStatus;
		
		char	szSrcName[8];
		char	szSrcIP[16];
		char	szDestName[8];
		char	szDestIP[16];
		int		nDestPort;
		
	} structDATA, *LPstructDATA;
	
	LPstructDATA m_lpData;
	
private:
	
};

#endif // !defined(AFX_CMD_102_H__CAE39A6F_BEE5_49A0_B5F2_93F360F15522__INCLUDED_)

