#ifdef __DEBUG
// DEBUG =======================================
	#define OSP_DB_DCMD_USER		"dmondcmd"
	#define OSP_DB_DCMD_PASS		"fnehfvm)*^"

	#define OSP_DB_IP		"192.168.0.38"
	#define OSP_DB_IP_PUB	"183.110.44.20"
	#define OSP_DB_IP_BCK	"192.168.171.37"
	#define OSP_DB_NAME		"zangsi"
	#define OSP_DB_USER		"dmondcmd"
	#define OSP_DB_PASS		"fnehfvm)*^"
	#define OSP_DB_PORT		3306

	#define OSP_TONG_DB_IP	 "183.110.46.38"
	#define OSP_TONG_DB_NAME "cnfs"
	#define OSP_TONG_DB_USER "dmondcmd"
	#define OSP_TONG_DB_PASS "fnehfvm)*^"

	#define OSP_LOG_DB_IP_PUB	"183.110.44.33"
	#define OSP_LOG_DB_NAME		"zangsi"
	#define OSP_LOG_DB_USER		"dmondcmd"
	#define OSP_LOG_DB_PASS		"fnehfvm)*^"

	#define OSP_CPR_DB_IP		"192.168.0.129"
	#define OSP_CPR_DB_IP_PUB	"183.110.44.28"
	#define OSP_CPR_DB_NAME		"zangsi_cpr"
	#define OSP_CPR_DB_USER		"dmondcmd"
	#define OSP_CPR_DB_PASS		"fnehfvm)*^"

	#define CMD_SERVER_IP		"183.110.44.95"
	#define REMOTE_SERVER_IP	"183.110.44.95"
	#define REMOTE_CLIENT_PORT	5003
	#define DB_COMMAND_SERVER_IP	"192.168.1.254" // 중개서버 위치
	#define DB_COMMAND_SERVER_PORT	4999
	#define DB_SUB_COMMAND_SERVER_PORT	4998
	#define DB_SUB_COMMAND_SERVER_IP	"192.168.1.253" // SUB 중개서버 위치

	#define TONG_DB_NAME "cnfs"
	#define TONG_DB_USER "fanoman"
	#define TONG_DB_PASS "akqthtk)(^"
// END DEBUG =======================================
#elif __QCTEST
// QC =======================================
	#define OSP_DB_DCMD_USER		"dmondcmd"
	#define OSP_DB_DCMD_PASS		"fnehfvm)*^"

	#define OSP_DB_IP		"183.110.44.118" // 183.110.46.120 : 제휴 , 183.110.44.118 : QC
	#define OSP_DB_IP_PUB	"183.110.44.118"
	#define OSP_DB_IP_BCK	"183.110.44.118"
	#define OSP_DB_NAME		"zangsi"
	#define OSP_DB_USER		"dmondcmd"
	#define OSP_DB_PASS		"fnehfvm)*^"
	#define OSP_DB_PORT		3306

	#define OSP_TONG_DB_IP	 "183.110.46.120" // 통합 BCK
	#define OSP_TONG_DB_NAME "cnfs"
	#define OSP_TONG_DB_USER "dmondcmd"
	#define OSP_TONG_DB_PASS "fnehfvm)*^"

	#define OSP_LOG_DB_IP_PUB	"183.110.44.118"
	#define OSP_LOG_DB_NAME		"zangsi"
	#define OSP_LOG_DB_USER		"dmondcmd"
	#define OSP_LOG_DB_PASS		"fnehfvm)*^"

	#define OSP_CPR_DB_IP		"183.110.44.118"
	#define OSP_CPR_DB_IP_PUB	"183.110.44.118"
	#define OSP_CPR_DB_NAME		"zangsi_cpr"
	#define OSP_CPR_DB_USER		"dmondcmd"
	#define OSP_CPR_DB_PASS		"fnehfvm)*^"

	#define CMD_SERVER_IP		 "183.110.44.124"
	#define REMOTE_SERVER_IP	 "183.110.44.124"
	#define REMOTE_CLIENT_PORT	 5003
	#define DB_COMMAND_SERVER_IP	"183.110.44.124"
	#define DB_COMMAND_SERVER_PORT	 4999
	#define DB_SUB_COMMAND_SERVER_IP	 "183.110.44.124"
	#define DB_SUB_COMMAND_SERVER_PORT	 4998

	#define TONG_DB_NAME "cnfs"
	#define TONG_DB_USER "fanoman"
	#define TONG_DB_PASS "akqthtk)(^"
