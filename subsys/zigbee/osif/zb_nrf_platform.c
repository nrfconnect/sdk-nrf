/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/init.h>
#include <ram_pwrdn.h>

#include <hal/nrf_power.h>
#if !NRF_POWER_HAS_RESETREAS
#include <hal/nrf_reset.h>
#endif

#ifdef CONFIG_ZIGBEE_SHELL
#include <zigbee/zigbee_shell.h>
#endif
#include <zboss_api.h>
#include "zb_nrf_platform.h"
#include "zb_nrf_crypto.h"

#ifdef CONFIG_ZIGBEE_LIBRARY_NCP_DEV
#define SYS_REBOOT_NCP 0x10
#endif /* CONFIG_ZIGBEE_LIBRARY_NCP_DEV */

/* Value that is returned while reading a single byte from the erased flash page .*/
#define FLASH_EMPTY_BYTE 0xFF
/* Broadcast Pan ID value */
#define ZB_BROADCAST_PAN_ID 0xFFFFU
/* The number of bytes to be checked before concluding that the ZBOSS NVRAM is not initialized. */
#define ZB_PAGE_INIT_CHECK_LEN 32


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
	int64_t alarm_timestamp;
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
static bool stack_is_started;

#ifdef CONFIG_ZIGBEE_DEBUG_FUNCTIONS
/**@brief Function for checking if the ZBOSS thread has been created.
 */
bool zigbee_debug_zboss_thread_is_created(void)
{
	if (zboss_tid) {
		return true;
	}
	return false;
}

/**@brief Function for suspending ZBOSS thread.
 */
void zigbee_debug_suspend_zboss_thread(void)
{
	k_thread_suspend(zboss_tid);
}

/**@brief Function for resuming ZBOSS thread.
 */
void zigbee_debug_resume_zboss_thread(void)
{
	k_thread_resume(zboss_tid);
}

/**@brief Function for getting the state of the Zigbee stack thread
 *        processing suspension.
 */
bool zigbee_is_zboss_thread_suspended(void)
{
	if (zboss_tid) {
		if (!(zboss_tid->base.thread_state & _THREAD_SUSPENDED)) {
			return false;
		}
	}
	return true;
}
#endif /* defined(CONFIG_ZIGBEE_DEBUG_FUNCTIONS) */

/**@brief Function for checking if the Zigbee stack has been started.
 *
 * @retval true   Zigbee stack has been started.
 * @retval false  Zigbee stack has not been started yet.
 */
bool zigbee_is_stack_started(void)
{
	return stack_is_started;
}

static void zb_app_cb_process(zb_bufid_t bufid)
{
	zb_ret_t ret_code = RET_OK;
	zb_app_cb_t new_app_cb;

	/* Mark the processing callback as non-scheduled. */
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
					(zb_uint8_t)new_app_cb.param);
			break;
		case ZB_CALLBACK_TYPE_TWO_PARAMS:
			ret_code = zb_schedule_app_callback2(
					new_app_cb.func2,
					(zb_uint8_t)new_app_cb.param,
					new_app_cb.user_param);
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

	/* Check if processing callback is already scheduled. */
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
	while (zb_schedule_app_callback(zb_app_cb_process, 0) != RET_OK) {
		k_sleep(K_MSEC(1000));
	}
	zigbee_event_notify(ZIGBEE_EVENT_APP);

	(void)item;
}

