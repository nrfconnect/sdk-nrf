/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdbool.h>
#include <stdio.h>
#include <app_event_manager.h>
#include <math.h>
#include <nrf_modem.h>
#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <modem/nrf_modem_lib.h>
#include <modem/pdn.h>

#define MODULE modem_module

#include "modules_common.h"
#include "events/app_module_event.h"
#include "events/data_module_event.h"
#include "events/modem_module_event.h"
#include "events/util_module_event.h"
#include "events/cloud_module_event.h"

#ifdef CONFIG_LWM2M_CARRIER
#include <lwm2m_carrier.h>
#endif /* CONFIG_LWM2M_CARRIER */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_MODEM_MODULE_LOG_LEVEL);

BUILD_ASSERT(!IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT),
		"The Modem module does not support this configuration");


struct modem_msg_data {
	union {
		struct app_module_event app;
		struct cloud_module_event cloud;
		struct util_module_event util;
		struct modem_module_event modem;
	} module;
};

/* Modem module super states. */
static enum state_type {
	/* Initialization state where all libraries that the module depends
	 * on need to be initialized before you can enter any other state.
	 */
	STATE_INIT,
	STATE_DISCONNECTED,
	STATE_CONNECTING,
	STATE_CONNECTED,
	STATE_SHUTDOWN,
} state;

/* Enumerator that specifies the data type that is sampled. */
enum sample_type {
	MODEM_DYNAMIC,
	MODEM_STATIC,
	BATTERY_VOLTAGE
};

/* Struct that holds data from the modem information module. */
static struct modem_param_info modem_param;

/* Value that holds the latest RSRP value. */
static int16_t rsrp_value_latest;

/* Value that holds the latest LTE network mode. */
static enum lte_lc_lte_mode nw_mode_latest;

const k_tid_t module_thread;

/* Modem module message queue. */
#define MODEM_QUEUE_ENTRY_COUNT		10
#define MODEM_QUEUE_BYTE_ALIGNMENT	4

K_MSGQ_DEFINE(msgq_modem, sizeof(struct modem_msg_data),
	      MODEM_QUEUE_ENTRY_COUNT, MODEM_QUEUE_BYTE_ALIGNMENT);

K_SEM_DEFINE(nrf_modem_initialized, 0, 1);
NRF_MODEM_LIB_ON_INIT(asset_tracker_init_hook, on_modem_lib_init, NULL);

static void on_modem_lib_init(int ret, void *ctx)
{
	k_sem_give(&nrf_modem_initialized);
}

static struct module_data self = {
	.name = "modem",
	.msg_q = &msgq_modem,
	.supports_shutdown = true,
};

/* Forward declarations. */
static void send_cell_update(uint32_t cell_id, uint32_t tac);
static void send_psm_update(int tau, int active_time);
static void send_edrx_update(float edrx, float ptw);
static inline int adjust_rsrp(int input);

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type state)
{
	switch (state) {
	case STATE_INIT:
		return "STATE_INIT";
	case STATE_DISCONNECTED:
		return "STATE_DISCONNECTED";
	case STATE_CONNECTING:
		return "STATE_CONNECTING";
	case STATE_CONNECTED:
		return "STATE_CONNECTED";
	case STATE_SHUTDOWN:
		return "STATE_SHUTDOWN";
	default:
		return "Unknown state";
	}
}

static void state_set(enum state_type new_state)
{
	if (new_state == state) {
		LOG_DBG("State: %s", state2str(state));
		return;
	}

	LOG_DBG("State transition %s --> %s",
		state2str(state),
		state2str(new_state));

	state = new_state;
}

