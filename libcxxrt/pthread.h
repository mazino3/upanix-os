#ifndef _PTHREAD_H_
#define _PTHREAD_H_

#define PTHREAD_ONCE_INIT	0

#define PTHREAD_MUTEX_NORMAL	1
#define __LOCK_INITIALIZER	{ 0, 0 }

#define PTHREAD_MUTEX_INITIALIZER	{0,0,0,PTHREAD_MUTEX_NORMAL,__LOCK_INITIALIZER}
#define PTHREAD_COND_INITIALIZER	{__LOCK_INITIALIZER,0}

#if defined (__cplusplus)
extern "C" {
#endif

typedef struct _pthread_descr_struct *_pthread_descr;
typedef unsigned int pthread_key_t;
typedef int pthread_once_t;
typedef unsigned long int pthread_t;
typedef long long int __pthread_cond_align_t;

struct _pthread_fastlock {
    long int __status;
    int __spinlock;
};

typedef struct {
    int __m_reserved;
    int __m_count;
    _pthread_descr __m_owner;
    int __m_kind;
    struct _pthread_fastlock __m_lock;
} pthread_mutex_t;

typedef struct {
    struct _pthread_fastlock __c_lock;
    _pthread_descr __c_waiting;
    char __padding[48 - sizeof(struct _pthread_fastlock) -
		   sizeof(_pthread_descr) -
		   sizeof(__pthread_cond_align_t)];
    __pthread_cond_align_t __align;
} pthread_cond_t;

int sched_yield();
int pthread_mutex_lock(pthread_mutex_t *);
int pthread_mutex_unlock(pthread_mutex_t *);
int pthread_key_create(pthread_key_t *, void (*destr_func) (void *));
void *pthread_getspecific(pthread_key_t);
int pthread_setspecific(pthread_key_t, const void *);
int pthread_once(pthread_once_t *, void (*init_routine) (void));
int pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *);
int pthread_cond_signal(pthread_cond_t *);

#if defined (__cplusplus)
}
#endif

#endif
