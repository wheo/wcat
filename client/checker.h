#ifndef _CHECKER_H_
#define _CHECKER_H_

class CChecker : public PThread
{
public:
	CChecker(void);
	~CChecker(void);

	void Create(const char *ip, const char *nic);

protected:
	char _nic[32];
	char _ip[32];

	int _port;

protected:
	void Run();
	void OnTerminate(){};

	int Check();
};

#endif // _CHECKER_H_
