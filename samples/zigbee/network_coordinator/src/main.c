/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Simple Zigbee network coordinator implementation
 */

#include <zephyr.h>
#include <device.h>
#include <logging/log.h>
#include <dk_buttons_and_leds.h>

#include <zboss_api.h>
#include <zb_mem_config_max.h>
#include <zigbee/zigbee_error_handler.h>
#include <zigbee/zigbee_app_utils.h>
#include <zb_nrf_platform.h>


#define RUN_STATUS_LED          DK_LED1
#define RUN_LED_BLINK_INTERVAL  1000

/* LED indicating that network is opened for new nodes */
#define ZIGBEE_NETWORK_STATE_LED          DK_LED3

/* Button which reopens the Zigbee Network */
#define KEY_ZIGBEE_NETWORK_REOPEN         DK_BTN1_MSK

/**
 * If set to ZB_TRUE then device will not open the network
 * after forming or reboot.
 */
#define ZIGBEE_MANUAL_STEERING            ZB_FALSE

#define ZIGBEE_PERMIT_LEGACY_DEVICES      ZB_FALSE

#ifndef ZB_COORDINATOR_ROLE
#error Define ZB_COORDINATOR_ROLE to compile coordinator source code.
#endif

LOG_MODULE_REGISTER(app);


/**@brief Callback used in order to visualise network steering period.
 *
 * @param[in]   param   Not used. Required by callback type definition.
 */
static void steering_finished(zb_uint8_t param)
{
	ARG_UNUSED(param);
	LOG_INF("Network steering finished");
	dk_set_led_off(ZIGBEE_NETWORK_STATE_LED);
}

/**@brief Callback for button events.
 *
 * @param[in]   button_state  Bitmask containing buttons state.
 * @param[in]   has_changed   Bitmask containing buttons
 *                            that has changed their state.
 */
static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	/* Calculate bitmask of buttons that are pressed
	 * and have changed their state.
	 */
	uint32_t buttons = button_state & has_changed;
	zb_bool_t comm_status;

	if (buttons & KEY_ZIGBEE_NETWORK_REOPEN) {
		(void)(ZB_SCHEDULE_APP_ALARM_CANCEL(
			steering_finished, ZB_ALARM_ANY_PARAM));

		comm_status = bdb_start_top_level_commissioning(
			ZB_BDB_NETWORK_STEERING);
		if (comm_status) {
			LOG_INF("Top level comissioning restated");
		} else {
			LOG_INF("Top level comissioning hasn't finished yet!");
		}
	}
}

/**@brief Function for initializing LEDs and Buttons. */
static void configure_gpio(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		LOG_ERR("Cannot init buttons (err: %d)", err);
	}

	err = dk_leds_init();
	if (err) {
		LOG_ERR("Cannot init LEDs (err: %d)", err);
	}
}

/**@brief Zigbee stack event handler.
 *
 * @param[in]   bufid   Reference to the Zigbee stack buffer
 *                      used to pass signal.
 */
void zboss_signal_handler(zb_bufid_t bufid)
{
	/* Read signal description out of memory buffer. */
	zb_zdo_app_signal_hdr_t *sg_p = NULL;
	zb_zdo_app_signal_type_t sig = zb_get_app_signal(bufid, &sg_p);
	zb_ret_t status = ZB_GET_APP_SIGNAL_STATUS(bufid);
	zb_ret_t zb_err_code;
	zb_bool_t comm_status;
	zb_time_t timeout_bi;

	switch (sig) {
	case ZB_BDB_SIGNAL_DEVICE_REBOOT:
		/* BDB initialization completed after device reboot,
		 * use NVRAM contents during initialization.
		 * Device joined/rejoined and started.
		 */
		if (status == RET_OK) {
			if (ZIGBEE_MANUAL_STEERING == ZB_FALSE) {
				LOG_INF("Start network steering");
				comm_status = bdb_start_top_level_commissioning(
					ZB_BDB_NETWORK_STEERING);
				ZB_COMM_STATUS_CHECK(comm_status);
			} else {
				LOG_INF("Coordinator restarted successfully");
			}
		} else {
			LOG_ERR("Failed to initialize Zigbee stack using NVRAM data (status: %d)",
				status);
		}
		break;

	case ZB_BDB_SIGNAL_STEERING:
		if (status == RET_OK) {
			if (ZIGBEE_PERMIT_LEGACY_DEVICES == ZB_TRUE) {
				LOG_INF("Allow pre-Zigbee 3.0 devices to join the network");
				zb_bdb_set_legacy_device_support(1);
			}

			/* Schedule an alarm to notify about the end
			 * of steering period
			 */
			LOG_INF("Network steering started");
			zb_err_code = ZB_SCHEDULE_APP_ALARM(
				steering_finished, 0,
				ZB_TIME_ONE_SECOND *
					ZB_ZGP_DEFAULT_COMMISSIONING_WINDOW);
			ZB_ERROR_CHECK(zb_err_code);
		}
		break;

	case ZB_ZDO_SIGNAL_DEVICE_ANNCE: {
		zb_zdo_signal_device_annce_params_t *dev_annce_params =
			ZB_ZDO_SIGNAL_GET_PARAMS(
				sg_p, zb_zdo_signal_device_annce_params_t);

		LOG_INF("New device commissioned or rejoined (short: 0x%04hx)",
			dev_annce_params->device_short_addr);

		zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(steering_finished,
							   ZB_ALARM_ANY_PARAM);
		if (zb_err_code == RET_OK) {
			LOG_INF("Joining period extended.");
			zb_err_code = ZB_SCHEDULE_APP_ALARM(
				steering_finished, 0,
				ZB_TIME_ONE_SECOND *
					ZB_ZGP_DEFAULT_COMMISSIONING_WINDOW);
			ZB_ERROR_CHECK(zb_err_code);
		}
	} break;

	default:
		/* Call default signal handler. */
		ZB_ERROR_CHECK(zigbee_default_signal_handler(bufid));
		break;
	}

	/* Update network status LED */
	if (ZB_JOINED() &&
	    (ZB_SCHEDULE_GET_ALARM_TIME(steering_finished, ZB_ALARM_ANY_PARAM,
					&timeout_bi) == RET_OK)) {
		dk_set_led_on(ZIGBEE_NETWORK_STATE_LED);
	} else {
		dk_set_led_off(ZIGBEE_NETWORK_STATE_LED);
	}

	/*
	 * All callbacks should either reuse or free passed buffers.
	 * If bufid == 0, the buffer is invalid (not passed).
	 */
	if (bufid) {
		zb_buf_free(bufid);
	}
}

void error(void)
{
	dk_set_leds_state(DK_ALL_LEDS_MSK, DK_NO_LEDS_MSK);

	while (true) {
		/* Spin for ever */
		k_sleep(K_MSEC(1000));
	}
}

void main(void)
{
	int blink_status = 0;

	LOG_INF("Starting ZBOSS Coordinator example");

	/* Initialize */
	configure_gpio();

	/* Start Zigbee default thread */
	zigbee_enable();

	LOG_INF("ZBOSS Coordinator example started");

	while (1) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
