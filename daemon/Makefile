# ---------------------------------------------------------------------------
#	Directory Information
# ---------------------------------------------------------------------------
# common directory
COMMSRC = $(HOME)/project1/server/zangsi_with_dcmd/daemon/src
COMMOBJ = $(HOME)/project1/server/zangsi_with_dcmd/daemon/obj
COMMINC = $(HOME)/project1/server/zangsi_with_dcmd/daemon/inc

# daemon directory
DAEMBIN = $(HOME)/project1/server/zangsi_with_dcmd/daemon/bin
DAEMSRC = $(HOME)/project1/server/zangsi_with_dcmd/daemon/src
DAEMOBJ = $(HOME)/project1/server/zangsi_with_dcmd/daemon/obj
DAEMINC = $(HOME)/project1/server/zangsi_with_dcmd/daemon/inc

# MySQL directory
MYDBINC = $(HOME)/project1/server/zangsi_with_dcmd/mysql/include
MYDBLIB = $(HOME)/project1/server/zangsi_with_dcmd/mysql/lib

# ---------------------------------------------------------------------------
#	Compiler Information & Copy Remove Options
# ---------------------------------------------------------------------------
CC      = g++
RM      = rm -f
SYSINC  = /usr/include
COMMINCS= -I. -I$(COMMINC) -I$(MYDBINC)
DAEMINCS= -I. -I$(COMMINC) -I$(DAEMINC) -I$(MYDBINC)
DEBUGS	= -D_THREAD_MODULE_ -D_THREAD_SAFE -D_REENTRANT -D__DEBUG 
CFLAGS  = $(DEBUGS)

# ---------------------------------------------------------------------------
#	Source List Information 
# ---------------------------------------------------------------------------
# common source
CC_COMMYDB  = commydb.cc
CC_DAEMCOM  = daemcom.cc

# ---------------------------------------------------------------------------
# In Contents Server 
# ---------------------------------------------------------------------------
CC_DAEM5003 = daem5003.cc	#컨텐츠 삭제처리

CC_DAEM8001 = daem8001.cc	#쓰레기 파일 찾기 ( 파일 기준 ) #FILE기준 쓰레기 파일 및 DB 삭제
CC_DAEM8004 = daem8004.cc	#TEMP_DB기준 파일없는 자료찾기

# ---------------------------------------------------------------------------
#  In CMD01 
# ---------------------------------------------------------------------------
CC_DAEM5002 = daem5002.cc	#컨텐츠 연장처리
CC_DAEM5004 = daem5004.cc	#삭제된 컨텐츠 정보삭제
CC_DAEM5006 = daem5006.cc	#vir_id2 정리
CC_DAEM5007 = daem5007.cc	#컨텐츠별 다운진행수를 0으로 셋

CC_DAEM5021 = daem5021.cc	#내디스크용량계산
CC_DAEM5023 = daem5023.cc	#내디스크삭제처리
CC_DAEM5025 = daem5025.cc	#요청자료 정리 데몬

CC_DAEM5121 = daem5121.cc	#총 출금 건수, 총 출금액, 한달간 총 출금액 생성
CC_DAEM5122 = daem5122.cc       #관리자 상담글 통계

CC_DAEM5403 = daem5403.cc	#관리자 상담글 집계 처리

CC_DAEM5501 = daem5501.cc	#현재 날짜로부터 30일전의 받은 쪽지 삭제
CC_DAEM5502 = daem5502.cc	#현재 날짜로부터 30일전의 보낸 쪽지 삭제


# ---------------------------------------------------------------------------
#  In BKS01 
# ---------------------------------------------------------------------------
CC_DAEM5011 = daem5011.cc	#수수료계산

CC_DAEM5024 = daem5024.cc	#판매자격취소자들의 컨텐츠를 취소일로부터 1주일뒤 삭제

CC_DAEM5101 = daem5101.cc	#회원가입현황
CC_DAEM5103 = daem5103.cc	#분류별판매집계
CC_DAEM51031 = daem51031.cc	#무료다운로드 집계
CC_DAEM5106 = daem5106.cc	#사용자실적분석
CC_DAEM5107 = daem5107.cc	#업로드 권한 정보 추출
CC_DAEM5109 = daem5109.cc       #웹,채팅,다운,업로그 사용자수 계산

CC_DAEM5110 = daem5110.cc       #탈퇴회원 정보 삭제
CC_DAEM5112 = daem5112.cc       #메일링 회원 분류별 다운로드 집계

CC_DAEM5123 = daem5123.cc       #리콜 집계
CC_DAEM5124 = daem5124.cc       #리콜 처리

CC_DAEM5505 = daem5505.cc	#일간 평균 로그인 수 집계

CC_DAEM5901 = daem5901.cc	#크래킹 할려는 사용자 찾기

CC_DAEM9001 = daem9001.cc	# 추석 이벤트
CC_DAEM9002 = daem9002.cc	# 추석 이벤트
CC_DAEM9003 = daem9003.cc	# top100

CC_DAEMADMINOUT = daemadminout.cc	# 관리운영 통계

# ---------------------------------------------------------------------------
#  In SCH01  
# ---------------------------------------------------------------------------
CC_DAEMSCHSTATS = daemschstats.cc	# 검색어 집계

# ---------------------------------------------------------------------------
#  no cron
# ---------------------------------------------------------------------------
# source
CC_DAEMCYB = daembuzcyb.cc		#캐시현황 새로 넣기
CC_DAEMSFH = daemsetfiloghash.cc		#필로그 해시정보 디비입력
CC_DAEMSYNC = daemsync.cc	#차단리스트 분류별 동기화
CC_DAEMUDTINFO = daemudtinfo.cc	# 인스톨 서버의 로그 읽어와 업데이트 모듈 사용자 파악.
#######################################################################
COM_OBJFILES = \
	$(COMMOBJ)/$(CC_COMMYDB:.cc=.o) \
	$(COMMOBJ)/$(CC_DAEMCOM:.cc=.o)

DAEM5002 = $(DAEMOBJ)/$(CC_DAEM5002:.cc=.o)
DAEM5003 = $(DAEMOBJ)/$(CC_DAEM5003:.cc=.o)
DAEM5004 = $(DAEMOBJ)/$(CC_DAEM5004:.cc=.o)
DAEM5006 = $(DAEMOBJ)/$(CC_DAEM5006:.cc=.o)
DAEM5007 = $(DAEMOBJ)/$(CC_DAEM5007:.cc=.o)

DAEM5011 = $(DAEMOBJ)/$(CC_DAEM5011:.cc=.o)

DAEM5021 = $(DAEMOBJ)/$(CC_DAEM5021:.cc=.o)
DAEM5023 = $(DAEMOBJ)/$(CC_DAEM5023:.cc=.o)
DAEM5024 = $(DAEMOBJ)/$(CC_DAEM5024:.cc=.o)
DAEM5025 = $(DAEMOBJ)/$(CC_DAEM5025:.cc=.o)

DAEM5101 = $(DAEMOBJ)/$(CC_DAEM5101:.cc=.o)
DAEM5103 = $(DAEMOBJ)/$(CC_DAEM5103:.cc=.o)
DAEM51031 = $(DAEMOBJ)/$(CC_DAEM51031:.cc=.o)
DAEM5106 = $(DAEMOBJ)/$(CC_DAEM5106:.cc=.o)
DAEM5107 = $(DAEMOBJ)/$(CC_DAEM5107:.cc=.o)
DAEM5109 = $(DAEMOBJ)/$(CC_DAEM5109:.cc=.o)

DAEM5110 = $(DAEMOBJ)/$(CC_DAEM5110:.cc=.o)
DAEM5112 = $(DAEMOBJ)/$(CC_DAEM5112:.cc=.o)

DAEM5121 = $(DAEMOBJ)/$(CC_DAEM5121:.cc=.o)
DAEM5122 = $(DAEMOBJ)/$(CC_DAEM5122:.cc=.o)
DAEM5123 = $(DAEMOBJ)/$(CC_DAEM5123:.cc=.o)
DAEM5124 = $(DAEMOBJ)/$(CC_DAEM5124:.cc=.o)

DAEM5403 = $(DAEMOBJ)/$(CC_DAEM5403:.cc=.o)

DAEM5501 = $(DAEMOBJ)/$(CC_DAEM5501:.cc=.o)
DAEM5502 = $(DAEMOBJ)/$(CC_DAEM5502:.cc=.o)
DAEM5505 = $(DAEMOBJ)/$(CC_DAEM5505:.cc=.o)

