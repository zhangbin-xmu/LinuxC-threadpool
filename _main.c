#include <stdio.h>
#include <unistd.h>
#include "threadpool.h"


void* work(void* arg)
{
	char* p = (char*)arg;
	printf("threadpool callback fuction : %s.\n", p);
	sleep(1);
}

int main(void)
{
	THREAD_POOL* pool = thread_pool_create(10, 20);
	thread_pool_add_task(pool, work, "1");
	thread_pool_add_task(pool, work, "2");
	thread_pool_add_task(pool, work, "3");
	thread_pool_add_task(pool, work, "4");
	thread_pool_add_task(pool, work, "5");
	thread_pool_add_task(pool, work, "6");
	thread_pool_add_task(pool, work, "7");
	thread_pool_add_task(pool, work, "8");
	thread_pool_add_task(pool, work, "9");
	thread_pool_add_task(pool, work, "10");
	thread_pool_add_task(pool, work, "11");
	thread_pool_add_task(pool, work, "12");
	thread_pool_add_task(pool, work, "13");
	thread_pool_add_task(pool, work, "14");
	thread_pool_add_task(pool, work, "15");
	thread_pool_add_task(pool, work, "16");
	thread_pool_add_task(pool, work, "17");
	thread_pool_add_task(pool, work, "18");
	thread_pool_add_task(pool, work, "19");
	thread_pool_add_task(pool, work, "20");
	thread_pool_add_task(pool, work, "21");
	thread_pool_add_task(pool, work, "22");
	thread_pool_add_task(pool, work, "23");
	thread_pool_add_task(pool, work, "24");
	thread_pool_add_task(pool, work, "25");
	thread_pool_add_task(pool, work, "26");
	thread_pool_add_task(pool, work, "27");
	thread_pool_add_task(pool, work, "28");
	thread_pool_add_task(pool, work, "29");
	thread_pool_add_task(pool, work, "30");
	thread_pool_add_task(pool, work, "31");
	thread_pool_add_task(pool, work, "32");
	thread_pool_add_task(pool, work, "33");
	thread_pool_add_task(pool, work, "34");
	thread_pool_add_task(pool, work, "35");
	thread_pool_add_task(pool, work, "36");
	thread_pool_add_task(pool, work, "37");
	thread_pool_add_task(pool, work, "38");
	thread_pool_add_task(pool, work, "39");
	thread_pool_add_task(pool, work, "40");

	sleep(5);
	thread_pool_destroy(pool);
	return 0;
}