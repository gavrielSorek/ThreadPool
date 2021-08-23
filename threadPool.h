//Gavriel Sorek 318525185
#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include "osqueue.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
typedef struct thread_pool
{
 int startDestroy;
 int continueAssigment; //for threads to know, when the thread_pool destroyed if to continue execute assigment.
 int numOfThreads;
 int isErrorOccur;
 pthread_t *ntids;
 OSQueue* queue;
 pthread_mutex_t *lock;
 pthread_cond_t* cond;
 pthread_t mainThread;
}ThreadPool;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif
