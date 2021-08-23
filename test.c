//Gavriel Sorek 318525185
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "osqueue.h"
#include "threadPool.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct functionAndParam {
    void (*computeFunc)(void *);
    void *param;
} FunctionAndData;

void hello (void* a)
{
   printf("hello FROM %d\n", pthread_self());
}
void helloNum (void* a)
{
    printf("hello FROM %d\n", (*(int*)a));
}
void helloSlow (void* a)
{
    sleep(2);
    printf("hello\n");
}
void destroy(void* a) {
    FunctionAndData* funcAndP = (FunctionAndData*)a;
    funcAndP->computeFunc((ThreadPool*)funcAndP->param);
}

void test_thread_pool_sanity()
{
   int i;
   
   ThreadPool* tp = tpCreate(5);
   
   for(i=0; i<10; ++i)
   {
       tpInsertTask(tp,hello,NULL);
//       FunctionAndData* f = malloc(sizeof (FunctionAndData));
//       f->computeFunc = tpDestroy;
//       f->param = tp;
      // tpInsertTask(tp,destroy, f);

   }
   
   tpDestroy(tp,1);

}
void test_thread_pool_sanity2()
{
    printf("if hello was writen the test failed\n");
    int i;

    ThreadPool* tp = tpCreate(5);
    tpDestroy(tp,0);
//    for(i=0; i<10; ++i)
//    {
//        tpInsertTask(tp,helloSlow,NULL);
//
//    }

    //tpDestroy(tp,0);
}
void test_thread_pool_sanity3()
{
    printf("if hello * 10 wasn't writen test failed\n");
    int i;

    ThreadPool* tp = tpCreate(5);

    for(i=0; i<10; ++i)
    {
        tpInsertTask(tp,helloSlow,NULL);

    }

    tpDestroy(tp,2);
}
void test_thread_pool_sanity4()
{
    printf("if hello * 2 wasn't writen test failed\n");
    int i;

    ThreadPool* tp = tpCreate(5);

    for(i=0; i<2; ++i)
    {
        tpInsertTask(tp,helloSlow,NULL);

    }

    tpDestroy(tp,1);
    //tpInsertTask(tp,hello,NULL);
}
void test_thread_pool_sanity5()
{
    printf("if hello * 2 wasn't writen test failed\n");
    int i;

    int num = 8200;
    ThreadPool* tp = tpCreate(5);

    for(i=0; i<2; ++i)
    {
        tpInsertTask(tp,helloNum,&num);

    }

    tpDestroy(tp,1);
    //tpInsertTask(tp,hello,NULL);
}

int main()
{

   // test_thread_pool_sanity();
//  test_thread_pool_sanity2();
//   test_thread_pool_sanity3();
  //  test_thread_pool_sanity4();
    test_thread_pool_sanity5();
   printf("Hello, World!\n");

   return 0;
}
