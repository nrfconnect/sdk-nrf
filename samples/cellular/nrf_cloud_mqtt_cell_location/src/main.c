/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <nrf_modem_at.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_codec.h>
#include <net/nrf_cloud_defs.h>
#include <app_version.h>
#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(nrf_cloud_mqtt_cell_location,
	CONFIG_NRF_CLOUD_MQTT_CELL_LOCATION_SAMPLE_LOG_LEVEL);

/* Network connection states */
#define NETWORK_UP         BIT(0)
#define CLOUD_READY        BIT(1)
#define CLOUD_DISCONNECTED BIT(3)

/* Macros used to subscribe to specific Zephyr NET management events. */
#define L4_EVENT_MASK	      (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)

/* Modem FW version required to properly run this sample */
#define MFWV_MAJ_SAMPLE_REQ 1
#define MFWV_MIN_SAMPLE_REQ 3
#define MFWV_REV_SAMPLE_REQ 4

/* Synchronization primitives */
static K_SEM_DEFINE(cell_info_ready_sem, 0, 1);
static K_MUTEX_DEFINE(cell_info_mutex);
static K_EVENT_DEFINE(connection_events);

/* Device information */
static char device_id[NRF_CLOUD_CLIENT_ID_MAX_LEN + 1];

/* Cell positioning configuration */
enum nrf_cloud_location_type active_cell_pos_type = LOCATION_TYPE_SINGLE_CELL;
static enum lte_lc_neighbor_search_type search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT;

/* Cell measurement data buffers */
static struct lte_lc_ncell neighbor_cells[CONFIG_LTE_NEIGHBOR_CELLS_MAX];
static struct lte_lc_cell gci_cells[CONFIG_MQTT_CELL_GCI_COUNT];
static struct lte_lc_cells_info cell_info;
static struct modem_param_info mdm_param;

/* Location request configuration */
static struct nrf_cloud_location_config config = {
	.hi_conf = IS_ENABLED(CONFIG_MQTT_CELL_DEFAULT_HICONF_VAL),
	.fallback = IS_ENABLED(CONFIG_MQTT_CELL_DEFAULT_FALLBACK_VAL),
	.do_reply = IS_ENABLED(CONFIG_MQTT_CELL_DEFAULT_DOREPLY_VAL)
};

/* State tracking */
static enum lte_lc_rrc_mode cur_rrc_mode = LTE_LC_RRC_MODE_IDLE;
static bool rrc_idle_wait;

static bool ver_check(int32_t reqd_maj, int32_t reqd_min, int32_t reqd_rev,
		      int32_t maj, int32_t min, int32_t rev)
{
	if (maj > reqd_maj) {
		return true;
	} else if ((maj == reqd_maj) && (min > reqd_min)) {
		return true;
	} else if ((maj == reqd_maj) && (min == reqd_min) && (rev >= reqd_rev)) {
		return true;
	}
	return false;
}

static void check_modem_fw_version(void)
{
	char mfwv_str[128];
	uint32_t major;
	uint32_t minor;
	uint32_t rev;

	if (modem_info_string_get(MODEM_INFO_FW_VERSION, mfwv_str, sizeof(mfwv_str)) <= 0) {
		LOG_WRN("Failed to get modem FW version");
		return;
	}

	if ((sscanf(mfwv_str, "mfw_nrf9160_%u.%u.%u", &major, &minor, &rev) != 3) &&
	    (sscanf(mfwv_str, "mfw_nrf91x1_%u.%u.%u", &major, &minor, &rev) != 3)) {
		LOG_WRN("Unable to parse modem FW version number");
		return;
	}

	/* Ensure the modem firmware version meets the requirement for this sample */
	if (!ver_check(MFWV_MAJ_SAMPLE_REQ, MFWV_MIN_SAMPLE_REQ, MFWV_REV_SAMPLE_REQ,
		       major, minor, rev)) {
		LOG_ERR("This sample requires modem FW version %d.%d.%d or later",
			MFWV_MAJ_SAMPLE_REQ, MFWV_MIN_SAMPLE_REQ, MFWV_REV_SAMPLE_REQ);
		LOG_INF("Update modem firmware and restart");
		k_sleep(K_FOREVER);
	}

	/* Enable GCI search if modem fw version allows */
	search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_COMPLETE;
}

