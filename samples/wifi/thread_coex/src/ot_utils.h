/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef OT_UTILS_H_
#define OT_UTILS_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#include "zephyr/net/openthread.h"

#include <openthread/instance.h>
#include <version.h>
#include <openthread/config.h>
#include <openthread/cli.h>
#include <openthread/diag.h>
#include <openthread/error.h>
#include <openthread/joiner.h>
#include <openthread/link.h>
#include <openthread/tasklet.h>
#include <openthread/platform/logging.h>
#include <openthread/dataset_ftd.h>
#include <openthread/thread.h>

#include <openthread/platform/radio.h>

/**
 * Initialize Thread throughput test
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int ot_throughput_test_init(bool is_ot_client, bool is_ot_zperf_udp);

/**
 * @brief Run Thread throughput test
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int ot_throughput_test_run(bool is_ot_zperf_udp);

/**
 * @brief Exit Thread throughput test
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int ot_tput_test_exit(void);

/**
 * Initialize Thread device
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int ot_initialization(void);

/**
 * @brief Start thread joiner
 *
 * @return None.
 */
void ot_start_joiner(const char *pskd);

/**
 * @brief Set thread network key to null
 *
 * @return None.
 */
void ot_setNullNetworkKey(otInstance *aInstance);

/**
 * @brief Get peer address
 *
 * @return None.
 */
void ot_get_peer_address(uint64_t timeout_ms);

/**
 * @brief Thread throughput upload
 *
 * @return None.
 */
void ot_start_zperf_test_send(const char *peer_addr, uint32_t duration_sec,
		uint32_t packet_size_bytes, uint32_t rate_bps, bool is_ot_zperf_udp);

/**
 * @brief Thread throughput download
 *
 * @return None.
 */
void ot_start_zperf_test_recv(bool is_ot_zperf_udp);

/**
 * @brief Thread throughput test
 *
 * @return None.
 */
void ot_zperf_test(bool is_ot_zperf_udp);

/**
 * @brief Configure Thread network
 *
 * @return None.
 */
void ot_setNetworkConfiguration(otInstance *aInstance);

#endif /* OT_UTILS_H_ */
