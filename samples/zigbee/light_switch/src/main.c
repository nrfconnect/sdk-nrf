/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Dimmer switch for HA profile implementation.
 */

#include <zephyr.h>
#include <device.h>
#include <logging/log.h>
#include <dk_buttons_and_leds.h>
#include <ram_pwrdn.h>

#include <zboss_api.h>
#include <zboss_api_addons.h>
#include <zigbee/zigbee_app_utils.h>
#include <zigbee/zigbee_error_handler.h>
#include <zb_nrf_platform.h>
#include "zb_mem_config_custom.h"

#if CONFIG_ZIGBEE_FOTA
#include <zigbee/zigbee_fota.h>
#include <power/reboot.h>
#include <dfu/mcuboot.h>

/* LED indicating OTA Client Activity. */
#define OTA_ACTIVITY_LED          DK_LED2
#endif /* CONFIG_ZIGBEE_FOTA */

#if CONFIG_BT_NUS
#include "nus_cmd.h"

/* LED which indicates that Central is connected. */
#define NUS_STATUS_LED            DK_LED1
/* UART command that will turn on found light bulb(s). */
#define COMMAND_ON                "n"
/**< UART command that will turn off found light bulb(s). */
#define COMMAND_OFF               "f"
/**< UART command that will turn toggle found light bulb(s). */
#define COMMAND_TOGGLE            "t"
/**< UART command that will increase brightness of found light bulb(s). */
#define COMMAND_INCREASE          "i"
/**< UART command that will decrease brightness of found light bulb(s). */
#define COMMAND_DECREASE          "d"
#endif /* CONFIG_BT_NUS */

/* Source endpoint used to control light bulb. */
#define LIGHT_SWITCH_ENDPOINT      1
/* Delay between the light switch startup and light bulb finding procedure. */
#define MATCH_DESC_REQ_START_DELAY K_SECONDS(2)
/* Timeout for finding procedure. */
#define MATCH_DESC_REQ_TIMEOUT     K_SECONDS(5)
/* Find only non-sleepy device. */
#define MATCH_DESC_REQ_ROLE        ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE

/* Do not erase NVRAM to save the network parameters after device reboot or
 * power-off. NOTE: If this option is set to ZB_TRUE then do full device erase
 * for all network devices before running other samples.
 */
#define ERASE_PERSISTENT_CONFIG    ZB_FALSE
/* LED indicating that light switch successfully joind Zigbee network. */
#define ZIGBEE_NETWORK_STATE_LED   DK_LED3
/* LED indicating that light witch found a light bulb to control. */
#define BULB_FOUND_LED             DK_LED4
/* Button ID used to switch on the light bulb. */
#define BUTTON_ON                  DK_BTN1_MSK
/* Button ID used to switch off the light bulb. */
#define BUTTON_OFF                 DK_BTN2_MSK
/* Dim step size - increases/decreses current level (range 0x000 - 0xfe). */
#define DIMM_STEP                  15
/* Button ID used to enable sleepy behavior. */
#define BUTTON_SLEEPY              DK_BTN3_MSK

/* Transition time for a single step operation in 0.1 sec units.
 * 0xFFFF - immediate change.
 */
#define DIMM_TRANSACTION_TIME      2

/* Time after which the button state is checked again to detect button hold,
 * the dimm command is sent again.
 */
#define BUTTON_LONG_POLL_TMO       K_MSEC(500)

#if !defined ZB_ED_ROLE
#error Define ZB_ED_ROLE to compile light switch (End Device) source code.
#endif

LOG_MODULE_REGISTER(app);

struct bulb_context {
	zb_uint8_t     endpoint;
	zb_uint16_t    short_addr;
	struct k_timer find_alarm;
};

struct buttons_context {
	uint32_t       state;
	atomic_t       long_poll;
	struct k_timer alarm;
};

static struct bulb_context bulb_ctx;
static struct buttons_context buttons_ctx;
static zb_uint8_t  attr_zcl_version = ZB_ZCL_VERSION;
static zb_uint8_t  attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;
static zb_uint16_t attr_identify_time;

/* Declare attribute list for Basic cluster. */
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &attr_zcl_version,
				 &attr_power_source);

/* Declare attribute list for Identify cluster. */
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &attr_identify_time);

