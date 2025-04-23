nohup /root/daemon_we/dcmdserver 4999 &
nohup /root/daemon_we/cmdserver 5001 &
#nohup /root/daemon_we/cmdserver_c 4001
nohup /root/daemon_we/fupserver 5005 &
nohup /root/daemon_we/fdnserver 5003 &
nohup /root/daemon_we/we_dnlist_server_01 &
ps -ef | grep server
