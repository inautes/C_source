# ---------------------------------------------------------------------------
#	Directory Information
# ---------------------------------------------------------------------------
# common directory
COMMBIN = $(HOME)/project1/server/zangsi_with_dcmd/bin
COMMSRC = $(HOME)/project1/server/zangsi_with_dcmd/common/src
COMMOBJ = $(HOME)/project1/server/zangsi_with_dcmd/common/obj
COMMINC = $(HOME)/project1/server/zangsi_with_dcmd/common/inc

# cmdserver directory
CMDSSRC = $(HOME)/project1/server/zangsi_with_dcmd/cmdsvr/src
CMDSOBJ = $(HOME)/project1/server/zangsi_with_dcmd/cmdsvr/obj
CMDSINC = $(HOME)/project1/server/zangsi_with_dcmd/cmdsvr/inc

# dnmserver directory 다운로드 중첩 제거 
DNMSSRC = $(HOME)/project1/server/zangsi_with_dcmd/dnmsvr/src
DNMSOBJ = $(HOME)/project1/server/zangsi_with_dcmd/dnmsvr/obj
DNMSINC = $(HOME)/project1/server/zangsi_with_dcmd/dnmsvr/inc



# fdnserver directory
FDNSSRC = $(HOME)/project1/server/zangsi_with_dcmd/fdnsvr/src
FDNSOBJ = $(HOME)/project1/server/zangsi_with_dcmd/fdnsvr/obj
FDNSINC = $(HOME)/project1/server/zangsi_with_dcmd/fdnsvr/inc

# fupserver directory
FUPSSRC = $(HOME)/project1/server/zangsi_with_dcmd/fupsvr/src
FUPSOBJ = $(HOME)/project1/server/zangsi_with_dcmd/fupsvr/obj
FUPSINC = $(HOME)/project1/server/zangsi_with_dcmd/fupsvr/inc

# dcmdserver directory
DCMDSRC = $(HOME)/project1/server/zangsi_with_dcmd/dcmdsvr/src
DCMDOBJ = $(HOME)/project1/server/zangsi_with_dcmd/dcmdsvr/obj
DCMDINC = $(HOME)/project1/server/zangsi_with_dcmd/dcmdsvr/inc

# InstallServer directory
INSTSRC = $(HOME)/project1/server/zangsi_with_dcmd/installserver/src
INSTOBJ = $(HOME)/project1/server/zangsi_with_dcmd/installserver/obj
INSTINC = $(HOME)/project1/server/zangsi_with_dcmd/installserver/inc


# MySQL directory
MYDBINC  = $(HOME)/project1/server/zangsi_with_dcmd/mysql/include
MYDBLIB  = $(HOME)/project1/server/zangsi_with_dcmd/mysql/lib

# ---------------------------------------------------------------------------
#	Compiler Information & Copy Remove Options
# ---------------------------------------------------------------------------
CC      = g++
RM      = rm -f
SYSINC  = /usr/include
COMMINCS= -I. -I$(COMMINC) -I$(MYDBINC)
CMDSINCS= -I. -I$(COMMINC) -I$(CMDSINC) -I$(MYDBINC)
DNMSINCS= -I. -I$(COMMINC) -I$(DNMSINC) -I$(MYDBINC)
#MYDSINCS= -I. -I$(COMMINC) -I$(MYDSINC) -I$(MYDBINC)
FDNSINCS= -I. -I$(COMMINC) -I$(FDNSINC) -I$(MYDBINC)
FUPSINCS= -I. -I$(COMMINC) -I$(FUPSINC) -I$(MYDBINC)
DCMDINCS= -I. -I$(COMMINC) -I$(DCMDINC) -I$(MYDBINC)
INSTINCS= -I. -I$(COMMINC) -I$(INSTINC) -I$(MYDBINC)

DEBUGS	= -D_THREAD_MODULE_ -D_THREAD_SAFE -D_REENTRANT -D_DEBUG_ -D__DEBUG
#CFLAGS  = $(DEBUGS) 
CFLAGS  = $(DEBUGS) -O2 -W -g -Wno-write-strings

