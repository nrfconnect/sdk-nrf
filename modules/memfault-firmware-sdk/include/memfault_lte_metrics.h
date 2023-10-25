/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MEMFAULT_LTE_METRICS_H_
#define MEMFAULT_LTE_METRICS_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Initialize default LTE metrics. */
void memfault_lte_metrics_init(void);

/** @brief Update LTE metrics.
 *
 *  You can use the metrics on a heartbeat collection.
 */
void memfault_lte_metrics_update(void);

#ifdef __cplusplus
}
#endif

#endif /* MEMFAULT_LTE_METRICS_H_ */
