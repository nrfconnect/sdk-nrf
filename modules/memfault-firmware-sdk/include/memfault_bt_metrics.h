/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MEMFAULT_BT_METRICS_H_
#define MEMFAULT_BT_METRICS_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Initialize default Bluetooth metrics. */
void memfault_bt_metrics_init(void);

/** @brief Updates Bluetooth metrics.
 *
 * It can be used on a heartbeat collection.
 */
void memfault_bt_metrics_update(void);

#ifdef __cplusplus
}
#endif

#endif /* MEMFAULT_BT_METRICS_H_ */