/* Declare cluster list for Dimmer Switch device (Identify, Basic, Scenes,
 * Groups, On Off, Level Control).
 * Only clusters Identify and Basic have attributes.
 */
ZB_HA_DECLARE_DIMMER_SWITCH_CLUSTER_LIST(dimmer_switch_clusters,
					 basic_attr_list,
					 identify_attr_list);

/* Declare endpoint for Dimmer Switch device. */
ZB_HA_DECLARE_DIMMER_SWITCH_EP(dimmer_switch_ep,
			       LIGHT_SWITCH_ENDPOINT,
			       dimmer_switch_clusters);

/* Declare application's device context (list of registered endpoints)
 * for Dimmer Switch device.
 */
#ifndef CONFIG_ZIGBEE_FOTA
ZBOSS_DECLARE_DEVICE_CTX_1_EP(dimmer_switch_ctx, dimmer_switch_ep);
#else

  #if LIGHT_SWITCH_ENDPOINT == CONFIG_ZIGBEE_FOTA_ENDPOINT
    #error "Light switch and Zigbee OTA endpoints should be different."
  #endif

extern zb_af_endpoint_desc_t zigbee_fota_client_ep;
ZBOSS_DECLARE_DEVICE_CTX_2_EP(dimmer_switch_ctx,
			      zigbee_fota_client_ep,
			      dimmer_switch_ep);
#endif /* CONFIG_ZIGBEE_FOTA */

/* Forward declarations. */
static void light_switch_button_handler(struct k_timer *timer);
static void find_light_bulb_alarm(struct k_timer *timer);
static void find_light_bulb(zb_bufid_t bufid);
static void light_switch_send_on_off(zb_bufid_t bufid, zb_uint16_t on_off);


/**@brief Callback for button events.
 *
 * @param[in]   button_state  Bitmask containing buttons state.
 * @param[in]   has_changed   Bitmask containing buttons that has
 *                            changed their state.
 */
static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	zb_uint16_t cmd_id;
	zb_ret_t zb_err_code;

	/* Inform default signal handler about user input at the device. */
	user_input_indicate();

	if (bulb_ctx.short_addr == 0xFFFF) {
		LOG_DBG("No bulb found yet.");
		return;
	}

	switch (has_changed) {
	case BUTTON_ON:
		LOG_DBG("ON - button changed");
		cmd_id = ZB_ZCL_CMD_ON_OFF_ON_ID;
		break;
	case BUTTON_OFF:
		LOG_DBG("OFF - button changed");
		cmd_id = ZB_ZCL_CMD_ON_OFF_OFF_ID;
		break;
	default:
		LOG_DBG("Unhandled button");
		return;
	}

	switch (button_state) {
	case BUTTON_ON:
	case BUTTON_OFF:
		LOG_DBG("Button pressed");
		buttons_ctx.state = button_state;

		/* Alarm can be scheduled only once. Next alarm only resets
		 * counting.
		 */
		k_timer_start(&buttons_ctx.alarm, BUTTON_LONG_POLL_TMO,
			      K_NO_WAIT);
		break;
	case 0:
		LOG_DBG("Button released");

		k_timer_stop(&buttons_ctx.alarm);

		if (atomic_set(&buttons_ctx.long_poll, ZB_FALSE) == ZB_FALSE) {
			/* Allocate output buffer and send on/off command. */
			zb_err_code = zb_buf_get_out_delayed_ext(
				light_switch_send_on_off, cmd_id, 0);
			ZB_ERROR_CHECK(zb_err_code);
		}
	}
}

/**@brief Function for initializing LEDs and Buttons. */
static void configure_gpio(void)
{
	int err;

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("Cannot init buttons (err: %d)", err);
	}

	err = dk_leds_init();
	if (err) {
		LOG_ERR("Cannot init LEDs (err: %d)", err);
	}
}

static void alarm_timers_init(void)
{
	k_timer_init(&buttons_ctx.alarm, light_switch_button_handler, NULL);
	k_timer_init(&bulb_ctx.find_alarm, find_light_bulb_alarm, NULL);
}

