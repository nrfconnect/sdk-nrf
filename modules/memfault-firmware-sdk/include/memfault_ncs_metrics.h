/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MEMFAULT_NCS_METRICS_H_
#define MEMFAULT_NCS_METRICS_H_

#include <zephyr/sys/slist.h>

#include <memfault/metrics/metrics.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Structure contains thread data for stack unused space measurement. */
struct memfault_ncs_metrics_thread {
	/** Thread name. */
	const char *thread_name;

	/** Metric key defined for thread stack unused space measurement. */
	MemfaultMetricId key;

	/** List node for internal use. */
	sys_snode_t node;
};

/** @brief Add thread for an unused stack place measurement on every heartbeat event.
 *
 * @param[in] thread The thread data to add for a measurement.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int memfault_ncs_metrics_thread_add(struct memfault_ncs_metrics_thread *thread);

#ifdef __cplusplus
}
#endif

#endif /* MEMFAULT_NCS_METRICS_H_ */
