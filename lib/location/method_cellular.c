/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/location.h>
#include <modem/lte_lc.h>

#include "location_core.h"
#include "location_utils.h"
#include "location_service_utils.h"
#include "cloud_service/cloud_service.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

struct method_cellular_positioning_work_args {
	struct k_work work_item;
	struct location_cellular_config cellular_config;
};

static struct method_cellular_positioning_work_args method_cellular_positioning_work;

static K_SEM_DEFINE(cellmeas_data_ready, 0, 1);

static struct lte_lc_ncell neighbor_cells[CONFIG_LTE_NEIGHBOR_CELLS_MAX];
static struct lte_lc_cells_info cell_data = {
	.neighbor_cells = neighbor_cells,
};
static bool running;

void method_cellular_lte_ind_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NEIGHBOR_CELL_MEAS: {
		if (!running) {
			return;
		}
		LOG_DBG("Cell measurements results received");

		/* Copy current cell information. */
		memcpy(&cell_data.current_cell,
		       &evt->cells_info.current_cell,
		       sizeof(struct lte_lc_cell));

		/* Copy neighbor cell information if present. */
		if (evt->cells_info.ncells_count > 0 && evt->cells_info.neighbor_cells) {
			memcpy(cell_data.neighbor_cells,
			       evt->cells_info.neighbor_cells,
			       sizeof(struct lte_lc_ncell) * evt->cells_info.ncells_count);

			cell_data.ncells_count = evt->cells_info.ncells_count;
		} else {
			cell_data.ncells_count = 0;
			LOG_DBG("No neighbor cell information from modem.");
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

	/* Starting measurements with lte_lc default parameters */
	err = lte_lc_neighbor_cell_measurement(NULL);
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
	int64_t ncellmeas_start_time;
	int ret;
	struct method_cellular_positioning_work_args *work_data =
		CONTAINER_OF(work, struct method_cellular_positioning_work_args, work_item);
	const struct location_cellular_config cellular_config = work_data->cellular_config;

	location_core_timer_start(cellular_config.timeout);

	ncellmeas_start_time = k_uptime_get();

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

	/* Stop the timer and let rest_client timer handle the request */
	location_core_timer_stop();

#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
	location_core_event_cb_cellular_request(&cell_data);
#else
	struct cloud_service_pos_req params = { 0 };
	struct location_data location;
	struct location_data location_result = { 0 };
	int64_t ncellmeas_time;

	if (cell_data.current_cell.id == LTE_LC_CELL_EUTRAN_ID_INVALID) {
		LOG_WRN("Current cell ID not valid");
		location_core_event_cb_error();
		running = false;
		return;
	}

	/* NCELLMEAS done at this point of time. Store current time to response. */
	location_utils_systime_to_location_datetime(&location_result.datetime);

	/* Check if timeout is given */
	params.timeout_ms = cellular_config.timeout;
	if (cellular_config.timeout != SYS_FOREVER_MS) {
		/* +1 to round the time up */
		ncellmeas_time = (k_uptime_get() - ncellmeas_start_time) + 1;

		/* Check if timeout has already elapsed */
		if (ncellmeas_time >= cellular_config.timeout) {
			LOG_WRN("Timeout occurred during neighbour cell measurement");
			location_core_event_cb_timeout();
			running = false;
			return;
		}
		/* Take time used for neighbour cell measurements into account */
		params.timeout_ms = cellular_config.timeout - ncellmeas_time;
	}

	params.service = cellular_config.service;
	params.cell_data = &cell_data;
	ret = cloud_service_location_get(&params, &location);
	if (ret) {
		LOG_ERR("Failed to acquire location using cellular positioning, error: %d", ret);
		if (ret == -ETIMEDOUT) {
			location_core_event_cb_timeout();
		} else {
			location_core_event_cb_error();
		}
	} else {
		location_result.latitude = location.latitude;
		location_result.longitude = location.longitude;
		location_result.accuracy = location.accuracy;
		if (running) {
			running = false;
			location_core_event_cb(&location_result);
		}
	}
#endif /* defined(CONFIG_LOCATION_SERVICE_EXTERNAL) */
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
		running = false;

		/* Cancel/stopping might trigger a NCELLMEAS notification */
		(void)lte_lc_neighbor_cell_measurement_cancel();
		(void)k_work_cancel(&method_cellular_positioning_work.work_item);
		k_sem_reset(&cellmeas_data_ready);
	} else {
		return -EPERM;
	}

	return 0;
}

int method_cellular_init(void)
{
	running = false;

	k_work_init(&method_cellular_positioning_work.work_item,
		    method_cellular_positioning_work_fn);
	lte_lc_register_handler(method_cellular_lte_ind_handler);

#if defined(CONFIG_LOCATION_SERVICE_HERE)
	int ret = location_service_utils_provision_ca_certificates();

	if (ret) {
		LOG_ERR("Certificate provisioning failed, ret %d", ret);
		if (ret == -EACCES) {
			LOG_WRN("err: -EACCES, that might indicate that modem is in state where "
				"cert cannot be written, i.e. not in pwroff or offline");
		}
		return ret;
	}
#endif
	return 0;
}
