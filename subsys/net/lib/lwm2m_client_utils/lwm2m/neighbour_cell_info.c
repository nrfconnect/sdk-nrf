/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <zephyr/net/lwm2m.h>
#include <net/lwm2m_client_utils.h>
#include <lwm2m_engine.h>

#include <modem/modem_info.h>
#include <modem/lte_lc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lwm2m_neighbour_cell, CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL);
#define MAX_INSTANCE_COUNT CONFIG_LWM2M_CLIENT_UTILS_SIGNAL_MEAS_INFO_INSTANCE_COUNT

static K_SEM_DEFINE(rrc_idle, 0, 1);
static bool measurement_scheduled;

static void update_signal_meas_object(const struct lte_lc_ncell *const cell, uint16_t index)
{
	int obj_inst_id;
	char path[sizeof("10256/65535/0")];

	obj_inst_id = lwm2m_signal_meas_info_index_to_inst_id(index);

	snprintk(path, sizeof(path), "10256/%" PRIu16 "/0", obj_inst_id);
	lwm2m_engine_set_s32(path, cell->phys_cell_id);
	/* We don't set the resource 1 as the lte_lc_ncell struct doesn't
	 * contain MCC and MNC for calculating ECGI
	 */
	snprintk(path, sizeof(path), "10256/%" PRIu16 "/2", obj_inst_id);
	lwm2m_engine_set_s32(path, cell->earfcn);
	snprintk(path, sizeof(path), "10256/%" PRIu16 "/3", obj_inst_id);
	lwm2m_engine_set_s32(path, RSRP_IDX_TO_DBM(cell->rsrp));
	snprintk(path, sizeof(path), "10256/%" PRIu16 "/4", obj_inst_id);
	lwm2m_engine_set_s32(path, RSRQ_IDX_TO_DB(cell->rsrq));
	snprintk(path, sizeof(path), "10256/%" PRIu16 "/5", obj_inst_id);
	lwm2m_engine_set_s32(path, cell->time_diff);
}

static void reset_signal_meas_object(uint16_t index)
{
	int obj_inst_id;
	char path[sizeof("10256/65535/0")];

	obj_inst_id = lwm2m_signal_meas_info_index_to_inst_id(index);

	snprintk(path, sizeof(path), "10256/%" PRIu16 "/0", obj_inst_id);
	lwm2m_engine_set_s32(path, 0);
	snprintk(path, sizeof(path), "10256/%" PRIu16 "/2", obj_inst_id);
	lwm2m_engine_set_s32(path, 0);
	snprintk(path, sizeof(path), "10256/%" PRIu16 "/3", obj_inst_id);
	lwm2m_engine_set_s32(path, 0);
	snprintk(path, sizeof(path), "10256/%" PRIu16 "/4", obj_inst_id);
	lwm2m_engine_set_s32(path, 0);
	snprintk(path, sizeof(path), "10256/%" PRIu16 "/5", obj_inst_id);
	lwm2m_engine_set_s32(path, 0);
}

int lwm2m_update_signal_meas_objects(const struct lte_lc_cells_info *const cells)
{
	static int neighbours;
	int i = 0;

	if (cells == NULL) {
		LOG_ERR("Invalid pointer");
		return -EINVAL;
	}

	if (cells->ncells_count == 0) {
		LOG_DBG("No neighbouring cells found");
		return -ENODATA;
	}

	LOG_INF("Updating information for %d neighbouring cells", cells->ncells_count);

	for (i = 0; i < MAX_INSTANCE_COUNT && i < cells->ncells_count; i++) {
		update_signal_meas_object(&cells->neighbor_cells[i], i);
	}

	/* If we have less neighbouring cells than last time, reset
	 * object instances exceeding the current cell count
	 */
	if (cells->ncells_count < neighbours) {
		for (i = cells->ncells_count; i < MAX_INSTANCE_COUNT && i < neighbours; i++) {
			reset_signal_meas_object(i);
		}
	}
	neighbours = cells->ncells_count;

	return 0;
}

void lwm2m_ncell_schedule_measurement(void)
{
	if (measurement_scheduled) {
		LOG_WRN("Measurement already scheduled, waiting for RRC idle");
		return;
	}

	if (k_sem_take(&rrc_idle, K_NO_WAIT) == -EBUSY) {
		LOG_INF("RRC connected, measure when idle");
		measurement_scheduled = true;
		return;
	}

	lte_lc_neighbor_cell_measurement(LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT);
	measurement_scheduled = false;
	k_sem_give(&rrc_idle);
}

void ncell_notification_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NEIGHBOR_CELL_MEAS: {
		int err = lwm2m_update_signal_meas_objects(&evt->cells_info);

		if (err == -ENODATA) {
			LOG_DBG("No neighboring cells available");
		} else if (err) {
			LOG_ERR("lwm2m_update_signal_meas_objects, error: %d", err);
		}
	};
		break;
	case LTE_LC_EVT_RRC_UPDATE:
		if (evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED) {
			k_sem_reset(&rrc_idle);
		} else if (evt->rrc_mode == LTE_LC_RRC_MODE_IDLE) {
			if (measurement_scheduled) {
				lte_lc_neighbor_cell_measurement(
					LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT);
				measurement_scheduled = false;
			}
			k_sem_give(&rrc_idle);
		}
		break;
	default:
		break;
	}
}

int lwm2m_ncell_handler_register(void)
{
	LOG_INF("Registering ncell notification handler");
	lte_lc_register_handler(ncell_notification_handler);
	return 0;
}
