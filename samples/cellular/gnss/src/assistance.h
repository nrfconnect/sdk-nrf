/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ASSISTANCE_H_
#define ASSISTANCE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the assistance module.
 *
 * @param[in] assistance_work_q Work queue that can be used by the assistance module.
 *
 * @retval 0 on success.
 * @retval -1 in case of an error.
 */
int assistance_init(struct k_work_q *assistance_work_q);

/**
 * @brief Handles an assistance data request.
 *
 * @details Fetches and injects assistance data to the GNSS.
 *
 * @param[in] agnss_request A-GNSS data requested by GNSS.
 *
 * @retval 0 on success.
 * @retval <0 in case of an error.
 */
int assistance_request(struct nrf_modem_gnss_agnss_data_frame *agnss_request);

/**
 * @brief Returns assistance module state.
 *
 * @retval true if assistance module is downloading data.
 * @retval false if assistance module is idle.
 */
bool assistance_is_active(void);

#ifdef __cplusplus
}
#endif

#endif /* ASSISTANCE_H_ */
