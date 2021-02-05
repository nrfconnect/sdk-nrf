/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>
#include <zephyr/types.h>
#include <zephyr.h>

#include <zboss_api.h>
#include <zigbee/zigbee_app_utils.h>
#include <zb_nrf_platform.h>


#define STACKSIZE               1024
#define PRIORITY                (CONFIG_ZBOSS_DEFAULT_THREAD_PRIORITY + 1)
#define N_THREADS               4

#define IEEE_CHANNEL_MASK       (1l << CONFIG_ZIGBEE_CHANNEL)


K_SEM_DEFINE(zboss_init_lock, 0, 1);
static uint32_t zboss_signals_collected;

/**@brief Zigbee stack event handler.
 *
 * @param[in]   bufid   Reference to the Zigbee stack buffer
 *                      used to pass signal.
 */
void zboss_signal_handler(zb_bufid_t bufid)
{
	zb_zdo_app_signal_hdr_t *sg_p = NULL;
	zb_zdo_app_signal_type_t sig = zb_get_app_signal(bufid, &sg_p);
	zb_ret_t status = ZB_GET_APP_SIGNAL_STATUS(bufid);

	/* Count received signals. */
	zboss_signals_collected++;

	switch (sig) {
	case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
		zassert_not_equal(status, RET_OK,
			"Production configuration must not be present on erased device.");
		break;

	case ZB_COMMON_SIGNAL_CAN_SLEEP:
		/* Enter poll so other tasks may be processed.
		 * This should be the third and the last startup signal.
		 */
		k_sem_give(&zboss_init_lock);
		zb_sleep_now();
		break;

	default:
		zassert_equal(sig, ZB_ZDO_SIGNAL_SKIP_STARTUP,
			"Unexpected signal received after zboss_start_no_autostart().");
		break;
	}

	/* Free passed buffer to avoid buffer leaks. */
	if (bufid) {
		zb_buf_free(bufid);
	}
}

void test_zboss_startup_signals(void)
{
	zboss_signals_collected = 0;

	/*
	 * The whole ZBOSS flash area needs to be erased,
	 * so the timeout value has to be quite big.
	 */
	k_sem_take(&zboss_init_lock, K_MSEC(1000));

	zassert_equal(zboss_signals_collected, 3,
		"ZBOSS startup failed. Not all startup signals were collected.");
}


K_MSGQ_DEFINE(zb_callback_queue, sizeof(uint32_t), N_THREADS, 4);

void add_to_queue_from_callback(uint8_t int_to_put)
{
	const uint32_t ref_int[N_THREADS] = {0, 1, 2, 3};
	int ret_val;

	ret_val = k_msgq_put(&zb_callback_queue,
			     (void *)&ref_int[int_to_put],
			     K_NO_WAIT);
	__ASSERT(ret_val == 0, "Can not put data into the queue");
}


void thread_0(void)
{
	zigbee_schedule_callback(add_to_queue_from_callback, 0);
	while (1) {
		k_sleep(K_MSEC(10));
	}
}

void thread_1(void)
{
	zigbee_schedule_callback(add_to_queue_from_callback, 1);
	while (1) {
		k_sleep(K_MSEC(10));
	}
}

void thread_2(void)
{
	zigbee_schedule_callback(add_to_queue_from_callback, 2);
	while (1) {
		k_sleep(K_MSEC(10));
	}
}

void thread_3(void)
{
	zigbee_schedule_callback(add_to_queue_from_callback, 3);
	while (1) {
		k_sleep(K_MSEC(10));
	}
}

K_THREAD_DEFINE(THREAD_0, STACKSIZE, thread_0, NULL, NULL, NULL,
		PRIORITY, 0, -1);

K_THREAD_DEFINE(THREAD_1, STACKSIZE, thread_1, NULL, NULL, NULL,
		PRIORITY, 0, -1);

K_THREAD_DEFINE(THREAD_2, STACKSIZE, thread_2, NULL, NULL, NULL,
		PRIORITY, 0, -1);

K_THREAD_DEFINE(THREAD_3, STACKSIZE, thread_3, NULL, NULL, NULL,
		PRIORITY, 0, -1);

void test_zboss_app_callbacks(void)
{
	k_tid_t thread_id_array[N_THREADS] = {THREAD_0, THREAD_1,
					      THREAD_2, THREAD_3};
	uint8_t expected_queue_usage_cnt = 0;

	for (uint8_t i = 0; i < ARRAY_SIZE(thread_id_array); i++) {
		k_thread_start(thread_id_array[i]);
		expected_queue_usage_cnt++;

		/* Let callback threads to execute. */
		k_sleep(K_MSEC(10));

		zassert_equal(
			expected_queue_usage_cnt,
			k_msgq_num_used_get(&zb_callback_queue),
			"Queue usage cnt differs from expected usage count.");
	}

	for (uint8_t i = 0; i < ARRAY_SIZE(thread_id_array); i++) {
		int data;
		int err = k_msgq_get(&zb_callback_queue, &data, K_NO_WAIT);

		zassert_equal(err, 0,
			      "Unable to fetch all elements from the queue.");
		zassert_equal(data, i,
			      "Incorrect element found on the queue.");
	}
}

void test_main(void)
{
	/* Erase NVRAM to have repeatability of test runs. */
	zigbee_erase_persistent_storage(ZB_TRUE);

	/* Start Zigbee default thread */
	zigbee_enable();

	ztest_test_suite(zboss_api_callback,
			 ztest_unit_test(test_zboss_startup_signals),
			 ztest_unit_test(test_zboss_app_callbacks));

	ztest_run_test_suite(zboss_api_callback);
}
