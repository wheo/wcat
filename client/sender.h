#ifndef _SENDER_H_
#define _SENDER_H_

class CSender : public PThread {
public:
	CSender(void);
	~CSender(void);

	void Create(const char *ip, int port, char* type);

protected:

	char _ip[32];
	char _type[8];
	int _port;
	FILE *fp; 

protected:

	void Run();
	void OnTerminate() {};

	int Check(const char *nic);
};

#endif //_SENDER_H_

