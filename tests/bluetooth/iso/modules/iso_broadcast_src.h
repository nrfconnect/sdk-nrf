/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ISO_BROADCAST_SRC_H_
#define _ISO_BROADCAST_SRC_H_

#include <stdint.h>

struct broadcaster_params {
	uint16_t sdu_size;
	uint8_t phy;
	uint8_t rtn;
	uint8_t num_bis;
	uint32_t sdu_interval_us;
	uint16_t latency_ms;
	uint8_t packing;
	uint8_t framing;
};

int iso_broadcaster_init(void);

#endif
