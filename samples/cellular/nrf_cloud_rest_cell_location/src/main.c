/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <nrf_modem_at.h>
#include <modem/modem_info.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <app_event_manager.h>
#include <caf/events/button_event.h>
#define MODULE main
#include <caf/events/module_state_event.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_rest.h>

LOG_MODULE_REGISTER(nrf_cloud_rest_cell_location_sample,
		    CONFIG_NRF_CLOUD_REST_CELL_LOCATION_SAMPLE_LOG_LEVEL);

#define BTN_NUM			1 /* See include/buttons_def.h */
#define JITP_REQ_WAIT_SEC	10
#define UI_REQ_WAIT_SEC		10
#define NET_CONN_WAIT_MIN	15

/* Provide a buffer large enough to hold the full HTTPS REST response */
#define REST_RX_BUF_SZ		1024

/* Modem FW version required to properly run this sample */
#define MFWV_MAJ_SAMPLE_REQ	1
#define MFWV_MIN_SAMPLE_REQ	3
#define MFWV_REV_SAMPLE_REQ	0
/* Modem FW version required for extended neighbor cells search */
#define MFWV_MAJ_EXT_SRCH	1
#define MFWV_MIN_EXT_SRCH	3
#define MFWV_REV_EXT_SRCH	1
/* Modem FW version required for extended GCI neighbor cells search */
#define MFWV_MAJ_EXT_SRCH_GCI	1
#define MFWV_MIN_EXT_SRCH_GCI	3
#define MFWV_REV_EXT_SRCH_GCI	4

/* Semaphore to indicate a button has been pressed */
static K_SEM_DEFINE(button_press_sem, 0, 1);

/* Semaphore to indicate that a network connection has been established */
static K_SEM_DEFINE(lte_connected_sem, 0, 1);

/* Semaphore to indicate that cell info has been received */
static K_SEM_DEFINE(cell_info_ready_sem, 0, 1);

/* Mutex for cell info struct */
static K_MUTEX_DEFINE(cell_info_mutex);

/* nRF Cloud device ID */
static char device_id[NRF_CLOUD_CLIENT_ID_MAX_LEN + 1];

/* Buffer used for REST calls */
static char rx_buf[REST_RX_BUF_SZ];

/* nRF Cloud REST context */
static struct nrf_cloud_rest_context rest_ctx = {
	.connect_socket = -1,
	.rx_buf = rx_buf,
	.rx_buf_len = sizeof(rx_buf),
	.fragment_size = 0,
	/* A JWT will be automatically generated using the configured nRF Cloud device ID.
	 * See CONFIG_NRF_CLOUD_REST_AUTOGEN_JWT and CONFIG_NRF_CLOUD_CLIENT_ID_SRC.
	 */
	.auth = NULL
};

/* Type of data to be sent in the cellular positioning request */
enum nrf_cloud_location_type active_cell_pos_type = LOCATION_TYPE_SINGLE_CELL;

/* Search type used for neighbor cell measurements; modem FW version depenedent */
static enum lte_lc_neighbor_search_type search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT;

/* Buffer to hold neighbor cell measurement data for multi-cell requests */
static struct lte_lc_ncell neighbor_cells[CONFIG_LTE_NEIGHBOR_CELLS_MAX];

/* Buffer to hold GCI cell measurement data for multi-cell requests */
static struct lte_lc_cell gci_cells[CONFIG_REST_CELL_GCI_COUNT];

/* Modem info struct used for modem FW version and cell info used for single-cell requests */
static struct modem_param_info mdm_param;

/* Structure to hold all cell info */
static struct lte_lc_cells_info cell_info;

/* Structure to hold configuration flags. */
static struct nrf_cloud_location_config config = {
	.hi_conf = IS_ENABLED(CONFIG_REST_CELL_DEFAULT_HICONF_VAL),
	.fallback = IS_ENABLED(CONFIG_REST_CELL_DEFAULT_FALLBACK_VAL),
	.do_reply = IS_ENABLED(CONFIG_REST_CELL_DEFAULT_DOREPLY_VAL)
};

/* REST request structure to contain cell info to be sent to nRF Cloud */
static struct nrf_cloud_rest_location_request cell_pos_req = {
	.cell_info = &cell_info,
	.wifi_info = NULL,
	.config = &config
};

/* Flag to indicate that the device is ready to perform requests */
static bool ready;

