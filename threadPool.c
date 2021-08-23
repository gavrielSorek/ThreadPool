//Gavriel Sorek 318525185
#include "threadPool.h"
#include <pthread.h>
#include <stdio.h>

typedef struct functionAndParam {
    void (*computeFunc)(void *);
    void *param;
} FunctionAndData;

/**
 * input: threadPool and array of threads ids.
 * the function waits for all the threads in the ids list.
 */
void waitAllThreads(pthread_t *ntids, ThreadPool *threadPool) {
    if (pthread_cond_broadcast(threadPool->cond) != 0) {  //signal to all threads
        perror("error in pthread_cond_broadcast"); //if error
        threadPool->isErrorOccur = 1;
        return;
    }
    void *retVal[threadPool->numOfThreads];
    int i = 0;
    for (i = 0; i < threadPool->numOfThreads; ++i) {
        if(ntids[i] != pthread_self()) {
            pthread_join(ntids[i], &retVal[i]);
        }
    }
}
/**
 * release memory from threadPool
 * @param threadPool the thread pool
 */
void releaseMemory(ThreadPool *threadPool) {
    if(threadPool == NULL)
        return;
    if(threadPool->ntids != NULL)
        free(threadPool->ntids);
    if(threadPool->queue != NULL)
        osDestroyQueue(threadPool->queue);
    if(threadPool->lock != NULL)
        free(threadPool->lock);
    if(threadPool->cond != NULL)
        free(threadPool->cond);
    free(threadPool);
}
/** input: threadPool and shouldWaitForTasks number.
 * the function destroying the threadPool, if shouldWaitForTasks is 0 the threadPool will not continue to
 * execute the tasks in the queue of the threadPool (o.w the threadPool continue to execute the task
 * in the queue).
 * */
void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks) {
    if(pthread_mutex_lock(threadPool->lock) != 0) {
        perror("error in pthread_mutex_lock");
        threadPool->isErrorOccur = 1;
    }
    if(threadPool->startDestroy) { //case if thread pool is already in destroying stage
        //printf("pool is already destroying\n");
        if(pthread_mutex_unlock(threadPool->lock) != 0) {
            perror("error in pthread_mutex_unlock");
            threadPool->isErrorOccur = 1;
        }
        return;
    }
    //printf("destroying %d\n", pthread_self());
    threadPool->startDestroy = 1;
    if (!shouldWaitForTasks)
        threadPool->continueAssigment = 0;
    if(pthread_mutex_unlock(threadPool->lock) != 0) {
        perror("error in pthread_mutex_unlock");
        threadPool->isErrorOccur = 1;
    }
    waitAllThreads(threadPool->ntids, threadPool);
    // free all tasks
    while (!osIsQueueEmpty(threadPool->queue)) {
        free (osDequeue(threadPool->queue));
    }
    pthread_t mainThreadId = threadPool->mainThread;
    //destroy
    pthread_mutex_destroy(threadPool->lock);
    pthread_cond_destroy(threadPool->cond);
    int isError = threadPool->isErrorOccur;
    releaseMemory(threadPool);
    if(isError)
        exit(-1);
    if(mainThreadId != pthread_self()) {// if not the main thread
        pthread_exit((void *) 0);
    }
}
/** input: threadPool.
 * the function represent thrad in the threadPool.
 * return: 0 while the thread finished.
 * */
int threadFunc(void *data) {
    ThreadPool *myThreadPool = (ThreadPool *) data;
    while (1) {
        if(pthread_mutex_lock(myThreadPool->lock) != 0) {
            perror("error in pthread_mutex_lock");
            myThreadPool->isErrorOccur = 1;
        }
        //printf("start thread func %d\n", pthread_self());
        if (myThreadPool->startDestroy && !myThreadPool->continueAssigment ||
            myThreadPool->startDestroy && osIsQueueEmpty(myThreadPool->queue)) {
            if(pthread_mutex_unlock(myThreadPool->lock) != 0) {
                perror("error in pthread_mutex_unlock");
                myThreadPool->isErrorOccur = 1;
            }
            return 0;
        }
        int isQueueEmpty = osIsQueueEmpty(myThreadPool->queue);
        while (!isQueueEmpty && myThreadPool->continueAssigment) {
            //printf("do work %d\n", pthread_self());
            FunctionAndData *functionAndData = (FunctionAndData *) osDequeue(
                    myThreadPool->queue); // deque the last mission
            if(pthread_mutex_unlock(myThreadPool->lock) != 0) {
                perror("error in pthread_mutex_unlock");
                myThreadPool->isErrorOccur = 1;
            }
            functionAndData->computeFunc(functionAndData->param);
            free(functionAndData); //free allocated memory
            if(pthread_mutex_lock(myThreadPool->lock) !=0 ) {
                perror("error in pthread_mutex_lock");
                myThreadPool->isErrorOccur = 1;
            }
            isQueueEmpty = osIsQueueEmpty(myThreadPool->queue);
        }
        isQueueEmpty = osIsQueueEmpty(myThreadPool->queue);
        //if finished all assigment, and pool is destroyed or
        //if pool is destroyed and !continueAssigment
        if (isQueueEmpty && myThreadPool->startDestroy ||
            myThreadPool->startDestroy && !myThreadPool->continueAssigment) {
            myThreadPool->continueAssigment = 0;
            //printf("thread finished early %d\n", pthread_self());
            if(pthread_mutex_unlock(myThreadPool->lock) != 0) {
                perror("error in pthread_mutex_unlock");
                myThreadPool->isErrorOccur = 1;
            }
            return 0;
        }
        //printf("waiting %d\n", pthread_self());
        if(pthread_cond_wait(myThreadPool->cond, myThreadPool->lock) != 0) {
            perror("error in pthread_cond_wait");
            myThreadPool->isErrorOccur = 1;
        }
        //printf("wait finished %d\n", pthread_self());
        if(pthread_mutex_unlock(myThreadPool->lock)!= 0) {
            perror("error in pthread_mutex_unlock");
            myThreadPool->isErrorOccur = 1;
        }
    }
}
/**input: num of threads
 * return: threadPool with the same number of threads.
 * */
