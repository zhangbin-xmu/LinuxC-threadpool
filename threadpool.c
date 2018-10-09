#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/signal.h>
#include "threadpool.h"


////////////////////////////////////////////////////////////
// 功能：创建线程池
// 输入：线程池中线程个数最小值，线程池中线程个数最大值，任务队列最大值
// 输出：
// 返回：线程池地址
////////////////////////////////////////////////////////////
THREAD_POOL* thread_pool_create(int thread_count_min, int thread_count_max, int task_count_max)
{
	// 为线程池开辟内存空间
	THREAD_POOL* thread_pool = NULL;
	thread_pool = (THREAD_POOL*)malloc(sizeof(THREAD_POOL));
	if (NULL == thread_pool) return NULL;

	// 初始化线程池参数
	thread_pool->shutdown = 0;
	thread_pool->thread_count_min = thread_count_min;
	thread_pool->thread_count_max = thread_count_max;
	thread_pool->thread_count_live = thread_count_min;
	thread_pool->thread_count_busy = 0;
	thread_pool->thread_count_over = 0;

	thread_pool->task_count_max = task_count_max;
	thread_pool->task_count_wait = 0;
	thread_pool->task_head = NULL;
	thread_pool->task_tail = NULL;

	// 为工作线程开辟内存空间
	thread_pool->pthreads = (pthread_t*)malloc(sizeof(pthread_t) * thread_count_max);
	if (NULL == thread_pool->pthreads) {
		thread_pool_destroy(thread_pool);
		return NULL;
	}

	// 初始化互斥锁和条件变量
	if (pthread_mutex_init(&(thread_pool->lock), NULL) 
		|| pthread_mutex_init(&(thread_pool->counter), NULL)
		|| pthread_cond_init(&(thread_pool->queue_not_empty), NULL) 
		|| pthread_cond_init(&(thread_pool->queue_not_full), NULL)) {

		thread_pool_destroy(thread_pool);
		return NULL;
	}

	// 启动工作线程
	int i;
	for (i = 0; i < thread_pool->thread_count_min; i++) {
		pthread_create(&(thread_pool->pthreads[i]), NULL, thread_pool_work, (void*)thread_pool);
	}

	// 启动管理线程
	pthread_create(&(thread_pool->admin), NULL, thread_pool_admin, (void*)thread_pool);
	return thread_pool;
}

////////////////////////////////////////////////////////////
// 功能：销毁线程池
// 输入：线程池地址
// 输出：
// 返回：无
////////////////////////////////////////////////////////////
void thread_pool_destroy(THREAD_POOL* thread_pool)
{
	if (NULL == thread_pool || thread_pool->shutdown) return;

	// 线程池上锁
	pthread_mutex_lock(&(thread_pool->lock));

	// 不再执行任务
	thread_pool->task_count_wait = 0;
	thread_pool->shutdown = 1;

	// 杀死所有工作线程
	int i;
	for (i = 0; i < thread_pool->thread_count_live; i++) {
		pthread_cond_signal(&(thread_pool->queue_not_empty));
	}

	// 线程池解锁
	pthread_mutex_unlock(&(thread_pool->lock));

	// 释放资源	
	free(thread_pool->pthreads);
	pthread_mutex_destroy(&(thread_pool->lock));
	pthread_cond_destroy(&(thread_pool->queue_not_empty));
	pthread_cond_destroy(&(thread_pool->queue_not_full));

	THREAD_TASK* p;
	while (thread_pool->task_head != NULL) {
		p = thread_pool->task_head;
		thread_pool->task_head = p->next;
		free(p);
	}
	free(thread_pool);
}

////////////////////////////////////////////////////////////
// 功能：添加任务
// 输入：线程池地址，回调函数，回调函数参数
// 输出：
// 返回：0-成功 -1-失败
////////////////////////////////////////////////////////////
int thread_pool_add_task(THREAD_POOL* thread_pool, void* (*callback_function)(void *arg), void *arg)
{
	if (NULL == thread_pool || NULL == callback_function || NULL == arg) return -1;

	// 线程池上锁
	pthread_mutex_lock(&(thread_pool->lock));

	// 等待任务队列盈余
	while ((thread_pool->task_count_wait == thread_pool->task_count_max) && !thread_pool->shutdown) {
		pthread_cond_wait(&(thread_pool->queue_not_full), &(thread_pool->lock));
	}

	// 线程池已销毁
	if (thread_pool->shutdown) {
		pthread_mutex_unlock(&(thread_pool->lock));
		return -1;
	}

	// 为任务开辟内存空间
	THREAD_TASK* task = (THREAD_TASK*)malloc(sizeof(THREAD_TASK));
	if (NULL == task) {
		pthread_mutex_unlock(&(thread_pool->lock));
		return -1;
	}

	// 初始化任务
	task->callback_function = callback_function;
	task->arg = arg;
	task->next = NULL;

	// 添加到任务队列
	thread_pool->task_count_wait++;
	if (thread_pool->task_head == NULL) {
		thread_pool->task_head = thread_pool->task_tail = task;
	}
	else {
		thread_pool->task_tail->next = task;
		thread_pool->task_tail = task;
	}

	// 任务队列非空
	pthread_cond_broadcast(&(thread_pool->queue_not_empty));

	// 线程池解锁
	pthread_mutex_unlock(&(thread_pool->lock));
	return 0;
}

