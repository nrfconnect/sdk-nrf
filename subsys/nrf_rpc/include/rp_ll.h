/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef RP_LL_API_H_
#define RP_LL_API_H_

#include <zephyr.h>
#include <metal/sys.h>
#include <metal/device.h>
#include <metal/alloc.h>
#include <openamp/open_amp.h>

/**
 * @file
 * @brief Experimental layer to simplify rpmsg communication between cores
 * on nRF5 chips.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Asynchronous event notification type */
enum rp_ll_event_type {
	RP_LL_EVENT_CONNECTED,  /**< @brief Handshake was successful */
	RP_LL_EVENT_ERROR,      /**< @brief Endpoint was not able to connect */
	RP_LL_EVENT_DATA,       /**< @brief New packet arrived */
};

struct rp_ll_endpoint;

/** @brief Callback called from endpoint's rx thread when an asynchronous event
 * occurred.
 *
 * @param endpoint endpoint on which event was generated
 * @param event    type of event
 * @param buf      pointer to data buffer for RP_LL_EVENT_DATA event
 * @param length   length of @a buf
 */
typedef void (*rp_ll_event_handler)(struct rp_ll_endpoint *endpoint,
	enum rp_ll_event_type event, const uint8_t *buf, size_t length);

/** @brief Contains endpoint information
 *
 * Content is not important for user of the API.
 */
struct rp_ll_endpoint {
	struct rpmsg_endpoint rpmsg_ep;
	rp_ll_event_handler callback;
	uint32_t flags;
};

/** @brief Initializes the communication
 */
int rp_ll_init(void);

/** @brief Uninitializes the communication
 */
void rp_ll_uninit(void);

/** @brief Initializes an endpoint
 *
 * @param endpoint        endpoint to initialize
 * @param endpoint_number identification of the endpoint
 * @param callback        callback called from rx thread to inform about new
 *                        packets or a success or a failure of connection
 *                        process
 * @param user_data       transparent data pointer for @a callback
 */
int rp_ll_endpoint_init(struct rp_ll_endpoint *endpoint,
	int endpoint_number, rp_ll_event_handler callback, void *user_data);

/** @brief Uninitializes endpoint
 *
 * @param endpoint endpoint to uninitialize
 */
void rp_ll_endpoint_uninit(struct rp_ll_endpoint *endpoint);

/** @brief Sends a packet via specified endpoint.
 *
 * @param endpoint endpoint to use
 * @param buf      data buffer to send
 * @param buf_len  size of @a buf
 */
int rp_ll_send(struct rp_ll_endpoint *endpoint, const uint8_t *buf,
	       size_t buf_len);

#ifdef __cplusplus
}
#endif

#endif
