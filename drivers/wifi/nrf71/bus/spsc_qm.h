/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief SPSC queue management for nRF71 Wi-Fi IPC (32-bit values).
 */

#ifndef SPSC_QM_H
#define SPSC_QM_H

#include <zephyr/sys/spsc_pbuf.h>

#include <stdint.h>
#include <stdbool.h>

typedef struct spsc_pbuf spsc_queue_t;

spsc_queue_t *spsc32_init(uint32_t address, size_t size);

bool spsc32_push(spsc_queue_t *pb, uint32_t value);

bool spsc32_pop(spsc_queue_t *pb, uint32_t *out_value);

bool spsc32_read_head(spsc_queue_t *pb, uint32_t *out_value);

#endif /* SPSC_QM_H */