# ---------------------------------------------------------------------------
#	Source List Information 
# ---------------------------------------------------------------------------
# common source
CC_COMMYDB  = commydb.cc
CC_COMCONF  = comconf.cc
CC_COMTHRD  = comthrd.cc
CC_COMCOMM  = comcomm.cc
CC_COMSOCK  = comsock.cc	#com 소켓
CC_COM9001  = com9001.cc	#업로드시작
CC_COM9002  = com9002.cc	#다운로드 시작
CC_COM9003  = com9003.cc	#내디스크사용량UPDATE
CC_COM9004  = com9004.cc	#성인물 등록 제한 ( 10개 )
CC_COM9005  = com9005.cc	#다운로드시 컨텐츠 정보 조회
CC_COM9006  = com9006.cc	#다운로드시 정액제 정보 조회
CC_COM9009  = com9009.cc	#컨텐츠 서버 소켓 카운트
CC_COM9101  = com9101.cc	#업로드끝
CC_COM9102  = com9102.cc	#다운로드 끝
CC_COM9103  = com9103.cc	#내자료실사용량UPDATE
CC_COM9104  = com9104.cc	#Upload 오류처리(wedisk/mydata)
CC_COM9105  = com9105.cc	#Upload 서버 파일 정보 저장 (wedisk/mydata)
CC_COM9201  = com9201.cc	#다운로드 중복 접속 제거
CC_CMD5	    = cmd5.cc		#md5 hash code
CC_MYDB_C   = MysqlDB.cc	#mysql 사용 클래스
# cmdserver source
CC_CMDMAIN  = cmdmain.cc
CC_CMDPROC  = cmdproc.cc
CC_CMDS1001 = cmds1001.cc	#적정서버찾기
CC_CMDS1002 = cmds1002.cc	#컨텐츠등록현황
CC_CMDS1003 = cmds1003.cc	#친구목록조회
CC_CMDS1004 = cmds1004.cc	#로그인인증
CC_CMDS10041 = cmds10041.cc	#관리자 로그인인증
CC_CMDS1011 = cmds1011.cc	#다운로드자료삭제
CC_CMDS1012 = cmds1012.cc	#다운로드자료삭제
CC_CMDS1005 = cmds1005.cc   #Admin Server 목록 요청
CC_CMDS1101 = cmds1101.cc   #MY디스크서버찾기

# cmdserver source
CC_DNMMAIN  = dnmmain.cc
CC_DNMPROC  = dnmproc.cc


# mydserver source
#CC_MYDMAIN  = mydmain.cc
#CC_MYDPROC  = mydproc.cc
#CC_MYDS2001 = myds2001.cc	#무료30M신청
#CC_MYDS2002 = myds2002.cc	#내디스크남은용량조회
#CC_MYDS2003 = myds2003.cc	#내디스크남은용량조회
#CC_MYDS2004 = myds2004.cc	#내디스크사이버머니조회
#CC_MYDS2011 = myds2011.cc	#로그인인증

# fdnserver source
CC_FDNMAIN  = fdnmain.cc
CC_FDNPROC  = fdnproc.cc
CC_FDNSOCK    = fdnsock.cc
CC_FDNCOMLIB  = fdncomlib.cc
CC_FDNCOMPROC = fdncomproc.cc
CC_FDNDOWNPROC  = fdndownproc.cc
CC_FDNGURUPROC  = fdnguruproc.cc
CC_FDNS3004 = fdns3004.cc	#무료자료실 다운로드
CC_FDNUSERQUE = fdnuserqueue.cc #사용자 관리 큐
#CC_FDNS3001 = fdns3001.cc	#사이버머니조회
#CC_FDNS3002 = fdns3002.cc	#파일다운로드시작
#CC_FDNS3003 = fdns3003.cc	#파일다운로드종료(사이버머니계산)
#CC_FDNS3011 = fdns3011.cc	#특정사용자의 공유폴더가격조회

