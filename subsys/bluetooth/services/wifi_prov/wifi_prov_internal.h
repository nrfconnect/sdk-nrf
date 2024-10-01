/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef WIFI_PROV_INTERNAL_H
#define WIFI_PROV_INTERNAL_H

#include <zephyr/types.h>
#include <zephyr/net_buf.h>
#include "request.pb.h"
#include "response.pb.h"
#include "result.pb.h"
#include "version.pb.h"

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

#ifdef __cplusplus
}
#endif

#endif /* WIFI_PROV_INTERNAL_H */