/* Handlers */
static bool app_event_handler(const struct app_event_header *aeh)
{
	struct modem_msg_data msg = {0};
	bool enqueue_msg = false;

	if (is_modem_module_event(aeh)) {
		struct modem_module_event *evt = cast_modem_module_event(aeh);

		msg.module.modem = *evt;
		enqueue_msg = true;
	}

	if (is_app_module_event(aeh)) {
		struct app_module_event *evt = cast_app_module_event(aeh);

		msg.module.app = *evt;
		enqueue_msg = true;
	}

	if (is_cloud_module_event(aeh)) {
		struct cloud_module_event *evt = cast_cloud_module_event(aeh);

		msg.module.cloud = *evt;
		enqueue_msg = true;
	}

	if (is_util_module_event(aeh)) {
		struct util_module_event *evt = cast_util_module_event(aeh);

		msg.module.util = *evt;
		enqueue_msg = true;
	}

	if (enqueue_msg) {
		int err = module_enqueue_msg(&self, &msg);

		if (err) {
			LOG_ERR("Message could not be enqueued");
			SEND_ERROR(modem, MODEM_EVT_ERROR, err);
		}
	}

	return false;
}

void nrf_modem_fault_handler(struct nrf_modem_fault_info *fault_info)
{
	LOG_ERR("Modem error: 0x%x, PC: 0x%x", fault_info->reason, fault_info->program_counter);
	SEND_ERROR(modem, MODEM_EVT_ERROR, -EFAULT);
}