DAEM5901 = $(DAEMOBJ)/$(CC_DAEM5901:.cc=.o)

DAEM8001 = $(DAEMOBJ)/$(CC_DAEM8001:.cc=.o)
DAEM8004 = $(DAEMOBJ)/$(CC_DAEM8004:.cc=.o)

DAEM9001 = $(DAEMOBJ)/$(CC_DAEM9001:.cc=.o)
DAEM9002 = $(DAEMOBJ)/$(CC_DAEM9002:.cc=.o)
DAEM9003 = $(DAEMOBJ)/$(CC_DAEM9003:.cc=.o)

DAEMADMINOUT = $(DAEMOBJ)/$(CC_DAEMADMINOUT:.cc=.o)
DAEMSCHSTATS = $(DAEMOBJ)/$(CC_DAEMSCHSTATS:.cc=.o)

DAEMSYNC = $(DAEMOBJ)/$(CC_DAEMSYNC:.cc=.o)
DAEMCYB = $(DAEMOBJ)/$(CC_DAEMCYB:.cc=.o)
DAEMSFH = $(DAEMOBJ)/$(CC_DAEMSFH:.cc=.o)
DAEMUDTINFO = $(DAEMOBJ)/$(CC_DAEMUDTINFO:.cc=.o)

DAEMAPACHE = $(DAEMOBJ)/$(CC_DAEMAPACHE:.cc=.o)

# ---------------------------------------------------------------------------
#	Generate Main Target Module
# ---------------------------------------------------------------------------
T5002  = $(DAEMBIN)/daem5002
T5003  = $(DAEMBIN)/daem5003
T5004  = $(DAEMBIN)/daem5004
T5006  = $(DAEMBIN)/daem5006
T5007  = $(DAEMBIN)/daem5007

T5011  = $(DAEMBIN)/daem5011

T5021  = $(DAEMBIN)/daem5021
T5023  = $(DAEMBIN)/daem5023
T5024  = $(DAEMBIN)/daem5024
T5025  = $(DAEMBIN)/daem5025

T5101  = $(DAEMBIN)/daem5101
T5103  = $(DAEMBIN)/daem5103
T51031 = $(DAEMBIN)/daem51031
T5106  = $(DAEMBIN)/daem5106
T5107  = $(DAEMBIN)/daem5107
T5109  = $(DAEMBIN)/daem5109

T5110  = $(DAEMBIN)/daem5110
T5112  = $(DAEMBIN)/daem5112

T5121    = $(DAEMBIN)/daem5121
T5122  = $(DAEMBIN)/daem5122
T5123  = $(DAEMBIN)/daem5123
T5124  = $(DAEMBIN)/daem5124

T5403  = $(DAEMBIN)/daem5403

T5501  = $(DAEMBIN)/daem5501
T5502  = $(DAEMBIN)/daem5502
T5505  = $(DAEMBIN)/daem5505
T5901    = $(DAEMBIN)/daem5901

T8001    = $(DAEMBIN)/daem8001
T8004    = $(DAEMBIN)/daem8004
T9001    = $(DAEMBIN)/daem9001
T9002    = $(DAEMBIN)/daem9002
T9003    = $(DAEMBIN)/daem9003

TADMINOUT    = $(DAEMBIN)/daemadminout
TSCHSTATS    = $(DAEMBIN)/daemschstats

TSYNC    = $(DAEMBIN)/daemsync
TCYB    = $(DAEMBIN)/daemcyb
TSFH    = $(DAEMBIN)/daemsfh
TUDTINFO    = $(DAEMBIN)/daemudtinfo
TAPACHE    = $(DAEMBIN)/daemapache

all: daem5002 daem5003 daem5004 daem5006 daem5007 \
	 daem5011 \
	 daem5021 daem5023 daem5024 daem5025 \
	 daem5101 daem5103 daem51031 daem5106 daem5107 daem5109 daem5110 daem5112 \
	 daem5121 daem5122 daem5123 daem5124 \
     daem5403 \
     daem5501 daem5502 daem5505 \
     daem5901 \
     daem8001 daem8004 \
     daem9001 daem9002 daem9003\
     daemadminout daemschstats \
     daemsync daemcyb daemsfh daemudtinfo

