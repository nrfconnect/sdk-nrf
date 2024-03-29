/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_SUIT_STORAGE_H__
#define MOCK_SUIT_STORAGE_H__

#include <zephyr/fff.h>
#include <mock_globals.h>

#include <suit_storage.h>

/* suit_storage.c */
FAKE_VALUE_FUNC(int, suit_storage_init);
FAKE_VALUE_FUNC(int, suit_storage_installed_envelope_get, const suit_manifest_class_id_t *,
		const uint8_t **, size_t *);
FAKE_VALUE_FUNC(int, suit_storage_install_envelope, const suit_manifest_class_id_t *, uint8_t *,
		size_t);

/* suit_storage_update.c */
FAKE_VALUE_FUNC(int, suit_storage_update_cand_get, const suit_plat_mreg_t **, size_t *);
FAKE_VALUE_FUNC(int, suit_storage_update_cand_set, suit_plat_mreg_t *, size_t);

static inline void mock_suit_storage_reset(void)
{
	RESET_FAKE(suit_storage_init);
	RESET_FAKE(suit_storage_installed_envelope_get);
	RESET_FAKE(suit_storage_install_envelope);
	RESET_FAKE(suit_storage_update_cand_get);
	RESET_FAKE(suit_storage_update_cand_set);
}

#endif /* MOCK_SUIT_STORAGE_H__ */