static void lte_evt_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS: {
		if (evt->nw_reg_status == LTE_LC_NW_REG_UICC_FAIL) {
			LOG_ERR("No SIM card detected!");
			SEND_ERROR(modem, MODEM_EVT_ERROR, -ENOTSUP);
			break;
		}

		if ((evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME) ||
		    (evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			LOG_DBG("Network registration status: %s",
				evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
				"Connected - home network" : "Connected - roaming");
		}

		break;
	}
	case LTE_LC_EVT_PSM_UPDATE:
		LOG_DBG("PSM parameter update: TAU: %d, Active time: %d",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		send_psm_update(evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE: {
		char log_buf[60];
		ssize_t len;

		len = snprintf(log_buf, sizeof(log_buf),
			       "eDRX parameter update: eDRX: %.2f, PTW: %.2f",
			       evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		if (len > 0) {
			LOG_DBG("%s", log_buf);
		}

		send_edrx_update(evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		break;
	}
	case LTE_LC_EVT_RRC_UPDATE:
		LOG_DBG("RRC mode: %s",
			evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			"Connected" : "Idle");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		LOG_DBG("LTE cell changed: Cell ID: %d, Tracking area: %d",
			evt->cell.id, evt->cell.tac);
		send_cell_update(evt->cell.id, evt->cell.tac);
		break;
	case LTE_LC_EVT_LTE_MODE_UPDATE:
		nw_mode_latest = evt->lte_mode;
		break;
	case LTE_LC_EVT_MODEM_EVENT:
		LOG_DBG("Modem domain event, type: %s",
			evt->modem_evt == LTE_LC_MODEM_EVT_LIGHT_SEARCH_DONE ?
				"Light search done" :
			evt->modem_evt == LTE_LC_MODEM_EVT_SEARCH_DONE ?
				"Search done" :
			evt->modem_evt == LTE_LC_MODEM_EVT_RESET_LOOP ?
				"Reset loop" :
			evt->modem_evt == LTE_LC_MODEM_EVT_BATTERY_LOW ?
				"Low battery" :
			evt->modem_evt == LTE_LC_MODEM_EVT_OVERHEATED ?
				"Modem is overheated" :
				"Unknown");

		/* If a reset loop happens in the field, it should not be necessary
		 * to perform any action. The modem will try to re-attach to the LTE network after
		 * the 30-minute block.
		 */
		if (evt->modem_evt == LTE_LC_MODEM_EVT_RESET_LOOP) {
			LOG_WRN("The modem has detected a reset loop. LTE network attach is now "
					"restricted for the next 30 minutes. Power-cycle the device to "
					"circumvent this restriction.");
			LOG_DBG("For more information see the nRF91 AT Commands - Command "
					"Reference Guide v2.0 - chpt. 5.36");
		}
		break;
	default:
		break;
	}
}

/* Handler that notifies the application of events related to the default PDN context, CID 0. */
void pdn_event_handler(uint8_t cid, enum pdn_event event, int reason)
{
	ARG_UNUSED(cid);

	switch (event) {
	case PDN_EVENT_CNEC_ESM:
		LOG_DBG("Event: PDP context %d, %s", cid, pdn_esm_strerror(reason));
		break;
	case PDN_EVENT_ACTIVATED:
		LOG_DBG("PDN_EVENT_ACTIVATED");
		{ SEND_EVENT(modem, MODEM_EVT_LTE_CONNECTED); }
		break;
	case PDN_EVENT_DEACTIVATED:
		LOG_DBG("PDN_EVENT_DEACTIVATED");
		{ SEND_EVENT(modem, MODEM_EVT_LTE_DISCONNECTED); }
		break;
	case PDN_EVENT_IPV6_UP:
		LOG_DBG("PDN_EVENT_IPV6_UP");
		break;
	case PDN_EVENT_IPV6_DOWN:
		LOG_DBG("PDN_EVENT_IPV6_DOWN");
		break;
	case PDN_EVENT_NETWORK_DETACH:
		LOG_DBG("PDN_EVENT_NETWORK_DETACH");
		break;
	default:
		LOG_WRN("Unexpected PDN event!");
		break;
	}
}

static void modem_rsrp_handler(char rsrp_value)
{
	/* RSRP raw values that represent actual signal strength are
	 * 0 through 97. RSRP is converted to dBm per "nRF91 AT Commands" v1.7.
	 */

	if (rsrp_value > 97) {
		return;
	}

	/* Set temporary variable to hold RSRP value. RSRP callbacks and other
	 * data from the modem info module are retrieved separately.
	 * This temporarily saves the latest value which are sent to
	 * the Data module upon a modem data request.
	 */
	rsrp_value_latest = adjust_rsrp(rsrp_value);

	LOG_DBG("Incoming RSRP status message, RSRP value is %d",
		rsrp_value_latest);
}

#ifdef CONFIG_LWM2M_CARRIER
static void print_carrier_error(const lwm2m_carrier_event_t *evt)
{
	const lwm2m_carrier_event_error_t *err = evt->data.error;
	static const char *const strerr[] = {
		[LWM2M_CARRIER_ERROR_NO_ERROR] =
			"No error",
		[LWM2M_CARRIER_ERROR_BOOTSTRAP] =
			"Bootstrap error",
		[LWM2M_CARRIER_ERROR_LTE_LINK_UP_FAIL] =
			"Failed to connect to the LTE network",
		[LWM2M_CARRIER_ERROR_LTE_LINK_DOWN_FAIL] =
			"Failed to disconnect from the LTE network",
		[LWM2M_CARRIER_ERROR_FOTA_PKG] =
			"Package refused from modem",
		[LWM2M_CARRIER_ERROR_FOTA_PROTO] =
			"Protocol error",
		[LWM2M_CARRIER_ERROR_FOTA_CONN] =
			"Connection to remote server failed",
		[LWM2M_CARRIER_ERROR_FOTA_CONN_LOST] =
			"Connection to remote server lost",
		[LWM2M_CARRIER_ERROR_FOTA_FAIL] =
			"Modem firmware update failed",
		[LWM2M_CARRIER_ERROR_CONFIGURATION] =
			"Illegal object configuration detected",
		[LWM2M_CARRIER_ERROR_INIT] =
			"Initialization failure",
		[LWM2M_CARRIER_ERROR_INTERNAL] =
			"Internal failure",
		[LWM2M_CARRIER_ERROR_RUN] =
			"Configuration failure",
	};

	__ASSERT(PART_OF_ARRAY(strerr, &strerr[err->type]), "Unhandled carrier library error");

	LOG_ERR("%s, reason %d\n", strerr[err->type], err->value);
}

static void print_carrier_deferred_reason(const lwm2m_carrier_event_t *evt)
{
	const lwm2m_carrier_event_deferred_t *def = evt->data.deferred;
	static const char *const strdef[] = {
		[LWM2M_CARRIER_DEFERRED_NO_REASON] =
			"No reason given",
		[LWM2M_CARRIER_DEFERRED_PDN_ACTIVATE] =
			"Failed to activate PDN",
		[LWM2M_CARRIER_DEFERRED_BOOTSTRAP_NO_ROUTE] =
			"No route to bootstrap server",
		[LWM2M_CARRIER_DEFERRED_BOOTSTRAP_CONNECT] =
			"Failed to connect to bootstrap server",
		[LWM2M_CARRIER_DEFERRED_BOOTSTRAP_SEQUENCE] =
			"Bootstrap sequence not completed",
		[LWM2M_CARRIER_DEFERRED_SERVER_NO_ROUTE] =
			"No route to server",
		[LWM2M_CARRIER_DEFERRED_SERVER_CONNECT] =
			"Failed to connect to server",
		[LWM2M_CARRIER_DEFERRED_SERVER_REGISTRATION] =
			"Server registration sequence not completed",
		[LWM2M_CARRIER_DEFERRED_SERVICE_UNAVAILABLE] =
			"Server in maintenance mode",
		[LWM2M_CARRIER_DEFERRED_SIM_MSISDN] =
			"Waiting for SIM MSISDN",
	};

	__ASSERT(PART_OF_ARRAY(strdef, &strdef[def->reason]),
		"Unhandled deferred carrier library error");

	LOG_ERR("Reason: %s, timeout: %d seconds\n", strdef[def->reason], def->timeout);
}

int lwm2m_carrier_event_handler(const lwm2m_carrier_event_t *evt)
{
	int err = 0;

	switch (evt->type) {
	case LWM2M_CARRIER_EVENT_INIT: {
		LOG_INF("LWM2M_CARRIER_EVENT_INIT");
		SEND_EVENT(modem, MODEM_EVT_CARRIER_INITIALIZED);
		break;
	}
	case LWM2M_CARRIER_EVENT_LTE_LINK_UP: {
		LOG_INF("LWM2M_CARRIER_EVENT_LTE_LINK_UP");
		SEND_EVENT(modem, MODEM_EVT_CARRIER_EVENT_LTE_LINK_UP_REQUEST);
		break;
	}
	case LWM2M_CARRIER_EVENT_LTE_LINK_DOWN: {
		LOG_INF("LWM2M_CARRIER_EVENT_LTE_LINK_DOWN");
		SEND_EVENT(modem, MODEM_EVT_CARRIER_EVENT_LTE_LINK_DOWN_REQUEST);
		break;
	}
	case LWM2M_CARRIER_EVENT_LTE_POWER_OFF:
		LOG_INF("LWM2M_CARRIER_EVENT_LTE_POWER_OFF");
		break;
	case LWM2M_CARRIER_EVENT_BOOTSTRAPPED:
		LOG_INF("LWM2M_CARRIER_EVENT_BOOTSTRAPPED");
		break;
	case LWM2M_CARRIER_EVENT_REGISTERED:
		LOG_INF("LWM2M_CARRIER_EVENT_REGISTERED");
		break;
	case LWM2M_CARRIER_EVENT_DEFERRED:
		LOG_INF("LWM2M_CARRIER_EVENT_DEFERRED");
		print_carrier_deferred_reason(evt);
		break;
	case LWM2M_CARRIER_EVENT_FOTA_START: {
		LOG_INF("LWM2M_CARRIER_EVENT_FOTA_START");
		SEND_EVENT(modem, MODEM_EVT_CARRIER_FOTA_PENDING);
		break;
	}
	case LWM2M_CARRIER_EVENT_REBOOT: {
		LOG_INF("LWM2M_CARRIER_EVENT_REBOOT");
		SEND_EVENT(modem, MODEM_EVT_CARRIER_REBOOT_REQUEST);

		/* 1 is returned here to indicate to the carrier library that
		 * the application will handle rebooting of the system to
		 * ensure it happens gracefully. The alternative is to
		 * return 0 and let the library reboot at its convenience.
		 */
		return 1;
	}
	case LWM2M_CARRIER_EVENT_ERROR: {
		const lwm2m_carrier_event_error_t *err = evt->data.error;

		LOG_ERR("LWM2M_CARRIER_EVENT_ERROR");
		print_carrier_error(evt);

		bool fota_error = err->type == LWM2M_CARRIER_ERROR_FOTA_PKG ||
				  err->type == LWM2M_CARRIER_ERROR_FOTA_PROTO ||
				  err->type == LWM2M_CARRIER_ERROR_FOTA_CONN ||
				  err->type == LWM2M_CARRIER_ERROR_FOTA_CONN_LOST ||
				  err->type == LWM2M_CARRIER_ERROR_FOTA_FAIL;
		if (fota_error) {
			SEND_EVENT(modem, MODEM_EVT_CARRIER_FOTA_STOPPED);
		}
		break;
	}
	}
	return err;
}
#endif /* CONFIG_LWM2M_CARRIER */


/* Static module functions. */
static void send_cell_update(uint32_t cell_id, uint32_t tac)
{
	struct modem_module_event *evt = new_modem_module_event();

	__ASSERT(evt, "Not enough heap left to allocate event");

	evt->type = MODEM_EVT_LTE_CELL_UPDATE;
	evt->data.cell.cell_id = cell_id;
	evt->data.cell.tac = tac;

	APP_EVENT_SUBMIT(evt);
}

static void send_psm_update(int tau, int active_time)
{
	struct modem_module_event *evt = new_modem_module_event();

	__ASSERT(evt, "Not enough heap left to allocate event");

	evt->type = MODEM_EVT_LTE_PSM_UPDATE;
	evt->data.psm.tau = tau;
	evt->data.psm.active_time = active_time;

	APP_EVENT_SUBMIT(evt);
}

static void send_edrx_update(float edrx, float ptw)
{
	struct modem_module_event *evt = new_modem_module_event();

	__ASSERT(evt, "Not enough heap left to allocate event");

	evt->type = MODEM_EVT_LTE_EDRX_UPDATE;
	evt->data.edrx.edrx = edrx;
	evt->data.edrx.ptw = ptw;

	APP_EVENT_SUBMIT(evt);
}

static inline int adjust_rsrp(int input)
{
	if (IS_ENABLED(CONFIG_MODEM_DYNAMIC_DATA_CONVERT_RSRP_TO_DBM)) {
		return RSRP_IDX_TO_DBM(input);
	}

	return input;
}

static int static_modem_data_get(void)
{
	int err;

	/* Request data from modem information module. */
	err = modem_info_params_get(&modem_param);
	if (err) {
		LOG_ERR("modem_info_params_get, error: %d", err);
		return err;
	}

	struct modem_module_event *modem_module_event = new_modem_module_event();

	__ASSERT(modem_module_event, "Not enough heap left to allocate event");

	strncpy(modem_module_event->data.modem_static.app_version,
		CONFIG_ASSET_TRACKER_V2_APP_VERSION,
		sizeof(modem_module_event->data.modem_static.app_version) - 1);

	strncpy(modem_module_event->data.modem_static.board_version,
		modem_param.device.board,
		sizeof(modem_module_event->data.modem_static.board_version) - 1);

	strncpy(modem_module_event->data.modem_static.modem_fw,
		modem_param.device.modem_fw.value_string,
		sizeof(modem_module_event->data.modem_static.modem_fw) - 1);

	strncpy(modem_module_event->data.modem_static.iccid,
		modem_param.sim.iccid.value_string,
		sizeof(modem_module_event->data.modem_static.iccid) - 1);

	strncpy(modem_module_event->data.modem_static.imei,
		modem_param.device.imei.value_string,
		sizeof(modem_module_event->data.modem_static.imei) - 1);

	modem_module_event->data.modem_static.app_version
		[sizeof(modem_module_event->data.modem_static.app_version) - 1] = '\0';

	modem_module_event->data.modem_static.board_version
		[sizeof(modem_module_event->data.modem_static.board_version) - 1] = '\0';

	modem_module_event->data.modem_static.modem_fw
		[sizeof(modem_module_event->data.modem_static.modem_fw) - 1] = '\0';

	modem_module_event->data.modem_static.iccid
		[sizeof(modem_module_event->data.modem_static.iccid) - 1] = '\0';

	modem_module_event->data.modem_static.imei
		[sizeof(modem_module_event->data.modem_static.imei) - 1] = '\0';

	modem_module_event->data.modem_static.timestamp = k_uptime_get();
	modem_module_event->type = MODEM_EVT_MODEM_STATIC_DATA_READY;

	APP_EVENT_SUBMIT(modem_module_event);
	return 0;
}

static void populate_event_with_dynamic_modem_data(struct modem_module_event *event,
						   struct modem_param_info *param)
{
	event->data.modem_dynamic.rsrp = rsrp_value_latest;
	event->data.modem_dynamic.band = (uint8_t)param->network.current_band.value;
	event->data.modem_dynamic.nw_mode = nw_mode_latest;
	event->data.modem_dynamic.mcc = modem_param.network.mcc.value;
	event->data.modem_dynamic.mnc = modem_param.network.mnc.value;
	event->data.modem_dynamic.area_code = param->network.area_code.value;
	event->type = MODEM_EVT_MODEM_DYNAMIC_DATA_READY;
	event->data.modem_dynamic.timestamp = k_uptime_get();
	event->data.modem_dynamic.cell_id = (uint32_t)param->network.cellid_dec;

	strncpy(event->data.modem_dynamic.apn,
		modem_param.network.apn.value_string,
		sizeof(event->data.modem_dynamic.apn) - 1);

	event->data.modem_dynamic.apn
		[sizeof(event->data.modem_dynamic.apn) - 1] = '\0';

	strncpy(event->data.modem_dynamic.ip_address,
		modem_param.network.ip_address.value_string,
		sizeof(event->data.modem_dynamic.ip_address) - 1);

	event->data.modem_dynamic.ip_address
		[sizeof(event->data.modem_dynamic.ip_address) - 1] = '\0';

	strncpy(event->data.modem_dynamic.mccmnc,
		modem_param.network.current_operator.value_string,
		sizeof(event->data.modem_dynamic.mccmnc));

	event->data.modem_dynamic.mccmnc
		[sizeof(event->data.modem_dynamic.mccmnc) - 1] = '\0';
}

static int dynamic_modem_data_get(void)
{
	int err;

	/* Request data from modem information module. */
	err = modem_info_params_get(&modem_param);
	if (err) {
		LOG_ERR("modem_info_params_get, error: %d", err);
		return err;
	}

	struct modem_module_event *modem_module_event = new_modem_module_event();

	__ASSERT(modem_module_event, "Not enough heap left to allocate event");

	populate_event_with_dynamic_modem_data(modem_module_event, &modem_param);

	APP_EVENT_SUBMIT(modem_module_event);
	return 0;
}

static bool data_type_is_requested(enum app_module_data_type *data_list,
				   size_t count,
				   enum app_module_data_type type)
{
	for (size_t i = 0; i < count; i++) {
		if (data_list[i] == type) {
			return true;
		}
	}

	return false;
}

static int configure_low_power(void)
{
	int err;
	bool enable = IS_ENABLED(CONFIG_MODEM_AUTO_REQUEST_POWER_SAVING_FEATURES);

	err = lte_lc_psm_req(enable);
	if (err) {
		LOG_ERR("lte_lc_psm_req, error: %d", err);
		return err;
	}

	if (enable) {
		LOG_DBG("PSM requested");
	} else {
		LOG_DBG("PSM disabled");
	}

	return 0;
}

static int lte_connect(void)
{
	int err;

	err = lte_lc_connect_async(lte_evt_handler);
	if (err) {
		LOG_ERR("lte_lc_connect_async, error: %d", err);

		return err;
	}

	SEND_EVENT(modem, MODEM_EVT_LTE_CONNECTING);

	return 0;
}

static int modem_data_init(void)
{
	int err;

	err = modem_info_init();
	if (err) {
		LOG_INF("modem_info_init, error: %d", err);
		return err;
	}

	err = modem_info_params_init(&modem_param);
	if (err) {
		LOG_INF("modem_info_params_init, error: %d", err);
		return err;
	}

	err = modem_info_rsrp_register(modem_rsrp_handler);
	if (err) {
		LOG_INF("modem_info_rsrp_register, error: %d", err);
		return err;
	}

	return 0;
}

static int setup(void)
{
	int err;

	err = lte_lc_init();
	if (err) {
		LOG_ERR("lte_lc_init, error: %d", err);
		return err;
	}

	/* Setup a callback for the default PDP context. */
	err = pdn_default_ctx_cb_reg(pdn_event_handler);
	if (err) {
		LOG_ERR("pdn_default_ctx_cb_reg, error: %d", err);
		return err;
	}

	err = configure_low_power();
	if (err) {
		LOG_ERR("configure_low_power, error: %d", err);
		return err;
	}

	err = lte_lc_modem_events_enable();
	if (err) {
		LOG_WRN("lte_lc_modem_events_enable failed, error: %d", err);
		LOG_DBG("Modem firmware versions older than 1.3.0 do not support "
			"enabling modem domain events");
	}

	err = modem_data_init();
	if (err) {
		LOG_ERR("modem_data_init, error: %d", err);
		return err;
	}

	return 0;
}

/* Message handler for STATE_INIT */
static void on_state_init(struct modem_msg_data *msg)
{
	if (IS_EVENT(msg, modem, MODEM_EVT_CARRIER_INITIALIZED)) {
		int err;

		state_set(STATE_DISCONNECTED);

		err = setup();
		__ASSERT(err == 0, "Failed running setup()");
		SEND_EVENT(modem, MODEM_EVT_INITIALIZED);
	}
}

/* Message handler for STATE_DISCONNECTED. */
static void on_state_disconnected(struct modem_msg_data *msg)
{
	if (IS_EVENT(msg, modem, MODEM_EVT_LTE_CONNECTED)) {
		state_set(STATE_CONNECTED);
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_LTE_CONNECTING)) {
		state_set(STATE_CONNECTING);
	}

	if ((IS_EVENT(msg, app, APP_EVT_LTE_DISCONNECT)) ||
	    (IS_EVENT(msg, modem, MODEM_EVT_CARRIER_EVENT_LTE_LINK_UP_REQUEST)) ||
	    (IS_EVENT(msg, cloud, CLOUD_EVT_LTE_CONNECT))) {
		int err;

		err = lte_connect();
		if (err) {
			LOG_ERR("Failed connecting to LTE, error: %d", err);
			SEND_ERROR(modem, MODEM_EVT_ERROR, err);
		}
	}
}

/* Message handler for STATE_CONNECTING. */
static void on_state_connecting(struct modem_msg_data *msg)
{
	if ((IS_EVENT(msg, app, APP_EVT_LTE_DISCONNECT)) ||
	    (IS_EVENT(msg, cloud, CLOUD_EVT_LTE_DISCONNECT))) {
		int err;

		err = lte_lc_offline();
		if (err) {
			LOG_ERR("LTE disconnect failed, error: %d", err);
			SEND_ERROR(modem, MODEM_EVT_ERROR, err);
			return;
		}

		state_set(STATE_DISCONNECTED);
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_LTE_CONNECTED)) {
		state_set(STATE_CONNECTED);
	}
}

/* Message handler for STATE_CONNECTED. */
static void on_state_connected(struct modem_msg_data *msg)
{
	if (IS_EVENT(msg, modem, MODEM_EVT_LTE_DISCONNECTED)) {
		state_set(STATE_DISCONNECTED);
	}

	if ((IS_EVENT(msg, app, APP_EVT_LTE_DISCONNECT)) ||
	    (IS_EVENT(msg, modem, MODEM_EVT_CARRIER_EVENT_LTE_LINK_DOWN_REQUEST)) ||
	    (IS_EVENT(msg, cloud, CLOUD_EVT_LTE_DISCONNECT))) {
		int err;

		err = lte_lc_offline();
		if (err) {
			LOG_ERR("LTE disconnect failed, error: %d", err);
			SEND_ERROR(modem, MODEM_EVT_ERROR, err);
			return;
		}

		state_set(STATE_DISCONNECTED);
	}
}

/* Message handler for all states. */
static void on_all_states(struct modem_msg_data *msg)
{
	if (IS_EVENT(msg, cloud, CLOUD_EVT_USER_ASSOCIATION_REQUEST)) {
		int err = lte_lc_psm_req(false);

		if (err) {
			LOG_ERR("lte_lc_psm_req, error: %d", err);
			SEND_ERROR(modem, MODEM_EVT_ERROR, err);
		}
	}

	if (IS_EVENT(msg, cloud, CLOUD_EVT_USER_ASSOCIATED)) {

		/* Re-enable low power features after cloud has been associated. */
		int err = configure_low_power();

		if (err) {
			LOG_ERR("configure_low_power, error: %d", err);
			SEND_ERROR(modem, MODEM_EVT_ERROR, err);
		}
	}

	if (IS_EVENT(msg, app, APP_EVT_START)) {
		int err;

		if (IS_ENABLED(CONFIG_LWM2M_CARRIER)) {
			/* The carrier library handles the LTE connection
			 * establishment.
			 */
			return;
		}

		err = lte_connect();
		if (err) {
			LOG_ERR("Failed connecting to LTE, error: %d", err);
			SEND_ERROR(modem, MODEM_EVT_ERROR, err);
			return;
		}
	}

	if (IS_EVENT(msg, app, APP_EVT_DATA_GET)) {
		if (data_type_is_requested(msg->module.app.data_list,
					   msg->module.app.count,
					   APP_DATA_MODEM_STATIC)) {

			int err;

			err = static_modem_data_get();
			if (err) {
				SEND_EVENT(modem,
					MODEM_EVT_MODEM_STATIC_DATA_NOT_READY);
			}
		}

		if (data_type_is_requested(msg->module.app.data_list,
					   msg->module.app.count,
					   APP_DATA_MODEM_DYNAMIC)) {

			int err;

			err = dynamic_modem_data_get();
			if (err) {
				SEND_EVENT(modem,
					MODEM_EVT_MODEM_DYNAMIC_DATA_NOT_READY);
			}
		}
	}

	if (IS_EVENT(msg, util, UTIL_EVT_SHUTDOWN_REQUEST)) {
		lte_lc_power_off();
		state_set(STATE_SHUTDOWN);
		SEND_SHUTDOWN_ACK(modem, MODEM_EVT_SHUTDOWN_READY, self.id);
	}
}

static void module_thread_fn(void)
{
	int err;
	struct modem_msg_data msg = { 0 };

	self.thread_id = k_current_get();

	err = module_start(&self);
	if (err) {
		LOG_ERR("Failed starting module, error: %d", err);
		SEND_ERROR(modem, MODEM_EVT_ERROR, err);
	}

	k_sem_take(&nrf_modem_initialized, K_FOREVER);

	if (IS_ENABLED(CONFIG_LWM2M_CARRIER)) {
		state_set(STATE_INIT);
	} else {
		state_set(STATE_DISCONNECTED);
		SEND_EVENT(modem, MODEM_EVT_INITIALIZED);

		err = setup();
		if (err) {
			LOG_ERR("Failed setting up the modem, error: %d", err);
			SEND_ERROR(modem, MODEM_EVT_ERROR, err);
		}
	}

	while (true) {
		module_get_next_msg(&self, &msg);

		switch (state) {
		case STATE_INIT:
			on_state_init(&msg);
			break;
		case STATE_DISCONNECTED:
			on_state_disconnected(&msg);
			break;
		case STATE_CONNECTING:
			on_state_connecting(&msg);
			break;
		case STATE_CONNECTED:
			on_state_connected(&msg);
			break;
		case STATE_SHUTDOWN:
			/* The shutdown state has no transition. */
			break;
		default:
			LOG_ERR("Invalid state: %d", state);
			break;
		}

		on_all_states(&msg);
	}
}

K_THREAD_DEFINE(modem_module_thread, CONFIG_MODEM_THREAD_STACK_SIZE,
		module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, modem_module_event);
APP_EVENT_SUBSCRIBE(MODULE, app_module_event);
APP_EVENT_SUBSCRIBE(MODULE, cloud_module_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, util_module_event);