// END QC =======================================
#elif __TONGTEST
// TONG =======================================
	#define OSP_DB_DCMD_USER		"dmondcmd"
	#define OSP_DB_DCMD_PASS		"fnehfvm)*^"

	#define OSP_DB_IP		"183.110.46.120"
	#define OSP_DB_IP_PUB	"183.110.46.120"
	#define OSP_DB_IP_BCK	"183.110.46.120"
	#define OSP_DB_NAME		"zangsi"
	#define OSP_DB_USER		"dmondcmd"
	#define OSP_DB_PASS		"fnehfvm)*^"
	#define OSP_DB_PORT		3306

	#define OSP_TONG_DB_IP	 "183.110.46.120"
	#define OSP_TONG_DB_NAME "cnfs"
	#define OSP_TONG_DB_USER "dmondcmd"
	#define OSP_TONG_DB_PASS "fnehfvm)*^"

	#define OSP_LOG_DB_IP_PUB	"183.110.46.120"
	#define OSP_LOG_DB_NAME		"zangsi"
	#define OSP_LOG_DB_USER		"dmondcmd"
	#define OSP_LOG_DB_PASS		"fnehfvm)*^"

	#define OSP_CPR_DB_IP		"183.110.46.120" // 183.110.44.120
	#define OSP_CPR_DB_IP_PUB	"183.110.46.120" // 183.110.44.120
	#define OSP_CPR_DB_NAME		"zangsi_cpr"
	#define OSP_CPR_DB_USER		"dmondcmd"
	#define OSP_CPR_DB_PASS		"fnehfvm)*^"

	#define CMD_SERVER_IP		 "222.239.92.193"
	#define REMOTE_SERVER_IP	 "222.239.92.193"
	#define REMOTE_CLIENT_PORT	 5003
	#define DB_COMMAND_SERVER_IP	"222.239.92.193"
	#define DB_COMMAND_SERVER_PORT	 4999
	#define DB_SUB_COMMAND_SERVER_IP	 "222.239.92.193"
	#define DB_SUB_COMMAND_SERVER_PORT	 4998

	#define TONG_DB_NAME "cnfs"
	#define TONG_DB_USER "fanoman"
	#define TONG_DB_PASS "akqthtk)(^"
// END TONG =======================================
#elif __LOCALTEST
// LOCAL =======================================
	#define OSP_DB_DCMD_USER		"dmondcmd"
	#define OSP_DB_DCMD_PASS		"fnehfvm)*^"

	#define OSP_DB_IP		"172.16.9.212"
	#define OSP_DB_IP_PUB	"172.16.9.212"
	#define OSP_DB_IP_BCK	"172.16.9.212"
	#define OSP_DB_NAME		"zangsi"
	#define OSP_DB_USER		"dmondcmd"
	#define OSP_DB_PASS		"fnehfvm)*^"
	#define OSP_DB_PORT		3306

	#define OSP_TONG_DB_IP	 "172.16.9.212"
	#define OSP_TONG_DB_NAME "cnfs"
	#define OSP_TONG_DB_USER "dmondcmd"
	#define OSP_TONG_DB_PASS "fnehfvm)*^"

	#define OSP_LOG_DB_IP_PUB	"172.16.9.212"
	#define OSP_LOG_DB_NAME		"zangsi"
	#define OSP_LOG_DB_USER		"dmondcmd"
	#define OSP_LOG_DB_PASS		"fnehfvm)*^"

	#define OSP_CPR_DB_IP		"172.16.9.212" //
	#define OSP_CPR_DB_IP_PUB	"172.16.9.212" //
	#define OSP_CPR_DB_NAME		"zangsi_cpr"
	#define OSP_CPR_DB_USER		"dmondcmd"
	#define OSP_CPR_DB_PASS		"fnehfvm)*^"

	#define CMD_SERVER_IP		 "172.16.9.193" // CMD 서버 위치
	#define REMOTE_SERVER_IP	 "172.16.9.193" // 중복 체크 데몬 위치 ,
	#define REMOTE_CLIENT_PORT	 5003
	#define DB_COMMAND_SERVER_IP	"172.16.9.193"
	#define DB_COMMAND_SERVER_PORT	 4999
	#define DB_SUB_COMMAND_SERVER_IP	 "172.16.9.193"
	#define DB_SUB_COMMAND_SERVER_PORT	 4998

	#define TONG_DB_NAME "cnfs"
	#define TONG_DB_USER "fanoman"
	#define TONG_DB_PASS "akqthtk)(^"
