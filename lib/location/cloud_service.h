/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CLOUD_SERVICE_H_
#define CLOUD_SERVICE_H_

#include <modem/location.h>
#include <modem/lte_lc.h>
#include <net/wifi_location_common.h>
#include <net/nrf_cloud.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Cloud positioning input parameters. */
struct cloud_service_pos_req {
	/** Neighbor cell data. */
	struct lte_lc_cells_info *cell_data;
	/** Wi-Fi scanning results data. */
	struct wifi_scan_info *wifi_data;
	/**
	 * @brief Timeout (in milliseconds) of the cloud positioning procedure.
	 * SYS_FOREVER_MS means that the timer is disabled.
	 */
	int32_t timeout_ms;
};

/**
 * @brief Generate location request, send, and parse response.
 *
 * @param[in] params Cloud positioning parameters to be used.
 * @param[out] location Storage for the result from response parsing.
 * @param[out] cloud_err nRF Cloud error code if the cloud responded with an error,
 *                       otherwise NRF_CLOUD_ERROR_NONE.
 *
 * @return 0 on success, positive CoAP result code if the cloud responded with an
 *         error over CoAP, or negative error code on failure.
 * @retval -ETIMEDOUT No response from the cloud in time.
 * @retval -EACCES Cloud connection is not established.
 * @retval -EFAULT The cloud responded with an error over MQTT.
 */
int cloud_service_location_get(
	const struct cloud_service_pos_req *params,
	struct location_data *location,
	enum nrf_cloud_error *cloud_err);

#ifdef __cplusplus
}
#endif

#endif /* CLOUD_SERVICE_H_ */
