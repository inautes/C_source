# ---------------------------------------------------------------------------
#	Directory Information
# ---------------------------------------------------------------------------
# common directory
COMMSRC = $(HOME)/project1/server/zangsi_with_dcmd/daemon/src
COMMOBJ = $(HOME)/project1/server/zangsi_with_dcmd/daemon/obj
COMMINC = $(HOME)/project1/server/zangsi_with_dcmd/daemon/inc

# daemon directory
DAEMBIN = $(HOME)/project1/server/zangsi_with_dcmd/daemon/rbin
DAEMSRC = $(HOME)/project1/server/zangsi_with_dcmd/daemon/src
DAEMOBJ = $(HOME)/project1/server/zangsi_with_dcmd/daemon/obj
DAEMINC = $(HOME)/project1/server/zangsi_with_dcmd/daemon/inc

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
DAEMINCS= -I. -I$(COMMINC) -I$(DAEMINC) -I$(MYDBINC)
DEBUGS	= -D_THREAD_MODULE_ -D_THREAD_SAFE -D_REENTRANT
CFLAGS  = $(DEBUGS)

# ---------------------------------------------------------------------------
#	Source List Information 
# ---------------------------------------------------------------------------
# common source 외부아이피
CC_COMMYDB  = commydb2.cc 
CC_DAEMCOM  = daemcom.cc
CC_PROPERTY = Property.cc

# source
CC_DAEMCYB = daembuzcyb.cc			#캐시현황 새로 넣기
CC_DAEMSFH = daemsetfiloghash.cc		#필로그 해시정보 디비입력
CC_DAEMSYNC = daemsync.cc			#차단리스트 분류별 동기화
CC_DAEMUDTINFO = daemudtinfo.cc			# 인스톨 서버의 로그 읽어와 업데이트 모듈 사용자 파악.


CC_DAEM5023 = daem5023.cc	#내디스크 삭제처리(현재 사용 중지)
CC_DAEM5110 = daem5110.cc       #탈퇴회원 정보 삭제(미완성)

# ---------------------------------------------------------------------------
# In Contents Server 
# ---------------------------------------------------------------------------
CC_DAEM8001 = daem8001.cc	#쓰레기 파일 찾기 ( 파일 기준 ) #FILE기준 쓰레기 파일 및 DB 삭제
CC_DAEM8003 = daem8003.cc	#쓰레기 파일 찾기 ( 파일 기준 ) #FILE기준 쓰레기 파일 및 DB 삭제
CC_DAEM8004 = daem8004.cc	#TEMP_DB기준 파일없는 자료찾기
CC_DAEM5003 = daem5003.cc	#컨텐츠 삭제처리
CC_DEL5003	= del5003.cc
#######################################################################
COM_OBJFILES = \
	$(COMMOBJ)/$(CC_COMMYDB:.cc=.o) \
	$(COMMOBJ)/$(CC_PROPERTY:.cc=.o) \
	$(COMMOBJ)/$(CC_DAEMCOM:.cc=.o)

DAEMSYNC = $(DAEMOBJ)/$(CC_DAEMSYNC:.cc=.o)
DAEMCYB = $(DAEMOBJ)/$(CC_DAEMCYB:.cc=.o)
DAEMSFH = $(DAEMOBJ)/$(CC_DAEMSFH:.cc=.o)
DAEMUDTINFO = $(DAEMOBJ)/$(CC_DAEMUDTINFO:.cc=.o)

DAEM5003 = $(DAEMOBJ)/$(CC_DAEM5003:.cc=.o)
DAEM8001 = $(DAEMOBJ)/$(CC_DAEM8001:.cc=.o)
DAEM8003 = $(DAEMOBJ)/$(CC_DAEM8003:.cc=.o)
DAEM8004 = $(DAEMOBJ)/$(CC_DAEM8004:.cc=.o)
DEL5003	 = $(DAEMOBJ)/$(CC_DEL5003:.cc=.o)
	
	
DAEM5023 = $(DAEMOBJ)/$(CC_DAEM5023:.cc=.o)
DAEM5110 = $(DAEMOBJ)/$(CC_DAEM5110:.cc=.o)

# ---------------------------------------------------------------------------
#	Generate Main Target Module
# ---------------------------------------------------------------------------
TSYNC    = $(DAEMBIN)/daemsync
TCYB    = $(DAEMBIN)/daembuzcyb
TSFH    = $(DAEMBIN)/daemsetfiloghash
TUDTINFO    = $(DAEMBIN)/daemudtinfo

T5003  = $(DAEMBIN)/daem5003
T8001  = $(DAEMBIN)/daem8001
T8003  = $(DAEMBIN)/daem8003
T8004  = $(DAEMBIN)/daem8004
T5003D = $(DAEMBIN)/del5003
T5023  = $(DAEMBIN)/daem5023
T5110  = $(DAEMBIN)/daem5110

