/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "coremark.h"

#define THREAD_STACK_SIZE   (512)

#define _COREMARK_THREAD_STACK_ARRAY_ITEM(n, _) CONCAT(CONCAT(coremark_thread_, n), _stack)

#define _COREMARK_THREAD_STACK_DEFINE(n, _)							\
	static K_THREAD_STACK_DEFINE(_COREMARK_THREAD_STACK_ARRAY_ITEM(n, _), THREAD_STACK_SIZE)

/**
 *  @brief Statically define thread stack structure array.
 *
 *  Helper macro to statically define thread stack structure array.
 *
 *  @param _name	 Name of stack structure array.
 *  @param _instance_num Number of elements in instance array.
 */
#define COREMARK_THREAD_STACK_INSTANCE_DEFINE(_name, _instance_num)		 \
	LISTIFY(_instance_num, _COREMARK_THREAD_STACK_DEFINE, (;));		 \
	static k_thread_stack_t *_name[] = {					 \
		LISTIFY(_instance_num, _COREMARK_THREAD_STACK_ARRAY_ITEM, (,))	 \
	}

/*
 * Predefined seed values are required by CoreMark for given run types.
 */
#if CONFIG_COREMARK_RUN_TYPE_VALIDATION
volatile ee_s32 seed1_volatile = 0x3415;
volatile ee_s32 seed2_volatile = 0x3415;
volatile ee_s32 seed3_volatile = 0x66;
#endif

#if CONFIG_COREMARK_RUN_TYPE_PERFORMANCE
volatile ee_s32 seed1_volatile = 0x0;
volatile ee_s32 seed2_volatile = 0x0;
volatile ee_s32 seed3_volatile = 0x66;
#endif

#if CONFIG_COREMARK_RUN_TYPE_PROFILE
volatile ee_s32 seed1_volatile = 0x8;
volatile ee_s32 seed2_volatile = 0x8;
volatile ee_s32 seed3_volatile = 0x8;
#endif


#ifndef CONFIG_COREMARK_THREADS_PRIORITY
#define CONFIG_COREMARK_THREADS_PRIORITY 0
#endif

#ifndef CONFIG_COREMARK_THREADS_TIMEOUT_MS
#define CONFIG_COREMARK_THREADS_TIMEOUT_MS 0
#endif

volatile ee_s32 seed4_volatile = CONFIG_COREMARK_ITERATIONS;
volatile ee_s32 seed5_volatile = 0;

ee_u32 default_num_contexts = CONFIG_COREMARK_THREADS_NUMBER;

static CORE_TICKS start_time_val;
static CORE_TICKS stop_time_val;

COREMARK_THREAD_STACK_INSTANCE_DEFINE(thread_stacks, CONFIG_COREMARK_THREADS_NUMBER);

static struct k_thread thread_descriptors[CONFIG_COREMARK_THREADS_NUMBER];

static int thread_cnt;

BUILD_ASSERT((CONFIG_COREMARK_THREADS_NUMBER >= 1), "Number of threads has to be positive");

void start_time(void)
{
	start_time_val = k_cycle_get_32();
}

void stop_time(void)
{
	stop_time_val = k_cycle_get_32();
}

CORE_TICKS get_time(void)
{
	return (stop_time_val - start_time_val);
}

secs_ret time_in_secs(CORE_TICKS ticks)
{
	secs_ret retval = (secs_ret)(k_cyc_to_ms_floor64(ticks)) / MSEC_PER_SEC;
	return retval;
}

void portable_init(core_portable *p, int *argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (sizeof(ee_ptr_int) != sizeof(ee_u8 *)) {
		ee_printf(
			"ERROR! Please define ee_ptr_int to a type that holds a "
			"pointer!\n");
		k_panic();
	}

	if (sizeof(ee_u32) != 4) {
		ee_printf("ERROR! Please define ee_u32 to a 32b unsigned type!\n");
		k_panic();
	}

	p->portable_id = 1;
}

void portable_fini(core_portable *p)
{
	p->portable_id = 0;
}

static void coremark_thread(void *id, void *pres, void *p3)
{
	ARG_UNUSED(id);
	iterate(pres);
}

ee_u8 core_start_parallel(core_results *res)
{
	__ASSERT(thread_cnt < CONFIG_COREMARK_THREADS_NUMBER, "Reached max number of threads");

	(void)k_thread_create(&thread_descriptors[thread_cnt],
			      thread_stacks[thread_cnt],
			      THREAD_STACK_SIZE,
			      coremark_thread,
			      (void *)thread_cnt,
			      res,
			      NULL,
			      CONFIG_COREMARK_THREADS_PRIORITY, 0, K_NO_WAIT);

	/* Thread creation is done from single thread.
	 * CoreMark guarantees number of active thread is limited to defined bound.
	 */

	thread_cnt++;

	return 0;
}

ee_u8 core_stop_parallel(core_results *res)
{
	int ret;

	__ASSERT(thread_cnt > 0, "Can't have negative number of active threads");
	thread_cnt--;

	/* Order in which threads are joined does not matter.
	 * CoreMark will never start threads unless all previously started are joined.
	 * Stack is guaranteed to be used by one thread only.
	 */

	if (thread_cnt == 0) {
		for (size_t i = 0; i < CONFIG_COREMARK_THREADS_NUMBER; i++) {
			ret = k_thread_join(&thread_descriptors[i],
					    K_MSEC(CONFIG_COREMARK_THREADS_TIMEOUT_MS));
			if (ret == -EAGAIN) {
				ee_printf("Error: Thread join failed due to timeout. Consider "
					  "increasing CONFIG_COREMARK_THREADS_TIMEOUT_MS\n");
				k_panic();
			} else if (ret) {
				ee_printf("Error: Thread %d join failed error: %d", i, ret);
				k_panic();
			}
		}
	}

	return 0;
}
