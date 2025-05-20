/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_DIGEST_SINK_H__
#define MOCK_DIGEST_SINK_H__

#include <zephyr/fff.h>
#include <mock_globals.h>

#include <suit_sink.h>
#include <suit_digest_sink.h>

FAKE_VALUE_FUNC(suit_plat_err_t, suit_digest_sink_get, struct stream_sink *, psa_algorithm_t,
		const uint8_t *);
FAKE_VALUE_FUNC(digest_sink_err_t, suit_digest_sink_digest_match, void *);

static inline void mock_digest_sink_reset(void)
{
	RESET_FAKE(suit_digest_sink_get);
	RESET_FAKE(suit_digest_sink_digest_match);
}
#endif /* MOCK_DIGEST_SINK_H__ */
