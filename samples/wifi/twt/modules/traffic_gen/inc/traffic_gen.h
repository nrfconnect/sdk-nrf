/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __TRAFFIC_GEN_H__
#define __TRAFFIC_GEN_H__

#define TRAFFIC_GEN_TYPE_TCP 1

#define TRAFFIC_GEN_ROLE_CLIENT 1
#define TRAFFIC_GEN_ROLE_SERVER 2

struct traffic_gen_config {
	/* global info */
	bool traffic_gen_report_received;
	int ctrl_sock_fd;
	int data_sock_fd;
	char buffer[CONFIG_TRAFFIC_GEN_PAYLOAD_SIZE];

	/* Kconfig info */
	int role;
	int type;
	int mode;
	int duration;
	int payload_len;
	const unsigned char *server_ip;
	int port;
};

struct server_config {
	int role;
	int type;
	int mode;
	int duration;
	int payload_len;
};

struct traffic_gen_report {
	int bytes_received;
	int packets_received;
	int elapsed_time;
	int throughput;
	int average_jitter;
};

/* Public API */
void traffic_gen_init(struct traffic_gen_config *tg_config);
int traffic_gen_start(struct traffic_gen_config *tg_config);
int traffic_gen_wait_for_report(struct traffic_gen_config *tg_config);
void traffic_gen_get_report(struct traffic_gen_config *tg_config);

#endif /* __TRAFFIC_GEN_H__ */
