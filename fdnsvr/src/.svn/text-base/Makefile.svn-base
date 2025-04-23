# ---------------------------------------------------------------------------
#	Directory Information
# ---------------------------------------------------------------------------
DSVRSRC = $(HOME)/fdnsvr/src
DSVRBIN = $(HOME)/fdnsvr/bin
DSVROBJ = $(HOME)/fdnsvr/obj
DSVRINC = $(HOME)/fdnsvr/inc
MYDBINC = /usr/local/mysql/include/mysql
MYDBLIB = /usr/local/mysql/lib/mysql

# ---------------------------------------------------------------------------
#	Compiler Information & Copy Remove Options
# ---------------------------------------------------------------------------
CC      = g++
RM      = rm -f

# ---------------------------------------------------------------------------
#	Source List Information 
# ---------------------------------------------------------------------------
CC_FDNMAIN  = fdnmain.cc
CO_FDNMAIN  = $(DSVROBJ)/$(CC_FDNMAIN:.cc=.o)

CC_FDNPROC  = fdnproc.cc
CO_FDNPROC  = $(DSVROBJ)/$(CC_FDNPROC:.cc=.o)

CC_CMDMYDB  = commydb.cc
CO_CMDMYDB  = $(DSVROBJ)/$(CC_CMDMYDB:.cc=.o)

CC_CMDCONF  = cmdconf.cc
CO_CMDCONF  = $(DSVROBJ)/$(CC_CMDCONF:.cc=.o)

CC_CMDTHRD  = apthread.cc
CO_CMDTHRD  = $(DSVROBJ)/$(CC_CMDTHRD:.cc=.o)

CC_CMDCOMM  = cmdcomm.cc
CO_CMDCOMM  = $(DSVROBJ)/$(CC_CMDCOMM:.cc=.o)

CC_CMDS1001 = cmds1001.cc
CO_CMDS1001 = $(DSVROBJ)/$(CC_CMDS1001:.cc=.o)

#######################################################################
CO_OBJFILES = \
	$(CO_FDNMAIN) \
	$(CO_FDNPROC) \
	$(CO_CMDMYDB) \
	$(CO_CMDCONF) \
	$(CO_CMDTHRD) \
	$(CO_CMDCOMM) 
			

# ---------------------------------------------------------------------------
#	Generate Main Target Module
# ---------------------------------------------------------------------------
TARGET = $(DSVRBIN)/fdnserver

all: fdnserver

# ---------------------------------------------------------------------------
#	Generate Each Target Module
# ---------------------------------------------------------------------------
fdnserver:	$(TARGET)

$(TARGET)	: $(CO_OBJFILES)
	@echo '=================================================================='
	@echo 'Create server... :' $@
	@echo '=================================================================='
	@$(CC) -o $@ $(CO_OBJFILES) -I. -I$(DSVRINC) -I$(MYDBINC) -D_THREAD_MODULE_ -D_THREAD_SAFE -D_REENTRANT -L$(MYDBLIB) -lmysqlclient -lz -lm -lcrypt -static -lpthread

# ---------------------------------------------------------------------------
#	Generate Object Module
# ---------------------------------------------------------------------------

$(CO_OBJFILES): $(DSVRSRC)/$(@F:.o=.cc)
	@echo 'Compile Source :' $(DSVRSRC)/$(@F:.o=.cc)
	@$(CC) -c $(DSVRSRC)/$(@F:.o=.cc) -o $@ -I. -I$(DSVRINC) -I$(MYDBINC) -D_THREAD_MODULE_ -D_THREAD_SAFE -D_REENTRANT -L$(MYDBLIB) -static

# ---------------------------------------------------------------------------
#	Clean Objects Module
# ---------------------------------------------------------------------------
cleanall:
	@echo 'Remove All Object Files'
	@make cleanfdnserver
cleanfdnserver:
	@echo 'Remove Object Files(cleanmfdnserver)'
	@$(RM) $(TARGET) $(CO_OBJFILES)

# ===========================================================================
#	End of Makefile
# ===========================================================================
