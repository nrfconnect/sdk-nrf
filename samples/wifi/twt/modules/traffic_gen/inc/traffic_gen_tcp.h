/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __TRAFFIC_GEN_TCP_H__
#define __TRAFFIC_GEN_TCP_H__

extern struct traffic_gen_report remote_report;

/* tcp client/server function prototypes */
int init_tcp_client(struct traffic_gen_config *tg_config);
int send_tcp_uplink_traffic(struct traffic_gen_config *tg_config);
int init_tcp_server(struct traffic_gen_config *tg_config);
int recv_tcp_downlink_traffic(struct traffic_gen_config *tg_config);

#endif /* __TRAFFIC_GEN_TCP_H__ */
