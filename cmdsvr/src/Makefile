# ---------------------------------------------------------------------------
#	Directory Information
# ---------------------------------------------------------------------------
DSVRSRC = $(HOME)/cmdsvr/src
DSVRBIN = $(HOME)/cmdsvr/bin
DSVROBJ = $(HOME)/cmdsvr/obj
DSVRINC = $(HOME)/cmdsvr/inc
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
CC_CMDMAIN  = cmdmain.cc
CO_CMDMAIN  = $(DSVROBJ)/$(CC_CMDMAIN:.cc=.o)

CC_CMDPROC  = cmdproc.cc
CO_CMDPROC  = $(DSVROBJ)/$(CC_CMDPROC:.cc=.o)

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

CC_CMDS1002 = cmds1002.cc
CO_CMDS1002 = $(DSVROBJ)/$(CC_CMDS1002:.cc=.o)

CC_CMDS1003 = cmds1003.cc
CO_CMDS1003 = $(DSVROBJ)/$(CC_CMDS1003:.cc=.o)

CC_CMDS1004 = cmds1004.cc
CO_CMDS1004 = $(DSVROBJ)/$(CC_CMDS1004:.cc=.o)

CC_CMDS2001 = cmds2001.cc
CO_CMDS2001 = $(DSVROBJ)/$(CC_CMDS2001:.cc=.o)
#######################################################################
CO_OBJFILES = \
	$(CO_CMDMAIN) \
	$(CO_CMDPROC) \
	$(CO_CMDMYDB) \
	$(CO_CMDCONF) \
	$(CO_CMDTHRD) \
	$(CO_CMDCOMM) \
	$(CO_CMDS1001) \
	$(CO_CMDS1002) \
	$(CO_CMDS1003) \
	$(CO_CMDS1004) \
	$(CO_CMDS2001) 
			

# ---------------------------------------------------------------------------
#	Generate Main Target Module
# ---------------------------------------------------------------------------
TARGET = $(DSVRBIN)/cmdserver

all: cmdserver

# ---------------------------------------------------------------------------
#	Generate Each Target Module
# ---------------------------------------------------------------------------
cmdserver:	$(TARGET)

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
	@make cleancmdserver
cleancmdserver:
	@echo 'Remove Object Files(cleancmdserver)'
	@$(RM) $(TARGET) $(CO_OBJFILES)

# ===========================================================================
#	End of Makefile
# ===========================================================================
