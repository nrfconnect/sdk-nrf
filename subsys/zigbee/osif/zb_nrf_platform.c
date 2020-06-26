/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdlib.h>
#include <kernel.h>
#include <logging/log.h>
#include <init.h>

#include <zboss_api.h>
#include "zb_nrf_platform.h"
#include "zb_nrf_crypto.h"


/**
 * Enumeration representing type of application callback to execute from ZBOSS
 * context.
 */
typedef enum {
	ZB_CALLBACK_TYPE_SINGLE_PARAM,
	ZB_CALLBACK_TYPE_TWO_PARAMS,
	ZB_CALLBACK_TYPE_ALARM_SET,
	ZB_CALLBACK_TYPE_ALARM_CANCEL,
	ZB_GET_OUT_BUF_DELAYED,
	ZB_GET_IN_BUF_DELAYED,
	ZB_GET_OUT_BUF_DELAYED_EXT,
	ZB_GET_IN_BUF_DELAYED_EXT,
} zb_callback_type_t;

/**
 * Type definition of element of the application callback and alarm queue.
 */
typedef struct {
	zb_callback_type_t type;
	zb_callback_t func;
	zb_callback2_t func2;
	zb_uint16_t param;
	zb_uint16_t user_param;
	s64_t alarm_timestamp;
} zb_app_cb_t;


LOG_MODULE_REGISTER(zboss_osif, CONFIG_ZBOSS_OSIF_LOG_LEVEL);

/* Signal object to indicate that frame has been received */
static struct k_poll_signal zigbee_sig = K_POLL_SIGNAL_INITIALIZER(zigbee_sig);

/** Global mutex to protect access to the ZBOSS global state.
 *
 * @note Functions for locking/unlocking the mutex are called directly from
 *       ZBOSS core, when the main ZBOSS global variable is accessed.
 */
static K_MUTEX_DEFINE(zigbee_mutex);

/**
 * Message queue, that is used to pass ZBOSS callbacks and alarms from
 * ISR and other threads to ZBOSS main loop context.
 */
K_MSGQ_DEFINE(zb_app_cb_msgq, sizeof(zb_app_cb_t),
	      CONFIG_ZIGBEE_APP_CB_QUEUE_LENGTH, 4);

/**
 * Work queue that will schedule processing of callbacks from the message queue.
 */
static struct k_work zb_app_cb_work;

/**
 * Atomic flag, indicating that the processing callback is still scheduled for
 * execution,
 */
volatile atomic_t zb_app_cb_process_scheduled = ATOMIC_INIT(0);

K_THREAD_STACK_DEFINE(zboss_stack_area, CONFIG_ZBOSS_DEFAULT_THREAD_STACK_SIZE);
static struct k_thread zboss_thread_data;
static k_tid_t zboss_tid;

static void zb_app_cb_process(zb_bufid_t bufid)
{
	zb_ret_t ret_code = RET_OK;
	zb_app_cb_t new_app_cb;

	/* Mark te processing callback as non-scheduled. */
	(void)atomic_set((atomic_t *)&zb_app_cb_process_scheduled, 0);

	/**
	 * From ZBOSS main loop context: process all requests.
	 *
	 * Note: the ZB_SCHEDULE_APP_ALARM is not thread-safe.
	 */
	while (!k_msgq_peek(&zb_app_cb_msgq, &new_app_cb)) {
		switch (new_app_cb.type) {
		case ZB_CALLBACK_TYPE_SINGLE_PARAM:
			ret_code = zb_schedule_app_callback(
					new_app_cb.func,
					(zb_uint8_t)new_app_cb.param,
					ZB_FALSE,
					0,
					ZB_FALSE);
			break;
		case ZB_CALLBACK_TYPE_TWO_PARAMS:
			ret_code = zb_schedule_app_callback(
					new_app_cb.func,
					(zb_uint8_t)new_app_cb.param,
					ZB_TRUE,
					new_app_cb.user_param,
					ZB_FALSE);
			break;
		case ZB_CALLBACK_TYPE_ALARM_SET:
		{
			/**
			 * Check if the timeout already passed. If so, use the
			 * lowest value that schedules an alarm, so the user
			 * is still able to cancel the alarm.
			 */
			zb_time_t delay =
				(k_uptime_get() > new_app_cb.alarm_timestamp ?
					1 :
					ZB_MILLISECONDS_TO_BEACON_INTERVAL(
						new_app_cb.alarm_timestamp -
						k_uptime_get())
				);
			ret_code = zb_schedule_app_alarm(
					new_app_cb.func,
					(zb_uint8_t)new_app_cb.param,
					delay);
			break;
		}
		case ZB_CALLBACK_TYPE_ALARM_CANCEL:
			ret_code = zb_schedule_alarm_cancel(
					new_app_cb.func,
					(zb_uint8_t)new_app_cb.param,
					NULL);
			break;
		case ZB_GET_OUT_BUF_DELAYED:
			ret_code = zb_buf_get_out_delayed_func(
				TRACE_CALL(new_app_cb.func));
			break;
		case ZB_GET_IN_BUF_DELAYED:
			ret_code = zb_buf_get_in_delayed_func(
				TRACE_CALL(new_app_cb.func));
			break;
		case ZB_GET_OUT_BUF_DELAYED_EXT:
			ret_code = zb_buf_get_out_delayed_ext_func(
					TRACE_CALL(new_app_cb.func2),
					new_app_cb.user_param,
					new_app_cb.param);
			break;
		case ZB_GET_IN_BUF_DELAYED_EXT:
			ret_code = zb_buf_get_in_delayed_ext_func(
					TRACE_CALL(new_app_cb.func2),
					new_app_cb.user_param,
					new_app_cb.param);
			break;
		default:
			break;
		}

		/* Check for ZBOSS scheduler queue overflow. */
		if (ret_code == RET_OVERFLOW) {
			break;
		}

		/* Flush the element from the message queue. */
		k_msgq_get(&zb_app_cb_msgq, &new_app_cb, K_NO_WAIT);
	}

	/**
	 * In case of overflow error - reschedule the processing callback
	 * to process remaining requests later.
	 */
	if (ret_code == RET_OVERFLOW) {
		k_work_submit(&zb_app_cb_work);
	}
}

