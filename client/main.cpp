#include "main.h"
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
	CSender *psm = NULL, *psb = NULL, *psnm = NULL, *psnb = NULL;

    memset(&g, 0, sizeof(wcat_s));              

    signal(SIGINT, sigfunc);                    
    signal(SIGTERM, sigfunc);
    signal(SIGHUP, sigfunc);                    

    read_config();                              

	_d("[MAIN] IP address : %s (main), %s (backup), node : %d\n", g.ip[0], g.ip[1], g.node);

	psm = new CSender();
	psb = new CSender();
	psnm = new CSender();
	psnb = new CSender();

	psm->Create(g.ip[0], 22000, (char*)"alive");
	psb->Create(g.ip[1], 22000, (char*)"alive");

	psnm->Create(g.ip[0], 22000, (char*)"nic");
	psnb->Create(g.ip[1], 22000, (char*)"nic");

    _d("[MAIN] started...\n");                  
                                                
    while(!exit_flag_main) {
        pthread_mutex_lock(&sleepMutex);        
        pthread_cond_wait(&sleepCond, &sleepMutex);  
        pthread_mutex_unlock(&sleepMutex);      
    }
                                                
    _d("[MAIN] Exiting...\n");                  

	SAFE_DELETE(psnb);
	SAFE_DELETE(psnm);
    SAFE_DELETE(psb);
	SAFE_DELETE(psm);
    _d("[MAIN] Exited...\n");                   

	return 0;
}


int read_config() {
	FILE *fp = fopen("/opt/tnmtech/config.ini", "r");
	if (fp) {
		char name[256];
		char value[256];

		g.nics = 0;
		while(1) {
			int ret = read_line(fp, name, value);
			if (ret < 0) {
				break;
			} else if (ret) {
				//_d("[CONFIG] KEY : %s, VALUE : %s\n", name, value);
				if (strcasestr(name, "main")) {
					sprintf(g.ip[0], "%s", value);
				}
				if (strcasestr(name, "backup")) {
					sprintf(g.ip[1], "%s", value);
				}
				if (strcasestr(name, "node")) {
					if (strcasestr(value, "compute")) {
						g.node = 0;
					} else {
						g.node = 1;
					}
				}
				for (int i=0; i<MAX_NUM_NICS; i++) {
					char nicname[32];
					sprintf(nicname, "nic%d", i);
					if (strcasestr(name, nicname)) {
						sprintf(g.nic[g.nics], "%s", value);
						g.nics++;
					}
				}
			}
		}
		fclose(fp);
	}

	return 0;
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

