/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <utils/common.h>
#include <eap_peer/eap_config.h>
#include <wpa_supplicant/config.h>
#include <zephyr/net/wifi_mgmt.h>

int config_process_blob(struct wpa_config *config, char *name, uint8_t *data,
				uint32_t data_len);

int process_certificates(void);