# ---------------------------------------------------------------------------
#	Generate Each Target Module
# ---------------------------------------------------------------------------
daem5002:	$(T5002)
daem5003:	$(T5003)
daem5004:	$(T5004)
daem5006:	$(T5006)
daem5007:	$(T5007)

daem5011:	$(T5011)

daem5021:	$(T5021)
daem5023:	$(T5023)
daem5024:	$(T5024)
daem5025:	$(T5025)

daem5101:	$(T5101)
daem5103:	$(T5103)
daem51031:	$(T51031)
daem5106:	$(T5106)
daem5107:	$(T5107)
daem5109:	$(T5109)

daem5110:	$(T5110)
daem5112:	$(T5112)

daem5121:	$(T5121)
daem5122:	$(T5122)
daem5123:	$(T5123)
daem5124:	$(T5124)

daem5403:	$(T5403)

daem5501:	$(T5501)
daem5502:	$(T5502)
daem5505:	$(T5505)

daem5901:	$(T5901)

daem8001:	$(T8001)
daem8004:	$(T8004)

daem9001:	$(T9001)
daem9002:	$(T9002)
daem9003:	$(T9003)

daemadminout:	$(TADMINOUT)
daemschstats:	$(TSCHSTATS)

daemsync:	$(TSYNC)
daemcyb:	$(TCYB)
daemsfh:	$(TSFH)
daemudtinfo:	$(TUDTINFO)

daemapache:    $(TAPACHE)

$(T5002)	: $(COM_OBJFILES) $(DAEM5002)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5002) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread
$(T5003)	: $(COM_OBJFILES) $(DAEM5003)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5003) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread
$(T5004)	: $(COM_OBJFILES) $(DAEM5004)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5004) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread
$(T5006)	: $(COM_OBJFILES) $(DAEM5006)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5006) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread
$(T5007)	: $(COM_OBJFILES) $(DAEM5007)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5007) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread

$(T5011)	: $(COM_OBJFILES) $(DAEM5011)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5011) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread

$(T5021)	: $(COM_OBJFILES) $(DAEM5021)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5021) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread
$(T5023)	: $(COM_OBJFILES) $(DAEM5023)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5023) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread
$(T5024)	: $(COM_OBJFILES) $(DAEM5024)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5024) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread
$(T5025)	: $(COM_OBJFILES) $(DAEM5025)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5025) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread

$(T5101)	: $(COM_OBJFILES) $(DAEM5101)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5101) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread
$(T5103)	: $(COM_OBJFILES) $(DAEM5103)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5103) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread
$(T51031)	: $(COM_OBJFILES) $(DAEM51031)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM51031) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread
$(T5106)	: $(COM_OBJFILES) $(DAEM5106)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5106) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread
$(T5107)	: $(COM_OBJFILES) $(DAEM5107)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5107) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	
$(T5109)	: $(COM_OBJFILES) $(DAEM5109)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5109) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	

$(T5110)	: $(COM_OBJFILES) $(DAEM5110)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5110) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	
$(T5112)	: $(COM_OBJFILES) $(DAEM5112)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5112) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	

$(T5121)	: $(COM_OBJFILES) $(DAEM5121)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5121) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	
$(T5122)	: $(COM_OBJFILES) $(DAEM5122)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5122) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	
$(T5123)	: $(COM_OBJFILES) $(DAEM5123)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5123) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	
$(T5124)	: $(COM_OBJFILES) $(DAEM5124)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5124) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	

$(T5403)	: $(COM_OBJFILES) $(DAEM5403)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5403) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread			

$(T5501)	: $(COM_OBJFILES) $(DAEM5501)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5501) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	
$(T5502)	: $(COM_OBJFILES) $(DAEM5502)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5502) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread		
$(T5505)	: $(COM_OBJFILES) $(DAEM5505)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5505) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread						
$(T5901)	: $(COM_OBJFILES) $(DAEM5901)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5901) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread						
		
$(T8001)	: $(COM_OBJFILES) $(DAEM8001)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM8001) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread
$(T8004)	: $(COM_OBJFILES) $(DAEM8004)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM8004) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	

$(T9001)	: $(COM_OBJFILES) $(DAEM9001)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM9001) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	
$(T9002)	: $(COM_OBJFILES) $(DAEM9002)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM9002) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	
$(T9003)	: $(COM_OBJFILES) $(DAEM9003)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM9003) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	
$(TADMINOUT)	: $(COM_OBJFILES) $(DAEMADMINOUT)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEMADMINOUT) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread 
$(TSCHSTATS)	: $(COM_OBJFILES) $(DAEMSCHSTATS)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEMSCHSTATS) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -lpthread	-lcurl 

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

