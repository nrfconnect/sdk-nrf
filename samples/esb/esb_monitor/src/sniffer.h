/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef __ESB_SNIFFER_H
#define __ESB_SNIFFER_H

#include <esb.h>

struct esb_sniffer_cfg {
	uint8_t base_addr0[4];
	uint8_t base_addr1[4];
	uint8_t pipe_prefix[8];
	uint8_t enabled_pipes;
	enum esb_bitrate bitrate;
	bool is_running;
};

int sniffer_init(void);

#endif
