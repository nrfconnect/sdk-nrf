/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing OS specific definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <zephyr/logging/log.h>

#include "osal_ops.h"

LOG_MODULE_REGISTER(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

const struct nrf_wifi_osal_ops nrf_wifi_os_zep_ops = {
};
