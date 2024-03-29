/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_FETCH_SOURCE_STREAMER_H__
#define MOCK_FETCH_SOURCE_STREAMER_H__

#include <zephyr/fff.h>
#include <mock_globals.h>

#include <suit_sink.h>

FAKE_VALUE_FUNC(suit_plat_err_t, suit_fetch_source_stream, const uint8_t *, size_t,
		struct stream_sink *);

static inline void mock_fetch_source_streamer_reset(void)
{
	RESET_FAKE(suit_fetch_source_stream);
}

#endif /* MOCK_FETCH_SOURCE_STREAMER_H__ */
