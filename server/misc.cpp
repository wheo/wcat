#include "main.h"
#include "misc.h"

#define WEB_LOG_PATH "/home/ciel/work/docker/tme8/code/tnmlogs/"

FILE *g_fpLog = NULL;
char compareDate[16] = {0};

double rnd(double x, int digit)
{
	return (floor((x)*pow(float(10), digit) + 0.5f)/pow(float(10), digit));
}

void _log(const char *format, ...)
{                                                 
	time_t ct;
	struct tm *t;                                 
	char currentDate[16] = {0};

	time(&ct);                                    
	t = localtime(&ct);                           

	sprintf(currentDate, "%04d%02d%02d", t->tm_year+1900, t->tm_mon + 1, t->tm_mday);

	if (strcmp(currentDate, compareDate)) {
		char filepath[256];
		char filename[32];
		char yesterdayDate[16] = {0};

		sprintf(filename, "%s.log", (char*)"tme8");
		sprintf(filepath, "%s%s", WEB_LOG_PATH, filename); 

		time(&ct);                                    
		ct = ct - (24*60*60);
		t = localtime(&ct);
		sprintf(yesterdayDate, "%04d%02d%02d", t->tm_year+1900, t->tm_mon + 1, t->tm_mday);

		if (g_fpLog) {
			char renamepath[256];
			sprintf(renamepath, "%s%s-%s.log", WEB_LOG_PATH, (char*)"tme8", yesterdayDate);

			fflush(g_fpLog);
			fclose(g_fpLog);
			g_fpLog = NULL;
			if ( -1 == rename( filepath, renamepath ) ) {
				//Fail Message
			}
		}
		sprintf(compareDate, "%s", currentDate);
		if (!g_fpLog) {
			g_fpLog = fopen(filepath, "a+");        
		}
	}

	if (g_fpLog) {
		va_list argptr;                               
		char buf[1024];                               

		va_start(argptr, format);                     
		vsprintf(buf, format, argptr);                

		fprintf(g_fpLog, "[%04d-%02d-%02d %02d:%02d:%02d] %s", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, buf);
		fflush(g_fpLog);                              

		va_end(argptr);
	}
}                                                 

unsigned long GetTickCount()
{
    timeval tv;
    static time_t sec = timeStart.tv_.tv_sec;
    static time_t usec = timeStart.tv_.tv_usec;

    gettimeofday(&tv, NULL);
    return (tv.tv_sec - sec)*1000 + (tv.tv_usec - usec)/1000;
}

PThread::PThread(char *a_szName , THREAD_EXIT_STATE a_eExitType)
{
	m_nID = 0;
	m_eState = eREADY;
	m_eExitType = a_eExitType;
	m_szName = NULL;
	m_bExit = false;
	if (a_szName) {
		int nLength =strlen(a_szName);
		m_szName = new char[nLength + 1];
		strcpy(m_szName, a_szName);
	}
}

PThread::~PThread()
{
	if (m_eState == eZOMBIE) {
		Join(m_nID);
	}

	if (m_szName)
		delete []m_szName;
}

int PThread::Start()
{
	int nResult = 0;
	m_bExit = false;
	if ((pthread_create(&m_nID, NULL, StartPoint, reinterpret_cast<void *>(this))) == 0) {
		if (m_eExitType == eDETACHABLE) {
			pthread_detach(m_nID);
		}
	} else {
		SetState(eABORTED);
		nResult = -1;
	}

	return nResult;
}

void* PThread::StartPoint(void* a_pParam)
{
	PThread* pThis = reinterpret_cast<PThread*>(a_pParam);
	pThis->SetState(eRUNNING);

	pThis->Run();

	if (pThis->GetExitType() == eDETACHABLE) {
		pThis->SetState(eTERMINATED);
	} else {
		pThis->SetState(eZOMBIE);
	}

	pthread_exit((void*)NULL);
	return NULL;
}

void PThread::Terminate()
{
	if (m_eState == eRUNNING) {
		m_bExit = true;
		if (m_eExitType == eJOINABLE) {
			OnTerminate();
			Join(m_nID);
		} else if (m_eExitType == eDETACHABLE) {
			OnTerminate();
		}
	} else if (m_eState == eZOMBIE) {
		Join(m_nID);
	}
}

// this function will be blocked until the StartPoint terminates
void PThread::Join(pthread_t a_nID)
{
	if (!pthread_join(a_nID, NULL)) {
		SetState(eTERMINATED);
	} else {
		printf("Failed to join thread [%s: %d]\n", __FUNCTION__, __LINE__);
	}
}

void PThread::SetState(THREAD_STATE a_eState)
{
	m_eState = a_eState;
}

THREAD_STATE PThread::GetState() const
{
	return m_eState;
}

THREAD_EXIT_STATE PThread::GetExitType() const
{
	return m_eExitType;
}

char* PThread::GetName() const
{
	return m_szName ? m_szName  : NULL;
}

bool PThread::IsTerminated() const
{
	return m_eState == eTERMINATED  ?  true : false;
}

bool PThread::IsRunning() const
{
	return m_eState == eRUNNING  ?  true : false;
}