static void get_cell_info(void)
{
	int err;

	if (active_cell_pos_type == LOCATION_TYPE_SINGLE_CELL) {
		LOG_INF("Getting current cell info...");

		/* Use the modem info library to easily obtain the required network info
		 * for a single-cell request without performing a neighbor cell measurement
		 */
		err = modem_info_params_get(&mdm_param);
		if (err) {
			LOG_ERR("Unable to obtain modem info, error: %d", err);
			return;
		}

		(void)k_mutex_lock(&cell_info_mutex, K_FOREVER);
		memset(&cell_info, 0, sizeof(cell_info));
		/* Required parameters */
		cell_info.current_cell.id	= (uint32_t)mdm_param.network.cellid_dec;
		cell_info.current_cell.mcc	= mdm_param.network.mcc.value;
		cell_info.current_cell.mnc	= mdm_param.network.mnc.value;
		cell_info.current_cell.tac	= mdm_param.network.area_code.value;
		/* Optional */
		cell_info.current_cell.rsrp	= mdm_param.network.rsrp.value;
		/* Omitted - optional parameters not available from modem_info */
		cell_info.current_cell.timing_advance	= NRF_CLOUD_LOCATION_CELL_OMIT_TIME_ADV;
		cell_info.current_cell.rsrq		= NRF_CLOUD_LOCATION_CELL_OMIT_RSRQ;
		cell_info.current_cell.earfcn		= NRF_CLOUD_LOCATION_CELL_OMIT_EARFCN;
		(void)k_mutex_unlock(&cell_info_mutex);

		k_sem_give(&cell_info_ready_sem);

	} else {
		struct lte_lc_ncellmeas_params ncellmeas_params = {
			.search_type = search_type,
			.gci_count = CONFIG_MQTT_CELL_GCI_COUNT
		};

		/* Set the result buffers */
		cell_info.neighbor_cells = neighbor_cells;
		cell_info.gci_cells = gci_cells;

		LOG_INF("Requesting neighbor cell measurement");
		err = lte_lc_neighbor_cell_measurement(&ncellmeas_params);
		if (err && (err != -EINPROGRESS)) {
			LOG_ERR("Failed to start neighbor cell measurement, error: %d", err);
		} else {
			LOG_INF("Waiting for measurement results...");
		}
	}
}

static void process_cell_pos_type_change(void)
{
	static int config_mode = (IS_ENABLED(CONFIG_MQTT_CELL_DEFAULT_HICONF_VAL) ?  0x01 : 0x00) |
				 (IS_ENABLED(CONFIG_MQTT_CELL_DEFAULT_FALLBACK_VAL) ? 0x02 : 0x00);

	/* Toggle active cell pos type */
	if (active_cell_pos_type == LOCATION_TYPE_SINGLE_CELL) {
		if (cur_rrc_mode == LTE_LC_RRC_MODE_IDLE) {
			/* RRC is idle, switch to multi-cell type */
			active_cell_pos_type = LOCATION_TYPE_MULTI_CELL;
		} else if (rrc_idle_wait == false) {
			LOG_INF("Waiting for RRC idle mode "
				"before requesting neighbor cell measurement...");
			/* Wait for RRC idle. If this function is called before
			 * idle, the wait is cancelled and single-cell is used.
			 */
			rrc_idle_wait = true;
			return;
		}
	} else {
		active_cell_pos_type = LOCATION_TYPE_SINGLE_CELL;
		if (IS_ENABLED(CONFIG_MQTT_CELL_CHANGE_CONFIG)) {
			/* After doing both multi and single cell requests,
			 * modify the configuration. Eventually all 4 combinations
			 * of config.hi_conf and config.fallback will be used.
			 */
			config_mode++;
			config.hi_conf = ((config_mode & 0x01) != 0);
			config.fallback = ((config_mode & 0x02) != 0);
			config.do_reply = true;
		}
	}

	rrc_idle_wait = false;
	get_cell_info();
}

static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & DK_BTN1_MSK) {
		process_cell_pos_type_change();
	}
}

static void send_device_status(void)
{
	int err;

	struct nrf_cloud_modem_info mdm_inf = {
		/* Include the application version */
		.application_version = APP_VERSION_STRING
	};

	struct nrf_cloud_device_status dev_status = {
		.modem = &mdm_inf,
		.conn_inf = NRF_CLOUD_INFO_SET
	};

	err = nrf_cloud_shadow_device_status_update(&dev_status);
	if (err) {
		LOG_ERR("Failed to send device status, error: %d", err);
	} else {
		LOG_INF("Device status sent to nRF Cloud");
	}
}