$(TAPACHE) : $(COM_OBJFILES) $(DAEMAPACHE)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEMAPACHE) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread   

# ---------------------------------------------------------------------------
#	Generate Object Module
# ---------------------------------------------------------------------------
$(DAEM5002): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static
$(DAEM5003): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static
$(DAEM5004): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static
$(DAEM5006): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static
$(DAEM5007): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static								
	
$(DAEM5011): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static

$(DAEM5021): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static
$(DAEM5023): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static
$(DAEM5024): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static
$(DAEM5025): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static
	
$(DAEM5101): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static
$(DAEM5103): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static
$(DAEM51031): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static
$(DAEM5106): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static
$(DAEM5107): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static	
$(DAEM5109): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static	

$(DAEM5110): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static	
$(DAEM5112): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static	

$(DAEM5121): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static	
$(DAEM5122): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static		
$(DAEM5123): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static		
$(DAEM5124): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static		

$(DAEM5403): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static				

$(DAEM5501): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static		
$(DAEM5502): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static			
$(DAEM5505): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static							
$(DAEM5901): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static								

$(DAEM8001): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static
$(DAEM8004): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static	

$(DAEM9001): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static	
$(DAEM9002): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static	
$(DAEM9003): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static	
$(DAEMADMINOUT): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static	
$(DAEMSCHSTATS): $(DAEMSRC)/$(@F:.o=.cc)
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
$(DAEMAPACHE): $(DAEMSRC)/$(@F:.o=.cc)
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
	@make cleandaem5002 cleandaem5003 cleandaem5004 cleandaem5006 cleandaem5007 \
					 cleandaem5011 \
					 cleandaem5021 cleandaem5023 cleandaem5024 cleandaem5025 \
	                 cleandaem5101 cleandaem5103 cleandaem51031 cleandaem5106 cleandaem5107 cleandaem5109 \
	                 cleandaem5110 cleandaem5112 cleandaem5121 cleandaem5122 cleandaem5123 cleandaem5124 \
	                 cleandaem5403 \
	                 cleandaem5501 cleandaem5502 cleandaem5505 \
	                 cleandaem5901 \
	                 cleandaem8001 cleandaem8004 \
	                 cleandaem9001 cleandaem9002 cleandaem9003 \
	                 cleandaemadminout cleandaemschstats \
	                 cleandaemsync cleandaemcyb cleandaemsfh cleandaemudtinfo

cleandaem5002:
	@echo 'Remove Object Files(cleandaem5002)'
	@$(RM) $(T5002) $(DAEM5002) $(COM_OBJFILES)
cleandaem5003:
	@echo 'Remove Object Files(cleandaem5003)'
	@$(RM) $(T5003) $(DAEM5003) $(COM_OBJFILES)
cleandaem5004:
	@echo 'Remove Object Files(cleandaem5004)'
	@$(RM) $(T5004) $(DAEM5004) $(COM_OBJFILES)
cleandaem5006:
	@echo 'Remove Object Files(cleandaem5006)'
	@$(RM) $(T5006) $(DAEM5006) $(COM_OBJFILES)
cleandaem5007:
	@echo 'Remove Object FIles(cleandaem5007)'
	@$(RM) $(T5007) $(DAEM5007) $(COM_OBJFILES)

cleandaem5011:
	@echo 'Remove Object Files(cleandaem5011)'
	@$(RM) $(T5011) $(DAEM5011) $(COM_OBJFILES)
cleandaem5021:
	@echo 'Remove Object Files(cleandaem5021)'
	@$(RM) $(T5021) $(DAEM5021) $(COM_OBJFILES)
cleandaem5023:
	@echo 'Remove Object Files(cleandaem5023)'
	@$(RM) $(T5023) $(DAEM5023) $(COM_OBJFILES)
cleandaem5024:
	@echo 'Remove Object Files(cleandaem5024)'
	@$(RM) $(T5024) $(DAEM5024) $(COM_OBJFILES)
cleandaem5025:
	@echo 'Remove Object Files(cleandaem5025)'
	@$(RM) $(T5025) $(DAEM5025) $(COM_OBJFILES)

