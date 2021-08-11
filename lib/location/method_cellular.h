/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef METHOD_CELLULAR_H
#define METHOD_CELLULAR_H

int method_cellular_configure_and_start(struct loc_cell_id_config *cell_config, uint16_t interval);
void method_cellular_init(void);

#endif /* METHOD_CELLULAR_H */
