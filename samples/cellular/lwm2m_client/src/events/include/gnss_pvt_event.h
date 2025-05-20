/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef GNSS_PVT_EVENT_H__
#define GNSS_PVT_EVENT_H__

#include <zephyr/kernel.h>
#include <nrf_modem_gnss.h>
#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gnss_pvt_event {
	struct app_event_header header;

	struct nrf_modem_gnss_pvt_data_frame pvt;
};

APP_EVENT_TYPE_DECLARE(gnss_pvt_event);

#ifdef __cplusplus
}
#endif

#endif /* GNSS_PVT_EVENT_H__ */
