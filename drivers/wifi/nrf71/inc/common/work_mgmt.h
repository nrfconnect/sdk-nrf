/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing work management function declarations for the nRF71 driver.
 */

#ifndef __WORK_MGMT_H__
#define __WORK_MGMT_H__

#include <stdbool.h>

#include <zephyr/kernel.h>

/** Bottom-half work queue used by the driver. */
extern struct k_work_q zep_wifi_bh_q;

/** Work item queue type. */
enum zep_work_type {
	/** Bottom-half work queue. */
	ZEP_WORK_TYPE_BH,
	/** TX-done work queue. */
	ZEP_WORK_TYPE_TX_DONE,
	/** RX work queue. */
	ZEP_WORK_TYPE_RX,
};

/** Driver work item backed by a Zephyr @c k_work object. */
struct zep_work_item {
	/** Whether the work item slot is in use. */
	bool in_use;
	/** Underlying Zephyr work object. */
	struct k_work work;
	/** Opaque data passed to the callback. */
	unsigned long data;
	/** Callback invoked when the work item runs. */
	void (*callback)(unsigned long data);
	/** Work queue to submit the item to. */
	enum zep_work_type type;
};

/**
 * @brief Allocate a work item.
 *
 * @param type Work queue type that determines where the item is scheduled.
 *
 * @return Pointer to a work item on success, NULL if no slots are available.
 */
struct zep_work_item *nrf_wifi_work_alloc(enum zep_work_type type);

/**
 * @brief Initialize a work item.
 *
 * @param work Pointer to the work item to initialize.
 * @param callback Function invoked when the work item runs.
 * @param data Opaque data passed to @p callback.
 */
void nrf_wifi_work_init(struct zep_work_item *work, void (*callback)(unsigned long callbk_data),
			unsigned long data);

/**
 * @brief Submit a work item to its configured work queue.
 *
 * @param work Pointer to an initialized work item.
 */
void nrf_wifi_work_schedule(struct zep_work_item *work);

/**
 * @brief Cancel a scheduled work item.
 *
 * @param work Pointer to the work item to cancel.
 */
void nrf_wifi_work_kill(struct zep_work_item *work);

/**
 * @brief Release a work item slot for reuse.
 *
 * @param work Pointer to the work item to release.
 */
void nrf_wifi_work_free(struct zep_work_item *work);

#endif /* __WORK_MGMT_H__ */
