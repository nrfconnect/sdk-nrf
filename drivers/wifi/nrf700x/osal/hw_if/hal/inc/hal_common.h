/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing common declarations for the
 * HAL Layer of the Wi-Fi driver.
 */

#ifndef __HAL_COMMON_H__
#define __HAL_COMMON_H__

enum wifi_nrf_status hal_rpu_hpq_enqueue(struct wifi_nrf_hal_dev_ctx *hal_ctx,
					 struct host_rpu_hpq *hpq,
					 unsigned int val);

enum wifi_nrf_status hal_rpu_hpq_dequeue(struct wifi_nrf_hal_dev_ctx *hal_ctx,
					 struct host_rpu_hpq *hpq,
					 unsigned int *val);
#endif /* __HAL_COMMON_H__ */
