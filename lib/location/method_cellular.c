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

#include "loc_core.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

extern location_event_handler_t event_handler;
extern struct loc_event_data current_event_data;

struct k_work_args {
	struct k_work work_item;
	struct loc_cellular_config cellular_config;
};

static struct k_work_args method_cellular_positioning_work;

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
		struct lte_lc_cells_info cells = evt->cells_info;
		struct lte_lc_cell cur_cell = cells.current_cell;

		LOG_DBG("Cell measurements results received");
		memset(&cell_data, 0, sizeof(struct lte_lc_cells_info));

		if (cur_cell.id) {
			/* Copy current and neighbor cell information */
			memcpy(&cell_data, &evt->cells_info, sizeof(struct lte_lc_cells_info));
			memcpy(neighbor_cells, evt->cells_info.neighbor_cells,
				sizeof(struct lte_lc_ncell) * cell_data.ncells_count);
			cell_data.neighbor_cells = neighbor_cells;
		} else {
			LOG_INF("No current cell information from modem.");
		}
		if (!cells.ncells_count) {
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
	int err;

	LOG_DBG("Triggering start of cell measurements");
	err = lte_lc_neighbor_cell_measurement();
	if (err) {
		LOG_ERR("Failed to initiate neighbor cell measurements: %d", err);
		loc_core_event_cb_error();
		running = false;
		return err;
	}
	return 0;
}

static void method_cellular_positioning_work_fn(struct k_work *work)
{
	struct multicell_location location;
	struct loc_location location_result = { 0 };
	int ret;
	struct k_work_args *work_data = CONTAINER_OF(work, struct k_work_args, work_item);
	const struct loc_cellular_config cellular_config = work_data->cellular_config;

	loc_core_timer_start(cellular_config.timeout);

	ret = method_cellular_ncellmeas_start();
	if (ret) {
		LOG_WRN("Cannot start cell measurements - cannot fetch location");
		loc_core_event_cb_error();
		running = false;
		return;
	}

	ret = k_sem_take(&cellmeas_data_ready, K_SECONDS(60)); /* TODO: timeout to params? */

	if (ret) {
		LOG_WRN("Timeout for waiting cell measurements - cannot fetch location");
		loc_core_event_cb_error();
		running = false;
	} else if (running) {
		if (cell_data.current_cell.id == 0) {
			LOG_WRN("No cells were found - cannot fetch location");
			loc_core_event_cb_error();
			running = false;
			return;
		}

		/* TODO: pass a device id / IMEI (fetched runtime?) */
		ret = multicell_location_get(&cell_data, NULL, &location);
		if (ret) {
			LOG_ERR("Failed to acquire location from multicell_location lib, error: %d",
				ret);
			loc_core_event_cb_error();
			running = false;
		} else {
			location_result.latitude = location.latitude;
			location_result.longitude = location.longitude;
			location_result.accuracy = location.accuracy;
			(void)lte_lc_neighbor_cell_measurement_cancel();
			if (running) {
				loc_core_event_cb(&location_result);
				running = false;
			}
		}
	}
}

int method_cellular_location_get(const struct loc_method_config *config)
{
	const struct loc_cellular_config cellular_config = config->cellular;

	if (running) {
		LOG_ERR("Previous operation on going.");
		return -EBUSY;
	}
	/* Note: LTE status not checked, let it fail in NCELLMEAS if no connection */

	method_cellular_positioning_work.cellular_config = cellular_config;
	k_work_submit_to_queue(loc_core_work_queue_get(),
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
			LOG_WRN("err: -EACCESS, that might indicate that modem is in "
				"state where cert cannot be written, i.e. not in pwroff");
		}
		return ret;
	}

	return 0;
}