# fupserver source
CC_FUPMAIN  = fupmain.cc
CC_FUPPROC  = fupproc.cc
CC_FUPSOCK  = fupsock.cc
CC_FUPCOMLIB  = fupcomlib.cc
CC_FUPCOMPROC = fupcomproc.cc
CC_FUPWEPROC  = fupweproc.cc
CC_FUPMYPROC  = fupmyproc.cc
CC_FUPGUPROC  = fupguruproc.cc
CC_FUPS4001 = fups4001.cc	#컨테츠등록
CC_FUPS4002 = fups4002.cc	#무료자료실 파일등록
CC_FUPS4003 = fups4003.cc	#내자료실 등록 (구조체는 fups4001 사용)
CC_FUPS4005 = fups4005.cc	#해쉬 관련 
CC_FUPS4006 = fups4006.cc	#유료화 관련 
CC_FUPSFLOG4005 = fupsflog4005.cc	#필로그 저작권
CC_FUPSFLOG4006 = fupsflog4006.cc	#필로그 제휴

# dcmdserver source

CC_DCMDMAIN  = dcmdmain.cc
CC_DCMDPROC  = dcmdproc.cc
CC_DCMDSOCK  = dcmdsock.cc
CC_DCMDCOMLIB = dcmdcomlib.cc
CC_DCMDFDNS3004 = dcmdfdns3004.cc
CC_DCMDFUPS4001 = dcmdfups4001.cc
CC_DCMDFUPS4002 = dcmdfups4002.cc
CC_DCMDFUPS4003 = dcmdfups4003.cc
CC_DCMDFUPS4005 = dcmdfups4005.cc
CC_DCMDFUPS4006 = dcmdfups4006.cc
CC_DCMDFUPS40051 = dcmdfups40051.cc #필로그 저작권
CC_DCMDFUPS40061 = dcmdfups40061.cc #필로그 제휴
CC_DCMD9001  = dcmd9001.cc	#업로드 시작
CC_DCMD9002  = dcmd9002.cc	#다운로드 시작
CC_DCMD9004  = dcmd9004.cc	#성인물 등록 제한 ( 10개 )
CC_DCMD9005  = dcmd9005.cc	#다운로드시 컨텐츠 정보 조회
CC_DCMD9006  = dcmd9006.cc	#다운로드시 정액제 정보 조회
CC_DCMD9009  = dcmd9009.cc	#컨텐츠 서버 소켓수
CC_DCMD9101  = dcmd9101.cc	#업로드 끝
CC_DCMD9102  = dcmd9102.cc	#다운로드 끝
CC_DCMD9103  = dcmd9103.cc	#내자료실사용량UPDATE
CC_DCMD9104  = dcmd9104.cc	#Upload 오류처리(wedisk/mydata)
CC_DCMD9105  = dcmd9105.cc	#Upload 서버 파일 정보 저장 (wedisk/mydata)
CC_DCMD9201  = dcmd9201.cc	#다운로드 중복 접속 종료
CC_DBQUEUE  = CDBQueue.cc		#QUEUE
CC_SCHQUEUE  = CSchQueue.cc		#QUEUE
CC_MYSQL_POOL  = mysql_pool.cc	#MYSQL POOL
CC_LOG64  = CLog64.cc


# InstallServer source
CC_INSTMAIN  = installmain.cc
CC_INSTPROC  = installproc.cc
CC_INSTSOCK    = installsock.cc
CC_INSTCOMLIB  = installcomlib.cc
CC_INSTGURUPROC  = installsendproc.cc


#######################################################################
COM_OBJFILES = \
	$(COMMOBJ)/$(CC_COMMYDB:.cc=.o) \
	$(COMMOBJ)/$(CC_COMCONF:.cc=.o) \
	$(COMMOBJ)/$(CC_COMTHRD:.cc=.o) \
	$(COMMOBJ)/$(CC_COMCOMM:.cc=.o) \
	$(COMMOBJ)/$(CC_COMSOCK:.cc=.o) \
	$(COMMOBJ)/$(CC_COM9001:.cc=.o) \
	$(COMMOBJ)/$(CC_COM9002:.cc=.o) \
	$(COMMOBJ)/$(CC_COM9003:.cc=.o) \
	$(COMMOBJ)/$(CC_COM9004:.cc=.o) \
	$(COMMOBJ)/$(CC_COM9005:.cc=.o) \
	$(COMMOBJ)/$(CC_COM9006:.cc=.o) \
	$(COMMOBJ)/$(CC_COM9009:.cc=.o) \
	$(COMMOBJ)/$(CC_COM9101:.cc=.o) \
	$(COMMOBJ)/$(CC_COM9102:.cc=.o) \
	$(COMMOBJ)/$(CC_COM9103:.cc=.o) \
	$(COMMOBJ)/$(CC_COM9104:.cc=.o) \
	$(COMMOBJ)/$(CC_COM9105:.cc=.o)	\
	$(COMMOBJ)/$(CC_COM9201:.cc=.o)	\
	$(COMMOBJ)/$(CC_MYDB_C:.cc=.o)	\
	$(COMMOBJ)/$(CC_CMD5:.cc=.o)
	
