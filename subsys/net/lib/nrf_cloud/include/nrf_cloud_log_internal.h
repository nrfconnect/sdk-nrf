/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#ifndef NRF_CLOUD_LOG_INTERNAL_H_
#define NRF_CLOUD_LOG_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Data associated with each log entry */
struct nrf_cloud_log_context {
	/** In a multi-core system, the source of the log message */
	int dom_id;
	/** Fixed or dynamic source information */
	uint32_t src_id;
	/** The name of the source */
	const char *src_name;
	/** The log level: LOG_LEVEL_NONE, LOG_LEVEL_ERR, etc. */
	int level;
	/** The real clock time at which the log entry was generated. It must
	 *  be Unix epoch time in ms, or else nRF Cloud cannot properly display
	 *  it or search for it.
	 */
	int64_t ts;
	/** Monotonically increasing sequence number */
	unsigned int sequence;
	/** When using REST, this points to the context structure */
	void *rest_ctx;
	/** When using REST, this is the device_id making the REST connection */
	char device_id[NRF_CLOUD_CLIENT_ID_MAX_LEN + 1];
};

void logs_backend_enable(bool enable);

void logs_init_context(void *rest_ctx, const char *dev_id, int log_level,
		       uint32_t src_id, const char *src_name,
		       uint8_t dom_id, int64_t ts,
		       struct nrf_cloud_log_context *context);

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_LOG_INTERNAL_H_ */
