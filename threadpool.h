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
	int shutdown;								// 线程池是否销毁
	int thread_count_min;						// 线程池中线程个数最小值
	int thread_count_max;						// 线程池中线程个数最大值
	int thread_count_live;						// 线程池中活跃线程个数
	int thread_count_busy;						// 线程池中繁忙线程个数
	int thread_count_over;						// 线程池中过剩线程个数

	int task_count_max;							// 任务队列最大值
	int task_count_wait;						// 任务队列当前值
	THREAD_TASK* task_head;						// 指向任务队列的头指针
	THREAD_TASK* task_tail;						// 指向任务队列的尾指针

	pthread_t* pthreads;						// 线程池中工作线程数组的地址
	pthread_t admin;							// 管理线程
	pthread_mutex_t lock;						// 线程池互斥信号量
	pthread_mutex_t counter;					// 线程计数互斥信号量
	pthread_cond_t queue_not_empty;				// 任务队列不为空的条件变量
	pthread_cond_t queue_not_full;				// 任务队列不为满的条件变量

}THREAD_POOL;


// 创建线程池
THREAD_POOL* thread_pool_create(int thread_count_min, int thread_count_max, int task_count_max);
// 销毁线程池
void thread_pool_destroy(THREAD_POOL* thread_pool);
// 添加任务
int thread_pool_add_task(THREAD_POOL* thread_pool, void* (*callback_function)(void *arg), void *arg);
// 管理线程
void* thread_pool_admin(void* arg);
// 工作线程
void* thread_pool_work(void* arg);

#endif // !THREAD_H
