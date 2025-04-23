nohup /home/kodev/daemon_we/we_dcmdserver 4999 &
nohup /home/kodev/daemon_we/we_cmdserver 5001 &
#nohup /home/kodev/daemon_we/we_cmdserver_c 4001
nohup /home/kodev/daemon_we/we_fupserver &
nohup /home/kodev/daemon_we/we_fdnserver &
nohup /home/kodev/daemon_we/we_installserver &
nohup /home/kodev/daemon_we/we_dnlist_server_01 &
ps -ef | grep server