cleandaem5101:
	@echo 'Remove Object Files(cleandaem5101)'
	@$(RM) $(T5101) $(DAEM5101) $(COM_OBJFILES)
cleandaem5103:
	@echo 'Remove Object Files(cleandaem5103)'
	@$(RM) $(T5103) $(DAEM5103) $(COM_OBJFILES)
cleandaem51031:
	@echo 'Remove Object Files(cleandaem51031)'
	@$(RM) $(T51031) $(DAEM51031) $(COM_OBJFILES)
cleandaem5106:
	@echo 'Remove Object Files(cleandaem5106)'
	@$(RM) $(T5106) $(DAEM5106) $(COM_OBJFILES)
cleandaem5107:
	@echo 'Remove Object Files(cleandaem5107)'
	@$(RM) $(T5107) $(DAEM5107) $(COM_OBJFILES)	
cleandaem5109:
	@echo 'Remove Object Files(cleandaem5109)'
	@$(RM) $(T5109) $(DAEM5109) $(COM_OBJFILES)	

cleandaem5110:
	@echo 'Remove Object Files(cleandaem5110)'
	@$(RM) $(T5110) $(DAEM5110) $(COM_OBJFILES)	
cleandaem5112:
	@echo 'Remove Object Files(cleandaem5112)'
	@$(RM) $(T5112) $(DAEM5112) $(COM_OBJFILES)	
cleandaem5121:
	@echo 'Remove Object Files(cleandaem5121)'
	@$(RM) $(T5121) $(DAEM5121) $(COM_OBJFILES)	
cleandaem5122:
	@echo 'Remove Object Files(cleandaem5122)'
	@$(RM) $(T5122) $(DAEM5122) $(COM_OBJFILES)		
cleandaem5123:
	@echo 'Remove Object Files(cleandaem5123)'
	@$(RM) $(T5123) $(DAEM5123) $(COM_OBJFILES)		
cleandaem5124:
	@echo 'Remove Object Files(cleandaem5124)'
	@$(RM) $(T5124) $(DAEM5124) $(COM_OBJFILES)		

cleandaem5403:
	@echo 'Remove Object Files(cleandaem5403)'
	@$(RM) $(T5403) $(DAEM5403) $(COM_OBJFILES)				

cleandaem5501:
	@echo 'Remove Object Files(cleandaem5501)'
	@$(RM) $(T5501) $(DAEM5501) $(COM_OBJFILES)		
cleandaem5502:
	@echo 'Remove Object Files(cleandaem5502)'
	@$(RM) $(T5502) $(DAEM5502) $(COM_OBJFILES)			
cleandaem5505:
	@echo 'Remove Object Files(cleandaem5505)'
	@$(RM) $(T5505) $(DAEM5505) $(COM_OBJFILES)							
cleandaem5901:
	@echo 'Remove Object Files(cleandaem5901)'
	@$(RM) $(T5901) $(DAEM5901) $(COM_OBJFILES)							

cleandaem8001:
	@echo 'Remove Object Files(cleandaem8001)'
	@$(RM) $(T8001) $(DAEM8001) $(COM_OBJFILES)
cleandaem8004:
	@echo 'Remove Object Files(cleandaem8004)'
	@$(RM) $(T8004) $(DAEM8004) $(COM_OBJFILES)	

cleandaem9001:
	@echo 'Remove Object Files(cleandaem9001)'
	@$(RM) $(T9001) $(DAEM9001) $(COM_OBJFILES)	
cleandaem9002:
	@echo 'Remove Object Files(cleandaem9002)'
	@$(RM) $(T9002) $(DAEM9002) $(COM_OBJFILES)	
cleandaem9003:
	@echo 'Remove Object Files(cleandaem9003)'
	@$(RM) $(T9003) $(DAEM9003) $(COM_OBJFILES)	
cleandaemadminout :
	@echo 'Remove Object Files(cleandaemadminout)'
	@$(RM) $(TADMINOUT) $(DAEMADMINOUT) $(COM_OBJFILES)	
cleandaemschstats:
	@echo 'Remove Object Files(cleandaemschstats)'
	@$(RM) $(TSCHSTATS) $(DAEMSCHSTATS) $(COM_OBJFILES)	

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


# ===========================================================================
#	End of Makefile
# ===========================================================================
