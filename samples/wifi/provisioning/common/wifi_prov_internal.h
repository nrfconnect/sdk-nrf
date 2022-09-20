/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef WIFI_PROV_INTERNAL_H
#define WIFI_PROV_INTERNAL_H

#include <zephyr/types.h>
#include <zephyr/net/buf.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Following functions are used when the handler and transport layer interact.
 * Note that it is data structure independent.
 */
int wifi_prov_get_info(struct net_buf_simple *info);

int wifi_prov_recv_req(struct net_buf_simple *req);

int wifi_prov_send_rsp(struct net_buf_simple *rsp);

int wifi_prov_send_result(struct net_buf_simple *result);

int wifi_prov_transport_layer_init(void);



#ifdef __cplusplus
}
#endif

#endif /* WIFI_PROV_INTERNAL_H */