all: daem8001 daem8003 daem8004 daem5003 del5003
# daemudtinfo daem5023 daem5110
     
     
# ---------------------------------------------------------------------------
#	Generate Each Target Module
# ---------------------------------------------------------------------------
daemsync:	$(TSYNC)
daemcyb:	$(TCYB)
daemsfh:	$(TSFH)
daemudtinfo:	$(TUDTINFO)
daem8001:	$(T8001)
daem8003:	$(T8003)
daem8004:	$(T8004)
daem5023:	$(T5023)
daem5110:	$(T5110)
daem5003:	$(T5003)
del5003:	$(T5003D)
	
$(T5003D)	: $(COM_OBJFILES) $(DEL5003)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DEL5003) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread
$(T5003)	: $(COM_OBJFILES) $(DAEM5003)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5003) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread
$(T8001)	: $(COM_OBJFILES) $(DAEM8001)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM8001) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread
$(T8003)	: $(COM_OBJFILES) $(DAEM8003)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM8003) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread
$(T8004)	: $(COM_OBJFILES) $(DAEM8004)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM8004) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread
$(TSYNC)	: $(COM_OBJFILES) $(DAEMSYNC)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEMSYNC) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	
$(TCYB)	: $(COM_OBJFILES) $(DAEMCYB)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEMCYB) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	
$(TSFH)	: $(COM_OBJFILES) $(DAEMSFH)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEMSFH) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread						
$(TUDTINFO)	: $(COM_OBJFILES) $(DAEMUDTINFO)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEMUDTINFO) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	

$(T5023)	: $(COM_OBJFILES) $(DAEM5023)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5023) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread
$(T5110)	: $(COM_OBJFILES) $(DAEM5110)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5110) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	

# ---------------------------------------------------------------------------
#	Generate Object Module
# ---------------------------------------------------------------------------
$(DAEM5003): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static

$(DEL5003): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static
$(DAEM8001): $(DAEMSRC)/$(@F:.o=.cc)
$(DAEM8001): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static
$(DAEM8003): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static
$(DAEM8004): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static	
$(DAEMSYNC): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static	
$(DAEMCYB): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static	
$(DAEMSFH): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static							
$(DAEMUDTINFO): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static							

$(DAEM5023): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static
$(DAEM5110): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static	

# ---------------------------------------------------------------------------
#	Common Object Module
# ---------------------------------------------------------------------------
$(COM_OBJFILES): $(COMMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(COMMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(COMMSRC)/$(@F:.o=.cc) -o $@ $(CFLAGS) $(COMMINCS) -L$(MYDBLIB) -static

# ---------------------------------------------------------------------------
#	Clean Objects Module
# ---------------------------------------------------------------------------
cleanall:
	@echo 'Remove All Object Files'
	@make -f real_contserver.mk cleandaem8001 cleandaem8003 cleandaem8004 cleandaemsync cleandaemcyb cleandaemsfh cleandaemudtinfo cleandaem5023 cleandaem5110 cleandaem5003 cleandel5003
cleandaem8001:
	@echo 'Remove Object Files(cleandaem8001)'
	@$(RM) $(T8001) $(DAEM8001) $(COM_OBJFILES)
cleandaem8003:
	@echo 'Remove Object Files(cleandaem8003)'
	@$(RM) $(T8003) $(DAEM8003) $(COM_OBJFILES)
cleandaem8004:
	@echo 'Remove Object Files(cleandaem8004)'
	@$(RM) $(T8004) $(DAEM8004) $(COM_OBJFILES)
cleandaemsync:
	@echo 'Remove Object Files(cleandaemsync)'
	@$(RM) $(TSYNC) $(DAEMSYNC) $(COM_OBJFILES)	
cleandaemcyb:
	@echo 'Remove Object Files(cleandaemcyb)'
	@$(RM) $(TCYB) $(DAEMCYB) $(COM_OBJFILES)	
cleandaemsfh:
	@echo 'Remove Object Files(cleandaemsfh)'
	@$(RM) $(TSFH) $(DAEMSFH) $(COM_OBJFILES)
cleandaemudtinfo:
	@echo 'Remove Object Files(cleandaemudtinfo)'
	@$(RM) $(TUDTINFO) $(DAEMUDTINFO) $(COM_OBJFILES)
cleandaem5023:
	@echo 'Remove Object Files(cleandaem5023)'
	@$(RM) $(T5023) $(DAEM5023) $(COM_OBJFILES)
cleandaem5003:
	@echo 'Remove Object Files(cleandaem5003)'
	@$(RM) $(T5003) $(DAEM5003) $(COM_OBJFILES)

cleandel5003:
	@echo 'Remove Object Files(cleandel5003)'
	@$(RM) $(T5003D) $(DEL5003) $(COM_OBJFILES)
cleandaem5110:
	cleandaem5110:
	@echo 'Remove Object Files(cleandaem5110)'
	@$(RM) $(T5110) $(DAEM5110) $(COM_OBJFILES)

# ===========================================================================
#	End of Makefile
# ===========================================================================
