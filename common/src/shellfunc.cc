/*
	
    int nServerExit = 0;
    system(" /etc/init.d/heartbeat status | grep running |  awk '{print $4}'  > /root/heartbit_pid.txt");
    
    struct stat statbuf;
	int nStat = stat("/root/heartbit_pid.txt",&statbuf);
	
	if(nStat != 0) 
	{
		//error
	}
	else
	{
		nServerExit = (int)statbuf.st_size;
		if( !nServerExit) 
		{
			system("/etc/init.d/heartbeat start");
			printf("start heartbeat\r\n");
		}
		
	}
	
	return 1;
    
}
*/

int RunProcess(char* pPath , char* pName  ,char* pArgu)
{
	char szScript[1024];
	memset(szScript,0x00,sizeof(szScript));
	
	sprintf(szScript , "#! /bin/bash\r\n sleep 3\r\n  ps -e | grep %s |  awk \'{print $1}\'  > /tmp/%s.txt \r\n ");
	
}



/*****************************************************************************
* system()함수를 실행
* (I) 1.char *pSystemQuery :  system()의 인자값.
*
* (R) 1.성공시 0을 리턴.
*	  2.실패시 음수를 리턴.	 	 		
*****************************************************************************/
int RunSystem(char* pSystemQuery)
{
	int nError = 0;
	nError = system(pSystemQuery);
	
	if(nError == 127 || nError < 0)
	{
		ZzLOG(ERROR,"system()호출에 실패 했습니다. nError : [%d]", nError);
		return -1;
	}	
	return 0;
}

/*****************************************************************************
* crontab 등록
* (I) 1.char *pCommand : crontab의 등록, 삭제명령에 대한 정보.
*	  2.char *pProcessName : 실행할 프로세스의 이름.
*     3.char* pData : 실행할 프로세스의 패스.	
*	
* (R) NULL
*****************************************************************************/
int cron_control(char* pCommand, char *pProcessName , char* pData)
{
	
	char szData [512];	
	memset(szData,0x00,sizeof(szData));
	
	char szSystemQuery[512];
	memset(szSystemQuery, 0x00 ,sizeof(szSystemQuery));
	
	srand(time(NULL));
	
	int nMin = 0;
	int nHour = 0;
	
	nHour = rand()%4 + 6;
	nMin = rand()%60;

	//0-59/2 * * * * %s/check_server				
	if( strcmp(pCommand , "add" ) == 0 )
	{
		if( pData == NULL )
		{
//			strcpy( szData , "0-59/2 * * * * ");
//			strcat( szData , getenv("HOME"));
//			strcat( szData , "/check_server ");

		//cron_control("add","check_server","0-59/2 * * * * %s/check_server");
			
			//pData = "24 07 * * * /home/ezwon/zangsi/daemon/bin/ws439.sh";
		}
		else
		{
			sprintf(szData,"%d %d * * * ",nMin, nHour);
			strcat(szData, pData);
			strcat(szData, pProcessName);			
		}
		
		sprintf( szSystemQuery, "echo -e \"`crontab -l | grep -v %s | grep -v \"#\"` \n%s \" | crontab - ",pProcessName,szData);
		RunSystem(szSystemQuery);		
	}
	else if( strcmp(pCommand , "del" ) == 0 )
	{
		sprintf( szSystemQuery, "echo \"`crontab -l | grep -v %s | grep -v \"#\"`\" | crontab -" ,pProcessName);
		RunSystem(szSystemQuery);
	}
		
}