/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_INTEGRATION_H_
#define NRF_CLOUD_INTEGRATION_H_

#include <zephyr/kernel.h>
#include <modem/lte_lc.h>
#include <net/multicell_location.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *location_service_get_certificate_nrf_cloud(void);

int location_service_get_cell_location_nrf_cloud(
	const struct multicell_location_params *params,
	char *const rcv_buf,
	const size_t rcv_buf_len,
	struct multicell_location *const location);

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_INTEGRATION_H_ */
