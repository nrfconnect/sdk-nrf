/* Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SHADOW_H
#define SHADOW_H

void send_initial_log_level(void);
void shadow_config_cloud_connected(void);
void handle_shadow_event(struct nrf_cloud_obj_shadow_data *const shadow);

#endif /* SHADOW_H */
