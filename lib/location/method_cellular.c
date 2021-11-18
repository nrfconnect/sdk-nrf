/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr.h>
#include <logging/log.h>
#include <modem/location.h>
#include <modem/lte_lc.h>
#include <net/multicell_location.h>

#include "location_core.h"
#include "location_utils.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

BUILD_ASSERT(
	(int)LOCATION_SERVICE_ANY == (int)MULTICELL_SERVICE_ANY &&
	(int)LOCATION_SERVICE_NRF_CLOUD == (int)MULTICELL_SERVICE_NRF_CLOUD &&
	(int)LOCATION_SERVICE_HERE == (int)MULTICELL_SERVICE_HERE &&
	(int)LOCATION_SERVICE_SKYHOOK == (int)MULTICELL_SERVICE_SKYHOOK &&
	(int)LOCATION_SERVICE_POLTE == (int)MULTICELL_SERVICE_POLTE,
	"Incompatible enums location_service and multicell_service");

struct method_cellular_positioning_work_args {
	struct k_work work_item;
	struct location_cellular_config cellular_config;
};

static struct method_cellular_positioning_work_args method_cellular_positioning_work;

static K_SEM_DEFINE(cellmeas_data_ready, 0, 1);

static struct lte_lc_ncell neighbor_cells[CONFIG_MULTICELL_LOCATION_MAX_NEIGHBORS];
static struct lte_lc_cells_info cell_data = {
	.neighbor_cells = neighbor_cells,
};
static bool running;

void method_cellular_lte_ind_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NEIGHBOR_CELL_MEAS: {
		LOG_DBG("Cell measurements results received");
		memset(&cell_data, 0, sizeof(struct lte_lc_cells_info));

		if (evt->cells_info.current_cell.id) {
			/* Copy current and neighbor cell information */
			memcpy(&cell_data, &evt->cells_info, sizeof(struct lte_lc_cells_info));
			memcpy(neighbor_cells, evt->cells_info.neighbor_cells,
			       sizeof(struct lte_lc_ncell) * cell_data.ncells_count);
		} else {
			LOG_INF("No current cell information from modem.");
		}
		if (!evt->cells_info.ncells_count) {
			LOG_INF("No neighbor cell information from modem.");
		}
		k_sem_give(&cellmeas_data_ready);
	} break;
	default:
		break;
	}
}

static int method_cellular_ncellmeas_start(void)
{
	struct location_utils_modem_params_info modem_params = { 0 };
	int err;

	LOG_DBG("Triggering cell measurements");

	err = lte_lc_neighbor_cell_measurement(LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT);
	if (err) {
		LOG_WRN("Failed to initiate neighbor cell measurements: %d, "
			"next: fallback to get modem parameters",
			err);

		/* Doing fallback to get only the mandatory items manually:
		 * cell id, mcc, mnc and tac
		 */
		err = location_utils_modem_params_read(&modem_params);
		if (err < 0) {
			LOG_ERR("Could not obtain modem parameters");
			return err;
		}
		memset(&cell_data, 0, sizeof(struct lte_lc_cells_info));

		/* Filling only the mandatory parameters: */
		cell_data.current_cell.mcc = modem_params.mcc;
		cell_data.current_cell.mnc = modem_params.mnc;
		cell_data.current_cell.tac = modem_params.tac;
		cell_data.current_cell.id = modem_params.cell_id;
		cell_data.current_cell.phys_cell_id = modem_params.phys_cell_id;
		k_sem_give(&cellmeas_data_ready);
	}
	return 0;
}

static void method_cellular_positioning_work_fn(struct k_work *work)
{
	struct multicell_location location;
	struct location_data location_result = { 0 };
	int ret;
	struct method_cellular_positioning_work_args *work_data =
		CONTAINER_OF(work, struct method_cellular_positioning_work_args, work_item);
	const struct location_cellular_config cellular_config = work_data->cellular_config;

	location_core_timer_start(cellular_config.timeout);

	LOG_DBG("Triggering neighbor cell measurements");
	ret = method_cellular_ncellmeas_start();
	if (ret) {
		LOG_WRN("Cannot start neighbor cell measurements");
		location_core_event_cb_error();
		running = false;
		return;
	}

	ret = k_sem_take(&cellmeas_data_ready, K_FOREVER);
	if (!running) {
		return;
	}

	if (cell_data.current_cell.id == 0) {
		LOG_WRN("No cells were found");
		location_core_event_cb_error();
		running = false;
		return;
	}

	/* enum multicell_service can be used directly because of BUILD_ASSERT */
	ret = multicell_location_get(cellular_config.service, &cell_data, &location);
	if (ret) {
		LOG_ERR("Failed to acquire location from multicell_location lib, error: %d", ret);
		location_core_event_cb_error();
	} else {
		location_result.method = LOCATION_METHOD_CELLULAR;
		location_result.latitude = location.latitude;
		location_result.longitude = location.longitude;
		location_result.accuracy = location.accuracy;
		if (running) {
			location_core_event_cb(&location_result);
		}
	}
	running = false;
}

int method_cellular_location_get(const struct location_method_config *config)
{
	/* Note: LTE status not checked, let it fail in NCELLMEAS if no connection */

	method_cellular_positioning_work.cellular_config = config->cellular;
	k_work_submit_to_queue(location_core_work_queue_get(),
			       &method_cellular_positioning_work.work_item);

	running = true;

	return 0;
}

int method_cellular_cancel(void)
{
	if (running) {
		(void)lte_lc_neighbor_cell_measurement_cancel();
		(void)k_work_cancel(&method_cellular_positioning_work.work_item);
		running = false;
		k_sem_reset(&cellmeas_data_ready);
	} else {
		return -EPERM;
	}

	return 0;
}

int method_cellular_init(void)
{
	int ret;

	running = false;

	k_work_init(&method_cellular_positioning_work.work_item,
		    method_cellular_positioning_work_fn);
	lte_lc_register_handler(method_cellular_lte_ind_handler);

	ret = multicell_location_provision_certificate(false);
	if (ret) {
		LOG_ERR("Certificate provisioning failed, ret %d", ret);
		if (ret == -EACCES) {
			LOG_WRN("err: -EACCESS, that might indicate that modem is in state where "
				"cert cannot be written, i.e. not in pwroff or offline");
		}
		return ret;
	}

	return 0;
}
