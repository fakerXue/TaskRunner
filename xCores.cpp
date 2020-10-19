
#include "xCores.h"

using namespace x2lib::xCores;

Mutex::Mutex()
{
#ifdef __WIN32__
	m_mutex = (int)CreateMutex(NULL, FALSE, NULL);
#elif defined __LINUX__
	if (pthread_mutex_init((pthread_mutex_t*)&m_mutex, NULL)<0)
	{
		perror("pthread_mutex_init");
	}
#endif
}

Mutex::~Mutex()
{
#ifdef __WIN32__
	CloseHandle((HANDLE)m_mutex);
#elif defined __LINUX__
	pthread_mutex_destroy((pthread_mutex_t*)&m_mutex);
#endif
}

bool Mutex::Lock() const
{
#ifdef __WIN32__
	return WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)m_mutex, INFINITE);
#elif defined __LINUX__
	return 0 == pthread_mutex_lock((pthread_mutex_t*)&m_mutex);
#endif
}

bool Mutex::Unlock() const
{
#ifdef __WIN32__
	return 0 != ReleaseMutex((HANDLE)m_mutex);
#elif defined __LINUX__
	return 0 == pthread_mutex_unlock((pthread_mutex_t*)&m_mutex);
#endif
}


Signal::Signal(int iInit, int iMax, char pName[128])
{
#if 0 // 直接给函数参数赋值可能会破坏堆栈
	if (pName[0] == 0)
	{
		sprintf(pName, "xCores::Signal_%08d_%08d", (int)this, (int)&m_signal);
	}
#else
	char szName[128] = { 0 };
	if (pName[0] == 0)
		sprintf(szName, "xCores::Signal_%08d_%08d", (int)this, (int)&m_signal);
	else
		sprintf(szName, pName);
#endif
#ifdef __WIN32__
    m_signal = (int)::CreateSemaphoreA(NULL, iInit, iMax, szName);
#if 0
    m_signal = (int)::CreateEvent(NULL, FALSE, FALSE, NULL);
#endif
#elif __LINUX__
	for (int i = 0; i<255; i++)
	{
		m_signal = semget(ftok(".", i), 1, 0666 | IPC_CREAT | IPC_EXCL);
		if (-1 != m_signal)
			break;
	}
	if (m_signal == -1)
	{
		perror("semget failed!\n");
		return;
	}
	union semun sem_union;
	sem_union.val = 0; // 初始化为0，表示首次调用Wait会阻塞，1表示首次调用Wait不阻塞
	if (semctl(m_signal, 0, SETVAL, sem_union))
	{
		perror("semctl error!\n");
	}
#endif	
}

Signal::~Signal()
{
#ifdef __WIN32__
	CloseHandle((HANDLE)m_signal);
#elif __LINUX__
	union semun sem_union;
	if (semctl(m_signal, 0, IPC_RMID, sem_union))
		printf("Failed to delete semaphore\n");
#endif
}

bool Signal::Wait(unsigned long msTimeout) const
{
#ifdef __WIN32__
	return WAIT_TIMEOUT == WaitForSingleObject((HANDLE)m_signal, msTimeout);
#elif __LINUX__
	struct sembuf sem_b = { 0/*信号量编号*/, -1/*V操作*/, SEM_UNDO };
	if (semop(m_signal, &sem_b, 1) == -1)
	{
		printf("semaphore_p failed\n");
		return 0;
	}
	return 1;
#endif
}

bool Signal::Notify(int nCnt) const
{
#ifdef __WIN32__
    return 0 != ::ReleaseSemaphore((HANDLE)m_signal, nCnt, NULL);
#if 0
    return 0 != ::SetEvent((HANDLE)m_signal);
#endif
#elif __LINUX__
	struct sembuf sem_b = { 0/*信号量编号*/, 1/*V操作*/, SEM_UNDO };
	if (semop(m_signal, &sem_b, 1) == -1)
	{
		printf("semaphore_v failed\n");
		return 0;
	}
	return 1;
#endif
}


