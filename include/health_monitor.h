/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef HEALTH_MONITOR_H_
#define HEALTH_MONITOR_H_

/** @brief Health Monitor Event type. */
typedef enum health_event_type_t {
	HEALTH_EVENT_INVALID = 0,
	HEALTH_EVENT_OUT_OF_TX_BUFFERS,
	HEALTH_EVENT_OUT_OF_RX_BUFFERS,
	HEALTH_EVENT_DISCONNECTED,
	HEALTH_EVENT_SRP_REG,
	HEALTH_EVENT_NO_PREFIX,
	HEALTH_EVENT_WEAK_SIGNAL,
	HEALTH_EVENT_NO_NEIGBOURS,
	HEALTH_EVENT_STATS,
	HEALTH_EVENT__COUNT

} health_event_type_t;

/** @brief Health Monitor counters structure. */
struct counters_data {
	uint32_t state_changes;
	uint32_t child_added;
	uint32_t child_removed;
	uint32_t partition_id_changes;
	uint32_t key_sequence_changes;
	uint32_t network_data_changes;
	uint32_t active_dataset_changes;
	uint32_t pending_dataset_changes;
};

/** @brief Health Monitor event structure containing event with data. */
struct health_event_data {
	/** Health Monitor event type */
	health_event_type_t event_type;

	/** Health Monitor parameters */
	union {
		/** Health Monitor HEALTH_EVENT_STATS parameters */
		struct counters_data *counters; /* counters passed as pointer to limit the size */
	};
};

/** @brief Pointer to the event handler function.
 *
 * @param event    Pointer to the health event generated.
 * @param context  Context passed to the health_monitor_set_event_callback function.
 */
typedef void (*health_monitor_event_callback)(struct health_event_data *event, void *context);

/**
 * @brief Initializes the Health Monitor with default parameters.
 *
 * Initializes the Health Monitor for using given work_q for processing.
 *
 * @param[in] workqueue     Queue to use as main processing thread.
 *                                         unspecified internal failure
 */
void health_monitor_init(struct k_work_q *workqueue);

/**
 * @brief Set the raport interval
 *
 * Sets how often health raport needs to be generated.
 * This will reset the timer.
 *
 * @param[in] seconds     Number of seconds between raports.
 */
void health_monitor_set_raport_interval(uint32_t seconds);

/**
 * @brief Set the Health Monitor event callback
 *
 * In order to receive the notifications application needs to set the valid callback.
 * If no callback is set, health data is gathered but no notification is send.
 *
 * @param[in] callback  Callback function for processing the events. The callback will be called in
 *                      the work_q thread passed to the init function.
 * @param[in] context   Custom data to pass to the callback.
 */
void health_monitor_set_event_callback(health_monitor_event_callback callback, void *context);

#endif
