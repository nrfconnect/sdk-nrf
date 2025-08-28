/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef __ESB_SNIFFER_H
#define __ESB_SNIFFER_H

#include <esb.h>

#ifdef __cplusplus
extern "C" {
#endif

struct esb_sniffer_cfg {
	uint8_t base_addr0[4];
	uint8_t base_addr1[4];
	uint8_t pipe_prefix[8];
	uint8_t enabled_pipes;
	enum esb_bitrate bitrate;
	bool is_running;
};

struct rtt_frame {
	uint32_t ms;
	uint32_t us;
	struct esb_payload payload;
} __packed;

int sniffer_init(void);
void sniffer_shell_init(struct esb_sniffer_cfg *sniffer_cfg);

#ifdef __cplusplus
}
#endif

#endif /* __ESB_SNIFFER_H  */
