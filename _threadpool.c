#include <stdlib.h>
#include "threadpool.h"


////////////////////////////////////////////////////////////
// 功能：创建线程池
// 输入：线程池开启的线程个数，任务队列任务个数上限
// 输出：
// 返回：线程池地址
////////////////////////////////////////////////////////////
THREAD_POOL* thread_pool_create(int thread_count, int queue_task_max)
{
	// 开辟内存空间
	THREAD_POOL* thread_pool = NULL;
	thread_pool = (THREAD_POOL*)malloc(sizeof(THREAD_POOL));
	if (NULL == thread_pool) return NULL;

	// 初始化线程池
	thread_pool->thread_count = thread_count;
	thread_pool->queue_task_max = queue_task_max;
	thread_pool->queue_task_cur = 0;
	thread_pool->task_head = NULL;
	thread_pool->task_tail = NULL;

	if (pthread_mutex_init(&(thread_pool->lock), NULL)) {
		return NULL;
	}
	if (pthread_cond_init(&(thread_pool->queue_empty), NULL)) {
		return NULL;
	}
	if (pthread_cond_init(&(thread_pool->queue_not_empty), NULL)) {
		return NULL;
	}
	if (pthread_cond_init(&(thread_pool->queue_not_full), NULL)) {
		return NULL;
	}

	thread_pool->pthreads = (pthread_t*)malloc(sizeof(pthread_t) * thread_count);
	if (NULL == thread_pool->pthreads) {
		return NULL;
	}

	thread_pool->queue_close = 0;
	thread_pool->pool_close = 0;

	int i;
	for (i = 0; i < thread_pool->thread_count; i++) {
		pthread_create(&(thread_pool->pthreads[i]), NULL, thread_pool_work, (void *)thread_pool);
	}

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
	if (NULL == thread_pool) return;

	pthread_mutex_lock(&(thread_pool->lock));
	if (thread_pool->queue_close || thread_pool->pool_close) {
		pthread_mutex_unlock(&(thread_pool->lock));
		return;
	}

	thread_pool->queue_close = 1;
	while (thread_pool->queue_task_cur != 0) {
		pthread_cond_wait(&(thread_pool->queue_empty), &(thread_pool->lock));
	}

	thread_pool->pool_close = 1;
	pthread_mutex_unlock(&(thread_pool->lock));
	pthread_cond_broadcast(&(thread_pool->queue_not_empty));
	pthread_cond_broadcast(&(thread_pool->queue_not_full));

	int i;
	for (i = 0; i < thread_pool->thread_count; i++) {
		pthread_join(thread_pool->pthreads[i], NULL);
	}

	pthread_mutex_destroy(&(thread_pool->lock));
	pthread_cond_destroy(&(thread_pool->queue_empty));
	pthread_cond_destroy(&(thread_pool->queue_not_empty));
	pthread_cond_destroy(&(thread_pool->queue_not_full));
	free(thread_pool->pthreads);

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

	pthread_mutex_lock(&(thread_pool->lock));
	while ((thread_pool->queue_task_cur == thread_pool->queue_task_max) 
		&& !(thread_pool->queue_close || thread_pool->pool_close)) {

		pthread_cond_wait(&(thread_pool->queue_not_full), &(thread_pool->lock));
	}

	if (thread_pool->queue_close || thread_pool->pool_close) {
		pthread_mutex_unlock(&(thread_pool->lock));
		return -1;
	}

	THREAD_TASK* task = (THREAD_TASK*)malloc(sizeof(THREAD_TASK));
	if (NULL == task) {
		pthread_mutex_unlock(&(thread_pool->lock));
		return -1;
	}

	task->callback_function = callback_function;
	task->arg = arg;
	task->next = NULL;

	if (thread_pool->task_head == NULL) {
		thread_pool->task_head = thread_pool->task_tail = task;
		pthread_cond_broadcast(&(thread_pool->queue_not_empty));
	}
	else {
		thread_pool->task_tail->next = task;
		thread_pool->task_tail = task;
	}
	thread_pool->queue_task_cur++;

	pthread_mutex_unlock(&(thread_pool->lock));
	return 0;
}


////////////////////////////////////////////////////////////
// 功能：执行任务
// 输入：线程池地址
// 输出：
// 返回：无
////////////////////////////////////////////////////////////
void* thread_pool_work(void* arg)
{
	THREAD_POOL* thread_pool = (THREAD_POOL*)arg;
	THREAD_TASK* task = NULL;

	while (1) {

		pthread_mutex_lock(&(thread_pool->lock));
		while ((thread_pool->queue_task_cur == 0) && !thread_pool->pool_close) {
			pthread_cond_wait(&(thread_pool->queue_not_empty), &(thread_pool->lock));
		}

		if (thread_pool->pool_close) {
			pthread_mutex_unlock(&(thread_pool->lock));
			pthread_exit(NULL);
		}

		thread_pool->queue_task_cur--;
		task = thread_pool->task_head;

		if (thread_pool->queue_task_cur == 0) {
			thread_pool->task_head = thread_pool->task_tail = NULL;
		}
		else {
			thread_pool->task_head = task->next;
		}

		if (thread_pool->queue_task_cur == 0) {
			pthread_cond_signal(&(thread_pool->queue_empty));
		}

		if (thread_pool->queue_task_cur == thread_pool->queue_task_max - 1) {
			pthread_cond_broadcast(&(thread_pool->queue_not_full));
		}

		pthread_mutex_unlock(&(thread_pool->lock));

		(*(task->callback_function))(task->arg);
		free(task);
		task = NULL;
	}
}
