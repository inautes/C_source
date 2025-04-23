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
# common source
CC_COMMYDB  = commydb1.cc
CC_DAEMCOM  = daemcom.cc
CC_MYDB_C   = MysqlDB.cc	#mysql 사용 클래스
# ---------------------------------------------------------------------------
# In Contents Server 
# ---------------------------------------------------------------------------
CC_DAEM5003 = daem5003.cc	#컨텐츠 삭제처리

# ---------------------------------------------------------------------------
#  In FBKS01 
# ---------------------------------------------------------------------------
CC_DAEM5002 = daem5002.cc	#컨텐츠 연장처리
CC_DAEM5004 = daem5004.cc	#삭제된 컨텐츠 정보삭제
CC_DAEM5006 = daem5006.cc	#VIR_ID2 동기화데몬
CC_DAEM5007 = daem5007.cc	#T_CONTENTS_FILE_USER_CNT 초기화 (0)

CC_DAEM5011 = daem5011.cc	#수수료계산
CC_DAEM5021 = daem5021.cc	#내디스크용량계산
CC_DAEM5023 = daem5023.cc	#내디스크삭제처리
CC_DAEM5024 = daem5024.cc	#업로드 자격 취소자 컨텐츠 삭제 처리(미적용)
CC_DAEM5025 = daem5025.cc	#요청자료 게시판 삭제 데몬

CC_DAEM5101 = daem5101.cc	#회원가입현황
CC_DAEM5103 = daem5103.cc	#분류별판매집계
CC_DAEM51031 = daem51031.cc	#무료다운로드집계
CC_DAEM5106 = daem5106.cc	#사용자실적분석
CC_DAEM5107 = daem5107.cc	#업로드 권한 정보 추출
CC_DAEM5108 = daem5108.cc	#웹,채팅,다운,업로그 사용자수 계산
CC_DAEM5111 = daem5111.cc	#준업로드 포인트 집계
CC_DAEM5112 = daem5112.cc	#Contents 구매 수량 집계
CC_DAEM5901 = daem5901.cc	#비정상 결제자 추출


CC_DAEM5120 = daem5120.cc   #전체 통계
CC_DAEM5122 = daem5122.cc	#관리자 상담글 집계
CC_DAEM5123 = daem5123.cc	#보상처리
CC_DAEM5124 = daem5124.cc	#보상처리

CC_DAEM5403 = daem5403.cc	#관리자 상담글 집계 처리

CC_DAEM5501 = daem5501.cc	#현재 날짜로부터 30일전의 받은 쪽지 삭제
CC_DAEM5502 = daem5502.cc	#현재 날짜로부터 30일전의 보낸 쪽지 삭제
CC_DAEM5505 = daem5505.cc	#일간 평균 로그인수 집계

CC_DAEM5701 = daem5701.cc	#inout_amount 내역 백업

CC_DAEM9001 = daem9001.cc	# 추석 이벤트
CC_DAEM9002 = daem9002.cc	# 추석 이벤트
#CC_DAEM9003 = daem9003.cc	# Top100   # 자바에서 처리함.

CC_DAEMADMINOUT = daemadminout.cc	# 관리운영 통계 (자바로 변경 wb.stat.StatManaCateFreeDown)

# ---------------------------------------------------------------------------
#  In FLOGIN_LOG
# ---------------------------------------------------------------------------
CC_DAEM5120_log = daem5120_log.cc   #전체 통계 서브 : 재로그인 정보 수집
CC_DAEM5506 = daem5506.cc	#현재 날짜로부터 90일전의 login_log 정보삭제(미적용)

# ---------------------------------------------------------------------------
#  In FSUMDB
# ---------------------------------------------------------------------------
CC_DAEM5601 = daem5601.cc	#다운로드 내역 백업
CC_DAEM5602 = daem5602.cc	#inout_amount 내역 백업

CC_DAEM5601_INST = daem5601_inst.cc	#다운로드 내역 백업
CC_DAEM5602_INST = daem5602_inst.cc	#inout_amount 내역 백업

CC_DAEM5601_DEL = daem5601_del.cc	#다운로드 내역 백업
CC_DAEM5602_DEL = daem5602_del.cc	#inout_amount 내역 백업


# ---------------------------------------------------------------------------
#  In FSCH01
# ---------------------------------------------------------------------------
CC_DAEMSCHSTATS = daemschstats.cc	# 검색어 집계



#######################################################################
COM_OBJFILES = \
	$(COMMOBJ)/$(CC_COMMYDB:.cc=.o) \
	$(COMMOBJ)/$(CC_DAEMCOM:.cc=.o) 
