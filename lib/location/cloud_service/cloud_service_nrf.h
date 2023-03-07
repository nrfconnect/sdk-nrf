/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CLOUD_SERVICE_NRF_H_
#define CLOUD_SERVICE_NRF_H_

#include "cloud_service.h"

#ifdef __cplusplus
extern "C" {
#endif

int cloud_service_nrf_pos_get(
	const struct cloud_service_pos_req *params,
	char *const rcv_buf,
	const size_t rcv_buf_len,
	struct location_data *const location);

#ifdef __cplusplus
}
#endif

#endif /* CLOUD_SERVICE_NRF_H_ */
