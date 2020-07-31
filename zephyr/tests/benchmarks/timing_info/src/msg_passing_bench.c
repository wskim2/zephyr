/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <zephyr.h>
#include <tc_util.h>
#include <ksched.h>
#include "timing_info.h"

extern char sline[];

/* mailbox*/
/* K_MBOX_DEFINE(test_msg_queue) */
K_MSGQ_DEFINE(benchmark_q, sizeof(int), 10, 4);
K_MSGQ_DEFINE(benchmark_q_get, sizeof(int), 3, 4);
K_MBOX_DEFINE(benchmark_mbox);

/* Declare a semaphore for the msgq*/
K_SEM_DEFINE(mbox_sem, 1, 1);

/* common location for the swap to write the tsc data*/
extern uint32_t arch_timing_value_swap_end;
extern uint64_t arch_timing_value_swap_common;

/* location of the time stamps*/
uint64_t __msg_q_put_state;
uint64_t __msg_q_get_state;

uint64_t msg_q_put_w_cxt_start_time;
uint64_t msg_q_put_w_cxt_end_time;

uint64_t msg_q_put_wo_cxt_start_time;  /* without context switch */
uint64_t msg_q_put_wo_cxt_end_time;

uint64_t msg_q_get_w_cxt_start_time;
uint64_t msg_q_get_w_cxt_end_time;

uint64_t msg_q_get_wo_cxt_start_time;
uint64_t msg_q_get_wo_cxt_end_time;

uint32_t __mbox_sync_put_state;
uint64_t mbox_sync_put_start_time;
uint64_t mbox_sync_put_end_time;

uint32_t __mbox_sync_get_state;
uint64_t mbox_sync_get_start_time;
uint64_t mbox_sync_get_end_time;

uint64_t mbox_async_put_start_time;
uint64_t mbox_async_put_end_time;

uint64_t mbox_get_w_cxt_start_time;
uint64_t mbox_get_w_cxt_end_time;

/*For benchmarking msg queues*/
k_tid_t producer_w_cxt_switch_tid;
k_tid_t producer_wo_cxt_switch_tid;
k_tid_t producer_get_w_cxt_switch_tid;
k_tid_t consumer_get_w_cxt_switch_tid;
extern k_tid_t consumer_tid;
k_tid_t thread_mbox_sync_put_send_tid;
k_tid_t thread_mbox_sync_put_receive_tid;
k_tid_t thread_mbox_sync_get_send_tid;
k_tid_t thread_mbox_sync_get_receive_tid;
k_tid_t thread_mbox_async_put_send_tid;
k_tid_t thread_mbox_async_put_receive_tid;

/* To time thread creation*/
extern K_THREAD_STACK_DEFINE(my_stack_area, STACK_SIZE);
extern K_THREAD_STACK_DEFINE(my_stack_area_0, STACK_SIZE);
extern struct k_thread my_thread;
extern struct k_thread my_thread_0;

/* thread functions*/
void thread_producer_msgq_w_cxt_switch(void *p1, void *p2, void *p3);
void thread_producer_msgq_wo_cxt_switch(void *p1, void *p2, void *p3);

void thread_producer_get_msgq_w_cxt_switch(void *p1, void *p2, void *p3);
void thread_consumer_get_msgq_w_cxt_switch(void *p1, void *p2, void *p3);

void thread_mbox_sync_put_send(void *p1, void *p2, void *p3);
void thread_mbox_sync_put_receive(void *p1, void *p2, void *p3);

void thread_mbox_sync_get_send(void *p1, void *p2, void *p3);
void thread_mbox_sync_get_receive(void *p1, void *p2, void *p3);

void thread_mbox_async_put_send(void *p1, void *p2, void *p3);
void thread_mbox_async_put_receive(void *p1, void *p2, void *p3);

volatile uint64_t time_check;
int received_data_get;
int received_data_consumer;
int data_to_send;

#define MBOX_CHECK(status) { if (status != 0) { \
		if (status == -ENOMSG) {		      \
			TC_PRINT("Returned -ENOMSG\n");	      \
			return;				      \
		} else {         /* Status will be -EAGAIN */ \
			TC_PRINT("Returned -EAGAIN\n");	      \
			return;				      \
		}					      \
	}						      \
}					      \