/* Current RRC mode */
static enum lte_lc_rrc_mode cur_rrc_mode = LTE_LC_RRC_MODE_IDLE;

/* Flag to indicate that a neighbor cell measurement should be taken once RRC mode is idle */
static bool rrc_idle_wait;

#if defined(CONFIG_REST_CELL_LOCATION_DO_JITP)
/* Flag to indicate if the user requested JITP to be performed */
static bool jitp_requested;
#endif

/* Register a listener for application events, specifically a button event */
static bool app_event_handler(const struct app_event_header *aeh);
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, button_event);

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

	LOG_INF("Modem FW version: %s", mfwv_str);

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

	/* Enable GCI/extended search if modem fw version allows */
	if (ver_check(MFWV_MAJ_EXT_SRCH_GCI, MFWV_MIN_EXT_SRCH_GCI, MFWV_REV_EXT_SRCH_GCI,
		      major, minor, rev)) {
		search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_COMPLETE;
		LOG_INF("Using LTE LC neighbor search type GCI extended complete for %d cells",
			CONFIG_REST_CELL_GCI_COUNT);
	} else if (ver_check(MFWV_MAJ_EXT_SRCH, MFWV_MIN_EXT_SRCH, MFWV_REV_EXT_SRCH,
			     major, minor, rev)) {
		search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_COMPLETE;
		LOG_INF("Using LTE LC neighbor search type extended complete");
	} else {
		LOG_INF("Using LTE LC neighbor search type default");
	}
}

static void get_cell_info(void)
{
	if (!ready) {
		return;
	}

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
			.gci_count = CONFIG_REST_CELL_GCI_COUNT
		};

		/* Set the result buffers */
		cell_info.neighbor_cells = neighbor_cells;
		cell_info.gci_cells = gci_cells;

		LOG_INF("Requesting neighbor cell measurement");
		err = lte_lc_neighbor_cell_measurement(&ncellmeas_params);
		if (err) {
			LOG_ERR("Failed to start neighbor cell measurement, error: %d", err);
		} else {
			LOG_INF("Waiting for measurement results...");
		}
	}
}

