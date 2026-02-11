/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>

#define THREAD_STACK_SIZE 1024
#define THREAD_A_PRIORITY 0
#define THREAD_B_PRIORITY 0
#define COUNT_TIME_MS	  250
#define MAX_REPETITIONS	  50

static volatile uint32_t grtc_isr_call_counter;
static int32_t grtc_channel;

const uint32_t id_thread_a = 1;
const uint32_t id_thread_b = 2;
static volatile bool thread_term_request;

K_SEM_DEFINE(test_done_sem, 0, 1);

/* Thread worker is common for both threads */
static void thread_worker(void *param1, void *param2, void *param3)
{
	uint32_t *thread_id = (uint32_t *)param1;
	uint32_t cycles;

	while (1) {
		if (thread_term_request) {
			break;
		}
		cycles = sys_clock_cycle_get_32();
		printk("[Thread %u] Cycles: %u\n", *thread_id, cycles);
		k_msleep(COUNT_TIME_MS / 2);
	}
}

K_THREAD_DEFINE(thread_a, THREAD_STACK_SIZE, thread_worker, (void *)&id_thread_a, NULL, NULL,
		THREAD_A_PRIORITY, 0, 0);

K_THREAD_DEFINE(thread_b, THREAD_STACK_SIZE, thread_worker, (void *)&id_thread_b, NULL, NULL,
		THREAD_B_PRIORITY, 0, 0);

/*
 * The GRTC timer operation is looped.
 * In each ISR handler call new count value is set
 * to continue.
 */
static void timer_compare_interrupt_handler(int32_t id, uint64_t expire_time, void *user_data)
{
	int err;
	uint64_t test_ticks;
	int64_t compare_value = 0;
	uint32_t cycles;

	cycles = sys_clock_cycle_get_32();
	printk("[GRTC ISR] Cycles: %u\n", cycles);

	if (grtc_isr_call_counter++ < MAX_REPETITIONS) {
		test_ticks = z_nrf_grtc_timer_get_ticks(K_MSEC(COUNT_TIME_MS));
		err = z_nrf_grtc_timer_set(grtc_channel, test_ticks,
					   timer_compare_interrupt_handler, NULL);
		__ASSERT(err == 0, "[ISR] Unexpected error raised when setting timer\n");
		z_nrf_grtc_timer_compare_read(grtc_channel, &compare_value);
		__ASSERT(K_TIMEOUT_EQ(K_TICKS(compare_value), K_TICKS(test_ticks)),
			 "[ISR] Compare register set failed\n");

	} else {
		k_sem_give(&test_done_sem);
	}
}

/*
 * Verify GRTC timer behaviour
 * when 'sys_clock_cycle_get_32()'
 * is called from multiple threads
 * and from timer's compare ISR handler
 */
int main(void)
{
	int err;
	int64_t compare_value;
	uint64_t test_ticks;
	uint32_t cycles;

	grtc_channel = z_nrf_grtc_timer_chan_alloc();
	printk("Allocated GRTC channel %d\n", grtc_channel);
	__ASSERT(grtc_channel > 0, "Failed to allocate GRTC channel\n");

	test_ticks = z_nrf_grtc_timer_get_ticks(K_MSEC(COUNT_TIME_MS));
	err = z_nrf_grtc_timer_set(grtc_channel, test_ticks, timer_compare_interrupt_handler, NULL);
	__ASSERT(err == 0, "Unexpected error raised when setting timer\n");

	z_nrf_grtc_timer_compare_read(grtc_channel, &compare_value);
	__ASSERT(K_TIMEOUT_EQ(K_TICKS(compare_value), K_TICKS(test_ticks)),
		 "Compare register set failed\n");

	printk("GRTC multiple access test started, repetitions: %u\n", MAX_REPETITIONS);
	while (k_sem_take(&test_done_sem, K_NO_WAIT) != 0) {
		cycles = sys_clock_cycle_get_32();
		printk("[Main] Cycles: %u\n", cycles);
		k_msleep(COUNT_TIME_MS / 4);
	}
	thread_term_request = true;
	printk("Test status: PASS\n");
	z_nrf_grtc_timer_chan_free(grtc_channel);

	return 0;
}