ThreadPool *tpCreate(int numOfThreads) {
    ThreadPool *threadPool = malloc(sizeof(ThreadPool));
    if(threadPool == NULL) {
        perror("error in malloc");
        exit(-1);
    }
    //initiate variables
    threadPool->ntids =NULL;
    threadPool->queue = NULL;
    threadPool->lock = NULL;
    threadPool->cond = NULL;
    threadPool->isErrorOccur = 0;

    threadPool->mainThread = pthread_self();
    threadPool->ntids = calloc(sizeof(pthread_t), numOfThreads);
    threadPool->startDestroy = 0;
    threadPool->numOfThreads = numOfThreads;
    threadPool->queue = osCreateQueue();
    //create the mutex
    threadPool->lock = malloc(sizeof(pthread_mutex_t));
    threadPool->cond = malloc(sizeof(pthread_cond_t));
    if(threadPool->cond == NULL || threadPool->lock == NULL || threadPool->queue == NULL || threadPool->ntids == NULL) {
        perror("error in memory allocation");
        releaseMemory(threadPool);
        exit(-1);
    }
    threadPool->continueAssigment = 1; //default
    if (pthread_mutex_init(threadPool->lock, NULL) != 0) {//if error
        perror("error in pthread_mutex_init");
        releaseMemory(threadPool);
        exit(-1);
    }
    if (pthread_cond_init(threadPool->cond, NULL) != 0) { //if error
        perror("error in pthread_cond_init");
        releaseMemory(threadPool);
        exit(-1);
    }
    int i = 0;
    for (i = 0; i < threadPool->numOfThreads; ++i) {
        int err = pthread_create(&(threadPool->ntids[i]), NULL, threadFunc,
                                 threadPool);
        if (err != 0) { //error
            perror("can't create thread\n");
            releaseMemory(threadPool);
            exit(-1);
        }
    }
    return threadPool;
}

/**
 * input: threadPool ,function and arguments to function.
 *  the function enter the function and the params to threadPool queue.
 *  return: 0 if success , and -1 if the pool is in demolition process.
 * */
int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param) {
    if(pthread_mutex_lock(threadPool->lock) !=0) {
        perror("error in pthread_mutex_lock");
        threadPool->isErrorOccur = 1;
    }
    int destroy = threadPool->startDestroy;
    if (destroy) { //if pool is destroyed , no new missions
        if(pthread_mutex_unlock(threadPool->lock) != 0) {
            perror("error in pthread_mutex_unlock");
            threadPool->isErrorOccur = 1;
        }
        return -1;
    }
    FunctionAndData *functionAndData = malloc(sizeof(FunctionAndData));
    if(functionAndData == NULL) {
        perror("error in malloc");
        threadPool->isErrorOccur = 1;
        if(pthread_mutex_unlock(threadPool->lock) != 0) {
            perror("error in pthread_mutex_unlock");
            threadPool->isErrorOccur = 1;
        }
        return -1; //error
    }
    functionAndData->computeFunc = computeFunc;
    functionAndData->param = param;
    osEnqueue(threadPool->queue, (void *) functionAndData);
    if(pthread_mutex_unlock(threadPool->lock) != 0 ) {
        perror("error in pthread_mutex_unlock");
        threadPool->isErrorOccur = 1;
    }
    if (pthread_cond_broadcast(threadPool->cond) != 0) {  //signal to all threads
        perror("error in pthread_cond_broadcast");
        threadPool->isErrorOccur = 1;
    }
    return 0;
}
