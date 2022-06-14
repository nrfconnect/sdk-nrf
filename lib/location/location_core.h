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
};

int location_core_init(void);
int location_core_event_handler_set(location_event_handler_t handler);
int location_core_validate_params(const struct location_config *config);
int location_core_location_get(const struct location_config *config);
int location_core_cancel(void);

void location_core_event_cb(const struct location_data *location);
void location_core_event_cb_error(void);
void location_core_event_cb_timeout(void);
#if defined(CONFIG_LOCATION_METHOD_GNSS_AGPS_EXTERNAL)
void location_core_event_cb_agps_request(const struct nrf_modem_gnss_agps_data_frame *request);
#endif
#if defined(CONFIG_LOCATION_METHOD_GNSS_PGPS_EXTERNAL)
void location_core_event_cb_pgps_request(const struct gps_pgps_request *request);
#endif

void location_core_config_log(const struct location_config *config);
void location_core_timer_start(int32_t timeout);
void location_core_timer_stop(void);
struct k_work_q *location_core_work_queue_get(void);

#endif /* LOCATION_CORE_H */