CMD_OBJFILES = \
	$(CMDSOBJ)/$(CC_CMDMAIN:.cc=.o) \
	$(CMDSOBJ)/$(CC_CMDPROC:.cc=.o) \
	$(CMDSOBJ)/$(CC_CMDS1001:.cc=.o) \
	$(CMDSOBJ)/$(CC_CMDS1002:.cc=.o) \
	$(CMDSOBJ)/$(CC_CMDS1003:.cc=.o) \
	$(CMDSOBJ)/$(CC_CMDS1004:.cc=.o) \
	$(CMDSOBJ)/$(CC_CMDS10041:.cc=.o) \
	$(CMDSOBJ)/$(CC_CMDS1011:.cc=.o) \
	$(CMDSOBJ)/$(CC_CMDS1012:.cc=.o) \
	$(CMDSOBJ)/$(CC_CMDS1005:.cc=.o) \
	$(CMDSOBJ)/$(CC_CMDS1101:.cc=.o)
	
DNM_OBJFILES = \
	$(DNMSOBJ)/$(CC_DNMMAIN:.cc=.o) \
	$(DNMSOBJ)/$(CC_DNMPROC:.cc=.o)
	
#MYD_OBJFILES = \
#	$(MYDSOBJ)/$(CC_MYDMAIN:.cc=.o) \
#	$(MYDSOBJ)/$(CC_MYDPROC:.cc=.o) \
#	$(MYDSOBJ)/$(CC_MYDS2001:.cc=.o) \
#	$(MYDSOBJ)/$(CC_MYDS2002:.cc=.o) \
#	$(MYDSOBJ)/$(CC_MYDS2003:.cc=.o) \
#	$(MYDSOBJ)/$(CC_MYDS2004:.cc=.o) \
#	$(MYDSOBJ)/$(CC_MYDS2011:.cc=.o)

FDN_OBJFILES = \
	$(FDNSOBJ)/$(CC_FDNMAIN:.cc=.o) \
	$(FDNSOBJ)/$(CC_FDNPROC:.cc=.o) \
	$(FDNSOBJ)/$(CC_FDNSOCK:.cc=.o) \
	$(FDNSOBJ)/$(CC_FDNCOMLIB:.cc=.o) \
	$(FDNSOBJ)/$(CC_FDNCOMPROC:.cc=.o)\
	$(FDNSOBJ)/$(CC_FDNDOWNPROC:.cc=.o) \
	$(FDNSOBJ)/$(CC_FDNGURUPROC:.cc=.o)\
	$(FDNSOBJ)/$(CC_FDNS3004:.cc=.o)\
	$(FDNSOBJ)/$(CC_FDNUSERQUE:.cc=.o)
#	$(FDNSOBJ)/$(CC_FDNS3001:.cc=.o) \
#	$(FDNSOBJ)/$(CC_FDNS3002:.cc=.o) \
#	$(FDNSOBJ)/$(CC_FDNS3003:.cc=.o) \
#	$(FDNSOBJ)/$(CC_FDNS3011:.cc=.o)

FUP_OBJFILES = \
	$(FUPSOBJ)/$(CC_FUPMAIN:.cc=.o) \
	$(FUPSOBJ)/$(CC_FUPPROC:.cc=.o) \
	$(FUPSOBJ)/$(CC_FUPSOCK:.cc=.o) \
	$(FUPSOBJ)/$(CC_FUPCOMLIB:.cc=.o) \
	$(FUPSOBJ)/$(CC_FUPCOMPROC:.cc=.o)\
	$(FUPSOBJ)/$(CC_FUPWEPROC:.cc=.o) \
	$(FUPSOBJ)/$(CC_FUPMYPROC:.cc=.o)\
	$(FUPSOBJ)/$(CC_FUPGUPROC:.cc=.o)\
	$(FUPSOBJ)/$(CC_FUPS4001:.cc=.o)  \
	$(FUPSOBJ)/$(CC_FUPS4002:.cc=.o)  \
	$(FUPSOBJ)/$(CC_FUPS4003:.cc=.o) \
	$(FUPSOBJ)/$(CC_FUPS4005:.cc=.o) \
	$(FUPSOBJ)/$(CC_FUPS4006:.cc=.o) \
	$(FUPSOBJ)/$(CC_FUPSFLOG4005:.cc=.o) \
	$(FUPSOBJ)/$(CC_FUPSFLOG4006:.cc=.o) 
	

