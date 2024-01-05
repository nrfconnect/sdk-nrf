/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LOCATION_CORE_H
#define LOCATION_CORE_H

#define LOCATION_METHOD_INTERNAL_WIFI_CELLULAR 100

/** Information required to carry out a location request. */
struct location_request_info {
	const struct location_wifi_config *wifi;
	const struct location_cellular_config *cellular;
	const struct location_gnss_config *gnss;

	/** Configuration given for currently ongoing location request. */
	struct location_config config;

	uint8_t methods_count;
	/**
	 * This list will store modified list of used methods, including internal methods,
	 * taking into account combining of Wi-Fi and cellular methods.
	 */
	enum location_method methods[CONFIG_LOCATION_METHODS_LIST_SIZE];

	/** Event data for currently ongoing location request. */
	struct location_event_data current_event_data;

	/** Location method of the currently used method. */
	int current_method;

	/** Index to the methods for the currently used method. */
	int current_method_index;

	/** Whether to perform fallback for current location request processing. */
	bool execute_fallback;

	/**
	 * Device uptime when location request timer expires.
	 * This is used in cloud location method to calculate timeout for the cloud operation.
	 */
	int64_t timeout_uptime;
};

struct location_method_api {
	enum location_method method;
	char method_string[28];
	int  (*init)(void);
	int  (*validate_params)(const struct location_method_config *config);
	int  (*location_get)(const struct location_request_info *request);
	int  (*cancel)();
	int  (*timeout)();
#if defined(CONFIG_LOCATION_DATA_DETAILS)
	void  (*details_get)(struct location_data_details *details);
#endif
};

int location_core_init(void);
int location_core_event_handler_set(location_event_handler_t handler);
int location_core_validate_params(const struct location_config *config);
int location_core_location_get(const struct location_config *config);
int location_core_cancel(void);

void location_core_event_cb(const struct location_data *location);
void location_core_event_cb_error(void);
void location_core_event_cb_timeout(void);
#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL) && defined(CONFIG_NRF_CLOUD_AGNSS)
void location_core_event_cb_agnss_request(const struct nrf_modem_gnss_agnss_data_frame *request);
#endif
#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL) && defined(CONFIG_NRF_CLOUD_PGPS)
void location_core_event_cb_pgps_request(const struct gps_pgps_request *request);
#endif

#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
void location_core_event_cb_cloud_location_request(struct location_data_cloud *request);
void location_core_cloud_location_ext_result_set(
	enum location_ext_result result,
	struct location_data *location);
#endif

void location_core_config_log(const struct location_config *config);
void location_core_timer_start(int32_t timeout);
struct k_work_q *location_core_work_queue_get(void);

#endif /* LOCATION_CORE_H */
