#include "thread_pool.h" //线程池
#include "unistd.h"
struct test
{
   int arg1;
   int arg2;
};

// 两个任务函数
void task_handler1(void *data) // 任务1
{
   static int index = 0;
   printf("Hello, this is 1th test.index=%d\r\n", index++);
}

void task_handler2(void *data) // 任务2
{
   static int index = 0;
   printf("Hello, this is 2th test.index=%d\r\n", index++);
}

void task_handler3(void *data)
{
   static int index = 0;
   struct test *t = (struct test *)data;

   printf("Hello, this is 3th test.index=%d\r\n", index++);
   printf("arg1: %d, arg2: %d\n", t->arg1, t->arg2);
}

int main(int argc, char **argv)
{
   thread_pool_t *tp = NULL; // 线程池的变量
   int i = 0;

   tp = thread_pool_init(); // 对于线程池初始化
   // sleep(1);
   thread_task_t *test1 = thread_task_alloc(0); // thread_task_t代表能被线程处理的任务
   thread_task_t *test2 = thread_task_alloc(0);
   thread_task_t *test3 = thread_task_alloc(sizeof(struct test));
   test1->handler = task_handler1; // 给任务指定要处理的任务函数
   test2->handler = task_handler2;
   test3->handler = task_handler3;
   ((struct test *)test3->ctx)->arg1 = 666;
   ((struct test *)test3->ctx)->arg2 = 888;
   // for(i=0; i<10;i++){
   thread_task_post(tp, test1); // 将任务加入到任务对列中
   thread_task_post(tp, test2);
   thread_task_post(tp, test3);
   //}
   sleep(10);
   thread_pool_destroy(tp); // 销毁线程池
}
// gcc *.c -o threadPool -pthread