DCMD_OBJFILES = \
	$(DCMDOBJ)/$(CC_MYSQL_POOL:.cc=.o) \
	$(DCMDOBJ)/$(CC_LOG64:.cc=.o) \
	$(DCMDOBJ)/$(CC_DBQUEUE:.cc=.o) \
	$(DCMDOBJ)/$(CC_SCHQUEUE:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMDMAIN:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMDPROC:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMDSOCK:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMDCOMLIB:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMDFDNS3004:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMDFUPS4001:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMDFUPS4002:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMDFUPS4003:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMDFUPS4005:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMDFUPS4006:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMDFUPS40051:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMDFUPS40061:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMD9001:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMD9002:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMD9004:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMD9005:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMD9006:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMD9009:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMD9101:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMD9102:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMD9103:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMD9104:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMD9105:.cc=.o) \
	$(DCMDOBJ)/$(CC_DCMD9201:.cc=.o) 

INST_OBJFILES = \
	$(INSTOBJ)/$(CC_INSTMAIN:.cc=.o) \
	$(INSTOBJ)/$(CC_INSTPROC:.cc=.o) \
	$(INSTOBJ)/$(CC_INSTSOCK:.cc=.o) \
	$(INSTOBJ)/$(CC_INSTCOMLIB:.cc=.o) \
	$(INSTOBJ)/$(CC_INSTGURUPROC:.cc=.o) 

	
# ---------------------------------------------------------------------------
#	Generate Main Target Module
# ---------------------------------------------------------------------------
TARGET1 = $(COMMBIN)/cmdserver
#TARGET2 = $(COMMBIN)/mydserver
TARGET3 = $(COMMBIN)/fdnserver
TARGET4 = $(COMMBIN)/fupserver
TARGET5 = $(COMMBIN)/dcmdserver
TARGET6 = $(COMMBIN)/dnmserver
TARGET7 = $(COMMBIN)/installserver

TARGET8 = $(COMMBIN)/fdnserver_d

#all: cmdserver mydserver fupserver fdnserver dcmdserver installserver
all: cmdserver fupserver fdnserver dcmdserver installserver

# ---------------------------------------------------------------------------
#	Generate Each Target Module
# ---------------------------------------------------------------------------
cmdserver:	$(TARGET1)
#mydserver:	$(TARGET2)
fdnserver:	$(TARGET3)
fupserver:	$(TARGET4)
dcmdserver:	$(TARGET5)
dnmserver:	$(TARGET6)
installserver : $(TARGET7)

# Include DEBUG infomation

fdnserver_d:	$(TARGET8)

$(TARGET1)	: $(COM_OBJFILES) $(CMD_OBJFILES)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -O2 -o $@ $(CMD_OBJFILES) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -static -lcrypt  -lpthread

#$(TARGET2)	: $(COM_OBJFILES) $(MYD_OBJFILES)
#	@echo '=================================================================='
#	@echo 'Create server... :' $@
#	@echo '=================================================================='
#	@$(CC) -O2 -o $@ $(MYD_OBJFILES) $(COM_OBJFILES) $(DEBUGS) $(MYDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -static -lcrypt  -lpthread