#	$(COMMOBJ)/$(CC_MYDB_C:.cc=.o) 
	

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
DAEM5108 = $(DAEMOBJ)/$(CC_DAEM5108:.cc=.o)
DAEM5111 = $(DAEMOBJ)/$(CC_DAEM5111:.cc=.o)
DAEM5112 = $(DAEMOBJ)/$(CC_DAEM5112:.cc=.o)
DAEM5901 = $(DAEMOBJ)/$(CC_DAEM5901:.cc=.o)
DAEM5120 = $(DAEMOBJ)/$(CC_DAEM5120:.cc=.o)
DAEM5120_log = $(DAEMOBJ)/$(CC_DAEM5120_log:.cc=.o)
DAEM5122 = $(DAEMOBJ)/$(CC_DAEM5122:.cc=.o)
DAEM5123 = $(DAEMOBJ)/$(CC_DAEM5123:.cc=.o)
DAEM5124 = $(DAEMOBJ)/$(CC_DAEM5124:.cc=.o)
DAEM5403 = $(DAEMOBJ)/$(CC_DAEM5403:.cc=.o)
DAEM5501 = $(DAEMOBJ)/$(CC_DAEM5501:.cc=.o)
DAEM5502 = $(DAEMOBJ)/$(CC_DAEM5502:.cc=.o)
DAEM5505 = $(DAEMOBJ)/$(CC_DAEM5505:.cc=.o)
DAEM5506 = $(DAEMOBJ)/$(CC_DAEM5506:.cc=.o)
DAEM5601 = $(DAEMOBJ)/$(CC_DAEM5601:.cc=.o)
DAEM5602 = $(DAEMOBJ)/$(CC_DAEM5602:.cc=.o)
DAEM5601_INST = $(DAEMOBJ)/$(CC_DAEM5601_INST:.cc=.o)
DAEM5602_INST = $(DAEMOBJ)/$(CC_DAEM5602_INST:.cc=.o)
DAEM5601_DEL = $(DAEMOBJ)/$(CC_DAEM5601_DEL:.cc=.o)
DAEM5602_DEL = $(DAEMOBJ)/$(CC_DAEM5602_DEL:.cc=.o)
DAEM5701 = $(DAEMOBJ)/$(CC_DAEM5701:.cc=.o)
DAEM9001 = $(DAEMOBJ)/$(CC_DAEM9001:.cc=.o)
DAEM9002 = $(DAEMOBJ)/$(CC_DAEM9002:.cc=.o)
DAEMSCHSTATS = $(DAEMOBJ)/$(CC_DAEMSCHSTATS:.cc=.o)
DAEMADMINOUT = $(DAEMOBJ)/$(CC_DAEMADMINOUT:.cc=.o)

#DAEM9003 = $(DAEMOBJ)/$(CC_DAEM9003:.cc=.o)   # 자바에서 처리함.

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
T51031  = $(DAEMBIN)/daem51031
T5106    = $(DAEMBIN)/daem5106
T5107    = $(DAEMBIN)/daem5107
T5108    = $(DAEMBIN)/daem5108

T5111    = $(DAEMBIN)/daem5111
T5112    = $(DAEMBIN)/daem5112
T5901    = $(DAEMBIN)/daem5901

T5120    = $(DAEMBIN)/daem5120
T5120_log    = $(DAEMBIN)/daem5120_log
T5122    = $(DAEMBIN)/daem5122
T5123    = $(DAEMBIN)/daem5123
T5124    = $(DAEMBIN)/daem5124

T5403    = $(DAEMBIN)/daem5403

T5501    = $(DAEMBIN)/daem5501
T5502    = $(DAEMBIN)/daem5502
T5505    = $(DAEMBIN)/daem5505
T5506    = $(DAEMBIN)/daem5506

T5601    = $(DAEMBIN)/daem5601
T5602    = $(DAEMBIN)/daem5602

T5601_INST    = $(DAEMBIN)/daem5601_inst
T5602_INST    = $(DAEMBIN)/daem5602_inst
T5601_DEL    = $(DAEMBIN)/daem5601_del
T5602_DEL    = $(DAEMBIN)/daem5602_del
T5701    = $(DAEMBIN)/daem5701
T9001    = $(DAEMBIN)/daem9001
T9002    = $(DAEMBIN)/daem9002
#T9003    = $(DAEMBIN)/daem9003  # 자바에서 처리함.

TSCHSTATS    = $(DAEMBIN)/daemschstats

TADMINOUT    = $(DAEMBIN)/daemadminout

all: daem5002 daem5004 daem5006 daem5021 daem5024  \
     daem5111 daem5601 daem5602 \
     daem5112 daem5901 \
     daem5601_inst daem5602_inst \
     daem5601_del daem5602_del \
     daem5505 daem5506 daemschstats

