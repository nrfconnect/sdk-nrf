/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>

#include <nrf71_icr.h>
#include <vtf_monitoring/vtf_monitoring.h>

LOG_MODULE_REGISTER(xo_offset_monitor, CONFIG_VTF_LOG_LEVEL);

static int xo_offset_init(void)
{
	uint32_t xo_offset = (uint32_t)CONFIG_VTF_FREQ_OFFSET_DEFAULT_VALUE;
	int source;

	source = nrf71_icr_read(XTALCOMP_OFFSET, &xo_offset);
	if (source < 0) {
		LOG_ERR("Failed to read XO offset: %d", source);
		return source;
	}

	vtf_snapshots[VTF_CH_FREQ_OFFSET].i32 = (int32_t)xo_offset;
	LOG_DBG("XO offset is %d ppm (source %d)", (int32_t)xo_offset, source);

	return 0;
}

VTF_CHANNEL_DEFINE(vtf_channel_xo_offset, VTF_CH_FREQ_OFFSET, NULL, xo_offset_init,
		   VTF_SAMPLE_TYPE_INT, i32, CONFIG_VTF_FREQ_OFFSET_DEFAULT_VALUE);
