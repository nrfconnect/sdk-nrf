/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Prepare an alert message to be sent to nRF Cloud.
 *
 * @param output Pointer to nrf_cloud_data structure where the prepared alert message will be stored
 * @param type The type of alert to be sent (from nrf_cloud_alert_type enum)
 * @param value The numeric value associated with the alert
 * @param description Text description of the alert
 *
 * @return 0 on success, otherwise, a negative error number.
 */
int alert_prepare(struct nrf_cloud_data *output, enum nrf_cloud_alert_type type, float value,
		  const char *description);
