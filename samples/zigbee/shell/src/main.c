/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *
 * @brief Zigbee shell sample.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>

#include <zboss_api.h>
#include <zigbee/zigbee_error_handler.h>
#include <zigbee/zigbee_app_utils.h>
#include <zb_nrf_platform.h>
#include "zb_range_extender.h"


/* Device endpoint, used to receive ZCL commands. */
#define APP_ZIGBEE_ENDPOINT              CONFIG_ZIGBEE_SHELL_ENDPOINT

/* Type of power sources available for the device.
 * For possible values see section 3.2.2.2.8 of ZCL specification.
 */
#define SHELL_INIT_BASIC_POWER_SOURCE    ZB_ZCL_BASIC_POWER_SOURCE_DC_SOURCE

/* LED indicating that device successfully joined Zigbee network. */
#define ZIGBEE_NETWORK_STATE_LED         DK_LED3

/* LED used for device identification. */
#ifdef CONFIG_BOARD_NRF52840DONGLE_NRF52840
#define IDENTIFY_LED                     DK_LED1
#else
#define IDENTIFY_LED                     DK_LED4
#endif /* defined CONFIG_BOARD_NRF52840DONGLE_NRF52840 */

/* Button used to enter the Identify mode. */
#ifdef CONFIG_BOARD_NRF52840DONGLE_NRF52840
#define IDENTIFY_MODE_BUTTON             DK_BTN1_MSK
#else
#define IDENTIFY_MODE_BUTTON             DK_BTN4_MSK
#endif /* defined CONFIG_BOARD_NRF52840DONGLE_NRF52840 */

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

/* Main application customizable context.
 * Stores all settings and static values.
 */
struct zb_device_ctx {
	zb_zcl_basic_attrs_t basic_attr;
	zb_zcl_identify_attrs_t identify_attr;
};

/* Zigbee device application context storage. */
static struct zb_device_ctx dev_ctx;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(
	identify_attr_list,
	&dev_ctx.identify_attr.identify_time);

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(
	basic_attr_list,
	&dev_ctx.basic_attr.zcl_version,
	&dev_ctx.basic_attr.power_source);

ZB_DECLARE_RANGE_EXTENDER_CLUSTER_LIST(
	app_template_clusters,
	basic_attr_list,
	identify_attr_list);

ZB_DECLARE_RANGE_EXTENDER_EP(
	app_zigbee_ep,
	APP_ZIGBEE_ENDPOINT,
	app_template_clusters);

ZBOSS_DECLARE_DEVICE_CTX_1_EP(
	app_zigbee_ctx,
	app_zigbee_ep);


/**@brief Function for initializing all clusters attributes. */
static void app_clusters_attr_init(void)
{
	/* Basic cluster attributes data */
	dev_ctx.basic_attr.zcl_version = ZB_ZCL_VERSION;
	dev_ctx.basic_attr.power_source = SHELL_INIT_BASIC_POWER_SOURCE;

	/* Identify cluster attributes data. */
	dev_ctx.identify_attr.identify_time =
		ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;
}

/**@brief Function to toggle the identify LED
 *
 * @param  bufid  Unused parameter, required by ZBOSS scheduler API.
 */
static void toggle_identify_led(zb_bufid_t bufid)
{
	static int blink_status;

	dk_set_led(IDENTIFY_LED, (++blink_status) % 2);
	ZB_SCHEDULE_APP_ALARM(toggle_identify_led, bufid, ZB_MILLISECONDS_TO_BEACON_INTERVAL(100));
}

/**@brief Function to handle identify notification events on the first endpoint.
 *
 * @param  bufid  Unused parameter, required by ZBOSS scheduler API.
 */
static void identify_cb(zb_bufid_t bufid)
{
	zb_ret_t zb_err_code;

	if (bufid) {
		/* Schedule a self-scheduling function that will toggle the LED */
		ZB_SCHEDULE_APP_CALLBACK(toggle_identify_led, bufid);
	} else {
		/* Cancel the toggling function alarm and turn off LED */
		zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(toggle_identify_led, ZB_ALARM_ANY_PARAM);
		ZVUNUSED(zb_err_code);

		dk_set_led(IDENTIFY_LED, 0);
	}
}

/**@brief Starts identifying the device.
 *
 * @param  bufid  Unused parameter, required by ZBOSS scheduler API.
 */
static void start_identifying(zb_bufid_t bufid)
{
	ZVUNUSED(bufid);

	if (ZB_JOINED()) {
		/* Check if endpoint is in identifying mode,
		 * if not put desired endpoint in identifying mode.
		 */
		if (dev_ctx.identify_attr.identify_time ==
		    ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE) {

			zb_ret_t zb_err_code = zb_bdb_finding_binding_target(
				APP_ZIGBEE_ENDPOINT);

			if (zb_err_code == RET_OK) {
				LOG_INF("Enter identify mode");
			} else if (zb_err_code == RET_INVALID_STATE) {
				LOG_WRN("RET_INVALID_STATE - Cannot enter identify mode");
			} else {
				ZB_ERROR_CHECK(zb_err_code);
			}
		} else {
			LOG_INF("Cancel identify mode");
			zb_bdb_finding_binding_target_cancel();
		}
	} else {
		LOG_WRN("Device not in a network - cannot enter identify mode");
	}
}

/**@brief Callback for button events.
 *
 * @param[in]   button_state  Bitmask containing buttons state.
 * @param[in]   has_changed   Bitmask containing buttons that have changed their state.
 */
static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	if (IDENTIFY_MODE_BUTTON & has_changed) {
		if (IDENTIFY_MODE_BUTTON & button_state) {
			/* Button changed its state to pressed */
		} else {
			/* Button changed its state to released */
			ZB_SCHEDULE_APP_CALLBACK(start_identifying, 0);
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
 * @param[in]   bufid   Reference to the Zigbee stack buffer used to pass signal.
 */
void zboss_signal_handler(zb_bufid_t bufid)
{
	zb_zdo_app_signal_hdr_t *sig_handler = NULL;
	zb_zdo_app_signal_type_t sig = zb_get_app_signal(bufid, &sig_handler);
	zb_ret_t status = ZB_GET_APP_SIGNAL_STATUS(bufid);

	/* Update network status LED. */
	zigbee_led_status_update(bufid, ZIGBEE_NETWORK_STATE_LED);

	switch (sig) {
	case ZB_ZDO_SIGNAL_LEAVE:
		if (status == RET_OK) {
			zb_zdo_signal_leave_params_t *leave_params =
				ZB_ZDO_SIGNAL_GET_PARAMS(
					sig_handler,
					zb_zdo_signal_leave_params_t);
			LOG_INF("Network left (leave type: %d)",
				leave_params->leave_type);
			/* Let the user decide when to start the next formation
			 * or rejoin procedure.
			 */
		} else {
			LOG_ERR("Unable to leave network (status: %d)", status);
		}
		break;
	default:
		/* Call default signal handler. */
		ZB_ERROR_CHECK(zigbee_default_signal_handler(bufid));
		break;
	}

	if (bufid) {
		zb_buf_free(bufid);
	}
}

int main(void)
{
	LOG_INF("Starting Zigbee shell application");

	/* Initialize */
	configure_gpio();

	/* Register device context (endpoints). */
	ZB_AF_REGISTER_DEVICE_CTX(&app_zigbee_ctx);

	app_clusters_attr_init();

	/* Register handlers to identify notifications */
	ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(APP_ZIGBEE_ENDPOINT, identify_cb);

	LOG_INF("Zigbee shell application started");

	return 0;
}