static void zb_app_cb_process_schedule(struct k_work *item)
{
	zb_app_cb_t new_app_cb;

	if (k_msgq_peek(&zb_app_cb_msgq, &new_app_cb)) {
		return;
	}

	/* Check if processing callback is altready scheduled. */
	if (atomic_set((atomic_t *)&zb_app_cb_process_scheduled, 1) == 1) {
		return;
	}

	/**
	 * From working thread, non-ISR context: schedule processing callback.
	 * Repeat endlessly, because the user was already informed that the
	 * request will be handled.
	 *
	 * Note: the ZB_SCHEDULE_APP_CALLBACK is thread-safe.
	 */
	while (zb_schedule_app_callback(zb_app_cb_process,
					0, ZB_FALSE, 0, ZB_FALSE) != RET_OK) {
		k_sleep(K_MSEC(1000));
	}
	zigbee_event_notify(ZIGBEE_EVENT_APP);

	(void)item;
}

static int zigbee_init(struct device *unused)
{
	zb_ieee_addr_t ieee_addr;
	zb_uint32_t channel_mask;

	/* Initialise work queue for processing app callback and alarms. */
	k_work_init(&zb_app_cb_work, zb_app_cb_process_schedule);

#if ZB_TRACE_LEVEL
	/* Set Zigbee stack logging level and traffic dump subsystem. */
	ZB_SET_TRACE_LEVEL(CONFIG_ZBOSS_TRACE_LOG_LEVEL);
	ZB_SET_TRACE_MASK(CONFIG_ZBOSS_TRACE_MASK);
	ZB_SET_TRAF_DUMP_OFF();
#endif /* ZB_TRACE_LEVEL */

	/* Initialize Zigbee stack. */
	ZB_INIT("zigbee_thread");

	/* Set device address to the value read from FICR registers. */
	zb_osif_get_ieee_eui64(ieee_addr);
	zb_set_long_address(ieee_addr);

	/* Keep or erase NVRAM to save the network parameters
	 * after device reboot or power-off.
	 */
	zb_set_nvram_erase_at_start(ZB_FALSE);

	/* Set channels on which the coordinator will try
	 * to create a new network
	 */
#if defined(CONFIG_ZIGBEE_CHANNEL_SELECTION_MODE_SINGLE)
	channel_mask = (1UL << CONFIG_ZIGBEE_CHANNEL);
#elif defined(CONFIG_ZIGBEE_CHANNEL_SELECTION_MODE_MULTI)
	channel_mask = CONFIG_ZIGBEE_CHANNEL_MASK;
#else
#error Channel mask undefined!
#endif

#if defined(CONFIG_ZIGBEE_ROLE_COORDINATOR)
	zb_set_network_coordinator_role(channel_mask);
#elif defined(CONFIG_ZIGBEE_ROLE_ROUTER)
	zb_set_network_router_role(channel_mask);
#elif defined(CONFIG_ZIGBEE_ROLE_END_DEVICE)
	zb_set_network_ed_role(channel_mask);
#else
#error Zigbee device role undefined!
#endif

	return 0;
}

