#include "main.h"
#include "core.h"
#include "sender.h"

bool exit_flag_main = false;

pthread_mutex_t sleepMutex;
pthread_cond_t sleepCond;

void sigfunc(int signum) {
	if (signum == SIGINT || signum == SIGTERM) {
		exit_flag_main = true;
	}
	pthread_cond_signal(&sleepCond);
}

wcat_s g;

int read_config();
int read_line(FILE *fp, char *name, char *value);

int main(int argc, char *argv[]) {
	CCore *pc = NULL;
	CSender *psm = NULL;

	memset(&g, 0, sizeof(wcat_s));

	signal(SIGINT, sigfunc);
	signal(SIGTERM, sigfunc);
	signal(SIGHUP, sigfunc);

	read_config();

	_d("[MAIN] Role = %s, IP address = %s(monitoring server), IP = %s/%s(main/backup)\n", g.role ? "backup" : "main", g.ip, g.ip_main, g.ip_backup);

	pc = new CCore();
	pc->Create();

	if (g.role == 0) {
		psm = new CSender();
		psm->Create(g.ip_backup, 22000);
	}

	_d("[MAIN] started...\n");

	while(!exit_flag_main) {
		pthread_mutex_lock(&sleepMutex);
		pthread_cond_wait(&sleepCond, &sleepMutex);
		pthread_mutex_unlock(&sleepMutex);
	}

	_d("[MAIN] Exiting...\n");
	SAFE_DELETE(psm);
	SAFE_DELETE(pc);
	_d("[MAIN] Exited...\n");

	return 0;
}

int read_config() {
	FILE *fp = fopen("/opt/tnmtech/config.ini", "r");
	if (fp) {
		char name[256];
		char value[256];

		while(1) {
			int ret = read_line(fp, name, value);
			if (ret < 0) {
				break;
			} else if (ret) {
				if (strcasestr(name, "role")) {
					if (strcasecmp(value, "main") == 0) {
						g.role = 0;
					} else {
						g.role = 1;
					}
				}
				if (strcasestr(name, "monitor")) {
					sprintf(g.ip, "%s", value);
				} 
				if (strcasestr(name, "main")) {
					sprintf(g.ip_main, "%s", value);
				}
				if (strcasestr(name, "backup")) {
					sprintf(g.ip_backup, "%s", value);
				}
				//_d("[CONFIG] KEY : %s, VALUE : %s\n", name, value);
			}
		}
		fclose(fp);
	}
}

int read_line(FILE *fp, char *name, char *value) {
	int count = 0;
	int consumed = 0;
	int phase = 0;
	for (int i=0; i<256; i++) {
		char ch = fgetc(fp);
		if (ch == EOF) {
			//_d("[CONFIG] meet EOF\n");
			return -1;
		}
		if (ch == '[') {
			phase = 2;
		} else if (ch == '=' || ch ==':') {
			phase = 1;
			name[count++] = 0;
		} else if (ch == ']') {
			phase = 3;
			value[consumed++] = 0;
		} else if (ch == '#') {
			phase = 4;
		} else if (phase == 0) {
			name[count++] = ch;
		} else if (phase == 2) {
			value[consumed++] = ch;
		} else if (ch == '\n' && phase==4) {
			break;
		} else if (ch == '\n' && phase==3) {
			break;
		}
	}

	return consumed;
}