////////////////////////////////////////////////////////////
// 功能：管理线程
// 输入：线程池地址
// 输出：
// 返回：无
////////////////////////////////////////////////////////////
void* thread_pool_admin(void* arg)
{
	THREAD_POOL* thread_pool = (THREAD_POOL*)arg;

	while (!thread_pool->shutdown) {

		sleep(10);

		// 线程池上锁
		pthread_mutex_lock(&(thread_pool->lock));


		//printf("============= Admin Start ============\n");
		//printf("thread_count_min: --%d--\n", thread_pool->thread_count_min);
		//printf("thread_count_max: --%d--\n", thread_pool->thread_count_max);
		//printf("thread_count_live: --%d--\n", thread_pool->thread_count_live);
		//printf("thread_count_busy: --%d--\n", thread_pool->thread_count_busy);
		//printf("thread_count_over: --%d--\n", thread_pool->thread_count_over);
		//printf("task_count_max: --%d--\n", thread_pool->task_count_max);
		//printf("task_count_wait: --%d--\n", thread_pool->task_count_wait);


		// 任务队列当前值
		int task_count = thread_pool->task_count_wait;
		// 线程池中活跃线程个数
		int thread_count_live = thread_pool->thread_count_live;
		// 线程池中繁忙线程个数
		int thread_count_busy = thread_pool->thread_count_busy;

		// 创建新线程
		if (task_count >= thread_pool->task_count_max) {

			int i;
			for (i = 0; i < thread_pool->thread_count_max; i++) {

				if (thread_count_live >= thread_pool->thread_count_max) break;
				if (task_count <= thread_pool->task_count_max / 2) break;

				if (thread_pool->pthreads[i] == 0 || ESRCH == pthread_kill(thread_pool->pthreads[i], 0)) {
					pthread_create(&(thread_pool->pthreads[i]), NULL, thread_pool_work, (void*)thread_pool);
					thread_pool->thread_count_live++;
					task_count--;
				}
			}
		}

		// 销毁过剩线程
		if ((thread_count_busy * 2) < thread_count_live && thread_count_live > thread_pool->thread_count_min) {

			thread_pool->thread_count_over = (thread_count_live - thread_count_busy) / 2;
			int i;
			for (i = 0; i < (thread_count_live - thread_count_busy) / 2; i++) {
				pthread_cond_signal(&(thread_pool->queue_not_empty));
			}
		}


		//printf("============= Admin End ============\n");
		//printf("thread_count_min: --%d--\n", thread_pool->thread_count_min);
		//printf("thread_count_max: --%d--\n", thread_pool->thread_count_max);
		//printf("thread_count_live: --%d--\n", thread_pool->thread_count_live);
		//printf("thread_count_busy: --%d--\n", thread_pool->thread_count_busy);
		//printf("thread_count_over: --%d--\n", thread_pool->thread_count_over);
		//printf("task_count_max: --%d--\n", thread_pool->task_count_max);
		//printf("task_count_wait: --%d--\n", thread_pool->task_count_wait);


		// 线程池解锁
		pthread_mutex_unlock(&(thread_pool->lock));
	}

	return NULL;
}

////////////////////////////////////////////////////////////
// 功能：工作线程
// 输入：线程池地址
// 输出：
// 返回：无
////////////////////////////////////////////////////////////
void* thread_pool_work(void* arg)
{
	THREAD_POOL* thread_pool = (THREAD_POOL*)arg;

	while (1) {
		// 线程池上锁
		pthread_mutex_lock(&(thread_pool->lock));

		// 清理过剩线程，等待任务
		while ((thread_pool->task_count_wait == 0) && !thread_pool->shutdown) {
			
			pthread_cond_wait(&(thread_pool->queue_not_empty), &(thread_pool->lock));

			if (thread_pool->thread_count_over > 0) {
				thread_pool->thread_count_over--;
				if (thread_pool->thread_count_live > thread_pool->thread_count_min) {
					thread_pool->thread_count_live--;
					pthread_mutex_unlock(&(thread_pool->lock));				
					pthread_exit(NULL);
				}
			}
		}

		// 线程池已销毁
		if (thread_pool->shutdown) {
			pthread_mutex_unlock(&(thread_pool->lock));
			pthread_exit(NULL);
		}

		// 任务出列
		THREAD_TASK* task = NULL;
		task = thread_pool->task_head;
		thread_pool->task_count_wait--;
		thread_pool->thread_count_busy++;

		if (thread_pool->task_count_wait == 0) {
			thread_pool->task_head = thread_pool->task_tail = NULL;
		}
		else {
			thread_pool->task_head = task->next;
		}

		// 任务队列未满
		if (thread_pool->task_count_wait < thread_pool->task_count_max) {
			pthread_cond_broadcast(&(thread_pool->queue_not_full));
		}

		// 线程池解锁
		pthread_mutex_unlock(&(thread_pool->lock));

		// 执行任务
		(*(task->callback_function))(task->arg);
		free(task);
		task = NULL;

		pthread_mutex_lock(&(thread_pool->counter));
		thread_pool->thread_count_busy--;
		pthread_mutex_unlock(&(thread_pool->counter));
	}
}