void msg_passing_bench(void)
{
	uint32_t total_cycles;

	/*******************************************************************/
	/* Msg queue for put*/
	int received_data = 0;

	producer_w_cxt_switch_tid =
		k_thread_create(&my_thread, my_stack_area, STACK_SIZE,
				thread_producer_msgq_w_cxt_switch, NULL,
				NULL, NULL, 2 /*priority*/, 0, K_MSEC(50));

	uint32_t msg_status =  k_msgq_get(&benchmark_q, &received_data,
				       K_MSEC(300));

	producer_wo_cxt_switch_tid =
		k_thread_create(&my_thread_0, my_stack_area_0, STACK_SIZE,
				thread_producer_msgq_wo_cxt_switch,
				NULL, NULL, NULL, -2 /*priority*/, 0,
				K_NO_WAIT);

	k_thread_abort(producer_w_cxt_switch_tid);
	k_thread_abort(producer_wo_cxt_switch_tid);
	msg_q_put_w_cxt_end_time = (uint32_t)arch_timing_value_swap_common;
	ARG_UNUSED(msg_status);

	/*******************************************************************/

	/* Msg queue for get*/

	producer_get_w_cxt_switch_tid =
		k_thread_create(&my_thread, my_stack_area,
				STACK_SIZE,
				thread_producer_get_msgq_w_cxt_switch, NULL,
				NULL, NULL, 1 /*priority*/, 0, K_MSEC(50));
	consumer_get_w_cxt_switch_tid =
		k_thread_create(&my_thread_0, my_stack_area_0,
				STACK_SIZE,
				thread_consumer_get_msgq_w_cxt_switch,
				NULL, NULL, NULL,
				2 /*priority*/, 0, K_MSEC(50));
	k_sleep(K_MSEC(2000));  /* make the main thread sleep */
	k_thread_abort(producer_get_w_cxt_switch_tid);
	msg_q_get_w_cxt_end_time = (arch_timing_value_swap_common);

	/*******************************************************************/

	/* Msg queue for get*/
	/* from previous step got the msg_q full now just do a simple read*/
	TIMING_INFO_PRE_READ();
	msg_q_get_wo_cxt_start_time = TIMING_INFO_OS_GET_TIME();

	received_data_get =  k_msgq_get(&benchmark_q_get,
					&received_data_consumer,
					K_NO_WAIT);

	TIMING_INFO_PRE_READ();
	msg_q_get_wo_cxt_end_time = TIMING_INFO_OS_GET_TIME();


	/*******************************************************************/

	/* Msg box to benchmark sync put */

	thread_mbox_sync_put_send_tid  =
		k_thread_create(&my_thread, my_stack_area,
				STACK_SIZE,
				thread_mbox_sync_put_send,
				NULL, NULL, NULL,
				2 /*priority*/, 0, K_NO_WAIT);
	thread_mbox_sync_put_receive_tid =
		k_thread_create(&my_thread_0, my_stack_area_0,
				STACK_SIZE,
				thread_mbox_sync_put_receive,
				NULL, NULL, NULL,
				1 /*priority*/, 0, K_NO_WAIT);
	k_sleep(K_MSEC(1000));  /* make the main thread sleep */
	mbox_sync_put_end_time = (arch_timing_value_swap_common);

	/*******************************************************************/

	/* Msg box to benchmark sync get */

	thread_mbox_sync_get_send_tid  =
		k_thread_create(&my_thread, my_stack_area,
				STACK_SIZE,
				thread_mbox_sync_get_send,
				NULL, NULL, NULL,
				1 /*prio*/, 0, K_NO_WAIT);
	thread_mbox_sync_get_receive_tid =
		k_thread_create(&my_thread_0, my_stack_area_0,
				STACK_SIZE,
				thread_mbox_sync_get_receive, NULL,
				NULL, NULL, 2 /*priority*/, 0, K_NO_WAIT);
	k_sleep(K_MSEC(1000)); /* make the main thread sleep */
	mbox_sync_get_end_time = (arch_timing_value_swap_common);

	/*******************************************************************/

	/* Msg box to benchmark async put */

	thread_mbox_async_put_send_tid  =
		k_thread_create(&my_thread, my_stack_area,
				STACK_SIZE,
				thread_mbox_async_put_send,
				NULL, NULL, NULL,
				2 /*prio*/, 0, K_NO_WAIT);
	thread_mbox_async_put_receive_tid =
		k_thread_create(&my_thread_0, my_stack_area_0,
				STACK_SIZE,
				thread_mbox_async_put_receive,
				NULL, NULL, NULL,
				3 /*priority*/, 0, K_NO_WAIT);
	k_sleep(K_MSEC(1000)); /* make the main thread sleep */

	/*******************************************************************/
	int single_element_buffer = 0, status;
	struct k_mbox_msg rx_msg = {
		.size = sizeof(int),
		.rx_source_thread = K_ANY,
		.tx_target_thread = K_ANY
	};
	TIMING_INFO_PRE_READ();
	mbox_get_w_cxt_start_time = TIMING_INFO_OS_GET_TIME();

	status = k_mbox_get(&benchmark_mbox, &rx_msg, &single_element_buffer,
			    K_MSEC(300));
	MBOX_CHECK(status);

	TIMING_INFO_PRE_READ();
	mbox_get_w_cxt_end_time = TIMING_INFO_OS_GET_TIME();

	/*******************************************************************/

	/* Only print lower 32bit of time result */
	total_cycles = CALCULATE_CYCLES(msg_q, put_w_cxt);
	PRINT_STATS("Message queue put with context switch", total_cycles);

	total_cycles = CALCULATE_CYCLES(msg_q, put_wo_cxt);
	PRINT_STATS("Message queue put without context switch", total_cycles);

	total_cycles = CALCULATE_CYCLES(msg_q, get_w_cxt);
	PRINT_STATS("Message queue get with context switch", total_cycles);

	total_cycles = CALCULATE_CYCLES(msg_q, get_wo_cxt);
	PRINT_STATS("Message queue get without context switch", total_cycles);

	total_cycles = CALCULATE_CYCLES(mbox, sync_put);
	PRINT_STATS("Mailbox synchronous put", total_cycles);

	total_cycles = CALCULATE_CYCLES(mbox, sync_get);
	PRINT_STATS("Mailbox synchronous get", total_cycles);

	total_cycles = CALCULATE_CYCLES(mbox, async_put);
	PRINT_STATS("Mailbox asynchronous put", total_cycles);

	total_cycles = CALCULATE_CYCLES(mbox, get_w_cxt);
	PRINT_STATS("Mailbox get without context switch", total_cycles);
}

