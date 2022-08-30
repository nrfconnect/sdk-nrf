/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <modem/lte_lc.h>
#include <dk_buttons_and_leds.h>
#include <net/multicell_location.h>

#include <zephyr/logging/log.h>

#define DEVICE_ID_MAX_LEN 64

#if defined(CONFIG_MULTICELL_LOCATION_SAMPLE_DEV_ID_IMEI)
#define CGSN_RESPONSE_LENGTH 19
#define IMEI_LEN 15
#define DEV_ID_IMEI_LEN (sizeof(CONFIG_MULTICELL_LOCATION_SAMPLE_DEV_ID_IMEI_PREFIX) - 1 \
			 + IMEI_LEN)
BUILD_ASSERT(DEV_ID_IMEI_LEN <= DEVICE_ID_MAX_LEN,
	"Device ID prefix length plus IMEI length must not exceed DEVICE_ID_MAX_LEN");
#elif defined(CONFIG_MULTICELL_LOCATION_SAMPLE_DEV_ID_COMPILE_TIME)
BUILD_ASSERT(sizeof(CONFIG_MULTICELL_LOCATION_SAMPLE_DEV_ID_COMPILE_TIME_STRING) > 1,
	"Compile time device ID is not set");
#endif

LOG_MODULE_REGISTER(multicell_location_sample, CONFIG_MULTICELL_LOCATION_SAMPLE_LOG_LEVEL);

BUILD_ASSERT(!IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT),
	"The sample does not support automatic LTE connection establishment");

static atomic_t connected;
static K_SEM_DEFINE(lte_connected, 0, 1);
static K_SEM_DEFINE(cell_data_ready, 0, 1);
static struct k_work_delayable periodic_search_work;
static struct k_work cell_change_search_work;
static struct lte_lc_ncell neighbor_cells[CONFIG_LTE_NEIGHBOR_CELLS_MAX];
static struct lte_lc_cells_info cell_data = {
	.neighbor_cells = neighbor_cells,
};

static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		     (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			break;
		}

		LOG_INF("Network registration status: %s",
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			"Connected - home network" : "Connected - roaming");
		k_sem_give(&lte_connected);
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		LOG_INF("PSM parameter update: TAU: %d, Active time: %d",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE: {
		char log_buf[60];
		ssize_t len;

		len = snprintk(log_buf, sizeof(log_buf),
			       "eDRX parameter update: eDRX: %f, PTW: %f",
			       evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		if (len > 0) {
			LOG_INF("%s", log_buf);
		}
		break;
	}
	case LTE_LC_EVT_RRC_UPDATE:
		LOG_INF("RRC mode: %s",
			evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			"Connected" : "Idle");
		break;
	case LTE_LC_EVT_CELL_UPDATE: {
		LOG_INF("LTE cell changed: Cell ID: %d, Tracking area: %d",
			evt->cell.id, evt->cell.tac);

#if defined(CONFIG_MULTICELL_LOCATION_SAMPLE_REQUEST_CELL_CHANGE)
		static uint32_t prev_cell_id;

		if (evt->cell.id != prev_cell_id) {
			if (evt->cell.id != LTE_LC_CELL_EUTRAN_ID_INVALID) {
				k_work_submit(&cell_change_search_work);
			}

			prev_cell_id = evt->cell.id;
		}
#endif
		break;
	}
	case LTE_LC_EVT_LTE_MODE_UPDATE:
		LOG_INF("Active LTE mode changed: %s",
			evt->lte_mode == LTE_LC_LTE_MODE_NONE ? "None" :
			evt->lte_mode == LTE_LC_LTE_MODE_LTEM ? "LTE-M" :
			evt->lte_mode == LTE_LC_LTE_MODE_NBIOT ? "NB-IoT" :
			"Unknown");
		break;
	case LTE_LC_EVT_NEIGHBOR_CELL_MEAS:
		LOG_INF("Neighbor cell measurements received");

		if (evt->cells_info.current_cell.id == LTE_LC_CELL_EUTRAN_ID_INVALID) {
			LOG_DBG("Cell ID not valid.");
			break;
		}

		/* Copy current cell information. */
		memcpy(&cell_data.current_cell,
		       &evt->cells_info.current_cell,
		       sizeof(struct lte_lc_cell));

		/* Copy neighbor cell information if present. */
		if (evt->cells_info.ncells_count > 0 && evt->cells_info.neighbor_cells) {
			memcpy(neighbor_cells,
			       evt->cells_info.neighbor_cells,
			       sizeof(struct lte_lc_ncell) * evt->cells_info.ncells_count);

			cell_data.ncells_count = evt->cells_info.ncells_count;
		} else {
			cell_data.ncells_count = 0;
		}

		LOG_INF("Neighbor cells found: %d", cell_data.ncells_count);

		k_sem_give(&cell_data_ready);
		break;
	default:
		break;
	}
}