int zigbee_init(void)
{
	/* Initialise work queue for processing app callback and alarms. */
	k_work_init(&zb_app_cb_work, zb_app_cb_process_schedule);

#if ZB_TRACE_LEVEL
	/* Set Zigbee stack logging level and traffic dump subsystem. */
	ZB_SET_TRACE_LEVEL(CONFIG_ZBOSS_TRACE_LOG_LEVEL);
	ZB_SET_TRACE_MASK(CONFIG_ZBOSS_TRACE_MASK);
#if CONFIG_ZBOSS_TRAF_DUMP
	ZB_SET_TRAF_DUMP_ON();
#else /* CONFIG_ZBOSS_TRAF_DUMP */
	ZB_SET_TRAF_DUMP_OFF();
#endif /* CONFIG_ZBOSS_TRAF_DUMP */
#endif /* ZB_TRACE_LEVEL */

#ifndef CONFIG_ZB_TEST_MODE_MAC
	/* Initialize Zigbee stack. */
	ZB_INIT("zigbee_thread");

	/* Set device address to the value read from FICR registers. */
	zb_ieee_addr_t ieee_addr;
	zb_osif_get_ieee_eui64(ieee_addr);
	zb_set_long_address(ieee_addr);

	/* Keep or erase NVRAM to save the network parameters
	 * after device reboot or power-off.
	 */
	zb_set_nvram_erase_at_start(ZB_FALSE);

	if (IS_ENABLED(CONFIG_ZIGBEE_TC_REJOIN_ENABLED)) {
		zb_secur_set_tc_rejoin_enabled((zb_bool_t)CONFIG_ZIGBEE_TC_REJOIN_ENABLED);
	}

	/* Don't set zigbee role for NCP device */
#ifndef CONFIG_ZIGBEE_LIBRARY_NCP_DEV

	/* Set channels on which the coordinator will try
	 * to create a new network
	 */
#if defined(CONFIG_ZIGBEE_CHANNEL_SELECTION_MODE_SINGLE)
	zb_uint32_t channel_mask = (1UL << CONFIG_ZIGBEE_CHANNEL);
#elif defined(CONFIG_ZIGBEE_CHANNEL_SELECTION_MODE_MULTI)
	zb_uint32_t channel_mask = CONFIG_ZIGBEE_CHANNEL_MASK;
#else
#error Channel mask undefined!
#endif


#if defined(CONFIG_ZIGBEE_ROLE_COORDINATOR)
	zb_set_network_coordinator_role(channel_mask);
#elif defined(CONFIG_ZIGBEE_ROLE_ROUTER)
	zb_set_network_router_role(channel_mask);

/* Enable full distributed network operability. */
#if (ZBOSS_MAJOR == 3U) && (ZBOSS_MINOR == 11U)
	zb_enable_distributed();
#endif

#elif defined(CONFIG_ZIGBEE_ROLE_END_DEVICE)
	zb_set_network_ed_role(channel_mask);
#else
#error Zigbee device role undefined!
#endif

#endif /* CONFIG_ZIGBEE_LIBRARY_NCP_DEV */

#endif /* CONFIG_ZB_TEST_MODE_MAC */

	return 0;
}

static void zboss_thread(void *arg1, void *arg2, void *arg3)
{
	zb_ret_t zb_err_code;

	zb_err_code = zboss_start_no_autostart();
	__ASSERT(zb_err_code == RET_OK, "Error when starting ZBOSS stack!");

	stack_is_started = true;
#ifdef CONFIG_ZIGBEE_SHELL
	zb_shell_configure_endpoint();
#endif /* defined(CONFIG_ZIGBEE_SHELL) */

	while (1) {
		zboss_main_loop_iteration();
	}
}

zb_bool_t zb_osif_is_inside_isr(void)
{
	return (zb_bool_t)(__get_IPSR() != 0);
}

void zb_osif_enable_all_inter(void)
{
	__ASSERT(zb_osif_is_inside_isr() == 0,
		 "Unable to unlock mutex from interrupt context");
	k_mutex_unlock(&zigbee_mutex);
}

void zb_osif_disable_all_inter(void)
{
	__ASSERT(zb_osif_is_inside_isr() == 0,
		 "Unable to lock mutex from interrupt context");
	k_mutex_lock(&zigbee_mutex, K_FOREVER);
}

