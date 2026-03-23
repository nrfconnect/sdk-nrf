/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef VPR_OFFLOADING_TEMP_MONITOR_H__
#define VPR_OFFLOADING_TEMP_MONITOR_H__

#include <stdint.h>

/** @brief IPC endpoint name used by both parties. */
#define TEMP_MONITOR_EP_NAME "temp_monitor"

/**
 * @name Messages for temperature monitor library.
 * @{
 */

/** @brief Start temperature monitoring. */
#define TEMP_MONITOR_MSG_START 0

/** @brief Stop temperature monitoring. */
#define TEMP_MONITOR_MSG_STOP 1

/** @brief Average temperature report. */
#define TEMP_MONITOR_MSG_AVG 2

/** @brief Temperature above the upper limit. */
#define TEMP_MONITOR_MSG_ABOVE 3

/** @brief Temperature below the lower limit. */
#define TEMP_MONITOR_MSG_BELOW 4

/** @brief Error report. */
#define TEMP_MONITOR_MSG_STATUS 5

#define PACKED
/**@} */

/** @brief Structure with temperature monitor parameters. */
struct temp_monitor_start {
	/* Measurement frequency. */
	uint16_t odr;
	/* Number of measurements after report about average temperature is sent. */
	uint16_t report_avg_period;

	/* Upper limit. */
	int16_t limitl;

	/* Lower limit. */
	int16_t limith;
} PACKED;

/** @brief Structure used for reporting messages (average, limit and status). */
struct temp_monitor_report {
	int16_t value;
} PACKED;

/** @brief Temperature monitor message structure. */
struct temp_monitor_msg {
	/** Message type. */
	uint8_t type;

	union {
		/** Structure with start message parameters. */
		struct temp_monitor_start start;
		/** Structure used for reporting messages. */
		struct temp_monitor_report report;
	};
} PACKED;

/** @brief Event handler.
 *
 * @brief type Message type.
 * @brief value Value associated with the report.
 */
typedef void (*temp_monitor_event_handler_t)(uint8_t type, int16_t value);

/** @brief Set user event handler.
 *
 * @param handler User handler.
 */
void temp_monitor_set_handler(temp_monitor_event_handler_t handler);

/** @brief Function for starting the temperature monitor.
 *
 * @param params Monitoring parameters.
 *
 * @retval 0 if message is sent successfully.
 * @retval negative if sending failed.
 */
int temp_monitor_start(const struct temp_monitor_start *params);

/** @brief Function for stopping the temperature monitor.
 *
 * @param params Monitoring parameters.
 *
 * @retval 0 if message is sent successfully.
 * @retval negative if sending failed.
 */
int temp_monitor_stop(void);

#endif /* VPR_OFFLOADING_TEMP_MONITOR_H__ */