#	daem5003 외부 아이피
#	daem9001 \
#	daem9002 \
#       daem5403 daem5501 daem5502 \
#	daem5120 daem5120_log daem5122 daem5123 daem5124 \     
#	daem5103 daem51031 daem5106 daem5107 daem5108 \
#	daem5101
#	daem5025
#	daem5023
#	daem5011 \
#	daem5701 \
#	daemadminout
#	daem9003 \

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
daem5108:	$(T5108)

daem5111:	$(T5111)
daem5112:	$(T5112)
daem5901:	$(T5901)
daem5120:	$(T5120)

daem5120_log:	$(T5120_log)
daem5122:	$(T5122)
daem5123:	$(T5123)
daem5124:	$(T5124)

daem5403:	$(T5403)

daem5501:	$(T5501)
daem5502:	$(T5502)
daem5505:	$(T5505)
daem5506:	$(T5506)

daem5601:	$(T5601)
daem5602:	$(T5602)

daem5601_inst:	$(T5601_INST)
daem5602_inst:	$(T5602_INST)

daem5601_del:	$(T5601_DEL)
daem5602_del:	$(T5602_DEL)

daem5701:	$(T5701)

daem9001:	$(T9001)
daem9002:	$(T9002)
#daem9003:	$(T9003)

daemschstats:	$(TSCHSTATS)

daemadminout:	$(TADMINOUT)


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
$(T5025)	: $(COM_OBJFILES) $(DAEM5025)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5025) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread
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
$(T5108)	: $(COM_OBJFILES) $(DAEM5108)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5108) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	
$(T5111)	: $(COM_OBJFILES) $(DAEM5111)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5111) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	
$(T5112)	: $(COM_OBJFILES) $(DAEM5112)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5112) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	
$(T5901)	: $(COM_OBJFILES) $(DAEM5901)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5901) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	
$(T5120)	: $(COM_OBJFILES) $(DAEM5120)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5120) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	
$(T5120_log)	: $(COM_OBJFILES) $(DAEM5120_log)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5120_log) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread	
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
$(T5506)	: $(COM_OBJFILES) $(DAEM5506)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5506) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread						
$(T5601)	: $(COM_OBJFILES) $(DAEM5601)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5601) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread						
$(T5602)	: $(COM_OBJFILES) $(DAEM5602)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5602) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread						

$(T5601_INST)	: $(COM_OBJFILES) $(DAEM5601_INST)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5601_INST) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread						
$(T5602_INST)	: $(COM_OBJFILES) $(DAEM5602_INST)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5602_INST) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread						
$(T5601_DEL)	: $(COM_OBJFILES) $(DAEM5601_DEL)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5601_DEL) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread						
$(T5602_DEL)	: $(COM_OBJFILES) $(DAEM5602_DEL)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5602_DEL) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread						

$(T5701)	: $(COM_OBJFILES) $(DAEM5701)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEM5701) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread						
		
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

#$(T9003)        : $(COM_OBJFILES) $(DAEM9003)
#	@echo '=================================================================='
#	@echo 'Create server... :' $@
#	@echo '=================================================================='
#	@$(CC) -o $@ $(DAEM9003) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread 

