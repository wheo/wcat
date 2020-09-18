#ifndef _CORE_H_
#define _CORE_H_

typedef struct
{
	int event; // 0-dead, 1-link down, 2-link up, // by wheo 3-link all down

	char nic[32]; // nic name
} event_s;

class CClient
{
public:
	CClient(void);
	~CClient(void);

	bool Create(const char *ip, const char *name);
	bool Check(const char *ip);

	bool Link(const char *ip, const char *nic, bool isUp);

	void Alive();

	int IsAlive();
	int IsEvent()
	{
		return _event.GetCount();
	};
	char *GetIpAddr()
	{
		return _ip;
	};
	char *GetHostname()
	{
		return _name;
	};

	event_s *GetEvent();
	FILE *fp;

protected:
	char _ip[32];
	char _name[32];

	time_t _time_started;
	time_t _time_last;

	int _alive;

	CMyList _event;
};

class CRecv : public PThread
{
public:
	CRecv(void);
	~CRecv(void);

	bool Create();
	void *GetNext();

	bool IsAliveMain();

	void Check();

	int GetCount()
	{
		return _list.GetCount();
	};

protected:
	CMyList _list;
	POSITION _pos;

	int _sd;

protected:
	void Run();
	void OnTerminate(){};
};

class CCore : public PThread
{
public:
	CCore(void);
	~CCore(void);

	bool Create();

protected:
	void Run();
	void OnTerminate(){};
};

#endif // _CORE_H_
