nohup /home/kodev/daemon_we_211/we_dcmdserver 4999 &
nohup /home/kodev/daemon_we_211/we_cmdserver 5001 &
#nohup /home/kodev/daemon_we_211/we_cmdserver_c 4001
nohup /home/kodev/daemon_we_211/we_fupserver &
nohup /home/kodev/daemon_we_211/we_fdnserver &
nohup /home/kodev/daemon_we_211/we_installserver &
nohup /home/kodev/daemon_we_211/we_dnlist_server_01 &
ps -ef | grep server
