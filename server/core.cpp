#include "main.h"
#include "core.h"

CClient::CClient(void)
{
	memset(_ip, 0, 32);
	memset(_name, 0, 32);
	fp = fopen("/opt/tnmtech/check_client.txt", "a+");
}

CClient::~CClient(void)
{
	fclose(fp);
}

bool CClient::Create(const char *ip, const char *name)
{
	sprintf(_ip, "%s", ip);
	sprintf(_name, "%s", name);
	time(&_time_started);
	time(&_time_last);

	_alive = 2;
	return true;
}

bool CClient::Check(const char *ip)
{
	if (strcmp(_ip, ip) == 0)
	{
		time(&_time_last);
		if (_alive == 0)
		{
			_d("[CLIENT] %s is recovering\n", _ip);
			_alive = 1;
		}
		return true;
	}
	return false;
}

bool CClient::Link(const char *ip, const char *nic, bool isUp)
{
	if (strcmp(_ip, ip) == 0)
	{
		event_s *pes = (event_s *)malloc(sizeof(event_s));

		pes->event = isUp ? 2 : 1;
		sprintf(pes->nic, "%s", nic);

		_event.AddTail(pes);
		return true;
	}
	return false;
}

void CClient::Alive()
{
	time_t cur;
	time(&cur);

	struct tm st;
	localtime_r(&cur, &st);

	if ((cur - _time_last) > 3)
	{
		fprintf(fp, "[%04d-%02d-%02d %02d:%02d:%02d] [%s] cur - time_last : %d, _alive : %d\n", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, _ip, cur - _time_last, _alive);
		fflush(fp);
	}

	if ((cur - _time_last) >= 180)
	{
		if (_alive == 2)
		{
			_d("[CLIENT] %s is down\n", _ip);
			_alive = 0;

			event_s *pes = (event_s *)malloc(sizeof(event_s));
			pes->event = 0;
			memset(pes->nic, 0, 32);

			_event.AddTail(pes);
		}
	}
	else
	{
		if (_alive == 1)
		{
			_d("[CLIENT] %s is recovered\n", _ip);
			_alive = 2;
		}
	}
}

int CClient::IsAlive()
{
	time_t cur;
	time(&cur);
	if ((cur - _time_last) >= 3)
	{
		if (_alive == 2)
		{
			_d("[CLIENT] %s(Main) is down\n", _ip);
			_alive = 0;

			return -1;
		}
		return 0;
	}
	else
	{
		if (_alive == 1)
		{
			_d("[CLIENT] %s(Main) is recovered\n", _ip);
			_alive = 2;
		}
	}
	return 1;
}

event_s *CClient::GetEvent()
{
	if (_event.GetCount())
	{
		return (event_s *)_event.RemoveHead();
	}

	return NULL;
}

CRecv::CRecv(void)
{
	_sd = -1;
}

CRecv::~CRecv(void)
{
	if (_sd >= 0)
	{
		close(_sd);
		shutdown(_sd, 2);
	}
	Terminate();
}

bool CRecv::Create()
{
	Start();
	return true;
}

void CRecv::Check()
{
	_pos = _list.GetHeadPosition();
}

void *CRecv::GetNext()
{
	while (!m_bExit && _pos)
	{
		CClient *pc = (CClient *)_list.GetNext(_pos);
		if (strcmp(pc->GetIpAddr(), g.ip_main) == 0)
		{
		}
		else
		{
			pc->Alive();
			if (pc->IsEvent())
			{
				return pc;
			}
		}
	}
	return NULL;
}

bool CRecv::IsAliveMain()
{
	POSITION pos = _list.GetHeadPosition();
	while (pos)
	{
		CClient *pc = (CClient *)_list.GetNext(pos);
		if (strcmp(pc->GetIpAddr(), g.ip_main) == 0)
		{
			if (pc->IsAlive() <= 0)
			{
				return false;
			}
			return true;
		}
	}
	return false;
}

