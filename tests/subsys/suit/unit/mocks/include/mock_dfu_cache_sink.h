/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_DFU_CACHE_SINK_H__
#define MOCK_DFU_CACHE_SINK_H__


#include <zephyr/fff.h>
#include <mock_globals.h>

#include <suit_sink.h>

FAKE_VALUE_FUNC(suit_plat_err_t, suit_dfu_cache_sink_get, struct stream_sink*, uint8_t,
		const uint8_t*, size_t, bool);
FAKE_VALUE_FUNC(suit_plat_err_t, suit_dfu_cache_sink_commit, void*);

static inline void mock_dfu_cache_sink_reset(void)
{
	RESET_FAKE(suit_dfu_cache_sink_get);
	RESET_FAKE(suit_dfu_cache_sink_commit);
}

#endif /* MOCK_DFU_CACHE_SINK_H__ */