void thread_producer_msgq_w_cxt_switch(void *p1, void *p2, void *p3)
{
	int data_to_send = 5050;

	arch_timing_value_swap_end = 1U;
	TIMING_INFO_PRE_READ();
	msg_q_put_w_cxt_start_time = (uint32_t)TIMING_INFO_OS_GET_TIME();
	k_msgq_put(&benchmark_q, &data_to_send, K_NO_WAIT);
}


void thread_producer_msgq_wo_cxt_switch(void *p1, void *p2, void *p3)
{
	int data_to_send = 5050;

	TIMING_INFO_PRE_READ();
	msg_q_put_wo_cxt_start_time = TIMING_INFO_OS_GET_TIME();

	k_msgq_put(&benchmark_q, &data_to_send, K_NO_WAIT);

	TIMING_INFO_PRE_READ();
	msg_q_put_wo_cxt_end_time = TIMING_INFO_OS_GET_TIME();
}


void thread_producer_get_msgq_w_cxt_switch(void *p1, void *p2, void *p3)
{
	int status = 0;

	while (1) {
		if (status == 0) {
			data_to_send++;
		}
		status = k_msgq_put(&benchmark_q_get, &data_to_send,
				    K_MSEC(20));
	}
}

void thread_consumer_get_msgq_w_cxt_switch(void *p1, void *p2, void *p3)
{
	producer_get_w_cxt_switch_tid->base.timeout.dticks = _EXPIRED;
	arch_timing_value_swap_end = 1U;
	TIMING_INFO_PRE_READ();
	msg_q_get_w_cxt_start_time = TIMING_INFO_OS_GET_TIME();
	received_data_get =  k_msgq_get(&benchmark_q_get,
					&received_data_consumer,
					K_MSEC(300));
	TIMING_INFO_PRE_READ();
	time_check = TIMING_INFO_OS_GET_TIME();
}