void CRecv::Run()
{
	struct sockaddr_in sin_sv;
	struct sockaddr_in sin_cl;

	char buff[64];

	_sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (_sd < 0)
	{
		_d("[RECV] Failed to create socket\n");
	}

	memset(&sin_sv, 0, sizeof(sin_sv));
	memset(&sin_cl, 0, sizeof(sin_cl));

	sin_sv.sin_family = AF_INET;
	sin_sv.sin_addr.s_addr = htonl(INADDR_ANY);
	sin_sv.sin_port = htons(22000);

	if (bind(_sd, (struct sockaddr *)&sin_sv, sizeof(sin_sv)) < 0)
	{
		_d("[RECV} Failed to bind\n");
	}

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 50000;
	if (setsockopt(_sd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
	{
		perror("Error");
	}

	_d("[RECV] started...\n");

	while (!m_bExit)
	{
		socklen_t clen = sizeof(struct sockaddr_in);
		int recved = recvfrom(_sd, buff, 64, 0, (struct sockaddr *)&sin_cl, &clen);
		if (recved < 0)
		{
			//_d("[RECV] Failed to recvfrom\n");
			continue;
		}

		const char *ip = inet_ntoa(sin_cl.sin_addr);
		CClient *pc = NULL;

		//_d("[%s] recv buf : %s\n", ip, buff);

		//_d("[RECV] received from %s\n", ip);
		POSITION pos = _list.GetHeadPosition();
		while (!m_bExit && pos)
		{
			CClient *pc_temp = (CClient *)_list.GetNext(pos);
			if (buff[3] == 'T')
			{
				if (pc_temp->Check(ip))
				{
					pc = pc_temp;
					break;
				}
			}
			else if (buff[3] == 'U' || buff[3] == 'D')
			{
				if (pc_temp->Link(ip, &buff[36], buff[3] == 'U' ? true : false))
				{
					pc = pc_temp;
					break;
				}
			}
			else
			{
				_d("[RECV] Unknown packet\n");
			}
		}

		if (!pc)
		{
			_d("[RECV] A new client detected...from %s(%s)\n", inet_ntoa(sin_cl.sin_addr), &buff[4]);
			pc = new CClient();
			pc->Create(ip, &buff[4]);

			_list.AddTail(pc);
		}
	}

	_d("[RECV] exiting...\n");

	if (_list.GetCount())
	{
		POSITION pos = _list.GetHeadPosition();
		while (pos)
		{
			CClient *pc = (CClient *)_list.GetNext(pos);
			SAFE_DELETE(pc);
		}
		_list.RemoveAll();
	}
	_d("[RECV] exited...\n");
}

CCore::CCore(void)
{
}

CCore::~CCore(void)
{
	Terminate();
}

bool CCore::Create()
{
	Start();

	return true;
}

void CCore::Run()
{
	int count = 10;
	bool bIsSendAlt = false;

	struct tm st;

	int64_t elapsed;
	CRecv *pr = new CRecv;

	FILE *fp = fopen("/opt/tnmtech/log.txt", "w");

	pr->Create();

	time(&g.begin);

	localtime_r(&g.begin, &st);

	_d("[CORE] started...\n");
	fprintf(fp, "[%04d-%02d-%02d %02d:%02d:%02d] agent starting...\n", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
	fflush(fp);

	while (!m_bExit)
	{
		time_t cur;
		//std::this_thread::sleep_for(std::chrono::seconds(1));
		//sleep(1);
		usleep(500000);

		time(&cur);
		localtime_r(&cur, &st);
		elapsed = cur - g.begin;

		if (count)
		{
			count--;
			if (count == 0)
			{
				_d("[CORE] %ld sec elapsed, %d clients registered\n", elapsed, pr->GetCount());
				count = 10;
			}
		}

		if (g.role == 1)
		{
			if (pr->IsAliveMain())
			{
				if (bIsSendAlt)
				{
					_d("[CORE] Main.App is recovered... so Backup.App going to be offline\n");
					fprintf(fp, "[%04d-%02d-%02d %02d:%02d:%02d] agent(MAIN) is online...\n", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
					fflush(fp);
					bIsSendAlt = false;
				}
			}
			else
			{
				if (bIsSendAlt == false)
				{
					_d("[CORE] Main.App is out of control... so Backup.App going to be online\n");
					fprintf(fp, "[%04d-%02d-%02d %02d:%02d:%02d] agent(MAIN) is offline...\n", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
					fflush(fp);
					bIsSendAlt = true;
				}
			}
		}

		pr->Check();
		while (!m_bExit)
		{
			CClient *pc = (CClient *)pr->GetNext();
			//_d("----- event check get next -----\n");
			if (pc)
			{
				char msg[512];
				char cmd[512];

				while (!m_bExit)
				{
					event_s *pes = pc->GetEvent();
					if (pes)
					{
						if ((g.role == 1 && bIsSendAlt) || (g.role == 0))
						{
							_d("[CORE] Send Event.%d to %s (%s@%s)\n", pes->event, g.ip, pc->GetHostname(), pc->GetIpAddr());
							if (pes->event == 0)
							{
								sprintf(msg, "\'%s|HOST.Shutdown|%s %s Host was Shutdown\'", pc->GetIpAddr(), pc->GetIpAddr(), pc->GetHostname());
							}
							else if (pes->event == 1)
							{
								sprintf(msg, "\'%s|HOST.Nic3ConnectivityLost.%s.down|%s %s Host %s NIC down.\'", pc->GetIpAddr(), pes->nic, pc->GetIpAddr(), pc->GetHostname(), pes->nic);
							}
							else if (pes->event == 2)
							{
								sprintf(msg, "\'%s|HOST.Nic3ConnectivityLost.%s.up|%s %s Host %s NIC up.\'", pc->GetIpAddr(), pes->nic, pc->GetIpAddr(), pc->GetHostname(), pes->nic);
							}
							else
							{
								_d("[CORE] Unknown event\n");
								break;
							}
#if 1
							sprintf(cmd, "logger %s", msg);
							system(cmd);
#endif

							fprintf(fp, "[%04d-%02d-%02d %02d:%02d:%02d] send via logger > %s\n", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, msg);
							fflush(fp);
							_d("CMD : %s\n", cmd);
						}
						free(pes);
					}
					else
					{
						break;
					}
				}
			}
			else
			{
				break;
			}
		}
	}

	_d("[CORE] exiting...\n");
	SAFE_DELETE(pr);
	_d("[CORE] exited...\n");

	fclose(fp);
}
