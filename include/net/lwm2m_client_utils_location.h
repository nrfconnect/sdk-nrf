/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file lwm2m_client_utils_location.h
 * @defgroup lwm2m_client_utils_location LwM2M LOCATION
 * @ingroup lwm2m_client_utils
 * @{
 * @brief API for the LwM2M based LOCATION
 */

#ifndef LWM2M_CLIENT_UTILS_LOCATION_H__
#define LWM2M_CLIENT_UTILS_LOCATION_H__

#include <zephyr/kernel.h>
#include <nrf_modem_gnss.h>
#include <zephyr/net/lwm2m.h>
#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#define LOCATION_ASSIST_NEEDS_UTC			BIT(0)
#define LOCATION_ASSIST_NEEDS_EPHEMERIES		BIT(1)
#define LOCATION_ASSIST_NEEDS_ALMANAC			BIT(2)
#define LOCATION_ASSIST_NEEDS_KLOBUCHAR			BIT(3)
#define LOCATION_ASSIST_NEEDS_NEQUICK			BIT(4)
#define LOCATION_ASSIST_NEEDS_TOW			BIT(5)
#define LOCATION_ASSIST_NEEDS_CLOCK			BIT(6)
#define LOCATION_ASSIST_NEEDS_LOCATION			BIT(7)
#define LOCATION_ASSIST_NEEDS_INTEGRITY			BIT(8)

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGPS)
struct gnss_agps_request_event {
	struct app_event_header header;

	struct nrf_modem_gnss_agps_data_frame agps_req;
};

APP_EVENT_TYPE_DECLARE(gnss_agps_request_event);

#endif
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_CELL)
struct cell_location_request_event {
	struct app_event_header header;
};

APP_EVENT_TYPE_DECLARE(cell_location_request_event);

struct cell_location_inform_event {
	struct app_event_header header;
};

APP_EVENT_TYPE_DECLARE(cell_location_inform_event);
#endif

int location_event_handler_init(struct lwm2m_ctx *ctx);

#endif /* LWM2M_CLIENT_UTILS_LOCATION_H__ */

/**@} */