void thread_mbox_sync_put_send(void *p1, void *p2, void *p3)
{
	int single_element_buffer = 1234, status;
	struct k_mbox_msg tx_msg = {
		.size = sizeof(int),
		.info = 5050,
		.tx_data = &single_element_buffer,
		.rx_source_thread = K_ANY,
		.tx_target_thread = K_ANY,
	};

	TIMING_INFO_PRE_READ();
	mbox_sync_put_start_time = TIMING_INFO_OS_GET_TIME();
	arch_timing_value_swap_end = 1U;

	status = k_mbox_put(&benchmark_mbox, &tx_msg, K_MSEC(300));
	MBOX_CHECK(status);

	TIMING_INFO_PRE_READ();
	time_check = TIMING_INFO_OS_GET_TIME();
}

void thread_mbox_sync_put_receive(void *p1, void *p2, void *p3)
{
	int single_element_buffer = 1234, status;
	struct k_mbox_msg rx_msg = {
		.size = sizeof(int),
		.rx_source_thread = K_ANY,
		.tx_target_thread = K_ANY
	};

	status = k_mbox_get(&benchmark_mbox, &rx_msg, &single_element_buffer,
			    K_MSEC(300));
	MBOX_CHECK(status);
}

void thread_mbox_sync_get_send(void *p1, void *p2, void *p3)
{
	int single_element_buffer = 1234, status;
	struct k_mbox_msg tx_msg = {
		.size = sizeof(int),
		.info = 5050,
		.tx_data = &single_element_buffer,
		.rx_source_thread = K_ANY,
		.tx_target_thread = K_ANY,
	};

	status = k_mbox_put(&benchmark_mbox, &tx_msg, K_MSEC(300));
	MBOX_CHECK(status);
}

void thread_mbox_sync_get_receive(void *p1, void *p2, void *p3)
{
	int single_element_buffer, status;
	struct k_mbox_msg rx_msg = {
		.size = sizeof(int),
		.rx_source_thread = K_ANY,
		.tx_target_thread = K_ANY
	};

	arch_timing_value_swap_end = 1U;
	TIMING_INFO_PRE_READ();
	mbox_sync_get_start_time = TIMING_INFO_OS_GET_TIME();

	status = k_mbox_get(&benchmark_mbox, &rx_msg, &single_element_buffer,
			    K_MSEC(300));
	MBOX_CHECK(status);
}

void thread_mbox_async_put_send(void *p1, void *p2, void *p3)
{
	int single_element_buffer = 1234;
	struct k_mbox_msg tx_msg = {
		.size = sizeof(int),
		.info = 5050,
		.tx_data = &single_element_buffer,
		.rx_source_thread = K_ANY,
		.tx_target_thread = K_ANY,
	};

	TIMING_INFO_PRE_READ();
	mbox_async_put_start_time = TIMING_INFO_OS_GET_TIME();
	k_mbox_async_put(&benchmark_mbox, &tx_msg, &mbox_sem);
	TIMING_INFO_PRE_READ();
	mbox_async_put_end_time = TIMING_INFO_OS_GET_TIME();
	k_mbox_async_put(&benchmark_mbox, &tx_msg, &mbox_sem);
}

void thread_mbox_async_put_receive(void *p1, void *p2, void *p3)
{
	int single_element_buffer, status;
	struct k_mbox_msg rx_msg = {
		.size = sizeof(int),
		.rx_source_thread = K_ANY,
		.tx_target_thread = K_ANY
	};

	status = k_mbox_get(&benchmark_mbox, &rx_msg, &single_element_buffer,
			    K_MSEC(300));
	MBOX_CHECK(status);
}
