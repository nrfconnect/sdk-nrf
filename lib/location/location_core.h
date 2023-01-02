/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LOCATION_CORE_H
#define LOCATION_CORE_H

struct location_method_api {
	enum location_method method;
	char method_string[10];
	int  (*init)(void);
	int  (*validate_params)(const struct location_method_config *config);
	int  (*location_get)(const struct location_method_config *config);
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
#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL) && defined(CONFIG_NRF_CLOUD_AGPS)
void location_core_event_cb_agps_request(const struct nrf_modem_gnss_agps_data_frame *request);
#endif
#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL) && defined(CONFIG_NRF_CLOUD_PGPS)
void location_core_event_cb_pgps_request(const struct gps_pgps_request *request);
#endif
#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL) && defined(CONFIG_LOCATION_METHOD_CELLULAR)
void location_core_event_cb_cellular_request(struct lte_lc_cells_info *request);
void location_core_cellular_ext_result_set(
	enum location_ext_result result,
	struct location_data *location);
#endif

#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL) && defined(CONFIG_LOCATION_METHOD_WIFI)
void location_core_event_cb_wifi_request(struct wifi_scan_info *request);
void location_core_wifi_ext_result_set(
	enum location_ext_result result,
	struct location_data *location);
#endif

void location_core_config_log(const struct location_config *config);
void location_core_timer_start(int32_t timeout);
void location_core_timer_stop(void);
struct k_work_q *location_core_work_queue_get(void);

#endif /* LOCATION_CORE_H */