static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		LOG_DBG("LTE_LC_EVT_NW_REG_STATUS: %d", evt->nw_reg_status);
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		    (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			break;
		}

		LOG_DBG("Network registration status: %s",
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			"Connected - home network" : "Connected - roaming");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		if (evt->cell.id == LTE_LC_CELL_EUTRAN_ID_INVALID) {
			break;
		}

		/* Get new info when cell ID changes */
		LOG_INF("Cell info changed");
		get_cell_info();

		break;
	case LTE_LC_EVT_RRC_UPDATE:
		cur_rrc_mode = evt->rrc_mode;
		if (cur_rrc_mode == LTE_LC_RRC_MODE_IDLE) {
			LOG_INF("RRC mode: idle");
		} else {
			LOG_INF("RRC mode: connected");
		}
		if (rrc_idle_wait && (cur_rrc_mode == LTE_LC_RRC_MODE_IDLE)) {
			process_cell_pos_type_change();
		}
		break;
	case LTE_LC_EVT_NEIGHBOR_CELL_MEAS:
		if ((search_type < LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_DEFAULT) &&
		    (evt->cells_info.current_cell.id == LTE_LC_CELL_EUTRAN_ID_INVALID)) {
			LOG_WRN("Current cell ID not valid in neighbor cell measurement results");
			break;
		}

		(void)k_mutex_lock(&cell_info_mutex, K_FOREVER);
		/* Copy current cell information. */
		memcpy(&cell_info.current_cell,
		       &evt->cells_info.current_cell,
		       sizeof(cell_info.current_cell));

		/* Copy neighbor cell information if present. */
		cell_info.ncells_count = evt->cells_info.ncells_count;
		if ((evt->cells_info.ncells_count > 0) && (evt->cells_info.neighbor_cells)) {
			memcpy(neighbor_cells,
			       evt->cells_info.neighbor_cells,
			       sizeof(neighbor_cells[0]) * cell_info.ncells_count);
			LOG_INF("Received measurements for %u neighbor cells",
				cell_info.ncells_count);
		} else {
			LOG_WRN("No neighbor cells were measured");
		}

		/* Copy GCI cell information if present. */
		cell_info.gci_cells_count = evt->cells_info.gci_cells_count;
		if ((evt->cells_info.gci_cells_count > 0) && (evt->cells_info.gci_cells)) {
			memcpy(gci_cells,
			       evt->cells_info.gci_cells,
			       sizeof(gci_cells[0]) * cell_info.gci_cells_count);
			LOG_INF("Received measurements for %u GCI cells",
				cell_info.gci_cells_count);
		} else if (search_type == LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_COMPLETE) {
			LOG_WRN("No GCI cells were measured");
		}

		(void)k_mutex_unlock(&cell_info_mutex);
		k_sem_give(&cell_info_ready_sem);

		break;
	default:
		break;
	}
}

static bool cred_check(struct nrf_cloud_credentials_status *const cs)
{
	int ret = nrf_cloud_credentials_check(cs);

	if (ret) {
		LOG_ERR("nRF Cloud credentials check failed, error: %d", ret);
		return false;
	}

	if (!cs->ca || !cs->ca_aws || !cs->prv_key) {
		LOG_WRN("Missing required nRF Cloud credential(s) in sec tag %u:", cs->sec_tag);
		if (!cs->ca || !cs->ca_aws) {
			LOG_WRN("\t-CA Cert");
		}
		if (!cs->prv_key) {
			LOG_WRN("\t-Private Key");
		}
	}

	return (cs->ca && cs->ca_aws && cs->prv_key);
}

static void await_credentials(void)
{
	struct nrf_cloud_credentials_status cs;

	while (!cred_check(&cs)) {
		LOG_INF("Waiting for credentials to be installed...");
		LOG_INF("Press the reset button once the credentials are installed");
		k_sleep(K_FOREVER);
	}

	LOG_INF("nRF Cloud credentials detected!");
}

/* Disconnect from nRF Cloud and update internal state.
 * Note: May be called when already disconnected, resulting in some redundant log messages.
 */
static void disconnect_cloud(void)
{
	int err;

	k_event_clear(&connection_events, CLOUD_READY);
	LOG_DBG("Cleared CLOUD_READY");

	LOG_INF("Disconnecting from nRF Cloud");
	err = nrf_cloud_disconnect();

	if ((err == -EACCES) || (err == -ENOTCONN)) {
		LOG_DBG("Already disconnected from nRF Cloud");
	} else if (err) {
		LOG_ERR("Cannot disconnect from nRF Cloud, error: %d. Continuing anyways", err);
	} else {
		LOG_INF("Successfully disconnected from nRF Cloud");
	}

	k_event_post(&connection_events, CLOUD_DISCONNECTED);
}

