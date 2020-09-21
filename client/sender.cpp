#include "main.h"
#include "sender.h"

CSender::CSender(void)
{
}

CSender::~CSender(void)
{
	Terminate();
	//fclose(fpLog);
}

void CSender::Create(const char *ip, int port, char *type)
{
	sprintf(_ip, "%s", ip);
	sprintf(_type, "%s", type);
	_port = port;

	//fpLog = fopen("log.txt", "a+");

	Start();
}

void CSender::Run()
{
	sd = -1;
	int len;

	char buff[256]; // 4 (header) | 32 (hostname) | 28 (NIC name) = 64 bytes
	char hostname[256];

	int scount = 1; // per 3sec but send once ASAP
	int ccount = 2; // per 2sec
	int cindex = 0;
	int cflag = 0x1f;

	memset(buff, 0, 256);
	_d("[SENDER] starting...\n");

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
	buff[2] = 'C'; // TWCT : ALIVE state, TWCD : LINK down, TWCU : LINK up, TWCE : br-egress1, br-egress2 All down

	gethostname(hostname, 256);

	len = strlen(hostname);
	memcpy(&buff[4], hostname, len);

	//g.nics : 이더넷 개수 (현재 5개)
	//g.nic[n] : 이더넷 장치 이름
	for (int i = 0; i < g.nics; i++)
	{
		if (Check(g.nic[i]))
		{
			cflag |= (0x1 << i);
		}
		else
		{
			cflag &= ~(0x1 << i);
		}
	}

	_d("[SENDER] started (sendto %s:%d, hostname : %s, type : %s)\n", _ip, _port, hostname, _type);

	while (!m_bExit)
	{
		sleep(1);

		if (strcmp(_type, (char *)"alive") == 0)
		{
			if (scount)
			{
				scount--;
				if (scount == 0)
				{
					buff[3] = 'T';
					memset(&buff[36], 0, 28);

					int sended = sendto(sd, buff, 64, 0, (struct sockaddr *)&sin, sizeof(sin));
					if (sended < 64)
					{
						_d("[SENDER] Failed to send alive(%d)\n", sended);
					}
					else
					{
						_d("[SENDER] Success to send alive(%d) host: %s, port : %d\n", sended, _ip, _port);
					}

					scount = 3;
				}
			}
		}

		if (strcmp(_type, (char *)"nic") == 0)
		{
			if (ccount)
			{
				ccount--;
				if (ccount == 0)
				{
					//int send = 0;
					int index = cindex % g.nics;
					if (Check(g.nic[index]))
					{
						if (cflag & (0x1 << index))
						{
							buff[3] = 'D';
							sprintf(&buff[36], "%s", g.nic[index]);
							_d("[SENDER] %s is LINK_DOWN(%d)\n", g.nic[index], index);
							cflag &= ~(0x1 << index);
							//send = 1;
							Send(buff);

							if (index < 2)
							{
								if ((cflag & 0x3) == 0x00)
								{
									buff[3] = 'E';
									sprintf(&buff[36], "%s", "br-egress-all");
									_d("[SENDER] %s is LINK_DOWN\n", "br-egress-all");
									//send = 2;
									Send(buff);
								}
							}
						}
					}
					else
					{
						if (!(cflag & (0x1 << index)))
						{
							buff[3] = 'U';
							sprintf(&buff[36], "%s", g.nic[index]);
							_d("[SENDER] %s is LINK_UP(%d)\n", g.nic[index], index);
							cflag |= (0x1 << index);
							//PrintFlag(cflag);
							//send = 1;
							Send(buff);

							if (index < 2)
							{
								if (cflag & (0x1) && cflag & (0x2))
								{
									//_d("all up(index : %d), %s : ", index, _ip);
									//PrintFlag(cflag);
									// !0x3 is 00011 br-egress1 up && br-egress2 up
									buff[3] = 'F';
									sprintf(&buff[36], "%s", "br-egress-all");
									_d("[SENDER] %s is LINK_UP\n", "br-egress-all");
									//send = 3;
									Send(buff);
								}
							}
						}
					}

					ccount = 2;
					cindex++;
				}
			}
		}
	}

	_d("[SENDER] exiting...\n");

	close(sd);
	_d("[SENDER] exited...\n");
}

int CSender::Send(char *buff)
{
	int sended = sendto(sd, buff, 64, 0, (struct sockaddr *)&sin, sizeof(sin));
	if (sended < 64)
	{
		_d("[SENDER] (%s) Failed to send link state (%d)\n", &buff[36], sended);
	}
	else
	{
		_d("[SENDER] (%s) Success to send link state (%d)\n", &buff[36], sended);
	}
	return sended;
}

void CSender::PrintFlag(int flag)
{
	std::cout << "flag : ";
	for (int i = MAX_NUM_NICS; i >= 0; i--)
	{
		std::cout << ((flag >> i) & 1);
	}
	std::cout << std::endl;
}

int CSender::Check(const char *nic)
{
	int ret = 0;

	char cmd[256];
	FILE *fp = NULL;

	// node 가 compute 면 0
	if (g.node == 0)
	{
		sprintf(cmd, "ovs-ofctl show %s | grep dpdk -A 4", nic);
	}
	else
	{
		sprintf(cmd, "ethtool %s | grep detected", nic);
	}
	fp = popen(cmd, "r");
	if (fp)
	{
		char buff[1024];
		while (!m_bExit)
		{
			char *line = fgets(buff, sizeof(buff), fp);
			if (line == NULL)
			{
				break;
			}
			if (strcasestr(line, "state"))
			{
				if (strcasestr(line, "LINK_DOWN"))
				{
					ret = 1;
				}
			}
			if (strcasestr(line, "detected"))
			{
				if (strcasestr(line, "no"))
				{
					ret = 1;
				}
			}
		}
		pclose(fp);
	}

	return ret;
}