static int lte_connect(void)
{
	int err;

	if (IS_ENABLED(CONFIG_MULTICELL_LOCATION_SAMPLE_PSM)) {
		err = lte_lc_psm_req(true);
		if (err) {
			LOG_ERR("Failed to request PSM, error: %d", err);
		}
	} else {
		err = lte_lc_psm_req(false);
		if (err) {
			LOG_ERR("Failed to disable PSM, error: %d", err);
		}
	}

	if (IS_ENABLED(CONFIG_MULTICELL_LOCATION_SAMPLE_EDRX)) {
		err = lte_lc_edrx_req(true);
		if (err) {
			LOG_ERR("Failed to request eDRX, error: %d", err);
		}
	} else {
		err = lte_lc_edrx_req(false);
		if (err) {
			LOG_ERR("Failed to disable eDRX, error: %d", err);
		}
	}

	err = lte_lc_init_and_connect_async(lte_handler);
	if (err) {
		LOG_ERR("Modem could not be configured, error: %d",
			err);
		return err;
	}

	/* Check LTE events of type LTE_LC_EVT_NW_REG_STATUS in
	 * lte_handler() to determine when the LTE link is up.
	 */

	return 0;
}

static void start_cell_measurements(void)
{
	int err;

	if (!atomic_get(&connected)) {
		return;
	}

	err = lte_lc_neighbor_cell_measurement(LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT);
	if (err) {
		LOG_ERR("Failed to initiate neighbor cell measurements, error: %d",
			err);
	}
}

static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & DK_BTN1_MSK) {
		if (!atomic_get(&connected)) {
			LOG_INF("Ignoring button press, not connected to network");
			return;
		}

		LOG_INF("Button 1 pressed, starting cell measurements");
		start_cell_measurements();
	}
}

static void cell_change_search_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	if (!atomic_get(&connected)) {
		return;
	}

	LOG_INF("Cell change triggered start of cell measurements");
	start_cell_measurements();
}

static void periodic_search_work_fn(struct k_work *work)
{
	LOG_INF("Periodical start of cell measurements");
	start_cell_measurements();

	k_work_reschedule(k_work_delayable_from_work(work),
		K_SECONDS(CONFIG_MULTICELL_LOCATION_SAMPLE_REQUEST_PERIODIC_INTERVAL));
}

