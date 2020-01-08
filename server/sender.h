#ifndef _SENDER_H_
#define _SENDER_H_

class CSender : public PThread {
public:
	CSender(void);
	~CSender(void);

	void Create(const char *ip, int port);

protected:

	char _ip[32];
	int _port;

protected:

	void Run();
	void OnTerminate() {};
};

#endif //_SENDER_H_

