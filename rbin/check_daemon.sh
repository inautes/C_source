#!/bin/bash
#################################################################
#variable
LIVE_CNT=0;
##################################################################
#argument
# input daemon process name to check_list
if [ -z "$1" ] || [ -z "$2" ]  # 명령어줄 인자 여부의 표준 확인 작업.
then
  echo "사용법: check..sh $0  processname daemon_path";
  exit -1;
fi

PROCESS_NAME=$1
DAEMON_PATH=$2   
SERVER_PORT=$3
OPTION=$4
#################################################################

echo "$0 $1 $2 $3 $4";

GetProcessCount()
{
  if [ "$SERVER_PORT" -gt "0" ]
  then
  	LIVE_CNT=`ps -ef | grep $PROCESS_NAME | grep $SERVER_PORT | grep -vP "$0|grep" | wc -l`
	else
        LIVE_CNT=`ps -ef | grep $PROCESS_NAME | grep -vP "$0|grep" | wc -l`
  fi
}

KillProcess()
{
 GetProcessCount
 if [ "$LIVE_CNT" -gt "0" ]
 then
 ps ef | grep $PROCESS_NAME | grep -vP "$0|grep" | awk '{ print $2 }' | xargs kill -9;
 #echo "kill Process $PROCESS_NAME";
 fi


}
StartProcess()
{
	PROCESS="$DAEMON_PATH$PROCESS_NAME $SERVER_PORT $OPTION"
	nohup $PROCESS &
}
###################################################################

GetProcessCount

if [ "$LIVE_CNT" -gt "0" ]
  then
  echo "already run $PROCESSNAME ... $LIVE_CNT "
  else
  StartProcess
fi
