/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_SUIT_H__
#define MOCK_SUIT_H__

#include <zephyr/fff.h>
#include <mock_globals.h>

#include <suit.h>

FAKE_VALUE_FUNC(int, suit_processor_init);
FAKE_VALUE_FUNC(int, suit_process_sequence, const uint8_t *, size_t, enum suit_command_sequence);
FAKE_VALUE_FUNC(int, suit_processor_get_manifest_metadata, const uint8_t *, size_t, bool,
		struct zcbor_string *, struct zcbor_string *, enum suit_cose_alg *, unsigned int *);

static inline void mock_suit_processor_reset(void)
{
	RESET_FAKE(suit_processor_init);
	RESET_FAKE(suit_process_sequence);
	RESET_FAKE(suit_processor_get_manifest_metadata);
}

#endif /* MOCK_SUIT_H__ */
