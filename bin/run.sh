nohup /root/project1/server/zangsi_with_dcmd/bin/dcmdserver 4999 &
nohup /root/project1/server/zangsi_with_dcmd/bin/cmdserver 5001 &
#nohup /root/zangsi/bin/cmdserver_c 4001
nohup /root/project1/server/zangsi_with_dcmd/bin/fupserver 5005 &
ps -ef | grep server