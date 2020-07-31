/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <zephyr.h>
#include <tc_util.h>
#include <ksched.h>
#include "timing_info.h"


extern char sline[];

K_SEM_DEFINE(sem_bench, 0, 1);
K_SEM_DEFINE(sem_bench_1, 0, 1);

/* To time thread creation*/
extern K_THREAD_STACK_DEFINE(my_stack_area, STACK_SIZE);
extern K_THREAD_STACK_DEFINE(my_stack_area_0, STACK_SIZE);
extern struct k_thread my_thread;
extern struct k_thread my_thread_0;

/* uint64_t thread_yield_start_time[1000]; */
/* uint64_t thread_yield_end_time[1000]; */
extern uint64_t thread_start_time;
extern uint64_t thread_end_time;
uint64_t sem_take_start_time;
uint64_t sem_take_end_time;
uint64_t sem_give_start_time;
uint64_t sem_give_end_time;

uint32_t swap_called;
uint64_t test_time2;
uint64_t test_time1;

void thread_sem0_test(void *p1, void *p2, void *p3);
void thread_sem1_test(void *p1, void *p2, void *p3);
void thread_sem0_give_test(void *p1, void *p2, void *p3);
void thread_sem1_give_test(void *p1, void *p2, void *p3);

k_tid_t sem0_tid;
k_tid_t sem1_tid;

extern uint64_t arch_timing_value_swap_common;
extern uint32_t arch_timing_value_swap_end;

void semaphore_bench(void)
{
	uint32_t total_cycles;

	/* Thread yield */

	sem0_tid = k_thread_create(&my_thread, my_stack_area,
				   STACK_SIZE, thread_sem0_test,
				   NULL, NULL, NULL,
				   2 /*priority*/, 0, K_NO_WAIT);
	sem1_tid = k_thread_create(&my_thread_0, my_stack_area_0,
				   STACK_SIZE, thread_sem1_test,
				   NULL, NULL, NULL,
				   2 /*priority*/, 0, K_NO_WAIT);

	k_sleep(K_MSEC(1000));

	sem_take_end_time = (arch_timing_value_swap_common);

	sem0_tid = k_thread_create(&my_thread, my_stack_area,
				   STACK_SIZE, thread_sem0_give_test,
				   NULL, NULL, NULL,
				   2 /*priority*/, 0, K_NO_WAIT);
	sem1_tid = k_thread_create(&my_thread_0, my_stack_area_0,
				   STACK_SIZE, thread_sem1_give_test,
				   NULL, NULL, NULL,
				   2 /*priority*/, 0, K_NO_WAIT);

	k_sleep(K_MSEC(1000));

	sem_give_end_time = (arch_timing_value_swap_common);

	/* Semaphore without context switch */
	TIMING_INFO_PRE_READ();
	uint32_t sem_give_wo_cxt_start_time = TIMING_INFO_OS_GET_TIME();

	k_sem_give(&sem_bench);

	TIMING_INFO_PRE_READ();
	uint32_t sem_give_wo_cxt_end_time = TIMING_INFO_OS_GET_TIME();

	TIMING_INFO_PRE_READ();
	uint32_t sem_take_wo_cxt_start_time = TIMING_INFO_OS_GET_TIME();

	k_sem_take(&sem_bench, K_MSEC(10));

	TIMING_INFO_PRE_READ();
	uint32_t sem_take_wo_cxt_end_time = TIMING_INFO_OS_GET_TIME();

	total_cycles = CALCULATE_CYCLES(sem, take);
	PRINT_STATS("Semaphore take with context switch", total_cycles);

	total_cycles = CALCULATE_CYCLES(sem, give);
	PRINT_STATS("Semaphore give with context switch", total_cycles);

	total_cycles = CALCULATE_CYCLES(sem, take_wo_cxt);
	PRINT_STATS("Semaphore take without context switch", total_cycles);

	total_cycles = CALCULATE_CYCLES(sem, give_wo_cxt);
	PRINT_STATS("Semaphore give without context switch", total_cycles);
}

/******************************************************************************/
K_MUTEX_DEFINE(mutex0);
void mutex_bench(void)
{
	uint32_t total_cycles;

	uint64_t mutex_lock_start_time;
	uint64_t mutex_lock_end_time;
	uint64_t mutex_lock_diff = 0U;

	uint64_t mutex_unlock_start_time;
	uint64_t mutex_unlock_end_time;
	uint64_t mutex_unlock_diff = 0U;
	uint32_t count = 0U;

	for (int i = 0; i < 1000; i++) {
		int64_t before = k_uptime_get();

		TIMING_INFO_PRE_READ();
		mutex_lock_start_time = TIMING_INFO_OS_GET_TIME();

		k_mutex_lock(&mutex0, K_MSEC(100));

		TIMING_INFO_PRE_READ();
		mutex_lock_end_time = TIMING_INFO_OS_GET_TIME();

		TIMING_INFO_PRE_READ();
		mutex_unlock_start_time = TIMING_INFO_OS_GET_TIME();

		k_mutex_unlock(&mutex0);

		TIMING_INFO_PRE_READ();
		mutex_unlock_end_time = TIMING_INFO_OS_GET_TIME();

		/* If timer interrupt occurs we need to omit that sample*/
		int64_t after = k_uptime_get();

		if (after - before) {
			continue;
		}

		count++;

		mutex_lock_diff += CALCULATE_CYCLES(mutex, lock);
		mutex_unlock_diff += CALCULATE_CYCLES(mutex, unlock);
	}

	total_cycles = mutex_lock_diff / count;
	PRINT_STATS("Mutex lock", total_cycles);

	total_cycles = mutex_unlock_diff / count;
	PRINT_STATS("Mutex unlock", total_cycles);
}

/******************************************************************************/
void thread_sem1_test(void *p1, void *p2, void *p3)
{

	k_sem_give(&sem_bench); /* sync the 2 threads*/

	arch_timing_value_swap_end = 1U;
	TIMING_INFO_PRE_READ();
	sem_take_start_time =  TIMING_INFO_OS_GET_TIME();
	k_sem_take(&sem_bench, K_MSEC(10));
}

uint32_t sem_count;
void thread_sem0_test(void *p1, void *p2, void *p3)
{
	k_sem_take(&sem_bench, K_MSEC(10));/* To sync threads */

	k_sem_give(&sem_bench);
	sem_count++;
	k_thread_abort(sem0_tid);
}
/******************************************************************************/
void thread_sem1_give_test(void *p1, void *p2, void *p3)
{
	k_sem_give(&sem_bench);         /* sync the 2 threads*/

	k_sem_take(&sem_bench_1, K_MSEC(1000)); /* clear the previous sem_give*/
}

void thread_sem0_give_test(void *p1, void *p2, void *p3)
{
	k_sem_take(&sem_bench, K_MSEC(10));/* To sync threads */

	/* To make sure that the sem give will cause a swap to occur */
	k_thread_priority_set(sem1_tid, 1);

	arch_timing_value_swap_end = 1U;
	TIMING_INFO_PRE_READ();
	sem_give_start_time =  TIMING_INFO_OS_GET_TIME();
	k_sem_give(&sem_bench_1);

}
