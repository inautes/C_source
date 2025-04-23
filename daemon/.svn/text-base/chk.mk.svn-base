# ---------------------------------------------------------------------------
#	Directory Information
# ---------------------------------------------------------------------------
# common directory

COMMSRC = $(HOME)/zangsi_with_dcmd/common/src
COMMOBJ = $(HOME)/zangsi_with_dcmd/common/obj
COMMINC = $(HOME)/zangsi_with_dcmd/common/inc

# daemon common source
DAEMBIN = $(HOME)/zangsi_with_dcmd/daemon/bin
DAEMSRC = $(HOME)/zangsi_with_dcmd/daemon/src
DAEMOBJ = $(HOME)/zangsi_with_dcmd/daemon/obj
DAEMINC = $(HOME)/zangsi_with_dcmd/daemon/inc

# chkdaemon directory
CHKSBIN = $(HOME)/zangsi_with_dcmd/daemon/rbin
CHKSSRC = $(HOME)/zangsi_with_dcmd/daemon/src
CHKSOBJ = $(HOME)/zangsi_with_dcmd/daemon/obj
CHKSINC = $(HOME)/zangsi_with_dcmd/daemon/inc


# MySQL directory
MYDBINC = /usr/local/mysql/include/mysql
MYDBLIB = /usr/local/mysql/lib/mysql

# ---------------------------------------------------------------------------
#	Compiler Information & Copy Remove Options
# ---------------------------------------------------------------------------
CC      = g++
RM      = rm -f
SYSINC  = /usr/include
COMMINCS= -I. -I$(COMMINC) -I$(MYDBINC)
CHKSINCS= -I. -I$(COMMINC) -I$(CHKSINC) -I$(MYDBINC)
DEBUGS	= -D_THREAD_MODULE_ -D_THREAD_SAFE -D_REENTRANT 
#-D_DEBUG_ -D__DEBUG
CFLAGS  = $(DEBUGS)

# ---------------------------------------------------------------------------
#	Source List Information 
# ---------------------------------------------------------------------------
# common source
CC_COMMYDB  = commydb1.cc
CC_COMCONF  = comconf.cc
CC_COMTHRD  = comthrd.cc
CC_COMCOMM  = comcomm.cc

# check server daemon source
CC_CHKSMAIN  = chkdaem.cc
CC_CHKSSOCK  = chkdaem_socket.cc

# daemon common source
CC_DAEMCOM   = daemcom.cc


#######################################################################
COM_OBJFILES = \
	$(COMMOBJ)/$(CC_COMMYDB:.cc=.o) \
	$(COMMOBJ)/$(CC_COMCONF:.cc=.o) \
	$(COMMOBJ)/$(CC_COMTHRD:.cc=.o) \
	$(COMMOBJ)/$(CC_COMCOMM:.cc=.o)
	
CHKS_OBJFILES = \
	$(CHKSOBJ)/$(CC_CHKSMAIN:.cc=.o) \
	$(CHKSOBJ)/$(CC_CHKSSOCK:.cc=.o) \
	$(CHKSOBJ)/$(CC_DAEMCOM:.cc=.o) \
	
	
# ---------------------------------------------------------------------------
#	Generate Main Target Module
# ---------------------------------------------------------------------------
TARGET1 = $(CHKSBIN)/chkdaemon


all: chkdaemon

# ---------------------------------------------------------------------------
#	Generate Each Target Module
# ---------------------------------------------------------------------------
chkdaemon:	$(TARGET1)

$(TARGET1)	: $(COM_OBJFILES)  $(CHKS_OBJFILES)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(CHKS_OBJFILES) $(COM_OBJFILES) $(DEBUGS) $(CHKSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread

# ---------------------------------------------------------------------------
#	Generate Object Module
# ---------------------------------------------------------------------------
$(CHKS_OBJFILES): $(CHKSSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(CHKSSRC)/$(@F:.o=.cc)
	@$(CC) -c $(CHKSSRC)/$(@F:.o=.cc) -o $@ $(DEBUGS) $(CHKSINCS) -L$(MYDBLIB) -static

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
	@echo 'Remove Object Files(cleanchkdaemon)'
	@$(RM) $(TARGET1) $(CHKS_OBJFILES) $(COM_OBJFILES)
	
#	@echo 'Remove All Object Files'
#	@make cleanchkdaemon

cleanchkdaemon:
	@echo 'Remove Object Files(cleanchkdaemon)'
	@$(RM) $(TARGET1) $(CHKS_OBJFILES) $(COM_OBJFILES)


# ===========================================================================
#	End of Makefile
# ===========================================================================





