/**@brief Function for sending ON/OFF requests to the light bulb.
 *
 * @param[in]   bufid    Non-zero reference to Zigbee stack buffer that will be
 *                       used to construct on/off request.
 * @param[in]   cmd_id   ZCL command id.
 */
static void light_switch_send_on_off(zb_bufid_t bufid, zb_uint16_t cmd_id)
{
	LOG_INF("Send ON/OFF command: %d", cmd_id);

	ZB_ZCL_ON_OFF_SEND_REQ(bufid,
			       bulb_ctx.short_addr,
			       ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
			       bulb_ctx.endpoint,
			       LIGHT_SWITCH_ENDPOINT,
			       ZB_AF_HA_PROFILE_ID,
			       ZB_ZCL_DISABLE_DEFAULT_RESPONSE,
			       cmd_id,
			       NULL);
}

/**@brief Function for sending step requests to the light bulb.
 *
 * @param[in]   bufid        Non-zero reference to Zigbee stack buffer that
 *                           will be used to construct step request.
 * @param[in]   cmd_id       ZCL command id.
 */
static void light_switch_send_step(zb_bufid_t bufid, zb_uint16_t cmd_id)
{
	LOG_INF("Send step level command: %d", cmd_id);

	ZB_ZCL_LEVEL_CONTROL_SEND_STEP_REQ(bufid,
					   bulb_ctx.short_addr,
					   ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
					   bulb_ctx.endpoint,
					   LIGHT_SWITCH_ENDPOINT,
					   ZB_AF_HA_PROFILE_ID,
					   ZB_ZCL_DISABLE_DEFAULT_RESPONSE,
					   NULL,
					   cmd_id,
					   DIMM_STEP,
					   DIMM_TRANSACTION_TIME);
}

/**@brief Callback function receiving finding procedure results.
 *
 * @param[in]   bufid   Reference to Zigbee stack buffer used to pass
 *                      received data.
 */
static void find_light_bulb_cb(zb_bufid_t bufid)
{
	/* Get the beginning of the response. */
	zb_zdo_match_desc_resp_t *resp =
			       (zb_zdo_match_desc_resp_t *) zb_buf_begin(bufid);
	/* Get the pointer to the parameters buffer, which stores APS layer
	 * response.
	 */
	zb_apsde_data_indication_t *ind  = ZB_BUF_GET_PARAM(bufid,
						zb_apsde_data_indication_t);
	zb_uint8_t *match_ep;

	if ((resp->status == ZB_ZDP_STATUS_SUCCESS) &&
		(resp->match_len > 0) &&
		(bulb_ctx.short_addr == 0xFFFF)) {

		/* Match EP list follows right after response header. */
		match_ep = (zb_uint8_t *)(resp + 1);

		/* We are searching for exact cluster, so only 1 EP
		 * may be found.
		 */
		bulb_ctx.endpoint   = *match_ep;
		bulb_ctx.short_addr = ind->src_addr;

		LOG_INF("Found bulb addr: %d ep: %d",
			bulb_ctx.short_addr,
			bulb_ctx.endpoint);

		k_timer_stop(&bulb_ctx.find_alarm);
		dk_set_led_on(BULB_FOUND_LED);
	} else {
		LOG_INF("Bulb not found, try again");
	}

	if (bufid) {
		zb_buf_free(bufid);
	}
}

/**@brief Find bulb allarm handler.
 *
 * @param[in]   timer   Address of timer.
 */
static void find_light_bulb_alarm(struct k_timer *timer)
{
	ZB_ERROR_CHECK(zb_buf_get_out_delayed(find_light_bulb));
}

/**@brief Function for sending ON/OFF and Level Control find request.
 *
 * @param[in]   bufid   Reference to Zigbee stack buffer that will be used to
 *                      construct find request.
 */
