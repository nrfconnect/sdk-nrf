/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_SUIT_METADATA_H__
#define MOCK_SUIT_METADATA_H__

#include <zephyr/fff.h>
#include <mock_globals.h>

#include <suit_metadata.h>

/* suit_metadata.c */
FAKE_VALUE_FUNC(suit_plat_err_t, suit_metadata_version_from_array, suit_version_t *, int32_t *,
		size_t);

static inline void mock_suit_metadata_reset(void)
{
	RESET_FAKE(suit_metadata_version_from_array);
}

#endif /* MOCK_SUIT_MCI_H__ */
