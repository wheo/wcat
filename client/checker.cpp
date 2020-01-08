#include "main.h"
#include "checker.h"

CChecker::CChecker(void)
{
	_port = 22001;
}

CChecker::~CChecker(void)
{
	Terminate();
}

void CChecker::Create(const char *ip, const char *nic)
{
	sprintf(_ip, "%s", ip);
	sprintf(_nic, "%s", nic);

	_d("[CHECKER] created... for %s@%s\n", nic, ip);
	Start();
}

void CChecker::Run()
{
	int sd = -1;
	int len;
	struct sockaddr_in sin;

	char buff[256];
	char hostname[256];

	memset(buff, 0, 256);

	_d("[CHECKER] starting...\n");

	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0)
	{
		_d("[SENDER] Failed to create socket\n");
	}

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(_ip);
	sin.sin_port = htons(_port);

	buff[0] = 'T';
	buff[1] = 'W';
	buff[2] = 'C';
	buff[3] = 'L';

	gethostname(hostname, 256);

	len = strlen(hostname);
	memcpy(&buff[4], &len, 4);
	memcpy(&buff[8], hostname, len);

	_d("[CHECKER] started...\n");
	while (!m_bExit)
	{
		sleep(2);

		int ret = Check();
		if (ret < 0)
		{
			_d("[CHECKER] Critical!!!\n");
		}
		if (ret)
		{
			_d("[CHECKER] LINK_DOWN detected at %s...\n", _nic);
		}
	}

	_d("[CHECKER] exiting...\n");
	close(sd);
	_d("[CHECKER] exited...\n");
}

int CChecker::Check()
{
	int ret = 0;

	char cmd[256];
	FILE *fp = NULL;

	sprintf(cmd, "ovs-ofctl show %s | grep dpdk -A 4", _nic);
	fp = popen(cmd, "r");
	if (fp)
	{
		int phys = -1;
		char buff[1024];
		while (!m_bExit)
		{
			char *line = fgets(buff, sizeof(buff), fp);
			if (line == NULL)
			{
				break;
			}
			if (strstr(line, "dpdk"))
			{
				phys = atoi(line);
			}
			if (strcasestr(line, "state"))
			{
				if (strcasestr(line, "LINK_DOWN"))
				{
					_d("NIC %s.%d > state : LINK_DOWN\n", _nic, phys);
					ret = 1;
				}
			}
		}
		pclose(fp);
	}

	return ret;
}