static void process_cell_pos_type_change(void)
{
	static int config_mode = (IS_ENABLED(CONFIG_REST_CELL_DEFAULT_HICONF_VAL) ?  0x01 : 0x00) |
				 (IS_ENABLED(CONFIG_REST_CELL_DEFAULT_FALLBACK_VAL) ? 0x02 : 0x00);

	if (!ready) {
		return;
	}

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
		if (IS_ENABLED(CONFIG_REST_CELL_CHANGE_CONFIG)) {
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

static bool app_event_handler(const struct app_event_header *aeh)
{
	struct button_event *btn_evt = is_button_event(aeh) ? cast_button_event(aeh) : NULL;

	if (!btn_evt) {
		return false;
	} else if (!btn_evt->pressed) {
		return true;
	}

	LOG_DBG("Button pressed");

	process_cell_pos_type_change();

	k_sem_give(&button_press_sem);

	return true;
}

static void modem_time_wait(void)
{
	int err;
	char time_buf[64];

	LOG_INF("Waiting for modem to acquire network time...");

	do {
		k_sleep(K_SECONDS(3));
		err = nrf_modem_at_cmd(time_buf, sizeof(time_buf), "AT%%CCLK?");
	} while (err != 0);

	LOG_INF("Network time obtained");
}

#if defined(CONFIG_REST_CELL_LOCATION_DO_JITP)
static void request_jitp(void)
{
	int err;

	k_sem_reset(&button_press_sem);

	LOG_INF("---> Press button %d to request just-in-time provisioning (JITP)",
		BTN_NUM);
	LOG_INF("     This only needs to be done once per device");
	LOG_INF("     Waiting %d seconds...", JITP_REQ_WAIT_SEC);

	err = k_sem_take(&button_press_sem, K_SECONDS(JITP_REQ_WAIT_SEC));
	if (err == 0) {
		jitp_requested = true;
		LOG_INF("JITP will be performed after network connection is obtained");
	} else {
		if (err != -EAGAIN) {
			LOG_ERR("k_sem_take error: %d", err);
		}

		LOG_INF("JITP will be skipped");
	}
}

static void do_jitp(void)
{
	int err = 0;

	LOG_INF("Performing JITP...");
	err = nrf_cloud_rest_jitp(CONFIG_NRF_CLOUD_SEC_TAG);

	if (err == 0) {
		LOG_INF("Waiting 30s for cloud provisioning to complete...");
		k_sleep(K_SECONDS(30));
		k_sem_reset(&button_press_sem);
		LOG_INF("Associate device with nRF Cloud account and press button %d when complete",
			BTN_NUM);
		(void)k_sem_take(&button_press_sem, K_FOREVER);
	} else if (err == 1) {
		LOG_INF("Device already provisioned");
	} else {
		LOG_ERR("Device provisioning failed");
	}
}
#endif

static void send_device_status(void)
{
	int err;

	struct nrf_cloud_modem_info mdm_inf = {
		/* Include all available modem info. Change to NRF_CLOUD_INFO_NO_CHANGE to
		 * reduce the volume of data transmitted.
		 */
		.device = NRF_CLOUD_INFO_SET,
		.network = NRF_CLOUD_INFO_SET,
		.sim = NRF_CLOUD_INFO_SET,
		/* Use the modem info already obtained */
		.mpi = &mdm_param,
		/* Include the application version */
		.application_version = CONFIG_REST_CELL_LOCATION_SAMPLE_VERSION
	};

	struct nrf_cloud_device_status dev_status = {
		.modem = &mdm_inf,
		/* Deprecated: The service info "ui" section is no longer used by nRF Cloud */
		.svc = NULL,
		.conn_inf = NRF_CLOUD_INFO_SET
	};

	/* Keep the connection alive so it can be used by the initial cellular
	 * positioning request.
	 */
	rest_ctx.keep_alive = true;

	err = nrf_cloud_rest_shadow_device_status_update(&rest_ctx, device_id, &dev_status);
	if (err) {
		LOG_ERR("Failed to send device status, error: %d", err);
	} else {
		LOG_INF("Device status sent to nRF Cloud");
	}

	/* Set keep alive to false so the connection is closed after the initial
	 * positioning request. The subsequent request may not occur immediately.
	 */
	rest_ctx.keep_alive = false;
}

int init(void)
{
	int err;

	/* Application event manager is used for button press events from the
	 * common application framework (CAF) library
	 */
	err = app_event_manager_init();
	if (err) {
		LOG_ERR("Application Event Manager could not be initialized");
		return err;
	}

	err = nrf_modem_lib_init();
	if (err) {
		LOG_ERR("Modem library initialization failed, error: %d", err);
		return err;
	}

	/* Modem info library is used to obtain the modem FW version
	 * and network info for single-cell requests
	 */
	err = modem_info_init();
	if (err) {
		LOG_ERR("Modem info initialization failed, error: %d", err);
		return err;
	}

	err = modem_info_params_init(&mdm_param);
	if (err) {
		LOG_ERR("Modem info params initialization failed, error: %d", err);
		return err;
	}

	/* Check modem FW version */
	check_modem_fw_version();

	/* Print the nRF Cloud device ID. This device ID should match the ID used
	 * to provision the device on nRF Cloud or to register a public JWT signing key.
	 */
	err = nrf_cloud_client_id_get(device_id, sizeof(device_id));
	if (err) {
		LOG_ERR("Failed to get device ID, error: %d", err);
		return err;
	}

	LOG_INF("Device ID: %s", device_id);

	/* Inform the app event manager that this module is ready to receive events */
	module_set_state(MODULE_STATE_READY);

#if defined(CONFIG_REST_CELL_LOCATION_DO_JITP)
	/* Present option for JITP via REST */
	request_jitp();
#endif

	return 0;
}

static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		LOG_DBG("LTE_LC_EVT_NW_REG_STATUS: %d", evt->nw_reg_status);
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		    (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			if (ready) {
				LOG_ERR("Disconnected from network, restart device to reconnect");
			}
			break;
		}

		LOG_DBG("Network registration status: %s",
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			"Connected - home network" : "Connected - roaming");

		k_sem_give(&lte_connected_sem);
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		if (!ready || (evt->cell.id == LTE_LC_CELL_EUTRAN_ID_INVALID)) {
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

static void connect_to_network(void)
{
	int err;

	LOG_INF("Waiting for network...");

	k_sem_reset(&lte_connected_sem);

	err = lte_lc_connect_async(lte_handler);
	if (err) {
		LOG_ERR("Failed to init LTE module, unable to continue, error: %d", err);
		k_sleep(K_FOREVER);
	}

	err = k_sem_take(&lte_connected_sem, K_MINUTES(NET_CONN_WAIT_MIN));
	if (err == 0) {
		LOG_INF("Connected to network");
	} else if (err == -EAGAIN) {
		LOG_ERR("Failed to connect to network, rebooting in 30s...");
		(void)lte_lc_power_off();
		k_sleep(K_SECONDS(30));
		sys_reboot(SYS_REBOOT_COLD);
	} else {
		LOG_ERR("k_sem_take error %d, unable to continue", err);
		k_sleep(K_FOREVER);
	}

	/* Modem must have valid time/date in order to generate JWTs with
	 * an expiration time
	 */
	modem_time_wait();
}

static void check_credentials(void)
{
	int err = nrf_cloud_credentials_configured_check();

	if (err == -ENOTSUP) {
		LOG_ERR("Required nRF Cloud credentials were not found");
		LOG_INF("Install credentials and then reboot the device");
		k_sleep(K_FOREVER);
	} else if (err) {
		LOG_ERR("nrf_cloud_credentials_configured_check() failed, error: %d", err);
		LOG_WRN("Continuing without verifying that credentials are installed");
	}
}

int main(void)
{
	int err;

	LOG_INF("nRF Cloud REST Cellular Location Sample, version: %s",
		CONFIG_REST_CELL_LOCATION_SAMPLE_VERSION);

	err = init();
	if (err) {
		LOG_ERR("Initialization failed");
		return 0;
	}

	/* Before connecting, ensure nRF Cloud credentials are installed */
	check_credentials();

	connect_to_network();

#if defined(CONFIG_REST_CELL_LOCATION_DO_JITP)
	if (jitp_requested) {
		/* Perform JITP via REST */
		do_jitp();
	}
#endif

	/* Send the device status which contains HW/FW version info and
	 * details about the network connection.
	 */
	if (IS_ENABLED(CONFIG_REST_CELL_SEND_DEVICE_STATUS)) {
		send_device_status();
	}

	/* Initialized, connected, and ready to send cellular location requests */
	ready = true;

	/* Get initial cell info */
	get_cell_info();

	while (1) {
		struct nrf_cloud_location_result cell_pos_result;

		/* Wait for cell info to become available; triggered by
		 * a button press or a change in cell ID.
		 */
		(void)k_sem_take(&cell_info_ready_sem, K_FOREVER);

		(void)k_mutex_lock(&cell_info_mutex, K_FOREVER);

		if (cell_info.current_cell.id != LTE_LC_CELL_EUTRAN_ID_INVALID) {
			LOG_INF("Current cell info: Cell ID: %u, TAC: %u, MCC: %d, MNC: %d",
				cell_info.current_cell.id, cell_info.current_cell.tac,
				cell_info.current_cell.mcc, cell_info.current_cell.mnc);
		} else {
			LOG_WRN("No current serving cell available");
		}

		if (cell_info.ncells_count || cell_info.gci_cells_count) {
			LOG_INF("Performing multi-cell request with "
				"%u neighbor cells and %u GCI cells",
				cell_info.ncells_count, cell_info.gci_cells_count);
		} else {
			LOG_INF("Performing single-cell request");
		}

		LOG_INF("Request configuration:");
		LOG_INF("  High confidence interval   = %s",
			config.hi_conf ? "true" : "false");
		LOG_INF("  Fallback to rough location = %s",
			config.fallback ? "true" : "false");
		LOG_INF("  Reply with result          = %s",
			config.do_reply ? "true" : "false");

		/* Perform REST call */
		err = nrf_cloud_rest_location_get(&rest_ctx, &cell_pos_req, &cell_pos_result);

		(void)k_mutex_unlock(&cell_info_mutex);

		if (err) {
			LOG_ERR("Request failed, error: %d", err);
			if (cell_pos_result.err != NRF_CLOUD_ERROR_NONE) {
				LOG_ERR("nRF Cloud error code: %d", cell_pos_result.err);
			}
			continue;
		}

		LOG_INF("Cellular location request fulfilled with %s",
			cell_pos_result.type == LOCATION_TYPE_SINGLE_CELL ? "single-cell" :
			cell_pos_result.type == LOCATION_TYPE_MULTI_CELL ?  "multi-cell" :
									    "unknown");

		if (config.do_reply) {
			LOG_INF("Lat: %f, Lon: %f, Uncertainty: %u",
				cell_pos_result.lat,
				cell_pos_result.lon,
				cell_pos_result.unc);
		}
	}
}
