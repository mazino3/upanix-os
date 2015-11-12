#include <assert.h>
#include "pthread.h"

int sched_yield()
{
	return 0;
}

static void* threadDataTable[64]; 
static int freeEntry = 0;

typedef struct
{
	 unsigned long   mattr_flags;
	 int             mattr_kind;
	 int             mattr_spare1;
	 int             mattr_prioceiling;
	 int             mattr_protocol;
	 int             mattr_spares2[6];
	 char            mattr_name[31+1];
} pthread_mutexattr_t;

int pthread_key_create(pthread_key_t* key, void (*f)(void*)) 
{
	assert(freeEntry < 64);
 
	*key = freeEntry;
	freeEntry++;
	return 0;
}
 
int pthread_once(pthread_once_t* control, void (*init)(void)) 
{
	if (*control == 0) {
		(*init)();
		*control = 1;
	}
	return 0;
}
 
void* pthread_getspecific(pthread_key_t key) 
{
	return threadDataTable[key];
}
 
int pthread_setspecific(pthread_key_t key, const void* data) 
{
	threadDataTable[key] = (void*)data;
	return 0;
}
 
int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* t)  
{
	mutex->__m_count = 0;
	return 0;
}
 
int pthread_mutex_lock(pthread_mutex_t* mutex) 
{
	assert(mutex->__m_count == 0);
	mutex->__m_count = 1;
	return 0;
}
 
int pthread_mutex_unlock(pthread_mutex_t* mutex) 
{
	assert(mutex->__m_count != 0);
	mutex->__m_count = 0;
	return 0;
}
 
int pthread_cond_wait(pthread_cond_t* c, pthread_mutex_t* m) 
{
	return 0;
}
 
int pthread_cond_signal(pthread_cond_t* c) 
{
	return 0;
}
