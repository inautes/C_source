# ---------------------------------------------------------------------------
#	Directory Information
# ---------------------------------------------------------------------------
# common directory
COMMSRC = $(HOME)/zangsi_with_dcmd/daemon/src
COMMOBJ = $(HOME)/zangsi_with_dcmd/daemon/obj
COMMINC = $(HOME)/zangsi_with_dcmd/daemon/inc

# daemon directory
DAEMBIN = $(HOME)/zangsi_with_dcmd/daemon/rbin
DAEMSRC = $(HOME)/zangsi_with_dcmd/daemon/src
DAEMOBJ = $(HOME)/zangsi_with_dcmd/daemon/obj
DAEMINC = $(HOME)/zangsi_with_dcmd/daemon/inc

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
DAEMINCS= -I. -I$(COMMINC) -I$(DAEMINC) -I$(MYDBINC)
DEBUGS	= -D_THREAD_MODULE_ -D_THREAD_SAFE -D_REENTRANT
CFLAGS  = $(DEBUGS)

# ---------------------------------------------------------------------------
#	Source List Information 
# ---------------------------------------------------------------------------
# common source
CC_COMMYDB  = commydb3.cc
CC_DAEMCOM  = daemcom.cc
CC_DBSTRPROC  = DBStrProc.cc
 
# cmdserver source
CC_DBSTRESSTEST = DBStressTest.cc

#######################################################################
COM_OBJFILES = \
	$(COMMOBJ)/$(CC_COMMYDB:.cc=.o) \
	$(COMMOBJ)/$(CC_DAEMCOM:.cc=.o) \
	$(COMMOBJ)/$(CC_DBSTRPROC:.cc=.o)

DBSTRESSTEST = $(DAEMOBJ)/$(CC_DBSTRESSTEST:.cc=.o)

# ---------------------------------------------------------------------------
#	Generate Main Target Module
# ---------------------------------------------------------------------------
TDBSTRESSTEST    = $(DAEMBIN)/DBStressTest



all: DBStressTest

# ---------------------------------------------------------------------------
#	Generate Each Target Module
# ---------------------------------------------------------------------------
DBStressTest:	$(TDBSTRESSTEST)

$(TDBSTRESSTEST)	: $(COM_OBJFILES) $(DBSTRESSTEST)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(DBSTRESSTEST) $(COM_OBJFILES) $(DEBUGS) $(CMDSINCS) -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread


# ---------------------------------------------------------------------------
#	Generate Object Module
# ---------------------------------------------------------------------------
$(DBSTRESSTEST): $(DAEMSRC)/$(@F:.o=.cc)
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
	@make -f real2.mk cleanDBStressTest

cleanDBStressTest:
	@echo 'Remove Object Files(cleanDBStressTest)'
	@$(RM) $(TDBSTRESSTEST) $(DBSTRESSTEST) $(COM_OBJFILES)

# ===========================================================================
#	End of Makefile
# ===========================================================================