static void zboss_thread(void *arg1, void *arg2, void *arg3)
{
	zb_ret_t zb_err_code;

	zb_err_code = zboss_start_no_autostart();
	__ASSERT(zb_err_code == RET_OK, "Error when starting ZBOSS stack!");

	while (1) {
		zboss_main_loop_iteration();
	}
}

zb_ret_t zigbee_schedule_callback(zb_callback_t func, zb_uint8_t param)
{
	zb_app_cb_t new_app_cb = {
		.type = ZB_CALLBACK_TYPE_SINGLE_PARAM,
		.func = func,
		.param = param,
	};

	if (k_msgq_put(&zb_app_cb_msgq, &new_app_cb, K_NO_WAIT)) {
		return RET_OVERFLOW;
	}

	k_work_submit(&zb_app_cb_work);
	return RET_OK;
}

zb_ret_t zigbee_schedule_callback2(zb_callback_t func,
				   zb_uint8_t param,
				   zb_uint16_t user_param)
{
	zb_app_cb_t new_app_cb = {
		.type = ZB_CALLBACK_TYPE_TWO_PARAMS,
		.func = func,
		.param = param,
		.user_param = user_param,
	};

	if (k_msgq_put(&zb_app_cb_msgq, &new_app_cb, K_NO_WAIT)) {
		return RET_OVERFLOW;
	}

	k_work_submit(&zb_app_cb_work);
	return RET_OK;
}

zb_ret_t zigbee_schedule_alarm(zb_callback_t func,
			       zb_uint8_t param,
			       zb_time_t run_after)
{
	zb_app_cb_t new_app_cb = {
		.type = ZB_CALLBACK_TYPE_ALARM_SET,
		.func = func,
		.param = param,
		.alarm_timestamp = k_uptime_get() +
				   ZB_TIME_BEACON_INTERVAL_TO_MSEC(run_after),
	};

	if (k_msgq_put(&zb_app_cb_msgq, &new_app_cb, K_NO_WAIT)) {
		return RET_OVERFLOW;
	}

	k_work_submit(&zb_app_cb_work);
	return RET_OK;
}

zb_ret_t zigbee_schedule_alarm_cancel(zb_callback_t func, zb_uint8_t param)
{
	zb_app_cb_t new_app_cb = {
		.type = ZB_CALLBACK_TYPE_ALARM_CANCEL,
		.func = func,
		.param = param,
	};

	if (k_msgq_put(&zb_app_cb_msgq, &new_app_cb, K_NO_WAIT)) {
		return RET_OVERFLOW;
	}

	k_work_submit(&zb_app_cb_work);
	return RET_OK;
}

zb_ret_t zigbee_get_out_buf_delayed(zb_callback_t func)
{
	zb_app_cb_t new_app_cb = {
		.type = ZB_GET_OUT_BUF_DELAYED,
		.func = func,
	};

	if (k_msgq_put(&zb_app_cb_msgq, &new_app_cb, K_NO_WAIT)) {
		return RET_OVERFLOW;
	}

	k_work_submit(&zb_app_cb_work);
	return RET_OK;
}

zb_ret_t zigbee_get_in_buf_delayed(zb_callback_t func)
{
	zb_app_cb_t new_app_cb = {
		.type = ZB_GET_IN_BUF_DELAYED,
		.func = func,
	};

	if (k_msgq_put(&zb_app_cb_msgq, &new_app_cb, K_NO_WAIT)) {
		return RET_OVERFLOW;
	}

	k_work_submit(&zb_app_cb_work);
	return RET_OK;
}

zb_ret_t zigbee_get_out_buf_delayed_ext(zb_callback2_t func, zb_uint16_t param,
					zb_uint16_t max_size)
{
	zb_app_cb_t new_app_cb = {
		.type = ZB_GET_OUT_BUF_DELAYED_EXT,
		.func2 = func,
		.user_param = param,
		.param = max_size,
	};

	if (k_msgq_put(&zb_app_cb_msgq, &new_app_cb, K_NO_WAIT)) {
		return RET_OVERFLOW;
	}

	k_work_submit(&zb_app_cb_work);
	return RET_OK;
}

