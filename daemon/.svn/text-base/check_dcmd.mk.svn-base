# ---------------------------------------------------------------------------
#	Directory Information
# ---------------------------------------------------------------------------
# common directory

COMMSRC = $(HOME)/zangsi/common/src
COMMOBJ = $(HOME)/zangsi/common/obj
COMMINC = $(HOME)/zangsi/common/inc

# daemon common source
DAEMBIN = $(HOME)/zangsi/daemon/bin
DAEMSRC = $(HOME)/zangsi/daemon/src
DAEMOBJ = $(HOME)/zangsi/daemon/obj
DAEMINC = $(HOME)/zangsi/daemon/inc

# chkdaemon directory
CHKSBIN = $(HOME)/zangsi/daemon/rbin
CHKSSRC = $(HOME)/zangsi/daemon/src
CHKSOBJ = $(HOME)/zangsi/daemon/obj
CHKSINC = $(HOME)/zangsi/daemon/inc


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

# check server daemon source
CC_CHKSMAIN  = check_dcmd.cc

# daemon common source
CC_DAEMCOM   = daemcom.cc


#######################################################################
COM_OBJFILES = \
	
CHKS_OBJFILES = \
	$(CHKSOBJ)/$(CC_CHKSMAIN:.cc=.o) \
	$(CHKSOBJ)/$(CC_DAEMCOM:.cc=.o) \
	
	
# ---------------------------------------------------------------------------
#	Generate Main Target Module
# ---------------------------------------------------------------------------
TARGET1 = $(CHKSBIN)/check_dcmd


all: check_dcmd

# ---------------------------------------------------------------------------
#	Generate Each Target Module
# ---------------------------------------------------------------------------
check_dcmd:	$(TARGET1)

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
	@echo 'Remove Object Files(cleancheck_dcmd)'
	@$(RM) $(TARGET1) $(CHKS_OBJFILES) $(COM_OBJFILES)
	
#	@echo 'Remove All Object Files'
#	@make cleancheck_dcmd

cleancheck_dcmd:
	@echo 'Remove Object Files(cleancheck_dcmd)'
	@$(RM) $(TARGET1) $(CHKS_OBJFILES) $(COM_OBJFILES)


# ===========================================================================
#	End of Makefile
# ===========================================================================





















