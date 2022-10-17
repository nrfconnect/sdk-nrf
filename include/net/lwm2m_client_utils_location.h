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
/**
 * @brief Set the A-GPS request mask
 *
 * @param agps_req The A-GPS request coming from the GNSS module.
 */
void location_assistance_agps_set_mask(const struct nrf_modem_gnss_agps_data_frame *agps_req);

/**
 * @brief Send the A-GPS assistance request to LwM2M server
 *
 * @param ctx LwM2M client context for sending the data.
 * @param confirmable Boolean value to indicate should the server return confirmation.
 * @return Returns a negative error code (errno.h) indicating
 *         reason of failure or 0 for success.
 */
int location_assistance_agps_request_send(struct lwm2m_ctx *ctx, bool confirmable);
#endif

/**
 * @brief Send the Ground Fix request to LwM2M server
 *
 * @param ctx LwM2M client context for sending the data.
 * @param confirmable Boolean value to indicate should the server return confirmation.
 * @return Returns a negative error code (errno.h) indicating
 *         reason of failure or 0 for success.
 */
int location_assistance_ground_fix_request_send(struct lwm2m_ctx *ctx, bool confirmable);

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS)
/**
 * @brief Send the P-GPS assistance request to LwM2M server
 *
 * @param ctx LwM2M client context for sending the data.
 * @param confirmable Boolean value to indicate should the server return confirmation.
 * @return Returns a negative error code (errno.h) indicating
 *         reason of failure or 0 for success.
 */
int location_assistance_pgps_request_send(struct lwm2m_ctx *ctx, bool confirmable);
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_EVENTS)
int location_event_handler_init(struct lwm2m_ctx *ctx);
#endif /* CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_EVENTS */

#endif /* LWM2M_CLIENT_UTILS_LOCATION_H__ */

/**@} */
