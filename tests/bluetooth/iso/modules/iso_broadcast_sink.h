/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _ISO_BROADCAST_SINK_H_
#define _ISO_BROADCAST_SINK_H_

#include <stdint.h>

typedef void (*sim_iso_recv_cb_t)(uint32_t last_count, uint32_t counts_fail,
				  uint32_t counts_success);

/** @brief Initialize the ISO broadcast sink.
 *
 * @note This code is intended for CI testing and is based on a Zephyr sample.
 * Please see Zephyr ISO samples for a more implementation friendly starting point.
 *
 * @retval 0 The initialization was successful, error otherwise.
 */
int iso_broadcast_sink_init(sim_iso_recv_cb_t sim_iso_recv_cb);

#endif
