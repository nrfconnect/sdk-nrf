/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr.h>

#include <logging/log.h>

#include <modem/lte_lc.h>
#include <net/multicell_location.h>
#include <modem/location.h>

#include "location.h"

LOG_MODULE_REGISTER(method_cellular, CONFIG_LOCATION_LOG_LEVEL);

extern location_event_handler_t event_handler;
extern struct loc_event_data current_event_data;

#define METHOD_CELLULAR_STACK_SIZE 2048
#define METHOD_CELLULAR_PRIORITY  5
K_THREAD_STACK_DEFINE(method_cellular_stack, METHOD_CELLULAR_STACK_SIZE);

struct k_work_q method_cellular_work_q;
static struct k_work method_cellular_positioning_work;

static K_SEM_DEFINE(cellmeas_data_ready, 0, 1);

static struct lte_lc_ncell neighbor_cells[CONFIG_MULTICELL_LOCATION_MAX_NEIGHBORS];
static struct lte_lc_cells_info cell_data = {
	.neighbor_cells = neighbor_cells,
};
static bool lte_connected;
static bool running;

void method_cellular_lte_ind_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		     (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			lte_connected = false;
			break;
		}

		LOG_INF("Network registration status: %s",
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			"Connected - home network" : "Connected - roaming");
		lte_connected = true;
		break;		
	case LTE_LC_EVT_NEIGHBOR_CELL_MEAS:
		LOG_INF("Neighbor cell measurements received");

		/* Copy current and neighbor cell information */
		memcpy(&cell_data, &evt->cells_info, sizeof(struct lte_lc_cells_info));
		memcpy(neighbor_cells, evt->cells_info.neighbor_cells,
			sizeof(struct lte_lc_ncell) * cell_data.ncells_count);
		cell_data.neighbor_cells = neighbor_cells;

		k_sem_give(&cellmeas_data_ready);
		break;
	default:
		break;
	}		
}

static int method_cellular_ncellmeas_start(void)
{
	int err = lte_lc_neighbor_cell_measurement();
	if (err) {
		LOG_ERR("Failed to initiate neighbor cell measurements: %d", err);
		event_location_callback_error();
		running = false;
		return err;
	}
	k_work_submit(&method_cellular_positioning_work);
	return 0;
}

static void method_cellular_positioning_work_fn(struct k_work *work)
{
	struct multicell_location location;
	int ret;

	ARG_UNUSED(work);
	k_sem_take(&cellmeas_data_ready, K_FOREVER); /* TODO: cannot be forever? interval only */

	if (running) {
		ret = multicell_location_get(&cell_data, &location);
		if (ret) {
			LOG_ERR("Failed to acquire location from multicell_location lib, error: %d", ret);		
			event_location_callback_error();
		} else {
			event_data_init(LOC_EVT_LOCATION, LOC_METHOD_CELL_ID);
			current_event_data.location.latitude = location.latitude;
			current_event_data.location.longitude = location.longitude;
			current_event_data.location.accuracy = location.accuracy;
			//TODO:
			/*current_event_data.location.datetime.valid = true;
			current_event_data.location.datetime.year = pvt_data.datetime.year;
			current_event_data.location.datetime.month = pvt_data.datetime.month;
			current_event_data.location.datetime.day = pvt_data.datetime.day;
			current_event_data.location.datetime.hour = pvt_data.datetime.hour;
			current_event_data.location.datetime.minute = pvt_data.datetime.minute;
			current_event_data.location.datetime.second = pvt_data.datetime.seconds;
			current_event_data.location.datetime.ms = pvt_data.datetime.ms;*/
			(void)lte_lc_neighbor_cell_measurement_cancel();
			if (running) {
				event_location_callback(&current_event_data);
				running = false;
			}
		}
	}	
}

int method_cellular_configure_and_start(const struct loc_method_config *config, uint16_t interval)
{
	int ret = 0;
	const struct loc_cell_id_config *gnss_config = &config->config.cell_id;

	ARG_UNUSED(gnss_config);

	if (interval > 0 ) {
		LOG_ERR("Periodic cellular positioning mode not supported at the moment.");
		return -EINVAL;
	}

	if (running) {
		LOG_ERR("Previous operation on going.");
		return -EBUSY;
	}

	ret = method_cellular_ncellmeas_start();
	if (ret) {
		return ret;
	}

	LOG_INF("Triggered start of cell measurements");
	running = true;

	return 0;
}

int method_cellular_cancel(void)
{
	if (running) {
		(void)lte_lc_neighbor_cell_measurement_cancel();
		(void)k_work_cancel(&method_cellular_positioning_work);
		running = false;
		k_sem_reset(&cellmeas_data_ready);
	} else {
		LOG_DBG("Not running");
		return -ESRCH;
	}

	return 0;
}

int method_cellular_init(void)
{
	int ret;

	lte_connected = false;
	running = false;
	
	k_work_queue_start(
		&method_cellular_work_q, method_cellular_stack,
		K_THREAD_STACK_SIZEOF(method_cellular_stack),
		METHOD_CELLULAR_PRIORITY, NULL);

	k_work_init(&method_cellular_positioning_work, method_cellular_positioning_work_fn);
	lte_lc_register_handler(method_cellular_lte_ind_handler);

	/* TODO: fetch LTE connection status from modem?
	 * ...because we do not know when lib is initialized
	 */

	ret = multicell_location_provision_certificate(false);
	if (ret) {
		LOG_ERR("Certificate provisioning failed");
		return ret;
	}

	return 0;
}
