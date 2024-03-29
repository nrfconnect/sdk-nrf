/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_SUIT_PLAT_DECODE_UTIL_H__
#define MOCK_SUIT_PLAT_DECODE_UTIL_H__

#include <zephyr/fff.h>
#include <mock_globals.h>
#include <suit_plat_decode_util.h>

/* suit_plat_decode_util.c */
FAKE_VALUE_FUNC(suit_plat_err_t, suit_plat_decode_component_id, struct zcbor_string *, uint8_t *,
		intptr_t *, size_t *);
FAKE_VALUE_FUNC(suit_plat_err_t, suit_plat_decode_address_size, struct zcbor_string *, intptr_t *,
		size_t *);
FAKE_VALUE_FUNC(suit_plat_err_t, suit_plat_decode_component_number, struct zcbor_string *,
		uint32_t *);
FAKE_VALUE_FUNC(suit_plat_err_t, suit_plat_decode_key_id, struct zcbor_string *, uint32_t *);

#ifdef CONFIG_SUIT_PLATFORM
FAKE_VALUE_FUNC(suit_plat_err_t, suit_plat_decode_component_type, struct zcbor_string *,
		suit_component_type_t *);
#endif /* CONFIG_SUIT_PLATFORM */

#ifdef CONFIG_SUIT_MCI
FAKE_VALUE_FUNC(suit_plat_err_t, suit_plat_decode_manifest_class_id, struct zcbor_string *,
		suit_manifest_class_id_t **);
#endif /* CONFIG_SUIT_MCI */

static inline void mock_suit_plat_decode_util_reset(void)
{
	RESET_FAKE(suit_plat_decode_component_id);
#ifdef CONFIG_SUIT_PLATFORM
	RESET_FAKE(suit_plat_decode_component_type);
#endif /* CONFIG_SUIT_PLATFORM */
	RESET_FAKE(suit_plat_decode_address_size);
	RESET_FAKE(suit_plat_decode_component_number);
	RESET_FAKE(suit_plat_decode_key_id);

#ifdef CONFIG_SUIT_MCI
	RESET_FAKE(suit_plat_decode_manifest_class_id);
#endif /* CONFIG_SUIT_MCI */
}

#endif /* MOCK_SUIT_PLAT_DECODE_UTIL_H__ */
