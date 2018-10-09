#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>


////////////////////////////////////////////////////////////
// 队列任务数据结构
////////////////////////////////////////////////////////////
typedef struct _THREAD_TASK
{
	void* (*callback_function)(void* arg);		// 线程回调函数
	void* arg;									// 回调函数参数
	struct _THREAD_TASK* next;

}THREAD_TASK;

////////////////////////////////////////////////////////////
// 线程池数据结构
////////////////////////////////////////////////////////////
typedef struct _THREAD_POOL
{
	int thread_count;							// 线程池中开启线程的个数
	int queue_task_max;							// 队列任务个数上限
	int queue_task_cur;							// 队列当前的任务个数
	THREAD_TASK* task_head;						// 指向任务队列的头指针
	THREAD_TASK* task_tail;						// 指向任务队列的尾指针
	pthread_t* pthreads;						// 线程池中所有线程的pthread_t
	pthread_mutex_t lock;						// 互斥信号量
	pthread_cond_t queue_empty;					// 队列为空的条件变量
	pthread_cond_t queue_not_empty;				// 队列不为空的条件变量
	pthread_cond_t queue_not_full;				// 队列不为满的条件变量
	int queue_close;							// 队列是否已经关闭
	int pool_close;								// 线程池是否已经关闭

}THREAD_POOL;


// 创建线程池
THREAD_POOL* thread_pool_create(int thread_count, int queue_task_max);
// 销毁线程池
void thread_pool_destroy(THREAD_POOL* thread_pool);
// 添加任务
int thread_pool_add_task(THREAD_POOL* thread_pool, void* (*callback_function)(void *arg), void *arg);
// 执行任务
void* thread_pool_work(void* arg);


#endif // !THREAD_H
