#include "main.h"
#include "sender.h"

CSender::CSender(void)
{
}

CSender::~CSender(void)
{
	Terminate();
}

void CSender::Create(const char *ip, int port)
{
	sprintf(_ip, "%s", ip);
	_port = port;

	Start();
}

void CSender::Run()
{
	int sd = -1;
	struct sockaddr_in sin;

	char buff[64];

	memset(buff, 0, 64);
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
	buff[2] = 'C';
	buff[3] = 'T';

	_d("[SENDER] started (sendto %s:%d)\n", _ip, _port);

	while (!m_bExit)
	{
		sleep(1);

		int sended = sendto(sd, buff, 64, 0, (struct sockaddr *)&sin, sizeof(sin));
		if (sended < 64)
		{
			_d("[SENDER} Failed to send (%d)\n", sended);
		}
	}

	_d("[SENDER] exiting...\n");
	close(sd);
	_d("[SENDER] exited...\n");
}
