/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <stdio.h>
#include <event_manager.h>
#include <math.h>
#include <modem/lte_lc.h>
#include <modem/modem_info.h>

#define MODULE modem_module

#include "modules_common.h"
#include "events/app_module_event.h"
#include "events/data_module_event.h"
#include "events/modem_module_event.h"
#include "events/util_module_event.h"
#include "events/cloud_module_event.h"

#include <logging/log.h>
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
	STATE_DISCONNECTED,
	STATE_CONNECTING,
	STATE_CONNECTED,
	STATE_SHUTDOWN,
} state;

/* Struct that holds data from the modem information module. */
static struct modem_param_info modem_param;

/* Value that always holds the latest RSRP value. */
static int16_t rsrp_value_latest;
const k_tid_t module_thread;

/* Modem module message queue. */
#define MODEM_QUEUE_ENTRY_COUNT		10
#define MODEM_QUEUE_BYTE_ALIGNMENT	4

K_MSGQ_DEFINE(msgq_modem, sizeof(struct modem_msg_data),
	      MODEM_QUEUE_ENTRY_COUNT, MODEM_QUEUE_BYTE_ALIGNMENT);

static struct module_data self = {
	.name = "modem",
	.msg_q = &msgq_modem,
	.supports_shutdown = true,
};

/* Forward declarations. */
static void send_cell_update(uint32_t cell_id, uint32_t tac);
static void send_neighbor_cell_update(struct lte_lc_cells_info *cell_info);
static void send_psm_update(int tau, int active_time);
static void send_edrx_update(float edrx, float ptw);

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type state)
{
	switch (state) {
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
static bool event_handler(const struct event_header *eh)
{
	struct modem_msg_data msg = {0};
	bool enqueue_msg = false;

	if (is_modem_module_event(eh)) {
		struct modem_module_event *evt = cast_modem_module_event(eh);

		msg.module.modem = *evt;
		enqueue_msg = true;
	}

	if (is_app_module_event(eh)) {
		struct app_module_event *evt = cast_app_module_event(eh);

		msg.module.app = *evt;
		enqueue_msg = true;
	}

	if (is_cloud_module_event(eh)) {
		struct cloud_module_event *evt = cast_cloud_module_event(eh);

		msg.module.cloud = *evt;
		enqueue_msg = true;
	}

	if (is_util_module_event(eh)) {
		struct util_module_event *evt = cast_util_module_event(eh);

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

static void lte_evt_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if (evt->nw_reg_status == LTE_LC_NW_REG_UICC_FAIL) {
			LOG_ERR("No SIM card detected!");
			SEND_ERROR(modem, MODEM_EVT_ERROR, -ENOTSUP);
			break;
		}

		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		    (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			SEND_EVENT(modem, MODEM_EVT_LTE_DISCONNECTED);
			break;
		}

		LOG_DBG("Network registration status: %s",
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			"Connected - home network" : "Connected - roaming");

		SEND_EVENT(modem, MODEM_EVT_LTE_CONNECTED);
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		LOG_DBG("PSM parameter update: TAU: %d, Active time: %d",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		send_psm_update(evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE: {
		char log_buf[60];
		ssize_t len;

		len = snprintf(log_buf, sizeof(log_buf),
			       "eDRX parameter update: eDRX: %f, PTW: %f",
			       evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		if (len > 0) {
			LOG_DBG("%s", log_strdup(log_buf));
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
	case LTE_LC_EVT_NEIGHBOR_CELL_MEAS:
		LOG_DBG("Neighbor cell measurements received");
		send_neighbor_cell_update((struct lte_lc_cells_info *)&evt->cells_info);
		break;
	default:
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
	rsrp_value_latest = rsrp_value - 140;

	LOG_DBG("Incoming RSRP status message, RSRP value is %d",
		rsrp_value_latest);
}

/* Static module functions. */
static void send_cell_update(uint32_t cell_id, uint32_t tac)
{
	struct modem_module_event *evt = new_modem_module_event();

	evt->type = MODEM_EVT_LTE_CELL_UPDATE;
	evt->data.cell.cell_id = cell_id;
	evt->data.cell.tac = tac;

	EVENT_SUBMIT(evt);
}

static void send_psm_update(int tau, int active_time)
{
	struct modem_module_event *evt = new_modem_module_event();

	evt->type = MODEM_EVT_LTE_PSM_UPDATE;
	evt->data.psm.tau = tau;
	evt->data.psm.active_time = active_time;

	EVENT_SUBMIT(evt);
}

static void send_edrx_update(float edrx, float ptw)
{
	struct modem_module_event *evt = new_modem_module_event();

	evt->type = MODEM_EVT_LTE_EDRX_UPDATE;
	evt->data.edrx.edrx = edrx;
	evt->data.edrx.ptw = ptw;

	EVENT_SUBMIT(evt);
}

static inline int adjust_rsrp(int input)
{
	return input - 140;
}

static inline int adjust_rsrq(int input)
{
	return round(input * 0.5 - 19.5);
}

static void send_neighbor_cell_update(struct lte_lc_cells_info *cell_info)
{
	struct modem_module_event *evt = new_modem_module_event();

	BUILD_ASSERT(sizeof(evt->data.neighbor_cells.cell_data) ==
		     sizeof(struct lte_lc_cells_info));
	BUILD_ASSERT(sizeof(evt->data.neighbor_cells.neighbor_cells) >=
		     sizeof(struct lte_lc_ncell) * CONFIG_LTE_NEIGHBOR_CELLS_MAX);

	memcpy(&evt->data.neighbor_cells.cell_data, cell_info, sizeof(struct lte_lc_cells_info));
	memcpy(&evt->data.neighbor_cells.neighbor_cells, cell_info->neighbor_cells,
	       sizeof(struct lte_lc_ncell) * evt->data.neighbor_cells.cell_data.ncells_count);

	/* Convert RSRP to dBm and RSRQ to dB per "nRF91 AT Commands" v1.7. */
	evt->data.neighbor_cells.cell_data.current_cell.rsrp =
			adjust_rsrp(evt->data.neighbor_cells.cell_data.current_cell.rsrp);
	evt->data.neighbor_cells.cell_data.current_cell.rsrq =
			adjust_rsrq(evt->data.neighbor_cells.cell_data.current_cell.rsrq);

	for (size_t i = 0; i < evt->data.neighbor_cells.cell_data.ncells_count; i++) {
		evt->data.neighbor_cells.neighbor_cells[i].rsrp =
			adjust_rsrp(evt->data.neighbor_cells.neighbor_cells[i].rsrp);
		evt->data.neighbor_cells.neighbor_cells[i].rsrq =
			adjust_rsrq(evt->data.neighbor_cells.neighbor_cells[i].rsrq);
	}

	evt->type = MODEM_EVT_NEIGHBOR_CELLS_DATA_READY;
	evt->data.neighbor_cells.timestamp = k_uptime_get();

	EVENT_SUBMIT(evt);
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

	modem_module_event->data.modem_static.nw_mode_ltem = modem_param.network.lte_mode.value;
	modem_module_event->data.modem_static.nw_mode_nbiot = modem_param.network.nbiot_mode.value;
	modem_module_event->data.modem_static.nw_mode_gps = modem_param.network.gps_mode.value;
	modem_module_event->data.modem_static.band = modem_param.network.current_band.value;

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

	modem_module_event->data.modem_static.app_version
		[sizeof(modem_module_event->data.modem_static.app_version) - 1] = '\0';

	modem_module_event->data.modem_static.board_version
		[sizeof(modem_module_event->data.modem_static.board_version) - 1] = '\0';

	modem_module_event->data.modem_static.modem_fw
		[sizeof(modem_module_event->data.modem_static.modem_fw) - 1] = '\0';

	modem_module_event->data.modem_static.iccid
		[sizeof(modem_module_event->data.modem_static.iccid) - 1] = '\0';

	modem_module_event->data.modem_static.timestamp = k_uptime_get();
	modem_module_event->type = MODEM_EVT_MODEM_STATIC_DATA_READY;

	EVENT_SUBMIT(modem_module_event);
	return 0;
}

static void populate_event_with_dynamic_modem_data(struct modem_module_event *event,
						   struct modem_param_info *param)
{
	/* If this flag is set all sampled parameter values will be included in the event regardless
	 * if they have changed or not.
	 */
	bool include = IS_ENABLED(CONFIG_MODEM_SEND_ALL_SAMPLED_DATA);

	/* Flag that checks if parameters has been added to the event. */
	bool params_added = false;

	/* Set all entries in the dynamic modem data structure to 0 to be sure that all 'fresh'
	 * flags become false by default. This is to avoid sending garbage or old data due to a flag
	 * being accidently set to true.
	 */
	memset(&event->data.modem_dynamic, 0, sizeof(struct modem_module_dynamic_modem_data));

	/* Structure that holds previous sampled dynamic modem data. By default, set all members of
	 * the structure to invalid values.
	 */
	static struct modem_module_dynamic_modem_data prev = { .rsrp = UINT8_MAX };

	/* Compare the latest sampled parameters with the previous. If there has been a change we
	 * want to include the parameters in the event.
	 */
	if ((prev.rsrp != rsrp_value_latest) || include) {
		event->data.modem_dynamic.rsrp = rsrp_value_latest;
		prev.rsrp = rsrp_value_latest;

		event->data.modem_dynamic.rsrp_fresh = true;
		params_added = true;
	}

	if ((strcmp(prev.ip_address, param->network.ip_address.value_string) != 0) || include) {
		strncpy(event->data.modem_dynamic.ip_address,
			modem_param.network.ip_address.value_string,
			sizeof(event->data.modem_dynamic.ip_address) - 1);

		strncpy(prev.ip_address,
			param->network.ip_address.value_string,
			sizeof(prev.ip_address) - 1);

		event->data.modem_dynamic.ip_address
			[sizeof(event->data.modem_dynamic.ip_address) - 1] = '\0';

		prev.ip_address[sizeof(prev.ip_address) - 1] = '\0';

		event->data.modem_dynamic.ip_address_fresh = true;
		params_added = true;
	}

	if ((prev.cell_id != param->network.cellid_dec) || include) {
		event->data.modem_dynamic.cell_id = param->network.cellid_dec;
		prev.cell_id = param->network.cellid_dec;

		event->data.modem_dynamic.cell_id_fresh = true;
		params_added = true;
	}

	if ((strcmp(prev.mccmnc, param->network.current_operator.value_string) != 0) || include) {
		strncpy(event->data.modem_dynamic.mccmnc,
			modem_param.network.current_operator.value_string,
			sizeof(event->data.modem_dynamic.mccmnc));

		strncpy(prev.mccmnc, param->network.current_operator.value_string,
			sizeof(prev.mccmnc));

		event->data.modem_dynamic.mccmnc
			[sizeof(event->data.modem_dynamic.mccmnc) - 1] = '\0';

		prev.mccmnc[sizeof(prev.mccmnc) - 1] = '\0';

		event->data.modem_dynamic.mccmnc_fresh = true;
		params_added = true;
	}

	if ((prev.area_code != modem_param.network.area_code.value) || include) {
		event->data.modem_dynamic.area_code = param->network.area_code.value;
		prev.area_code = param->network.area_code.value;

		event->data.modem_dynamic.area_code_fresh = true;
		params_added = true;
	}

	if (params_added) {
		event->type = MODEM_EVT_MODEM_DYNAMIC_DATA_READY;
		event->data.modem_dynamic.timestamp = k_uptime_get();
	} else {
		LOG_DBG("No dynamic modem parameters have changed from the last sample request.");
		event->type = MODEM_EVT_MODEM_DYNAMIC_DATA_NOT_READY;
	}
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

	populate_event_with_dynamic_modem_data(modem_module_event, &modem_param);

	EVENT_SUBMIT(modem_module_event);
	return 0;
}

static bool static_modem_data_requested(enum app_module_data_type *data_list,
					size_t count)
{
	for (size_t i = 0; i < count; i++) {
		if (data_list[i] == APP_DATA_MODEM_STATIC) {
			return true;
		}
	}

	return false;
}

static bool dynamic_modem_data_requested(enum app_module_data_type *data_list,
					 size_t count)
{
	for (size_t i = 0; i < count; i++) {
		if (data_list[i] == APP_DATA_MODEM_DYNAMIC) {
			return true;
		}
	}

	return false;
}

static bool battery_data_requested(enum app_module_data_type *data_list,
				   size_t count)
{
	for (size_t i = 0; i < count; i++) {
		if (data_list[i] == APP_DATA_BATTERY) {
			return true;
		}
	}

	return false;
}

static bool neighbor_cells_data_requested(enum app_module_data_type *data_list,
				     size_t count)
{
	for (size_t i = 0; i < count; i++) {
		if (data_list[i] == APP_DATA_NEIGHBOR_CELLS) {
			return true;
		}
	}

	return false;
}

static int battery_data_get(void)
{
	int err;

	/* Replace this function with a function that specifically
	 * requests battery.
	 */
	err = modem_info_params_get(&modem_param);
	if (err) {
		LOG_ERR("modem_info_params_get, error: %d", err);
		return err;
	}

	struct modem_module_event *modem_module_event =
			new_modem_module_event();

	modem_module_event->data.bat.battery_voltage =
			modem_param.device.battery.value;
	modem_module_event->data.bat.timestamp = k_uptime_get();
	modem_module_event->type = MODEM_EVT_BATTERY_DATA_READY;

	EVENT_SUBMIT(modem_module_event);

	return 0;
}

static int neighbor_cells_measurement_start(void)
{
	int err;

	err = lte_lc_neighbor_cell_measurement();
	if (err) {
		LOG_ERR("Failed to start neighbor cell measurements, error: %d", err);
		return err;
	}

	return 0;
}

static int configure_low_power(void)
{
	int err;

	err = lte_lc_psm_req(true);
	if (err) {
		LOG_ERR("lte_lc_psm_req, error: %d", err);
		return err;
	}

	LOG_DBG("PSM requested");

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

	if (IS_ENABLED(CONFIG_MODEM_AUTO_REQUEST_POWER_SAVING_FEATURES)) {
		err = configure_low_power();
		if (err) {
			LOG_ERR("configure_low_power, error: %d", err);
			return err;
		}
	}

	err = modem_data_init();
	if (err) {
		LOG_ERR("modem_data_init, error: %d", err);
		return err;
	}

	return 0;
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
}

/* Message handler for STATE_CONNECTING. */
static void on_state_connecting(struct modem_msg_data *msg)
{
	if (IS_EVENT(msg, app, APP_EVT_LTE_DISCONNECT)) {
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
}

/* Message handler for all states. */
static void on_all_states(struct modem_msg_data *msg)
{
	if (IS_EVENT(msg, app, APP_EVT_START)) {
		int err;

		err = lte_connect();
		if (err) {
			LOG_ERR("Failed connecting to LTE, error: %d", err);
			SEND_ERROR(modem, MODEM_EVT_ERROR, err);
			return;
		}
	}

	if (IS_EVENT(msg, app, APP_EVT_DATA_GET)) {
		if (static_modem_data_requested(msg->module.app.data_list,
						msg->module.app.count)) {

			int err;

			err = static_modem_data_get();
			if (err) {
				SEND_EVENT(modem,
					MODEM_EVT_MODEM_STATIC_DATA_NOT_READY);
			}
		}

		if (dynamic_modem_data_requested(msg->module.app.data_list,
						 msg->module.app.count)) {

			int err;

			err = dynamic_modem_data_get();
			if (err) {
				SEND_EVENT(modem,
					MODEM_EVT_MODEM_DYNAMIC_DATA_NOT_READY);
			}
		}

		if (battery_data_requested(msg->module.app.data_list,
					   msg->module.app.count)) {

			int err;

			err = battery_data_get();
			if (err) {
				SEND_EVENT(modem,
					MODEM_EVT_BATTERY_DATA_NOT_READY);
			}
		}

		if (neighbor_cells_data_requested(msg->module.app.data_list,
						  msg->module.app.count)) {
			int err;

			err = neighbor_cells_measurement_start();
			if (err) {
				SEND_EVENT(modem, MODEM_EVT_NEIGHBOR_CELLS_DATA_NOT_READY);
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
	struct modem_msg_data msg;

	self.thread_id = k_current_get();

	err = module_start(&self);
	if (err) {
		LOG_ERR("Failed starting module, error: %d", err);
		SEND_ERROR(modem, MODEM_EVT_ERROR, err);
	}

	state_set(STATE_DISCONNECTED);

	err = setup();
	if (err) {
		LOG_ERR("Failed setting up the modem, error: %d", err);
		SEND_ERROR(modem, MODEM_EVT_ERROR, err);
	}

	while (true) {
		module_get_next_msg(&self, &msg);

		switch (state) {
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
			LOG_WRN("Invalid state: %d", state);
			break;
		}

		on_all_states(&msg);
	}
}

K_THREAD_DEFINE(modem_module_thread, CONFIG_MODEM_THREAD_STACK_SIZE,
		module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE_EARLY(MODULE, modem_module_event);
EVENT_SUBSCRIBE(MODULE, app_module_event);
EVENT_SUBSCRIBE(MODULE, cloud_module_event);
EVENT_SUBSCRIBE_FINAL(MODULE, util_module_event);
