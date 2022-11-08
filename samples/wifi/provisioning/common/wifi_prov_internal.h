/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef WIFI_PROV_INTERNAL_H
#define WIFI_PROV_INTERNAL_H

#include <zephyr/types.h>
#include <zephyr/net/buf.h>
#include "common/proto/request.pb.h"
#include "common/proto/response.pb.h"
#include "common/proto/result.pb.h"
#include "common/proto/version.pb.h"

#include <net/wifi_credentials.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INFO_MSG_MAX_LENGTH        Info_size
#define REQUEST_MSG_MAX_LENGTH     Request_size
#define RESPONSE_MSG_MAX_LENGTH    Response_size
#define RESULT_MSG_MAX_LENGTH      Result_size

/**
 * @brief Get encoded Info message.
 *
 * @return 0 if Info retrieved successfully, negative error code otherwise.
 */
int wifi_prov_get_info(struct net_buf_simple *info);

/**
 * @brief Handle received Request message.
 *
 * The Request is message is encoded into a byte stream, and needs to be decoded
 * before accessing its field.
 *
 * @return 0 if message handled successfully, negative error code otherwise.
 */
int wifi_prov_recv_req(struct net_buf_simple *req);

/**
 * @brief Send Response message.
 *
 * The Response message is encoded into a byte stream.
 *
 * @return 0 if message sent successfully, negative error code otherwise.
 */
int wifi_prov_send_rsp(struct net_buf_simple *rsp);

/**
 * @brief Send Result message.
 *
 * The Result message is encoded into a byte stream.
 *
 * @return 0 if message sent successfully, negative error code otherwise.
 */
int wifi_prov_send_result(struct net_buf_simple *result);

/**
 * @brief Initialize Bluetooth LE transport layer.
 *
 * This will enable the BT stack and start advertisement.
 *
 * @return 0 if transport layer initialized successfully, negative error code otherwise.
 */
int wifi_prov_transport_layer_init(void);

/**
 * @brief Fetch the first stored settings entry.
 *
 * @param creds pointer to credentials struct
 */
void wifi_credentials_get_first(struct wifi_credentials_personal *creds);

/**
 * @brief Check if there exists valid configuration (i.e., the device is provisioned).
 *
 * @return true if valid configuration found, false otherwise.
 */
bool wifi_has_config(void);

/**
 * @brief Remove the configuration.
 *
 * @return 0 if configuration removed successfully, negative error code otherwise.
 */
int wifi_remove_config(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_PROV_INTERNAL_H */
