
#if !defined(__gnu_linux__)
#define TEMP_FAILURE_RETRY(x)  x
#endif

#if defined(__APPLE__)
#define  PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#include "Atomic.h"
#include <cstdio>
#endif

#if defined(__gnu_linux__) || defined(__APPLE__)
#include <errno.h> // TEMP_FAILURE_RETRY
#include <unistd.h> // TEMP_FAILURE_RETRY

#endif

#include "IPC.h"

Mutex::Mutex()
{
#if defined(__gnu_linux__) || defined(__APPLE__)
    pthread_mutexattr_t attr;
    ::pthread_mutexattr_init(&attr);
    ::pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    ::pthread_mutex_init(&m_mutex, &attr);
    ::pthread_mutexattr_destroy(&attr);
#else
    ::InitializeCriticalSection(&m_mutex);
#endif
}

Mutex::~Mutex()
{
#if defined(__gnu_linux__) || defined(__APPLE__)
    ::pthread_mutex_destroy(&m_mutex);
#else
    ::DeleteCriticalSection(&m_mutex);
#endif
}
    
void Mutex::Lock()
{
#if defined(__gnu_linux__) || defined(__APPLE__)
    ::pthread_mutex_lock(&m_mutex);
#else
    ::EnterCriticalSection(&m_mutex);
#endif
}

bool Mutex::TryLock()
{
#if defined(__gnu_linux__) || defined(__APPLE__)
    return 0 == ::pthread_mutex_trylock(&m_mutex);
#else
    return TRUE == ::TryEnterCriticalSection(&m_mutex);
#endif
}
    
void Mutex::Unlock()
{
#if defined(__gnu_linux__) || defined(__APPLE__)
    ::pthread_mutex_unlock(&m_mutex);
#else
    ::LeaveCriticalSection(&m_mutex);
#endif
}

ScopeMutex::ScopeMutex(Mutex& m) : m_mutex(m)
{
    m_mutex.Lock();
}

ScopeMutex::~ScopeMutex()
{
    m_mutex.Unlock();
}

RWLock::RWLock()
{
#if defined(__gnu_linux__) || defined(__APPLE__)
    ::pthread_rwlock_init(&m_rwLock, NULL );
#else
    //::InitializeSRWLock(&m_rwLock);
#endif
}
    
RWLock::~RWLock()
{ // windows do nothing~
#if defined(__gnu_linux__) || defined(__APPLE__)
    ::pthread_rwlock_destroy(&m_rwLock );
#endif
}
    
void RWLock::Rdlock()
{
#if defined(__gnu_linux__) || defined(__APPLE__)
    ::pthread_rwlock_rdlock(&m_rwLock);
#else
    m_rwLock.Lock();
    //::AcquireSRWLockShared(&m_rwLock);
#endif
}
    
void RWLock::Wrlock()
{
#if defined(__gnu_linux__) || defined(__APPLE__)
    ::pthread_rwlock_wrlock(&m_rwLock );
#else
    m_rwLock.Lock();
    //::AcquireSRWLockExclusive(&m_rwLock);
#endif
}
    
void RWLock::UnRdlock()
{
#if defined(__gnu_linux__) || defined(__APPLE__)
    ::pthread_rwlock_unlock(&m_rwLock );
#else
    m_rwLock.Unlock();
    //::ReleaseSRWLockShared(&m_rwLock);
#endif
}

void RWLock::UnWrlock()
{
#if defined(__gnu_linux__) || defined(__APPLE__)
    ::pthread_rwlock_unlock(&m_rwLock );
#else
    m_rwLock.Unlock();
    //::ReleaseSRWLockExclusive(&m_rwLock);
#endif
}


ScopeReadLock::ScopeReadLock(RWLock &m) : m_lock(m)
{
    m_lock.Rdlock();
}
    
ScopeReadLock::~ScopeReadLock()
{
    m_lock.UnRdlock();
}

ScopeWriteLock::ScopeWriteLock(RWLock & m) : m_lock(m)
{
    m_lock.Wrlock();
}
    
ScopeWriteLock::~ScopeWriteLock()
{
    m_lock.UnWrlock();
}

#if defined(__gnu_linux__) || defined(__APPLE__)
Condition::Condition(Mutex* m) : m_mutex(m)
{
#if defined(__gnu_linux__) || defined(__APPLE__)
    ::pthread_cond_init(&m_cond, NULL);
#else
    ::InitializeConditionVariable(&m_cond);
#endif
}
    
Condition::~Condition()
{
#if defined(__gnu_linux__) || defined(__APPLE__)
    pthread_cond_destroy(&m_cond);
#endif
}

void Condition::Signal()
{
#if defined(__gnu_linux__) || defined(__APPLE__)
    ::pthread_cond_signal(&m_cond);
#else
    ::WakeConditionVariable(&m_cond);
#endif
}

void Condition::Broadcast()
{
#if defined(__gnu_linux__) || defined(__APPLE__)
    ::pthread_cond_broadcast(&m_cond);
#else
    ::WakeAllConditionVariable(&m_cond);
#endif
}

void Condition::Wait()
{
#if defined(__gnu_linux__) || defined(__APPLE__)
    ::pthread_cond_wait(&m_cond, &m_mutex->m_mutex);
#else
    ::SleepConditionVariableCS(&m_cond, &m_mutex->m_mutex, INFINITE);
#endif
}
    
void Condition::Lock()
{
    m_mutex->Lock();
}

void Condition::Unlock()
{
    m_mutex->Unlock();
}
#endif

#if defined(__APPLE__)
unsigned Semaphore::m_id = 0;
#endif

Semaphore::Semaphore(long lInit, long lMaxForWindows)
{
#if defined(__gnu_linux__)
    ::sem_init(&m_sem, PTHREAD_PROCESS_PRIVATE, lInit);
#elif defined(__APPLE__)

    AtomicChange(&m_id, 1);
    snprintf(m_name, sizeof(m_name), "defaultSem%d", m_id);

    m_sem = ::sem_open(m_name, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, lInit);
    sem_unlink(m_name);

    if (m_sem == SEM_FAILED)
        m_name[1000000] = 0;
#else
    m_sem = ::CreateSemaphore(0, lInit, lMaxForWindows, 0);
#endif
}

Semaphore::~Semaphore()
{
#if defined(__gnu_linux__)
    ::sem_destroy(&m_sem);
#elif defined(__APPLE__)
    ::sem_close(m_sem);
#else
    ::CloseHandle(m_sem);
#endif
}

void Semaphore::Wait()
{
#if defined(__gnu_linux__)
    TEMP_FAILURE_RETRY(::sem_wait(&m_sem));
#elif defined(__APPLE__)
    TEMP_FAILURE_RETRY(::sem_wait(m_sem));
#else
    ::WaitForSingleObject(m_sem, INFINITE);
#endif
}

void Semaphore::Post(long lReleaseCntForWin)
{
#if defined(__gnu_linux__)
    ::sem_post(&m_sem);
#elif defined(__APPLE__)
    ::sem_post(m_sem);
#else
    ::ReleaseSemaphore(m_sem, lReleaseCntForWin, 0);
#endif
}

int  Semaphore::Value()
{
    int  val = 0;
#if defined(__gnu_linux__)
    ::sem_getvalue(&m_sem, &val);
#elif defined(__APPLE__)
    ::sem_getvalue(m_sem, &val);
#elif defined(__WIN32__)
#endif

    return val;
}