zb_ret_t zigbee_schedule_callback(zb_callback_t func, zb_uint8_t param)
{
	if ((zboss_tid) && (k_current_get() == zboss_tid) && (!zb_osif_is_inside_isr())) {
		return zb_schedule_app_callback(func, param);
	}

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

zb_ret_t zigbee_schedule_callback2(zb_callback2_t func,
				   zb_uint8_t param,
				   zb_uint16_t user_param)
{
	if ((zboss_tid) && (k_current_get() == zboss_tid) && (!zb_osif_is_inside_isr())) {
		return zb_schedule_app_callback2(func, param, user_param);
	}

	zb_app_cb_t new_app_cb = {
		.type = ZB_CALLBACK_TYPE_TWO_PARAMS,
		.func2 = func,
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
	if ((zboss_tid) && (k_current_get() == zboss_tid) && (!zb_osif_is_inside_isr())) {
		return zb_schedule_app_alarm(func, param, run_after);
	}

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
	if ((zboss_tid) && (k_current_get() == zboss_tid) && (!zb_osif_is_inside_isr())) {
		return zb_schedule_alarm_cancel(func, param, NULL);
	}

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
	if ((zboss_tid) && (k_current_get() == zboss_tid) && (!zb_osif_is_inside_isr())) {
		return zb_buf_get_out_delayed_func(func);
	}

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
	if ((zboss_tid) && (k_current_get() == zboss_tid) && (!zb_osif_is_inside_isr())) {
		return zb_buf_get_in_delayed_func(func);
	}

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
	if ((zboss_tid) && (k_current_get() == zboss_tid) && (!zb_osif_is_inside_isr())) {
		return zb_buf_get_out_delayed_ext_func(func, param, max_size);
	}

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
	if ((zboss_tid) && (k_current_get() == zboss_tid) && (!zb_osif_is_inside_isr())) {
		return zb_buf_get_in_delayed_ext_func(func, param, max_size);
	}

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

#ifdef CONFIG_ZIGBEE_HAVE_SERIAL
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
	/* Log ZBOSS error message and flush logs. */
	LOG_ERR("ZBOSS fatal error occurred");
	LOG_PANIC();

#ifdef CONFIG_ZIGBEE_HAVE_SERIAL
	/* Flush ZBOSS trace logs. */
	ZB_OSIF_SERIAL_FLUSH();
#endif

	/* By default reset device or halt if so configured. */
	if (IS_ENABLED(CONFIG_ZBOSS_RESET_ON_ASSERT)) {
		zb_reset(0);
	}
	if (IS_ENABLED(CONFIG_ZBOSS_HALT_ON_ASSERT)) {
		k_fatal_halt(K_ERR_KERNEL_PANIC);
	}
}

uint32_t zigbee_pibcache_pan_id_clear(void)
{
	/* For consistency with zb_nwk_nib_init(), the 0xFFFFU is used,
	 * i.e. ZB_BROADCAST_PAN_ID.
	 */
	ZB_PIBCACHE_PAN_ID() = ZB_BROADCAST_PAN_ID;
	return ZB_BROADCAST_PAN_ID;
}

void zb_reset(zb_uint8_t param)
{
	uint8_t reas = (uint8_t)SYS_REBOOT_COLD;

	ZVUNUSED(param);

#ifdef CONFIG_ZIGBEE_LIBRARY_NCP_DEV
	reas = (uint8_t)SYS_REBOOT_NCP;
#endif /* CONFIG_ZIGBEE_LIBRARY_NCP_DEV */

	nrf_power_gpregret_set(NRF_POWER, 0, reas);

	/* Power on unused sections of RAM to allow MCUboot to use it. */
	if (IS_ENABLED(CONFIG_RAM_POWER_DOWN_LIBRARY)) {
		power_up_unused_ram();
	}

	sys_reboot(reas);
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

void zigbee_event_notify(zigbee_event_t event)
{
	k_poll_signal_raise(&zigbee_sig, event);
}

uint32_t zigbee_event_poll(uint32_t timeout_us)
{
	/* Configure event/signals to wait for in zigbee_event_poll function. */
	static struct k_poll_event wait_events[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &zigbee_sig),
	};

	unsigned int signaled = 0;
	int result;
	/* Store timestamp of event polling start. */
	int64_t timestamp_poll_start = k_uptime_ticks();

	k_poll(wait_events, 1, K_USEC(timeout_us));

	k_poll_signal_check(&zigbee_sig, &signaled, &result);
	if (signaled) {
		k_poll_signal_reset(&zigbee_sig);

		LOG_DBG("Received new Zigbee event: 0x%02x", result);
	}

	return k_ticks_to_us_floor32(k_uptime_ticks() - timestamp_poll_start);
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

/**
 * @brief Get the reason that triggered the last reset
 *
 * @return @ref reset_source
 * */
zb_uint8_t zb_get_reset_source(void)
{
	uint32_t reas;
	uint8_t zb_reason;
#ifdef CONFIG_ZIGBEE_LIBRARY_NCP_DEV
	static uint8_t zephyr_reset_type = 0xFF;

	/* Read the value at the first API call, then use data from RAM. */
	if (zephyr_reset_type == 0xFF) {
		zephyr_reset_type = nrf_power_gpregret_get(NRF_POWER, 0);
	}
#endif /* CONFIG_ZIGBEE_LIBRARY_NCP_DEV */

#if NRF_POWER_HAS_RESETREAS

	reas = nrf_power_resetreas_get(NRF_POWER);
	nrf_power_resetreas_clear(NRF_POWER, reas);
	if (reas & NRF_POWER_RESETREAS_RESETPIN_MASK) {
		zb_reason = ZB_RESET_SRC_RESET_PIN;
	} else if (reas & NRF_POWER_RESETREAS_SREQ_MASK) {
		zb_reason = ZB_RESET_SRC_SW_RESET;
	} else if (reas) {
		zb_reason = ZB_RESET_SRC_OTHER;
	} else {
		zb_reason = ZB_RESET_SRC_POWER_ON;
	}

#else

	reas = nrf_reset_resetreas_get(NRF_RESET);
	nrf_reset_resetreas_clear(NRF_RESET, reas);
	if (reas & NRF_RESET_RESETREAS_RESETPIN_MASK) {
		zb_reason = ZB_RESET_SRC_RESET_PIN;
	} else if (reas & NRF_RESET_RESETREAS_SREQ_MASK) {
		zb_reason = ZB_RESET_SRC_SW_RESET;
	} else if (reas) {
		zb_reason = ZB_RESET_SRC_OTHER;
	} else {
		zb_reason = ZB_RESET_SRC_POWER_ON;
	}

#endif

#ifdef CONFIG_ZIGBEE_LIBRARY_NCP_DEV
	if ((zb_reason == ZB_RESET_SRC_SW_RESET) &&
	    (zephyr_reset_type != SYS_REBOOT_NCP)) {
		zb_reason = ZB_RESET_SRC_OTHER;
	}

	/* The NCP reset type is used only by this API call.
	 * Reset the value inside the register, so after the next, external
	 * SW reset, the value will not trigger NCP logic.
	 */
	if (zephyr_reset_type == SYS_REBOOT_NCP) {
		nrf_power_gpregret_set(NRF_POWER, 0, (uint8_t)SYS_REBOOT_COLD);
	}
#endif /* CONFIG_ZIGBEE_LIBRARY_NCP_DEV */

	return zb_reason;
}

zb_bool_t zigbee_is_nvram_initialised(void)
{
	zb_uint8_t buf[ZB_PAGE_INIT_CHECK_LEN] = {0};
	zb_uint8_t i;
	zb_ret_t ret_code;

	ret_code = zb_osif_nvram_read(0, 0, buf, sizeof(buf));
	if (ret_code != RET_OK) {
		return ZB_FALSE;
	}

	for (i = 0; i < sizeof(buf); i++) {
		if (buf[i] != FLASH_EMPTY_BYTE) {
			return ZB_TRUE;
		}
	}

	return ZB_FALSE;
}

ZB_WEAK_PRE zb_uint32_t ZB_WEAK zb_osif_get_fw_version(void)
{
	return 0x01;
}

ZB_WEAK_PRE zb_uint32_t ZB_WEAK zb_osif_get_ncp_protocol_version(void)
{
#ifdef ZB_NCP_PROTOCOL_VERSION
	return ZB_NCP_PROTOCOL_VERSION;
#else /* ZB_NCP_PROTOCOL_VERSION */
	return 0x01;
#endif /* ZB_NCP_PROTOCOL_VERSION */
}

ZB_WEAK_PRE zb_ret_t ZB_WEAK zb_osif_bootloader_run_after_reboot(void)
{
	return RET_OK;
}

ZB_WEAK_PRE void ZB_WEAK zb_osif_bootloader_report_successful_loading(void)
{
}