static void print_cell_data(void)
{
	if (cell_data.current_cell.id == LTE_LC_CELL_EUTRAN_ID_INVALID) {
		LOG_WRN("No cells were found");
		return;
	}

	LOG_INF("Current cell:");
	LOG_INF("\tMCC: %03d", cell_data.current_cell.mcc);
	LOG_INF("\tMNC: %03d", cell_data.current_cell.mnc);
	LOG_INF("\tCell ID: %d", cell_data.current_cell.id);
	LOG_INF("\tTAC: %d", cell_data.current_cell.tac);
	LOG_INF("\tEARFCN: %d", cell_data.current_cell.earfcn);
	LOG_INF("\tTiming advance: %d", cell_data.current_cell.timing_advance);
	LOG_INF("\tMeasurement time: %lld", cell_data.current_cell.measurement_time);
	LOG_INF("\tPhysical cell ID: %d", cell_data.current_cell.phys_cell_id);
	LOG_INF("\tRSRP: %d", cell_data.current_cell.rsrp);
	LOG_INF("\tRSRQ: %d", cell_data.current_cell.rsrq);

	if (cell_data.ncells_count == 0) {
		LOG_INF("*** No neighbor cells found ***");
		return;
	}

	for (size_t i = 0; i < cell_data.ncells_count; i++) {
		LOG_INF("Neighbor cell %d", i + 1);
		LOG_INF("\tEARFCN: %d", cell_data.neighbor_cells[i].earfcn);
		LOG_INF("\tTime difference: %d", cell_data.neighbor_cells[i].time_diff);
		LOG_INF("\tPhysical cell ID: %d", cell_data.neighbor_cells[i].phys_cell_id);
		LOG_INF("\tRSRP: %d", cell_data.neighbor_cells[i].rsrp);
		LOG_INF("\tRSRQ: %d", cell_data.neighbor_cells[i].rsrq);
	}
}

static void request_location(enum multicell_service service, const char *service_str)
{
	int err;
	struct multicell_location_params params = { 0 };
	struct multicell_location location;

	LOG_INF("Sending location request for %s ...", service_str);

	params.service = service;
	params.cell_data = &cell_data;
	params.timeout = SYS_FOREVER_MS;
	err = multicell_location_get(&params, &location);
	if (err) {
		LOG_ERR("Failed to acquire location, error: %d", err);
		return;
	}

	LOG_INF("Location obtained from %s: ", service_str);
	LOG_INF("\tLatitude: %f", location.latitude);
	LOG_INF("\tLongitude: %f", location.longitude);
	LOG_INF("\tAccuracy: %.0f", location.accuracy);
}

void main(void)
{
	int err;

	LOG_INF("Multicell location sample has started");

	k_work_init(&cell_change_search_work, cell_change_search_work_fn);
	k_work_init_delayable(&periodic_search_work, periodic_search_work_fn);

	err = multicell_location_provision_certificate(false);
	if (err) {
		LOG_ERR("Certificate provisioning failed, exiting application");
		return;
	}

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("Failed to initialize DK library, error: %d", err);
	}

	err = lte_connect();
	if (err) {
		LOG_ERR("Failed to connect to LTE network, error: %d", err);
		return;
	}

	LOG_INF("Connecting to LTE network, this may take several minutes...");

	k_sem_take(&lte_connected, K_FOREVER);
	atomic_set(&connected, 1);

	LOG_INF("Connected to LTE network");

	if (IS_ENABLED(CONFIG_MULTICELL_LOCATION_SAMPLE_REQUEST_PERIODIC)) {
		LOG_INF("Requesting neighbor cell information every %d seconds",
			CONFIG_MULTICELL_LOCATION_SAMPLE_REQUEST_PERIODIC_INTERVAL);
		k_work_schedule(&periodic_search_work, K_NO_WAIT);
	} else {
		start_cell_measurements();
	}

	while (true) {
		k_sem_take(&cell_data_ready, K_FOREVER);

		if (CONFIG_MULTICELL_LOCATION_SAMPLE_PRINT_DATA) {
			print_cell_data();
		}

		/* Request location for all different services to demonstrate the possibilities */
#if defined(CONFIG_MULTICELL_LOCATION_SERVICE_NRF_CLOUD)
		request_location(MULTICELL_SERVICE_NRF_CLOUD, "nRF Cloud");
#endif
#if defined(CONFIG_MULTICELL_LOCATION_SERVICE_HERE)
		request_location(MULTICELL_SERVICE_HERE, "HERE");
#endif
		request_location(MULTICELL_SERVICE_ANY, "Any");
	}
}
