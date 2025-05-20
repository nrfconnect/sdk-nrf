/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/zperf.h>

/**
 * @brief thread zperf upload
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int zperf_upload(const char *peer_addr, uint32_t duration_sec, uint32_t packet_size_bytes,
	uint32_t rate_bps, bool is_ot_zperf_udp);

/**
 * @brief thread zperf upload statistics
 *
 * @return None
 */
void zperf_log_upload_stats(struct zperf_results *results, bool is_udp);

/**
 * @brief thread zperf download
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int zperf_download(bool is_udp);

/**
 * @brief thread zperf download callback
 *
 * @return None
 */
void zperf_download_cb(enum zperf_status status,
	struct zperf_results *result,
	void *user_data);