static void find_light_bulb(zb_bufid_t bufid)
{
	zb_zdo_match_desc_param_t *req;

	/* Initialize pointers inside buffer and reserve space for
	 * zb_zdo_match_desc_param_t request.
	 */
	req = zb_buf_initial_alloc(bufid,
		sizeof(zb_zdo_match_desc_param_t) + (1) * sizeof(zb_uint16_t));

	req->nwk_addr         = MATCH_DESC_REQ_ROLE;
	req->addr_of_interest = MATCH_DESC_REQ_ROLE;
	req->profile_id       = ZB_AF_HA_PROFILE_ID;

	/* We are searching for 2 clusters: On/Off and Level Control Server. */
	req->num_in_clusters  = 2;
	req->num_out_clusters = 0;
	req->cluster_list[0]  = ZB_ZCL_CLUSTER_ID_ON_OFF;
	req->cluster_list[1]  = ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL;

	/* Set 0xFFFF to reset short address in order to parse
	 * only one response.
	 */
	bulb_ctx.short_addr = 0xFFFF;
	(void)zb_zdo_match_desc_req(bufid, find_light_bulb_cb);
}

/**@brief Callback for detecting button press duration.
 *
 * @param[in]   timer   Address of timer.
 */
static void light_switch_button_handler(struct k_timer *timer)
{
	zb_ret_t zb_err_code;
	zb_uint16_t cmd_id;

	if (dk_get_buttons() & buttons_ctx.state) {
		atomic_set(&buttons_ctx.long_poll, ZB_TRUE);
		if (buttons_ctx.state == BUTTON_ON) {
			cmd_id = ZB_ZCL_LEVEL_CONTROL_STEP_MODE_UP;
		} else {
			cmd_id = ZB_ZCL_LEVEL_CONTROL_STEP_MODE_DOWN;
		}

		/* Allocate output buffer and send step command. */
		zb_err_code = zb_buf_get_out_delayed_ext(light_switch_send_step,
							 cmd_id,
							 0);
		if (!zb_err_code) {
			LOG_WRN("Buffer is full");
		}

		k_timer_start(&buttons_ctx.alarm, BUTTON_LONG_POLL_TMO,
			      K_NO_WAIT);
	} else {
		atomic_set(&buttons_ctx.long_poll, ZB_FALSE);
	}
}

#ifdef CONFIG_ZIGBEE_FOTA
static void confirm_image(void)
{
	if (!boot_is_img_confirmed()) {
		int ret = boot_write_img_confirmed();

		if (ret) {
			LOG_ERR("Couldn't confirm image: %d", ret);
		} else {
			LOG_INF("Marked image as OK");
		}
	}
}

static void ota_evt_handler(const struct zigbee_fota_evt *evt)
{
	switch (evt->id) {
	case ZIGBEE_FOTA_EVT_PROGRESS:
		dk_set_led(OTA_ACTIVITY_LED, evt->dl.progress % 2);
		break;

	case ZIGBEE_FOTA_EVT_FINISHED:
		LOG_INF("Reboot application.");
		sys_reboot(SYS_REBOOT_COLD);
		break;

	case ZIGBEE_FOTA_EVT_ERROR:
		LOG_ERR("OTA image transfer failed.");
		break;

	default:
		break;
	}
}
#endif /* CONFIG_ZIGBEE_FOTA */

/**@brief Zigbee stack event handler.
 *
 * @param[in]   bufid   Reference to the Zigbee stack buffer
 *                      used to pass signal.
 */
