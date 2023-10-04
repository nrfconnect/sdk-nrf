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
#include "cloud_service/cloud_service.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

static struct lte_lc_ncell neighbor_cells[CONFIG_LTE_NEIGHBOR_CELLS_MAX];
static struct lte_lc_cell gci_cells[CONFIG_LTE_NEIGHBOR_CELLS_MAX];
static struct lte_lc_cells_info scan_cellular_info = {
	.neighbor_cells = neighbor_cells,
	.gci_cells = gci_cells
};

static volatile bool running;
static volatile bool timeout_occurred;
/* Indicates when individual ncellmeas operation is completed. This is internal to this file. */
static struct k_sem scan_cellular_sem_ncellmeas_evt;
/* Requested number of cells to be searched. */
static int8_t scan_cellular_cell_count;

/** Handler for timeout. */
static void scan_cellular_timeout_work_fn(struct k_work *work);

/** Work item for timeout handler. */
K_WORK_DELAYABLE_DEFINE(scan_cellular_timeout_work, scan_cellular_timeout_work_fn);

/**
 * Handler for backup timeout, which ensures we won't be waiting for LTE_LC_EVT_NEIGHBOR_CELL_MEAS
 * event forever after lte_lc_neighbor_cell_measurement_cancel() in case it would never be sent.
 */
static void scan_cellular_timeout_backup_work_fn(struct k_work *work);

/** Work item for backup timeout handler. */
K_WORK_DELAYABLE_DEFINE(scan_cellular_timeout_backup_work, scan_cellular_timeout_backup_work_fn);

struct lte_lc_cells_info *scan_cellular_results_get(void)
{
	if (scan_cellular_info.current_cell.id == LTE_LC_CELL_EUTRAN_ID_INVALID) {
		LOG_WRN("Current cell ID not valid");
		return NULL;
	}

	return &scan_cellular_info;
}

void scan_cellular_lte_ind_handler(const struct lte_lc_evt *const evt)
{
	if (!running) {
		return;
	}

	switch (evt->type) {
	case LTE_LC_EVT_NEIGHBOR_CELL_MEAS: {
		k_work_cancel_delayable(&scan_cellular_timeout_backup_work);

		LOG_DBG("Cell measurements results received: "
			"ncells_count=%d, gci_cells_count=%d, current_cell.id=0x%08X",
			evt->cells_info.ncells_count,
			evt->cells_info.gci_cells_count,
			evt->cells_info.current_cell.id);

		if (evt->cells_info.current_cell.id != LTE_LC_CELL_EUTRAN_ID_INVALID) {
			/* Copy current cell information. We are seeing this is not set for GCI
			 * search sometimes but we have it for the previous normal neighbor search.
			 */
			memcpy(&scan_cellular_info.current_cell,
			&evt->cells_info.current_cell,
			sizeof(struct lte_lc_cell));
		}

		/* Copy neighbor cell information if present */
		if (evt->cells_info.ncells_count > 0 && evt->cells_info.neighbor_cells) {
			memcpy(scan_cellular_info.neighbor_cells,
			       evt->cells_info.neighbor_cells,
			       sizeof(struct lte_lc_ncell) * evt->cells_info.ncells_count);

			scan_cellular_info.ncells_count = evt->cells_info.ncells_count;
		} else {
			LOG_DBG("No neighbor cell information from modem");
		}

		/* Copy surrounding cell information if present */
		if (evt->cells_info.gci_cells_count > 0 && evt->cells_info.gci_cells) {
			memcpy(scan_cellular_info.gci_cells,
			       evt->cells_info.gci_cells,
			       sizeof(struct lte_lc_cell) * evt->cells_info.gci_cells_count);

			scan_cellular_info.gci_cells_count = evt->cells_info.gci_cells_count;
		} else {
			LOG_DBG("No surrounding cell information from modem");
		}

		k_sem_give(&scan_cellular_sem_ncellmeas_evt);
	} break;
	default:
		break;
	}
}

void scan_cellular_execute(int32_t timeout, uint8_t cell_count)
{
	struct lte_lc_ncellmeas_params ncellmeas_params = {
		.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_LIGHT,
		.gci_count = 0
	};
	int err;

	running = true;
	timeout_occurred = false;
	scan_cellular_cell_count = cell_count;
	scan_cellular_info.current_cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;
	scan_cellular_info.ncells_count = 0;
	scan_cellular_info.gci_cells_count = 0;

	LOG_DBG("Triggering cell measurements");

	if (timeout != SYS_FOREVER_MS && timeout > 0) {
		LOG_DBG("Starting cellular timer with timeout=%d", timeout);
		k_work_schedule(&scan_cellular_timeout_work, K_MSEC(timeout));
	}

	err = lte_lc_neighbor_cell_measurement(&ncellmeas_params);
	if (err) {
		LOG_ERR("Failed to initiate neighbor cell measurements: %d", err);
		goto end;
	}
	err = k_sem_take(&scan_cellular_sem_ncellmeas_evt, K_FOREVER);
	if (err) {
		/* Semaphore was reset so stop search procedure */
		err = 0;
		goto end;
	}

	if (timeout_occurred) {
		LOG_DBG("Timeout occurred after 1st neighbor measurement");
		goto end;
	}

	/* Calculate the number of GCI cells to be requested.
	 * We should subtract 1 for current cell as ncell_count won't include it.
	 * GCI count requested from modem includes also current cell so we should add 1.
	 * So these two cancel each other.
	 */
	scan_cellular_cell_count = scan_cellular_cell_count - scan_cellular_info.ncells_count;

	LOG_DBG("scan_cellular_execute: scan_cellular_cell_count=%d", scan_cellular_cell_count);

	if (scan_cellular_cell_count > 1) {

		ncellmeas_params.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_LIGHT;
		ncellmeas_params.gci_count = scan_cellular_cell_count;

		err = lte_lc_neighbor_cell_measurement(&ncellmeas_params);
		if (err) {
			LOG_WRN("Failed to initiate GCI cell measurements: %d", err);
			/* Clearing 'err' because normal neighbor search has succeeded
			 * so those are still valid and positioning can procede with that data
			 */
			err = 0;
			goto end;
		}
		k_sem_take(&scan_cellular_sem_ncellmeas_evt, K_FOREVER);
	}

end:
	k_work_cancel_delayable(&scan_cellular_timeout_work);
	running = false;
}

int scan_cellular_cancel(void)
{
	if (running) {
		/* Cancel/stopping might trigger a NCELLMEAS notification */
		(void)lte_lc_neighbor_cell_measurement_cancel();
		k_sem_reset(&scan_cellular_sem_ncellmeas_evt);
	} else {
		return -EPERM;
	}

	running = false;

	return 0;
}

int scan_cellular_init(void)
{
	lte_lc_register_handler(scan_cellular_lte_ind_handler);

	k_sem_init(&scan_cellular_sem_ncellmeas_evt, 0, 1);

	return 0;
}

static void scan_cellular_timeout_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	LOG_INF("Cellular method specific timeout expired");

	(void)lte_lc_neighbor_cell_measurement_cancel();
	timeout_occurred = true;

	k_work_schedule(&scan_cellular_timeout_backup_work, K_MSEC(2000));
}

static void scan_cellular_timeout_backup_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	k_sem_reset(&scan_cellular_sem_ncellmeas_evt);
}