zb_ret_t zigbee_get_in_buf_delayed_ext(zb_callback2_t func, zb_uint16_t param,
					zb_uint16_t max_size)
{
	zb_app_cb_t new_app_cb = {
		.type = ZB_GET_IN_BUF_DELAYED_EXT,
		.func2 = func,
		.user_param = param,
		.param = max_size,
	};

	if (k_msgq_put(&zb_app_cb_msgq, &new_app_cb, K_NO_WAIT)) {
		return RET_OVERFLOW;
	}

	k_work_submit(&zb_app_cb_work);
	return RET_OK;
}

/**@brief SoC general initialization. */
void zb_osif_init(void)
{
	static bool platform_inited;

	if (platform_inited) {
		return;
	}
	platform_inited = true;

#ifdef CONFIG_ZB_HAVE_SERIAL
	/* Initialise serial trace */
	zb_osif_serial_init();
#endif

	/* Initialise random generator */
	zb_osif_rng_init();

	/* Initialise AES ECB */
	zb_osif_aes_init();

#ifdef ZB_USE_SLEEP
	/* Initialise power consumption routines */
	zb_osif_sleep_init();
#endif /*ZB_USE_SLEEP*/
}

void zb_osif_abort(void)
{
	LOG_ERR("Fatal error occurred");
	k_fatal_halt(K_ERR_KERNEL_PANIC);
}

zb_void_t zb_reset(zb_uint8_t param)
{
	ZVUNUSED(param);

	LOG_ERR("Fatal error occurred");
	k_fatal_halt(K_ERR_KERNEL_PANIC);
}

void zb_osif_enable_all_inter(void)
{
	__ASSERT(zb_osif_is_inside_isr() == 0,
		 "Unable to unlock mutex from interrupt context");
	k_mutex_unlock(&zigbee_mutex);
}

zb_bool_t zb_osif_is_inside_isr(void)
{
	return (zb_bool_t)(__get_IPSR() != 0);
}

void zb_osif_disable_all_inter(void)
{
	__ASSERT(zb_osif_is_inside_isr() == 0,
		 "Unable to lock mutex from interrupt context");
	k_mutex_lock(&zigbee_mutex, K_FOREVER);
}

void zb_osif_busy_loop_delay(zb_uint32_t count)
{
	k_busy_wait(count);
}

__weak zb_uint32_t zb_get_utc_time(void)
{
	LOG_ERR("Unable to obtain UTC time. "
		"Please implement %s in your application to provide the current UTC time.",
		__func__);
	return ZB_TIME_BEACON_INTERVAL_TO_MSEC(ZB_TIMER_GET()) / 1000;
}

/**@brief Read IEEE long address from FICR registers. */
void zb_osif_get_ieee_eui64(zb_ieee_addr_t ieee_eui64)
{
	u64_t factoryAddress;

	/* Read random address from FICR. */
	factoryAddress = (uint64_t)NRF_FICR->DEVICEID[0] << 32;
	factoryAddress |= NRF_FICR->DEVICEID[1];

	/* Set constant manufacturer ID to use MAC compression mechanisms. */
	factoryAddress &= 0x000000FFFFFFFFFFLL;
	factoryAddress |= (uint64_t)(CONFIG_ZIGBEE_VENDOR_OUI) << 40;

	memcpy(ieee_eui64, &factoryAddress, sizeof(factoryAddress));
}

void zigbee_event_notify(zigbee_event_t event)
{
	k_poll_signal_raise(&zigbee_sig, event);
}

u32_t zigbee_event_poll(u32_t timeout_ms)
{
	/* Configure event/signals to wait for in wait_for_event function */
	static struct k_poll_event wait_events[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &zigbee_sig),
	};

	unsigned int signaled;
	int result;
	s64_t time_stamp = k_uptime_get();

	k_poll(wait_events, 1, K_MSEC(timeout_ms));

	k_poll_signal_check(&zigbee_sig, &signaled, &result);
	if (signaled) {
		k_poll_signal_reset(&zigbee_sig);

		LOG_DBG("Received new Zigbee event: 0x%02x", result);
	}

	return k_uptime_delta(&time_stamp);
}

void zigbee_enable(void)
{
	zboss_tid = k_thread_create(&zboss_thread_data,
				    zboss_stack_area,
				    K_THREAD_STACK_SIZEOF(zboss_stack_area),
				    zboss_thread,
				    NULL, NULL, NULL,
				    CONFIG_ZBOSS_DEFAULT_THREAD_PRIORITY,
				    0, K_NO_WAIT);
	k_thread_name_set(&zboss_thread_data, "zboss");
}

SYS_INIT(zigbee_init, POST_KERNEL, CONFIG_ZBOSS_DEFAULT_THREAD_PRIORITY);