void zboss_signal_handler(zb_bufid_t bufid)
{
	zb_zdo_app_signal_hdr_t    *sig_hndler = NULL;
	zb_zdo_app_signal_type_t    sig = zb_get_app_signal(bufid, &sig_hndler);
	zb_ret_t                    status = ZB_GET_APP_SIGNAL_STATUS(bufid);

	/* Update network status LED. */
	zigbee_led_status_update(bufid, ZIGBEE_NETWORK_STATE_LED);

#ifdef CONFIG_ZIGBEE_FOTA
	/* Pass signal to the OTA client implementation. */
	zigbee_fota_signal_handler(bufid);
#endif /* CONFIG_ZIGBEE_FOTA */

	switch (sig) {
	case ZB_BDB_SIGNAL_DEVICE_REBOOT:
	/* fall-through */
	case ZB_BDB_SIGNAL_STEERING:
		/* Call default signal handler. */
		ZB_ERROR_CHECK(zigbee_default_signal_handler(bufid));
		if (status == RET_OK) {
			/* Check the light device address. */
			if (bulb_ctx.short_addr == 0xFFFF) {
				k_timer_start(&bulb_ctx.find_alarm,
					      MATCH_DESC_REQ_START_DELAY,
					      MATCH_DESC_REQ_TIMEOUT);
			}
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

#if CONFIG_BT_NUS

static void turn_on_cmd(struct k_work *item)
{
	ARG_UNUSED(item);
	zb_buf_get_out_delayed_ext(light_switch_send_on_off,
				   ZB_ZCL_CMD_ON_OFF_ON_ID, 0);
}

static void turn_off_cmd(struct k_work *item)
{
	ARG_UNUSED(item);
	zb_buf_get_out_delayed_ext(light_switch_send_on_off,
				   ZB_ZCL_CMD_ON_OFF_OFF_ID, 0);
}

static void toggle_cmd(struct k_work *item)
{
	ARG_UNUSED(item);
	zb_buf_get_out_delayed_ext(light_switch_send_on_off,
				   ZB_ZCL_CMD_ON_OFF_TOGGLE_ID, 0);
}

static void increase_cmd(struct k_work *item)
{
	ARG_UNUSED(item);
	zb_buf_get_out_delayed_ext(light_switch_send_step,
				   ZB_ZCL_LEVEL_CONTROL_STEP_MODE_UP, 0);
}

static void decrease_cmd(struct k_work *item)
{
	ARG_UNUSED(item);
	zb_buf_get_out_delayed_ext(light_switch_send_step,
				   ZB_ZCL_LEVEL_CONTROL_STEP_MODE_DOWN, 0);
}

static void on_nus_connect(struct k_work *item)
{
	ARG_UNUSED(item);
	dk_set_led_on(NUS_STATUS_LED);
}

static void on_nus_disconnect(struct k_work *item)
{
	ARG_UNUSED(item);
	dk_set_led_off(NUS_STATUS_LED);
}

static struct nus_entry commands[] = {
	NUS_COMMAND(COMMAND_ON, turn_on_cmd),
	NUS_COMMAND(COMMAND_OFF, turn_off_cmd),
	NUS_COMMAND(COMMAND_TOGGLE, toggle_cmd),
	NUS_COMMAND(COMMAND_INCREASE, increase_cmd),
	NUS_COMMAND(COMMAND_DECREASE, decrease_cmd),
	NUS_COMMAND(NULL, NULL),
};

#endif /* CONFIG_BT_NUS */

void main(void)
{
	LOG_INF("Starting ZBOSS Light Switch example");

	/* Initialize. */
	configure_gpio();
	alarm_timers_init();

	zigbee_erase_persistent_storage(ERASE_PERSISTENT_CONFIG);
	zb_set_ed_timeout(ED_AGING_TIMEOUT_64MIN);
	zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(3000));

	/* Set default bulb short_addr. */
	bulb_ctx.short_addr = 0xFFFF;

	/* If "sleepy button" is defined, check its state during Zigbee
	 * initialization and enable sleepy behavior at device if defined button
	 * is pressed. Additionally, power off unused sections of RAM to lower
	 * device power consumption.
	 */
#if defined BUTTON_SLEEPY
	if (dk_get_buttons() & BUTTON_SLEEPY) {
		zigbee_configure_sleepy_behavior(true);

		if (IS_ENABLED(CONFIG_RAM_POWER_DOWN_LIBRARY)) {
			power_down_unused_ram();
		}
	}
#endif

#ifdef CONFIG_ZIGBEE_FOTA
	/* Initialize Zigbee FOTA download service. */
	zigbee_fota_init(ota_evt_handler);

	/* Mark the current firmware as valid. */
	confirm_image();

	/* Register callback for handling ZCL commands. */
	ZB_ZCL_REGISTER_DEVICE_CB(zigbee_fota_zcl_cb);
#endif /* CONFIG_ZIGBEE_FOTA */

	/* Register dimmer switch device context (endpoints). */
	ZB_AF_REGISTER_DEVICE_CTX(&dimmer_switch_ctx);

	/* Start Zigbee default thread. */
	zigbee_enable();

#if CONFIG_BT_NUS
	/* Initalize NUS command service. */
	nus_cmd_init(on_nus_connect, on_nus_disconnect, commands);
#endif /* CONFIG_BT_NUS */

	LOG_INF("ZBOSS Light Switch example started");

	while (1) {
		k_sleep(K_FOREVER);
	}
}