/* Handler for events from nRF Cloud Lib. */
static void cloud_event_handler(const struct nrf_cloud_evt *nrf_cloud_evt)
{
	int err = 0;

	switch (nrf_cloud_evt->type) {
	case NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR: %d", nrf_cloud_evt->status);
		/* Disconnect from cloud immediately rather than wait for retry timeout. */
		err = nrf_cloud_disconnect();
		if ((err == -EACCES) || (err == -ENOTCONN)) {
			LOG_DBG("Already disconnected from nRF Cloud");
		} else if (err) {
			LOG_ERR("Cannot disconnect from nRF Cloud, error: %d. Continuing anyways",
				err);
		} else {
			LOG_INF("Successfully disconnected from nRF Cloud");
		}
		break;
	case NRF_CLOUD_EVT_READY:
		LOG_INF("Connection to nRF Cloud ready");
		k_event_post(&connection_events, CLOUD_READY);
		break;
	case NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED");
		/* The nRF Cloud library itself has disconnected for some reason.
		 * Disconnect from cloud immediately rather than wait for retry timeout.
		 */
		disconnect_cloud();
		break;
	case NRF_CLOUD_EVT_RX_DATA_DISCON:
		LOG_INF("Device was removed from your account.");
		break;
	case NRF_CLOUD_EVT_ERROR:
		LOG_DBG("NRF_CLOUD_EVT_ERROR: %d", nrf_cloud_evt->status);
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTING:
	case NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST:
	case NRF_CLOUD_EVT_USER_ASSOCIATED:
	case NRF_CLOUD_EVT_SENSOR_DATA_ACK:
	case NRF_CLOUD_EVT_RX_DATA_SHADOW:
	case NRF_CLOUD_EVT_FOTA_START:
	case NRF_CLOUD_EVT_FOTA_DONE:
	case NRF_CLOUD_EVT_FOTA_ERROR:
		LOG_DBG("Cloud event: %d", nrf_cloud_evt->type);
		break;
	default:
		LOG_DBG("Unknown event type: %d", nrf_cloud_evt->type);
		break;
	}
}


int init(void)
{
	int err = 0;

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("dk_buttons_init, error: %d", err);
	}

	err = nrf_modem_lib_init();
	if (err) {
		LOG_ERR("Modem library initialization failed, error: %d", err);
		return err;
	}

	/* Print the nRF Cloud device ID. This device ID should match the ID used
	 * to provision the device on nRF Cloud or to register a public JWT signing key.
	 */
	err = nrf_cloud_client_id_get(device_id, sizeof(device_id));
	if (err) {
		LOG_ERR("Failed to get device ID, error: %d", err);
		return err;
	}
	LOG_INF("Device ID: %s", device_id);

	/* Check modem FW version */
	check_modem_fw_version();

	/* Before connecting, ensure nRF Cloud credentials are installed */
	await_credentials();

	/* Initiate Connection */
	LOG_INF("Enabling connectivity...");
	lte_lc_register_handler(lte_handler);
	conn_mgr_all_if_connect(true);
	k_event_wait(&connection_events, NETWORK_UP, false, K_FOREVER);

	/* Initialize nrf_cloud library. */
	struct nrf_cloud_init_param params = {
		.event_handler = cloud_event_handler,
		.application_version = APP_VERSION_STRING
	};

	/* Initialize modem info parameters */
	(void) modem_info_params_init(&mdm_param);

	err = nrf_cloud_init(&params);
	if (err) {
		LOG_ERR("nRF Cloud library could not be initialized, error: %d", err);
		return err;
	}

	return err;
}

/* Connect to nRF Cloud and wait for ready state. */
static int connect_cloud(void)
{
	int err;

	LOG_INF("Connecting to nRF Cloud");
	k_event_clear(&connection_events, CLOUD_DISCONNECTED);

	err = nrf_cloud_connect();
	if (err == NRF_CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED) {
		LOG_WRN("Already connected to nRF Cloud");
		return 0;
	}
	if (err != 0) {
		LOG_ERR("Could not connect to nRF Cloud, error: %d", err);
		return err;
	}

	k_event_wait(&connection_events, CLOUD_READY, false, K_FOREVER);
	return 0;
}


