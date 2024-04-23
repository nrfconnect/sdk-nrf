/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef SLM_CMUX_
#define SLM_CMUX_

struct modem_pipe;

void slm_cmux_init(void);

/* CMUX channels that are used by other modules. */
enum cmux_channel {
#if defined(CONFIG_SLM_PPP)
	CMUX_PPP_CHANNEL,
#endif
#if defined(CONFIG_SLM_GNSS_OUTPUT_NMEA_ON_CMUX_CHANNEL)
	CMUX_GNSS_CHANNEL,
#endif
	CMUX_EXT_CHANNEL_COUNT
};
struct modem_pipe *slm_cmux_reserve(enum cmux_channel);
void slm_cmux_release(enum cmux_channel);

#endif