$(TSCHSTATS)	: $(COM_OBJFILES) $(DAEMSCHSTATS)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEMSCHSTATS) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -lpthread	 -lcurl 
$(TADMINOUT)	: $(COM_OBJFILES) $(DAEMADMINOUT)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DAEMADMINOUT) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread 

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
$(DAEM5108): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static	
$(DAEM5111): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static	
$(DAEM5112): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static	
$(DAEM5901): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static	
$(DAEM5120): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static	
$(DAEM5120_log): $(DAEMSRC)/$(@F:.o=.cc)
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
$(DAEM5506): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static								
$(DAEM5601): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static								
$(DAEM5602): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static								
$(DAEM5601_INST): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static								
$(DAEM5602_INST): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static
$(DAEM5601_DEL): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static
$(DAEM5602_DEL): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static
$(DAEM5701): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static
$(DAEM9001): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static	
$(DAEM9002): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static		
#$(DAEM9003): $(DAEMSRC)/$(@F:.o=.cc)
#	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
#	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static
$(DAEMSCHSTATS): $(DAEMSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DAEMSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DAEMSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(DAEMINCS) -L$(MYDBLIB) -static	
$(DAEMADMINOUT): $(DAEMSRC)/$(@F:.o=.cc)
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
	@make -f real_cron.mk cleandaem5002 cleandaem5003 cleandaem5004 cleandaem5006\
	                 cleandaem5011 \
	                 cleandaem5021 cleandaem5023 cleandaem5024 cleandaem5025 \
	                 cleandaem5101 cleandaem5103 cleandaem51031 cleandaem5106 cleandaem5107 cleandaem5108 \
	                 cleandaem5111 cleandaem5112 cleandaem5901 \
	                 cleandaem5120 cleandaem5120_log cleandaem5122 cleandaem5123 cleandaem5124 \
	                 cleandaem5403 \
	                 cleandaem5501 cleandaem5502 cleandaem5505 cleandaem5506 \
	                 cleandaem5601 cleandaem5602 \
	                 cleandaem5601_inst cleandaem5602_inst \
	                 cleandaem5601_del \
	                 cleandaem5701 \
	                 cleandaem9001 \
	                 cleandaem9002 \
	                 cleandaemschstats \
	                 cleandaemadminout \ 
			 cleandaem5602_del

#	                 cleandaem9003 \

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
	@echo 'Remove Object Files(cleandaem5007)'
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
	@$(RM) $(T5023) $(DAEM5025) $(COM_OBJFILES)
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
cleandaem5108:
	@echo 'Remove Object Files(cleandaem5108)'
	@$(RM) $(T5108) $(DAEM5108) $(COM_OBJFILES)	
cleandaem5111:
	@echo 'Remove Object Files(cleandaem5111)'
	@$(RM) $(T5111) $(DAEM5111) $(COM_OBJFILES)	
cleandaem5112:
	@echo 'Remove Object Files(cleandaem5112)'
	@$(RM) $(T5112) $(DAEM5112) $(COM_OBJFILES)	
cleandaem5901:
	@echo 'Remove Object Files(cleandaem5901)'
	@$(RM) $(T5901) $(DAEM5901) $(COM_OBJFILES)	
cleandaem5120:
	@echo 'Remove Object Files(cleandaem5120)'
	@$(RM) $(T5120) $(DAEM5120) $(COM_OBJFILES)	
cleandaem5120_log:
	@echo 'Remove Object Files(cleandaem5120_log)'
	@$(RM) $(T5120_log) $(DAEM5120_log) $(COM_OBJFILES)	
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
cleandaem5506:
	@echo 'Remove Object Files(cleandaem5506)'
	@$(RM) $(T5506) $(DAEM5506) $(COM_OBJFILES)							
cleandaem5601:
	@echo 'Remove Object Files(cleandaem5601)'
	@$(RM) $(T5601) $(DAEM5601) $(COM_OBJFILES)							
cleandaem5602:
	@echo 'Remove Object Files(cleandaem5602)'
	@$(RM) $(T5602) $(DAEM5602) $(COM_OBJFILES)							
cleandaem5601_inst:
	@echo 'Remove Object Files(cleandaem5601_inst)'
	@$(RM) $(T5601_INST) $(DAEM5601_INST) $(COM_OBJFILES)							
cleandaem5602_inst:
	@echo 'Remove Object Files(cleandaem5602_inst)'
	@$(RM) $(T5602_INST) $(DAEM5602_INST) $(COM_OBJFILES)							
cleandaem5601_del:
	@echo 'Remove Object Files(cleandaem5601_del)'
	@$(RM) $(T5601_DEL) $(DAEM5601_DEL) $(COM_OBJFILES)							
cleandaem5602_del:
	@echo 'Remove Object Files(cleandaem5602_del)'
	@$(RM) $(T5602_DEL) $(DAEM5602_DEL) $(COM_OBJFILES)							
cleandaem5701:
	@echo 'Remove Object Files(cleandaem5701)'
	@$(RM) $(T5701) $(DAEM5701) $(COM_OBJFILES)							
cleandaem9001:
	@echo 'Remove Object Files(cleandaem9001)'
	@$(RM) $(T9001) $(DAEM9001) $(COM_OBJFILES)	
cleandaem9002:
	@echo 'Remove Object Files(cleandaem9002)'
	@$(RM) $(T9002) $(DAEM9002) $(COM_OBJFILES)		
#cleandaem9003:
#	@echo 'Remove Object Files(cleandaem9003)'
#	@$(RM) $(T9003) $(DAEM9003) $(COM_OBJFILES)
cleandaemschstats:
	@echo 'Remove Object Files(cleandaemschstats)'
	@$(RM) $(TSCHSTATS) $(DAEMSCHSTATS) $(COM_OBJFILES)	
cleandaemadminout :
	@echo 'Remove Object Files(cleandaemschstats)'
	@$(RM) $(TADMINOUT) $(DAEMADMINOUT) $(COM_OBJFILES)	

# ===========================================================================
#	End of Makefile
# ===========================================================================