/* Callback to track network connectivity */
static struct net_mgmt_event_callback l4_callback;
static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint64_t event,
			     struct net_if *iface)
{
	switch (event) {
	case NET_EVENT_L4_CONNECTED:
		LOG_INF("Connected to network");
		k_event_post(&connection_events, NETWORK_UP);
		break;
	case NET_EVENT_L4_DISCONNECTED:
		LOG_INF("Network connectivity lost");
		break;
	default:
		LOG_DBG("Unknown event: 0x%08llX", event);
		return;
	}
}

/* Callback to track connectivity layer events */
static struct net_mgmt_event_callback conn_cb;
static void connectivity_event_handler(struct net_mgmt_event_callback *cb,
				       uint64_t event,
				       struct net_if *iface)
{
	if (event == NET_EVENT_CONN_IF_FATAL_ERROR) {
		LOG_ERR("NET_EVENT_CONN_IF_FATAL_ERROR");
		return;
	}
}

/* Start tracking network availability */
static int prepare_network_tracking(void)
{
	net_mgmt_init_event_callback(&l4_callback, l4_event_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_callback);

	net_mgmt_init_event_callback(&conn_cb, connectivity_event_handler, CONN_LAYER_EVENT_MASK);
	net_mgmt_add_event_callback(&conn_cb);

	return 0;
}

SYS_INIT(prepare_network_tracking, APPLICATION, 0);

static void cloud_service_location_ready_cb(const struct nrf_cloud_location_result *const result)
{
	if (!result) {
		return;
	}

	if (result->err == NRF_CLOUD_ERROR_NONE) {
		const char *type_str = (result->type == LOCATION_TYPE_SINGLE_CELL) ? "single-cell" :
				       (result->type == LOCATION_TYPE_MULTI_CELL) ? "multi-cell" :
										     "unknown";
		LOG_INF("Cellular location request fulfilled with %s", type_str);
		LOG_INF("Lat: %f, Lon: %f, Uncertainty: %u m", result->lat, result->lon,
			result->unc);
	} else {
		LOG_ERR("Unable to determine location from cloud service, error: %d", result->err);
	}
}

int main(void)
{
	int err;

	LOG_INF("nRF Cloud MQTT Cellular Location Sample, version: %s", APP_VERSION_STRING);

	err = init();
	if (err) {
		LOG_ERR("Initialization failed");
		return 0;
	}

	/* Connect to nRF Cloud */
	err = connect_cloud();
	if (err) {
		LOG_ERR("Cloud connection failed");
		return 0;
	}

	/* Send device status with HW/FW version and network details */
	if (IS_ENABLED(CONFIG_MQTT_CELL_SEND_DEVICE_STATUS)) {
		send_device_status();
	}

	/* Get initial cell info */
	get_cell_info();

	/* Main loop: process cell location requests */
	while (1) {
		/* Wait for cell info (triggered by button press or cell ID change) */
		(void)k_sem_take(&cell_info_ready_sem, K_FOREVER);

		(void)k_mutex_lock(&cell_info_mutex, K_FOREVER);

		/* Log current cell information */
		if (cell_info.current_cell.id != LTE_LC_CELL_EUTRAN_ID_INVALID) {
			LOG_INF("Current cell info: Cell ID: %u, TAC: %u, MCC: %d, MNC: %d",
				cell_info.current_cell.id, cell_info.current_cell.tac,
				cell_info.current_cell.mcc, cell_info.current_cell.mnc);
		} else {
			LOG_WRN("No current serving cell available");
		}

		/* Log request type */
		if (cell_info.ncells_count || cell_info.gci_cells_count) {
			LOG_INF("Performing multi-cell request with "
				"%u neighbor cells and %u GCI cells",
				cell_info.ncells_count, cell_info.gci_cells_count);
		} else {
			LOG_INF("Performing single-cell request");
		}

		/* Log request configuration */
		LOG_INF("Request configuration:");
		LOG_INF("  High confidence interval   = %s", config.hi_conf ? "true" : "false");
		LOG_INF("  Fallback to rough location = %s", config.fallback ? "true" : "false");
		LOG_INF("  Reply with result          = %s", config.do_reply ? "true" : "false");

		/* Send location request to nRF Cloud */
		err = nrf_cloud_location_request(&cell_info, NULL, &config,
						 cloud_service_location_ready_cb);
		if (err == -EACCES) {
			LOG_ERR("Cloud connection is not established");
		} else if (err) {
			LOG_ERR("Failed to request positioning data, error: %d", err);
		}

		(void)k_mutex_unlock(&cell_info_mutex);
	}
}
