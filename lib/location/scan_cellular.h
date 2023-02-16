/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SCAN_CELLULAR_H
#define SCAN_CELLULAR_H

#include <modem/lte_lc.h>

int scan_cellular_init(void);
int scan_cellular_start(struct k_sem *ncellmeas_ready);
struct lte_lc_cells_info *scan_cellular_results_get(void);
int scan_cellular_cancel(void);

#endif /* SCAN_CELLULAR_H */
