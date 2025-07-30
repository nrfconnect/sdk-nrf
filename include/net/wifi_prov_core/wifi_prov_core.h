/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZEPHYR_INCLUDE_NET_WIFI_PROV_H_
#define ZEPHYR_INCLUDE_NET_WIFI_PROV_H_

#include <zephyr/types.h>
#include <zephyr/net_buf.h>

#include "request.pb.h"
#include "response.pb.h"
#include "result.pb.h"
#include "version.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WiFi Provisioning API
 * @defgroup wifi_prov_core WiFi Provisioning
 * @{
 */

/* Message size constants */
#define INFO_MSG_MAX_LENGTH        Info_size
#define REQUEST_MSG_MAX_LENGTH     Request_size
#define RESPONSE_MSG_MAX_LENGTH    Response_size
#define RESULT_MSG_MAX_LENGTH      Result_size

/* WiFi Provisioning Service Version */
#define PROV_SVC_VER              0x01

/**
 * @brief Handle received WiFi provisioning request
 *
 * @param req_stream Request buffer containing encoded protobuf message
 * @return 0 on success, negative error code on failure
 */
int wifi_prov_recv_req(struct net_buf_simple *req_stream);

/**
 * @brief Get WiFi provisioning service information
 *
 * @param info Buffer to store encoded info message
 * @return 0 on success, negative error code on failure
 */
int wifi_prov_get_info(struct net_buf_simple *info);

/**
 * @brief Initialize WiFi provisioning service
 *
 * @return 0 on success, negative error code on failure
 */
int wifi_prov_init(void);

/**
 * @brief Get WiFi provisioning state
 *
 * @return true if WiFi provisioning is active, false otherwise
 */
bool wifi_prov_state_get(void);

/**
 * @brief Send response to the transport layer
 *
 * @param rsp Response buffer to send
 * @return 0 on success, negative error code on failure
 */
int wifi_prov_send_rsp(struct net_buf_simple *rsp);

/**
 * @brief Send result to the transport layer
 *
 * @param result Result buffer to send
 * @return 0 on success, negative error code on failure
 */
int wifi_prov_send_result(struct net_buf_simple *result);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_WIFI_PROV_H_ */