$(TARGET3)	: $(COM_OBJFILES) $(FDN_OBJFILES)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -O2 -o $@ $(FDN_OBJFILES) $(COM_OBJFILES) $(DEBUGS) $(FDNSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -static -lcrypt  -lpthread

$(TARGET4)	: $(COM_OBJFILES) $(FUP_OBJFILES)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -O2 -o $@ $(FUP_OBJFILES) $(COM_OBJFILES) $(DEBUGS) $(FUPSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -static -lcrypt  -lpthread

$(TARGET5)	: $(COM_OBJFILES) $(DCMD_OBJFILES)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -O2 -o $@ $(DCMD_OBJFILES) $(COM_OBJFILES) $(DEBUGS) $(DCMDINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -static -lcrypt  -lpthread

$(TARGET6)	: $(COM_OBJFILES) $(DNM_OBJFILES)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -O2 -o $@ $(DNM_OBJFILES) $(COM_OBJFILES) $(DEBUGS) $(DNMSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -static -lcrypt  -lpthread
$(TARGET7)	: $(COM_OBJFILES) $(INST_OBJFILES)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -O2  -o $@ $(INST_OBJFILES) $(COM_OBJFILES) $(DEBUGS) $(INSTINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -static -lcrypt  -lpthread

$(TARGET8)	: $(COM_OBJFILES) $(FDN_OBJFILES)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -O2 -g -o $@ $(FDN_OBJFILES) $(COM_OBJFILES) $(DEBUGS) $(FDNSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -static -lcrypt  -lpthread

# ---------------------------------------------------------------------------
#	Generate Object Module
# ---------------------------------------------------------------------------
$(CMD_OBJFILES): $(CMDSSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(CMDSSRC)/$(@F:.o=.cc)
	@$(CC) -O2 -c $(CMDSSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -static


$(FDN_OBJFILES): $(FDNSSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(FDNSSRC)/$(@F:.o=.cc)
	@$(CC) -O2 -c $(FDNSSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(FDNSINCS) -L$(MYDBLIB) -static

$(FUP_OBJFILES): $(FUPSSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(FUPSSRC)/$(@F:.o=.cc)
	@$(CC) -O2 -c $(FUPSSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(FUPSINCS) -L$(MYDBLIB) -static

$(DCMD_OBJFILES): $(DCMDSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DCMDSRC)/$(@F:.o=.cc)
	@$(CC) -O2 -c $(DCMDSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DCMDINCS) -L$(MYDBLIB) -static

$(DNM_OBJFILES): $(DNMSSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DNMSSRC)/$(@F:.o=.cc)
	@$(CC) -O2 -c $(DNMSSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DNMSINCS) -L$(MYDBLIB) -static
$(INST_OBJFILES): $(INSTSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(INSTSRC)/$(@F:.o=.cc)
	@$(CC)  -O2 -c $(INSTSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(INSTINCS) -L$(MYDBLIB)  -static

# ---------------------------------------------------------------------------
#	Common Object Module
# ---------------------------------------------------------------------------
$(COM_OBJFILES): $(COMMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(COMMSRC)/$(@F:.o=.cc)
	@$(CC) -O2 -c $(COMMSRC)/$(@F:.o=.cc) -o $@ $(CFLAGS) $(COMMINCS) -L$(MYDBLIB) -static

# ---------------------------------------------------------------------------
#	Clean Objects Module
# ---------------------------------------------------------------------------
cleanall:
	@echo 'Remove All Object Files'
	@make  -f real.mk cleancmdserver cleanfdnserver cleanfupserver cleandcmdserver cleaninstallserver

cleancmdserver:
	@echo 'Remove Object Files(cleancmdserver)'
	@$(RM) $(TARGET1) $(CMD_OBJFILES) $(COM_OBJFILES)

cleanfdnserver:
	@echo 'Remove Object Files(cleanfdnserver)'
	@$(RM) $(TARGET3) $(FDN_OBJFILES) $(COM_OBJFILES)

cleanfupserver:
	@echo 'Remove Object Files(cleanfupserver)'
	@$(RM) $(TARGET4) $(FUP_OBJFILES) $(COM_OBJFILES)

cleandcmdserver:
	@echo 'Remove Object Files(cleandcmdserver)'
	@$(RM) $(TARGET5) $(DCMD_OBJFILES) $(COM_OBJFILES)

cleandnmserver:
	@echo 'Remove Object Files(cleandnmserver)'
	@$(RM) $(TARGET6) $(DNM_OBJFILES) $(COM_OBJFILES)
cleaninstallserver:
	@echo 'Remove Object Files(cleaninstallserver)'
	@$(RM) $(TARGET7) $(INST_OBJFILES) $(COM_OBJFILES)

# ===========================================================================
#	End of Makefile
# ===========================================================================