// END LOCAL =======================================
#elif __LOCALTEST_211
// LOCAL =======================================
	#define OSP_DB_DCMD_USER		"dmondcmd"
	#define OSP_DB_DCMD_PASS		"fnehfvm)*^"

	#define OSP_DB_IP		"172.16.9.211"
	#define OSP_DB_IP_PUB	"172.16.9.211"
	#define OSP_DB_IP_BCK	"172.16.9.211"
	#define OSP_DB_NAME		"zangsi"
	#define OSP_DB_USER		"dmondcmd"
	#define OSP_DB_PASS		"fnehfvm)*^"
	#define OSP_DB_PORT		3306

	#define OSP_TONG_DB_IP	 "172.16.9.211"
	#define OSP_TONG_DB_NAME "cnfs"
	#define OSP_TONG_DB_USER "dmondcmd"
	#define OSP_TONG_DB_PASS "fnehfvm)*^"

	#define OSP_LOG_DB_IP_PUB	"172.16.9.211"
	#define OSP_LOG_DB_NAME		"zangsi"
	#define OSP_LOG_DB_USER		"dmondcmd"
	#define OSP_LOG_DB_PASS		"fnehfvm)*^"

	#define OSP_CPR_DB_IP		"172.16.9.211" //
	#define OSP_CPR_DB_IP_PUB	"172.16.9.211" //
	#define OSP_CPR_DB_NAME		"zangsi_cpr"
	#define OSP_CPR_DB_USER		"dmondcmd"
	#define OSP_CPR_DB_PASS		"fnehfvm)*^"

	#define CMD_SERVER_IP		 "172.16.9.193" // CMD 서버 위치
	#define REMOTE_SERVER_IP	 "172.16.9.193" // 중복 체크 데몬 위치 ,
	#define REMOTE_CLIENT_PORT	 5003
	#define DB_COMMAND_SERVER_IP	"172.16.9.193"
	#define DB_COMMAND_SERVER_PORT	 4999
	#define DB_SUB_COMMAND_SERVER_IP	 "172.16.9.193"
	#define DB_SUB_COMMAND_SERVER_PORT	 4998

	#define TONG_DB_NAME "cnfs"
	#define TONG_DB_USER "fanoman"
	#define TONG_DB_PASS "akqthtk)(^"
// END LOCAL2_11 =======================================
#else
// REAL =======================================
	#define OSP_DB_DCMD_USER		"dmondcmd"
	#define OSP_DB_DCMD_PASS		"fnehfvm)*^"

	#define OSP_DB_IP		"192.168.0.38"
	#define OSP_DB_IP_PUB	"49.236.131.20"
	#define OSP_DB_IP_BCK	"192.168.171.37"
	#define OSP_DB_NAME		"zangsi"
	#define OSP_DB_USER		"dmondcmd"
	#define OSP_DB_PASS		"fnehfvm)*^"
	#define OSP_DB_PORT		3306

	#define OSP_TONG_DB_IP	 "110.79.133.38"
	#define OSP_TONG_DB_NAME "cnfs"
	#define OSP_TONG_DB_USER "dmondcmd"
	#define OSP_TONG_DB_PASS "fnehfvm)*^"

	#define OSP_LOG_DB_IP_PUB	"49.236.131.33"
	#define OSP_LOG_DB_NAME		"zangsi"
	#define OSP_LOG_DB_USER		"dmondcmd"
	#define OSP_LOG_DB_PASS		"fnehfvm)*^"

	#define OSP_CPR_DB_IP		"192.168.0.129"
	#define OSP_CPR_DB_IP_PUB	"49.236.131.28"
	#define OSP_CPR_DB_NAME		"zangsi_cpr"
	#define OSP_CPR_DB_USER		"dmondcmd"
	#define OSP_CPR_DB_PASS		"fnehfvm)*^"

	#define CMD_SERVER_IP		"49.236.131.95"
	#define REMOTE_SERVER_IP	"49.236.131.95"
	#define REMOTE_CLIENT_PORT	5003
	#define DB_COMMAND_SERVER_IP	"192.168.0.254"
	#define DB_COMMAND_SERVER_PORT	4999
	#define DB_SUB_COMMAND_SERVER_PORT	4998
	#define DB_SUB_COMMAND_SERVER_IP	"192.168.0.253"

	#define TONG_DB_NAME "cnfs"
	#define TONG_DB_USER "fanoman"
	#define TONG_DB_PASS "akqthtk)(^"
// END REAL =======================================
#endif


