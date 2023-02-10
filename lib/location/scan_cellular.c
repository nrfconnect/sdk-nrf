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
static struct lte_lc_cells_info scan_cellular_info = {
	.neighbor_cells = neighbor_cells
};
static struct k_sem *scan_cellular_ready;

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
	switch (evt->type) {
	case LTE_LC_EVT_NEIGHBOR_CELL_MEAS: {
		if (scan_cellular_ready == NULL) {
			return;
		}
		LOG_DBG("Cell measurements results received");

		/* Copy current cell information */
		memcpy(&scan_cellular_info.current_cell,
		       &evt->cells_info.current_cell,
		       sizeof(struct lte_lc_cell));

		/* Copy neighbor cell information if present */
		if (evt->cells_info.ncells_count > 0 && evt->cells_info.neighbor_cells) {
			memcpy(scan_cellular_info.neighbor_cells,
			       evt->cells_info.neighbor_cells,
			       sizeof(struct lte_lc_ncell) * evt->cells_info.ncells_count);

			scan_cellular_info.ncells_count = evt->cells_info.ncells_count;
		} else {
			scan_cellular_info.ncells_count = 0;
			LOG_DBG("No neighbor cell information from modem.");
		}

		k_sem_give(scan_cellular_ready);
		scan_cellular_ready = NULL;
	} break;
	default:
		break;
	}
}

int scan_cellular_start(struct k_sem *ncellmeas_ready)
{
	struct location_utils_modem_params_info modem_params = { 0 };
	int err;

	scan_cellular_ready = ncellmeas_ready;
	scan_cellular_info.current_cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;

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
		} else {
			memset(&scan_cellular_info, 0, sizeof(struct lte_lc_cells_info));

			/* Filling only the mandatory parameters */
			scan_cellular_info.current_cell.mcc = modem_params.mcc;
			scan_cellular_info.current_cell.mnc = modem_params.mnc;
			scan_cellular_info.current_cell.tac = modem_params.tac;
			scan_cellular_info.current_cell.id = modem_params.cell_id;
			scan_cellular_info.current_cell.phys_cell_id = modem_params.phys_cell_id;
		}
		k_sem_give(scan_cellular_ready);
		scan_cellular_ready = NULL;
	}
	return err;
}

int scan_cellular_cancel(void)
{
	if (scan_cellular_ready != NULL) {
		/* Cancel/stopping might trigger a NCELLMEAS notification */
		(void)lte_lc_neighbor_cell_measurement_cancel();
		k_sem_reset(scan_cellular_ready);
	} else {
		return -EPERM;
	}

	scan_cellular_ready = NULL;

	return 0;
}

int scan_cellular_init(void)
{
	lte_lc_register_handler(scan_cellular_lte_ind_handler);

	return 0;
}
